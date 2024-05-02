//
// Created by zj on 5/2/2024.
//

#pragma once

#include "config.h"
#include <string>
#include <string_view>

namespace storage {
class Hash {
 private:
  static hash_t splitmix64(hash_t x);
  /**
   * @brief The hash function for std::string.
   * @param str The string to be hashed.
   * @return The hash value.
   */
 public:
  hash_t operator()(const std::string &str);
  hash_t operator()(const std::string_view &str);
};
}