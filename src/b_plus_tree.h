//
// Created by zj on 4/28/2024.
//

#pragma once
#pragma clang diagnostic push
#pragma ide diagnostic ignored "Simplify"
#include "buffer_pool_manager.h"
#include "b_plus_tree_frame.h"
#include "lru_k_replacer.h"
#include "utility.h"
#include "stlite/vector.h"

namespace storage {
template<typename KeyType, typename ValueType>
class BPlusTree {
 public:
  static constexpr int PagesPerFrame = BPT_PAGES_PER_FRAME;
  using InternalFrame = BPlusTreeInternalFrame<KeyType, page_id_t, PagesPerFrame>;
  using LeafFrame = BPlusTreeLeafFrame<KeyType, ValueType, PagesPerFrame>;
  using BasicFrameGuard = BufferPoolManager<PagesPerFrame>::BasicFrameGuard;
  class PositionHint;

  explicit BPlusTree(BufferPoolManager<PagesPerFrame> *bpm, page_id_t &root_page_id, bool reset = false);

  auto Insert(const KeyType &key, const ValueType &value) -> bool;

  auto Remove(const KeyType &key) -> bool;

  auto GetValue(const KeyType &key, ValueType *value = nullptr) -> PositionHint;

  auto LowerBound(const KeyType &key) -> PositionHint;

  auto PartialSearch(const auto &key) -> std::vector<std::pair<KeyType, ValueType>>;

  auto RemoveAll(const auto &key) -> void;

  auto SetValue(const KeyType &key,
                const ValueType &value,
                const PositionHint &hint) -> bool; // insert if not found, return true if inserted

  auto SetValue(const KeyType &key, const ValueType &value) -> bool;

  auto GetRootId() const -> page_id_t;

  auto Empty() const -> bool ;

  class PositionHint {
    friend class BPlusTree;
    friend class Iterator;
   public:
    PositionHint() = default;
    PositionHint(page_id_t page_id, int index) : page_id_(page_id), index_(index) {}

    auto PageId() const -> page_id_t { return page_id_; }
    auto Index() const -> int { return index_; }
    auto found() const -> bool { return page_id_ != INVALID_PAGE_ID; }
    operator bool() const { return found(); }
    bool operator==(const PositionHint &) const = default;

   private:
    page_id_t page_id_{INVALID_PAGE_ID};
    int index_{};
  };

  class Iterator {
    friend class BPlusTree;
   public:
    Iterator() = default;
    Iterator(const BPlusTree *bpt, const PositionHint &hint) : bpt_(bpt), hint_(hint) {
      if (hint.found()) {
        frame_ = bpt_->bpm_->FetchFrameBasic(hint_.PageId());
      }
    }

    auto operator++() -> Iterator & {
      ++hint_.index_;
      if (hint_.index_ > Frame()->GetSize()) {
        if (Frame()->GetNextPageId() == INVALID_PAGE_ID) {
          hint_ = {};
          frame_.Drop();
        } else {
          hint_ = {Frame()->GetNextPageId(), 1};
          frame_ = bpt_->bpm_->FetchFrameBasic(Frame()->GetNextPageId());
        }
      }
      return *this;
    }
    auto operator*() -> std::pair<KeyType, ValueType> {
      return {Frame()->Key(hint_.Index()), Frame()->Value(hint_.Index())};
    }
    auto SetValue(const ValueType &value) -> void { FrameMut()->SetValue(hint_.Index(), value); }
    auto Key() const -> KeyType { return Frame()->KeyAt(hint_.Index()); }
    auto Value() const -> ValueType { return Frame()->ValueAt(hint_.Index()); }

    auto operator==(const Iterator &other) const -> bool { return bpt_ == other.bpt_ && hint_ == other.hint_; }
    auto operator!=(const Iterator &other) const -> bool { return !(*this == other); }
   private:
    const BPlusTree *bpt_{};
    PositionHint hint_{};
    BasicFrameGuard frame_{};
    auto Frame() const -> const LeafFrame * { return frame_.As<LeafFrame>(); }
    auto FrameMut() -> LeafFrame * { return frame_.AsMut<LeafFrame>(); }
  };

  auto End() -> Iterator { return Iterator(this, {}); }

  auto Validate() -> bool;

  void Print();

 private:
  BufferPoolManager<PagesPerFrame> *bpm_;
  page_id_t &root_page_id_;

  class Context {
   public:
    page_id_t root_page_id_{INVALID_PAGE_ID};
    sjtu::vector<PositionHint> stack_;
    BasicFrameGuard current_frame_{};
    auto IsRootPage(page_id_t page_id) const -> bool { return page_id == root_page_id_; }
  };

  auto CreateRootFrame() -> BasicFrameGuard;
  auto SetRootId(page_id_t root_id) -> void;
  auto KeyIndex(const KeyType &key, auto *frame) -> int;
  auto FindLeafFrame(const KeyType &key) -> Context;
  static void MoveData(auto *array, size_t begin, size_t end, int offset); // [begin, end)
  auto InsertInLeaf(const KeyType &key, const ValueType &value, Context &context) -> void;
  auto InsertInInternal(const KeyType &key, page_id_t new_page_id, Context &context) -> void;
  auto InsertInParent(page_id_t old_page_id,
                      const KeyType &key,
                      page_id_t new_page_id,
                      Context &context) -> void;
  template<typename FrameType>
  auto RemoveInFrame(Context &context) -> void; // simple remove
  // \return pair<page_id_t, bool> page_id_t is the page_id of the sibling, bool is true if the sibling is right sibling
  auto FindSibling(Context &context, const InternalFrame *parent_frame);
  auto RemoveInLeaf(Context &context) -> void;
  auto RemoveInInternal(Context &context) -> void;

  auto ValidateBPlusTree(page_id_t root_page_id,
                         page_id_t page_id,
                         const KeyType &lower_bound,
                         const KeyType &upper_bound) -> int; // return depth of the tree, throw std::runtime_error if invalid

  void PrintTree(page_id_t page_id, const BPlusTreeFrame *page);
};

} // namespace storage

#include "b_plus_tree.cpp"
#pragma clang diagnostic pop