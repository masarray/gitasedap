#include "TestSupport.h"
#include "core/ParameterSpec.h"
#include "core/StateMigration.h"

#include <array>
#include <span>

namespace gs = gitasedap::core;
namespace gsstate = gitasedap::core::state;

int main()
{
  {
    const std::array<double, 6> legacy{
      72.5,
      21.0,
      63.0,
      -2.5,
      1.0,
      static_cast<double>(gs::InputSource::PassivePiezo)
    };

    gsstate::CanonicalParameterValues migrated{};

    GS_REQUIRE(
      gsstate::migrateSerializedParameterValues(
        1,
        std::span<const double>(legacy),
        migrated
      )
    );

    GS_REQUIRE(
      migrated[gs::toIndex(gs::ParameterId::Body)]
      == legacy[0]
    );
    GS_REQUIRE(
      migrated[gs::toIndex(gs::ParameterId::InputSource)]
      == legacy[5]
    );

    GS_REQUIRE(
      migrated[gs::toIndex(gs::ParameterId::BodyProfileA)]
      == static_cast<double>(
        gs::BodyProfileChoice::NaturalDevelopment
      )
    );

    GS_REQUIRE(
      migrated[gs::toIndex(gs::ParameterId::BodyProfileB)]
      == static_cast<double>(
        gs::BodyProfileChoice::DreadnoughtDevelopment
      )
    );

    GS_REQUIRE(
      migrated[gs::toIndex(gs::ParameterId::BodyCompareSlot)]
      == static_cast<double>(gs::BodyCompareSlot::A)
    );
  }

  {
    std::array<double, 9> current{
      44.0,
      25.0,
      35.0,
      1.5,
      0.0,
      static_cast<double>(gs::InputSource::Magnetic),
      static_cast<double>(gs::BodyProfileChoice::RawConditioned),
      static_cast<double>(
        gs::BodyProfileChoice::DreadnoughtDevelopment
      ),
      static_cast<double>(gs::BodyCompareSlot::B)
    };

    gsstate::CanonicalParameterValues migrated{};

    GS_REQUIRE(
      gsstate::migrateSerializedParameterValues(
        2,
        std::span<const double>(current),
        migrated
      )
    );

    GS_REQUIRE(migrated == current);
  }

  {
    const std::array<double, 5> malformed{};
    gsstate::CanonicalParameterValues migrated{};

    GS_REQUIRE(
      !gsstate::migrateSerializedParameterValues(
        1,
        std::span<const double>(malformed),
        migrated
      )
    );
  }

  {
    const std::array<double, 9> future{};
    gsstate::CanonicalParameterValues migrated{};

    GS_REQUIRE(
      !gsstate::migrateSerializedParameterValues(
        3,
        std::span<const double>(future),
        migrated
      )
    );
  }

  return 0;
}
