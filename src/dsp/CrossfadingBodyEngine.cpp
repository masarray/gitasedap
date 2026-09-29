#include "dsp/CrossfadingBodyEngine.h"

#include <algorithm>
#include <cmath>

namespace gitasedap::dsp
{

void CrossfadingBodyEngine::prepare(
  double sampleRate,
  double initialBodyAmountNormalized,
  double transitionMilliseconds
) noexcept
{
  const double safeRate = std::max(sampleRate, 8000.0);
  const double safeMilliseconds =
    std::clamp(transitionMilliseconds, 1.0, 100.0);

  mTransitionSamples = std::max<std::size_t>(
    1,
    static_cast<std::size_t>(
      std::llround(
        safeRate * safeMilliseconds * 0.001
      )
    )
  );

  for(auto& engine : mEngines)
  {
    engine.prepare(
      safeRate,
      initialBodyAmountNormalized
    );
  }

  mActiveIndex = 0;
  mTransitionPosition = 0;
  mTargetProfileHash = 0;
  mHasPreparedProfile = false;
  mTransitioning = false;
}

void CrossfadingBodyEngine::reset() noexcept
{
  for(auto& engine : mEngines)
    engine.reset();

  mTransitionPosition = 0;
  mTransitioning = false;

  if(mHasPreparedProfile)
  {
    mTargetProfileHash =
      mEngines[mActiveIndex].activeProfileHash();
  }
  else
  {
    mTargetProfileHash = 0;
  }
}

void CrossfadingBodyEngine::activateInitial(
  const PreparedBodyProfile& prepared
) noexcept
{
  mEngines[mActiveIndex].configure(prepared);
  mEngines[mActiveIndex].reset();

  mTargetProfileHash = prepared.contentHash;
  mTransitionPosition = 0;
  mHasPreparedProfile = true;
  mTransitioning = false;
}

bool CrossfadingBodyEngine::beginProfileTransition(
  const PreparedBodyProfile& prepared
) noexcept
{
  if(!mHasPreparedProfile)
  {
    activateInitial(prepared);
    return true;
  }

  if(mTransitioning)
    return false;

  if(
    prepared.contentHash
    == mEngines[mActiveIndex].activeProfileHash()
  )
  {
    mTargetProfileHash = prepared.contentHash;
    return true;
  }

  const auto next = inactiveIndex();

  mEngines[next].configure(prepared);
  mEngines[next].reset();

  mTargetProfileHash = prepared.contentHash;
  mTransitionPosition = 0;
  mTransitioning = true;

  return true;
}

void CrossfadingBodyEngine::setBodyAmountNormalized(
  double normalizedAmount
) noexcept
{
  for(auto& engine : mEngines)
    engine.setBodyAmountNormalized(normalizedAmount);
}

double CrossfadingBodyEngine::processSample(double input) noexcept
{
  if(!mHasPreparedProfile)
    return std::isfinite(input) ? input : 0.0;

  if(!mTransitioning)
    return mEngines[mActiveIndex].processSample(input);

  const auto next = inactiveIndex();

  const double from =
    mEngines[mActiveIndex].processSample(input);

  const double to =
    mEngines[next].processSample(input);

  const double mix = std::clamp(
    static_cast<double>(mTransitionPosition + 1)
      / static_cast<double>(mTransitionSamples),
    0.0,
    1.0
  );

  const double output = from + ((to - from) * mix);

  ++mTransitionPosition;

  if(mTransitionPosition >= mTransitionSamples)
  {
    mActiveIndex = next;
    mTransitionPosition = 0;
    mTransitioning = false;
    mTargetProfileHash =
      mEngines[mActiveIndex].activeProfileHash();
  }

  return std::isfinite(output) ? output : 0.0;
}

std::uint64_t CrossfadingBodyEngine::activeProfileHash() const noexcept
{
  if(!mHasPreparedProfile)
    return 0;

  return mEngines[mActiveIndex].activeProfileHash();
}

} // namespace gitasedap::dsp
