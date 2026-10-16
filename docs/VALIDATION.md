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

## Build and publication ownership

Root VERSION owns source and package identity. The packaging script configures, builds and tests the generic package; CI separately qualifies the native profile. Packaging reuses a valid build directory without deleting an arbitrary caller-supplied path.

The successful CI run seals the Debian package, exact Git source archive and commit identity with SHA-256. Publication downloads that run's artifact and compares remote bytes before and after draft promotion. Public tags and assets are immutable; advancing VERSION is the release signal. Commit subjects carry no control-plane meaning.

The source build retrieves Common at the exact commit recorded in CMake. Initial configuration therefore needs network access unless that dependency is already materialised. Human playtesting remains separate from automated core/package qualification.
