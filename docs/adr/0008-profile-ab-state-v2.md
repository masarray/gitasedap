# ADR-0008: Canonical factory-profile A/B comparison and state schema v2

- **Status:** Accepted for P4C
- **Date:** 2026-10-01
- **Scope:** Factory body profile selection, level-matched A/B workflow, and backward-compatible state migration

## Context

P4A created the hybrid body engine and P4B created generation-safe profile
publication/crossfading.

The next requirement is not more DSP complexity. It is a reliable listening
workflow that lets a tester compare:

- Raw / P3 conditioned signal,
- Natural Development,
- Dreadnought Development,

without hard switching, state ambiguity, or "louder wins" bias.

The selector must also survive DAW project save/restore without changing the
meaning of the six canonical parameters already used by earlier engineering
builds.

## Decision

P4C adds three append-only host parameters:

- ID 6: `body_profile_a`
- ID 7: `body_profile_b`
- ID 8: `body_compare_slot`

Existing IDs 0..5 do not move.

Default comparison state:

- A = Natural Development
- B = Dreadnought Development
- Listen = A

Both A and B can independently select:

- Raw / P3
- Natural Development
- Dreadnought Development

The Listen A/B selector chooses which slot is actually sent to the body runtime.

## Raw / P3 as a canonical profile

Raw comparison is not implemented by bypassing the body subsystem.

Instead it is a canonical identity profile:

- one FIR tap = 1.0
- zero modal modes
- output gain = 1.0

That means Raw, Natural, and Dreadnought all use the same P4B profile-switching
path and the same 15 ms bounded crossfade.

This avoids a separate hard-bypass comparison path.

## Factory profile compilation

All three factory profiles are compiled during `OnReset()`.

The audio callback only:

1. reads the selected A/B parameter values,
2. chooses one already-prepared fixed-size profile,
3. copies it into a bounded desired slot only when the content hash changes,
4. starts a P4B transition at a block boundary when possible.

There is no coefficient design, heap allocation, lock, file I/O, or profile
parsing in `ProcessBlock()`.

## Coalescing

A/B clicks can occur faster than the 15 ms crossfade.

The runtime stores one latest desired fixed-size prepared profile.

While a transition is active:

- the current transition completes,
- intermediate superseded selections are discarded,
- the latest desired profile starts on the next eligible block.

External/custom profile generations from `PreparedStateExchange` are consumed
once per generation, so an old external active state cannot continuously
overwrite a later factory A/B selection.

## Level matching

P4C changes the two development profiles' output compensation to unity and
bumps their canonical development keys to:

- `factory.natural.dev.v2`
- `factory.dreadnought.dev.v2`

The body-profile schema itself remains version 1.

A deterministic four-second pink-noise gate checks at BODY=100:

- Natural vs Raw within ±0.35 dB RMS,
- Dreadnought vs Raw within ±0.35 dB RMS,
- Natural vs Dreadnought within ±0.25 dB RMS.

This is an **engineering comparison gate**, not a claim of perceptual loudness
equivalence on real guitar material.

Real-guitar level matching and tonal approval remain part of P4D.

## State schema v2

State version 1 serialized six parameters:

0. Body
1. Air
2. Enhance
3. Output
4. Bypass
5. Input Source

State version 2 serializes the same first six values followed by the three P4C
comparison parameters.

When a version-1 state is loaded:

- all six historical values are restored unchanged,
- A defaults to Natural Development,
- B defaults to Dreadnought Development,
- Listen defaults to A.

The migration is explicit and framework-independent in `StateMigration.h`.

Unknown future versions remain rejected rather than guessed.

## Consequences

Positive:

- fairer real-guitar comparisons,
- click-safe Raw/Natural/Dread switching,
- reproducible A/B slot state,
- old engineering sessions continue to load,
- future profile UI can grow without changing IDs 0..8.

Cost:

- three additional host-visible parameters,
- state schema increments from v1 to v2,
- development-profile hashes change because their level compensation changed.

That cost is accepted because P4C is the correct point to lock the comparison
contract before perceptual tuning begins.
