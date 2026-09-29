#include "TestSupport.h"
#include "lab/DeadlineBenchmark.h"

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

  GS_REQUIRE(result.iterations == 128);
  GS_REQUIRE(result.deadlineMicroseconds > 1000.0);
  GS_REQUIRE(result.meanMicroseconds >= 0.0);
  GS_REQUIRE(result.p95Microseconds <= result.maximumMicroseconds);
  GS_REQUIRE(result.p99Microseconds <= result.maximumMicroseconds);
  GS_REQUIRE(result.p999Microseconds <= result.maximumMicroseconds);
  GS_REQUIRE(result.p99DeadlineRatio >= 0.0);
  GS_REQUIRE(sink >= 0.0F);

  return 0;
}
