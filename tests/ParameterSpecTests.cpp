#include "core/ParameterSpec.h"

namespace gs = gitasedap::core;

constexpr bool allContinuousSpecsValid() noexcept
{
  return gs::continuousSpecIsValid(gs::kBodySpec)
      && gs::continuousSpecIsValid(gs::kAirSpec)
      && gs::continuousSpecIsValid(gs::kEnhanceSpec)
      && gs::continuousSpecIsValid(gs::kOutputSpec);
}

constexpr bool stableCanonicalOrder() noexcept
{
  return gs::kCanonicalParameterKeys[0] == "body"
      && gs::kCanonicalParameterKeys[1] == "air"
      && gs::kCanonicalParameterKeys[2] == "enhance"
      && gs::kCanonicalParameterKeys[3] == "output_db"
      && gs::kCanonicalParameterKeys[4] == "bypass"
      && gs::kCanonicalParameterKeys[5] == "input_source"
      && gs::kCanonicalParameterKeys[6] == "body_profile_a"
      && gs::kCanonicalParameterKeys[7] == "body_profile_b"
      && gs::kCanonicalParameterKeys[8] == "body_compare_slot";
}

static_assert(gs::parameterCount() == 9);
static_assert(gs::canonicalKeysAreUnique());
static_assert(allContinuousSpecsValid());
static_assert(stableCanonicalOrder());

static_assert(gs::toIndex(gs::ParameterId::Body) == 0);
static_assert(gs::toIndex(gs::ParameterId::InputSource) == 5);
static_assert(gs::toIndex(gs::ParameterId::BodyProfileA) == 6);
static_assert(gs::toIndex(gs::ParameterId::BodyProfileB) == 7);
static_assert(gs::toIndex(gs::ParameterId::BodyCompareSlot) == 8);

int main()
{
  return 0;
}
