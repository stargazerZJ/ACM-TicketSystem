//
// Created by zj on 5/3/2024.
//

#pragma once

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

} // namespace storage