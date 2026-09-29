#include "dsp/SourceAdapter.h"

#include <algorithm>
#include <cmath>

namespace gitasedap::dsp
{
namespace
{

constexpr double kRumbleCutoffHz = 30.0;
constexpr double kButterworthQ = 0.7071067811865476;
constexpr double kPrimaryQuackHz = 2600.0;
constexpr double kPrimaryQuackQ = 0.85;
constexpr double kSecondaryHarshHz = 5200.0;
constexpr double kSecondaryHarshQ = 1.10;
constexpr double kEnvelopeFloor = 1.0e-5;
constexpr double kTransientWidth = 0.65;

[[nodiscard]] core::InputSource sanitizeSource(
  core::InputSource source
) noexcept
{
  const int index = static_cast<int>(source);

  if(
    index < 0
    || index >= static_cast<int>(core::InputSource::Count)
  )
  {
    return core::InputSource::ActivePiezo;
  }

  return source;
}

} // namespace

void SourceAdapter::prepare(double sampleRate) noexcept
{
  mSampleRate = std::max(sampleRate, 8000.0);

  mRumbleHighPass.setHighPass(
    mSampleRate,
    kRumbleCutoffHz,
    kButterworthQ
  );

  mPrimaryQuackBand.setBandPass(
    mSampleRate,
    kPrimaryQuackHz,
    kPrimaryQuackQ
  );

  mSecondaryHarshBand.setBandPass(
    mSampleRate,
    kSecondaryHarshHz,
    kSecondaryHarshQ
  );

  mFastEnvelope.prepare(mSampleRate, 0.35, 14.0);
  mSlowEnvelope.prepare(mSampleRate, 10.0, 110.0);

  mPrimaryReduction.prepare(mSampleRate, 0.25, 24.0);
  mSecondaryReduction.prepare(mSampleRate, 0.20, 18.0);

  mInputGain.prepare(mSampleRate, 20.0);
  mAntiQuackAmount.prepare(mSampleRate, 20.0);
  mTransientThreshold.prepare(mSampleRate, 25.0);
  mPrimaryDepth.prepare(mSampleRate, 25.0);
  mSecondaryDepth.prepare(mSampleRate, 25.0);

  const auto profile = profileFor(mSourceType);

  mInputGain.reset(1.0);
  mAntiQuackAmount.reset(1.0);
  mTransientThreshold.reset(profile.transientThreshold);
  mPrimaryDepth.reset(profile.primaryDepth);
  mSecondaryDepth.reset(profile.secondaryDepth);

  reset();
}

void SourceAdapter::reset() noexcept
{
  mRumbleHighPass.reset();
  mPrimaryQuackBand.reset();
  mSecondaryHarshBand.reset();

  mFastEnvelope.reset();
  mSlowEnvelope.reset();
  mPrimaryReduction.reset();
  mSecondaryReduction.reset();

  mInputGain.reset(mInputGain.target());
  mAntiQuackAmount.reset(mAntiQuackAmount.target());
  mTransientThreshold.reset(mTransientThreshold.target());
  mPrimaryDepth.reset(mPrimaryDepth.target());
  mSecondaryDepth.reset(mSecondaryDepth.target());

  beginBlock();
  mLastMeter = {};
}

void SourceAdapter::setSourceType(core::InputSource source) noexcept
{
  const auto safeSource = sanitizeSource(source);

  if(safeSource == mSourceType)
    return;

  mSourceType = safeSource;
  applyProfileTargets(profileFor(mSourceType));
}

void SourceAdapter::setInputTrimDb(double decibels) noexcept
{
  const double safeDb = std::clamp(decibels, -24.0, 18.0);
  mInputGain.setTarget(dbToLinear(safeDb));
}

void SourceAdapter::setAntiQuackAmount(double normalizedAmount) noexcept
{
  mAntiQuackAmount.setTarget(
    std::clamp(normalizedAmount, 0.0, 1.0)
  );
}

void SourceAdapter::beginBlock() noexcept
{
  mBlockPeak = 0.0;
  mBlockSumSquares = 0.0;
  mBlockMaximumTransient = 0.0;
  mBlockMaximumReductionDb = 0.0;
  mBlockSamples = 0;
}

double SourceAdapter::processSample(double input) noexcept
{
  const double finiteInput = std::isfinite(input) ? input : 0.0;
  const double absoluteInput = std::abs(finiteInput);

  mBlockPeak = std::max(mBlockPeak, absoluteInput);
  mBlockSumSquares += finiteInput * finiteInput;
  ++mBlockSamples;

  const double trimmed = finiteInput * mInputGain.next();
  const double protectedInput = mRumbleHighPass.process(trimmed);

  const double fast = mFastEnvelope.process(protectedInput);
  const double slow = mSlowEnvelope.process(protectedInput);

  const double transientRatio = std::max(
    0.0,
    (fast - slow) / (slow + kEnvelopeFloor)
  );

  const double threshold = mTransientThreshold.next();
  const double transientScore = std::clamp(
    (transientRatio - threshold) / kTransientWidth,
    0.0,
    1.0
  );

  mBlockMaximumTransient = std::max(
    mBlockMaximumTransient,
    transientScore
  );

  const double primaryBand = mPrimaryQuackBand.process(protectedInput);
  const double secondaryBand = mSecondaryHarshBand.process(protectedInput);

  const double spectralDenominator = fast + kEnvelopeFloor;

  const double primaryPresence = std::clamp(
    std::abs(primaryBand) / spectralDenominator,
    0.0,
    1.0
  );

  const double secondaryPresence = std::clamp(
    std::abs(secondaryBand) / spectralDenominator,
    0.0,
    1.0
  );

  const double amount = mAntiQuackAmount.next();
  const double primaryDepth = mPrimaryDepth.next();
  const double secondaryDepth = mSecondaryDepth.next();

  const double primaryTarget =
    transientScore * primaryPresence * primaryDepth * amount;

  const double secondaryTarget =
    transientScore * secondaryPresence * secondaryDepth * amount;

  const double primaryReduction =
    std::clamp(mPrimaryReduction.process(primaryTarget), 0.0, 0.60);

  const double secondaryReduction =
    std::clamp(mSecondaryReduction.process(secondaryTarget), 0.0, 0.40);

  const double effectiveReduction = std::max(
    primaryReduction,
    secondaryReduction
  );

  mBlockMaximumReductionDb = std::max(
    mBlockMaximumReductionDb,
    reductionToDb(effectiveReduction)
  );

  const double output =
    protectedInput
    - (primaryBand * primaryReduction)
    - (secondaryBand * secondaryReduction);

  return std::isfinite(output) ? output : 0.0;
}

SourceAdapterMeter SourceAdapter::endBlock() noexcept
{
  SourceAdapterMeter meter;
  meter.inputPeakLinear = mBlockPeak;

  if(mBlockSamples > 0)
  {
    meter.inputRmsLinear = std::sqrt(
      mBlockSumSquares / static_cast<double>(mBlockSamples)
    );
  }

  if(meter.inputPeakLinear > 0.0)
  {
    meter.inputHeadroomDb = std::clamp(
      -20.0 * std::log10(meter.inputPeakLinear),
      -48.0,
      120.0
    );
  }

  meter.maximumTransientScore = mBlockMaximumTransient;
  meter.maximumReductionDb = mBlockMaximumReductionDb;

  mLastMeter = meter;
  return meter;
}

SourceAdapter::SourceProfile SourceAdapter::profileFor(
  core::InputSource source
) noexcept
{
  switch(sanitizeSource(source))
  {
    case core::InputSource::PassivePiezo:
      return SourceProfile{0.14, 0.44, 0.24};

    case core::InputSource::Magnetic:
      return SourceProfile{0.27, 0.14, 0.06};

    case core::InputSource::ActivePiezo:
    default:
      return SourceProfile{0.17, 0.38, 0.20};
  }
}

double SourceAdapter::dbToLinear(double decibels) noexcept
{
  return std::pow(10.0, decibels / 20.0);
}

double SourceAdapter::reductionToDb(double reduction) noexcept
{
  const double remaining = std::max(1.0 - reduction, 1.0e-6);
  return std::max(0.0, -20.0 * std::log10(remaining));
}

void SourceAdapter::applyProfileTargets(SourceProfile profile) noexcept
{
  mTransientThreshold.setTarget(profile.transientThreshold);
  mPrimaryDepth.setTarget(profile.primaryDepth);
  mSecondaryDepth.setTarget(profile.secondaryDepth);
}

} // namespace gitasedap::dsp
