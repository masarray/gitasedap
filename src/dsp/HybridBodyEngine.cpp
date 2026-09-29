#include "dsp/HybridBodyEngine.h"

#include <algorithm>
#include <cmath>

namespace gitasedap::dsp
{

void HybridBodyEngine::prepare(
  double sampleRate,
  double initialBodyAmountNormalized
) noexcept
{
  mBodyAmount.prepare(
    std::max(sampleRate, 8000.0),
    20.0
  );

  mBodyAmount.reset(
    std::clamp(initialBodyAmountNormalized, 0.0, 1.0)
  );

  reset();
}

void HybridBodyEngine::configure(
  const PreparedBodyProfile& prepared
) noexcept
{
  mTransfer.configure(prepared.transfer);
  mModal.configure(prepared.modal);

  mProfileOutputGain =
    std::clamp(prepared.outputGain, 0.0, 2.0);

  mActiveProfileHash = prepared.contentHash;
  mActiveProfileId = prepared.id;
  mHasPreparedProfile = true;
}

void HybridBodyEngine::reset() noexcept
{
  mTransfer.reset();
  mModal.reset();
  mBodyAmount.reset(mBodyAmount.target());
}

void HybridBodyEngine::setBodyAmountNormalized(
  double normalizedAmount
) noexcept
{
  mBodyAmount.setTarget(
    std::clamp(normalizedAmount, 0.0, 1.0)
  );
}

double HybridBodyEngine::processSample(double input) noexcept
{
  const double finiteInput = std::isfinite(input) ? input : 0.0;

  if(!mHasPreparedProfile)
    return finiteInput;

  const double transferred = mTransfer.processSample(finiteInput);
  const double modal = mModal.processSample(finiteInput);

  // Profile transfer keeps tap[0] as the direct identity. Inject only the
  // correction component so BODY=0 is exact conditioned input and increasing
  // BODY progressively adds the static transfer/modal character.
  const double correction =
    ((transferred * mProfileOutputGain) - finiteInput)
    + modal;

  const double bodyAmount = mBodyAmount.next();
  const double output = finiteInput + (correction * bodyAmount);

  return std::isfinite(output) ? output : finiteInput;
}

} // namespace gitasedap::dsp
