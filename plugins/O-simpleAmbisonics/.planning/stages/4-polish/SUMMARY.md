# Stage 4 — Polish — SUMMARY

**Plugin:** O-simpleAmbisonics · **Target:** v1.0.0, **no tag** · **Date:** 2026-10-14
**Status:** Part 1 (T1–T11) complete → ⏸ CP1 preset-bar visual checkpoint (Taylor). Part 2 (T12–T15) not started.
**Gate:** `run-gate.sh O-simpleAmbisonics 3-gui 4-polish`: build / pluginval / dsp-critic / ui-critic PASSED; its interactive code-review prompt was answered with `--skip-review` because T2 *is* that review (all four critics).

## Part 1

### T1 — pre-flight + baselines
- `main`, 1 worktree. Foreign dirty paths at start (other sessions; never staged or touched): `.claude/agent-memory/{dsp,gui}-agent.md`, `PLUGINS.md`, and the `withBackend` WebView2 sweep across O-Bassoon, O-Freeze, O-Gain, O-Octagon, O-Reed, O-ReverseDelay, O-Tapestop, O-simpleAdditive, O-simpleBeatmaker, O-simpleFM (plus its own STATUS / NOTES / page / processor WIP), O-simpleGrain, O-simplePhysicalModelSynth, O-simpleSampler, O-simpleWavetable (CHANGELOG / CMakeLists / PluginEditor). **Deviation:** O-simpleWavetable was dirty, not clean as the plan assumed → read through `git show HEAD:` only, like O-simpleFM. `modules/persistence/preset-manager` clean.
- R-BUILD 0 warnings; harness 251 / 251; digest-2.1 `9189d911a1fe98d5` PASS, digest-2.2 `7f35bc9f91e321b9` PASS, digest-2.3 `edd1acfd21f54bcd`; PERF-01 0; `~/Library/O-simpleAmbisonics` absent.
- Header baseline (serve-ui + generic stub, 1180 × 780, order 3): en / fr / zh `.title-block` 425, `.header-strip` 655 / 655, `#chipFormat` 236, `#chipLatency` 196, `#gear-btn` left 1135 — equals Stage 3.

### T2 — critic pass + triage
`critic-orchestrator` ran all four critics (dsp 7.8 PASSED, ui 6.8, architecture, foundation 8.5); `merge-critic-reports.py` → `critic/unified.json` exit 1: **3 blocking reports of ONE defect**, 7 warnings, 25 notes / info. No finding needed a frozen-contract change (no STOP). Full triage, verdicts and gates: `plugins/O-simpleAmbisonics/CODE_REVIEW.md`.

| ID | Sev | Finding | Verdict | Task |
|---|---|---|---|---|
| WR-01 | blocker ×3 | no `.withBackend (webview2)` → Windows "page not found" | fix | T3 |
| WR-02 | warning | one NaN / Inf input sample latches the air-LP state → silent for the session | fix | T3 |
| WR-03 | warning | `reset()` not overridden | fix | T3 |
| WR-04 | warning | Input Mode switches in one sample (click) | fix | T3 |
| WR-05 | warning | tail length 0 | fix | T3 |
| WR-06 | warning | lessons reset Input Mode (R-D3) | fix | T6 |
| WR-07 | warning | lesson ids in four hand copies; a bad id looks applied | fix | T6 / T8 / T10 |
| IN-01, 04, 07, 08, 09, 11, 19, 20 | note / info | unprepared processBlock spin; kConvMaxK 16 < K 32 at 384 k; DPR change; canvas label contrast; knob ARIA keys; right-click gesture; editorScale carry-over; VST3 category | fix | T3 |
| IN-13, IN-14 | note | stale lesson light; single-string user-owned exclusion | fix | T8 / T6 |
| IN-16 | note | ARCHITECTURE predates Stages 2–4 | fix | T14 |
| IN-17 | note | page literals of C++ facts | partly (chip retired) | T8 |
| IN-02, 06, 12, 18 | note / info | +15 dBFS corner; language on restore-into-open-editor; small text at 0.6×; open-time fit persisted | accepted by design | — |
| IN-03, 05, 10, 15, 21 | note / info | onset vs plan; FFTFallback SpinLock; fr decimal commas; TripleBuffer one-tick; pluginVersion unread | reviewed, not changed | — |

### T3 — critic fixes (executed by the orchestrator, sequential, one writer)
**Deviation:** the plan allowed dispatching dsp-agent / gui-agent; the orchestrator made every edit itself (one writer throughout, no parallel edits).
- `PluginEditor.cpp` `.withBackend (Backend::webview2)` before `withWinWebView2Options` (WR-01).
- `PluginProcessor.cpp` conditioner: non-finite → 0 (WR-02); a seeded 20 ms `LinearSmoothedValue` Left-only weight, the exact Stage-2 branches outside a ramp (WR-04); `reset()` override (engine.reset + re-seeded gains; no-op before prepare) (WR-03); `tailSeconds` = (64 + convolver taps at rate) / fs, set in prepareToPlay (WR-05); processBlock clears and returns when unprepared (IN-01); restore stores editorScale 0 when absent / rejected (IN-19). `BinauralConvolver.h` `kConvMaxK` 16 → 32 (IN-04). `CMakeLists.txt` `VST3_CATEGORIES "Fx" "Spatial"` (IN-20).
- Page: `watchPixelRatio()` (IN-07); ring / dB-ruler label alpha 0.45 / 0.6 → 0.75 ≈ 5.8:1 (IN-08); knob aria-valuemin / max / now + Home / End / PageUp / PageDown (IN-09); knob primary button only (IN-11).
- Gates (each DSP fix + an NC that FAILS as designed):

| Line | Value | NC |
|---|---|---|
| S4 WR-02 | 4 / 4 renders (Bin / St × 1 / 12 m) finite, alive, bit-equal to the zeroed input | engine-direct NaN → tail NaN |
| S4 WR-03 | silence after reset all 0; tone after reset bit-equal to a fresh instance | no reset: last non-zero frame 383 |
| S4 WR-04 | ratio 1.0026 vs the **same-motion** reference (Stereo; Binaural ≤ 2 too) | hard splice 579 |
| S4 WR-05 | 48 k Binaural measured 317 ≤ reported 320 samples (6.667 ms); 44.1 / 96 k, Stereo covered | tail 0 < 317 |
| S4 IN-01 | unprepared processBlock returns, buffer cleared | prepared instance outputs (liveness) |
| S4 IN-04 | K 30 / 32 ≤ kConvMaxK 32 | 32 > 16 |
| S4 IN-19 | absent → 0, "3.0" → 0 | — (not DSP) |
| S4 PERF | 0 allocations: Input Mode ramps every 10 blocks, a NaN, a reset() in the window | (PERF-01 liveness line unchanged) |

  **Deviation (WR-04, PLAN K7):** against the static Mono-sum / Left-only pair the switch reads 2.52 (Stereo) / 2.10 (Binaural) — the 20 ms linear ramp's own slope corner, the same corner every 20 ms ramp in the plugin has. The arm was moved to a same-motion reference (the legs pre-blended by the identical ramp, fed through Mono sum); the 2.0 ceiling was not raised. The static ratio is printed beside the verdict.

### T4 — verify_2_1 V3 → Stereo
`for dec in ("Stereo",)`; header note points the Binaural onset to `verify_2_3.py` V2 / V1; "placeholder" gone (`grep -ci` = 0). Installed VST3: **22 / 22 ALL PASS** (before and after the T11 install).

### T5 — encReadN + chevron / puck
- `label.encReadN` en "({n}+1)² = {ch} ch", fr "… can.", zh "… 声道" (+ the static markup fallback).
- `drawFrontChevron()`: drawn after the puck, a 3 px `--well-bot` backing stroke, the tangential slide anchored `[probe-anchor:chevron-slide]` (exactly once).
- frame-probe **A14**: A = pitch 0 (no chevron), B = pitch 40; chevron = B's new full-alpha warm pixels near the puck: 89 px, gap to the puck / halo / ▲ mask **8.25 px** (≥ 4). `--negative-control chevron` (slide disabled): the chevron lands on the puck — 0 own pixels, gap — → A14 FAILS as designed.
- header-fit-probe (new) **H1**: `.enc-read` 144 / 144 at orders 3 and 2 in en / fr / zh; NC `encreadn` (old copy) → H1 [en] FAILS as designed.

### T6 — recipe table, apply core, lessons, identity, state
- `Source/PresetRecipes.h` (`ambpresets`, header-only, ASCII) = the PLAN factory table verbatim (Init + 8; orderStep `{ order 0, azimuth 45 }`). `kLessons` deleted.
- Processor: `isUserOwnedParam` (outputLevel, inputMode — the one list), `recipeTargets` (own `convertTo0to1`, order override), `resolveTargets`, `applyNormalisedTargets` (the one core), `applyFactoryPreset` (lesson; orderStep walk per P4), `loadPresetByName` (browser; no walk), `applyUserPresetJson` (strict), identity (`presetLock`, name / targets / valid / stale / revision), `currentPreset` in get / set state (orderStep restored with its order index), `sanitisePresetName`. Ctor = Init, unmodified, no disk access. No hunk in `createParameterLayout` / `fillParams`; the T6 diff adds none to `prepareToPlay` / `processBlock` (those carry only the T3 fixes).
- Harness: ST6 amended; G-FACTORY[table / init / apply / skew-NC]; G-LESSON[walk 0 1 2 0 / restart / owned] + NC; G-ORDER3-DIST (22 states, 0 violations) + NC; G-PRESET-STATE[identity / restore] (hostile "../x" + U+202E + BEL → "xy"); G-PRESET-CLICK + NC.
- **G-PRESET-CLICK (QUAL-01) values:** 12 transitions (8 bank loads, then orderStep 0 → 1 → 2 → 0); 9 ≤ 2.0 against the static pair. **Deviation (PLAN T6 note / K7):** 3 arms exceed the static pair from their legitimate smoothed moves — Overhead (elevation 90 + order 2: 2.58), Speakers vs Headphones (decoder → Stereo + order 3: 2.16), the following orderStep click (Stereo → Binaural + order 3 → 1: 4.33). Each switched to the same-motion `ReferenceModel` reference (Stage-2 DSP-07 / B8 method): 0.9999 / 1.0001 / 1.0003 — the preset load adds nothing to the smoothed jump. Ceiling unchanged. NC splice: 268.1 / 10.4 / 62.1 → FAILS as designed.

### T7 — module, natives, presetState, user presets (TRACER GATE)
- CMake: `ouaricon_add_module(O-simpleAmbisonics preset-manager)` before the one binary-data target; `Source/ui/public/modules/preset-manager.js` added to its SOURCES (gitignored, never committed).
- Processor: `presetManager { apvts, "O-simpleAmbisonics" }` (folders, `savePreset`, `deletePreset`, `initializeFactoryPresets` only), `buildWalkOrder`, `neighbourIn`, `getPresetWalkOrder`, `getNeighbourPreset`, `buildFactoryPresetDefs` (10 keys), `ensureFactoryBankOnDisk` (lazy, natives only), `saveUserPreset` (refusal before any I/O), `deleteUserPreset`, `getPresetCatalog`, `refreshPresetTargetsIfNeeded`.
- Editor: `/modules/preset-manager.js` branch; 11 new natives (16); `emitPresetState`; timer `refreshPresetTargetsIfNeeded → meterFrame → presetState`; `uiReady` snapshots the counters first, then forces both events.
- Harness: G-FACTORY[defs]; G-PRESET-USER[roundtrip / malformed (6 / 6 roots) / missing / clamp / refuse (18 / 18)] + NC; G-PRESET-NAME (9 / 9); G-PRESET-WALK; "S4 … never wrote the real preset folder".
- **Tracer gate:** (1) 0 warnings, module header 0; (2) harness green, `~/Library/O-simpleAmbisonics` untouched; (3) 16 natives, `presetState` in the built VST3; (4) S2 = 0 (+ `presetManager.savePreset` = 1), S4 every SOURCE has a branch, `preset-manager.js` gitignored, S7 clean; (5) boot-all-uis `--strict-tips` 0 warn / 0 failed / 0 DEAD. **(6) carried to CP1:** a live Standalone lesson click cannot be driven headlessly; its processor half is gated (ST6, G-LESSON, G-PRESET-STATE) and its page half by the stub probes.

### T8 — header preset bar (R-D1)
- `#chipLatency` markup + CSS retired; FM-style bar in its slot: ◀ 22 · `#presetName` 213 (ellipsis; a brass modified dot) · ▶ 22 · Save 64 · Delete 64, gap 5; page-built listbox under the name (factory labels via `FACTORY_LABELERS`, a dotted separator, raw user names), dismissed by capture-phase `pointerdown` outside + Escape; one Save / Replace? / Delete? popover (title, input, nowrap message, Cancel / OK). `preset-manager.js` imported dynamically (failure disables the bar only).
- P11: the lesson click only applies; `presetState` lights the button iff unmodified and sets the caption (else `label.tourDefault`). `setRaw` added (the page had none). The name box keeps the static "Init" until the first push (no "—" flash).
- **Pins as measured** (text width + 20 px): Save en 24.9 / fr 39.5 / zh 21.6, Delete 40.6 / 36.1 / 21.6 → 64; name: widest factory label fr "Source ponctuelle (FOA)" 119.8 (en 115.9, zh 84.2) → ≥ 139.8, largest keeping the strip's 4 px slack = 213; popover buttons fr ANNULER 53.0 → 74; list option / popover title / message lines pinned at the zh line box (24 / 16 / 16 px).

### T9 — i18n, states, stub
- New: 5 tips (`presetPrev / Name / Next / Save / Delete`, bodies: presets never move Output Level or Input Mode), 18 labels; removed `chipLatency`, `label.latencyBody`, `label.latencyDetail`; the latency sentence appended to the Decoder tip in en / fr / zh; `label.tourCaption.orderStep` rewritten. Glossaries already carried every term (no shared-file edit). fr `reviewed: false`, zh `'mt'` throughout; fr `preset.ok` "OK" `sameAsEn: true`. Counts: fr / zh entries 149 → **169**.
- `i18n-states.json`: 8 lesson states emit their `presetState` (orderStep → order 1); + 12 preset states (Init, the widest label modified, a 64-char user name, list open / closed, Save open / empty / factory-label refused / Replace? / closed, Delete? / closed). `generic-overrides.json`: the 4 preset natives with the real bank + 2 user names.

### T10 — Part-1 gate sweep
| Gate | Result |
|---|---|
| check-i18n / i18n-fr-lint / i18n-zh-lint (`--plugin`) | exit 0 / 0 / 0; zh 169 entries checked, 0 findings |
| check-ui-labels | exit 0, `seed=overrides` (first run: 11 [7] geometry fails — Cancel width in fr, zh line boxes in the list / popover — fixed by the pins above) |
| boot-all-uis `--strict-tips` | 0 warn, 0 failed, 0 DEAD, 0 late; text 120, aria 23, i18n 109 |
| frame-probe | 14 / 14; NCs fuma / pole / idle / chevron FAIL as designed |
| header-fit H1 / H2 / H3 | ALL PASS en / fr / zh: title 425, gear 1135, strip 655 / 655, every caption fits, 9 factory labels fit, 64-char user name ellipsised 301 / 187 with box 213 and lefts [712, 739, 957, 984, 1053] unchanged; NC encreadn FAILS |
| lesson-row-budget | exit 0; fr slack 74.0 px (≥ 8); [P11] click lights nothing / presetState lights / modified lights nothing |
| tooltip-clamp | ALL PASS at 1180 × 780 and 708 × 468 incl. `#presetDelete` (enabled via a user presetState) |
| generate_sh_icons `--check` | matches |
| R-STATIC S1–S7 | clean. **Deviation:** S4's literal `grep -c juce_add_binary_data CMakeLists.txt` = 1 never held (HEAD = 3, comment mentions); the invariant `^juce_add_binary_data` = 1 holds, count kept at HEAD's 3 |
| R-BRIDGE | `comm` empty; 16 natives; events {meterFrame, presetState} both sides |

### T11 — build, install, validate
- R-BUILD 0 warnings; harness **289 / 289** (251 + 38; only the amended ST6 differs from the T1 baseline besides the new lines), digests 2.1 / 2.2 PASS, 2.3 `edd1acfd21f54bcd`, PERF-01 0, PERF-02 ≈ 190×.
- R-INSTALL (`build-and-install.sh` + Standalone): installed VST3 / AU `cmp`-identical; `presetState` in the VST3 and the Standalone binaries.
- `auval -v aufx OSAm OuDv` SUCCEEDED; pluginval s10 VST3 SUCCESS, AU SUCCESS.
- verify_2_1 22 / 22, verify_2_2 35 / 35, verify_2_3 42 / 42 ALL PASS; verify_3_state S1–S5 ALL PASS (S4 `currentPreset="Init"` → "Test" restored + re-saved; S5 absent → "").
- pluginval's editor test wrote the factory bank once (9 JSON, 10 keys each, no outputLevel / inputMode; `.factory-version` 1.0.0) — K5, accepted.

### Open for CP1 (Taylor)
Items 1–6 of the plan's CP1, plus tracer item 6 (a live lesson click in the Standalone moves its controls, Output does not move) and IN-07 (a DPR change between displays repaints sharp).
