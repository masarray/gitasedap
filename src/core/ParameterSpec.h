#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace gitasedap::core
{

// These numeric IDs are the canonical host-facing parameter identity.
// Do not reorder or reuse IDs after a public release.
enum class ParameterId : int
{
  Body = 0,
  Air,
  Enhance,
  OutputDb,
  Bypass,
  InputSource,
  Count
};

enum class InputSource : int
{
  ActivePiezo = 0,
  PassivePiezo,
  Magnetic,
  Count
};

struct ContinuousParameterSpec
{
  std::string_view key;
  std::string_view displayName;
  std::string_view unit;
  double defaultValue;
  double minimum;
  double maximum;
  double step;
};

inline constexpr ContinuousParameterSpec kBodySpec{
  "body", "Body", "%", 55.0, 0.0, 100.0, 0.1
};

inline constexpr ContinuousParameterSpec kAirSpec{
  "air", "Air", "%", 35.0, 0.0, 100.0, 0.1
};

inline constexpr ContinuousParameterSpec kEnhanceSpec{
  "enhance", "Enhance", "%", 40.0, 0.0, 100.0, 0.1
};

inline constexpr ContinuousParameterSpec kOutputSpec{
  "output_db", "Output", "dB", 0.0, -12.0, 12.0, 0.1
};

inline constexpr std::array<std::string_view, 6> kCanonicalParameterKeys{
  kBodySpec.key,
  kAirSpec.key,
  kEnhanceSpec.key,
  kOutputSpec.key,
  "bypass",
  "input_source"
};

inline constexpr std::array<std::string_view, 3> kInputSourceNames{
  "Active Piezo",
  "Passive Piezo",
  "Magnetic"
};

[[nodiscard]] constexpr int toIndex(ParameterId id) noexcept
{
  return static_cast<int>(id);
}

[[nodiscard]] constexpr std::size_t parameterCount() noexcept
{
  return static_cast<std::size_t>(ParameterId::Count);
}

[[nodiscard]] constexpr bool continuousSpecIsValid(const ContinuousParameterSpec& spec) noexcept
{
  return spec.minimum <= spec.defaultValue
      && spec.defaultValue <= spec.maximum
      && spec.minimum < spec.maximum
      && spec.step > 0.0;
}

[[nodiscard]] constexpr bool canonicalKeysAreUnique() noexcept
{
  for(std::size_t i = 0; i < kCanonicalParameterKeys.size(); ++i)
  {
    for(std::size_t j = i + 1; j < kCanonicalParameterKeys.size(); ++j)
    {
      if(kCanonicalParameterKeys[i] == kCanonicalParameterKeys[j])
        return false;
    }
  }

  return true;
}

static_assert(parameterCount() == kCanonicalParameterKeys.size());
static_assert(toIndex(ParameterId::Body) == 0);
static_assert(toIndex(ParameterId::InputSource) == 5);
static_assert(continuousSpecIsValid(kBodySpec));
static_assert(continuousSpecIsValid(kAirSpec));
static_assert(continuousSpecIsValid(kEnhanceSpec));
static_assert(continuousSpecIsValid(kOutputSpec));
static_assert(canonicalKeysAreUnique());

} // namespace gitasedap::core
