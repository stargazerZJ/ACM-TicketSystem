//
// Created by zj on 5/3/2024.
//

#pragma once

#include <bit>
#include <iterator>

namespace storage {
#pragma pack(push, 1)
template<typename T1, typename T2>
struct PackedPair {
  using first_type = T1;
  using second_type = T2;
  T1 first;
  T2 second;
  bool operator ==(const PackedPair &) const = default;
  auto operator <=>(const PackedPair &) const = default;
};
#pragma pack(pop)

template<class ForwardIt, class T>
ForwardIt upper_bound(ForwardIt first, ForwardIt last, const T &value) {
  // if (first == last) return last;
  unsigned int count = std::distance(first, last) + 1;
  ForwardIt it = first - 1;

  unsigned int mid = 0;
  for (unsigned int b = std::bit_floor(count); b; b >>= 1) {
    if (mid + b < count && !(value < *(it + mid + b))) {
      mid |= b;
    }
  }

  std::advance(it, mid + 1);
  // auto std = std::upper_bound(first, last, value);
  // if (std != it) {
  //   throw "!";
  // }
  return it;
}

template<class ForwardIt, class T>
ForwardIt lower_bound(ForwardIt first, ForwardIt last, const T &value) {
  unsigned int count = std::distance(first, last) + 1;
  ForwardIt it = first - 1;

  unsigned int mid = 0;
  for (unsigned int b = std::bit_floor(count); b; b >>= 1) {
    if (mid + b < count && (*(it + mid + b) < value)) {
      mid |= b;
    }
  }

  std::advance(it, mid + 1);
  return it;
}

template<class It, class Comp>
void merge(It f1, It l1, It f2, It l2, It o, Comp comp = std::less<decltype(*f1)>()) {
  for (; f1 != l1; ++f1) {
    while (f2 != l2 && comp(*f2, *f1)) *(o++) = *(f2++);
    *(o++) = std::move(*f1);
  }
  while (f2 != l2) *(o++) = *(f2++);
}

template<class It, class Comp>
void inplace_merge(It l, It mid, It r, Comp comp = std::less<decltype(*l)>()) {
  auto *temp = new std::remove_reference_t<decltype(*l)>[r - l];
  merge(l, mid, mid, r, temp, comp);
  // memcpy(l, temp, sizeof(It) * (r - l));
  std::move(temp, temp + (r - l), l);
  delete[] temp;
}

template<class It, class Comp = std::less<decltype(*std::declval<It>())> >
void sort(It l, It r, Comp comp = std::less<decltype(*l)>()) {
  if (r - l <= 1) return;
  auto mid = l + (r - l) / 2;
  storage::sort(l, mid, comp);
  storage::sort(mid, r, comp);
  storage::inplace_merge(l, mid, r, comp);
}
} // namespace storage

namespace utils {
inline void set_field(char *dest, std::string_view src, size_t size) {
  size_t len = std::min(src.size(), size);
  std::copy_n(src.begin(), len, dest);
  std::fill(dest + len, dest + size, '\0');
}
inline bool cmp_field(const char *dest, std::string_view src, size_t size) {
  size_t len = std::min(src.size(), size);
  return std::equal(src.begin(), src.begin() + len, dest);
}
inline std::string_view get_field(const char *src, size_t size) {
  // terminate at '\0' or size
  return std::string_view(src, std::find(src, src + size, '\0') - src); // NOLINT(*-return-braced-init-list)
}

inline int stoi(std::string_view str) {
  int ret = 0;
  for (char c : str) {
    ASSERT(isdigit(c));
    ret = ret * 10 + c - '0';
  }
  return ret;
}
} // namespace utils
