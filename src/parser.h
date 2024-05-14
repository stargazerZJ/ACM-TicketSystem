//
// Created by zj on 5/13/2024.
//

#pragma once

#include <fastio.h>
#include <marcos.h>
#include <sstream>
#include <string_view>
#include <string>

namespace utils {
class Args;
class Parser {
  public:
    using Command = std::string;

    /// @brief Format: `[timestamp] command -a value -b value`
    static std::pair<Command, Args> Read(const std::string &line);
};
class Args {
  friend class Parser;

  public:
    Args() = default;

    auto GetFlag(char name) const -> std::string_view {
      ASSERT(name >= 'a' && name <= 'z');
      return flags[name - 'a'];
    }
    auto GetTimestamp() const -> int { return timestamp_; }

  private:
    int timestamp_{};
    std::string flags[26];

    void SetFlag(char name, std::string &&value) {
      ASSERT(name >= 'a' && name <= 'z');
      flags[name - 'a'] = std::move(value);
    }
    void SetTimestamp(int timestamp) { timestamp_ = timestamp; }
};
} // namespace utils
