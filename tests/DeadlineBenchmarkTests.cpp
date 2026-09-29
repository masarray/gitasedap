#include "lab/DeadlineBenchmark.h"

#include <cassert>
#include <vector>

namespace gslab = gitasedap::lab;

int main()
{
  std::vector<float> block(64, 0.1F);
  volatile float sink = 0.0F;

  const auto result = gslab::DeadlineBenchmark::run(
    48000.0,
    block.size(),
    16,
    128,
    [&] {
      float local = 0.0F;

      for(auto& sample : block)
      {
        sample *= 0.999F;
        local += sample;
      }

      sink = local;
    }
  );

  assert(result.iterations == 128);
  assert(result.deadlineMicroseconds > 1000.0);
  assert(result.meanMicroseconds >= 0.0);
  assert(result.p95Microseconds <= result.maximumMicroseconds);
  assert(result.p99Microseconds <= result.maximumMicroseconds);
  assert(result.p999Microseconds <= result.maximumMicroseconds);
  assert(result.p99DeadlineRatio >= 0.0);
  assert(sink >= 0.0F);

  return 0;
}
