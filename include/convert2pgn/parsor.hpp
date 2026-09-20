// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Function f(x)
#pragma once
#include "configurator.hpp"

namespace Parsor {

using namespace convert2pgn;

ProgramArgs parse_args(int argc, std::vector<std::string>);
std::vector<int> parse_frame_filter(std::vector<std::string>);
std::vector<int> parse_map_frames(std::vector<std::string>);
void check_files(ProgramArgs args);
void print_help();
void print_version();

}  // namespace Parsor