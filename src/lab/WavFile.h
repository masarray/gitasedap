#pragma once

#include "lab/AudioBuffer.h"

#include <cstdint>
#include <filesystem>

namespace gitasedap::lab
{

struct WavData
{
  std::uint32_t sampleRate{0};
  AudioBuffer audio;
};

class WavFile
{
public:
  [[nodiscard]] static WavData read(const std::filesystem::path& path);

  static void writeFloat32(
    const std::filesystem::path& path,
    std::uint32_t sampleRate,
    const AudioBuffer& audio
  );
};

} // namespace gitasedap::lab
