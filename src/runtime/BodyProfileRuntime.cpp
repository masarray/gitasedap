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

  mAppliedProfileHash = 0;
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

void BodyProfileRuntime::beginAudioBlock() noexcept
{
  const auto* prepared =
    mExchange.acquireForAudioBlock();

  if(prepared == nullptr)
    return;

  if(!mEngine.hasPreparedProfile())
  {
    mEngine.activateInitial(*prepared);
    mAppliedProfileHash = prepared->contentHash;
    return;
  }

  if(
    !mEngine.isTransitioning()
    && prepared->contentHash != mAppliedProfileHash
  )
  {
    if(mEngine.beginProfileTransition(*prepared))
      mAppliedProfileHash = prepared->contentHash;
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
  mAppliedProfileHash = 0;
}

} // namespace gitasedap::runtime
