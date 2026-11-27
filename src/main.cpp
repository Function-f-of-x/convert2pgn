// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Function f(x)
#include <iostream>
#include <numeric>
#include <print>
#include <ranges>

#include "configurator.hpp"

using namespace convert2pgn;

int main(int argc, char* argv[]) {
  std::vector<std::string> raw_args(argv, argv + argc);
  std::unordered_map<std::string_view, LogLevel::Type> possible_value_map = {{"silent", LogLevel::SILENT}, {"fatal", LogLevel::FATAL}, {"error", LogLevel::ERROR}, {"warn", LogLevel::WARN}, {"warning", LogLevel::WARN}, {"info", LogLevel::INFO}, {"debug", LogLevel::DEBUG}, {"trace", LogLevel::TRACE}, {"0", LogLevel::SILENT}, {"1", LogLevel::FATAL}, {"2", LogLevel::ERROR}, {"3", LogLevel::WARN}, {"4", LogLevel::INFO}, {"5", LogLevel::DEBUG}, {"6", LogLevel::TRACE}};
  for (const auto&& [index, arg] : raw_args | std::views::enumerate) {
    if (arg == "--loglevel") {
      if (index + 1 < raw_args.size()) {
        if (possible_value_map.contains(raw_args[index + 1])) {
          g_logger.set_max_log_level(possible_value_map[raw_args[index + 1]]);
        }
      }
    } else if (arg == "--verbose") {
      g_logger.set_verbose(true);
    }
  }
  g_logger.debug("Minimal logger initialization complete!");
  g_logger.info("Convert2pgn - a utility to convert any type of media into a Chess.com compatible PGN animation");
  g_logger.debug("Count of args: {}", argc);
  g_logger.debug("Vector of args: {}", std::accumulate(argv, argv + argc, std::string{},
                                                       [](std::string a, char* b) -> std::string { a += "["; a += std::string(b); a += "] "; return a; }));

  return 0;
}