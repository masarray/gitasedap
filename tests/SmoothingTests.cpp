#include "TestSupport.h"
#include "dsp/Smoothing.h"

#include <cmath>

namespace gsdsp = gitasedap::dsp;

int main()
{
  {
    gsdsp::LinearSmoother smoother;
    smoother.prepare(1000.0, 10.0);
    smoother.reset(0.0);
    smoother.setTarget(1.0);

    double previous = 0.0;

    for(int sample = 0; sample < 10; ++sample)
    {
      const double current = smoother.next();
      GS_REQUIRE(current >= previous);
      GS_REQUIRE((current - previous) <= 0.1000000001);
      previous = current;
    }

    GS_REQUIRE(std::abs(smoother.current() - 1.0) < 1.0e-12);
    GS_REQUIRE(smoother.settled());
  }

  {
    gsdsp::BypassCrossfade bypass;
    bypass.prepare(1000.0, 5.0);
    bypass.reset(false);

    for(int sample = 0; sample < 10; ++sample)
      GS_REQUIRE(std::abs(bypass.process(1.0, 0.5) - 0.5) < 1.0e-12);

    bypass.setBypassed(true);

    double previous = 0.5;

    for(int sample = 0; sample < 5; ++sample)
    {
      const double current = bypass.process(1.0, 0.5);

      GS_REQUIRE(current >= previous);
      GS_REQUIRE((current - previous) <= 0.1000000001);
      previous = current;
    }

    GS_REQUIRE(std::abs(previous - 1.0) < 1.0e-12);

    bypass.setBypassed(false);

    for(int sample = 0; sample < 5; ++sample)
      previous = bypass.process(1.0, 0.5);

    GS_REQUIRE(std::abs(previous - 0.5) < 1.0e-12);
  }

  return 0;
}
