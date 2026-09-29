#pragma once

#include "dsp/BodyProfileCompiler.h"
#include "dsp/ModalBodyBank.h"
#include "dsp/Smoothing.h"
#include "dsp/TransferFilter.h"

namespace gitasedap::dsp
{

class HybridBodyEngine
{
public:
  void prepare(
    double sampleRate,
    double initialBodyAmountNormalized
  ) noexcept;

  void configure(
    const PreparedBodyProfile& prepared
  ) noexcept;

  void reset() noexcept;

  void setBodyAmountNormalized(double normalizedAmount) noexcept;

  [[nodiscard]] double processSample(double input) noexcept;

  [[nodiscard]] std::uint64_t activeProfileHash() const noexcept
  {
    return mActiveProfileHash;
  }

  [[nodiscard]] BodyProfileId activeProfileId() const noexcept
  {
    return mActiveProfileId;
  }

  [[nodiscard]] bool hasPreparedProfile() const noexcept
  {
    return mHasPreparedProfile;
  }

private:
  TransferFilter mTransfer;
  ModalBodyBank mModal;
  LinearSmoother mBodyAmount;

  double mProfileOutputGain{1.0};
  std::uint64_t mActiveProfileHash{0};
  BodyProfileId mActiveProfileId{BodyProfileId::NaturalDevelopment};
  bool mHasPreparedProfile{false};
};

} // namespace gitasedap::dsp
