#pragma once

#include "lab/AudioBuffer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace gitasedap::lab
{

struct GoldenTolerance
{
  double absolute{1.0e-6};
  double relative{1.0e-5};
};

struct GoldenComparison
{
  bool shapeMatches{false};
  std::size_t sampleCount{0};
  std::size_t mismatchedSamples{0};
  double maximumAbsoluteError{0.0};
  double rmsError{0.0};

  [[nodiscard]] bool passed() const noexcept
  {
    return shapeMatches && mismatchedSamples == 0;
  }
};

[[nodiscard]] inline GoldenComparison compareGolden(
  const AudioBuffer& expected,
  const AudioBuffer& actual,
  GoldenTolerance tolerance = {}
) noexcept
{
  GoldenComparison result;

  result.shapeMatches =
    expected.channelCount() == actual.channelCount()
    && expected.frameCount() == actual.frameCount();

  if(!result.shapeMatches)
    return result;

  double sumSquares = 0.0;

  for(std::size_t channel = 0; channel < expected.channelCount(); ++channel)
  {
    const auto expectedSamples = expected.channel(channel);
    const auto actualSamples = actual.channel(channel);

    for(std::size_t frame = 0; frame < expectedSamples.size(); ++frame)
    {
      const double expectedValue =
        static_cast<double>(expectedSamples[frame]);
      const double actualValue =
        static_cast<double>(actualSamples[frame]);

      const double error = std::abs(expectedValue - actualValue);
      result.maximumAbsoluteError =
        std::max(result.maximumAbsoluteError, error);
      sumSquares += error * error;
      ++result.sampleCount;

      const double scale = std::max(
        std::abs(expectedValue),
        std::abs(actualValue)
      );
      const double allowed =
        tolerance.absolute + (tolerance.relative * scale);

      if(!std::isfinite(error) || error > allowed)
        ++result.mismatchedSamples;
    }
  }

  if(result.sampleCount > 0)
  {
    result.rmsError = std::sqrt(
      sumSquares / static_cast<double>(result.sampleCount)
    );
  }

  return result;
}

} // namespace gitasedap::lab
