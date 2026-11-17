---
title: "O-ParametricEQ Stage 4 (Polish) — Phase 4.1 Execution Plan (build: factory bank, host text, page fixes, i18n, G10 mode, Windows prep)"
created: 2026-11-16
juce_version: "8.0.15"
phase: "4.1"
summary: "Nine waves: (W0) pre-flight, 0.9.0 baselines (render 171 / 0 with --lid-dir, session SHAs, probe N1e); (W1) glossary: 18 EQ entries in both suite glossaries + the fr prose companion, suite-wide lint re-run reported; (W2) C++: FactoryPresets.h (13 defs in engineering units, convertTo0to1 once), initFactoryBank bank-hash port, ParamText.h host-text functions on every float, 0.10.0; (W3) page: deleteBand ends listen at once, .tip-point removed after the fade, fr / zh revision against the grown glossary (zh analyzer + Pre / Post); (W4) render harness rows FP1 / FP1o / FP2 / FP3 / FP4 / FP5 / FP5i / HT1 / HT1g / M1j and the --g10 gated mode (G10a – d, G10i, pass-through, net-cost linearity); (W5) PN1 name-fit port, Windows WebView2 static gate port, ci-tests.yml option line; (W6) build, battery, five UI gates, install, pluginval 10, auval; (W7) editor bench rows E-del-listen / E-listen33 / E-tipfade / E-tip-lang / E-tip-v13 / E-preset-listen + 14 mutants; (W8) summary, STATUS, CHANGELOG, one path-scoped commit. Audio path frozen: XR-lid SHA-equal to 0.5.0, N1e hash 6274ff9b5b0e488a. No push, no tag."
stages: [4]
agents: [dsp-agent, gui-agent, build-gate-runner]
---

# O-ParametricEQ — Stage 4 / Phase 4.1 Execution Plan (build)

## Goal

Everything Stage 4 ships is in source, gated, built and installed, so that 4.2 is validation only:

- **Factory bank (FUNC-07).** 13 task presets, written from engineering units, refreshed by a bank hash so a re-voice or a rename reaches `~/Library` at an unchanged version, with no orphan and no `User/` touch.
- **Host text.** Every float parameter reads in its unit in a host (`999.9998779` → `1.00 kHz`), and typed text parses back. No audio change.
- **Carried page fixes.** Delete ends listen at once; the point tip keeps its size through its fade. Each carried item from VERIFICATION-3.2 / 3.3 has a row that kills its regression.
- **Localisation (UI-05, build half).** The suite glossaries cover the EQ vocabulary; every fr and zh row is revised against them; all five UI gates read 0. French stays `reviewed: false` until Taylor's spot read in 4.2.
- **G10 (PERF-02, mechanism).** A gated `--g10` mode in the render harness. 4.1 runs it once; 4.2 runs it as the binding reading.
- **Windows prep (COMPAT-02).** `ci-tests.yml` can dispatch O-ParametricEQ, and a macOS static gate proves the WebView2 wiring first.

Version 0.10.0 built, installed, validated. Audio path unchanged.

**In scope:** `Source/FactoryPresets.h` (new), `Source/ParamText.h` (new), `Source/PluginProcessor.h/.cpp` (constructor: bank init; parameter creation: attributes; nothing in `processBlock`), `Source/ui/public/index.html`, `Source/ui/public/js/i18n.js`, `CMakeLists.txt` (version, new headers), `tests/render-harness/main.cpp` (rows + `--g10`), `tests/meter-ballistics/` (M1j, if it lands there), `tests/preset-name-fit/` (new), `tests/windows-webview2-gate.sh` (new), `CHANGELOG.md`, the editor bench under `stages/4-polish/execute-4.1/`, and outside the plugin: `.github/workflows/ci-tests.yml` (one option line), `scripts/i18n-fr-glossary.js`, `scripts/i18n-zh-glossary.js`, `.planning/quick/archive/2026-Q3/260826-ieq-multi-language-tooltips-across-all-vst-p/260826-ieq-FR-GLOSSARY.md`.

**Out of scope (4.2):**

| 4.2 | Why not now |
|---|---|
| Push `main` + Windows dispatch | C1: after 4.1 is committed, with Taylor's go-ahead at that moment. |
| Binding G10 reading, `--long`, full fresh battery | 4.2's job (fresh rebuild on the final tree). |
| Taylor sitting: P6 checklist, listening pass, French spot read | Needs this phase's bank and French installed. |
| Re-voiced presets, fr flip to `reviewed: true` | Outcome of the sitting. |
| Lint findings the new glossary raises in other plugins | Reported only (RESEARCH pitfalls). |
| SIMD, correction hold | Not needed (RESEARCH Q3: ≥ 3× headroom). |

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/4-polish/RESEARCH.md` | All of it. Q1 – Q6, pitfalls. |
| `stages/4-polish/CONTEXT.md` | P1 – P6, carry table, constraints. Binding. |
| `stages/4-polish/research-4/render-default-0.9.0.txt` | X-cpu readings; the two XR-lid FAILs are a missing `--lid-dir`. |
| `stages/3-gui/VERIFICATION-3.3.md`, `VERIFICATION-3.2.md` (Issues 3, 5), `VERIFICATION-3.1.md` (E16 / R2) | Carried items. |
| `stages/3-gui/verify-3.3/run-harness.sh` | The battery recipe (session dump → SHA list → designer → render with `--lid-dir` → probe N1e). |
| `stages/3-gui/execute-3.3/editor-bench/` | The bench (`src/Main.cpp` re-execs under `CFFIXED_USER_HOME`, `run-all.sh`, `analyze.py`, `verdict33.py`, `lockstate`). W7 copies and extends it. |
| `stages/3-gui/execute-3.3/mutants/run-mutants33.py` | Mutant runner shape. |
| `../O-GranulatorInstrument/Source/PluginProcessor.cpp:140-166` (`initFactoryBank`), `Source/FactoryPresets.h` (`makeFactoryPresets`, `bankHash`, `:2363-2375` the unit → normalised conversion) | P1 port source. |
| `../O-GranulatorInstrument/tests/preset_name_fit.js`, `tests/windows-webview2-gate.sh` | W5 port sources. |
| `modules/preset-manager/…/OuariconPresetManager.h:702` (`initializeFactoryPresets`), `applyPresetJson`, `getPresetList` | Module behaviour (unchanged at 1.1.1). |
| `Source/PluginProcessor.cpp:168` (float creation), `:289-290` (preset manager setup); `Source/PresetSaveGuard.h` | W2 edit points. |
| `Source/ui/public/index.html` `:667` (`LISTEN_DISABLE_SETTLE_MS`), `:996-1000` (enabled hook), `:1325` (`endListenOn`), `:1352` (`deleteBand`), `:1412` (strip `aria-pressed`), `:468` (`.tip-point`), `:2380-2412` (point tip, `clearPointTip`, `hideTooltip`) | W3 edit points. |
| `.github/workflows/ci-tests.yml:76-79` | W5 option line. |
| Memory: `pattern_factory_preset_normalized_ignores_skew`, `feedback_presets_never_set_output_gain`, `pattern_preset_apply_needs_reset_to_defaults`, `pattern_savepresettofile_has_no_factory_guard`, `pattern_preset_load_passes_through_defaults_page_hooks`, `pattern_juce_mac_home_ignores_HOME_use_cffixed_user_home`, `pattern_parallel_localizers_need_glossary_first`, `pattern_new_lint_zero_must_agree_with_independent_scan`, `pattern_appearance_none_select_exposes_caption_width`, `pattern_language_width_pins_content_sized_boxes_move`, `pattern_gate_stimulus_below_threshold_is_vacuous`, `pattern_realtime_paced_bench_core_idle_artefact`, `pattern_locked_screen_throttles_wkwebview_bench`, `pattern_executor_watchdog_stall_run_long_commands_in_background`, `critical_webview2_runtime_gotchas_windows`, `index_git_shared_checkout` | Traps this phase walks through. |

---

## Plan-phase decisions

**C1 (Taylor, 2026-11-16): push `main` at 4.2, as recommended.** 4.1 does not push. 4.2 asks for the go-ahead once more at the moment of the push (outward-facing, public repo, 295+ commits), then dispatches `ci-tests.yml -f plugin=O-ParametricEQ`.

**C2 (Taylor, 2026-11-16): keep "Mono Low End", as recommended.** 13 presets. FP5i prints its mono vs stereo broadband level; the 4.2 hands-on checklist gets a line for it.

**Amendment made in this plan phase:** ROADMAP risk row "196 WebView relays", retiring phase reworded from "WebView2 timing in Stage 4 Windows CI" to "load proven by pluginval 10 on windows-latest (4.2); open time and preset settle owed to the first Windows hands-on (no hardware)" (RESEARCH Q2). STATUS roadmap checksum refreshed.

The plan's calls:

- **P1 Factory bank (RESEARCH Q1).**
  - `Source/FactoryPresets.h`, namespace `opeq::factory`:
    - A def table in engineering units: per preset a name and a list of `{ paramId, value }` (Hz, dB, Q, choice index, bool). Only the bands a preset uses; everything else comes from the module's reset pass. Bands in ascending frequency from b01. `auto_gain` only where RESEARCH's table sets it.
    - `makeFactoryPresets (apvts)`: one `rp->convertTo0to1 (v)` per value through the live parameter (the O-GranulatorInstrument `:2363-2375` shape). A choice value is converted through the choice parameter's own range, never as `index / (n − 1)` by hand. A `jassert` that every id exists.
    - `bankHash (defs)`: the O-GranulatorInstrument function as-is (XXH64 over the canonical name-sorted, key-sorted serialisation, hex).
    - A static check (`static_assert` or a harness row; FP1o covers it) that no def names `output_gain`, `bypass` or `customState`.
  - The 13 presets are RESEARCH Q1's table verbatim (Default, Vocal Presence, Kick Tighten, De-Mud, Air Shelf, Rumble Cut, Telephone, Hum Notch 50 Hz, Hum Notch 60 Hz, Gentle Master Tilt, Mono Low End, Side Air, L/R Pair Match). "Default" has no values: it is the reset pass alone.
  - `PluginProcessor::initFactoryBank()`: port of O-GranulatorInstrument `:150-166` line for line (delete `Factory/*.json` directly in the dir + both sentinels on a mismatch or missing stamp; `initializeFactoryPresets`; stamp written **last**). Called at the end of the constructor, after `setValidatedApply (true)`. Never touches `User/`.
  - `preset-manager` stays 1.1.1. No `customSave` / `customLoad`.
- **P2 Host text (RESEARCH Q6).**
  - `Source/ParamText.h`: free functions, harness-testable, no JUCE GUI dependency:
    - `freqToText (v)`: `< 100 Hz` → 1 dp `"42.5 Hz"`; `< 1 kHz` → 0 dp `"440 Hz"`; `≥ 1 kHz` → 2 dp `"1.20 kHz"` (≥ 10 kHz: 1 dp `"12.0 kHz"`).
    - `dbToText (v)`: signed, 1 dp, `"+3.0 dB"`, `"-2.5 dB"`, `"0.0 dB"` (no `-0.0`).
    - `qToText (v)`: 2 dp. `percentToText (v)`: 0 dp `"100 %"`.
    - Parsers: trim; accept a leading `+`; a bare number; `k` / `kHz` (× 1000) / `Hz`; `dB`; `%`; case-insensitive. A string with no leading number returns the parameter's **default** (not 0, not the minimum). Results are clamped to the range by JUCE.
  - At `PluginProcessor.cpp:168`: build each float with `juce::AudioParameterFloatAttributes().withStringFromValueFunction (…).withValueFromStringFunction (…)`, picked by the parameter's kind (freq / gain / output gain / Q / gain scale) from the existing def table. No `withLabel`. Choice and bool parameters unchanged.
  - The `--dump-params` golden has no text column: `tests/golden/params.tsv` must stay byte-identical (HT1g).
- **P3 Page (RESEARCH Q6).**
  - `deleteBand`: call `endListenOn (b)` right after `setValue (false)`; update the comment (the enabled hook still serves host and preset disables).
  - Point tip: `clearPointTip` stays the immediate clear for re-show paths. `hideTooltip` starts the fade and defers the class + label-binding removal to `transitionend` (`propertyName === 'opacity'`) with a fallback timer of the fade time + 50 ms; a new `showTooltip` / `showPointTip` cancels the pending removal.
  - No change to the strip `aria-pressed` code (E-listen33 gates it as is).
- **P4 Glossary and revision (RESEARCH Q4).**
  - 18 entries (RESEARCH's 17 rows + `high cut` zh): into `scripts/i18n-fr-glossary.js`, `scripts/i18n-zh-glossary.js`, and the fr prose companion, together.
  - The three marked changes are taken: zh `analyzer` → 频谱分析仪; zh `pre / post` → 处理前 / 处理后; fr `mid signal` → signal central. Each width-checked by `check-ui-labels` at the widest language. If the zh analyzer segment does not fit, keep 频谱分析 with a `termNote` and record the deviation; if Pre / Post does not fit, the segment width is pinned at zh (`pattern_language_width_pins_content_sized_boxes_move`), never the caption shrunk.
  - Revise all fr and zh rows in `i18n.js`. fr stays `reviewed: false`; zh stays `'mt'`.
  - Independent scan before trusting a lint 0: a grep of `i18n.js` en strings for each new glossary key, compared with the lint's row count for that key (`pattern_new_lint_zero_must_agree_with_independent_scan`).
- **P5 G10 mode (RESEARCH Q3).**
  - `--g10`: Release only (the row prints `NOT GATED: Debug` otherwise). 48 kHz stereo, block 512, 60 s of audio per case, 3 repeats, median gated, min / max printed. Timing thread at `QOS_CLASS_USER_INTERACTIVE` (print the QoS read back). Never sleeps. Prints the 1-minute load average.
  - Rows: G10a ≤ 10 %, G10b ≤ 35 %, G10c ≤ 0.3 % **and** ≤ 1.5 × pass-through, G10d net-cost linearity `(a − c) ≤ 24 × (d − c) × 1.15`, G10i INFO against the 35 % line.
  - **G10d is read on net costs.** ARCHITECTURE's gross form passes trivially; subtracting the 0-band baseline is what makes it a linearity test. This is a reading, not an amendment; the row text says so.
  - Pass-through: a minimal `juce::AudioProcessor` whose `processBlock` returns, timed in the same loop with the same per-block noise copy.
- **P6 Windows prep (RESEARCH Q2).** `- O-ParametricEQ` under `ci-tests.yml` `options:`. Port the static WebView2 gate with its six negative controls.

---

## Tasks

### Wave 0 — pre-flight

1. [ ] **Location and baseline.** `git branch --show-current` = `main`; `git status --short` (record other plugins' dirty paths; touch none). Configure + build `O-ParametricEQ_VST3 O-ParametricEQ_AU O-ParametricEQ_Standalone O-ParametricEQ-render-harness O-ParametricEQ-designer-harness` at 0.9.0 (run in the background; `pattern_executor_watchdog_stall_run_long_commands_in_background`).
   - Files: none (scratch under `stages/4-polish/execute-4.1/baseline/`).
   - Depends on: none.
2. [ ] **0.9.0 baselines.** Using the verify-3.3 recipe: `--dump-sessions` → `sessions-090.sha` (must equal `stages/3-gui/verify-3.3/sessions-090.sha`); designer 150 / 0 / 18; render default **with `--lid-dir`** 171 / 0; probe N1e 16 / 16, hash `6274ff9b5b0e488a`; `--dump-params` diff vs `tests/golden/params.tsv` empty. A deviation here stops the phase.
   - Depends on: 1.

### Wave 1 — glossary (before any revision)

3. [ ] **Glossary entries.** Add the 18 entries (P4) to `scripts/i18n-fr-glossary.js`, `scripts/i18n-zh-glossary.js`, and the fr prose companion. Keep each file's existing entry format and ordering rules.
   - Files: the three glossary files.
   - Depends on: 2.
4. [ ] **Suite-wide lint re-run.** `i18n-fr-lint` and `i18n-zh-lint` over all plugins. Save logs to `execute-4.1/gates/lint-suite-{fr,zh}.log`. Findings in other plugins are listed in SUMMARY, not fixed. Findings in O-ParametricEQ are the W3 work list.
   - Depends on: 3.

### Wave 2 — C++ (dsp-agent)

5. [ ] **`Source/FactoryPresets.h`.** Def table (13 presets, RESEARCH Q1 values), `makeFactoryPresets`, `bankHash` (P1).
   - Files: `Source/FactoryPresets.h`, `CMakeLists.txt` (source list if headers are listed).
   - Depends on: 2.
6. [ ] **`initFactoryBank`.** Port (P1); call at the end of the constructor. Header declaration; comment citing PLAN-4.1 P1 and the two traps.
   - Files: `Source/PluginProcessor.h/.cpp`.
   - Depends on: 5.
7. [ ] **`Source/ParamText.h` + attributes.** P2 functions and parsers; float creation at `:168` passes the attributes by kind.
   - Files: `Source/ParamText.h`, `Source/PluginProcessor.cpp`.
   - Depends on: 2.
8. [ ] **Version 0.10.0.** `OPEQ_VERSION 0.10.0`; CHANGELOG `## [0.10.0] - <date>` (bracket form) with Added (factory bank, host text), Fixed (Delete ends listen at once; point tip size through fade), Changed (fr / zh revised).
   - Files: `CMakeLists.txt`, `CHANGELOG.md`.
   - Depends on: 5, 7.

### Wave 3 — page (gui-agent)

9. [ ] **`deleteBand` ends listen.** P3, one call + comment.
   - Files: `Source/ui/public/index.html`.
   - Depends on: 2.
10. [ ] **Point tip removed after the fade.** P3.
    - Files: `Source/ui/public/index.html`.
    - Depends on: 2.
11. [ ] **fr / zh revision.** Every fr and zh row in `i18n.js` against the grown glossary, including `bands-full`. The three marked changes (P4). fr `reviewed: false`, zh `'mt'` unchanged. Then the independent scan (P4).
    - Files: `Source/ui/public/js/i18n.js` (and `index.html` only if a width pin is needed).
    - Depends on: 4.

### Wave 4 — render harness rows (dsp-agent)

All rows run under the harness's existing `CFFIXED_USER_HOME` re-exec. FP2 additionally uses its own sub-home per case.

12. [ ] **FP1, FP1o** (RESEARCH Q1 "Rows" table, exact definitions), plus the scan of every written factory JSON for `output_gain` / `bypass` / `customState` keys.
    - Files: `tests/render-harness/main.cpp`.
    - Depends on: 6.
13. [ ] **FP3 (E16), FP4 (R2).** FP3 reads `Factory/Vocal Presence.json` bytes before and after the refused save. FP4 writes its hand-made file into `User/` and counts begin / end gesture pairs with a parameter listener.
    - Files: `tests/render-harness/main.cpp`.
    - Depends on: 6.
14. [ ] **FP2 bank hash.** Cases (a) – (e). (c) and (d) drive a test hook that swaps the def table (`#if OPEQ_TEST_HOOKS`, harness-only define; the plugin targets never see it). mtimes read with 1 s filesystem resolution in mind: (b) compares mtimes **and** inode / content hash.
    - Files: `tests/render-harness/main.cpp`, `Source/PluginProcessor.h/.cpp` (hook only under the define), `CMakeLists.txt` (define on the harness target only).
    - Depends on: 6.
15. [ ] **FP5 + FP5i.** Each preset at 44.1 / 48 / 88.2 / 96 / 192 kHz, mono and stereo, block 512 and the harness's random block sequence; load mid-stream into 2 s of pink noise. Finite; local κ inside G3's step-row measure for the load transient; settled response vs the designer's closed form with the G9 (a) machinery and tolerance. FP5i INFO: auto-gain value per preset (FAIL if any sits at −18 / +12), mono vs stereo broadband level change (Mono Low End shows the whole-signal cut).
    - Files: `tests/render-harness/main.cpp`.
    - Depends on: 6.
16. [ ] **HT1 + HT1g.** HT1: for each kind, 20 values spread over the range (skew-aware): `getValueForText (getText (v))` within display precision (relative 0.5 % on freq, 0.05 dB, 0.005 Q, 0.5 %). Parse cases: `"1k"` → 1000, `"1.2 kHz"` → 1200, `"-3 dB"`, `"+3"`, `"150%"`, `"440hz"`; `"abc"` and `""` → the default. `"-0.0 dB"` never printed. HT1g: `--dump-params` byte-equal to the golden.
    - Files: `tests/render-harness/main.cpp`.
    - Depends on: 7.
17. [ ] **M1j.** Block completions jittered ±30 % of a block period (seeded) at 1024 and 2048 / 44.1 kHz, reads at the editor tick. `PeakHold` with H = ⌈block / tick⌉ + 1: 0 empty reads. Control: H without the +1 shows > 0 empty reads (the control must fire, or the row is vacuous). Put it where M1 lives (render harness) unless `tests/meter-ballistics` is the cleaner fit; either way it runs in the battery.
    - Files: `tests/render-harness/main.cpp` or `tests/meter-ballistics/`.
    - Depends on: 2.
18. [ ] **`--g10` mode** (P5). Not in the default run; usage line updated.
    - Files: `tests/render-harness/main.cpp`.
    - Depends on: 2.

### Wave 5 — page-side gates and Windows prep

19. [ ] **PN1 / PN1n.** Port `preset_name_fit.js` to `tests/preset-name-fit/`: all 13 names fit `#preset-name` with no ellipsis at 800 × 500 under the en, fr and zh-Hans page fonts; a 40-character name overflows (control). Names read from `FactoryPresets.h` (parsed, not copied), so a renamed preset is re-checked.
    - Files: `tests/preset-name-fit/`.
    - Depends on: 5, 11.
20. [ ] **WebView2 static gate.** Port `windows-webview2-gate.sh` to `tests/`; run it; all six negative controls fire.
    - Files: `tests/windows-webview2-gate.sh`.
    - Depends on: 2.
21. [ ] **`ci-tests.yml` option line.** `- O-ParametricEQ` under `options:`. `default:` unchanged. YAML parses (`python3 -c 'import yaml…'` or `actionlint` if installed).
    - Files: `.github/workflows/ci-tests.yml`.
    - Depends on: 2.

### Wave 6 — build, battery, install, validators

22. [ ] **Build** all five targets (background). 0 warnings new to this phase in `Source/`.
    - Depends on: 8 – 18.
23. [ ] **Battery (4.1 reading).** Designer 150 / 0 / 18. Render default with `--lid-dir` (the W0 dump): 171 + new rows / 0. Session dump SHA list equal to `verify-3.3/sessions-090.sha` (16 / 16). Probe N1e 16 / 16, hash `6274ff9b5b0e488a`. `tests/g9b/run.sh` PASS. `tests/meter-ballistics` PASS. `--g10` once (record; 4.2 is binding). PN1 / PN1n, WebView2 gate.
    - Depends on: 19, 20, 22.
24. [ ] **Five UI gates at 0.** `check-i18n`, `i18n-fr-lint`, `i18n-zh-lint`, `check-ui-labels` (widest language), `boot-all-uis --strict-tips`. Logs to `execute-4.1/gates/`.
    - Depends on: 11, 22.
25. [ ] **Install + validators.** `./scripts/build-and-install.sh O-ParametricEQ` (dual-variant sweep, AU cache cleared). pluginval 10 on VST3 and AU, `auval -v aufx …`. Then confirm the real `~/Library/O-ParametricEQ/Presets/Factory/` holds 13 JSON + `.factory-version` + `.bank-hash`, and `User/` is unchanged from before install. Logs to `execute-4.1/validators/`.
    - Depends on: 23, 24.

### Wave 7 — editor bench rows + mutants

26. [ ] **Bench.** Copy `stages/3-gui/execute-3.3/editor-bench/` to `stages/4-polish/execute-4.1/editor-bench/`; keep the `CFFIXED_USER_HOME` re-exec before any processor exists (a mutated bank must never prune the real `Factory/`). Run lockstate first (`pattern_locked_screen_throttles_wkwebview_bench`). Add rows (RESEARCH Q1 / Q6 definitions):
    - **E-del-listen:** latch b, delete b; `setListenBand (0)` within one frame (< 20 ms). Control timing: without the fix it arrives ≥ 80 ms.
    - **E-listen33:** latch off, drag band 3 past the threshold; `#btn-listen[aria-pressed]` stays `"false"` at every frame.
    - **E-tipfade:** sample `getComputedStyle (tip).fontSize` every frame through the fade: 11.5 px throughout; `.tip-point` gone after the fade; a re-show mid-fade keeps the class.
    - **E-tip-lang:** switch language while the `bands-full` tip shows; `left + width ≤ innerWidth`, and the width re-measures.
    - **E-tip-v13:** hover a control while the point tip is up; the hover tip has no `data-i18n = "bands-full"`.
    - **E-preset-listen:** latch b03; load Kick Tighten (b03 stays on) → listen holds after the 80 ms settle; load De-Mud (b03 off) → listen ends.
    - Re-run the committed 3.3 rows (Release, one process) as regression: ALL-PASS.
    - Files: `stages/4-polish/execute-4.1/editor-bench/`.
    - Depends on: 25.
27. [ ] **Mutants (14).** One build each, scored against the named row; record in `execute-4.1/mutants/mutants41-results.json`.

    | # | Mutant | Must be caught by |
    |---|---|---|
    | M1 | `deleteBand` without `endListenOn` | E-del-listen |
    | M2 | `.tip-point` removed at fade start (old code) | E-tipfade |
    | M3 | strip `aria-pressed` bound to `ui.listen` | E-listen33 |
    | M4 | freq def hand-normalised as `(v − min) / (max − min)` | FP1 |
    | M5 | choice def as `index / (n − 1)` without the range | FP1 (or equivalent — record if so) |
    | M6 | one def names `output_gain` | FP1o |
    | M7 | `initFactoryBank` sentinel-only (no hash compare) | FP2 (c) |
    | M8 | no orphan delete | FP2 (d) |
    | M9 | delete recurses into `User/` | FP2 (e) |
    | M10 | `PresetSaveGuard` factory check off | FP3 |
    | M11 | `setValidatedApply (false)` | FP4 |
    | M12 | parser returns 0 on garbage | HT1 |
    | M13 | `freqToText` drops the `k` scale | HT1 |
    | M14 | G10d gross formula | G10d self-check: a synthetic non-linear cost (a sleep-free busy loop ∝ bands²) passes gross, fails net |

    - Depends on: 26.

### Wave 8 — bookkeeping + commit

28. [ ] **SUMMARY-4.1.md** (tallies, deviations, other-plugin lint findings, the 4.1 G10 reading, the 4.2 hands-on checklist draft: P6 items + Mono Low End on mono + host text in Logic / Reaper + presets in both). **STATUS.md** (phase execute, `next_action: stage_4_verify_4.1`, Current Position line). Update CHANGELOG date.
    - Depends on: 27.
29. [ ] **Commit.** Re-check `git branch --show-current` and `git status --short` immediately before. Path-scoped only:
    ```bash
    git commit -- plugins/O-ParametricEQ .github/workflows/ci-tests.yml scripts/i18n-fr-glossary.js scripts/i18n-zh-glossary.js .planning/quick/archive/2026-Q3/260826-ieq-multi-language-tooltips-across-all-vst-p/260826-ieq-FR-GLOSSARY.md
    ```
    New files need `git add` on those exact paths first (temp-index method per `index_git_shared_checkout` if another session has staged entries). Gitignored `Source/ui/public/modules/*.js` copies must not be committed. **No push, no tag.**
    - Depends on: 28.

---

## Dependency graph

```
W0: 1 → 2
W1: 2 → 3 → 4
W2: 2 → 5 → 6 ; 2 → 7 ; {5,7} → 8
W3: 2 → 9 ; 2 → 10 ; 4 → 11
W4: 6 → {12, 13, 14, 15} ; 7 → 16 ; 2 → {17, 18}
W5: {5,11} → 19 ; 2 → 20 ; 2 → 21
W6: {8–18} → 22 → 23 ; {11,22} → 24 ; {23,24} → 25
W7: 25 → 26 → 27
W8: 27 → 28 → 29
```

W1 and W2 run in parallel; W3's revision (11) waits on W1.

---

## Success criteria

- [ ] 13 factory presets load; each reads back its def in engineering units and every other parameter at its default (FP1); `output_gain` and `bypass` untouched (FP1o).
- [ ] Bank hash: re-voice and rename reach `Factory/` at 0.10.0 with no orphan; `User/` untouched (FP2 a – e).
- [ ] Factory overwrite refused; validated apply clamps and keeps defaults (FP3, FP4).
- [ ] Every preset renders finite and correct at five rates, mono and stereo (FP5); no auto-gain clamp (FP5i).
- [ ] Host text in units, round-trips, garbage → default (HT1); params golden byte-equal (HT1g).
- [ ] Delete ends listen within one frame; point tip keeps 11.5 px through its fade; E-listen33, E-tip-lang, E-tip-v13, E-preset-listen PASS; 3.3 bench rows ALL-PASS.
- [ ] M1j PASS with its control firing.
- [ ] Glossaries carry the EQ vocabulary; fr / zh revised; five UI gates 0; PN1 PASS and PN1n fires.
- [ ] `--g10` runs and prints G10a – d, G10i (4.1 reading recorded; budgets expected met per RESEARCH Q3).
- [ ] WebView2 static gate PASS, six controls fire; `ci-tests.yml` lists O-ParametricEQ.
- [ ] Audio frozen: designer 150 / 0 / 18; render 171 + new / 0 with `--lid-dir`; sessions SHA-equal to 0.5.0 16 / 16; N1e hash `6274ff9b5b0e488a`; G9 (b) and meter-ballistics PASS.
- [ ] pluginval 10 VST3 / AU and auval pass; real `Factory/` holds 13 + 2 sentinels.
- [ ] 14 / 14 mutants caught (an equivalent mutant is recorded with its reason, not counted).
- [ ] One path-scoped commit; no push, no tag.

---

## Risks

| Risk | Level | Mitigation |
|---|---|---|
| A process writes the real `Factory/` with a mutated or test bank | MEDIUM | Harness and bench re-exec under `CFFIXED_USER_HOME` before constructing a processor; FP2's hook exists only under `OPEQ_TEST_HOOKS` on the harness target; W6 task 25 checks the real dir after install. |
| Attributes change parameter identity in a host (Logic AU cache) | LOW | IDs, ranges and version hints unchanged; only text functions. HT1g + XR-lid prove no state change; install clears the AU cache. |
| zh 频谱分析仪 / 处理前 / 处理后 overflow their segments | MEDIUM | `check-ui-labels` at the widest language; pin the segment, or keep 频谱分析 with a `termNote` (P4). |
| New glossary entries raise lint findings in other plugins | MEDIUM | Reported, not fixed (out of scope). The O-ParametricEQ lint must be 0. |
| Bench timing rows flake on a busy or locked machine | LOW-MEDIUM | lockstate first; E-del-listen gates on frames (< 20 ms vs ≥ 80 ms), wide margin. |
| FP5's closed-form check on a preset with auto gain | LOW | Compare the response with auto gain's measured scalar divided out, as G9 (a) already does for its auto-gain draws. |
| Other sessions' staging joins the commit | MEDIUM | Re-check immediately before; path-scoped commit; temp-index method if anything is staged. |

---

## After 4.1

`/plugin-verify O-ParametricEQ 4-polish` (verify 4.1), then `/plugin-plan O-ParametricEQ 4-polish` for 4.2 (fresh battery with `--g10` and `--long`, C1 push + Windows dispatch, Taylor sitting, re-voice, fr flip; 0.11.0 if source changes).
