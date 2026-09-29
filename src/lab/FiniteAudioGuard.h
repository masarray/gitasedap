#pragma once

#include "lab/AudioBuffer.h"

#include <cmath>
#include <cstddef>
#include <limits>

namespace gitasedap::lab
{

struct FiniteAudioReport
{
  std::size_t sampleCount{0};
  std::size_t nonFiniteCount{0};
  std::size_t firstNonFiniteChannel{
    std::numeric_limits<std::size_t>::max()
  };
  std::size_t firstNonFiniteFrame{
    std::numeric_limits<std::size_t>::max()
  };
  double peakAbsolute{0.0};
  double rms{0.0};

  [[nodiscard]] bool allFinite() const noexcept
  {
    return nonFiniteCount == 0;
  }
};

[[nodiscard]] inline FiniteAudioReport analyzeFiniteAudio(
  const AudioBuffer& buffer
) noexcept
{
  FiniteAudioReport report;
  double sumSquares = 0.0;

  for(std::size_t channel = 0; channel < buffer.channelCount(); ++channel)
  {
    const auto samples = buffer.channel(channel);

    for(std::size_t frame = 0; frame < samples.size(); ++frame)
    {
      const double value = static_cast<double>(samples[frame]);
      ++report.sampleCount;

      if(!std::isfinite(value))
      {
        if(report.nonFiniteCount == 0)
        {
          report.firstNonFiniteChannel = channel;
          report.firstNonFiniteFrame = frame;
        }

        ++report.nonFiniteCount;
        continue;
      }

      const double absolute = std::abs(value);
      if(absolute > report.peakAbsolute)
        report.peakAbsolute = absolute;

      sumSquares += value * value;
    }
  }

  const auto finiteCount = report.sampleCount - report.nonFiniteCount;

  if(finiteCount > 0)
  {
    report.rms = std::sqrt(
      sumSquares / static_cast<double>(finiteCount)
    );
  }

  return report;
}

} // namespace gitasedap::lab
