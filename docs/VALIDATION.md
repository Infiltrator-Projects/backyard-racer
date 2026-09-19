# Validation

## Evidence model

Compilation, deterministic regression tests, platform integration and human visual/interaction testing prove different things.

## Automated gates

- .github/workflows/ci.yml

tests/ covers gameplay progression, paint state, pixel-core behaviour and race sessions. These deterministic tests protect the game rules independently of the rendering backend.

## Manual/environment evidence

Visual quality, feel, input responsiveness, authored asset alignment and whether racing is enjoyable require human playtesting in addition to deterministic game-state tests.

Manual evidence supplements automation and should record the platform/environment actually observed.

## Release criterion

The exact source intended for release/publication must pass required automated checks, and generated/package artifacts must correspond to that source identity.

## Regression rule

Reproducible defects should gain permanent automated coverage at the narrowest layer that captures the original failure.
