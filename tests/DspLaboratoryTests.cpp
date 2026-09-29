#include "lab/FiniteAudioGuard.h"
#include "lab/GoldenCompare.h"
#include "lab/OfflineRenderer.h"
#include "lab/SignalGenerator.h"

#include <cassert>
#include <cstddef>
#include <limits>

namespace gslab = gitasedap::lab;

namespace
{

class GainProcessor final : public gslab::IOfflineProcessor
{
public:
  explicit GainProcessor(float gain) noexcept
  : mGain(gain)
  {
  }

  void prepare(const gslab::PrepareSpec& spec) override
  {
    mPreparedChannels = spec.channelCount;
    mPreparedMaximumBlock = spec.maximumBlockSize;
  }

  void reset() noexcept override
  {
  }

  void process(
    float* const* channels,
    std::size_t channelCount,
    std::size_t frameCount
  ) noexcept override
  {
    assert(channelCount == mPreparedChannels);
    assert(frameCount <= mPreparedMaximumBlock);

    for(std::size_t channel = 0; channel < channelCount; ++channel)
    {
      for(std::size_t frame = 0; frame < frameCount; ++frame)
        channels[channel][frame] *= mGain;
    }
  }

private:
  float mGain{1.0F};
  std::size_t mPreparedChannels{0};
  std::size_t mPreparedMaximumBlock{0};
};

} // namespace

int main()
{
  const auto input = gslab::SignalGenerator::sine(
    2,
    4097,
    48000.0,
    997.0,
    0.4F
  );

  GainProcessor gainA(0.5F);
  GainProcessor gainB(0.5F);

  const auto render64 = gslab::OfflineRenderer::render(
    input,
    48000.0,
    gainA,
    gslab::RenderOptions{64, true}
  );

  const auto render127 = gslab::OfflineRenderer::render(
    input,
    48000.0,
    gainB,
    gslab::RenderOptions{127, true}
  );

  const auto blockInvariant = gslab::compareGolden(
    render64,
    render127,
    gslab::GoldenTolerance{0.0, 0.0}
  );

  assert(blockInvariant.passed());

  const auto report = gslab::analyzeFiniteAudio(render64);
  assert(report.allFinite());
  assert(report.peakAbsolute <= 0.200001);

  auto corrupt = render64;
  corrupt.channel(1)[12] = std::numeric_limits<float>::infinity();

  const auto corruptReport = gslab::analyzeFiniteAudio(corrupt);
  assert(!corruptReport.allFinite());
  assert(corruptReport.nonFiniteCount == 1);
  assert(corruptReport.firstNonFiniteChannel == 1);
  assert(corruptReport.firstNonFiniteFrame == 12);

  const auto mismatch = gslab::compareGolden(
    render64,
    corrupt,
    gslab::GoldenTolerance{1.0e-7, 0.0}
  );

  assert(!mismatch.passed());
  assert(mismatch.mismatchedSamples == 1);

  return 0;
}
