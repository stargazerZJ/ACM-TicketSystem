//
// Created by zj on 3/11/2024.
//

#pragma once

#include <cstddef>
#include <functional>
#include "exceptions.hpp"

namespace sjtu {

template<class T, class Compare = std::less<T>>
class priority_queue {
 public:
  priority_queue() = default;
  priority_queue(const priority_queue &other) : size_(other.size_), cmp(other.cmp) {
    top_ = copy_heap(other.top_);
  }
  priority_queue(priority_queue &&other) noexcept: size_(other.size_), cmp(std::move(other.cmp)), top_(other.top_) {
    other.size_ = 0;
    other.top_ = nullptr;
  }
  ~priority_queue() { delete_heap(top_); }
  priority_queue &operator=(const priority_queue &other) {
    if (this == &other) return *this;
    this->~priority_queue();
    new(this) priority_queue(other);
    return *this;
  }
  priority_queue &operator=(priority_queue &&other) noexcept {
    if (this == &other) return *this;
    this->~priority_queue();
    new(this) priority_queue(std::move(other));
    return *this;
  }
  const T &top() const {
    if (empty()) throw container_is_empty();
    return top_->data;
  }
  void push(const T &e) {
    if (empty()) {
      top_ = new node(nullptr, nullptr, e);
      ++ size_;
      return ;
    }
    if (cmp(e, top())) top_->child = new node(nullptr, top_->child, e);
    else top_ = new node(top_, nullptr, e);
    ++ size_;
  }
  void pop() {
    if (empty()) throw container_is_empty();
    node *t = merges(top_->child);
    delete top_;
    top_ = t;
    -- size_;
  }
  [[nodiscard]] size_t size() const { return size_; }
  [[nodiscard]] bool empty() const { return top_ == nullptr; }
  void merge(priority_queue &other) {
    top_ = meld(top_, other.top_);
    size_ += other.size_;
    other.size_ = 0;
    other.top_ = nullptr;
  }
 private:
  struct node;
  size_t size_ = 0;
  node *top_ = nullptr;
  [[no_unique_address]] Compare cmp = {};
  struct node {
    node *child = nullptr;
    node *sibling = nullptr;
    T data;
    node(node *child, node *sibling, auto &&...args) : child(child), sibling(sibling), data(std::forward<decltype(args)>(args)...) {}
  };
  static node *copy_heap(const node *other) {
    if (other == nullptr) return nullptr;
    return new node(copy_heap(other->child), copy_heap(other->sibling), other->data);
  }
  static void delete_heap(const node *ptr) {
    if (ptr == nullptr) return ;
    delete_heap(ptr->child);
    delete_heap(ptr->sibling);
    delete ptr;
  }
  node *meld(node *x, node *y, node *sibling = nullptr) {
    if (x == nullptr) std::swap(x, y);
    if (y == nullptr) { x->sibling = sibling; return x; }
    if (cmp(x->data, y->data)) std::swap(x, y);
    y->sibling = x->child;
    x->child = y;
    x->sibling = sibling;
    return x;
  }
  node *merges(node *x) {
    if (x == nullptr || x->sibling == nullptr) return x;
    node *y = x->sibling;
    node *c = y->sibling;
    node *d = meld(x, y, c);
    return meld(merges(c), d);
  }
};

}