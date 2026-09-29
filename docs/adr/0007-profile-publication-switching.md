# ADR-0007: Generation-aware prepared profile publication and bounded audio crossfade

- **Status:** Accepted for P4B engineering implementation
- **Date:** 2026-09-29
- **Scope:** Prepared profile publication, stale-result rejection, realtime acquisition, profile crossfade, and deferred reclamation

## Context

P4A can compile immutable sample-rate-specific body profiles, but a production
profile system also needs to survive rapid user changes without:

- locks in the audio callback,
- profile parsing or coefficient design in the callback,
- destroying heavyweight state on the callback,
- stale worker results overwriting newer requests,
- audible hard switches,
- unbounded queues/state retention.

## Decision

P4B composes two existing architectural ideas:

1. `PreparedStateExchange<PreparedBodyProfile>` for generation-aware
   control-plane publication and deferred reclamation.
2. `CrossfadingBodyEngine` for bounded dual-engine audio transitions.

## Control-plane flow

```
begin request N
    ↓
compile complete PreparedBodyProfile
    ↓
publish generation N
    ↓
stale generations rejected
```

Compilation and allocation remain outside the per-sample path.

## Audio-plane flow

At the start of an audio block:

1. acquire the newest published prepared state,
2. if no body profile is active, copy it into the active fixed-size engine,
3. otherwise, if no transition is already running, copy it into the inactive
   fixed-size engine and start a linear crossfade,
4. process both engines only for the bounded transition window,
5. promote the target engine when the transition completes.

No prepared-state pointer is retained by the DSP engine after coefficient copy.

Therefore old prepared objects may be reclaimed outside the callback even while
their copied coefficients are still sounding in a transition.

## Rapid switching / coalescing

A transition is not recursively retargeted while already in progress.

If newer profiles arrive during a transition:

- `PreparedStateExchange` keeps the newest published active state,
- the current audio transition finishes,
- at the next block boundary the engine compares hashes,
- if a newer profile is pending, one new transition starts.

This converts arbitrary rapid clicking into a bounded sequence of audible
transitions without needing three, four, or unbounded parallel engines.

Intermediate profile requests may therefore be skipped when superseded. This is
intentional control-plane coalescing and does not apply to musical host
automation such as BODY.

## Crossfade

P4B uses a short linear crossfade (15 ms default).

The two body paths are highly correlated. A linear fade avoids the gain bump
that an equal-power crossfade could introduce.

The crossfade length is prepared from sample rate and never allocated dynamically.

## Reclamation

Prepared profile ownership remains in `PreparedStateExchange`.

Old generations are reclaimed only from control-plane calls to
`drainReclaimable()` or shutdown.

The audio callback never destroys profile objects.

## Plugin integration

The plugin uses `BodyProfileRuntime` even though P4B exposes only the Natural
Development profile in the current product UI.

This means later profile selectors/custom training can publish prepared states
without rewriting the audio path.

No new host-facing parameter ID is introduced in P4B.

## Realtime bounds

Normal body processing:

- one HybridBodyEngine.

During a profile transition:

- two HybridBodyEngine instances for a fixed transition window.

There is never an unbounded number of parallel body engines.

## Testing

P4B regression tests cover:

- stale generation rejection,
- first-profile activation,
- click-safe Natural -> Dreadnought transition,
- newer-profile deferral during an active transition,
- thousands of rapid publications,
- bounded prepared-state ownership after control-plane draining,
- finite output throughout stress,
- deterministic shutdown.

## Future

P7 workers may perform external/custom profile compilation and call the same
request/prepare/publish API.

The P4B publication contract is intentionally reusable and should not be
replaced by UI-specific state handoff.
