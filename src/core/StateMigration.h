#pragma once

#include "core/ParameterSpec.h"
#include "core/StateSchema.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace gitasedap::core::state
{

using CanonicalParameterValues =
  std::array<double, ::gitasedap::core::parameterCount()>;

[[nodiscard]] constexpr CanonicalParameterValues
defaultCanonicalParameterValues() noexcept
{
  CanonicalParameterValues values{};

  values[::gitasedap::core::toIndex(::gitasedap::core::ParameterId::Body)] =
    ::gitasedap::core::kBodySpec.defaultValue;
  values[::gitasedap::core::toIndex(::gitasedap::core::ParameterId::Air)] =
    ::gitasedap::core::kAirSpec.defaultValue;
  values[::gitasedap::core::toIndex(::gitasedap::core::ParameterId::Enhance)] =
    ::gitasedap::core::kEnhanceSpec.defaultValue;
  values[::gitasedap::core::toIndex(::gitasedap::core::ParameterId::OutputDb)] =
    ::gitasedap::core::kOutputSpec.defaultValue;
  values[::gitasedap::core::toIndex(::gitasedap::core::ParameterId::Bypass)] = 0.0;
  values[::gitasedap::core::toIndex(::gitasedap::core::ParameterId::InputSource)] =
    static_cast<double>(::gitasedap::core::InputSource::ActivePiezo);

  values[::gitasedap::core::toIndex(::gitasedap::core::ParameterId::BodyProfileA)] =
    static_cast<double>(
      ::gitasedap::core::BodyProfileChoice::NaturalDevelopment
    );

  values[::gitasedap::core::toIndex(::gitasedap::core::ParameterId::BodyProfileB)] =
    static_cast<double>(
      ::gitasedap::core::BodyProfileChoice::DreadnoughtDevelopment
    );

  values[::gitasedap::core::toIndex(::gitasedap::core::ParameterId::BodyCompareSlot)] =
    static_cast<double>(::gitasedap::core::BodyCompareSlot::A);

  return values;
}

[[nodiscard]] inline bool migrateSerializedParameterValues(
  std::uint32_t version,
  std::span<const double> serialized,
  CanonicalParameterValues& destination
) noexcept
{
  const auto expectedCount = serializedParameterCount(version);

  if(
    expectedCount == 0
    || serialized.size() != expectedCount
    || expectedCount > destination.size()
  )
  {
    return false;
  }

  destination = defaultCanonicalParameterValues();

  for(std::size_t index = 0; index < expectedCount; ++index)
    destination[index] = serialized[index];

  return true;
}

static_assert(
  defaultCanonicalParameterValues()[
    ::gitasedap::core::toIndex(::gitasedap::core::ParameterId::BodyProfileA)
  ]
  == static_cast<double>(
    ::gitasedap::core::BodyProfileChoice::NaturalDevelopment
  )
);

static_assert(
  defaultCanonicalParameterValues()[
    ::gitasedap::core::toIndex(::gitasedap::core::ParameterId::BodyProfileB)
  ]
  == static_cast<double>(
    ::gitasedap::core::BodyProfileChoice::DreadnoughtDevelopment
  )
);

} // namespace gitasedap::core::state
