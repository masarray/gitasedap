# GitaSedap Engineering Standards

## 1. Language and build

- C++20 baseline.
- CMake is the build source of truth.
- No IDE-only project configuration.
- Dependencies are pinned to exact versions/commits.
- No floating `main`/`master` dependency references in release builds.
- Compiler warnings are treated seriously; selected warnings become errors in CI.
- Build presets document supported configurations.

## 2. Change design

Significant choices use short Architecture Decision Records (ADRs), for example:
- plugin framework,
- parameter/state framework,
- convolution backend,
- profile binary format,
- worker implementation.

An ADR records context, decision, alternatives, and consequences.

## 3. Realtime rule

Code reachable from the audio callback must be assumed hostile to latency until proven otherwise.

Forbidden:
- heap allocation/free,
- locks,
- blocking atomics/spin waits,
- filesystem/network,
- message-thread calls,
- synchronous logging,
- throwing across process boundary,
- unbounded containers/loops,
- lazy initialization.

Realtime-safe helpers should be easy to identify and isolated from general application services.

## 4. Memory and lifetime

### Ownership
- Prefer value semantics and `std::unique_ptr`.
- Shared ownership requires justification.
- Raw pointers may be non-owning views only when lifetime is obvious/documented.
- Every async callback uses a lifetime token or other safe ownership mechanism.

### Allocation
- Allocate DSP buffers in prepare/setup.
- Reserve variable-size control-plane containers before hot loops when sizes are predictable.
- Bound caches and queues.
- Large temporary analysis buffers belong to workers and are released deterministically.

### Leak management
CI/release testing includes:
- repeated plugin construction/destruction,
- repeated editor open/close,
- repeated profile load/swap,
- worker cancel/shutdown,
- sanitizer/diagnostic runs.

"OS cleans it up at exit" is not acceptable.

## 5. Canonicalization

Canonicalization prevents equivalent objects from being loaded/prepared multiple times.

Use:
- canonical parameter IDs,
- canonical profile IDs,
- normalized file paths at the boundary,
- content hashes for profile assets,
- normalized sample-rate-independent profile representation,
- one state schema.

Do not canonicalize by mutable display names.

## 6. Coalescing

Expensive duplicate control requests should collapse to the newest relevant request.

Examples:
- profile preparation,
- preset preview,
- UI repaint/resize,
- expensive analysis setting changes.

Use generations so stale work cannot overwrite newer state.

Do not use coalescing as an excuse to lose musically significant host automation events.

## 7. Worker and concurrency standard

- Small bounded worker pool.
- Bounded task queue.
- Cancellation support.
- Generation/staleness checks.
- Deterministic shutdown.
- No detached threads.
- No worker owns a pointer into an object whose lifetime is not guaranteed.
- Worker result is immutable before publication.
- Publication to audio uses a realtime-safe handoff.

Thread sanitizer may not be practical on every Windows configuration; concurrency must therefore also be stress-tested and code-reviewed.

## 8. Parameter smoothing

Every audible continuous parameter is evaluated for zipper noise.

Use the cheapest appropriate smoothing:
- linear,
- exponential,
- one-pole,
- short crossfade for structural state.

Do not smooth parameters that represent discrete modes by interpolating invalid intermediate states.

## 9. Numerical safety

DSP code must:
- reject/sanitize non-finite external values,
- guard unstable Q/frequency combinations,
- handle sample-rate changes,
- handle zero-length/very small host blocks if framework permits them,
- clear/reset state predictably,
- disable/avoid denormal slowdowns,
- avoid hidden integer overflow in sizes/indexing.

Debug builds may assert stronger invariants.

## 10. Error handling

Control plane:
- structured error type with category/context,
- no silent parse failures,
- user-facing message remains concise.

Audio plane:
- fail safe to last valid state or bypassed submodule,
- no exception escapes callback,
- no blocking recovery.

## 11. Performance engineering order

1. Correctness.
2. Measurement.
3. Algorithm choice.
4. Memory/copy reduction.
5. Cache locality.
6. SIMD/vectorization.
7. Micro-optimization.

Do not hand-vectorize code without a benchmark demonstrating value.

## 12. Data layout

For hot repeated structures such as modal resonators:
- benchmark array-of-structures vs structure-of-arrays,
- keep frequently used coefficients contiguous,
- avoid pointer chasing,
- align buffers only when the selected SIMD backend benefits.

## 13. GUI performance

- Paint must not decode images or parse files.
- Cache vector paths/rasterized assets where useful.
- Repaint only dirty regions/components.
- Meter refresh rate is bounded.
- High-frequency UI changes are coalesced.
- No UI timer should wake faster than needed.
- UI never reads complex mutable DSP state directly.

## 14. Serialization

- Explicit schema version.
- Locale-independent.
- Stable parameter IDs.
- Bounded payload sizes.
- Checksum for binary factory profile assets.
- Migration tests for released versions.
- Atomic file replace when the standalone tooling writes user profile files.

## 15. Testing layers

### Unit
Math, filters, state, codecs, migrations.

### DSP invariant
Stability, finite output, latency, channel layout, reset.

### Golden audio
Known input -> deterministic render -> tolerance comparison.

### Property/torture
Random legal parameter values, automation, block-size changes.

### Integration
Plugin load, host state, editor lifecycle.

### Performance
Callback histogram, throughput, worker compile time, memory.

### Listening
Level-matched A/B and, where useful, blind preference/ABX-style sessions.

No single testing layer replaces the others.

## 16. CI philosophy

A proposed baseline:
- configure/build Debug,
- unit + DSP tests,
- configure/build Release,
- plugin validation,
- static analysis subset,
- artifact upload,
- benchmark smoke test.

Full sanitizer, long soak, and host matrix can run scheduled/pre-release if too expensive for every commit.

## 17. Dependency hygiene

For every third-party dependency record:
- version/commit,
- source URL,
- license,
- purpose,
- whether linked/distributed,
- update procedure.

Avoid large dependencies for functionality that can be implemented safely in a small module.

## 18. Security

Profile/preset parsers are attack surfaces even in an audio plugin.

- length/size caps,
- bounded loops,
- integer overflow checks,
- path sanitization,
- no executable scripting in preset format,
- fuzz target once format stabilizes.

## 19. Review checklist

Every PR should answer, when relevant:
- Does this touch the audio callback?
- Any allocation or lock?
- Any new worker/lifetime behavior?
- Any state schema change?
- Any CPU/memory change?
- Any backward-compatibility impact?
- Tests added?
- Benchmarks added/updated?
- User-visible behavior documented?

## 20. Release discipline

- Semantic versioning policy documented before public beta.
- Release from tag, not an arbitrary workspace.
- CI artifact must identify commit SHA.
- Checksums published.
- Changelog generated from reviewed changes.
- Emergency fixes still go through reproducible build and minimum validation gates.
