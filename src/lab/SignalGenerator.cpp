#include "lab/SignalGenerator.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace gitasedap::lab
{
namespace
{

class XorShift64Star
{
public:
  explicit XorShift64Star(std::uint64_t seed) noexcept
  : mState(seed == 0 ? 0x9E3779B97F4A7C15ULL : seed)
  {
  }

  [[nodiscard]] std::uint64_t next() noexcept
  {
    auto x = mState;
    x ^= x >> 12U;
    x ^= x << 25U;
    x ^= x >> 27U;
    mState = x;
    return x * 2685821657736338717ULL;
  }

  [[nodiscard]] float symmetricFloat() noexcept
  {
    const auto bits = static_cast<std::uint32_t>(next() >> 40U);
    constexpr float denominator = 8388607.5F;
    return (static_cast<float>(bits) / denominator) - 1.0F;
  }

private:
  std::uint64_t mState;
};

void validateCommon(
  std::size_t channels,
  std::size_t frames,
  float amplitude
)
{
  if(channels == 0)
    throw std::invalid_argument("channels must be non-zero");

  if(frames == 0)
    throw std::invalid_argument("frames must be non-zero");

  if(!std::isfinite(amplitude) || amplitude < 0.0F)
    throw std::invalid_argument("amplitude must be finite and non-negative");
}

} // namespace

AudioBuffer SignalGenerator::impulse(
  std::size_t channels,
  std::size_t frames,
  std::size_t impulseFrame,
  float amplitude
)
{
  validateCommon(channels, frames, amplitude);

  if(impulseFrame >= frames)
    throw std::out_of_range("impulseFrame is outside the buffer");

  AudioBuffer buffer(channels, frames);

  for(std::size_t channel = 0; channel < channels; ++channel)
    buffer.channel(channel)[impulseFrame] = amplitude;

  return buffer;
}

AudioBuffer SignalGenerator::sine(
  std::size_t channels,
  std::size_t frames,
  double sampleRate,
  double frequencyHz,
  float amplitude,
  double phaseRadians
)
{
  validateCommon(channels, frames, amplitude);

  if(
    !std::isfinite(sampleRate)
    || !std::isfinite(frequencyHz)
    || !std::isfinite(phaseRadians)
    || sampleRate <= 0.0
    || frequencyHz < 0.0
    || frequencyHz >= (sampleRate * 0.5)
  )
  {
    throw std::invalid_argument("invalid sine parameters");
  }

  AudioBuffer buffer(channels, frames);
  const double phaseIncrement =
    (2.0 * std::numbers::pi * frequencyHz) / sampleRate;

  for(std::size_t frame = 0; frame < frames; ++frame)
  {
    const auto sample = static_cast<float>(
      std::sin(phaseRadians + (phaseIncrement * static_cast<double>(frame)))
      * static_cast<double>(amplitude)
    );

    for(std::size_t channel = 0; channel < channels; ++channel)
      buffer.channel(channel)[frame] = sample;
  }

  return buffer;
}

AudioBuffer SignalGenerator::logarithmicSweep(
  std::size_t channels,
  std::size_t frames,
  double sampleRate,
  double startHz,
  double endHz,
  float amplitude
)
{
  validateCommon(channels, frames, amplitude);

  if(
    !std::isfinite(sampleRate)
    || !std::isfinite(startHz)
    || !std::isfinite(endHz)
    || sampleRate <= 0.0
    || startHz <= 0.0
    || endHz <= startHz
    || endHz >= (sampleRate * 0.5)
  )
  {
    throw std::invalid_argument("invalid logarithmic sweep parameters");
  }

  AudioBuffer buffer(channels, frames);

  if(frames == 1)
    return buffer;

  const double duration =
    static_cast<double>(frames - 1) / sampleRate;
  const double logRatio = std::log(endHz / startHz);
  const double phaseScale =
    2.0 * std::numbers::pi * startHz * duration / logRatio;

  for(std::size_t frame = 0; frame < frames; ++frame)
  {
    const double time = static_cast<double>(frame) / sampleRate;
    const double exponent = std::exp((time / duration) * logRatio);
    const double phase = phaseScale * (exponent - 1.0);

    const auto sample = static_cast<float>(
      std::sin(phase) * static_cast<double>(amplitude)
    );

    for(std::size_t channel = 0; channel < channels; ++channel)
      buffer.channel(channel)[frame] = sample;
  }

  return buffer;
}

AudioBuffer SignalGenerator::whiteNoise(
  std::size_t channels,
  std::size_t frames,
  std::uint64_t seed,
  float amplitude
)
{
  validateCommon(channels, frames, amplitude);

  AudioBuffer buffer(channels, frames);
  XorShift64Star random(seed);

  for(std::size_t channel = 0; channel < channels; ++channel)
  {
    auto samples = buffer.channel(channel);

    for(auto& sample : samples)
      sample = random.symmetricFloat() * amplitude;
  }

  return buffer;
}

AudioBuffer SignalGenerator::pinkNoise(
  std::size_t channels,
  std::size_t frames,
  std::uint64_t seed,
  float amplitude
)
{
  validateCommon(channels, frames, amplitude);

  AudioBuffer buffer(channels, frames);
  XorShift64Star random(seed);

  for(std::size_t channel = 0; channel < channels; ++channel)
  {
    double b0 = 0.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double b3 = 0.0;
    double b4 = 0.0;
    double b5 = 0.0;
    double b6 = 0.0;

    auto samples = buffer.channel(channel);

    for(auto& sample : samples)
    {
      const double white = static_cast<double>(random.symmetricFloat());

      b0 = (0.99886 * b0) + (white * 0.0555179);
      b1 = (0.99332 * b1) + (white * 0.0750759);
      b2 = (0.96900 * b2) + (white * 0.1538520);
      b3 = (0.86650 * b3) + (white * 0.3104856);
      b4 = (0.55000 * b4) + (white * 0.5329522);
      b5 = (-0.7616 * b5) - (white * 0.0168980);

      const double pink =
        b0 + b1 + b2 + b3 + b4 + b5 + b6 + (white * 0.5362);

      b6 = white * 0.115926;

      const double scaled = pink * 0.11 * static_cast<double>(amplitude);
      sample = static_cast<float>(std::clamp(scaled, -1.0, 1.0));
    }
  }

  return buffer;
}

} // namespace gitasedap::lab
