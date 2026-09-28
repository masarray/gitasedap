# GitaSedap Product Requirements Document

**Status:** Foundation v1  
**Primary product:** Real-time acoustic guitar body/enhancement VST  
**Initial platform:** Windows VST3  
**License:** GPL-3.0

## 1. Vision

GitaSedap should make a typical acoustic-guitar piezo pickup feel closer to a well-recorded acoustic instrument: woody, dimensional, balanced, open, responsive, and polished, without turning the sound into an obvious effect and without imposing studio-class latency or CPU cost.

The product should be comfortable for live singing/guitar performance first, while still sounding refined enough for recording.

## 2. User problem

Typical piezo pickup output can be:

- hard or "quacky" on transients,
- thin or plastic in the upper mids,
- weak in realistic soundboard/body resonance,
- overly direct compared with a microphone,
- difficult to polish without many EQ/dynamics controls,
- prone to feedback when body resonance is added indiscriminately.

The user should not need to become a DSP engineer to solve those problems.

## 3. Product principles

1. **Natural before spectacular.** Preserve the identity and articulation of the guitar.
2. **Body is not reverb.** Acoustic-body reconstruction is treated as a short transfer/resonance problem, not as a room effect.
3. **Dynamic, not static.** Hard strumming, gentle picking, and fingerstyle must not receive the same correction.
4. **Live-first real-time behavior.** No lookahead in the main performance path.
5. **Small front panel, deep engine.** The default UI exposes a few musical controls; advanced tuning remains optional.
6. **Measured performance.** CPU, callback timing, memory, leaks, and regression behavior are release criteria.
7. **No hidden network dependency.** Core audio processing, presets, and profiles work fully offline.

## 4. Target users

### Primary
- Singer-guitarists using undersaddle piezo pickups.
- Live acoustic players using DAWs/plugin hosts.
- Home-recording users who want a polished acoustic sound from direct pickup.

### Secondary
- Acoustic magnetic-pickup users.
- Engineers creating reusable guitar profiles.
- Developers/researchers who want an open acoustic-body DSP implementation.

## 5. V1 scope

### Required
- Input source mode: Active Piezo / Passive Piezo / Magnetic.
- Dynamic anti-quack/source conditioning.
- Hybrid acoustic body engine.
- Body amount and body model/profile selection.
- Air/openess control with harshness protection.
- Polish/enhance macro.
- Input/output metering.
- Bypass with click-safe transitions.
- Factory profiles.
- Preset/state save and recall.
- Windows VST3.
- Deterministic offline render tests and real-time performance tests.
- Scalable GUI inspired by compact premium vocal/acoustic hardware, without copying trade dress.
- Vector-based metallic knobs derived from legally usable project assets.

### Planned after the core is proven
- Custom pickup+microphone profile training.
- Automatic feedback/notch assistant.
- macOS AU.
- CLAP.
- Vocal enhancement.
- Harmony generation.

## 6. Explicit non-goals for V1

- Guitar amp simulation.
- Long room/hall reverb.
- Full vocal processor.
- Pitch correction.
- Neural-network inference in the live audio path.
- Cloud processing or mandatory login.
- Linear-phase processing in Live mode.
- Exact cloning of proprietary BOSS, TC-Helicon, Fishman, ToneDexter, NUX, or Zoom algorithms.

## 7. Core signal path

```text
Input
  |
  v
Source Adapter / Input Conditioning
  |
  v
Dynamic Anti-Quack
  |
  v
Mic-Transfer / Spectral Body Transform
  |
  v
Dynamic Modal Body Bank
  |
  v
Body Space / Short Phase-Time Character
  |
  v
Phase-Aware Body Morph
  |
  v
Natural Polish
  |
  v
Air / String Openness
  |
  v
Feedback Guard
  |
  v
Output / Protection
```

## 8. Body engine requirements

### 8.1 Mic-transfer layer
- Causal and live-safe.
- Minimum-phase or controlled mixed-phase representation.
- Short FIR by default.
- Profile preparation is performed off the audio thread.
- Backend may choose direct SIMD FIR for short kernels and partitioned convolution for longer kernels.
- No runtime heap allocation during processing.

### 8.2 Modal resonance layer
- Approximately 6-12 learned/factory resonant modes as a starting design range.
- Each mode can encode frequency, gain, Q/damping, excitation sensitivity, and dynamic behavior.
- Narrow low-frequency body modes should not force an excessively long FIR.
- Resonators must be stable under all supported sample rates and parameter transitions.

### 8.3 Dynamic behavior
The engine should react to playing intensity using fast and slow envelopes.

Expected behavior:
- Soft playing: preserve intimacy and string detail.
- Medium playing: body opens naturally.
- Hard strumming: body remains full but damping/anti-boom protection prevents uncontrolled resonance.
- Anti-quack activity increases mainly around offending transients rather than applying a permanent deep EQ scoop.

### 8.4 Phase/time behavior
- Live mode favors immediate, minimum-phase behavior.
- Natural mode may retain a controlled short time spread.
- No audible combing from careless dry/wet blending.
- Any direct/body morph must be phase-aware and click-safe.

## 9. Main user controls

The compact front panel should expose approximately:

- **Body** - overall acoustic-body reconstruction amount.
- **Air** - openness, string detail, and microphone-like top-end.
- **Enhance** - transparent polish/dynamics macro.
- **Body Type/Profile** - Natural, Dreadnought, OM/Concert, Jumbo, Nylon, Studio families as profiles become available.
- **Input Source** - Active Piezo, Passive Piezo, Magnetic.
- **Output** - trim/level.
- **Bypass**.

An advanced panel may later expose:
- resonance/depth/damping,
- anti-quack amount,
- harshness,
- warmth/presence,
- transient,
- compressor behavior,
- body-space,
- feedback controls.

## 10. Operating modes

### Live
- Zero-lookahead.
- Causal/minimum-phase emphasis.
- Strongest CPU and feedback-safety constraints.
- Reported algorithmic latency target: 0 samples unless a host/framework requirement makes that impossible.

### Natural
- Default balance between immediacy and dimensional body response.
- Still zero-lookahead.
- Slightly richer short phase/time character.

### Studio
- May use a longer prepared body tail or richer mixed-phase profile.
- Must remain practical in real-time.
- Any non-zero algorithmic latency must be explicit to the host and user.

## 11. Performance requirements

Performance is measured on documented reference machines; percentages are not compared across unrelated hardware.

### Audio-thread requirements
- No memory allocation/deallocation.
- No mutex, condition variable, sleep, blocking queue, file operation, network call, UI call, or unbounded loop.
- No destruction of heavyweight shared ownership objects.
- No logging from the callback.
- Denormal-safe.
- Bounded work per sample/block.

### Deadline target
At 48 kHz / 64-sample buffers on the project reference Windows machine:
- No xruns in sustained automated tests.
- p99 callback execution should consume less than 25% of the available buffer deadline for one standard mono instance.
- p99.9 is tracked and must not show pathological spikes.
- Performance regression >10% requires review before merge.

### Memory target
- Zero known leaks at release.
- Audio-path buffers are allocated in prepare/setup, not process.
- Profile swaps must not cause transient unbounded memory growth.
- Memory use is benchmarked with editor closed and open.

## 12. State and compatibility requirements

- Stable canonical parameter IDs from the first public release.
- Versioned serialized state schema.
- Forward migration code for older state versions where practical.
- Unknown future fields should not corrupt loading.
- Presets reference profiles through canonical IDs, not UI labels.
- Locale-independent numeric serialization.
- Sample-rate-specific coefficients are derived at load/prepare time; canonical profiles store sample-rate-independent source data whenever practical.

## 13. Factory profile requirements

A factory profile contains:
- canonical profile ID,
- profile version,
- source type,
- descriptive metadata,
- transfer representation,
- modal resonance data,
- dynamic correction curves,
- body-space data,
- tuning constraints,
- checksum.

Factory profiles must load without network access.

## 14. Custom profiling target

A later phase allows simultaneous recording of:
- Channel 1: pickup DI,
- Channel 2: microphone.

Training pipeline:
1. time alignment,
2. frame segmentation,
3. spectral/cross-spectral estimation,
4. coherence/noise rejection,
5. regularized transfer estimation,
6. magnitude smoothing,
7. phase/minimum-phase decomposition,
8. modal fitting,
9. residual FIR construction,
10. validation and profile packaging.

The heavy analysis runs outside the real-time thread.

## 15. UX requirements

- Compact, professional, hardware-inspired front panel.
- No oversized typography or oversized cards.
- Scalable vector UI and HiDPI support.
- Clear signal flow and few primary knobs.
- Parameter reset by double-click/context action.
- Keyboard accessibility where framework support allows.
- Tooltips for non-obvious controls.
- Visible clipping/overload state.
- Advanced controls hidden by default.
- Preset changes and profile swaps are click-safe.

## 16. Quality acceptance criteria for V1

V1 is not considered finished until:

1. Plugin scans and loads in the defined host matrix.
2. Live path passes zero-allocation/real-time-safety review.
3. Sanitizer and leak tests pass.
4. Unit tests and DSP invariant tests pass.
5. Golden audio renders are stable within defined tolerances.
6. Parameter torture tests produce no NaN, Inf, unstable filters, or crashes.
7. Rapid preset/profile switching is click-safe and leak-free.
8. CPU deadline tests meet the reference-machine budget.
9. Listening tests show a repeatable preference over raw piezo for the intended natural/polished target.
10. Documentation, license notices, dependency versions, and release artifacts are complete.

## 17. Success metrics

Technical:
- callback timing budget met,
- zero known leaks,
- no crashes in host stress tests,
- deterministic test renders,
- stable state migration.

Audio:
- reduced piezo quack without killing articulation,
- stronger realistic body impression,
- no obvious "short reverb pasted on DI" character,
- natural behavior across playing intensity,
- useful gain-before-feedback in Live profiles.

UX:
- a new user can reach a convincing tone with Body, Air, and Enhance without opening Advanced controls.

## 18. Release philosophy

GitaSedap uses phase gates rather than feature accumulation. A phase is complete only when its exit criteria are met. New major DSP features should not be layered on top of an unstable real-time foundation.
