# Decisions

This file records durable architectural/gameplay choices for Backyard Racer.

## ADR-001 — Classic Street Rod is reference material, not a compatibility target

**Decision.** Preserve mechanics worth keeping while owning the new game's rules, simulation and presentation.

**Rationale.** The original games provide proven ideas but also carry platform and design constraints that need not be reproduced.

**Consequence.** Public source/research may accelerate understanding without making upstream behaviour the specification.

## ADR-002 — Game state is independent of presentation

**Decision.** Car ownership, installed/spare parts, money, reputation, race state and progression live in the game model rather than UI widgets.

**Rationale.** Persistent progression and deterministic tests require state to survive presentation changes.

**Consequence.** The renderer can be replaced/refined without rewriting gameplay rules.

## ADR-003 — Full-frame asset-driven presentation is the target

**Decision.** The game composes the complete frame from authored graphical assets and software surfaces rather than presenting a toolkit-style interface.

**Rationale.** The desired garage/racing experience depends on a cohesive game image rather than desktop-widget aesthetics.

**Consequence.** Primitive X11/system-font drawing is transitional, while Common graphics primitives support compositing/scaling.

## ADR-004 — Pixel art and authored imagery use different scaling policies

**Decision.** Pixel-art surfaces retain nearest-neighbour scaling; photographic/authored imagery may use Common's alpha-aware bilinear path.

**Rationale.** One scaling filter is not visually correct for both asset classes.

**Consequence.** Presentation code selects scaling according to asset intent.

## ADR-005 — Race simulation uses deterministic fixed-step state

**Decision.** Live quarter-mile racing advances through the established fixed-step scheduler rather than frame-rate-dependent rules.

**Rationale.** Gameplay behaviour should not change with presentation refresh rate.

**Consequence.** Race-session tests can reproduce control/shift/distance behaviour deterministically.

## ADR-006 — Save publication is durable

**Decision.** Persistent progression uses atomic/durable first-party file publication through Common primitives.

**Rationale.** A crash during save should not casually destroy the user's garage/progression.

**Consequence.** Save format/state changes must preserve recoverable publication semantics.
