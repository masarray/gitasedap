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
  std::array<double, core::parameterCount()>;

[[nodiscard]] constexpr CanonicalParameterValues
defaultCanonicalParameterValues() noexcept
{
  CanonicalParameterValues values{};

  values[core::toIndex(core::ParameterId::Body)] =
    core::kBodySpec.defaultValue;
  values[core::toIndex(core::ParameterId::Air)] =
    core::kAirSpec.defaultValue;
  values[core::toIndex(core::ParameterId::Enhance)] =
    core::kEnhanceSpec.defaultValue;
  values[core::toIndex(core::ParameterId::OutputDb)] =
    core::kOutputSpec.defaultValue;
  values[core::toIndex(core::ParameterId::Bypass)] = 0.0;
  values[core::toIndex(core::ParameterId::InputSource)] =
    static_cast<double>(core::InputSource::ActivePiezo);

  values[core::toIndex(core::ParameterId::BodyProfileA)] =
    static_cast<double>(
      core::BodyProfileChoice::NaturalDevelopment
    );

  values[core::toIndex(core::ParameterId::BodyProfileB)] =
    static_cast<double>(
      core::BodyProfileChoice::DreadnoughtDevelopment
    );

  values[core::toIndex(core::ParameterId::BodyCompareSlot)] =
    static_cast<double>(core::BodyCompareSlot::A);

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
    core::toIndex(core::ParameterId::BodyProfileA)
  ]
  == static_cast<double>(
    core::BodyProfileChoice::NaturalDevelopment
  )
);

static_assert(
  defaultCanonicalParameterValues()[
    core::toIndex(core::ParameterId::BodyProfileB)
  ]
  == static_cast<double>(
    core::BodyProfileChoice::DreadnoughtDevelopment
  )
);

} // namespace gitasedap::core::state
