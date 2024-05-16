//
// Created by zj on 5/13/2024.
//

#include "parser.h"

#include <iomanip>

namespace utils {
std::pair<Parser::Command, Args> Parser::Read(const std::string &line) {
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
auto Parser::ParseDate(std::string_view date_string) -> business::date_t {
  int month = utils::stoi(date_string.substr(0, 2));
  int day = utils::stoi(date_string.substr(3, 2));

  // Validate month and day range
  ASSERT(month >= 6 && month <= 8);
  ASSERT(day >= 1 && day <= 31);

  int dayOfYear = 0;
  switch (month) {
    case 6:
      ASSERT(day <= 30); // June has 30 days
      dayOfYear = day - 1;
      break;
    case 7:
      ASSERT(day <= 31); // July has 31 days
      dayOfYear = 30 + day;
      break;
    case 8:
      ASSERT(day <= 31); // August has 31 days
      dayOfYear = 61 + day;
      break;
    default:
      ASSERT(false); // Invalid month
  }

  ASSERT(dayOfYear >= 0 && dayOfYear <= 91);
  return static_cast<business::date_t>(dayOfYear);
}
auto Parser::DateString(business::date_t date) -> std::string {
  ASSERT(date >= 0 && date <= 91);

  int month = 6;
  int day = date + 1;

  if (date < 30) {
    month = 6;
  } else if (date < 61) {
    month = 7;
    day = date - 30 + 1;
  } else {
    month = 8;
    day = date - 61 + 1;
  }

  std::ostringstream oss;
  oss << std::setw(2) << std::setfill('0') << month << "-"
      << std::setw(2) << std::setfill('0') << day;
  return oss.str();
}
auto Parser::ParseTime(std::string_view time_string) -> business::time_t {
  int hour = utils::stoi(time_string.substr(0, 2));
  int minute = utils::stoi(time_string.substr(3, 2));

  // Validate hour and minute range
  ASSERT(hour >= 0 && hour <= 23);
  ASSERT(minute >= 0 && minute <= 59);

  return static_cast<business::time_t>(hour * 60 + minute);
}
auto Parser::TimeString(business::time_t time) -> std::string {
  ASSERT(time >= 0 && time <= 1439);

  int hour = time / 60;
  int minute = time % 60;

  std::ostringstream oss;
  oss << std::setw(2) << std::setfill('0') << hour << ":"
      << std::setw(2) << std::setfill('0') << minute;
  return oss.str();
}
std::string Parser::DateTimeString(business::abs_time_t datetime) {
  if (datetime == -1) {
    return "xx-xx xx:xx";
  }

  ASSERT(datetime >= 0 && datetime < 92 * 1440);

  business::date_t date = datetime / 1440;
  business::time_t time = datetime % 1440;

  std::ostringstream oss;
  oss << DateString(date) << " " << TimeString(time);
  return oss.str();
}
Parser::DelimitedStrIterator& Parser::DelimitedStrIterator::
operator++() {
  if (end_ == std::string_view::npos) {
    start_ = std::string_view::npos;
  } else {
    start_ = end_ + 1;
    end_ = src_.find(delimiter, start_);
  }
  return *this;
}
std::string_view Parser::DelimitedStrIterator::operator*() const {
  if (start_ == std::string_view::npos) {
    return {};
  }
  return src_.substr(start_, (end_ == std::string_view::npos) ? end_ : end_ - start_);
}
} // namespace utils