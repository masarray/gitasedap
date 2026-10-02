#include "TestSupport.h"
#include "lab/RealGuitarCalibration.h"
#include "lab/SignalGenerator.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>

namespace gslab = gitasedap::lab;

namespace
{

struct Resonator
{
  double b0{0.0};
  double b2{0.0};
  double a1{0.0};
  double a2{0.0};
  double z1{0.0};
  double z2{0.0};

  [[nodiscard]] double process(double input) noexcept
  {
    const double output = (b0 * input) + z1;

    z1 = (-a1 * output) + z2;
    z2 = (b2 * input) - (a2 * output);

    return output;
  }
};

[[nodiscard]] Resonator makeResonator(
  double sampleRate,
  double frequencyHz,
  double q
)
{
  const double omega =
    2.0 * std::numbers::pi * frequencyHz / sampleRate;

  const double alpha =
    std::sin(omega) / (2.0 * q);

  const double a0 = 1.0 + alpha;
  const double inverseA0 = 1.0 / a0;

  Resonator result;
  result.b0 = alpha * inverseA0;
  result.b2 = -alpha * inverseA0;
  result.a1 =
    (-2.0 * std::cos(omega)) * inverseA0;
  result.a2 =
    (1.0 - alpha) * inverseA0;

  return result;
}

[[nodiscard]] gslab::AudioBuffer makeReference(
  const gslab::AudioBuffer& piezo,
  double sampleRate,
  std::size_t lagSamples
)
{
  gslab::AudioBuffer reference(
    1,
    piezo.frameCount()
  );

  auto resonator =
    makeResonator(sampleRate, 220.0, 8.0);

  for(std::size_t frame = 0;
      frame < piezo.frameCount();
      ++frame)
  {
    const double source =
      static_cast<double>(piezo.channel(0)[frame]);

    const double colored =
      (source * 0.72)
      + (resonator.process(source) * 0.55);

    const auto destination = frame + lagSamples;

    if(destination < reference.frameCount())
    {
      reference.channel(0)[destination] =
        static_cast<float>(colored);
    }
  }

  return reference;
}

} // namespace

int main()
{
  constexpr double sampleRate = 48000.0;
  constexpr std::size_t frames =
    static_cast<std::size_t>(sampleRate * 12.0);
  constexpr std::size_t lagSamples = 144;

  const auto piezo = gslab::SignalGenerator::pinkNoise(
    1,
    frames,
    0x50344443414C4942ULL,
    0.20F
  );

  const auto reference = makeReference(
    piezo,
    sampleRate,
    lagSamples
  );

  const auto report = gslab::analyzeRealGuitarPair(
    piezo,
    reference,
    sampleRate
  );

  GS_REQUIRE(report.captureAcceptedForTuning());
  GS_REQUIRE(report.supportedBandCount >= 30);
  GS_REQUIRE(report.alignmentScore > 0.70);

  GS_REQUIRE(
    std::abs(
      report.estimatedReferenceLagSamples
      - static_cast<std::ptrdiff_t>(lagSamples)
    )
    <= 16
  );

  GS_REQUIRE(report.spectralShapeRmsDb > 0.1);
  GS_REQUIRE(report.modeCandidateCount > 0);

  bool foundBodyCandidate = false;

  for(std::size_t index = 0;
      index < report.modeCandidateCount;
      ++index)
  {
    const auto& candidate =
      report.modeCandidates[index];

    GS_REQUIRE(std::isfinite(candidate.frequencyHz));
    GS_REQUIRE(std::isfinite(candidate.prominenceDb));
    GS_REQUIRE(std::isfinite(candidate.estimatedQ));
    GS_REQUIRE(std::isfinite(candidate.confidence));

    if(
      candidate.frequencyHz >= 170.0
      && candidate.frequencyHz <= 300.0
    )
    {
      foundBodyCandidate = true;
    }
  }

  GS_REQUIRE(foundBodyCandidate);

  {
    auto clipped = piezo;

    for(std::size_t frame = 0;
        frame < clipped.frameCount();
        frame += 1000)
    {
      clipped.channel(0)[frame] = 1.0F;
    }

    const auto clippedReport =
      gslab::analyzeRealGuitarPair(
        clipped,
        reference,
        sampleRate
      );

    GS_REQUIRE(
      gslab::hasCalibrationIssue(
        clippedReport.issues,
        gslab::CalibrationIssue::PiezoClipping
      )
    );

    GS_REQUIRE(
      !clippedReport.captureAcceptedForTuning()
    );
  }

  {
    gslab::RealGuitarCalibrationOptions options;
    options.minimumDurationSeconds = 20.0;

    const auto shortReport =
      gslab::analyzeRealGuitarPair(
        piezo,
        reference,
        sampleRate,
        options
      );

    GS_REQUIRE(
      gslab::hasCalibrationIssue(
        shortReport.issues,
        gslab::CalibrationIssue::InsufficientDuration
      )
    );
  }

  return 0;
}
