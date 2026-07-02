---
phase: quick-260702-evn
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - scripts/regen-registry-used-by.sh
  - modules/registry.yaml
autonomous: true
requirements: [UPD-02, IMP-02, UPD-03]

must_haves:
  truths:
    - "`scripts/regen-registry-used-by.sh` derives every module's used_by from disk by grepping plugins/*/CMakeLists.txt and plugins/*/Source for the module's file tokens, excluding plugins/tache_plugins."
    - "Running the script rewrites ONLY the used_by blocks and the header (version/last_updated + reminder comment) in modules/registry.yaml — no other field is touched."
    - "The script is deterministic and idempotent: a second run with no disk changes produces zero further diff."
    - "The header carries a comment stating it must be bumped on every edit, and version/last_updated advance whenever used_by content changes."
    - "The summary contains a per-module before/after diff of used_by, including the vu-meter/playable-keyboard drift corrections to []."
  artifacts:
    - scripts/regen-registry-used-by.sh
    - modules/registry.yaml
  key_links:
    - "module registry path -> modules/<path>/{cpp,js} file basenames -> grep tokens over plugins/*/{CMakeLists.txt,Source}"
    - "detected consumer -> plugins/<P>/CMakeLists.txt PLUGIN_VERSION|VERSION (fallback root project VERSION 1.0.0) -> used_by[].version"
---

<objective>
Ship `scripts/regen-registry-used-by.sh`, a deterministic, re-runnable tool that regenerates every module's `used_by` list in `modules/registry.yaml` from disk truth (UPD-02/IMP-02 from review 260701-in8), then run it to correct the current stale lists and bump the registry header (UPD-03).

The registry's per-module `used_by` fields have drifted badly: `vu-meter` cites retired dirs (OuariconComp/OuariconAnalogEQ), `preset-manager` lists 3 names while ~21 plugins ship `preset-manager.js`, `scala-tuning-engine` shows `used_by []` despite 12 consumers, and `note-expression` omits three consumers and misnames one. The fix is a script that derives each module's `used_by` by grepping `plugins/*/CMakeLists.txt` and `plugins/*/Source` for the module's own file tokens (the basenames of its `cpp/` and `js/` files), excluding `plugins/tache_plugins`.

Purpose: keep `used_by` an auditable, mechanically-verifiable reflection of disk truth instead of a hand-maintained list that silently rots.
Output: the regenerator script, a freshly-regenerated `modules/registry.yaml`, and a per-module before/after `used_by` diff in the SUMMARY.
</objective>

<execution_context>
@$HOME/.claude/gsd-core/workflows/execute-plan.md
@$HOME/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@CLAUDE.md
@modules/registry.yaml

# Reference — how consumers reference module files (mixed conventions, both must be caught by token grep):
#   - clean invocation:  plugins/O-Formant/CMakeLists.txt:40  ouaricon_add_module(O-Formant note-expression)
#   - vendored copy:      plugins/O-Formant/CMakeLists.txt:30  Source/TuningEngine.cpp
#   - explicit file ref:  plugins/O-Contrabass/CMakeLists.txt:39  ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp
@.claude/skills/module-system/SKILL.md
</context>

<tasks>

<task type="auto">
  <name>Task 1: Write scripts/regen-registry-used-by.sh (disk-truth used_by regenerator)</name>
  <files>scripts/regen-registry-used-by.sh</files>
  <action>
Create an executable bash script (`#!/usr/bin/env bash`, `set -euo pipefail`) that resolves the repo root via `git rev-parse --show-toplevel` so it runs from any cwd, targets `$ROOT/modules/registry.yaml`, and delegates the rewrite to an embedded `python3 - "$ROOT" <<'PY' ... PY` block (python3 3.14 confirmed present; use stdlib only — do NOT import PyYAML, it is not installed). Keep everything in the single `.sh` file per the requested deliverable name.

Python logic, in order:

1. Read `registry.yaml` into a list of lines. Parse module entries by scanning for `^  - name: (\S+)` (two-space indent, under `modules:`). Within each entry capture `^    path: (\S+)`. Record, per module, the name, the path, and the line index of its `    used_by:` line and the block extent. Treat `used_by:` as the last field of each entry (verified true for all entries): the block runs from the `    used_by:` line up to — but not including — the next line matching `^  - name:`, OR the next column-0 line (`^\S` or `^#`), OR EOF.

2. Compute each module's grep TOKENS = the set of file basenames of every `*.h`, `*.cpp`, `*.js` under `$ROOT/modules/<path>/cpp` and `.../js` (recurse; e.g. note-expression's `cpp/vst3/NoteExpression_VST3.cpp` contributes `NoteExpression_VST3.cpp`, and `NoteExpression.h` is the primary catch). Match these as FIXED STRINGS. Additionally, treat the module's registry `name` as a CMake-only token matched via `ouaricon_add_module(...<name>...)` against each plugin's `CMakeLists.txt`, to catch clean module invocations that vendor no source. A plugin is a consumer if ANY token matches.

3. Enumerate consumer candidates: every entry `$ROOT/plugins/*` that IS a directory and whose basename is NOT `tache_plugins`. For each candidate plugin, search `plugins/<P>/CMakeLists.txt` plus everything under `plugins/<P>/Source` (recursive). Use `grep -rlI -F -e <tok> ...` for the file-basename tokens (fixed-string, `-I` to skip binaries) and a separate `grep -rlE 'ouaricon_add_module\([^)]*\b<name>\b'` on `CMakeLists.txt` for the module-name token; OR the results.

4. Resolve each consumer plugin's version once (cache by plugin), from `plugins/<P>/CMakeLists.txt`, in order: (a) `PLUGIN_VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?`; (b) `VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?` but ONLY on lines that do NOT contain `cmake_minimum_required`; (c) fallback to the root `$ROOT/CMakeLists.txt` `project\([^)]*VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)` (currently 1.0.0 — this is how O-Contrabass, which declares no plugin version, resolves); (d) literal `unknown` if all else fails.

5. Render the new used_by block: consumers sorted case-insensitively by name. Empty set -> the single line `    used_by: []`. Otherwise `    used_by:` followed by, per consumer, `      - plugin: <name>` and `        version: <ver>`. Splice this over the module's existing used_by block (from step 1), preserving every other line byte-for-byte.

6. Header handling (UPD-03): ensure a reminder comment line exists immediately above the `^version:` line; if absent, insert one instructing that `version` and `last_updated` MUST be bumped on every edit and noting the regen script does this automatically (maintainers may bump minor/major by hand for significant changes). Determine `changed` = (any used_by block differs from its original text) OR (the comment was just inserted). If `changed`: set `last_updated:` to today's date (`YYYY-MM-DD`, from `date`) and bump the patch component of the `^version:` value (e.g. 1.0.0 -> 1.0.1). If NOT changed: leave the header untouched (this is what makes re-runs idempotent).

7. Write the file back only when the assembled content differs from the original. Always print to stdout a per-module report line of the form `<module>: [<old, comma-joined>] -> [<new, comma-joined>]` (bare `[]` when empty), plus a final status line (`REGEN: changed` or `REGEN: no-op (already fresh)`). Exit 0.

Determinism/idempotency contract the script must satisfy: identical disk state yields identical used_by output; when nothing changed, the header (both version and last_updated) is left alone so a second consecutive run produces zero file diff. Do NOT hardcode any plugin names, retired-name mappings, or consumer lists — everything derives from disk. Do NOT touch any field other than `used_by` blocks and the three header lines/comment.

Do not run the script in this task; only author it.
  </action>
  <verify>
    <automated>bash -n scripts/regen-registry-used-by.sh && test -x scripts/regen-registry-used-by.sh && head -1 scripts/regen-registry-used-by.sh | grep -q '^#!/usr/bin/env bash'</automated>
  </verify>
  <done>Script exists, is executable, passes `bash -n`, embeds a python3 stdlib-only rewriter, excludes plugins/tache_plugins, derives tokens from module cpp/js basenames, and hardcodes no plugin names.</done>
</task>

<task type="auto">
  <name>Task 2: Run the regenerator, verify preservation + idempotency, capture before/after diff</name>
  <files>modules/registry.yaml</files>
  <action>
Before running, capture the current state for the SUMMARY: `git --no-pager diff modules/registry.yaml` (confirms the only pre-existing working-tree change is the in-flight bow-friction addition of O-simplePhysicalModelSynth) and a snapshot of every module's current used_by.

Run `bash scripts/regen-registry-used-by.sh` from the repo root. Capture its full stdout — the per-module `old -> new` report lines are the required before/after diff for the SUMMARY.

Then verify disk truth landed correctly and nothing unrelated was clobbered:

- The in-flight bow-friction edit is PRESERVED: after regen, bow-friction's used_by still contains O-Bowed, O-Contrabass, and O-simplePhysicalModelSynth (the script re-derives exactly this set, so the uncommitted edit is reproduced, not lost).
- Drift corrected as expected: `scala-tuning-engine` has 12 consumers, `note-expression` has 11 (now including O-Bassoon, O-Contrabass, O-MicrotonalSampler, and O-Lyrica spelled correctly), `vu-meter` and `playable-keyboard` regenerate to `[]` (their historical consumers ship inline/divergent implementations, not the module's files — this is correct disk truth, NOT an error; document it in the SUMMARY and do not hand-add consumers).
- No retired directory names survive in any used_by entry.
- Header bumped per UPD-03: `version` advanced, `last_updated` set to today, reminder comment present.
- Only used_by blocks and header lines changed — no description/provides/config/tags field was altered (`git diff` review).
- YAML still parses and the module count is intact.

If any assertion fails, fix the script (Task 1) and re-run rather than hand-editing registry.yaml. Record the captured before/after report verbatim in the SUMMARY.
  </action>
  <verify>
    <automated>ROOT=$(git rev-parse --show-toplevel); cd "$ROOT" && \
c() { awk -v m="$1" '$0=="  - name: "m{f=1;next} /^  - name: /{f=0} f&&/^ +- plugin:/{n++} END{print n+0}' modules/registry.yaml; }; \
bash scripts/regen-registry-used-by.sh >/dev/null 2>&1 && \
H1=$(git diff modules/registry.yaml | shasum) && bash scripts/regen-registry-used-by.sh >/dev/null 2>&1 && \
H2=$(git diff modules/registry.yaml | shasum) && [ "$H1" = "$H2" ] && \
[ "$(c scala-tuning-engine)" = 12 ] && [ "$(c note-expression)" = 11 ] && [ "$(c vu-meter)" = 0 ] && \
[ "$(c playable-keyboard)" = 0 ] && grep -A6 'name: bow-friction' modules/registry.yaml >/dev/null && \
awk '$0=="  - name: bow-friction"{f=1;next} /^  - name: /{f=0} f' modules/registry.yaml | grep -q 'O-simplePhysicalModelSynth' && \
! grep -nE '^ +- plugin: (OuariconMarimba|OuariconComp|OuariconAnalogEQ|OuariconTremolo|OFreqPulse|OLyrica)\b' modules/registry.yaml && \
grep -qE '^last_updated: 2026-' modules/registry.yaml && \
python3 -c "import re,sys; t=open('modules/registry.yaml').read(); v=re.search(r'^version: (\d+)\.(\d+)\.(\d+)',t,re.M); sys.exit(0 if v and (int(v.group(1)),int(v.group(2)),int(v.group(3)))>(1,0,0) else 1)"</automated>
  </verify>
  <done>Regen ran; scala-tuning-engine=12, note-expression=11, vu-meter=[], playable-keyboard=[]; the in-flight bow-friction O-simplePhysicalModelSynth entry is preserved; no retired names remain; header version/last_updated bumped and reminder comment present; a second consecutive run yields zero further diff (idempotent); per-module before/after diff captured for the SUMMARY.</done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| filesystem -> script | Script reads repo files (module dirs, plugins/*, CMakeLists) and writes one tracked file, modules/registry.yaml. All inputs are in-repo, developer-controlled. |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-evn-01 | Tampering | modules/registry.yaml rewrite | medium | mitigate | Surgical block-splice touches only used_by blocks + 3 header lines/comment; all other lines preserved byte-for-byte; `git diff` review + YAML-parse assertion in verify guard against structural corruption. |
| T-evn-02 | Tampering | in-flight working-tree edit (bow-friction) | medium | mitigate | Script re-derives bow-friction consumers from disk (identical to the uncommitted edit), reproducing rather than clobbering it; verify asserts O-simplePhysicalModelSynth survives. |
| T-evn-03 | Denial of Service | grep token collision (generic basenames) | low | accept | Tokens are module-specific file basenames (e.g. TuningEngine.cpp, NoteExpression.h); collision risk is negligible in this repo and validated against reviewer-stated expected consumer counts. No package installs -> no legitimacy gate required. |
</threat_model>

<verification>
- `scripts/regen-registry-used-by.sh` exists, is executable, passes `bash -n`, and hardcodes no plugin names (derives everything from disk).
- Running it excludes plugins/tache_plugins and rewrites only used_by blocks + header.
- Regenerated used_by matches disk truth: scala-tuning-engine=12, note-expression=11 (O-Bassoon/O-Contrabass/O-MicrotonalSampler added, O-Lyrica spelled correctly), preset-manager real O-* names, analog-eq-unit/compressor-unit=O-Marimba, vu-meter=[] and playable-keyboard=[] (inline/divergent copies, not module files), bow-friction preserves O-simplePhysicalModelSynth.
- No retired directory names (OuariconMarimba/OuariconComp/OuariconAnalogEQ/OuariconTremolo/OFreqPulse/OLyrica) remain in any used_by entry.
- Header (UPD-03): version bumped, last_updated = today, reminder comment present.
- Idempotent: a second consecutive run produces zero further diff.
- SUMMARY contains the per-module before/after used_by diff printed by the script.
</verification>

<success_criteria>
- `scripts/regen-registry-used-by.sh` is a deterministic, re-runnable regenerator that derives used_by from disk truth (UPD-02/IMP-02) and bumps the header on change (UPD-03).
- `modules/registry.yaml` used_by lists reflect disk truth with no retired names and no clobbered unrelated fields; the in-flight bow-friction edit is preserved.
- Per-module before/after used_by diff recorded in the SUMMARY.
</success_criteria>

<output>
Create `.planning/quick/260702-evn-regenerate-modules-registry-yaml-used-by/260702-evn-SUMMARY.md` when done. Include the script's full per-module before/after used_by report verbatim.
</output>