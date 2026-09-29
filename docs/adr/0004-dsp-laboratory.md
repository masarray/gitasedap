# ADR-0004: Framework-independent DSP laboratory

- **Status:** Accepted for P2
- **Date:** 2026-09-29
- **Scope:** Offline rendering, fixtures, regression, and performance measurement

## Context

Acoustic DSP cannot be developed professionally if every experiment requires
opening a DAW and judging an uncontrolled recording by ear.

Before P3 introduces source conditioning and dynamic anti-quack, the project
needs repeatable measurement tools that do not depend on iPlug2 or a plugin host.

## Decision

Create a small plain-C++20 laboratory layer under `src/lab`.

The laboratory owns test/offline memory and may allocate. Product realtime DSP
must remain allocation-free once prepared.

### AudioBuffer

Planar float32 storage is the canonical P2 in-memory audio representation.

### OfflineRenderer

DSP modules expose a minimal preparation/reset/process contract for offline
testing.

The renderer prepares once, splits a fixture into deterministic blocks, and
allows the same processor to be rendered with different block sizes.

### WAV fixtures

The reader accepts PCM16, PCM24, PCM32, and IEEE float32.

The writer produces canonical IEEE float32 RIFF/WAVE fixtures.

RF64, compressed codecs, and WAVE_FORMAT_EXTENSIBLE are deliberately rejected in
P2 instead of silently mis-parsed.

### Synthetic signals

The lab supplies impulse, sine, logarithmic sweep, deterministic white noise, and
deterministic pink noise.

Noise uses a fixed in-project PRNG rather than an implementation-defined standard
library distribution.

### Numeric guards

Every DSP milestone can use NaN/Inf detection, peak/RMS inspection, and tolerant
golden comparison.

Golden comparisons use absolute + relative tolerance because exact
floating-point transcendental results can vary across compilers.

### Parameter torture

A deterministic generator creates normalized parameter events using a fixed
PRNG, exact edge values, and sorted sample offsets.

### Deadline benchmark

The benchmark harness records mean, p95, p99, p99.9, maximum callback time,
buffer deadline, and p99/deadline ratio.

GitHub-hosted runners are smoke evidence only. Release CPU budgets are evaluated
on a documented reference machine; noisy shared CI is not used as a hard
realtime-performance oracle.

## CLI

`gitasedap_dsp_lab` provides generate, inspect, compare, and benchmark commands
without launching a DAW.

## Fixture truth

The repository does not fabricate "real guitar" paired fixtures.

Real pickup/microphone reference pairs must come from the same documented
performance. Synthetic signals are engineering stimuli, not perceptual truth.

## Consequences

P3/P4 DSP can now be rendered, compared, corruption-checked, and benchmarked
outside the plugin host while keeping the product dependency surface small.
