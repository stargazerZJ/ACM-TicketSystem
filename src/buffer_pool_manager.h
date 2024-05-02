//
// Created by zj on 4/27/2024.
//

#pragma once

#include "config.h"
#include <string>
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
  auto GetPageId() -> page_id_t { return page_id_; }
  auto GetPinCount() -> int { return pin_count_; }
  auto IsDirty() -> bool { return is_dirty_; }

 private:
  page_id_t page_id_ = INVALID_PAGE_ID;
  bool is_dirty_ = false;
  int pin_count_ = 0;
  char data_[kFrameSize]{};
};

template<>
class BufferPoolManager<1> {
 public:
  explicit BufferPoolManager(const std::string &file_path, bool reset = false) : pages_(file_path) {
    pages_.initialize(reset);
  }

  class BasicFrameGuard { // Currently, one frame can only be pinned once
    friend BufferPoolManager<1>;
   public:
    BasicFrameGuard() = default;
    BasicFrameGuard(const BasicFrameGuard &) = delete; // because only one frame can be pinned currently
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
    void Drop() { // as no caching is implemented, this is equivalent to unpin
      if (frame_ == nullptr) return;
      if (frame_->is_dirty_) {
        bpm_->pages_.setPage(frame_->page_id_, reinterpret_cast<const int *>(frame_->GetData()));
      }
      delete frame_;
      frame_ = nullptr;
    }
    void Delete() {
      bpm_->pages_.deletePage(frame_->page_id_);
      delete frame_;
      frame_ = nullptr;
    }

   private:
    BasicFrameGuard(BufferPoolManager<1> *bpm, Frame<1> *frame) : bpm_(bpm), frame_(frame) {}
    BufferPoolManager<1> *bpm_;
    Frame<1> *frame_;
  };
  auto NewFrameGuarded(page_id_t *page_id = nullptr) -> BasicFrameGuard;
  auto FetchFrameBasic(page_id_t page_id) -> BasicFrameGuard;

  int &getInfo(unsigned int n) { return pages_.getInfo(n); }

 private:
  external_memory::Pages pages_;
};

} // namespace storage