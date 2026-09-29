#pragma once

#include <cassert>
#include <cstddef>
#include <span>
#include <vector>

namespace gitasedap::lab
{

class AudioBuffer
{
public:
  AudioBuffer() = default;

  AudioBuffer(std::size_t channels, std::size_t frames)
  {
    resize(channels, frames);
  }

  void resize(std::size_t channels, std::size_t frames)
  {
    mChannels = channels;
    mFrames = frames;
    mData.assign(channels * frames, 0.0F);
  }

  void clear() noexcept
  {
    for(auto& sample : mData)
      sample = 0.0F;
  }

  [[nodiscard]] std::size_t channelCount() const noexcept
  {
    return mChannels;
  }

  [[nodiscard]] std::size_t frameCount() const noexcept
  {
    return mFrames;
  }

  [[nodiscard]] bool empty() const noexcept
  {
    return mChannels == 0 || mFrames == 0;
  }

  [[nodiscard]] float* channelData(std::size_t channel) noexcept
  {
    assert(channel < mChannels);
    return mData.data() + (channel * mFrames);
  }

  [[nodiscard]] const float* channelData(std::size_t channel) const noexcept
  {
    assert(channel < mChannels);
    return mData.data() + (channel * mFrames);
  }

  [[nodiscard]] std::span<float> channel(std::size_t channel) noexcept
  {
    return {channelData(channel), mFrames};
  }

  [[nodiscard]] std::span<const float> channel(
    std::size_t channel
  ) const noexcept
  {
    return {channelData(channel), mFrames};
  }

  [[nodiscard]] std::span<float> storage() noexcept
  {
    return mData;
  }

  [[nodiscard]] std::span<const float> storage() const noexcept
  {
    return mData;
  }

private:
  std::size_t mChannels{0};
  std::size_t mFrames{0};
  std::vector<float> mData;
};

} // namespace gitasedap::lab
