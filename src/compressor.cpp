#include "compressor.hpp"

#include <avcpp/formatcontext.h>

#include <system_error>
#include <print>

#include "configurator.hpp"
namespace Compressor {

using namespace convert2pgn;

void compress(const ProgramArgs& args) {
  std::error_code error;
  av::FormatContext input;

  input.openInput(args.input_file, error);
  if (error) {
    std::println(stderr, "Error opening input file {}: " + error.message(), args.input_file);
    throw ProgramError("input_file_error");
  }
  
}

}  // namespace Compressor