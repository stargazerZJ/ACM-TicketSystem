//
// Created by zj on 11/29/2023.
//

#ifndef BOOKSTORE_SRC_EXTERNAL_HASH_MAP_H_
#define BOOKSTORE_SRC_EXTERNAL_HASH_MAP_H_

#include <map>
#include "external_memory.h"
#include "external_vector.h"

namespace external_memory {
using Hash_t = unsigned long long;
/**
 * @brief A hash function that maps a string to a 64-bit unsigned integer.
 * @details The hash function is based on splitmix64.
 * @see http://xorshift.di.unimi.it/splitmix64.c
 */
class Hash {
 private:
  static Hash_t splitmix64(Hash_t x);
  /**
   * @brief The hash function for std::string.
   * @param str The string to be hashed.
   * @return The hash value.
   */
 public:
  Hash_t operator()(const std::string &str);
};
/**
 * @brief A hash map that maps a key to an unsigned integer.
 * @details The hash map is based on extendible hashing.
 * @see https://en.wikipedia.org/wiki/Extendible_hashing
 * @see https://www.geeksforgeeks.org/extendible-hashing-dynamic-approach-to-dbms/
 * @see https://dl.acm.org/doi/10.1145/320083.320092
 * @tparam Key The type of the key.
 * @attention It's not recommended to store zero in the map, because the `at` method returns 0 if the key is not in the map.
 * @attention The key must be hashable, and the uniformity of the hash function is important.
 * @attention No collision handling is implemented! To avoid collision, it's recommended to insert less than 1e6 keys.
 * @note The dictionary is cached in the memory.
 * @note The bucket containing the most recently accessed key is cached in the memory.
 */
template<class Key>
class Map {
 private:
  const std::string
      file_name_; // file_name_ + "_dict" is the file for the directory, and file_name_ + "_data" is the file for the data.
  Array dict_; // the directory
  Pages data_; // the data
  int &size_ = data_.getInfo(1); // the size of the map
  int &global_depth_ = data_.getInfo(2); // the global depth of the directory
  static constexpr unsigned int
      kMaxGlobalDepth = 23; // If the global depth is larger than this value, the program will throw an exception.
  static constexpr unsigned int
      kSizeOfPair = sizeof(Hash_t) + sizeof(unsigned int); // the size of a pair of key and value
  static constexpr unsigned int
      kPairsPerPage = (kPageSize - sizeof(unsigned short) * 2) / kSizeOfPair; // the number of pairs in a page
  static_assert(kSizeOfPair == 12, "The size of a pair of key and value is not 12!");

  inline unsigned int getBucketId(const Hash_t &key); // get the id of the bucket that may contain the key

  struct Bucket {
   private:
    void flush() const; // flush the bucket to the disk
   public:
    Map<Key> &map; // the map that the bucket belongs to
    unsigned int id = 0; // the id of the bucket, 0 means that the bucket is not initialized
    unsigned int local_depth = 0; // the local depth of the bucket
    std::map<Hash_t, unsigned int> data;
    Bucket(Map &map) : map(map) {};
    Bucket(Map &map, unsigned int id);
    Bucket(Bucket &&bucket) noexcept;
    Bucket &operator=(Bucket &&bucket) noexcept;
    ~Bucket();
    explicit Bucket(const Bucket &bucket) = delete;
    Bucket &operator=(const Bucket &bucket) = delete;
    [[nodiscard]] Hash_t getLocalHighBit(const Hash_t &key) const;
    [[nodiscard]] unsigned int size() const;
    [[nodiscard]] bool full() const; // remember that inserting to a full bucket is possible when the key is already in the bucket!
    [[nodiscard]] bool empty() const;
    [[nodiscard]] bool contains(const Hash_t &key) const;
    [[nodiscard]] unsigned int &operator[](const Hash_t &key); // using this method to insert into a full bucket is undefined!
    bool insert(const Hash_t &key, unsigned int value);
    bool erase(const Hash_t &key);
    [[nodiscard]] std::map<Hash_t, unsigned int>::const_iterator find(const Hash_t &key) const;
    [[nodiscard]] std::map<Hash_t, unsigned int>::const_iterator end() const;
    [[nodiscard]] Bucket split(); // the id of the new bucket is not set!
  };

  Bucket cache_ = {*this};

  void flush(); // flush the Bucket cache to the disk. The dictionary is not flushed.
  /**
   * @brief Get the bucket that contains the key.
   * @details
   *    Returns the reference to cache_ if the bucket is cached.
   *    Otherwise, fetch the bucket from the disk and return the reference to cache_.
   * @attention If getBucket is called with another key, the reference to cache_ will be changed.
   * @attention Other methods may change the reference to cache_.
   */
  Bucket &getBucket(const Hash_t &key);
  unsigned int splitBucket(const Hash_t &key); // return the id of the new bucket, cache_ is set to the bucket that may contain the key
  void deleteBucket(const Hash_t &key); // delete an empty bucket, cache_ is cleared
  void expand();
  void insertByHash(const Hash_t &key, unsigned int value);
  void eraseByHash(const Hash_t &key);

 public:
  /**
   * @brief Construct a new Map object.
   */
  explicit Map(std::string file_name = "map")
      : file_name_(std::move(file_name)), dict_(file_name_ + "_dict"), data_(file_name_ + "_data") {};
  /**
   * @brief Destroy the Map object.
   */
  ~Map();
  /**
   * @brief Initialize the map.
   * @param reset Whether to reset the map.
   */
  void initialize(bool reset = false);
  /**
   * @brief Insert a key-value pair into the map, if the key is not in the map.
   * @param key The key.
   * @param value The value.
   */
  void insert(const Key &key, unsigned int value);
  /**
   * @brief Get the value of the key. If the key is not in the map, insert the key with value 0.
   * @param key The key.
   * @return The reference value of the key.
   * @attention The reference may be invalid after calling `insert`, `at`, `operator []` or `erase`.
   */
  [[nodiscard]] unsigned int &operator[](const Key &key);
  /**
   * @brief Erase the key from the map.
   * @param key The key.
   */
  void erase(const Key &key);
  /**
   * @brief Get the value of the key. If the key is not in the map, return 0.
   * @param key The key.
   * @return The value of the key. If the key is not in the map, return 0.
   */
  [[nodiscard]] unsigned int at(const Key &key);
  /**
   * @brief Get the size of the map.
   * @return The size of the map.
   */
  [[nodiscard]] unsigned int size() const;
  /**
   * @brief Get the global depth of the directory.
   * @return The global depth of the directory.
   */
  [[nodiscard]] unsigned int globalDepth() const;
};
template<class Key>
unsigned int Map<Key>::globalDepth() const {
  return global_depth_;
}
template<class Key>
unsigned int Map<Key>::size() const {
  return size_;
}
template<class Key>
unsigned int Map<Key>::at(const Key &key) {
  Hash_t hash = Hash()(key);
  Bucket &bucket = getBucket(hash);
  auto it = bucket.find(hash);
  if (it == bucket.end()) {
    return 0;
  } else {
    return it->second;
  }
}
template<class Key>
void Map<Key>::erase(const Key &key) {
  Hash_t hash = Hash()(key);
  eraseByHash(hash);
}
template<class Key>
void Map<Key>::eraseByHash(const Hash_t &key) {
  Bucket &bucket = getBucket(key);
  if (bucket.erase(key)) {
    size_--;
  }
  if (bucket.empty() && bucket.local_depth > 0) {
    deleteBucket(key);
  }
}
template<class Key>
unsigned int &Map<Key>::operator[](const Key &key) {
  Hash_t hash = Hash()(key);
  insertByHash(hash, 0);
  return getBucket(hash)[hash];
}
template<class Key>
void Map<Key>::insert(const Key &key, unsigned int value) {
  Hash_t hash = Hash()(key);
  insertByHash(hash, value);
}
template<class Key>
void Map<Key>::insertByHash(const Hash_t &key, unsigned int value) {
  Bucket &bucket = getBucket(key);
  if (bucket.full() && !bucket.contains(key)) {
    if (bucket.local_depth == global_depth_) {
      expand();
    }
    splitBucket(key);
    insertByHash(key, value);
  } else {
    size_ += bucket.insert(key, value);
  }
}
template<class Key>
Map<Key>::~Map() {
  flush();
}
template<class Key>
void Map<Key>::expand() {
  if (global_depth_ == kMaxGlobalDepth) {
    throw std::runtime_error("The global depth has reached the maximum!");
  }
  dict_.double_size();
  global_depth_++;
}
template<class Key>
void Map<Key>::deleteBucket(const Hash_t &key) {
  Bucket old(std::move(getBucket(key)));
  unsigned int bucket_id = old.id;
  unsigned int local_depth = old.local_depth;
  unsigned int sibling_id = getBucketId(key ^ (1 << (local_depth - 1)));
  for (unsigned int i = (key & ((1 << local_depth) - 1)); i < (1 << global_depth_); i += (1 << local_depth)) {
    dict_.set(i, sibling_id);
  }
  data_.deletePage(bucket_id);
  old.id = 0;
  Bucket &sibling = getBucket(sibling_id);
  sibling.local_depth--;
}
template<class Key>
unsigned int Map<Key>::splitBucket(const Hash_t &key) {
  Bucket old(std::move(getBucket(key)));
  bool flag = old.getLocalHighBit(key);
  unsigned int new_id = data_.newPage();
  Bucket new_bucket = old.split();
  new_bucket.id = new_id;
  unsigned int new_local_depth = new_bucket.local_depth;
  for (unsigned int i =
      (key & ((1 << new_local_depth) - 1))
          | (1 << (new_local_depth - 1));
       i < (1 << global_depth_); i += (1 << new_local_depth)) {
    dict_.set(i, new_id);
  }
  cache_ = std::move(flag ? new_bucket : old);
  return new_id;
}
template<class Key>
Map<Key>::Bucket &Map<Key>::getBucket(const Hash_t &key) {
  unsigned int bucket_id = getBucketId(key);
  if (bucket_id == cache_.id) {
    return cache_;
  } else {
    cache_ = {*this, bucket_id};
    return cache_;
  }
}
template<class Key>
void Map<Key>::flush() {
  cache_ = {*this};
}
template<class Key>
void Map<Key>::initialize(bool reset) {
  data_.initialize(reset);
  dict_.initialize(reset);
  dict_.cache();
  if (reset) {
    unsigned int id = data_.newPage();
    Bucket bucket(*this, id);
    dict_.push_back(id);
  }
}
template<class Key>
unsigned int Map<Key>::getBucketId(const Hash_t &key) {
  return dict_.get(key & ((1 << global_depth_) - 1));
}
/**
 * @brief A multimap that maps a key to a vector of non-zero integers.
 * @details The multimap is based on extendible hashing.
 * @tparam Key The type of the key.
 * @attention Storing zero as value is undefined!
 * @attention There is way to erase a key-value pair. To erase a key-value pair, update the vector of the key. For performance, it's recommended to erase lazily after calling `findAll`.
 * @attention The key must be hashable, and the uniformity of the hash function is important.
 * @attention No collision handling is implemented! To avoid collision, it's recommended to insert less than 1e6 keys.
 * @note The vector storage (i.e. Vectors) class is shared by all classes that use it. Therefore, it's passed as a reference to the constructor.
 */
template<class Key> // the value is int
class MultiMap {
 private:
  const std::string file_name_;
  Map<Key> vector_pos_;
  Vectors &vectors_; // the vector storage
 public:
  /**
   * @brief Construct a new MultiMap object.
   */
  MultiMap(std::string file_name, Vectors &vectors) : file_name_(std::move(file_name)), vector_pos_(file_name_ + "_dict"), vectors_(vectors) {};
  /**
   * @brief Destroy the MultiMap object.
   */
  ~MultiMap() = default;
  /**
   * @brief Initialize the multimap.
   * @param reset Whether to reset the multimap.
   */
  void initialize(bool reset = false);
  /**
   * @brief Insert a key-value pair into the multimap.
   * @param key The key.
   * @param value The value.
   */
  void insert(const Key &key, int value);
  /**
   * @brief Erase the key from the multimap.
   * @param key The key.
   */
  void erase(const Key &key);
  /**
   * @brief Update the vector of the key.
   * @details If the vector is empty, the key is erased.
   * @param key The key.
   * @param values The vector.
   */
  void update(const Key &key, std::vector<int> &&values = {});
  /**
   * @brief Find all values of the key.
   * @details The result may contain duplicated values as well as deleted values (as there is no way to erase a key-value pair). It's the user's responsibility to remove them. It's recommended to call `update` to remove them in this multimap.
   * @param key The key.
   * @return The result.
   */
  std::vector<int> findAll(const Key &key);
};
template<class Key>
std::vector<int> MultiMap<Key>::findAll(const Key &key) {
  unsigned int pos = vector_pos_.at(key);
  return vectors_.getVector(pos).getData();
}
template<class Key>
void MultiMap<Key>::erase(const Key &key) {
  unsigned int pos = vector_pos_.at(key);
  auto vector = vectors_.getVector(pos);
  if (vector.del()) {
    vector_pos_.erase(key);
  }
}
template<class Key>
void MultiMap<Key>::update(const Key &key, std::vector<int> &&values) {
  unsigned int pos = vector_pos_.at(key);
  auto vector = vectors_.getVector(pos);
  if (vector.update(std::move(values))) {
    pos = vector.getPos();
    if (pos) vector_pos_[key] = pos;
    else vector_pos_.erase(key);
  }
}
template<class Key>
void MultiMap<Key>::insert(const Key &key, int value) {
  unsigned int pos = vector_pos_.at(key);
  auto vector = vectors_.getVector(pos);
  if (vector.push_back(value)) {
    pos = vector.getPos();
    vector_pos_[key] = pos;
  }
}
template<class Key>
void MultiMap<Key>::initialize(bool reset) {
  vector_pos_.initialize(reset);
}
} // namespace external_memory

#endif //BOOKSTORE_SRC_EXTERNAL_HASH_MAP_H_
