//
// Created by zj on 11/29/2023.
//

#ifndef BOOKSTORE_SRC_EXTERNAL_MEMORY_H_
#define BOOKSTORE_SRC_EXTERNAL_MEMORY_H_

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cstring>

namespace external_memory {
constexpr char kFileExtension[] = ".db";
constexpr unsigned int kPageSize = 4096;
static constexpr unsigned int kIntegerPerPage = kPageSize / sizeof(int); // number of integers in a page
using Page = int[kIntegerPerPage]; // a page
template<class T>
concept ListElement = requires(T t) {
  { t.fromBytes(nullptr) };
  { T::byte_size() } -> std::convertible_to<unsigned int>;
}
    && requires(const T t) { t.toBytes(nullptr); }
    && std::is_constructible_v<T, const char *>
    && (T::byte_size() >= sizeof(unsigned int));
/**
 * @brief A class for storing a list of elements in external memory.
 *
 * @details
 * The list is stored in a file, whose name (and path) is specified in the constructor.
 *
 * `initialize` must be called before using the list.
 *
 * @tparam Elem The type of the elements.
 * @tparam recover_space Whether to recover space.
 *
 * @attention The list is 1-indexed.
 * @attention No bound checking is performed.
 */
template<ListElement Elem, bool recover_space = true>
class List {
 private:
  const std::string file_name_; // name (and path) of the file
  std::fstream file_; // the file
  static constexpr unsigned int
      byte_size_ = Elem::byte_size(); // size of each element when stored in external file, in bytes
  static constexpr unsigned int
      data_begin_ = recover_space * sizeof(unsigned int); // the beginning of the data, in bytes
  struct Bytes {
    char data[byte_size_];
    Bytes() = default;
    explicit Bytes(const Elem &elem) { elem.toBytes(data); }
  }; // a byte array
  unsigned int size_ = 0; // the current maximum index of the elements
  unsigned int free_head_ = 0; // the head of the free elements
  bool cached_ = false; // whether the whole list is cached
  std::vector<Bytes> cache_; // the cache
  void getHead(unsigned int n, unsigned int &dest); // get the first 4 bytes of the n-th element
  void setHead(unsigned int n, unsigned int value); // set the first 4 bytes of the n-th element
 public:
  /**
   * @brief Construct a new List object.
   *
   * @param file_name The name (and path) of the file.
   */
  explicit List(const std::string &file_name = "list") : file_name_(file_name + kFileExtension) {};
  /**
   * @brief Destroy the List object.
   * @details
   * The destructor flushes the cache and closes the file.
   */
  ~List();
  /**
   * @brief Initialize the list.
   *
   * @details
   * When `reset` is `true`, the file is truncated.
   * Otherwise, the file is opened.
   *
   * @param reset Whether to reset the file.
   *
   * @attention `initialize` must be called before using the list.
   * @attention `initialize` must not be called twice.
   */
  void initialize(bool reset = false);
  /**
   * @brief Get the i-th element, 1-based.
   * @param n The index of the element, 1-based.
   * @param dest The destination.
   * @attention No bound checking is performed.
   */
  void get(unsigned n, Elem &dest);
  /**
   * @brief Get the i-th element, 1-based.
   * @param n The index of the element, 1-based.
   * @return Elem The i-th element.
   * @attention No bound checking is performed.
   */
  [[nodiscard]] Elem get(unsigned n);
  /**
   * @brief Set the i-th element, 1-based.
   * @param n The index of the element, 1-based.
   * @param value The value to be set.
   * @attention No bound checking is performed.
   */
  void set(unsigned n, const Elem &value);
  /**
   * @brief Insert a new element.
   * @details If `recover_space` is `false`, the new element is appended to the end of the list.
   * @param value The value to be inserted.
   * @return unsigned int The index of the new element, 1-based.
   */
  unsigned int insert(const Elem &value);
  /**
   * @brief Erase the i-th element.
   * @details
   *  If `recover_space` is `true`, the space is recovered.
   *  Otherwise, the function does nothing.
   * @param n The index of the element, 1-based.
   * @attention No bound checking is performed.
   */
  void erase(unsigned n);
  /**
   * @brief Cache the whole list.
   *
   * @details
   * The whole list is read into the cache.
   * Subsequent operations will be performed on the cache.
   */
  void cache();
  /**
   * @brief Write the cache back to the file.
   *
   * @details
   * The whole list is written back to the file.
   * Subsequent operations will be performed on the file.
   */
  void flush();
  /**
   * @brief Get the current maximum index of the elements, 1-based. When `recover_space` is `false`, this is the size of the list.
   *
   * @return unsigned int The current maximum index of the elements, 1-based.
   */
  [[nodiscard]] unsigned int size() const { return size_; }
};
template<ListElement Elem, bool recover_space>
void List<Elem, recover_space>::flush() {
  if (cached_) {
    file_.seekp(data_begin_, std::ios::beg);
    for (unsigned int i = 0; i < size_; ++i) {
      file_.write(cache_[i].data, byte_size_);
    }
    cached_ = false;
  }
}
template<ListElement Elem, bool recover_space>
void List<Elem, recover_space>::cache() {
  if (!cached_) {
    cache_.resize(size_);
    file_.seekg(data_begin_, std::ios::beg);
    for (unsigned int i = 0; i < size_; ++i) {
      file_.read(cache_[i].data, byte_size_);
    }
    cached_ = true;
  }
}
template<ListElement Elem, bool recover_space>
void List<Elem, recover_space>::erase(unsigned int n) {
  if constexpr (recover_space) {
    setHead(n, free_head_);
    free_head_ = n;
  }
}
template<ListElement Elem, bool recover_space>
void List<Elem, recover_space>::setHead(unsigned int n, unsigned int value) {
  if (cached_) {
    *reinterpret_cast<unsigned int *>(cache_[n - 1].data) = value;
  } else {
    file_.seekp(data_begin_ + (n - 1) * byte_size_, std::ios::beg);
    file_.write(reinterpret_cast<char *>(&value), sizeof(unsigned int));
  }
}
template<ListElement Elem, bool recover_space>
void List<Elem, recover_space>::getHead(unsigned int n, unsigned int &dest) {
  if (cached_) {
    dest = *reinterpret_cast<unsigned int *>(cache_[n - 1].data);
  } else {
    file_.seekg(data_begin_ + (n - 1) * byte_size_, std::ios::beg);
    file_.read(reinterpret_cast<char *>(&dest), sizeof(unsigned int));
  }
}
template<ListElement Elem, bool recover_space>
unsigned int List<Elem, recover_space>::insert(const Elem &value) {
  if (!recover_space || free_head_ == 0) {
    if (cached_) {
      cache_.emplace_back(value);
    } else {
      file_.seekp(0, std::ios::end);
      Bytes tmp(value);
      file_.write(tmp.data, byte_size_);
    }
    return ++size_;
  } else {
    unsigned int n = free_head_;
    getHead(free_head_, free_head_);
    set(n, value);
    return n;
  }
}
template<ListElement Elem, bool recover_space>
void List<Elem, recover_space>::set(unsigned int n, const Elem &value) {
  if (cached_) {
    value.toBytes(cache_[n - 1].data);
  } else {
    Bytes tmp(value);
    file_.seekp(data_begin_ + (n - 1) * byte_size_, std::ios::beg);
    file_.write(tmp.data, byte_size_);
  }
}
template<ListElement T, bool recover_space>
T List<T, recover_space>::get(unsigned int n) {
  if (cached_) {
    return T(cache_[n - 1].data);
  } else {
    Bytes tmp;
    file_.seekg(data_begin_ + (n - 1) * byte_size_, std::ios::beg);
    file_.read(tmp.data, byte_size_);
    return T(tmp.data);
  }
}
template<ListElement T, bool recover_space>
void List<T, recover_space>::get(unsigned int n, T &dest) {
  if (cached_) {
    dest.fromBytes(cache_[n - 1].data);
  } else {
    Bytes tmp;
    file_.seekg(data_begin_ + (n - 1) * byte_size_, std::ios::beg);
    file_.read(tmp.data, byte_size_);
    dest.fromBytes(tmp.data);
  }
}
template<ListElement T, bool recover_space>
void List<T, recover_space>::initialize(bool reset) {
  if (reset) {
    file_.open(file_name_, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
  } else {
    file_.open(file_name_, std::ios::in | std::ios::out | std::ios::binary);
  }
  if (!file_.is_open()) {
    throw std::runtime_error("Cannot open file " + file_name_);
  }
  if constexpr (recover_space) {
    if (reset) {
      free_head_ = 0;
      file_.write(reinterpret_cast<char *>(&free_head_), sizeof(unsigned int));
    } else {
      file_.read(reinterpret_cast<char *>(&free_head_), sizeof(unsigned int));
    }
  }
  if (reset) size_ = 0;
  else {
    file_.seekg(0, std::ios::end);
    size_ = (static_cast<unsigned int>(file_.tellg()) - data_begin_) / byte_size_;
  }
}
template<ListElement T, bool recover_space>
List<T, recover_space>::~List() {
  if constexpr (recover_space) {
    file_.seekp(0);
    file_.write(reinterpret_cast<char *>(&free_head_), sizeof(unsigned int));
  }
  flush();
  file_.close();
}
/**
 * @brief A class for storing a list of integers in external memory.
 *
 * @details
 * The list is stored in a file, whose name (and path) is specified in the constructor.
 *
 * `initialize` must be called before using the list.
 *
 * @attention The list is 0-indexed.
 * @attention No bound checking is performed.
 */
class Array {
 private:
  unsigned int size_; // number of elements
  const std::string file_name_; // name (and path) of the file
  std::fstream file_; // the file
  bool cached_; // whether the whole list is cached
  std::vector<int> cache_; // the cache
 public:
  /**
   * @brief Construct a new Array object.
   *
   * @param name The name (and path) of the file.
   */
  explicit Array(std::string name = "array") : size_(0), file_name_(std::move(name) + kFileExtension), cached_() {}
  Array(const Array &) = delete;
  Array &operator=(const Array &) = delete;
  /**
   * @brief Destroy the Array object.
   *
   * @details
   * The destructor closes the file.
   * If the list is cached, the cache is written back to the file.
   */
  ~Array();
  /**
   * @brief Initialize the list.
   *
   * @details
   * When `reset` is `true`, the file is truncated.
   * Otherwise, the file is opened.
   *
   * @param reset Whether to reset the file.
   *
   * @attention `initialize` must be called before using the list.
   * @attention `initialize` must not be called twice.
   */
  void initialize(bool reset = false);
  /**
   * @brief Get the size of the list.
   *
   * @return unsigned int The size of the list.
   */
  unsigned int size() const;
  /**
   * @brief Get the value of the n-th element (0-based).
   *
   * @param n The index of the element, 0-based.
   * @return int The value of the n-th element (0-based).
   */
  int get(unsigned int n);
  /**
   * @brief Set the value of the n-th element (0-based).
   *
   * @param n The index of the element, 0-based.
   * @param value The value to be set.
   */
  void set(unsigned int n, int value);
  /**
   * @brief Append a new element to the list.
   *
   * @param value The value to be appended.
   * @return unsigned int The index of the new element.
   */
  unsigned int push_back(int value);
  /**
   * @brief Cache the whole list.
   *
   * @details
   * The whole list is read into the cache.
   * Subsequent operations will be performed on the cache.
   */
  void cache();
  /**
   * @brief Write the cache back to the file.
   *
   * @details
   * The whole list is written back to the file.
   * Subsequent operations will be performed on the file.
   */
  void flush();
  /**
   * @brief Append a copy of the list to the end of the file, doubling the size of the file.
   *
   */
  void double_size();
  /**
   * @brief Halve the size of the file.
   *
   */
  void halve_size();
};
/**
 * @brief A class for storing pages of integers in external memory.
 *
 * @details
 * The pages are stored in a file, whose name (and path) is specified in the constructor.
 *
 * The first page is used to store information about the pages. This page is referred to as the info page.
 *
 * `initialize` must be called before using the pages.
 *
 * @attention The pages are 1-indexed.
 * @attention The info page is 1-indexed.
 * @attention The first integer of the info page is reserved for storing the head of the free pages.
 * @attention No bound checking is performed.
 *
 * @note The info page is implicitly cached.
 */
class Pages {
 private:
  unsigned int size_; // number of pages
  const std::string file_name_; // name (and path) of the file
  std::fstream file_; // the file
  unsigned int cache_index_; // the index of the cached page
  Page cache_; // the cache
  Page info_; // the info page
  int &free_head_ = info_[0]; // the head of the free pages
 public:
  /**
   * @brief Construct a new Pages object.
   *
   * @param name The name (and path) of the file.
   */
  explicit Pages(std::string name = "pages")
      : size_(), file_name_(std::move(name) + kFileExtension), cache_index_(), cache_(), info_() {}
  Pages(const Pages &) = delete;
  Pages &operator=(const Pages &) = delete;
  /**
   * @brief Destroy the Pages object.
   *
   * @details
   * The destructor closes the file.
   * If the cache is not empty, the cache is written back to the file.
   */
  ~Pages();
  /**
   * @brief Initialize the pages.
   *
   * @details
   * When `reset` is `true`, the file is truncated.
   * Otherwise, the file is opened.
   *
   * @param reset Whether to reset the file.
   *
   * @attention `initialize` must be called before using the pages.
   * @attention `initialize` must not be called twice.
   */
  void initialize(bool reset = false);
  /**
   * @brief Flush the info page cache.
   */
  void flushInfo();
  /**
   * @brief Get the current maximum index of the pages, 1-based.
   *
   * @return unsigned int The current maximum index of the pages, 1-based.
   */
  unsigned int size() const;
  /**
   * @brief Get the i-th info.
   *
   * @param i The index of the info, 1-based.
   * @return int The i-th info.
   * @attention No bound checking is performed.
   */
  int &getInfo(unsigned int n);
  /**
   * @brief Get the i-th info.
   *
   * @param i The index of the info, 1-based.
   * @param dest The destination.
   * @attention No bound checking is performed.
   */
  void getInfo(unsigned int n, int &dest);
  /**
   * @brief Set the i-th info.
   *
   * @param i The index of the info, 1-based.
   * @param value The value to be set.
   *
   * @attention No bound checking is performed.
   */
  void setInfo(unsigned int n, int value);
  /**
   * @brief Fetch the n-th page into the cache.
   *
   * @param n The index of the page, 1-based.
   * @param reset Whether to reset page content into 0.
   *
   * @attention No bound checking is performed.
   */
  void fetchPage(unsigned int n, bool reset = false);
  /**
   * @brief Flush the page cache back to the file.
   */
  void flush();
  /**
   * @brief Get the n-th page, 1-based.
   *
   * @param n The index of the page, 1-based.
   * @param dest The destination.
   *
   * @attention No bound checking is performed.
   */
  void getPage(unsigned n, int *dest);
  /**
   * @brief Set the n-th page, 1-based.
   *
   * @param n The index of the page, 1-based.
   * @param value The value to be set.
   *
   * @attention No bound checking is performed.
   */
  void setPage(unsigned n, const int *value);
  /**
   * @brief Get a part of the n-th page, 1-based.
   *
   * @param n The index of the page, 1-based.
   * @param offset The offset of the part, 0-based, in integers.
   * @param len The length of the part, the number of integers.
   * @param dest The destination.
   *
   * @attention Must read within the n-th page.
   * @attention No bound checking is performed.
   */
  void getPart(unsigned n, unsigned offset, unsigned len, int *dest);
  /**
   * @brief Set a part of the n-th page, 1-based.
   *
   * @param n The index of the page, 1-based.
   * @param offset The offset of the part, 0-based, in integers.
   * @param len The length of the part, the number of integers.
   * @param value The value to be set.
   *
   * @attention Must write within the n-th page.
   * @attention No bound checking is performed.
   */
  void setPart(unsigned n, unsigned offset, unsigned len, const int *value);
  /**
   * @brief Convert the page number and the offset to an absolute position.
   *
   * @param n The index of the page, 1-based.
   * @param offset The offset of the integer, 0-based, in integers, must be smaller than `kIntegerPerPage`.
   * @return unsigned int The absolute position.
   */
  static unsigned int toPosition(unsigned n, unsigned offset);
  /**
   * @brief Convert the absolute position to the page number and the offset.
   *
   * @param position The absolute position.
   * @return std::pair<unsigned, unsigned> The page number and the offset.
   */
  static std::pair<unsigned, unsigned> toPageOffset(unsigned position);
  /**
   * @brief Allocate a new page.
   *
   * @param value The value to be set.
   * @return unsigned int The index of the new page, 1-based.
   *
   * @note The new page is cleared and fetched into the cache.
   */
  unsigned int newPage(const int *value = nullptr);
  /**
   * @brief Deallocate a page.
   *
   * @param n The index of the page, 1-based.
   *
   * @note The page is not actually erased.
   * @note An integer is written to the beginning of the page to indicate the next free page.
   * @note The rest of the page is not modified.
   */
  void deletePage(unsigned int n);
};
std::string strNRead(const char *src, unsigned int n);

} // namespace external_memory

#endif //BOOKSTORE_SRC_EXTERNAL_MEMORY_H_
