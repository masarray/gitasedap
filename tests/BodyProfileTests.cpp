#include "TestSupport.h"
#include "dsp/BodyProfile.h"
#include "dsp/BodyProfileCompiler.h"

#include <array>
#include <cmath>
#include <limits>

namespace gsdsp = gitasedap::dsp;

int main()
{
  const auto& natural = gsdsp::naturalDevelopmentProfile();
  const auto& dreadnought = gsdsp::dreadnoughtDevelopmentProfile();

  GS_REQUIRE(gsdsp::validateBodyProfileDefinition(natural).ok());
  GS_REQUIRE(gsdsp::validateBodyProfileDefinition(dreadnought).ok());

  GS_REQUIRE(!natural.canonicalKey.empty());
  GS_REQUIRE(!dreadnought.canonicalKey.empty());
  GS_REQUIRE(natural.canonicalKey != dreadnought.canonicalKey);

  const auto naturalHashA =
    gsdsp::hashBodyProfileDefinition(natural);
  const auto naturalHashB =
    gsdsp::hashBodyProfileDefinition(natural);
  const auto dreadnoughtHash =
    gsdsp::hashBodyProfileDefinition(dreadnought);

  GS_REQUIRE(naturalHashA != 0);
  GS_REQUIRE(naturalHashA == naturalHashB);
  GS_REQUIRE(naturalHashA != dreadnoughtHash);

  constexpr std::array<double, 4> sampleRates{
    44100.0,
    48000.0,
    88200.0,
    96000.0
  };

  for(const double sampleRate : sampleRates)
  {
    const auto naturalPrepared =
      gsdsp::compileBodyProfile(natural, sampleRate);

    const auto dreadnoughtPrepared =
      gsdsp::compileBodyProfile(dreadnought, sampleRate);

    GS_REQUIRE(naturalPrepared.ok());
    GS_REQUIRE(dreadnoughtPrepared.ok());

    GS_REQUIRE(
      naturalPrepared.prepared.contentHash
      == naturalHashA
    );
    GS_REQUIRE(
      dreadnoughtPrepared.prepared.contentHash
      == dreadnoughtHash
    );

    GS_REQUIRE(
      naturalPrepared.prepared.transfer.tapCount
      == natural.transferTapCount
    );
    GS_REQUIRE(
      naturalPrepared.prepared.modal.modeCount
      == natural.modeCount
    );

    for(std::size_t index = 0;
        index < naturalPrepared.prepared.modal.modeCount;
        ++index)
    {
      const auto& mode =
        naturalPrepared.prepared.modal.modes[index];

      GS_REQUIRE(std::isfinite(mode.b0));
      GS_REQUIRE(std::isfinite(mode.b1));
      GS_REQUIRE(std::isfinite(mode.b2));
      GS_REQUIRE(std::isfinite(mode.a1));
      GS_REQUIRE(std::isfinite(mode.a2));
      GS_REQUIRE(std::isfinite(mode.gain));

      GS_REQUIRE(std::abs(mode.a2) < 1.0);
    }
  }

  {
    auto invalid = natural;
    invalid.transferTaps[0] =
      std::numeric_limits<double>::infinity();

    GS_REQUIRE(
      !gsdsp::validateBodyProfileDefinition(invalid).ok()
    );
    GS_REQUIRE(
      !gsdsp::compileBodyProfile(invalid, 48000.0).ok()
    );
  }

  {
    auto invalid = natural;
    invalid.modeCount = gsdsp::kMaximumBodyModes + 1;

    GS_REQUIRE(
      !gsdsp::validateBodyProfileDefinition(invalid).ok()
    );
  }

  {
    auto invalid = natural;
    invalid.modes[0].q = 1000.0;

    GS_REQUIRE(
      !gsdsp::validateBodyProfileDefinition(invalid).ok()
    );
  }

  GS_REQUIRE(
    !gsdsp::compileBodyProfile(natural, 1000.0).ok()
  );

  return 0;
}
