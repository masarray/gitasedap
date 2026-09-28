#include "core/StateSchema.h"

#include <cstdint>

namespace gsstate = gitasedap::core::state;

constexpr bool canonicalHeaderBytesAreStable() noexcept
{
  constexpr auto bytes = gsstate::encodeHeader();

  return bytes[0] == static_cast<std::uint8_t>('G')
      && bytes[1] == static_cast<std::uint8_t>('S')
      && bytes[2] == static_cast<std::uint8_t>('D')
      && bytes[3] == static_cast<std::uint8_t>('P')
      && bytes[4] == 1u
      && bytes[5] == 0u
      && bytes[6] == 0u
      && bytes[7] == 0u;
}

constexpr bool rejectsCorruptMagic() noexcept
{
  auto bytes = gsstate::encodeHeader();
  bytes[0] = static_cast<std::uint8_t>('X');
  return !gsstate::isSupportedHeader(bytes);
}

constexpr bool rejectsFutureVersion() noexcept
{
  constexpr auto bytes = gsstate::encodeHeader(gsstate::kCurrentVersion + 1u);
  return !gsstate::isSupportedHeader(bytes);
}

constexpr bool rejectsVersionZero() noexcept
{
  constexpr auto bytes = gsstate::encodeHeader(0u);
  return !gsstate::isSupportedHeader(bytes);
}

static_assert(canonicalHeaderBytesAreStable());
static_assert(gsstate::isSupportedHeader(gsstate::encodeHeader()));
static_assert(rejectsCorruptMagic());
static_assert(rejectsFutureVersion());
static_assert(rejectsVersionZero());

int main()
{
  return 0;
}
