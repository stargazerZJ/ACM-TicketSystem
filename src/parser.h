//
// Created by zj on 5/13/2024.
//

#pragma once

#include "fastio.h"
#include "utility.h"
#include "marcos.h"
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

    /// @brief Format: "mm-dd" -> 0 ~ 92 (0: 06-01, 92: 08-31)
    static auto ParseDate(std::string_view date_string) -> business::date_t;
    /// @brief Format: 0 ~ 92 -> "mm-dd" (0: 06-01, 92: 08-31)
    static auto DateString(business::date_t date) -> std::string;
    /// @brief Format: "hi:mi" -> 0 ~ 1439 (0: 00:00, 1439: 23:59)
    static auto ParseTime(std::string_view time_string) -> business::date_t;
    /// @brief Format: 0 ~ 1439 -> "hi:mi" (0: 00:00, 1439: 23:59)
    static auto TimeString(business::date_t time) -> std::string;
    // static std::string DateTimeString(business::date_t datetime); // TODO: define this function

    class DelimitedStrIterator {
      public:
        explicit DelimitedStrIterator(std::string_view src)
          : src_(src), start_(0), end_(src.find(delimiter)) {
        }

        DelimitedStrIterator &operator++();

        std::string_view operator*() const;

      private:
        static constexpr char delimiter = '|';
        std::string_view src_;
        size_t start_;
        size_t end_;
    };
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
