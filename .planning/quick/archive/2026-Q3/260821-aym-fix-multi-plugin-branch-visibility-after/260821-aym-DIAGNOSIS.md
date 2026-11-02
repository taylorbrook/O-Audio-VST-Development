# Diagnosis: multi-plugin branch visibility failure after /clear

**Date:** 2026-08-21
**Decision (user-approved):** Adopt trunk-based development — all plugin work on `main`. Do NOT build branch-aware machinery.

## Root cause

The Plugin Freedom System models plugin work as `(plugin, stage, phase)` resolved from
files at fixed paths (`plugins/<Name>/.planning/STATUS.md`). The one-branch-per-plugin +
worktree convention was added as prose in CLAUDE.md (commit 7849b31b) but the tooling was
never given the 4th coordinate — *location* (branch, worktree):

1. **No durable record.** STATUS-template.md has no `branch`/`worktree` field; PLUGINS.md
   has no branch column; `active-plugin.schema.json`, `checkpoint.schema.json`, and
   `plugin-workflow.input.json` all declare `additionalProperties: false` (branch field is
   schema-illegal).
2. **No reader.** `grep -rn "worktree" .claude/` → 0 executable hits. No command, skill,
   or hook runs `git branch --show-current`. O-Octagon's hand-added `branch:` frontmatter
   field had zero consumers.
3. **No injector after /clear.** Only `SessionStart.py` fires (dependency validation
   only); the second SessionStart hook matches `compact`, which /clear does not match.
4. **STATUS.md is branch-versioned.** Phase commits update STATUS.md on the feature
   branch, so main's copy goes stale while looking fresh (`last_updated` recent). A fresh
   session on main reads an older stage and silently redoes/clobbers finished work.

## Evidence of prior failures

- `84bca599` — O-Emulator Stage 2 phases ran in the main checkout while the plugin lived
  in `~/Dev/VST-development-emulator` on `feat/o-emulator-impl`; Stage-1 placeholders were
  fabricated; hand-reconciled later.
- 2026-08-19 — O-Bitrot and O-GrainScatter improve sessions in the shared checkout: shared
  index staged both plugins' files; HEAD switched mid-task; near-miss wrong-branch commit.
- `improve/o-spectralshaper-botanical` cut from `feat/o-octagon` — release stranded behind
  9 unrelated commits.
- O-simpleGrain v1.0.2/v1.1.0 — shipped source lost via uncommitted work + tree revert.

## Current repo state (favorable for migration)

`65cd3398` (2026-08-21) swept everything to `main`: 0 extra branches, 0 extra worktrees,
all in-flight work (O-Emulator stage 3 next) merged. Migration cost is zero right now.

## Remaining risk under trunk-based + mitigations

- **Shared-index race** between concurrent sessions in the single checkout (exists with or
  without branches): mitigate with path-scoped commits (`git commit -- plugins/<Name> PLUGINS.md`)
  and re-running `git branch --show-current` + `git status --short` immediately before
  every commit. Never trust session-start snapshots.
- **Per-plugin rollback** without branches: `backups/<Plugin>/vX.Y.Z/` snapshots + git tags
  + path-scoped reverts.
- **Exceptional branch use** (repo-wide risky refactors only): SessionStart hook will
  surface any non-main branch or extra worktree after every /clear, so no session is blind.

## Scope of the fix (agreed)

1. **CLAUDE.md** — rewrite the "Parallel Plugin Development" section (lines ~104–155) for
   trunk-based: all plugin work on main in the single checkout; concurrent-session commit
   discipline; branches only for exceptional repo-wide work (must be short-lived, and the
   hook surfaces them); condense the PLUGINS.md union-merge notes to the exceptional-branch
   case (no routine merges anymore).
2. **`.claude/hooks/SessionStart.py`** — append a "Git Context" section after validation:
   current branch (WARN if not main), `git worktree list` (WARN if >1), dirty/staged file
   count, unpushed-commit count vs origin/main, and an in-flight plugin scan (read
   `plugins/*/.planning/STATUS.md` frontmatter: plugin, stage/phase, status — only
   non-complete ones). Keep total added runtime well under the 5 s hook timeout; git
   commands are local and fast. Fail soft (never block session start).
3. **`.claude/skills/context-resume/SKILL.md` + `.claude/commands/continue.md`** — add a
   lightweight location check to the resume protocol: verify `git branch --show-current`
   == main; if not, STOP and surface the branch instead of proceeding.
4. **Cleanup:** remove the empty, unreferenced `.claude/worktrees/` directory.

Explicitly OUT of scope: schema changes, PLUGINS.md column changes, per-command
preconditions across the 12 phase commands, plugin-workflow SKILL.md orchestration changes
(unneeded under trunk-based).
