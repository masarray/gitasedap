#pragma once

#include "lab/AudioBuffer.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gitasedap::lab
{

inline constexpr std::size_t kCalibrationBandCount = 48;
inline constexpr std::size_t kMaximumCalibrationModeCandidates = 10;

enum class CalibrationIssue : std::uint32_t
{
  None = 0,
  InsufficientDuration = 1U << 0U,
  NonFinitePiezo = 1U << 1U,
  NonFiniteReference = 1U << 2U,
  PiezoClipping = 1U << 3U,
  ReferenceClipping = 1U << 4U,
  PiezoLevelTooLow = 1U << 5U,
  ReferenceLevelTooLow = 1U << 6U,
  WeakAlignment = 1U << 7U,
  InsufficientSpectralSupport = 1U << 8U
};

[[nodiscard]] constexpr CalibrationIssue operator|(
  CalibrationIssue lhs,
  CalibrationIssue rhs
) noexcept
{
  return static_cast<CalibrationIssue>(
    static_cast<std::uint32_t>(lhs)
    | static_cast<std::uint32_t>(rhs)
  );
}

constexpr CalibrationIssue& operator|=(
  CalibrationIssue& lhs,
  CalibrationIssue rhs
) noexcept
{
  lhs = lhs | rhs;
  return lhs;
}

[[nodiscard]] constexpr bool hasCalibrationIssue(
  CalibrationIssue issues,
  CalibrationIssue test
) noexcept
{
  return (
    static_cast<std::uint32_t>(issues)
    & static_cast<std::uint32_t>(test)
  ) != 0U;
}

struct RealGuitarCalibrationOptions
{
  double minimumDurationSeconds{8.0};
  double maximumAnalysisSeconds{60.0};
  double maximumAlignmentLagMilliseconds{60.0};
  std::size_t alignmentDecimation{8};

  double minimumFrequencyHz{65.0};
  double maximumFrequencyHz{10000.0};
  double analysisBandQ{3.0};
  double minimumBandSupportDb{-32.0};

  double clippingThresholdLinear{0.985};
  double minimumRmsDbFs{-55.0};
  double minimumAlignmentScore{0.12};

  double candidateMinimumFrequencyHz{70.0};
  double candidateMaximumFrequencyHz{1500.0};
  double candidateMinimumProminenceDb{0.8};
};

struct CalibrationBandMeasurement
{
  double frequencyHz{0.0};
  double rawTransferDb{0.0};
  double relativeTransferDb{0.0};
  double piezoSupportDb{0.0};
  bool supported{false};
};

struct CalibrationModeCandidate
{
  double frequencyHz{0.0};
  double prominenceDb{0.0};
  double estimatedQ{0.0};
  double confidence{0.0};
};

struct RealGuitarCalibrationReport
{
  double sampleRate{0.0};
  std::size_t analyzedFrames{0};
  double durationSeconds{0.0};

  double piezoPeakDbFs{-200.0};
  double referencePeakDbFs{-200.0};
  double piezoRmsDbFs{-200.0};
  double referenceRmsDbFs{-200.0};

  std::ptrdiff_t estimatedReferenceLagSamples{0};
  double estimatedReferenceLagMilliseconds{0.0};
  double alignmentScore{0.0};

  double transferNormalizationDb{0.0};
  double spectralShapeRmsDb{0.0};
  double spectralShapeMaxAbsDb{0.0};

  double bodyBandMeanDb{0.0};
  double quackBandMeanDb{0.0};
  double airBandMeanDb{0.0};

  std::size_t supportedBandCount{0};

  std::array<
    CalibrationBandMeasurement,
    kCalibrationBandCount
  > bands{};

  std::size_t modeCandidateCount{0};

  std::array<
    CalibrationModeCandidate,
    kMaximumCalibrationModeCandidates
  > modeCandidates{};

  CalibrationIssue issues{CalibrationIssue::None};

  [[nodiscard]] bool captureAcceptedForTuning() const noexcept
  {
    return issues == CalibrationIssue::None;
  }
};

[[nodiscard]] RealGuitarCalibrationReport analyzeRealGuitarPair(
  const AudioBuffer& piezo,
  const AudioBuffer& reference,
  double sampleRate,
  const RealGuitarCalibrationOptions& options = {}
);

} // namespace gitasedap::lab
