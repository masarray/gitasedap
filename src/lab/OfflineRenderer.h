#pragma once

#include "lab/AudioBuffer.h"

#include <cstddef>

namespace gitasedap::lab
{

struct PrepareSpec
{
  double sampleRate{48000.0};
  std::size_t maximumBlockSize{64};
  std::size_t channelCount{1};
};

class IOfflineProcessor
{
public:
  virtual ~IOfflineProcessor() = default;

  virtual void prepare(const PrepareSpec& spec) = 0;
  virtual void reset() noexcept = 0;

  virtual void process(
    float* const* channels,
    std::size_t channelCount,
    std::size_t frameCount
  ) noexcept = 0;
};

struct RenderOptions
{
  std::size_t blockSize{64};
  bool resetBeforeRender{true};
};

class OfflineRenderer
{
public:
  [[nodiscard]] static AudioBuffer render(
    const AudioBuffer& input,
    double sampleRate,
    IOfflineProcessor& processor,
    const RenderOptions& options = {}
  );
};

} // namespace gitasedap::lab
