#include "GitaSedap.h"
#include "IPlug_include_in_plug_src.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <span>

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

  GetParam(kParamBodyProfileA)->InitEnum(
    "Body Profile A",
    static_cast<int>(gs::BodyProfileChoice::NaturalDevelopment),
    {"Raw / P3", "Natural Development", "Dreadnought Development"}
  );

  GetParam(kParamBodyProfileB)->InitEnum(
    "Body Profile B",
    static_cast<int>(gs::BodyProfileChoice::DreadnoughtDevelopment),
    {"Raw / P3", "Natural Development", "Dreadnought Development"}
  );

  GetParam(kParamBodyCompareSlot)->InitEnum(
    "Body Compare Slot",
    static_cast<int>(gs::BodyCompareSlot::A),
    {"A", "B"}
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

    if(!pGraphics->LoadFont(DEFAULT_FONT, "Segoe UI", ETextStyle::Normal))
      (void) pGraphics->LoadFont(DEFAULT_FONT, "Arial", ETextStyle::Normal);

    pGraphics->EnableMouseOver(true);
    pGraphics->AttachTextEntryControl();

    const IColor shell{255, 28, 31, 35};
    const IColor panel{255, 37, 41, 46};
    const IColor panelRaised{255, 45, 49, 55};
    const IColor surface{255, 56, 61, 68};
    const IColor surfaceDark{255, 21, 24, 28};
    const IColor accent{255, 71, 188, 205};
    const IColor accentSoft{255, 104, 211, 225};
    const IColor textPrimary{255, 234, 238, 241};
    const IColor textSecondary{255, 159, 168, 176};
    const IColor line{255, 68, 74, 82};
    const IColor shadow{120, 0, 0, 0};

    const IVStyle sectionStyle = DEFAULT_STYLE
      .WithShowLabel(false)
      .WithShowValue(false)
      .WithDrawFrame(true)
      .WithFrameThickness(1.f)
      .WithRoundness(0.08f)
      .WithDrawShadows(false)
      .WithColor(kBG, panel)
      .WithColor(kFG, panel)
      .WithColor(kFR, line);

    const IVStyle headerStyle = sectionStyle
      .WithColor(kBG, panelRaised)
      .WithColor(kFG, panelRaised);

    const IVStyle knobStyle = DEFAULT_STYLE
      .WithShowLabel(true)
      .WithShowValue(true)
      .WithDrawFrame(false)
      .WithDrawShadows(true)
      .WithShadowOffset(2.f)
      .WithRoundness(1.f)
      .WithWidgetFrac(0.68f)
      .WithColor(kBG, shell)
      .WithColor(kFG, surface)
      .WithColor(kPR, accent)
      .WithColor(kFR, line)
      .WithColor(kHL, accentSoft)
      .WithColor(kSH, shadow)
      .WithColor(kX1, surfaceDark)
      .WithLabelText(IText(12.f, textSecondary))
      .WithValueText(IText(12.f, textPrimary));

    const IVStyle outputStyle = knobStyle
      .WithWidgetFrac(0.62f)
      .WithColor(kPR, accentSoft);

    const IVStyle segmentedStyle = DEFAULT_STYLE
      .WithShowLabel(false)
      .WithShowValue(false)
      .WithDrawFrame(true)
      .WithFrameThickness(1.f)
      .WithDrawShadows(false)
      .WithRoundness(0.16f)
      .WithColor(kBG, surfaceDark)
      .WithColor(kFG, panelRaised)
      .WithColor(kPR, accent)
      .WithColor(kFR, line)
      .WithColor(kHL, accentSoft)
      .WithColor(kSH, shadow)
      .WithLabelText(IText(10.f, textSecondary))
      .WithValueText(IText(10.f, textPrimary));

    const IVStyle listenStyle = segmentedStyle
      .WithColor(kPR, accentSoft)
      .WithValueText(IText(12.f, textPrimary));

    const IVStyle bypassStyle = DEFAULT_STYLE
      .WithShowLabel(true)
      .WithShowValue(false)
      .WithDrawFrame(true)
      .WithFrameThickness(1.f)
      .WithDrawShadows(false)
      .WithRoundness(1.f)
      .WithWidgetFrac(0.72f)
      .WithColor(kBG, surfaceDark)
      .WithColor(kFG, surface)
      .WithColor(kPR, accent)
      .WithColor(kFR, line)
      .WithColor(kHL, accentSoft)
      .WithLabelText(IText(11.f, textSecondary))
      .WithValueText(IText(11.f, textPrimary));

    pGraphics->AttachPanelBackground(shell);

    const IRECT bounds = pGraphics->GetBounds();
    const IRECT outer = bounds.GetPadded(-14.f);

    const IRECT header(
      outer.L,
      outer.T,
      outer.R,
      outer.T + 54.f
    );

    const IRECT profilePanel(
      outer.L,
      header.B + 10.f,
      outer.R,
      header.B + 102.f
    );

    const IRECT footer(
      outer.L,
      outer.B - 88.f,
      outer.R,
      outer.B
    );

    const IRECT main(
      outer.L,
      profilePanel.B + 10.f,
      outer.R,
      footer.T - 10.f
    );

    pGraphics->AttachControl(
      new IVPanelControl(header, "", headerStyle)
    );
    pGraphics->AttachControl(
      new IVPanelControl(profilePanel, "", sectionStyle)
    );
    pGraphics->AttachControl(
      new IVPanelControl(main, "", sectionStyle)
    );
    pGraphics->AttachControl(
      new IVPanelControl(footer, "", sectionStyle)
    );

    const IRECT brand(
      header.L + 18.f,
      header.T + 7.f,
      header.L + 250.f,
      header.B - 7.f
    );

    const IRECT phase(
      header.R - 300.f,
      header.T + 10.f,
      header.R - 18.f,
      header.B - 10.f
    );

    pGraphics->AttachControl(
      new ITextControl(
        brand.GetFromTop(24.f),
        "GitaSedap",
        IText(21.f, textPrimary, nullptr, EAlign::Near)
      )
    );

    pGraphics->AttachControl(
      new ITextControl(
        brand.GetFromBottom(16.f),
        "ACOUSTIC GUITAR ENHANCER",
        IText(10.f, textSecondary, nullptr, EAlign::Near)
      )
    );

    pGraphics->AttachControl(
      new ITextControl(
        phase,
        "P4C  •  LEVEL-MATCHED BODY A/B  •  ZERO LATENCY",
        IText(10.f, accentSoft, nullptr, EAlign::Far)
      )
    );

    pGraphics->AttachControl(
      new ITextControl(
        IRECT(
          profilePanel.L + 16.f,
          profilePanel.T + 7.f,
          profilePanel.R - 16.f,
          profilePanel.T + 23.f
        ),
        "BODY PROFILE  •  QUICK COMPARE",
        IText(10.f, textSecondary, nullptr, EAlign::Near)
      )
    );

    const float compareWidth = 112.f;
    const float gap = 10.f;
    const float selectorWidth =
      (
        profilePanel.W()
        - 28.f
        - compareWidth
        - (gap * 2.f)
      ) * 0.5f;

    const IRECT profileA(
      profilePanel.L + 14.f,
      profilePanel.T + 29.f,
      profilePanel.L + 14.f + selectorWidth,
      profilePanel.B - 11.f
    );

    const IRECT profileB(
      profileA.R + gap,
      profileA.T,
      profileA.R + gap + selectorWidth,
      profileA.B
    );

    const IRECT listenArea(
      profileB.R + gap,
      profileA.T,
      profilePanel.R - 14.f,
      profileA.B
    );

    pGraphics->AttachControl(
      new ITextControl(
        profileA.GetFromTop(16.f),
        "A PROFILE",
        IText(9.f, textSecondary, nullptr, EAlign::Near)
      )
    );

    pGraphics->AttachControl(
      new ITextControl(
        profileB.GetFromTop(16.f),
        "B PROFILE",
        IText(9.f, textSecondary, nullptr, EAlign::Near)
      )
    );

    pGraphics->AttachControl(
      new ITextControl(
        listenArea.GetFromTop(16.f),
        "LISTEN",
        IText(9.f, textSecondary)
      )
    );

    const std::vector<const char*> profileLabels{
      "RAW",
      "NATURAL",
      "DREAD"
    };

    pGraphics->AttachControl(
      new IVTabSwitchControl(
        profileA.GetFromBottom(39.f),
        kParamBodyProfileA,
        profileLabels,
        "",
        segmentedStyle,
        EVShape::Rectangle,
        EDirection::Horizontal
      )
    );

    pGraphics->AttachControl(
      new IVTabSwitchControl(
        profileB.GetFromBottom(39.f),
        kParamBodyProfileB,
        profileLabels,
        "",
        segmentedStyle,
        EVShape::Rectangle,
        EDirection::Horizontal
      )
    );

    const std::vector<const char*> compareLabels{"A", "B"};

    pGraphics->AttachControl(
      new IVTabSwitchControl(
        listenArea.GetFromBottom(39.f),
        kParamBodyCompareSlot,
        compareLabels,
        "",
        listenStyle,
        EVShape::Rectangle,
        EDirection::Horizontal
      )
    );

    const float mainWidth = main.W();
    const float primaryAreaWidth = mainWidth * 0.77f;

    const IRECT primaryArea(
      main.L + 14.f,
      main.T + 12.f,
      main.L + primaryAreaWidth,
      main.B - 12.f
    );

    const IRECT outputArea(
      primaryArea.R + 8.f,
      main.T + 12.f,
      main.R - 14.f,
      main.B - 12.f
    );

    pGraphics->AttachControl(
      new ITextControl(
        IRECT(
          primaryArea.L + 8.f,
          primaryArea.T,
          primaryArea.R - 8.f,
          primaryArea.T + 18.f
        ),
        "TONE SHAPING",
        IText(10.f, textSecondary, nullptr, EAlign::Near)
      )
    );

    constexpr int primaryCount = 3;
    const int primaryParams[primaryCount]{
      kParamBody,
      kParamAir,
      kParamEnhance
    };
    const char* primaryLabels[primaryCount]{
      "BODY",
      "AIR",
      "ENHANCE"
    };

    const float knobTop = primaryArea.T + 20.f;
    const float knobBottom = primaryArea.B - 2.f;
    const float cellWidth =
      (primaryArea.W() - 8.f) / static_cast<float>(primaryCount);

    for(int i = 0; i < primaryCount; ++i)
    {
      const float left =
        primaryArea.L + 4.f + (cellWidth * static_cast<float>(i));

      const IRECT cell(
        left,
        knobTop,
        left + cellWidth,
        knobBottom
      );

      pGraphics->AttachControl(
        new IVKnobControl(
          cell.GetCentredInside(142.f),
          primaryParams[i],
          primaryLabels[i],
          knobStyle,
          true,
          false,
          -140.f,
          140.f,
          -140.f,
          EDirection::Vertical,
          DEFAULT_GEARING,
          3.f
        )
      );
    }

    pGraphics->AttachControl(
      new ITextControl(
        IRECT(
          outputArea.L + 6.f,
          outputArea.T,
          outputArea.R - 6.f,
          outputArea.T + 18.f
        ),
        "LEVEL",
        IText(10.f, textSecondary)
      )
    );

    pGraphics->AttachControl(
      new IVKnobControl(
        IRECT(
          outputArea.L + 8.f,
          outputArea.T + 22.f,
          outputArea.R - 8.f,
          outputArea.B - 2.f
        ),
        kParamOutputDb,
        "OUTPUT",
        outputStyle,
        true,
        false,
        -140.f,
        140.f,
        0.f,
        EDirection::Vertical,
        DEFAULT_GEARING,
        3.f
      )
    );

    const IRECT sourceLabel(
      footer.L + 16.f,
      footer.T + 10.f,
      footer.L + 130.f,
      footer.T + 28.f
    );

    pGraphics->AttachControl(
      new ITextControl(
        sourceLabel,
        "INPUT SOURCE",
        IText(10.f, textSecondary, nullptr, EAlign::Near)
      )
    );

    const IRECT sourceControl(
      footer.L + 14.f,
      footer.T + 31.f,
      footer.R - 170.f,
      footer.B - 12.f
    );

    const std::vector<const char*> sourceLabels{
      "ACTIVE PIEZO",
      "PASSIVE PIEZO",
      "MAGNETIC"
    };

    pGraphics->AttachControl(
      new IVTabSwitchControl(
        sourceControl,
        kParamInputSource,
        sourceLabels,
        "",
        segmentedStyle,
        EVShape::Rectangle,
        EDirection::Horizontal
      )
    );

    const IRECT bypassRect(
      footer.R - 146.f,
      footer.T + 14.f,
      footer.R - 16.f,
      footer.B - 12.f
    );

    pGraphics->AttachControl(
      new IVToggleControl(
        bypassRect,
        kParamBypass,
        "BYPASS",
        bypassStyle,
        "ACTIVE",
        "BYPASSED"
      )
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

  int position = chunk.GetBytes(
    header.data(),
    static_cast<int>(header.size()),
    startPos
  );

  if(position < 0 || !gs::state::isSupportedHeader(header))
    return -1;

  const auto decoded = gs::state::decodeHeader(header);
  const auto serializedCount =
    gs::state::serializedParameterCount(decoded.version);

  if(
    serializedCount == 0
    || serializedCount > gs::parameterCount()
  )
  {
    return -1;
  }

  std::array<double, gs::parameterCount()> serialized{};

  for(std::size_t index = 0; index < serializedCount; ++index)
  {
    position = chunk.Get(&serialized[index], position);

    if(position < 0)
      return -1;
  }

  gs::state::CanonicalParameterValues migrated{};

  if(
    !gs::state::migrateSerializedParameterValues(
      decoded.version,
      std::span<const double>(
        serialized.data(),
        serializedCount
      ),
      migrated
    )
  )
  {
    return -1;
  }

  for(int index = 0; index < kNumParams; ++index)
  {
    GetParam(index)->Set(
      migrated[static_cast<std::size_t>(index)]
    );
  }

  OnParamReset(kPresetRecall);
  return position;
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

gs::BodyCompareSlot GitaSedap::currentBodyCompareSlot() const noexcept
{
  const int raw = static_cast<int>(
    std::lround(GetParam(kParamBodyCompareSlot)->Value())
  );

  const int bounded = std::clamp(
    raw,
    0,
    static_cast<int>(gs::BodyCompareSlot::Count) - 1
  );

  return static_cast<gs::BodyCompareSlot>(bounded);
}

gs::BodyProfileChoice GitaSedap::currentBodyProfileChoice() const noexcept
{
  const int parameterIndex =
    currentBodyCompareSlot() == gs::BodyCompareSlot::A
      ? kParamBodyProfileA
      : kParamBodyProfileB;

  const int raw = static_cast<int>(
    std::lround(GetParam(parameterIndex)->Value())
  );

  const int bounded = std::clamp(
    raw,
    0,
    static_cast<int>(gs::BodyProfileChoice::Count) - 1
  );

  return static_cast<gs::BodyProfileChoice>(bounded);
}

const gsdsp::PreparedBodyProfile*
GitaSedap::currentPreparedBodyProfile() const noexcept
{
  if(!mFactoryBodyProfilesReady)
    return nullptr;

  const auto choice = currentBodyProfileChoice();
  const auto index = static_cast<std::size_t>(choice);

  if(index >= mFactoryBodyProfiles.size())
    return nullptr;

  return &mFactoryBodyProfiles[index];
}

bool GitaSedap::prepareFactoryBodyProfiles(double sampleRate) noexcept
{
  const std::array<const gsdsp::BodyProfileDefinition*,
                   kFactoryBodyProfileCount> definitions{
    &gsdsp::rawConditionedProfile(),
    &gsdsp::naturalDevelopmentProfile(),
    &gsdsp::dreadnoughtDevelopmentProfile()
  };

  for(std::size_t index = 0; index < definitions.size(); ++index)
  {
    const auto compiled = gsdsp::compileBodyProfile(
      *definitions[index],
      sampleRate
    );

    if(!compiled.ok())
    {
      mFactoryBodyProfilesReady = false;
      return false;
    }

    mFactoryBodyProfiles[index] = compiled.prepared;
  }

  mFactoryBodyProfilesReady = true;
  return true;
}

void GitaSedap::OnReset()
{
  const double sampleRate = std::max(GetSampleRate(), 8000.0);

  mSourceAdapter.setSourceType(currentInputSource());
  mSourceAdapter.prepare(sampleRate);

  mBodyRuntime.shutdownAfterAudioStopped();

  mBodyRuntime.prepare(
    sampleRate,
    std::clamp(
      GetParam(kParamBody)->Value() / 100.0,
      0.0,
      1.0
    )
  );

  if(prepareFactoryBodyProfiles(sampleRate))
  {
    if(const auto* prepared = currentPreparedBodyProfile())
    {
      mBodyRuntime.requestPreparedProfileAtAudioBlock(*prepared);
      mBodyRuntime.beginAudioBlock();
    }
  }

  mOutputGain.prepare(sampleRate, 20.0);
  mOutputGain.reset(
    dbToLinear(GetParam(kParamOutputDb)->Value())
  );

  mBypassCrossfade.prepare(sampleRate, 5.0);
  mBypassCrossfade.reset(GetParam(kParamBypass)->Bool());
}

void GitaSedap::ProcessBlock(
  sample** inputs,
  sample** outputs,
  int nFrames
)
{
  if(nFrames <= 0)
    return;

  const int inputChannels = NInChansConnected();
  const int outputChannels = NOutChansConnected();

  if(outputChannels <= 0)
    return;

  if(
    inputChannels <= 0
    || inputs == nullptr
    || inputs[0] == nullptr
  )
  {
    for(int channel = 0; channel < outputChannels; ++channel)
    {
      if(outputs[channel] != nullptr)
        std::fill_n(outputs[channel], nFrames, sample{0});
    }

    return;
  }

  mSourceAdapter.setSourceType(currentInputSource());

  if(const auto* prepared = currentPreparedBodyProfile())
  {
    mBodyRuntime.requestPreparedProfileAtAudioBlock(*prepared);
  }

  mBodyRuntime.beginAudioBlock();

  mBodyRuntime.setBodyAmountNormalized(
    std::clamp(
      GetParam(kParamBody)->Value() / 100.0,
      0.0,
      1.0
    )
  );

  mOutputGain.setTarget(
    dbToLinear(GetParam(kParamOutputDb)->Value())
  );

  mBypassCrossfade.setBypassed(
    GetParam(kParamBypass)->Bool()
  );

  mSourceAdapter.beginBlock();

  const sample* const monoInput = inputs[0];

  for(int frame = 0; frame < nFrames; ++frame)
  {
    const double dry = static_cast<double>(monoInput[frame]);

    const double conditioned =
      mSourceAdapter.processSample(dry);

    const double bodied =
      mBodyRuntime.processSample(conditioned);

    const double wet =
      bodied * mOutputGain.next();

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
