//
// Created by zj on 4/28/2024.
//

#pragma once
#include "b_plus_tree_frame.h"
#include "buffer_pool_manager.h"
#include "stlite/vector.h"

namespace storage {
template<typename KeyType, typename ValueType>
class BPlusTree {
 private:
  static constexpr int PagesPerFrame = 1;
 public:
  using InternalFrame = BPlusTreeInternalFrame<KeyType, page_id_t, PagesPerFrame>;
  using LeafFrame = BPlusTreeLeafFrame<KeyType, ValueType, PagesPerFrame>;
  using BasicFrameGuard = BufferPoolManager<PagesPerFrame>::BasicFrameGuard;
  class PositionHint;

  explicit BPlusTree(BufferPoolManager<PagesPerFrame> *bpm);

  auto Insert(const KeyType &key, const ValueType &value) -> bool;

  auto Remove(const KeyType &key) -> bool;

  auto GetValue(const KeyType &key, ValueType *value) -> PositionHint;

  auto SetValue(const KeyType &key, const ValueType &value, const PositionHint &hint) -> bool;

  auto SetValue(const KeyType &key, const ValueType &value) -> bool;

  auto GetRootId() -> page_id_t;

  class PositionHint {
    friend class BPlusTree<KeyType, ValueType>;
   public:
    PositionHint() = default;
    PositionHint(page_id_t page_id, int index) : page_id_(page_id), index_(index) {}

    auto PageId() const -> page_id_t { return page_id_; }
    auto Index() const -> int { return index_; }
    auto found() const -> bool { return page_id_ != INVALID_PAGE_ID; }
    operator bool() const { return found(); }

   private:
    page_id_t page_id_{INVALID_PAGE_ID};
    int index_{};
  };

 private:
  BufferPoolManager<PagesPerFrame> *bpm_;
  page_id_t header_page_id_{};

  class Context {
   public:
    page_id_t root_page_id_{INVALID_PAGE_ID};
    sjtu::vector<PositionHint> writer_set_;
    BasicFrameGuard current_frame_{};
    auto IsRootPage(page_id_t page_id) -> bool { return page_id == root_page_id_; }
  };

  auto CreateRootFrame() -> BasicFrameGuard;
  auto SetRootId(page_id_t root_id) -> void;
  auto KeyIndex(const KeyType &key, auto *frame) -> int;
  auto FindLeafFrame(const KeyType &key) -> Context;
  static void MoveData(auto *array, size_t begin, size_t end, int offset);
  auto InsertInLeaf(const KeyType &key, const ValueType &value, Context &context) -> void;
  auto InsertInInternal(const KeyType &key, page_id_t new_page_id, BPlusTree::Context &context) -> void;
  auto InsertInParent(const KeyType &key, page_id_t new_page_id, BPlusTree::Context &context) -> void;
};

} // namespace storage