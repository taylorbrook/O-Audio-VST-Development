# Stage 4: Polish / Validation — Research

**Date:** 2026-11-07
**Inputs:** CONTEXT-4.md (D42–D47 and 4 open questions), `Source/dsp/FeedbackChain.h`, `EngineConstants.h`, `Engine.cpp:545-563`, `ModSources.h:229-270`, `UiTelemetry.h`, `PluginProcessor.cpp:179-259, 440-648`, preset-manager 1.1.1 (`modules/persistence/preset-manager/cpp/OuariconPresetManager.h`), harness `engine_gates_22b.h / 24.h / 25bb.h / 26.h / 26b.h / 33.h`, `.github/workflows/{ci-tests,build-and-release}.yml`, memory (factory-preset skew, bank sentinel, stale CustomState, ValueTree type loss, output-gain rule).
**Numbering:** findings continue from 3.3 (last R133): **R134–R156**.

---

## 1. D43 — AGC / saturator staging (the one DSP change)

### R134 — why the AGC is dead today
Chain order (`FeedbackChain.h:213-220`): knob → HP → LP → diffuser → `tanh(d·x)/d` → AGC → scrub. The saturator output is bounded by `1/d` (`d = 1 + 7·sat`). The AGC gain `min(1, 0.7/env)` falls below 1 only when `env > 0.7`, so it is reachable only while `1/d > 0.7`, i.e. `sat < 6.12 %`. At the default 20 %, `1/d = 0.417`, so g_agc ≡ 1. This confirms the 3.3 finding.

### R135 — static model of the three options
Scratch model (session scratchpad `agc_static.py`). It is a peak-amplitude static model of a self-sustaining loop with excess gain G before the chain:

| sat | d | old (today) | (1) kAgcCeil 0.35 | (2) AGC before sat | (3) ceiling tracks sat |
|---|---|---|---|---|---|
| 0 % | 1.00 | G > +1.86 dB | +0.38 dB | +1.28 dB | **+1.86 dB (≡ old)** |
| 6.1 % | 1.43 | +13.7 dB | +0.82 dB | +2.37 dB | +1.86 dB |
| **20 %** | 2.40 | **never** | +3.25 dB | +5.11 dB | **+1.86 dB** |
| 40 % | 3.80 | never | never | +8.58 dB | +1.86 dB |
| 100 % | 8.00 | never | never | +14.96 dB | +1.86 dB |

Saturator compression when a runaway loop is held: (2) −1.3 / −5.1 / −10 / −15 dB at sat 0 / 20 / 50 / 100 %. (3) −1.86 dB at every setting.

- **(1) Lower the ceiling:** this only moves the dead zone, which still opens above sat 26.5 %. It also compresses harder at sat 0. **Rejected.**
- **(2) AGC before the saturator:** the AGC engages at defaults only for loop excess above +5.1 dB, and the threshold climbs with sat, so below that the saturator stops growth on its own. It also changes sat-0 behaviour, because the pre-sat ceiling of 0.7 → tanh gives 0.604 instead of 0.7. It meets "reachable at defaults" only for hot loops. **Not recommended.**
- **(3) Ceiling tracks saturation — RECOMMENDED.** Run the follower on the *normalised* saturator output `d·|y| = |tanh(d·x)|` and keep `g = min(1, kAgcCeil/env)`:
  ```cpp
  inline void stepAgc (float& xl, float& xr, float d) noexcept
  {
      const float peak = d * std::max (std::abs (xl), std::abs (xr));   // = |tanh(d·x)| ≤ 1
      env = peak > env ? agc.att * env + (1.0f - agc.att) * peak
                       : agc.rel * env + (1.0f - agc.rel) * peak;
      const float g = std::min (1.0f, kAgcCeil / env);
      ...
  ```
  Properties:
  - The engagement threshold is invariant to the saturation knob: a +1.86 dB loop excess at every sat, with the hold at a fixed tanh depth of 0.7.
  - **It is bitwise identical to today at sat = 0**, because `d == 1.0f` makes `d·peak` exact. Every sat-0 carried row is therefore a free negative control for "nothing else moved".
  - `env ≤ 1` becomes a *structural* bound, which tightens FB4 (R138).
  - The `loopAgc()` telemetry formula (`Engine.cpp:563`) and the LG readout (`engine_gates_33.h:793`) do not change, because env is already normalised.
  - Saturation colour stays intact. The first pass through tanh runs at full drive, and the AGC only scales down what recirculates. A held runaway return sits at `0.7/d`, about −3.1 dB below the old `≈1/d` pin.
  - The call site in `process()` already has `d` in scope (`FeedbackChain.h:217`).

### R136 — pumping risk under (3)
At defaults the AGC engages when the post-knob, pre-sat peak exceeds 0.361 (−8.9 dBFS). At feedback 50 %, where `g_knob = 0.475`, that means a tap peak above about 0.76 (−2.4 dBFS) once grain overlap is counted. It can therefore fire on loud program material, not only on runaway, so it needs a measured gate rather than an assumption. The 1 ms attack and 400 ms release are unchanged. A peak follower with a 400 ms release on a transient-rich loop is the classic way pumping happens.

### R137 — D43 gate set (proposed rows `AG*`, new header `engine_gates_4.h`)
| Row | Config | Verdict |
|---|---|---|
| AG1 | FB4 drive-100 corner (G > 1), sat 20 %, fb 100 %, 20 s | min g_agc over 10–20 s < 0.9 (engaged). Loop return peak ≤ `kAgcCeil/d` · (1 + attack-overshoot margin, measured). Finite |
| AG1n | Same render, **legacy staging** via a test hook (`OGRAN_TEST_HOOKS`: `bool testAgcLegacy` makes peak un-normalised) | g_agc ≡ 1 for the whole render. This proves AG1 sees the change |
| AG2 | FB3n ARCH-law phase-locked growth config at sat 0 / 20 / 50 / 100 % | The AGC engages at every sat (min g_agc < 1). The return holds at `0.7/d ± tol`. No dB/gen growth after the hold (fit per-generation energy, not mean frame dB; memory pattern_feedback_loop_phase_lock) |
| AG3 | Every carried FB / Z7d / TA1 row at **sat 0** | Bitwise == baseline C, the 3.3 build |
| AG4 | Pumping: defaults + fb 50 %, (a) −6 dBFS-peak noise bursts, (b) a 0 dBFS-peak transient loop, 30 s | (a) g_agc ≡ 1. (b) Record min g_agc and the g_agc modulation depth (dB p-p over 400 ms windows). **The threshold is signed at plan** from the measured value. Proposed: ≤ 1 dB p-p |
| AG5 | Z7d (fb 100 %, freeze off, exact 0 by ≈ 296 s) and FZ / TA1 freeze rows at default sat | Exact zero is still reached, no later than before (the AGC only lowers gain). Freeze rows: the chain runs while frozen but the ring write is stopped (`Engine.cpp:447, 526`), so frozen content should be unchanged → bitwise vs C expected. If a freeze row moves, stop and investigate |

### R138 — carried rows that move under D43, to re-baseline deliberately (W2o rule: widen seeds, never thresholds)
These rows are expected to move only where sat > 0 and the loop is hot. Candidates: FB2a, FB2b and FB2c (fb 80–100 %, sat at default), FB3n (growth slope; hold now engages earlier), FB4 (env values), LG (g_agc now < 1 in hot configs), Z7d (decay digest), and any golden digest from an fb > 0 render. **FB4's bound changes:** `env ≤ 0.7 + 1/d` becomes `env ≤ 1.0` (structural). That is a tightening and needs a note, not a loosening. Procedure: run diff-vs-C with D43 alone. Every moved row gets an old → new line with a one-line cause. Any row that moves at sat 0 is a defect (AG3). Then **freeze baseline D**.

### R139 — sequencing
D43 → baseline D → everything else gated bitwise vs D. D47(a) and (e) are audio-neutral, and D47(b), (c) and (d) touch only the harness. Presets and state tests touch no DSP. Landing D43 first makes D the only audio baseline the release ships against.

---

## 2. D42 / FUNC-09 — factory presets

### R140 — no category support in preset-manager 1.1.1 → name prefix
`FactoryPresetDef` = `{ name, parameters (normalised), customState }`. There are no categories, folders or description field. `getPresetList()` merges the Factory and User sets and sorts them case-insensitively (`:606`). The preset menu (`index.html:3351`) is plugin-local and flat (`.pm-item`, `.pm-sep`).
**Recommend a name prefix** with a `·` separator (not in the sanitiser's `/\:` set): `Ambient · …`, `Harmoniser · …`, `Rhythmic · …`, `Cascade · …`. Sorting then groups the presets. There is no UI change and no new i18n surface, because preset names are not localised. Category sub-headers in the menu are possible later, but they would add 4 strings × 2 languages plus a fit gate, so leave them out of 1.0.0.
**Name budget:** `.pname b` is ≈ 176 px at 14 px EB Garamond (300 − 2×26 − 52 − 20 padding) with an ellipsis. Gate it with the nowrap scrollWidth fit (memory pattern_scrollwidth_fit_gate_misses_wrapped_caption). Keep names to about 24 characters including the prefix.

### R141 — authoring convention (skew + choices)
220 parameters, 25 of them skewed (size, density, filter_cutoff, ring_freq ×4; buffer_length, fb_low_cut, fb_high_cut, lfo1–4_rate, env_attack and env_release, several via `setSkewForCentre`). **Author the table in engineering units** (Hz, ms, %, choice *index*, bool 0/1) and convert once through each parameter's own `convertTo0to1` before `initializeFactoryPresets`. Never write linear fractions, and never write k/N for choices (memory pattern_factory_preset_normalized_ignores_skew, all three variants). Omit `output_gain`. `setExcludedParameters({"output_gain"})` already guards apply, so add a gate row as well.

### R142 — customState per preset: give each one a fixed seed
`applyNonParameterState` resets custom scale and masks to defaults first (`PluginProcessor.cpp:571`), so an absent field reads as default. **The seed is the exception:** an absent seed keeps the live instance seed (`:573`), so a factory preset would sound different in each instance. Give every factory preset `customState = { "seed": "<int64 decimal string>" }`, plus `custom` and `masks` only where a harmoniser preset needs them. **Descriptions (D42):** the module has no field for them. Put the text in a plugin-side table next to the defs, for the CHANGELOG and README. If the text also goes in `customState.description`, it is inert because apply ignores unknown keys. Do not render descriptions in the UI for 1.0.0, which keeps the i18n surface at zero. *Plan should confirm this with Taylor if D42 meant on-screen descriptions.*

### R143 — gating the build-tree bank, not the installed one
Factory JSON is written to `~/Library/O-Granulator/Presets/Factory`, and only when `.factory-version` ≠ `JucePlugin_VersionString`, which is **0.1.0** today (CMakeLists:34). Iterating the voicing under one version leaves a stale bank, and renaming a preset orphans its old JSON (memory pattern_factory_bank_sentinel).
- **Primary gate path, with no disk:** factor the table into a pure `makeFactoryPresets (apvts)` (for example `Source/FactoryPresets.h`). The harness builds a fresh processor and serialises each def into preset JSON exactly as `initializeFactoryPresets` does, then calls the public `presetManager.applyPresetData(json)` (`:181`). It renders and asserts. This is the build-tree bank by construction.
- **Real-path row:** one row deletes `.factory-version` and the Factory dir first, constructs the processor, and checks that the 16 files exist with the expected names, and that `loadPreset(name)` → param read-back equals the table.
- **Version bump** to 1.0.0 at the end of Stage 4 (CHANGELOG `## [1.0.0]`), so a user's bank regenerates. Before the DAW pass, run `rm -rf ~/Library/O-Granulator/Presets/Factory` so the hands-on build reads the current bank.

### R144 — preset gate rows (proposed `PR*`)
- **PR1:** each of the 16 presets applies, and every parameter's engineering-unit read-back matches the table within the parameter's interval and float-snap tolerance (catches skew and choice errors). Every parameter absent from the table equals its default. `output_gain` is never written or reset (set it to −7 dB first, load, and it still reads −7 dB).
- **PR2:** each preset renders 20 s on the standard stimulus. Output is finite, peak ≤ A, and audibly non-silent. Bank loudness spread is recorded and must fall within a window signed at plan (memory: balance by voicing, never by gain).
- **PR3:** category use-case liveness. Harmoniser presets show the pitch-follow / scale route active (the harness's Pitch source telemetry). Rhythmic presets lock to the host tempo (TempoTracker locked at 120 and 93 bpm). Cascade presets reach g_agc < 1 or a loop gain readout ≥ −3 dB (this exercises D43). Ambient presets have ≥ 2 s tail after the input stops.
- **PR4:** load preset A → load preset B. The result equals a fresh load of B, bitwise over a 10 s render with the same seed. This covers the reset-to-defaults and non-parameter reset.
- **PR5:** two preset files in the real path round-trip (save user → load) == the factory apply.

---

## 3. COMPAT-02 — state round-trip on a fresh instance

### R145 — the stale `<CustomState>` trap does not apply, but keep the two-reopen row
The session codec does **not** use `OuariconPresetManager::setStateFromXml` / `getStateAsXml`. It uses `copyState` / `replaceState` with its own `Engine` / `CustomScale` / `DegreeMasks` children, and strips them on both legs (`removeCustomChildren`, `PluginProcessor.cpp:460-470, 475, 520`). That is the O-Octagon pattern (memory critical_preset_manager_stale_customstate_child: grep for `setStateFromXml` before counting a consumer). The two-reopen row still goes in, because the trap is invisible to a single reopen.

### R146 — the type-loss trap is already handled
`uiLanguage` is parsed from a string (`:523-528`), and the seed is an int64 decimal string parsed with `getLargeIntValue` after a `containsOnly` guard. Mod routings are APVTS parameters (16 slots × source / dest / depth…), so they round-trip as floats. The gate must still assert them: `isVoid` guards plus the value equality after XML.

### R147 — round-trip gate rows (proposed `ST*`)
- **ST1 (all-parameters sweep):** set every parameter to a non-default value. Use 3 seeded random sets in engineering units, snapped to the interval, with choices at their last index and bools inverted. Add all 16 mod slots routed with non-zero depth, an int64 seed above 2^53 and above INT64_MAX as uint64, a custom scale (`edo 19`), 2 non-default masks, and uiLanguage `zh-Hans`. Then `getStateInformation` → a **fresh** instance → `setStateInformation` → every parameter `getValue()` equal, plus the seed, scale, masks and language. Then render 10 s on both instances, which must be bitwise equal.
- **ST2 (two reopen):** save → reopen (instance B) → change the custom scale and seed → save → reopen (instance C). C holds the *changed* values. Negative control: the same harness against a codec with the strip removed (hook) shows the stale value. Without it the row could pass vacuously.
- **ST3 (preset files):** the same sweep through `capturePresetData` → `applyPresetData` on a fresh instance (`output_gain` excluded by design).
- **ST4 (host path):** use the reusable pedalboard gate `plugins/O-Lyrica/tests/state-roundtrip/two_reopen.py` on the **installed** VST3. It decodes the JUCE-base64 IComponent framing (memory pattern_pedalboard_vst3_raw_state_framing). Run it via `uv` and compare decoded values, not raw_value (memory pattern_vst3_raw_value…).

---

## 4. COMPAT-03 — I/O layouts and Windows

### R148 — layouts
`isBusesLayoutSupported` (`:248-259`) accepts mono→mono, mono→stereo and stereo→stereo, and rejects disabled buses, stereo→mono and anything wider. `processBlock` loops are bounded by the buffer's own channel count (`:265-268`). Memory traps: the Standalone 3–7 ch canonical-set OOB is already bounded, and a mono-out bus gets 2 ch (rejected here anyway).
**Gate LY1:** for each accepted layout, `setBusesLayout` → prepare → render 10 s at 48 / 44.1 / 96 k. Output is finite on every channel. For mono→stereo, both outputs are non-silent and L ≠ R at spread > 0. Mono→mono is non-silent. **LY1n:** the rejected layouts (stereo→mono, 5.1, disabled) return false. auval exercises AU channel configs, and pluginval 10 exercises bus layouts on VST3.

### R149 — Windows without tagging
`.github/workflows/ci-tests.yml` is `workflow_dispatch` with a `plugin` input, and its `windows-vst3` job (`:323`) builds the VST3 and runs pluginval on Windows. `build-and-release.yml` also has a dispatch with `validate_only: true` (no release), but it carries the signing-secret surface, so **prefer `ci-tests.yml`.**
**Caveat:** a dispatch builds the *remote* ref. `main` is 103 commits ahead of origin, so a push is a prerequisite. The push and the dispatch are outward-facing, so **the plan makes this a Taylor-confirmed step.** CONTEXT-4 marks it optional, and the public `/publish` CI proves Windows anyway.

---

## 5. UI-04 — i18n (gates only)

### R150 — the surface is unchanged if R140 and R142 hold
Prefix-named presets add no strings, and descriptions are not rendered, so lint, fit and labels stay as they were at 3.3. Re-run them once after the presets land. The menu items show preset names, which are not localised, so the `#preset-menu` width is language-independent. The `Factory` / `User` kind chip is already localised.

---

## 6. D47 — carried housekeeping

### R151 — (a) Env 1e30 pin
`EnvFollower::process` (`ModSources.h:262-263`) uses `x = |s·g|`. Add the R96 predicate used by `CaptureWriter.h:105` and `Analysis.h:800`:
`if (! (x <= kAnalysisTapMax)) x = 0.0f;`
This is bitwise-neutral for |s·g| ≤ 1e4 (+80 dBFS post env-gain). Turn the Z7h3 INFO into a row: Env cells after a 1e30 injection == the zero-sample reference (no pin). For |s·g| ≤ 1e4 the change is exactly a no-op, so every carried row stays bitwise vs D.

### R152 — (b) event-aware `renderPool` (Z6a)
`eg22b::renderPool` → `eg22::runLoop` ignores `Job::events`. Its `jassert` compiles out in Release (`engine_gates_22b.h:293`), so Z6a's freeze toggles never fire. An event-aware pool already exists: `renderPoolEv` (`engine_gates_26.h:529`).
**Recommend:** dispatch inside `renderPool`. Jobs with events go through the eg25 setup/loop path, and jobs without events keep `runLoop`, so their bytes do not change. Then add a liveness clause to Z6a: `freezeEngages ≥ 3` (the same counter Z7a uses, `engine_gates_26.h:595`), so the row cannot go vacuous again.
**Pitfall:** there are 7 `renderPool` call sites. Audit each for non-empty `events` *before* the change. Any other row that passed events silently would start honouring them and move. Record each one as moved by design.

### R153 — (c) C7 machine-speed calibration
The duty witness can't see a uniformly slowed core (thermal throttling, or E-core scheduling on Apple silicon). Add a fixed-work calibration loop: about 50 ms of a dependent FMA chain with a result sink so it can't be optimised away. Time it before and after each of the 7 runs. Any run whose calibration ratio vs the session's first calibration exceeds 1.10 is counted as contended, and fewer than 5 valid runs gives `[NOVERDICT]` (the existing path, `engine_gates_26b.h:155`). Print the ns/op so cross-machine C7 numbers can be compared. Also pin the timing thread to `QOS_CLASS_USER_INTERACTIVE` (`pthread_set_qos_class_self_np`) to keep it on P-cores.

### R154 — (d) the W2o rule
Write it into the harness header notes (`engine_gates_22b.h:30`) and the Stage 4 PLAN: *when a seeded row moves, widen the seed set, never the threshold.*

### R155 — (e) reset `uiSnap` in prepare
`TripleBuffer` has no reset, and the reader may be live across `prepareToPlay`. Zeroing all three buffers would race the reader. **Recommend:** at the end of `Engine::prepare`, write `UiSnapshot{}` into `uiSnap.writeBuffer()` and `publish()`. The reader then picks up a cleared frame through the normal acquire path. This is audio-neutral: the DSP never reads `uiSnap` (`UiTelemetry.h` header). Gate it with the existing UI snapshot probe: after `prepare`, the first `readUiSnapshot()` has 0 grains and is not frozen.

---

## 7. Release / QUAL-04

### R156 — release checklist items
- pluginval strictness 10 on VST3 and AU, and `auval -v aufx <code> <mfr>`, on the installed binaries. Clear the AU cache first and run the dual-variant sweep (`build-and-install.sh` Phase 4).
- CHANGELOG `## [1.0.0]` in bracket form (memory: the partial-fix heading swallows the file), with CMake `VERSION 1.0.0` bumped at the same time. This also regenerates the factory bank.
- **No tag in Stage 4** (memory feedback_never_tag_unless_publish).
- Hands-on (D46): Logic AU plus Reaper VST3. Cover the 4 preset categories, the AGC readout at defaults on a Cascade preset (it should now move), the pumping listen at fb 50 % on a drum loop, and a freeze re-listen as confirmation only (head-stop already closed at 2.4). Clear the installed Factory dir before the session (R143).

---

## Answers to the CONTEXT-4 open questions

1. **AGC placement:** option (3). The follower runs on `d·|y|`, which makes the ceiling track saturation. It engages at every sat at a fixed +1.86 dB loop excess, is bitwise at sat 0, gives a structural `env ≤ 1`, and needs no telemetry change. AG1–AG5 gate it, with AG1n as the legacy-staging negative control.
2. **Preset gate path:** gate the build-tree table through `applyPresetData` with no disk. Add one real-path row with the sentinel and Factory dir deleted.
3. **Categories:** the module has none. Use a name prefix (`Ambient · …` and so on), with no UI or i18n change.
4. **Local Windows check:** `ci-tests.yml` `workflow_dispatch` → `windows-vst3` with no tag. It needs a push to origin, which Taylor confirms.

## Risks
| Risk | Mitigation |
|---|---|
| The AGC pumps on loud program material at fb 50 % (R136) | AG4 is measured, and its threshold is signed at plan. Fallback: raise `kAgcCeil` toward 0.8 (the threshold rises to a +2.75 dB loop excess). This also moves the sat-0 rows, so AG3 is lost and those rows need their own re-baseline |
| Event-aware `renderPool` moves other rows (R152) | Audit the 7 call sites first |
| A stale factory bank in the hands-on (R143) | Delete the Factory dir before the DAW pass, and bump the version at the end |
| The Windows dispatch needs a push | Taylor-confirmed step. It is optional, and `/publish` covers Windows anyway |
