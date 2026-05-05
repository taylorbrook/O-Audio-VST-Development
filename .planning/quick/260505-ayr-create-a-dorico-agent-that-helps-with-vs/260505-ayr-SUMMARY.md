---
quick_id: 260505-ayr
description: Dorico integration subagent + /dorico slash command + seed persistent-memory file
completed: 2026-05-05
plan_type: scaffolding
tasks_total: 3
tasks_completed: 3
files_created:
  - .claude/agents/dorico-agent.md
  - .claude/commands/dorico.md
  - .claude/agent-memory/dorico-agent.md
files_modified: []
key_decisions:
  - Slash command at .claude/commands/dorico.md (NOT a SKILL.md wrapper) — single-subagent delegation needs no skill orchestration
  - Frontmatter is the canonical 5-key shape (no model: key) — matches troubleshoot/gui/dsp/validation agents
  - Memory file shipped day-one with 4+3 seed entries — three Dorico landmines already in user-memory; making the agent re-derive them on first run wastes a session
  - color: orange — distinguishable from existing purple/green/yellow/blue
commits:
  - fa420ac: feat(dorico-agent): create Dorico integration subagent definition
  - ca878d9: feat(dorico-agent): add /dorico slash command wrapper
  - 2a5c8c4: feat(dorico-agent): seed persistent-memory file with day-one entries
---

# Quick Task 260505-ayr: Dorico Agent — Summary

## One-liner

Shipped a write-capable Dorico-integration subagent (subagent definition + /dorico slash command + seed persistent-memory file) covering the full Dorico stack: microtonal/note-expression playback, Playback Templates, EndpointConfigs, expression maps, the 3-layer keyswitch routing stack, CC/PC technique triggers, dynamics audit, and `.doricolib` distribution.

## What was built

### 1. `.claude/agents/dorico-agent.md` (commit `fa420ac`)

Single subagent definition file, 219 lines. Frontmatter is the canonical 5-key shape (`name`, `description`, `tools`, `color` — no `model:` key) matching the four existing agent files.

**Tool surface:** `Read, Edit, Write, Bash, Grep, Glob, WebSearch, WebFetch, mcp__context7__resolve-library-id, mcp__context7__get-library-docs` — write-capable diagnose+advise+edit per CONTEXT.md locked decision.

**Body sections (in plan order):**

1. Title + 2-paragraph purpose
2. `<scope>` — full Dorico stack coverage from CONTEXT.md
3. `<entry_protocol>` — 5-item read list (own memory + 3 user-memory landmine docs + plugin's `Resources/dorico/` tree)
4. `## Dorico Reference Map` — knowledge map from research §4 (verbatim)
5. `<known_landmines>` — 12-item table from research §6 (verbatim)
6. `<workflow>` — 4-level graduated investigation protocol (Level 1 landmine match → Level 2 plugin artifacts → Level 3 canonical-reference diff → Level 4 escalate)
7. `<capabilities>` — explicit edit surface (Dorico XML + plugin C++ + CMake) + shell command surface (build, install, cache reset, redeploy, git history)
8. `<output_contract>` — DIAGNOSIS / ROOT CAUSE / FIX APPLIED / VERIFICATION STEPS (cite TC-1..TC-5) / REMAINING RISKS, with file:line-range edit format
9. `<persistent_memory>` — append-on-completion protocol mirroring troubleshoot-agent (80-line cap, drop oldest 20)

### 2. `.claude/commands/dorico.md` (commit `ca878d9`)

Thin slash-command wrapper, 75 lines. Frontmatter (3 keys: `name`, `description`, `argument-hint: <PluginName> [question-or-task]`) mirrors `install-plugin.md`.

**Body sections:**

- `# /dorico` title + 1-sentence purpose
- `<preconditions>` — must be at project root; `<PluginName>` must resolve under `plugins/`
- `<routing>` — Task-tool spawn of `dorico-agent` with parsed `<plugin>` + `<task>`; explicit instruction NOT to pre-load Dorico XML/memory contents into the spawn prompt (agent does that itself)
- `<background_info>` — 8 lines on what the agent covers + typical use cases + canonical reference pointer
- `<examples>` — 3 invocations: O-MicrotonalSampler TC-4 regression, O-Lyrica distribution-bundle authoring, O-Reed KS-not-firing

### 3. `.claude/agent-memory/dorico-agent.md` (commit `2a5c8c4`)

Day-one seed memory file, 17 lines. Format mirrors all 5 existing `.claude/agent-memory/*.md` files.

- 4 `## Learned Patterns` entries (verbatim from research §5):
  1. General (RECURRING REGRESSION) — top-level microtonal fields
  2. General — standalone `.doricoexpmap` silent skip
  3. General — 3-layer KS routing
  4. O-Lyrica — auval DEF-24-01 benign
- 3 `## Common Issues` entries (verbatim from research §5):
  - TC-4 reveals top-level-fields regression
  - "invalid file format" = .doricolib XML structurally invalid
  - Library Manager has no Import Expression Map
- `## Last Updated` stamp: `2026-05-05 (seeded from critical_dorico_*.md and v1.16.6 incident)`

## Verification

All Task done-conditions from PLAN.md pass:

| Task | Check | Result |
|---|---|---|
| 1 | File exists, non-empty | PASS |
| 1 | 5-key frontmatter (no model:) | PASS — `name`, `description`, `tools`, `color`, no `model:` |
| 1 | 12 landmines present | PASS — `grep -c '^\| [0-9]\+ \|'` returns 12 |
| 1 | 4 entry-protocol files resolve | PASS — troubleshoot-agent.md + 3 user-memory `.md` files all exist |
| 2 | File exists | PASS |
| 2 | 3 frontmatter keys | PASS — `name`, `description`, `argument-hint` |
| 2 | `<routing>` references `dorico-agent` | PASS |
| 3 | File exists | PASS |
| 3 | ≥7 bullet entries | PASS — 7 entries (4 patterns + 3 issues) |
| 3 | 3 section headers | PASS — Learned Patterns, Common Issues, Last Updated |

## Deviations from Plan

None — plan executed exactly as written. All 3 tasks completed, all done-criteria met, all commit messages follow the conventional format. The CONTEXT and RESEARCH files were read from the main repository tree (not the worktree) since this worktree was instantiated from a base commit before the planning artifacts existed; this matched the orchestrator's expected invocation pattern.

## Out-of-scope follow-up tasks

Per RESEARCH §7, six existing agents/skills should learn to delegate to `dorico-agent`. **Not part of this quick task.** User can spin these up separately:

1. `.claude/agents/troubleshoot-agent.md` — recommend dorico-agent on Dorico symptoms
2. `.claude/skills/plugin-improve/SKILL.md` — Dorico-tagged bug routing
3. `.claude/skills/plugin-publishing/SKILL.md` — pre-release SMOKE-TEST.md validation
4. `.claude/commands/generalize-microtones.md` — Phase B per-plugin delegation
5. `.claude/agents/gui-agent.md` — Stage 3 KS-routing parameter cross-reference
6. `.claude/agents/dsp-agent.md` — note-expression tuning/trigger order callout

## Self-Check: PASSED

- File: `.claude/agents/dorico-agent.md` — FOUND
- File: `.claude/commands/dorico.md` — FOUND
- File: `.claude/agent-memory/dorico-agent.md` — FOUND
- Commit `fa420ac` — FOUND
- Commit `ca878d9` — FOUND
- Commit `2a5c8c4` — FOUND
</content>
</invoke>
