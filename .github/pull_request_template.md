## Owning issue

Closes/advances #

## Exact baseline

- Base branch:
- Base SHA:
- Previous verified CI run:

## Problem

What concrete product/engineering requirement does this PR satisfy?

## Authority

Existing canonical authority used/extended:

Files/subsystems intentionally changed:

Confirm:
- [ ] No second DSP/state/profile/worker authority introduced.
- [ ] Canonical IDs/schemas remain compatible or include explicit migration.

## Design

Explain the smallest coherent production design and why it fits the architecture.

## Realtime review

- [ ] No callback allocation/deallocation.
- [ ] No blocking lock/condition variable/sleep.
- [ ] No file/network/UI/logging from callback.
- [ ] No unbounded work.
- [ ] No heavy destruction on callback.
- [ ] Structural work is prepared/published safely where applicable.
- [ ] Click/automation/reset behavior reviewed.

Realtime impact:

## Memory / lifecycle review

Ownership model:

Queue/cache bounds:

Shutdown/cancellation behavior:

Leak/race risks reviewed:

## Tests and evidence

New tests:

Regression tests:

CI run:

Artifacts/listening evidence:

## Performance

Relevant benchmark context and before/after p99/p99.9:

If not performance relevant, explain why.

## State compatibility

Parameter IDs:

Serialized state:

Profiles/presets:

Migration:

## Scope exclusions

What this PR intentionally does not change.

## Handoff

Implemented:

Verified:

Measured:

Not yet verified / blocked:

Exact next action:
