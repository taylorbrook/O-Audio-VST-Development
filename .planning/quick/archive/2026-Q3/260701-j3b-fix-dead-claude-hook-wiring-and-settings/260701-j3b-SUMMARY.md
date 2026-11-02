---
phase: quick-260701-j3b
plan: 1
subsystem: claude-automation
tags: [hooks, settings, security, agent-memory]
status: complete
requires: []
provides:
  - "agent-memory SubagentStart matcher aligned 1:1 with .claude/agent-memory/*.md files"
  - "env-gated CLAUDE_HOOK_DEBUG instrumentation on inject-agent-memory.py and task-validator-dispatch.py"
  - "de-fanged settings.local.json auto-approve allow-list (no recursive-delete wildcard)"
  - "cleaned .claude/hooks/ (two orphan scripts removed; CLI utilities retained)"
affects:
  - .claude/settings.json
  - .claude/hooks/inject-agent-memory.py
  - .claude/hooks/task-validator-dispatch.py
  - .claude/settings.local.json
tech-stack:
  added: []
  patterns: [env-gated-stderr-debug-echo, allowlist-subcommand-scoping]
key-files:
  created: []
  modified:
    - .claude/settings.json
    - .claude/hooks/inject-agent-memory.py
    - .claude/hooks/task-validator-dispatch.py
    - .claude/settings.local.json   # gitignored/untracked — edited on disk, not committed
  deleted:
    - .claude/hooks/Stop.py
    - .claude/hooks/UserPromptSubmit.py
decisions:
  - "BUG-02 resolved as VERIFY+INSTRUMENT, NOT rewire — SubagentStart/TaskCompleted are valid events in installed Claude Code 2.1.198"
  - "git add/commit/push remain subcommand-scoped (not path-scoped) by design across the 38-plugin monorepo"
  - "Both merge-critic-reports.py and detect-research-conflicts.py retained as live CLI utilities (review mislabelled them as orphans)"
metrics:
  duration: ~5min
  completed: 2026-07-01
  tasks: 3
  files: 5
status_note: complete
requirements: [BUG-02, BUG-03, BUG-04, IMP-01]
---

# Phase quick-260701-j3b Plan 1: Fix Dead Claude Hook Wiring and Settings Summary

Verified the two event hooks are correctly wired for the installed Claude Code build, instrumented both with env-gated debug echoes, aligned the agent-memory matcher 1:1 with existing memory files, removed a dangerous auto-approve wildcard, and deleted two stale orphan hook scripts.

## Installed Version / Premise Correction

- `claude --version` recorded: **2.1.198 (Claude Code)** (>= 2.1.197 — well above the SubagentStart introduction floor).
- **Premise correction (BUG-02):** The review's claim that `inject-agent-memory.py` (SubagentStart) and `task-validator-dispatch.py` (TaskCompleted) are wired to "non-existent" events reflected the OLD 9-event Anthropic hooks docs. The installed build registers `executeSubagentStartHooks` and supports `SubagentStart`/`TaskCompleted`, delivering exactly the payload fields these hooks read (`agent_type`; `task_subject`/`task_description`). Both hooks were therefore wired to REAL events with CORRECT input contracts. Rewiring would have BROKEN the contracts and permanently no-op'd the hooks — so BUG-02 was handled as **VERIFY + INSTRUMENT, not rewire**.

## What Was Done

### Task 1 — Verify + instrument event hooks; fix agent-memory matcher (BUG-02 + BUG-04) — commit `71e8fbd`
- `settings.json` SubagentStart matcher changed from
  `troubleshoot-agent|dsp-agent|gui-agent|research-planning-agent|validation-agent|research-lead`
  to `troubleshoot-agent|dsp-agent|gui-agent|research-planning-agent|validation-agent|dorico-agent`.
  Added `dorico-agent` (`.claude/agent-memory/dorico-agent.md` exists), removed `research-lead` (no memory file; real research agent `research-planning-agent` already listed). No other block touched. Event names left intact.
- `inject-agent-memory.py`: added `import os` + an env-gated (`CLAUDE_HOOK_DEBUG`) one-line **stderr** echo after `agent_type` resolution, naming the event, resolved `agent_type`, and whether a memory file was found. Kept stdout reserved for the `hookSpecificOutput` JSON contract.
- `task-validator-dispatch.py`: added an env-gated (`CLAUDE_HOOK_DEBUG`) one-line **stderr** echo immediately after `task_content` is assembled, before the non-code skip.
- Both hooks remain fail-safe; normal (env-unset) runs stay silent (verified).

### Task 2 — De-fang the auto-approve allow-list (BUG-03) — no commit (gitignored)
Edited `.claude/settings.local.json` on disk only (confirmed untracked/gitignored; not committed):
- Removed the blanket destructive entry `Bash(rm -rf *)` entirely (no narrower delete glob substituted — deletes now require per-command approval). This is the load-bearing security fix (threat T-j3b-01).
- Scoped over-broad globs to canonical subcommand/path form (T-j3b-02):
  - `Bash(git add *)` -> `Bash(git add:*)`
  - `Bash(git commit -m ' *)` -> `Bash(git commit:*)`
  - `Bash(git push *)` -> `Bash(git push:*)`
  - `Bash(cd *)` -> `Bash(cd /Users/taylorbrook/Dev/VST-development:*)`
- All other entries (including read-only `Bash(git check-ignore *)`) left untouched. File remains valid JSON, 41 entries, still untracked.
- **By design:** git add/commit/push are subcommand-scoped (not path-scoped) — path-scoping git across the 38-plugin monorepo would cause constant approval prompts. The security fix is removal of the recursive-delete wildcard.

### Task 3 — Clean stale orphan hook scripts; retain CLI utilities (IMP-01) — commit `cde44f5`
- Deleted `.claude/hooks/Stop.py` (obsolete pre-GSD PLUGINS.md construction-marker stage-commit convention; not wired to any event) and `.claude/hooks/UserPromptSubmit.py` (built on a wrong input contract — reads `os.environ["USER_PROMPT"]`, which Claude Code never sets; superseded by the `/continue` context-resume skill). Both were git-tracked; removed via `git rm`. Stale `__pycache__` entries for both (untracked) also removed.
- **Retained** both referenced CLI utilities under `hooks/` (review mislabelled BOTH as orphans — corrected here):
  - `merge-critic-reports.py` — invoked by `.claude/agents/critic-orchestrator.md`.
  - `detect-research-conflicts.py` — an argparse CLI (`--dir`/`--files`) invoked as Step 4 of the research-lead workflow (`.claude/agents/research-lead.md:64`) and documented in `research-team-protocol.md:57`. It is live code, not a stdin-driven event hook. Dropping `research-lead` from the SubagentStart memory matcher (Task 1) is unrelated — that concerns memory injection, not whether the agent is live.

## Verification

All automated checks passed:
- `claude --version` = 2.1.198; settings.json valid JSON; matcher includes `dorico-agent`, excludes `research-lead`; SubagentStart/TaskCompleted event blocks unchanged.
- Under `CLAUDE_HOOK_DEBUG=1`: `inject-agent-memory.py` emits stdout `additionalContext` (nested under `hookSpecificOutput`, the correct Claude Code contract) for `dsp-agent` and `dorico-agent`, plus a stderr debug line; emits NO injection for `research-lead` (no memory file). `task-validator-dispatch.py` prints a stderr debug line and exits 0 on a non-code task.
- Env-unset runs: both hooks silent on stderr (no per-event noise).
- `settings.local.json`: no `rm -rf` entry; git/cd scoped; valid JSON; still untracked.
- `.claude/hooks/`: Stop.py + UserPromptSubmit.py gone; merge-critic-reports.py + detect-research-conflicts.py present and still referenced; settings.json has no reference to any deleted script.

### Not covered by automated verify (manual, fresh-session)
Full runtime firing of SubagentStart/TaskCompleted can only be confirmed inside a fresh Claude Code session (synthetic-payload tests prove the SCRIPTS execute correctly under the documented schemas, not that the running CLI dispatches these events in this project's workflows). To confirm end-to-end: in a new session with `CLAUDE_HOOK_DEBUG=1` exported, spawn a matched subagent (e.g. dsp-agent) and confirm the inject-agent-memory stderr debug line appears and memory content is injected.

## Deviations from Plan

**1. [Verification-command imprecision — not a code defect] Task 1 additionalContext check reported False**
- **Found during:** Task 1 automated verify.
- **Issue:** The plan's verify one-liner checks `'additionalContext' in <top-level dict>`, but the hook (correctly) nests `additionalContext` under `hookSpecificOutput` per the Claude Code hook contract — so the top-level membership test printed `False`.
- **Resolution:** No code change needed — the hook output is correct. Re-ran a nesting-aware check (`'additionalContext' in d['hookSpecificOutput']`) which returned True for both dsp-agent and dorico-agent, satisfying the substantive done criterion.
- **Files modified:** none.

Otherwise plan executed as written. No Rule 1/2/3 code fixes required; no architectural (Rule 4) decisions; no authentication gates.

## Known Stubs
None.

## Threat Mitigations Applied
- **T-j3b-01** (high, Elevation/Destruction): removed `Bash(rm -rf *)` from the auto-approve allow-list — no destructive delete is auto-approved.
- **T-j3b-02** (medium, Tampering): scoped git to subcommands and cd to the project root, shrinking the unattended-exec surface.
- **T-j3b-03** (low, Info Disclosure — accepted): matcher aligned 1:1 with existing memory files (added dorico-agent, dropped research-lead).

## Commits
- `71e8fbd` fix(quick-260701-j3b): verify+instrument event hooks; fix agent-memory matcher
- `cde44f5` chore(quick-260701-j3b): remove stale orphan hook scripts (IMP-01)
- Task 2 (settings.local.json): intentionally NOT committed (gitignored/untracked; edited on disk).

## Self-Check: PASSED
- settings.json matcher verified on disk (dorico-agent in, research-lead out).
- Both hook instrumentation edits verified via synthetic-payload runs.
- settings.local.json de-fanged and confirmed still untracked.
- Commits 71e8fbd and cde44f5 confirmed in git log; each contains ONLY this task's files.
- Stop.py and UserPromptSubmit.py confirmed absent; both CLI utilities confirmed present + referenced.
