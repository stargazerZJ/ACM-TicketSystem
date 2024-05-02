//
// Created by zj on 5/2/2024.
//

#pragma once

#include <cctype>
#include <cstdio>
#include <concepts>

namespace fastio {

class FastIO {
 public:
  static auto read(auto &&...a) { return ((READ(a)), ...); }
  static auto write(auto &&...a) { return ((WRITE(a)), ...); }
 private:
  static void READ(std::integral auto &a) {
    int fl = 1;
    char c;
    a = 0;
    while (!std::isdigit(c = getchar())) if (c == '-') fl = -1;
    do a = a * 10 + (c & 15); while (std::isdigit(c = getchar()));
    a *= fl;
  }
  static int READ(char *s) {
    int l = 0;
    char c;
    while (!std::isprint(c = getchar()));
    do s[l++] = c; while (std::isprint(c = getchar()));
    return l;
  }
  static void READ(std::string &s) {
    int l = 0;
    char c;
    while (!std::isprint(c = getchar()));
    do s += c; while (std::isprint(c = getchar()));
  }
  static void WRITE(std::integral auto a) {
    if (a < 0) putchar('-'), a = -a;
    while (a > 9) putchar(a % 10 | '0'), a /= 10;
    putchar(a | '0');
  }
  static void WRITE(const char *s) {
    while (*s) putchar(*s++);
  }
  static void WRITE(const std::string &s) {
    for (char c : s) putchar(c);
  }
  static void WRITE(const std::string_view &s) {
    for (char c : s) putchar(c);
  }
};

}