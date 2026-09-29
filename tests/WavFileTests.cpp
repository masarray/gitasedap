#include "TestSupport.h"
#include "lab/GoldenCompare.h"
#include "lab/SignalGenerator.h"
#include "lab/WavFile.h"

#include <filesystem>
#include <system_error>

namespace gslab = gitasedap::lab;

int main()
{
  const auto source = gslab::SignalGenerator::whiteNoise(
    2,
    4096,
    0x123456789ABCDEF0ULL,
    0.2F
  );

  const auto path =
    std::filesystem::temp_directory_path()
    / "gitasedap_p2_wav_roundtrip.wav";

  gslab::WavFile::writeFloat32(path, 48000U, source);
  const auto loaded = gslab::WavFile::read(path);

  GS_REQUIRE(loaded.sampleRate == 48000U);

  const auto result = gslab::compareGolden(
    source,
    loaded.audio,
    gslab::GoldenTolerance{0.0, 0.0}
  );

  GS_REQUIRE(result.passed());

  std::error_code error;
  std::filesystem::remove(path, error);

  return 0;
}
