# ADR-0006: Bounded hybrid body core with canonical prepared profiles

- **Status:** Accepted for P4A engineering implementation
- **Date:** 2026-09-29
- **Scope:** Short causal transfer FIR, modal bank, profile schema v1, profile compilation, and BODY correction morph

## Context

P3 produces a controlled source. P4 begins the actual acoustic-body transformation.

The project architecture explicitly rejects two naive extremes:

- one long IR pretending to be the whole body, and
- a collection of static EQ boosts pretending to be resonance.

The body core must remain causal, lightweight, testable outside the host, and suitable for later immutable prepared-state publication.

## Decision

P4A uses two complementary components:

1. a short direct causal FIR for broad residual transfer shape,
2. a bounded modal bank for narrow body resonances.

The profile definition is semantic/canonical. The compiler converts it into a
sample-rate-specific prepared profile.

Runtime DSP never parses profiles or designs coefficients.

## Fixed bounds

P4A deliberately fixes conservative maximums:

- transfer FIR: 128 taps,
- modal bank: 12 modes.

Runtime storage is fixed-size and contiguous.

There is no heap allocation, container growth, lock, file I/O, profile parsing,
or coefficient design in the per-sample path.

## TransferFilter

The transfer filter is a bounded direct convolution implementation.

The first profile tap is the immediate causal path. Later taps represent residual
spectral/time correction.

For these short kernels, direct convolution is intentionally chosen first.

Partitioned FFT convolution remains a benchmark-gated backend for longer future
profiles and is not introduced speculatively.

## ModalBodyBank

Modes are compiled into normalized second-order band-pass coefficients.

Runtime coefficient/state arrays use a structure-of-arrays layout to reduce
pointer chasing and keep the hot loop contiguous.

P4A is static. Playing-intensity-dependent excitation/damping belongs to P5.

## BODY morph

P4A does not introduce the final phase-aware P5 morph.

Instead, profiles preserve an identity first FIR tap and the runtime injects only
the correction component:

```
correction = (transfer(profileGain) - input) + modal
output     = input + bodyAmount * correction
```

Therefore BODY=0 is exact conditioned input.

This is a development-safe correction morph, not the final P5 direct/body phase
model.

## Canonical profile schema

Schema v1 includes:

- stable numeric profile ID,
- canonical string key,
- display name,
- schema version,
- bounded transfer taps,
- bounded modal frequency/Q/gain triples,
- output compensation.

Display names are never persistent identity.

A deterministic FNV-1a content hash is computed from explicit canonical fields.

The hash is content identity/evidence; it is not a cryptographic signature.

## Profile compiler

The compiler is control-plane work.

It validates:

- schema version,
- canonical key,
- FIR count,
- FIR finiteness and L1 magnitude bound,
- modal count,
- modal frequency/Q/gain ranges,
- output gain,
- target sample rate,
- mode frequency relative to Nyquist.

It then produces sample-rate-specific immutable prepared coefficients.

## Development profiles

P4A includes two explicitly non-final engineering profiles:

- Natural Development,
- Dreadnought Development.

Their FIR/modal values are conservative placeholders for architecture,
measurement, switching, and listening workflow development.

They are **not** claimed to reproduce a measured microphone/body response.

Final perceptual profile tuning requires real pickup/microphone evidence or
carefully approved guitar reference material.

## Realtime impact

P4A is zero-lookahead and reports no additional plugin latency.

For each sample the maximum work is bounded by:

- up to 128 FIR multiply-adds,
- up to 12 modal biquads,
- one smoother,
- scalar correction morph.

The actual development profiles currently use substantially fewer than the
maximum taps.

## Next P4 slice

P4B adds generation-aware prepared-profile publication and click-safe dual-engine
profile switching without destroying prepared states on the audio thread.
