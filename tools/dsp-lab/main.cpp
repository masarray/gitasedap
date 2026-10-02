#include "core/ParameterSpec.h"
#include "dsp/BodyProfile.h"
#include "dsp/BodyProfileCompiler.h"
#include "dsp/CrossfadingBodyEngine.h"
#include "dsp/HybridBodyEngine.h"
#include "dsp/SourceAdapter.h"
#include "dsp/TransferFilter.h"
#include "lab/DeadlineBenchmark.h"
#include "lab/FiniteAudioGuard.h"
#include "lab/GoldenCompare.h"
#include "lab/RealGuitarCalibration.h"
#include "lab/SignalGenerator.h"
#include "lab/WavFile.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>
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

[[nodiscard]] gslab::AudioBuffer makeSyntheticBodyFixture(
  std::uint32_t sampleRate
)
{
  const auto frames = static_cast<std::size_t>(sampleRate);
  gslab::AudioBuffer audio(1, frames);

  const std::array<double, 6> frequencies{
    82.41,
    110.00,
    146.83,
    196.00,
    246.94,
    329.63
  };

  for(std::size_t frame = 0; frame < frames; ++frame)
  {
    const double time =
      static_cast<double>(frame) / static_cast<double>(sampleRate);

    const double decay = std::exp(-time * 5.0);
    const double attack = std::exp(-time * 70.0);

    double sample = 0.0;

    for(std::size_t harmonic = 0;
        harmonic < frequencies.size();
        ++harmonic)
    {
      sample +=
        std::sin(
          2.0
          * std::numbers::pi
          * frequencies[harmonic]
          * time
        )
        * (0.11 / static_cast<double>(harmonic + 1));
    }

    sample *= decay;

    sample +=
      0.10
      * attack
      * std::sin(
        2.0 * std::numbers::pi * 2800.0 * time
      );

    audio.channel(0)[frame] =
      static_cast<float>(std::clamp(sample, -0.9, 0.9));
  }

  return audio;
}

[[nodiscard]] gslab::AudioBuffer processBody(
  const gslab::AudioBuffer& input,
  double sampleRate,
  const gsdsp::PreparedBodyProfile& profile,
  double bodyAmount
)
{
  gsdsp::HybridBodyEngine engine;
  engine.prepare(sampleRate, bodyAmount);
  engine.configure(profile);

  gslab::AudioBuffer output(
    input.channelCount(),
    input.frameCount()
  );

  for(std::size_t channel = 0;
      channel < input.channelCount();
      ++channel)
  {
    engine.reset();

    for(std::size_t frame = 0;
        frame < input.frameCount();
        ++frame)
    {
      output.channel(channel)[frame] =
        static_cast<float>(
          engine.processSample(
            static_cast<double>(
              input.channel(channel)[frame]
            )
          )
        );
    }
  }

  return output;
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

[[nodiscard]] gslab::AudioBuffer firstChannel(
  const gslab::AudioBuffer& input
)
{
  if(input.channelCount() == 0)
    throw std::invalid_argument("audio has no channels");

  gslab::AudioBuffer output(1, input.frameCount());

  std::copy(
    input.channel(0).begin(),
    input.channel(0).end(),
    output.channel(0).begin()
  );

  return output;
}

[[nodiscard]] gslab::AudioBuffer selectedChannel(
  const gslab::AudioBuffer& input,
  std::size_t channel
)
{
  if(channel >= input.channelCount())
    throw std::invalid_argument("requested channel does not exist");

  gslab::AudioBuffer output(1, input.frameCount());

  std::copy(
    input.channel(channel).begin(),
    input.channel(channel).end(),
    output.channel(0).begin()
  );

  return output;
}

void printCalibrationSummary(
  const gslab::RealGuitarCalibrationReport& report
)
{
  std::cout
    << std::setprecision(10)
    << "sample_rate=" << report.sampleRate << "\n"
    << "analyzed_frames=" << report.analyzedFrames << "\n"
    << "duration_seconds=" << report.durationSeconds << "\n"
    << "piezo_peak_dbfs=" << report.piezoPeakDbFs << "\n"
    << "reference_peak_dbfs=" << report.referencePeakDbFs << "\n"
    << "piezo_rms_dbfs=" << report.piezoRmsDbFs << "\n"
    << "reference_rms_dbfs=" << report.referenceRmsDbFs << "\n"
    << "reference_lag_samples=" << report.estimatedReferenceLagSamples << "\n"
    << "reference_lag_ms=" << report.estimatedReferenceLagMilliseconds << "\n"
    << "alignment_score=" << report.alignmentScore << "\n"
    << "transfer_normalization_db=" << report.transferNormalizationDb << "\n"
    << "spectral_shape_rms_db=" << report.spectralShapeRmsDb << "\n"
    << "spectral_shape_max_abs_db=" << report.spectralShapeMaxAbsDb << "\n"
    << "body_band_mean_db=" << report.bodyBandMeanDb << "\n"
    << "quack_band_mean_db=" << report.quackBandMeanDb << "\n"
    << "air_band_mean_db=" << report.airBandMeanDb << "\n"
    << "supported_bands=" << report.supportedBandCount << "\n"
    << "mode_candidates=" << report.modeCandidateCount << "\n"
    << "issue_mask="
    << static_cast<std::uint32_t>(report.issues)
    << "\n"
    << "capture_accepted="
    << (report.captureAcceptedForTuning() ? "true" : "false")
    << "\n";

  for(std::size_t index = 0;
      index < report.modeCandidateCount;
      ++index)
  {
    const auto& candidate = report.modeCandidates[index];

    std::cout
      << "mode_" << index << "_frequency_hz="
      << candidate.frequencyHz << "\n"
      << "mode_" << index << "_prominence_db="
      << candidate.prominenceDb << "\n"
      << "mode_" << index << "_estimated_q="
      << candidate.estimatedQ << "\n"
      << "mode_" << index << "_confidence="
      << candidate.confidence << "\n";
  }
}

void writeCalibrationCsv(
  const std::string& path,
  const gslab::RealGuitarCalibrationReport& report
)
{
  std::ofstream output(path, std::ios::trunc);

  if(!output)
    throw std::runtime_error("unable to create calibration report");

  output << std::setprecision(10);

  output
    << "# sample_rate," << report.sampleRate << "\n"
    << "# analyzed_frames," << report.analyzedFrames << "\n"
    << "# duration_seconds," << report.durationSeconds << "\n"
    << "# piezo_peak_dbfs," << report.piezoPeakDbFs << "\n"
    << "# reference_peak_dbfs," << report.referencePeakDbFs << "\n"
    << "# piezo_rms_dbfs," << report.piezoRmsDbFs << "\n"
    << "# reference_rms_dbfs," << report.referenceRmsDbFs << "\n"
    << "# reference_lag_samples,"
    << report.estimatedReferenceLagSamples << "\n"
    << "# reference_lag_ms,"
    << report.estimatedReferenceLagMilliseconds << "\n"
    << "# alignment_score," << report.alignmentScore << "\n"
    << "# transfer_normalization_db,"
    << report.transferNormalizationDb << "\n"
    << "# spectral_shape_rms_db,"
    << report.spectralShapeRmsDb << "\n"
    << "# spectral_shape_max_abs_db,"
    << report.spectralShapeMaxAbsDb << "\n"
    << "# body_band_mean_db," << report.bodyBandMeanDb << "\n"
    << "# quack_band_mean_db," << report.quackBandMeanDb << "\n"
    << "# air_band_mean_db," << report.airBandMeanDb << "\n"
    << "# supported_bands," << report.supportedBandCount << "\n"
    << "# issue_mask,"
    << static_cast<std::uint32_t>(report.issues) << "\n"
    << "# capture_accepted,"
    << (report.captureAcceptedForTuning() ? "true" : "false")
    << "\n";

  for(std::size_t index = 0;
      index < report.modeCandidateCount;
      ++index)
  {
    const auto& candidate = report.modeCandidates[index];

    output
      << "# mode_candidate," << index
      << "," << candidate.frequencyHz
      << "," << candidate.prominenceDb
      << "," << candidate.estimatedQ
      << "," << candidate.confidence
      << "\n";
  }

  output
    << "frequency_hz,raw_transfer_db,relative_transfer_db,"
       "piezo_support_db,supported\n";

  for(const auto& band : report.bands)
  {
    output
      << band.frequencyHz << ","
      << band.rawTransferDb << ","
      << band.relativeTransferDb << ","
      << band.piezoSupportDb << ","
      << (band.supported ? 1 : 0)
      << "\n";
  }

  if(!output)
    throw std::runtime_error("failed while writing calibration report");
}

[[nodiscard]] gs::InputSource parseSourceType(
  std::string_view text
)
{
  if(text == "active")
    return gs::InputSource::ActivePiezo;

  if(text == "passive")
    return gs::InputSource::PassivePiezo;

  if(text == "magnetic")
    return gs::InputSource::Magnetic;

  throw std::invalid_argument(
    "source must be active|passive|magnetic"
  );
}

[[nodiscard]] const gsdsp::BodyProfileDefinition&
parseBodyProfile(std::string_view text)
{
  if(text == "raw")
    return gsdsp::rawConditionedProfile();

  if(text == "natural")
    return gsdsp::naturalDevelopmentProfile();

  if(text == "dread")
    return gsdsp::dreadnoughtDevelopmentProfile();

  throw std::invalid_argument(
    "profile must be raw|natural|dread"
  );
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
    << "  benchmark-transfer [sampleRate] [blockSize] [iterations]\n"
    << "  benchmark-body [sampleRate] [blockSize] [iterations]\n"
    << "  benchmark-body-switch [sampleRate] [blockSize] [iterations]\n"
    << "  source-demo <input.wav> <processed.wav> [sampleRate]\n"
    << "  body-demo <input.wav> <processed.wav> [sampleRate]\n"
    << "  profile-compare <raw.wav> <natural.wav> <dread.wav> [sampleRate]\n"
    << "  calibrate-pair <piezo.wav> <reference.wav> <report.csv>\n"
    << "  calibrate-stereo <capture.wav> <report.csv>\n"
    << "  evaluate-profile <piezo.wav> <reference.wav> <profile> <source> <processed.wav> <report.csv>\n";
}

int commandCalibratePair(int argc, char** argv)
{
  if(argc < 5)
  {
    throw std::invalid_argument(
      "calibrate-pair requires piezo WAV, reference WAV, and report path"
    );
  }

  const auto piezoWav = gslab::WavFile::read(argv[2]);
  const auto referenceWav = gslab::WavFile::read(argv[3]);

  if(piezoWav.sampleRate != referenceWav.sampleRate)
  {
    throw std::invalid_argument(
      "piezo and reference WAV sample rates must match"
    );
  }

  const auto piezo = firstChannel(piezoWav.audio);
  const auto reference = firstChannel(referenceWav.audio);

  const auto report = gslab::analyzeRealGuitarPair(
    piezo,
    reference,
    static_cast<double>(piezoWav.sampleRate)
  );

  writeCalibrationCsv(argv[4], report);
  printCalibrationSummary(report);

  return report.captureAcceptedForTuning() ? 0 : 2;
}

int commandCalibrateStereo(int argc, char** argv)
{
  if(argc < 4)
  {
    throw std::invalid_argument(
      "calibrate-stereo requires capture WAV and report path"
    );
  }

  const auto capture = gslab::WavFile::read(argv[2]);

  if(capture.audio.channelCount() < 2)
  {
    throw std::invalid_argument(
      "stereo calibration capture needs piezo on channel 1 and reference on channel 2"
    );
  }

  const auto piezo = selectedChannel(capture.audio, 0);
  const auto reference = selectedChannel(capture.audio, 1);

  const auto report = gslab::analyzeRealGuitarPair(
    piezo,
    reference,
    static_cast<double>(capture.sampleRate)
  );

  writeCalibrationCsv(argv[3], report);
  printCalibrationSummary(report);

  return report.captureAcceptedForTuning() ? 0 : 2;
}

int commandEvaluateProfile(int argc, char** argv)
{
  if(argc < 8)
  {
    throw std::invalid_argument(
      "evaluate-profile requires piezo WAV, reference WAV, profile, source, processed WAV, and report path"
    );
  }

  const auto piezoWav = gslab::WavFile::read(argv[2]);
  const auto referenceWav = gslab::WavFile::read(argv[3]);

  if(piezoWav.sampleRate != referenceWav.sampleRate)
  {
    throw std::invalid_argument(
      "piezo and reference WAV sample rates must match"
    );
  }

  const auto sourceType = parseSourceType(argv[5]);
  const auto& profile = parseBodyProfile(argv[4]);

  auto piezo = firstChannel(piezoWav.audio);
  const auto reference = firstChannel(referenceWav.audio);

  gsdsp::SourceAdapter adapter;
  adapter.setSourceType(sourceType);
  adapter.prepare(piezoWav.sampleRate);

  auto conditioned = processSourceAdapter(
    piezo,
    adapter
  );

  const auto compiled = gsdsp::compileBodyProfile(
    profile,
    piezoWav.sampleRate
  );

  if(!compiled.ok())
    throw std::runtime_error("unable to compile selected body profile");

  auto processed = processBody(
    conditioned,
    piezoWav.sampleRate,
    compiled.prepared,
    1.0
  );

  gslab::WavFile::writeFloat32(
    argv[6],
    piezoWav.sampleRate,
    processed
  );

  const auto report = gslab::analyzeRealGuitarPair(
    processed,
    reference,
    static_cast<double>(piezoWav.sampleRate)
  );

  writeCalibrationCsv(argv[7], report);

  std::cout
    << "profile=" << profile.canonicalKey << "\n"
    << "source=" << argv[5] << "\n"
    << "processed=" << argv[6] << "\n"
    << "report=" << argv[7] << "\n";

  printCalibrationSummary(report);

  return report.captureAcceptedForTuning() ? 0 : 2;
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

int commandBenchmarkTransfer(int argc, char** argv)
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

  const auto compiled = gsdsp::compileBodyProfile(
    gsdsp::naturalDevelopmentProfile(),
    sampleRate
  );

  if(!compiled.ok())
    throw std::runtime_error("unable to compile Natural development profile");

  gsdsp::TransferFilter filter;
  filter.configure(compiled.prepared.transfer);

  std::vector<double> input(blockSize, 0.0);

  for(std::size_t frame = 0; frame < blockSize; ++frame)
  {
    input[frame] =
      0.2
      * std::sin(
        2.0
        * std::numbers::pi
        * 440.0
        * static_cast<double>(frame)
        / static_cast<double>(sampleRate)
      );
  }

  volatile double sink = 0.0;

  const auto result = gslab::DeadlineBenchmark::run(
    sampleRate,
    blockSize,
    512,
    iterations,
    [&] {
      double local = 0.0;

      for(const double sample : input)
        local += filter.processSample(sample);

      sink = local;
    }
  );

  std::cout
    << "benchmark=transfer_filter\n"
    << "sample_rate=" << sampleRate << "\n"
    << "block_size=" << blockSize << "\n"
    << "tap_count=" << compiled.prepared.transfer.tapCount << "\n";

  printBenchmark(result);
  std::cout << "sink=" << sink << "\n";

  return 0;
}

int commandBenchmarkBody(int argc, char** argv)
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

  const auto compiled = gsdsp::compileBodyProfile(
    gsdsp::naturalDevelopmentProfile(),
    sampleRate
  );

  if(!compiled.ok())
    throw std::runtime_error("unable to compile Natural development profile");

  gsdsp::HybridBodyEngine engine;
  engine.prepare(sampleRate, 1.0);
  engine.configure(compiled.prepared);

  std::vector<double> input(blockSize, 0.0);

  for(std::size_t frame = 0; frame < blockSize; ++frame)
  {
    const double time =
      static_cast<double>(frame) / static_cast<double>(sampleRate);

    input[frame] =
      (0.16 * std::sin(2.0 * std::numbers::pi * 110.0 * time))
      + (0.08 * std::sin(2.0 * std::numbers::pi * 220.0 * time))
      + (0.04 * std::sin(2.0 * std::numbers::pi * 880.0 * time));
  }

  volatile double sink = 0.0;

  const auto result = gslab::DeadlineBenchmark::run(
    sampleRate,
    blockSize,
    512,
    iterations,
    [&] {
      double local = 0.0;

      for(const double sample : input)
        local += engine.processSample(sample);

      sink = local;
    }
  );

  std::cout
    << "benchmark=hybrid_body\n"
    << "sample_rate=" << sampleRate << "\n"
    << "block_size=" << blockSize << "\n"
    << "tap_count=" << compiled.prepared.transfer.tapCount << "\n"
    << "mode_count=" << compiled.prepared.modal.modeCount << "\n"
    << "profile_hash=" << compiled.prepared.contentHash << "\n";

  printBenchmark(result);
  std::cout << "sink=" << sink << "\n";

  return 0;
}

int commandBenchmarkBodySwitch(int argc, char** argv)
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

  const auto natural = gsdsp::compileBodyProfile(
    gsdsp::naturalDevelopmentProfile(),
    sampleRate
  );

  const auto dreadnought = gsdsp::compileBodyProfile(
    gsdsp::dreadnoughtDevelopmentProfile(),
    sampleRate
  );

  if(!natural.ok() || !dreadnought.ok())
    throw std::runtime_error("unable to compile P4B development profiles");

  gsdsp::CrossfadingBodyEngine engine;
  engine.prepare(sampleRate, 1.0, 15.0);
  engine.activateInitial(natural.prepared);

  std::vector<double> input(blockSize, 0.0);

  for(std::size_t frame = 0; frame < blockSize; ++frame)
  {
    const double time =
      static_cast<double>(frame) / static_cast<double>(sampleRate);

    input[frame] =
      (0.16 * std::sin(2.0 * std::numbers::pi * 110.0 * time))
      + (0.08 * std::sin(2.0 * std::numbers::pi * 220.0 * time))
      + (0.04 * std::sin(2.0 * std::numbers::pi * 880.0 * time));
  }

  bool targetDreadnought = true;
  volatile double sink = 0.0;

  const auto result = gslab::DeadlineBenchmark::run(
    sampleRate,
    blockSize,
    512,
    iterations,
    [&] {
      if(!engine.isTransitioning())
      {
        (void) engine.beginProfileTransition(
          targetDreadnought
            ? dreadnought.prepared
            : natural.prepared
        );

        targetDreadnought = !targetDreadnought;
      }

      double local = 0.0;

      for(const double sample : input)
        local += engine.processSample(sample);

      sink = local;
    }
  );

  std::cout
    << "benchmark=hybrid_body_switch\n"
    << "sample_rate=" << sampleRate << "\n"
    << "block_size=" << blockSize << "\n"
    << "transition_samples=" << engine.transitionSamples() << "\n";

  printBenchmark(result);
  std::cout << "sink=" << sink << "\n";

  return 0;
}

int commandBodyDemo(int argc, char** argv)
{
  if(argc < 4)
  {
    throw std::invalid_argument(
      "body-demo requires input and processed output paths"
    );
  }

  const auto sampleRate = parseU32(
    argc > 4 ? argv[4] : nullptr,
    48000U
  );

  const auto compiled = gsdsp::compileBodyProfile(
    gsdsp::naturalDevelopmentProfile(),
    sampleRate
  );

  if(!compiled.ok())
    throw std::runtime_error("unable to compile Natural development profile");

  auto input = makeSyntheticBodyFixture(sampleRate);
  auto processed = processBody(
    input,
    sampleRate,
    compiled.prepared,
    1.0
  );

  gslab::WavFile::writeFloat32(argv[2], sampleRate, input);
  gslab::WavFile::writeFloat32(argv[3], sampleRate, processed);

  const auto report = gslab::analyzeFiniteAudio(processed);

  std::cout
    << "fixture=synthetic_body_engineering\n"
    << "sample_rate=" << sampleRate << "\n"
    << "profile=" << gsdsp::naturalDevelopmentProfile().canonicalKey << "\n"
    << "profile_hash=" << compiled.prepared.contentHash << "\n"
    << "tap_count=" << compiled.prepared.transfer.tapCount << "\n"
    << "mode_count=" << compiled.prepared.modal.modeCount << "\n"
    << "finite=" << (report.allFinite() ? "true" : "false") << "\n"
    << "peak=" << report.peakAbsolute << "\n"
    << "rms=" << report.rms << "\n";

  return report.allFinite() ? 0 : 2;
}

int commandProfileCompare(int argc, char** argv)
{
  if(argc < 5)
  {
    throw std::invalid_argument(
      "profile-compare requires raw, natural, and dread output paths"
    );
  }

  const auto sampleRate = parseU32(
    argc > 5 ? argv[5] : nullptr,
    48000U
  );

  const auto raw = gsdsp::compileBodyProfile(
    gsdsp::rawConditionedProfile(),
    sampleRate
  );

  const auto natural = gsdsp::compileBodyProfile(
    gsdsp::naturalDevelopmentProfile(),
    sampleRate
  );

  const auto dreadnought = gsdsp::compileBodyProfile(
    gsdsp::dreadnoughtDevelopmentProfile(),
    sampleRate
  );

  if(!raw.ok() || !natural.ok() || !dreadnought.ok())
  {
    throw std::runtime_error(
      "unable to compile P4C comparison profiles"
    );
  }

  // Listening evidence: guitar-like synthetic fixture. These files make the
  // profile differences easy to inspect, but their RMS is intentionally not
  // used as the hard level-match oracle because their spectrum is highly
  // content-specific.
  auto listeningInput = makeSyntheticBodyFixture(sampleRate);

  auto rawOutput = processBody(
    listeningInput,
    sampleRate,
    raw.prepared,
    1.0
  );

  auto naturalOutput = processBody(
    listeningInput,
    sampleRate,
    natural.prepared,
    1.0
  );

  auto dreadOutput = processBody(
    listeningInput,
    sampleRate,
    dreadnought.prepared,
    1.0
  );

  gslab::WavFile::writeFloat32(
    argv[2],
    sampleRate,
    rawOutput
  );

  gslab::WavFile::writeFloat32(
    argv[3],
    sampleRate,
    naturalOutput
  );

  gslab::WavFile::writeFloat32(
    argv[4],
    sampleRate,
    dreadOutput
  );

  const auto rawListeningReport =
    gslab::analyzeFiniteAudio(rawOutput);
  const auto naturalListeningReport =
    gslab::analyzeFiniteAudio(naturalOutput);
  const auto dreadListeningReport =
    gslab::analyzeFiniteAudio(dreadOutput);

  // Calibration evidence: the exact deterministic pink-noise gate used by the
  // regression suite. This prevents a narrow synthetic phrase from defining
  // the static comparison gain.
  const auto calibration = gslab::SignalGenerator::pinkNoise(
    1,
    static_cast<std::size_t>(sampleRate) * 4U,
    0x5034434C564C4D54ULL,
    0.20F
  );

  const auto rawCalibration = processBody(
    calibration,
    sampleRate,
    raw.prepared,
    1.0
  );

  const auto naturalCalibration = processBody(
    calibration,
    sampleRate,
    natural.prepared,
    1.0
  );

  const auto dreadCalibration = processBody(
    calibration,
    sampleRate,
    dreadnought.prepared,
    1.0
  );

  const auto rawReport =
    gslab::analyzeFiniteAudio(rawCalibration);
  const auto naturalReport =
    gslab::analyzeFiniteAudio(naturalCalibration);
  const auto dreadReport =
    gslab::analyzeFiniteAudio(dreadCalibration);

  const auto levelDb = [](double reference, double candidate) {
    if(reference <= 0.0 || candidate <= 0.0)
      return 0.0;

    return 20.0 * std::log10(candidate / reference);
  };

  const double naturalVsRaw = levelDb(
    rawReport.rms,
    naturalReport.rms
  );

  const double dreadVsRaw = levelDb(
    rawReport.rms,
    dreadReport.rms
  );

  const double dreadVsNatural = levelDb(
    naturalReport.rms,
    dreadReport.rms
  );

  const bool listeningFinite =
    rawListeningReport.allFinite()
    && naturalListeningReport.allFinite()
    && dreadListeningReport.allFinite();

  const bool calibrationFinite =
    rawReport.allFinite()
    && naturalReport.allFinite()
    && dreadReport.allFinite();

  const bool levelMatched =
    std::abs(naturalVsRaw) <= 0.35
    && std::abs(dreadVsRaw) <= 0.35
    && std::abs(dreadVsNatural) <= 0.25;

  std::cout
    << "fixture=synthetic_body_comparison\n"
    << "calibration=deterministic_pink_noise_4s\n"
    << "sample_rate=" << sampleRate << "\n"
    << "raw_profile=" << gsdsp::rawConditionedProfile().canonicalKey << "\n"
    << "natural_profile=" << gsdsp::naturalDevelopmentProfile().canonicalKey << "\n"
    << "dread_profile=" << gsdsp::dreadnoughtDevelopmentProfile().canonicalKey << "\n"
    << "listening_raw_rms=" << rawListeningReport.rms << "\n"
    << "listening_natural_rms=" << naturalListeningReport.rms << "\n"
    << "listening_dread_rms=" << dreadListeningReport.rms << "\n"
    << "calibration_raw_rms=" << rawReport.rms << "\n"
    << "calibration_natural_rms=" << naturalReport.rms << "\n"
    << "calibration_dread_rms=" << dreadReport.rms << "\n"
    << "natural_vs_raw_db=" << naturalVsRaw << "\n"
    << "dread_vs_raw_db=" << dreadVsRaw << "\n"
    << "dread_vs_natural_db=" << dreadVsNatural << "\n"
    << "listening_finite=" << (listeningFinite ? "true" : "false") << "\n"
    << "calibration_finite=" << (calibrationFinite ? "true" : "false") << "\n"
    << "engineering_level_match=" << (levelMatched ? "true" : "false") << "\n";

  return listeningFinite && calibrationFinite && levelMatched ? 0 : 2;
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

    if(command == "calibrate-pair")
      return commandCalibratePair(argc, argv);

    if(command == "calibrate-stereo")
      return commandCalibrateStereo(argc, argv);

    if(command == "evaluate-profile")
      return commandEvaluateProfile(argc, argv);

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

    if(command == "benchmark-transfer")
      return commandBenchmarkTransfer(argc, argv);

    if(command == "benchmark-body")
      return commandBenchmarkBody(argc, argv);

    if(command == "benchmark-body-switch")
      return commandBenchmarkBodySwitch(argc, argv);

    if(command == "source-demo")
      return commandSourceDemo(argc, argv);

    if(command == "body-demo")
      return commandBodyDemo(argc, argv);

    if(command == "profile-compare")
      return commandProfileCompare(argc, argv);

    printUsage();
    return 1;
  }
  catch(const std::exception& error)
  {
    std::cerr << "error=" << error.what() << "\n";
    return 2;
  }
}
