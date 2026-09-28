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
      && gs::kCanonicalParameterKeys[5] == "input_source";
}

static_assert(gs::parameterCount() == 6);
static_assert(gs::canonicalKeysAreUnique());
static_assert(allContinuousSpecsValid());
static_assert(stableCanonicalOrder());

int main()
{
  // Compile-time invariants above are the test. Keep an executable so CTest
  // verifies the translation unit under every supported toolchain/configuration.
  return 0;
}
