#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace gitasedap::lab
{

struct ParameterEvent
{
  std::uint32_t parameterIndex{0};
  std::uint64_t sampleOffset{0};
  double normalizedValue{0.0};

  [[nodiscard]] bool operator==(const ParameterEvent&) const noexcept = default;
};

class ParameterTorture
{
public:
  [[nodiscard]] static std::vector<ParameterEvent> generate(
    std::uint32_t parameterCount,
    std::uint64_t totalSamples,
    std::size_t eventCount,
    std::uint64_t seed = 0x475344505432ULL
  )
  {
    if(parameterCount == 0)
      throw std::invalid_argument("parameterCount must be non-zero");

    if(totalSamples == 0)
      throw std::invalid_argument("totalSamples must be non-zero");

    std::vector<ParameterEvent> events;
    events.reserve(eventCount);

    XorShift64Star random(seed);

    for(std::size_t index = 0; index < eventCount; ++index)
    {
      const auto parameter = static_cast<std::uint32_t>(
        random.next() % parameterCount
      );

      const auto sampleOffset = random.next() % totalSamples;

      double value = 0.0;

      switch(index % 8U)
      {
        case 0:
          value = 0.0;
          break;
        case 1:
          value = 1.0;
          break;
        case 2:
          value = 0.5;
          break;
        default:
          value = random.unitDouble();
          break;
      }

      events.push_back(
        ParameterEvent{parameter, sampleOffset, value}
      );
    }

    std::stable_sort(
      events.begin(),
      events.end(),
      [](const ParameterEvent& lhs, const ParameterEvent& rhs) {
        if(lhs.sampleOffset != rhs.sampleOffset)
          return lhs.sampleOffset < rhs.sampleOffset;

        return lhs.parameterIndex < rhs.parameterIndex;
      }
    );

    return events;
  }

private:
  class XorShift64Star
  {
  public:
    explicit XorShift64Star(std::uint64_t seed) noexcept
    : mState(seed == 0 ? 0x9E3779B97F4A7C15ULL : seed)
    {
    }

    [[nodiscard]] std::uint64_t next() noexcept
    {
      auto x = mState;
      x ^= x >> 12U;
      x ^= x << 25U;
      x ^= x >> 27U;
      mState = x;
      return x * 2685821657736338717ULL;
    }

    [[nodiscard]] double unitDouble() noexcept
    {
      constexpr double denominator = 9007199254740992.0;
      const auto top53 = next() >> 11U;
      return static_cast<double>(top53) / denominator;
    }

  private:
    std::uint64_t mState;
  };
};

} // namespace gitasedap::lab
