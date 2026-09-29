#include "TestSupport.h"
#include "core/ParameterSpec.h"
#include "dsp/SourceAdapter.h"
#include "lab/AudioBuffer.h"
#include "lab/FiniteAudioGuard.h"
#include "lab/SignalGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

namespace gs = gitasedap::core;
namespace gsdsp = gitasedap::dsp;
namespace gslab = gitasedap::lab;

namespace
{

[[nodiscard]] gslab::AudioBuffer render(
  const gslab::AudioBuffer& input,
  double sampleRate,
  gs::InputSource source,
  double antiQuackAmount,
  std::size_t blockSize
)
{
  gsdsp::SourceAdapter adapter;
  adapter.setSourceType(source);
  adapter.prepare(sampleRate);
  adapter.setAntiQuackAmount(antiQuackAmount);

  gslab::AudioBuffer output(
    input.channelCount(),
    input.frameCount()
  );

  std::size_t offset = 0;

  while(offset < input.frameCount())
  {
    const auto frames = std::min(
      blockSize,
      input.frameCount() - offset
    );

    adapter.beginBlock();

    for(std::size_t frame = 0; frame < frames; ++frame)
    {
      for(std::size_t channel = 0; channel < input.channelCount(); ++channel)
      {
        output.channel(channel)[offset + frame] = static_cast<float>(
          adapter.processSample(
            static_cast<double>(
              input.channel(channel)[offset + frame]
            )
          )
        );
      }
    }

    (void) adapter.endBlock();
    offset += frames;
  }

  return output;
}

[[nodiscard]] double rmsRange(
  const gslab::AudioBuffer& buffer,
  std::size_t start,
  std::size_t end
)
{
  if(start >= end || end > buffer.frameCount())
    return 0.0;

  double sum = 0.0;
  std::size_t count = 0;

  for(std::size_t frame = start; frame < end; ++frame)
  {
    const double value =
      static_cast<double>(buffer.channel(0)[frame]);

    sum += value * value;
    ++count;
  }

  return count == 0
    ? 0.0
    : std::sqrt(sum / static_cast<double>(count));
}

[[nodiscard]] gslab::AudioBuffer makeTransientFixture(
  double sampleRate
)
{
  const auto frames = static_cast<std::size_t>(
    std::llround(sampleRate * 0.8)
  );

  gslab::AudioBuffer buffer(1, frames);

  const auto writeTone = [&](
    std::size_t start,
    std::size_t length,
    double frequency,
    double amplitude
  ) {
    for(std::size_t i = 0; i < length && start + i < frames; ++i)
    {
      const double phase =
        2.0
        * std::numbers::pi
        * frequency
        * static_cast<double>(i)
        / sampleRate;

      buffer.channel(0)[start + i] = static_cast<float>(
        std::sin(phase) * amplitude
      );
    }
  };

  const auto firstStart = static_cast<std::size_t>(
    std::llround(sampleRate * 0.10)
  );
  const auto firstLength = static_cast<std::size_t>(
    std::llround(sampleRate * 0.03)
  );

  const auto sustainedStart = static_cast<std::size_t>(
    std::llround(sampleRate * 0.25)
  );
  const auto sustainedLength = static_cast<std::size_t>(
    std::llround(sampleRate * 0.45)
  );

  writeTone(firstStart, firstLength, 2600.0, 0.50);
  writeTone(sustainedStart, sustainedLength, 2600.0, 0.20);

  return buffer;
}

} // namespace

int main()
{
  constexpr std::array<double, 4> sampleRates{
    44100.0,
    48000.0,
    88200.0,
    96000.0
  };

  constexpr std::array<gs::InputSource, 3> sources{
    gs::InputSource::ActivePiezo,
    gs::InputSource::PassivePiezo,
    gs::InputSource::Magnetic
  };

  for(const double sampleRate : sampleRates)
  {
    const auto frames = static_cast<std::size_t>(
      std::llround(sampleRate * 0.25)
    );

    const auto sweep = gslab::SignalGenerator::logarithmicSweep(
      1,
      frames,
      sampleRate,
      20.0,
      sampleRate * 0.45,
      0.35F
    );

    for(const auto source : sources)
    {
      const auto output = render(
        sweep,
        sampleRate,
        source,
        1.0,
        64
      );

      GS_REQUIRE(gslab::analyzeFiniteAudio(output).allFinite());
    }
  }

  {
    const auto dc = gslab::AudioBuffer(1, 96000);

    auto dcInput = dc;
    std::fill(
      dcInput.channel(0).begin(),
      dcInput.channel(0).end(),
      0.25F
    );

    const auto output = render(
      dcInput,
      48000.0,
      gs::InputSource::ActivePiezo,
      0.0,
      64
    );

    double tailPeak = 0.0;

    for(std::size_t frame = output.frameCount() - 1024;
        frame < output.frameCount();
        ++frame)
    {
      tailPeak = std::max(
        tailPeak,
        std::abs(static_cast<double>(output.channel(0)[frame]))
      );
    }

    GS_REQUIRE(tailPeak < 1.0e-5);
  }

  {
    const auto low = gslab::SignalGenerator::sine(
      1,
      96000,
      48000.0,
      10.0,
      0.20F
    );

    const auto mid = gslab::SignalGenerator::sine(
      1,
      96000,
      48000.0,
      1000.0,
      0.20F
    );

    const auto lowOut = render(
      low,
      48000.0,
      gs::InputSource::ActivePiezo,
      0.0,
      127
    );

    const auto midOut = render(
      mid,
      48000.0,
      gs::InputSource::ActivePiezo,
      0.0,
      127
    );

    const double lowRms = rmsRange(lowOut, 48000, 96000);
    const double midRms = rmsRange(midOut, 48000, 96000);

    GS_REQUIRE(midRms > 0.01);
    GS_REQUIRE(lowRms < (midRms * 0.20));
  }

  {
    constexpr double sampleRate = 48000.0;
    const auto fixture = makeTransientFixture(sampleRate);

    const auto disabled = render(
      fixture,
      sampleRate,
      gs::InputSource::ActivePiezo,
      0.0,
      64
    );

    const auto active = render(
      fixture,
      sampleRate,
      gs::InputSource::ActivePiezo,
      1.0,
      64
    );

    const auto magnetic = render(
      fixture,
      sampleRate,
      gs::InputSource::Magnetic,
      1.0,
      64
    );

    const auto attackStart = static_cast<std::size_t>(
      sampleRate * 0.10
    );
    const auto attackEnd = attackStart + static_cast<std::size_t>(
      sampleRate * 0.01
    );

    const auto sustainStart = static_cast<std::size_t>(
      sampleRate * 0.55
    );
    const auto sustainEnd = static_cast<std::size_t>(
      sampleRate * 0.65
    );

    const double disabledAttack = rmsRange(
      disabled,
      attackStart,
      attackEnd
    );
    const double activeAttack = rmsRange(
      active,
      attackStart,
      attackEnd
    );
    const double magneticAttack = rmsRange(
      magnetic,
      attackStart,
      attackEnd
    );

    GS_REQUIRE(disabledAttack > 0.01);
    GS_REQUIRE(activeAttack < (disabledAttack * 0.80));
    GS_REQUIRE(activeAttack < (magneticAttack * 0.90));

    const double disabledSustain = rmsRange(
      disabled,
      sustainStart,
      sustainEnd
    );
    const double activeSustain = rmsRange(
      active,
      sustainStart,
      sustainEnd
    );

    GS_REQUIRE(disabledSustain > 0.01);
    GS_REQUIRE(activeSustain > (disabledSustain * 0.97));
    GS_REQUIRE(activeSustain < (disabledSustain * 1.03));
  }

  {
    constexpr double sampleRate = 48000.0;
    const auto fixture = makeTransientFixture(sampleRate);

    const auto render64 = render(
      fixture,
      sampleRate,
      gs::InputSource::PassivePiezo,
      1.0,
      64
    );

    const auto render127 = render(
      fixture,
      sampleRate,
      gs::InputSource::PassivePiezo,
      1.0,
      127
    );

    for(std::size_t frame = 0; frame < fixture.frameCount(); ++frame)
    {
      const double difference = std::abs(
        static_cast<double>(render64.channel(0)[frame])
        - static_cast<double>(render127.channel(0)[frame])
      );

      GS_REQUIRE(difference <= 1.0e-7);
    }
  }

  {
    gsdsp::SourceAdapter adapter;
    adapter.prepare(96000.0);

    const auto noise = gslab::SignalGenerator::whiteNoise(
      1,
      96000,
      0x5033445354524553ULL,
      0.30F
    );

    adapter.beginBlock();

    double peak = 0.0;

    for(std::size_t frame = 0; frame < noise.frameCount(); ++frame)
    {
      if(frame % 37 == 0)
      {
        adapter.setAntiQuackAmount(
          ((frame / 37) % 2) == 0 ? 0.0 : 1.0
        );
      }

      if(frame % 101 == 0)
      {
        adapter.setInputTrimDb(
          ((frame / 101) % 2) == 0 ? -24.0 : 18.0
        );
      }

      if(frame % 4096 == 0)
      {
        const auto sourceIndex = static_cast<int>(
          (frame / 4096)
          % static_cast<std::size_t>(gs::InputSource::Count)
        );

        adapter.setSourceType(
          static_cast<gs::InputSource>(sourceIndex)
        );
      }

      const double output = adapter.processSample(
        static_cast<double>(noise.channel(0)[frame])
      );

      GS_REQUIRE(std::isfinite(output));
      peak = std::max(peak, std::abs(output));
    }

    const auto meter = adapter.endBlock();

    GS_REQUIRE(std::isfinite(meter.inputHeadroomDb));
    GS_REQUIRE(meter.inputPeakLinear > 0.0);
    GS_REQUIRE(meter.inputRmsLinear > 0.0);
    GS_REQUIRE(peak < 8.0);
  }

  {
    gsdsp::SourceAdapter adapter;
    adapter.prepare(48000.0);

    adapter.beginBlock();
    GS_REQUIRE(std::isfinite(
      adapter.processSample(std::numeric_limits<double>::quiet_NaN())
    ));
    GS_REQUIRE(std::isfinite(
      adapter.processSample(std::numeric_limits<double>::infinity())
    ));
    (void) adapter.endBlock();
  }

  {
    gsdsp::SourceAdapter adapter;
    adapter.prepare(48000.0);
    adapter.beginBlock();

    (void) adapter.processSample(0.25);
    (void) adapter.processSample(-0.50);
    (void) adapter.processSample(0.125);

    const auto meter = adapter.endBlock();

    GS_REQUIRE(std::abs(meter.inputPeakLinear - 0.50) < 1.0e-12);
    GS_REQUIRE(
      std::abs(meter.inputHeadroomDb - 6.020599913279624)
      < 1.0e-9
    );
  }

  return 0;
}
