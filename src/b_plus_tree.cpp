//
// Created by zj on 4/29/2024.
//

#include "config.h"
#include "b_plus_tree.h"

namespace storage {
template<typename KeyType, typename ValueType>
BPlusTree<KeyType, ValueType>::BPlusTree(BufferPoolManager<PagesPerFrame> *bpm, page_id_t &root_page_id)
    : bpm_(bpm), root_page_id_(root_page_id) {
  static_assert(sizeof(InternalFrame) <= Frame<PagesPerFrame>::kFrameSize);
  static_assert(sizeof(LeafFrame) <= Frame<PagesPerFrame>::kFrameSize);
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::CreateRootFrame() -> BasicFrameGuard {
  auto root_frame_guard = bpm_->NewFrameGuarded(&root_page_id_);
  auto root_frame = root_frame_guard.AsMut<LeafFrame>();
  root_frame->Init();
  return root_frame_guard;
}

template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::Insert(const KeyType &key, const ValueType &value) -> bool {
  Context ctx = FindLeafFrame(key);
  if (ctx.stack_.empty()) {
    ctx.current_frame_ = CreateRootFrame();
    ctx.root_page_id_ = ctx.current_frame_.PageId();
    ctx.stack_.emplace_back(ctx.root_page_id_, 0);
  }
  {
    auto leaf = ctx.current_frame_.template As<LeafFrame>();
    if (ctx.stack_.back().Index() <= leaf->GetSize() &&
        leaf->KeyAt(ctx.stack_.back().Index()) == key) {
      return false;
    }
  }
  auto leaf = ctx.current_frame_.template AsMut<LeafFrame>();
  if (leaf->GetSize() < LeafFrame::GetMaxSize()) {
    InsertInLeaf(key, value, ctx);
  } else {
    // split
    auto old_page_id = ctx.stack_.back().PageId();
    auto new_leaf_guard = bpm_->NewFrameGuarded();
    auto new_leaf = new_leaf_guard.AsMut<LeafFrame>();
    new_leaf->Init();
    auto split_index = (leaf->GetSize() + 1) / 2;
    bool left = key < leaf->KeyAt(split_index);
    if (left) {
      --split_index;
    }
    size_t move_count = (leaf->GetSize() - split_index);
    std::memcpy(new_leaf->Keys() + 1, leaf->Keys() + split_index + 1, move_count * sizeof(KeyType));
    std::memcpy(new_leaf->Values() + 1, leaf->Values() + split_index + 1, move_count * sizeof(ValueType));
    new_leaf->SetSize(move_count);
    leaf->SetSize(split_index);
    new_leaf->SetNextPageId(leaf->GetNextPageId());
    leaf->SetNextPageId(new_leaf_guard.PageId());
    if (left) {
      InsertInLeaf(key, value, ctx);
    } else {
      // switch context to new leaf
      ctx.current_frame_ = std::move(new_leaf_guard);
      auto new_index = ctx.stack_.back().Index() - split_index;
      ctx.stack_.back() = {ctx.current_frame_.PageId(), new_index};
      InsertInLeaf(key, value, ctx);
      new_leaf_guard = std::move(ctx.current_frame_);
    }
    InsertInParent(old_page_id, new_leaf->KeyAt(1), new_leaf_guard.PageId(), ctx);
  }
  return true;
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::Remove(const KeyType &key) -> bool {
  auto ctx = FindLeafFrame(key);
  if (ctx.stack_.empty()) {
    return false;
  }
  auto leaf = ctx.current_frame_.template As<LeafFrame>();
  if (ctx.stack_.back().Index() > leaf->GetSize()
      || leaf->KeyAt(ctx.stack_.back().Index()) != key) {
    return false;
  }
  RemoveInLeaf(ctx);
  return true;
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::GetValue(const KeyType &key, ValueType *value) -> PositionHint {
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
      auto index = KeyIndex(key, current_frame) - 1;
      if (0 < index && index <= current_frame->GetSize() && current_frame->KeyAt(index) == key) {
        if (value) *value = current_frame->ValueAt(index);
        return {current_page_id, index};
      } else {
        return {};
      }
    }
  } while (true);
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::LowerBound(const KeyType &key) -> PositionHint {
  auto ctx = FindLeafFrame(key);
  if (ctx.stack_.empty()) {
    return {};
  }
  auto leaf = ctx.current_frame_.template As<LeafFrame>();
  auto index = ctx.stack_.back().Index();
  if (0 < index && index <= leaf->GetSize() && leaf->KeyAt(index) == key) {
    return ctx.stack_.back();
  }
  if (index < leaf->GetSize()) {
    return {ctx.stack_.back().PageId(), index + 1};
  } else {
    if (leaf->GetNextPageId() == INVALID_PAGE_ID) {
      return {};
    } else {
      return {leaf->GetNextPageId(), 1};
    }
  }
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::GetRootId() const -> page_id_t {
  return root_page_id_;
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::SetRootId(page_id_t root_id) -> void {
  root_page_id_ = root_id;
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::KeyIndex(const KeyType &key, auto *frame) -> int {
  // find the first index i that key < frame->KeyAt(i), frame->GetSize() + 1 if not found
  // TODO(opt): interpolation search
  auto keys = frame->Keys();
  auto it = upper_bound(keys + 1, keys + frame->GetSize() + 1, key);
  return it - keys;
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::FindLeafFrame(const KeyType &key) -> Context {
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
      ctx.stack_.emplace_back(current_page_id, index);
      current_page_id = current_frame->ValueAt(index);
    } else {
      auto current_frame = current_frame_guard.template As<LeafFrame>();
      auto index = KeyIndex(key, current_frame) - 1;
      ctx.stack_.emplace_back(current_page_id, index);
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
                                                 Context &context) -> void {
  auto leaf_frame = context.current_frame_.template AsMut<LeafFrame>();
  auto index = context.stack_.back().Index() + 1;
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
                                                     Context &context) -> void {
  auto internal_frame = context.current_frame_.template AsMut<InternalFrame>();
  auto index = context.stack_.back().Index() + 1;
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
auto BPlusTree<KeyType, ValueType>::InsertInParent(page_id_t old_page_id,
                                                   const KeyType &key,
                                                   page_id_t new_page_id,
                                                   Context &context) -> void {
  if (context.IsRootPage(old_page_id)) {
    auto new_root_guard = bpm_->NewFrameGuarded();
    auto new_root = new_root_guard.template AsMut<InternalFrame>();
    new_root->Init();
    new_root->SetSize(1);
    new_root->SetKeyAt(1, key);
    new_root->SetValueAt(0, context.root_page_id_);
    new_root->SetValueAt(1, new_page_id);
    SetRootId(new_root_guard.PageId());
    return;
  }
  context.stack_.pop_back();
  context.current_frame_ = bpm_->FetchFrameBasic(context.stack_.back().PageId());
  auto parent_frame = context.current_frame_.template AsMut<InternalFrame>();
  if (parent_frame->GetSize() < InternalFrame::GetMaxSize()) {
    InsertInInternal(key, new_page_id, context);
  } else {
    // split
    old_page_id = context.stack_.back().PageId();
    auto new_internal_guard = bpm_->NewFrameGuarded();
    auto new_internal = new_internal_guard.template AsMut<InternalFrame>();
    new_internal->Init();
    auto split_index = (parent_frame->GetSize() + 1) / 2 + 1;
    bool left = key < parent_frame->KeyAt(split_index);
    if (left) {
      split_index--;
    }
    auto key_to_insert = parent_frame->KeyAt(split_index);
    size_t move_count = (parent_frame->GetSize() - split_index);
    std::memcpy(new_internal->Keys() + 1, parent_frame->Keys() + split_index + 1, move_count * sizeof(KeyType));
    std::memcpy(new_internal->Values(), parent_frame->Values() + split_index, (move_count + 1) * sizeof(page_id_t));
    new_internal->SetSize(move_count);
    parent_frame->SetSize(split_index - 1);
    if (left) {
      if (key_to_insert < key) {
        parent_frame->IncreaseSize(1);
        new_internal->SetValueAt(0, new_page_id);
        key_to_insert = key;
      } else {
        InsertInInternal(key, new_page_id, context);
      }
    } else {
      // switch context to new internal
      context.current_frame_ = std::move(new_internal_guard);
      auto new_index = context.stack_.back().Index() - split_index;
      context.stack_.back() = {context.current_frame_.PageId(), new_index};
      InsertInInternal(key, new_page_id, context);
      new_internal_guard = std::move(context.current_frame_);
    }
    InsertInParent(old_page_id, key_to_insert, new_internal_guard.PageId(), context);
  }
}
template<typename KeyType, typename ValueType>
template<typename FrameType>
auto BPlusTree<KeyType, ValueType>::RemoveInFrame(Context &context) -> void {
  auto frame = context.current_frame_.template AsMut<FrameType>();
  auto index = context.stack_.back().Index();
  auto size = frame->GetSize();
  MoveData(frame->Keys(), index + 1, size + 1, -1);
  MoveData(frame->Values(), index + 1, size + 1, -1);
  frame->IncreaseSize(-1);
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::FindSibling(Context &context, const InternalFrame *parent_frame) {
  auto index = context.stack_[context.stack_.size() - 2].Index();
  if (index == 0) {
    return std::make_pair(parent_frame->ValueAt(1), true);
  } else {
    return std::make_pair(parent_frame->ValueAt(index - 1), false);
  }
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::RemoveInLeaf(Context &context) -> void {
  RemoveInFrame<LeafFrame>(context);
  auto leaf = context.current_frame_.template AsMut<LeafFrame>();
  if (context.IsRootPage(context.current_frame_.PageId())) {
    if (leaf->GetSize() == 0) {
      context.current_frame_.Delete();
      SetRootId(INVALID_PAGE_ID);
    }
    return;
  }
  if (leaf->GetSize() >= LeafFrame::GetMinSize()) {
    return;
  }
  auto parent_frame_guard = bpm_->FetchFrameBasic(
      context.stack_[context.stack_.size() - 2].PageId());
  auto parent_frame = parent_frame_guard.template AsMut<InternalFrame>();
  auto [sibling_page_id, sibling_is_right] = FindSibling(context, parent_frame);
  auto parent_index = context.stack_[context.stack_.size() - 2].Index() + sibling_is_right;
  auto sibling_frame_guard = bpm_->FetchFrameBasic(sibling_page_id);
  auto sibling_frame = sibling_frame_guard.template AsMut<LeafFrame>();
  if (sibling_frame->GetSize() > LeafFrame::GetMinSize()) {
    // borrow
    if (sibling_is_right) {
      auto key = sibling_frame->KeyAt(1);
      auto value = sibling_frame->ValueAt(1);
      leaf->SetKeyAt(leaf->GetSize() + 1, key);
      leaf->SetValueAt(leaf->GetSize() + 1, value);
      leaf->IncreaseSize(1);
      // switch context to sibling
      context.current_frame_ = std::move(sibling_frame_guard);
      context.stack_.back() = {sibling_page_id, 1};
      RemoveInFrame<LeafFrame>(context);
      parent_frame->SetKeyAt(parent_index, sibling_frame->KeyAt(1));
    } else {
      auto key = sibling_frame->KeyAt(sibling_frame->GetSize());
      auto value = sibling_frame->ValueAt(sibling_frame->GetSize());
      sibling_frame->IncreaseSize(-1);
      context.stack_.back() = {context.stack_.back().PageId(), 0};
      InsertInLeaf(key, value, context);
      parent_frame->SetKeyAt(parent_index, key);
    }
  } else {
    // merge
    auto left = sibling_is_right ? leaf : sibling_frame;
    auto right = sibling_is_right ? sibling_frame : leaf;
    auto move_count = right->GetSize();
    std::memcpy(left->Keys() + left->GetSize() + 1, right->Keys() + 1, move_count * sizeof(KeyType));
    std::memcpy(left->Values() + left->GetSize() + 1, right->Values() + 1, move_count * sizeof(ValueType));
    left->IncreaseSize(move_count);
    left->SetNextPageId(right->GetNextPageId());
    if (sibling_is_right) {
      sibling_frame_guard.Delete();
    } else {
      context.current_frame_.Delete();
    }
    context.current_frame_ = std::move(parent_frame_guard);
    context.stack_.pop_back();
    if (sibling_is_right) {
      auto pos = context.stack_.back();
      pos = {pos.PageId(), pos.Index() + 1};
      context.stack_.back() = pos;
    }
    RemoveInInternal(context);
  }
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::RemoveInInternal(Context &context) -> void {
  RemoveInFrame<InternalFrame>(context);
  auto internal = context.current_frame_.template AsMut<InternalFrame>();
  if (context.IsRootPage(context.current_frame_.PageId())) {
    if (internal->GetSize() == 0) {
      SetRootId(internal->ValueAt(0));
      context.current_frame_.Delete();
    }
    return;
  }
  if (internal->GetSize() >= InternalFrame::GetMinSize()) {
    return;
  }
  auto parent_frame_guard = bpm_->FetchFrameBasic(
      context.stack_[context.stack_.size() - 2].PageId());
  auto parent_frame = parent_frame_guard.template AsMut<InternalFrame>();
  auto [sibling_page_id, sibling_is_right] = FindSibling(context, parent_frame);
  auto parent_index = context.stack_[context.stack_.size() - 2].Index() + sibling_is_right;
  auto parent_key = parent_frame->KeyAt(parent_index);
  auto sibling_frame_guard = bpm_->FetchFrameBasic(sibling_page_id);
  auto sibling_frame = sibling_frame_guard.template AsMut<InternalFrame>();
  if (sibling_frame->GetSize() > InternalFrame::GetMinSize()) {
    // borrow
    if (sibling_is_right) {
      auto key = sibling_frame->KeyAt(1);
      auto value = sibling_frame->ValueAt(0);
      internal->SetKeyAt(internal->GetSize() + 1, parent_key);
      internal->SetValueAt(internal->GetSize() + 1, value);
      internal->IncreaseSize(1);
      MoveData(sibling_frame->Keys(), 2, sibling_frame->GetSize() + 1, -1);
      MoveData(sibling_frame->Values(), 1, sibling_frame->GetSize() + 1, -1);
      sibling_frame->IncreaseSize(-1);
      parent_frame->SetKeyAt(parent_index, key);
    } else {
      auto key = sibling_frame->KeyAt(sibling_frame->GetSize());
      auto value = sibling_frame->ValueAt(sibling_frame->GetSize());
      MoveData(internal->Keys(), 1, internal->GetSize() + 1, 1);
      MoveData(internal->Values(), 0, internal->GetSize() + 1, 1);
      internal->SetKeyAt(1, parent_key);
      internal->SetValueAt(0, value);
      internal->IncreaseSize(1);
      sibling_frame->IncreaseSize(-1);
      parent_frame->SetKeyAt(parent_index, key);
    }
  } else {
    // merge
    auto left = sibling_is_right ? internal : sibling_frame;
    auto right = sibling_is_right ? sibling_frame : internal;
    auto move_count = right->GetSize();
    left->SetKeyAt(left->GetSize() + 1, parent_key);
    std::memcpy(left->Keys() + left->GetSize() + 2, right->Keys() + 1, move_count * sizeof(KeyType));
    std::memcpy(left->Values() + left->GetSize() + 1, right->Values(), (move_count + 1) * sizeof(page_id_t));
    left->IncreaseSize(move_count + 1);
    if (sibling_is_right) {
      sibling_frame_guard.Delete();
    } else {
      context.current_frame_.Delete();
    }
    context.current_frame_ = std::move(parent_frame_guard);
    context.stack_.pop_back();
    if (sibling_is_right) {
      auto pos = context.stack_.back();
      pos = {pos.PageId(), pos.Index() + 1};
      context.stack_.back() = pos;
    }
    RemoveInInternal(context);
  }
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::SetValue(const KeyType &key,
                                             const ValueType &value,
                                             const PositionHint &hint) -> bool {
  if (!hint.found()) return SetValue(key, value);
  auto guard = bpm_->FetchFrameBasic(hint.PageId());
  auto leaf = guard.template AsMut<LeafFrame>();
  if (leaf->IsLeafFrame() && hint.Index() < leaf->GetSize() && leaf->KeyAt(hint.Index()) == key) {
    leaf->SetValueAt(hint.Index(), value);
    return false;
  }
  return SetValue(key, value);
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::SetValue(const KeyType &key, const ValueType &value) -> bool {
  if (Insert(key, value)) {
    return true;
  }
  auto hint = GetValue(key);
  auto guard = bpm_->FetchFrameBasic(hint.PageId());
  auto leaf = guard.template AsMut<LeafFrame>();
  leaf->SetValueAt(hint.Index(), value);
  return false;
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::PartialSearch(const auto &key) -> std::vector<std::pair<KeyType, ValueType>> {
  using KeyTypeFirst = KeyType::first_type;
  using KeyTypeSecond = KeyType::second_type;
  static_assert(std::is_same_v<decltype(key), const KeyTypeFirst &>);
  std::vector<std::pair<KeyType, ValueType>> result;
  auto pos = LowerBound(KeyType(key, std::numeric_limits<KeyTypeSecond>::min()));
  auto it = Iterator(this, pos);
  while (it != End() && it.Key().first == key) {
    result.emplace_back(it.Key(), it.Value());
    ++it;
  }
  return result;
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::RemoveAll(const auto &key) -> void {
  auto toDelete = PartialSearch(key);
  for (auto &pair : toDelete) {
    Remove(pair.first);
  }
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::ValidateBPlusTree(page_id_t root_page_id,
                                                      page_id_t page_id,
                                                      const KeyType &lower_bound,
                                                      const KeyType &upper_bound) -> int {
  if (page_id < 1) {
    throw std::runtime_error("Invalid page id");
  }
  auto frame_guard = bpm_->FetchFrameBasic(page_id);
  bool is_leaf = frame_guard.template As<BPlusTreeFrame>()->IsLeafFrame();
  if (is_leaf) {
    auto leaf = frame_guard.template As<LeafFrame>();
    if (page_id == root_page_id) {
      if (leaf->GetSize() == 0) {
        throw std::runtime_error("Root page size is 0");
      }
    } else {
      if (leaf->GetSize() < LeafFrame::GetMinSize()) {
        throw std::runtime_error("Leaf frame size too small");
      }
    }
    if (leaf->GetSize() > LeafFrame::GetMaxSize()) {
      throw std::runtime_error("Leaf frame size too large");
    }
    if (leaf->KeyAt(1) < lower_bound || leaf->KeyAt(leaf->GetSize()) >= upper_bound) {
      throw std::runtime_error("Leaf frame key out of range");
    }
    for (int i = 1; i <= leaf->GetSize(); i++) {
      if (i > 1 && leaf->KeyAt(i) <= leaf->KeyAt(i - 1)) {
        throw std::runtime_error("Leaf frame key not in order");
      }
    }
    return 1;
  } else {
    auto internal = frame_guard.template As<InternalFrame>();
    if (page_id == root_page_id) {
      if (internal->GetSize() == 0) {
        throw std::runtime_error("Root page size is 0");
      }
    } else {
      if (internal->GetSize() < InternalFrame::GetMinSize()) {
        throw std::runtime_error("Internal frame size too small");
      }
    }
    if (internal->GetSize() > InternalFrame::GetMaxSize()) {
      throw std::runtime_error("Internal frame size too large");
    }
    if (internal->KeyAt(1) < lower_bound || internal->KeyAt(internal->GetSize()) >= upper_bound) {
      throw std::runtime_error("Internal frame key out of range");
    }
    int depth = 0;
    for (int i = 0; i <= internal->GetSize(); i++) {
      if (i > 1 && internal->KeyAt(i) <= internal->KeyAt(i - 1)) {
        throw std::runtime_error("Internal frame key not in order");
      }
      auto child_depth = ValidateBPlusTree(root_page_id,
                                           internal->ValueAt(i),
                                           i == 0 ? lower_bound : internal->KeyAt(i),
                                           i == internal->GetSize() ? upper_bound : internal->KeyAt(i + 1));
      if (i == 0) {
        depth = child_depth;
      } else if (child_depth != depth) {
        throw std::runtime_error("Internal frame child depth not consistent");
      }
    }
    return depth + 1;
  }
}
template<typename KeyType, typename ValueType>
auto BPlusTree<KeyType, ValueType>::Validate() -> bool {
  auto root_page_id = GetRootId();
  if (root_page_id == INVALID_PAGE_ID) {
    return true;
  }
  try {
    ValidateBPlusTree(root_page_id, root_page_id, KeyType(), std::numeric_limits<KeyType>::max());
  } catch (std::runtime_error &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return false;
  }
  return true;
}
template<typename KeyType, typename ValueType>
void BPlusTree<KeyType, ValueType>::Print() {
  auto root_page_id = GetRootId();
  if (root_page_id == INVALID_PAGE_ID) return;
  auto guard = bpm_->FetchFrameBasic(root_page_id);
  PrintTree(guard.PageId(), guard.template As<BPlusTreeFrame>());
}
template<typename KeyType, typename ValueType>
void BPlusTree<KeyType, ValueType>::PrintTree(page_id_t page_id, const BPlusTreeFrame *page) {
  if (page_id < 1) return;
  if (page->IsLeafFrame()) {
    auto *leaf = reinterpret_cast<const LeafFrame *>(page);
    std::cout << "Leaf Page: " << page_id << "\tNext: " << leaf->GetNextPageId() << std::endl;

    // Print the contents of the leaf page.
    std::cout << "Contents: ";
    for (int i = 1; i <= leaf->GetSize(); i++) {
      std::cout << leaf->KeyAt(i);
      if ((i + 1) <= leaf->GetSize()) {
        std::cout << ", ";
      }
    }
    std::cout << std::endl;
    std::cout << std::endl;

  } else {
    auto *internal = reinterpret_cast<const InternalFrame *>(page);
    std::cout << "Internal Page: " << page_id << std::endl;

    // Print the contents of the internal page.
    std::cout << "Contents: ";
    for (int i = 0; i <= internal->GetSize(); i++) {
      if (i > 0) {
        std::cout << internal->KeyAt(i) << ": " << internal->ValueAt(i);
      } else {
        std::cout << "<null>: " << internal->ValueAt(i);
      }
      if ((i + 1) <= internal->GetSize()) {
        std::cout << ", ";
      }
    }
    std::cout << std::endl;
    std::cout << std::endl;

    for (int i = 0; i <= internal->GetSize(); i++) {
      auto guard = bpm_->FetchFrameBasic(internal->ValueAt(i));
      PrintTree(internal->ValueAt(i), guard.template As<BPlusTreeFrame>());
    }
  }
}

//template
//class storage::BPlusTree<hash_t, int>;
//
//template
//class storage::BPlusTree<std::pair<hash_t, int>, int>;
}