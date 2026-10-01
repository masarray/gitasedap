#include "TestSupport.h"
#include "dsp/BodyProfile.h"
#include "dsp/BodyProfileCompiler.h"
#include "runtime/BodyProfileRuntime.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace gsdsp = gitasedap::dsp;
namespace gsr = gitasedap::runtime;

int main()
{
  constexpr double sampleRate = 48000.0;

  const auto raw = gsdsp::compileBodyProfile(
    gsdsp::rawConditionedProfile(),
    sampleRate
  );

  const auto natural = gsdsp::compileBodyProfile(
    gsdsp::naturalDevelopmentProfile(),
    sampleRate
  );

  const auto dreadnought = gsdsp::compileBodyProfile(
    gsdsp::dreadnoughtDevelopmentProfile(),
    sampleRate
  );

  GS_REQUIRE(raw.ok());
  GS_REQUIRE(natural.ok());
  GS_REQUIRE(dreadnought.ok());

  gsr::BodyProfileRuntime runtime;
  runtime.prepare(sampleRate, 1.0);

  runtime.requestPreparedProfileAtAudioBlock(raw.prepared);
  runtime.beginAudioBlock();

  GS_REQUIRE(
    runtime.appliedProfileHash()
    == raw.prepared.contentHash
  );

  GS_REQUIRE(!runtime.isTransitioning());
  GS_REQUIRE(runtime.ownedPreparedStateCount() == 0);

  double previous = 0.10;

  for(std::size_t frame = 0; frame < 2048; ++frame)
    previous = runtime.processSample(0.10);

  runtime.requestPreparedProfileAtAudioBlock(
    natural.prepared
  );
  runtime.beginAudioBlock();

  GS_REQUIRE(runtime.isTransitioning());
  GS_REQUIRE(
    runtime.appliedProfileHash()
    == natural.prepared.contentHash
  );

  // While Natural is transitioning in, user changes their mind repeatedly.
  // Only the latest desired factory profile should survive.
  runtime.requestPreparedProfileAtAudioBlock(
    dreadnought.prepared
  );
  runtime.requestPreparedProfileAtAudioBlock(
    raw.prepared
  );
  runtime.requestPreparedProfileAtAudioBlock(
    dreadnought.prepared
  );

  GS_REQUIRE(
    runtime.desiredProfileHash()
    == dreadnought.prepared.contentHash
  );

  double maximumStep = 0.0;

  for(std::size_t frame = 0;
      frame < runtime.transitionSamples();
      ++frame)
  {
    const double current = runtime.processSample(0.10);

    GS_REQUIRE(std::isfinite(current));

    maximumStep = std::max(
      maximumStep,
      std::abs(current - previous)
    );

    previous = current;
  }

  GS_REQUIRE(!runtime.isTransitioning());
  GS_REQUIRE(maximumStep < 0.02);

  runtime.beginAudioBlock();

  GS_REQUIRE(runtime.isTransitioning());
  GS_REQUIRE(
    runtime.appliedProfileHash()
    == dreadnought.prepared.contentHash
  );

  for(std::size_t frame = 0;
      frame < runtime.transitionSamples();
      ++frame)
  {
    GS_REQUIRE(
      std::isfinite(runtime.processSample(0.10))
    );
  }

  GS_REQUIRE(!runtime.isTransitioning());

  // Toggle A/B-style selection for a long run. Direct factory selection must
  // never allocate into PreparedStateExchange ownership.
  const std::array<const gsdsp::PreparedBodyProfile*, 3> profiles{
    &raw.prepared,
    &natural.prepared,
    &dreadnought.prepared
  };

  for(std::size_t block = 0; block < 5000; ++block)
  {
    const auto* desired = profiles[block % profiles.size()];

    runtime.requestPreparedProfileAtAudioBlock(*desired);
    runtime.beginAudioBlock();

    for(std::size_t frame = 0; frame < 64; ++frame)
    {
      const double input =
        0.12
        * std::sin(
          2.0
          * 3.14159265358979323846
          * 220.0
          * static_cast<double>((block * 64) + frame)
          / sampleRate
        );

      GS_REQUIRE(
        std::isfinite(runtime.processSample(input))
      );
    }

    GS_REQUIRE(runtime.ownedPreparedStateCount() == 0);
  }

  runtime.shutdownAfterAudioStopped();
  GS_REQUIRE(runtime.ownedPreparedStateCount() == 0);

  return 0;
}
