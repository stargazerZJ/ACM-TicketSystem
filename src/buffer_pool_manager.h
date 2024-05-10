//
// Created by zj on 4/27/2024.
//

#pragma once

#include <lru_k_replacer.h>
#include <numeric>
#include <string>

#include "config.h"
#include "disk_manager.h"
#include "external_memory/external_memory.h"

namespace storage {
/**
 * @brief Currently, no caching is implemented.
 * @tparam PagesPerFrame
 */
template<int PagesPerFrame>
class BufferPoolManager;

template<int PagesPerFrame>
class Frame {
  friend class BufferPoolManager<PagesPerFrame>;

  public:
    static constexpr int kFrameSize = PAGE_SIZE * PagesPerFrame;
    auto GetData() -> char * { return data_; }
    auto GetPageId() const -> page_id_t { return page_id_; }
    auto GetPinCount() const -> int { return pin_count_; }
    auto IsDirty() const -> bool { return is_dirty_; }

  private:
    page_id_t page_id_ = INVALID_PAGE_ID;
    bool is_dirty_ = false;
    int pin_count_ = 0;
    char data_[kFrameSize]{};

    void Reset() {
      page_id_ = INVALID_PAGE_ID;
      is_dirty_ = false;
      pin_count_ = 0;
      memset(data_, 0, sizeof(data_));
    }
};

template<int PagesPerFrame>
class BufferPoolManager {
  public:
    explicit BufferPoolManager(const std::string &file_path,
                               bool reset,
                               size_t pool_size = BUFFER_POOL_SIZE) : disk_(file_path, reset),
                                                                      replacer_(pool_size), pool_size_(pool_size),
                                                                      buffer_(pool_size), free_list_(pool_size) {
      std::iota(free_list_.begin(), free_list_.end(), 0);
    }
    ~BufferPoolManager() { FlushAllFrames(); }

    class BasicFrameGuard {
      friend BufferPoolManager;

      public:
        BasicFrameGuard() = default;
        BasicFrameGuard(const BasicFrameGuard &that) : bpm_(that.bpm_), frame_(that.frame_) {
          if (frame_ != nullptr) {
            ++frame_->pin_count_;
          }
        }
        BasicFrameGuard(BasicFrameGuard &&that) noexcept: bpm_(that.bpm_), frame_(that.frame_) {
          that.frame_ = nullptr;
        }

        BasicFrameGuard &operator=(const BasicFrameGuard &that) {
          if (this == &that || frame_ == that.frame_) {
            return *this;
          }
          Drop();
          bpm_ = that.bpm_;
          frame_ = that.frame_;
          if (frame_ != nullptr) {
            ++frame_->pin_count_;
          }
          return *this;
        }
        BasicFrameGuard &operator=(BasicFrameGuard &&that) noexcept {
          if (this == &that) {
            return *this;
          }
          Drop();
          bpm_ = that.bpm_;
          frame_ = that.frame_;
          that.frame_ = nullptr;
          return *this;
        }

        ~BasicFrameGuard() {
          Drop();
        }

        auto PageId() -> page_id_t { return frame_->GetPageId(); }
        auto GetData() -> char * { return frame_->GetData(); }
        auto GetData() const -> const char * { return frame_->GetData(); }
        auto GetDataMut() -> char * {
          frame_->is_dirty_ = true;
          return frame_->GetData();
        }
        template<class T>
        auto As() const -> const T * {
          return reinterpret_cast<const T *>(GetData());
        }
        template<class T>
        auto AsMut() -> T * {
          return reinterpret_cast<T *>(GetDataMut());
        }
        void Drop() {
          if (frame_ == nullptr) return;
          bpm_->UnpinFrame(PageId());
          frame_ = nullptr;
        }
        void Delete() {
          if (frame_ == nullptr) return;
          bpm_->DeletePage(PageId());
          frame_ = nullptr;
        }

      private:
        BasicFrameGuard(BufferPoolManager *bpm, Frame<PagesPerFrame> *frame) : bpm_(bpm), frame_(frame) {
        }
        BufferPoolManager *bpm_;
        Frame<PagesPerFrame> *frame_;
    };
    auto NewFrameGuarded(page_id_t *page_id = nullptr) -> BasicFrameGuard;
    auto FetchFrameBasic(page_id_t page_id) -> BasicFrameGuard;
    int &GetInfo(int n) { return disk_.GetInfo(n); }

  private:
    using Frame = Frame<PagesPerFrame>;
    using frame_id_t = LRUKReplacer::frame_id_t;
    const size_t pool_size_;
    DiskManager<PagesPerFrame> disk_;
    LRUKReplacer replacer_;
    std::unordered_map<page_id_t, frame_id_t> page_table_;
    std::vector<Frame> buffer_;
    std::vector<frame_id_t> free_list_;

    auto FetchFrame(page_id_t page_id) -> Frame *;
    auto UnpinFrame(page_id_t page_id, bool is_dirty) -> bool;
    auto DeletePage(page_id_t page_id) -> void; // delete regardless of pin count
    void FlushAllFrames();
    auto EnsureFreeList() -> bool;
};
template<int PagesPerFrame>
auto BufferPoolManager<PagesPerFrame>::NewFrameGuarded(page_id_t *page_id) -> BasicFrameGuard {
  if (!EnsureFreeList()) throw std::runtime_error("No free frame");
  page_id_t page_id_ = disk_.AllocateFrame();
  if (page_id) *page_id = page_id_;
  frame_id_t frame_id = free_list_.back();
  free_list_.pop_back();
  auto &frame = buffer_[frame_id];
  frame.page_id_ = page_id_;
  page_table_[page_id_] = frame_id;
  replacer_.RecordAccess(page_id_);
  return {this, &frame};
}
template<int PagesPerFrame>
auto BufferPoolManager<PagesPerFrame>::FetchFrameBasic(page_id_t page_id) -> BasicFrameGuard {
  auto frame = FetchFrame(page_id);
  if (frame == nullptr) throw std::runtime_error("No free frame");
  return {this, frame};
}
template<int PagesPerFrame>
bool BufferPoolManager<PagesPerFrame>::EnsureFreeList() {
  if (free_list_.empty()) {
    frame_id_t frame_id;
    if (!replacer_.Evict(&frame_id)) {
      return false;
    }
    auto &frame = buffer_[frame_id];
    if (frame.IsDirty()) {
      disk_.WriteFrame(frame.GetPageId(), frame.GetData());
    }
    page_table_.erase(frame.GetPageId());
    frame.Reset();
    free_list_.push_back(frame_id);
  }
  return true;
}
template<int PagesPerFrame>
auto BufferPoolManager<PagesPerFrame>::FetchFrame(page_id_t page_id) -> Frame * {
  if (auto it = page_table_.find(page_id); it != page_table_.end()) {
    auto &frame = buffer_[it->second];
    replacer_.RecordAccess(page_id);
    if (frame.GetPinCount() == 0) {
      replacer_.SetNonevictable(it->second);
    }
    ++frame.pin_count_;
    return &frame;
  }
  if (!EnsureFreeList()) return nullptr;
  frame_id_t frame_id = free_list_.back();
  free_list_.pop_back();
  auto &frame = buffer_[frame_id];
  frame.page_id_ = page_id;
  disk_.ReadFrame(page_id, frame.GetData());
  page_table_[page_id] = frame_id;
  replacer_.RecordAccess(page_id);
  ++frame.pin_count_;
  return &frame;
}
template<int PagesPerFrame>
auto BufferPoolManager<PagesPerFrame>::UnpinFrame(page_id_t page_id, bool is_dirty) -> bool {
  if (auto it = page_table_.find(page_id); it != page_table_.end()) {
    auto &frame = buffer_[it->second];
    if (frame.GetPinCount() <= 0) {
      return false;
    }
    if (--frame.pin_count_ == 0) {
      replacer_.SetEvictable(it->second, page_id);
    }
    frame.is_dirty_ |= is_dirty;
    return true;
  }
  return false;
}
template<int PagesPerFrame>
auto BufferPoolManager<PagesPerFrame>::DeletePage(page_id_t page_id) -> void {
  if (auto it = page_table_.find(page_id); it != page_table_.end()) {
    auto &frame = buffer_[it->second];
    frame.Reset();
    page_table_.erase(it);
    replacer_.Remove(it->second, page_id);
    free_list_.push_back(it->second);
  }
  disk_.DeallocateFrame(page_id);
}
template<int PagesPerFrame>
void BufferPoolManager<PagesPerFrame>::FlushAllFrames() {
  for (auto &frame : buffer_) {
    if (frame.IsDirty()) {
      disk_.WriteFrame(frame.GetPageId(), frame.GetData());
    }
  }
}

template<>
class BufferPoolManager<1> {
  public:
    explicit BufferPoolManager(const std::string &file_path, bool reset = false) : disk_(file_path) {
      disk_.initialize(reset);
    }

    class BasicFrameGuard {
      friend BufferPoolManager;

      public:
        BasicFrameGuard() = default;
        BasicFrameGuard(const BasicFrameGuard &) = delete;
        BasicFrameGuard(BasicFrameGuard &&that) noexcept: bpm_(that.bpm_), frame_(that.frame_) {
          that.frame_ = nullptr;
        }

        BasicFrameGuard &operator=(const BasicFrameGuard &) = delete;
        BasicFrameGuard &operator=(BasicFrameGuard &&that) noexcept {
          if (this == &that) {
            return *this;
          }
          Drop();
          bpm_ = that.bpm_;
          frame_ = that.frame_;
          that.frame_ = nullptr;
          return *this;
        }

        ~BasicFrameGuard() {
          Drop();
        }

        auto PageId() -> page_id_t { return frame_->GetPageId(); }
        auto GetData() -> char * { return frame_->GetData(); }
        auto GetData() const -> const char * { return frame_->GetData(); }
        auto GetDataMut() -> char * {
          frame_->is_dirty_ = true;
          return frame_->GetData();
        }
        template<class T>
        auto As() const -> const T * {
          return reinterpret_cast<const T *>(GetData());
        }
        template<class T>
        auto AsMut() -> T * {
          return reinterpret_cast<T *>(GetDataMut());
        }
        void Drop() {
          // as no caching is implemented, this is equivalent to unpin
          if (frame_ == nullptr) return;
          if (frame_->is_dirty_) {
            bpm_->disk_.setPage(frame_->page_id_, reinterpret_cast<const int *>(frame_->GetData()));
          }
          delete frame_;
          frame_ = nullptr;
        }
        void Delete() {
          bpm_->disk_.deletePage(frame_->page_id_);
          delete frame_;
          frame_ = nullptr;
        }

      private:
        BasicFrameGuard(BufferPoolManager *bpm, Frame<1> *frame) : bpm_(bpm), frame_(frame) {
        }
        BufferPoolManager *bpm_;
        Frame<1> *frame_;
    };
    auto NewFrameGuarded(page_id_t *page_id = nullptr) -> BasicFrameGuard;
    auto FetchFrameBasic(page_id_t page_id) -> BasicFrameGuard;

    int &getInfo(unsigned int n) { return disk_.getInfo(n); }

  private:
    external_memory::Pages disk_;
};
} // namespace storage
