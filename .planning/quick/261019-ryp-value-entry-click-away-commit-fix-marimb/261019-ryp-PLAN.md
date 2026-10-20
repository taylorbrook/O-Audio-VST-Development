---
phase: quick-261019-ryp
type: index
wave: 1
depends_on: []
autonomous: true
requirements: [QR-ryp-01, QR-ryp-02, QR-ryp-03, QR-ryp-04]
plans:
  - 261019-ryp-01-PLAN.md
  - 261019-ryp-02-PLAN.md
  - 261019-ryp-03-PLAN.md
  - 261019-ryp-04-PLAN.md
  - 261019-ryp-05-PLAN.md
---

# Quick 261019-ryp: plan index (no tasks here; execute the five plans below)

**Request (follow-up to 261019-gk9):** (1) add the value-entry click-away commit fix to the 35 plugins lacking it; (2) convert O-Marimba's shared effect-unit knobs (`modules/effects/*`) from double-click reset to value entry + Alt-click reset.

This file is an index and carries no tasks. The work is in the five plans below. All are **wave 1 and parallel**, with no `depends_on` between them and **no overlapping file paths**: each plugin folder belongs to exactly one plan, `modules/effects/` and `modules/registry.yaml` belong to plan 05 only, and each plan's only other file is its own SUMMARY. Plan 05 READS `plugins/O-AnalogEQ/Source/ui/public/index.html` (owned by plan 01) as a reference and never writes it.

## Plans

| Plan | Theme | Plugins (owned folders) | Tracer (Task 1) | Est. tokens |
|---|---|---|---|---|
| [01](261019-ryp-01-PLAN.md) | inline index.html effects + O-Contrabass | O-AnalogEQ, O-AnalogSaturation, O-Bass, O-Bitrot, O-Chorus, O-Comp, O-Emulator, O-Contrabass | O-AnalogEQ (probe RED today) | 95k |
| [02](261019-ryp-02-PLAN.md) | O-Prism lineage + Family-C editors | O-Prism, O-Strata, O-simpleWavetable, O-Bells, O-Bowed, O-Lyrica, O-Wind | O-Prism (the reference implementation; probe inert today) | 90k |
| [03](261019-ryp-03-PLAN.md) | simple-synth family (js/app.js) + O-IntonationPad | O-simpleAdditive, O-simpleBeatmaker, O-simpleGrain, O-simplePhysicalModelSynth, O-simpleSampler, O-simpleSubtractive, O-IntonationPad | O-simpleAdditive (probe RED today) | 85k |
| [04](261019-ryp-04-PLAN.md) | js/app.js effects | O-GrainScatter, O-FreqPulse, O-Octagon, O-Orbit, O-ReverseDelay, O-Tapestop, O-MultiBandCompressor, O-Polystutter | O-GrainScatter (probe RED today) | 95k |
| [05](261019-ryp-05-PLAN.md) | O-Marimba both gaps + `modules/effects` + module-JS instruments | O-Marimba (+ modules/effects, modules/registry.yaml), O-Formant, O-Texture, O-MicrotonalSampler, O-TextureForge | O-Marimba (unit probe 10 failures today) | 130k |

35 plugins = 8 + 7 + 7 + 8 + 5. Builds are serialized through one lock, so wall clock is about 35 sequential builds whatever the split; the parallelism buys overlapping edit/gate/probe time. Plan 05 carries fewer plugins because Gap 2 is its heaviest task.

**Excluded, in no plan's files (D-05):** O-Bassoon, O-Freeze, O-Gain, O-Reed, O-simpleFM (other sessions' uncommitted work). PLUGINS.md is not touched; every SUMMARY lists old -> new versions for a later PLUGINS.md pass. Already fixed in gk9 and not re-touched: O-Tremolo, O-Detune, O-DigiDelay, O-simpleReverb, O-simpleAmbisonics, O-SpectralShaper.

## Shared tools (written and validated by the planner; the plans reference them, executors do not edit them)

`.planning/quick/261019-ryp-value-entry-click-away-commit-fix-marimb/tools/`

| Tool | What it proves | Validation at planning time |
|---|---|---|
| `ryp-gate.js <file>[=N]...` | Static D-08 check: exactly N document capture-phase `pointerdown` handlers per file, each comparing the target with `!==`, each blurring (or committing), each removed capture-phase at least as often as added | 6 already-fixed pages OK; all 35 targets `FAIL-HANDLER-COUNT ... handlers=0` (site counts match the census); negative controls (no removal, a `contains` target test, a commented-out fix) all FAIL |
| `ryp-probe.js --plugin --a --b --type [...]` | Headless click-away probe on the real UI tree + repo stub: double-click dial A opens a field, typed text, press on dial B closes it AND commits A, then B still drags | O-Tremolo PASS (fixed); O-AnalogEQ, O-simpleAdditive, O-GrainScatter, O-Marimba (Sound and Effects tabs) FAIL; O-Prism PASS (bug does not reproduce there) |
| `ryp-unit-probe.js` | Gap-2 behaviour on O-Marimba's Effects tab, seeded with the module C++ ranges: double-click opens without writing, typed values land skew-aware in exactly start/set/end, host write keeps the field, Esc / untouched Enter write nothing, Alt-click resets in exactly start/set/end with no drag | RED today: 10 failures (every control's double-click resets) |
| `/tmp/gk9-gates/gk9-gate.js` (reused, D-08) | `--syntax`, `--boot`; dblclick/Alt mode for the three units | present; recreate from the gk9-07 plan's "Gate script" if absent |

## Planner findings the executors rely on

1. **Census correction (counts only, not scope):** the census's multi-site counts (O-FreqPulse 3, O-Lyrica 2, O-MicrotonalSampler 2, O-Prism 3, O-Strata 3) are `createElement('input')` counts. Reading each site shows a single value-entry implementation per plugin. The extras are mod-matrix checkboxes and range sliders (Prism, Strata), band-mix and divider range sliders (FreqPulse), a generator-dialog number field (Lyrica) and slot-table number fields (MicrotonalSampler). None is a commit-on-blur value field, so those gate args carry `=1`. Every plan tells the executor to confirm by reading and to widen the fix (and raise N) if one turns out to be an entry field.
2. **Bug reproduction varies.** The fix is inert where a page's drag lets focus move (O-Prism today). D-01 applies it everywhere regardless; probes record "inert" baselines.
3. **Gap 2 specifics:** the compressor and EQ strips set `pointer-events: none` on their knobs while bypassed (by design; the probe switches them on). The compressor's drag sends no gesture pair, a pre-existing gap that is left untouched and listed as a follow-up. The O-AnalogEQ plugin already carries the same dual-ring knob with a gk9 entry over a split tooltip, and is the port reference for analog-eq-unit. `used_by` versions in the registry are version-when-added and stay at 1.16.0.
4. **Traps flagged in the plans:** O-Texture's CMakeLists also holds `ONNXRUNTIME_VERSION 1.19.2` (not the plugin version). O-simpleGrain's `OSIMPLEGRAIN_VERSION_CODE` must track VERSION. O-TextureForge needs a bundle-parity check before its src edit and `npm run build` after (no installs). O-Bitrot, O-Octagon, O-ReverseDelay and O-Tapestop boot on per-plugin stubs, so the probe needs readout selectors. O-Polystutter's teardown is a module-level `cleanupTextInput`.

## Decision IDs used in the plans (numbered from 261019-ryp-CONTEXT.md, which carries no D-numbers)

| ID | Decision | Plans |
|---|---|---|
| D-01 | Click-away fix at every entry site: document capture `pointerdown`, `e.target !== input`, removed in the teardown; applied even where inert | 01-05 |
| D-02 | O-Marimba effect units: double-click -> Prism-semantics value entry (click-away included); Alt-click -> gesture-bracketed default reset | 05 |
| D-03 | Touched modules: minor bump in module.yaml + changelog, README note where one documents gestures/versions; self-contained (no dependency on Marimba's app.js) | 05 |
| D-04 | New patch bump per plugin + `## [X.Y.Z]` CHANGELOG entry (gk9 versions unreleased) | 01-05 |
| D-05 | Skip O-Bassoon, O-Freeze, O-Gain, O-Reed, O-simpleFM; PLUGINS.md untouched | all (none touched) |
| D-06 | Path-scoped commits (modules with O-Marimba in one commit), branch + status re-check before each, never `git add -A`, never tag | 01-05 |
| D-07 | Builds serialized via the gk9 mkdir lock `/tmp/gk9-build.lock`, `build-and-install.sh` in the background, then auval | 01-05 |
| D-08 | Gates: gk9-gate reuse + registration/removal check + headless probe (A open, press B -> closed + committed, B drags) | 01-05 |

## Requirement IDs

| ID | Requirement | Plans |
|---|---|---|
| QR-ryp-01 | In all 35 plugins, pressing another control while a value-entry field is open commits the typed value and closes the field before the new drag starts (every entry site) | 01-05 |
| QR-ryp-02 | O-Marimba Effects-tab knobs (analog-eq-unit, compressor-unit, reverb-unit): double-click opens value entry with Prism semantics incl. click-away; Alt/Option-click resets to default in one gesture; no double-click reset remains | 05 |
| QR-ryp-03 | Touched modules bumped minor with changelog + README note + registry version; each module self-contained | 05 |
| QR-ryp-04 | Each plugin patch-bumped + CHANGELOG, built under the lock, installed, auval-listed, committed path-scoped; exclusions and PLUGINS.md untouched; never tagged | 01-05 |

## Coverage audit (CONTEXT census -> plan)

- **GOAL (CONTEXT description):** Gap 1 -> plans 01-05; Gap 2 -> plan 05.
- **Gap 1, 35 plugins:** AnalogEQ 01 · AnalogSaturation 01 · Bass 01 · Bells 02 · Bitrot 01 · Bowed 02 · Chorus 01 · Comp 01 · Contrabass 01 · Emulator 01 · Formant 05 · FreqPulse 04 · GrainScatter 04 · IntonationPad 03 · Lyrica 02 · Marimba 05 · MicrotonalSampler 05 · MultiBandCompressor 04 · Octagon 04 · Orbit 04 · Polystutter 04 · Prism 02 · ReverseDelay 04 · simpleAdditive 03 · simpleBeatmaker 03 · simpleGrain 03 · simplePhysicalModelSynth 03 · simpleSampler 03 · simpleSubtractive 03 · simpleWavetable 02 · Strata 02 · Tapestop 04 · Texture 05 · TextureForge 05 (src + `npm run build` -> tracked bundle) · Wind 02.
- **Gap 1 "watch for":** Alt-click / in-field presses and entry boxes nested in the dial -> the `e.target !== input` rule in every plan.
- **Gap 2:** analog-eq-unit (:656-669), compressor-unit (:287-288), reverb-unit (:227-228) -> plan 05 Task 1 (entry + Alt reset + click-away, module.yaml minor bumps, README/changelog notes, self-contained).
- **CONTEXT constraints:** exclusions + PLUGINS.md (D-05) · path-scoped commits, modules with Marimba (D-06) · build lock + background + auval (D-07) · gk9-gate reuse + registration/removal check + headless probe (D-08) · new patch bump + CHANGELOG (D-04).
- **RESEARCH / REQUIREMENTS.md:** none for a quick task (no research phase by instruction).

No census item is unplanned; no item outside the CONTEXT is planned.
