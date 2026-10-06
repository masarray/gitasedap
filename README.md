# GitaSedap

GitaSedap is an open-source real-time acoustic-guitar enhancer focused on turning piezo pickup input into a more natural, resonant, polished, microphone-like acoustic tone while staying light enough for live performance.

## Product direction

The first product is a low-latency VST3 acoustic-guitar processor built around a hybrid body engine:

**Source conditioning -> dynamic anti-quack -> mic-transfer FIR -> modal body resonators -> body-space -> phase-aware morph -> polish -> air -> feedback protection -> output**

The core goal is not "piezo + EQ" or "piezo + reverb". The goal is a responsive acoustic-body reconstruction that preserves articulation and feels natural under soft fingerstyle, normal picking, and hard strumming.

## Engineering priorities

- Real-time safe audio thread: no allocation, locks, file I/O, logging, or blocking calls.
- Zero-lookahead Live path and causal processing.
- Immutable prepared DSP state with atomic/block-boundary swaps.
- Coalesced UI/control work without degrading host automation semantics.
- Bounded worker threads for profile analysis, IR preparation, preset loading, and expensive transforms.
- Canonical, versioned preset/profile/state formats with migrations.
- Preallocated memory, RAII ownership, sanitizer coverage, and zero known leaks.
- Measured CPU, callback deadline, memory, startup, and audio-regression budgets.
- Deterministic, pinned dependencies and reproducible CI.

## Documentation

- [Product Requirements](docs/PRD.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Implementation Phases](docs/IMPLEMENTATION_PHASES.md)
- [Project Handoff](docs/PROJECT_HANDOFF.md)
- [Multi-Thread Orchestration](docs/MULTI_THREAD_ORCHESTRATION.md)
- [Engineering Standards](docs/ENGINEERING_STANDARDS.md)
- [Quality Gates](docs/QUALITY_GATES.md)

## Initial platform

- C++20
- CMake
- iPlug2 (pinned immutable revision)
- Steinberg VST3 SDK (pinned immutable revision)
- Windows VST3 first
- 44.1 / 48 / 88.2 / 96 kHz
- Mono guitar input, stereo-capable output

AU and CLAP are planned after the VST3 engine is stable.

## Status

The cumulative engineering branch has progressed through **P4D calibration tooling**.

Verified cumulative gates include the reproducible VST3 toolchain, canonical state/lifecycle infrastructure, bounded workers/prepared-state publication, DSP laboratory, Source Adapter/anti-quack, hybrid body engine, click-safe profile switching, level-matched Raw/Natural/Dread A/B, and real-guitar calibration analysis tooling.

**P4D tonal acceptance is still open** and requires a simultaneous real piezo + reference-microphone capture before the Natural/Dreadnought development profiles may be treated as finished factory tones.

The project currently uses a stacked PR chain. Before resuming work after a gap, read [Project Handoff](docs/PROJECT_HANDOFF.md) and [Multi-Thread Orchestration](docs/MULTI_THREAD_ORCHESTRATION.md) instead of assuming `main` is the latest verified baseline.

## License

GPL-3.0. See [LICENSE](LICENSE).
