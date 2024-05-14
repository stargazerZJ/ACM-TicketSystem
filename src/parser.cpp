//
// Created by zj on 5/13/2024.
//

#include "parser.h"
std::pair<utils::Parser::Command, utils::Args> utils::Parser::Read(const std::string &line) {
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
utils::Parser::DelimitedStrIterator& utils::Parser::DelimitedStrIterator::
operator++() {
  if (end_ == std::string_view::npos) {
    start_ = std::string_view::npos;
  } else {
    start_ = end_ + 1;
    end_ = src_.find(delimiter, start_);
  }
  return *this;
}
std::string_view utils::Parser::DelimitedStrIterator::operator*() const {
  if (start_ == std::string_view::npos) {
    return {};
  }
  return src_.substr(start_, (end_ == std::string_view::npos) ? end_ : end_ - start_);
}
