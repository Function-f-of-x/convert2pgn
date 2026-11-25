// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Function f(x)
#include "parsor.hpp"

#include <algorithm>
#include <fstream>
#include <print>
#include <ranges>
#include <sstream>

#include "chess-library/chess.hpp"
#include "configurator.hpp"

namespace Parsor {
using namespace convert2pgn;

class ParsorVisitor : public chess::pgn::Visitor {
 private:
  bool validate = true;
  chess::Board board;
  bool is_game_ended = false;

 public:
  int64_t halfmove_count = 0;
  bool is_valid = true;

  ParsorVisitor() : Visitor() {}

  ParsorVisitor(bool validate) : Visitor(), validate(validate) {}

  void move(std::string_view move_san, std::string_view comment) override {
    ++halfmove_count;
    if (!is_valid || !validate) return;

    chess::Move move = chess::uci::parseSan(board, move_san);
    if (move == chess::Move::NO_MOVE || !board.isLegal(move)) {
      is_valid = false;
      std::println(stderr, "Warning: Game file contains illegal moves or an unknown variant is used. Chess.com may not be able to parse the game.");
    } else {
      board.makeMove(move);
    }
  }

  void startPgn() override {
    if (is_game_ended) {
      std::println(stderr, "Error parsing game file: can't have more than one game in a file");
      throw ProgramError("more_than_one_game_in_pgn");
    }
  }
  void header(std::string_view key, std::string_view value) override {
    if (key == "FEN") {
      board.setFen(value);
    }
  }
  void startMoves() override {}
  void endPgn() override {
    is_game_ended = true;
  }
};

ProgramArgs parse_args(std::vector<std::string>& raw_args) {
  ProgramArgs args;
  std::vector<std::string> raw_include_frames, raw_exclude_frames, raw_map_frames;
  int64_t last_frame = -1;
  enum class Parsing {
    positional,
    input,
    output,
    game,
    include_frames,
    exclude_frames,
    map_frames,
    frame_threshold,
    filenames
  };
  Parsing now = Parsing::positional;

  filter_args(raw_args);

  for (auto&& [index, arg] : raw_args | std::views::enumerate) {
    if (arg.starts_with('-') && now != Parsing::filenames) {
      if (arg == "--") {
        now = Parsing::filenames;
      } else if (arg == "--help") {
        print_help();
        return {};
      } else if (arg == "--version") {
        print_version();
        return {};
      } else if (arg == "-i" || arg == "--input") {
        now = Parsing::input;
      } else if (arg == "-o" || arg == "--output") {
        now = Parsing::output;
      } else if (arg == "-g" || arg == "--game") {
        now = Parsing::game;
      } else if (arg == "-if" || arg == "--include-frames") {
        now = Parsing::include_frames;
      } else if (arg == "-ef" || arg == "--exclude-frames") {
        now = Parsing::exclude_frames;
      } else if (arg == "-mf" || arg == "--map-frames") {
        now = Parsing::map_frames;
      } else if (arg == "--frame-threshold") {
        now = Parsing::frame_threshold;
      } else {
        std::println(stderr, "Invalid argument: {}\nTip: use --help to list all available arguments\nTip 2: if your file starts with a dash, prefix it with ./ (on linux) or with .\\ and wrap it in \"double quotes\" (on windows)", arg);
        throw ProgramError("invalid_arg");
      }
      continue;
    }
    if (arg.size() > 2 && arg.front() == '\'' && arg.back() == '\'') {
      arg = arg.substr(1, arg.size() - 2);
    }
    switch (now) {
      case Parsing::positional: {
        if (!args.has_input_file) {
          args.input_file = arg;
          args.has_input_file = true;
        } else {
          std::println(stderr, "Error parsing input file: you cannot have more than one input file.\nTip: If you want to use a sequance of images, specify a file pattern e.g. 'frames/frame_%04d.png', where %04d means a 4 digit number with leading zeros (0000, 0001, 0002...)");
          throw ProgramError("more_than_one_input_file");
        }
        break;
      }
      case Parsing::input: {
        if (!args.has_input_file) {
          args.input_file = arg;
          args.has_input_file = true;
        } else {
          std::println(stderr, "Error parsing input file: you cannot have more than one input file.\nTip: If you want to use a sequance of images, specify a file pattern e.g. 'frames/frame_%04d.png', where %04d means a 4 digit number with leading zeros (0000, 0001, 0002...)");
          throw ProgramError("more_than_one_input_file");
        }
        now = Parsing::positional;
        break;
      }
      case Parsing::filenames: {
        if (!args.has_input_file) {
          args.input_file = arg;
          args.has_input_file = true;
        } else {
          std::println(stderr, "Error parsing input file: you cannot have more than one input file.\nTip: If you want to use a sequance of images, specify a file pattern e.g. 'frames/frame_%04d.png', where %04d means a 4 digit number with leading zeros (0000, 0001, 0002...)");
          throw ProgramError("more_than_one_input_file");
        }
        break;
      }
      case Parsing::output: {
        if (!args.has_output_file) {
          args.output_file = arg;
          args.has_output_file = true;
        } else {
          std::println(stderr, "Error parsing output file: you cannot have more than one output file");
          throw ProgramError("more_than_one_output_file");
        }
        now = Parsing::positional;
        break;
      }
      case Parsing::game: {
        if (!args.has_game_file) {
          args.game_file = arg;
          args.has_game_file = true;
        } else {
          std::println(stderr, "Error parsing game file: you cannot have more than one game file");
          throw ProgramError("more_than_one_game_file");
        }
        now = Parsing::positional;
        break;
      }
      case Parsing::include_frames: {
        raw_include_frames.push_back(arg);
        args.has_include_frames = true;
        break;
      }
      case Parsing::exclude_frames: {
        raw_exclude_frames.push_back(arg);
        args.has_exclude_frames = true;
        break;
      }
      case Parsing::map_frames: {
        raw_map_frames.push_back(arg);
        args.has_map_frames = true;
        break;
      }
      case Parsing::frame_threshold: {
        if (last_frame == -1) {
          try {
            last_frame = std::stoll(arg);
            if (last_frame < 1) {
              std::println(stderr, "Error parsing frame threshold: specified value is negative or zero.");
              throw ProgramError("threshold_leq_zero");
            }
          } catch (const std::invalid_argument&) {
            std::println(stderr, "Error parsing frame threshold: specified value is not a valid number.");
            throw ProgramError("threshold_nan");
          } catch (const std::out_of_range&) {
            std::println(stderr, "Error parsing frame threshold: specified value exceeds max value of int64_t ({})", INT64_MAX);
            throw ProgramError("threshold_out_of_range");
          }
        } else {
          std::println(stderr, "Error parsing frame threshold: you cannot have more than one threshold");
          throw ProgramError("more_than_one_threshold");
        }
        now = Parsing::positional;
        break;
      }
    }
  }
  if (args.input_file.contains("%")) {
    args.is_input_image_sequence = true;
  }
  check_files(args, last_frame);

  if (args.has_include_frames) {
    args.include_frames = parse_frame_filter(raw_include_frames, "Include frames", last_frame);
  }
  if (args.has_exclude_frames) {
    args.exclude_frames = parse_frame_filter(raw_exclude_frames, "Exclude frames", last_frame);
  }
  if (args.has_map_frames) {
    args.map_frames = parse_frame_mappings(raw_map_frames, last_frame);
  }
  return args;
}

void filter_args(std::vector<std::string>& raw_args) {
  merge_single_quotes(raw_args);
  std::vector<std::string> filtered_args;
  for (auto&& [index, arg] : raw_args | std::views::enumerate) {
    if (index == 0) {
      continue;
    }
    StringTrim::trim(arg);
    if (arg.empty()) {
      continue;
    }
    if (arg.starts_with("-") && arg.contains("=")) {
      std::println(stderr, "Error parsing flags: --key=value format is not supported, use --key value");
      throw ProgramError("invalid_arg");
    } else {
      filtered_args.push_back(arg);
    }
  }
  raw_args = filtered_args;
}

void merge_single_quotes(std::vector<std::string>& raw_args) {
  std::vector<std::string> merged_args;
  bool looking_for_single_quote = false;
  std::string single_quoted_arg = "";
  for (const auto& arg : raw_args) {
    if (arg.starts_with("'") && !arg.ends_with("'") && !looking_for_single_quote) {
      looking_for_single_quote = true;
      single_quoted_arg += arg;
      continue;
    }
    if (looking_for_single_quote) {
      single_quoted_arg += " " + arg;
      if (arg.ends_with("'")) {
        looking_for_single_quote = false;
      } else {
        continue;
      }
    }
    if (!single_quoted_arg.empty()) {
      merged_args.push_back(single_quoted_arg);
      single_quoted_arg = "";
    } else {
      merged_args.push_back(arg);
    }
  }
  if (looking_for_single_quote) {
    std::println(stderr, "Error parsing flags: single quote is 'not closed");
    throw ProgramError("invalid_arg");
  }
  raw_args = merged_args;
}

std::vector<int64_t> parse_frame_range(const std::string& raw_range, std::string_view range_type, int64_t last_frame) {
  std::vector<int64_t> processed_range;
  std::vector<std::string> range = raw_range | std::views::split('~') | std::ranges::to<std::vector<std::string>>();
  try {
    if (range.size() == 1) {
      int64_t frame = std::stoll(range.at(0));
      if (frame < 1) {
        std::println(stderr, "Error parsing {}: one of the arguments is negative or zero.", range_type);
        throw ProgramError("range_arg_leq_zero");
      }
      processed_range.push_back(frame);
    } else if (range.size() == 2) {
      int64_t range_start, range_end;
      if (range.at(0).empty()) {
        range_start = 1;
      } else {
        range_start = std::stoll(range.at(0));
      }
      if (range.at(1).empty()) {
        if (last_frame == -1) {
          std::println(stderr, "Error parsing {}: using open ranges with no game file provided results in a never ending loop. Use --frame-threshold to specify a threshold.", range_type);
          throw ProgramError("range_open_without_threshold");
        }
        range_end = last_frame;
      } else {
        range_end = std::stoll(range.at(1));
      }
      if (range_start < 1 || range_end < 1) {
        std::println(stderr, "Error parsing {}: one of the arguments is negative or zero.", range_type);
        throw ProgramError("range_arg_leq_zero");
      }
      if (range_start > range_end) {
        std::println(stderr, "Error parsing {}: start of range bigger than end of range.", range_type);
        throw ProgramError("range_start_bigger_than_end");
      }
      for (int64_t i = range_start; i <= range_end; ++i) {
        processed_range.push_back(i);
      }
    } else if (range.size() == 3) {
      if (range.at(0).empty() && range.at(1).empty() && range.at(2).empty()) {
        if (last_frame == -1) {
          std::println(stderr, "Error parsing {}: using open ranges with no game file provided results in a never ending loop. Use --frame-threshold to specify a threshold.", range_type);
          throw ProgramError("range_open_without_threshold");
        }

        for (int64_t i = 1; i <= last_frame; ++i) {
          processed_range.push_back(i);
        }

      } else {
        std::println(stderr, "Error parsing {}: too many separators", range_type);
        throw ProgramError("range_arg_too_many_separators");
      }
    } else {
      std::println(stderr, "Error parsing {}: too many separators", range_type);
      throw ProgramError("range_arg_too_many_separators");
    }
  } catch (const std::invalid_argument&) {
    std::println(stderr, "Error parsing {}: one of the arguments is not a valid number.", range_type);
    throw ProgramError("range_arg_nan");
  } catch (const std::out_of_range&) {
    std::println(stderr, "Error parsing {}: one of the arguments exceeds max value of int64_t ({})", range_type, INT64_MAX);
    throw ProgramError("range_arg_out_of_range");
  }

  return processed_range;
}

std::vector<int64_t> parse_frame_filter(const std::vector<std::string>& raw_filters, std::string_view filter_type, int64_t last_frame) {
  std::vector<int64_t> processed_filters;
  for (auto& filter : raw_filters) {
    std::vector<int64_t> range = parse_frame_range(filter, filter_type, last_frame);
    processed_filters.insert_range(processed_filters.end(), range);
  }
  std::ranges::sort(processed_filters);
  auto [first, last] = std::ranges::unique(processed_filters);
  processed_filters.erase(first, last);

  return processed_filters;
}

std::unordered_map<int64_t, int64_t> parse_frame_mappings(const std::vector<std::string>& raw_mappings, int64_t last_frame) {
  std::unordered_map<int64_t, int64_t> processed_mappings;
  for (const auto& mapping : raw_mappings) {
    std::vector<std::string> split_mapping = mapping | std::views::split(':') | std::ranges::to<std::vector<std::string>>();
    if (split_mapping.size() != 2) {
      std::println(stderr, "Error parsing Map frames: map must have exactly 1 map separator");
      throw ProgramError("mapping_arg_invalid_separators");
    }
    std::vector<int64_t> mapping_frame_range = parse_frame_range(split_mapping[0], "Map frames", last_frame),
                         mapping_move_range = parse_frame_range(split_mapping[1], "Map frames", last_frame);

    if (mapping_frame_range.size() == 1 && mapping_move_range.size() == 1) {
      processed_mappings.insert_or_assign(mapping_move_range[0], mapping_frame_range[0]);
    } else if (mapping_frame_range.size() > 1 && mapping_move_range.size() == 1) {
      for (int64_t i = 0; i < mapping_frame_range.size(); i++) {
        processed_mappings.insert_or_assign(mapping_move_range[0] + i, mapping_frame_range[i]);
      }
    } else if (mapping_frame_range.size() >= 1 && mapping_move_range.size() > 1) {
      for (int64_t i = 0; i < mapping_move_range.size(); i++) {
        processed_mappings.insert_or_assign(mapping_move_range[i], mapping_frame_range[i % mapping_frame_range.size()]);
      }
    }
  }
  if (!processed_mappings.empty()) {
    for (int64_t i = 0; i <= std::ranges::max(processed_mappings | std::views::keys); ++i) {
      if (!processed_mappings.contains(i)) {
        processed_mappings.insert_or_assign(i, 0);
      }
    }
  }
  return processed_mappings;
}

void check_files(const ProgramArgs& args) {
  int64_t i_am_a_cute_kittty_nya_OwO = -static_cast<int64_t>(0xDEADC0DE & 0xB00B5) >> 63;
  check_files(args, i_am_a_cute_kittty_nya_OwO);
}

void check_files(const ProgramArgs& args, int64_t& last_frame) {
  if (args.has_game_file) {
    try {
      std::ifstream game_file;
      game_file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
      game_file.open(args.game_file);
      game_file.exceptions(std::ios_base::badbit);
      std::stringstream game;
      game << game_file.rdbuf();
      ParsorVisitor visitor;
      chess::pgn::StreamParser validator(game);
      validator.readGames(visitor);
      if (last_frame == -1) {
        last_frame = visitor.halfmove_count;
      } else if (last_frame > visitor.halfmove_count) {
        last_frame = visitor.halfmove_count;
        std::println(stderr, "Warning: frame threshold can't be bigger than amount of halfmoves in a game file and therefore is overwritten");
      }
    } catch (std::ios_base::failure& e) {
      std::println(stderr, "Error trying to open game file {}: {}\nTip: if your path contains spaces, wrap it in 'single quotes'", args.game_file, e.what());
      throw ProgramError("game_file_error");
    } catch (ProgramError&) {
      throw;
    } catch (std::exception& e) {
      std::println(stderr, "Error trying to parse game file {}: {}", args.game_file, e.what());
      throw ProgramError("game_file_error");
    }
  }
}

void print_help() {
  constexpr std::string_view help_menu = R"helpmenu(
convert2pgn - a utility to convert any type of media into a Chess.com compatible PGN animation
  )helpmenu";
  std::println(help_menu);
}

void print_version() {
  constexpr std::string_view version_info = R"versioninfo(
convert2pgn v1.0.0  
  )versioninfo";
  std::println(version_info);
}

}  // namespace Parsor