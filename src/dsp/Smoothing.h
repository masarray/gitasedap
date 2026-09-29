#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace gitasedap::dsp
{

class LinearSmoother
{
public:
  void prepare(double sampleRate, double rampMilliseconds) noexcept
  {
    const double safeRate = std::max(sampleRate, 1.0);
    const double safeMilliseconds = std::max(rampMilliseconds, 0.0);

    mRampSamples = std::max<std::size_t>(
      1,
      static_cast<std::size_t>(
        std::llround(safeRate * safeMilliseconds * 0.001)
      )
    );
  }

  void reset(double value) noexcept
  {
    mCurrent = value;
    mTarget = value;
    mStep = 0.0;
    mRemaining = 0;
  }

  void setTarget(double value) noexcept
  {
    if(value == mTarget)
      return;

    mTarget = value;
    mRemaining = mRampSamples;
    mStep =
      (mTarget - mCurrent) / static_cast<double>(mRemaining);
  }

  [[nodiscard]] double next() noexcept
  {
    if(mRemaining == 0)
      return mCurrent;

    mCurrent += mStep;
    --mRemaining;

    if(mRemaining == 0)
      mCurrent = mTarget;

    return mCurrent;
  }

  [[nodiscard]] double current() const noexcept
  {
    return mCurrent;
  }

  [[nodiscard]] double target() const noexcept
  {
    return mTarget;
  }

  [[nodiscard]] bool settled() const noexcept
  {
    return mRemaining == 0;
  }

private:
  std::size_t mRampSamples{1};
  std::size_t mRemaining{0};
  double mCurrent{0.0};
  double mTarget{0.0};
  double mStep{0.0};
};

class BypassCrossfade
{
public:
  void prepare(double sampleRate, double fadeMilliseconds = 5.0) noexcept
  {
    mBypassMix.prepare(sampleRate, fadeMilliseconds);
  }

  void reset(bool bypassed) noexcept
  {
    mBypassMix.reset(bypassed ? 1.0 : 0.0);
  }

  void setBypassed(bool bypassed) noexcept
  {
    mBypassMix.setTarget(bypassed ? 1.0 : 0.0);
  }

  [[nodiscard]] double process(double dry, double wet) noexcept
  {
    const double bypass = mBypassMix.next();
    return wet + ((dry - wet) * bypass);
  }

  [[nodiscard]] double bypassMix() const noexcept
  {
    return mBypassMix.current();
  }

private:
  LinearSmoother mBypassMix;
};

} // namespace gitasedap::dsp
