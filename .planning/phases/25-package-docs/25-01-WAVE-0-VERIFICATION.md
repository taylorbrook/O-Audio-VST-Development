# Plan 25-01 v2 — Wave 0 + Canary Verification Log

**Plan:** `25-01-author-and-plumbing-PLAN.md` (v2 Playback Template pivot)
**Wave 0 owner (A2 + A4):** human-on-dev-machine
**Canary owner (Task 8):** human-on-dev-machine
**Created by:** parallel worktree executor (agent-a0ae28d99e03f181b), 2026-04-26

---

## Status: PENDING — awaiting human verification on the dev machine

The parallel-execution worktree completed the deterministic file-authoring
tasks (Tasks 1-7) but cannot perform the three checkpoint tasks because
they require physical interaction with Dorico 6 on the user's dev machine:

| Task | Type | Reason worktree cannot execute |
|------|------|--------------------------------|
| 0a (A2) | `checkpoint:human-verify` | Requires building a stripped `.dorico_pt` on disk, dragging it into Dorico 6 GUI, applying the template, and writing a quarter-sharp accidental on a Dorico project — no Dorico available in worktree, no GUI access. |
| 0b (A4) | `checkpoint:human-verify` | Requires drag-dropping `/tmp/ample_china/Ample China.dorico_pt` onto Dorico 6 and inspecting `~/Library/Application Support/Steinberg/Dorico 6/` post-state — no Dorico in worktree. |
| 8 (Canary) | `checkpoint:human-verify` | Requires building all 8 cohort `_VST3` targets, running `cmake --install`, opening Dorico 6, and confirming the Playback Template appears in the picker + quarter-sharp playback. |

These checkpoints need to be run by the user (or a DAW-capable agent
on the dev machine) AFTER the worktree merges back. Per plan §verification:
"Wave 0 (Tasks 0a, 0b) MUST pass before Tasks 1-7 run." Since the worktree
ran Tasks 1-7 deterministically without Dorico-side feedback, the user
should treat A2/A4 as **post-merge gates**: if either fails on the dev
machine, escalate to `25-01-A2-FAIL-fix-PLAN.md` or `25-01-A4-FAIL-fix-PLAN.md`
per plan D-16.

The deterministic work is **independent** of A2/A4 outcomes — even if A2
or A4 fails, the recovered XML, templated XML files, helper, install script
template, and module.cmake extensions are reusable. Only the install
destination strategy (file extraction vs file landing) would change.

---

## A2 Result

**Status:** SUPERSEDED — A2 protocol's premise (`.dorico_pt` Playback Template as the distribution mechanism) was invalidated mid-test. Path B (standalone `.doricolib`) was tested instead and PASSED end-to-end. See `25-FINDING-path-b-validation.md`.

**Date:** 2026-04-27
**Dorico:** 6 (macOS, current install at `/Applications/Dorico 6.app`)
**Plugin under test:** O-Lyrica-dev (CID `ABCDEF019182FAEB4F7544764F4C7972`, advertised name `O-Lyrica-dev`)

### A2 protocol attempt (the originally-specified test)

Built `/tmp/a2-test.dorico_pt` per Plan 25-01 Task 0a with one slot referencing O-Lyrica-dev's CID and no `slot1.pluginstate`. Drag-drop import into Dorico 6 succeeded (template appeared in `Play → Playback Template` picker as "Test State-less"). Apply: **slot stayed empty** — Dorico did not load O-Lyrica-dev. No error dialog.

Iteration 1: hypothesized name/CID mismatch (XML had `<pluginName>O-Lyrica</pluginName>` but the dev VST3 advertises itself as `O-Lyrica-dev`). Patched `<pluginName>` to `O-Lyrica-dev`, removed prior import from `~/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/Test State-less` and `EndpointConfigs/Test State-less`, re-imported. Slot **still empty** after apply.

This pulled the architecture into question: the `.dorico_pt`'s purpose is "one-click full project setup" (load plugin + assign expression map). What the v1.5 milestone actually needs is just for Dorico to **know about the expression map** so the user can assign it to their already-loaded Ouaricon plugins. The Playback Template was over-engineered for the use case.

### Path B test (standalone `.doricolib` distribution)

**Hypothesis:** drop `Ouaricon-VST3-NoteExpression.doricolib` into `~/Library/Application Support/Steinberg/Dorico 6/Expression Maps/User/` (the path Plan 25-01 v1 had right but with the wrong file extension), restart Dorico, expect "Ouaricon VST3 Note Expression" to appear in `Play → Endpoints → Expression Map` dropdown, assign it to a loaded O-Lyrica-dev, write quarter-sharp C4, listen for ~269 Hz.

Iteration 1: copied the merged-tree `.doricolib` (recovered from cd2c2c6) to `Expression Maps/User/`. Dorico restart + Library Manager → Import: **rejected** with "Error opening file: invalid file format". Comparison against `/Applications/Dorico 6.app/Contents/Resources/playback/PluginPresetLibraries/HALion Sonic/expressionMapsDefinitions.xml` revealed the recovered XML body has only `<expressionMapDefinitions>` as a child of `<kScoreLibrary>`, while a valid `.doricolib` requires ~48 sibling library containers (`<temperaments>`, `<accidentalSystems>`, `<instruments>`, `<noteheadSetDefinitions>`, …) all present at the top level — even if empty (`<entities array="true"/>`).

Iteration 2: built `/tmp/Ouaricon-VST3-NoteExpression-v2.doricolib` (6,431 B) by taking the HALion Sonic factory skeleton (48 top-level containers) and replacing its `<expressionMapDefinitions>/<entities>` body with the recovered Ouaricon `<ExpressionMapDefinition>` element (only one expression map, our canonical microtonal-routing definition). Replaced the file in `Expression Maps/User/`. Library Manager → Import: **PASS** — no error.

Quarter-sharp test: in `Play → Endpoints` on a channel with O-Lyrica-dev loaded, "Ouaricon VST3 Note Expression" now appears in the Expression Map dropdown. Assigned. Wrote C4 with quarter-sharp accidental. Hit play. **Heard ~269 Hz** (between standard C4 = 261.63 Hz and C♯ = 277.18 Hz) — VST3 Note Expression routing confirmed end-to-end.

### Verdict

**Path B PASS.** The expression-map-based distribution mechanism that Plan 25-01 v1 was attempting (but with the wrong file extension) works correctly when the file is a valid `.doricolib` (full kScoreLibrary skeleton). The `.dorico_pt` Playback Template architecture is unnecessary for the v1.5 microtonal-routing use case.

### Three bugs in commit `819b2b4` blocking any Dorico use of the merged work

1. **Invalid `.doricolib` format** — the recovered XML body at `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib` is missing the ~48 top-level kScoreLibrary containers Dorico requires for parser acceptance. The same defect appears in the embedded `playbacktemplatedeps.doricolib.in` (copied byte-exact per D-03 "recover, do not re-author"). The recovered XML is an expression-map *definition* fragment, not a complete library bundle.
2. **Dev/prod name mismatch** — `endpointconfig.xml.in` hard-codes prod plugin names (`<pluginName>O-Lyrica</pluginName>`, etc.) but `<pluginID>` pulls dev OR prod CIDs from `@OLYRICA_PLUGINID@`. Dev installs would silently drop slots due to name/CID disagreement. Affects only Path A.
3. **Path A is over-engineered** — `.dorico_pt` Playback Template approach assumes one-click full-project setup. v1.5 use case only needs the expression map available for manual assignment after the user loads their plugin.

### Escalation

A2 in its original form was never PASSed because the test premise (Path A as the distribution mechanism) is rejected. Plan 25-01 v2 is superseded by `25-FINDING-path-b-validation.md`. Phase 25 needs to replan to Path B.

---

## A4 Result

**Status:** SUPERSEDED — A4 protocol verified the `.dorico_pt` drag-drop extraction layout, which is no longer relevant under Path B (only `.doricolib` is shipped). Not run.

## Canary Install Result (original)

**Status:** SUPERSEDED — original canary protocol installed via `cmake --install . --component ouaricon_note_expression_OLyrica` and inspected the dual-asset (`.dorico_pt` + `.doricolib`) layout in Dorico 6's auto-discovery directories. Path B requires only one asset in one location; the canary needs to be redesigned during replan.

---

## Original A4 / Canary protocol below — kept for reference only

## A4 Result (original)

**Status:** PENDING — run on dev machine
**To run:** see Plan 25-01 Task 0b `<how-to-verify>` block
**Append result here:** with bash output, confirmed destinations, PASS/FAIL.

If FAIL: HALT plan, escalate to D-11 reconsideration (drag-drop install
rejected; explicit `Play -> Playback Template -> Import` becomes the
user-facing flow). Promote to `25-01-A4-FAIL-fix-PLAN.md`.

---

## Canary Install Result

**Status:** PENDING — run on dev machine after A2 + A4 PASS
**To run:** see Plan 25-01 Task 8 `<how-to-verify>` block
**Append result here:** with bash output, manual-Dorico observations,
and PASS/FAIL.

If FAIL: HALT and escalate to `25-01-canary-FAIL-fix-PLAN.md` per D-18.

Expected files after canary install (macOS):
- `~/Library/Application Support/Ouaricon/Microtonal Suite/Ouaricon-Microtonal-Suite.dorico_pt`
- `~/Library/Application Support/Ouaricon/Microtonal Suite/Ouaricon-VST3-NoteExpression.doricolib`
- `~/Library/Application Support/Steinberg/Dorico 6/PlaybackTemplateSpecs/Ouaricon Microtonal Suite/playbacktemplatespec.xml`
- `~/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml`
- `~/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ouaricon Microtonal Suite/playbacktemplatedeps.doricolib`
- `~/Library/Application Support/Steinberg/Dorico 6/Default Library Additions/Ouaricon-VST3-NoteExpression.doricolib`

Expected Dorico observations:
- "Ouaricon Microtonal Suite" appears in `Play -> Playback Template`.
- "Ouaricon VST3 Note Expression" appears in `Library -> Expression Maps`.
- Quarter-sharp C4 plays at ~269.29 Hz (not 261.63 Hz).

CMake install component name: `ouaricon_note_expression_<TARGET_NAME>` (e.g.
`ouaricon_note_expression_OLyrica`).

CID substitution sanity:
```bash
ACTUAL_CID=$(python3 -c "import json,re; raw=open('$HOME/Library/Audio/Plug-Ins/VST3/O-Lyrica-dev.vst3/Contents/Resources/moduleinfo.json').read(); raw=re.sub(r',(\\s*[}\\]])',r'\\1',raw); d=json.loads(raw); print([c['CID'] for c in d['Classes'] if c['Category']=='Audio Module Class'][0])")
grep -q "$ACTUAL_CID" "$HOME/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml" && echo "OLYRICA CID SUBSTITUTED OK"
grep -c "@.*_PLUGINID@" "$HOME/Library/Application Support/Steinberg/Dorico 6/EndpointConfigs/Ouaricon Microtonal Suite/endpointconfig.xml"  # expect 0
```
