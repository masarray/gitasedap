# Multi-Thread Orchestration

This document defines how multiple ChatGPT threads, agents, or human contributors can work on GitaSedap concurrently without creating duplicate authorities, regression loops, or incompatible branches.

---

## 1. Principle: parallelize independent evidence, not competing truth

Multi-threading development is useful only when workstreams have clear ownership.

Allowed parallelism:

```text
                    +-> P4D tonal capture/calibration (#12)
                    |
integrated baseline +-> P5 architecture/tests (#13) ------> P6 (#14)
                    |
                    +-> P7 offline/profile platform (#15)
                    |
                    +-> P8 visual/component prototypes (#16)
```

Integration still follows dependency gates.

Two threads must not independently implement the same canonical subsystem and "choose the better one later".

That is duplicate authority and creates hidden regression debt.

---

## 2. Coordination issues

Canonical large work items:

- #11 integration of the stacked baseline,
- #12 P4D tonal calibration,
- #13 P5 Dynamic Body,
- #14 P6 Polish/Air/live safety,
- #15 P7 Profile/Preset/Training platform,
- #16 P8 Production GUI/UX,
- #17 P9 hardening,
- #18 P10 release.

A new thread should continue an existing issue rather than opening a duplicate milestone.

Sub-issues are acceptable only when a milestone issue explicitly splits a coherent work package.

---

## 3. Workstream claim protocol

Before code changes, add a comment to the target issue using:

```text
WORKSTREAM CLAIM

Thread/work package:
Base branch:
Base SHA:
Planned branch:
Authority/files expected to change:
Interfaces consumed:
Interfaces produced:
Expected PR scope:
Known overlap risk:
```

If another active claim overlaps the same authoritative subsystem, do not race.

Resolve the overlap by one of:

1. split files/responsibility,
2. define producer/consumer interface,
3. serialize the work,
4. explicitly supersede the older claim.

At the end, replace/append a handoff comment with the verified result.

---

## 4. Branch convention

Preferred:

```text
issue-<number>/<short-purpose>
```

Examples:

- `issue-12/p4d-natural-calibration`
- `issue-13/dynamic-modal-excitation`
- `issue-15/profile-trainer-transfer-estimation`

Avoid vague names like `fix`, `new`, `test2`, or personal-thread names.

A branch must name one owning issue in its PR.

---

## 5. Baseline resolution

A thread must never assume `main` is current.

Resolve in this order:

1. read `docs/PROJECT_HANDOFF.md`,
2. inspect issue #11 status,
3. inspect the owning issue's latest coordination comment,
4. inspect its dependency PRs,
5. record the exact base SHA in the workstream claim.

Before #11 completes, the cumulative P4D engineering base is:
`5421fdc9e2cc1fce73229e3f42f62c17d295e6eb`.

After #11 completes, use the new integrated `main` SHA recorded in the handoff.

Never reconstruct a baseline by cherry-picking random commits from old threads.

---

## 6. Authority and conflict matrix

| Area | Canonical authority | Concurrent edits |
|---|---|---|
| host parameter IDs | `src/core/ParameterSpec.h` | serialize unless append-only change explicitly coordinated |
| serialized state/migration | `src/core/StateSchema.h`, `StateMigration.h` | serialize |
| body profile semantic data | `src/dsp/BodyProfile.*` | P4D owns tonal changes; P5/P7 coordinate schema needs |
| body compiler | `BodyProfileCompiler.*` | producer/consumer interface required |
| body audio engine | `HybridBodyEngine.*` | P5 owns structural evolution |
| profile crossfade | `CrossfadingBodyEngine.*` | normally frozen unless bug/gate requires change |
| publication runtime | `BodyProfileRuntime.*` | P7 consumes/extents, no second publisher |
| prepared exchange | `PreparedStateExchange.h` | infrastructure authority; do not duplicate |
| workers/coalescing | existing `src/runtime` components | P7 reuses; do not create parallel pools |
| lab/evidence | `src/lab`, `tools/dsp-lab` | parallel commands/tests are usually safe if names/scopes do not collide |
| plugin adapter/UI | `src/plugin/GitaSedap.*` | P8 owns production UI; DSP threads avoid opportunistic layout changes |

When in doubt, prefer a narrow new helper with one direction of dependency instead of broad edits across authorities.

---

## 7. Producer/consumer pattern

Parallel work should use stable boundaries.

Example:

```text
P7 trainer worker
   -> produces canonical BodyProfileDefinition / prepared candidate
   -> existing compiler/runtime validates/publishes
   -> P5/P6 audio path consumes existing prepared state
```

Bad:

```text
P7 trainer creates its own DSP engine
P5 creates separate profile representation
P8 stores profile state in widgets
```

A workstream may create a new interface only when:
- ownership is explicit,
- dependency direction is clear,
- it does not duplicate an existing authority,
- tests prove the boundary,
- the owning milestone documents it.

---

## 8. Merge/integration policy

### One coherent PR purpose

A PR should not mix:
- DSP redesign,
- state migration,
- worker platform,
- GUI redesign,
- release packaging,

unless the pieces are truly inseparable.

### Stacked sub-PRs inside a milestone

Allowed when needed:

```text
issue-13/p5a-intensity-model
   -> issue-13/p5b-dynamic-modal
   -> issue-13/p5c-body-space
```

But each layer must:
- have a clear base,
- be independently reviewable,
- keep previous tests green,
- document what is provisional vs verified.

### Integration ordering

Dependency order beats creation time.

A newer PR must not be merged first just because its CI is green if its semantic base has not landed.

---

## 9. Regression ownership

Every workstream owns protection against regressions it can cause.

Minimum PR evidence:

- relevant unit/invariant tests,
- previous subsystem regression tests,
- finite audio checks where DSP changes,
- lifecycle/state tests where state/runtime changes,
- benchmark where hot path changes,
- official validator where plugin boundary changes,
- real listening evidence where a tonal claim is made.

Do not delete or weaken an existing test merely to make a branch pass.

If a test no longer represents the product requirement, update the requirement, ADR, and test together.

---

## 10. Performance and memory review

For every change that can reach a frequent path, answer in the PR:

- Does it allocate?
- Does it destroy ownership?
- Is any lock reachable?
- Is work bounded?
- Is state precomputed?
- Can duplicate work be coalesced?
- Is data contiguous/cache-friendly?
- Are copies necessary?
- What does p99/p99.9 do?
- Can shutdown race with it?

Optimization order remains:

```text
correct architecture
 -> remove unnecessary work
 -> remove allocations/copies
 -> improve locality
 -> batch/coalesce
 -> SIMD measured hotspots
 -> micro-optimize
```

Do not introduce SIMD/template complexity without a measured hotspot.

---

## 11. P4D/P5 overlap rule

P4D (#12) owns the accepted static body tone.

P5 (#13) owns dynamic behavior around that body.

While P4D waits on capture, P5 may safely work on:
- intensity estimator,
- modulation data model,
- stability tests,
- synthetic soft/medium/hard fixtures,
- bounded runtime structure.

P5 must not:
- silently rewrite Natural/Dread static coefficients,
- declare tonal success on synthetic material,
- merge a profile-dependent tuning that invalidates P4D evidence.

Once P4D publishes an accepted body profile revision, P5 rebases/updates expected baselines and continues dynamic tuning.

---

## 12. P7 parallel rule

P7 (#15) may progress in parallel because training is control-plane/offline.

Safe early P7 work:
- profile container/schema,
- parser/validator,
- regularized transfer estimation,
- modal extraction experiments,
- deterministic exporter,
- bounded cache policy,
- worker task types,
- cancellation/coalescing tests.

P7 must reuse:
- canonical BodyProfile model,
- existing compiler,
- existing PreparedStateExchange/publication,
- existing bounded worker infrastructure.

Do not integrate a custom trained profile into live audio through a private shortcut.

---

## 13. P8 parallel rule

P8 (#16) may prototype:
- layout,
- scalable components,
- cached vector asset handling,
- meter latest-value transport,
- editor lifecycle stress.

Before P6/P7 contracts stabilize, P8 must not invent final host parameters or persist widget-only state as audio truth.

The production UI should bind to canonical parameters/profile IDs when the contract is ready.

---

## 14. Stop-the-line conditions

Any thread must stop feature expansion when:

- a previous green regression turns red,
- parameter/state identity becomes ambiguous,
- callback allocation/lock appears,
- queue/cache becomes unbounded,
- lifecycle race is plausible,
- two authorities exist for the same concept,
- output becomes non-finite,
- profile switching clicks/crashes,
- measured p99 materially regresses without justification,
- a tonal claim has no level-matched real evidence,
- a branch cannot explain its base/dependency chain.

Restore a verified baseline before adding more features.

---

## 15. Pull request completion checklist

A PR is ready to call verified only when its description states:

- owning issue,
- exact base SHA,
- intended authority changes,
- previous verified behavior,
- new behavior,
- tests,
- CI run,
- realtime impact,
- memory/lifecycle impact,
- state compatibility,
- performance result if relevant,
- evidence/artifacts,
- remaining blocked work.

"Build succeeded" is never enough.

---

## 16. Handoff comment template

Use this on the owning issue at the end of a thread:

```text
WORKSTREAM HANDOFF

Issue:
Branch:
Base SHA:
Head SHA:
PR:
CI run:
Artifacts:

Implemented:
- ...

Verified:
- ...

Measured:
- ...

Not yet tested:
- ...

Blocked by:
- ...

Authority/files changed:
- ...

State compatibility:
- ...

Realtime contract:
- ...

Memory/lifecycle:
- ...

Known risks:
- ...

Next exact action:
- ...
```

This comment is the durable bridge between ChatGPT threads.

---

## 17. Long-gap recovery

If the project sits untouched for months:

1. do not immediately update dependencies,
2. first reproduce the last recorded CI baseline,
3. verify branch/PR status has not changed,
4. verify the pinned toolchain can still build,
5. only then address dependency/host drift in a dedicated issue,
6. resume the owning milestone from measured evidence.

A long pause is not permission to restart architecture from scratch.
