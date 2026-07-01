---
phase: 260701-in8
plan: 01
subsystem: system-review
tags: [review, pfs, automation, ci, modules, cmake, hooks]
requires: []
provides:
  - artifact: .planning/quick/260701-in8-complete-a-review-of-this-system-for-cre/260701-in8-REVIEW.md
affects: []
tech-stack:
  added: []
  patterns: [read-only-audit, severity-ranked-review]
key-files:
  created:
    - .planning/quick/260701-in8-complete-a-review-of-this-system-for-cre/260701-in8-REVIEW.md
  modified: []
decisions:
  - "Read-only review — no source/script/config/plugin file modified; only REVIEW.md written."
  - "Scope limited to the SYSTEM (scripts, .claude, modules, CMake, CI); plugin DSP treated only as drift signal."
metrics:
  duration: ~11min
  completed: 2026-07-01
status: complete
requirements: [REVIEW-01]
---

# Quick Task 260701-in8: Plugin Freedom System Review Summary

Severity-ranked read-only review of the Plugin Freedom System (agentic workflow +
build/CI/module infrastructure) delivered as a single REVIEW.md with 6 bugs, 5
improvements, and 6 update recommendations — each carrying `file:line` evidence.

## What Was Done

- Inspected the automation/orchestration layer: `scripts/` (build-and-install.sh/.ps1,
  apply-juce-patches.sh, verify-au-link.sh), `.github/workflows/build-and-release.yml`,
  and `.claude/` (settings, hooks, agents, agent-memory, skills, commands, artifacts).
- Reconciled `modules/registry.yaml` against on-disk consumers (grep of plugin CMakeLists /
  module tokens), quantified WebView2 flag coverage, and confirmed the JUCE patch filename mismatch.
- Synthesized findings into three deliverable sections (Bugs / Improvement Suggestions /
  Update Recommendations) with an executive summary and Section A/B detail appendix.

## Top Findings

**Bugs**
- BUG-01 (HIGH) — CI CMake-target resolver (`build-and-release.yml:87,439`) can't resolve
  `${PROJECT_NAME}` targets → `O-Texture` release breaks; local script already handles it (drift).
- BUG-02 (HIGH) — `settings.json:68,80` wire hooks to invalid events `SubagentStart`/`TaskCompleted`
  → agent-memory injection + task validation never fire (silently dead).
- BUG-03 (HIGH) — `settings.local.json:12` auto-approves `Bash(rm -rf *)`.
- BUG-04 (MEDIUM) — inject-agent-memory matcher/agent-memory-file mismatch (dorico-agent excluded; research-lead has no memory).
- BUG-05 (MEDIUM) — `apply-juce-patches.sh:66` partial-apply reapply risk (no `--forward` guard).
- BUG-06 (LOW) — `build-and-install.sh:2` missing `set -u`/`pipefail`.

**Update Recommendations**
- UPD-01 (HIGH) — JUCE patch filename `...8.0.4.patch` holds 8.0.9 content (refs at apply-juce-patches.sh:22, registry:288).
- UPD-02 (HIGH) — `registry.yaml` `used_by` pervasively stale (retired folder names; `scala-tuning-engine used_by: []` vs 12 real consumers; preset-manager 3-vs-19; note-expression omits 3).
- UPD-03 (MEDIUM) — registry header `version 1.0.0`/`last_updated 2026-01-14` frozen.

**Positive** — WebView2 gap closed: 38/38 real plugins set BOTH required Windows flags
(historical 34/35-missing regression class resolved). `plugins/tache_plugins/` (18 stray
non-Ouaricon dirs) is the reason a raw count reports 39 vs 38 real plugins.

## Deviations from Plan

None — plan executed exactly as written (3 tasks, read-only, single REVIEW.md deliverable).

## Constraints Honored

- No source/script/config/plugin file modified (only REVIEW.md created).
- No git commits created (per task constraints; docs commit left to orchestrator).
- WIP files on `improve/o-formant-v1.25.1-rt-safety` and the `plugins/O-Orbit/libs/SAF` submodule left untouched.

## Self-Check: PASSED

- REVIEW.md exists at output path — FOUND.
- Task 1/2/3 automated verifications — all PASS (Section A, Section B, three deliverable sections, IDs, file:line tokens present).
- `git status --porcelain` shows only the untracked quick-task dir as this task's output; no inspected file staged/modified by this task.
