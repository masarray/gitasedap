# ADR-0001: Select iPlug2 as the initial plugin framework

- **Status:** Accepted for P1
- **Date:** 2026-09-28
- **Decision scope:** Windows VST3 foundation and future cross-platform plugin wrapper
- **Pinned revision:** `d54f69050f517e43b941d88c2a170f0a840b9ee4`

## Context

GitaSedap needs a plugin framework that supports:

- efficient native C++ DSP,
- Windows VST3,
- compact custom/vector UI,
- HiDPI scaling,
- future CLAP/AU targets,
- CMake-based reproducible builds,
- a licensing model compatible with an open-source GPL-3.0 project,
- keeping the framework at the product boundary so the DSP core remains independently testable.

JUCE was initially considered. The current JUCE 9 distribution is offered under the JUCE commercial licence or AGPLv3. GitaSedap currently declares GPL-3.0 and is intended to remain a free/open-source project, so introducing a different strong-copyleft/commercial framework licence at the foundation would add avoidable licensing and contributor complexity.

iPlug2 is actively maintained, supports VST3/CLAP/AU/AAX and vector-capable IGraphics backends, has current CMake support, and is distributed under a liberal zlib-like licence.

## Decision

Use **iPlug2** for the initial plugin wrapper and UI framework.

The dependency is pinned to one exact commit. Builds may use a developer-local checkout through `GITASEDAP_IPLUG2_DIR`, but CI uses the pinned revision.

The GitaSedap DSP/domain layer must not depend on iPlug2 types. Framework-specific code stays under `src/plugin` and UI/framework adapters.

## Why this fits GitaSedap

1. **Low framework overhead:** suitable for a live-first native audio plugin.
2. **Permissive dependency licence:** avoids forcing a repository relicensing decision.
3. **Vector UI path:** suitable for the planned hardware-inspired scalable interface.
4. **CMake support:** supports deterministic CI and out-of-source builds.
5. **Multiple plugin formats:** VST3 now, with CLAP/AU possible later.
6. **Boundary-friendly:** the DSP core can stay plain C++20 and independently benchmarkable.

## Consequences

Positive:

- P1 can build a native VST3 without coupling DSP code to a large application framework.
- Future GUI work can use IGraphics while preserving a separate engine layer.
- The repository can keep GPL-3.0 while respecting iPlug2 attribution requirements.
- Framework replacement remains possible because core code is framework-independent.

Costs:

- Some application services available in larger frameworks must be built explicitly.
- We must maintain our own canonical state, worker, profiling, and realtime infrastructure rather than hiding those decisions behind framework utilities.
- Production GUI work will require deliberate IGraphics architecture instead of relying on stock generic widgets.

## Guardrails

- No iPlug2 class may leak into the core DSP API.
- Do not put profile parsing, worker management, or business/domain state inside the plugin wrapper.
- Keep framework revision pinned.
- Framework upgrades require a dedicated PR with build, host, CPU, state, and UI regression checks.
- Re-evaluate only with measured technical evidence or a material licensing/platform requirement change.
