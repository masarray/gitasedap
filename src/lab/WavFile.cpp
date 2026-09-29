#include "lab/WavFile.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace gitasedap::lab
{
namespace
{

[[nodiscard]] std::uint16_t readU16(
  const std::vector<std::uint8_t>& bytes,
  std::size_t offset
)
{
  return static_cast<std::uint16_t>(bytes.at(offset))
      | static_cast<std::uint16_t>(
          static_cast<std::uint16_t>(bytes.at(offset + 1)) << 8U
        );
}

[[nodiscard]] std::uint32_t readU32(
  const std::vector<std::uint8_t>& bytes,
  std::size_t offset
)
{
  return static_cast<std::uint32_t>(bytes.at(offset))
      | (static_cast<std::uint32_t>(bytes.at(offset + 1)) << 8U)
      | (static_cast<std::uint32_t>(bytes.at(offset + 2)) << 16U)
      | (static_cast<std::uint32_t>(bytes.at(offset + 3)) << 24U);
}

void writeU16(std::ostream& output, std::uint16_t value)
{
  const std::uint8_t bytes[]{
    static_cast<std::uint8_t>(value & 0xFFU),
    static_cast<std::uint8_t>((value >> 8U) & 0xFFU)
  };

  output.write(
    reinterpret_cast<const char*>(bytes),
    static_cast<std::streamsize>(sizeof(bytes))
  );
}

void writeU32(std::ostream& output, std::uint32_t value)
{
  const std::uint8_t bytes[]{
    static_cast<std::uint8_t>(value & 0xFFU),
    static_cast<std::uint8_t>((value >> 8U) & 0xFFU),
    static_cast<std::uint8_t>((value >> 16U) & 0xFFU),
    static_cast<std::uint8_t>((value >> 24U) & 0xFFU)
  };

  output.write(
    reinterpret_cast<const char*>(bytes),
    static_cast<std::streamsize>(sizeof(bytes))
  );
}

[[nodiscard]] std::string fourCc(
  const std::vector<std::uint8_t>& bytes,
  std::size_t offset
)
{
  if(offset + 4 > bytes.size())
    throw std::runtime_error("truncated WAV chunk");

  return std::string(
    reinterpret_cast<const char*>(bytes.data() + offset),
    4
  );
}

[[nodiscard]] std::int32_t readSigned24(
  const std::vector<std::uint8_t>& bytes,
  std::size_t offset
)
{
  std::int32_t value =
    static_cast<std::int32_t>(bytes.at(offset))
    | (static_cast<std::int32_t>(bytes.at(offset + 1)) << 8)
    | (static_cast<std::int32_t>(bytes.at(offset + 2)) << 16);

  if((value & 0x00800000) != 0)
    value |= static_cast<std::int32_t>(0xFF000000);

  return value;
}

} // namespace

WavData WavFile::read(const std::filesystem::path& path)
{
  std::ifstream input(path, std::ios::binary | std::ios::ate);

  if(!input)
    throw std::runtime_error("unable to open WAV file");

  const auto end = input.tellg();

  if(end < 0)
    throw std::runtime_error("unable to determine WAV file size");

  const auto fileSize = static_cast<std::size_t>(end);

  constexpr std::size_t kMaximumFixtureBytes =
    static_cast<std::size_t>(1024) * 1024 * 1024;

  if(fileSize < 12 || fileSize > kMaximumFixtureBytes)
    throw std::runtime_error("WAV file size is outside laboratory limits");

  input.seekg(0, std::ios::beg);

  std::vector<std::uint8_t> bytes(fileSize);

  input.read(
    reinterpret_cast<char*>(bytes.data()),
    static_cast<std::streamsize>(bytes.size())
  );

  if(!input)
    throw std::runtime_error("unable to read complete WAV file");

  if(fourCc(bytes, 0) != "RIFF" || fourCc(bytes, 8) != "WAVE")
    throw std::runtime_error("not a RIFF/WAVE file");

  std::uint16_t audioFormat = 0;
  std::uint16_t channelCount = 0;
  std::uint32_t sampleRate = 0;
  std::uint16_t blockAlign = 0;
  std::uint16_t bitsPerSample = 0;
  std::size_t dataOffset = 0;
  std::size_t dataSize = 0;
  bool foundFormat = false;
  bool foundData = false;

  std::size_t offset = 12;

  while(offset + 8 <= bytes.size())
  {
    const auto id = fourCc(bytes, offset);
    const auto chunkSize = static_cast<std::size_t>(
      readU32(bytes, offset + 4)
    );
    const auto payloadOffset = offset + 8;

    if(payloadOffset + chunkSize > bytes.size())
      throw std::runtime_error("WAV chunk exceeds file bounds");

    if(id == "fmt ")
    {
      if(chunkSize < 16)
        throw std::runtime_error("WAV fmt chunk is too small");

      audioFormat = readU16(bytes, payloadOffset);
      channelCount = readU16(bytes, payloadOffset + 2);
      sampleRate = readU32(bytes, payloadOffset + 4);
      blockAlign = readU16(bytes, payloadOffset + 12);
      bitsPerSample = readU16(bytes, payloadOffset + 14);
      foundFormat = true;
    }
    else if(id == "data")
    {
      dataOffset = payloadOffset;
      dataSize = chunkSize;
      foundData = true;
    }

    offset = payloadOffset + chunkSize + (chunkSize & 1U);
  }

  if(!foundFormat || !foundData)
    throw std::runtime_error("WAV file is missing fmt or data chunk");

  if(channelCount == 0 || channelCount > 64)
    throw std::runtime_error("unsupported WAV channel count");

  if(sampleRate < 8000U || sampleRate > 384000U)
    throw std::runtime_error("unsupported WAV sample rate");

  const bool supportedPcm =
    audioFormat == 1U
    && (bitsPerSample == 16U
        || bitsPerSample == 24U
        || bitsPerSample == 32U);

  const bool supportedFloat =
    audioFormat == 3U && bitsPerSample == 32U;

  if(!supportedPcm && !supportedFloat)
    throw std::runtime_error("unsupported WAV sample encoding");

  const auto bytesPerSample =
    static_cast<std::size_t>(bitsPerSample / 8U);

  const auto expectedBlockAlign =
    static_cast<std::size_t>(channelCount) * bytesPerSample;

  if(blockAlign != expectedBlockAlign || blockAlign == 0)
    throw std::runtime_error("invalid WAV block alignment");

  if(dataSize % blockAlign != 0)
    throw std::runtime_error("WAV data chunk is not frame-aligned");

  const auto frameCount = dataSize / blockAlign;
  AudioBuffer audio(channelCount, frameCount);

  for(std::size_t frame = 0; frame < frameCount; ++frame)
  {
    for(std::size_t channel = 0; channel < channelCount; ++channel)
    {
      const auto sampleOffset =
        dataOffset
        + (frame * static_cast<std::size_t>(blockAlign))
        + (channel * bytesPerSample);

      float sample = 0.0F;

      if(audioFormat == 3U)
      {
        sample = std::bit_cast<float>(readU32(bytes, sampleOffset));
      }
      else if(bitsPerSample == 16U)
      {
        const auto raw = static_cast<std::int16_t>(
          readU16(bytes, sampleOffset)
        );
        sample = static_cast<float>(
          static_cast<double>(raw) / 32768.0
        );
      }
      else if(bitsPerSample == 24U)
      {
        const auto raw = readSigned24(bytes, sampleOffset);
        sample = static_cast<float>(
          static_cast<double>(raw) / 8388608.0
        );
      }
      else
      {
        const auto raw = static_cast<std::int32_t>(
          readU32(bytes, sampleOffset)
        );
        sample = static_cast<float>(
          static_cast<double>(raw) / 2147483648.0
        );
      }

      audio.channel(channel)[frame] = sample;
    }
  }

  return WavData{sampleRate, std::move(audio)};
}

void WavFile::writeFloat32(
  const std::filesystem::path& path,
  std::uint32_t sampleRate,
  const AudioBuffer& audio
)
{
  if(sampleRate < 8000U || sampleRate > 384000U)
    throw std::invalid_argument("unsupported WAV sample rate");

  if(audio.channelCount() == 0 || audio.channelCount() > 64)
    throw std::invalid_argument("unsupported WAV channel count");

  constexpr std::uint32_t bytesPerSample = 4U;

  const auto blockAlign64 =
    audio.channelCount() * bytesPerSample;
  const auto dataSize64 =
    audio.frameCount() * blockAlign64;

  if(
    blockAlign64 > std::numeric_limits<std::uint16_t>::max()
    || dataSize64 > std::numeric_limits<std::uint32_t>::max()
  )
  {
    throw std::runtime_error("fixture exceeds RIFF/WAVE 32-bit limits");
  }

  const auto blockAlign = static_cast<std::uint16_t>(blockAlign64);
  const auto dataSize = static_cast<std::uint32_t>(dataSize64);
  const auto riffSize = static_cast<std::uint32_t>(36U + dataSize);
  const auto byteRate = sampleRate * static_cast<std::uint32_t>(blockAlign);

  std::ofstream output(path, std::ios::binary | std::ios::trunc);

  if(!output)
    throw std::runtime_error("unable to create WAV file");

  output.write("RIFF", 4);
  writeU32(output, riffSize);
  output.write("WAVE", 4);

  output.write("fmt ", 4);
  writeU32(output, 16U);
  writeU16(output, 3U);
  writeU16(output, static_cast<std::uint16_t>(audio.channelCount()));
  writeU32(output, sampleRate);
  writeU32(output, byteRate);
  writeU16(output, blockAlign);
  writeU16(output, 32U);

  output.write("data", 4);
  writeU32(output, dataSize);

  for(std::size_t frame = 0; frame < audio.frameCount(); ++frame)
  {
    for(std::size_t channel = 0; channel < audio.channelCount(); ++channel)
    {
      const auto bits = std::bit_cast<std::uint32_t>(
        audio.channel(channel)[frame]
      );
      writeU32(output, bits);
    }
  }

  if(!output)
    throw std::runtime_error("failed while writing WAV file");
}

} // namespace gitasedap::lab
