#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace gitasedap::core
{

// These numeric IDs are the canonical host-facing parameter identity.
// Do not reorder or reuse IDs after a public release. New IDs are appended.
enum class ParameterId : int
{
  Body = 0,
  Air = 1,
  Enhance = 2,
  OutputDb = 3,
  Bypass = 4,
  InputSource = 5,

  // P4C: append-only body comparison state. Existing IDs 0..5 stay stable.
  BodyProfileA = 6,
  BodyProfileB = 7,
  BodyCompareSlot = 8,

  Count
};

enum class InputSource : int
{
  ActivePiezo = 0,
  PassivePiezo,
  Magnetic,
  Count
};

enum class BodyProfileChoice : int
{
  RawConditioned = 0,
  NaturalDevelopment,
  DreadnoughtDevelopment,
  Count
};

enum class BodyCompareSlot : int
{
  A = 0,
  B,
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

inline constexpr std::array<std::string_view, 9> kCanonicalParameterKeys{
  kBodySpec.key,
  kAirSpec.key,
  kEnhanceSpec.key,
  kOutputSpec.key,
  "bypass",
  "input_source",
  "body_profile_a",
  "body_profile_b",
  "body_compare_slot"
};

inline constexpr std::array<std::string_view, 3> kInputSourceNames{
  "Active Piezo",
  "Passive Piezo",
  "Magnetic"
};

inline constexpr std::array<std::string_view, 3> kBodyProfileChoiceNames{
  "Raw / P3",
  "Natural Development",
  "Dreadnought Development"
};

inline constexpr std::array<std::string_view, 2> kBodyCompareSlotNames{
  "A",
  "B"
};

[[nodiscard]] constexpr int toIndex(ParameterId id) noexcept
{
  return static_cast<int>(id);
}

[[nodiscard]] constexpr std::size_t parameterCount() noexcept
{
  return static_cast<std::size_t>(ParameterId::Count);
}

[[nodiscard]] constexpr bool continuousSpecIsValid(
  const ContinuousParameterSpec& spec
) noexcept
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
static_assert(toIndex(ParameterId::BodyProfileA) == 6);
static_assert(toIndex(ParameterId::BodyProfileB) == 7);
static_assert(toIndex(ParameterId::BodyCompareSlot) == 8);
static_assert(continuousSpecIsValid(kBodySpec));
static_assert(continuousSpecIsValid(kAirSpec));
static_assert(continuousSpecIsValid(kEnhanceSpec));
static_assert(continuousSpecIsValid(kOutputSpec));
static_assert(canonicalKeysAreUnique());

} // namespace gitasedap::core
