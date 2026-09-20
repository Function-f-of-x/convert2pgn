// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Function f(x)
#include <iostream>
#include <numeric>
#include <print>

#include "configurator.hpp"

using convert2pgn::print_debug;

int main(int argc, char* argv[]) {
  std::println("Convert2pgn - a utility to convert any type of media into a Chess.com compatible PGN animation");
  print_debug("Count of args: {}", argc);
  print_debug("Vector of args: {}", std::accumulate(argv, argv + argc, std::string{},
                                                     [](std::string a, char* b) -> std::string { a += "[ "; a += std::string(b); a += " ] "; return a; }));
  
  return 0;
}