#pragma once

#include "dsp/BodyProfile.h"
#include "dsp/ModalBodyBank.h"
#include "dsp/TransferFilter.h"

#include <cstdint>

namespace gitasedap::dsp
{

struct PreparedBodyProfile
{
  BodyProfileId id{BodyProfileId::NaturalDevelopment};
  std::uint32_t schemaVersion{kBodyProfileSchemaVersion};
  std::uint64_t contentHash{0};
  double sampleRate{48000.0};

  PreparedTransferFilter transfer;
  PreparedModalBodyBank modal;
  double outputGain{1.0};
};

enum class BodyProfileCompileError
{
  None,
  InvalidDefinition,
  UnsupportedSampleRate,
  ModeAboveNyquist
};

struct BodyProfileCompileResult
{
  BodyProfileCompileError error{BodyProfileCompileError::None};
  PreparedBodyProfile prepared;

  [[nodiscard]] bool ok() const noexcept
  {
    return error == BodyProfileCompileError::None;
  }
};

[[nodiscard]] BodyProfileCompileResult compileBodyProfile(
  const BodyProfileDefinition& definition,
  double sampleRate
) noexcept;

} // namespace gitasedap::dsp
