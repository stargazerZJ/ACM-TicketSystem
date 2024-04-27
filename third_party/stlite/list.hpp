#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-dcl21-cpp"
#pragma once
#ifndef SJTU_LIST_HPP
#define SJTU_LIST_HPP


#include <memory>
#include <cstddef>

namespace sjtu {

template <typename T> class list;

namespace impl {
class nodeBase {
  template <typename T> friend class sjtu::list;
 protected:
  nodeBase *prev = this;
  nodeBase *next = this;

  nodeBase *insert(nodeBase *pos) {
    prev = pos->prev;
    next = pos;
    pos->prev->next = this;
    pos->prev = this;
    return this;
  }

  void erase() {
    prev->next = next;
    next->prev = prev;
    next = prev = this;
  }
};
}

/**
 * @tparam T Type of the elements.
 * Be careful that T may not be default constructable.
 *
 * @brief A list that supports operations like std::list.
 *
 * We encourage you to design the implementation yourself.
 * As for the memory management, you may use std::allocator,
 * new/delete, malloc/free or something else.
*/
template <typename T>
class list : protected sjtu::impl::nodeBase {
 private:
  size_t size_ = 0;
 protected:
  class node;
 public:
  class iterator;
  class const_iterator;

 public:

  /**
   * Constructs & Assignments
   * At least three: default constructor, copy constructor/assignment
   * Bonus: move/initializer_list constructor/assignment
   */
  list() = default;
  list(const list &other) {
    for (nodeBase *p = other.next; p != &other; p = p->next) push_back(static_cast<node*>(p)->data);
  }
  list(list &&other)  noexcept {
    size_ = other.size_;
    prev = other.prev;
    next = other.next;
    other.prev = other.next = &other;
  }
  list &operator=(const list &other) {
    if (this == &other) return *this;
    clear();
    new(this) list(other);
    return *this;
  }
  list &operator=(list &&other)  noexcept {
    if (this == &other) return *this;
    clear();
    new(this) list(std::move(other));
    return *this;
  }
  list(std::initializer_list<T> init) {
    for (const auto &t : init) push_back(t);
  }

  /* Destructor. */
  ~list() { clear(); }

  /* Access the first / last element. */
  T &front() noexcept { return static_cast<node*>(next)->get_data(); }
  T &back()  noexcept { return static_cast<node*>(prev)->get_data(); }
  const T &front() const noexcept { return static_cast<node*>(next)->get_data(); }
  const T &back()  const noexcept { return static_cast<node*>(prev)->get_data(); }

  /* Return an iterator pointing to the first element. */
  iterator begin() noexcept { return iterator(next); }
  const_iterator begin() const noexcept { return const_iterator(next); }
  const_iterator cbegin() const noexcept { return const_iterator(next); }

  /* Return an iterator pointing to one past the last element. */
  iterator end() noexcept { return iterator(this); }
  const_iterator end() const noexcept { return const_iterator(this); }
  const_iterator cend() const noexcept { return const_iterator(this); }

  /* Checks whether the container is empty. */
  [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
  /* Return count of elements in the container. */
  [[nodiscard]] size_t size() const noexcept { return size_; }

  /* Clear the contents. */
  void clear() noexcept {
    for (nodeBase *p = next; p != this; ) {
      nodeBase *q =p->next;
      delete static_cast<node*>(p);
      p = q;
    }
    prev = next = this;
    size_ = 0;
  }

  /**
   * @brief Insert value before pos (pos may be the end() iterator).
   * @return An iterator pointing to the inserted value.
   */
  iterator insert(iterator pos, const T &value) { ++size_; return iterator((new node(value))->insert(pos.data)); }
  iterator insert(iterator pos, T &&value) { ++size_; return iterator((new node(std::move(value)))->insert(pos.data)); }

  /**
   * @brief Remove the element at pos (remove end() iterator is invalid).
   * returns an iterator pointing to the following element, if pos pointing to
   * the last element, end() will be returned.
   */
  iterator erase(iterator pos) noexcept {
    node* x = static_cast<node*>(pos.data);
    nodeBase *p = x->next;
    x->erase();
    delete x;
    -- size_;
    return iterator(p);
  }

  /* Add an element to the front/back. */
  void push_front(const T &value) { (new node(value))->insert(next); ++size_; }
  void push_back (const T &value) { (new node(value))->insert(this); ++size_; }
  void push_front(T &&value) { (new node(std::move(value)))->insert(next); ++size_; }
  void push_back (T &&value) { (new node(std::move(value)))->insert(this); ++size_; }

  /* Removes the first/last element. */
  void pop_front() noexcept { node* x = static_cast<node*>(next); x->erase(); delete x; -- size_; }
  void pop_back () noexcept { node* x = static_cast<node*>(prev); x->erase(); delete x; -- size_; }

 protected:
  class node : protected nodeBase {
    friend class list;
   private:
    T data;
   public:
    explicit node(const T&t) : data(t) {}
    explicit node(T &&t) : data(std::move(t)) {}

    T& get_data() { return data; }
    const T& get_data() const { return data; }
  };
 public:

  /**
   * Basic requirements:
   * operator ++, --, *, ->
   * operator ==, != between iterators and const iterators
   * constructing a const iterator from an iterator
   *
   * If your implementation meets these requirements,
   * you may not comply with the following template.
   * You may even move this template outside the class body,
   * as long as your code works well.
   *
   * Contact TA whenever you are not sure.
   */
  class iterator {
    friend class list;
    friend class const_iterator;
   private:
    nodeBase *data = nullptr;
    iterator(nodeBase *data) : data(data) {}

   public:
    iterator() = default;
    iterator &operator++()  { data = data->next; return *this; }
    iterator &operator--()  { data = data->prev; return *this; }
    iterator operator++(int) { auto ret = *this; ++ *this; return ret; }
    iterator operator--(int) { auto ret = *this; -- *this; return ret; }

    T &operator*()  const noexcept { return static_cast<node*>(data)->data; }
    T *operator->() const noexcept { return &(static_cast<node*>(data)->data); }

    /* An operator to check whether two iterators are same (pointing to the same memory) */
    friend bool operator == (const iterator &, const iterator &) = default;
  };

  /**
   * Const iterator should have same functions as iterator, just for a const object.
   * It should be able to construct from an iterator.
   * It should be able to compare with an iterator.
   */
  class const_iterator {
    friend class list;
   private:
    const nodeBase *data;
    explicit const_iterator(const nodeBase *data) : data(data) {}
    const_iterator(const iterator &other) : data(other.data) {}
   public:
    const_iterator() = default;
    const_iterator &operator++()  { data = data->next; return *this; }
    const_iterator &operator--()  { data = data->prev; return *this; }
    const_iterator operator++(int) { auto ret = *this; ++ *this; return ret; }
    const_iterator operator--(int) { auto ret = *this; -- *this; return ret; }

    const T &operator*()  const noexcept { return static_cast<const node*>(data)->data; }
    const T *operator->() const noexcept { return &(static_cast<const node*>(data)->data); }

    friend bool operator == (const const_iterator &, const const_iterator &) = default;
  };
};

} // namespace sjtu

#endif // SJTU_LIST_HPP
#pragma clang diagnostic pop