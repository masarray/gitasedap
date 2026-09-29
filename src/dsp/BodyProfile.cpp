#include "dsp/BodyProfile.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

namespace gitasedap::dsp
{
namespace
{

constexpr std::uint64_t kFnvOffset = 14695981039346656037ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;

void hashByte(std::uint64_t& hash, std::uint8_t value) noexcept
{
  hash ^= static_cast<std::uint64_t>(value);
  hash *= kFnvPrime;
}

template <typename UInt>
void hashUnsignedLittleEndian(std::uint64_t& hash, UInt value) noexcept
{
  static_assert(std::is_unsigned_v<UInt>);

  for(std::size_t byte = 0; byte < sizeof(UInt); ++byte)
  {
    hashByte(
      hash,
      static_cast<std::uint8_t>(
        (value >> (byte * 8U)) & static_cast<UInt>(0xFFU)
      )
    );
  }
}

void hashString(std::uint64_t& hash, std::string_view text) noexcept
{
  hashUnsignedLittleEndian(
    hash,
    static_cast<std::uint64_t>(text.size())
  );

  for(const char character : text)
  {
    hashByte(
      hash,
      static_cast<std::uint8_t>(
        static_cast<unsigned char>(character)
      )
    );
  }
}

void hashDouble(std::uint64_t& hash, double value) noexcept
{
  hashUnsignedLittleEndian(
    hash,
    std::bit_cast<std::uint64_t>(value)
  );
}

BodyProfileDefinition makeNaturalDevelopmentProfile()
{
  BodyProfileDefinition profile;
  profile.id = BodyProfileId::NaturalDevelopment;
  profile.canonicalKey = "factory.natural.dev.v1";
  profile.displayName = "Natural Development";
  profile.schemaVersion = kBodyProfileSchemaVersion;

  // Development-only residual transfer. Tap zero preserves the causal direct
  // path; later taps introduce a small, bounded spectral correction. These
  // coefficients are engineering placeholders until real pickup/mic evidence
  // is available.
  constexpr std::array<double, 24> taps{
    1.0000,
   -0.0600,
    0.0340,
   -0.0210,
    0.0180,
   -0.0120,
    0.0090,
   -0.0065,
    0.0050,
   -0.0040,
    0.0032,
   -0.0026,
    0.0021,
   -0.0017,
    0.00135,
   -0.00105,
    0.00082,
   -0.00064,
    0.00050,
   -0.00039,
    0.00030,
   -0.00023,
    0.00017,
   -0.00012
  };

  profile.transferTapCount = taps.size();

  for(std::size_t index = 0; index < taps.size(); ++index)
    profile.transferTaps[index] = taps[index];

  constexpr std::array<ModalModeSpec, 8> modes{
    ModalModeSpec{  96.0,  8.0, 0.080},
    ModalModeSpec{ 117.0, 10.0, 0.070},
    ModalModeSpec{ 188.0,  7.0, 0.040},
    ModalModeSpec{ 231.0,  6.5, 0.032},
    ModalModeSpec{ 327.0,  5.5, 0.022},
    ModalModeSpec{ 462.0,  4.8, 0.016},
    ModalModeSpec{ 711.0,  4.0, 0.010},
    ModalModeSpec{1030.0,  3.5, 0.006}
  };

  profile.modeCount = modes.size();

  for(std::size_t index = 0; index < modes.size(); ++index)
    profile.modes[index] = modes[index];

  profile.outputGain = 0.94;
  return profile;
}

BodyProfileDefinition makeDreadnoughtDevelopmentProfile()
{
  BodyProfileDefinition profile;
  profile.id = BodyProfileId::DreadnoughtDevelopment;
  profile.canonicalKey = "factory.dreadnought.dev.v1";
  profile.displayName = "Dreadnought Development";
  profile.schemaVersion = kBodyProfileSchemaVersion;

  constexpr std::array<double, 28> taps{
    1.0000,
   -0.0500,
    0.0400,
   -0.0260,
    0.0210,
   -0.0150,
    0.0120,
   -0.0090,
    0.0072,
   -0.0058,
    0.0047,
   -0.0038,
    0.0031,
   -0.0025,
    0.0020,
   -0.0016,
    0.00130,
   -0.00104,
    0.00083,
   -0.00066,
    0.00052,
   -0.00041,
    0.00032,
   -0.00025,
    0.00019,
   -0.00014,
    0.00010,
   -0.00007
  };

  profile.transferTapCount = taps.size();

  for(std::size_t index = 0; index < taps.size(); ++index)
    profile.transferTaps[index] = taps[index];

  constexpr std::array<ModalModeSpec, 10> modes{
    ModalModeSpec{  89.0, 10.5, 0.105},
    ModalModeSpec{ 108.0, 12.0, 0.095},
    ModalModeSpec{ 171.0,  8.5, 0.055},
    ModalModeSpec{ 214.0,  7.5, 0.046},
    ModalModeSpec{ 286.0,  6.5, 0.034},
    ModalModeSpec{ 395.0,  5.5, 0.026},
    ModalModeSpec{ 548.0,  4.8, 0.018},
    ModalModeSpec{ 742.0,  4.2, 0.012},
    ModalModeSpec{ 980.0,  3.8, 0.008},
    ModalModeSpec{1280.0,  3.2, 0.005}
  };

  profile.modeCount = modes.size();

  for(std::size_t index = 0; index < modes.size(); ++index)
    profile.modes[index] = modes[index];

  profile.outputGain = 0.91;
  return profile;
}

} // namespace

const BodyProfileDefinition& naturalDevelopmentProfile() noexcept
{
  static const BodyProfileDefinition profile =
    makeNaturalDevelopmentProfile();

  return profile;
}

const BodyProfileDefinition& dreadnoughtDevelopmentProfile() noexcept
{
  static const BodyProfileDefinition profile =
    makeDreadnoughtDevelopmentProfile();

  return profile;
}

BodyProfileValidationResult validateBodyProfileDefinition(
  const BodyProfileDefinition& profile
) noexcept
{
  if(profile.schemaVersion != kBodyProfileSchemaVersion)
  {
    return {
      BodyProfileValidationError::UnsupportedSchema
    };
  }

  if(profile.canonicalKey.empty())
  {
    return {
      BodyProfileValidationError::MissingCanonicalKey
    };
  }

  if(
    profile.transferTapCount == 0
    || profile.transferTapCount > kMaximumTransferTaps
  )
  {
    return {
      BodyProfileValidationError::InvalidTapCount
    };
  }

  double transferMagnitude = 0.0;

  for(std::size_t index = 0;
      index < profile.transferTapCount;
      ++index)
  {
    const double tap = profile.transferTaps[index];

    if(!std::isfinite(tap))
      return {BodyProfileValidationError::NonFiniteTap};

    transferMagnitude += std::abs(tap);
  }

  if(transferMagnitude > 4.0)
  {
    return {
      BodyProfileValidationError::UnsafeTransferMagnitude
    };
  }

  if(profile.modeCount > kMaximumBodyModes)
  {
    return {
      BodyProfileValidationError::InvalidModeCount
    };
  }

  for(std::size_t index = 0; index < profile.modeCount; ++index)
  {
    const auto& mode = profile.modes[index];

    if(
      !std::isfinite(mode.frequencyHz)
      || !std::isfinite(mode.q)
      || !std::isfinite(mode.gain)
    )
    {
      return {BodyProfileValidationError::NonFiniteMode};
    }

    if(mode.frequencyHz < 20.0 || mode.frequencyHz > 20000.0)
    {
      return {
        BodyProfileValidationError::InvalidModeFrequency
      };
    }

    if(mode.q < 0.25 || mode.q > 30.0)
      return {BodyProfileValidationError::InvalidModeQ};

    if(std::abs(mode.gain) > 0.5)
      return {BodyProfileValidationError::InvalidModeGain};
  }

  if(
    !std::isfinite(profile.outputGain)
    || profile.outputGain <= 0.0
    || profile.outputGain > 2.0
  )
  {
    return {
      BodyProfileValidationError::InvalidOutputGain
    };
  }

  return {};
}

std::uint64_t hashBodyProfileDefinition(
  const BodyProfileDefinition& profile
) noexcept
{
  std::uint64_t hash = kFnvOffset;

  hashUnsignedLittleEndian(
    hash,
    static_cast<std::uint32_t>(profile.id)
  );
  hashUnsignedLittleEndian(hash, profile.schemaVersion);
  hashString(hash, profile.canonicalKey);

  hashUnsignedLittleEndian(
    hash,
    static_cast<std::uint64_t>(profile.transferTapCount)
  );

  for(std::size_t index = 0;
      index < profile.transferTapCount;
      ++index)
  {
    hashDouble(hash, profile.transferTaps[index]);
  }

  hashUnsignedLittleEndian(
    hash,
    static_cast<std::uint64_t>(profile.modeCount)
  );

  for(std::size_t index = 0; index < profile.modeCount; ++index)
  {
    hashDouble(hash, profile.modes[index].frequencyHz);
    hashDouble(hash, profile.modes[index].q);
    hashDouble(hash, profile.modes[index].gain);
  }

  hashDouble(hash, profile.outputGain);

  return hash;
}

} // namespace gitasedap::dsp
