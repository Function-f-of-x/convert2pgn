#include "compressor.hpp"

#include <avcpp/formatcontext.h>

#include <print>
#include <system_error>

#include "configurator.hpp"
namespace Compressor {

using namespace convert2pgn;

void compress(const ProgramArgs& args) {
  std::error_code error;

  av::FormatContext context;
  av::Dictionary options;

  if (args.is_input_image_sequence) {
    context.setFormat(av::InputFormat("image2"));
    options.set("framerate", "30");  // arbitrary number
  }

  context.openInput(args.input_file, options, error);
  if (error) {
    std::println(stderr, "[COMPRESSOR] Error opening input file {}: {}", args.input_file, error.message());
    throw ProgramError("input_file_error");
  }

  context.findStreamInfo(error);
  if (error) {
    std::println(stderr, "[COMPRESSOR] Error finding stream info in input file {}: {}", args.input_file, error.message());
    throw ProgramError("input_file_error");
  }
  
}

}  // namespace Compressor