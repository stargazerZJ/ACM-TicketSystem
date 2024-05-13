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
  bool operator == (const PackedPair &) const = default;
  auto operator <=> (const PackedPair &) const = default;
};
#pragma pack(pop)

template <class ForwardIt, class T>
ForwardIt upper_bound(ForwardIt first, ForwardIt last, const T& value) {
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

} // namespace storage

namespace utils {
inline void set_field(char *dest, std::string_view src, size_t size) {
  size_t len = std::min(src.size(), size);
  std::copy_n(src.begin(), len, dest);
  std::fill(dest + len, dest + size, '\0');
}

} // namespace utils