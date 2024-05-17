//
// Created by zj on 5/13/2024.
//

#include "parser.h"

#include <iomanip>

namespace utils {
std::pair<Parser::Command, Args> Parser::Read(std::string_view line) {
  Args args;
  size_t pos = 0;

  // Read timestamp
  ASSERT(line[pos] == '[');
  ++pos;
  size_t timestamp_end = line.find(']', pos);
  ASSERT(timestamp_end != std::string::npos);
  args.timestamp_ = std::stoi(std::string(line.substr(pos, timestamp_end - pos)));
  pos = timestamp_end + 1;

  // Skip whitespace
  while (pos < line.size() && std::isspace(line[pos])) {
    ++pos;
  }

  // Read command
  size_t command_end = line.find(' ', pos);
  Command command = line.substr(pos, command_end - pos);
  pos = command_end + 1;

  // Read flags and values
  while (pos < line.size()) {
    // Skip whitespace
    while (pos < line.size() && std::isspace(line[pos])) {
      ++pos;
    }

    if (pos >= line.size() || line[pos] != '-') {
      break;
    }
    ++pos;  // skip '-'

    // Read flag name
    char flag = line[pos];
    ++pos;

    // Skip whitespace
    while (pos < line.size() && std::isspace(line[pos])) {
      ++pos;
    }

    // Read flag value
    size_t value_end = line.find(' ', pos);
    if (value_end == std::string::npos) {
      value_end = line.size();
    }
    std::string_view value = line.substr(pos, value_end - pos);
    args.SetFlag(flag, value);
    pos = value_end + 1;
  }

  return std::make_pair(command, args);
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
      dayOfYear = 30 + day - 1;
      break;
    case 8:
      ASSERT(day <= 31); // August has 31 days
      dayOfYear = 61 + day - 1;
      break;
    default:
      ASSERT(false); // Invalid month
  }

  ASSERT(dayOfYear >= 0 && dayOfYear <= 91);
  return static_cast<business::date_t>(dayOfYear);
}
auto Parser::DateString(business::date_t date) -> std::string {
  ASSERT(date >= 0 && date <= 91 + 4);

  int month = 6;
  int day = date + 1;

  if (date < 30) {
    month = 6;
  } else if (date < 61) {
    month = 7;
    day = date - 30 + 1;
  } else if (date < 92) {
    month = 8;
    day = date - 61 + 1;
  } else {
    month = 9;
    day = date - 92 + 1;
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

  ASSERT(datetime >= 0 && datetime < (92 + 4) * 1440);

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
  return src_.substr(start_, (end_ == std::string_view::npos)
                               ? end_
                               : end_ - start_);
}
} // namespace utils