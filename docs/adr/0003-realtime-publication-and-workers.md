# ADR-0003: Realtime publication, coalescing, and bounded workers

- **Status:** Accepted for P1C
- **Date:** 2026-09-29
- **Scope:** Structural DSP preparation and control-plane concurrency

## Context

GitaSedap will prepare expensive immutable DSP state outside the audio callback:
FIR kernels, modal coefficients, dynamic curves, profile transforms, and future
trained-profile data.

The project therefore needs a publication/lifetime model before those DSP
objects become large.

Naive approaches are rejected:

- `shared_ptr` swaps that may run the final destructor on the callback,
- mutex-protected DSP state,
- one thread per request,
- unbounded task queues,
- stale worker results overwriting newer user intent,
- deleting a replaced state while the callback is acquiring it.

## Decision

P1C introduces three framework-independent runtime primitives.

### 1. PreparedStateExchange

A structural request receives a monotonically increasing generation token.

Workers prepare a complete immutable state for that generation. Publication is
accepted only if the generation is still current.

The control plane owns all prepared-state heap allocations.

At the audio block boundary:

1. load the pending pointer,
2. hazard-protect it,
3. verify it is still pending,
4. claim it atomically,
5. publish it as active,
6. clear the hazard.

The callback never deletes a state.

The control plane periodically reclaims any owned state that is not:

- pending,
- active, or
- hazard-protected.

This avoids `shared_ptr` last-release destruction and avoids a realtime
retirement queue that could overflow.

The audio-side operation is bounded to a few atomic pointer operations once per
block. Sequentially consistent ordering is intentionally used in P1C for a
simple, auditable correctness baseline. Memory-order relaxation is a future
benchmark-driven optimization, not a foundation gamble.

### 2. BoundedWorkerPool

Non-realtime heavy work uses a small fixed worker set and a fixed-capacity task
ring.

Properties:

- no detached threads,
- no thread-per-click behavior,
- bounded queued work,
- cooperative pool stop,
- per-task cancellation token,
- deterministic join,
- queued tasks cancelled during shutdown,
- task exceptions counted without synchronous logging.

The worker pool is control-plane infrastructure only.

### 3. CoalescingQueue

High-rate control requests with the same canonical key replace the older pending
value rather than accumulating duplicate work.

The queue:

- has compile-time bounded capacity,
- is control-plane locked,
- preserves deterministic order between different keys,
- refreshes sequence order when a key is replaced.

It must not be used to discard musically meaningful host automation events.

## Lifecycle

The host/plugin owner must stop the realtime callback before destroying the
PreparedStateExchange.

Worker shutdown order remains:

1. stop accepting tasks,
2. cancel queued work,
3. request cooperative stop,
4. join workers,
5. stop audio if not already stopped,
6. reclaim remaining prepared states.

Concrete plugin lifecycle integration will occur before structural DSP starts
using these primitives.

## Validation

P1C tests verify:

- newest-generation publication,
- stale-result rejection,
- pending-state coalescing,
- no prepared-state destruction on audio-side activation,
- explicit control-plane reclamation,
- bounded worker queue behavior,
- per-task cancellation,
- worker exception containment,
- deterministic shutdown/rejection after stop,
- bounded coalescing queue semantics.
