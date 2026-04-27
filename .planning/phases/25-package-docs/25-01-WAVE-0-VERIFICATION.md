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

**Status:** PENDING — run on dev machine
**To run:** see Plan 25-01 Task 0a `<how-to-verify>` block
**Append result here:** with timestamp, Dorico version, observed behavior,
and PASS/FAIL.

If FAIL: HALT plan, escalate to D-05 reconsideration (curated `slot<N>.pluginstate`
authoring becomes mandatory). Promote to `25-01-A2-FAIL-fix-PLAN.md`.

---

## A4 Result

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
