#include "runtime/BodyProfileRuntime.h"

#include <algorithm>
#include <utility>

namespace gitasedap::runtime
{

void BodyProfileRuntime::prepare(
  double sampleRate,
  double initialBodyAmountNormalized
) noexcept
{
  mSampleRate = std::max(sampleRate, 8000.0);

  mEngine.prepare(
    mSampleRate,
    initialBodyAmountNormalized
  );

  mDesiredPreparedProfile = {};
  mDesiredProfileHash = 0;
  mAppliedProfileHash = 0;
  mHasDesiredPreparedProfile = false;
}

void BodyProfileRuntime::resetAudioState() noexcept
{
  mEngine.reset();
}

BodyProfileRuntime::RequestToken
BodyProfileRuntime::beginProfileRequest()
{
  return mExchange.beginRequest();
}

BodyProfileRuntime::PreparedHandle
BodyProfileRuntime::prepareProfile(
  RequestToken token,
  const dsp::BodyProfileDefinition& definition,
  double sampleRate,
  dsp::BodyProfileCompileError* error
)
{
  const auto compiled = dsp::compileBodyProfile(
    definition,
    sampleRate
  );

  if(error != nullptr)
    *error = compiled.error;

  if(!compiled.ok())
    return nullptr;

  return Exchange::prepare(
    token,
    compiled.prepared
  );
}

BodyProfileRuntime::PublishResult BodyProfileRuntime::publish(
  PreparedHandle prepared
)
{
  return mExchange.publish(std::move(prepared));
}

void BodyProfileRuntime::requestPreparedProfileAtAudioBlock(
  const dsp::PreparedBodyProfile& prepared
) noexcept
{
  if(prepared.contentHash == 0)
    return;

  if(
    prepared.contentHash == mDesiredProfileHash
    && (
      mHasDesiredPreparedProfile
      || prepared.contentHash == mAppliedProfileHash
    )
  )
  {
    return;
  }

  mDesiredPreparedProfile = prepared;
  mDesiredProfileHash = prepared.contentHash;
  mHasDesiredPreparedProfile = true;
}

void BodyProfileRuntime::beginAudioBlock() noexcept
{
  // External/custom profiles use the generation-safe exchange. Copy the newest
  // immutable state into the same bounded desired slot used by factory
  // selections. No exchanged pointer is retained by the DSP engine.
  if(const auto* prepared = mExchange.acquireForAudioBlock())
  {
    requestPreparedProfileAtAudioBlock(*prepared);
  }

  if(!mHasDesiredPreparedProfile)
    return;

  if(!mEngine.hasPreparedProfile())
  {
    mEngine.activateInitial(mDesiredPreparedProfile);
    mAppliedProfileHash = mDesiredProfileHash;
    mHasDesiredPreparedProfile = false;
    return;
  }

  if(mEngine.isTransitioning())
    return;

  if(mDesiredProfileHash == mEngine.activeProfileHash())
  {
    mAppliedProfileHash = mDesiredProfileHash;
    mHasDesiredPreparedProfile = false;
    return;
  }

  if(mEngine.beginProfileTransition(mDesiredPreparedProfile))
  {
    // "Applied" denotes the profile selected for the current bounded
    // transition. activeProfileHash() remains the sounding source until the
    // crossfade completes.
    mAppliedProfileHash = mDesiredProfileHash;
    mHasDesiredPreparedProfile = false;
  }
}

void BodyProfileRuntime::setBodyAmountNormalized(
  double normalizedAmount
) noexcept
{
  mEngine.setBodyAmountNormalized(normalizedAmount);
}

double BodyProfileRuntime::processSample(double input) noexcept
{
  return mEngine.processSample(input);
}

std::size_t BodyProfileRuntime::drainReclaimable()
{
  return mExchange.drainReclaimable();
}

std::size_t BodyProfileRuntime::ownedPreparedStateCount() const
{
  return mExchange.ownedStateCount();
}

void BodyProfileRuntime::shutdownAfterAudioStopped() noexcept
{
  mExchange.shutdownAfterAudioStopped();
  mDesiredPreparedProfile = {};
  mDesiredProfileHash = 0;
  mAppliedProfileHash = 0;
  mHasDesiredPreparedProfile = false;
}

} // namespace gitasedap::runtime
