#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>

namespace gitasedap::dsp
{

class Biquad
{
public:
  void reset() noexcept
  {
    mZ1 = 0.0;
    mZ2 = 0.0;
  }

  void setHighPass(
    double sampleRate,
    double frequencyHz,
    double q
  ) noexcept
  {
    const auto design = commonDesign(sampleRate, frequencyHz, q);
    const double cosine = std::cos(design.omega);

    const double b0 = (1.0 + cosine) * 0.5;
    const double b1 = -(1.0 + cosine);
    const double b2 = b0;

    setNormalized(
      b0,
      b1,
      b2,
      design.a0,
      -2.0 * cosine,
      1.0 - design.alpha
    );
  }

  void setBandPass(
    double sampleRate,
    double frequencyHz,
    double q
  ) noexcept
  {
    const auto design = commonDesign(sampleRate, frequencyHz, q);
    const double cosine = std::cos(design.omega);

    setNormalized(
      design.alpha,
      0.0,
      -design.alpha,
      design.a0,
      -2.0 * cosine,
      1.0 - design.alpha
    );
  }

  [[nodiscard]] double process(double input) noexcept
  {
    const double output = (mB0 * input) + mZ1;
    mZ1 = (mB1 * input) - (mA1 * output) + mZ2;
    mZ2 = (mB2 * input) - (mA2 * output);

    if(std::abs(mZ1) < 1.0e-30)
      mZ1 = 0.0;

    if(std::abs(mZ2) < 1.0e-30)
      mZ2 = 0.0;

    return output;
  }

private:
  struct CommonDesign
  {
    double omega{0.0};
    double alpha{0.0};
    double a0{1.0};
  };

  [[nodiscard]] static CommonDesign commonDesign(
    double sampleRate,
    double frequencyHz,
    double q
  ) noexcept
  {
    const double safeRate = std::max(sampleRate, 1.0);
    const double nyquist = safeRate * 0.5;
    const double safeFrequency = std::clamp(
      frequencyHz,
      1.0,
      std::max(1.0, nyquist * 0.95)
    );
    const double safeQ = std::clamp(q, 0.05, 20.0);
    const double omega =
      2.0 * std::numbers::pi * safeFrequency / safeRate;
    const double alpha =
      std::sin(omega) / (2.0 * safeQ);

    return CommonDesign{
      omega,
      alpha,
      1.0 + alpha
    };
  }

  void setNormalized(
    double b0,
    double b1,
    double b2,
    double a0,
    double a1,
    double a2
  ) noexcept
  {
    const double inverseA0 = 1.0 / a0;

    mB0 = b0 * inverseA0;
    mB1 = b1 * inverseA0;
    mB2 = b2 * inverseA0;
    mA1 = a1 * inverseA0;
    mA2 = a2 * inverseA0;
  }

  double mB0{1.0};
  double mB1{0.0};
  double mB2{0.0};
  double mA1{0.0};
  double mA2{0.0};
  double mZ1{0.0};
  double mZ2{0.0};
};

} // namespace gitasedap::dsp
