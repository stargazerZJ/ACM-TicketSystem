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

class Parser {
  public:
  using Command = std::string;

  class Args {
    friend class Parser;
    public:
    Args() = default;

    auto GetFlag(char name) -> const std::string_view {
      ASSERT(name >= 'a' && name <= 'z');
      return flags[name - 'a'];
    }
    auto GetTimestamp() -> int { return timestamp_; }
    private:
    int timestamp_{};
    std::string flags[26];

    void SetFlag(char name, std::string &&value) {
      ASSERT(name >= 'a' && name <= 'z');
      flags[name - 'a'] = std::move(value);
    }
    void SetTimestamp(int timestamp) { timestamp_ = timestamp; }
  };
  /// @brief Format: `[timestamp] command -a value -b value`
  static std::pair<Command, Args> Read(const std::string &line) {
    Args args;
    std::stringstream ss(line);
    std::string token;

    // Extract timestamp
    if (std::getline(ss, token, ' ') && token.front() == '[' && token.back() == ']') {
      token = token.substr(1, token.size() - 2);
      args.SetTimestamp(std::stoi(token));
    } else {
      throw std::invalid_argument("Invalid input format: missing or malformed timestamp");
    }

    // Extract command
    Command command;
    if (!(ss >> command)) {
      throw std::invalid_argument("Invalid input format: missing command");
    }

    // Extract flags and values
    while (ss >> token) {
      if (token.front() == '-' && token.size() == 2 && isalpha(token[1]) && islower(token[1])) {
        char flag = token[1];
        std::string flagValue;
        if (!(ss >> flagValue)) {
          throw std::invalid_argument("Invalid input format: missing flag value");
        }
        args.SetFlag(flag, std::move(flagValue));
      } else {
        throw std::invalid_argument("Invalid input format: malformed flag");
      }
    }

    return std::make_pair(std::move(command), std::move(args));
  }
};

} // namespace utils