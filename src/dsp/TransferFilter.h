#pragma once

#include "dsp/BodyProfile.h"

#include <algorithm>
#include <array>
#include <cstddef>

namespace gitasedap::dsp
{

struct PreparedTransferFilter
{
  std::size_t tapCount{1};
  std::array<double, kMaximumTransferTaps> taps{1.0};
};

class TransferFilter
{
public:
  void configure(
    const PreparedTransferFilter& prepared
  ) noexcept
  {
    mTapCount = std::clamp<std::size_t>(
      prepared.tapCount,
      1,
      kMaximumTransferTaps
    );
    mTaps = prepared.taps;
    reset();
  }

  void reset() noexcept
  {
    mDelay.fill(0.0);
    mWriteIndex = 0;
  }

  [[nodiscard]] double processSample(double input) noexcept
  {
    mDelay[mWriteIndex] = input;

    double output = 0.0;
    std::size_t readIndex = mWriteIndex;

    for(std::size_t tap = 0; tap < mTapCount; ++tap)
    {
      output += mTaps[tap] * mDelay[readIndex];

      if(readIndex == 0)
        readIndex = kMaximumTransferTaps - 1;
      else
        --readIndex;
    }

    ++mWriteIndex;

    if(mWriteIndex >= kMaximumTransferTaps)
      mWriteIndex = 0;

    return output;
  }

  [[nodiscard]] std::size_t tapCount() const noexcept
  {
    return mTapCount;
  }

private:
  std::size_t mTapCount{1};
  std::size_t mWriteIndex{0};
  std::array<double, kMaximumTransferTaps> mTaps{1.0};
  std::array<double, kMaximumTransferTaps> mDelay{};
};

} // namespace gitasedap::dsp
