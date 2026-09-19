# Design

## First-principles position

Backyard Racer begins with the experience and behaviour it must own. Existing products, frameworks and techniques are evidence to study, not specifications that must be copied.

## Goals

- preserve the hands-on appeal of classic garage/racing games without surface cloning
- keep vehicle state, economy and race simulation coherent and testable
- use full-frame asset-driven presentation rather than toolkit-looking game UI
- retain proven mechanics when they remain stronger than a fashionable replacement

## Non-goals

The project is not a compatibility remake and does not inherit another game's obsolete platform architecture merely because its mechanics are useful references.

## Dependency and language policy

Prefer first-party C/C++ for native/core logic where appropriate. Use platform-native code at real platform boundaries and exact first-party shared dependencies for generic primitives. Dependencies must not become hidden owners of product semantics.

## Failure and quality

Invalid, unavailable and unsupported states are explicit. Persistent state should be durable and recoverable at the level promised by the product. A design change should improve correctness, fidelity, usability, performance, resilience or maintainability; newness alone is not a benefit.

## Completion quality

A feature is complete when behaviour, edge cases, tests and documentation agree, not simply when the common case renders or calculates once.
