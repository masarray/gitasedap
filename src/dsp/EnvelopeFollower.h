#pragma once

#include <algorithm>
#include <cmath>

namespace gitasedap::dsp
{

class EnvelopeFollower
{
public:
  void prepare(
    double sampleRate,
    double attackMilliseconds,
    double releaseMilliseconds
  ) noexcept
  {
    mAttackCoefficient = timeCoefficient(
      sampleRate,
      attackMilliseconds
    );
    mReleaseCoefficient = timeCoefficient(
      sampleRate,
      releaseMilliseconds
    );
  }

  void reset(double value = 0.0) noexcept
  {
    mEnvelope = std::max(value, 0.0);
  }

  [[nodiscard]] double process(double input) noexcept
  {
    const double magnitude = std::abs(input);
    const double coefficient =
      magnitude > mEnvelope
        ? mAttackCoefficient
        : mReleaseCoefficient;

    mEnvelope =
      (coefficient * mEnvelope)
      + ((1.0 - coefficient) * magnitude);

    if(std::abs(mEnvelope) < 1.0e-30)
      mEnvelope = 0.0;

    return mEnvelope;
  }

  [[nodiscard]] double value() const noexcept
  {
    return mEnvelope;
  }

private:
  [[nodiscard]] static double timeCoefficient(
    double sampleRate,
    double milliseconds
  ) noexcept
  {
    const double safeRate = std::max(sampleRate, 1.0);
    const double seconds = std::max(milliseconds, 0.001) * 0.001;
    return std::exp(-1.0 / (safeRate * seconds));
  }

  double mAttackCoefficient{0.0};
  double mReleaseCoefficient{0.0};
  double mEnvelope{0.0};
};

} // namespace gitasedap::dsp
