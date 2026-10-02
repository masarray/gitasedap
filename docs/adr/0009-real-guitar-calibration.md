# ADR-0009: Evidence-driven P4D real-guitar calibration

- **Status:** Accepted
- **Date:** 2026-10-02
- **Scope:** Paired pickup/microphone analysis for factory-profile calibration

## Decision

P4D adds a framework-independent offline calibration analyzer before changing
factory tonal coefficients.

The analyzer consumes simultaneous piezo and microphone material and produces:

- input quality gates,
- approximate reference lag from transient-envelope correlation,
- 48 logarithmically spaced spectral-band measurements,
- relative transfer after broad gain normalization,
- supported-band count,
- spectral-shape RMS/max error,
- body/quack/air regional means,
- candidate low/mid body resonances with approximate Q/prominence/confidence.

## Why no automatic factory-profile mutation

Automatic fitting belongs to the later custom-profile/training platform.

P4D instead uses measured candidates to tune the two fixed development profiles
under review. This keeps the factory-tone decision understandable and avoids
prematurely introducing a large optimization/training subsystem.

## Spectral analysis

The analyzer uses a fixed 48-band logarithmic constant-Q measurement bank.

This is intentionally lighter and easier to audit than introducing an FFT
framework solely for P4D.

It is offline/control-plane code. It is never linked into the plugin callback.

For each supported band:

```
rawTransferDb = 10 log10(referencePower / piezoPower)
relativeTransferDb = rawTransferDb - median(midbandTransfer)
```

The median normalization removes arbitrary interface/preamp gain difference.

The resulting curve describes **shape**, not absolute microphone level.

## Alignment

Magnitude calibration does not require sample-perfect phase alignment, but bad
pairing often indicates an invalid capture.

The analyzer therefore estimates reference lag using a decimated transient
envelope correlation over a bounded ±60 ms search.

The lag is used to align the analysis windows and is reported as a capture
quality metric.

## Candidate modes

Low/mid local maxima are detected only where:
- the piezo has sufficient spectral support,
- frequency is inside the body-candidate range,
- smoothed prominence exceeds a fixed minimum.

Approximate Q is estimated from the measured -3 dB width when available.

These values are **candidates for review**, not automatically trusted modal
coefficients.

## Optimization and realtime impact

P4D analysis may allocate vectors and perform repeated offline filtering because
it runs in the laboratory/control plane.

It adds:
- zero work to the audio callback,
- zero plugin latency,
- zero realtime memory ownership.

The production engine remains the already verified bounded P4A/P4B path.

## Acceptance

P4D is not complete merely because the analyzer passes synthetic tests.

The engineering tooling can be verified in CI, but the P4D tonal gate requires
a real simultaneous piezo/reference capture and an approved level-matched A/B
result.
