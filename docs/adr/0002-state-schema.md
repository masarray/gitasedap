# ADR-0002: Versioned canonical plug-in state envelope

- **Status:** Accepted for P1B
- **Date:** 2026-09-29
- **Scope:** Host project state, presets, and future migration boundary

## Context

GitaSedap will eventually persist more than scalar parameters: selected body
profiles, profile versions, structural engine choices, and other non-realtime
configuration.

Relying indefinitely on framework-default positional parameter serialization
would make later migrations fragile and would couple persisted sessions to
implementation details.

The state contract should be established before public releases and before the
profile platform exists.

## Decision

Enable iPlug2 state chunks and prepend every GitaSedap host-state payload with a
small canonical binary envelope:

```text
offset  size  field
0       4     ASCII magic "GSDP"
4       4     schema version, unsigned little-endian
8       ...   payload for that schema version
```

Schema version 1 delegates the payload to iPlug2's parameter serialization after
the GitaSedap header.

The header encoding/decoding logic lives in framework-independent core code.
The plug-in wrapper is only an adapter between `IByteChunk` and that canonical
header.

## Why explicit bytes

The header is encoded byte-by-byte instead of serializing a C++ struct. This
avoids:

- ABI padding assumptions,
- compiler layout differences,
- native-endian ambiguity,
- accidental persistence of implementation-only fields.

## Compatibility policy

- `kCurrentVersion` is the state written by current code.
- `kMinimumSupportedVersion` defines the oldest version that may be read.
- Unknown future versions are rejected rather than guessed.
- A released schema version is never silently redefined.
- Future schema changes must add an explicit migration path and regression
  fixtures before support is removed.

There is no migration for the pre-P1B development shell because it has not been
released as a public product state contract.

## Realtime implications

Serialization is host/control-plane work. It must not become part of the audio
callback.

State restoration updates framework parameters. The realtime processor observes
those values through its normal parameter path and performs click-safe smoothing;
the state loader does not mutate heavyweight realtime DSP graphs directly.

Future structural state will be compiled into an immutable prepared state off
the audio thread before publication.

## Validation

P1B adds:

- compile-time tests for exact canonical header bytes,
- invalid-magic rejection,
- unsupported-version rejection,
- official Steinberg VST3 validator execution,
- validator local-instance mode to stress repeated construction/destruction.
