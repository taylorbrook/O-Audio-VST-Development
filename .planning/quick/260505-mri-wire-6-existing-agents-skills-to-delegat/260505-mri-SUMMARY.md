---
quick_id: 260505-mri
mode: quick
status: complete
description: Wire 6 existing agents/skills/commands to delegate Dorico-specific work to dorico-agent (insertion-only routing/cross-reference subsections).
started: 2026-05-05T23:51:55Z
completed: 2026-05-05T23:55:10Z
duration: ~3m
task_count: 6
commits:
  - dc1cfe2  # Task 1: troubleshoot-agent.md
  - 2b6e112  # Task 2: plugin-improve/SKILL.md
  - 3b73f5d  # Task 3: plugin-publishing/SKILL.md
  - 85c8abe  # Task 4: generalize-microtones.md
  - 9771a12  # Task 5: gui-agent.md
  - 2f66c40  # Task 6: dsp-agent.md
files_modified:
  - .claude/agents/troubleshoot-agent.md
  - .claude/skills/plugin-improve/SKILL.md
  - .claude/skills/plugin-publishing/SKILL.md
  - .claude/commands/generalize-microtones.md
  - .claude/agents/gui-agent.md
  - .claude/agents/dsp-agent.md
decisions_honored:
  - D1: "## Dorico Delegation" subsection format (numbered-step variant for plugin-publishing)
  - D2: plugin-publishing SMOKE-TEST is advisory only (does NOT block release)
  - D3: generalize-microtones Phase B sequential — one Task() per plugin, halt-on-failure
---

# Quick Task 260505-mri — SUMMARY

## One-liner

Inserted Dorico-routing subsections into 6 agent/skill/command files so they delegate Dorico-specific work (microtonal regression, expression-map editing, smoke-tests, KS-routing, NE-tuning) to the new `dorico-agent` (created in 260505-ayr) — insertion-only, one atomic commit per file, six commits total.

## What Changed

| # | File | Insertion | Commit |
|---|------|-----------|--------|
| 1 | `.claude/agents/troubleshoot-agent.md` | `## Dorico Delegation` after Responsibilities list — "return to invoker" handoff (no Task tool). | `dc1cfe2` |
| 2 | `.claude/skills/plugin-improve/SKILL.md` | `## Dorico Delegation` between Phase 0.5 and Phase 0.6 — direct `Task(dorico-agent, ...)` invocation. | `2b6e112` |
| 3 | `.claude/skills/plugin-publishing/SKILL.md` | `### 4b. Microtonal Cohort SMOKE-TEST (Advisory)` between Step 4 and Step 5 — D2 advisory-only semantics. | `3b73f5d` |
| 4 | `.claude/commands/generalize-microtones.md` | `### 2a. Dorico Delegation (Phase B per-plugin)` between Step 2 and Step 3 — D3 sequential pattern (8-plugin cohort). | `85c8abe` |
| 5 | `.claude/agents/gui-agent.md` | `## Dorico Delegation` between `</required_reading>` and `<template_library>` — KS-routing Layer 2 cross-reference (NOT a delegation rule). | `9771a12` |
| 6 | `.claude/agents/dsp-agent.md` | `## Dorico Delegation` between `</required_reading>` and `<complexity_aware>` — NE-tuning trigger-order cross-reference (NOT a delegation rule). | `2f66c40` |

## Verification

All per-task verify commands from PLAN.md passed:

- **Task 1:** 1× `## Dorico Delegation` heading; 2× "Return to invoker recommending"; correct anchor adjacency.
- **Task 2:** 1× `## Dorico Delegation` heading; 1× `subagent_type="dorico-agent"`; section order Phase 0.5 → Dorico Delegation → Phase 0.6 confirmed (lines 267 → 281 → 293).
- **Task 3:** 1× `### 4b. Microtonal Cohort SMOKE-TEST (Advisory)` heading; 1× "advisory — does NOT block release" (D2 carried); 1× "O-MicrotonalSampler" (cohort landed); section order `### 4.` → `### 4b.` → `### 5.` confirmed (lines 145 → 167 → 190).
- **Task 4:** 1× `### 2a. Dorico Delegation (Phase B per-plugin)` heading; 1× "sequential — one Task() per plugin" (D3 carried); 2× "O-IntonationPad" (8-plugin cohort landed); section order `### 2.` → `### 2a.` → `### 3.` confirmed (lines 25 → 40 → 57).
- **Task 5:** 1× `## Dorico Delegation` heading; 1× "Cross-reference (NOT a delegation rule)"; 1× "Layer 2 of the 3-layer Dorico keyswitch routing stack"; 1× "return to invoker"; tag adjacency `</required_reading>` → `## Dorico Delegation` → `<template_library>` confirmed (lines 143 → 145 → 160).
- **Task 6:** 1× `## Dorico Delegation` heading; 1× "Cross-reference (NOT a delegation rule)"; 1× ``BEFORE the DSP model's `trigger`` (load-bearing trigger-order rule); 1× "Return to invoker"; tag adjacency `</required_reading>` → `## Dorico Delegation` → `<complexity_aware>` confirmed (lines 165 → 167 → 183).

## Cross-cutting Integrity

- **Insertion-only.** No body rewrites in any file. Every edit was a single `Edit` tool call appending a new subsection between two existing structural anchors.
- **Handoff phrasing varies correctly by tool availability:**
  - Files WITHOUT Task tool (troubleshoot-agent, gui-agent, dsp-agent) → "Return to invoker recommending..." phrasing.
  - Files WITH Task tool / running in main agent context (plugin-improve, plugin-publishing, generalize-microtones) → Direct `Task(subagent_type="dorico-agent", ...)` syntax.
- **Cohort distinction preserved.** plugin-publishing uses 7-plugin cohort (includes O-MicrotonalSampler, no O-IntonationPad/O-Prism). generalize-microtones uses 8-plugin cohort (includes O-IntonationPad and O-Prism, no O-MicrotonalSampler). NOT unified.
- **Atomic commits.** One file per commit, six commits in canonical order on top of the planning commit `3a080c4`.

## Deviations from Plan

None — plan executed exactly as written. All anchors matched verbatim on first attempt; no auto-fixes (Rules 1-3) needed; no architectural questions (Rule 4) raised.

## Self-Check: PASSED

**Files:**
- FOUND: `.claude/agents/troubleshoot-agent.md`
- FOUND: `.claude/skills/plugin-improve/SKILL.md`
- FOUND: `.claude/skills/plugin-publishing/SKILL.md`
- FOUND: `.claude/commands/generalize-microtones.md`
- FOUND: `.claude/agents/gui-agent.md`
- FOUND: `.claude/agents/dsp-agent.md`

**Commits:**
- FOUND: `dc1cfe2` (Task 1)
- FOUND: `2b6e112` (Task 2)
- FOUND: `3b73f5d` (Task 3)
- FOUND: `85c8abe` (Task 4)
- FOUND: `9771a12` (Task 5)
- FOUND: `2f66c40` (Task 6)
