#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace gitasedap::dsp
{

inline constexpr std::size_t kMaximumTransferTaps = 128;
inline constexpr std::size_t kMaximumBodyModes = 12;
inline constexpr std::uint32_t kBodyProfileSchemaVersion = 1;

enum class BodyProfileId : std::uint32_t
{
  RawConditioned = 0,
  NaturalDevelopment = 1,
  DreadnoughtDevelopment = 2
};

struct ModalModeSpec
{
  double frequencyHz{0.0};
  double q{1.0};
  double gain{0.0};
};

struct BodyProfileDefinition
{
  BodyProfileId id{BodyProfileId::NaturalDevelopment};
  std::string_view canonicalKey;
  std::string_view displayName;
  std::uint32_t schemaVersion{kBodyProfileSchemaVersion};

  std::size_t transferTapCount{0};
  std::array<double, kMaximumTransferTaps> transferTaps{};

  std::size_t modeCount{0};
  std::array<ModalModeSpec, kMaximumBodyModes> modes{};

  double outputGain{1.0};
};

enum class BodyProfileValidationError
{
  None,
  UnsupportedSchema,
  MissingCanonicalKey,
  InvalidTapCount,
  NonFiniteTap,
  UnsafeTransferMagnitude,
  InvalidModeCount,
  NonFiniteMode,
  InvalidModeFrequency,
  InvalidModeQ,
  InvalidModeGain,
  InvalidOutputGain
};

struct BodyProfileValidationResult
{
  BodyProfileValidationError error{BodyProfileValidationError::None};

  [[nodiscard]] bool ok() const noexcept
  {
    return error == BodyProfileValidationError::None;
  }
};

[[nodiscard]] const BodyProfileDefinition& rawConditionedProfile() noexcept;
[[nodiscard]] const BodyProfileDefinition& naturalDevelopmentProfile() noexcept;
[[nodiscard]] const BodyProfileDefinition& dreadnoughtDevelopmentProfile() noexcept;

[[nodiscard]] BodyProfileValidationResult validateBodyProfileDefinition(
  const BodyProfileDefinition& profile
) noexcept;

[[nodiscard]] std::uint64_t hashBodyProfileDefinition(
  const BodyProfileDefinition& profile
) noexcept;

} // namespace gitasedap::dsp
