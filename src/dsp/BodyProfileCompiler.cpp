#include "dsp/BodyProfileCompiler.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace gitasedap::dsp
{

BodyProfileCompileResult compileBodyProfile(
  const BodyProfileDefinition& definition,
  double sampleRate
) noexcept
{
  BodyProfileCompileResult result;

  if(!validateBodyProfileDefinition(definition).ok())
  {
    result.error = BodyProfileCompileError::InvalidDefinition;
    return result;
  }

  if(
    !std::isfinite(sampleRate)
    || sampleRate < 8000.0
    || sampleRate > 384000.0
  )
  {
    result.error = BodyProfileCompileError::UnsupportedSampleRate;
    return result;
  }

  result.prepared.id = definition.id;
  result.prepared.schemaVersion = definition.schemaVersion;
  result.prepared.contentHash = hashBodyProfileDefinition(definition);
  result.prepared.sampleRate = sampleRate;
  result.prepared.outputGain = definition.outputGain;

  result.prepared.transfer.tapCount =
    definition.transferTapCount;

  for(std::size_t index = 0;
      index < definition.transferTapCount;
      ++index)
  {
    result.prepared.transfer.taps[index] =
      definition.transferTaps[index];
  }

  result.prepared.modal.modeCount = definition.modeCount;

  const double nyquist = sampleRate * 0.5;

  for(std::size_t index = 0;
      index < definition.modeCount;
      ++index)
  {
    const auto& source = definition.modes[index];

    if(source.frequencyHz >= (nyquist * 0.95))
    {
      result.error = BodyProfileCompileError::ModeAboveNyquist;
      return result;
    }

    const double omega =
      2.0 * std::numbers::pi * source.frequencyHz / sampleRate;

    const double sine = std::sin(omega);
    const double cosine = std::cos(omega);
    const double alpha = sine / (2.0 * source.q);
    const double a0 = 1.0 + alpha;
    const double inverseA0 = 1.0 / a0;

    auto& destination = result.prepared.modal.modes[index];

    // Constant-skirt band-pass, then explicitly scaled by profile gain.
    destination.b0 = alpha * inverseA0;
    destination.b1 = 0.0;
    destination.b2 = -alpha * inverseA0;
    destination.a1 = (-2.0 * cosine) * inverseA0;
    destination.a2 = (1.0 - alpha) * inverseA0;
    destination.gain = source.gain;
  }

  return result;
}

} // namespace gitasedap::dsp
