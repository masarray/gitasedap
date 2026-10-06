# GitaSedap Project Handoff

**Handoff date:** 2026-10-06  
**Purpose:** canonical continuation point for new ChatGPT threads, human contributors, and coding agents.

This document is the first file to read when resuming the project after a long gap.

---

## 1. Product mission

GitaSedap is a Windows VST3 acoustic-guitar processor whose V1 goal is to turn direct piezo pickup sound into a more natural, dimensional, responsive, polished acoustic tone while remaining light and predictable enough for live use.

The intended path is:

```text
Input
  -> Source Adapter / anti-quack
  -> short causal transfer FIR
  -> modal acoustic body
  -> dynamic body / body space
  -> phase-aware body morph
  -> Natural Polish
  -> Air / harshness protection
  -> feedback/live safety
  -> output protection
```

Do not reduce the architecture to "piezo + EQ" or "piezo + reverb".

---

## 2. Current canonical engineering baseline

The cumulative verified development head is:

- branch: `p4d/real-guitar-calibration`
- commit: `5421fdc9e2cc1fce73229e3f42f62c17d295e6eb`
- PR: #10
- final cumulative CI run: **37001612039**

That run verified:

- Windows Release build,
- **19/19 CTests**,
- P2/P3/P4A/P4B/P4C/P4D engineering evidence,
- Steinberg validator default mode: **47/47**,
- Steinberg validator local-instance mode: **47/47**,
- zero GitaSedap project/test/lab compiler warnings.

Important distinction:

> **P4D tooling is verified. P4D tonal calibration is still open.**

Natural Development and Dreadnought Development must not be called finished factory tones until real simultaneous piezo/reference-microphone material has been analyzed and accepted.

---

## 3. Current PR stack

The project was intentionally built as a stacked chain. At this handoff all PRs below are still open and must be integrated in dependency order.

| PR | Scope | Base |
|---|---|---|
| #1 | Foundation / product architecture | `main` |
| #2 | P1A toolchain/plugin shell | PR #1 branch |
| #3 | P1B state/lifecycle/validator | PR #2 branch |
| #4 | P1C generation-safe runtime + bounded workers | PR #3 branch |
| #5 | P2 DSP laboratory | PR #4 branch |
| #6 | P3 Source Adapter / dynamic anti-quack | PR #5 branch |
| #7 | P4A Hybrid Body core | PR #6 branch |
| #8 | P4B generation-safe body profile publication | PR #7 branch |
| #9 | P4C level-matched profile A/B + state v2 | PR #8 branch |
| #10 | P4D real-guitar calibration tooling | PR #9 branch |

**Do not build new production work from old `main` while this stack is still unresolved.**

Issue #11 owns collapsing the verified stack into one canonical `main`.

---

## 4. Framework and dependency decisions already made

Production plugin framework is **iPlug2**, not JUCE.

Pinned dependency contracts:

- iPlug2: `d54f69050f517e43b941d88c2a170f0a840b9ee4`
- Steinberg VST3 SDK: `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`

The pins live in `cmake/Dependencies.cmake`.

Do not replace the framework or float dependencies to branches/tags without an ADR, build-size/license review, migration strategy, and proof that the change is necessary.

---

## 5. What is implemented now

### Foundation / infrastructure

- C++20 + CMake.
- Pinned reproducible dependencies.
- Windows VST3 build.
- Official Steinberg validator CI.
- Versioned canonical state.
- State v1 -> v2 migration.
- Generation-safe prepared-state exchange.
- Bounded worker/coalescing infrastructure.
- Deterministic shutdown/reclamation rules.
- Offline DSP laboratory.
- WAV read/write, fixtures, golden comparison, finite-audio checks.
- Deadline benchmarking.

### P3 source conditioning

- Active Piezo / Passive Piezo / Magnetic source model.
- Source conditioning.
- Transient-selective anti-quack path.
- Smoothing/click-safe foundation.

### P4 body engine

- bounded short causal FIR transfer layer,
- bounded modal body bank,
- canonical Body Profile schema,
- profile compiler,
- deterministic content hashing,
- prepared sample-rate-specific profiles,
- HybridBodyEngine,
- safe dual-engine profile crossfade,
- generation-aware profile publication,
- Raw/P3 identity comparison profile,
- Natural Development profile,
- Dreadnought Development profile,
- A/B comparison slots,
- engineering level-match gates,
- state v2 for A/B profile selection.

### P4D calibration tooling

- simultaneous piezo/reference quality analysis,
- bounded lag/alignment search,
- 48-band logarithmic spectral transfer measurement,
- broad gain normalization,
- body/quack/air regional summaries,
- modal frequency/prominence/Q candidates,
- CSV reports,
- pair/stereo capture workflows,
- Raw/Natural/Dread profile evaluation CLI.

---

## 6. Current host-facing parameter identity

Parameter IDs are canonical and append-only:

| ID | Meaning |
|---:|---|
| 0 | Body |
| 1 | Air |
| 2 | Enhance |
| 3 | Output |
| 4 | Bypass |
| 5 | Input Source |
| 6 | Body Profile A |
| 7 | Body Profile B |
| 8 | Body Compare Slot / Listen A-B |

Do not reorder or repurpose these IDs.

A future public parameter must append a new ID and must include state compatibility analysis.

---

## 7. Canonical authority map

Do not create parallel authorities.

| Concept | Existing authority |
|---|---|
| Public parameter identity | `src/core/ParameterSpec.h` |
| Serialized state/version migration | `src/core/StateSchema.h`, `StateMigration.h` |
| Body profile semantic definition | `src/dsp/BodyProfile.*` |
| Runtime body profile compilation | `src/dsp/BodyProfileCompiler.*` |
| Static hybrid body processing | `src/dsp/HybridBodyEngine.*` |
| Click-safe body profile transition | `src/dsp/CrossfadingBodyEngine.*` |
| Structural body publication | `src/runtime/BodyProfileRuntime.*` |
| General prepared-state handoff | `src/runtime/PreparedStateExchange.h` |
| Bounded async work | existing runtime bounded worker/coalescing components |
| Calibration/evidence | `src/lab/*`, `tools/dsp-lab` |
| Plugin/framework adapter and current editor | `src/plugin/GitaSedap.*` |

If a new milestone needs functionality near one of these responsibilities, extend the authority or add a narrowly scoped collaborator. Do not invent another engine/state/cache/worker authority.

---

## 8. Real-time invariants that already exist

Anything reachable from the audio callback must remain:

- allocation-free/deallocation-free,
- blocking-lock-free,
- file/network/logging-free,
- bounded,
- finite,
- deterministic under reset/lifecycle,
- free from heavyweight destruction,
- free from profile parsing/coefficient design,
- safe under rapid automation/profile switching.

Structural work is prepared off-thread/control-plane and published at block boundaries.

Old generations are reclaimed outside the callback.

Do not weaken these rules to speed up a milestone.

---

## 9. Immediate unfinished gate: P4D tonal calibration

Tracking: **Issue #12**.

Required next evidence is a simultaneous real-guitar performance:

Preferred:
- stereo WAV,
- channel 1 = piezo,
- channel 2 = condenser/reference microphone,
- 48 kHz / 24-bit or 32-bit float,
- 30–60 seconds,
- no EQ/compression/limiter/AGC/reverb/mastering,
- no clipping.

See `docs/P4D_CAPTURE_PROTOCOL.md`.

The correct process is:

```text
capture
 -> quality/alignment gate
 -> RAW/P3 vs mic
 -> Natural vs mic
 -> Dread vs mic
 -> measured hypothesis
 -> one controlled coefficient revision
 -> regression
 -> level-matched A/B
 -> keep/reject/refine
```

Do not tune several filters/modes at once by ear and call the result calibrated.

---

## 10. Remaining large milestones

The following issues are the canonical future work items:

| Issue | Milestone | Integration dependency |
|---|---|---|
| #11 | Collapse verified stacked PRs into `main` | immediate coordination gate |
| #12 | P4D tonal real-guitar calibration | real capture required |
| #13 | P5 Dynamic Body + Body Space + phase-aware morph | final tuning depends #12 |
| #14 | P6 Natural Polish + Air + live safety | depends P5 |
| #15 | P7 Preset/Profile platform + custom training | control-plane portions can parallelize |
| #16 | P8 Production GUI/UX | final controls depend P6/P7 contract |
| #17 | P9 Performance/reliability hardening | after feature set stabilizes |
| #18 | P10 Host certification/beta/release | final release gate |

Do not create replacement milestone issues unless these are obsolete or explicitly superseded.

---

## 11. Safe parallel work

See `docs/MULTI_THREAD_ORCHESTRATION.md` for full rules.

High-level:

- #11 integration is the first repository hygiene priority.
- #12 P4D tonal waits on/uses real capture.
- #13 P5 may develop isolated interfaces/tests while #12 waits, but accepted factory-profile-dependent tuning must follow #12.
- #15 P7 offline/control-plane tooling can progress substantially in parallel with P5/P6 if it reuses existing worker/publication authority and does not mutate live DSP.
- #16 P8 component/visual infrastructure can prototype in parallel, but final production control contract follows P6/P7.
- #17 and #18 are intentionally late gates, not places to hide unfinished feature work.

---

## 12. Branch/base rule for future threads

### Before issue #11 is complete

For isolated continuation work, use the P4D cumulative verified baseline:

```text
p4d/real-guitar-calibration
5421fdc9e2cc1fce73229e3f42f62c17d295e6eb
```

Do not branch from old `main`.

Do not merge new production work ahead of unresolved stack dependencies unless the issue explicitly documents why integration is safe.

### After issue #11 is complete

The final integrated `main` SHA recorded in this document becomes the only normal production base.

Every future thread must inspect this document and the target issue before branching.

---

## 13. Thread startup protocol

Every new coding thread should do this before changing code:

1. Read `AGENTS.md`.
2. Read this handoff.
3. Read `docs/MULTI_THREAD_ORCHESTRATION.md`.
4. Read the target milestone issue completely.
5. Inspect current issue/PR comments for an active workstream claim.
6. Resolve the canonical base SHA.
7. State which subsystem/files the thread intends to own.
8. Inspect existing implementation/tests before proposing replacements.
9. Define acceptance criteria and regression risk.
10. Only then create/continue the branch.

If the intended files overlap another active workstream's authoritative files, coordinate first instead of racing two implementations.

---

## 14. Required end-of-thread handoff

A thread is not complete until it leaves a durable issue/PR handoff containing:

```text
Milestone / issue:
Branch:
Baseline SHA:
Head SHA:
PR:
CI run:
Artifacts:

Implemented:
Verified:
Measured:
Not yet verified:
Open risks/blockers:
Canonical authority changed?:
State compatibility:
Realtime impact:
Memory/lifecycle impact:
Exact next action:
```

Use the words **implemented**, **verified**, **measured**, **planned**, **blocked**, and **not tested** precisely.

---

## 15. Things the next thread must not redo

- Do not restart the project with JUCE.
- Do not make another state schema/model beside the canonical core state.
- Do not make another body engine to avoid understanding P4.
- Do not make another worker pool for P7.
- Do not replace profile switching with shared_ptr swapping in the callback.
- Do not use display text as persistent profile identity.
- Do not turn body resonance into room reverb.
- Do not add long linear-phase/live latency casually.
- Do not add neural inference to the V1 live path.
- Do not tune tonal profiles from synthetic fixtures and claim real-guitar proof.
- Do not optimize by intuition when a benchmark can answer the question.
- Do not silently change host parameter IDs.
- Do not continue feature work on top of a failing regression baseline.

---

## 16. Definition of success from here

The project is finished when it reaches P10 with:

- believable, accepted real-guitar body tone,
- responsive dynamic body behavior,
- polished/air/open sound without piezo harshness,
- stable live behavior,
- canonical profile/preset system,
- premium compact production GUI,
- zero known leaks/races/realtime violations,
- documented CPU/deadline margin,
- host certification,
- reproducible tagged V1 artifact.

Until then, every milestone should leave a stronger verified baseline rather than more branches that require archaeology.
