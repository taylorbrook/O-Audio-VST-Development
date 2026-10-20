---
phase: quick-261019-ryp
plan: 03
subsystem: plugin-ui (value entry)
tags: [webview, value-entry, click-away, simple-synths, O-IntonationPad]
status: complete
requires: [261019-gk9 attachValueEntry ports]
provides: [click-away commit in 7 plugins]
affects: [PLUGINS.md version rows (follow-up)]
key-files:
  modified:
    - plugins/O-simpleAdditive/Source/ui/public/js/app.js
    - plugins/O-simpleBeatmaker/Source/ui/public/js/app.js
    - plugins/O-simpleGrain/Source/ui/public/js/app.js
    - plugins/O-simplePhysicalModelSynth/Source/ui/public/js/app.js
    - plugins/O-simpleSampler/Source/ui/public/js/app.js
    - plugins/O-simpleSubtractive/Source/ui/public/js/app.js
    - plugins/O-IntonationPad/Source/ui/public/js/app.js
    - plugins/*/CMakeLists.txt, plugins/*/CHANGELOG.md (same 7)
decisions:
  - "Canonical O-Tremolo form applied verbatim; every site's blur already commits, so the handler calls input.blur()"
metrics:
  completed: 2026-10-19
actuals:
  tasks: 3
  commits: 7
  tokens: 6000
plan_head_before: 4a8cec0ef718b82b65da77f8ccb8413061f2962e
plan_head_after: b3ff58a51f68a78ffd772728265af1e738c77bd4
---

# Phase quick-261019-ryp Plan 03: click-away commit, simple-synth family and O-IntonationPad

Added the document capture-phase `pointerdown` click-away commit (D-01) to the single `attachValueEntry` in all seven plugins. Each one: probe failed before the edit and passed after it, then patch-bumped, built under the shared lock, auval-listed, and committed path-scoped.

## Per plugin

| Plugin | Version | Commit | Entry site (handler line) | Probe before / after | auval |
|---|---|---|---|---|---|
| O-simpleAdditive | 1.4.2 -> 1.4.3 | 8c1a48dd | js/app.js:256 (292) | FAIL (3) / PASS | aumu OSiA OuDv O-simpleAdditive-dev |
| O-simpleBeatmaker | 1.4.2 -> 1.4.3 | 47109d71 | js/app.js:326 (362) | FAIL (3) / PASS | aumu OSiB OuDv O-simpleBeatmaker-dev |
| O-simpleGrain | 1.5.2 -> 1.5.3, VERSION_CODE 0x010502 -> 0x010503 | 806b19ff | js/app.js:261 (297) | FAIL (3) / PASS | aumu OsGr OuDv O-simpleGrain-dev |
| O-simplePhysicalModelSynth | 1.4.2 -> 1.4.3 | 0bd2a97b | js/app.js:279 (315) | FAIL (3) / PASS | aumu OsPM OuDv O-simplePhysicalModelSynth-dev |
| O-simpleSampler | 1.6.2 -> 1.6.3 | 45a1034d | js/app.js:279 (315) | FAIL (3) / PASS | aumu OsSm OuDv O-simpleSampler-dev |
| O-simpleSubtractive | 1.7.2 -> 1.7.3 | b5371aa6 | js/app.js:261 (297) | FAIL (3) / PASS | aumu OSiS OuDv O-simpleSubtractive-dev |
| O-IntonationPad | 2.11.1 -> 2.11.2 | b3ff58a5 | js/app.js:1371 (1405) | FAIL (3) / PASS | aumu OuIP OuDv O-IntonationPad-dev |

The bug reproduced on all seven. None was inert. Every "before" run failed the same 3 checks: field still open after B is pressed, typed value not committed, field still focused after the drag. Dragging B already worked before the fix.

Probe args. Each `/tmp/ryp-probe-<Name>{-before,}.log` starts with its own args line, so it can be rerun.
- O-simpleAdditive: `--a '#knob-scanPosition' --b '#knob-scanLfoRate' --type 1`
- O-simpleBeatmaker: `--a '#knob-swing' --b '#knob-humanize' --type 0.2`
- O-simpleGrain: `--a '#knob-density' --b '#knob-position' --type 3`
- O-simplePhysicalModelSynth: `--a '#knob-excitationPosition' --b '#knob-excitationColor' --type 0.2`
- O-simpleSampler: `--a '#knob-loopCrossfade' --b '#knob-tune' --type 0.2`
- O-simpleSubtractive: `--a '#knob-cutoff' --b '#knob-resonance' --type 3`
- O-IntonationPad: `--a '#complexityKnob .knob-visual' --b '#stereoSpreadKnob .knob-visual' --type 3`. Its dials are built at runtime with id `<param>Knob`, and the drag starts on `.knob-visual`.

## Site details
- **Blur already commits at every site** (`input.addEventListener('blur', commit)`), so the handler is `if (e.target !== input) input.blur();` (rule 3).
- **Single teardown:** Enter -> `commit()` -> `close()`, blur -> `commit()` -> `close()`, Esc -> `close()`. The removal is the first statement after the `closed` guard in `close()`. No other exit path removes the field.
- The six simple-* `attachValueEntry` bodies are byte-identical apart from one comment in Beatmaker, so they got the same edit (2-space indent, double quotes). O-IntonationPad got the same edit in its own style (4-space indent, single quotes). The field opens from either the dial or the readout and is always appended to the readout. The target test stays `e.target !== input`.
- Nothing else changed: no edits to drag handlers, Alt-click resets, other listeners, CSS or i18n.

## Gates
- `ryp-gate.js`: baseline `FAIL-HANDLER-COUNT ... click-away handlers=0` on all 7. After the edit, `OK ... handlers=1` on all 7.
- `gk9-gate --syntax`: SYNTAX-OK on all 7.
- `gk9-gate --boot`: pageErrors=0 before and after on all 7.
- Build logs `/tmp/ryp-build-<Name>.log`: all end `RYP-EXIT=0` with the auval line just above.
- Plan verify blocks (Tasks 1-3) rerun at the end: all 7 PASS. Each commit's `show --stat` lists only `plugins/<Name>/`. No tags created. PLUGINS.md not touched.

## Deviations from Plan
- **CHANGELOG spacing (cosmetic).** O-IntonationPad's entries put no blank line between `### Fixed` and the bullet, so the new entry does the same. The six simple-* files keep their blank-line style.
- **Builds queued as one chain.** After Additive, the other six builds ran as one background chain. Each plugin still took the lock separately, so other executors could build in between. Commits were made one at a time as each build log reached `RYP-EXIT=0`.
- **Commit count measurement.** No ledger file was written before the first commit. `plan_head_before` is the parent of the first commit (8c1a48dd^). That range holds 17 commits because parallel executors committed in between. Exactly 7 of them touch this plan's paths, and those are the 7 listed above.

## Known Stubs
None.

## Follow-ups
- PLUGINS.md version rows. O-simpleAdditive 1.4.3, O-simpleBeatmaker 1.4.3, O-simpleGrain 1.5.3, O-simplePhysicalModelSynth 1.4.3, O-simpleSampler 1.6.3, O-simpleSubtractive 1.7.3, O-IntonationPad 2.11.2.
- The five excluded plugins (O-Bassoon, O-Freeze, O-Gain, O-Reed, O-simpleFM; D-05) still need the click-away fix once their sessions commit.
- Not checked by hand in a DAW. Verification here is the headless stub probe plus auval.

## Self-Check: PASSED
- All 7 commits are on main (8c1a48dd, 47109d71, 806b19ff, 0bd2a97b, 45a1034d, b5371aa6, b3ff58a5). `git status --short -- plugins/<Name>` is clean for all 7.
