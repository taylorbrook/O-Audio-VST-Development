# Quick 261019-ryp — Context (follow-up to 261019-gk9)

## Gap 1 — click-away commit fix (35 plugins)
Bug found in gk9 plan 07 (fix commit `2eda4d38`, O-Tremolo): the value-entry `<input>` commits on blur, but dial drag handlers `preventDefault()` their mousedown/pointerdown, which suppresses the focus change → grabbing another dial leaves the entry open with its readout frozen. Fix (inside the value-entry open function):

```js
const onOutsidePointer = (e) => { if (e.target !== input) input.blur(); };
document.addEventListener('pointerdown', onOutsidePointer, true);
// in close():
document.removeEventListener('pointerdown', onOutsidePointer, true);
```
Already fixed: O-Tremolo, O-Detune, O-DigiDelay, O-simpleReverb, O-simpleAmbisonics, O-SpectralShaper.

Needs fix (census by grep — entry `<input>` present, no capture-phase outside handler):
O-AnalogEQ, O-AnalogSaturation, O-Bass, O-Bells, O-Bitrot, O-Bowed, O-Chorus, O-Comp, O-Contrabass, O-Emulator, O-Formant, O-FreqPulse (3 entry sites), O-GrainScatter, O-IntonationPad, O-Lyrica (2), O-Marimba, O-MicrotonalSampler (2), O-MultiBandCompressor, O-Octagon, O-Orbit, O-Polystutter, O-Prism (3), O-ReverseDelay, O-simpleAdditive, O-simpleBeatmaker, O-simpleGrain, O-simplePhysicalModelSynth, O-simpleSampler, O-simpleSubtractive, O-simpleWavetable, O-Strata (3), O-Tapestop, O-Texture, O-TextureForge (edit `Source/ui/src/app.js`, then `npm run build` → tracked `public/js/app.bundle.js`), O-Wind.
Every entry site in a plugin gets the fix (some plugins have >1 entry implementation — e.g. readout entry + knob-overlay entry). Apply even where the bug may not reproduce — it's inert there.
Watch for: Alt-click handlers that open/reset inside the input (don't let the outside handler fire on the input itself); entry boxes whose ancestor is the dial (target check must be `e.target !== input`, not `!container.contains`).

## Gap 2 — O-Marimba Effects tab (shared effect units)
`modules/effects/{analog-eq-unit,compressor-unit,reverb-unit}/js/*.js` — O-Marimba is the ONLY consumer (embeds the module JS straight from `modules/` via CMakeLists.txt:81-83). Current dblclick resets:
- analog-eq-unit.js:656-669 (dblclick → setNormalisedValue(DEFAULT_VALUES) for freq/gain — no gesture pair)
- compressor-unit.js:287-288
- reverb-unit.js:227-228
Convert to the suite contract: dblclick → inline value entry (Prism `attachValueEntry` semantics: overlay on readout, Enter/blur/click-away commit, Esc cancel, untouched text not written, skew-aware scaledToNorm from live state.properties, unit parse, sliderDragStarted→setNormalisedValue→sliderDragEnded, includes the click-away fix); Alt-click → reset to default wrapped in the gesture pair. Reference: `plugins/O-Prism/Source/ui/public/index.html:3066-3170` and the gk9 Marimba port in `plugins/O-Marimba/Source/ui/public/js/app.js`.
Bump each touched module's `module.yaml` version (minor: behaviour change) and note it in the module README/changelog if one exists; keep the module self-contained (no dependency on Marimba's app.js).

## Already done in gk9 (don't redo)
All 41 plugins have the version bump from gk9 but those versions are unreleased (no tags). This follow-up does a NEW patch bump per plugin + CHANGELOG `## [X.Y.Z]` entry (keep it simple and consistent).

## Constraints (same as gk9)
- Skip O-Bassoon, O-Freeze, O-Gain, O-Reed, O-simpleFM (other sessions' uncommitted work).
- Do not touch PLUGINS.md (dirty from another session).
- Path-scoped commits `git commit -- plugins/<Name>` (and `-- modules/effects/<unit>` for gap 2 — commit modules together with O-Marimba in one commit is fine: `git commit -- modules/effects plugins/O-Marimba`). Re-check branch + status before each commit. Never `git add -A`. Never tag.
- Builds serialized via the gk9 mkdir lock protocol (`/tmp/gk9-build.lock`, see `.planning/quick/261019-gk9-default-level-6db-on-instruments-double-/261019-gk9-PLAN.md`), via `./scripts/build-and-install.sh <Name>` in background, then auval.
- Gate: reuse `/tmp/gk9-gates/gk9-gate.js` if present (recreate from the gk9 plan if not) + a check that each entry site registers and removes the capture-phase pointerdown handler; headless probe: open entry on dial A, pointerdown on dial B → entry closed, value committed, B drag works.
