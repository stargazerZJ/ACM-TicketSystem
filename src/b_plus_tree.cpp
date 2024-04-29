//
// Created by zj on 4/29/2024.
//

#include "config.h"
#include "b_plus_tree.h"
#include <algorithm>

namespace storage {
template<typename KeyType, typename ValueType>
BPlusTree<KeyType, ValueType>::BPlusTree(BufferPoolManager<PagesPerFrame> *bpm)
    : bpm_(bpm), header_page_id_(INVALID_PAGE_ID) {
  auto header_frame_guard = bpm->NewFrameGuarded(&header_page_id_);
  auto header_frame = header_frame_guard.template AsMut<BPlusTreeHeaderFrame>();
  header_frame->root_page_id_ = INVALID_PAGE_ID;
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::CreateRootFrame() -> BPlusTree::BasicFrameGuard {
  auto header_frame_guard = bpm_->FetchFrameBasic(header_page_id_);
  auto header_frame = header_frame_guard.template AsMut<BPlusTreeHeaderFrame>();
//  if (header_frame->root_page_id_ != INVALID_PAGE_ID) {
//    return bpm_->FetchFrameBasic(header_frame->root_page_id_);
//  }
  auto root_frame_guard = bpm_->NewFrameGuarded(&header_frame->root_page_id_);
  auto root_frame = root_frame_guard.template AsMut<LeafFrame>();
  root_frame->Init();
  return root_frame_guard;
}

template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::Insert(const KeyType &key, const ValueType &value) -> bool {
  Context ctx = FindLeafFrame(key);
  if (ctx.writer_set_.empty()) {
    ctx.current_frame_ = CreateRootFrame();
    ctx.writer_set_.emplace_back(ctx.current_frame_.PageId(), 0);
  }
  {
    auto leaf = ctx.current_frame_.template As<LeafFrame>();
    if (ctx.writer_set_.back().Index() + 1 <= leaf->GetSize() &&
        leaf->KeyAt(ctx.writer_set_.back().Index()) == key) {
      return false;
    }
  }
  auto leaf = ctx.current_frame_.template AsMut<LeafFrame>();
  if (leaf->GetSize() < LeafFrame::GetMaxSize()) {
    InsertInLeaf(key, value, ctx);
  } else {
    // split
    auto new_leaf_guard = bpm_->NewFrameGuarded();
    auto new_leaf = new_leaf_guard.template AsMut<LeafFrame>();
    new_leaf->Init();
    auto split_index = leaf->GetMinSize();
    bool left = key < leaf->KeyAt(split_index);
    if (left) {
      split_index--;
    }
    size_t move_count = (leaf->GetSize() - split_index);
    std::memcpy(new_leaf->Keys() + 1, leaf->Keys() + split_index + 1, move_count * sizeof(KeyType));
    std::memcpy(new_leaf->Values() + 1, leaf->Values() + split_index + 1, move_count * sizeof(KeyType));
    new_leaf->SetSize(move_count);
    leaf->SetSize(split_index);
    new_leaf->SetNextPageId(leaf->GetNextPageId());
    leaf->SetNextPageId(new_leaf_guard.PageId());
    if (left) {
      InsertInLeaf(key, value, ctx);
    } else {
      // switch context to new leaf
      ctx.current_frame_ = std::move(new_leaf_guard);
      auto new_index = ctx.writer_set_.back().Index() - split_index;
      ctx.writer_set_.back() = {ctx.current_frame_.PageId(), new_index};
      InsertInLeaf(key, value, ctx);
    }
    InsertInParent(new_leaf->KeyAt(1), new_leaf_guard.PageId(), ctx);
  }
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::Remove(const KeyType &key) -> bool {
  return false;
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::GetValue(const KeyType &key, ValueType *value) -> BPlusTree::PositionHint {
  auto current_page_id = GetRootId();
  if (current_page_id == INVALID_PAGE_ID) {
    return {};
  }
  do {
    auto current_frame_guard = bpm_->FetchFrameBasic(current_page_id);
    bool is_leaf = current_frame_guard.template As<BPlusTreeFrame>()->IsLeafFrame();
    if (!is_leaf) {
      auto current_frame = current_frame_guard.template As<InternalFrame>();
      auto index = KeyIndex(key, current_frame) - 1;
      current_page_id = current_frame->ValueAt(index);
    } else {
      auto current_frame = current_frame_guard.template As<LeafFrame>();
      auto index = KeyIndex(key, current_frame);
      if (index < current_frame->GetSize() && current_frame->KeyAt(index) == key) {
        *value = current_frame->ValueAt(index);
        return {current_page_id, index};
      } else {
        return {};
      }
    }
  } while (true);
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::GetRootId() -> page_id_t {
  auto header_frame_guard = bpm_->FetchFrameBasic(header_page_id_);
  auto header_frame = header_frame_guard.template As<BPlusTreeHeaderFrame>();
  return header_frame->root_page_id_;
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::SetRootId(page_id_t root_id) -> void {
  auto header_frame_guard = bpm_->FetchFrameBasic(header_page_id_);
  auto header_frame = header_frame_guard.template AsMut<BPlusTreeHeaderFrame>();
  header_frame->root_page_id_ = root_id;
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::KeyIndex(const KeyType &key, auto *frame) -> int {
  // find the first index i that frame->KeyAt(i) <= key, frame->GetSize() + 1 if not found
  // TODO(opt): interpolation search
  auto keys = frame->Keys();
  auto it = std::lower_bound(keys + 1, keys + frame->GetSize() + 1, key);
  return it - keys;
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::FindLeafFrame(const KeyType &key) -> BPlusTree::Context {
  Context ctx;
  ctx.root_page_id_ = GetRootId();
  if (ctx.root_page_id_ == INVALID_PAGE_ID) {
    return ctx;
  }
  page_id_t current_page_id = ctx.root_page_id_;
  do {
    auto current_frame_guard = bpm_->FetchFrameBasic(current_page_id);
    bool is_leaf = current_frame_guard.template As<BPlusTreeFrame>()->IsLeafFrame();
    if (!is_leaf) {
      auto current_frame = current_frame_guard.template As<InternalFrame>();
      auto index = KeyIndex(key, current_frame) - 1;
      ctx.writer_set_.emplace_back(current_page_id, index);
      current_page_id = current_frame->ValueAt(index);
    } else {
      auto current_frame = current_frame_guard.template As<LeafFrame>();
      auto index = KeyIndex(key, current_frame) - 1;
      ctx.writer_set_.emplace_back(current_page_id, index);
      ctx.current_frame_ = std::move(current_frame_guard);
      return ctx;
    }
  } while (true);
}
template<typename KeyType, typename ValueType>
void BPlusTree<KeyType, ValueType>::MoveData(auto *array, size_t begin, size_t end, int offset) {
  char buffer[sizeof(LeafFrame)];
  size_t move_size = (end - begin) * sizeof(decltype(*array));
  std::memcpy(buffer, array + begin, move_size);
  std::memcpy(array + begin + offset, buffer, move_size);
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::InsertInLeaf(const KeyType &key,
                                                 const ValueType &value,
                                                 BPlusTree::Context &context) -> void {
  auto leaf_frame = context.current_frame_.template AsMut<LeafFrame>();
  auto index = context.writer_set_.back().Index() + 1;
  auto keys = leaf_frame->Keys();
  auto values = leaf_frame->Values();
  auto size = leaf_frame->GetSize();
  MoveData(keys, index, size + 1, 1);
  MoveData(values, index, size + 1, 1);
  keys[index] = key;
  values[index] = value;
  leaf_frame->IncreaseSize(1);
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::InsertInInternal(const KeyType &key,
                                                     page_id_t new_page_id,
                                                     BPlusTree::Context &context) -> void {
  auto internal_frame = context.current_frame_.template AsMut<InternalFrame>();
  auto index = context.writer_set_.back().Index() + 1;
  auto keys = internal_frame->Keys();
  auto values = internal_frame->Values();
  auto size = internal_frame->GetSize();
  MoveData(keys, index, size + 1, 1);
  MoveData(values, index, size + 1, 1);
  keys[index] = key;
  values[index] = new_page_id;
  internal_frame->IncreaseSize(1);
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::InsertInParent(const KeyType &key,
                                                     page_id_t new_page_id,
                                                     BPlusTree::Context &context) -> void {
  //TODO
}

template
class storage::BPlusTree<hash_t, int>;
}