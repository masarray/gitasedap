#pragma once

#include "dsp/BodyProfile.h"

#include <algorithm>
#include <array>
#include <cstddef>

namespace gitasedap::dsp
{

struct PreparedModalMode
{
  double b0{0.0};
  double b1{0.0};
  double b2{0.0};
  double a1{0.0};
  double a2{0.0};
  double gain{0.0};
};

struct PreparedModalBodyBank
{
  std::size_t modeCount{0};
  std::array<PreparedModalMode, kMaximumBodyModes> modes{};
};

class ModalBodyBank
{
public:
  void configure(
    const PreparedModalBodyBank& prepared
  ) noexcept
  {
    mModeCount = std::min(
      prepared.modeCount,
      kMaximumBodyModes
    );

    for(std::size_t index = 0; index < mModeCount; ++index)
    {
      const auto& mode = prepared.modes[index];
      mB0[index] = mode.b0;
      mB1[index] = mode.b1;
      mB2[index] = mode.b2;
      mA1[index] = mode.a1;
      mA2[index] = mode.a2;
      mGain[index] = mode.gain;
    }

    for(std::size_t index = mModeCount;
        index < kMaximumBodyModes;
        ++index)
    {
      mB0[index] = 0.0;
      mB1[index] = 0.0;
      mB2[index] = 0.0;
      mA1[index] = 0.0;
      mA2[index] = 0.0;
      mGain[index] = 0.0;
    }

    reset();
  }

  void reset() noexcept
  {
    mZ1.fill(0.0);
    mZ2.fill(0.0);
  }

  [[nodiscard]] double processSample(double input) noexcept
  {
    double sum = 0.0;

    for(std::size_t index = 0; index < mModeCount; ++index)
    {
      const double output =
        (mB0[index] * input) + mZ1[index];

      mZ1[index] =
        (mB1[index] * input)
        - (mA1[index] * output)
        + mZ2[index];

      mZ2[index] =
        (mB2[index] * input)
        - (mA2[index] * output);

      if(mZ1[index] > -1.0e-30 && mZ1[index] < 1.0e-30)
        mZ1[index] = 0.0;

      if(mZ2[index] > -1.0e-30 && mZ2[index] < 1.0e-30)
        mZ2[index] = 0.0;

      sum += output * mGain[index];
    }

    return sum;
  }

  [[nodiscard]] std::size_t modeCount() const noexcept
  {
    return mModeCount;
  }

private:
  std::size_t mModeCount{0};

  std::array<double, kMaximumBodyModes> mB0{};
  std::array<double, kMaximumBodyModes> mB1{};
  std::array<double, kMaximumBodyModes> mB2{};
  std::array<double, kMaximumBodyModes> mA1{};
  std::array<double, kMaximumBodyModes> mA2{};
  std::array<double, kMaximumBodyModes> mGain{};

  std::array<double, kMaximumBodyModes> mZ1{};
  std::array<double, kMaximumBodyModes> mZ2{};
};

} // namespace gitasedap::dsp
