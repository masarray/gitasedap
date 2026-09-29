#include "lab/OfflineRenderer.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace gitasedap::lab
{

AudioBuffer OfflineRenderer::render(
  const AudioBuffer& input,
  double sampleRate,
  IOfflineProcessor& processor,
  const RenderOptions& options
)
{
  if(sampleRate <= 0.0)
    throw std::invalid_argument("sampleRate must be positive");

  if(options.blockSize == 0)
    throw std::invalid_argument("blockSize must be non-zero");

  AudioBuffer output = input;

  const PrepareSpec spec{
    sampleRate,
    options.blockSize,
    input.channelCount()
  };

  processor.prepare(spec);

  if(options.resetBeforeRender)
    processor.reset();

  std::vector<float*> channelPointers(input.channelCount(), nullptr);

  std::size_t frameOffset = 0;

  while(frameOffset < output.frameCount())
  {
    const auto framesThisBlock = std::min(
      options.blockSize,
      output.frameCount() - frameOffset
    );

    for(std::size_t channel = 0; channel < output.channelCount(); ++channel)
    {
      channelPointers[channel] =
        output.channelData(channel) + frameOffset;
    }

    processor.process(
      channelPointers.data(),
      channelPointers.size(),
      framesThisBlock
    );

    frameOffset += framesThisBlock;
  }

  return output;
}

} // namespace gitasedap::lab
