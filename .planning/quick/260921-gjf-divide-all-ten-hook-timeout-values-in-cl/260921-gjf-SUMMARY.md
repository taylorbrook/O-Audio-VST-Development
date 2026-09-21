---
phase: 260921-gjf
plan: 01
subsystem: claude-tooling
status: complete
quick_task: true
tags: [config, hooks, claude-code, bugfix]
requires: []
provides:
  - "Hook timeout budgets in .claude/settings.json expressed in the schema's documented unit (seconds)"
affects:
  - ".claude/settings.json"
tech_stack:
  added: []
  patterns: []
key_files:
  created: []
  modified:
    - .claude/settings.json
decisions:
  - "Applied ten scoped Edit calls anchored on the preceding `command` line rather than a jq round-trip, preserving byte-for-byte formatting outside the ten changed lines (diff is exactly 10/10)."
  - "T-gjf-02 accepted as planned: hooks are now subject to a real wall-clock ceiling. If one is later observed being killed, raise that individual value rather than reverting the unit."
metrics:
  duration: 3min
  completed: 2026-09-21
actuals:
  tokens: 1200
  tasks: 2
  commits: 1
plan_head_before: 29621020979cc259aebc69b25d6260d6e745eb3f
---

# Quick Task 260921-gjf: Hook Timeout Unit Fix Summary

Corrected the unit error in all ten `hooks[].hooks[].timeout` values in `.claude/settings.json` — the schema documents seconds, the file carried millisecond-shaped values, so `SubagentStop.py` was declaring an 8.3-hour budget instead of 30 seconds and a hung hook would never have been reaped.

## What Changed

`.claude/settings.json` — ten integer `timeout` values divided by 1000:

| Hook script | Before | After |
|---|---|---|
| `SessionStart.py` | 5000 | 5 |
| `PostCompact-SessionStart.py` | 5000 | 5 |
| `PostToolUse.py` | 2000 | 2 |
| `validators/validate-research-frontmatter.py` | 5000 | 5 |
| `regenerate-manifest.py` | 10000 | 10 |
| `PreCompact.py` | 10000 | 10 |
| `inject-agent-memory.py` | 3000 | 3 |
| `task-validator-dispatch.py` | 15000 | 15 |
| `SubagentStop.py` | 30000 | 30 |
| `write-back-agent-memory.py` | 10000 | 10 |

Nothing else was touched: `$schema`, every `command` string, every `matcher`, and `enabledPlugins` are byte-identical. No reformatting, no jq round-trip — each of the ten replacements was anchored on the `"command"` line immediately above its target, since several targets shared the same numeric value.

## Tasks Completed

| Task | Name | Commit |
|---|---|---|
| 1 | Divide the ten hook timeout values by 1000 | 4817fefd |
| 2 | Commit path-scoped, excluding the unrelated dirty file | 4817fefd |

Task 1 produced the working-tree edit; Task 2 committed it. One commit total, as the plan intended.

## Verification

Task 1 gate (whole-array equality + integer-type check + `10\t10` numstat):
```
GATE_PASS
```

Task 2 gate (branch is `main`, tip commit contains exactly one file and it is `.claude/settings.json`, the unrelated file is still uncommitted, no residual diff):
```
GATE_PASS
```

Plan verification block:
```
--- jq ---
[5,5,2,5,10,10,3,15,30,10]
--- show --stat ---
commit 4817fefdd0d07879bbec31abe315ea7d96de3d71
Author: Taylor Brook <52675429+taylorbrook@users.noreply.github.com>
Date:   Mon Sep 21 11:58:53 2026 -0700

    fix(claude): correct hook timeout unit from milliseconds to seconds

 .claude/settings.json | 20 ++++++++++----------
 1 file changed, 10 insertions(+), 10 deletions(-)
--- status ---
 M .claude/agent-memory/research-planning-agent.md
```

All three expectations met: the array reads `[5,5,2,5,10,10,3,15,30,10]`, the diff is exactly 10 insertions / 10 deletions in one file, and the pre-existing `.claude/agent-memory/research-planning-agent.md` modification is still present and uncommitted — proving the path-scoped commit did not sweep it in.

## Preconditions

Task 1's precondition held: the live file read `[5000,5000,2000,5000,10000,10000,3000,15000,30000,10000]`, so no concurrent session had already applied the edit and the plan's authorized scope matched the file on disk.

Task 2's precondition held: `git status --short` showed the pre-existing unrelated modification to `.claude/agent-memory/research-planning-agent.md`. Per CLAUDE.md commit discipline, `git branch --show-current` and `git status --short` were re-run immediately before the commit (not from a session-start snapshot); staging was clean of foreign content, so the pathspec commit was safe to make.

## Deviations from Plan

None — plan executed exactly as written.

## Threat Notes

`T-gjf-01` (DoS via an unreapable hook) is the defect this task fixed. `T-gjf-03` (tampering via the shared `.git/index`) was mitigated as specified: pathspec commit, no `git add`, no `-a`, with the immediately-preceding re-check. `T-gjf-02` remains accepted — hooks are now subject to a real ceiling.

No new security-relevant surface was introduced, so no threat flags.

## Self-Check: PASSED

- `.claude/settings.json` exists and parses as JSON (verified by the `jq -e` gates).
- Commit `4817fefd` exists in `git log` (`git show --stat HEAD` resolved it).
</content>
</invoke>
