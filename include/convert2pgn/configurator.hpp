// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Function f(x)
#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <format>
#include <print>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace convert2pgn {

namespace StringTrim {

constexpr std::string_view WHITESPACE = " \t\n\r\f\v";

inline std::string& rtrim(std::string& str, const std::string_view& t = WHITESPACE) {
  str.erase(str.find_last_not_of(t) + 1);
  return str;
}

inline std::string& ltrim(std::string& str, const std::string_view& t = WHITESPACE) {
  str.erase(0, str.find_first_not_of(t));
  return str;
}

inline std::string& trim(std::string& str, const std::string_view& t = WHITESPACE) {
  return ltrim(rtrim(str, t), t);
}

inline std::string ltrim_copy(std::string str, const std::string_view& t = WHITESPACE) {
  ltrim(str, t);
  return str;
}

inline std::string rtrim_copy(std::string str, const std::string_view& t = WHITESPACE) {
  rtrim(str, t);
  return str;
}

inline std::string trim_copy(std::string str, const std::string_view& t = WHITESPACE) {
  trim(str, t);
  return str;
}

}  // namespace StringTrim

struct Pixel {
  uint8_t r, g, b, a;
};

struct ProgramArgs {
  std::string input_file;
  bool is_input_image_sequence = false;
  std::string output_file;
  std::string game_file;
  std::vector<int64_t> include_frames;
  std::vector<int64_t> exclude_frames;
  std::unordered_map<int64_t, int64_t> map_frames;
  bool has_input_file = false, has_output_file = false, has_game_file = false, has_include_frames = false, has_exclude_frames = false, has_map_frames = false;
};

using RawVideo = std::vector<std::array<std::array<Pixel, 8>, 8>>;

template <typename... Args>
void print_debug([[maybe_unused]] std::format_string<Args...> str, [[maybe_unused]] Args&&... args) {
#ifndef NDEBUG
  std::println(str, std::forward<Args>(args)...);
#endif
}

class ProgramError : public std::runtime_error {
 public:
  explicit ProgramError(const std::string& error_type) : std::runtime_error(error_type) {}
};

namespace LogLevel {
  enum Type : uint8_t {
    FATAL = 1,
    ERROR = 2,
    WARN = 3,
    INFO = 4,
    DEBUG = 5,
    TRACE = 6,
  };
}

class Logger {
 public:
  Logger() = default;
  Logger(const LogLevel::Type level, const bool verbose = false) : max_log_level_(level), verbose_(verbose) {}

  void set_caller(const std::string_view caller) {
    caller_ = caller;
  }

  void set_max_log_level(const LogLevel::Type level) {
    max_log_level_ = level;
  }

  Logger with_caller(const std::string_view caller) {
    Logger logger(max_log_level_, verbose_);
    logger.set_caller(caller);
    return logger;
  }

  template <typename... Args>
  void log(std::string_view caller, LogLevel::Type level, std::format_string<Args...> str, Args&&... args) {
    if (level > max_log_level_) {
      return;
    }

    std::string formatted;

    if (verbose_) {
      auto utc = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
      auto now = std::chrono::zoned_time(std::chrono::current_zone(), utc);
      formatted += std::format("{}[{:%T}] [{}/{}]: ", log_level_to_ansi(level), now, caller, log_level_to_string(level));
    } else if (level <= LogLevel::WARN) {
      formatted += std::format("{}[{}/{}]: ", log_level_to_ansi(level), caller, log_level_to_string(level));
    }

    formatted += std::format(str, std::forward<Args>(args)...);

    if (!log_level_to_ansi(level).empty()) {
      formatted += "\x1b[0m";
    }

    std::print(formatted);
  }

  template <typename... Args>
  void log(LogLevel::Type level, std::format_string<Args...> str, Args&&... args) {
    this->log(caller_, level, str, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void trace(std::format_string<Args...> str, Args&&... args) {
    this->log(caller_, LogLevel::TRACE, str, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void debug(std::format_string<Args...> str, Args&&... args) {
    this->log(caller_, LogLevel::DEBUG, str, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void info(std::format_string<Args...> str, Args&&... args) {
    this->log(caller_, LogLevel::INFO, str, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void warn(std::format_string<Args...> str, Args&&... args) {
    this->log(caller_, LogLevel::WARN, str, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void error(std::format_string<Args...> str, Args&&... args) {
    this->log(caller_, LogLevel::ERROR, str, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void fatal(std::format_string<Args...> str, Args&&... args) {
    this->log(caller_, LogLevel::FATAL, str, std::forward<Args>(args)...);
  }

 private:
  std::string caller_ = "Main";
  bool verbose_ = false;
  LogLevel::Type max_log_level_ = LogLevel::INFO;

  constexpr std::string_view log_level_to_ansi(LogLevel::Type level) {
    switch (level) {
      case LogLevel::TRACE: {
        return "\x1b[35m";
      }
      case LogLevel::DEBUG: {
        return "\x1b[36m";
      }
      case LogLevel::INFO: {
        return "";
      }
      case LogLevel::WARN: {
        return "\x1b[33m";
      }
      case LogLevel::ERROR: {
        return "\x1b[31m";
      }
      case LogLevel::FATAL: {
        return "\x1b[91m";
      }
      default: {
        return "";
      }
    }
  }

  constexpr std::string_view log_level_to_string(LogLevel::Type level) {
    switch (level) {
      case LogLevel::TRACE: {
        return "TRACE";
      }
      case LogLevel::DEBUG: {
        return "DEBUG";
      }
      case LogLevel::INFO: {
        return "INFO";
      }
      case LogLevel::WARN: {
        return "WARN";
      }
      case LogLevel::ERROR: {
        return "ERROR";
      }
      case LogLevel::FATAL: {
        return "FATAL";
      }
      default: {
        return "";
      }
    }
  }
};

}  // namespace convert2pgn