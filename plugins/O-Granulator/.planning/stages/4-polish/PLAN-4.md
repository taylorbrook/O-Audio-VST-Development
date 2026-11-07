# Stage 4 — Polish / Validation — Plan

**Plugin:** O-Granulator
**Stage:** 4 of 4 (Polish)
**GSD phase:** plan
**Date:** 2026-11-07
**Branch:** `main` (trunk-based) @ `2d388e52`

**Inputs:**
- `CONTEXT-4.md` (D42–D47 locked)
- `RESEARCH-4.md` (R134–R156). **Where it differs from CONTEXT-4, RESEARCH-4 wins**, as amended by the sign-offs and P143–P162 below.

**Re-read at plan:**
- `FeedbackChain.h:179–220`: `stepAgc (xl, xr)` runs on the un-normalised peak, and `d = satD.value (c)` is in scope at the call site (per sample, ramped).
- `EngineConstants.h:140`: `kAgcCeil = 0.7f`.
- `Engine.cpp:563`: the `loopAgcTel` formula.
- `engine_gates_26.h:342, 435`: Z7h3 is INFO.
- `engine_gates_25bb.h:1099–1122`: Z6a goes through `eg22b::renderPool`.
- `renderPool` call sites: `22b:360`, `24:853`, `24:931`, `24b:577`, `25ab:993`, `25bb:1099`.
- `main.cpp:621`: section S state codec, with the `make()`, `stateOf()`, `load()` and `sameNonParamState()` helpers.
- `PluginProcessor.cpp:184–187`: no `initializeFactoryPresets` call exists yet.
- `CMakeLists.txt:34`: `VERSION 0.1.0`. There is no `CHANGELOG.md`.
- `../O-Lyrica/tests/state-roundtrip/two_reopen.py` exists.

---

## Sign-offs (Taylor, 2026-11-07, at plan)

| # | Item | Decision |
|---|---|---|
| 1 | **D43** placement (R135) | **Option 3.** The follower runs on `d·max(|yL|,|yR|)` (= `|tanh(d·x)|`), and `g = min(1, kAgcCeil/env)` is unchanged. The ceiling tracks saturation. Sat 0 is bitwise identical. |
| 2 | **D42** descriptions (R142) | **Docs only.** A plugin-side table feeds the CHANGELOG and README. Descriptions are not rendered in the UI, so no new i18n strings. |
| 3 | **AG4** pumping threshold (R136/R137) | **≤ 1 dB p-p** g_agc modulation depth over 400 ms windows, at defaults + fb 50 %, on a 0 dBFS transient loop. On a FAIL, **stop and ask**. Do not raise `kAgcCeil` without a sign-off (that would lose AG3). |
| 4 | **COMPAT-03 Windows** (R149) | **No push or dispatch in Stage 4.** The Windows VST3 build is proven by release CI at `/publish`. COMPAT-03 closes at `complete-pending-CI`. |

---

## Entry Check

- Re-hash the six contracts against the STATUS.md frontmatter. Stage 4 edits none of them. A version bump is not a contract edit.
- `git diff --stat 1455e501 HEAD -- plugins/O-Granulator/Source plugins/O-Granulator/tests plugins/O-Granulator/CMakeLists.txt modules scripts` is empty, which means the tree is the one 3.3 verified.
- `cmp Source/ui/public/index.html .planning/mockups/v2-ui.html` is clean.

---

## Goal

Ship-ready 1.0.0 candidate: **FUNC-09, COMPAT-02, COMPAT-03, UI-04, QUAL-04**, with the AGC alive at defaults.

- **D43:** the AGC engages at every saturation setting (AG1–AG5 + AG1n). Sat-0 audio is bitwise unchanged. Moved rows are re-baselined deliberately, then **baseline D** is frozen.
- **D47:**
  - The Env 1e30 pin (Z7h3 becomes a row).
  - `uiSnap` is cleared in `prepare`.
  - `renderPool` is event-aware, and Z6a gets a liveness clause.
  - C7 gets machine-speed calibration.
  - The W2o rule is written down.
  - Audio stays bitwise vs D.
- **D42:** 16 factory presets with name-prefix categories, authored in engineering units with fixed seeds. Gated on the build-tree bank (PR1–PR5).
- **COMPAT-02:** state round-trip on a fresh instance, through the codec, the preset files and the installed VST3 (ST1–ST4).
- **COMPAT-03:** layouts LY1 / LY1n. Windows is covered at `/publish`.
- **UI-04:** the i18n gates stay green (no new strings), plus a preset-name fit check.
- **Release:** version 1.0.0, `CHANGELOG.md` `## [1.0.0]`, install, auval, pluginval 10 VST3 + AU. **No tag.**
- **QUAL-04:** HANDS-ON-4.md + DAW-4.md (Logic AU + Reaper VST3). Taylor runs both at verify.

---

## Plan-phase decisions

- **P143 — baseline C on the unmodified tree (T0).**
  - Run the harness default set in the background (600 s watchdog).
  - The carried extract (sorted `[PASS]/[FAIL]`, INFO/timing dropped) must equal the 520-row 3.3-verify extract. That extract is **C**. If it differs, stop.
  - Record the pre-state:
    - N3 / N2 / N1 / N0 `--strict` + R12 rc 0
    - HA PASS
    - codec bytes 9/9 == B
    - param-dump clean
    - `generic-overrides.json` ×2 byte-identical
    - the five UI gates + probe 220 × 2
  - No commit.

- **P144 — D43 code (R135).**
  - Change the signature to `stepAgc (float& xl, float& xr, float d)`, with `peak = d * std::max (|xl|, |xr|)`. Pass the per-sample `d` at `FeedbackChain.h:219`.
  - Add a test hook behind `#if OGRAN_TEST_HOOKS`: `bool testAgcLegacy = false;`. When it is true, use the old un-normalised peak.
  - Update the doc comment: "Stereo-linked peak AGC on the normalised saturator output; env ≤ 1 structurally."
  - `Engine.cpp:563` and the LG readout are **unchanged** (R135).
  - No other DSP statement changes.

- **P145 — D43 gates in the new `tests/render-harness/engine_gates_4.h`** (section AG; R137):
  - **AG1:** the FB4 drive-100 corner at sat 20 %, fb 100 %, 20 s.
    - min g_agc over 10–20 s < 0.9.
    - The return peak ≤ `kAgcCeil/d` × (1 + m), where m is the measured attack overshoot, recorded and fixed at T1 as `≤ m_meas + 0.01`.
    - Finite.
  - **AG1n:** the same render with `testAgcLegacy`. g_agc ≡ 1 for the whole render. **Must FAIL AG1's predicate.**
  - **AG2:** the FB3n phase-locked growth config at sat 0 / 20 / 50 / 100 %.
    - min g_agc < 1 at every sat.
    - Hold at `0.7/d ± tol` (tol measured and recorded).
    - No growth after the hold: the per-generation energy fit has a slope ≤ +0.05 dB/gen (pattern_feedback_loop_phase_lock; fit energy, not mean frame dB).
  - **AG3:** every carried FB / Z7d / TA1 row's render, re-run at **sat 0**, is bitwise equal to the same render on a `testAgcLegacy` instance. This is the in-harness proof that sat 0 is untouched, independent of the C extract.
  - **AG4** (sign-off 3), at defaults + fb 50 %:
    - (a) −6 dBFS-peak noise bursts, 30 s: g_agc ≡ 1.
    - (b) a 0 dBFS-peak transient loop (the harness kick/snare synth, seeded), 30 s: modulation depth ≤ 1.0 dB p-p over 400 ms windows. Record min g_agc and the depth.
    - On a FAIL: **stop and ask.**
  - **AG5:**
    - Z7d config at default sat: exact zero is reached, at a time ≤ the C value.
    - FZ / TA1 freeze rows: **bitwise vs C** (the ring write is stopped while frozen). If one moves, stop and investigate (R137).
  - **AG6** (FB4 bound tightening, R138): env ≤ 1.0 + 1e-6 across the FB4 corner set. This replaces FB4's `≤ 0.7 + 1/d` clause. It is a tightening, recorded.

- **P146 — D43 re-baseline protocol (R138, W2o).**
  - Run the default set and diff its carried extract vs C.
  - Expected movers: FB2a/b/c, FB3n, FB4, LG*, Z7d, and fb > 0 golden digests.
  - For every moved row, SUMMARY-4 §D43 gets: row id, old → new value, a one-line cause, and the sat setting.
  - **Any moved row whose config has sat == 0 is a defect** (contradicts AG3). Stop.
  - **Never loosen a threshold.** A seeded row that now fails gets a wider seed set, never a wider threshold (W2o).
  - Golden digests that move are regenerated with an attribution line in `tests/golden/README` (or the existing ledger).
  - Then N-suites `--strict` + R12 + HA:
    - Expected unchanged unless a suite renders fb > 0 at sat > 0. In that case the suite's reference is re-baselined under the same protocol and recorded.
    - codec bytes 9/9 == B (state is unaffected).
  - **Freeze baseline D** = the post-D43 carried extract + N-suite references + goldens. **Commit.**

- **P147 — D47(a) Env pin (R151).**
  - In `ModSources.h` `EnvFollower::process`, after `x = |s·g|`, add `if (! (x <= kAnalysisTapMax)) x = 0.0f;`.
  - Z7h3 becomes a **row**: Env cells after the 1e30 injection == the zero-sample reference.
  - **Z7h3n:** a hook (or the pre-change header) shows the pinned ≥ 0.999 count > 0. This keeps the row from passing vacuously.

- **P148 — D47(e) `uiSnap` clear (R155).**
  - At the end of `Engine::prepare`: `uiSnap.writeBuffer() = UiSnapshot{}; uiSnap.publish();`.
  - New row **US1:** render a busy 2 s → `prepare` → the first `readUiSnapshot()` shows 0 grains, not frozen.
  - **US1n:** without the clear (hook), the first read shows grains > 0.
  - JUCE guarantees `prepareToPlay` is not concurrent with `processBlock`, so single-writer still holds.

- **P149 — D47(b) event-aware `renderPool` (R152).**
  - **Audit first.** For each of the 6 call sites above, log whether any job carries non-empty `events`. Put the table in SUMMARY-4.
  - Then dispatch inside `eg22b::renderPool`:
    - Jobs with `events` go through the `renderPoolEv` setup/loop path (`engine_gates_26.h:529`).
    - Jobs without `events` use `runLoop` unchanged, which keeps their bytes by construction.
  - Z6a gets a liveness clause: `freezeEngages ≥ 3` (the Z7a counter).
  - Any other row that moves is **by design** (it now honours events it always declared). Record it as old → new with the job name. **Never re-baseline a row without events.**

- **P150 — D47(c) C7 calibration (R153).**
  - A ~50 ms dependent-FMA chain with a volatile sink, timed before and after each of the 7 runs.
  - A run with ratio > 1.10 vs the session's first calibration counts as contended. Fewer than 5 valid runs → `[NOVERDICT]` (the existing path).
  - Print ns/op.
  - Set the timing thread to `QOS_CLASS_USER_INTERACTIVE` (macOS only, behind `#if JUCE_MAC`).
  - The C7 threshold is unchanged.

- **P151 — D47(d) W2o.** Add a note to the `engine_gates_22b.h:30` header and to this plan's protocol (P146): *a seeded row that moves gets a wider seed set, never a wider threshold.*

- **P152 — D47 neutrality.**
  - After P147–P151, the carried extract vs **D** may differ only in:
    - Z7h3 INFO → row
    - Z6a (+ any P149 by-design movers, attributed)
    - C7 output format
    - new rows
  - N-suites + R12 + HA + codec bytes + block-size null unchanged vs D.
  - **Commit.**

- **P153 — factory preset table (R140–R142, sign-off 2).**
  - New file `Source/FactoryPresets.h` containing:
    - `struct FactoryPresetSpec { const char* name; const char* description; int64_t seed; std::vector<std::pair<const char*, float>> values; juce::String customJson; };`
      - `values` are in **engineering units**: Hz, ms, %, dB, choice *index*, bool 0/1.
    - `inline std::vector<FactoryPresetSpec> factoryPresetSpecs()`
    - `inline std::vector<OuariconPresetManager::FactoryPresetDef> makeFactoryPresets (juce::AudioProcessorValueTreeState&)`, which converts each value once via that parameter's own `getNormalisableRange().convertTo0to1` (choice/bool via their ranges).
  - `makeFactoryPresets` asserts (jassert + harness row) that:
    - every key is a real parameter
    - `output_gain` is absent
    - every value is inside its range
  - Each def's `customState` = `{"seed":"<int64 decimal>"}` (plus `custom` / `masks` for harmoniser presets that need them). Descriptions go in the spec only; they are inert in JSON.
  - `PluginProcessor.cpp`: call `presetManager.initializeFactoryPresets (makeFactoryPresets (apvts))` next to the existing setup at `:184–187`, in the module's documented order. Re-read the 1.1.1 header for the ordering before wiring.

- **P154 — the 16 presets (D42).**
  - Names are `<Category> · <Name>`, ≤ 24 characters in total. Categories:
    - `Ambient ·` ×4
    - `Harmoniser ·` ×4: scale_root + pitch_follow active, at least one with a custom scale / masks
    - `Rhythmic ·` ×4: tempo sync, LFO Tempo-mode routes
    - `Cascade ·` ×4: fb ≥ 70 %, at least two at sat > 0 so D43 is audible
  - Voicing by Claude at execute, informed by the DAW-2.x / 3.3 hands-on notes. **Balanced by voicing, never by `output_gain`** (feedback_presets_never_set_output_gain).
  - Taylor auditions at verify (DAW-4). Re-voicing after that is a verify finding, not a plan change.

- **P155 — preset gates in `engine_gates_4.h`** (section PR; R143/R144). Every row builds a fresh processor and applies a def via `presetManager.applyPresetData (json)`, serialised exactly as `initializeFactoryPresets` does (no disk):
  - **PR0:** table validity. Exactly 16, 4 per prefix, unique names, ≤ 24 characters, no `/\:`, no `output_gain` key, every value in range.
  - **PR1:** after apply, each parameter's engineering read-back equals the table within the interval/snap tolerance, and absent params == default.
    - Set `output_gain` to −7 dB before the load; it still reads −7 dB after.
    - **PR1n:** a deliberately linear-fraction conversion of one skewed param (test-only) fails the read-back.
  - **PR2:** a 20 s render per preset on the standard stimulus is finite, has peak ≤ A (the carried output-law bound), and is non-silent (RMS > −60 dBFS).
    - Loudness: RMS over 2–20 s, per preset. **The spread across the 16 is ≤ 9 dB** (window signed at plan; it brackets ambient-vs-cascade intent). Record the values.
  - **PR3:** use-case liveness per category.
    - Harmoniser: the pitch route is active (Pitch-source telemetry ≠ idle) and output partials are quantised to the scale (the existing ScaleQuantiser probe).
    - Rhythmic: TempoTracker is locked at 120 and 93 bpm, and onsets are aligned to the grid within the carried TT tolerance.
    - Cascade: min g_agc < 1, **or** LG readout ≥ −3 dB during the render (this exercises D43).
    - Ambient: ≥ 2 s of tail > −60 dBFS after the input stops.
  - **PR4:** load A → load B is bitwise equal to a fresh load of B (10 s, same seed), over all 16×15 ordered pairs. If the runtime is excessive, use a seeded 32-pair subset plus all same-category pairs, and record which.
  - **PR5:** the real path.
    - Point the preset root at a temp dir (the module's directory override if one exists; otherwise HOME redirection for the harness process only).
    - Delete `.factory-version` + `Factory/`, construct, and expect 16 files with the expected names.
    - `loadPreset (name)` read-back == PR1.
    - Save as user → load → bitwise == factory apply (10 s render).
  - **The harness never touches `~/Library/O-Granulator/Presets`.**

- **P156 — state round-trip in `engine_gates_4.h`** (section ST; R145–R147), using the existing `make` / `stateOf` / `load`:
  - **ST1:** 3 seeded random full-parameter sets:
    - engineering units, snapped to the interval
    - choices at the last index, bools inverted
    - all 16 mod slots routed with non-zero depth
    - seeds 2^53 + 1 and 0xF00DFACECAFEBEEF
    - custom `edo 19`, 2 masks, uiLanguage zh-Hans
    - Save, load into a **fresh** instance, and every `getValue()` is equal, along with the non-param state (`sameNonParamState`).
    - A 10 s render of both instances is bitwise equal.
  - **ST2:** save → B → change the scale + seed → save → C, and C holds the changed values.
    - **ST2n:** with the `removeCustomChildren` strip disabled (a hook), C shows the stale value. Must FAIL ST2's predicate.
  - **ST3:** the ST1 sweep through `capturePresetData` → `applyPresetData` on a fresh instance (`output_gain` excluded by design and asserted unchanged).
  - **ST4** (host path, T5) is in P159.

- **P157 — layouts (section LY; R148).**
  - **LY1:** for mono→mono, mono→stereo and stereo→stereo, run `setBusesLayout` → `prepareToPlay` → 10 s at 44.1 / 48 / 96 k.
    - All channels finite.
    - Mono→stereo: both channels non-silent and L ≠ R at spread > 0.
    - Mono→mono non-silent.
    - 0 scrubs.
  - **LY1n:** `isBusesLayoutSupported` is false for stereo→mono, 5.1→5.1 and a disabled input.

- **P158 — version + CHANGELOG (R156).**
  - Change `CMakeLists.txt:34` to `VERSION 1.0.0`.
  - New `CHANGELOG.md`:
    - `## [1.0.0] - 2026-11-xx` in bracket form, with Added (the feature set + 16 presets with descriptions) and Changed (AGC restage).
    - No other `##` headings that could be mistaken for a version.
  - **Watch:** if the state blob or the HA / codec bytes embed `JucePlugin_VersionString`, the bump moves codec bytes. Re-run the codec bytes row right after the bump. A diff confined to the version string is attributed in SUMMARY (B → B′). Any other byte diff is a stop.

- **P159 — install + host validation (R156, ST4).**
  - Run `rm -rf ~/Library/O-Granulator/Presets/Factory ~/Library/O-Granulator/Presets/.factory-version`. This is Taylor's machine and removes the generated factory bank only, never `User/`. List the directory before deleting.
  - Run `./scripts/build-and-install.sh O-Granulator` (dual-variant sweep). Expect VST3 + AU + Standalone with 0 warnings. Rebuild the Standalone after the install.
  - Validate the install:
    - `auval -v aufx …` PASS
    - pluginval strictness 10 VST3 + AU PASS
    - after first instantiation, 16 files present in the installed `Factory/`
  - **ST4:** adapt `../O-Lyrica/tests/state-roundtrip/two_reopen.py` → `tests/state-roundtrip/two_reopen.py` (target path + O-Granulator param subset incl. skewed and choice params). Run it via `uv` on the **installed** VST3 and compare decoded values (pattern_vst3_raw_value…), with the JUCE-base64 framing per pattern_pedalboard_vst3_raw_state_framing.
  - Take one Standalone `screencapture` with the preset menu open for SUMMARY.

- **P160 — UI gates (UI-04, R150).** The page is not edited.
  - Order:
    1. overrides ×2 byte-identical
    2. `ui_frontend_check.js` (+ NCs)
    3. probe 220 × 2 (+ NC)
    4. check-i18n
    5. fr-lint
    6. zh-lint
    7. check-ui-labels (`default + 10`)
    8. boot-all-uis `--strict-tips`
  - Exit 77 = not run = not a pass.
  - **New preset-name fit check (PF1)**, added to `ui_frontend_check.js`:
    - Load the 16 names into `.pname b` with `white-space: nowrap`.
    - `scrollWidth ≤ clientWidth` for each name, in EN / FR / zh-Hans (the names are unlocalised, but the box font may differ per language).
    - **PF1n:** a 40-character name fails.
  - If a name fails, shorten the name (P154). Never widen the box.

- **P161 — HANDS-ON-4.md + DAW-4.md (D46, QUAL-04).** Taylor runs both at verify.
  - **DAW-4, Logic AU:**
    - All 16 presets auditioned, judged by category intent.
    - A Cascade preset at default sat: the loop-gain readout now shows AGC action.
    - fb 50 % on a drum loop: no audible pumping (AG4 by ear).
    - Freeze re-listen as a confirmation only. Head-stop closed at 2.4.
    - Preset A → B switching while playing.
    - Session save → quit → reopen ×2 restores everything (COMPAT-02 by host).
    - Mono and stereo insert.
  - **DAW-4, Reaper VST3** (D46): load, the 4 categories, automation of 3 params, save/reopen, and mono / stereo track.
  - Each item carries its FAIL consequence (which requirement reopens).
  - **HANDS-ON-4 (Standalone):** preset menu grouping by prefix, name ellipsis at the widest name, and language switch with the preset menu open.
  - Pre-step in both docs: confirm the installed `Factory/` dir has the 16 current files (R143).

- **P162 — REQUIREMENTS + STATUS.**
  - At execute: FUNC-09, COMPAT-02 and UI-04 → `complete`. COMPAT-03 → `complete-pending-CI` (Windows at `/publish`, sign-off 4). QUAL-04 → `partial` (hands-on pending).
  - At verify: QUAL-04 → `complete` iff DAW-4 + HANDS-ON-4 PASS.
  - STATUS: `stage_4_execute_complete`, `next_action: stage_4_verify`.

---

## Source layout (deltas)

| File | Change |
|---|---|
| `Source/dsp/FeedbackChain.h` | `stepAgc (xl, xr, d)` with the normalised peak, plus the `testAgcLegacy` hook (P144). **The only DSP change.** |
| `Source/dsp/ModSources.h` | Env input predicate (P147). Bitwise-neutral ≤ +80 dBFS. |
| `Source/dsp/Engine.cpp` | `uiSnap` clear at the end of `prepare` (P148). |
| `Source/FactoryPresets.h` (new) | spec table + `makeFactoryPresets` (P153, P154) |
| `Source/PluginProcessor.cpp` | `initializeFactoryPresets` wiring (P153), plus the ST2n hook if needed (test hooks only) |
| `CMakeLists.txt` | `VERSION 1.0.0` (P158); add `FactoryPresets.h` to the sources if headers are listed |
| `CHANGELOG.md` (new) | `## [1.0.0]` (P158) |
| `tests/render-harness/engine_gates_4.h` (new), `main.cpp` | sections AG, PR, ST, LY, US; `--only4` filter |
| `tests/render-harness/engine_gates_22b.h` | event-aware `renderPool` dispatch + W2o note (P149, P151) |
| `tests/render-harness/engine_gates_25bb.h` | Z6a liveness (P149) |
| `tests/render-harness/engine_gates_26.h` | Z7h3 INFO → row + Z7h3n (P147) |
| `tests/render-harness/engine_gates_26b.h` | C7 calibration (P150) |
| `tests/golden/*` | re-baselined digests, attributed (P146) |
| `tests/state-roundtrip/two_reopen.py` (new) | ST4 (P159) |
| `tests/ui_frontend_check.js` | PF1 / PF1n (P160) |
| `.planning/stages/4-polish/HANDS-ON-4.md`, `DAW-4.md`, `SUMMARY-4.md` (new) | P161, P162 |

Unchanged:
- `modules/**`, `scripts/**`, the page (`index.html`, `i18n.js`) and every contract file
- `Analysis.h`, `CaptureWriter.h`, `ModMatrix.h`, `OutputLaw.h`
- the `loopAgcTel` formula

---

## Tasks

### Wave 0 — precondition

1. **T0 — Entry Check + baseline C** (P143). Log under `${TMPDIR:-/tmp}/ogran-4/`. Run the harness in the background. No commit.
   - Depends on: none

### Wave 1 — the one DSP change

2. **T1 — D43 + AG gates + re-baseline → D** (P144, P145, P146).
   1. Code.
   2. `engine_gates_4.h` section AG + `--only4`.
   3. Run AG1–AG6 / AG1n until green.
   4. AG4 FAIL → **stop and ask**. A sat-0 mover → **stop**.
   5. Full default set → moved-row ledger → N-suites / R12 / HA / codec.
   6. Freeze D. **Commit.**
   - Files: `FeedbackChain.h`, `engine_gates_4.h`, `main.cpp`, `tests/golden/*`
   - Depends on: T0

### Wave 2 — carried housekeeping (audio-neutral vs D)

3. **T2 — D47 (a)–(e)** (P147–P151).
   - Order: (b)'s call-site audit first, then (a), (e), (b), (c), (d).
   - Neutrality proof per P152. **Commit.**
   - Files: `ModSources.h`, `Engine.cpp`, `engine_gates_22b/25bb/26/26b.h`, `engine_gates_4.h` (US1)
   - Depends on: T1

### Wave 3 — presets, state, layouts (no DSP)

4. **T3 — factory presets** (P153, P154, P155).
   - Table + wiring, then voicing.
   - PR0–PR5 + PR1n green.
   - Carried extract == D + new rows. **Commit.**
   - Files: `FactoryPresets.h`, `PluginProcessor.cpp`, `CMakeLists.txt` (sources), `engine_gates_4.h`
   - Depends on: T2
5. **T4 — state + layouts** (P156, P157). ST1–ST3 + ST2n, LY1 / LY1n green. **Commit.**
   - Files: `engine_gates_4.h` (+ the ST2n hook in `PluginProcessor.cpp` under `OGRAN_TEST_HOOKS` only)
   - Depends on: T3 (ST3 uses the preset path)

### Wave 4 — release candidate

6. **T5 — version + CHANGELOG + install + host validation** (P158, P159).
   - Re-run the codec-bytes row after the bump.
   - Clear the installed Factory bank (list first).
   - Install, then auval, pluginval 10 ×2, then ST4.
   - **Commit.**
   - Files: `CMakeLists.txt`, `CHANGELOG.md`, `tests/state-roundtrip/two_reopen.py`
   - Depends on: T4
7. **T6 — UI gate run + PF1** (P160). Everything green with exit codes recorded. **Commit.**
   - Files: `tests/ui_frontend_check.js` (+ P154 name changes if PF1 fails → re-run T3's PR0 and T5's install)
   - Depends on: T5

### Wave 5 — docs + handoff

8. **T7 — docs** (P161, P162).
   - `HANDS-ON-4.md`, `DAW-4.md`.
   - `SUMMARY-4.md`:
     - D43 ledger (old → new per moved row) and the AG4 measured depth
     - the `renderPool` audit table
     - PR2 loudness table + the 16 descriptions
     - C7 ns/op
     - codec B → B′ attribution if any
     - NC results
     - deviations
   - REQUIREMENTS + STATUS.
   - **Commit** path-scoped.
   - Depends on: T6

**Every commit:** re-check `git branch --show-current` and `git status --short` immediately before, then `git commit -- plugins/O-Granulator` (+ `PLUGINS.md` only if touched). Never `-a` or `-A`. **No tag** (`/publish` only).

---

## Dependency graph

```
T0 ── T1 ──┬─(AG green, no sat-0 mover)── D frozen ── T2 ── T3 ── T4 ── T5 ── T6 ── T7
           ├─(AG4 > 1 dB p-p)──────────── STOP → sign-off
           └─(sat-0 row moved)─────────── STOP → defect
```

D43 lands first so that D is the only audio baseline the release ships against (R139). Every later task is gated bitwise vs D.

---

## Gate criteria (Stage 4)

| Gate | Pass |
|---|---|
| Harness default set | All PASS. The carried extract vs D = the attributed T2 movers + new rows only. AG1n / Z7h3n / US1n / PR1n / ST2n / LY1n-style NCs each FAIL their predicate. |
| D43 | AG1–AG6 PASS. AG3 bitwise at sat 0. AG4 ≤ 1.0 dB p-p. AG5 freeze rows bitwise vs C. Every C → D mover in the ledger, none at sat 0, no threshold loosened. |
| Presets | PR0–PR5 PASS. Loudness spread ≤ 9 dB. `output_gain` untouched. Build-tree bank only in the harness. |
| State | ST1–ST3 PASS on fresh instances, bitwise renders. ST4 (installed VST3, two reopens) PASS. |
| Layouts | LY1 at 3 rates × 3 layouts. LY1n false for all rejected layouts. |
| N3 / N2 / N1 / N0 `--strict`, R12, HA, block-size null | rc 0 vs D. N9 / N10 null 0. |
| codec bytes | 9/9 == B (or B′ with a version-string-only diff, attributed) |
| Build | VST3 + AU + Standalone, 0 warnings. Version 1.0.0. CHANGELOG `## [1.0.0]`. |
| auval · pluginval 10 VST3 + AU | PASS · PASS |
| UI gates | overrides ×2 identical. Frontend 0 failures (+ PF1, PF1n FAILS). Probe 220/220 ×2. i18n · fr · zh · labels · boot-all-uis clean. Page unchanged. |
| Tag | **none** |

---

## Risks / watch items

- **D43 pumping (R136).** AG4 is measured. A FAIL is a stop, not a tune. The `kAgcCeil` fallback loses AG3 and needs a sign-off.
- **Per-sample `d`.** `satD` ramps, so the normalisation uses the same `d` that the saturator used on that sample. This is exact by construction; check it in code review.
- **Re-baseline creep.** Only rows with sat > 0 and fb > 0 may move at T1. Any other mover is a stop. The W2o rule applies.
- **`renderPool` dispatch (R152).** Audit before changing. A row without events that moves means the dispatch is wrong.
- **Preset authoring traps.** Skew and choice k/N (PR1 + PR1n). A missing seed makes a preset differ per instance (R142; PR4 catches it). A stale installed bank (P159 pre-clear + version bump).
- **Version bump vs codec bytes.** Check immediately (P158).
- **Harness disk hygiene.** PR5 must never write to the real `~/Library` preset root.
- **Deleting the installed Factory dir** is a destructive local step on Taylor's machine. List it first, and touch only `Factory/` + `.factory-version`, never `User/`.
- **Windows.** Not exercised in Stage 4 (sign-off 4). `FactoryPresets.h` uses no platform APIs, and preset file names contain `·` (U+00B7). **Watch at `/publish`:** Windows file names with `·` are legal. Confirm that the module's sanitiser does not strip or mangle the character on any platform (check in PR5 on macOS and note it for the CI run).

## Deviations protocol

As in 3.x: record every departure in SUMMARY-4 (what, why, evidence, which gate). **Stop and ask** for:
- any audio-moving change other than D43
- AG4 FAIL, or a sat-0 mover
- a contract edit or a module change
- any push or tag

## Success criteria (gate for verify)

- [ ] AGC engages at default saturation, sat-0 bitwise, no pumping ≤ 1 dB p-p (AG1–AG6 + AG1n); baseline D frozen with an attributed ledger
- [ ] Carried items closed: Env pin row, `uiSnap` clear, event-aware `renderPool` + Z6a liveness, C7 calibration, W2o written down; audio bitwise vs D
- [ ] 16 factory presets, 4 per category, engineering-unit authored, fixed seeds, never set `output_gain`, A→B == fresh B (PR0–PR5) — FUNC-09
- [ ] Full state round-trip on fresh instances via codec, preset files and the installed VST3 (ST1–ST4) — COMPAT-02
- [ ] Mono→mono / mono→stereo / stereo→stereo gated, rejected layouts refused (LY1 / LY1n); Windows at `/publish` — COMPAT-03
- [ ] i18n gates green, preset names fit (PF1) — UI-04
- [ ] Version 1.0.0, CHANGELOG `## [1.0.0]`, auval + pluginval 10 VST3/AU PASS, installed, no tag
- [ ] HANDS-ON-4.md + DAW-4.md (Logic AU + Reaper VST3) ready; QUAL-04 completes at verify
