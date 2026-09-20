// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Function f(x)
#include "parsor.hpp"

#include <algorithm>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <print>
#include <ranges>

#include "chess-library/chess.hpp"
#include "configurator.hpp"

namespace Parsor {
using namespace convert2pgn;

class ParsorVisitor : public chess::pgn::Visitor {
 private:
  bool validate = true;
  chess::Board board;
  bool is_valid  = true;
  
 public:
  int64_t halfmove_count = 0;

  ParsorVisitor() : Visitor() {}

  ParsorVisitor(bool validate) : Visitor(), validate(validate) {}

  void move(std::string_view move_san, std::string_view comment) override {
    ++halfmove_count;
    if (!is_valid || !validate) return;

    chess::Move move = chess::uci::parseSan(board, move_san);
    if (move == chess::Move::NO_MOVE || !board.isLegal(move)) {
      is_valid = false;
      std::println(stderr, "Warning: Game file contaiins illegal moves or an unknown variant is used. Chess.com may not be able to parse the game.");
    } else {
      board.makeMove(move);
    }
  }

  void startPgn() override {
    halfmove_count = 0;
  }
  void header(std::string_view key, std::string_view value) override {
    if (key == "FEN") {
      board.setFen(value);
    }
  }
  void startMoves() override {}
  void endPgn() override {}
};

ProgramArgs parse_args(int argc, std::vector<std::string> argv) {
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
    frame_treshold
  };
  Parsing now = Parsing::positional;
  for (auto&& [index, arg] : argv | std::views::enumerate) {
    if (index == 0) {
      continue;
    }
    if (arg.starts_with('-')) {
      if (arg == "--") {
        now = Parsing::positional;
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
      } else if (arg == "--frame-treshold") {
        now = Parsing::frame_treshold;
      } else {
        std::println(stderr, "Invalid argument: {}\nTip: use --help to list all available arguments", arg);
        throw ProgramError("invalid_arg");
      }
      continue;
    }
    switch (now) {
      if (arg != "") {
        case Parsing::positional: {
          args.input_files.push_back(arg);
          args.has_input_files = true;
          break;
        }
        case Parsing::input: {
          args.input_files.push_back(arg);
          args.has_input_files = true;
          break;
        }
        case Parsing::output: {
          if (args.has_output_file) {
            args.output_file = arg;
            args.has_output_file = true;
          } else {
            std::println(stderr, "Error parsing output file: you cannot have more than one output file");
            throw ProgramError("more_than_one_output_file");
          }
          break;
        }
        case Parsing::game: {
          if (args.has_game_file) {
            args.game_file = arg;
            args.has_game_file = true;
          } else {
            std::println(stderr, "Error parsing game file: you cannot have more than one game file");
            throw ProgramError("more_than_one_game_file");
          }
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
        case Parsing::frame_treshold: {
          if (last_frame == -1) {
            try {
              last_frame = std::stoll(arg);
              if (last_frame < 1) {
                std::println(stderr, "Error parsing frame treshold: specified value is negative or zero.");
                throw ProgramError("treshold_leq_zero");
              }
            } catch (const std::invalid_argument&) {
              std::println(stderr, "Error parsing frame treshold: specified value is not a valid number.");
              throw ProgramError("treshold_nan");
            } catch (const std::out_of_range&) {
              std::println(stderr, "Error parsing frame treshold: specified value exceeds max value of int64_t ({})", INT64_MAX);
              throw ProgramError("treshold_out_of_range");
            }
          } else {
            std::println(stderr, "Error parsing frame treshold: you cannot have more than one treshold");
            throw ProgramError("more_than_one_treshold");
          }
          break;
        }
      }
    }
  }
  check_files(args);

  if (args.has_game_file) {
    std::ifstream game_file(args.game_file);
    std::stringstream game;
    game << game_file.rdbuf();
    ParsorVisitor visitor(false);
    chess::pgn::StreamParser counter(game);
    counter.readGames(visitor);
    last_frame = visitor.halfmove_count;
  }
  if (args.has_include_frames) {
    args.include_frames = parse_frame_filter(raw_include_frames, "Include frames", last_frame);
  }
  if (args.has_exclude_frames) {
    args.exclude_frames = parse_frame_filter(raw_exclude_frames, "Exclude frames", last_frame);
  }
  return args;
}

std::vector<int64_t> parse_frame_filter(std::vector<std::string> raw_filters, std::string_view filter_type, int64_t last_frame) {
  std::vector<int64_t> processed_filters;
  for (auto& filter : raw_filters) {
    std::vector<std::string> range = filter | std::views::split('~') | std::ranges::to<std::vector<std::string>>();
    try {
      if (range.size() == 1) {
        int64_t frame = std::stoll(range.at(0));
        if (frame < 1) {
          std::println(stderr, "Error parsing {}: one of the arguments is negative or zero.", filter_type);
          throw ProgramError("filter_arg_leq_zero");
        }
        processed_filters.push_back(frame);
      } else if (range.size() == 2) {
        int64_t range_start, range_end;
        if (range.at(0) == "") {
          range_start = 1;
        } else {
          range_start = std::stoll(range.at(0));
        }
        if (range.at(1) == "") {
          if (last_frame == -1) {
            std::println(stderr, "Error parsiong {}: using open ranges with no game file provided results in a never ending loop. Use --frame-treshold to specify a treshold.", filter_type);
          }
          range_end = last_frame;
        } else {
          range_end = std::stoll(range.at(1));
        }
        if (range_start < 1 || range_end < 1) {
          std::println(stderr, "Error parsing {}: one of the arguments is negative or zero.", filter_type);
          throw ProgramError("filter_arg_leq_zero");
        }
        if (range_start > range_end) {
          std::println(stderr, "Error parsing {}: start of range bigger than end of range.", filter_type);
          throw ProgramError("filter_range_start_bigger_than_end");
        }
        for (int64_t i = range_start; i <= range_end; ++i) {
          processed_filters.push_back(i);
        }
      } else if (range.size() == 3) {
        if (range.at(0) == "" && range.at(1) == "" && range.at(2) == "") {
          for (int64_t i = 1; i <= last_frame; ++i) {
            processed_filters.push_back(i);
          }
        } else {
          std::println(stderr, "Error parsing {}: too many separators", filter_type);
          throw ProgramError("filter_arg_too_many_separators");
        }
      } else {
        std::println(stderr, "Error parsing {}: too many separators", filter_type);
        throw ProgramError("filter_arg_too_many_separators");
      }
    } catch (const std::invalid_argument&) {
      std::println(stderr, "Error parsing {}: one of the arguments is not a valid number.", filter_type);
      throw ProgramError("filter_arg_nan");
    } catch (const std::out_of_range&) {
      std::println(stderr, "Error parsing {}: one of the arguments exceeds max value of int64_t ({})", filter_type, INT64_MAX);
      throw ProgramError("filter_arg_out_of_range");
    }
  }

  {
    std::ranges::sort(processed_filters);
    auto [first, last] = std::ranges::unique(processed_filters);
    processed_filters.erase(first, last);
  }

  return processed_filters;
}

void check_files(ProgramArgs args) {
  if (args.has_input_files) {
    for (auto&& [index, path] : args.input_files | std::views::enumerate) {
      std::ifstream input_file;
      input_file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
      try {
        input_file.open(path);
        input_file.exceptions(std::ios_base::badbit);
        (void)input_file.rdbuf();
      } catch (std::ios_base::failure& e) {
        std::println(stderr, "Error trying to open input file #{} ({}): {}", index, path, e.what());
        throw ProgramError(std::format("input_file_error"));
      }
      input_file.close();
    }
  }

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
    } catch (std::ios_base::failure& e) {
      std::println(stderr, "Error trying to open game file {}: {}", args.game_file, e.what());
      throw ProgramError("game_file_error");
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