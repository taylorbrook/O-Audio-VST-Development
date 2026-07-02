---
phase: quick-260702-ffl
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - .claude/compaction-snapshot.md
  - .claude/resource-index.json
  - .claude/frontmatter-issues.txt
  - .claude/skills/spike-findings-VST-development/SKILL.md
  - .claude/skills/ui-mockup/references/ui-design-rules.md
  - plugins/tache_plugins/**
  - archive/tache_plugins/**
  - scripts/regen-registry-used-by.sh
autonomous: true
requirements: [UPD-04, UPD-05, IMP-04]
must_haves:
  truths:
    - "Stale transient artifact compaction-snapshot.md removed; resource-index.json regenerated with a current (2026-07-02) timestamp."
    - "No stale PWYW/licensing-integration guidance remains in the 5 audited skill files, while installer LicenseFile, Xcode-license, and library-tradeoff references are preserved."
    - "plugins/tache_plugins archived to archive/tache_plugins via git mv with history preserved; ls plugins/ is clean of it."
    - "Zero references to the old plugins/tache_plugins path in CMakeLists.txt, scripts/, or .claude/."
    - "Submodule plugins/O-Orbit/libs/SAF is untouched and unstaged."
  artifacts:
    - archive/tache_plugins/
    - .claude/resource-index.json
    - scripts/regen-registry-used-by.sh
  key_links:
    - "generate-resource-index.py owns BOTH resource-index.json and frontmatter-issues.txt — run it; do not hand-edit frontmatter-issues.txt."
    - "git mv preserves history for the 348 tracked files under tache_plugins."
---

<objective>
Housekeeping sweep from review 260701-in8: remove stale transient artifacts, strip residual PWYW/licensing-integration tokens (keeping legitimate installer/build references), and archive the non-Ouaricon `plugins/tache_plugins/` collection so plugin-count and `ls plugins/` tooling is clean.

Purpose: Reduce repo clutter and eliminate stale/misleading references left after the PWYW-licensing removal, without disturbing working build config or the O-Orbit submodule.
Output: Deleted/regenerated `.claude/` transient artifacts; two doc edits; `plugins/tache_plugins/` → `archive/tache_plugins/`; a cleaned `regen-registry-used-by.sh`.
</objective>

<execution_context>
@$HOME/.claude/gsd-core/workflows/execute-plan.md
@$HOME/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@./CLAUDE.md

# Generator that owns resource-index.json AND frontmatter-issues.txt:
@.claude/scripts/generate-resource-index.py
# Script that references the old tache_plugins path (must be cleaned):
@scripts/regen-registry-used-by.sh
</context>

<constraints>
- SCOPED STAGING ONLY. The working tree currently has many unrelated modified/added files. Stage ONLY the explicit paths this plan touches. Never `git add -A` at repo root.
- DO NOT stage, move, or otherwise touch anything under the git submodule `plugins/O-Orbit/libs/SAF`. The tache_plugins move is isolated from O-Orbit; verify the submodule stays unstaged.
- Verify existence before deleting/moving; if a target is already gone, note it and continue (idempotent).
- Installer `LicenseFile=` references (Inno Setup, pointing at an EULA/license text) are legitimate installer functionality — KEEP them. Only strip stale PWYW/licensing-integration guidance.
</constraints>

<tasks>

<task type="auto">
  <name>Task 1: UPD-04 — clear stale transient artifacts, regenerate resource index</name>
  <files>.claude/compaction-snapshot.md, .claude/resource-index.json, .claude/frontmatter-issues.txt</files>
  <action>
Three transient artifacts under `.claude/`, all git-tracked. Handle each per its owner:

1. Verify the three files exist (`ls -la` on each). If any is already absent, note it and continue.

2. `git rm .claude/compaction-snapshot.md` — it is a stale template snapshot referencing the nonexistent MinimalKick plugin (a tache_plugin being archived in Task 3) and hardcoded placeholder rows. No generator owns this file, so a plain removal is durable.

3. Regenerate the resource index by running the generator: `python3 .claude/scripts/generate-resource-index.py`. Its imports (pyyaml, jsonschema) are confirmed available. This single run:
   - Rewrites `.claude/resource-index.json` with a fresh `generated` timestamp from current `research/` frontmatter (fixes the stale 2026-04-05 stamp).
   - Rewrites OR deletes `.claude/frontmatter-issues.txt` per current disk truth — the generator writes it only when research docs are missing frontmatter and calls `unlink(missing_ok=True)` otherwise. This generator OWNS frontmatter-issues.txt. Do NOT hand-delete it separately: a standalone delete is not durable, and regeneration is the correct un-staling action (it will either refresh the list to today's truth or remove the file organically if all research docs now have frontmatter). Either outcome resolves review item UPD-04's staleness concern.

4. Stage exactly these three paths (handles modify AND delete): `git add -A .claude/compaction-snapshot.md .claude/resource-index.json .claude/frontmatter-issues.txt`.

Note in the summary whether frontmatter-issues.txt was refreshed or removed by the generator, and that the literal "delete" review bullet was satisfied via the generator that owns the file (documented deviation).
  </action>
  <verify>
    <automated>test ! -e .claude/compaction-snapshot.md && grep -q '"generated": "2026-07-02' .claude/resource-index.json && echo PASS</automated>
  </verify>
  <done>compaction-snapshot.md removed; resource-index.json regenerated with a 2026-07-02 `generated` timestamp; frontmatter-issues.txt refreshed or removed by the generator; only these three `.claude/` paths staged.</done>
</task>

<task type="auto">
  <name>Task 2: UPD-05 — strip residual licensing-integration tokens, keep legitimate refs</name>
  <files>.claude/skills/spike-findings-VST-development/SKILL.md, .claude/skills/ui-mockup/references/ui-design-rules.md</files>
  <action>
Audit the five review-flagged files for `licens*` tokens and apply ONLY the two stale-token edits below. The other three tokens are legitimate and MUST stay untouched.

Disposition table (verified line numbers):

- `.claude/skills/plugin-planning/assets/architecture-template.md:774` — "...licensing complexity, doesn't use JUCE ecosystem" — KEEP. This is a library-rejection tradeoff note, not PWYW guidance. No edit.
- `.claude/skills/plugin-packaging/assets/inno-template.iss:37` — `LicenseFile=` — KEEP. Legitimate Inno Setup installer directive (EULA/license text). No edit. (Explicitly preserved per plan constraint.)
- `.claude/skills/system-setup/references/execution-notes.md:84` — "Some systems require accepting Xcode license" — KEEP. Legitimate macOS build-setup step. No edit.
- `.claude/skills/spike-findings-VST-development/SKILL.md:9` — the shared-module parenthetical lists "aesthetic templates, preset manager, licensing, etc." — STRIP the stale `licensing` token: the licensing/PWYW shared module was removed, so remove ", licensing" from that list, leaving "aesthetic templates, preset manager, etc." (This file is auto-loaded as a skill, so the stale module reference is actively misleading.)
- `.claude/skills/ui-mockup/references/ui-design-rules.md:743` — the frame-rate example row cell reads "License check, version info" — REWORD to a neutral rarely-updating example, e.g. "Version info, status readout", removing the stale "License check" implication (the suite has no license-check UI post-PWYW). This is a cosmetic doc-example reword with zero functional impact; keep the row's illustrative purpose intact.

Do not modify the three KEEP files. Stage only the two edited files.
  </action>
  <verify>
    <automated>! grep -qi 'preset manager, licensing' .claude/skills/spike-findings-VST-development/SKILL.md && ! grep -qi 'License check' .claude/skills/ui-mockup/references/ui-design-rules.md && grep -qi 'LicenseFile=' .claude/skills/plugin-packaging/assets/inno-template.iss && echo PASS</automated>
  </verify>
  <done>Stale `licensing` token removed from SKILL.md module list; stale "License check" example reworded in ui-design-rules.md; Inno `LicenseFile=`, Xcode-license, and library-tradeoff references all preserved unchanged.</done>
</task>

<task type="auto">
  <name>Task 3: IMP-04 — archive tache_plugins and clean old-path references</name>
  <files>plugins/tache_plugins/** → archive/tache_plugins/**, scripts/regen-registry-used-by.sh</files>
  <action>
Move the non-Ouaricon `plugins/tache_plugins/` collection (17 subdirs, 348 git-tracked files, no top-level CMakeLists — the root CMake glob at CMakeLists.txt:48 already skips it because it lacks a top-level CMakeLists.txt) out of `plugins/` and clean the one script that references the old path.

1. Verify `plugins/tache_plugins/` exists and is tracked (`git ls-files plugins/tache_plugins | head`). If already moved, note and skip to step 4 verification.

2. `mkdir -p archive` — the destination parent does not yet exist; `git mv` requires it (there is no `archive` entry in `.gitignore`, so tracking is preserved).

3. `git mv plugins/tache_plugins archive/tache_plugins` — moves the entire tracked tree at once, preserving git history for all 348 files. This is isolated from the O-Orbit submodule; do not touch `plugins/O-Orbit/libs/SAF`.

4. Clean the now-moot tache_plugins exclusion in `scripts/regen-registry-used-by.sh` so zero old-path references remain:
   - In the header docstring (around line 9), the sentence notes that plugins/tache_plugins is excluded from consumer scanning. Reword that sentence to drop the tache_plugins clause (it is no longer under `plugins/`), keeping the surrounding description of how consumers are derived intact.
   - In the candidate-enumeration loop (around lines 127-128), remove the two-line guard that skips the `tache_plugins` entry. After the move, `plugins/` no longer contains that entry, so the guard is unreachable dead code — removing it changes no behavior and clears the reference.

5. Stage ONLY: `git add -A plugins/tache_plugins archive/tache_plugins scripts/regen-registry-used-by.sh`. Confirm the O-Orbit submodule is not staged.

Out-of-scope (leave as-is, note in summary): `docs/codebase/STRUCTURE.md` and `research/stutter-effects/stutter-effects-research-findings.md` also mention the old path, but they are documentation/research and fall outside IMP-04's verification scope (CMakeLists / scripts / .claude).
  </action>
  <verify>
    <automated>test -d archive/tache_plugins && test ! -d plugins/tache_plugins && [ -z "$(grep -rn 'tache_plugins' scripts/ .claude/ 2>/dev/null)" ] && [ -z "$(grep -rln 'tache_plugins' --include=CMakeLists.txt . 2>/dev/null | grep -vE '^\./(build|archive)/')" ] && [ -z "$(git diff --cached --name-only | grep 'O-Orbit/libs/SAF')" ] && echo PASS</automated>
  </verify>
  <done>tache_plugins archived to archive/tache_plugins via git mv (history preserved); ls plugins/ clean of it; regen-registry-used-by.sh tache_plugins exclusion (docstring + loop guard) removed; zero old-path references in scripts/, .claude/, or CMakeLists.txt; O-Orbit submodule unstaged.</done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

No new trust boundaries. This is a housekeeping sweep: file deletion, doc-string edits, and a tracked `git mv`. No external input is processed, no network calls, no package installs, no runtime code paths added or changed.

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-ffl-01 | Tampering | Accidental broad `git add -A` staging unrelated dirty working-tree files or the O-Orbit submodule | medium | mitigate | Scoped staging by explicit path in every task; Task 3 verify asserts `O-Orbit/libs/SAF` is not staged |
| T-ffl-02 | Denial of Service | Removing regen-script tache_plugins guard alters module `used_by` scan | low | accept | Guard is unreachable after the move (plugins/ no longer contains tache_plugins); removal is a behavior-preserving no-op |
</threat_model>

<verification>
- `.claude/compaction-snapshot.md` no longer exists; `.claude/resource-index.json` has a `generated` timestamp of 2026-07-02; `frontmatter-issues.txt` refreshed or removed by the generator.
- Only the two stale `licens*` tokens edited (SKILL.md module list, ui-design-rules example); Inno `LicenseFile=`, Xcode license, and architecture-template tradeoff note preserved.
- `plugins/tache_plugins/` gone; `archive/tache_plugins/` present with 17 subdirs.
- `grep -rn 'tache_plugins' scripts/ .claude/` and the CMakeLists sweep (excluding build/ and archive/) both return zero matches.
- `git diff --cached --name-only` contains no `plugins/O-Orbit/libs/SAF` path.
- Only the paths in `files_modified` are staged; no unrelated working-tree files pulled in.
</verification>

<success_criteria>
All three review items closed: UPD-04 transient artifacts cleared/regenerated, UPD-05 stale licensing-integration tokens stripped (legitimate installer/build refs kept), IMP-04 tache_plugins archived via git mv with zero residual old-path references in CMakeLists/scripts/.claude and the O-Orbit submodule untouched.
</success_criteria>

<output>
Create `.planning/quick/260702-ffl-housekeeping-sweep-260701-in8-delete-sta/260702-ffl-SUMMARY.md` when done. Record: whether frontmatter-issues.txt was refreshed or removed by the generator, the two UPD-05 edits applied vs the three KEEP items, and confirmation that the O-Orbit submodule and unrelated working-tree files stayed unstaged.
</output>