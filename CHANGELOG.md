# Changelog

This file records user-visible, compatibility, architecture and validation changes for Backyard Racer.

## Unreleased

- Advance the exact Infiltratr Common dependency from 1.19.7 to released 1.19.35 (`7cc5de3de0e94ed2cfcff0840bbb5346eb5c9c9f`), inheriting the current atomic-file durability and software-surface clipping/aliasing hardening without changing game-owned behaviour.
- Replaced the procedural vehicle-body renderer with the approved model-specific authored car artwork; runtime paint, wheel and removable-bumper layers are derived from those images.
- Added independently removable/refittable front and rear bumpers with a $50-per-end garage cost and persistent save state.
- Classified listings now open a full newspaper article; purchase requires a separate explicit action from that article.
- Canonical documentation baseline aligned with the Infiltrator project family.

## Policy

Record meaningful behaviour and support changes. Internal research/refactoring needs an entry only when it changes product expectations, dependencies or portability.

## Historical identity

Git history, tags and releases remain authoritative for exact historical source identity.
