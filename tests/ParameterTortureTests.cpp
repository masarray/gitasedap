#include "lab/ParameterTorture.h"

#include <cassert>
#include <cstddef>

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

  assert(first == second);
  assert(first.size() == 5000);

  std::uint64_t previousOffset = 0;

  for(std::size_t index = 0; index < first.size(); ++index)
  {
    const auto& event = first[index];

    assert(event.parameterIndex < 6);
    assert(event.sampleOffset < (48000U * 10U));
    assert(event.normalizedValue >= 0.0);
    assert(event.normalizedValue <= 1.0);

    if(index > 0)
      assert(event.sampleOffset >= previousOffset);

    previousOffset = event.sampleOffset;
  }

  assert(first != gslab::ParameterTorture::generate(
    6,
    48000U * 10U,
    5000,
    0xD00DFEEEU
  ));

  return 0;
}
