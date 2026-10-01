#include "core/StateSchema.h"

#include <cstdint>

namespace gsstate = gitasedap::core::state;

constexpr bool currentHeaderBytesAreStable() noexcept
{
  constexpr auto bytes = gsstate::encodeHeader();

  return bytes[0] == static_cast<std::uint8_t>('G')
      && bytes[1] == static_cast<std::uint8_t>('S')
      && bytes[2] == static_cast<std::uint8_t>('D')
      && bytes[3] == static_cast<std::uint8_t>('P')
      && bytes[4] == 2u
      && bytes[5] == 0u
      && bytes[6] == 0u
      && bytes[7] == 0u;
}

constexpr bool version1RemainsSupported() noexcept
{
  return gsstate::isSupportedHeader(
    gsstate::encodeHeader(1u)
  )
  && gsstate::serializedParameterCount(1u) == 6;
}

constexpr bool rejectsCorruptMagic() noexcept
{
  auto bytes = gsstate::encodeHeader();
  bytes[0] = static_cast<std::uint8_t>('X');
  return !gsstate::isSupportedHeader(bytes);
}

constexpr bool rejectsFutureVersion() noexcept
{
  constexpr auto bytes =
    gsstate::encodeHeader(gsstate::kCurrentVersion + 1u);
  return !gsstate::isSupportedHeader(bytes);
}

constexpr bool rejectsVersionZero() noexcept
{
  constexpr auto bytes = gsstate::encodeHeader(0u);
  return !gsstate::isSupportedHeader(bytes);
}

static_assert(currentHeaderBytesAreStable());
static_assert(version1RemainsSupported());
static_assert(gsstate::isSupportedHeader(gsstate::encodeHeader()));
static_assert(gsstate::serializedParameterCount(2u) == 9);
static_assert(rejectsCorruptMagic());
static_assert(rejectsFutureVersion());
static_assert(rejectsVersionZero());

int main()
{
  return 0;
}
