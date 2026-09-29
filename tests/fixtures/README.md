# GitaSedap reference audio fixture convention

P2 establishes the fixture contract before acoustic DSP tuning begins.

Binary guitar recordings are intentionally **not** invented or synthesized as
reference truth. Real paired pickup/microphone recordings will be added when
captured or supplied.

## Canonical audio format

Preferred committed regression fixture format:

- RIFF/WAVE
- IEEE float32
- mono for one physical source unless stereo is semantically required
- 48 kHz preferred reference rate
- normalized only when the fixture's purpose explicitly requires it
- no lossy compression

The P2 WAV reader also accepts PCM16, PCM24, and PCM32 so field/reference
recordings can be imported without an external conversion step.

## Directory convention

```text
tests/fixtures/audio/
  synthetic/
    impulse_48k.wav
    sweep_20_20k_48k.wav
    pink_48k.wav

  paired/
    <fixture-id>/
      pickup.wav
      microphone.wav
      README.md

  golden/
    <module>/
      <fixture-id>__<preset-or-case>.wav
```

## Paired fixture identity

A paired pickup/microphone fixture directory must document:

- fixture ID
- instrument/body family
- pickup type
- pickup location if known
- microphone model/type if known
- microphone placement/distance if known
- interface/preamp if relevant
- sample rate
- gain staging notes
- performance style
- intensity: soft / medium / hard
- whether both channels were recorded simultaneously
- any edits, trimming, or alignment performed

The pickup and microphone files in one paired fixture must represent the same
performance. They must never be paired from unrelated takes and presented as a
training/reference pair.

## Recommended performance coverage

For profile research:

- open chords
- single notes across the neck
- fingerstyle
- soft strumming
- medium strumming
- hard strumming
- muted/percussive attacks where relevant

## Golden render rule

A golden render is not approved merely because code generated it.

When product DSP begins in P3:

1. render a known input through a named case,
2. analyze NaN/Inf, level, and comparison metrics,
3. listen level-matched,
4. approve the render as the intended baseline,
5. record the input fixture ID and processing case in the PR.

Intentional golden changes require an explicit listening/regression note.

Cross-platform comparisons use numeric tolerances rather than assuming
bit-identical transcendental math across all compilers.
