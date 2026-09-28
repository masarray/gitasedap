# GitaSedap Implementation Phases

Development is gate-driven. A phase is not "done" because code exists; it is done when its exit criteria pass.

## P0 - Product and engineering foundation

### Objective
Freeze the product intent and establish the technical rules before feature code.

### Deliverables
- PRD.
- Architecture.
- Engineering standards.
- Quality gates.
- Initial README.
- ADR template/decision process when implementation begins.

### Exit criteria
- Core signal path agreed.
- V1/non-V1 scope explicit.
- Realtime rules explicit.
- Performance targets measurable.
- State/profile canonicalization strategy defined.

## P1 - Toolchain, repository scaffold, and empty plugin shell

### Objective
Create a reproducible, boring, reliable build before DSP complexity.

### Deliverables
- C++20 + CMake project.
- Framework decision documented.
- Pinned dependency versions.
- Windows VST3 target.
- Standalone/unit-test targets where useful.
- Formatting/lint configuration.
- Debug/Release presets.
- GitHub Actions build.
- Basic plugin scan/load.
- Minimal parameter/state skeleton.
- CI artifact containing test VST3.

### Tests
- Clean clone -> configure -> build.
- Plugin scanner validation.
- State save/load smoke test.
- Repeated editor open/close.
- Basic sanitizer or diagnostic configuration.

### Exit criteria
CI green from a clean checkout, plugin loads in at least one reference host, and no feature DSP is merged before the build foundation is stable.

## P2 - DSP laboratory and benchmark harness

### Objective
Build the measurement tools before tuning the sound.

### Deliverables
- Framework-independent offline renderer.
- WAV fixture loader/writer.
- Test signal generator.
- Golden render harness.
- CPU callback benchmark harness.
- NaN/Inf detector.
- Deterministic parameter sweep/torture tool.
- Reference piezo/microphone fixture convention.

### Exit criteria
A DSP module can be rendered offline, compared deterministically, benchmarked, and regression-tested without launching a DAW.

## P3 - Source Adapter and Dynamic Anti-Quack

### Objective
Create a clean, controlled source before body reconstruction.

### Deliverables
- Input headroom/metering.
- Source type model.
- DC/rumble protection.
- Fast/slow envelope follower.
- Transient metric.
- Dynamic anti-quack filters.
- Parameter smoothing.
- Bypass/crossfade utility.

### Audio tests
- impulse,
- sine sweep,
- pink noise,
- soft/medium/hard guitar fixtures,
- extreme automation.

### Exit criteria
- No instability at supported sample rates.
- No allocations in process.
- Harsh transient reduction is measurable/audible without permanently hollowing normal playing.
- CPU budget comfortably below the final target.

## P4 - Hybrid Body Engine Core

### Objective
Produce the first convincing piezo-to-acoustic-body transformation.

### Deliverables
- TransferFilter interface.
- Short causal FIR implementation.
- ModalBodyBank implementation.
- Factory profile format v1.
- Profile compiler -> PreparedState.
- Safe profile switching.
- Initial Natural/Dreadnought development profiles.

### Engineering work
- Direct FIR benchmark.
- Optional partitioned backend benchmark.
- Modal stability limits.
- Canonical profile IDs/versioning.
- Content hash/checksum.

### Exit criteria
- Body transformation is clearly more natural than raw piezo in blind internal comparisons.
- No obvious reverb-like tail.
- No unstable modes.
- Profile change does not block audio.
- Rapid switching produces no leak or crash.

## P5 - Dynamic Body and Body Space

### Objective
Make the body feel alive rather than like a static IR.

### Deliverables
- playing-intensity mapping,
- dynamic modal excitation,
- dynamic damping/anti-boom behavior,
- controlled body-space network/residual phase,
- Live vs Natural body behavior,
- phase-aware direct/body morph.

### Exit criteria
- Soft, medium, and hard playing differ musically.
- Hard strumming does not create runaway low-mid bloom.
- Body morph has no obvious combing or clicks.
- Live mode remains zero-lookahead.

## P6 - Polish, Air, and Live Safety

### Objective
Turn the core body tone into a finished performance sound.

### Deliverables
- NaturalPolish dynamics.
- Air macro with harshness protection.
- Output level/makeup strategy.
- Output protection.
- Profile-aware feedback restraint.
- Optional manual notch foundation.

### Exit criteria
- Enhance improves consistency without audible pumping.
- Air opens the sound without restoring piezo harshness.
- Level matching is good enough for meaningful A/B tests.
- Live profiles remain stable at realistic stage gain.

## P7 - Preset/Profile Platform and Custom Training

### Objective
Make the engine extensible and reproducible.

### Deliverables
- canonical preset schema v1,
- migration framework,
- profile cache,
- worker pool,
- coalescing queue,
- cancellation/generation system,
- custom pickup+mic training prototype,
- regularized transfer estimation,
- modal extraction,
- residual FIR build,
- profile validator/exporter.

### Exit criteria
- Worker tasks never block audio.
- Stale worker results are dropped.
- Profile/preset parsing is bounded.
- A trained profile can be created and recalled reproducibly.
- Corrupt input cannot crash the plugin.

## P8 - Production GUI/UX

### Objective
Expose a premium, simple hardware-style interface over the mature engine.

### Deliverables
- scalable main panel,
- vector metallic knobs,
- compact display,
- Body/Air/Enhance controls,
- source/profile selector,
- meters,
- preset browser,
- advanced panel,
- HiDPI scaling,
- accessibility/keyboard basics,
- parameter tooltips/reset actions.

### Performance rules
- UI timers/repaints coalesced.
- No audio polling at excessive rates.
- Meter data passed via lock-free/latest-value mechanism.
- Vector/raster assets cached.
- No repeated asset decode in paint.

### Exit criteria
- Smooth resize/interaction.
- Editor can be repeatedly opened/closed without leak.
- UI thread load does not create audio glitches.
- Main workflow is understandable without documentation.

## P9 - Performance and Reliability Hardening

### Objective
Treat efficiency and lifecycle behavior as product features.

### Work
- profiler-guided SIMD/vectorization,
- cache-local modal processing,
- FIR backend threshold tuning,
- remove unnecessary copies,
- preallocation audit,
- lock audit,
- ownership/lifetime audit,
- worker shutdown stress,
- 1,000+ preset/profile-switch stress,
- repeated plugin create/destroy stress,
- ASan/UBSan where toolchain supports,
- Windows diagnostics/heap checks,
- fuzz preset/profile parser,
- long-run audio soak test.

### Exit criteria
- Reference CPU/deadline budget met.
- Zero known leaks.
- No race found by available tooling/stress.
- No unbounded queues or caches.
- No callback spikes attributable to control-plane destruction.
- Regression benchmark stored as release evidence.

## P10 - Host Certification, Beta, and Release

### Objective
Finish the product rather than endlessly add features.

### Host matrix
At minimum define and test current supported versions of:
- REAPER,
- one Steinberg host if available,
- one additional mainstream Windows VST3 host.

Additional hosts can be added as resources permit.

### Deliverables
- pluginval validation,
- host automation tests,
- preset/state recall across sessions,
- sample-rate switching,
- buffer-size switching,
- offline bounce,
- live input test,
- release notes,
- dependency/license notices,
- signed checksums for artifacts,
- versioned GitHub Release,
- known-issues document.

Code signing is not assumed for this open-source project unless the project later chooses to fund it.

### Exit criteria
- All release quality gates pass.
- No critical/high crash, corruption, or audio-glitch issue open.
- Install/use/remove workflow documented.
- Release artifact corresponds exactly to a tagged commit.

## P11 - Post-V1 evolution

Only after V1 is stable:
- richer automatic feedback control,
- macOS AU,
- CLAP,
- profile ecosystem,
- optional vocal enhancement,
- optional harmony engine,
- advanced acoustic models.

Each major feature begins with its own PRD/ADR instead of being inserted directly into the audio callback.

## Milestone discipline

Each phase should normally land through small reviewable PRs:
- Foundation,
- implementation,
- tests/benchmarks,
- tuning/fixtures,
- hardening.

Avoid one giant PR combining architecture, DSP, GUI, and release changes.

## Definition of Done for every DSP PR

A DSP PR is not complete unless applicable items exist:
- unit/invariant test,
- offline audio fixture test,
- CPU impact note,
- allocation/realtime-safety review,
- state compatibility note,
- before/after listening evidence,
- no new warnings,
- documentation for externally visible behavior.
