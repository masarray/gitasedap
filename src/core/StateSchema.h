#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace gitasedap::core::state
{

inline constexpr std::array<std::uint8_t, 4> kMagic{
  static_cast<std::uint8_t>('G'),
  static_cast<std::uint8_t>('S'),
  static_cast<std::uint8_t>('D'),
  static_cast<std::uint8_t>('P')
};

inline constexpr std::uint32_t kMinimumSupportedVersion = 1;
inline constexpr std::uint32_t kCurrentVersion = 1;
inline constexpr std::size_t kHeaderSize = 8;

using HeaderBytes = std::array<std::uint8_t, kHeaderSize>;

struct DecodedHeader
{
  bool magicValid{false};
  std::uint32_t version{0};
};

[[nodiscard]] constexpr HeaderBytes encodeHeader(
  std::uint32_t version = kCurrentVersion
) noexcept
{
  return HeaderBytes{
    kMagic[0],
    kMagic[1],
    kMagic[2],
    kMagic[3],
    static_cast<std::uint8_t>(version & 0xFFu),
    static_cast<std::uint8_t>((version >> 8u) & 0xFFu),
    static_cast<std::uint8_t>((version >> 16u) & 0xFFu),
    static_cast<std::uint8_t>((version >> 24u) & 0xFFu)
  };
}

[[nodiscard]] constexpr DecodedHeader decodeHeader(
  const HeaderBytes& bytes
) noexcept
{
  const bool magicValid =
    bytes[0] == kMagic[0]
    && bytes[1] == kMagic[1]
    && bytes[2] == kMagic[2]
    && bytes[3] == kMagic[3];

  const std::uint32_t version =
    static_cast<std::uint32_t>(bytes[4])
    | (static_cast<std::uint32_t>(bytes[5]) << 8u)
    | (static_cast<std::uint32_t>(bytes[6]) << 16u)
    | (static_cast<std::uint32_t>(bytes[7]) << 24u);

  return DecodedHeader{magicValid, version};
}

[[nodiscard]] constexpr bool isSupportedVersion(
  std::uint32_t version
) noexcept
{
  return version >= kMinimumSupportedVersion
      && version <= kCurrentVersion;
}

[[nodiscard]] constexpr bool isSupportedHeader(
  const HeaderBytes& bytes
) noexcept
{
  const auto decoded = decodeHeader(bytes);
  return decoded.magicValid && isSupportedVersion(decoded.version);
}

static_assert(kCurrentVersion >= kMinimumSupportedVersion);
static_assert(encodeHeader()[0] == static_cast<std::uint8_t>('G'));
static_assert(encodeHeader()[4] == 1u);

} // namespace gitasedap::core::state
