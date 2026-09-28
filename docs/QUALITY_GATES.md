# GitaSedap Quality Gates

This document converts "sounds good and feels light" into release evidence.

## Gate A - Build reproducibility

Pass when:
- clean checkout configures without manual IDE edits,
- dependencies resolve to pinned versions,
- Debug and Release compile,
- artifact embeds/reports project version and commit information where practical.

## Gate B - Plugin validity

Pass when:
- VST3 scans successfully in the reference validator,
- creates/destroys repeatedly,
- editor opens/closes repeatedly,
- supported bus layouts negotiate correctly,
- state roundtrip succeeds.

## Gate C - Realtime safety

Pass when the callback path has:
- zero intentional heap allocation/free,
- zero blocking lock,
- zero file/network access,
- zero synchronous logging,
- bounded loops/work,
- deferred heavyweight destruction.

Use code review plus runtime instrumentation/tests.

## Gate D - DSP correctness

Test:
- silence,
- impulse,
- DC,
- sine sweep,
- pink noise,
- maximum legal parameter values,
- sample-rate changes,
- reset,
- variable block sizes,
- guitar fixtures.

Pass when:
- no NaN/Inf,
- no unstable resonance,
- no unexpected DC runaway,
- no channel corruption,
- deterministic output within defined tolerance.

## Gate E - Audio regression

For each approved fixture store:
- input hash,
- preset/profile ID,
- sample rate,
- expected latency,
- render hash/metric snapshot.

Use tolerances rather than bit identity when compiler/platform floating-point differences make bit identity unrealistic.

Any intentional golden change requires a listening note and review.

## Gate F - CPU deadline

Reference scenario:
- one mono guitar instance,
- 48 kHz,
- 64-sample block,
- editor closed,
- standard Natural profile.

Track:
- mean callback time,
- p95,
- p99,
- p99.9/max diagnostic,
- CPU utilization,
- underruns/xruns.

Target:
- p99 <25% of one buffer deadline on the documented reference machine.
- no sustained xruns.
- >10% regression against baseline triggers review.

Also test:
- 44.1/96 kHz,
- 32/64/128/256 sample blocks,
- multiple plugin instances.

## Gate G - Memory/lifecycle

Stress sequences:
- create/destroy plugin 1,000 times in harness where practical,
- editor open/close 1,000 cycles,
- swap profiles repeatedly,
- cancel worker tasks during destruction,
- load malformed profile/preset files.

Pass when:
- no known leak,
- no use-after-free,
- no unbounded queue/cache growth,
- memory returns to stable plateau after repeated operations.

## Gate H - Worker correctness

Pass when:
- queue is bounded,
- stale generation cannot overwrite newer generation,
- cancellation works,
- shutdown joins cleanly,
- audio continues using last valid state during long analysis,
- worker failure is surfaced without audio interruption.

## Gate I - State compatibility

Pass when:
- current state roundtrips,
- supported older fixtures migrate forward,
- unknown/malformed fields fail safely,
- canonical parameter IDs remain stable.

## Gate J - UI performance

Pass when:
- vector UI scales correctly,
- no asset decode in paint loop,
- meter/UI refresh is bounded,
- rapid knob dragging does not create audio glitches,
- profile browsing coalesces expensive preparation,
- editor destruction cancels/detaches callbacks safely.

## Gate K - Host matrix

Before release, document tested versions and outcomes for:
- REAPER,
- at least one additional mainstream Windows VST3 host,
- Steinberg host if available.

Scenarios:
- scan,
- insert/remove,
- automation,
- preset/state recall,
- project reopen,
- offline render,
- live input,
- sample-rate/buffer changes.

## Gate L - Listening acceptance

Use level-matched comparisons.

Evaluate:
- raw piezo vs processed,
- Body low/medium/high,
- Air off/on,
- Enhance off/on,
- Live vs Natural,
- soft/medium/hard playing.

Reject tuning that wins only because it is louder.

Desired result:
- less piezo quack,
- more convincing acoustic body,
- preserved articulation,
- no obvious short-reverb effect,
- controlled low-mid bloom,
- open but non-harsh top end.

## Gate M - Release package

Pass when:
- version/tag fixed,
- CI green,
- release notes complete,
- third-party notices complete,
- known issues documented,
- artifact checksums available,
- artifact traceable to exact commit,
- install/remove instructions tested.

No new feature should be added after release-candidate freeze unless it fixes a release blocker.
