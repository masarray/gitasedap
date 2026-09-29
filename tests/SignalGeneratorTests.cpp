#include "TestSupport.h"
#include "lab/GoldenCompare.h"
#include "lab/SignalGenerator.h"

#include <cmath>

namespace gslab = gitasedap::lab;

int main()
{
  const auto impulse = gslab::SignalGenerator::impulse(2, 128, 17, 0.75F);

  GS_REQUIRE(impulse.channel(0)[17] == 0.75F);
  GS_REQUIRE(impulse.channel(1)[17] == 0.75F);
  GS_REQUIRE(impulse.channel(0)[16] == 0.0F);

  const auto whiteA = gslab::SignalGenerator::whiteNoise(
    1,
    2048,
    12345U
  );
  const auto whiteB = gslab::SignalGenerator::whiteNoise(
    1,
    2048,
    12345U
  );

  GS_REQUIRE(
    gslab::compareGolden(
      whiteA,
      whiteB,
      gslab::GoldenTolerance{0.0, 0.0}
    ).passed()
  );

  const auto sweep = gslab::SignalGenerator::logarithmicSweep(
    1,
    48000,
    48000.0,
    20.0,
    20000.0,
    0.5F
  );

  for(const auto sample : sweep.channel(0))
  {
    GS_REQUIRE(std::isfinite(sample));
    GS_REQUIRE(std::abs(sample) <= 0.500001F);
  }

  const auto pink = gslab::SignalGenerator::pinkNoise(
    1,
    4096,
    98765U
  );

  for(const auto sample : pink.channel(0))
    GS_REQUIRE(std::isfinite(sample));

  return 0;
}
