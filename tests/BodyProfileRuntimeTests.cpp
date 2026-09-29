#include "TestSupport.h"
#include "dsp/BodyProfile.h"
#include "dsp/BodyProfileCompiler.h"
#include "runtime/BodyProfileRuntime.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <cstddef>
#include <cstdint>

namespace gsdsp = gitasedap::dsp;
namespace gsr = gitasedap::runtime;

namespace
{

[[nodiscard]] gsr::BodyProfileRuntime::PreparedHandle prepare(
  gsr::BodyProfileRuntime::RequestToken token,
  const gsdsp::BodyProfileDefinition& definition,
  double sampleRate
)
{
  gsdsp::BodyProfileCompileError error{};

  auto handle = gsr::BodyProfileRuntime::prepareProfile(
    token,
    definition,
    sampleRate,
    &error
  );

  if(error != gsdsp::BodyProfileCompileError::None)
    return nullptr;

  return handle;
}

} // namespace

int main()
{
  constexpr double sampleRate = 48000.0;

  gsr::BodyProfileRuntime runtime;
  runtime.prepare(sampleRate, 1.0);

  {
    const auto staleToken = runtime.beginProfileRequest();
    auto stale = prepare(
      staleToken,
      gsdsp::naturalDevelopmentProfile(),
      sampleRate
    );

    const auto newestToken = runtime.beginProfileRequest();
    auto newest = prepare(
      newestToken,
      gsdsp::dreadnoughtDevelopmentProfile(),
      sampleRate
    );

    GS_REQUIRE(stale != nullptr);
    GS_REQUIRE(newest != nullptr);

    GS_REQUIRE(
      runtime.publish(std::move(stale))
      == gsr::BodyProfileRuntime::PublishResult::StaleGeneration
    );

    GS_REQUIRE(
      runtime.publish(std::move(newest))
      == gsr::BodyProfileRuntime::PublishResult::Published
    );

    runtime.beginAudioBlock();

    GS_REQUIRE(runtime.appliedProfileHash() != 0);
    GS_REQUIRE(!runtime.isTransitioning());
  }

  double previous = 0.0;

  for(std::size_t sample = 0; sample < 4096; ++sample)
    previous = runtime.processSample(0.10);

  const auto naturalHash =
    gsdsp::hashBodyProfileDefinition(
      gsdsp::naturalDevelopmentProfile()
    );

  {
    const auto token = runtime.beginProfileRequest();
    auto natural = prepare(
      token,
      gsdsp::naturalDevelopmentProfile(),
      sampleRate
    );

    GS_REQUIRE(natural != nullptr);

    GS_REQUIRE(
      runtime.publish(std::move(natural))
      == gsr::BodyProfileRuntime::PublishResult::Published
    );

    runtime.beginAudioBlock();

    GS_REQUIRE(runtime.isTransitioning());
    GS_REQUIRE(runtime.appliedProfileHash() == naturalHash);

    double maximumStep = 0.0;

    for(std::size_t sample = 0;
        sample < runtime.transitionSamples();
        ++sample)
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
    GS_REQUIRE(maximumStep < 0.01);
  }

  {
    const auto dreadToken = runtime.beginProfileRequest();
    auto dread = prepare(
      dreadToken,
      gsdsp::dreadnoughtDevelopmentProfile(),
      sampleRate
    );

    GS_REQUIRE(dread != nullptr);

    GS_REQUIRE(
      runtime.publish(std::move(dread))
      == gsr::BodyProfileRuntime::PublishResult::Published
    );

    runtime.beginAudioBlock();
    GS_REQUIRE(runtime.isTransitioning());

    const auto newestToken = runtime.beginProfileRequest();
    auto newestNatural = prepare(
      newestToken,
      gsdsp::naturalDevelopmentProfile(),
      sampleRate
    );

    GS_REQUIRE(newestNatural != nullptr);

    GS_REQUIRE(
      runtime.publish(std::move(newestNatural))
      == gsr::BodyProfileRuntime::PublishResult::Published
    );

    runtime.beginAudioBlock();
    GS_REQUIRE(runtime.isTransitioning());

    for(std::size_t sample = 0;
        sample < runtime.transitionSamples();
        ++sample)
    {
      GS_REQUIRE(std::isfinite(runtime.processSample(0.08)));
    }

    GS_REQUIRE(!runtime.isTransitioning());

    runtime.beginAudioBlock();

    GS_REQUIRE(runtime.isTransitioning());
    GS_REQUIRE(runtime.appliedProfileHash() == naturalHash);

    for(std::size_t sample = 0;
        sample < runtime.transitionSamples();
        ++sample)
    {
      GS_REQUIRE(std::isfinite(runtime.processSample(0.08)));
    }

    GS_REQUIRE(!runtime.isTransitioning());
  }

  for(std::size_t iteration = 0; iteration < 3000; ++iteration)
  {
    const auto token = runtime.beginProfileRequest();
    const bool useNatural = (iteration % 2U) == 0;

    auto handle = prepare(
      token,
      useNatural
        ? gsdsp::naturalDevelopmentProfile()
        : gsdsp::dreadnoughtDevelopmentProfile(),
      sampleRate
    );

    GS_REQUIRE(handle != nullptr);

    GS_REQUIRE(
      runtime.publish(std::move(handle))
      == gsr::BodyProfileRuntime::PublishResult::Published
    );

    runtime.beginAudioBlock();

    for(std::size_t frame = 0; frame < 64; ++frame)
    {
      const double input =
        0.12
        * std::sin(
          2.0
          * 3.14159265358979323846
          * 220.0
          * static_cast<double>((iteration * 64) + frame)
          / sampleRate
        );

      GS_REQUIRE(
        std::isfinite(runtime.processSample(input))
      );
    }

    (void) runtime.drainReclaimable();
    GS_REQUIRE(runtime.ownedPreparedStateCount() <= 1);
  }

  runtime.shutdownAfterAudioStopped();

  GS_REQUIRE(runtime.ownedPreparedStateCount() == 0);
  GS_REQUIRE(runtime.activeGeneration() == 0);

  return 0;
}
