#include "TestSupport.h"
#include "dsp/BodyProfile.h"
#include "dsp/BodyProfileCompiler.h"
#include "dsp/HybridBodyEngine.h"
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

[[nodiscard]] gslab::AudioBuffer render(
  const gslab::AudioBuffer& input,
  double sampleRate,
  const gsdsp::PreparedBodyProfile& profile
)
{
  gsdsp::HybridBodyEngine engine;
  engine.prepare(sampleRate, 1.0);
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

[[nodiscard]] double levelDifferenceDb(
  double referenceRms,
  double candidateRms
)
{
  if(referenceRms <= 0.0 || candidateRms <= 0.0)
    return 0.0;

  return 20.0 * std::log10(candidateRms / referenceRms);
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
  constexpr double sampleRate = 48000.0;
  constexpr std::size_t frames =
    static_cast<std::size_t>(sampleRate * 4.0);

  const auto fixture = gslab::SignalGenerator::pinkNoise(
    1,
    frames,
    0x5034434C564C4D54ULL,
    0.20F
  );

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

  const auto rawOutput = render(
    fixture,
    sampleRate,
    raw.prepared
  );

  const auto naturalOutput = render(
    fixture,
    sampleRate,
    natural.prepared
  );

  const auto dreadnoughtOutput = render(
    fixture,
    sampleRate,
    dreadnought.prepared
  );

  const auto rawReport =
    gslab::analyzeFiniteAudio(rawOutput);
  const auto naturalReport =
    gslab::analyzeFiniteAudio(naturalOutput);
  const auto dreadnoughtReport =
    gslab::analyzeFiniteAudio(dreadnoughtOutput);

  GS_REQUIRE(rawReport.allFinite());
  GS_REQUIRE(naturalReport.allFinite());
  GS_REQUIRE(dreadnoughtReport.allFinite());

  // Raw/P3 is an identity body profile. It allows click-safe comparison using
  // the exact same profile runtime rather than bypassing the body subsystem.
  GS_REQUIRE(maximumDifference(fixture, rawOutput) <= 1.0e-7);

  const double naturalVsRaw = levelDifferenceDb(
    rawReport.rms,
    naturalReport.rms
  );

  const double dreadVsRaw = levelDifferenceDb(
    rawReport.rms,
    dreadnoughtReport.rms
  );

  const double naturalVsDread = levelDifferenceDb(
    naturalReport.rms,
    dreadnoughtReport.rms
  );

  // P4C's level-match gate is an engineering RMS gate on deterministic pink
  // noise. It prevents "louder wins" bias during A/B testing. It is not a
  // substitute for perceptual loudness matching on real guitar material.
  GS_REQUIRE(std::abs(naturalVsRaw) <= 0.35);
  GS_REQUIRE(std::abs(dreadVsRaw) <= 0.35);
  GS_REQUIRE(std::abs(naturalVsDread) <= 0.25);

  GS_REQUIRE(
    maximumDifference(rawOutput, naturalOutput)
    > 1.0e-4
  );

  GS_REQUIRE(
    maximumDifference(rawOutput, dreadnoughtOutput)
    > 1.0e-4
  );

  return 0;
}
