# ADR-0005: Causal source conditioning and transient-selective anti-quack

- **Status:** Accepted for P3 engineering implementation
- **Date:** 2026-09-29
- **Scope:** Source Adapter, transient detector, dynamic anti-quack, smoothing, and bypass transition

## Context

The body engine should not be asked to repair uncontrolled pickup artifacts.

P3 therefore creates a bounded, causal preprocessing stage before P4 body
reconstruction.

The product requirement is not to impose a permanent broad upper-mid scoop.
Piezo harshness should be reduced primarily when the playing event is both
transient-like and energy-rich in the quack/harshness bands.

## Decision

Use a zero-lookahead source adapter composed of:

1. finite-input sanitization,
2. smoothed input trim,
3. 30 Hz second-order rumble/DC high-pass,
4. fast and slow broadband envelope followers,
5. normalized transient score from fast-vs-slow envelope separation,
6. fixed stable band-pass observers in upper-mid/high bands,
7. transient- and band-presence-dependent subtraction,
8. attack/release smoothing of reduction amount,
9. per-block peak/RMS/headroom/transient/reduction metering.

The processing path performs no allocation, lock, wait, file I/O, logging, or
coefficient design while processing samples.

## Why fixed band-pass subtraction

Recalculating parametric-EQ coefficients at audio rate is unnecessary for this
milestone.

A fixed band-pass observer followed by bounded dynamic subtraction gives:

- stable fixed poles,
- cheap per-sample work,
- continuously variable reduction without coefficient interpolation,
- easy offline measurement,
- zero lookahead.

P4/P6 may later refine the spectral model if real guitar evidence shows a need.

## Initial band model

P3 uses conservative engineering starting points:

- primary quack observer: approximately 2.6 kHz, Q 0.85,
- secondary harsh observer: approximately 5.2 kHz, Q 1.10.

These values are **not declared universal acoustic-guitar truth**.

They are intentionally isolated as implementation constants and must be tuned
against real, level-matched guitar fixtures before the perceptual P3 gate is
closed.

## Source types

The existing canonical source identity remains authoritative:

- Active Piezo
- Passive Piezo
- Magnetic

Source selection changes only bounded detector/reduction behavior in P3.

No claim is made that software can repair analog loading already caused by an
inappropriate hardware input impedance, especially for passive piezo pickups.

No permanent source-type EQ curve is introduced without fixture evidence.

## Source-type starting behavior

The two piezo modes allow stronger transient-selective reduction.

Magnetic mode remains deliberately conservative.

Changing source type ramps detector/reduction parameters rather than switching
audible gain discontinuously.

## Smoothing

Continuous changes use precomputed linear ramps.

Dynamic reduction itself uses asymmetric attack/release envelope followers.

Bypass uses a short linear dry/wet crossfade. A linear crossfade is preferred
here because the dry and conditioned paths are strongly correlated; an
equal-power crossfade could create a level bump.

## Metering

The Source Adapter reports:

- raw input peak,
- raw input RMS,
- headroom relative to full scale,
- maximum transient score,
- maximum estimated reduction for the processed block.

P3 keeps this inside the DSP API. UI publication belongs to the later GUI
milestone and must use a realtime-safe bridge.

## Testing

P3 engineering tests cover:

- 44.1 / 48 / 88.2 / 96 kHz,
- sweep finiteness,
- DC rejection,
- 10 Hz rumble attenuation,
- transient-selective upper-mid reduction,
- sustained-tone preservation,
- source-type differentiation,
- block-size invariance,
- extreme legal automation,
- non-finite input containment,
- input headroom metering,
- smoother/bypass transition behavior.

CI also records source-adapter callback timing at 48 and 96 kHz.

Shared CI timing is evidence, not a hard release realtime oracle.

## Realtime and latency impact

The algorithm is causal and zero-lookahead.

No plugin latency is reported.

The hot path is a fixed number of:

- biquad operations,
- envelope updates,
- scalar smoothers,
- bounded arithmetic/clamping.

## State compatibility

P3 adds no new host-facing parameter IDs.

Existing canonical state remains valid.

The existing Input Source parameter becomes functional rather than decorative.

Body, Air, and Enhance remain reserved for their owning later milestones.

## Perceptual limitation

Synthetic fixtures can prove selectivity and stability, but they cannot prove
that a piezo guitar sounds natural.

The final P3 perceptual exit criterion requires real guitar material, ideally
including soft/medium/hard piezo recordings. Paired microphone material is even
more valuable for later P4 transfer/body work.

P3 engineering can therefore be verified independently while acoustic tuning is
explicitly gated on real fixture evidence.
