---
quick_id: 260827-crb
description: disable per-push GitHub Actions builds; build only via /publish
date: 2026-08-27
---

# Quick Task: disable per-push GitHub Actions builds; build only via /publish

## Finding
- `build-and-release.yml` already triggers only on `*-v*` tag push (created by `/publish`) or `workflow_dispatch`. Nothing to change.
- `ci-tests.yml` triggers `on: [push, pull_request]` — downloads JUCE and runs `cmake --build` on every commit. This is the unwanted build.

## Tasks
1. `ci-tests.yml`: replace `on: [push, pull_request]` with `workflow_dispatch` only; rewrite header comments that argued for per-push.
2. `plugins/O-Octagon/CMakeLists.txt:170`: update the comment that says the probes run per-commit.
3. Commit path-scoped; update STATE.md quick table.
