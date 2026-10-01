#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "core/ParameterSpec.h"
#include "core/StateMigration.h"
#include "core/StateSchema.h"
#include "dsp/BodyProfileCompiler.h"
#include "dsp/SourceAdapter.h"
#include "runtime/BodyProfileRuntime.h"
#include "dsp/Smoothing.h"

#include <array>

namespace gs = gitasedap::core;
namespace gsdsp = gitasedap::dsp;
namespace gsruntime = gitasedap::runtime;

constexpr int kNumPresets = 1;

enum EParams
{
  kParamBody = gs::toIndex(gs::ParameterId::Body),
  kParamAir = gs::toIndex(gs::ParameterId::Air),
  kParamEnhance = gs::toIndex(gs::ParameterId::Enhance),
  kParamOutputDb = gs::toIndex(gs::ParameterId::OutputDb),
  kParamBypass = gs::toIndex(gs::ParameterId::Bypass),
  kParamInputSource = gs::toIndex(gs::ParameterId::InputSource),
  kParamBodyProfileA = gs::toIndex(gs::ParameterId::BodyProfileA),
  kParamBodyProfileB = gs::toIndex(gs::ParameterId::BodyProfileB),
  kParamBodyCompareSlot = gs::toIndex(gs::ParameterId::BodyCompareSlot),
  kNumParams = gs::toIndex(gs::ParameterId::Count)
};

static_assert(kNumParams == static_cast<int>(gs::parameterCount()));

using namespace iplug;
using namespace igraphics;

class GitaSedap final : public Plugin
{
public:
  explicit GitaSedap(const InstanceInfo& info);

  bool SerializeState(IByteChunk& chunk) const override;
  int UnserializeState(const IByteChunk& chunk, int startPos) override;

#if IPLUG_EDITOR
  bool OnHostRequestingSupportedViewConfiguration(int width, int height) override
  {
    return width >= PLUG_MIN_WIDTH && height >= PLUG_MIN_HEIGHT;
  }
#endif

#if IPLUG_DSP
  void OnReset() override;
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif

private:
#if IPLUG_DSP
  [[nodiscard]] static double dbToLinear(double db) noexcept;
  [[nodiscard]] gs::InputSource currentInputSource() const noexcept;
  [[nodiscard]] gs::BodyCompareSlot currentBodyCompareSlot() const noexcept;
  [[nodiscard]] gs::BodyProfileChoice currentBodyProfileChoice() const noexcept;
  [[nodiscard]] const gsdsp::PreparedBodyProfile* currentPreparedBodyProfile() const noexcept;
  [[nodiscard]] bool prepareFactoryBodyProfiles(double sampleRate) noexcept;

  static constexpr std::size_t kFactoryBodyProfileCount =
    static_cast<std::size_t>(gs::BodyProfileChoice::Count);

  gsdsp::SourceAdapter mSourceAdapter;
  gsruntime::BodyProfileRuntime mBodyRuntime;
  std::array<gsdsp::PreparedBodyProfile, kFactoryBodyProfileCount>
    mFactoryBodyProfiles{};
  bool mFactoryBodyProfilesReady{false};

  gsdsp::LinearSmoother mOutputGain;
  gsdsp::BypassCrossfade mBypassCrossfade;
#endif
};
