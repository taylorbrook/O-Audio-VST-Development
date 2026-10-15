---
milestone: family-retrofit
domain: mixed
execute_agent: general-purpose
version_bump: major
base_version: 2.0.0
target_version: 3.0.0
created: 2026-10-15
---

# Plan: O-simple family retrofit

**Plugin:** O-SimpleReverb → O-simpleReverb
**Milestone:** family-retrofit
**Created:** 2026-10-15
**Phase:** Plan
**Inputs:** `CONTEXT.md`, `RESEARCH.md` (§ numbers below refer to it, plus the "Mockup Sign-off" table M1–M5 / Q2–Q5), `mockup/index.html` (signed off 2026-10-15), brief `plugins/O-SimpleReverb/improvements/family-retrofit.md`.

Paths are relative to the plugin folder. "Amb" = `plugins/O-simpleAmbisonics/`.

---

## Summary

**Improvement:** a case-only rename to O-simpleReverb and a full O-simple family UI at 1180×780. It adds four wells, sigpath stations, two lesson rows (6 types + Init and 4 concepts) and keeps the 48 utility presets in the browser.
**Approach:** port Ambisonics' editor, payload and preset *surface* onto Reverb's existing preset-manager XML state. The ER taps and the tone curve come from C++ natives (single source, no JS tables). The DSP is frozen and proven by a bit-exact null against v2.0.0 after every audio-touching task.
**Version:** 2.0.0 → 3.0.0 (MAJOR: user-visible rename; IDs, ranges, `OuSr` and state format unchanged, plus one additive `editorScale` property).

### Corrections to RESEARCH.md that this plan locks

| Item | RESEARCH says | Plan uses | Why |
|---|---|---|---|
| Size-vs-Decay lesson id | `sizeDecay` (§8) | **`sizeVsDecay`** | The signed-off mockup's `data-preset` |
| Character lesson | "Character (absorption)" (CONTEXT) | **"Character: dark ↔ bright"**, fileName `Lesson - Character` | Sign-off Q3 |
| `savePresetWithDialog` | refused or kept (§1.3) | **registered as refused** (Amb `refusedDialog()`) | Sign-off Q2. preset-manager.js still calls it |
| `loadPresetFromFile` | refused or kept | **kept** (v2 chooser) and reached from the gear menu "Load file…" | Sign-off Q2 |
| VERSION | `"3.0.0"` in W0 (§1.2) | stays `"2.0.0"` (quoted) until the post-verify bump | The skill's post-milestone step owns the bump |
| Git tag | — | **none** | Memory: never tag unless `/publish` |

---

## Global rules for every task

- **Shared checkout.** All commits are path-scoped (`git commit -- <paths>`). Re-run `git branch --show-current && git status --short` immediately before each commit. Never `git add -A`, never stash or check out. `PLUGINS.md` and the other plugins' files are dirty from other sessions, so stage only the Reverb row (memory index `index_git_shared_checkout`).
- **DSP frozen.** In `Source/dsp/*.h` and `ModulationFx.h`, only banner comments change. The one audio-path edit is Task 3's pure static extraction.
- **Null after every audio-touching task** (Tasks 1, 3, 4, 5) with the Task 2 harness. A failing null blocks the next task.
- **Builds:** run long commands in the background (600 s watchdog). Use `./scripts/build-and-install.sh O-simpleReverb` for installs (dual-variant sweep plus AU cache).
- **No tag. No version bump until post-verify.**

---

## Task Breakdown

### Task 0: Backup (pre-execute)

**Outcome:** `backups/O-SimpleReverb/v2.0.0/` holds a full copy of the plugin tree, including the gitignored `Source/ui/public/modules/`.

**Verification:**
- [ ] `diff -rq` between the backup and the live tree is empty (ignoring `.planning/improvements/family-retrofit/`)
- [ ] HALT if it fails

**Dependencies:** None · **Effort:** Small

---

### Task 1: Rename (W0, its own commit)

**Outcome:** the plugin is `O-simpleReverb` everywhere live. Identity (`OuSr`, manufacturer) is unchanged.

**Steps:**
1. Two-step `git mv plugins/O-SimpleReverb plugins/O-simpleReverb.tmp && git mv plugins/O-simpleReverb.tmp plugins/O-simpleReverb` (§2.3). `git status` must show only `R` entries for the tree.
2. `CMakeLists.txt`: target, `PRODUCT_NAME "O-simpleReverb${OUARICON_DEV_SUFFIX}"`, `O-simpleReverb_UIResources`, `O-simpleReverb-render-check`, every target reference. Add the family flags `IS_SYNTH FALSE`, `NEEDS_MIDI_INPUT/OUTPUT FALSE`, `IS_MIDI_EFFECT FALSE`, `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`, `VST3_CATEGORIES "Fx" "Reverb"`. Quote `VERSION "2.0.0"`. Keep `OSIMPLEREVERB_TEST_HOOKS`.
3. Literals: `PluginProcessor.h` `getName()` → `"O-simpleReverb"` (a literal, not `JucePlugin_Name`); `PluginProcessor.cpp` presetManager folder; `PluginEditor.cpp` WebView2 data folder `"O-simpleReverb_WebView"`.
4. Q4: the guarded case-only preset-folder rename (§2.4 snippet) in the processor ctor, before `initializeFactoryPresets()`.
5. Banner comments (§2.1 list); `tests/render-check/main.cpp` and `measure.h` strings; `.claude/scripts/canary-test.py:4,146-147`; `README.md:52` display text (keep the URL slug).
6. Reconfigure CMake (the target name changed). Check any local `SKIP_PLUGINS` cache for the old spelling.

**Verification:**
- [ ] `grep -rn "O-SimpleReverb"` over live paths (excluding the §2.2 historical list, `CHANGELOG.md`, `.planning/**`, `CODE_REVIEW.md`, `tests/fixtures/*.tsv`) returns zero
- [ ] `ninja O-simpleReverb_VST3 O-simpleReverb_AU` builds with 0 warnings; `build-and-install.sh` sweeps `O-SimpleReverb{,-dev}.{vst3,component}`
- [ ] `auval -v aufx OuSr <mfr>` passes; `auval -a | grep -i simplereverb` shows one entry per installed variant, named O-simpleReverb
- [ ] render-check builds as `O-simpleReverb-render-check` and is green at 126 gates + 30 mutants
- [ ] Commit: `git commit -- plugins/O-SimpleReverb plugins/O-simpleReverb .claude/scripts/canary-test.py README.md` (`PLUGINS.md` and the registry wait for Task 10)

**Dependencies:** Task 0 · **Effort:** Medium

---

### Task 2: v2.0.0 null harness and baseline

**Outcome:** a repeatable bit-exact null between v2.0.0 and the working tree.

**Steps:**
1. `git archive O-SimpleReverb-v2.0.0 CMakeLists.txt modules plugins/O-SimpleReverb scripts | tar -x` into the scratchpad. Copy `Source/ui/public/modules/*.js` in. Build out of tree.
2. **Determinism first:** render v2.0.0 twice and `cmp`. If the two renders differ, stop and report.
3. Write `tests/pedalboard/verify_3_null.py`. Use `uv run --with pedalboard`, one process per VST3 (shared class ID). Set parameters by `raw_value` and match them by name. Stimuli: pink burst + silence, sine sweep, impulse. Cover all 48 bank presets at 44.1 and 96 kHz, plus one automation run with mid-block TYPE / SIZE / DECAY moves. Write `.npy` and compare bit-exact.
4. **Positive control:** v3 with LPON flipped must differ.
5. Null the Task 1 build.

**Verification:**
- [ ] v2 vs v2: identical
- [ ] v2 vs renamed v3: identical across the matrix
- [ ] Positive control: differs

**Dependencies:** Task 1 · **Effort:** Medium

---

### Task 3: Tone-map static extraction (the one audio-path edit)

**Outcome:** `warmCutoffHz(c, fs)`, `brightGainDb(c)` and `typeEqCoefficients(type, fs)` are pure statics in `PluginProcessor.cpp` (or a small JUCE-free `Source/ToneMap.h`). The DSP and the future `getToneCurve` both call them (§4).

**Verification:**
- [ ] The float expression order is preserved exactly (diff review)
- [ ] Task 2 null: bit-exact
- [ ] render-check green

**Dependencies:** Task 2 · **Effort:** Small

---

### Task 4: Meter payload (C++)

**Outcome:** a `MeterFrame { inDb, wetDb, outPeakL, outPeakR, spanMs, live }` is published once per processBlock through a `TripleBuffer`. `outputPeak` is gone.

**Steps:**
1. Copy `Source/TripleBuffer.h` verbatim from Amb (namespace `osr`), **outside** `Source/dsp/`. Create `Source/MeterFrame.h`: trivially copyable, `static_assert`, JUCE-free.
2. Port `Source/MeterFramePayload.h`. Keep `scaled` / `Writer` / `fnvInt` / `LiveTracker` / `sampleFrame`. Rewrite `QuantisedFrame`, `quantise`, `hash` and `format` for the §6.1 fields and quantisation.
3. Processor: accumulators per §6.1 and §6.3 (double `wetSumSq`, halving cap at about 1 s). Reset only when `!hasNewData()`. Tap points: `inDb` on `dryBuffer`; `wetDb` after Low Cut and before WET gain; out peaks on the final buffer, with R = L on mono. Remove `outputPeak`.
4. render-check: port gate §4 behind `#if __has_include("MeterFramePayload.h")`. Add payload gates (deterministic format, hash-equal means bytes-equal, a NaN/Inf fuzz frame still parses, one zero frame on the idle edge) and the mutant `frameNoReset`.

**Verification:**
- [ ] render-check green, including the §12 no-allocation gate; the new mutant is caught
- [ ] The v1.14.0 negative-control build of render-check still compiles
- [ ] Task 2 null: bit-exact

**Dependencies:** Task 3 · **Effort:** Medium

---

### Task 5: Processor preset core, lessons, editorScale

**Outcome:** the processor serves the family preset surface over the unchanged module XML state (§7.3, D7, D8).

**Steps:**
1. `std::atomic<float> editorScale{0}`. Write it to `parameters.state` before `getStateAsXml()` and read it after `setStateFromXml`, the same way as `uiLanguage`.
2. `Source/PresetRecipes.h` (namespace `srpresets`): `Entry entries[6]`, `findById` / `findByName`. Eleven entries with ids `init, booth, room, hall, spring, plate, ambient, sizeVsDecay, character, lowCut, insertSend`. Recipes come from RESEARCH §8. The display name of `character` is "Character: dark ↔ bright".
3. `applyFactoryPreset(id)`: reset every parameter to its default (begin / setValueNotifyingHost / end, skipped within 1e-6), apply the recipe through `convertTo0to1`, `setCurrentPresetName(fileName)`, then increment the revision and snapshot the targets. **Never write lessons to disk.**
4. `getPresetCatalog()` returns lessons, the factory bank in bank (TYPE) order and the user presets sorted case-insensitively. `getPresetWalkOrder()` returns the bank order, then the user presets (Q5).
5. presetState is `{name, id, factory, modified}`, with modified against the post-load snapshot. A restored session counts as unmodified. A restored lesson name is relit through `findByName`.
6. Save and delete go through the module (factory-guarded) behind `sanitisePresetName`. They refuse bank names and lesson file names.
7. render-check: 11 ids resolve; each apply is idempotent from a dirty state; insert lessons are ≤ +5 dB re input (insertSend is exempt as a Send); §9 still counts 8 per type (no lesson files on disk); `editorScale` / `uiLanguage` round-trip.

**Verification:**
- [ ] The gates above are green; the existing `state-v1.14.0.bin` gate is green
- [ ] Task 2 null: bit-exact

**Dependencies:** Task 4 (same files; serial) · **Effort:** Medium

---

### Task 6: Editor rewrite (C++)

**Outcome:** `PluginEditor.h/.cpp` takes the Amb shape and the bridge census is set-equal.

**Steps:**
1. Eight relays: a combo relay for `TYPE`; slider relays for `CHARACTER, WET, DRY, DECAY, SIZE, LPFREQ`; a **`WebToggleButtonRelay`** for `LPON`. Declare the relays before the WebBrowserComponent and the attachments (member order).
2. **20 natives:**
   - Family set: `getParameterDefaults`, `getUiLanguage`, `setUiLanguage`, `uiReady`, `applyFactoryPreset`, `getPresetCatalog`, `getPluginVersion`.
   - Models: `getReverbModel`, `getEarlyReflectionTaps(type,size)`, `getToneCurve(type,character,lowCutHz,lowCutOn)`.
   - preset-manager.js: `loadPreset`, `getPresetList`, `getCurrentPreset`, `selectNextPreset` and `selectPreviousPreset` (both using the walk order), `savePreset`, `deletePreset`, `isFactoryPreset`, `loadPresetFromFile` (kept), and `savePresetWithDialog` (**refused**).
3. Models: an editor-owned `osr::EarlyReflections erModel`, prepared at the processor rate (48 k fallback) and read through `tapSamples` / `tapGain` (§3). For Spring, return the `springEchoesMs` from `SpringEngine::kSpring`. `getToneCurve` returns 128 log bins, 20 Hz up to min(20 k, 0.49 fs). It evaluates the biquads in double, through the Task 3 statics, and returns a separate `lowCutDb[]` (§4).
4. Resize: `applyUiScale`, `editorScale`, `setResizeLimits` 0.6–1.5× and `setFixedAspectRatio`. Keep `setSize (1180, 780)` as the **last literal** `setSize` in the file (check-ui-labels parses it).
5. A 30 Hz timer: `emitFrame` (hash-gated, `emitEventIfBrowserIsVisible`) and `emitPresetState`. Drain one `read()` at construction. Add gesture tracking, `withKeepPageLoadedWhenBrowserIsHidden`, and the `#if JUCE_WINDOWS` block with `withBackend(webview2)` and `withUserDataFolder()`.
6. Resource provider: serve `/modules/preset-manager.js` and `/img/insects.png`. Remove the `paper.jpg` and `flora.png` branches.
7. render-check: the tone curve matches the real filters within 0.1 dB (12 freqs × 5 CHARACTER × 6 types); the ER native matches an independent `EarlyReflections` and the §12 first arrivals; the decay well tracks the tail (wetDb slope within ±20 % of base × DECAY, 6 types × DECAY {0.5, 2.0}); the mutant `toneCurveStale`; `--write-frames` writes `tests/ui-fixtures/frames.json`.

**Verification:**
- [ ] Census script: the names in `index.html` + `preset-manager.js` that `getNativeFunction(...)` calls equal the `withNativeFunction` names in `PluginEditor.cpp` (20 = 20)
- [ ] render-check green, including the new mutant
- [ ] 0 warnings

**Dependencies:** Task 5 · **Effort:** Large

---

### Task 7: Page (index.html + i18n.js + assets)

**Outcome:** the production page is the signed-off mockup, wired to the real relays, natives and frame events.

**Steps:**
1. Rebuild `Source/ui/public/index.html` from `mockup/index.html`, as a single file plus an inline module (Amb shape). Apply the sign-off amendments:
   - **M1:** the Tone and Mix knobs sit on the Engine knob baseline (all six aligned), with the IN/OUT pill above.
   - **M2:** the type names stay English in fr and zh.
   - **M3:** the pill words are IN/OUT, ACT./DÉS. and 开/关.
   - **M4:** the decay well shows absolute dBFS.
   - **M5:** the spare space stays empty.
   - **Q2:** the gear menu gets "Load file…".
   - **Q3:** no "absorption" anywhere.
   - **Tone well:** calls the native `getToneCurve`. Drop the mockup's JS curve.
   - **ER and tone calls:** latest-wins, one call in flight, triggered on TYPE / SIZE / CHARACTER / LPFREQ / LPON change.
   - **Decay well:** scrolls by wall clock (rAF), holds the last value, freezes on live → idle, and draws the brass slope from `getReverbModel().baseT60 × DECAY` after the pre-delay.
   - **Out-meter:** Amb JS.
   - **Engine station sub-line by type:** "16 lines · Hadamard" (+ "· slow modulation" for Room / Hall / Ambient); Plate: "Dattorro figure-8 · octave shimmer in the loop"; Spring: "3 dispersive springs".
   - **Footer:** "Ouaricon Audio · O-simpleReverb" | tour caption | technical note. The wordmark is i18n-exempt. Show the version from `getPluginVersion` in the gear popover.
2. `js/i18n.js`: write the en/fr/zh glossary **first** (memory `pattern_parallel_localizers_need_glossary_first`), then the keys. Keep the canon block byte-identical. fr `reviewed:false`, zh `reviewed:'mt'`. Tooltips explain *why*. Use setLabel tokens and `data-i18n-vars` for composed readouts.
3. Assets: copy `img/insects.png` from Amb; delete `img/paper.jpg` and `img/flora.png`. In CMake binary data add `modules/preset-manager.js` and `img/insects.png`, and remove the two deleted images.

**Verification:**
- [ ] The Standalone opens at 1180×780. Every control moves its parameter and every well responds within one frame of a gesture (hands-on via `/show-standalone`)
- [ ] `grep -i absorption` over `Source/ui` returns zero
- [ ] check-i18n green, including the embedded-and-served pairing for preset-manager.js

**Dependencies:** Task 6 · **Effort:** Large

---

### Task 8: Test tree and suite gates

**Outcome:** the Reverb test tree matches the family and every gate is green.

**Steps:**
1. `tests/ui-stub/generic-overrides.json`: the real ranges (§7.1) and the static natives (catalog 11 + 48 + 2 user, getPresetList, getReverbModel, Room-50 taps, Room-flat tone curve).
2. `tests/ui-fixtures/frames.json`, produced by `render-check --write-frames` (C++-producible states only).
3. `tests/ui-probes/`:
   - frame-probe (decay-well and out-meter states);
   - header-fit-probe (`NAME='O-simpleReverb'`);
   - lesson-row-budget for **two rows** (6 and 5), each pinned at fr;
   - tooltip-clamp-probe.
4. `tests/i18n-states.json` rewritten: LPON OUT, Plate and Spring taps, preset popover, gear popover.
5. `tests/pedalboard/verify_3_state.py`: decode a v2.0.0 `raw_state`, load it in v3, check all 8 values by name plus `uiLanguage`; round-trip `editorScale`.
6. Retire `tests/ui_tip_render_check.js`.

**Verification:**
- [ ] check-i18n, `check-ui-labels --plugin O-simpleReverb`, i18n-fr-lint, i18n-zh-lint and `boot-all-uis --plugin O-simpleReverb` are green
- [ ] All 4 probes pass at 0.6×, 1.0× and 1.5× in en, fr and zh-Hans (no overflow)
- [ ] verify_3_state passes; verify_3_null still bit-exact

**Dependencies:** Task 7 · **Effort:** Large

---

### Task 9: Install and validate

**Outcome:** the installed binaries are validated and the old names are gone.

**Verification:**
- [ ] `./scripts/build-and-install.sh O-simpleReverb`: 0 warnings, both old-case variants swept, AU cache cleared
- [ ] `auval` passes; pluginval level 10 passes for VST3 and AU
- [ ] render-check: 126 + new gates, 30 + 2 mutants, all green
- [ ] verify_3_null bit-exact on the **installed** VST3; verify_3_state green
- [ ] `ls ~/Library/Audio/Plug-Ins/{VST3,Components} | grep -i simplereverb` shows only the new-name bundles

**Dependencies:** Task 8 · **Effort:** Medium

---

### Task 10: Docs and registry (no version bump, no tag)

**Outcome:** the docs and registries reflect O-simpleReverb.

**Steps:**
- `.planning/` BRIEF, REQUIREMENTS, ROADMAP and STATUS in Amb's YAML-frontmatter shape. `activeMilestone: family-retrofit` stays until post-verify.
- Regenerate `.planning/params.tsv` through param-dump.
- Draft a `CHANGELOG.md` 3.0.0 entry with these notes: the rename, why no preset-folder migration is needed, the guarded rename for case-sensitive volumes, the ◀/▶ walk-order change, and the dropped Save-to-file dialog. It is finalised post-verify. Old entries are untouched.
- `NOTES.md` updated.
- `PLUGINS.md` Reverb row only. `scripts/regen-registry-used-by.sh`, commit the Reverb-related hunks only.
- Path-scoped commit.

**Verification:**
- [ ] `grep "^| O-" PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d` is empty
- [ ] Re-grep for live `O-SimpleReverb` hits: zero

**Dependencies:** Task 9 · **Effort:** Small

---

## Dependency Graph

```
T0 ─> T1 ─> T2 ─> T3 ─> T4 ─> T5 ─> T6 ─> T7 ─> T8 ─> T9 ─> T10
       │            null   null   null   census  gates  gates  install
       └─ own commit
```

**Parallelizable:** almost nothing. Tasks 3–6 share `PluginProcessor.*` and the editor, and each audio task is gated on a null. The one safe overlap: the Task 7 i18n glossary and fr/zh tooltip drafting can start while Task 6 is building.

## Execution Order

| Order | Task | Gate before proceeding |
|---|---|---|
| 0 | Backup | diff clean |
| 1 | Rename | build + auval + render-check, own commit |
| 2 | Null harness | v2 deterministic; renamed nulls |
| 3 | Tone statics | null |
| 4 | Payload | render-check + null |
| 5 | Preset core | render-check + null |
| 6 | Editor | census 20 = 20 + render-check |
| 7 | Page | Standalone hands-on + check-i18n |
| 8 | Tests | full suite matrix |
| 9 | Install/validate | auval, pluginval 10, null on the installed binary |
| 10 | Docs | duplicate-row check, zero live hits |

Suggested commit points: after T1, T2, T5, T6, T7, T8 and T10, each path-scoped.

---

## Risk Notes

| Risk (RESEARCH §10) | Mitigation built into |
|---|---|
| R1 case-only mv in the shared index | T1 two-step mv, own commit, re-check status right before committing |
| R2 stale cache, SKIP_PLUGINS or canary | T1 steps 5–6, zero-hit grep |
| R3 AU cache and dual variants | T1 and T9 via build-and-install.sh |
| R4 bridge gap (dead control) | T6 census, 20 = 20 |
| R5 tone or ER drift | T3 shared statics, T6 gates and mutant |
| R6 walk-order surprise | T10 CHANGELOG |
| R7 accumulator overflow | T4 halving cap |
| R8 French lesson-row width | T8 lesson-row-budget per row at fr |
| R9 "absorption" copy | T7 grep |
| R10 dirty PLUGINS.md | T10 row-only and path-scoped |
| R11 v1.14.0 negative control | T4 `__has_include` guard |
| **New:** v2 not deterministic | T2 step 2 stops the plan before any edit relies on the null |

---

## Domain Agent Instructions

**Execute agent:** general-purpose. The domain is mixed (GUI 9 / DSP 3 / Polish 2). The gui-agent lacks Bash, and render-check and the gates need it.

**Rules:**
- No allocation, locks or logging on the audio thread. The meter accumulators are plain members, published once per block.
- Relays are declared before the WebBrowserComponent and the attachments.
- Every `getNativeFunction` name has a `withNativeFunction` (census).
- WebView2: `withBackend(webview2)` + `withUserDataFolder()` on Windows.
- Lesson apply resets to defaults first, and the recipe goes through `convertTo0to1` (skew-aware).
- UI fixture states come from C++ (`--write-frames`), never hand-written.
- Canvas text: `document.fonts.load()` then repaint (EB Garamond).
- `pointerdown` preventDefault kills document mousedown dismissers. Use pointerdown dismissers for the popovers.

**Read first:**
- `RESEARCH.md`: decisions D1–D8, §§3–9, and the sign-off table
- `mockup/index.html`: the page source of truth
- Amb `Source/PluginEditor.cpp`, `Source/MeterFramePayload.h`, `Source/PresetRecipes.h`, `Source/PluginProcessor.cpp:300-501`: the shapes to port
- Reverb `Source/PluginProcessor.cpp` (`typePresets :58-160`, `renderSlot`, tone stage `:445-506`, `:778-800`): the tap points
- `modules/persistence/preset-manager/cpp/OuariconPresetManager.h`: the state authority (unchanged)

---

## Success Criteria

From CONTEXT.md:

1. [x] The mockup was signed off before plan (2026-10-15)
2. [ ] O-simpleReverb loads v2.0.0 sessions and presets with identical values; auval and pluginval 10 pass for VST3 + AU (T5, T8, T9)
3. [ ] All four wells respond live; the decay well tracks the real tail within ±20 % (T6, T7)
4. [ ] 10 lessons + Init from PresetRecipes.h; 48 utility presets browsable (T5, T7)
5. [ ] Family gates green, plus render-check 126 + 30 mutants and the new gates and mutants (T8, T9)
6. [ ] Bit-exact null against v2.0.0 (T2–T5, T9)
7. [ ] 0 warnings; old bundle names swept (T9)

---

*Generated by improve-milestone plan phase*
