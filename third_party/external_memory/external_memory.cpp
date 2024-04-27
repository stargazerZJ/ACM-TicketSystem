//
// Created by zj on 11/29/2023.
//

#include "external_memory.h"

namespace external_memory {
void Array::initialize(bool reset) {
  if (reset) {
    file_.open(file_name_, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
  } else {
    file_.open(file_name_, std::ios::in | std::ios::out | std::ios::binary);
  }
  if (!file_.is_open()) {
    throw std::runtime_error("Cannot open file " + file_name_);
  }
  file_.seekg(0, std::ios::end);
  size_ = file_.tellg() / sizeof(int);
}
unsigned int Array::size() const {
  return size_;
}
int Array::get(unsigned int n) {
  if (cached_) {
    return cache_[n];
  } else {
    file_.seekg(n * sizeof(int), std::ios::beg);
    int value = 0;
    file_.read(reinterpret_cast<char *>(&value), sizeof(int));
    return value;
  }
}
void Array::set(unsigned int n, int value) {
  if (cached_) {
    cache_[n] = value;
  } else {
    file_.seekp(n * sizeof(int), std::ios::beg);
    file_.write(reinterpret_cast<char *>(&value), sizeof(int));
  }
}
unsigned int Array::push_back(int value) {
  if (cached_) {
    cache_.push_back(value);
    size_ = cache_.size();
    return cache_.size() - 1;
  } else {
    file_.seekp(0, std::ios::end);
    file_.write(reinterpret_cast<char *>(&value), sizeof(int));
    return size_++;
  }
}
void Array::cache() {
  if (!cached_) {
    cache_.resize(size_);
    file_.seekg(0, std::ios::beg);
    for (unsigned int i = 0; i < size_; ++i) {
      file_.read(reinterpret_cast<char *>(&cache_[i]), sizeof(int));
    }
    cached_ = true;
  }
}
void Array::flush() {
  if (cached_) {
    file_.close();
    file_.open(file_name_, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    for (unsigned int i = 0; i < size_; ++i) {
      file_.write(reinterpret_cast<char *>(&cache_[i]), sizeof(int));
    }
    cached_ = false;
  }
}
void Array::double_size() {
  if (cached_) {
    cache_.resize(size_ << 1);
    memcpy(cache_.data() + size_, cache_.data(), size_ * sizeof(int));
  } else {
    for (unsigned int i = 0; i < size_; ++i) {
      int value = 0;
      file_.seekg(i * sizeof(int), std::ios::beg);
      file_.read(reinterpret_cast<char *>(&value), sizeof(int));
      file_.seekp((i + size_) * sizeof(int), std::ios::beg);
      file_.write(reinterpret_cast<char *>(&value), sizeof(int));
    }
  }
  size_ <<= 1;
}
void Array::halve_size() {
  if (cached_) {
    cache_.resize(size_ >> 1);
  } else {
    file_.close();
    std::filesystem::resize_file(file_name_, (size_ >> 1) * sizeof(int));
    file_.open(file_name_, std::ios::in | std::ios::out | std::ios::binary);
  }
  size_ >>= 1;
}
Array::~Array() {
  if (cached_) {
    flush();
  }
  file_.close();
}

void Pages::initialize(bool reset) {
  if (reset) {
    file_.open(file_name_, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
    memset(info_, 0, sizeof(Page));
    file_.write(reinterpret_cast<char *>(info_), sizeof(Page));
    size_ = 0;
    free_head_ = 0;
  } else {
    file_.open(file_name_, std::ios::in | std::ios::out | std::ios::binary);
    file_.seekg(0, std::ios::end);
    size_ = file_.tellg() / kPageSize - 1;
    file_.seekg(0, std::ios::beg);
    file_.read(reinterpret_cast<char *>(info_), sizeof(Page));
    free_head_ = info_[0];
  }
  cache_index_ = 0;
  if (!file_.is_open()) {
    throw std::runtime_error("Cannot open file " + file_name_);
  }
}
unsigned int Pages::size() const {
  return size_;
}
void Pages::getInfo(unsigned int n, int &dest) {
  dest = info_[n];
}
void Pages::fetchPage(unsigned int n, bool reset) {
  if (n == cache_index_) {
    if (reset) {
      memset(cache_, 0, kPageSize);
    }
    return;
  }
  flush();
  if (reset) {
    memset(cache_, 0, kPageSize);
  } else {
    file_.seekg(n * kPageSize, std::ios::beg);
    file_.read(reinterpret_cast<char *>(cache_), kPageSize);
  }
  cache_index_ = n;
}
void Pages::flush() {
  if (cache_index_) {
    file_.seekp(cache_index_ * kPageSize, std::ios::beg);
    file_.write(reinterpret_cast<char *>(cache_), kPageSize);
    cache_index_ = 0;
  }
}
void Pages::getPage(unsigned int n, int *dest) {
  if (n == cache_index_) {
    memcpy(dest, cache_, kPageSize);
  } else {
    file_.seekg(n * kPageSize, std::ios::beg);
    file_.read(reinterpret_cast<char *>(dest), kPageSize);
  }
}
void Pages::setPage(unsigned int n, const int *value) {
  if (n == cache_index_) {
    memcpy(cache_, value, kPageSize);
  } else {
    file_.seekp(n * kPageSize, std::ios::beg);
    file_.write(reinterpret_cast<const char *>(value), kPageSize);
  }
}
void Pages::getPart(unsigned int n, unsigned int offset, unsigned int len, int *dest) {
  if (n == cache_index_) {
    memcpy(dest, cache_ + offset, len * sizeof(int));
  } else {
    file_.seekg(n * kPageSize + offset * sizeof(int), std::ios::beg);
    file_.read(reinterpret_cast<char *>(dest), len * sizeof(int));
  }
}
void Pages::setPart(unsigned int n, unsigned int offset, unsigned int len, const int *value) {
  if (n == cache_index_) {
    memcpy(cache_ + offset, value, len * sizeof(int));
  } else {
    file_.seekp(n * kPageSize + offset * sizeof(int), std::ios::beg);
    file_.write(reinterpret_cast<const char *>(value), len * sizeof(int));
  }
}
unsigned int Pages::toPosition(unsigned int n, unsigned int offset) {
  return n * kIntegerPerPage + offset;
}
std::pair<unsigned, unsigned> Pages::toPageOffset(unsigned int position) {
  return {position / kIntegerPerPage, position % kIntegerPerPage};
}
unsigned int Pages::newPage(const int *value) {
  if (free_head_) {
    unsigned int n = free_head_;
    getPart(n, 0, 1, &free_head_);
    fetchPage(n, true);
    if (value) {
      setPage(n, value);
    }
    return n;
  } else {
    unsigned int n = ++size_;
    fetchPage(n, true);
    if (value) {
      setPage(n, value);
    }
    return n;
  }
}
void Pages::deletePage(unsigned int n) {
  setPart(n, 0, 1, &free_head_);
  free_head_ = n;
}
Pages::~Pages() {
  flush();
  flushInfo();
  file_.close();
}
void Pages::flushInfo() {
  file_.seekp(0, std::ios::beg);
  file_.write(reinterpret_cast<char *>(info_), sizeof(Page));
}
void Pages::setInfo(unsigned int n, int value) {
  info_[n] = value;
}
int &Pages::getInfo(unsigned int n) {
  return info_[n];
}
std::string strNRead(const char *src, unsigned int n) {
  return {src, strnlen(src, n)};
}
} // namespace external_memory