---
created: 2026-08-11T12:00
title: Secrets-free push/PR workflow that builds and runs the test targets
area: tooling
files:
  - .github/workflows/build-and-release.yml:131
---

## Problem

**No test target in this repository has ever run in CI.**

`build-and-release.yml` is the only workflow that builds anything. It is **tag-triggered**, it is
**secrets-bearing** (Apple signing identity, notarytool credentials), and it has never set
`-DOUARICON_BUILD_TESTS=ON`. Every one of the twelve-plus render harnesses and console test targets
in `plugins/*/tests/` therefore runs only when a developer remembers to configure and run it by
hand, on one machine.

The concrete residual risk, stated plainly:

> **A JUCE bump performed without running the test targets ships silently.**

O-Octagon Phase 2.1 makes this sharper than it was. Its Layer-2 channel-map gate is a
`static_assert` comparing a committed SHA-256 against a golden **generated from parsed JUCE source
at build time** (`plugins/O-Octagon/tests/tools/gen_juce_channel_order.py`). That gate is designed
to fail the build the moment JUCE's `ChannelType` enum values or 8-channel set membership change —
which is exactly the change class that would silently misroute audio to the wrong speakers in a
concert hall. **But the gate only exists inside the `O-Octagon-geometry-test` target, so today it
only fires for whoever happens to configure with `OUARICON_BUILD_TESTS=ON`.**

## Solution

A **separate**, secrets-free workflow on `push` and `pull_request`:

```yaml
# .github/workflows/tests.yml   (NEW FILE — see the constraint below)
on: [push, pull_request]
jobs:
  tests:
    runs-on: macos-latest
    steps:
      - configure with -DOUARICON_BUILD_TESTS=ON
      - build every *-test / *-render-test target
      - run each one; fail the job on any non-zero exit
```

This benefits all twelve existing harnesses, not only O-Octagon.

### Hard constraint — do NOT modify `build-and-release.yml`

It is tag-triggered and carries repository secrets. Widening its trigger surface to `push` or
`pull_request` would expose those secrets to every branch and every fork PR. This must be a new,
separate workflow file with no `secrets:` usage at all.

## Why this is not being done now

Out of scope for O-Octagon Phase 2.1 (PLAN-2.1 P13). It is a repo-level tooling change whose
blast radius is every plugin, and folding it into a plugin phase would mean shipping an
unreviewed CI change under a DSP commit. Logged rather than quietly dropped.

## Acceptance

- [ ] A new workflow runs on `push` and `pull_request` and uses no secrets
- [ ] It configures with `-DOUARICON_BUILD_TESTS=ON`
- [ ] It builds and runs every test target that currently exists, failing the job on non-zero exit
- [ ] `build-and-release.yml` is untouched
- [ ] Deliberately breaking O-Octagon's committed SHA on a branch fails the new job
