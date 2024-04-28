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
  auto GetData() -> char * { return data_; }
  auto GetPageId() -> page_id_t { return page_id_; }
  auto GetPinCount() -> int { return pin_count_; }
  auto IsDirty() -> bool { return is_dirty_; }

 private:
  page_id_t page_id_ = INVALID_PAGE_ID;
  bool is_dirty_ = false;
  int pin_count_ = 0;
  char data_[PAGE_SIZE * PagesPerFrame]{};
};

template<>
class BufferPoolManager<1> {
 public:
  explicit BufferPoolManager(const std::string &file_path, bool reset = false) : pages_(file_path) {
    pages_.initialize(reset);
  }

  auto NewFrameGuarded(page_id_t *page_id);
  auto FetchFrameBasic(page_id_t page_id);

  class BasicFrameGuard { // Currently, one frame can only be pinned once
    friend BufferPoolManager<1>;
   public:

    ~BasicFrameGuard() {
      if (frame_ != nullptr) {
        Drop();
      }
    }

    auto PageId() -> page_id_t { return frame_->GetPageId(); }
    auto GetData() -> char * { return frame_->GetData(); }
    auto GetDataMut() -> char * {
      frame_->is_dirty_ = true;
      return frame_->GetData();
    }
    template<class T>
    auto As() -> const T * {
      return reinterpret_cast<const T *>(GetData());
    }
    template<class T>
    auto AsMut() -> T * {
      return reinterpret_cast<T *>(GetDataMut());
    }
    void Drop() { // as no caching is implemented, this is equivalent to unpin
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
 private:
  external_memory::Pages pages_;
};
auto BufferPoolManager<1>::NewFrameGuarded(page_id_t *page_id) {
  auto frame = new Frame<1>();
  *page_id = pages_.newPage(reinterpret_cast<const int *>(frame->GetData()));
  frame->page_id_ = *page_id;
  return BasicFrameGuard(this, frame);
}
auto BufferPoolManager<1>::FetchFrameBasic(page_id_t page_id) {
  auto frame = new Frame<1>();
  pages_.fetchPage(page_id);
  pages_.getPage(page_id, reinterpret_cast<int *>(frame->GetData()));
  frame->page_id_ = page_id;
  return BasicFrameGuard(this, frame);
}

} // namespace storage