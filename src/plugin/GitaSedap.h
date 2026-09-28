#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "core/ParameterSpec.h"
#include "core/StateSchema.h"

namespace gs = gitasedap::core;

constexpr int kNumPresets = 1;

enum EParams
{
  kParamBody = gs::toIndex(gs::ParameterId::Body),
  kParamAir = gs::toIndex(gs::ParameterId::Air),
  kParamEnhance = gs::toIndex(gs::ParameterId::Enhance),
  kParamOutputDb = gs::toIndex(gs::ParameterId::OutputDb),
  kParamBypass = gs::toIndex(gs::ParameterId::Bypass),
  kParamInputSource = gs::toIndex(gs::ParameterId::InputSource),
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

  // Realtime-owned scalar state only. No allocation or locking in ProcessBlock.
  double mCurrentOutputGain{1.0};
  double mCurrentBypassMix{0.0};
#endif
};
