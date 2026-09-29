#include "GitaSedap.h"
#include "IPlug_include_in_plug_src.h"

#include <algorithm>
#include <cmath>

#if IPLUG_EDITOR
#include "IControls.h"
#endif

GitaSedap::GitaSedap(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamBody)->InitDouble(
    gs::kBodySpec.displayName.data(),
    gs::kBodySpec.defaultValue,
    gs::kBodySpec.minimum,
    gs::kBodySpec.maximum,
    gs::kBodySpec.step,
    gs::kBodySpec.unit.data()
  );

  GetParam(kParamAir)->InitDouble(
    gs::kAirSpec.displayName.data(),
    gs::kAirSpec.defaultValue,
    gs::kAirSpec.minimum,
    gs::kAirSpec.maximum,
    gs::kAirSpec.step,
    gs::kAirSpec.unit.data()
  );

  GetParam(kParamEnhance)->InitDouble(
    gs::kEnhanceSpec.displayName.data(),
    gs::kEnhanceSpec.defaultValue,
    gs::kEnhanceSpec.minimum,
    gs::kEnhanceSpec.maximum,
    gs::kEnhanceSpec.step,
    gs::kEnhanceSpec.unit.data()
  );

  GetParam(kParamOutputDb)->InitDouble(
    gs::kOutputSpec.displayName.data(),
    gs::kOutputSpec.defaultValue,
    gs::kOutputSpec.minimum,
    gs::kOutputSpec.maximum,
    gs::kOutputSpec.step,
    gs::kOutputSpec.unit.data()
  );

  GetParam(kParamBypass)->InitBool("Bypass", false);
  GetParam(kParamInputSource)->InitEnum(
    "Input Source",
    static_cast<int>(gs::InputSource::ActivePiezo),
    {"Active Piezo", "Passive Piezo", "Magnetic"}
  );

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(
      *this,
      PLUG_WIDTH,
      PLUG_HEIGHT,
      PLUG_FPS,
      GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT)
    );
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_LIGHT_GRAY);

    const IRECT bounds = pGraphics->GetBounds();
    const IRECT inner = bounds.GetPadded(-18.f);
    const IRECT title = inner.GetFromTop(34.f);
    const IRECT footer = inner.GetFromBottom(56.f);
    const IRECT knobs = IRECT(inner.L, title.B + 12.f, inner.R, footer.T - 10.f);

    pGraphics->AttachControl(
      new ITextControl(title, "GitaSedap  |  P3 SOURCE CONDITIONING", IText(20.f))
    );

    constexpr int knobCount = 4;
    const int params[knobCount]{
      kParamBody,
      kParamAir,
      kParamEnhance,
      kParamOutputDb
    };
    const char* labels[knobCount]{
      "BODY",
      "AIR",
      "ENHANCE",
      "OUTPUT"
    };

    const float cellWidth = knobs.W() / static_cast<float>(knobCount);
    for(int i = 0; i < knobCount; ++i)
    {
      const float left = knobs.L + (cellWidth * static_cast<float>(i));
      const IRECT cell(left, knobs.T, left + cellWidth, knobs.B);
      pGraphics->AttachControl(
        new IVKnobControl(cell.GetCentredInside(112.f), params[i], labels[i])
      );
    }

    const IRECT sourceRect(footer.L, footer.T, footer.L + 230.f, footer.B);
    const IRECT bypassRect(footer.R - 180.f, footer.T, footer.R, footer.B);

    pGraphics->AttachControl(
      new IVMenuButtonControl(sourceRect, kParamInputSource, "INPUT SOURCE")
    );
    pGraphics->AttachControl(
      new IVToggleControl(bypassRect, kParamBypass, "BYPASS")
    );
  };
#endif
}

bool GitaSedap::SerializeState(IByteChunk& chunk) const
{
  const auto header = gs::state::encodeHeader();
  chunk.PutBytes(header.data(), static_cast<int>(header.size()));
  return SerializeParams(chunk);
}

int GitaSedap::UnserializeState(const IByteChunk& chunk, int startPos)
{
  gs::state::HeaderBytes header{};
  const int payloadPos = chunk.GetBytes(
    header.data(),
    static_cast<int>(header.size()),
    startPos
  );

  if(payloadPos < 0 || !gs::state::isSupportedHeader(header))
    return -1;

  return UnserializeParams(chunk, payloadPos);
}

#if IPLUG_DSP
double GitaSedap::dbToLinear(double db) noexcept
{
  return std::pow(10.0, db / 20.0);
}

gs::InputSource GitaSedap::currentInputSource() const noexcept
{
  const int raw = static_cast<int>(
    std::lround(GetParam(kParamInputSource)->Value())
  );

  const int bounded = std::clamp(
    raw,
    0,
    static_cast<int>(gs::InputSource::Count) - 1
  );

  return static_cast<gs::InputSource>(bounded);
}

void GitaSedap::OnReset()
{
  const double sampleRate = std::max(GetSampleRate(), 8000.0);

  mSourceAdapter.setSourceType(currentInputSource());
  mSourceAdapter.prepare(sampleRate);

  mOutputGain.prepare(sampleRate, 20.0);
  mOutputGain.reset(dbToLinear(GetParam(kParamOutputDb)->Value()));

  mBypassCrossfade.prepare(sampleRate, 5.0);
  mBypassCrossfade.reset(GetParam(kParamBypass)->Bool());
}

void GitaSedap::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  if(nFrames <= 0)
    return;

  const int inputChannels = NInChansConnected();
  const int outputChannels = NOutChansConnected();

  if(outputChannels <= 0)
    return;

  if(inputChannels <= 0 || inputs == nullptr || inputs[0] == nullptr)
  {
    for(int channel = 0; channel < outputChannels; ++channel)
    {
      if(outputs[channel] != nullptr)
        std::fill_n(outputs[channel], nFrames, sample{0});
    }
    return;
  }

  mSourceAdapter.setSourceType(currentInputSource());
  mOutputGain.setTarget(dbToLinear(GetParam(kParamOutputDb)->Value()));
  mBypassCrossfade.setBypassed(GetParam(kParamBypass)->Bool());

  mSourceAdapter.beginBlock();

  const sample* const monoInput = inputs[0];

  for(int frame = 0; frame < nFrames; ++frame)
  {
    const double dry = static_cast<double>(monoInput[frame]);

    const double conditioned = mSourceAdapter.processSample(dry);
    const double wet = conditioned * mOutputGain.next();

    const sample value = static_cast<sample>(
      mBypassCrossfade.process(dry, wet)
    );

    for(int channel = 0; channel < outputChannels; ++channel)
    {
      if(outputs[channel] != nullptr)
        outputs[channel][frame] = value;
    }
  }

  (void) mSourceAdapter.endBlock();
}
#endif
