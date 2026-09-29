#pragma once

#include "core/ParameterSpec.h"
#include "dsp/Biquad.h"
#include "dsp/EnvelopeFollower.h"
#include "dsp/Smoothing.h"

#include <cstddef>

namespace gitasedap::dsp
{

struct SourceAdapterMeter
{
  double inputPeakLinear{0.0};
  double inputRmsLinear{0.0};
  double inputHeadroomDb{120.0};
  double maximumTransientScore{0.0};
  double maximumReductionDb{0.0};
};

class SourceAdapter
{
public:
  void prepare(double sampleRate) noexcept;
  void reset() noexcept;

  void setSourceType(core::InputSource source) noexcept;
  void setInputTrimDb(double decibels) noexcept;
  void setAntiQuackAmount(double normalizedAmount) noexcept;

  void beginBlock() noexcept;
  [[nodiscard]] double processSample(double input) noexcept;
  [[nodiscard]] SourceAdapterMeter endBlock() noexcept;

  [[nodiscard]] SourceAdapterMeter lastMeter() const noexcept
  {
    return mLastMeter;
  }

  [[nodiscard]] core::InputSource sourceType() const noexcept
  {
    return mSourceType;
  }

private:
  struct SourceProfile
  {
    double transientThreshold{0.2};
    double primaryDepth{0.3};
    double secondaryDepth{0.15};
  };

  [[nodiscard]] static SourceProfile profileFor(
    core::InputSource source
  ) noexcept;

  [[nodiscard]] static double dbToLinear(double decibels) noexcept;
  [[nodiscard]] static double reductionToDb(double reduction) noexcept;

  void applyProfileTargets(SourceProfile profile) noexcept;

  double mSampleRate{48000.0};
  core::InputSource mSourceType{core::InputSource::ActivePiezo};

  Biquad mRumbleHighPass;
  Biquad mPrimaryQuackBand;
  Biquad mSecondaryHarshBand;

  EnvelopeFollower mFastEnvelope;
  EnvelopeFollower mSlowEnvelope;
  EnvelopeFollower mPrimaryReduction;
  EnvelopeFollower mSecondaryReduction;

  LinearSmoother mInputGain;
  LinearSmoother mAntiQuackAmount;
  LinearSmoother mTransientThreshold;
  LinearSmoother mPrimaryDepth;
  LinearSmoother mSecondaryDepth;

  double mBlockPeak{0.0};
  double mBlockSumSquares{0.0};
  double mBlockMaximumTransient{0.0};
  double mBlockMaximumReductionDb{0.0};
  std::size_t mBlockSamples{0};

  SourceAdapterMeter mLastMeter;
};

} // namespace gitasedap::dsp
