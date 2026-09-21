// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Function f(x)
#pragma once
#include "configurator.hpp"

namespace Parsor {

using namespace convert2pgn;

ProgramArgs parse_args(const std::vector<std::string>& raw_args);
void filter_args(std::vector<std::string>& raw_args);
std::vector<int64_t> parse_frame_filter(const std::vector<std::string>& raw_filters, std::string_view filter_type, int64_t last_frame = -1);
std::vector<int64_t> parse_frame_mappings(const std::vector<std::string>& raw_mappings, int64_t last_frame = -1);
void check_files(const ProgramArgs& args);
void check_files(const ProgramArgs& args, int64_t& last_frame);
void print_help();
void print_version();

}  // namespace Parsor