---
phase: quick-260907-bgx
plan: 01
type: execute
wave: 1
depends_on: []
autonomous: true
requirements: [QUICK-260907-bgx]
files_modified:
  - plugins/*/.planning/i18n-index-draft.html          # 33 deletions
  - plugins/*/.planning/i18n-labels-skeleton.js        # 33 of 37 deletions
  - plugins/*/.planning/i18n-inventory.tsv             # 33 of 37 deletions
  - .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/260907-bgx-MANIFEST.md
  - .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/260907-bgx-deferred-items.md

estimate:
  tokens: 55000
  raw_tokens: 34000
  tasks: 3
  confidence: low

must_haves:
  truths:
    - "No plugin ships or serves anything that reads i18n-index-draft.html, i18n-labels-skeleton.js, or i18n-inventory.tsv — proven by a classified grep before any file is removed."
    - "`node scripts/check-i18n.js` produces byte-identical output before and after the deletions."
    - "The 4 sibling-only plugins (O-AnalogSaturation, O-Bitrot, O-Emulator, O-SimpleReverb) still have their skeleton.js and inventory.tsv on disk — they are out of scope, not forgotten."
    - "plugins/<P>/.planning/params.tsv survives on every plugin that has one — serve-ui.js reads it."
    - "Every deletion is a path-scoped commit naming exactly three files; the 8 untracked 260906-uu7 files are still untracked at the end."
  artifacts:
    - .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/260907-bgx-MANIFEST.md
    - .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/260907-bgx-deferred-items.md
  key_links:
    - "scripts/i18n-extract.js is the GENERATOR of all three files (write-only, fs.writeFileSync at ~L692-702). Its reference is not a dependency — but the executor must prove write-only independently, not assume it."
    - "scripts/serve-ui.js reads plugins/<name>/.planning/params.tsv — a different .tsv in the same directory. Deletion must be basename-exact; never glob *.tsv."
---

<objective>
Remove the 33 unserved `i18n-index-draft.html` drafts and their `i18n-labels-skeleton.js` /
`i18n-inventory.tsv` siblings from `plugins/*/.planning/`, gated on a classified
reference grep, as 33 path-scoped commits.

Purpose: `scripts/i18n-extract-README.md` and the generated skeleton header both say these
are throwaway intermediates ("NOT a shippable file... then delete this file"). 33 plugins
still carry all three. They are ungated, unserved dead weight in the tree.

Output: 99 tracked files removed across 33 commits, plus a manifest recording the grep
verdict per plugin and a deferred-items file for the 4 out-of-scope sibling sets.
</objective>

<planning_time_observations>
These were measured live at planning time (2026-09-07) and AUTHORIZE the scope below.
The executor RE-VERIFIES all of them in Task 1 and halts on any disagreement.

**Counts:**
- `ls plugins/*/.planning/i18n-index-draft.html` → **33**
- `ls plugins/*/.planning/i18n-labels-skeleton.js` → **37**
- `ls plugins/*/.planning/i18n-inventory.tsv` → **37**
- skeleton set and inventory set are IDENTICAL (verified by `diff`)

**Tracking (decides the deletion mechanism):** all 33 drafts, all 37 skeletons and all 37
inventories are **TRACKED** (`git ls-files` returns 33/37/37). `.planning/` is NOT ignored.
So the mechanism is `git rm` and "one path-scoped commit per plugin" has real content —
this is not the all-untracked case the constraints hedge against.

**The 33 in-scope plugins** (draft + both siblings):

```
O-AnalogEQ  O-Bassoon  O-Bells  O-Bowed  O-Chorus  O-Comp  O-Contrabass
O-Detune  O-Formant  O-Freeze  O-FreqPulse  O-Gain  O-GrainScatter  O-Lyrica
O-MicrotonalSampler  O-MultiBandCompressor  O-Octagon  O-Orbit  O-Prism  O-Reed
O-ReverseDelay  O-simpleAdditive  O-simpleBeatmaker  O-simpleFM  O-simpleGrain
O-simplePhysicalModelSynth  O-simpleSampler  O-simpleSubtractive  O-Tapestop
O-Texture  O-TextureForge  O-Tremolo  O-Wind
```

**The 4 OUT-OF-SCOPE sibling-only plugins** (siblings present, draft already removed —
the clamp-carrying four): `O-AnalogSaturation`, `O-Bitrot`, `O-Emulator`, `O-SimpleReverb`.
Do NOT delete their files. They go in deferred-items for the user to decide.

**Reference-grep result at planning time — the load-bearing finding.** A naive
"any hit in a script blocks it" rule would block all 33 and make this a no-op, because the
generator names all three basenames. It is write-only. Full hit list, classified:

| Hit path | Class | Verdict |
|---|---|---|
| `scripts/i18n-extract.js` L65-67, L692-702, L1264 | GENERATOR — `fs.writeFileSync` only, plus a doc comment and a generated header template | **not a block** (executor must prove write-only) |
| `scripts/i18n-extract-README.md` L19-21 | Documentation of the generator; explicitly says "then delete" | **not a block** |
| `plugins/*/.planning/i18n-labels-skeleton.js` L2 | The target file's own generated header | **not a block** (self-reference) |
| 10 files under `.planning/quick/*/`, `.planning/STATE.md`, `research/i18n-zh-hans-localization.md` | Narrative / SUMMARY / PLAN / research prose | **not a block** (record only) |

Zero hits in any `CMakeLists.txt`, `.github/`, `.claude/`, test, hook, `Makefile`, or
`package.json` (there is no `package.json`). **No read construct anywhere** —
`readFileSync` / `require` / `from '...'` against these basenames returns nothing.
**No glob dependency** — the only `.planning` path resolutions in `scripts/` are
`serve-ui.js` → `params.tsv` and `verify-suite-battery.sh` → a fixed quick dir.
Expected verdict: **33 CLEAR, 0 BLOCKED.**
</planning_time_observations>

<context>
@/Users/taylorbrook/Dev/VST-development/CLAUDE.md
@/Users/taylorbrook/Dev/VST-development/.planning/STATE.md
@/Users/taylorbrook/Dev/VST-development/scripts/i18n-extract-README.md
</context>

<tasks>

<task type="tracer">
  <name>Task 1: Re-verify the gate, then delete + commit ONE plugin end-to-end (O-Gain)</name>
  <files>
    .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/260907-bgx-MANIFEST.md
    .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/check-i18n-before.txt
    plugins/O-Gain/.planning/i18n-index-draft.html
    plugins/O-Gain/.planning/i18n-labels-skeleton.js
    plugins/O-Gain/.planning/i18n-inventory.tsv
  </files>

  <precondition>Working tree is on branch `main` (`git branch --show-current` prints exactly `main`) and `git status --short` shows only the 8 known-untracked `260906-uu7` files. Halt if either differs.</precondition>

  <action>
Prove the gate on live state, then run the whole mechanism once on a single plugin before
touching the other 32. This is the tracer: audit -> git rm -> path-scoped commit -> gate
re-run, all on O-Gain. If the mechanism is wrong, it is wrong after one commit, not 33.

**Step 1 — capture the BEFORE baseline.** Run `node scripts/check-i18n.js` and tee its full
output to `.planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/check-i18n-before.txt`.
Record its exit code in the manifest. This file is the equality target for Task 3. Capture it
BEFORE anything is deleted; a baseline taken afterwards proves nothing.

**Step 2 — re-verify the three globs and the tracking status.** Run each of:
`ls plugins/*/.planning/i18n-index-draft.html 2>/dev/null | wc -l`, same for
`i18n-labels-skeleton.js` and `i18n-inventory.tsv`, and
`git ls-files 'plugins/*/.planning/i18n-index-draft.html' | wc -l` (and the two siblings).
Expect 33 / 37 / 37 for both the disk globs and the tracked globs. BSD `wc -l` left-pads its
output with spaces — pipe through `tr -d ' '` before any string comparison, or the equality
test fails on whitespace against a bare `33`. If any count disagrees with the
planning_time_observations, STOP and report; do not proceed on a changed tree.
Derive the in-scope plugin list from the live draft glob, not from the plan's hardcoded list —
use the hardcoded list only as a cross-check and report any difference.

**Step 3 — run the classified reference grep.** Single command over the whole repo:

```
grep -rIn -e 'i18n-index-draft\.html' -e 'i18n-labels-skeleton\.js' -e 'i18n-inventory\.tsv' . \
  --exclude-dir=.git --exclude-dir=node_modules --exclude-dir=build --exclude-dir=backups \
  --exclude-dir=Builds --exclude-dir=libs
```

This reaches `scripts/`, `tests/`, every `CMakeLists.txt`, `.claude/`, `.github/`, any
`Makefile`, and every `*.sh`/`*.js`/`*.mjs`/`*.cjs`/`*.py` at the repo root and under
`plugins/*/`, because it is unfiltered by extension. Classify EVERY hit path into exactly one
bucket and write the classification into the manifest:

- **SELF** — the hit path IS one of the target files. Not a reference.
- **NARRATIVE** — path matches `.planning/**/*.md` or `research/**/*.md`. Record, never block.
- **GENERATOR/DOC** — `scripts/i18n-extract.js` or `scripts/i18n-extract-README.md`. Not a
  block, but only once Step 4 proves it. Record the line numbers.
- **BLOCK** — anything else: a script, CMakeLists, test, hook, or CI workflow. A BLOCK hit
  disqualifies the plugins it names; a BLOCK hit that names no plugin (a generic path join)
  disqualifies ALL 33. Record the referencing path and line.

**Step 4 — prove the generator is write-only.** Do not take the plan's word for it. Assert
that no read construct anywhere in the repo targets these basenames:

```
grep -rIn -E "(readFileSync|readFile|createReadStream|require|import|fetch|open)\s*\(?[^)]*i18n-(index-draft|labels-skeleton|inventory)" . \
  --exclude-dir=.git --exclude-dir=node_modules --exclude-dir=build --exclude-dir=backups
```

Expected: no output. Then confirm the three `scripts/i18n-extract.js` path joins at ~L692-702
are each immediately followed by `fs.writeFileSync`. If ANY read is found, the generator
becomes a BLOCK and the whole task halts pending the user.

**Step 5 — prove there is no glob dependency.** The basename grep cannot see a script that
reaches these files by wildcard. Run `grep -rIn "\.planning" scripts/` and confirm the only
`.planning` path resolutions are `serve-ui.js` -> `params.tsv` and `verify-suite-battery.sh`
-> its fixed quick dir. Note in the manifest that `plugins/<name>/.planning/params.tsv` is a
LIVE file read by serve-ui.js, sitting in the same directory as our target `.tsv` — every
deletion must name `i18n-inventory.tsv` exactly and no command in this plan may ever glob
`.planning/*.tsv`.

**Step 6 — write the manifest** to
`.planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/260907-bgx-MANIFEST.md`
with: the three live counts, tracked-vs-untracked verdict, the full classified hit table, the
write-only and glob findings, and a per-plugin table of the 33 with a `CLEAR` or
`BLOCKED (<path>:<line>)` verdict. Do not delete anything from a BLOCKED plugin.

**Step 7 — the tracer deletion, on O-Gain only.** Only if O-Gain is CLEAR. Re-check
`git branch --show-current` prints `main` immediately before committing (a session-start
snapshot is stale; another session shares this index and HEAD).

```
git rm plugins/O-Gain/.planning/i18n-index-draft.html \
       plugins/O-Gain/.planning/i18n-labels-skeleton.js \
       plugins/O-Gain/.planning/i18n-inventory.tsv
```

Use `git rm`, never `git rm --cached`: a plain `--cached` leaves the file on disk, and a
later pathspec commit naming that path RESURRECTS it into the index. `git rm` removes from
disk and index together, which is what makes the pathspec commit safe.

Then commit, pathspec-scoped so a concurrent session's staged work cannot ride along:

```
git commit -m "chore(O-Gain): remove unserved i18n scaffolding (draft, skeleton, inventory)

Claude-Session: https://claude.ai/code/session_01VesWUWmc6iV9zFc3GFrPgn" \
  -- plugins/O-Gain/.planning/i18n-index-draft.html \
     plugins/O-Gain/.planning/i18n-labels-skeleton.js \
     plugins/O-Gain/.planning/i18n-inventory.tsv
```

**Step 8 — prove the tracer commit is clean.** `git show --stat HEAD` must list exactly 3
files, all deletions, all under `plugins/O-Gain/.planning/`. `git status --short` must still
show the 8 untracked `260906-uu7` files and nothing else. `ls plugins/O-Gain/.planning/` must
still contain `params.tsv` if it had one before. Then re-run `node scripts/check-i18n.js` and
diff against the Step 1 baseline — it must be identical, proving one deletion changed no gate.
  </action>

  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development && test -f .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/260907-bgx-MANIFEST.md && test -f .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/check-i18n-before.txt && test ! -e plugins/O-Gain/.planning/i18n-index-draft.html && test ! -e plugins/O-Gain/.planning/i18n-labels-skeleton.js && test ! -e plugins/O-Gain/.planning/i18n-inventory.tsv && test "$(git show --stat --format= --name-status HEAD | grep -c '^D.*plugins/O-Gain/.planning/i18n-')" = "3" && test "$(git show --format= --name-only HEAD | wc -l | tr -d ' ')" = "3" && node scripts/check-i18n.js > /tmp/bgx-after-tracer.txt 2>&1; diff .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/check-i18n-before.txt /tmp/bgx-after-tracer.txt && echo TRACER_OK</automated>
  </verify>

  <done>
MANIFEST.md exists with a per-plugin CLEAR/BLOCKED verdict for all 33 and the classified hit
table. The check-i18n baseline is captured. O-Gain's three files are gone from disk and index
in a single 3-file commit whose message carries the Claude-Session trailer, and check-i18n
output is unchanged from baseline.
  </done>
</task>

<task type="auto">
  <name>Task 2: Delete and commit the remaining 32 CLEAR plugins</name>
  <files>plugins/{32 remaining CLEAR plugins}/.planning/i18n-index-draft.html, i18n-labels-skeleton.js, i18n-inventory.tsv</files>

  <precondition>Task 1's MANIFEST.md exists and O-Gain's tracer commit is HEAD-reachable with exactly 3 deletions.</precondition>

  <action>
Repeat the proven Task 1 Step 7 mechanism for every remaining plugin the manifest marks
CLEAR. Skip every BLOCKED plugin. Never touch the 4 sibling-only plugins
(O-AnalogSaturation, O-Bitrot, O-Emulator, O-SimpleReverb) — they have no draft and are out
of scope.

Drive it from the manifest's CLEAR list, one plugin per iteration, with these guards inside
the loop — CLAUDE.md requires the location re-check immediately before every commit, not once
at session start:

1. Assert `git branch --show-current` prints exactly `main`; abort the whole loop otherwise.
2. Assert the plugin name contains no `/` and that none of the three constructed paths
   contains `libs/SAF` (`plugins/O-Orbit/libs/SAF` is a git submodule and is in scope for the
   suite but never for this task — O-Orbit's targets live at `plugins/O-Orbit/.planning/`).
3. Assert all three files exist on disk AND are tracked (`git ls-files --error-unmatch`).
   Skip the plugin and record it if any assertion fails.
4. `git rm` the three exact basenames. Never a wildcard — `plugins/<P>/.planning/params.tsv`
   is a live file read by serve-ui.js and sits in the same directory.
5. `git commit -m "chore(<Plugin>): remove unserved i18n scaffolding (draft, skeleton, inventory)"`
   with a blank line then `Claude-Session: https://claude.ai/code/session_01VesWUWmc6iV9zFc3GFrPgn`,
   pathspec-scoped with `--` followed by the same three paths.
6. Assert the resulting commit touched exactly 3 files
   (`git show --format= --name-only HEAD | wc -l | tr -d ' '` equals 3). If it touched more,
   a concurrent session's work joined the commit — stop immediately and report; do not
   continue creating commits.

Do not `git add -A` and do not `git commit -a` at any point. Run the loop in the foreground
in batches if needed — each iteration is fast, but a single command running all 32 risks the
600s executor watchdog; if it approaches that, split into batches of ~10 plugins.

Record the running commit count and the per-plugin SHA into the manifest as you go, so a
watchdog kill mid-loop leaves a resumable record rather than an unknown state.
  </action>

  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development && test "$(ls plugins/*/.planning/i18n-index-draft.html 2>/dev/null | wc -l | tr -d ' ')" = "0" && test "$(ls plugins/*/.planning/i18n-labels-skeleton.js 2>/dev/null | wc -l | tr -d ' ')" = "4" && test "$(ls plugins/*/.planning/i18n-inventory.tsv 2>/dev/null | wc -l | tr -d ' ')" = "4" && test "$(git log --format=%H --grep='remove unserved i18n scaffolding' | wc -l | tr -d ' ')" = "33" && test "$(git log --format=%H --grep='remove unserved i18n scaffolding' | while read s; do git show --format= --name-only "$s" | wc -l | tr -d ' '; done | sort -u)" = "3" && echo BATCH_OK</automated>
  </verify>

  <done>
All 33 CLEAR plugins have their three files removed from disk and index, each in its own
3-file commit carrying the Claude-Session trailer. The draft glob returns 0; the skeleton and
inventory globs return 4 (the out-of-scope sibling-only set).

Counts are stated for the expected 33-CLEAR / 0-BLOCKED case established at planning time.
If Task 1 marked N plugins BLOCKED, the expected values become `N` drafts and `4+N` siblings
with `33-N` commits — record the substitution in the manifest rather than editing the numbers
away silently.
  </done>
</task>

<task type="auto">
  <name>Task 3: Post-deletion gate sweep and deferred-items record</name>
  <files>
    .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/260907-bgx-deferred-items.md
    .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/check-i18n-after.txt
    .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/260907-bgx-MANIFEST.md
  </files>

  <precondition>Task 2 finished — the draft glob returns 0 (or N, for N BLOCKED plugins recorded in the manifest).</precondition>

  <action>
Prove nothing served depended on the removed files, and leave a record of what was
deliberately NOT touched.

**Step 1 — re-run the three globs** and record the final counts in the manifest. Expect
`0 / 4 / 4`. Pipe every `wc -l` through `tr -d ' '` — BSD pads.

**Step 2 — the repo-level i18n gate.** Run `node scripts/check-i18n.js`, tee to
`check-i18n-after.txt`, and `diff` it against `check-i18n-before.txt` from Task 1. Identical
output plus an identical exit code is the primary proof: this gate covers every localized
plugin, so an unchanged verdict across 99 deletions means no assertion was reading them.
A non-empty diff is a FAILURE — report it, do not rationalize it.

**Step 3 — the render gate on one affected plugin.** Run
`node scripts/check-ui-labels.js --plugin O-Gain`. This is the every-language label render
gate; it serves the plugin's `ui/` tree, which never contained `.planning/`, so a pass is
belt-and-braces confirmation that the served surface is untouched. Record the exit code.
Known hazard: a concurrent session's UI test server can hold the port and serve the OTHER
session's tree — if the run fails on a port bind or serves an unexpected plugin, record it as
INCONCLUSIVE (an environment condition, not a target regression) and rerun once. Do not
report a port clash as a deletion failure.

**Step 4 — confirm the live neighbour survived.** For every plugin that had a
`plugins/<P>/.planning/params.tsv` before this task, confirm it is still on disk:
`ls plugins/*/.planning/params.tsv 2>/dev/null | wc -l | tr -d ' '` must equal the count
`git ls-files 'plugins/*/.planning/params.tsv' | wc -l | tr -d ' '` returns. serve-ui.js
reads this file; a wildcard slip in Task 2 would show up here and nowhere else.

**Step 5 — confirm the working tree is clean of strays.** `git status --short` must show
exactly the 8 untracked `260906-uu7` files that were present at planning time, plus this
task's own new quick-dir artifacts. Nothing under `plugins/` may be modified or untracked.
Nothing under `plugins/O-Orbit/libs/SAF` may appear at all.

**Step 6 — write the deferred-items file** at
`.planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/260907-bgx-deferred-items.md`
containing:
  - The 4 sibling-only plugins by name — `O-AnalogSaturation`, `O-Bitrot`, `O-Emulator`,
    `O-SimpleReverb` — with the note that their `i18n-index-draft.html` was already removed
    (these are the clamp-carrying four) but their `i18n-labels-skeleton.js` and
    `i18n-inventory.tsv` remain, that both are TRACKED, that the same reference grep found no
    consumer for them either, and that they were left alone only because the task scope said
    "the 33 drafts plus their siblings". One decision for the user: delete these 8 files too,
    or keep them.
  - Every plugin the manifest marked BLOCKED, with the referencing path and line number.
  - The narrative-only hits recorded but not treated as blocks (the `.planning/quick/*` PLAN
    and SUMMARY docs, `.planning/STATE.md`, `research/i18n-zh-hans-localization.md`), noted as
    prose that now references deleted files — harmless, but a future reader will find them.

**Step 7 — commit the quick-dir artifacts** path-scoped, after re-checking
`git branch --show-current` is `main`:

```
git commit -m "docs(quick-260907-bgx): manifest, gate evidence, deferred items

Claude-Session: https://claude.ai/code/session_01VesWUWmc6iV9zFc3GFrPgn" \
  -- .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/
```

Note that `git commit -- <path>` only picks up TRACKED files, so these new files need
`git add` on those exact paths first. Add only this task's own artifacts by explicit path —
never `git add -A`, and never the directory of the unrelated `260906-uu7` task.
  </action>

  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development && node scripts/check-i18n.js > .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/check-i18n-after.txt 2>&1; diff .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/check-i18n-before.txt .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/check-i18n-after.txt && test -f .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/260907-bgx-deferred-items.md && test "$(ls plugins/*/.planning/params.tsv 2>/dev/null | wc -l | tr -d ' ')" = "$(git ls-files 'plugins/*/.planning/params.tsv' | wc -l | tr -d ' ')" && test -z "$(git status --porcelain -- plugins/)" && test "$(ls plugins/*/.planning/i18n-index-draft.html 2>/dev/null | wc -l | tr -d ' ')" = "0" && echo SWEEP_OK</automated>
    <human-check>Read `260907-bgx-deferred-items.md` and decide whether the 4 sibling-only plugins' skeleton.js + inventory.tsv should also be removed.</human-check>
  </verify>

  <done>
check-i18n output is byte-identical before and after all 99 deletions; check-ui-labels on
O-Gain passed (or is recorded INCONCLUSIVE with an environment reason); every `params.tsv`
survived; `git status --porcelain -- plugins/` is empty; deferred-items.md names the 4
sibling-only plugins and any BLOCKED plugin with its referencing path.
  </done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| plan scope -> filesystem | A path constructed in this plan becomes an irreversible `git rm` |
| shared checkout -> git index | A concurrent session's staged work can join a commit made here |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-bgx-01 | Tampering | `git rm` path construction | high | mitigate | Basename-exact deletion only; no wildcard anywhere. Task 3 Step 4 asserts `params.tsv` (a live serve-ui.js input in the same directory) count is unchanged. |
| T-bgx-02 | Denial of Service | i18n / UI gates | high | mitigate | check-i18n baseline captured BEFORE the first deletion (Task 1 Step 1) and diffed after (Task 3 Step 2); a classified reference grep gates every deletion. |
| T-bgx-03 | Tampering | shared `.git/index` across sessions | medium | mitigate | Every commit is pathspec-scoped with `--`; each commit asserts exactly 3 files touched and aborts the loop on any excess. Never `git add -A` / `git commit -a`. |
| T-bgx-04 | Tampering | `plugins/O-Orbit/libs/SAF` submodule | medium | mitigate | Task 2 guard 2 rejects any constructed path containing `libs/SAF`; grep scope excludes `libs`. |
| T-bgx-05 | Elevation of Privilege | generator misread as consumer | high | mitigate | Task 1 Step 4 independently proves `scripts/i18n-extract.js` is write-only rather than trusting the plan's classification; any read construct converts it to a BLOCK and halts. |
| T-bgx-SC | Tampering | package installs | low | accept | No package-manager installs in this task. No `package.json` exists in the repo. |
</threat_model>

<verification>
1. `node scripts/check-i18n.js` before-vs-after diff is empty and exit codes match.
2. `node scripts/check-ui-labels.js --plugin O-Gain` passes (or documented INCONCLUSIVE on a port clash).
3. Draft glob = 0, skeleton glob = 4, inventory glob = 4 (adjusted by N if any plugin BLOCKED).
4. Exactly 33 commits match `remove unserved i18n scaffolding`, each touching exactly 3 files.
5. `git status --porcelain -- plugins/` is empty; the 8 `260906-uu7` files are still untracked.
6. `ls plugins/*/.planning/params.tsv` count equals its `git ls-files` count.
</verification>

<success_criteria>
- MANIFEST.md records a CLEAR/BLOCKED verdict for all 33 plugins with the classified grep hit table.
- 99 tracked files removed across 33 path-scoped, trailer-carrying commits on `main`.
- No gate output changed.
- deferred-items.md names the 4 sibling-only plugins (O-AnalogSaturation, O-Bitrot, O-Emulator, O-SimpleReverb) for a user decision, plus any BLOCKED plugin.
</success_criteria>

<output>
Create `.planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/260907-bgx-SUMMARY.md` when done.
Record in it: the final glob counts, the grep classification verdict, the 33 commit SHAs, the
check-i18n before/after equality result, the check-ui-labels exit code, and the deferred set.
</output>