#include "lab/RealGuitarCalibration.h"

#include "lab/FiniteAudioGuard.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace gitasedap::lab
{
namespace
{

constexpr double kSilenceFloor = 1.0e-18;

[[nodiscard]] double linearToDb(double linear) noexcept
{
  if(!std::isfinite(linear) || linear <= kSilenceFloor)
    return -200.0;

  return 20.0 * std::log10(linear);
}

[[nodiscard]] double powerRatioToDb(
  double numerator,
  double denominator
) noexcept
{
  if(
    !std::isfinite(numerator)
    || !std::isfinite(denominator)
    || numerator <= kSilenceFloor
    || denominator <= kSilenceFloor
  )
  {
    return 0.0;
  }

  return 10.0 * std::log10(numerator / denominator);
}

[[nodiscard]] std::vector<double> makeEnvelope(
  const AudioBuffer& audio,
  std::size_t maximumFrames,
  std::size_t decimation
)
{
  const auto source = audio.channel(0);
  const auto frames = std::min(source.size(), maximumFrames);
  const auto safeDecimation = std::max<std::size_t>(1, decimation);

  const auto bins = frames / safeDecimation;
  std::vector<double> envelope(bins, 0.0);

  for(std::size_t bin = 0; bin < bins; ++bin)
  {
    const auto start = bin * safeDecimation;
    double sum = 0.0;

    for(std::size_t offset = 0; offset < safeDecimation; ++offset)
    {
      const auto frame = start + offset;
      const double current = static_cast<double>(source[frame]);

      const double previous =
        frame == 0
          ? current
          : static_cast<double>(source[frame - 1]);

      sum += std::abs(current - previous);
    }

    envelope[bin] = sum / static_cast<double>(safeDecimation);
  }

  if(envelope.empty())
    return envelope;

  double mean = 0.0;

  for(const double value : envelope)
    mean += value;

  mean /= static_cast<double>(envelope.size());

  for(auto& value : envelope)
    value -= mean;

  return envelope;
}

struct LagEstimate
{
  std::ptrdiff_t lagSamples{0};
  double score{0.0};
};

[[nodiscard]] LagEstimate estimateEnvelopeLag(
  const AudioBuffer& piezo,
  const AudioBuffer& reference,
  double sampleRate,
  const RealGuitarCalibrationOptions& options
)
{
  const auto safeDecimation =
    std::max<std::size_t>(1, options.alignmentDecimation);

  const auto maximumFrames = static_cast<std::size_t>(
    std::min(
      options.maximumAnalysisSeconds,
      12.0
    )
    * sampleRate
  );

  const auto piezoEnvelope =
    makeEnvelope(piezo, maximumFrames, safeDecimation);

  const auto referenceEnvelope =
    makeEnvelope(reference, maximumFrames, safeDecimation);

  const auto bins = std::min(
    piezoEnvelope.size(),
    referenceEnvelope.size()
  );

  if(bins < 32)
    return {};

  const auto maximumLagSamples = static_cast<std::ptrdiff_t>(
    std::llround(
      sampleRate
      * options.maximumAlignmentLagMilliseconds
      * 0.001
    )
  );

  const auto maximumLagBins = std::max<std::ptrdiff_t>(
    1,
    maximumLagSamples
      / static_cast<std::ptrdiff_t>(safeDecimation)
  );

  double bestScore = -1.0;
  std::ptrdiff_t bestLagBins = 0;

  for(std::ptrdiff_t lag = -maximumLagBins;
      lag <= maximumLagBins;
      ++lag)
  {
    const std::size_t piezoStart =
      lag < 0
        ? static_cast<std::size_t>(-lag)
        : 0U;

    const std::size_t referenceStart =
      lag > 0
        ? static_cast<std::size_t>(lag)
        : 0U;

    if(
      piezoStart >= bins
      || referenceStart >= bins
    )
    {
      continue;
    }

    const auto count = std::min(
      bins - piezoStart,
      bins - referenceStart
    );

    if(count < 32)
      continue;

    double dot = 0.0;
    double piezoEnergy = 0.0;
    double referenceEnergy = 0.0;

    for(std::size_t index = 0; index < count; ++index)
    {
      const double a = piezoEnvelope[piezoStart + index];
      const double b = referenceEnvelope[referenceStart + index];

      dot += a * b;
      piezoEnergy += a * a;
      referenceEnergy += b * b;
    }

    const double denominator =
      std::sqrt(piezoEnergy * referenceEnergy);

    if(denominator <= kSilenceFloor)
      continue;

    const double score = dot / denominator;

    if(score > bestScore)
    {
      bestScore = score;
      bestLagBins = lag;
    }
  }

  if(bestScore < 0.0)
    return {};

  return LagEstimate{
    bestLagBins * static_cast<std::ptrdiff_t>(safeDecimation),
    std::clamp(bestScore, 0.0, 1.0)
  };
}

struct BandPower
{
  double piezo{0.0};
  double reference{0.0};
};

struct Biquad
{
  double b0{0.0};
  double b1{0.0};
  double b2{0.0};
  double a1{0.0};
  double a2{0.0};
  double z1{0.0};
  double z2{0.0};

  [[nodiscard]] double process(double input) noexcept
  {
    const double output = (b0 * input) + z1;

    z1 =
      (b1 * input)
      - (a1 * output)
      + z2;

    z2 =
      (b2 * input)
      - (a2 * output);

    return output;
  }
};

[[nodiscard]] Biquad makeBandPass(
  double sampleRate,
  double frequencyHz,
  double q
)
{
  const double omega =
    2.0
    * std::numbers::pi
    * frequencyHz
    / sampleRate;

  const double sine = std::sin(omega);
  const double cosine = std::cos(omega);
  const double alpha = sine / (2.0 * q);
  const double a0 = 1.0 + alpha;
  const double inverseA0 = 1.0 / a0;

  Biquad filter;
  filter.b0 = alpha * inverseA0;
  filter.b1 = 0.0;
  filter.b2 = -alpha * inverseA0;
  filter.a1 = (-2.0 * cosine) * inverseA0;
  filter.a2 = (1.0 - alpha) * inverseA0;
  return filter;
}

[[nodiscard]] BandPower measureBandPower(
  const AudioBuffer& piezo,
  const AudioBuffer& reference,
  std::size_t piezoStart,
  std::size_t referenceStart,
  std::size_t frameCount,
  double sampleRate,
  double frequencyHz,
  double q
)
{
  Biquad piezoFilter =
    makeBandPass(sampleRate, frequencyHz, q);

  Biquad referenceFilter =
    makeBandPass(sampleRate, frequencyHz, q);

  const auto piezoSamples = piezo.channel(0);
  const auto referenceSamples = reference.channel(0);

  const auto settleFrames = std::min<std::size_t>(
    frameCount / 4U,
    static_cast<std::size_t>(sampleRate * 0.05)
  );

  double piezoPower = 0.0;
  double referencePower = 0.0;
  std::size_t measured = 0;

  for(std::size_t frame = 0; frame < frameCount; ++frame)
  {
    const double p = piezoFilter.process(
      static_cast<double>(
        piezoSamples[piezoStart + frame]
      )
    );

    const double r = referenceFilter.process(
      static_cast<double>(
        referenceSamples[referenceStart + frame]
      )
    );

    if(frame < settleFrames)
      continue;

    piezoPower += p * p;
    referencePower += r * r;
    ++measured;
  }

  if(measured == 0)
    return {};

  const double inverse =
    1.0 / static_cast<double>(measured);

  return BandPower{
    piezoPower * inverse,
    referencePower * inverse
  };
}

[[nodiscard]] double median(std::vector<double> values)
{
  if(values.empty())
    return 0.0;

  const auto middle = values.begin()
    + static_cast<std::ptrdiff_t>(values.size() / 2U);

  std::nth_element(values.begin(), middle, values.end());

  if((values.size() & 1U) != 0U)
    return *middle;

  const double upper = *middle;
  const auto lowerMiddle = std::max_element(
    values.begin(),
    middle
  );

  return 0.5 * (upper + *lowerMiddle);
}

[[nodiscard]] double meanBand(
  const RealGuitarCalibrationReport& report,
  double lowHz,
  double highHz
) noexcept
{
  double weighted = 0.0;
  double weight = 0.0;

  for(const auto& band : report.bands)
  {
    if(
      !band.supported
      || band.frequencyHz < lowHz
      || band.frequencyHz > highHz
    )
    {
      continue;
    }

    const double supportWeight = std::clamp(
      1.0
      + (band.piezoSupportDb / 32.0),
      0.10,
      1.0
    );

    weighted += band.relativeTransferDb * supportWeight;
    weight += supportWeight;
  }

  return weight > 0.0 ? weighted / weight : 0.0;
}

void extractModeCandidates(
  RealGuitarCalibrationReport& report,
  const RealGuitarCalibrationOptions& options
)
{
  std::array<double, kCalibrationBandCount> smooth{};

  for(std::size_t index = 0; index < report.bands.size(); ++index)
  {
    double sum = report.bands[index].relativeTransferDb;
    double count = 1.0;

    if(index > 0)
    {
      sum += report.bands[index - 1].relativeTransferDb;
      count += 1.0;
    }

    if(index + 1 < report.bands.size())
    {
      sum += report.bands[index + 1].relativeTransferDb;
      count += 1.0;
    }

    smooth[index] = sum / count;
  }

  struct RankedCandidate
  {
    CalibrationModeCandidate candidate;
    double rank{0.0};
  };

  std::vector<RankedCandidate> candidates;

  for(std::size_t index = 1;
      index + 1 < report.bands.size();
      ++index)
  {
    const auto& band = report.bands[index];

    if(
      !band.supported
      || band.frequencyHz < options.candidateMinimumFrequencyHz
      || band.frequencyHz > options.candidateMaximumFrequencyHz
    )
    {
      continue;
    }

    const double peak = smooth[index];

    if(
      peak <= smooth[index - 1]
      || peak <= smooth[index + 1]
    )
    {
      continue;
    }

    const double localBase = std::max(
      smooth[index - 1],
      smooth[index + 1]
    );

    const double prominence = peak - localBase;

    if(prominence < options.candidateMinimumProminenceDb)
      continue;

    std::size_t left = index;
    std::size_t right = index;
    const double halfPower = peak - 3.0;

    while(
      left > 0
      && smooth[left] > halfPower
    )
    {
      --left;
    }

    while(
      right + 1 < report.bands.size()
      && smooth[right] > halfPower
    )
    {
      ++right;
    }

    double q = 4.0;

    if(right > left)
    {
      const double width =
        report.bands[right].frequencyHz
        - report.bands[left].frequencyHz;

      if(width > 0.0)
      {
        q = std::clamp(
          band.frequencyHz / width,
          0.5,
          30.0
        );
      }
    }

    const double supportConfidence = std::clamp(
      1.0
      + (band.piezoSupportDb / 32.0),
      0.0,
      1.0
    );

    const double prominenceConfidence = std::clamp(
      prominence / 6.0,
      0.0,
      1.0
    );

    const double confidence =
      supportConfidence
      * prominenceConfidence
      * std::max(report.alignmentScore, 0.25);

    candidates.push_back(
      RankedCandidate{
        CalibrationModeCandidate{
          band.frequencyHz,
          prominence,
          q,
          confidence
        },
        prominence * (0.5 + confidence)
      }
    );
  }

  std::sort(
    candidates.begin(),
    candidates.end(),
    [](const RankedCandidate& lhs, const RankedCandidate& rhs) {
      return lhs.rank > rhs.rank;
    }
  );

  report.modeCandidateCount = std::min(
    candidates.size(),
    kMaximumCalibrationModeCandidates
  );

  for(std::size_t index = 0;
      index < report.modeCandidateCount;
      ++index)
  {
    report.modeCandidates[index] =
      candidates[index].candidate;
  }

  std::sort(
    report.modeCandidates.begin(),
    report.modeCandidates.begin()
      + static_cast<std::ptrdiff_t>(report.modeCandidateCount),
    [](const CalibrationModeCandidate& lhs,
       const CalibrationModeCandidate& rhs) {
      return lhs.frequencyHz < rhs.frequencyHz;
    }
  );
}

} // namespace

RealGuitarCalibrationReport analyzeRealGuitarPair(
  const AudioBuffer& piezo,
  const AudioBuffer& reference,
  double sampleRate,
  const RealGuitarCalibrationOptions& options
)
{
  if(
    piezo.channelCount() == 0
    || reference.channelCount() == 0
  )
  {
    throw std::invalid_argument(
      "calibration audio must have at least one channel"
    );
  }

  if(
    !std::isfinite(sampleRate)
    || sampleRate < 8000.0
    || sampleRate > 384000.0
  )
  {
    throw std::invalid_argument(
      "calibration sample rate is unsupported"
    );
  }

  if(
    !std::isfinite(options.minimumDurationSeconds)
    || options.minimumDurationSeconds <= 0.0
    || !std::isfinite(options.maximumAnalysisSeconds)
    || options.maximumAnalysisSeconds
        < options.minimumDurationSeconds
    || options.alignmentDecimation == 0
    || options.analysisBandQ <= 0.0
  )
  {
    throw std::invalid_argument(
      "calibration options are invalid"
    );
  }

  RealGuitarCalibrationReport report;
  report.sampleRate = sampleRate;

  const auto piezoFinite = analyzeFiniteAudio(piezo);
  const auto referenceFinite = analyzeFiniteAudio(reference);

  report.piezoPeakDbFs =
    linearToDb(piezoFinite.peakAbsolute);
  report.referencePeakDbFs =
    linearToDb(referenceFinite.peakAbsolute);
  report.piezoRmsDbFs =
    linearToDb(piezoFinite.rms);
  report.referenceRmsDbFs =
    linearToDb(referenceFinite.rms);

  if(!piezoFinite.allFinite())
    report.issues |= CalibrationIssue::NonFinitePiezo;

  if(!referenceFinite.allFinite())
    report.issues |= CalibrationIssue::NonFiniteReference;

  if(
    piezoFinite.peakAbsolute
    >= options.clippingThresholdLinear
  )
  {
    report.issues |= CalibrationIssue::PiezoClipping;
  }

  if(
    referenceFinite.peakAbsolute
    >= options.clippingThresholdLinear
  )
  {
    report.issues |= CalibrationIssue::ReferenceClipping;
  }

  if(report.piezoRmsDbFs < options.minimumRmsDbFs)
    report.issues |= CalibrationIssue::PiezoLevelTooLow;

  if(report.referenceRmsDbFs < options.minimumRmsDbFs)
    report.issues |= CalibrationIssue::ReferenceLevelTooLow;

  const auto lag = estimateEnvelopeLag(
    piezo,
    reference,
    sampleRate,
    options
  );

  report.estimatedReferenceLagSamples = lag.lagSamples;
  report.estimatedReferenceLagMilliseconds =
    (
      static_cast<double>(lag.lagSamples)
      * 1000.0
      / sampleRate
    );

  report.alignmentScore = lag.score;

  if(lag.score < options.minimumAlignmentScore)
    report.issues |= CalibrationIssue::WeakAlignment;

  const std::size_t piezoStart =
    lag.lagSamples < 0
      ? static_cast<std::size_t>(-lag.lagSamples)
      : 0U;

  const std::size_t referenceStart =
    lag.lagSamples > 0
      ? static_cast<std::size_t>(lag.lagSamples)
      : 0U;

  const auto availablePiezo =
    piezo.frameCount() > piezoStart
      ? piezo.frameCount() - piezoStart
      : 0U;

  const auto availableReference =
    reference.frameCount() > referenceStart
      ? reference.frameCount() - referenceStart
      : 0U;

  const auto availableFrames = std::min(
    availablePiezo,
    availableReference
  );

  const auto maximumFrames = static_cast<std::size_t>(
    options.maximumAnalysisSeconds * sampleRate
  );

  report.analyzedFrames = std::min(
    availableFrames,
    maximumFrames
  );

  report.durationSeconds =
    static_cast<double>(report.analyzedFrames)
    / sampleRate;

  if(report.durationSeconds < options.minimumDurationSeconds)
    report.issues |= CalibrationIssue::InsufficientDuration;

  const double maximumFrequency = std::min(
    options.maximumFrequencyHz,
    sampleRate * 0.42
  );

  const double frequencyRatio = std::pow(
    maximumFrequency / options.minimumFrequencyHz,
    1.0
      / static_cast<double>(kCalibrationBandCount - 1U)
  );

  std::array<BandPower, kCalibrationBandCount> powers{};

  double maximumPiezoPower = 0.0;
  double frequency = options.minimumFrequencyHz;

  for(std::size_t index = 0;
      index < kCalibrationBandCount;
      ++index)
  {
    report.bands[index].frequencyHz = frequency;

    powers[index] = measureBandPower(
      piezo,
      reference,
      piezoStart,
      referenceStart,
      report.analyzedFrames,
      sampleRate,
      frequency,
      options.analysisBandQ
    );

    maximumPiezoPower = std::max(
      maximumPiezoPower,
      powers[index].piezo
    );

    frequency *= frequencyRatio;
  }

  std::vector<double> normalizationValues;
  normalizationValues.reserve(kCalibrationBandCount);

  for(std::size_t index = 0;
      index < kCalibrationBandCount;
      ++index)
  {
    auto& band = report.bands[index];

    band.piezoSupportDb = powerRatioToDb(
      powers[index].piezo,
      maximumPiezoPower
    );

    band.rawTransferDb = powerRatioToDb(
      powers[index].reference,
      powers[index].piezo
    );

    band.supported =
      powers[index].piezo > kSilenceFloor
      && powers[index].reference > kSilenceFloor
      && band.piezoSupportDb >= options.minimumBandSupportDb;

    if(band.supported)
    {
      ++report.supportedBandCount;

      if(
        band.frequencyHz >= 180.0
        && band.frequencyHz <= 2500.0
      )
      {
        normalizationValues.push_back(
          band.rawTransferDb
        );
      }
    }
  }

  if(normalizationValues.size() < 4U)
  {
    normalizationValues.clear();

    for(const auto& band : report.bands)
    {
      if(band.supported)
      {
        normalizationValues.push_back(
          band.rawTransferDb
        );
      }
    }
  }

  report.transferNormalizationDb =
    median(std::move(normalizationValues));

  double errorSquares = 0.0;
  double maximumAbsolute = 0.0;
  std::size_t errorCount = 0;

  for(auto& band : report.bands)
  {
    band.relativeTransferDb =
      band.rawTransferDb
      - report.transferNormalizationDb;

    if(!band.supported)
      continue;

    errorSquares +=
      band.relativeTransferDb
      * band.relativeTransferDb;

    maximumAbsolute = std::max(
      maximumAbsolute,
      std::abs(band.relativeTransferDb)
    );

    ++errorCount;
  }

  if(errorCount > 0)
  {
    report.spectralShapeRmsDb = std::sqrt(
      errorSquares / static_cast<double>(errorCount)
    );

    report.spectralShapeMaxAbsDb = maximumAbsolute;
  }

  if(report.supportedBandCount < 12U)
  {
    report.issues |=
      CalibrationIssue::InsufficientSpectralSupport;
  }

  report.bodyBandMeanDb =
    meanBand(report, 80.0, 500.0);

  report.quackBandMeanDb =
    meanBand(report, 1500.0, 4500.0);

  report.airBandMeanDb =
    meanBand(report, 4500.0, maximumFrequency);

  extractModeCandidates(report, options);

  return report;
}

} // namespace gitasedap::lab
