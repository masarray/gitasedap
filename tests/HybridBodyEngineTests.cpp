#include "TestSupport.h"
#include "dsp/BodyProfile.h"
#include "dsp/BodyProfileCompiler.h"
#include "dsp/HybridBodyEngine.h"
#include "lab/AudioBuffer.h"
#include "lab/FiniteAudioGuard.h"
#include "lab/SignalGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace gsdsp = gitasedap::dsp;
namespace gslab = gitasedap::lab;

namespace
{

[[nodiscard]] gslab::AudioBuffer renderBody(
  const gslab::AudioBuffer& input,
  double sampleRate,
  const gsdsp::PreparedBodyProfile& profile,
  double bodyAmount
)
{
  gsdsp::HybridBodyEngine engine;
  engine.prepare(sampleRate, bodyAmount);
  engine.configure(profile);

  gslab::AudioBuffer output(
    input.channelCount(),
    input.frameCount()
  );

  for(std::size_t channel = 0;
      channel < input.channelCount();
      ++channel)
  {
    engine.reset();

    for(std::size_t frame = 0;
        frame < input.frameCount();
        ++frame)
    {
      output.channel(channel)[frame] =
        static_cast<float>(
          engine.processSample(
            static_cast<double>(
              input.channel(channel)[frame]
            )
          )
        );
    }
  }

  return output;
}

[[nodiscard]] double maximumDifference(
  const gslab::AudioBuffer& lhs,
  const gslab::AudioBuffer& rhs
)
{
  double maximum = 0.0;

  for(std::size_t channel = 0;
      channel < lhs.channelCount();
      ++channel)
  {
    for(std::size_t frame = 0;
        frame < lhs.frameCount();
        ++frame)
    {
      maximum = std::max(
        maximum,
        std::abs(
          static_cast<double>(lhs.channel(channel)[frame])
          - static_cast<double>(rhs.channel(channel)[frame])
        )
      );
    }
  }

  return maximum;
}

} // namespace

int main()
{
  constexpr std::array<double, 4> sampleRates{
    44100.0,
    48000.0,
    88200.0,
    96000.0
  };

  for(const double sampleRate : sampleRates)
  {
    const auto compiled = gsdsp::compileBodyProfile(
      gsdsp::naturalDevelopmentProfile(),
      sampleRate
    );

    GS_REQUIRE(compiled.ok());

    const auto input = gslab::SignalGenerator::logarithmicSweep(
      1,
      static_cast<std::size_t>(sampleRate),
      sampleRate,
      40.0,
      sampleRate * 0.40,
      0.25F
    );

    const auto bodyOff = renderBody(
      input,
      sampleRate,
      compiled.prepared,
      0.0
    );

    const auto bodyOn = renderBody(
      input,
      sampleRate,
      compiled.prepared,
      1.0
    );

    GS_REQUIRE(gslab::analyzeFiniteAudio(bodyOff).allFinite());
    GS_REQUIRE(gslab::analyzeFiniteAudio(bodyOn).allFinite());

    GS_REQUIRE(maximumDifference(input, bodyOff) <= 1.0e-7);
    GS_REQUIRE(maximumDifference(input, bodyOn) > 1.0e-4);
  }

  {
    constexpr double sampleRate = 48000.0;

    const auto natural = gsdsp::compileBodyProfile(
      gsdsp::naturalDevelopmentProfile(),
      sampleRate
    );

    const auto dreadnought = gsdsp::compileBodyProfile(
      gsdsp::dreadnoughtDevelopmentProfile(),
      sampleRate
    );

    GS_REQUIRE(natural.ok());
    GS_REQUIRE(dreadnought.ok());

    const auto impulse = gslab::SignalGenerator::impulse(
      1,
      static_cast<std::size_t>(sampleRate * 2.0),
      0,
      1.0F
    );

    const auto naturalOutput = renderBody(
      impulse,
      sampleRate,
      natural.prepared,
      1.0
    );

    const auto dreadnoughtOutput = renderBody(
      impulse,
      sampleRate,
      dreadnought.prepared,
      1.0
    );

    GS_REQUIRE(
      gslab::analyzeFiniteAudio(naturalOutput).allFinite()
    );
    GS_REQUIRE(
      gslab::analyzeFiniteAudio(dreadnoughtOutput).allFinite()
    );

    GS_REQUIRE(
      maximumDifference(naturalOutput, dreadnoughtOutput)
      > 1.0e-5
    );

    double lateTailPeak = 0.0;

    const std::size_t lateStart = static_cast<std::size_t>(
      sampleRate * 0.50
    );

    for(std::size_t frame = lateStart;
        frame < naturalOutput.frameCount();
        ++frame)
    {
      lateTailPeak = std::max(
        lateTailPeak,
        std::abs(
          static_cast<double>(
            naturalOutput.channel(0)[frame]
          )
        )
      );
    }

    GS_REQUIRE(lateTailPeak < 1.0e-4);
  }

  {
    constexpr double sampleRate = 48000.0;

    const auto compiled = gsdsp::compileBodyProfile(
      gsdsp::naturalDevelopmentProfile(),
      sampleRate
    );

    GS_REQUIRE(compiled.ok());

    gsdsp::HybridBodyEngine engine;
    engine.prepare(sampleRate, 0.5);
    engine.configure(compiled.prepared);

    for(std::size_t sample = 0; sample < 480000; ++sample)
    {
      if(sample % 97 == 0)
      {
        engine.setBodyAmountNormalized(
          ((sample / 97) % 2) == 0 ? 0.0 : 1.0
        );
      }

      const double input =
        0.2
        * std::sin(
          2.0
          * 3.14159265358979323846
          * 440.0
          * static_cast<double>(sample)
          / sampleRate
        );

      const double output = engine.processSample(input);

      GS_REQUIRE(std::isfinite(output));
      GS_REQUIRE(std::abs(output) < 4.0);
    }
  }

  return 0;
}
