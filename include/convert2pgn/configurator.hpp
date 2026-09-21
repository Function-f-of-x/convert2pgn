// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Function f(x)
#pragma once

#include <array>
#include <cstdint>
#include <print>
#include <stdexcept>
#include <string>
#include <vector>

namespace convert2pgn {

namespace StringTrim {

inline const std::string WHITESPACE = " \t\n\r\f\v";

inline std::string& rtrim(std::string& str, const std::string& t = WHITESPACE) {
  str.erase(str.find_last_not_of(t) + 1);
  return str;
}

inline std::string& ltrim(std::string& str, const std::string& t = WHITESPACE) {
  str.erase(0, str.find_first_not_of(t));
  return str;
}

inline std::string& trim(std::string& str, const std::string& t = WHITESPACE) {
  return ltrim(rtrim(str, t), t);
}

inline std::string ltrim_copy(std::string str) {
  ltrim(str);
  return str;
}

inline std::string rtrim_copy(std::string str) {
  rtrim(str);
  return str;
}

inline std::string trim_copy(std::string str) {
  trim(str);
  return str;
}

}  // namespace StringTrim

struct Pixel {
  uint8_t r, g, b, a;
};

struct ProgramArgs {
  std::vector<std::string> input_files;
  std::string output_file;
  std::string game_file;
  std::vector<int64_t> include_frames;
  std::vector<int64_t> exclude_frames;
  std::vector<std::pair<int64_t, int64_t>> map_frames;
  bool has_input_files = false, has_output_file = false, has_game_file = false, has_include_frames = false, has_exclude_frames = false, has_map_frames = false;
  std::string code;
};

using RawVideo = std::vector<std::array<std::array<Pixel, 8>, 8>>;

template <typename... Args>
void print_debug(std::format_string<Args...> str, Args&&... args) {
#ifndef NDEBUG
  std::println(str, std::forward<Args>(args)...);
#endif
}

class ProgramError : public std::runtime_error {
 public:
  explicit ProgramError(const std::string& error_type) : std::runtime_error(error_type) {}
};

}  // namespace convert2pgn