//
// Created by zj on 2/26/2024.
//

#pragma once

#include "exceptions.hpp"

namespace sjtu {

template<class T, class Allocator = std::allocator<T> >
class vector {
 private:
  template<bool is_const>
  class iterator_base;
 public:
  using value_type = T;
  using allocator_type = Allocator;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = typename std::allocator_traits<Allocator>::pointer;
  using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
  using iterator = iterator_base<false>;
  using const_iterator = iterator_base<true>;
  pointer data = nullptr;
 private:
  template<bool is_const>
  class iterator_base {
    friend class vector;
   private:
    using U = std::conditional_t<is_const, const T, T>;
   public:
    using difference_type = std::ptrdiff_t;
    using value_type = U;
    using pointer = U *;
    using reference = U &;
    using iterator_category = std::output_iterator_tag;
   private:
    pointer src = nullptr;
    pointer ptr = nullptr;
    iterator_base(pointer src, pointer ptr) : src(src), ptr(ptr) {}
   public:
    iterator_base() = default;
    iterator_base(const iterator_base<false> &other) : src(other.src), ptr(other.ptr) {}
    iterator_base operator+(const difference_type &n) const { return iterator_base(src, ptr + n); }
    friend iterator_base operator+(const difference_type &n, const iterator_base &it) { return it + n; }
    iterator_base operator-(const difference_type &n) const { return iterator_base(src, ptr - n); }
    template<bool is_const_>
    difference_type operator-(const iterator_base<is_const_> &rhs) const {
      if (src != rhs.src) throw invalid_iterator();
      return ptr - rhs.ptr; }
    iterator_base &operator+=(const difference_type &n) {
      ptr += n;
      return *this;
    }
    iterator_base &operator-=(const difference_type &n) {
      ptr -= n;
      return *this;
    }
    iterator_base &operator++() {
      ++ptr;
      return *this;
    }
    iterator_base operator++(int) { return iterator_base(src, ptr++); }
    iterator_base &operator--() {
      --ptr;
      return *this;
    }
    iterator_base operator--(int) { return iterator_base(src, ptr--); }
    reference operator*() const { return *ptr; }
    pointer operator->() const { return ptr; }
    reference operator[](const difference_type &n) const { return ptr[n]; }
    template<bool is_const_>
    bool operator==(const iterator_base<is_const_> &rhs) const { return ptr == rhs.ptr; }
    template<bool is_const_>
    bool operator!=(const iterator_base<is_const_> &rhs) const { return ptr != rhs.ptr; }
    template<bool is_const_>
    auto operator<=>(const iterator_base<is_const_> &rhs) const { return ptr <=> rhs.ptr; }
  };
 private:
  size_type length = 0;
  size_type capacity = 0;
  [[no_unique_address]] allocator_type alloc = Allocator();
  void expand() {
    if (length == capacity) {
      capacity = capacity == 0 ? 1 : capacity << 1;
      pointer new_data = alloc.allocate(capacity);
      for (size_type i = 0; i < length; ++i) {
        std::allocator_traits<Allocator>::construct(alloc, new_data + i, std::move(data[i]));
        std::allocator_traits<Allocator>::destroy(alloc, data + i);
      }
      alloc.deallocate(data, length);
      data = new_data;
    }
  }
 public:
  vector() = default;
  explicit vector(size_type count) {
    data = alloc.allocate(count);
    for (size_type i = 0; i < count; ++i) {
      std::allocator_traits<Allocator>::construct(alloc, data + i);
    }
    length = count;
    capacity = count;
  }
  vector(const vector &other) : alloc(other.alloc) {
    if (other.empty()) return;
    data = alloc.allocate(other.size());
    for (size_type i = 0; i < other.size(); ++i) {
      std::allocator_traits<Allocator>::construct(alloc, data + i, other[i]);
    }
    length = other.size();
    capacity = other.size();
  }
  vector(vector &&other) noexcept
      : data(other.data), length(other.length), capacity(other.capacity), alloc(other.alloc) {
    other.data = nullptr;
    other.length = 0;
    other.capacity = 0;
  }
  ~vector() {
    for (size_type i = 0; i < length; ++i) {
      std::allocator_traits<Allocator>::destroy(alloc, data + i);
    }
    alloc.deallocate(data, length);
  }
  vector &operator=(const vector &other) {
    if (this == &other) return *this;
    this->~vector();
    new(this) vector(other);
    return *this;
  }
  vector &operator=(vector &&other) noexcept {
    if (this == &other) return *this;
    this->~vector();
    new(this) vector(std::move(other));
    return *this;
  }
  T &at(const size_type &i) {
    if (i >= length) throw index_out_of_bound();
    return *(data + i);
  }
  const T &at(const size_type &i) const {
    if (i >= length) throw index_out_of_bound();
    return *(data + i);
  }
  T &operator[](const size_type &i) { return at(i); }
  const T &operator[](const size_type &i) const { return at(i); }
  T &front() {
    if (empty()) throw container_is_empty();
    return *data;
  }
  const T &front() const {
    if (empty()) throw container_is_empty();
    return *data;
  }
  T &back() {
    if (empty()) throw container_is_empty();
    return *(data + length - 1);
  }
  const T &back() const {
    if (empty()) throw container_is_empty();
    return *(data + length - 1);
  }
  iterator begin() { return {data, data}; }
  const_iterator begin() const { return {data, data}; }
  const_iterator cbegin() const { return {data, data}; }
  iterator end() { return {data, data + length}; }
  const_iterator end() const { return {data, data + length}; }
  const_iterator cend() const { return {data, data + length}; }
  [[nodiscard]] bool empty() const { return length == 0; }
  [[nodiscard]] size_type size() const { return length; }
  void clear() {
    for (size_type i = 0; i < length; ++i) {
      std::allocator_traits<Allocator>::destroy(alloc, data + i);
    }
    length = 0;
  }
  template <class Arg>
  requires std::is_convertible_v<Arg, T>
  iterator insert(const size_type &ind, Arg &&value) {
    if (ind > length) throw index_out_of_bound();
    expand();
    for (size_type i = length; i > ind; --i) {
      std::allocator_traits<Allocator>::construct(alloc, data + i, std::move(data[i-1]));
      std::allocator_traits<Allocator>::destroy(alloc, data + i - 1);
    }
    std::allocator_traits<Allocator>::construct(alloc, data + ind, std::forward<Arg>(value));
    ++length;
    return iterator(data, data + ind);
  }
  template <class Arg>
  requires std::is_convertible_v<Arg, T>
  iterator insert(const_iterator pos, Arg &&value) {
    size_type ind = pos.ptr - data;
    return insert(ind, std::forward<Arg>(value));
  }
  iterator erase(const size_type &ind) {
    if (ind >= length) throw index_out_of_bound();
    std::allocator_traits<Allocator>::destroy(alloc, data + ind);
    for (size_type i = ind + 1; i < length; ++ i) {
      std::allocator_traits<Allocator>::construct(alloc, data + i - 1, std::move(data[i]));
      std::allocator_traits<Allocator>::destroy(alloc, data + i);
    }
    -- length;
    return iterator(data, data + ind);
  }
  iterator erase(const_iterator pos) {
    size_type ind = pos.ptr - data;
    return erase(ind);
  }
  template <class Arg>
  requires std::is_convertible_v<Arg, T>
  void push_back(Arg &&value) {
    expand();
    std::allocator_traits<Allocator>::construct(alloc, data + length, std::forward<Arg>(value));
    ++ length;
  }
  reference emplace_back(auto&&...args) {
    expand();
    std::allocator_traits<Allocator>::construct(alloc, data + length, std::forward<decltype(args)>(args)...);
    ++ length;
    return data[length - 1];
  }
  void pop_back() {
    if (empty()) throw container_is_empty();
    std::allocator_traits<Allocator>::destroy(alloc, data + length - 1);
    -- length;
  }
  void shrink_to_fit() {
    if (length == capacity) return;
    capacity = length;
    pointer new_data = alloc.allocate(capacity);
    for (size_type i = 0; i < length; ++i) {
      std::allocator_traits<Allocator>::construct(alloc, new_data + i, std::move(data[i]));
      std::allocator_traits<Allocator>::destroy(alloc, data + i);
    }
    alloc.deallocate(data, length);
    data = new_data;
  }
  iterator get_iter(pointer p) {
    return iterator(data, p);
  }
};

}
