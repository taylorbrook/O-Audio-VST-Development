---
phase: quick-260719-hn4
plan: 01
subsystem: build-tooling / dependency-management
status: complete
tags: [juce, anira, onnxruntime, nanoflann, umappp, note-expression, framework-updates]
requires: []
provides:
  - research/framework-updates-2026-07.md
  - 260719-hn4-EVIDENCE.md
affects:
  - "future JUCE 8.0.14 upgrade (deferred, gated)"
tech-stack:
  added: []
  patterns:
    - "pinned-tag-only upstream fetch for reproducible diff sizing"
key-files:
  created:
    - research/framework-updates-2026-07.md
    - .planning/quick/260719-hn4-check-juce-and-framework-updates-review-/260719-hn4-EVIDENCE.md
  modified: []
decisions:
  - "JUCE re-base risk is HIGH not for diff size but because juce_VST3ClientExtensions.h RELOCATED to a new juce_audio_processors_headless module in 8.0.14 — CI copy + grep-gate paths must move"
  - "ANIRA v2.1.0 keeps ORT 1.19.2 (zero-string bump); v2.2.x moves ORT to 1.26.0 (requires hardcoded string edit in O-Texture CMakeLists)"
  - "nanoflann upstream latest is v1.7.1 (scout's v1.10.0 tag does not exist); both header-only deps are drop-in for actual repo usage"
metrics:
  duration: ~14min
  completed: 2026-07-19
  tasks: 2
  files: 2
---

# Quick Task 260719-hn4: Check JUCE & Framework Updates — Review Summary

Delivered a decision-ready `research/framework-updates-2026-07.md` that risk-ranks upstream updates
to every pinned framework (JUCE 8.0.9→8.0.14, ANIRA v2.0.3→v2.2.1 + ONNX Runtime, nanoflann, umappp,
pluginval), with the note-expression vendored-patch re-base quantified from actual 8.0.9-vs-8.0.14
diffs. No upgrade performed; no build-affecting file modified.

## What was done

- **Task 1 (EVIDENCE.md, commit 317fead):** Fetched upstream JUCE/ANIRA/nanoflann/umappp source at
  pinned tags via curl; produced per-file override diffs, `BREAKING_CHANGES.md` excerpts, repo-exposure
  greps, an ANIRA→ORT table, and header-only compat verdicts. 7 H2 evidence sections.
- **Task 2 (deliverable, commit 1d562b6):** Synthesized EVIDENCE into the 8-section review + integration
  plan (Summary table, Per-Dependency Review, JUCE Breaking-Change Review, Risk Assessment, Recommended
  Upgrade Order, Verification Strategy, Rollback Strategy, Deferred/Up-to-date).

## Key findings

- **Note-expression patch re-base is the headline HIGH risk.** `juce_audio_plugin_client_VST3.cpp`:
  325 upstream-churn lines / 32 hunks (re-base MODERATE — the `toMidiBuffer` anchor survives verbatim
  at 8.0.14:3590-3591). `juce_VST3ClientExtensions.h`: small diff (34 lines) but **the file relocated**
  from `juce_audio_processors/utilities/` to a new `juce_audio_processors_headless/utilities/` module
  (old path 404s at 8.0.14) and gained a companion `.cpp` — the CI `cp -R` + two `grep -q JUCE-NE-PATCH`
  gate paths in `build-and-release.yml` would break, so re-base rating HIGH.
- **Zero repo exposure to all four JUCE breaking changes** (createEditor direct-call, AlertWindow::show,
  getPlatformSpecificData, VSTPluginFormatHeadless) — grep-proven, plus 0 ExtensionsVisitor. 61
  `createEditor()` matches are overrides (unaffected).
- **ANIRA→ORT coupling:** v2.0.3/v2.1.0 = ORT 1.19.2; v2.2.0/v2.2.1 = ORT 1.26.0. O-Texture hardcodes
  `ONNXRUNTIME_VERSION 1.19.2` — a v2.2.x bump forces that string to 1.26.0 (dylib-embed/rpath break
  otherwise); v2.1.0 is a zero-string drop-in.
- **Header-only:** nanoflann v1.6.2→v1.7.1 (scout's v1.10.0 does not exist upstream) and umappp
  v3.2.0→v3.3.2 are both drop-in for the actual API surface used (umappp removed `parallel_optimization`,
  but O-TextureForge doesn't reference it). pluginval 1.0.4 == latest.

## Deviations from Plan

**None functional.** Two evidence corrections surfaced against the scout notes and are documented in
both artifacts (not deviations from the plan's method, which explicitly told me to gather deeper
evidence): nanoflann's upstream latest is **v1.7.1**, not the scout's non-existent v1.10.0; and the
ANIRA-coupled ORT version for the latest tag is **1.26.0**, not the upstream-project 1.27.1. The
`juce_VST3ClientExtensions.h` module relocation was discovered (old path 404) and is a load-bearing
finding for the re-base plan.

## Verification

- `research/framework-updates-2026-07.md` — 8/8 required H2 sections present (verified by grep gate).
- `260719-hn4-EVIDENCE.md` — 7 H2 sections (≥6 gate satisfied).
- All five pinned deps covered; note-expression re-base cites concrete diff numbers; JUCE
  breaking-change verdicts cite grep counts.
- `git status` for my changes shows only the two new doc files staged/committed; **no JUCE / vendored /
  CMake / plugin build file modified by this task** (pre-existing working-tree changes were left untouched
  and never staged).

## Commits

- `317fead` docs(quick-260719-hn4): gather framework-update evidence
- `1d562b6` docs(quick-260719-hn4): framework-updates review + integration plan

## Self-Check: PASSED
- FOUND: research/framework-updates-2026-07.md
- FOUND: .planning/quick/260719-hn4-check-juce-and-framework-updates-review-/260719-hn4-EVIDENCE.md
- FOUND commit: 317fead
- FOUND commit: 1d562b6
