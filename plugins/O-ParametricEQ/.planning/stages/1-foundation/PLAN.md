---
title: "O-ParametricEQ Stage 1 (Foundation) — Execution Plan"
created: 2026-11-12
juce_version: "8.0.15"
summary: "Six waves, serial: (W0) pre-flight, (W1) processor shell with the 196-parameter layout and the v1 state codec, (W2) render harness + spec checker, (W3) CMakeLists landed LAST + one configure/build + golden generation + harness gates and negative controls, (W4) install + COMPAT-01, (W5) bookkeeping + path-scoped commit. Nothing configures before every file the CMake names exists, because a broken plugin CMake breaks the shared configure for every session."
stages: [1]
agents: [foundation-shell, build-gate-runner]
---

# O-ParametricEQ — Stage 1 Execution Plan (Foundation)

## Goal

O-ParametricEQ 0.1.0 builds as VST3 + AU + Standalone (`aufx OuPQ OuDv`). All **196** parameters are registered in exact `parameter-spec.md` order, in 25 parameter groups (50 Bool / 72 Choice / 74 Float), every one with version hint 1. Audio passes through bit-exact. The v1 state format round-trips exactly and loads the three foreign states (unknown parameter, missing parameter, `stateVersion="2"`). Gate G11 passes (DSP-07); pluginval 10 on VST3 + AU and `auval` pass (COMPAT-01).

Out of scope: any DSP (Stage 2), the WebView editor and every file under `Source/ui/` (Stage 3; `GenericAudioProcessorEditor` until then), presets and `OuariconPresetManager` (Stage 4).

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/1-foundation/CONTEXT.md` | D1–D5. Binding. |
| `stages/1-foundation/RESEARCH.md` | Q1 CMake, Q2 layout, Q3 processor, Q4 state codec, Q5 harness rows, **Q6 gate commands (cited below as #0–#11)**, Pitfalls. Authoritative over ARCHITECTURE.md and ROADMAP where they differ. |
| `parameter-spec.md` (LOCKED, sha256 `60e7cf7f…c6229f`) | The contract. Definitions `:41-152`, Full Registration Table `:154-355`, reserved IDs `:357-368`, UI state `:370-383`. NOT the draft. **Not edited in Stage 1.** |
| `plugins/O-AnalogEQ/CMakeLists.txt`, `Source/PluginProcessor.cpp:344-352` | CMake base; layout predicate. |
| `plugins/O-GrainScatter/Source/PluginProcessor.cpp:41`, `:126` | Parameter-group precedent. |
| `plugins/O-Granulator/tests/render-harness/main.cpp` (**at HEAD**, `git show HEAD:<path>`) | Harness shape: `GOLDEN_DIR` guard, `row()` / tally, host-normalised setter, state helpers. |
| `scripts/param-dump/ParamDump.cmake` | `ouaricon_add_processor_console` (FATALs on a missing driver, `:75-78`; skips `PluginEditor*` TUs; consoles get `JUCE_WEB_BROWSER=0`). |
| `mockups/v1-PluginEditor-TEMPLATE.{h,cpp}`, `v1-integration-checklist.md` § 2 | Read-only. Names Stage 3 expects: class `OuariconParametricEQAudioProcessor`, public `parameters`, `uiLanguage`, static `languageCode` / `languageIndex`. |

---

## Plan-phase decisions (resolve CONTEXT / RESEARCH open items)

- **P1 `uiLanguage` is a state ROOT attribute**, not a `UI`-child attribute (spec `:380`, checklist § 2, O-AnalogEQ). The `UI` child holds `analyzerMode`, `displayRange`, `selectedBand`, `editorWidth`, `editorHeight`. This clarifies CONTEXT's "`UI` child (analyzer mode, UI language)".
- **P2 An invalid blob leaves the state unchanged.** ARCHITECTURE says "Invalid XML → defaults"; suite precedent and RESEARCH Open Q2 say unchanged, so a host handing over garbage cannot wipe a session. Recorded as a deviation from ARCHITECTURE in SUMMARY. Row S7.
- **P3 Save always writes `stateVersion="1"`.** Unknown `PARAM` children from a newer session stay in the tree and are re-saved; the version number is not preserved.
- **P4 The G11 golden comes from a plugin-local harness**, not the suite `param-dump` (which has no index, version-hint, range or skew column). `tests/render-harness/main.cpp --dump-params` writes it. The suite `param-dump` target is wired too (later UI gates use it); its output is not committed in Stage 1.
- **P5 The golden cannot certify itself.** `tests/golden/check_spec.py` ties the committed golden to `parameter-spec.md` independently of the C++.
- **P6 `vu-meter` is proven now by a compile:** `#include "VUMeterBridge.h"` in `PluginProcessor.h`, no member. This answers CONTEXT's open item. If the header fails to compile under the suite warning flags: remove the include and the module line, record it in SUMMARY as a Stage 3 input, and continue (ROADMAP fallback: O-AnalogEQ's `outputLevel` event).
- **P7 `OuariconPresetManager` is not constructed in Stage 1.** The CMake module line is enough. A processor that owns one makes every console write under the real `~/Library`.
- **P8 No `PluginEditor.cpp` in Stage 1.** `target_sources` names `Source/PluginProcessor.cpp` only. `createEditor()` returns the Generic editor unconditionally; Phase 3.1 adds the `#if JUCE_WEB_BROWSER` guard.
- **P9 `CMakeLists.txt` is written LAST**, after every file it names exists. RESEARCH's skeleton configures first, but `ouaricon_add_processor_console` FATALs on a missing driver, and the root globs `plugins/*`. There is one configure, in T5.
- **P10 C++17.** No `cxx_std_20` on the plugin or the consoles. The sources must not use C++20 features.
- **P11 Registry `used_by` is updated now** (`scripts/regen-registry-used-by.sh` derives it from the CMake module lines, so any session's next regen would add it anyway). `modules/registry.yaml` carries another session's uncommitted edit (five `version: 0.1.0 → 1.0.0` lines), so the commit takes our hunks only (T8).
- **P12 Negative controls never mutate the repo.** The harness takes `--golden-dir <dir>`, the checker takes `--spec <file> --golden <file>`. Perturbed copies live in `$S`.
- **P13 Round-trip exactness is gated on `getValue()` bit-equality across all 196.** If a float row misses by ≤ 1 ULP, do NOT loosen the row: STOP, record the IDs and the deltas, and escalate (DSP-07 says "exact").

---

## Tasks

**Roles.** **[agent]** = foundation-shell-agent writes files (it has no Bash). **[run]** = the execute session runs commands and commits. **BG** = `run_in_background`, output to `$S/<name>.log`. `$S` = the executing session's scratchpad, never `/tmp`. **#N** = RESEARCH Q6 gate N. Paths are relative to `plugins/O-ParametricEQ/` unless they start with `scripts/`, `build/`, `modules/` or `~`.

### Wave 0 — pre-flight

**T1. Pre-flight** [run]
- Files: none. Save every output to `$S/preflight.txt`.
- Depends on: none.
- Done when (any mismatch → STOP; a hash mismatch is a contract change):
  - #0 `shasum -a 256 plugins/O-ParametricEQ/.planning/parameter-spec.md` = `60e7cf7fc335929259b4930d156a4f0d91cfa3b0a9f26d751ec31c5328c6229f`.
  - #0 `grep -h PLUGIN_CODE plugins/*/CMakeLists.txt | grep -c OuPQ` = 0 (verified 0 at plan time).
  - `cat .github/juce-version.txt` = `8.0.15`. `grep OUARICON_BUILD_TESTS build/CMakeCache.txt` = `:BOOL=ON`.
  - `ls -A plugins/O-ParametricEQ` = `.planning` only.
  - `node scripts/check-i18n.js | tail -1` → record the line as the #10 baseline (plan time: `ALL CHECKS PASS — 48 localized plugin(s)`).
  - `scripts/resolve-target.sh cmake-target O-ParametricEQ` → record the output (the folder-name fallback; makes the T5 check non-vacuous).
  - `git branch --show-current` = `main`. Save `git status --short` to `$S/git-status-T1.txt`. Everything outside `plugins/O-ParametricEQ` is foreign. At plan time: `.claude/agent-memory/*`, `.claude/compaction-snapshot.md`, `.planning/STATE.md`, `modules/registry.yaml`, and untracked files under O-GrainScatter and O-GranulatorInstrument.

### Wave 1 — processor shell (agent writes; nothing configures yet)

**T2. `Source/PluginProcessor.h` + `Source/PluginProcessor.cpp`: layout, buses, bypass, pass-through** [agent]
- Both files start with the suite AGPL-3.0-or-later header (copy from `scripts/param-dump/main.cpp:1-18`).
- **Class.** `OuariconParametricEQAudioProcessor`, public `juce::AudioProcessorValueTreeState parameters`, state tree type `"OParametricEQ"`. `#include "VUMeterBridge.h"` in the header (P6).
- **`createParameterLayout()`**: one table-driven builder, registration order = spec table `:160-355`.
  - Group `global` (name `Global`, separator `|`): `bypass` Bool Off, `output_gain` Float (−36, 36, 0) default 0, `gain_scale` Float (0, 200, 0) default 100, `auto_gain` Bool Off. Names `Bypass`, `Output Gain`, `Gain Scale`, `Auto Gain`.
  - Groups `band01` … `band24` (names `Band 1` … `Band 24`), each in this order:

    | ID suffix | Name suffix | Type | Range / choices | Default |
    |---|---|---|---|---|
    | `enabled` | `On` | Bool | | Off |
    | `bypass` | `Bypass` | Bool | | Off |
    | `type` | `Type` | Choice 8 | Bell, Low Shelf, High Shelf, Low Cut, High Cut, Notch, Band Pass, Tilt Shelf | 0 |
    | `freq` | `Freq` | Float | (10, 30000, 0) + `setSkewForCentre (1000.0f)` | 1000 |
    | `gain` | `Gain` | Float | (−30, 30, 0) | 0 |
    | `q` | `Q` | Float | (0.025, 40, 0) + `setSkewForCentre (1.0f)` | 1 |
    | `slope` | `Slope` | Choice 8 | `6 dB/oct`, `12 dB/oct`, `18 dB/oct`, `24 dB/oct`, `36 dB/oct`, `48 dB/oct`, `72 dB/oct`, `96 dB/oct` | 1 |
    | `placement` | `Placement` | Choice 5 | Stereo, Left, Right, Mid, Side | 0 |

  - IDs: `"b" + juce::String (n).paddedLeft ('0', 2) + "_" + suffix`. Names: `"B" + juce::String (n) + " " + suffix` (unpadded, D1).
  - Every parameter: `juce::ParameterID { id, 1 }`. Interval 0 on every float range. **No lambda `NormalisableRange`.**
  - `slope` is registered for every band regardless of type (D3). Nothing in Stage 1 interprets `type`, `slope`, `q` or `placement`.
  - A comment block directly above the builder lists the 72 reserved IDs (`bNN_dyn_range`, `bNN_dyn_threshold`, `bNN_dyn_auto`, NN = 01–24, version hint 2, not registered in v1).
- **Buses.** Stereo in / stereo out by default. `isBusesLayoutSupported`: output must be `mono()` or `stereo()` (compare with `==`), and the main input set must equal the output set (RESEARCH Q3).
- **Bypass.** `bypassParam = parameters.getParameter ("bypass")` in the ctor; `getBypassParameter() const override` returns it. `processBlockBypassed` forwards to the same pass-through path.
- **`prepareToPlay`**: `setLatencySamples (0)`, and nowhere else. `getTailLengthSeconds()` = 0. No MIDI in or out.
- **`processBlock`**: `juce::ScopedNoDenormals`; clear output channels beyond the input count; touch no sample otherwise. Reads no parameter.
- **`createEditor()`**: `return new juce::GenericAudioProcessorEditor (*this);`. `hasEditor()` = true (P8).
- **Forbidden in `Source/`** (code AND comments, since T6 greps for the tokens): the host-notifying parameter setter, parameter-listener add / callback names, `IIR::Filter`, `OuariconPresetManager`, any include of an editor header. Describe them in words where a comment needs to.
- Depends on: T1.
- Done when: verified in T5 (compile) and T6 (L1–L5, B1, P1, #11).

**T3. State codec + UI-state members** [agent] (same two files)
- **Members** (public, all `std::atomic<int>`): `uiLanguage { 0 }`, `analyzerMode { 2 }`, `uiDisplayRange { 12 }`, `uiSelectedBand { 0 }`, `uiEditorWidth { 960 }`, `uiEditorHeight { 600 }`. Statics `juce::String languageCode (int)` (1 → `fr`, 2 → `zh-Hans`, else `en`) and `int languageIndex (const juce::String&)` (unknown → 0).
- **`getStateInformation`** (RESEARCH Q4 "Save"):
  1. `auto state = parameters.copyState();` Never mutate the live tree.
  2. Remove every child named `UI` from the copy.
  3. Root properties `stateVersion = 1` (P3), `pluginVersion = JucePlugin_VersionString`, `uiLanguage = languageCode (…)` (P1).
  4. Append one fresh `UI` child from the members. It carries **no `id` attribute**.
  5. `copyXmlToBinary`.
- **`setStateInformation`** (RESEARCH Q4 "Restore"):
  1. `getXmlFromBinary`; return if null or the root tag is not the state type. Nothing changes on a rejected blob (P2).
  2. `juce::ValueTree::fromXml`.
  3. Read `UI` and root `uiLanguage` with `isVoid()` guards. Every value arrives as a string var: convert with `(int) v`, then validate: `analyzerMode` 0–3, `displayRange` ∈ {6, 12, 30}, `selectedBand` 0–24, width `jlimit` 800–1600, height `jlimit` 500–1000, language through `languageIndex`. An absent or invalid property leaves its member unchanged.
  4. Remove every `UI` child from the incoming tree.
  5. For each registered parameter with no `PARAM` child, append `PARAM id=… value=<convertFrom0to1 (getDefaultValue())>`. This is a tree edit; it writes no parameter.
  6. `parameters.replaceState (tree)`.
- `stateVersion` is read and not branched on.
- Depends on: T2.
- Done when: verified in T6 (S1–S7).

### Wave 2 — harness + spec checker (agent writes)

**T4a. `tests/render-harness/main.cpp`** [agent]
- AGPL header. `#ifndef GOLDEN_DIR #error`. `createPluginFilter()` + `dynamic_cast` to the processor class for the UI members. Flags: `--dump-params` (print the golden to stdout and exit 0), `--golden-dir <dir>` (default `GOLDEN_DIR`, P12). Exit code = number of FAILs; prints `N PASS, M FAIL`.
- **Dump format.** A `# GENERATED by O-ParametricEQ-render-harness --dump-params; do not edit` line, a header line, then 196 tab-separated rows with `"\n"` endings: `index, id, name, group, type, versionHint, start, end, interval, skew, defaultNorm, defaultRaw, numSteps, choices, automatable`. Floats `%.9g`; `choices` joined with `|`; empty cells `-`. The group comes from a walk of `getParameterTree()`. Written through `std::cout` only (never `replaceWithText`).
- **Rows:**

  | Row | Check |
  |---|---|
  | L1 | 196 parameters; the in-memory dump equals `<golden-dir>/params.tsv` byte for byte |
  | L2 | every version hint 1; every parameter automatable; types 50 / 72 / 74; 25 groups, IDs `global`, `band01` … `band24` in order |
  | L3 | none of the 72 reserved IDs resolves through `parameters.getParameter` |
  | L4 | `getBypassParameter()` is the parameter with ID `bypass`; latency 0 after `prepareToPlay` at 44.1 k and 96 k; tail 0 |
  | L5 | normalised defaults of every `freq`, `q`, `gain`, `output_gain`, `gain_scale` exactly 0.5; raw defaults of the linear rows exactly 0 / 0 / 100; raw `freq` and `q` within 2e-4 relative of 1000 and 1.0 (print the actual values) |
  | B1 | accepted: (1,1), (2,2). Refused: (2,1), (1,2), (0,2), (2,0), 5.1 → 5.1 |
  | P1 | pass-through: seeded noise at 0.5 peak, mono and stereo, block sizes 1, 64, 512, 4096, with `bypass` Off and On, and with every parameter at a seeded random value; output `memcmp`-equal to input. Also through `processBlockBypassed` |
  | P1n | vacuity guard: the P1 comparator FAILS on a copy with one flipped sample; input RMS > −20 dBFS |
  | S1 | round trip: all 196 set to seeded random normalised values through the host-notifying path, UI members non-default (`analyzerMode` 3, range 30, band 17, 1234 × 777), language `zh-Hans`; a fresh instance restores all 196 `getValue()` bit-equal (P13) and every UI member equal; `A → save` and `A → B → save` are byte-equal XML |
  | S2 | XML shape: root tag `OParametricEQ`; `stateVersion="1"`, `pluginVersion`, `uiLanguage="zh-Hans"` present; exactly 196 `PARAM`; exactly one `UI`; no `id` on `UI` |
  | S3 | two reopens with a UI change between them: the second reopen shows the changed value, still exactly one `UI` child |
  | S4 | unknown parameter (`<PARAM id="b01_dyn_range" value="6"/>` added): loads; known values applied; the unknown child is present in the next save |
  | S5 | missing parameter: the target instance has `b07_gain` and `b07_freq` set non-default first, the blob lacks both. After restore `b07_gain` raw = 0 exactly and normalised 0.5 exactly; `b07_freq` raw within 1e-6 relative of a fresh instance's raw default (print whether bit-equal). Guard: the row FAILS if the pre-set values were not non-default before the restore |
  | S6 | `stateVersion="2"`: loads, values applied, next save writes `"1"` |
  | S7 | wrong root tag, truncated blob, empty blob: no crash; all 196 values, the UI members and the language unchanged from a non-default starting state |
  | S8 | UI validation: `analyzerMode="9"`, `displayRange="7"`, `selectedBand="99"`, `uiLanguage="xx"` leave those members unchanged except language (→ en per `languageIndex`); width `"99999"` clamps to 1600; a blob with no `UI` child and no `uiLanguage` leaves all of them unchanged |

- Depends on: T3.
- Done when: verified in T6.

**T4b. `tests/golden/check_spec.py`** [agent]
- Python 3 standard library only. CLI `--spec <file>` (default the plugin's `parameter-spec.md`), `--golden <file>` (default `tests/golden/params.tsv` beside the script). Exit 1 on any failure, naming each one; on success print `OK: 196 params (50 Bool / 72 Choice / 74 Float), 25 groups`.
- Parses "Full Registration Table (196)" from the spec: 196 rows, indices 0–195 contiguous, IDs unique. For every row asserts the golden's index, ID, name, type and group.
- Per parameter kind (values transcribed from spec `:45-148` into one table in the script): start, end, interval 0, default raw (tolerance 2e-4 relative on `freq` / `q`, exact elsewhere), default normalised, choice strings and count, `versionHint` 1, `automatable` 1. Skew: linear rows exactly 1; `freq` / `q` equal `ln 0.5 / ln ((centre − start) / (end − start))` within 1e-5.
- Asserts no golden row's ID matches `_dyn_`, and no `\r` in the golden.
- Depends on: T1 (parallel with T2–T4a).
- Done when: verified in T6.

### Wave 3 — land CMake, build, gate

**T5. `CMakeLists.txt` + configure + build** [agent writes, run executes]
- **Precondition:** `ls` shows `Source/PluginProcessor.{h,cpp}`, `tests/render-harness/main.cpp`, `tests/golden/check_spec.py`.
- File: per RESEARCH Q1.
  - `set(OPEQ_VERSION 0.1.0)`; the plugin declaration with target `OuariconParametricEQ`, `PLUGIN_CODE OuPQ`, `FORMATS VST3 AU Standalone`, `PRODUCT_NAME "O-ParametricEQ${OUARICON_DEV_SUFFIX}"`, `IS_SYNTH FALSE`, `NEEDS_MIDI_INPUT FALSE`, `NEEDS_MIDI_OUTPUT FALSE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE` (D4, D5).
  - `target_sources`: `Source/PluginProcessor.cpp` only (P8). Include dir `Source`. O-AnalogEQ's link list verbatim. `juce_generate_juce_header`.
  - `ouaricon_add_module (OuariconParametricEQ preset-manager)` and `… vu-meter`. `eb-garamond` has no helper: a comment says its embed lands in Phase 3.1.
  - Definitions: `JUCE_VST3_CAN_REPLACE_VST2=0`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`.
  - No UI binary-data block (Phase 3.1; the snippet stays in `mockups/`).
  - Test block under `OUARICON_BUILD_TESTS`: `ouaricon_add_param_dump`, `ouaricon_add_processor_console (… tests/render-harness/main.cpp render-harness)`, `GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/golden"` on `O-ParametricEQ-render-harness`.
  - **No comment may spell the plugin-declaration command name followed by `(`.**
- Depends on: T2, T3, T4a, T4b.
- Done when:
  - #11 `grep -c "juce_add_plugin *(" plugins/O-ParametricEQ/CMakeLists.txt` = 1, AND `scripts/resolve-target.sh cmake-target O-ParametricEQ` = `OuariconParametricEQ` (differs from the T1 record).
  - #1 (BG) `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release > $S/configure.log 2>&1` → rc 0. The log shows the `preset-manager` and `vu-meter` module lines for this target.
  - #2 (BG) `ninja -C build OuariconParametricEQ_VST3 OuariconParametricEQ_AU OuariconParametricEQ_Standalone O-ParametricEQ-render-harness O-ParametricEQ-param-dump > $S/build.log 2>&1` → rc 0. `grep "O-ParametricEQ/.*warning:" $S/build.log` → no lines (this is also the P6 `vu-meter` proof).
  - `git status --short plugins/O-ParametricEQ` shows nothing under `Source/ui/` (the configure-copied module JS is gitignored).
  - On a configure failure: fix at once, or `mv CMakeLists.txt CMakeLists.txt.wip` and reconfigure so concurrent sessions are not blocked.

**T6. Golden, harness, negative controls, static gates** [run]
- Depends on: T5. `H` = the path printed by `find build/plugins/O-ParametricEQ -name 'O-ParametricEQ-render-harness' -type f -perm +111`.
- Done when, in this order:
  1. **Generate the golden.** `$H --dump-params > plugins/O-ParametricEQ/tests/golden/params.tsv`. `grep -vc '^#' …/params.tsv` = 197 (header + 196). `grep -c $'\r' …/params.tsv` = 0.
  2. #4 **Tie it to the spec.** `python3 plugins/O-ParametricEQ/tests/golden/check_spec.py` → exit 0 and the `OK:` line. A failure here is a layout bug in T2, never a reason to edit the checker's expectations or the spec.
  3. #3 **Harness.** `$H > $S/harness.log 2>&1` → rc 0, `0 FAIL`. Every row ID appears in the log: L1–L5, B1, P1, P1n, S1–S8. `$H --dump-params | diff - plugins/O-ParametricEQ/tests/golden/params.tsv` → empty. Copy the L5 actual raw defaults and the S5 bit-equal note into SUMMARY (Stage 2 discuss input).
  4. **Negative controls** (P12), on copies in `$S/golden-bad/`:
     - `b05_freq` → `b5_freq` in the ID column: `$H --golden-dir $S/golden-bad` → rc ≠ 0 with L1 FAIL; `check_spec.py --golden $S/golden-bad/params.tsv` → exit 1.
     - Separately, the `b12_q` `start` 0.025 → 0.03: `check_spec.py` → exit 1 naming `b12_q`.
     - Separately, a scratch spec with rows 7 and 8 swapped: `check_spec.py --spec $S/spec-bad.md` → exit 1.
     - Rerun both tools without flags → `0 FAIL` / `OK:`.
  5. Suite param-dump smoke: the `O-ParametricEQ-param-dump` binary → rc 0, 196 parameter rows. Output to `$S`, not committed.
  6. #11 **Static gates** (from the repo root; `Source/` exists, so none is vacuous):
     - `test -f plugins/O-ParametricEQ/Source/PluginProcessor.cpp && ! grep -rnE "setValueNotifyingHost|addParameterListener|parameterChanged|IIR::Filter|OuariconPresetManager|PluginEditor" plugins/O-ParametricEQ/Source`
     - `! grep -nE "NormalisableRange<float>[^;]*\[" plugins/O-ParametricEQ/Source/*.h plugins/O-ParametricEQ/Source/*.cpp` (no lambda range)
     - `grep -c setLatencySamples plugins/O-ParametricEQ/Source/PluginProcessor.cpp` = 1
     - `grep -c "_dyn_" plugins/O-ParametricEQ/Source/PluginProcessor.cpp` ≥ 3 (the reserved-ID comment), and row L3 PASS
     - `test ! -e plugins/O-ParametricEQ/Source/ui/public/js/i18n.js && test ! -e plugins/O-ParametricEQ/Source/PluginEditor.cpp`
  7. #10 `node scripts/check-i18n.js | tail -1` = the T1 baseline line.

### Wave 4 — install + COMPAT-01

**T7. Install + validate** [run]
- One validator at a time. Every run is BG with its own log. A cold `auval` rescans the registry (can take many minutes).
- Depends on: T6.
- Done when:
  - #5 `./scripts/build-and-install.sh O-ParametricEQ > $S/install.log 2>&1` → rc 0. `ls -d ~/Library/Audio/Plug-Ins/VST3/O-ParametricEQ*.vst3 ~/Library/Audio/Plug-Ins/Components/O-ParametricEQ*.component` lists exactly the two `-dev` bundles.
  - #6 `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --timeout-ms 600000 --validate ~/Library/Audio/Plug-Ins/VST3/O-ParametricEQ-dev.vst3 > $S/pv-vst3-<n>.log 2>&1`, **2 runs** (the fuzz reseeds) → `SUCCESS`, rc 0 each.
  - #7 the same with `--validate-in-process ~/Library/Audio/Plug-Ins/Components/O-ParametricEQ-dev.component` → `SUCCESS`.
  - #8 `auval -v aufx OuPQ OuDv > $S/auval.log 2>&1` → `AU VALIDATION SUCCEEDED`. Copy the "Reported Channel Capabilities" block verbatim into SUMMARY (`[1, 1] [2, 2]` expected, INFERRED); the value is frozen from here on because Logic caches AU I/O per version. Negative control: `auval -v aufx ZZZZ OuDv` → rc ≠ 0. **Never `auval -a`.**
  - #9 Standalone smoke: `bash -c 'A=build/plugins/O-ParametricEQ/OuariconParametricEQ_artefacts/Release/Standalone/O-ParametricEQ-dev.app/Contents/MacOS/O-ParametricEQ-dev; "$A" & p=$!; sleep 6; kill -0 $p && kill $p && echo ALIVE'` → `ALIVE`. If `Source/` changed after T5, rebuild the Standalone target first (build-and-install builds VST3 + AU only). A macOS microphone prompt may appear (the effect has an input); allow or deny, the gate is "alive after 6 s".
  - Any FAIL: fix the root cause in `Source/`, then rerun T5 (build), T6 and T7 in full. If the AU channel capabilities would have to change after a first successful `auval`, STOP and escalate.

### Wave 5 — bookkeeping + commit

**T8. CHANGELOG, registry, PLUGINS.md, SUMMARY, commit** [agent writes CHANGELOG + SUMMARY; run does the rest]
- Files: `CHANGELOG.md` (new; heading exactly `## [0.1.0] - <date>`, bracket form), `.planning/stages/1-foundation/SUMMARY.md`, and at the repo root `PLUGINS.md` + `modules/registry.yaml`.
- **`PLUGINS.md`.** Edit ONLY the O-ParametricEQ row (line 77 at plan time): version `-` → `0.1.0`, date → the execute date. Status stays `🚧 Stage 1` until verify.
- **Registry (P11).** `bash scripts/regen-registry-used-by.sh`. Our change = `O-ParametricEQ` under `preset-manager` and `vu-meter` `used_by`, plus the header `version` / `last_updated` lines. The file already carries a foreign uncommitted edit (five `version: 0.1.0 → 1.0.0` lines) and regen may surface other sessions' disk state: commit a blob built from `git show HEAD:modules/registry.yaml` plus our hunks only (temp index); leave the working-tree file as regen wrote it. If regen produces nothing for this plugin, record that and skip the file.
- **SUMMARY contents:** per-task results; harness tally; all negative-control results; the #0 hash at start and end; L5 actual raw defaults and the S5 note; the auval channel-capabilities block verbatim; the P6 `vu-meter` result; deviations (P2 against ARCHITECTURE; ROADMAP's "placeholder editor carries the resource map" superseded by D4); notes for Stage 2: the wrappers never call `processBlockBypassed` when a bypass parameter exists, so the bypass crossfade lives inside `processBlock`; raw 1 kHz / Q 1 defaults are inexact (discuss item 5); P1 stays a permanent row until 2.2 replaces it with the transparency gate.
- Depends on: T7.
- Done when:
  - #0 re-run: the spec hash is unchanged.
  - Commit per Execution notes. `git show --stat HEAD` lists: `CMakeLists.txt`, `CHANGELOG.md`, `Source/PluginProcessor.{h,cpp}`, `tests/render-harness/main.cpp`, `tests/golden/{check_spec.py,params.tsv}`, `.planning/stages/1-foundation/SUMMARY.md`; `PLUGINS.md | 2 +-`; only our `modules/registry.yaml` hunks. No `Source/ui/public/modules/*`, no foreign path.
  - `git tag --points-at HEAD` is empty.

---

## Dependency graph

```
T1 ─┬─ T2 ─→ T3 ─→ T4a ─┐
    └─ T4b ─────────────┴─→ T5 (CMake LAST, configure, build) ─→ T6 ─→ T7 ─→ T8
                                                                  ↑      │
                                                                  └──────┘ (gate FAIL → fix Source, rerun T5–T7)
```

T4b ∥ (T2 → T3 → T4a) is the only parallel pair.

## Execution notes

- **Roles.** foundation-shell-agent has no Bash: it writes T2–T5's files, CHANGELOG and SUMMARY. The execute session runs every command, the install, the validators and the commit.
- **Shared `build/`** (`OUARICON_BUILD_TESTS=ON` already). Targets: `OuariconParametricEQ_{VST3,AU,Standalone}`, consoles `O-ParametricEQ-{render-harness,param-dump}`. The new folder is invisible to ninja until the explicit configure in T5; `build-and-install.sh` skips configure when `build/` exists.
- **600 s watchdog.** Configure, ninja, build-and-install, every pluginval run and `auval` are BG with logs in `$S`. The harness, checker, greps and check-i18n run in the foreground.
- **Commit discipline** (trunk-based on `main`, shared checkout):
  - Never `git add -A` or `git commit -a`. Immediately before committing: `git branch --show-current` (= `main`) and `git status --short`.
  - Every Stage 1 file is new, and `git commit -- <paths>` skips untracked files: `git add` the exact new paths first, then `git commit -- plugins/O-ParametricEQ PLUGINS.md` (plus the registry through the temp index, or in the same temp-index commit).
  - Naming `PLUGINS.md` stages the whole file: check `git diff PLUGINS.md` is our one row first; if it carries a foreign hunk, commit a blob rebuilt from HEAD plus our row.
  - `CMakeLists.txt` and every file it names land in the same commit.
  - Afterwards: `git log -1 --stat`. No tags.
- **Handoff.** After execute: `/clear` → `/plugin-verify O-ParametricEQ 1-foundation`.

## Success criteria (CONTEXT Stage 1 gate)

| Criterion | Proven by |
|---|---|
| VST3 + AU + Standalone build | T5 #2 (rc 0, no warnings in this plugin's TUs); T7 #9 ALIVE |
| `auval` passes (COMPAT-01) | T7 #8 + `ZZZZ` negative control |
| pluginval strictness 10 on VST3 and AU (COMPAT-01) | T7 #6 (×2), #7 |
| G11: golden layout committed and diff-clean (DSP-07) | T6.1, T6.3 (L1 + `diff`), T6.4 negative controls |
| Golden agrees with `parameter-spec.md`: 196, order, IDs, names, groups, ranges, skews, defaults | T6.2 `check_spec.py`; L2, L5 |
| G11: state round trip exact | S1 (196 `getValue()` bit-equal, byte-equal XML), S2, S3 |
| G11: unknown parameter, missing parameter, `stateVersion="2"` load; known values applied, others default | S4, S5 (with pre-set guard), S6 |
| Invalid blob harmless; UI values validated by type | S7, S8 |
| `(2,1)` and `(1,2)` refused; mono and stereo accepted | B1 |
| Audio passes through unchanged; latency 0; `bypass` is the host bypass parameter | P1, P1n, L4 |
| No parameter written from a listener; no first-order `IIR::Filter` ctor; no lambda range | T6.6 |
| Reserved 72 IDs unregistered and listed beside the layout | L3; T6.6 `_dyn_` grep |
| Nothing under `Source/ui/`; check-i18n count unchanged; target resolves | T5, T6.6, T6.7 |
| Spec untouched; no tag; foreign files never staged | T1 / T8 #0; T8 stat + tag check |

## Risks

- **R1 Round trip not bit-exact on skewed floats.** Raw → normalised → raw is lossy in principle; the sibling plugins pass the same row. Handled by P13 (STOP and escalate, never loosen).
- **R2 Shared-build configure breakage.** CMake last; on failure move the file aside (T5).
- **R3 `vu-meter` first compile.** Header-only and small; fallback in P6.
- **R4 AU channel capabilities are INFERRED** until the first `auval`. They are frozen once Logic can see 0.1.0; a later change needs a version bump and an escalation.
- **R5 Concurrent sessions.** `modules/registry.yaml` already has a foreign edit; `PLUGINS.md` may gain one. Verify each diff hunk by hunk (T8).
- **R6 Gate tokens in prose.** A comment in `Source/` that names a forbidden call turns T6.6 red; a CMake comment that names the plugin-declaration call breaks target resolution. Both are stated in the task text.
