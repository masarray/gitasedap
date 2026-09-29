#include "TestSupport.h"
#include "lab/ParameterTorture.h"

#include <cstddef>
#include <cstdint>

namespace gslab = gitasedap::lab;

int main()
{
  const auto first = gslab::ParameterTorture::generate(
    6,
    48000U * 10U,
    5000,
    0xD00DFEEDULL
  );

  const auto second = gslab::ParameterTorture::generate(
    6,
    48000U * 10U,
    5000,
    0xD00DFEEDULL
  );

  GS_REQUIRE(first == second);
  GS_REQUIRE(first.size() == 5000);

  std::uint64_t previousOffset = 0;

  for(std::size_t index = 0; index < first.size(); ++index)
  {
    const auto& event = first[index];

    GS_REQUIRE(event.parameterIndex < 6);
    GS_REQUIRE(event.sampleOffset < (48000U * 10U));
    GS_REQUIRE(event.normalizedValue >= 0.0);
    GS_REQUIRE(event.normalizedValue <= 1.0);

    if(index > 0)
      GS_REQUIRE(event.sampleOffset >= previousOffset);

    previousOffset = event.sampleOffset;
  }

  GS_REQUIRE(first != gslab::ParameterTorture::generate(
    6,
    48000U * 10U,
    5000,
    0xD00DFEEEU
  ));

  return 0;
}
