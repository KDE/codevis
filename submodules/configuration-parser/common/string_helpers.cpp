/*
    SPDX-FileCopyrightText: 2021 Tomaz Canabrava <tcanabrava@kde.org>
    SPDX-License-Identifier: MIT
*/

#include "string_helpers.h"
#include <QLoggingCategory>

std::string camel_case_to_underscore(const std::string &s) {
  std::string ret;
  for (size_t i = 0, end = s.size(); i < end; i++) {
    char x = s[i];
    if (x >= 'A' && x <= 'Z') {
      if (i != 0) {
        ret += '_';
      }
      ret += (char)(x | 32);
    } else {
      ret += x;
    }
  }
  return ret;
}

std::string underscore_to_camel_case(const std::string &s) {
  std::string ret;
  for (size_t i = 0, end = s.size() - 1; i < end; ++i) {
    if (s[i] == '_') {
      ret += s[i + 1] ^ 32;
      continue;
    }
    ret += s[i];
  }
  return ret;
}

std::string capitalize(const std::string &s, int pos) {
  std::string ret = s;
  if (pos == -1) {
      std::transform(ret.begin(), ret.end(),ret.begin(), ::toupper);
      return ret;
  }

  ret[pos] ^= 32;
  return ret;
}

std::string decapitalize(const std::string &s, int pos) {
  std::string ret = s;
  ret[pos] |= 32;
  return ret;
}

void clear_empty(std::ifstream &f) {
  while (f.peek() == ' ' || f.peek() == '\n') {
    f.ignore();
  }
}

QDebug &operator<<(QDebug &debug, const std::string &s) {
  debug << s.c_str();
  return debug;
}

QDebug &operator<<(QDebug &debug, const std::vector<std::string> &vector) {
  for (const std::string &str : vector) {
    debug << "\n" << str;
  }
  return debug;
}

std::string read_untill_delimiters(std::ifstream &f,
                                   const std::vector<char> &delimiters) {
  std::string ret_string;
  while (std::find(delimiters.begin(), delimiters.end(), f.peek()) ==
         delimiters.end()) {
    ret_string += f.get();
  }
  return ret_string;
}

void begin_header_guards(std::ofstream &f, const std::string &filename) {
  f << "#pragma once" << std::endl;
}
