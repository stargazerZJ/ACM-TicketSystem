//
// Created by zj on 5/13/2024.
//

#pragma once

#include <config.h>

#include <utility>

#include "buffer_pool_manager.h"

namespace storage {
/// @brief A variable-length type T must have member type `data_t`. Its actual size is `sizeof(T) + n * sizeof(data_t)`
template<class T>
concept var_length_object = requires(T t) { typename T::data_t; };

class VarLengthStore {
  public:
    static constexpr int PagesPerFrame = VLS_PAGES_PER_FRAME;
    using length_t = int32_t;
    static constexpr int kFrameSize = Frame<PagesPerFrame>::kFrameSize;
    using BasicFrameGuard = BufferPoolManager<PagesPerFrame>::BasicFrameGuard;

    template<var_length_object T>
    class Handle<T>;

    explicit VarLengthStore(BufferPoolManager<PagesPerFrame> *bpm, record_id_t &top_pos);

    template<var_length_object T>
    auto Allocate(length_t n = 0) -> Handle<T>;

    template<var_length_object T>
    auto Get(const record_id_t &pos) -> Handle<T>;

    template<var_length_object T>
    constexpr auto GetBaseSize() { return sizeof(T); }

    template<var_length_object T>
    constexpr auto GetDataSize() { return sizeof(T::data_t); }

    template<var_length_object T>
    constexpr auto GetObjectSize(length_t n = 0) { return GetBaseSize<T>() + n * GetDataSize<T>(); }

    template<var_length_object T>
    class Handle<T> {
      public:
        Handle() = default;

        auto Get() -> const T * { return reinterpret_cast<T *>(guard_.GetData() + pos_); }
        auto GetMut() -> T * { return reinterpret_cast<T *>(guard_.GetDataMut() + pos_); }

        auto operator ->() -> const T * { return Get(); }
        auto operator *() -> const T & { return *Get(); }

      private:
        BasicFrameGuard guard_{};
        length_t pos_{0};

        Handle(BasicFrameGuard guard, length_t pos) : guard_(std::move(guard)), pos_(pos) {
        }
    };

  private:
    BufferPoolManager<PagesPerFrame> *bpm_;
    record_id_t &top_pos_;

    static auto GetPageId(const record_id_t &pos) -> page_id_t { return pos / kFrameSize; }

    static auto GetRemainingSize(const record_id_t &pos) -> length_t { return (kFrameSize - pos) % kFrameSize; }
};
template<var_length_object T>
auto VarLengthStore::Allocate(length_t n) -> Handle<T> {
  auto object_size = GetObjectSize<T>(n);
  ASSERT(object_size <= kFrameSize);
  auto remaining_size = GetRemainingSize(top_pos_);
  BasicFrameGuard guard;
  if (remaining_size < object_size) {
    // Allocate a new frame from bpm
    guard = bpm_->NewFrameGuarded();
    top_pos_ = guard.PageId() * kFrameSize;
  } else {
    guard = bpm_->FetchFrameBasic(GetPageId(top_pos_));
  }
  Handle<T> ret = Handle<T>(guard, top_pos_);
  top_pos_ += object_size;
  return ret;
}
template<var_length_object T>
auto VarLengthStore::Get(const record_id_t &pos) -> Handle<T> {
  auto page_id = GetPageId(pos);
  auto offset = pos % kFrameSize;
  BasicFrameGuard guard = bpm_->FetchFrameBasic(page_id);
  return Handle<T>(guard, offset);
}
} // namespace storage