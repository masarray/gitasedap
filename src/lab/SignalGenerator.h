#pragma once

#include "lab/AudioBuffer.h"

#include <cstddef>
#include <cstdint>

namespace gitasedap::lab
{

class SignalGenerator
{
public:
  [[nodiscard]] static AudioBuffer impulse(
    std::size_t channels,
    std::size_t frames,
    std::size_t impulseFrame = 0,
    float amplitude = 1.0F
  );

  [[nodiscard]] static AudioBuffer sine(
    std::size_t channels,
    std::size_t frames,
    double sampleRate,
    double frequencyHz,
    float amplitude = 0.5F,
    double phaseRadians = 0.0
  );

  [[nodiscard]] static AudioBuffer logarithmicSweep(
    std::size_t channels,
    std::size_t frames,
    double sampleRate,
    double startHz,
    double endHz,
    float amplitude = 0.5F
  );

  [[nodiscard]] static AudioBuffer whiteNoise(
    std::size_t channels,
    std::size_t frames,
    std::uint64_t seed = 0x475344505032ULL,
    float amplitude = 0.25F
  );

  [[nodiscard]] static AudioBuffer pinkNoise(
    std::size_t channels,
    std::size_t frames,
    std::uint64_t seed = 0x475344505032ULL,
    float amplitude = 0.25F
  );
};

} // namespace gitasedap::lab
