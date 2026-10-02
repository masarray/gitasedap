# P4D Real-Guitar Calibration Capture Protocol

P4D is an evidence phase. Do not change Natural/Dreadnought profile coefficients
from memory or preference alone.

## Preferred capture

Record **piezo and microphone simultaneously** through the same audio interface
and clock.

Recommended session:

- 48 kHz / 24-bit PCM or 32-bit float WAV,
- no EQ, compression, limiter, noise reduction, AGC, reverb, or mastering,
- piezo on one interface input,
- condenser microphone on another input,
- fixed guitar and microphone position for the whole take,
- peaks roughly between -12 and -6 dBFS when practical,
- absolutely no digital clipping,
- 30-60 seconds total usable playing.

A single stereo WAV is acceptable if:
- channel 1 = piezo,
- channel 2 = microphone.

Two mono WAV files are also acceptable.

## Microphone placement

The goal is a useful natural reference, not a room recording.

Start with a consistent close/medium placement such as:
- roughly 30-45 cm from the guitar,
- aimed around the 12th-fret / soundhole-edge region,
- avoid pointing directly into the soundhole if that creates excessive boom.

Do not change microphone position between soft/medium/hard passages.

Document the microphone model and approximate placement when possible.

## Playing script

Keep all sections in the same recording.

1. 5-10 s soft open chords / gentle strumming.
2. 5-10 s medium strumming.
3. 5-10 s hard strumming.
4. 5-10 s single notes across low, middle, and high strings.
5. 5-10 s fingerstyle/arpeggio material.
6. Let several chords/notes decay naturally.

Avoid long silent sections.

## Why simultaneous capture matters

The piezo and microphone must describe the same performance.

P4D measures:
- relative spectral transfer shape,
- body-band difference,
- upper-mid/quack difference,
- air-band difference,
- candidate body resonances,
- approximate reference delay/alignment,
- residual spectral error after a candidate profile.

Separate performances are useful for subjective listening, but they are not a
valid paired calibration measurement.

## Calibration quality gates

The P4D analyzer rejects or flags captures with:
- insufficient duration,
- NaN/Inf samples,
- clipping,
- extremely low RMS level,
- weak timing/envelope alignment,
- insufficient spectral support.

Microphone and piezo recording gains do **not** need to be identical. The
analysis removes one broad transfer normalization offset before evaluating
spectral shape.

## What P4D will and will not do

P4D may propose modal frequency/Q/prominence candidates and measure residual
spectral error.

P4D does **not** automatically overwrite the factory profile.

Profile changes require:
1. valid capture,
2. measured hypothesis,
3. candidate profile,
4. offline regression,
5. level-matched Raw/Natural/Dread listening,
6. explicit acceptance of the real-guitar result.

This prevents profile tuning from becoming uncontrolled trial-and-error.
