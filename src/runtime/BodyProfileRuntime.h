#pragma once

#include "dsp/BodyProfile.h"
#include "dsp/BodyProfileCompiler.h"
#include "dsp/CrossfadingBodyEngine.h"
#include "runtime/PreparedStateExchange.h"

#include <cstddef>
#include <cstdint>

namespace gitasedap::runtime
{

class BodyProfileRuntime
{
public:
  using Exchange =
    PreparedStateExchange<dsp::PreparedBodyProfile>;

  using RequestToken = Exchange::RequestToken;
  using PreparedHandle = Exchange::Handle;
  using PublishResult = Exchange::PublishResult;

  void prepare(
    double sampleRate,
    double initialBodyAmountNormalized
  ) noexcept;

  void resetAudioState() noexcept;

  [[nodiscard]] RequestToken beginProfileRequest();

  [[nodiscard]] static PreparedHandle prepareProfile(
    RequestToken token,
    const dsp::BodyProfileDefinition& definition,
    double sampleRate,
    dsp::BodyProfileCompileError* error = nullptr
  );

  [[nodiscard]] PublishResult publish(
    PreparedHandle prepared
  );

  void beginAudioBlock() noexcept;

  void setBodyAmountNormalized(double normalizedAmount) noexcept;

  [[nodiscard]] double processSample(double input) noexcept;

  [[nodiscard]] std::size_t drainReclaimable();

  [[nodiscard]] std::size_t ownedPreparedStateCount() const;

  [[nodiscard]] std::uint64_t latestRequestedGeneration() const noexcept
  {
    return mExchange.latestRequestedGeneration();
  }

  [[nodiscard]] std::uint64_t activeGeneration() const noexcept
  {
    return mExchange.activeGeneration();
  }

  [[nodiscard]] std::uint64_t appliedProfileHash() const noexcept
  {
    return mAppliedProfileHash;
  }

  [[nodiscard]] bool isTransitioning() const noexcept
  {
    return mEngine.isTransitioning();
  }

  [[nodiscard]] std::size_t transitionSamples() const noexcept
  {
    return mEngine.transitionSamples();
  }

  void shutdownAfterAudioStopped() noexcept;

private:
  Exchange mExchange;
  dsp::CrossfadingBodyEngine mEngine;

  double mSampleRate{48000.0};
  std::uint64_t mAppliedProfileHash{0};
};

} // namespace gitasedap::runtime
