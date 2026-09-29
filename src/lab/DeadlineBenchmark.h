#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace gitasedap::lab
{

struct DeadlineBenchmarkResult
{
  std::size_t iterations{0};
  double deadlineMicroseconds{0.0};
  double meanMicroseconds{0.0};
  double p95Microseconds{0.0};
  double p99Microseconds{0.0};
  double p999Microseconds{0.0};
  double maximumMicroseconds{0.0};
  double p99DeadlineRatio{0.0};
};

class DeadlineBenchmark
{
public:
  template <typename Callback>
  [[nodiscard]] static DeadlineBenchmarkResult run(
    double sampleRate,
    std::size_t blockSize,
    std::size_t warmupIterations,
    std::size_t measuredIterations,
    Callback&& callback
  )
  {
    if(sampleRate <= 0.0)
      throw std::invalid_argument("sampleRate must be positive");

    if(blockSize == 0 || measuredIterations == 0)
      throw std::invalid_argument("invalid benchmark dimensions");

    for(std::size_t i = 0; i < warmupIterations; ++i)
      callback();

    std::vector<double> durations;
    durations.reserve(measuredIterations);

    double total = 0.0;

    for(std::size_t i = 0; i < measuredIterations; ++i)
    {
      const auto start = Clock::now();
      callback();
      const auto end = Clock::now();

      const double microseconds =
        std::chrono::duration<double, std::micro>(end - start).count();

      durations.push_back(microseconds);
      total += microseconds;
    }

    std::sort(durations.begin(), durations.end());

    DeadlineBenchmarkResult result;
    result.iterations = measuredIterations;
    result.deadlineMicroseconds =
      (static_cast<double>(blockSize) / sampleRate) * 1.0e6;
    result.meanMicroseconds =
      total / static_cast<double>(measuredIterations);
    result.p95Microseconds = percentile(durations, 0.95);
    result.p99Microseconds = percentile(durations, 0.99);
    result.p999Microseconds = percentile(durations, 0.999);
    result.maximumMicroseconds = durations.back();

    if(result.deadlineMicroseconds > 0.0)
    {
      result.p99DeadlineRatio =
        result.p99Microseconds / result.deadlineMicroseconds;
    }

    return result;
  }

private:
  using Clock = std::chrono::steady_clock;

  [[nodiscard]] static double percentile(
    const std::vector<double>& sorted,
    double fraction
  )
  {
    if(sorted.empty())
      return 0.0;

    const auto position = static_cast<std::size_t>(
      std::ceil(fraction * static_cast<double>(sorted.size()))
    );

    const auto index = std::clamp<std::size_t>(
      position == 0 ? 0 : position - 1,
      0,
      sorted.size() - 1
    );

    return sorted[index];
  }
};

} // namespace gitasedap::lab
