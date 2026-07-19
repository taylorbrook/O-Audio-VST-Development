---
phase: quick-260719-iun
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - plugins/O-Bells/**
  - plugins/O-Contrabass/**
  - plugins/O-GrainScatter/**
  - plugins/O-Tremolo/**
  - plugins/O-Bowed/**
  - plugins/O-Detune/Source/ui/public/modules/preset-manager.js
  - .claude/commands/improve-review.md
  - .claude/commands/improve-verify.md
  - .claude/agent-memory/dorico-agent.md
  - .planning/workflow/active-plugin.json
  - .planning/workflow/registry.json
  - .github/workflows/build-and-release.yml
autonomous: true
requirements:
  - CLEANUP-01 (clean + pushed working tree — framework-update precondition gate)
must_haves:
  truths:
    - "git status --porcelain shows no dirty PLUGIN or source files (only this quick task's .planning/quick/260719-iun artifacts may remain)"
    - "HEAD == origin/main: git rev-list --left-right --count origin/main...HEAD returns 0 0"
    - "Every commit message reflects the actual diff content (version bumps named from the CHANGELOG being committed)"
    - "O-Bells deleted footer-panel files (instrument-footer-panel.css/.js) are committed as deletions within the O-Bells group, not restored"
    - "Untracked O-Contrabass dirs (Resources/, Source/ui/, .planning/stages/*, tests/ui_frontend_check.js) are git add-ed and committed with the O-Contrabass group"
  artifacts:
    - ".planning/quick/260719-iun-triage-dirty-working-tree-commit-uncommi/260719-iun-SUMMARY.md (records every commit SHA + any explicit discards with justification)"
  key_links:
    - "git push origin main succeeds (local is 8 ahead / 0 behind — plain push, no rebase)"
    - "No file under research/framework-updates* is touched and no framework/JUCE version bump is committed"
---

<objective>
Triage the dirty working tree (~70 entries across 6 plugins + repo-level meta files), commit it in logical atomic groups (one commit per plugin/version-bump unit; meta files grouped sensibly), then `git push origin main` so `HEAD == origin/main` and no dirty plugin/source files remain.

This is the precondition gate from `research/framework-updates-2026-07.md`: NO JUCE/CMake/vendored-framework change may begin until the tree is clean and pushed.

Purpose: Unblock the framework-update work by clearing accumulated in-flight plugin changes safely, without losing intended work.
Output: A sequence of atomic commits on `main`, pushed to origin, plus a SUMMARY.md recording every commit SHA and any explicit discards.
</objective>

<execution_context>
@$HOME/.claude/gsd-core/workflows/execute-plan.md
@$HOME/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@./CLAUDE.md

# Pre-computed facts (verify against live diffs before committing — do not trust blindly):
# CHANGELOG top versions: O-Bells 4.1.1 | O-Contrabass 1.0.0 (first release) | O-GrainScatter 2.4.2 | O-Tremolo 1.6.0 | O-Bowed 1.4.1
# O-Detune change = preset-manager.js module v1.0.4 rollout only
# Untracked dirs checked at plan time: NO .DS_Store / build artifacts / .o present
# Branch: local main is 8 ahead, 0 behind origin/main → plain `git push origin main` suffices
</context>

<constraints>
- Executor runs SEQUENTIALLY on the main working tree. NO worktree isolation — these dirty changes only exist here. Commits land directly on `main`.
- INSPECT each group's diff before committing (`git diff -- <paths>`, `git diff --stat`, tail the CHANGELOG) so the commit message names the actual version bump and content.
- DISCARD policy is CONSERVATIVE: when in doubt, COMMIT. Only discard a hunk with clear evidence it is stale auto-regenerated noise. Every discard MUST be listed explicitly in SUMMARY.md with justification.
- Deleted files (O-Bells `instrument-footer-panel.css` / `.js`) are part of the O-Bells change set — commit the deletions, do NOT restore.
- Untracked dirs must be `git add`-ed with their plugin group. Confirm no junk (`.DS_Store`, build artifacts) before adding (checked clean at plan time; re-confirm).
- Do NOT touch anything under `research/framework-updates*` and do NOT begin any framework/JUCE upgrade. If a meta file's diff IS a framework/version bump, that is the one exception to "leave nothing dirty": FLAG it, leave it uncommitted, note in SUMMARY — do not commit and do not discard.
- Use `git add -- <explicit paths>` per group. NEVER `git add -A` / `git add .` — that would cross group boundaries and sweep the .planning/quick/260719-iun artifacts into a plugin commit.
</constraints>

<tasks>

<task type="auto">
  <name>Task 1: Preflight audit — inspect all groups, confirm no junk, record version bumps + discard candidates</name>
  <files>(read-only inspection; no file writes)</files>
  <action>
Run `git status --porcelain` to re-enumerate the full dirty set. For EACH of the following groups, run `git diff -- <group paths>` (and `git diff --stat`) plus tail the plugin's CHANGELOG.md to confirm the intended version bump and that the diff is coherent (not partial/corrupt):

Plugin groups (one atomic commit each):
1. plugins/O-Bells/** — modifications + DELETED Resources/ui/css/instrument-footer-panel.css and Resources/ui/modules/instrument-footer-panel.js + UNTRACKED plugins/O-Bells/CODE_REVIEW.md. Confirm CHANGELOG top entry (expect 4.1.1).
2. plugins/O-Contrabass/** — modifications to .planning/{REQUIREMENTS,STATUS,parameter-spec}.md, CHANGELOG, CMakeLists, NOTES, Source/{BowedContrabassVoice,PluginEditor,PluginProcessor}, tests/render-harness/* PLUS untracked Resources/, Source/ui/, .planning/stages/{2-dsp,3-gui,4-polish}/, tests/ui_frontend_check.js. Confirm CHANGELOG top entry (expect 1.0.0 first release).
3. plugins/O-GrainScatter/** — CHANGELOG, CMakeLists, CODE_REVIEW.md (tracked), NOTES, PluginEditor.cpp, PluginProcessor.cpp, dsp/{FreezeManager,GrainPool,TempoTracker}.h, ui/public/index.html. Confirm CHANGELOG (expect 2.4.2).
4. plugins/O-Tremolo/** — CHANGELOG, CMakeLists, NOTES, PluginEditor.{cpp,h}, PluginProcessor.{cpp,h}, ui/public/index.html. Confirm CHANGELOG (expect 1.6.0).
5. plugins/O-Bowed/ — CMakeLists.txt + CHANGELOG.md only. Confirm CHANGELOG (expect 1.4.1).
6. plugins/O-Detune/Source/ui/public/modules/preset-manager.js — confirm this is the preset-manager module v1.0.4 rollout (compare to the module source).

Repo-level / meta groups:
7. .claude/commands/improve-review.md + .claude/commands/improve-verify.md (both UNTRACKED, new).
8. .planning/workflow/active-plugin.json + .planning/workflow/registry.json + .claude/agent-memory/dorico-agent.md (workflow state + agent memory).
9. .github/workflows/build-and-release.yml — inspect carefully. If it is a CI-config change with NO framework/JUCE version bump, it commits as a ci chore. If it DOES change a framework/JUCE version pin, FLAG it and exclude from all commits (see constraints).

For untracked dirs (O-Contrabass Resources/, Source/ui/, .planning/stages/*): run `find <dir> -name '.DS_Store' -o -name '*.o' -o -path '*/build/*'` to reconfirm no junk before they are added in Task 2.

Record: confirmed version bump per plugin, any discard candidates (with evidence), and whether build-and-release.yml is safe to commit or must be flagged. This audit output feeds the commit messages in Task 2/3 and the SUMMARY.
  </action>
  <verify>
    <automated>git status --porcelain | wc -l  # non-zero count of entries to triage; audit ran</automated>
  </verify>
  <done>Every group's diff inspected; version bumps confirmed against CHANGELOG tails; no junk found in untracked dirs; build-and-release.yml classified (safe-to-commit vs framework-flag); discard candidates (if any) identified with evidence.</done>
</task>

<task type="auto">
  <name>Task 2: Commit the 6 plugin groups atomically (one commit per plugin/version-bump unit)</name>
  <files>plugins/O-Bells/**, plugins/O-Contrabass/**, plugins/O-GrainScatter/**, plugins/O-Tremolo/**, plugins/O-Bowed/**, plugins/O-Detune/Source/ui/public/modules/preset-manager.js</files>
  <action>
For each plugin group, `git add -- <explicit plugin paths ONLY>` then commit. NEVER `git add -A`. Include tracked modifications, deletions, AND untracked files/dirs for that plugin in the same commit. Use the version bump confirmed in Task 1 in the message.

1. O-Bells: `git add -- plugins/O-Bells` (captures modifications, the two footer-panel DELETIONS, and untracked CODE_REVIEW.md). Verify `git status --porcelain plugins/O-Bells` shows the deletions staged as `D`. Commit e.g. `improve: O-Bells v4.1.1 — <summary from CHANGELOG> (DSP + tuning-panel + footer-panel removal + code review)`.
2. O-Contrabass: `git add -- plugins/O-Contrabass` (captures .planning edits, source, render-harness, AND untracked Resources/ Source/ui/ .planning/stages/* tests/ui_frontend_check.js). Commit e.g. `feat: O-Contrabass v1.0.0 — first release (engine + WebView editor + polish + Dorico resources)`.
3. O-GrainScatter: `git add -- plugins/O-GrainScatter`. Commit e.g. `improve: O-GrainScatter v2.4.2 — <summary> (DSP + editor + code review)`.
4. O-Tremolo: `git add -- plugins/O-Tremolo`. Commit e.g. `improve: O-Tremolo v1.6.0 — <summary>`.
5. O-Bowed: `git add -- plugins/O-Bowed`. Commit e.g. `chore: O-Bowed v1.4.1 — CMakeLists + CHANGELOG` (adjust verb if CHANGELOG shows a behavioral change).
6. O-Detune: `git add -- plugins/O-Detune/Source/ui/public/modules/preset-manager.js`. Commit e.g. `chore: O-Detune — sync preset-manager module v1.0.4`.

After each commit, run `git status --porcelain plugins/<Name>` and confirm that plugin's tree is clean before moving to the next.
  </action>
  <verify>
    <automated>for p in O-Bells O-Contrabass O-GrainScatter O-Tremolo O-Bowed O-Detune; do c=$(git status --porcelain plugins/$p | wc -l); echo "$p dirty=$c"; done</automated>
  </verify>
  <done>All 6 plugin trees report dirty=0. O-Bells footer-panel files committed as deletions. O-Contrabass untracked dirs committed. Each commit message names the correct version bump verified against the CHANGELOG.</done>
</task>

<task type="auto">
  <name>Task 3: Commit meta groups, push origin main, verify clean + synced</name>
  <files>.claude/commands/improve-review.md, .claude/commands/improve-verify.md, .planning/workflow/active-plugin.json, .planning/workflow/registry.json, .claude/agent-memory/dorico-agent.md, .github/workflows/build-and-release.yml</files>
  <action>
Commit repo-level / meta groups (explicit paths only, no `git add -A`):

7. New slash commands: `git add -- .claude/commands/improve-review.md .claude/commands/improve-verify.md` → `feat: add /improve-review + /improve-verify slash commands`.
8. Workflow state + agent memory: `git add -- .planning/workflow/active-plugin.json .planning/workflow/registry.json .claude/agent-memory/dorico-agent.md` → `chore: sync workflow state + dorico-agent memory`. (If the dorico-agent.md diff is a substantive knowledge edit rather than mechanical, name it accordingly.)
9. CI workflow: only if Task 1 classified build-and-release.yml as SAFE (no framework/version bump) → `git add -- .github/workflows/build-and-release.yml` and commit `ci: <summary of build-and-release.yml change>`. If Task 1 FLAGGED it as a framework/version bump, DO NOT commit it — leave it uncommitted and record the flag in SUMMARY (this is the sanctioned exception to a fully clean tree).

Then push and verify:
- `git push origin main`
- `git rev-list --left-right --count origin/main...HEAD` MUST return `0 0`.
- `git status --porcelain` — the only permitted remaining entries are `.planning/quick/260719-iun-triage-dirty-working-tree-commit-uncommi/*` (this task's own artifacts, committed later by the orchestrator) and — only if flagged — `.github/workflows/build-and-release.yml`. NO plugin or source file may remain dirty.

Write SUMMARY.md: list every commit SHA + one-line description, any explicit discards with justification, and whether build-and-release.yml was committed or flagged.
  </action>
  <verify>
    <automated>git rev-list --left-right --count origin/main...HEAD; git status --porcelain | grep -E 'plugins/|/Source/' | grep -v '260719-iun' || echo "NO-DIRTY-SOURCE"</automated>
  </verify>
  <done>Meta groups committed (or build-and-release.yml flagged per Task 1). `git push origin main` succeeded. `git rev-list --left-right --count origin/main...HEAD` returns `0	0`. No dirty plugin/source file remains (only this task's .planning/quick/260719-iun artifacts, and possibly a flagged build-and-release.yml, may remain). SUMMARY.md records all SHAs + discards + the CI-file disposition.</done>
</task>

</tasks>

<threat_model>
Not applicable in the security-enforcement sense: this plan performs only git operations (stage / commit / push) on existing local changes. No new dependencies, no package installs, no new code, no external attack surface. The relevant safety risks are operational and are covered by <constraints>: (a) accidental cross-group staging via `git add -A` — mitigated by explicit-path staging; (b) committing a gated framework/version bump (build-and-release.yml) — mitigated by the Task 1 classify-and-flag step; (c) losing intended work via over-eager discard — mitigated by the conservative "commit when in doubt" policy with mandatory SUMMARY justification for any discard.
</threat_model>

<verification>
- `git rev-list --left-right --count origin/main...HEAD` returns `0	0` (HEAD == origin/main).
- `git status --porcelain` contains no `plugins/` or `Source/` entries; the only permitted remaining entries are this task's `.planning/quick/260719-iun-*` artifacts and (only if flagged) `.github/workflows/build-and-release.yml`.
- Each plugin commit message names the version bump matching that plugin's CHANGELOG top entry.
- O-Bells commit contains the two footer-panel files staged as `D` (deletions), not restored.
- O-Contrabass commit contains the previously-untracked Resources/, Source/ui/, .planning/stages/*, tests/ui_frontend_check.js.
- `git log --oneline` shows one atomic commit per plugin group plus the meta-group commits.
</verification>

<success_criteria>
Working tree triaged into atomic per-plugin + meta commits, pushed to origin/main (HEAD == origin/main, `0	0`), no dirty plugin/source files remaining, SUMMARY.md records every commit SHA and any explicit discard/flag with justification. No framework-update work started and nothing under research/framework-updates* touched.
</success_criteria>

<output>
Create `.planning/quick/260719-iun-triage-dirty-working-tree-commit-uncommi/260719-iun-SUMMARY.md` when done.
</output>