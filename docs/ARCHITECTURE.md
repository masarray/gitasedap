# GitaSedap Architecture

## 1. Architecture goals

The architecture separates real-time audio work from control-plane work so complex analysis can exist without making the callback unpredictable.

The guiding model is:

```text
                 CONTROL PLANE
 UI / Host State / Presets / Profile Files
                  |
          coalesced requests
                  |
          bounded worker pool
                  |
       immutable PreparedState
                  |
          atomic handoff
                  v
================================================
                 AUDIO PLANE
 per-block state snapshot -> bounded DSP pipeline
================================================
```

The audio plane never waits for the control plane.

## 2. Proposed repository layout

```text
/
  CMakeLists.txt
  cmake/
  docs/
  src/
    plugin/
      PluginProcessor.*
      PluginEditor.*
    core/
      Parameters.*
      StateSchema.*
      Version.*
      RealtimeGuards.*
      Diagnostics.*
    dsp/
      SourceAdapter.*
      EnvelopeFollower.*
      DynamicAntiQuack.*
      TransferFilter.*
      ModalBodyBank.*
      BodySpace.*
      BodyMorph.*
      NaturalPolish.*
      AirProcessor.*
      FeedbackGuard.*
      OutputStage.*
      AcousticEngine.*
    profile/
      ProfileModel.*
      ProfileCodec.*
      ProfileCompiler.*
      ProfileCache.*
      ProfileTrainer.*
    runtime/
      WorkerPool.*
      CoalescingQueue.*
      PreparedState.*
      StatePublisher.*
      DeferredReclaimer.*
    ui/
      Theme.*
      VectorKnob.*
      MainPanel.*
      AdvancedPanel.*
  tests/
    unit/
    dsp/
    regression/
    performance/
    fixtures/
  tools/
    offline-render/
    profile-analyzer/
```

Names may evolve, but the dependency direction should remain stable.

## 3. Dependency direction

Allowed high-level direction:

```text
UI ----> core/domain <---- plugin wrapper
              |
              v
          DSP engine
              ^
              |
     profile/runtime services
```

The DSP core must not depend on the UI.

Host/framework classes should be kept at the boundary so DSP can be tested using a standalone offline harness.

## 4. Canonical state model

The project should maintain one canonical product-state schema.

Rules:
- Parameter identity is a stable machine ID, separate from display text.
- Preset state stores semantic values, not raw widget state.
- Profile IDs are canonical UUID/string IDs.
- DSP coefficients are derived artifacts, not canonical persisted state.
- Every persisted schema has an explicit version.
- Migrations move old canonical states forward.
- Derived caches may be discarded and rebuilt.

This avoids multiple competing "truths" between UI, host automation, preset files, and the DSP engine.

## 5. Prepared immutable DSP state

Expensive or allocation-heavy work creates an immutable `PreparedState` off-thread.

A prepared state can include:
- FIR coefficients and backend plan,
- modal coefficients,
- dynamic curves,
- sample-rate-specific filters,
- profile metadata,
- precomputed smoothing constants,
- phase/body-space data.

Publishing sequence:

```text
Worker builds complete candidate
          |
          v
Validate candidate
          |
          v
Publish generation N
          |
          v
Audio thread swaps at block boundary
          |
          v
Old generation deferred for destruction off audio thread
```

Do not allow partially updated DSP objects to become visible to the callback.

## 6. Coalescing

Coalescing is used where repeated control-plane work would otherwise duplicate expensive preparation.

Examples:
- User drags body-type/profile selector rapidly.
- Sample rate/device preparation request supersedes an older preparation.
- Preset browser causes multiple near-simultaneous preview requests.
- UI resize/repaint work.
- Profile recompilation after several edits.

Pattern:
- assign every request a monotonically increasing generation,
- keep only the newest equivalent pending request,
- cancel or allow older work to finish,
- drop stale results whose generation is no longer current.

Important: host automation timing must not be destroyed by generic coalescing. Musical parameter automation follows the framework/host event model and uses smoothing as required. Coalescing is primarily for expensive control-plane tasks and high-rate UI gestures.

## 7. Worker model

Use a small bounded worker pool, not one thread per task.

Worker candidates:
- profile file parsing and validation,
- FIR/profile compilation,
- minimum-phase conversion,
- modal fitting,
- resampling,
- custom-profile analysis,
- preset scanning,
- non-real-time waveform/statistical analysis.

Worker rules:
- bounded queue,
- explicit cancellation token,
- generation-aware stale-result dropping,
- no access to mutable audio-thread state,
- no unbounded retry,
- clean shutdown with deterministic join,
- failures converted to structured errors for UI/control plane.

The initial implementation should prefer 1-2 worker threads unless profiling proves more are beneficial.

## 8. Audio-thread ownership and memory

### Forbidden in process callback
- `new`, `delete`, malloc/free,
- vector/string growth,
- locks,
- reference-count destruction that can free large graphs,
- filesystem/network,
- exceptions escaping the callback,
- synchronous logging,
- dynamic plugin/profile discovery.

### Preferred
- fixed/preallocated buffers,
- stack or stable object storage,
- RAII outside the callback,
- trivially owned DSP state,
- ring buffers only when necessary,
- lock-free SPSC handoff for small control messages,
- atomic scalar snapshots,
- deferred destruction/reclamation.

### Shared ownership warning
`std::shared_ptr` may perform an atomic decrement and trigger destruction on the audio thread. It should not be the default realtime handoff primitive. Prepared states should use a publication/reclamation scheme that guarantees heavyweight destruction outside the callback.

## 9. Parameter flow

Three categories:

### A. Audio-rate or automation-sensitive
Examples: bypass, Body, Air, Enhance, output trim.

- read from canonical parameter transport,
- smooth click-sensitive values,
- bounded per-sample or per-block work,
- preserve host automation semantics as far as the plugin framework exposes them.

### B. Structural
Examples: loading a different body profile.

- never rebuild filters synchronously in the callback,
- request preparation on worker,
- publish immutable prepared state,
- crossfade old/new state if audible discontinuity is possible.

### C. UI-only
Examples: panel fold state.

- must not affect audio serialization unless user experience requires persistence.

## 10. DSP pipeline

### 10.1 Source Adapter
Responsibilities:
- DC/rumble protection,
- source-type tonal normalization,
- input gain/headroom,
- safe preprocessing.

### 10.2 Dynamic Anti-Quack
Uses fast/slow envelope information and frequency-dependent control.

Design intent:
- transient-selective correction,
- avoid permanent deep upper-mid scoop,
- stable coefficient interpolation,
- no lookahead.

### 10.3 Transfer Filter
Represents broad pickup-to-body/microphone spectral transformation.

Preferred characteristics:
- causal,
- minimum-phase or controlled mixed-phase,
- short kernel in Live mode,
- direct SIMD backend for short kernels,
- partitioned backend only when profiling shows benefit.

Backend choice is an implementation detail behind one interface.

### 10.4 Modal Body Bank
Represents narrow/longer body resonances efficiently.

Implementation direction:
- 6-12 second-order resonant modes initially,
- structure-of-arrays storage for cache/SIMD friendliness when beneficial,
- coefficient validation,
- sample-rate-derived coefficients,
- dynamic excitation/damping modulation.

### 10.5 Body Space
Very short phase/time character; not a room reverb.

Candidates:
- residual FIR phase,
- tiny dispersive taps,
- short all-pass network.

It must remain bounded and subtle.

### 10.6 Body Morph
Combines direct/conditioned and body-transformed paths without uncontrolled combing.

Requirements:
- level-consistent macro behavior,
- phase-aware design,
- click-free parameter changes,
- no hidden latency in Live mode.

### 10.7 Natural Polish
Transparent dynamic refinement.

Possible implementation:
- dynamic EQ and/or two-band dynamics,
- gentle broad compression,
- transient-aware behavior,
- no pumping under normal guitar playing.

### 10.8 Air
Adds openness while protecting harshness.

Air macro may coordinate:
- adaptive high shelf,
- high-frequency dynamic control,
- subtle harmonic/texture component only if it survives listening and CPU tests.

### 10.9 Feedback Guard
Live-safety subsystem.

V1 can begin with static/profile-aware restraint. Automatic notch detection is gated to a later phase unless it meets stability and false-positive requirements.

## 11. FIR strategy

Do not optimize based on theory alone.

Start with two measured implementations:
1. SIMD direct convolution for short kernels.
2. Zero/low-latency head + partitioned tail for longer kernels.

A benchmark selects thresholds by:
- kernel length,
- host block size,
- sample rate,
- target CPU architecture.

The public DSP interface remains stable while backend thresholds can change.

## 12. Modal-bank optimization

Potential optimization order:
1. Correct scalar implementation.
2. Precomputed coefficients.
3. Cache-friendly contiguous storage.
4. Process groups using SoA layout.
5. SIMD only if profiler shows measurable benefit.

Do not add SIMD complexity before reference correctness exists.

## 13. Profile model

Canonical profile data should prefer physical/semantic representations:
- frequencies in Hz,
- Q/damping,
- gains in dB/linear documented units,
- normalized dynamic curves,
- phase/time metadata,
- residual transfer data in a documented canonical sample-rate or rate-independent representation.

At `prepare(sampleRate)`, profile data becomes optimized runtime coefficients.

## 14. Profile cache

The cache uses canonical IDs and content hashes.

Requirements:
- avoid duplicate decode/compile of the same profile,
- bounded size,
- LRU or simple fixed policy only if measurements justify it,
- immutable cached objects,
- thread-safe control-plane access,
- never evict/destroy heavy objects on the audio callback.

## 15. Failure isolation

Malformed profile/preset input must not destabilize audio.

Validation:
- finite numbers only,
- legal sample-rate range,
- resonance/Q bounds,
- FIR length caps,
- stable biquad poles,
- payload size limits,
- checksum/version validation.

On failure, continue using the last valid prepared state and report an error to UI.

## 16. Shutdown lifecycle

Order:
1. stop accepting new worker tasks,
2. cancel pending analysis,
3. detach UI callbacks,
4. stop/pause audio processing,
5. drain/defer realtime publications,
6. join workers,
7. release caches/resources.

All asynchronous callbacks use lifetime-safe weak tokens/generation tokens so no task writes into a destroyed plugin instance.

## 17. Observability

Debug/development builds may collect:
- callback duration histogram,
- worker queue depth,
- profile compile time,
- state generation count,
- peak memory,
- NaN/Inf guards.

Never synchronously print from the audio callback. Realtime metrics write to fixed counters/ring storage and are drained elsewhere.

## 18. Security and robustness

Even a desktop VST should treat external preset/profile files as untrusted:
- strict size limits,
- bounded parsing,
- no arbitrary path traversal,
- no executable content,
- checksums for shipped factory assets,
- fuzz parsing after the profile format stabilizes.

## 19. Framework decision

JUCE is the leading candidate because of mature VST3/GUI/audio abstractions, but Phase P1 must verify:
- exact license compatibility with the repository's GPL-3.0 distribution model,
- VST3 SDK integration requirements,
- build reproducibility,
- pluginval/host compatibility,
- no framework behavior that conflicts with realtime or state requirements.

The architecture intentionally keeps framework dependencies at the edge so another framework can be adopted if needed.
