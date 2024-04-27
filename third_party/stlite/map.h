#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-exception-baseclass"
//
// Created by zj on 3/26/2024.
//

#pragma once

#include <functional>
#include <cstddef>
//#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {
template<class T>
class default_compare {
 public:
  [[nodiscard]] auto operator()(const T &lhs, const T &rhs) const {
    if constexpr (std::three_way_comparable<T>) return lhs <=> rhs;
    else return lhs < rhs;
  }
};

template<class T, class Compare>
class three_way_compare_wrapper {
 public:
  [[nodiscard]] auto operator()(const T &lhs, const T &rhs) const {
    // Check if comp's return type is bool
    if constexpr (std::is_same_v<decltype(comp(lhs, rhs)), bool>) {
      // Perform the comparison to determine weak ordering
      if (comp(lhs, rhs)) { // lhs < rhs
        return std::weak_ordering::less;
      } else if (comp(rhs, lhs)) { // rhs < lhs
        return std::weak_ordering::greater;
      } else { // equal
        return std::weak_ordering::equivalent;
      }
    } else {
      // If comp's return type is not bool, return cmp(lhs, rhs) directly
      return comp(lhs, rhs);
    }
  }
 private:
  [[no_unique_address]] Compare comp;
};

template<class Key, class T, class Compare = default_compare<Key>>
class map {
 private:
  enum class Color : unsigned int;
  struct Node;
  struct Data_node;
  template<bool is_const>
  class iterator_base;
  template<bool is_const>
  struct Lower_Bound_Result_Base;
  using Lower_Bound_Result = Lower_Bound_Result_Base<false>;
  using Const_Lower_Bound_Result = Lower_Bound_Result_Base<true>;
 public:
  using iterator = iterator_base<false>;
  using const_iterator = iterator_base<true>;
  using value_type = std::pair<const Key, T>;

  map() = default;
  map(const map &other) : comp(other.comp) {
    copy_tree(&head, root(), other.root());
    size_ = other.size_;
  }
  map(map &&other) noexcept: comp(std::move(other.comp)) {
    root() = other.root();
    size_ = other.size_;
    other.root() = nullptr;
    other.size_ = 0;
  }
  map &operator=(const map &other) {
    if (this == &other) [[unlikely]] return *this;
    this->~map();
    new(this) map(other);
    return *this;
  }
  map &operator=(map &&other) noexcept {
    if (this == &other) [[unlikely]] return *this;
    this->~map();
    new(this) map(std::move(other));
    return *this;
  }
  ~map() { clear_tree(root()); }
  void clear() {
    clear_tree(root());
    root() = nullptr;
    size_ = 0;
  }
  T &at(const Key &key) {
    auto pos = lower_bound_(key);
    if (!pos.found()) [[unlikely]] throw index_out_of_bound();
    return pos.node()->value();
  }
  const T &at(const Key &key) const {
    auto pos = lower_bound_(key);
    if (!pos.found()) [[unlikely]] throw index_out_of_bound();
    return pos.node()->value();
  }
  T &operator[](const Key &key) {
    Lower_Bound_Result pos = lower_bound_(key);
    if (!pos.found()) {
      Node *node = new Data_node({}, {key, T()});
      insert(pos, node);
      ++size_;
      return node->value();
    }
    return pos.node()->value();
  }
  const T &operator[](const Key &key) const { return at(key); }
  iterator begin() {
    Node *pos = &head;
    while (pos->left() != nullptr) pos = pos->left();
    return {&head, pos};
  }
  const_iterator begin() const { return cbegin(); }
  const_iterator cbegin() const {
    const Node *pos = &head;
    while (pos->left() != nullptr) pos = pos->left();
    return {&head, pos};
  }
  iterator end() { return {&head, &head}; }
  const_iterator end() const { return cend(); }
  const_iterator cend() const { return {&head, &head}; }
  [[nodiscard]] bool empty() const { return root() == nullptr; }
  [[nodiscard]] size_t size() const { return size_; }
  std::pair<iterator, bool> insert(const value_type &value) {
    Lower_Bound_Result pos = lower_bound_(value.first);
    if (pos.found()) return {{&head, pos.node()}, false};
    Node *node = new Data_node({}, value);
    insert(pos, node);
    ++size_;
    return {{&head, node}, true};
  }
  void erase(iterator pos) {
    if (pos.is_header() || pos.src != &head) throw index_out_of_bound();
    erase({pos.ptr->parent(), pos.ptr->direction()});
    --size_;
    delete static_cast<Data_node *>(pos.ptr);
  }
  size_t count(const Key &key) const {
    auto pos = lower_bound_(key);
    return pos.found();
  }
  iterator find(const Key &key) {
    Lower_Bound_Result pos = lower_bound_(key);
    if (pos.found()) return {&head, pos.node()};
    else return end();
  }
  const_iterator find(const Key &key) const {
    auto pos = lower_bound_(key);
    if (pos.found()) return {&head, pos.node()};
    else return end();
  }
 private:
  size_t size_ = 0;
  Node head = {};
  [[no_unique_address]] three_way_compare_wrapper<Key, Compare> comp = {};
  Node *&root() { return head.left(); }
  Node *root() const { return head.left(); }
  enum class Color : unsigned int {
    RED, BLACK
  };
  using Direction = bool; // left -> false, right -> true
  static Direction opposite(Direction dir) { return !dir; }
  struct Node {
    Node *pt[3] = {};
    Color color = Color::RED;
    Node *&left() { return pt[0]; }
    Node *left() const { return pt[0]; }

    Node *&right() { return pt[1]; }
    Node *right() const { return pt[1]; }

    Node *&child(Direction dir) { return pt[dir]; }
    Node *child(Direction dir) const { return pt[dir]; }

    Node *&parent() { return pt[2]; }
    Node *parent() const { return pt[2]; }

    Node *grandparent() { return parent()->parent(); }

    Node *uncle() { return grandparent()->child(opposite(parent()->direction())); }

    [[nodiscard]] Direction direction() const { return parent()->right() == this; }
    [[nodiscard]] bool is_root() const { return parent()->parent() == nullptr; }
    [[nodiscard]] bool is_black() const { return color == Color::BLACK; }
    static bool is_black(const Node *node) { return node == nullptr || node->is_black(); }
    [[nodiscard]] bool is_red() const { return color == Color::RED; }
    const Key &key() const { return static_cast<const Data_node *>(this)->data.first; }
    T &value() { return static_cast<Data_node *>(this)->data.second; }
    const T &value() const { return static_cast<const Data_node *>(this)->data.second; }
    value_type &data() { return static_cast<Data_node *>(this)->data; }
    const value_type &data() const { return static_cast<const Data_node *>(this)->data; }

    void rotate(Direction dir) {
      //     |                       |
      //     N      l-rotate(N)      S
      //    / \     ---------->     / \
      //   L   S    <----------    N   R
      //      / \   r-rotate(S)   / \
      //     M   R               L   M
      Node *successor = child(opposite(dir));
      child(opposite(dir)) = successor->child(dir);
      successor->child(dir) = this;
      parent()->child(direction()) = successor;

      successor->parent() = parent();
      parent() = successor;
      if (child(opposite(dir))) [[likely]] child(opposite(dir))->parent() = this;
    }
  };
  struct Data_node : Node {
    value_type data = {};
  };
  template<bool is_const>
  struct Lower_Bound_Result_Base {
    using pointer = std::conditional_t<is_const, const Node *, Node *>;
    pointer parent;
    Direction dir;
    std::conditional_t<is_const, const Node *, Node *&> node() { return parent->child(dir); }
//    const Node *node() const { return parent->child(dir); }
    bool found() { return node() != nullptr; }
  };
  Lower_Bound_Result lower_bound_(const Key &key) {
    Node *pre = &head;
    Direction dir = false; // left
    Node *cur;
    while ((cur = pre->child(dir)) != nullptr) {
      auto cmp = comp(cur->key(), key);
      if (cmp < 0) {
        pre = cur;
        dir = true; // right
      } else if (cmp > 0) {
        pre = cur;
        dir = false; // left
      } else { // key == cur->key()
        return {pre, dir};
      }
    }
    return {pre, dir};
  }
  Const_Lower_Bound_Result lower_bound_(const Key &key) const {
    const Node *pre = &head;
    Direction dir = false; // left
    const Node *cur;
    while ((cur = pre->child(dir)) != nullptr) {
      auto cmp = comp(cur->key(), key);
      if (cmp < 0) {
        pre = cur;
        dir = true; // right
      } else if (cmp > 0) {
        pre = cur;
        dir = false; // left
      } else { // key == cur->key()
        return {pre, dir};
      }
    }
    return {pre, dir};
  }
  static void insert(Lower_Bound_Result result, Node *node) {
    result.node() = node;
    node->parent() = result.parent;

    maintain_after_insert(node);
  }
  static void maintain_after_insert(Node *node) {
    if (node->is_root() || node->parent()->is_root()) [[unlikely]] {
      // Case 1: Current node is root, which must be RED
      //  No need to fix.
      // Case 2: Parent is root and is BLACK
      //  No need to fix.
      // Case 3: Parent is root and is RED
      //   Paint parent to BLACK.
      //    <P>         [P]
      //     |   ====>   |
      //    <N>         <N>
      //   p.s.
      //    `<X>` is a RED node;
      //    `[X]` is a BLACK node (or NIL);
      //    `{X}` is either a RED node or a BLACK node;
      node->parent()->color = Color::BLACK;
      return;
    }
    Node *parent = node->parent();
    if (parent->is_black()) return;
    Node *uncle = node->uncle();
    Node *grandparent = parent->parent();
    if (uncle && uncle->is_red()) {
      // Case 4: Both parent and uncle are RED, grandparent must be BLACK
      //   Paint parent and uncle to BLACK;
      //   Paint grandparent to RED.
      //        [G]             <G>
      //        / \             / \
      //      <P> <U>  ====>  [P] [U]
      //       |               |
      //      <N>             <N>
      grandparent->color = Color::RED;
      parent->color = Color::BLACK;
      uncle->color = Color::BLACK;
      return maintain_after_insert(grandparent);
    }
    // Case 5 & 6: Parent is RED and uncle is BLACK, grandparent must be BLACK
    //   p.s. NIL nodes are also considered BLACK
    if (node->direction() != parent->direction()) {
      // Case 5: Current node is the opposite direction as parent
      //   Step 1. If node is a LEFT child, perform l-rotate to parent;
      //           If node is a RIGHT child, perform r-rotate to parent.
      //   Step 2. N = P, goto Case 6.
      //      [G]                 [G]
      //      / \    rotate(P)    / \
      //    <P> [U]  ========>  <N> [U]
      //      \                 /
      //      <N>             <P>

      // Step 1: Rotation
      parent->rotate(opposite(node->direction()));
      // Step 2: vvv
      std::swap(node, parent);
    }
    // Case 6: Current node is the same direction as parent
    //   Step 1. If node is a LEFT child, perform r-rotate to grandparent;
    //           If node is a RIGHT child, perform l-rotate to grandparent.
    //   Step 2. Paint parent (before rotate) to BLACK;
    //           Paint grandparent (before rotate) to RED.
    //        [G]                 <P>               [P]
    //        / \    rotate(G)    / \    repaint    / \
    //      <P> [U]  ========>  <N> [G]  ======>  <N> <G>
    //      /                         \                 \
    //    <N>                         [U]               [U]

    // Step 1
    grandparent->rotate(opposite(node->direction()));
    // Step 2
    parent->color = Color::BLACK;
    grandparent->color = Color::RED;
  }
  static void erase(Lower_Bound_Result result) {
    Node *node = result.node();
    if (node->is_root() && node->left() == nullptr && node->right() == nullptr) [[unlikely]] {
      // Case 0: Current node is the only node
      result.node() = nullptr;
      return;
    }
    if (node->left() && node->right()) {
      // Case 1: Current node is strictly internal
      //   Step 1. Find the successor S with the smallest key
      //           and its parent P on the right subtree.
      //   Step 2. Swap S and N,
      //           S is the node that will be deleted in place of N.
      //   Step 3. goto Case 2, 3
      Node *successor = node->right();
      if (successor->left() == nullptr) {
        //     |                    |
        //     N     swap(N, S)     S
        //    / \    =========>    / \
        //   L   S                L   N
        node->left()->parent() = successor;
        if (successor->right()) successor->right()->parent() = node;
        result.node() = successor;
        std::swap(node->left(), successor->left());
        node->right() = successor->right();
        successor->right() = node;
        successor->parent() = node->parent();
        node->parent() = successor;
        std::swap(node->color, successor->color);
      } else {
        do
          successor = successor->left();
        while (successor->left() != nullptr);
        //     |                    |
        //     N                    S
        //    / \                  / \
        //   L  ..   swap(N, S)   L  ..
        //       |   =========>       |
        //       P                    P
        //      / \                  / \
        //     S  ..                N  ..
        node->left()->parent() = successor;
        node->right()->parent() = successor;
        if (successor->right()) successor->right()->parent() = node;
        successor->parent()->left() = node;
        result.node() = successor;
        std::swap(node->left(), successor->left());
        std::swap(node->right(), successor->right());
        std::swap(node->parent(), successor->parent());
        std::swap(node->color, successor->color);
      }
    }
    if (node->left() == nullptr && node->right() == nullptr) {
      // Case 2: Current node is a leaf
      //   Step 1. Unlink and remove it.
      //   Step 2. If N is BLACK, maintain N;
      //           If N is RED, do nothing.

      // The maintain operation won't change the node itself,
      //  so we can perform maintain operation before unlink the node.
      if (node->is_black()) {
        maintain_after_remove(node);
      }
      node->parent()->child(node->direction()) = nullptr;
    } else {
      // Case 3: Current node has a single left or right child, which must be RED
      //   Step 1. Replace N with its child S
      //   Step 2. Paint S to BLACK
      Node *child = (node->left() == nullptr ? node->right() : node->left());
      node->parent()->child(node->direction()) = child;
      child->parent() = node->parent();
      child->color = Color::BLACK;
    }
  }
  static void maintain_after_remove(Node *node) {
    // node : A BLACK node to be painted to RED (or deleted)
    // Case 0 : Current node is root
    if (node->is_root()) return;
    Direction dir = node->direction();
    Node *parent = node->parent(); // must exist
    Node *sibling = parent->child(opposite(dir)); // must exist
    if (sibling->is_red()) {
      // Case 1: Sibling is RED, parent and nephews must be BLACK
      //   Step 1. If N is a left child, left rotate P;
      //           If N is a right child, right rotate P.
      //   Step 2. Paint S to BLACK, P to RED
      //   Step 3. Goto Case 2, 3, 4, 5
      //      [P]                   <S>               [S]
      //      / \    l-rotate(P)    / \    repaint    / \
      //    [N] <S>  ==========>  [P] [D]  ======>  <P> [D]
      //        / \               / \               / \
      //      [C] [D]           [N] [C]           [N] [C]

      // Step 1
      parent->rotate(dir);
      // Step 2
      sibling->color = Color::BLACK;
      parent->color = Color::RED;
      // Step 3: vvv
      sibling = parent->child(opposite(dir)); // must exist
    }
    Node *distant_nephew = sibling->child(opposite(dir));
    if (Node::is_black(distant_nephew)) {
      // Case 2~4: Distant nephew is NIL or BLACK
      Node *close_nephew = sibling->child(dir); // may not exist
      if (Node::is_black(close_nephew)) {
        // Case 2,3: Close nephew is NIL or BLACK
        if (parent->is_red()) {
          // Case 2: Sibling and nephews are BLACK, parent is RED
          //   Swap the color of P and S
          //      <P>             [P]
          //      / \             / \
          //    [N] [S]  ====>  [N] <S>
          //        / \             / \
          //      [C] [D]         [C] [D]
          std::swap(parent->color, sibling->color);
          return;
        } else {
          // Case 3: Sibling, parent and nephews are all black
          //   Step 1. Paint S to RED
          //   Step 2. Recursively maintain P
          //      [P]             [P]
          //      / \             / \
          //    [N] [S]  ====>  [N] <S>
          //        / \             / \
          //      [C] [D]         [C] [D]
          sibling->color = Color::RED;
          return maintain_after_remove(parent);
        }
      } else { // C exists and is RED
        // Case 4: Sibling is BLACK, close nephew is RED,
        //         distant nephew is BLACK
        //   Step 1. If N is a left child, right rotate S;
        //           If N is a right child, left rotate S.
        //   Step 2. Swap the color of close nephew and sibling
        //   Step 3. Goto case 5
        //                            {P}                {P}
        //      {P}                   / \                / \
        //      / \    r-rotate(S)  [N] <C>   repaint  [N] [C]
        //    [N] [S]  ==========>        \   ======>        \
        //        / \                     [S]                <S>
        //      <C> [D]                     \                  \
        //                                  [D]                [D]

        // Step 1
        sibling->rotate(opposite(dir));
        // Step 2
        sibling->color = Color::RED;
        close_nephew->color = Color::BLACK;
        // Step 3: vvv
        distant_nephew = sibling;
        sibling = close_nephew;
      }
    } // D exists and is RED
    // Case 5: Sibling is BLACK, distant nephew is RED
    //   Step 1. If N is a left child, left rotate P;
    //           If N is a right child, right rotate P.
    //   Step 2. Swap the color of parent and sibling
    //   Step 3. Paint distant nephew to BLACK
    //      {P}                 [S]                {S}
    //      / \    rotate(P)    / \     repaint    / \
    //    [N] [S]  ========>  {P} <D>   ======>  [P] [D]
    //        / \             / \                / \
    //      {C} <D>         [N] {C}            [N] {C}
    parent->rotate(dir);
    std::swap(parent->color, sibling->color);
    distant_nephew->color = Color::BLACK;
  }
  template<bool is_const>
  class iterator_base {
    friend class map;
   private:
    using U = std::conditional_t<is_const, const value_type, value_type>;
    using pointer = std::conditional_t<is_const, const Node *, Node *>;
   private:
    pointer src = nullptr;
    pointer ptr = nullptr;
    iterator_base(pointer src, pointer ptr) : src(src), ptr(ptr) {}
   private:
    [[nodiscard]] bool is_header() const { return src == ptr; }
   public:
    iterator_base() = default;
    iterator_base(const iterator_base<false> &other) // NOLINT(*-explicit-constructor)
        : src(other.src), ptr(other.ptr) {}
    iterator_base &operator++() {
      if (is_header()) [[unlikely]] throw invalid_iterator();
      if (ptr->right()) {
        ptr = ptr->right();
        while (ptr->left()) ptr = ptr->left();
      } else {
        while (ptr->direction()) {
          ptr = ptr->parent();
        }
        ptr = ptr->parent();
      }
      return *this;
    }
    iterator_base &operator--() {
      if (ptr == nullptr) [[unlikely]] throw invalid_iterator();
      if (ptr->left()) {
        ptr = ptr->left();
        while (ptr->right()) ptr = ptr->right();
      } else {
        while (ptr->parent() != nullptr && !ptr->direction()) {
          ptr = ptr->parent();
        }
        if (is_header()) [[unlikely]] throw invalid_iterator();
        ptr = ptr->parent();
      }
      return *this;
    }
    iterator_base operator++(int) {
      iterator_base ret = *(this);
      ++*(this);
      return ret;
    }
    iterator_base operator--(int) {
      iterator_base ret = *(this);
      --*(this);
      return ret;
    }
    U &operator*() { if (is_header()) [[unlikely]] throw invalid_iterator(); else return ptr->data(); }
    U *operator->() { if (is_header()) [[unlikely]] throw invalid_iterator(); else return &(ptr->data()); }
    template<bool is_const_>
    bool operator==(const iterator_base<is_const_> &rhs) const { return ptr == rhs.ptr; }
    template<bool is_const_>
    bool operator!=(const iterator_base<is_const_> &rhs) const { return ptr != rhs.ptr; }
  };
  static void copy_tree(Node *px, Node *&x, const Node *y) {
    if (y == nullptr) {
      x = nullptr;
      return;
    }
    x = new Data_node(*static_cast<const Data_node *>(y));
    x->parent() = px;
    copy_tree(x, x->left(), y->left());
    copy_tree(x, x->right(), y->right());
  }
  static void clear_tree(Node *x) {
    if (x == nullptr) return;
    clear_tree(x->left());
    clear_tree(x->right());
    delete static_cast<Data_node *>(x);
  }
  /**
   * Check the correctness of the red-black tree. For debugging only.
   */
  void full_check() const {
    return;
    if (head.parent() != nullptr) throw invalid_iterator();
    if (root() == nullptr) return;
    full_check(root());
  }
  size_t full_check(Node *x) const {
    if (x == nullptr) return 1;
    size_t l = full_check(x->left());
    size_t r = full_check(x->right());
    if (l != r) throw invalid_iterator();
    if (x->is_red() && (x->left() && x->left()->is_red() || x->right() && x->right()->is_red()))
      throw invalid_iterator();
    if (x->left() && x->left()->parent() != x) throw invalid_iterator();
    if (x->right() && x->right()->parent() != x) throw invalid_iterator();
    return l + x->is_black();
  }
};
template<typename T1, typename T2>
using pair = std::pair<T1, T2>;

} // namespace sjtu
#pragma clang diagnostic pop