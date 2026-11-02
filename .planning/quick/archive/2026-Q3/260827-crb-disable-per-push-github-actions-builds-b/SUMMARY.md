---
quick_id: 260827-crb
status: complete
date: 2026-08-27
commit: 6ae1760c
---

# Summary

- `.github/workflows/ci-tests.yml`: trigger changed from `on: [push, pull_request]` to `workflow_dispatch` only; header rewritten to state the no-build-on-push policy.
- `plugins/O-Octagon/CMakeLists.txt`: comment updated (probes no longer per-commit).
- `build-and-release.yml` untouched — already tag-only (`*-v*`, pushed by `/publish`) + manual dispatch.

Result: pushing commits to GitHub triggers no Actions build. Builds happen only via `/publish` (tag) or explicit manual dispatch.
