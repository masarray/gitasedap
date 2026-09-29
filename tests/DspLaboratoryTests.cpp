#include "TestSupport.h"
#include "lab/FiniteAudioGuard.h"
#include "lab/GoldenCompare.h"
#include "lab/OfflineRenderer.h"
#include "lab/SignalGenerator.h"

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
    mContractValid = true;
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
    if(
      channelCount != mPreparedChannels
      || frameCount > mPreparedMaximumBlock
    )
    {
      mContractValid = false;
      return;
    }

    for(std::size_t channel = 0; channel < channelCount; ++channel)
    {
      for(std::size_t frame = 0; frame < frameCount; ++frame)
        channels[channel][frame] *= mGain;
    }
  }

  [[nodiscard]] bool contractValid() const noexcept
  {
    return mContractValid;
  }

private:
  float mGain{1.0F};
  std::size_t mPreparedChannels{0};
  std::size_t mPreparedMaximumBlock{0};
  bool mContractValid{true};
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

  GS_REQUIRE(gainA.contractValid());
  GS_REQUIRE(gainB.contractValid());

  const auto blockInvariant = gslab::compareGolden(
    render64,
    render127,
    gslab::GoldenTolerance{0.0, 0.0}
  );

  GS_REQUIRE(blockInvariant.passed());

  const auto report = gslab::analyzeFiniteAudio(render64);
  GS_REQUIRE(report.allFinite());
  GS_REQUIRE(report.peakAbsolute <= 0.200001);

  auto corrupt = render64;
  corrupt.channel(1)[12] = std::numeric_limits<float>::infinity();

  const auto corruptReport = gslab::analyzeFiniteAudio(corrupt);
  GS_REQUIRE(!corruptReport.allFinite());
  GS_REQUIRE(corruptReport.nonFiniteCount == 1);
  GS_REQUIRE(corruptReport.firstNonFiniteChannel == 1);
  GS_REQUIRE(corruptReport.firstNonFiniteFrame == 12);

  const auto mismatch = gslab::compareGolden(
    render64,
    corrupt,
    gslab::GoldenTolerance{1.0e-7, 0.0}
  );

  GS_REQUIRE(!mismatch.passed());
  GS_REQUIRE(mismatch.mismatchedSamples == 1);

  return 0;
}
