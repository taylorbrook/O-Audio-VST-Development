---
phase: quick-260719-q0o
plan: 01
subsystem: build/dependencies
status: complete
tags: [anira, onnxruntime, fetchcontent, o-texture, dependency-bump]
requires: []
provides:
  - "O-Texture built against ANIRA v2.1.0 (ORT 1.19.2 unchanged)"
affects:
  - plugins/O-Texture
tech-stack:
  added: []
  patterns:
    - "ANIRA FetchContent bumped to a same-ORT tag: one-line GIT_TAG edit + cache invalidation, no ONNXRUNTIME_VERSION change"
key-files:
  created: []
  modified:
    - plugins/O-Texture/CMakeLists.txt
decisions:
  - "Took ANIRA v2.1.0 (provisions ORT 1.19.2), NOT v2.2.x (couples ORT 1.26.0) — smallest change surface, no dylib/rpath re-plumbing"
metrics:
  duration: ~2min
  completed: 2026-07-19
---

# Quick Task 260719-q0o: Bump ANIRA GIT_TAG v2.0.3 → v2.1.0 in O-Texture Summary

One-line ANIRA FetchContent pin bump (v2.0.3 → v2.1.0) in O-Texture, a zero-string-change drop-in since v2.1.0 provisions the same ONNX Runtime 1.19.2 — re-fetched, rebuilt, installed, and proven with a three-gate verification (otool @rpath resolution, auval AU-link, pluginval strictness 8).

## What Was Built

- **Task 1** — Changed `GIT_TAG v2.0.3` → `v2.1.0` in `plugins/O-Texture/CMakeLists.txt:15` (the only source edit). Left `set(ONNXRUNTIME_VERSION 1.19.2)` untouched. Invalidated the ANIRA FetchContent cache (`build/_deps/anira-{src,build,subbuild}`) so the next configure fetches the new tag cleanly. Commit `bfdd6e0`.
- **Task 2** — Ran `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` (ANIRA re-fetched, `git describe` → `v2.1.0`), then `./scripts/build-and-install.sh O-Texture` (resolves the `OuariconTexture` target, builds VST3 + AU, AU cache-clear + dual-variant sweep + fresh install per CLAUDE.md). Both `O-Texture-dev.vst3` and `O-Texture-dev.component` built and installed (58M each). No committable source changes (build/install artifacts are generated/external).
- **Task 3** — Three-gate verification (read-only, no commit):
  - (a) `otool -L` shows `@rpath/libonnxruntime.1.19.2.dylib`; `LC_RPATH` carries `@loader_path/../Frameworks`; embedded `libonnxruntime.1.19.2.dylib` + `libonnxruntime.dylib` symlink present in `Contents/Frameworks`.
  - (b) `scripts/verify-au-link.sh O-Texture` → `AU VALIDATION SUCCEEDED. auval accepted O-Texture (aumu OuTx OuDv)`.
  - (c) `pluginval --strictness-level 8 --validate-in-process --skip-gui-tests` → `SUCCESS`.

## Verification Results

| Gate | Result |
|------|--------|
| Source: GIT_TAG v2.1.0, ORT 1.19.2 unchanged, no v2.0.3 residue | PASS |
| Cache invalidated (`build/_deps/anira-src` removed) | PASS |
| ANIRA re-fetched at v2.1.0 (`git describe`) | PASS |
| VST3 + AU bundles build with Mach-O binaries | PASS |
| otool @rpath/libonnxruntime.1.19.2.dylib + @loader_path/../Frameworks rpath | PASS |
| Embedded 1.19.2 dylib + symlink present | PASS |
| verify-au-link.sh O-Texture (auval) | PASS |
| pluginval strictness 8 | PASS (SUCCESS) |

`--skip-gui-tests` was added to the pluginval invocation (as the plan explicitly permits) to avoid the WebView editor stalling the headless run — a benign harness accommodation, not a scope change.

## Deviations from Plan

None — plan executed exactly as written. `--skip-gui-tests` was applied per the plan's own allowance in Task 3(c) for the WebView editor.

## Threat Mitigations

- **T-q0o-02 (mitigate)** — Embedded ORT dylib resolution proven before the plugin is trusted in a host: otool confirms `@rpath/libonnxruntime.1.19.2.dylib` resolves via the `@loader_path/../Frameworks` rpath to the bundled `Contents/Frameworks/libonnxruntime.1.19.2.dylib`.
- **T-q0o-01 (accept)** — Supply-chain: same upstream repo, pinned tag vetted in `research/framework-updates-2026-07.md`, no new registry install, ORT stays 1.19.2; re-validated by verify-au-link + pluginval.

## Commits

- `bfdd6e0` — chore(quick-260719-q0o): bump ANIRA GIT_TAG v2.0.3 → v2.1.0 in O-Texture

(Tasks 2 and 3 are build/verify steps with no committable source changes.)

## Self-Check: PASSED

- `plugins/O-Texture/CMakeLists.txt` — modified (GIT_TAG v2.1.0), FOUND
- Commit `bfdd6e0` — FOUND in git log
- No known stubs. No new threat surface introduced.
