//
// Created by zj on 11/29/2023.
//

#include <iostream>
#include "external_hash_map.h"
namespace external_memory {
Hash_t Hash::splitmix64(Hash_t x) {
  // Reference: http://xorshift.di.unimi.it/splitmix64.c
  x += 0x9e3779b97f4a7c15;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
  x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
  return x ^ (x >> 31);
}
Hash_t Hash::operator()(const std::string &str) {
  /// compute the hash every 8 characters to increase the speed
  return std::hash<std::string>()(str);
  Hash_t hash = 0;
  unsigned int i = 0;
  for (; i + 8 <= str.size(); i += 8) {
    hash = splitmix64(hash ^ *reinterpret_cast<const Hash_t *>(str.data() + i));
  }
  for (; i < str.size(); ++i) {
    hash = splitmix64(hash ^ str[i]);
  }
  return hash;
}
template<class Key>
Hash_t Map<Key>::Bucket::getLocalHighBit(const Hash_t &key) const {
  return (key >> local_depth) & 1;
}
template<class Key>
Map<Key>::Bucket::Bucket(Map &map, unsigned int id) : map(map), id(id) {
  if (!id) {
    local_depth = 0;
    return;
  }
  Page tmp;
  map.data_.getPage(id, tmp);
  local_depth = tmp[0] >> 16;
  unsigned int size = tmp[0] & ((1 << 16) - 1);
  for (unsigned int i = 0; i < size; ++i) {
    Hash_t key = *reinterpret_cast<Hash_t *>(tmp + i * 3 + 1);
    int value = tmp[i * 3 + 3];
    data.insert(data.end(), {key, value});
  }
}
template<class Key>
void Map<Key>::Bucket::flush() const {
  if (!id) return;
  Page tmp;
  memset(tmp, 0, sizeof(tmp));
  tmp[0] = (local_depth << 16) | data.size();
  unsigned int i = 0;
  for (auto &pair : data) {
    *reinterpret_cast<Hash_t *>(tmp + i * 3 + 1) = pair.first;
    tmp[i * 3 + 3] = static_cast<int>(pair.second);
    ++i;
  }
  map.data_.setPage(id, tmp);
}
template<class Key>
Map<Key>::Bucket::Bucket(Map<Key>::Bucket &&bucket) noexcept
    : map(bucket.map), id(bucket.id), local_depth(bucket.local_depth), data(std::move(bucket.data)) {
  bucket.id = 0;
}
template<class Key>
typename Map<Key>::Bucket &Map<Key>::Bucket::operator=(Map::Bucket &&bucket) noexcept {
  if (this == &bucket) {
    return *this;
  }
  flush();
  id = bucket.id;
  local_depth = bucket.local_depth;
  data = std::move(bucket.data);
  bucket.id = 0;
  return *this;
}
template<class Key>
Map<Key>::Bucket::~Bucket() {
  flush();
}
template<class Key>
unsigned int Map<Key>::Bucket::size() const {
  return data.size();
}
template<class Key>
bool Map<Key>::Bucket::full() const {
  return data.size() >= kPairsPerPage;
}
template<class Key>
bool Map<Key>::Bucket::contains(const Hash_t &key) const {
  return data.contains(key);
}
template<class Key>
bool Map<Key>::Bucket::insert(const Hash_t &key, unsigned int value) {
  return data.insert({key, value}).second;
}
template<class Key>
bool Map<Key>::Bucket::erase(const Hash_t &key) {
  size_t size_before = data.size();
  data.erase(key);
  return data.size() < size_before;
}
template<class Key>
std::map<Hash_t, unsigned int>::const_iterator Map<Key>::Bucket::find(const Hash_t &key) const {
  return data.find(key);
}
template<class Key>
std::map<Hash_t, unsigned int>::const_iterator Map<Key>::Bucket::end() const {
  return data.end();
}
template<class Key>
Map<Key>::Bucket Map<Key>::Bucket::split() {
  Bucket ret(map);
  for (auto it = data.begin(); it != data.end();) {
    if (getLocalHighBit(it->first)) {
      ret.data.insert(ret.data.end(), *it);
      it = data.erase(it);
    } else {
      ++it;
    }
  }
  ret.local_depth = ++local_depth;
  return ret;
}
template<class Key>
bool Map<Key>::Bucket::empty() const {
  return data.empty();
}
template<class Key>
unsigned int &Map<Key>::Bucket::operator[](const Hash_t &key) {
  return data[key];
}
template
class Map<std::string>;
template
class MultiMap<std::string>;
} // namespace external_memory
