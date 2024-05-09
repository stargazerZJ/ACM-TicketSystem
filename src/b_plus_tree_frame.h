//
// Created by zj on 4/28/2024.
//

#pragma once
#include <algorithm>
#include <execution>
#include "buffer_pool_manager.h"

namespace storage {

enum class IndexFrameType : uint8_t {
  INTERNAL_FRAME = 0,
  LEAF_FRAME = 1,
};

/**
 * Both internal and leaf frame are inherited from this frame.
 *
 * It actually serves as a header part for each B+ tree frame and
 * contains information shared by both leaf frame and internal frame.
 *
 * Header format (size in bit, 4 bytes in total):
 * ---------------------------------------------
 * | FrameType (1) | CurrentSize (31) |  ...   |
 * ---------------------------------------------
 */
class BPlusTreeFrame {
 public:
  // Delete all constructor / destructor to ensure memory safety
  BPlusTreeFrame() = delete;
  BPlusTreeFrame(const BPlusTreeFrame &other) = delete;
  ~BPlusTreeFrame() = delete;

  auto IsLeafFrame() const -> bool { return frame_type_ == IndexFrameType::LEAF_FRAME; }
  void SetFrameType(IndexFrameType frame_type) { frame_type_ = frame_type; }

  auto GetSize() const -> int { return size_; }
  void SetSize(int size) { size_ = size; }
  void IncreaseSize(int amount) { size_ += amount; }

 private:
  // Member variables, attributes that both internal and leaf frame share
  // bitfield is used to save memory
  IndexFrameType frame_type_: 1;
  int size_: 31;
};

static_assert(sizeof(BPlusTreeFrame) == 4, "BPlusTreeFrame size is not correct");

/**
 * Store `n` indexed keys and `n + 1` child pointers (page_id) within internal frame.
 * Pointer PAGE_ID(i) points to a subtree in which all keys K satisfy:
 * K(i) <= K < K(i+1).
 *
 * Internal frame format (keys are stored in increasing order):
 * ----------------------------------------------------------------------------------------
 * | HEADER | KEY(1) | KEY(2) | ... | KEY(n) | PAGE_ID(0) | PAGE_ID(1) | ... | PAGE_ID(n) |
 * ----------------------------------------------------------------------------------------
 */
template<typename KeyType, typename ValueType, int PagePerFrame = 1>
class BPlusTreeInternalFrame : public BPlusTreeFrame {
 public:
  // Delete all constructor / destructor to ensure memory safety
  BPlusTreeInternalFrame() = delete;
  BPlusTreeInternalFrame(const BPlusTreeInternalFrame &other) = delete;

  /**
   * Writes the necessary header information to a newly created frame, must be called after
   * the creation of a new frame to make a valid `BPlusTreeInternalFrame`
   */
  void Init();

  /**
   * @return The pointer to the keys array
   */
  auto Keys() -> KeyType * { return keys_ - 1; }
  auto Keys() const -> const KeyType * { return keys_ - 1; }
  auto Values() -> ValueType * { return values_; }
  auto Values() const -> const ValueType * { return values_; }

  /**
   * @param index The index of the key to get. Index must be non-zero.
   * @return Key at index
   */
  auto KeyAt(int index) const -> KeyType { return keys_[index - 1]; }

  /**
   * @param index The index of the key to set. Index must be non-zero.
   * @param key The new value for key
   */
  void SetKeyAt(int index, const KeyType &key) { keys_[index - 1] = key; }

  /**
   * @param value The value to search for
   * @return The index that corresponds to the specified value
   */
  auto ValueIndex(const ValueType &value) const -> int;

  /**
   * @param index The index to search for
   * @return The value at the index
   */
  auto ValueAt(int index) const -> ValueType { return values_[index]; }
  void SetValueAt(int index, const ValueType &value) { values_[index] = value; }

  static constexpr int GetMaxSize() {
    return std::min(
        (Frame<PagePerFrame>::kFrameSize - kHeaderSize - sizeof(page_id_t)) / (sizeof(KeyType) + sizeof(page_id_t)),
        static_cast<size_t>(BPT_MAX_DEGREE)
    );
  }
  static constexpr int GetMinSize() { return kMaxSize / 2; }
 private:
  static constexpr int kHeaderSize = sizeof(BPlusTreeFrame);
  static constexpr int kMaxSize = GetMaxSize(); // the maximal index of keys in this frame

  KeyType keys_[kMaxSize];
  ValueType values_[kMaxSize + 1];

};
template<typename KeyType, typename ValueType, int PagePerFrame>
void BPlusTreeInternalFrame<KeyType, ValueType, PagePerFrame>::Init() {
  SetSize(0);
  SetFrameType(IndexFrameType::INTERNAL_FRAME);
}
template<typename KeyType, typename ValueType, int PagePerFrame>
auto BPlusTreeInternalFrame<KeyType, ValueType, PagePerFrame>::ValueIndex(const ValueType &value) const -> int {
  // use std::find and std::execution::par_unseq
  auto it = std::find(std::execution::par_unseq,
                      values_, values_ + GetSize() + 1, value);
  return it - values_;
}

/**
 * Store indexed key and value together within leaf page. Only support unique key.
 *
 * Leaf page format (keys are stored in order):
 * --------------------------------------------------------------
 * | HEADER | KEY(1) | ... | KEY(n) | VALUE(1) | ... | VALUE(n) |
 * --------------------------------------------------------------
 *
 * Header format (size in bit, 8 bytes in total):
 * -----------------------------------------------------
 * | PageType (1) | CurrentSize (31) | NextPageId (32) |
 * -----------------------------------------------------
 */
template<typename KeyType, typename ValueType, int PagePerFrame = 1>
class BPlusTreeLeafFrame : public BPlusTreeFrame {
 public:
  // Delete all constructor / destructor to ensure memory safety
  BPlusTreeLeafFrame() = delete;
  BPlusTreeLeafFrame(const BPlusTreeLeafFrame &other) = delete;

  /**
  * After creating a new leaf page from buffer pool, must call initialize
  * method to set default values
  */
  void Init();

  // Helper methods
  auto GetNextPageId() const -> page_id_t { return next_page_id_; }
  void SetNextPageId(page_id_t next_page_id) { next_page_id_ = next_page_id; }

  /**
   * @return The pointer to the keys array
   */
  auto Keys() -> KeyType * { return keys_ - 1; }
  auto Keys() const -> const KeyType * { return keys_ - 1; }
  auto Values() -> ValueType * { return values_ - 1; }
  auto Values() const -> const ValueType * { return values_ - 1; }
  /**
  * @param index The index of the key to get. Index must be non-zero.
  * @return Key at index
  */
  auto KeyAt(int index) const -> KeyType { return keys_[index - 1]; }

  /**
  * @param index The index of the key to set. Index must be non-zero.
  * @param key The new value for key
  */
  void SetKeyAt(int index, const KeyType &key) { keys_[index - 1] = key; }

  /**
  * @param index The index to search for
  * @return The value at the index
  */
  auto ValueAt(int index) const -> ValueType { return values_[index - 1]; }
  void SetValueAt(int index, const ValueType &value) { values_[index - 1] = value; }

  static constexpr int GetMaxSize() {
    return std::min(
        (Frame<PagePerFrame>::kFrameSize - kHeaderSize) / (sizeof(KeyType) + sizeof(ValueType)),
        static_cast<size_t>(BPT_MAX_DEGREE)
    );
  }
  static constexpr int GetMinSize() { return kMaxSize / 2; }

 private:
  static constexpr int kHeaderSize = sizeof(BPlusTreeFrame) + sizeof(page_id_t);
  static constexpr int kMaxSize = GetMaxSize(); // the maximal index of keys in this frame

  page_id_t next_page_id_ = INVALID_PAGE_ID;
  KeyType keys_[kMaxSize];
  ValueType values_[kMaxSize];
};

template<typename KeyType, typename ValueType, int PagePerFrame>
void BPlusTreeLeafFrame<KeyType, ValueType, PagePerFrame>::Init() {
  SetSize(0);
  SetFrameType(IndexFrameType::LEAF_FRAME);
  next_page_id_ = INVALID_PAGE_ID;
}

/**
 * The header frame is just used to retrieve the root frame,
 * preventing potential race condition under concurrent environment.
 */
class BPlusTreeHeaderFrame {
 public:
  // Delete all constructor / destructor to ensure memory safety
  BPlusTreeHeaderFrame() = delete;
  BPlusTreeHeaderFrame(const BPlusTreeHeaderFrame &other) = delete;

  page_id_t root_page_id_;
};

// instantiate template class
template
class BPlusTreeInternalFrame<hash_t, page_id_t, 4>;
static_assert(sizeof(BPlusTreeInternalFrame<hash_t, page_id_t, 4>) <= Frame<4>::kFrameSize,
              "BPlusTreeInternalFrame size is not correct");

template
class BPlusTreeLeafFrame<hash_t, int, 4>;
static_assert(sizeof(BPlusTreeLeafFrame<hash_t, int, 4>) <= Frame<4>::kFrameSize,
              "BPlusTreeLeafFrame size is not correct");

}  // namespace storage