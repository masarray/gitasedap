# AGENTS.md — GitaSedap Engineering Execution Contract

This file defines the mandatory working rules for coding agents, AI assistants, automated contributors, and human contributors working on **GitaSedap**.

The purpose is simple:

> **Progress must move forward in a controlled, measurable, non-regressive way.**
>
> GitaSedap must not be built with naive coding, random trial-and-error, patch stacking, or "make it work first, clean it later" engineering.

This repository is a real-time audio product. Architecture, realtime safety, memory behavior, CPU behavior, state compatibility, DSP correctness, and regression control are product requirements, not cleanup tasks.

The rules below are mandatory unless an explicit ADR or maintainer decision documents why an exception is necessary.

---

## 1. Core engineering doctrine

Every change must follow this order:

```text
Understand
  ↓
Measure / establish baseline
  ↓
Design
  ↓
Define invariants and acceptance criteria
  ↓
Implement smallest correct slice
  ↓
Test
  ↓
Benchmark when performance-relevant
  ↓
Review for realtime / memory / state / lifecycle safety
  ↓
Integrate
  ↓
Record the new verified baseline
```

Never use this order:

```text
Guess
  ↓
Patch
  ↓
Patch the patch
  ↓
Add special case
  ↓
Break old behavior
  ↓
Rewrite blindly
```

The project must be optimized **from architecture onward**, but optimization must be evidence-based.

This means:

- choose efficient data flow from the beginning,
- avoid architectural debt that is known to cause latency, leaks, contention, or duplication,
- preallocate realtime resources,
- design ownership and lifetime before adding concurrency,
- canonicalize state before building features around it,
- coalesce duplicate expensive control-plane work,
- use bounded workers instead of thread proliferation,
- use cache-friendly layouts where hot-path evidence justifies them,
- profile before micro-optimizing,
- never sacrifice correctness or maintainability for speculative cleverness.

**"No premature micro-optimization" does not mean "build naive code first."**

We reject both extremes:

1. naive architecture with a promise to optimize later, and
2. unmeasured low-level cleverness that makes code fragile.

The target is **professional performance-aware architecture plus measured hot-path optimization**.

---

## 2. Documents that must be read before coding

Before modifying production code, agents must read the relevant project documents:

1. `README.md`
2. `docs/PROJECT_HANDOFF.md`
3. `docs/MULTI_THREAD_ORCHESTRATION.md`
4. `docs/PRD.md`
5. `docs/ARCHITECTURE.md`
6. `docs/IMPLEMENTATION_PHASES.md`
7. `docs/ENGINEERING_STANDARDS.md`
8. `docs/QUALITY_GATES.md`
9. relevant ADRs, when present
10. relevant tests and benchmarks for the subsystem being changed

Do not begin implementation from an isolated issue description while ignoring the product architecture.

If two documents conflict:

- do not silently choose one,
- identify the conflict,
- prefer the more specific/recent accepted architecture decision,
- update documentation if the intended behavior changes.

---

## 3. Phase discipline

Development follows the phase plan in `docs/IMPLEMENTATION_PHASES.md`.

Agents must not jump ahead simply because a later feature is interesting.

Example:

- do not build production GUI before the DSP/state foundation is stable,
- do not add custom profile training before realtime publication/lifetime rules exist,
- do not add vocal/harmony processing while V1 acoustic-body acceptance criteria remain unmet.

A phase is complete only when its **exit criteria** pass.

"Code exists" is not completion.

---

## 4. No naive coding

Naive implementations are prohibited when the problem already has a known production-grade pattern.

Examples of prohibited patterns include:

- allocating per audio block,
- using a mutex in the audio callback,
- spawning a new thread for each request,
- detached threads,
- parsing profile files in `processBlock()`,
- rebuilding FIR/modal coefficients synchronously in the callback,
- unbounded queues,
- unbounded caches,
- repeated asset decoding in GUI paint,
- duplicate state models for UI/DSP/presets,
- using display names as persistent identifiers,
- blindly copying large buffers in hot paths,
- rebuilding identical derived data repeatedly,
- polling when event/latest-value transfer is sufficient,
- recursive retry without explicit bound,
- broad `shared_ptr` usage without realtime destruction analysis,
- writing ad-hoc special cases instead of fixing the canonical model,
- adding fallback behavior that hides a broken invariant.

When a production pattern exists, use it deliberately and document why it fits.

---

## 5. No random trial-and-error

Unstructured trial-and-error is prohibited.

Experimentation is allowed and often necessary in DSP, but it must be **hypothesis-driven**.

Every non-trivial experiment should have:

- a question,
- a hypothesis,
- a baseline,
- a controlled change,
- a measurement or listening criterion,
- a result,
- a decision: keep / reject / refine.

Bad:

> "Try changing several filters until it sounds better."

Good:

> "Hypothesis: hard-strum piezo harshness is dominated by transient-dependent 2–4 kHz energy. Measure fast-vs-slow envelope correlation, apply bounded dynamic reduction only during high transient score, then compare level-matched renders."

Do not leave abandoned experiments, dead code paths, unused flags, or half-integrated alternate engines in production code.

---

## 6. Preserve the last verified working baseline

Before changing an established subsystem:

1. identify the current verified baseline,
2. identify tests/fixtures/benchmarks that prove it,
3. record expected behavior,
4. make the smallest coherent change,
5. verify that old acceptance criteria still pass.

Never replace a known-good path with an older, partially working, or unverified implementation.

If a regression appears:

- stop feature expansion,
- isolate the first failing change,
- fix or revert,
- restore the verified baseline before proceeding.

Forward progress is more important than accumulating commits.

---

## 7. Canonical source of truth

There must be one canonical representation for each concept.

Examples:

- one canonical parameter ID,
- one canonical profile ID,
- one canonical preset/state schema,
- one canonical version source,
- one canonical profile metadata model,
- one authoritative prepared DSP state per generation.

Do not create parallel representations that drift.

Bad:

```text
UI bodyType string
DSP body index
preset body label
profile filename
special-case enum
```

Good:

```text
Canonical BodyProfileId
        │
        ├── UI display mapping
        ├── persisted preset reference
        └── compiled runtime state
```

Display labels are not persistent identity.

Derived values are not canonical state when they can be regenerated safely.

---

## 8. Realtime audio thread contract

Anything reachable from the audio callback is treated as realtime-critical.

### Forbidden on the audio thread

- heap allocation or deallocation,
- container growth,
- blocking locks,
- condition variables,
- sleeping,
- file I/O,
- network I/O,
- synchronous logging,
- GUI calls,
- profile parsing,
- expensive lazy initialization,
- blocking worker coordination,
- unbounded loops,
- heavyweight object destruction,
- operations whose worst-case duration is unknown.

### Required

- preallocated buffers,
- bounded work,
- stable memory,
- finite validated coefficients,
- denormal protection,
- click-safe state transitions,
- deterministic reset behavior,
- no exception escaping the callback.

If a proposed feature cannot meet these rules, redesign the feature.

---

## 9. Realtime state publication

Complex DSP changes use immutable prepared state.

Preferred model:

```text
Request
  ↓
Worker prepares complete candidate
  ↓
Validate candidate
  ↓
Mark with generation
  ↓
Publish
  ↓
Audio thread observes at block boundary
  ↓
Crossfade if required
  ↓
Old state reclaimed outside audio callback
```

Never expose partially prepared mutable state to the callback.

Never destroy a heavyweight old state in the callback.

Use generation numbers so an older worker result can never overwrite a newer request.

---

## 10. Worker architecture

Use a small bounded worker pool for non-realtime heavy tasks.

Suitable tasks include:

- profile decoding,
- FIR preparation,
- modal fitting,
- minimum-phase conversion,
- resampling,
- preset/profile indexing,
- custom training analysis,
- expensive asset preparation,
- offline statistics.

Workers must provide:

- bounded queue,
- cancellation,
- generation/stale-result checks,
- deterministic shutdown,
- explicit ownership,
- structured error reporting.

Forbidden:

- detached threads,
- one thread per profile click,
- unbounded task accumulation,
- workers mutating audio-thread-owned objects,
- callbacks into destroyed plugin/editor objects.

Default to the minimum number of workers that provides measured benefit.

More threads are not automatically faster.

---

## 11. Coalescing policy

Duplicate expensive control-plane requests should be coalesced.

Examples:

- rapidly changing selected profile,
- repeated preset preview,
- repeated GUI resize/repaint requests,
- analysis requests where only the newest result matters,
- redundant profile compilation.

Use a monotonically increasing generation/token.

Only the newest relevant result may be published.

However:

**Do not coalesce away musically meaningful host automation events.**

Realtime automation follows host/framework semantics and appropriate smoothing.

Coalescing is mainly for expensive control-plane work, not an excuse to lose musical timing.

---

## 12. Memory management and leak prevention

Memory correctness is designed, not patched later.

### Ownership rules

Prefer:

1. value semantics,
2. `std::unique_ptr`,
3. explicit non-owning references/views,
4. shared ownership only when justified.

Do not use `std::shared_ptr` casually in realtime state handoff.

The final reference decrement can trigger destruction at the wrong time.

### Allocation rules

- allocate audio buffers during prepare/setup,
- reserve predictable control-plane storage,
- bound all caches,
- bound all queues,
- avoid repeated temporary allocations in frequently called paths,
- reuse scratch buffers when lifetime and thread ownership are clear.

### Lifecycle rules

Every asynchronous operation must be safe when:

- plugin instance is destroyed,
- editor closes,
- host changes sample rate,
- project closes,
- worker is cancelled,
- preset/profile is replaced.

"Process exit releases memory" is not acceptable leak management.

---

## 13. Cache policy

Caching is allowed only with explicit:

- key,
- ownership,
- invalidation rule,
- size bound,
- thread-access rule,
- destruction location.

Prefer canonical IDs/content hashes as keys.

Never create an invisible unbounded cache.

Never perform expensive cache eviction/destruction on the realtime thread.

Do not cache data that is cheap enough to recompute unless measurement supports the cache.

---

## 14. Data layout and CPU efficiency

Hot-path data structures must be designed for predictable access.

Guidelines:

- prefer contiguous storage,
- minimize pointer chasing,
- avoid per-sample virtual dispatch in critical loops,
- avoid repeated format conversion,
- avoid repeated branch-heavy work when it can be prepared,
- separate hot data from cold metadata,
- consider structure-of-arrays for modal/filter banks,
- align/vectorize only when the selected SIMD path benefits.

Optimization order:

```text
Correct algorithm
  ↓
Eliminate unnecessary work
  ↓
Eliminate unnecessary allocation/copy
  ↓
Improve locality
  ↓
Batch/coalesce
  ↓
SIMD/vectorization
  ↓
Micro-optimize measured hotspots
```

Do not hand-vectorize code that is not a measured hotspot.

---

## 15. DSP engineering rules

DSP must be testable outside the plugin host.

Every major DSP module should support deterministic offline testing.

For relevant DSP changes, test:

- silence,
- impulse,
- DC,
- sine/sweep,
- noise,
- representative guitar fixtures,
- supported sample rates,
- multiple block sizes,
- reset,
- extreme legal parameter values,
- rapid parameter changes,
- NaN/Inf safety.

Musical tuning must use level-matched comparisons.

A louder output is not automatically a better output.

---

## 16. Acoustic body engine constraints

The intended architecture is hybrid:

```text
Source Adapter
  ↓
Dynamic Anti-Quack
  ↓
Short Causal Transfer FIR
  ↓
Dynamic Modal Body Bank
  ↓
Body Space
  ↓
Phase-Aware Morph
  ↓
Natural Polish
  ↓
Air
  ↓
Feedback Guard
  ↓
Output
```

Do not replace this with a naive long-IR-only design without an ADR and measured evidence.

Key principles:

- body resonance is not room reverb,
- Live mode is causal/zero-lookahead,
- narrow low modes should be handled efficiently,
- dynamic playing response matters,
- dry/body mixing must consider phase,
- Air must not simply restore harsh piezo treble,
- Polish must not destroy transient articulation.

---

## 17. FIR/convolution policy

Do not assume FFT convolution is automatically faster.

Benchmark by:

- FIR length,
- block size,
- sample rate,
- target CPU.

Short kernels may be faster with direct SIMD convolution.

Longer kernels may use a direct low-latency head plus partitioned tail.

Keep backend choice behind a stable abstraction.

Never force the product architecture to match one optimization backend.

---

## 18. Parameter and automation rules

Stable public parameter IDs must never be casually renamed after release.

For continuous parameters:

- use bounded smoothing where needed,
- preserve host automation semantics,
- avoid zipper noise.

For structural parameters:

- prepare off-thread,
- publish atomically,
- crossfade when needed.

Do not interpolate discrete states through invalid intermediate configurations.

---

## 19. GUI performance rules

The GUI must not become a hidden cause of audio instability.

Forbidden in paint/render hot paths:

- asset decoding,
- file parsing,
- expensive profile compilation,
- repeated geometry recreation that can be cached.

Required:

- bounded meter refresh,
- dirty-region/component repaint,
- coalesced high-rate UI requests,
- cached vector/raster assets where useful,
- no excessive polling,
- safe editor destruction,
- HiDPI-aware scalable assets.

The visual design should remain compact and professional rather than large-card/bulky UI.

---

## 20. Error handling

Do not hide architecture bugs behind silent fallbacks.

Control-plane failures should produce structured errors.

Audio-plane failure should:

- preserve the last valid state when possible,
- bypass only the affected subsystem if safe,
- never block,
- never throw into the host.

Malformed external preset/profile data is untrusted input.

Validate:

- size limits,
- numeric finiteness,
- indices,
- Q/frequency bounds,
- FIR length,
- checksum/version,
- stable filter poles.

---

## 21. Regression prevention

Every bug fix should add a regression test whenever technically feasible.

Every optimization must demonstrate that it preserves correctness.

Every behavior-changing PR must state:

- previous verified behavior,
- intended new behavior,
- tests protecting unchanged behavior,
- performance impact,
- state compatibility impact.

Do not delete a test merely because new code fails it unless the product requirement itself has intentionally changed.

If the requirement changed, update the test and documentation together.

---

## 22. Benchmark discipline

Performance claims require measurements.

Record enough context to reproduce:

- CPU model,
- OS,
- compiler/build type,
- sample rate,
- block size,
- number of plugin instances,
- editor open/closed,
- selected profile/mode.

Track at least:

- mean callback time,
- p95,
- p99,
- p99.9 or diagnostic max,
- xruns/underruns,
- peak memory where relevant.

A lower average with worse p99 spikes may be a regression for realtime audio.

---

## 23. Optimization must not create hidden complexity debt

An optimization is acceptable only if it has a clear benefit/cost ratio.

Before adding complexity, ask:

- What measured bottleneck does this solve?
- What is the expected gain?
- How will it be tested?
- Does it affect determinism?
- Does it increase lifetime/concurrency risk?
- Can it be isolated behind an interface?
- Can it be removed later without rewriting the product model?

Prefer architectural optimizations with broad benefit over fragile micro-tricks.

---

## 24. Small coherent PRs

Do not combine unrelated concerns into giant PRs.

A strong PR should have one clear purpose.

Good separation:

- build scaffold,
- realtime handoff infrastructure,
- source adapter,
- modal bank,
- profile compiler,
- GUI vector control,
- benchmark optimization.

Avoid mixing:

- DSP rewrite,
- GUI redesign,
- state migration,
- worker system,
- release packaging

in one change unless they are inseparable.

Small does not mean incomplete. Each PR must leave the repository in a valid state.

---

## 25. Before coding checklist

Before editing production code, answer:

1. What user/product requirement is being satisfied?
2. Which implementation phase owns this work?
3. What is the current verified baseline?
4. What subsystem is authoritative?
5. What invariants must remain true?
6. Does the change touch realtime code?
7. Does it create/transfer ownership?
8. Does it introduce async work?
9. Can duplicate work be coalesced?
10. Can expensive work be prepared ahead of time?
11. What is the worst-case memory behavior?
12. What is the worst-case execution behavior?
13. What tests prove correctness?
14. What benchmark proves acceptable performance?
15. What could regress?

If these questions cannot be answered, more design/research is needed before implementation.

---

## 26. During coding checklist

While implementing:

- keep changes scoped,
- do not duplicate existing logic,
- preserve canonical IDs/schema,
- avoid hidden allocation,
- avoid hidden copies,
- keep ownership obvious,
- keep worker/audio/UI boundaries explicit,
- remove temporary debug hacks before merge,
- keep comments focused on **why**, not restating obvious code,
- add assertions for invariants in debug builds,
- keep unsupported states impossible where practical.

If implementation starts requiring many exceptions, stop and reassess the design instead of stacking special cases.

---

## 27. After coding checklist

Before calling work complete:

1. build cleanly,
2. run relevant unit tests,
3. run DSP regression tests,
4. run sanitizer/diagnostic checks when relevant,
5. run plugin validation when plugin boundary changed,
6. run CPU benchmark when hot path changed,
7. verify no new callback allocations/locks,
8. verify lifecycle shutdown/cancellation,
9. verify state/preset compatibility,
10. inspect diff for accidental unrelated changes,
11. update documentation,
12. record next phase/known limitations.

Do not report success solely because compilation passed.

---

## 28. Definition of "optimized from the start"

For this project, "optimized from the start" means:

### Architecture-level optimization
- correct thread boundaries,
- canonical state,
- prepared immutable DSP states,
- bounded queues/caches,
- coalescing,
- no duplicated expensive work,
- preallocation,
- deterministic lifetime.

### Algorithm-level optimization
- choose an algorithm appropriate to the signal problem,
- modal filters for narrow resonances when appropriate,
- short FIR where long FIR is unnecessary,
- offline preparation for expensive transforms,
- no needless oversampling.

### Data-level optimization
- compact canonical data,
- derived runtime coefficients,
- contiguous hot data,
- no redundant copies.

### Execution-level optimization
- realtime bounded work,
- measured SIMD,
- cache-aware loops,
- backend chosen by benchmark.

### Product-level optimization
- do not implement features outside the active milestone,
- do not maintain multiple competing engines unnecessarily,
- do not spend CPU on inaudible complexity,
- do not spend engineering time on features that are not acceptance blockers.

---

## 29. What "professional" means here

Professional does **not** mean maximum abstraction, maximum template usage, or maximum cleverness.

Professional means:

- predictable,
- understandable,
- measurable,
- maintainable,
- testable,
- bounded,
- reproducible,
- reversible when necessary,
- safe under host lifecycle stress,
- fast enough with margin,
- difficult to regress accidentally.

Prefer boring reliable infrastructure around sophisticated DSP.

---

## 30. Decision hierarchy

When tradeoffs exist, prefer in this order:

1. correctness,
2. realtime safety,
3. audio quality,
4. deterministic behavior,
5. regression safety,
6. lifecycle/memory safety,
7. performance margin,
8. maintainability,
9. development speed,
10. convenience.

Development speed matters, but shortcuts that create regression loops are slower overall.

The fastest path is the one that does not need to be rebuilt repeatedly.

---

## 31. Stop conditions

An agent must stop expanding scope and investigate when any of these occur:

- previously passing tests regress,
- unexplained CPU spike appears,
- memory grows without bound,
- a worker result can race with destruction,
- audio callback begins allocating,
- parameter/state compatibility becomes ambiguous,
- two sources of truth emerge,
- implementation needs repeated special-case patches,
- output contains NaN/Inf,
- profile switching clicks/crashes,
- latest change cannot be explained causally,
- benchmarks cannot reproduce claimed performance.

Do not continue adding features on top of a broken foundation.

---

## 32. Allowed prototyping

Prototype code is allowed only when clearly isolated.

A prototype must be one of:

- offline experiment,
- benchmark target,
- throwaway research branch,
- test harness.

Prototype code must not silently become production architecture.

Before promotion to production:

- redesign against project standards,
- remove shortcuts,
- add tests,
- define ownership,
- define bounds,
- benchmark,
- document the decision.

---

## 33. Dependency policy

Before adding a dependency, document:

- problem it solves,
- why existing code/framework cannot solve it cleanly,
- size/build impact,
- runtime impact,
- license,
- maintenance activity,
- security/update implications.

Do not add a large dependency for a small utility.

All release dependencies must be pinned.

---

## 34. State migration policy

Once public releases exist:

- released parameter IDs are stable contracts,
- preset/profile schema changes require versioning,
- migration paths are tested,
- old sessions should not silently load into a materially different sound,
- incompatible changes require explicit version policy.

Never "fix" state by simply deleting compatibility code without a release decision.

---

## 35. Commit/PR quality

Commit messages should describe intent, not activity.

Good:

`dsp: add bounded modal-body state preparation`

Bad:

`update stuff`

PR description should include:

- problem,
- design,
- scope,
- tests,
- performance impact,
- realtime impact,
- state compatibility,
- remaining work.

---

## 36. Agent communication standard

When reporting progress, distinguish clearly:

- **implemented**,
- **verified**,
- **measured**,
- **planned**,
- **not yet tested**,
- **blocked**.

Never call something "fixed" if it has only been edited but not validated.

Never claim performance improvement without a benchmark.

Never claim regression-free behavior without relevant tests.

---

## 37. Final rule

The repository must become **simpler to reason about as it grows**, not harder.

Every new layer must justify its existence.

Every optimization must preserve clarity.

Every feature must integrate with the canonical architecture.

Every phase must leave a verified baseline for the next phase.

> **Move fast by removing uncertainty, not by skipping engineering.**
>
> **Build once, verify, then advance.**


---

## 38. Multi-thread and long-gap continuation contract

GitaSedap may be developed by multiple ChatGPT threads or contributors at the same time.

Parallel work is permitted only when ownership and dependencies are explicit.

Before starting a new workstream:

1. read `docs/PROJECT_HANDOFF.md`,
2. read `docs/MULTI_THREAD_ORCHESTRATION.md`,
3. continue the existing milestone issue rather than creating a duplicate,
4. resolve the exact baseline branch and SHA,
5. inspect existing workstream claims/comments,
6. declare the files/authority expected to change,
7. branch from the documented canonical baseline.

A thread must not assume `main` is current.

### One authority, many consumers

Two concurrent threads may consume the same canonical interface.

They must not independently create competing implementations for:
- parameter identity,
- serialized state,
- body profile identity,
- body engine,
- profile publisher,
- prepared-state exchange,
- worker pool,
- preset/profile cache,
- production UI state.

If concurrent work needs the same authoritative file, serialize it or agree on a producer/consumer boundary first.

### Workstream claim

The owning issue should contain a durable claim with:
- base branch/SHA,
- planned branch,
- intended authority/files,
- interfaces consumed/produced,
- overlap risk.

This is not bureaucracy; it prevents two isolated chat threads from solving the same problem differently.

### Integration discipline

Dependency order beats PR creation time.

Never merge a later stacked PR before its semantic base merely because its CI is green.

Never reconstruct an integration baseline by copying/cherry-picking random commits from old threads.

### Required thread handoff

Before a thread ends, record on the owning issue/PR:
- branch,
- base SHA,
- head SHA,
- PR,
- CI run,
- artifacts,
- implemented facts,
- verified facts,
- measured facts,
- untested/blocked work,
- state/realtime/memory impact,
- exact next action.

The repository must remain understandable without access to the original chat history.

### Long inactivity

After a long pause:
- reproduce the last verified baseline first,
- do not immediately upgrade dependencies or redesign architecture,
- verify branch/PR status,
- then resume the owning milestone from evidence.

A stale conversation is not a reason to restart the product.
