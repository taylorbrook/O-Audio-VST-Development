---
title: "O-GranulatorInstrument Stage 1 (Foundation) — Execution Plan"
created: 2026-11-07
juce_version: "8.0.15"
summary: "Seven waves, strictly serial: (W0) pre-flight, (W1) spec-only goldens + GiParamIds.h (261 params, 126 dests, 18 sources), (W2) silent instrument shell (Sidechain input typed kAux, int64-seed <Engine>/<Slots> scaffold, test hooks) + render harness, (W3) CMakeLists landed LAST + one configure/build + harness gates and negative controls, (W4) install + COMPAT-01 with the sidechain active, (W5) sidechain-inactive gate variant then restore, (W6) bookkeeping + path-scoped commit. A broken plugin CMake breaks configure for every session, so nothing configures before every named file exists."
stages: [1]
agents: [foundation-shell, build-gate-runner]
---

# O-GranulatorInstrument — Stage 1 Execution Plan (Foundation)

## Goal

O-GranulatorInstrument 0.1.0 builds as VST3 + AU + Standalone (`aumu OuGI OuDv`). All **261** parameters are registered in exact `parameter-spec.md` order (176 Float / 70 Choice / 15 Bool). The **126-entry** destination table and the **18-entry** source table are literal and append-only. The shell is a silent instrument with a stereo `Sidechain` input (VST3 `kAux`) that never reaches the output. The state scaffold has `<Engine>` + `<Slots>`. The render-harness scaffold has a MIDI script, seed injection, a `loadSlotSync` stub and a MockPlayHead. pluginval 10 passes on VST3 + AU with the sidechain active and inactive, and auval passes (COMPAT-01).

Out of scope: DSP and the fork (2.1), the WebView editor (3.1; `GenericAudioProcessorEditor` until then), any file under `Source/ui/`, presets and factory content (Stage 4).

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/1-foundation/CONTEXT.md` | D8 (Generic editor, WebView plumbing wired, no UI files), D9 (Sidechain on VST3 + AU, `kAux`, VST3-only drop on an attributable gate failure), D10 (polyphony wording kept). Binding. |
| `stages/1-foundation/RESEARCH.md` | Q1 kAux seam, Q2 CMake, Q3 params + trap rows, Q4 codec + S1–S5, Q5 harness rows, **Q6 gate commands (cited below as #0–#14)**, Pitfalls. Authoritative over ARCHITECTURE.md. |
| `parameter-spec.md` (LOCKED) | The only contract. Group tables `:46-195`, choice lists `:196-247`, dest table `:249-298`, state `:300-356`, Full ID list `:357-621`. NOT the draft. **Not edited in Stage 1.** |
| `mockups/v1-CMakeLists.txt` | Lands as `CMakeLists.txt` with four edits (T6). |
| `mockups/v1-PluginEditor.{h,cpp}`, `v1-integration-checklist.md` §2 | Read-only. Names Stage 3 expects: `GranulatorInstrumentProcessor`, public `parameters`, `std::atomic<int> uiLanguage`, static `languageCode` / `languageIndex`. |
| Parent O-Granulator **at HEAD** (`git show HEAD:<path>`). The working tree carries another session's uncommitted edits to `tests/render-harness/*` and `Source/dsp/*`. | `Source/GranulatorParamIds.h` (Def `:47-57`, `registrationOrder()` `:227-264`, dest literal + `modDestChoices()` `:282-330`); `Source/PluginProcessor.cpp` (`createParameterLayout` `:191-230`, codec `:440-555`); `tests/golden/extract_spec.py`; `tests/render-harness/main.cpp` (`:67-145` GOLDEN_DIR guard, Tally, `readTsv`/`goldenParams`; `:151-300` MockPlayHead; `:319` `setAllViaHostNormalised`; `:589-614` state helpers); `CMakeLists.txt:144-168`. |
| `plugins/O-Strata/tests/render-harness/main.cpp:359-420` | MIDI script shape. In this plugin it uses absolute sample times. |
| `modules/tuning/note-expression/cpp/NoteExpression.h:178-241` | `VST3Extensions` (not `final`) and `drainAndUpdate()`. |

---

## Plan-phase decisions (resolve RESEARCH open questions)

- **P1 `uiLanguage` is a state ROOT attribute, not an `<Engine>` attribute.** The spec ("properties of the APVTS state tree"), checklist `:52` and the parent all put it on the root. This clarifies CONTEXT's scaffold line. `<Engine>` holds `seed`, `stateVersion` and `modDestTableVersion`. The code string (`en` / `fr` / `zh-Hans`) is written on the `copyState()` copy and restored only when present. It is read tolerantly: a digit string maps to an index. Rows S1, S2 and S4 cover it.
- **P2 The `customScale` / `degreeMasks` codec moves to Phase 2.2.** The strip list reserves `CustomScale` and `DegreeMasks` now, next to `Engine` and `Slots`.
- **P3 `kAux` evidence in Stage 1 = harness row H2 + the code-path citation** `juce_audio_plugin_client_VST3.cpp:3108-3113`, recorded in SUMMARY. The Live/Reaper check is deferred to the Stage 4 DAW pass. No SDK probe.
- **P4 D10 header note: SKIP.** The wording is already correct, and the spec is not touched in Stage 1. #0 checks both hashes against STATUS at the start (T1) and at the end (T10).
- **P5 `loadSlotSync` uses slot base 0…3** (= the `rec_target` index). State `<Slot index>` and the page use 1…4. It stays a stub until 2.1.
- **P6 note-expression registry drift (1.1.1 in the registry, 1.1.2 on disk).** `scripts/regen-registry-used-by.sh` rewrites only the `used_by` blocks and the header lines (script header, verified), so it cannot fix the drift. Resolution: run regen (adds this plugin's 7 `used_by` rows), then **manually bump** the note-expression entry `version: 1.1.1` → `1.1.2`. SUMMARY records "regen + manual bump".
- **P7 The sidechain-disabled pluginval arm uses the gate-only CMake option `OGI_GATE_SC_DEFAULT_OFF`.** It defaults OFF; ON compiles `OGI_SC_DEFAULT_ACTIVE=0`. `OGI_GATE_NO_SIDECHAIN` is added only if a gate fails (D9 attribution, Contingency). **Never open Logic while the variant is installed.**
- **P8 (plan) The source sets the default for `OGI_SC_DEFAULT_ACTIVE`:** `#ifndef OGI_SC_DEFAULT_ACTIVE` → `1`. Consoles do not inherit the plugin target's compile definitions (`ParamDump.cmake:197-215` sets its own list), so the harness always tests the shipping default.
- **P9 (plan) `CMakeLists.txt` is written LAST, after every file it names exists.** RESEARCH skeleton step 5 configures before step 6 writes `main.cpp`, but `ParamDump.cmake:75-78` FATAL_ERRORs on a missing driver. There is one configure, in T6.
- **P10 (plan) Negative controls never mutate the repo.** The extractor takes `--spec <file> --out <dir>` and the harness takes `--golden-dir <dir>` (default `GOLDEN_DIR`). Perturbed copies live in `$S`.
- **P11 (plan) An AU-side failure is not a D9 trigger.** D9's remedy drops the bus on VST3 only, so it cannot fix an auval or AU-pluginval failure attributable to the input bus. On such a failure: STOP, record, escalate to the user. The AU I/O must be final before Logic first sees 0.1.0.

---

## Tasks

**Roles.** **[agent]** = foundation-shell-agent writes files (it has no Bash). **[run]** = the build/gate runner executes commands and commits. **BG** = `run_in_background`, with output to `$S/<name>.log`. `$S` = the executing session's scratchpad, never `/tmp`. **#N** = RESEARCH Q6 gate N. Paths are relative to `plugins/O-GranulatorInstrument/` unless they start with `scripts/`, `build/` or `~`.

### Wave 0 — pre-flight

**T1. Pre-flight** [run]
- Files: none. Save every output to `$S/preflight.txt`.
- Depends on: none.
- Done when (any mismatch → STOP; a hash mismatch is a contract change):
  - #0 Whole-file hash: `shasum -a 256 plugins/O-GranulatorInstrument/.planning/parameter-spec.md` = `b6bc70e3…ca751b`.
  - #0 Body hash: `sed -n '/^<!-- LOCKED BODY/,$p' plugins/O-GranulatorInstrument/.planning/parameter-spec.md | tail -n +2 | shasum -a 256` = `c5553e0c…543191`. Anchor on `^<!--`, because "LOCKED BODY" also appears in the prose on line 5.
  - #0 `grep -h PLUGIN_CODE plugins/*/CMakeLists.txt | grep -c OuGI` = 0.
  - `cat .github/juce-version.txt` = `8.0.15`.
  - `grep -c JUCE-NE-PATCH ~/JUCE/modules/juce_audio_processors_headless/utilities/juce_VST3ClientExtensions.h ~/JUCE/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp` ≥ 1 for each file. A missing marker would make the shared configure FATAL.
  - `grep OUARICON_BUILD_TESTS build/CMakeCache.txt` = `:BOOL=ON`.
  - `ls -A plugins/O-GranulatorInstrument` = `.planning` only.
  - `node scripts/check-i18n.js | tail -1` = `ALL CHECKS PASS — 47 localized plugin(s)` (baseline for #11).
  - `git branch --show-current` = `main`. Save `git status --short` to `$S/git-status-T1.txt`. Everything outside `plugins/O-GranulatorInstrument` is foreign. At plan time that was `.claude/agent-memory/*`, `.claude/compaction-snapshot.md`, O-Granulator `Source/dsp/*`, `Source/FactoryPresets.h` and `tests/render-harness/*`, and O-GrainScatter `nielsen-dafx26.txt`.

### Wave 1 — contract artefacts (spec-derived, no build)

**T2. Golden extractor + goldens** [agent writes, run executes]
- Files: `tests/golden/extract_spec.py`, plus the generated `tests/golden/params.tsv`, `mod-dests.tsv` and `mod-sources.tsv`.
- Start from the parent extractor at HEAD.
  - **Input: `parameter-spec.md` ONLY.** No YAML; host names come from the Full ID list. Replace U+2212 with `-`.
  - Parse Full ID list rows with `split(None, 3)`, because names contain spaces and `·`.
  - Widen the type regex to `Float(?: \(interval 1\))?|Choice|Bool`.
  - Templates: `sN_`, `menvN_`, `lfoN_`, `macroN`, `modM_`. The Mod-dest column adds `28·(N−1)` per stream and `N−1` for `lfoN_rate`. Skip `alias →` rows.
  - Parse the 32-entry scale grid, the 3-wide 126-entry dest grid (idx, id, host name, class) and the 18-row source table (idx, name, class).
  - CLI: `--spec <file> --out <dir>`. The defaults are the plugin's spec and `tests/golden`.
- Outputs are UTF-8, written with `newline='\n'`, and start with a `# GENERATED … do not edit` header:
  - `params.tsv` = idx id name type start end interval default skew centre choices
  - `mod-dests.tsv` = idx id name class
  - `mod-sources.tsv` = idx name class
- Cross-checks (exit 1 on any failure):
  - **Counts.** 261 rows, indices 0..260 contiguous, split 176/70/15. Every ID has a template row. 18 sources.
  - **Dest table.** 126 dests with `None` at 0. Every non-None dest is a Float, and its host name equals its Full-ID-list name (0 mismatches). The table equals the formula `b = 2 + 28·(N−1)`, built from the parent's 24 suffixes plus `scan_rate, loop_start, loop_end, key_track`, with fb + reverb at 114–121 and `lfoN_rate` at 122–125. Every Mod-dest column agrees with the table.
  - **Trap-row counts.** 19 `setSkewForCentre`, 14 raw skew, 6 interval-1, 43 negative-start. The recomputed `setSkewForCentre` skew equals the spec's printed skew ±5e-5.
  - **Choices and defaults.** Every choice list has ≥ 2 entries and an in-range default index. Host names are unique. Only `s1_enable` defaults On.
- Depends on: T1 (runs in parallel with T3).
- Done when:
  - #1 `python3 plugins/O-GranulatorInstrument/tests/golden/extract_spec.py` → exit 0 and prints `OK: 261 params (176 Float / 70 Choice / 15 Bool), 126 dests, 18 sources`.
  - Negative control: make a scratch copy of the spec with dest IDs at idx 2 ↔ 3 swapped and one Full-ID type changed. `extract_spec.py --spec $S/spec-bad.md --out $S/golden-bad` → exit 1, naming the failures.
  - `grep -c $'\r' plugins/O-GranulatorInstrument/tests/golden/*.tsv` → `0` for every file (CRLF goldens). Data rows = 261 / 126 / 18.

**T3. `Source/GiParamIds.h`** [agent]
- Copy the parent's shape (HEAD): `Def` plus a new **`float interval = 0.0f`** field, choice-list functions, per-group `std::array<Def, N>`, `registrationOrder()` and `allIdsInOrder()`.
- Group order (contract order): global 9 → amp 5 → `menvN_` 4×2 → `sN_` 38×4 → rec 7 → feedback 5 → reverb 3 → `lfoN_` 5×4 → `macroN` ×4 → `modM_` 3×16 = 261. IDs are loop-generated.
- Constants and tables:
  - literal `kModDestIds[126]` + `kModDestTableVersion = 1`
  - literal `kModSources[18]`, with each source's class beside it (per-voice / global / lfo-mode)
  - `kStateVersion = 1`
  - `modDestChoices()` = `None` + destination host names
  - `static_assert`s for 126 / 18 / 261
- **Interval 1 only** on `polyphony`, `pitch_bend_range` and `sN_root_note` (6 rows, all with start ≥ 0). Interval 0 everywhere else, including the 43 negative-start rows (stepped-range-zero trap).
- Skew is either a raw value or a `centre`; a `centre` becomes `setSkewForCentre` in the layout loop. **Never a lambda-mapped `NormalisableRange`** (the WebView frontend can't see it).
- Build host names by concatenation (`juce::String ("S") + juce::String (n) + " · " + suffix`, or `CharPointer_UTF8`). **Never `juce::String (const char*)` on a literal containing `·`**: that constructor is ASCII-only. No C++ identifier may be named `begin` or `end`.
- Depends on: T1.
- Done when: verified in T7 (G1–G6, D1–D5, SRC1, #13 lambda / `·` greps).

### Wave 2 — shell + harness (agent writes; nothing configures yet)

**T4. Processor shell** [agent]
- Files: `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`, and a placeholder `Source/PluginEditor.cpp` (a comment + `#include "PluginProcessor.h"`, nothing else). There is **no `PluginEditor.h` and no editor class** (D8).
- **Class.** `GranulatorInstrumentProcessor`, with public `juce::AudioProcessorValueTreeState parameters`.
- **Buses (D9).**
  - `static BusesProperties makeBuses()` = `withInput ("Sidechain", stereo(), OGI_SC_DEFAULT_ACTIVE != 0).withOutput ("Output", stereo(), true)`, with `#ifndef OGI_SC_DEFAULT_ACTIVE` → `1` (P8). No format branch now: this one function is the D9 fallback seam (Contingency).
  - `isBusesLayoutSupported`: the output must equal `stereo()`, and `getMainInputChannelSet()` must be one of `disabled()`, `mono()` or `stereo()`. Compare with `==` (AudioChannelSet is a bitset), never by `size()`.
- **VST3 (D9).** A plugin-local `struct GiVst3Extensions final : Ouaricon::NoteExpression::VST3Extensions`, with `bool getPluginHasMainInput() const override { return false; }`. Member `vst3Extensions`; `getVST3ClientExtensions() override { return &vst3Extensions; }`.
- **`prepareToPlay` and flags.**
  - `prepareToPlay`: `setLatencySamples (0)`, and nowhere else.
  - Cache the Sidechain channel count from the negotiated bus (`getBus (true, 0)` enabled → `getNumberOfChannels()`, else 0). Expose it through a public `getSidechainChannelCount()`. Never derive it from the buffer width.
  - `getTailLengthSeconds()` = 0. `acceptsMidi` = true, `producesMidi` = false, `isMidiEffect` = false.
- **`processBlock`.**
  - `juce::ScopedNoDenormals`, then `vst3Extensions.drainAndUpdate()`.
  - MIDI is consumed and not acted on.
  - `buffer.clear (ch, 0, n)` for every `ch < buffer.getNumChannels()`: output channels and input-only channels alike.
  - It reads no parameter and no sidechain.
- **`createParameterLayout()`.** Loops `registrationOrder()`:
  - Floats: `juce::ParameterID { id, 1 }` + `NormalisableRange<float> (start, end, interval, skew)`, then `setSkewForCentre (centre)` when `centre > 0`.
  - Choices and bools: `AudioParameterChoice` / `AudioParameterBool`, with `s1_enable` On.
  - Never call the reverb-unit's parameter-adding helper: it adds `rev_*` params, and the count would no longer be 261.
  - No parameter listeners, and no parameter write from any thread. Actions are Stage 3 native functions.
- **Seed and language.**
  - `std::atomic<uint64_t> seed`, filled from `Random::getSystemRandom().nextInt64()` in the ctor; public `getSeed()` / `setSeed (uint64_t)`.
  - `std::atomic<int> uiLanguage { 0 }`.
  - `static juce::String languageCode (int)`: 1 → `fr`, 2 → `zh-Hans`, else `en`.
  - `static int languageIndex (const juce::String&)`: unknown → 0.
- **State codec** (port the parent HEAD `:440-555`, RESEARCH Q4).
  - **Get.** `auto state = parameters.copyState()`; never mutate the live tree.
    - Strip `Engine`, `Slots`, `CustomScale` and `DegreeMasks` (P2).
    - Set the root attribute `uiLanguage` (P1).
    - Append `<Engine seed=String((juce::int64) seed) stateVersion="1" modDestTableVersion="1"/>` and an empty `<Slots/>`, then `copyXmlToBinary`.
    - No custom child carries an `id` attribute: APVTS treats a top-level child's `id` as a param ID.
  - **Set.**
    - Reject a blob unless it `hasTagName (parameters.state.getType())`; a rejected blob changes nothing.
    - Read `Engine` / `Slots`, strip them, then `replaceState`. Reset the non-param state; the seed is kept.
    - Guard every read with `isVoid()`. Ints come back as strings; never use `isBool()`.
    - Seed: only if the string is non-empty and `containsOnly ("-0123456789")`, then `(uint64_t) getLargeIntValue()`.
    - `uiLanguage`: only when present. A digit string becomes a clamped index; anything else goes through `languageIndex`.
- **Editor (D8).** `hasEditor()` = true. `createEditor()` has `#if JUCE_WEB_BROWSER` / `#else` branches, and in Stage 1 both return `new juce::GenericAudioProcessorEditor (*this)`. `PluginProcessor.*` never include `PluginEditor.h` or the drop-streaming header (consoles build with `JUCE_WEB_BROWSER=0`).
- **Test hooks** under `#if OGI_TEST_HOOKS` (P5), slot 0…3. Stage 1 bodies: `error = "loadSlotSync: stub until 2.1"; return false;`.
  - `bool loadSlotSync (int slot, const juce::File&, juce::String& error)`
  - `bool loadSlotSync (int slot, const juce::AudioBuffer<float>&, double sourceRate, const juce::String& name, juce::String& error)`
- **Comment discipline.** Comments in `Source/` must not spell any token the #13 greps look for: the host-notifying parameter setter, the listener add/callback names, the reverb parameter adder, the drop-streaming header or the editor header. Describe them in words; otherwise the gates go red on prose.
- Depends on: T3.
- Done when: verified in T6 (compile) and T7 (B1, B3, R1, S1–S5, H1, H2, #13).

**T5. Render harness `tests/render-harness/main.cpp`** [agent]
- **Copy from the parent HEAD.**
  - The `#ifndef GOLDEN_DIR #error` guard, `row()` / Tally, and `readTsv()` / `goldenParams()` (add the `interval` column).
  - `MockPlayHead` verbatim, including the segment script and `advance()`.
  - `setAllViaHostNormalised`, `stateOf`, `load`, `xmlOf`, `countChildren`.
  - Add `--golden-dir <dir>` (P10).
- **New: `MidiScript`.**
  - `struct MidiEv { juce::int64 t; juce::MidiMessage m; }`, sorted by absolute sample time.
  - Each host block takes the events with `blockStart ≤ t < blockStart + n`, at offset `t − blockStart`. Never block-relative.
  - Builders: `chord`, `noteOff`, `bend`, `cc` (74 / 64 / 1), `channelPressure`, `polyAT`, `mpeMember` (ch 2–16), `allNotesOff`. Pre-size with `MidiBuffer::ensureSize`.
  - Seed injection is `setSeed()` before `prepareToPlay`.
- **Rows.** The exit code is the number of FAILs; the harness prints `N PASS, M FAIL`.
  - **G1** 261 params; ID order == `params.tsv`.
  - **G2** 176 / 70 / 15.
  - **G3** Version hint 1 everywhere; host names byte-equal to the golden (`·` intact).
  - **G4** Ranges and defaults:
    - start / end / interval / skew equal the golden.
    - The 19 `setSkewForCentre` rows: skew == a reference `setSkewForCentre (centre)` AND within ±1e-4 of the spec skew.
    - The 14 raw-skew rows: exact.
    - The 6 interval-1 rows: `interval == 1`, and `get()` equals the default exactly.
    - The 43 negative-start rows: `interval == 0`.
    - Every Float: `getDefaultValue() == range.convertTo0to1 (specDefault)` exactly, and the engineering default within 1e-6 of the span. Never derive unity gain from `get()` on a skewed row.
  - **G5** 70 choice lists: strings, order and default index.
  - **G6** Bools: `s1_enable` On, the other 14 Off.
  - **D1** `kModDestIds` == `mod-dests.tsv` (126, in order).
  - **D2** Every non-None dest is an `AudioParameterFloat`.
  - **D3** All 16 `modM_dest` choice lists == `None` + the golden host names.
  - **D4** `kModDestTableVersion == 1`.
  - **D5** The literal == the formula `b = 2 + 28·(N−1)`.
  - **SRC1** All 16 `modM_source` lists == `mod-sources.tsv` (18).
  - **B1** Buses and MIDI:
    - Defaults: one input `Sidechain`, stereo, enabled; one output `Output`, stereo.
    - Accepted: input {disabled, mono, stereo} × output stereo.
    - Rejected: output mono / disabled / 5.1; input 5.1 / LCR.
    - After `setBusesLayout` + `prepareToPlay`, `getSidechainChannelCount()` = 2 / 1 / 0.
    - `acceptsMidi()` = true, `producesMidi()` = false, `isMidiEffect()` = false.
  - **B3** `getLatencySamples() == 0` after prepare at 44.1 k and 96 k; tail = 0.
  - **M1** MidiScript self-test: events at t = 0, 511, 512, 513, 4095 and 4096 are each delivered exactly once, at the right block and offset, for blocks {1, 441, 512, seeded variable}.
  - **R1** Sidechain-only render → output bitwise zero:
    - Layouts: stereo SC → stereo; mono SC → stereo (`12345.0f` written into buffer ch 1 as host garbage); control: SC disabled → stereo.
    - Stimulus: seeded noise at 0.7 + a full-scale 997 Hz sine on every input channel, written with `copyFrom` / `setSample` so `isClear` is false (a buffer flagged clear makes `clear()` a no-op).
    - MIDI: a 4-note chord at t = 0, notes on block boundaries ±1, bend, CC74, channel pressure, sustain on/off, MPE member notes with `mpe_enable` On, then note-offs.
    - Matrix: blocks {1, 32, 441, 512, 4096, seeded variable} × {no playhead, playing MockPlayHead} × {defaults, every param set to a seeded random value through the host-normalised path}, ≥ 1 s per case.
    - Verdict: every sample of every `buffer.getNumChannels()` channel `memcmp`-equals a zeroed buffer (so `-0.0f` fails), and every sample is finite.
  - **R1v** (vacuity guard) Input RMS > −20 dBFS on every SC channel before each `processBlock`.
  - **R1n** (vacuity guard) The R1 comparator FAILS on an unprocessed copy.
  - **S1a** Seed `0x0000000080000001` (> INT32_MAX). Set non-defaults through the host-normalised path: `polyphony` 12, `amp_release`, `glide`, `mod16_dest` 125, `s4_enable` On, `mod1_depth` −37, uiLanguage 2. Fresh instance → `getSeed()` equal, all 261 `getValue()` bitwise equal, uiLanguage equal.
  - **S1b** Seed `0xF00DFACECAFEBEEF` (> 2^63): the XML attribute is the negative int64 string, and it reads back equal.
  - **S2** XML shape: exactly one `<Engine stateVersion="1" modDestTableVersion="1">` and one `<Slots>` with 0 children; neither carries `id`. Root `uiLanguage="zh-Hans"`. 261 `PARAM` children.
  - **S3** A → B → change → C: still exactly one of each child, and the values follow the change (two reopens).
  - **S4** A blob without `<Engine>` / `uiLanguage` leaves the seed and the language unchanged. `uiLanguage="1"` → fr.
  - **S5** A wrong root tag leaves params, seed and language unchanged. Seed `"12ab"` / `""` leaves the seed unchanged.
  - **H1** `setSeed (x)` before `prepareToPlay` → `getSeed() == x` afterwards. Both `loadSlotSync` overloads return false with a non-empty error (proves the symbols link).
  - **H2** `getVST3ClientExtensions() != nullptr && ! …->getPluginHasMainInput()`. This is the only automated `kAux` evidence (P3).
- Any byte-compared dump uses `replaceWithText (t, false, false, "\n")`.
- Depends on: T4.
- Done when: verified in T7.

### Wave 3 — land CMake, build, gate

**T6. `CMakeLists.txt` + configure + build** [agent writes, run executes]
- **Precondition:** every file the CMake names exists (`ls` T2–T5's files). The root globs `plugins/*` without `CONFIGURE_DEPENDS`, so a CMakeLists naming a missing file breaks configure for every session.
- Files: `CMakeLists.txt` = `mockups/v1-CMakeLists.txt` verbatim except for four edits:
  1. **Defer the UI resources (D8).** Comment out the UI `juce_add_binary_data(…_UIResources …)` block and its `target_link_libraries(… _UIResources)` (mockup `:123-150`) under a `# Phase 3.1:` marker. The comment text must never spell the plugin-target call followed by `(`. The factory-source block stays commented.
  2. **Comment fixes.** note-expression is **1.1.2**, and it is the plugin-local `GiVst3Extensions`, not the module class, that returns `getPluginHasMainInput() = false`. The preset-manager / drop-streaming / eb-garamond comments say their embeds land in 3.1.
  3. **Gate option (P7).** `option(OGI_GATE_SC_DEFAULT_OFF "Gate-only: Sidechain inactive by default. Never ship." OFF)`. When ON: `target_compile_definitions(OuariconGranulatorInstrument PRIVATE OGI_SC_DEFAULT_ACTIVE=0)` + `message(WARNING "[O-GranulatorInstrument] OGI_GATE_SC_DEFAULT_OFF=ON: gate variant, do not ship")`.
  4. **Test block.** Keep the param-dump + its `cxx_std_20`. Add the parent's render-harness block (`:144-168` shape):
     - `ouaricon_add_processor_console(OuariconGranulatorInstrument ${CMAKE_CURRENT_SOURCE_DIR}/Source ${CMAKE_CURRENT_SOURCE_DIR}/tests/render-harness/main.cpp render-harness)`
     - `target_compile_features(O-GranulatorInstrument-render-harness PRIVATE cxx_std_20)` (consoles otherwise build at gnu++17)
     - `target_compile_definitions(O-GranulatorInstrument-render-harness PRIVATE GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/golden" OGI_TEST_HOOKS=1)`
- **Kept from the mockup:** `PLUGIN_CODE OuGI`, `VERSION 0.1.0`, `IS_SYNTH` / `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER` / `NEEDS_WEBVIEW2 TRUE`, `juce_gui_extra`, `juce_generate_juce_header`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, all 7 `ouaricon_add_module` lines, and `target_sources … PluginProcessor.cpp PluginEditor.cpp`.
- **Never create `Source/ui/public/js/i18n.js`** or anything else under `Source/ui/` (rule [8]). Configure copies module JS into `Source/ui/public/modules/`; those are gitignored artefacts.
- Depends on: T2, T3, T4, T5.
- Done when:
  - #13a `grep -c "juce_add_plugin *(" plugins/O-GranulatorInstrument/CMakeLists.txt` = 1, AND `scripts/resolve-target.sh cmake-target O-GranulatorInstrument` = `OuariconGranulatorInstrument`. Today it falls back to the folder name, so the check is non-vacuous.
  - #2 (BG) `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release > $S/configure.log 2>&1` → rc 0. The NE check: `grep -A6 "note-expression.*OuariconGranulatorInstrument" $S/configure.log | grep -m1 "JUCE-NE-PATCH markers verified"` prints the line. This is anchored on this target; RESEARCH's `"Adding module 'note-expression'"` matches every NE consumer. No `OGI_GATE_SC_DEFAULT_OFF` warning appears in the log.
  - #3 (BG) `ninja -C build OuariconGranulatorInstrument_VST3 OuariconGranulatorInstrument_AU OuariconGranulatorInstrument_Standalone O-GranulatorInstrument-render-harness O-GranulatorInstrument-param-dump > $S/build.log 2>&1` → rc 0. `grep "O-GranulatorInstrument/.*warning:" $S/build.log` → no lines.
  - On a configure failure: fix it at once, or `mv CMakeLists.txt CMakeLists.txt.wip` and reconfigure so concurrent sessions are not blocked.

**T7. Harness, negative controls, param-dump, static gates** [run]
- Depends on: T6.
- Done when:
  - #4 `build/plugins/O-GranulatorInstrument/O-GranulatorInstrument-render-harness_artefacts/Release/O-GranulatorInstrument-render-harness > $S/harness.log 2>&1` → rc 0, `0 FAIL`. Every row ID from T5 appears in the log (G1–G6, D1–D5, SRC1, B1, B3, M1, R1, R1v, R1n, S1a, S1b, S2–S5, H1, H2).
  - **Golden negative control.** Copy `tests/golden/*` to `$S/golden-bad/` and perturb it:
    - swap the IDs at idx 3 / 4 in `params.tsv`
    - change the `amp_release` default 1500 → 1501
    - change one `grain_limit` choice `256` → `512`
    - swap dest rows 2 ↔ 3
    - rename source 11 `Pressure` → `Aftertouch`
    
    Run `… --golden-dir $S/golden-bad` → rc ≠ 0, with at least G1, G4, G5, D1, D3 and SRC1 FAILing (record the count). Rerun without the flag → `0 FAIL`.
  - #5 `build/plugins/O-GranulatorInstrument/O-GranulatorInstrument-param-dump_artefacts/Release/O-GranulatorInstrument-param-dump > plugins/O-GranulatorInstrument/.planning/params.tsv`, then three independent checks against the spec:
    - `diff <(grep -v '^#' plugins/O-GranulatorInstrument/.planning/params.tsv | cut -f1) <(sed -n '/^## Full ID list/,/^## Design Sync/p' plugins/O-GranulatorInstrument/.planning/parameter-spec.md | awk '/^ *[0-9]+ +[a-z]/{print $2}')` → empty.
    - The same diff with `cut -f2` against `awk '/^ *[0-9]+ +[a-z]/{$1=$2=$3=""; sub(/^ +/,""); print}'` (host names) → empty.
    - `grep '^# params' plugins/O-GranulatorInstrument/.planning/params.tsv` → 261.
  - #13 greps. `Source/` must exist, so none of these is vacuous. All are run from the repo root:
    - `test -f plugins/O-GranulatorInstrument/Source/PluginProcessor.cpp && ! grep -rnE "setValueNotifyingHost|addParameterListener|parameterChanged|addParameters" plugins/O-GranulatorInstrument/Source`
    - `! grep -nE "WebViewDropStreaming|PluginEditor\.h" plugins/O-GranulatorInstrument/Source/PluginProcessor.h plugins/O-GranulatorInstrument/Source/PluginProcessor.cpp`
    - `! grep -nE "NormalisableRange<float>[^;]*\[" plugins/O-GranulatorInstrument/Source/*.h plugins/O-GranulatorInstrument/Source/*.cpp` (no lambda range)
    - `! grep -rnE 'String *\( *"[^"]*·' plugins/O-GranulatorInstrument/Source` (no `·` through the ASCII ctor)
    - `grep -c setLatencySamples plugins/O-GranulatorInstrument/Source/PluginProcessor.cpp` = 1, inside `prepareToPlay`
    - `test ! -e plugins/O-GranulatorInstrument/Source/PluginEditor.h && test ! -e plugins/O-GranulatorInstrument/Source/ui/public/js/i18n.js`
  - #11 `node scripts/check-i18n.js | tail -1` = `ALL CHECKS PASS — 47 localized plugin(s)` (unchanged). `node scripts/check-i18n.js --plugin O-GranulatorInstrument` → rc 0, 0 plugins localized.
  - #12 (BG, informational) `node scripts/boot-all-uis.js --plugin O-GranulatorInstrument` → "no UI root" with exit 0 is expected until 3.1. Exit 77 means Playwright is missing: record "not run". This is not a gate.

### Wave 4 — install + COMPAT-01 (shipping build, sidechain active)

**T8. Install + validate** [run]
- **One validator at a time.** Every run below is BG with its own log. A cold auval rescans the registry (83 s – 17 min).
- Depends on: T7.
- Done when:
  - #6 `./scripts/build-and-install.sh O-GranulatorInstrument > $S/install.log 2>&1` → rc 0. `ls -d ~/Library/Audio/Plug-Ins/VST3/O-GranulatorInstrument*.vst3 ~/Library/Audio/Plug-Ins/Components/O-GranulatorInstrument*.component` lists exactly the two `-dev` bundles (the script sweeps the alternate variant).
  - #7 `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 --timeout-ms 600000 --validate ~/Library/Audio/Plug-Ins/VST3/O-GranulatorInstrument-dev.vst3 > $S/pv-vst3-<n>.log 2>&1`, **3 runs** (the fuzz reseeds, NaN catch). Each → `SUCCESS`, rc 0, and each log shows `Main bus num input channels: 2` and `Main bus num output channels: 2`. The bus tests ("Enabling all buses", "Disabling non-main busses", "Restoring default layout") complete.
  - #8 The same with `--validate-in-process ~/Library/Audio/Plug-Ins/Components/O-GranulatorInstrument-dev.component` → `SUCCESS`. The warning "Current program is -1" is expected.
  - #9 `auval -v aumu OuGI OuDv > $S/auval-cold.log 2>&1` → `AU VALIDATION SUCCEEDED`. Copy the "Input Scope Bus Configuration" and "Reported Channel Capabilities (explicit)" blocks verbatim into SUMMARY; `[1, 2] [2, 2]` is expected (INFERRED). These values are frozen from here on. Negative control: `auval -v aumu ZZZZ OuDv` → rc ≠ 0. **Never `auval -a`.**
  - #10 Standalone smoke: `bash -c 'A=build/plugins/O-GranulatorInstrument/OuariconGranulatorInstrument_artefacts/Release/Standalone/O-GranulatorInstrument-dev.app/Contents/MacOS/O-GranulatorInstrument-dev; "$A" & p=$!; sleep 6; kill -0 $p && kill $p && echo ALIVE'` → `ALIVE`. If `Source/` changed after T6, run `ninja -C build OuariconGranulatorInstrument_Standalone` first: build-and-install builds VST3 + AU only.
  - Any FAIL → **Contingency** before T9.

### Wave 5 — sidechain-inactive arm (#14)

**T9. Disabled-SC gate variant, then restore** [run]
- **Never open Logic between step 2 and the reinstall in step 4.** Logic caches AU I/O per VERSION.
- Steps:
  1. (BG) `cmake -S . -B build -DOGI_GATE_SC_DEFAULT_OFF=ON > $S/cfg-scoff.log 2>&1`. The warning line is present, and `grep '^OGI_GATE_SC_DEFAULT_OFF' build/CMakeCache.txt` = `:BOOL=ON`.
  2. (BG) `./scripts/build-and-install.sh O-GranulatorInstrument > $S/install-scoff.log 2>&1`.
  3. (BG) Run on the variant:
     - pluginval VST3 ×2 → `SUCCESS`, and each log shows **`Main bus num input channels: 0`**. This proves the variant is what ran; "Enabling all buses" then re-enables the bus, so both states are covered.
     - pluginval AU in-process → `SUCCESS`.
     - `auval -v aumu OuGI OuDv` → PASS.
  4. Restore:
     - (BG) `cmake -S . -B build -DOGI_GATE_SC_DEFAULT_OFF=OFF > $S/cfg-restore.log 2>&1` → cache `:BOOL=OFF`, and no warning in the log.
     - (BG) build-and-install.
     - (BG) pluginval VST3 ×1 → `SUCCESS` with `Main bus num input channels: 2`.
     - Warm `auval -v aumu OuGI OuDv` → PASS, with channel capabilities identical to T8.
- Depends on: T8.
- Done when: steps 1–4 pass; `grep '^OGI_GATE_SC_DEFAULT_OFF' build/CMakeCache.txt` = `:BOOL=OFF`; the installed binaries are the shipping build.

**Contingency — D9 attribution.** Run this only when a T8 or T9 gate fails.
1. Keep the failing log. Classify the failing row: a bus/layout test, audio processing with the input active, or something else.
2. A/B test: add `option(OGI_GATE_NO_SIDECHAIN … OFF)`. When ON, it compiles `OGI_NO_SIDECHAIN=1` and `makeBuses()` omits `withInput`. Rebuild, reinstall and rerun the same gate.
3. The failure is attributable only if it fails with the bus AND passes without it. Anything else is not D9: fix the root cause, then rerun T8.
4. **Attributable on VST3 pluginval → D9 fallback.**
   - In `makeBuses()`, omit the input when `juce::PluginHostType::getPluginLoadedAs() == juce::AudioProcessor::wrapperType_VST3`. The wrapper type is set before `createPluginFilter()` (`juce_CreatePluginFilter.h:40-45`). AU keeps the bus.
   - Rerun T7, T8 and T9 in full.
   - Record the deviation and both outputs in SUMMARY, and as a dated note under D9 in `CONTEXT.md`.
5. **Attributable on AU (auval or AU pluginval) → STOP and escalate** (P11).

### Wave 6 — bookkeeping + commit

**T10. Bookkeeping, SUMMARY, commit** [agent writes SUMMARY; run does the rest]
- Files: `.planning/params.tsv` (from T7), `.planning/stages/1-foundation/SUMMARY.md` ("SUMMARY-1"), and `PLUGINS.md` + `modules/registry.yaml` at the repo root.
- **`PLUGINS.md`.** Edit ONLY row 76: `| O-GranulatorInstrument | 🚧 Stage 1 | 0.1.0 | Synth (Sample-Based Polyphonic Granular) | <date> |`.
- **Registry (P6).**
  - Run `bash scripts/regen-registry-used-by.sh`, then bump the note-expression entry `version: 1.1.1` → `1.1.2` by hand.
  - `git diff modules/registry.yaml` may contain only four kinds of change: this plugin's `used_by` entries (grain-engine, pitch-collections, reverb-unit, preset-manager, eb-garamond, note-expression, webview-drop-streaming), the header `version` / `last_updated`, and the note-expression version line.
  - Any other hunk is a concurrent session's disk state. Exclude it: build a temp-index blob from `git show HEAD:modules/registry.yaml` plus our hunks only.
- **SUMMARY contents:**
  - per-wave results; the harness tally; both negative-control results
  - the #0 hashes at start and end
  - the NE marker line
  - the pluginval bus-listing lines (shipping 2 ch / variant 0 ch) and the auval blocks, verbatim
  - the P3 code-path citation and the P6 resolution
  - deviations: the D9 fallback, if it fired, with both outputs
  - notes for Stage 2: R1 stays a permanent row (QUAL-02); skewed defaults are not bit-exact through APVTS; the SC channel count comes from `getSidechainChannelCount()`; an unrouted AU sidechain arrives as silence, not as a disabled bus (D6 decides on signal presence)
- Depends on: T9 (or the Contingency, if it ran).
- Done when:
  - #0 re-run: both spec hashes are unchanged. Before committing, `grep '^OGI_GATE_SC_DEFAULT_OFF' build/CMakeCache.txt` = `:BOOL=OFF` (R3).
  - The commit lands per Execution notes. `git show --stat HEAD` lists every new file: `CMakeLists.txt`; `Source/{GiParamIds.h, PluginProcessor.h, PluginProcessor.cpp, PluginEditor.cpp}`; `tests/golden/{extract_spec.py, params.tsv, mod-dests.tsv, mod-sources.tsv}`; `tests/render-harness/main.cpp`; `.planning/params.tsv`; `.planning/stages/1-foundation/{PLAN,SUMMARY}.md`.
  - The stat shows `PLUGINS.md | 2 +-` (exactly one row) and only our `modules/registry.yaml` hunks. It has no `Source/ui/public/modules/*` and no foreign path.
  - `git tag --points-at HEAD` is empty.
  - `STATUS.md` is not edited by the executor; the orchestrator owns it.

---

## Dependency graph

```
T1 ─┬─ T2 ──────────────────┐
    └─ T3 ─→ T4 ─→ T5 ──────┴─→ T6 (CMake LAST, configure, build) ─→ T7 ─→ T8 ─→ T9 ─→ T10
                                                                   │      │
                                                                   └──────┴─(gate FAIL)→ Contingency ─→ T7/T8/T9 rerun
```

Waves are serial. T2 ∥ T3 is the only parallel pair. CMake lands only after every file it names exists, because the shared configure discovers `plugins/*`.

## Execution notes

- **Roles.** foundation-shell-agent has no Bash: it writes T2–T6's files and SUMMARY. The runner executes every command, the install, the validators and the commit.
- **Copy sources.** Copy parent sources with `git show HEAD:plugins/O-Granulator/…`. The working tree has another session's uncommitted harness and DSP edits.
- **Shared `build/`** (`OUARICON_BUILD_TESTS=ON` is already set). Target names differ from the folder: `OuariconGranulatorInstrument_{VST3,AU,Standalone}`, consoles `O-GranulatorInstrument-{render-harness,param-dump}`, artefacts under `build/plugins/O-GranulatorInstrument/`. A new folder is invisible to ninja until an explicit `cmake -S . -B build` (T6); build-and-install skips configure when `build/` exists.
- **600 s watchdog.** Configure, ninja, build-and-install, every pluginval run and a cold auval are BG with logs in `$S`. The harness, param-dump, greps and check-i18n run in the foreground (< 1 min).
- **AU / Logic.** Never `auval -a` (it SIGABRTs here); always `auval -v aumu OuGI OuDv` + the `ZZZZ` negative control. Logic caches AU I/O per VERSION: the bus set is final before the first install, any D9 drop is VST3-only, and Logic stays closed throughout T9.
- **Commit discipline** (trunk-based on `main`, shared checkout with live sessions):
  - Never `git add -A` or `git commit -a`. Never stage the foreign files listed in T1.
  - Immediately before committing, re-run `git branch --show-current` (= `main`) and `git status --short`.
  - `git commit -- <paths>` silently skips untracked files, and every Stage 1 file is new. Either `git add` the exact new paths and then `git commit -- plugins/O-GranulatorInstrument PLUGINS.md modules/registry.yaml`, or commit through a temp index (`GIT_INDEX_FILE=… git read-tree HEAD; git update-index --add <paths>; git commit-tree`).
  - Naming `PLUGINS.md` or `registry.yaml` stages the WHOLE file. If either carries a foreign hunk, commit a blob rebuilt from `git show HEAD:<file>` plus our change only (temp index, `set -e`, fresh blob names).
  - `CMakeLists.txt` and every file it names land in the same commit, after the T6 configure.
  - Afterwards, re-check `git log -1 --stat` and that `HEAD` moved only by our commit.
  - No tags (tags are `/publish` only). A 0→1 gate needs `--force`.
- **Handoff.** After execute: `/clear` → `/plugin-verify O-GranulatorInstrument`.

## Success criteria (CONTEXT Stage 1 gate)

| Criterion | Proven by |
|---|---|
| VST3 + AU + Standalone build | T6 #3 (rc 0, no new-TU warnings), T8 #10 ALIVE |
| auval PASS | T8 #9 cold + negative control; T9 variant + warm restore |
| pluginval 10 on VST3 + AU with the sidechain **enabled** | T8 #7 (×3, `Main bus num input channels: 2`), #8 |
| pluginval 10 on VST3 + AU with the sidechain **disabled** (or the D9 fallback, recorded) | T9 (`Main bus num input channels: 0`, VST3 ×2 + AU + auval), or the Contingency record |
| Sidechain-only render (stereo + mono SC, MIDI present) → output bitwise zero | T7 #4 rows R1 + R1v + R1n |
| 261 params, ID order == spec Full ID list (golden), split 176 / 70 / 15 | T2 #1; T7 G1–G3; T7 #5 independent ID + name diff |
| Dest table golden (126, order frozen) + source table golden (18) | T7 D1–D5, SRC1; T2 formula cross-check |
| Ranges, defaults, skews incl. every `setSkewForCentre` row (19) and every interval-1 row (6) | T7 G4 (+ 14 raw-skew rows, 43 negative-start rows), G5, G6 |
| State round trip: params + `<Engine>` (seed > INT32_MAX and > 2^63) + empty `<Slots>` + root `uiLanguage` | T7 S1a, S1b, S2–S5 |
| NE configure check (`JUCE-NE-PATCH`) | T6 #2 |
| Buses / MIDI correct; latency 0; `kAux` | T7 B1, B3, M1, H2 (P3) |
| No param writes from listeners or the audio thread; no WebView headers in the processor; no lambda ranges | T7 #13 |
| Spec untouched; check-i18n unchanged; no tag; foreign files never staged | T1/T10 #0; T7 #11; T10 stat + tag check |

## Risks

- **R1 D9 gate failure.** RESEARCH rates it Low (the O-Texture precedent passes both validators, and JUCE's host ignores `busType`). Handled only through the Contingency's A/B attribution. An AU-side failure is a STOP (P11).
- **R2 Shared-build configure breakage.** CMake is written last, configure runs before any commit, and on failure the file is moved aside (T6).
- **R3 The variant gets shipped by accident.** If `OGI_GATE_SC_DEFAULT_OFF=ON` stays in the cache, the next build-and-install installs it. T9 ends only when the cache shows OFF and pluginval reports 2 input channels. T10 re-greps the cache before committing.
- **R4 `kAux` has no local verifier.** The JUCE host never reads `busType`, and the Steinberg validator is absent. H2 + the code-path citation are the Stage 1 evidence; Stage 4 Live/Reaper is the real check (P3).
- **R5 Unrouted AU sidechain = silence, not a disabled bus.** It does not matter for the shell. It is binding for 2.x Record-in (D6, signal presence), and is carried in the SUMMARY notes.
- **R6 Skewed defaults are not bit-exact through APVTS** (parent: worst 5e-8 of span). G4 checks the normalised value exactly and the engineering value with a tolerance. Stage 2 never derives unity gain from `get()`.
- **R7 pluginval runtime.** 261 params + Generic editor: `--timeout-ms 600000`, BG, one run at a time.
- **R8 Standalone microphone prompt.** The Sidechain is active by default, so the Standalone may open an input device and trigger the macOS mic-permission dialog during #10. Allow or deny; the gate is only "alive after 6 s". INFERRED.
- **R9 Concurrent sessions.** `PLUGINS.md` and `registry.yaml` may carry foreign hunks, and regen reads disk truth. Verify the diff hunk by hunk (T10).
- **R10 `juce8-critical-patterns.md` §4** ("input bus on instruments → missing input") is mis-attributed: it was an effect with no event bus (RESEARCH Q1d). `NEEDS_MIDI_INPUT TRUE` covers it. Informational.
