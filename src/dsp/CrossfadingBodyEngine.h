#pragma once

#include "dsp/BodyProfileCompiler.h"
#include "dsp/HybridBodyEngine.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gitasedap::dsp
{

class CrossfadingBodyEngine
{
public:
  void prepare(
    double sampleRate,
    double initialBodyAmountNormalized,
    double transitionMilliseconds = 15.0
  ) noexcept;

  void reset() noexcept;

  void activateInitial(
    const PreparedBodyProfile& prepared
  ) noexcept;

  [[nodiscard]] bool beginProfileTransition(
    const PreparedBodyProfile& prepared
  ) noexcept;

  void setBodyAmountNormalized(double normalizedAmount) noexcept;

  [[nodiscard]] double processSample(double input) noexcept;

  [[nodiscard]] bool hasPreparedProfile() const noexcept
  {
    return mHasPreparedProfile;
  }

  [[nodiscard]] bool isTransitioning() const noexcept
  {
    return mTransitioning;
  }

  [[nodiscard]] std::uint64_t activeProfileHash() const noexcept;

  [[nodiscard]] std::uint64_t targetProfileHash() const noexcept
  {
    return mTargetProfileHash;
  }

  [[nodiscard]] std::size_t transitionSamples() const noexcept
  {
    return mTransitionSamples;
  }

private:
  [[nodiscard]] std::size_t inactiveIndex() const noexcept
  {
    return 1U - mActiveIndex;
  }

  std::array<HybridBodyEngine, 2> mEngines;

  std::size_t mActiveIndex{0};
  std::size_t mTransitionPosition{0};
  std::size_t mTransitionSamples{1};

  std::uint64_t mTargetProfileHash{0};

  bool mHasPreparedProfile{false};
  bool mTransitioning{false};
};

} // namespace gitasedap::dsp
