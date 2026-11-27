#include "unifier.hpp"

#include <avcpp/formatcontext.h>

#include <filesystem>
#include <system_error>

#include "configurator.hpp"
namespace convert2pgn::Unifier {

std::string_view unify(const ProgramArgs& args) {
  g_logger.trace("Entered convert2pgn::Compressor::compress");
  std::error_code error;

  av::FormatContext ictx;
  av::Dictionary iopt;

  auto temp_dir = std::filesystem::temp_directory_path();

  if (args.is_input_image_sequence) {
    ictx.setFormat(av::InputFormat("image2"));
    iopt.set("framerate", "30");  // arbitrary number
  }

  ictx.openInput(args.input_file, iopt, error);
  if (error) {
    g_logger.fatal("Error opening input file {}: {}", args.input_file, error.message());
    throw ProgramError("input_file_error");
  }

  ictx.findStreamInfo(error);
  if (error) {
    g_logger.fatal("Error finding stream info in input file {}: {}", args.input_file, error.message());
    throw ProgramError("input_file_error");
  }
  
  int vid_stream_idx = -1;
  av::Stream in_vid_steam;
  for (int i = 0; i < ictx.streamsCount(); ++i) {
    auto stream = ictx.stream(i);
    if (stream.isVideo()) {
      vid_stream_idx = i;
      in_vid_steam = stream;
      break;
    }
  }
  if (vid_stream_idx == -1) {
    g_logger.fatal("Error getting video steam in input file {}: Video stream not found");
    throw ProgramError("input_file_error");
  }

  av::Codec decodec; // HAHHAHAHAHAHAHHAHAHAHHAH DECODE + CODEC = DECODEC XD XD XD XD XD......... sorry
  if (in_vid_steam.codecParameters().codecId() == AV_CODEC_ID_VP9) {
    decodec = av::findDecodingCodec("libvpx-vp9");
  }

}

}  // namespace Compressor