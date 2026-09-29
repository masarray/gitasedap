#include "core/ParameterSpec.h"
#include "dsp/SourceAdapter.h"
#include "lab/DeadlineBenchmark.h"
#include "lab/FiniteAudioGuard.h"
#include "lab/GoldenCompare.h"
#include "lab/SignalGenerator.h"
#include "lab/WavFile.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace gs = gitasedap::core;
namespace gsdsp = gitasedap::dsp;
namespace gslab = gitasedap::lab;

namespace
{

[[nodiscard]] std::uint32_t parseU32(
  const char* text,
  std::uint32_t fallback
)
{
  if(text == nullptr)
    return fallback;

  std::uint32_t value = 0;
  const std::string_view view(text);

  const auto result = std::from_chars(
    view.data(),
    view.data() + view.size(),
    value
  );

  if(result.ec != std::errc{} || result.ptr != view.data() + view.size())
    throw std::invalid_argument("invalid unsigned integer argument");

  return value;
}

[[nodiscard]] gslab::AudioBuffer generateSignal(
  std::string_view type,
  std::uint32_t sampleRate,
  std::size_t frames
)
{
  if(type == "impulse")
    return gslab::SignalGenerator::impulse(1, frames);

  if(type == "sine")
  {
    return gslab::SignalGenerator::sine(
      1,
      frames,
      sampleRate,
      440.0
    );
  }

  if(type == "sweep")
  {
    return gslab::SignalGenerator::logarithmicSweep(
      1,
      frames,
      sampleRate,
      20.0,
      std::min(20000.0, static_cast<double>(sampleRate) * 0.45)
    );
  }

  if(type == "white")
    return gslab::SignalGenerator::whiteNoise(1, frames);

  if(type == "pink")
    return gslab::SignalGenerator::pinkNoise(1, frames);

  throw std::invalid_argument(
    "signal type must be impulse|sine|sweep|white|pink"
  );
}

[[nodiscard]] gslab::AudioBuffer makeSyntheticQuackFixture(
  std::uint32_t sampleRate
)
{
  const auto frames = static_cast<std::size_t>(sampleRate);
  gslab::AudioBuffer audio(1, frames);

  const auto writeTone = [&](
    double startSeconds,
    double durationSeconds,
    double frequencyHz,
    double amplitude
  ) {
    const auto start = static_cast<std::size_t>(
      std::llround(startSeconds * static_cast<double>(sampleRate))
    );
    const auto length = static_cast<std::size_t>(
      std::llround(durationSeconds * static_cast<double>(sampleRate))
    );

    for(std::size_t frame = 0;
        frame < length && start + frame < frames;
        ++frame)
    {
      const double phase =
        2.0
        * std::numbers::pi
        * frequencyHz
        * static_cast<double>(frame)
        / static_cast<double>(sampleRate);

      audio.channel(0)[start + frame] = static_cast<float>(
        std::sin(phase) * amplitude
      );
    }
  };

  writeTone(0.10, 0.030, 2600.0, 0.50);
  writeTone(0.20, 0.025, 5200.0, 0.35);
  writeTone(0.35, 0.450, 2600.0, 0.20);

  return audio;
}

[[nodiscard]] gslab::AudioBuffer processSourceAdapter(
  const gslab::AudioBuffer& input,
  gsdsp::SourceAdapter& adapter
)
{
  gslab::AudioBuffer output(
    input.channelCount(),
    input.frameCount()
  );

  adapter.beginBlock();

  for(std::size_t frame = 0; frame < input.frameCount(); ++frame)
  {
    for(std::size_t channel = 0; channel < input.channelCount(); ++channel)
    {
      output.channel(channel)[frame] = static_cast<float>(
        adapter.processSample(
          static_cast<double>(input.channel(channel)[frame])
        )
      );
    }
  }

  (void) adapter.endBlock();
  return output;
}

void printBenchmark(
  const gslab::DeadlineBenchmarkResult& result
)
{
  std::cout
    << "iterations=" << result.iterations << "\n"
    << "deadline_us=" << result.deadlineMicroseconds << "\n"
    << "mean_us=" << result.meanMicroseconds << "\n"
    << "p95_us=" << result.p95Microseconds << "\n"
    << "p99_us=" << result.p99Microseconds << "\n"
    << "p999_us=" << result.p999Microseconds << "\n"
    << "max_us=" << result.maximumMicroseconds << "\n"
    << "p99_deadline_ratio=" << result.p99DeadlineRatio << "\n";
}

void printUsage()
{
  std::cout
    << "GitaSedap DSP Lab\n"
    << "  generate <type> <out.wav> [sampleRate] [frames]\n"
    << "  inspect <file.wav>\n"
    << "  compare <expected.wav> <actual.wav>\n"
    << "  benchmark [sampleRate] [blockSize] [iterations]\n"
    << "  benchmark-source [sampleRate] [blockSize] [iterations]\n"
    << "  source-demo <input.wav> <processed.wav> [sampleRate]\n";
}

int commandGenerate(int argc, char** argv)
{
  if(argc < 4)
    throw std::invalid_argument("generate requires type and output path");

  const auto sampleRate = parseU32(argc > 4 ? argv[4] : nullptr, 48000U);
  const auto frames = static_cast<std::size_t>(
    parseU32(argc > 5 ? argv[5] : nullptr, sampleRate)
  );

  auto audio = generateSignal(argv[2], sampleRate, frames);
  gslab::WavFile::writeFloat32(argv[3], sampleRate, audio);

  const auto report = gslab::analyzeFiniteAudio(audio);

  std::cout
    << "generated=" << argv[3] << "\n"
    << "sample_rate=" << sampleRate << "\n"
    << "channels=" << audio.channelCount() << "\n"
    << "frames=" << audio.frameCount() << "\n"
    << "finite=" << (report.allFinite() ? "true" : "false") << "\n"
    << "peak=" << report.peakAbsolute << "\n";

  return report.allFinite() ? 0 : 2;
}

int commandInspect(int argc, char** argv)
{
  if(argc < 3)
    throw std::invalid_argument("inspect requires a WAV path");

  const auto wav = gslab::WavFile::read(argv[2]);
  const auto report = gslab::analyzeFiniteAudio(wav.audio);

  std::cout
    << "sample_rate=" << wav.sampleRate << "\n"
    << "channels=" << wav.audio.channelCount() << "\n"
    << "frames=" << wav.audio.frameCount() << "\n"
    << "finite=" << (report.allFinite() ? "true" : "false") << "\n"
    << "non_finite=" << report.nonFiniteCount << "\n"
    << "peak=" << report.peakAbsolute << "\n"
    << "rms=" << report.rms << "\n";

  return report.allFinite() ? 0 : 2;
}

int commandCompare(int argc, char** argv)
{
  if(argc < 4)
    throw std::invalid_argument("compare requires expected and actual WAVs");

  const auto expected = gslab::WavFile::read(argv[2]);
  const auto actual = gslab::WavFile::read(argv[3]);

  if(expected.sampleRate != actual.sampleRate)
  {
    std::cout << "sample_rate_match=false\n";
    return 2;
  }

  const auto result = gslab::compareGolden(
    expected.audio,
    actual.audio
  );

  std::cout
    << "shape_match=" << (result.shapeMatches ? "true" : "false") << "\n"
    << "samples=" << result.sampleCount << "\n"
    << "mismatched=" << result.mismatchedSamples << "\n"
    << "max_abs_error=" << result.maximumAbsoluteError << "\n"
    << "rms_error=" << result.rmsError << "\n"
    << "passed=" << (result.passed() ? "true" : "false") << "\n";

  return result.passed() ? 0 : 2;
}

int commandBenchmark(int argc, char** argv)
{
  const auto sampleRate = parseU32(argc > 2 ? argv[2] : nullptr, 48000U);
  const auto blockSize = static_cast<std::size_t>(
    parseU32(argc > 3 ? argv[3] : nullptr, 64U)
  );
  const auto iterations = static_cast<std::size_t>(
    parseU32(argc > 4 ? argv[4] : nullptr, 5000U)
  );

  std::vector<float> buffer(blockSize, 0.125F);
  volatile float sink = 0.0F;

  const auto result = gslab::DeadlineBenchmark::run(
    sampleRate,
    blockSize,
    256,
    iterations,
    [&] {
      float local = 0.0F;

      for(auto& sample : buffer)
      {
        sample = (sample * 0.99991F) + 0.00001F;
        local += sample;
      }

      sink = local;
    }
  );

  std::cout
    << "sample_rate=" << sampleRate << "\n"
    << "block_size=" << blockSize << "\n";

  printBenchmark(result);
  std::cout << "sink=" << sink << "\n";

  return 0;
}

int commandBenchmarkSource(int argc, char** argv)
{
  const auto sampleRate = parseU32(argc > 2 ? argv[2] : nullptr, 48000U);
  const auto blockSize = static_cast<std::size_t>(
    parseU32(argc > 3 ? argv[3] : nullptr, 64U)
  );
  const auto iterations = static_cast<std::size_t>(
    parseU32(argc > 4 ? argv[4] : nullptr, 10000U)
  );

  if(blockSize == 0)
    throw std::invalid_argument("blockSize must be non-zero");

  std::vector<double> input(blockSize, 0.0);

  for(std::size_t frame = 0; frame < blockSize; ++frame)
  {
    const double time =
      static_cast<double>(frame) / static_cast<double>(sampleRate);

    const double attackWeight = frame < 12 ? 1.0 : 0.25;

    input[frame] =
      (0.20 * std::sin(2.0 * std::numbers::pi * 220.0 * time))
      + (
        attackWeight
        * 0.22
        * std::sin(2.0 * std::numbers::pi * 2600.0 * time)
      );
  }

  gsdsp::SourceAdapter adapter;
  adapter.setSourceType(gs::InputSource::ActivePiezo);
  adapter.prepare(sampleRate);

  volatile double sink = 0.0;

  const auto result = gslab::DeadlineBenchmark::run(
    sampleRate,
    blockSize,
    512,
    iterations,
    [&] {
      adapter.beginBlock();

      double local = 0.0;

      for(const double sample : input)
        local += adapter.processSample(sample);

      const auto meter = adapter.endBlock();
      sink = local + meter.maximumReductionDb;
    }
  );

  std::cout
    << "benchmark=source_adapter\n"
    << "sample_rate=" << sampleRate << "\n"
    << "block_size=" << blockSize << "\n";

  printBenchmark(result);
  std::cout << "sink=" << sink << "\n";

  return 0;
}

int commandSourceDemo(int argc, char** argv)
{
  if(argc < 4)
  {
    throw std::invalid_argument(
      "source-demo requires input and processed output paths"
    );
  }

  const auto sampleRate = parseU32(
    argc > 4 ? argv[4] : nullptr,
    48000U
  );

  auto input = makeSyntheticQuackFixture(sampleRate);

  gsdsp::SourceAdapter adapter;
  adapter.setSourceType(gs::InputSource::ActivePiezo);
  adapter.prepare(sampleRate);

  auto processed = processSourceAdapter(
    input,
    adapter
  );

  gslab::WavFile::writeFloat32(argv[2], sampleRate, input);
  gslab::WavFile::writeFloat32(argv[3], sampleRate, processed);

  const auto meter = adapter.lastMeter();
  const auto report = gslab::analyzeFiniteAudio(processed);

  std::cout
    << "fixture=synthetic_quack\n"
    << "sample_rate=" << sampleRate << "\n"
    << "input=" << argv[2] << "\n"
    << "processed=" << argv[3] << "\n"
    << "finite=" << (report.allFinite() ? "true" : "false") << "\n"
    << "input_peak=" << meter.inputPeakLinear << "\n"
    << "input_headroom_db=" << meter.inputHeadroomDb << "\n"
    << "max_transient_score=" << meter.maximumTransientScore << "\n"
    << "max_reduction_db=" << meter.maximumReductionDb << "\n";

  return report.allFinite() ? 0 : 2;
}

} // namespace

int main(int argc, char** argv)
{
  try
  {
    if(argc < 2)
    {
      printUsage();
      return 1;
    }

    const std::string_view command(argv[1]);

    if(command == "generate")
      return commandGenerate(argc, argv);

    if(command == "inspect")
      return commandInspect(argc, argv);

    if(command == "compare")
      return commandCompare(argc, argv);

    if(command == "benchmark")
      return commandBenchmark(argc, argv);

    if(command == "benchmark-source")
      return commandBenchmarkSource(argc, argv);

    if(command == "source-demo")
      return commandSourceDemo(argc, argv);

    printUsage();
    return 1;
  }
  catch(const std::exception& error)
  {
    std::cerr << "error=" << error.what() << "\n";
    return 2;
  }
}
