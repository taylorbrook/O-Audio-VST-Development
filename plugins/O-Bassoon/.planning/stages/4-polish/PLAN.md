# Stage 4: Polish — Plan

**Date:** 2026-05-01
**Plugin:** O-Bassoon
**Stage:** 4 of 4 (Polish / Validation / v1.0.0 Release)
**Phase:** plan
**Predecessors:**
- Stage 4 / discuss-phase ✅ COMPLETE — `stages/4-polish/CONTEXT.md` (8 user-confirmed decisions + 10 derived + 10 OQs + 10 risks)
- Stage 4 / research-phase ✅ COMPLETE — `stages/4-polish/RESEARCH.md` (10/10 OQ resolutions + 11 risks at Low/Low except R10 Medium/Low + 5 implementation skeletons + 18 static + 10 manual Gate 6 items)
**Hard gate (T1):** Stage 3 atomic commit (`feat(O-Bassoon): Stage 3 GUI - UI-01/UI-02 PASS`) MUST land on `main` before T3 begins. Per CONTEXT IV1 / RESEARCH OQ10. Working-tree audit confirms Stage 3 deltas are still uncommitted at plan-phase open (11 modified + 3 untracked groups).

---

## Goal

Ship O-Bassoon v1.0.0 in a single Stage 4 polish pass: re-confirm pluginval-10 macOS post Stage-3 commit, build and validate Windows VST3 (closes COMPAT-01), wire `OuariconPresetManager` + 4 factory presets via family-canonical infra, prove Dorico microtonal parity vs O-Lyrica baseline (closes COMPAT-02), close DSP-06 MPE-half via Logic Pro MPE-mode pitch-bend test (closes DSP-06 fully), write CHANGELOG v1.0.0 + update PLUGINS.md row + flip REQUIREMENTS rows COMPAT-01/02 + DSP-06 to complete, then land single atomic Stage 4 commit. v1.0.0 ships at that commit landing.

---

## Cycle Scope

**Single execute-phase (CONTEXT D1).** 4 deliverable buckets executed sequentially-where-coupled, parallel-where-independent. Per-task rev-3 inline iteration ceiling (CONTEXT D6 / IV3); independent budgets (R10 mitigation) — Win-build does not borrow from Dorico-parity etc.

**Bucket A: Stage 3 commit + working-tree split (T1–T2)**
STATUS.md 2-pass edit recipe (RESEARCH OQ10) + user-trigger wait for Stage 3 commit → resume with clean Stage-4-only working tree.

**Bucket B: Preset infra + CMake fixes (T3–T7)**
Wire `OuariconPresetManager` (header-only, family-canonical), add 4 factory presets via `Source/FactoryPresets.{h,cpp}`, fix `PLUGIN_VERSION` → `VERSION` keyword (silent-drop bug per RESEARCH §2), add preset-manager include path + new source.

**Bucket C: macOS build + verification (T8–T11)**
Local build, install via `/install-plugin O-Bassoon`, auval, pluginval-10 macOS AU+VST3 (re-confirm post Stage-3-commit), Logic-AU T9 smoke + T17 Gate 5 (Stage 3 carry-forward), 4-preset recall round-trip.

**Bucket D: Dorico parity (T12–T14)**
Author 8-beat 24-EDO test score per OQ2 recipe, ingest `.doricolib` via Library Manager Import (OQ1), tuner-verify ¼♯/¼♭/¾♯/¾♭/½♯/½♭ at ±5¢, A/B parity vs O-Lyrica baseline.

**Bucket E: DSP-06 MPE-half (T15–T16)**
Author 4-channel pitch-bend SMF (OQ9 Python recipe → `test-mpe-pitchbend.mid`), play through O-Bassoon-dev in Logic with multi-channel MIDI input enabled, tuner-verify per-channel-PB-shifted pitches at ±5¢.

**Bucket F: Windows build + verification (T17–T19)**
`.\scripts\build-and-install.ps1 O-Bassoon` (Win machine/VM hand-off), pluginval-10 Windows VST3, install verification in any Win DAW (Reaper / FL / Ableton-Win plugin scanner).

**Bucket G: Docs (T20–T23)**
`CHANGELOG.md` v1.0.0 entry (Keep-a-Changelog flavor per OQ6 + full template in RESEARCH §1), `PLUGINS.md` row update (🚧 Stage 0 → 📦 Installed 1.0.0 per OQ7), `REQUIREMENTS.md` flips (COMPAT-01/02 + DSP-06 → complete), `STATUS.md` final transition.

**Bucket H: Verify + atomic commit (T24–T27)**
18-item static-check battery (RESEARCH §5), Gate 6 10-item PASS roll-up, SUMMARY.md, single atomic Stage 4 commit (CONTEXT D7 / IV2).

**Out of scope (CONTEXT §Out of scope; v1.0.1 / v1.1 candidates):**
- DSP-05 attack-character morph v1.1 architectural pivot (NoiseExciter onset gate ramp 0→1)
- Preset browser UI
- PKG installer (`/package`)
- GitHub release / CI publish (`/publish`)
- Bitwig MPE host verification (Logic-MPE substitutes per CONTEXT D5)
- Cohort-wide `PLUGIN_VERSION` → `VERSION` sweep on other plugins (O-Bowed, O-Wind etc.)

---

## Task Dependency Graph (informational)

```
Bucket A — Stage 3 commit gate (HARD)
  T1 STATUS.md 2-pass split  ───  T2 wait-for-Stage-3-commit (user trigger)
                                    │
                                    ▼  (clean working tree, Stage 4 deltas only)
Bucket B — Preset infra
  T3 CMakeLists fix (VERSION + include + source)
  T4 FactoryPresets.h ─┐
  T5 FactoryPresets.cpp (4 presets) ─┤
  T6 PluginProcessor.h (include+member+helper)
  T7 PluginProcessor.cpp (ctor init + dispatch + state rewrite)
                                    │
                                    ▼
Bucket C — macOS build + verify
  T8 build O-Bassoon_VST3 + O-Bassoon_AU + install (CLAUDE.md cache-clear) + auval + pluginval-10
  T9 Logic-AU T9 smoke (Phase 3.1 8-item subset — Stage 3 carry)
  T10 Logic-AU T17 Gate 5 (8-item full incl. 60s long-tone — Stage 3 carry; OQ8 subjective)
  T11 4-preset recall round-trip in Logic
                                    │
        ┌───────────────────────────┼────────────────────────────┐
        ▼                           ▼                            ▼
Bucket D — Dorico parity        Bucket E — MPE-half          Bucket F — Windows
  T12 author .dorico file        T15 author .mid file          T17 Win build (ps1)
  T13 .doricolib import          T16 Logic MPE test → ±5¢      T18 pluginval-10 Win
  T14 tuner test → ±5¢                                         T19 Win DAW scan check
                                    │
        └───────────────────────────┼────────────────────────────┘
                                    ▼
Bucket G — Docs
  T20 CHANGELOG v1.0.0
  T21 PLUGINS.md row
  T22 REQUIREMENTS COMPAT-01/02 + DSP-06 → complete
  T23 STATUS.md → stage_4_complete + shipped_v1_0_0
                                    │
                                    ▼
Bucket H — Verify + commit
  T24 18-item static-check battery (RESEARCH §5)
  T25 Gate 6 PASS roll-up (10 manual items)
  T26 SUMMARY.md
  T27 atomic Stage 4 commit  →  v1.0.0 ships
```

**Parallelism:** Buckets D, E, F are independent of each other (different platforms / tools / hosts); they may run in any order or interleaved after Bucket C completes. Buckets A–C are strictly sequential. Bucket G is sequential within itself but waits for D/E/F to all complete (REQUIREMENTS flip needs all three closures). Bucket H is final.

---

## Bucket A — Stage 3 commit + working-tree split

### T1 — STATUS.md 2-pass split prep
- [ ] **T1** Lift Stage-4 deltas out of `STATUS.md` so the file reflects only Stage-3-closure context, ready for Commit 1.
  - Files: `plugins/O-Bassoon/.planning/STATUS.md`
  - Mechanism: `git diff plugins/O-Bassoon/.planning/STATUS.md` → identify Stage-4 lines (anything dated 2026-05-01 / referencing `stage_4_*` / Stage 4 phase cycle); save them as a working-tree patch (e.g., copy to `/tmp/o-bassoon-status-stage4-deltas.patch`), then revert STATUS.md to the Stage-3-only state. Manual hand-edit acceptable (RESEARCH OQ10 alternative).
  - Pass criterion: `git diff plugins/O-Bassoon/.planning/STATUS.md` shows ONLY Stage 3 closure context (`stage_3_complete`, Stage 3 phase cycle, T9/T17 carry-forward to Stage 4 noted but no Stage-4 cycle scope).
  - Depends on: none.

### T2 — Stage 3 atomic commit landing (HARD GATE)
- [ ] **T2** Wait for user trigger to land Stage 3 commit; verify working tree is clean of Stage-3 paths after commit; restore Stage-4 STATUS.md deltas.
  - Files (Commit 1 — landed by user, NOT this stage):
    - `plugins/O-Bassoon/CMakeLists.txt` (Stage 3 deltas: `juce_add_binary_data` block + WebView2 flags)
    - `plugins/O-Bassoon/Source/PluginEditor.{h,cpp}` (WebView UI rewrite)
    - `plugins/O-Bassoon/Source/PluginProcessor.{h,cpp}` (push-channel atomics)
    - `plugins/O-Bassoon/Source/BassoonVoice.h`, `Vibrato.h` (header-inline accessors)
    - `plugins/O-Bassoon/Resources/` (NEW — UI assets)
    - `plugins/O-Bassoon/.planning/stages/3-gui/` (NEW — 5 docs)
    - `plugins/O-Bassoon/.planning/{BRIEF,ROADMAP,REQUIREMENTS,STATUS}.md` (Stage 3 deltas only post T1)
  - Subject (locked at Stage 3 verify): `feat(O-Bassoon): Stage 3 GUI - UI-01/UI-02 PASS`
  - **HARD GATE:** Stage 4 execute-phase MUST NOT proceed past T2 until Commit 1 lands on `main`. User trigger required (`commit it` / `land it`). Per CONTEXT IV1 / D8.
  - After commit lands: re-apply saved Stage-4 STATUS.md patch; run `git status plugins/O-Bassoon/ PLUGINS.md` and verify only Stage-4-eligible paths remain modified/untracked (RESEARCH OQ10 working-tree clean check).
  - Pass criterion: `git log --oneline -1 main` shows Stage 3 subject; `git status` working tree shows only the Commit-2 file table from RESEARCH OQ10 (CMakeLists/PluginProcessor.{h,cpp} re-modified for Bucket B, plus Stage-4-only new files from later tasks).
  - Depends on: T1.

---

## Bucket B — Preset infra + CMake fixes

### T3 — CMakeLists.txt fixes
- [ ] **T3** Fix `PLUGIN_VERSION` → `VERSION` keyword; add preset-manager include path; add `Source/FactoryPresets.cpp` to `target_sources`.
  - Files: `plugins/O-Bassoon/CMakeLists.txt`
  - Edits (3, per RESEARCH §4.5):
    1. Line 14: `PLUGIN_VERSION "1.0.0"` → `VERSION "1.0.0"` (silent-drop bug; harmless at v1.0.0 but footgun for v1.0.1+).
    2. `target_sources(O-Bassoon ... PRIVATE)` block: add `Source/FactoryPresets.cpp`.
    3. `target_include_directories(O-Bassoon ... PRIVATE)` block: add `${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp`.
  - Pass criterion: `grep -n "VERSION\|PLUGIN_VERSION\|FactoryPresets.cpp\|preset-manager/cpp" plugins/O-Bassoon/CMakeLists.txt` shows 1 hit each for `VERSION "1.0.0"`, `FactoryPresets.cpp`, `preset-manager/cpp`; zero hits for `PLUGIN_VERSION`.
  - Depends on: T2.

### T4 — `Source/FactoryPresets.h` (NEW)
- [ ] **T4** Create header declaring the factory-preset builder.
  - Files: `plugins/O-Bassoon/Source/FactoryPresets.h` (NEW)
  - Content: namespace `FactoryPresets`, function `std::vector<OuariconPresetManager::FactoryPresetDef> build(juce::AudioProcessorValueTreeState&)`. Skeleton verbatim from RESEARCH §4.1.
  - Depends on: T2 (no compile dependency on T3, but landing T3 first keeps CMake → cpp ordering tight).

### T5 — `Source/FactoryPresets.cpp` (NEW — 4 factory presets)
- [ ] **T5** Implement `FactoryPresets::build()` returning the 4 ROADMAP presets ("Long Drone", "Microtonal Pad", "Tongued Long Tone", "Bright Bassoon") with normalized 0–1 param values.
  - Files: `plugins/O-Bassoon/Source/FactoryPresets.cpp` (NEW)
  - Content: 4-element vector with normalized values from RESEARCH §1 OQ5 (and §4.2 skeleton). Per-preset values are sane defaults; execute-phase tunes by ear at T11 (recall round-trip) if any preset sounds wrong.
  - Param-ID strings must exactly match `createParameterLayout()` IDs in `PluginProcessor.cpp` (`vibrato_rate`, `vibrato_depth`, `vibrato_onset`, `breath`, `tone`, `attack_character`, `attack_time`, `release_time`, `voice_count`, `output_gain`).
  - Pass criterion: file compiles standalone (`#include "FactoryPresets.h"` resolves; vector literal type-checks against `std::vector<OuariconPresetManager::FactoryPresetDef>`).
  - Depends on: T4.

### T6 — `Source/PluginProcessor.h` (preset-manager wiring)
- [ ] **T6** Add `OuariconPresetManager.h` include, `presetManager` member, and `initializeFactoryPresets()` private helper.
  - Files: `plugins/O-Bassoon/Source/PluginProcessor.h`
  - Edits (per RESEARCH §4.3):
    - `#include "OuariconPresetManager.h"` (after `#include "BassoonSynthesiser.h"`, before `#include "NoteExpression.h"`).
    - Public: `OuariconPresetManager& getPresetManager() { return presetManager; }` (optional, for future preset-browser UI).
    - Private members: `OuariconPresetManager presetManager;` (declare AFTER `parameters` so member-init order in the constructor matches declaration order — APVTS must construct before preset-manager binds to it).
    - Private helper: `void initializeFactoryPresets();`
  - Pass criterion: header compiles; `grep -n "OuariconPresetManager\|presetManager\|initializeFactoryPresets" plugins/O-Bassoon/Source/PluginProcessor.h` shows 4–5 hits.
  - Depends on: T3.

### T7 — `Source/PluginProcessor.cpp` (ctor init + dispatch + state rewrite)
- [ ] **T7** Wire preset-manager into constructor, dispatch factory-preset init, replace `getStateInformation`/`setStateInformation` with preset-manager wrappers.
  - Files: `plugins/O-Bassoon/Source/PluginProcessor.cpp`
  - Edits (per RESEARCH §4.4):
    - Constructor member-init list: append `, presetManager(parameters, "O-Bassoon")` after `parameters(...)` init.
    - End of constructor body (after voice setup loop): call `initializeFactoryPresets();`.
    - New method `OBassoonAudioProcessor::initializeFactoryPresets()`: idempotency guard (`if (factoryDir.isDirectory() && factoryDir.getNumberOfChildFiles(juce::File::findFiles) > 0) return;`) then `presetManager.initializeFactoryPresets(FactoryPresets::build(parameters));`. Add `#include "FactoryPresets.h"` at top of cpp.
    - Replace `getStateInformation` body (currently L318-323) with `auto xml = presetManager.getStateAsXml(); if (xml != nullptr) copyXmlToBinary(*xml, destData);`.
    - Replace `setStateInformation` body (currently L325-333) with `std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes)); if (xmlState != nullptr) presetManager.setStateFromXml(xmlState.get());`.
  - Behavioural note: state XML format may differ subtly from current `parameters.copyState().createXml()` because `OuariconPresetManager` wraps state. Existing user state from prior dev installs (if any) may not load cleanly post-upgrade — acceptable at v1.0 since plugin has not shipped publicly. Document in CHANGELOG Known Limitations only IF observed.
  - Pass criterion: cpp compiles; static-check #3 + #4 from RESEARCH §5 PASS (`grep -n "presetManager\|OuariconPresetManager" plugins/O-Bassoon/Source/PluginProcessor.{h,cpp}` ≥ 6 hits; `grep -n "FactoryPresets::build\|#include \"FactoryPresets.h\"" plugins/O-Bassoon/Source/PluginProcessor.{h,cpp}` = 1 include + 1 build call).
  - Depends on: T5, T6.

---

## Bucket C — macOS build + verification

### T8 — macOS build + install + auval + pluginval-10
- [ ] **T8** Build O-Bassoon_VST3 + O-Bassoon_AU; install per CLAUDE.md cache-clear protocol; run auval; run pluginval-10 on AU + VST3.
  - Commands (CLAUDE.md exact protocol):
    ```bash
    cd /Users/taylorbrook/Dev/VST-development/build && ninja O-Bassoon_VST3 O-Bassoon_AU
    killall -9 AudioComponentRegistrar 2>/dev/null || true
    rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
    rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3 ~/Library/Audio/Plug-Ins/Components/O-Bassoon-dev.component
    cp -R build/plugins/O-Bassoon/O-Bassoon_artefacts/Release/VST3/O-Bassoon-dev.vst3 ~/Library/Audio/Plug-Ins/VST3/
    cp -R build/plugins/O-Bassoon/O-Bassoon_artefacts/Release/AU/O-Bassoon-dev.component ~/Library/Audio/Plug-Ins/Components/
    auval -v aumu OBsn OuDv
    pluginval --strictness-level 10 --validate-in-process ~/Library/Audio/Plug-Ins/Components/O-Bassoon-dev.component
    pluginval --strictness-level 10 --validate-in-process ~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3
    ```
  - Pass criterion: ninja exits 0, auval prints `AU VALIDATION SUCCEEDED`, both pluginval-10 invocations exit 0.
  - Depends on: T7.

### T9 — Logic-AU T9 smoke (Phase 3.1 carry — 8-item subset)
- [ ] **T9** Open Logic with O-Bassoon-dev AU; verify the 8 Phase 3.1 Logic-AU smoke items pass (carried forward from Stage 3 verify per CONTEXT D9 / Stage 3 VERIFICATION).
  - Items (subjective bar — "no glitches in Logic"): plugin loads, UI renders, single noteOn produces audio, all 4 sections of knobs respond, Tuning tab opens (panel renders), About tab opens, save/recall preserves state, no console errors.
  - Pass criterion: subjective PASS roll-up "Phase 3.1 smoke OK".
  - Depends on: T8.

### T10 — Logic-AU T17 Gate 5 (Stage 3 carry — 8-item full incl. 60s long-tone)
- [ ] **T10** Run the 8-item Stage 3 Gate-5 full smoke incl. 60-second C3 long-tone + push-channel audition (active-voice dots / breath meter / vibrato dot live feedback).
  - Bar (OQ8 subjective): hold C3 for 60s — no clicks, pops, pitch drift, amplitude drift, NaN-driven silence, denormal-driven CPU spikes, vibrato-onset glitches, breath envelope flicker; push channels animate at 30 Hz without UI freeze.
  - Optional enhanced bar (OQ8): if anything sounds suspicious, render 60s C3 sustain via Logic Bounce → `phase4-60s-c3-regression.wav` → `python3 -c 'import scipy.io.wavfile as w, numpy; r = w.read("...")[1]; assert numpy.all(numpy.isfinite(r))'`.
  - Pass criterion: subjective PASS roll-up "Stage 3 polish OK".
  - Depends on: T8.

### T11 — 4-preset recall round-trip in Logic
- [ ] **T11** Recall each of the 4 factory presets in Logic AU dropdown; for each preset, save current state to .aupreset, reload, confirm parameter values match.
  - Bar: each preset audibly distinct from defaults; save/load round-trip preserves all 10 param values within float precision; preset names appear correctly in the AU dropdown.
  - Iteration budget: rev-3 (R10 mitigation — most likely consumer of inline iterations because preset-tuning requires ear). If a preset sounds wrong (e.g., "Bright Bassoon" too bright, "Long Drone" too quiet), tune the value(s) in `FactoryPresets.cpp` → delete `~/Library/Application Support/O-Bassoon/Presets/Factory/*.json` (force regenerate) → rebuild → re-test. Cap at 3 tuning rounds across all 4 presets combined.
  - Pass criterion: all 4 presets recall + round-trip PASS; subjective character matches names ("Long Drone" sustained / dark, "Microtonal Pad" slow swell, "Tongued Long Tone" articulated, "Bright Bassoon" brighter timbre).
  - Depends on: T8.

---

## Bucket D — Dorico microtonal parity (closes COMPAT-02 + DSP-06 NE-half)

### T12 — Author 24-EDO test score
- [ ] **T12** Author the 8-beat 24-EDO Dorico test score per OQ2 recipe.
  - Files: `plugins/O-Bassoon/.planning/stages/4-polish/test-score-microtonal.dorico` (NEW — committed at T27)
  - Recipe: New project from Solo→Piano template, swap Piano VST → O-Bassoon-dev, set 24-EDO key sig, write 8 quarter notes on C4 staff line per OQ2 table (none / ¼♯ / ½♯ / ¾♯ / none / ¼♭ / ½♭ / ¾♭).
  - Pass criterion: file saved at the path; opens in Dorico without errors.
  - Depends on: T8 (need O-Bassoon-dev installed).

### T13 — `.doricolib` ingestion via Library Manager Import
- [ ] **T13** Open Dorico → Library → Library Manager → Import → select `modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib`. Confirm "Ouaricon VST3 Note Expression" expression map appears under Library → Expression Maps.
  - Per-project assignment: in the test score from T12: Play → Endpoints → Add Plug-in (verify O-Bassoon-dev is loaded); Play → Endpoints → Expression Map → "Ouaricon VST3 Note Expression".
  - Pass criterion: expression map visible in Dorico's library; assigned to O-Bassoon-dev endpoint without warnings.
  - Depends on: T12.

### T14 — Tuner verification + O-Lyrica A/B parity
- [ ] **T14** Insert tuner plugin in Dorico's effect chain; play test score; verify each beat's pitch within ±5¢ of expected (OQ2 table). A/B with O-Lyrica.
  - Method A (per OQ2): tuner reads cents offset directly. Tolerance ±5¢ per spike-002 verdict bar.
  - A/B parity (CONTEXT D12): save copy of test score with O-Lyrica as channel VST → play → verify identical audible pitches. This proves note-expression module's `applyPendingTuning` composition is ABI-stable across consumers.
  - Pass criterion (Gate 6 item #6): all 8 beats land within ±5¢ of expected; A/B vs O-Lyrica tracks audibly identical.
  - Iteration budget: rev-3 (independent from T11 budget). If a beat drifts >5¢: troubleshoot at `applyPendingTuning` order (spike-002 known surprise) before treating as a real defect.
  - Depends on: T13.

---

## Bucket E — DSP-06 MPE-half (closes DSP-06 fully)

### T15 — Author 4-channel pitch-bend SMF
- [ ] **T15** Run OQ9 Python script (mido) to author `test-mpe-pitchbend.mid` with 4 simultaneous notes on channels 2/3/4/5 + per-channel pitch-bend (+1, 0, +2, -1 semitone).
  - Files: `plugins/O-Bassoon/.planning/stages/4-polish/test-mpe-pitchbend.mid` (NEW — committed at T27)
  - Script: one-shot at execute-phase per RESEARCH §7 hand-off ("the .mid output is what's committed, not the generator"). Recipe is reproducible from RESEARCH §1 OQ9.
  - Pass criterion: .mid file plays in any MIDI player (e.g., Logic / Reaper) and shows 4 simultaneous noteOn events on different channels with appropriate PB events preceding each.
  - Depends on: T8 (need O-Bassoon-dev installed for the consuming test, but file authorship itself has no plugin dependency).

### T16 — Logic MPE pitch-bend test → tuner verification
- [ ] **T16** New Logic project → Software Instrument track → load O-Bassoon-dev AU. **Critical:** enable per-track multi-channel MIDI input (Track Inspector → MIDI Channel = "All", remove any "Translate to Channel 1" filter). Drag `test-mpe-pitchbend.mid` onto track. Insert tuner. Play.
  - Expected (per OQ9 table, all ±5¢):
    - C3 +1 semi = C#3 ≈ 138.59 Hz
    - E3 +0 semi = E3 ≈ 164.81 Hz
    - G3 +2 semi = A3 ≈ 220.00 Hz
    - B3 −1 semi = Bb3 ≈ 233.08 Hz
  - Pass criterion (Gate 6 item #7): all 4 voices play simultaneously at distinct, per-channel-PB-shifted pitches within ±5¢. Each voice's pitch is independent.
  - Fallback if Path A fails (CONTEXT R5): Logic 11 typing-keyboard MPE input mode (Path B from OQ3); final fallback: trust Stage 2 inline `juce::Synthesiser::handlePitchWheel` static-check + accept partial DSP-06 MPE-half.
  - Depends on: T8, T15.

---

## Bucket F — Windows VST3 build + verification (closes COMPAT-01)

### T17 — Windows VST3 build via `build-and-install.ps1`
- [ ] **T17** On Windows machine/VM (CONTEXT D3 confirmed available): run `.\scripts\build-and-install.ps1 O-Bassoon` (or with `-Reconfigure` if first build).
  - Pre-flight: ps1 phase 1 verifies CMake, Ninja, Visual Studio 2022 (vswhere), JUCE at `C:\JUCE` or `$env:JUCE_DIR`.
  - Build path: 7-phase pipeline (per family precedent O-AnalogEQ / O-Wind / O-MicrotonalSampler).
  - Install: ps1 copies to `$env:COMMONPROGRAMFILES\VST3\O-Bassoon-dev.vst3` and clears Ableton plugin scan cache.
  - Pass criterion: ps1 exits 0; O-Bassoon-dev.vst3 present at install path.
  - Depends on: T7. Independent of macOS bucket (different host).

### T18 — Windows pluginval-10
- [ ] **T18** Run `pluginval.exe --strictness-level 10 --validate "$env:COMMONPROGRAMFILES\VST3\O-Bassoon-dev.vst3"`.
  - Pass criterion: exit 0. Family precedent (O-AnalogEQ + O-Wind + O-MicrotonalSampler) all clear strict-10 on Windows; static-linked WebView2 (`JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` in CMakeLists L113) eliminates the silent-blank failure mode.
  - Pluginval download (if not already on the Win box): <https://github.com/Tracktion/pluginval/releases>.
  - Depends on: T17.

### T19 — Windows DAW install verification
- [ ] **T19** Open at least one Windows DAW (Reaper / FL Studio / Ableton Live for Windows) and verify O-Bassoon-dev appears in plugin scanner; load 1 instance; play 1 noteOn; confirm audio.
  - Pass criterion (Gate 6 item #10): DAW lists O-Bassoon-dev; instance loads without crash; UI renders via WebView2 (NOT silent-blank IE fallback — verify by clicking knobs and observing parameter change feedback); 1 noteOn produces audio.
  - **Critical guard (memory critical pattern):** if WebView is silent-blank, IE backend fallback occurred — confirm `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` is in compiled binary (build cache may be stale; force `-Reconfigure`).
  - Depends on: T17.

---

## Bucket G — Docs (CHANGELOG / PLUGINS / REQUIREMENTS / STATUS)

### T20 — `CHANGELOG.md` v1.0.0 entry
- [ ] **T20** Create `plugins/O-Bassoon/CHANGELOG.md` with v1.0.0 entry per RESEARCH §1 OQ6 template (Keep-a-Changelog flavor, family precedent O-MicrotonalSampler v1.11.0).
  - Files: `plugins/O-Bassoon/CHANGELOG.md` (NEW)
  - Sections: H1 `# O-Bassoon Changelog`; `## [1.0.0] - 2026-05-01`; `### Added — Modal-Synthesis Bassoon for Microtonal Long Tones` (DSP architecture / Microtonal pipeline / Expression surface / WebView UI / Factory presets); `### Validation` (pluginval-10 macOS+Win + auval + Logic-AU 60s + Dorico parity + DSP-06 MPE-half); `### Known Limitations` (DSP-05 morph subtle / no preset browser / no PKG installer / Bitwig MPE not verified); `### Phase Trail` (commit shas: Phase 2.1–2.4, Stage 1, Stage 3 (T2), Stage 4 (T27 placeholder)).
  - Full template lifted verbatim from RESEARCH §1 OQ6 (lines 326-440 of RESEARCH.md).
  - Pass criterion (static-check #12): `grep -n "## \[1.0.0\] - 2026-05-01" plugins/O-Bassoon/CHANGELOG.md` returns 1 hit.
  - Depends on: T11, T14, T16, T19 (all closures landed before writing the validation section).

### T21 — `PLUGINS.md` row update
- [ ] **T21** Update O-Bassoon row from `🚧 Stage 0` → `📦 Installed 1.0.0`.
  - Files: `PLUGINS.md` (repo root)
  - Edit (per OQ7): replace existing L58 row
    ```
    | O-Bassoon | 🚧 Stage 0 | - | Synth (Physical Model Bassoon) | 2026-04-27 |
    ```
    with
    ```
    | O-Bassoon | 📦 Installed | 1.0.0 | Synth (Physical Model Bassoon) | 2026-05-01 |
    ```
  - Type field stays "Synth (Physical Model Bassoon)" (cohort-uniform naming per OQ7 — do NOT change to "Modal Synthesis").
  - Pass criterion (static-check #13): `grep -n "O-Bassoon.*Installed.*1.0.0" PLUGINS.md` returns 1 hit; `grep -n "O-Bassoon.*Stage 0" PLUGINS.md` returns 0 hits.
  - Depends on: T11, T14, T16, T19 (all closures verified).

### T22 — `REQUIREMENTS.md` flips
- [ ] **T22** Flip COMPAT-01, COMPAT-02, DSP-06 status from {partial / pending / partial} → complete; update version pill (v1.0.5 → v1.0.6); add "Stage 4" closure column entries.
  - Files: `plugins/O-Bassoon/.planning/REQUIREMENTS.md`
  - Rows:
    - **COMPAT-01** (pluginval cross-platform): partial → **complete** (closure: pluginval-10 macOS AU+VST3 PASS via T8; pluginval-10 Win VST3 PASS via T18).
    - **COMPAT-02** (Dorico microtonal parity): pending → **complete** (closure: 24-EDO 8-beat test score, all beats ±5¢, A/B parity vs O-Lyrica per T14).
    - **DSP-06** (NE end-to-end + MPE end-to-end): partial → **complete** (closure: NE-half via T13/T14 Dorico Library Manager Import + tuner; MPE-half via T16 4-channel pitch-bend Logic test).
    - **DSP-05** stays **partial** (v1.1 candidate per CONTEXT D10; document in CHANGELOG Known Limitations only).
  - Pass criterion (static-check #14): `grep -n "COMPAT-01\|COMPAT-02\|DSP-06" plugins/O-Bassoon/.planning/REQUIREMENTS.md | head -10` shows all three rows containing "complete".
  - Depends on: T11, T14, T16, T19.

### T23 — `STATUS.md` final transition
- [ ] **T23** Update STATUS.md from `stage_4_in_progress` / `phase: discuss_complete` → `stage_4_complete` / `phase: shipped_v1_0_0`; add Stage 4 closure section (Gate 6 PASS items, commit sha placeholder for T27, REQUIREMENTS deltas v1.0.5 → v1.0.6).
  - Files: `plugins/O-Bassoon/.planning/STATUS.md`
  - Carry forward Stage 3 closure context preserved at T1.
  - Pass criterion: yaml header shows `status: stage_4_complete`, `phase: shipped_v1_0_0`, `next_action: none` (or `v1_0_1_planning` if there's a clear v1.0.1 backlog).
  - Depends on: T20, T21, T22.

---

## Bucket H — Verify + atomic commit

### T24 — 18-item static-check battery (RESEARCH §5)
- [ ] **T24** Run all 18 static checks from RESEARCH §5; record output in SUMMARY.md.
  - Items 1–14 are pure greps (machine-checkable on macOS); items 15–17 are macOS auval + pluginval-10 (re-run if code changed since T8); item 18 is Win pluginval-10 (already PASS at T18 — cite the run rather than re-execute).
  - Pass criterion: all 18 checks PASS; any failures trigger inline fix + re-run within rev-3 budget.
  - Depends on: T7, T8, T18.

### T25 — Gate 6 PASS roll-up (10 manual items)
- [ ] **T25** Confirm all 10 Gate 6 manual items PASS (CONTEXT D9 / RESEARCH §5 manual table):
    1. Logic-AU T9 smoke (Phase 3.1 8-item subset) → T9 ✅
    2. Logic-AU T17 Gate 5 (8-item full incl. 60s long-tone) → T10 ✅
    3. pluginval-10 macOS AU+VST3 → T8 ✅
    4. pluginval-10 Windows VST3 → T18 ✅
    5. Dorico Playback Template ingestion → T13 ✅
    6. Dorico microtonal score parity vs O-Lyrica → T14 ✅
    7. DSP-06 MPE-half via Logic-MPE → T16 ✅
    8. 4-preset recall round-trip → T11 ✅
    9. CHANGELOG.md v1.0.0 entry written → T20 ✅
    10. Windows install verification → T19 ✅
  - Pass criterion: 10/10 PASS. If any fail, inline fix per per-task rev-3 budget; if budget exhausted, escalate to user.
  - Depends on: T8, T9, T10, T11, T14, T16, T18, T19, T20.

### T26 — `SUMMARY.md`
- [ ] **T26** Write `plugins/O-Bassoon/.planning/stages/4-polish/SUMMARY.md` (family-canonical schema): Gate 6 10-item table, 18 static-check results, REQUIREMENTS v1.0.6 deltas, Phase 2.1 → Phase 2.4 → Stage 1 → Stage 3 → Stage 4 commit-trail with shas (Stage 4 sha is placeholder until T27 lands), Known Limitations carry-forward to v1.0.1 / v1.1 (DSP-05 morph + preset browser + PKG installer + Bitwig).
  - Files: `plugins/O-Bassoon/.planning/stages/4-polish/SUMMARY.md` (NEW)
  - Depends on: T24, T25.

### T27 — Atomic Stage 4 commit (v1.0.0 ships)
- [ ] **T27** Single atomic commit. Subject (CONTEXT-locked): `feat(O-Bassoon): Stage 4 polish + v1.0.0 release - COMPAT-01/02 + DSP-06 PASS`.
  - Body: Stage 4 single-pass execute summary, Gate 6 10/10 PASS table, REQUIREMENTS v1.0.6 deltas, Phase 2.1 → Phase 2.4 → Stage 1 → Stage 3 → Stage 4 commit shas in trail.
  - Files (Commit-2 table from RESEARCH OQ10):
    - `plugins/O-Bassoon/CMakeLists.txt` (mod — VERSION fix + preset-manager include + FactoryPresets.cpp source)
    - `plugins/O-Bassoon/Source/FactoryPresets.{h,cpp}` (NEW)
    - `plugins/O-Bassoon/Source/PluginProcessor.{h,cpp}` (mod — preset-manager wiring)
    - `plugins/O-Bassoon/CHANGELOG.md` (NEW)
    - `plugins/O-Bassoon/.planning/REQUIREMENTS.md` (mod)
    - `plugins/O-Bassoon/.planning/STATUS.md` (mod — Stage 4 deltas)
    - `plugins/O-Bassoon/.planning/stages/4-polish/` (NEW — CONTEXT, RESEARCH, PLAN, SUMMARY, VERIFICATION + test-score-microtonal.dorico + test-mpe-pitchbend.mid)
    - `PLUGINS.md` (mod)
  - **No DSP-05 v1.1 work in commit** (CONTEXT IV4 — out-of-scope register).
  - **No mid-stage commits** (CONTEXT IV2).
  - **v1.0.0 ships at this commit landing on `main`** (CONTEXT IV5).
  - Pass criterion: `git log --oneline -1 main` shows the locked subject; `git status` is clean.
  - Depends on: T26.

---

## Success Criteria (Gate 6 — 10-item bar)

PASS = all 10 manual + 18 static checks PASS; single atomic commit landed.

- [ ] **Gate 6 item 1** Logic-AU T9 smoke (Phase 3.1 8-item subset) — subjective "no glitches"
- [ ] **Gate 6 item 2** Logic-AU T17 Gate 5 (8-item full incl. 60s long-tone + push-channel audition) — subjective per OQ8
- [ ] **Gate 6 item 3** pluginval-10 macOS AU + VST3 — both exit 0
- [ ] **Gate 6 item 4** pluginval-10 Windows VST3 — exit 0
- [ ] **Gate 6 item 5** Dorico Playback Template ingestion — "Ouaricon VST3 Note Expression" appears in Library → Expression Maps
- [ ] **Gate 6 item 6** Dorico 24-EDO microtonal parity — 8 beats × ±5¢; A/B vs O-Lyrica audibly identical
- [ ] **Gate 6 item 7** DSP-06 MPE-half — 4-voice channel-rotated pitch-bend test, all 4 within ±5¢
- [ ] **Gate 6 item 8** 4-preset recall round-trip — all 4 presets recall + save/load preserves params
- [ ] **Gate 6 item 9** CHANGELOG.md v1.0.0 entry written + readable
- [ ] **Gate 6 item 10** Windows install verification — DAW scanner finds O-Bassoon-dev; UI renders via WebView2 (not silent-blank IE fallback)

**Static checks (RESEARCH §5):** 18 items — 14 greps + auval + 2 macOS pluginval-10 + 1 Win pluginval-10. All must PASS.

**REQUIREMENTS deltas (v1.0.5 → v1.0.6):**
- COMPAT-01 partial → complete
- COMPAT-02 pending → complete
- DSP-06 partial → complete
- DSP-05 stays partial (v1.1 candidate, documented limitation)
- UI-01 / UI-02 already complete at Stage 3

---

## Process Invariants (carried from CONTEXT)

- **IV1** Stage 3 atomic commit MUST land on `main` BEFORE Stage 4 execute-phase begins (T2 hard gate).
- **IV2** Single Stage 4 atomic commit on Gate 6 PASS (no mid-stage commits).
- **IV3** rev-3 inline iteration ceiling, per-task independent budget.
- **IV4** No DSP-05 v1.1 work in Stage 4 (out-of-scope register; deferred to v1.0.1 / v1.1).
- **IV5** No Stage 5 — v1.0.0 ships at Stage 4 atomic commit landing on `main`.

---

## Iteration Budget Allocation (per-task rev-3, independent)

Per CONTEXT D6 + RESEARCH R10. Each independent task group has its own rev-3 budget; Win-build does not borrow from Dorico-parity etc.

| Task group | Most-likely consumer | Budget | Escalation |
|------------|---------------------|--------|------------|
| Bucket B (preset infra) | T7 cpp wiring (state-format change) | rev-3 | If exhausted: roll back to plain `parameters.copyState()` + ship presets without state migration. |
| Bucket C (macOS verify) | T11 4-preset tuning | rev-3 | If exhausted: ship "good enough" preset values; flag in CHANGELOG Known Limitations. |
| Bucket D (Dorico parity) | T14 ±5¢ tolerance | rev-3 | If exhausted: investigate `applyPendingTuning` order (spike-002 surprise); escalate to user before declaring DSP-06 NE-half failed. |
| Bucket E (DSP-06 MPE) | T16 Logic multi-channel MIDI input setup | rev-3 | If exhausted: fallback Path B (Logic 11 typing-keyboard MPE); final fallback partial DSP-06 + ship as-is per CONTEXT R5. |
| Bucket F (Win build) | T17 ps1 first-build environment friction | rev-3 | If exhausted: escalate to user (Win machine/VM access constraint). |

---

## Open Questions / Out-of-Scope at plan-phase

**No NEW open questions** (RESEARCH §7 hand-off lock). Pure planning.

**Plan-phase scope locks (per RESEARCH §7):**
- Task ordering = research recommendation (Bucket A → B → C → D/E/F parallel → G → H).
- Per-task rev-3 budgets, independent.
- Python MIDI authoring script is one-shot at execute-phase (T15); the .mid file is what's committed, not the generator.

---

## Hand-off to Execute-Phase

**Plan deliverables locked:**
- 27 tasks across 8 buckets
- Hard gate at T2 (Stage 3 atomic commit lands on `main`)
- Stage-4 atomic-commit subject locked: `feat(O-Bassoon): Stage 4 polish + v1.0.0 release - COMPAT-01/02 + DSP-06 PASS`
- Gate 6 10-item bar + 18 static checks
- Per-task rev-3 budgets with explicit escalation paths

**Ready for:** `/plugin-execute O-Bassoon 4-polish`

---
*Generated 2026-05-01 from `/plugin-plan O-Bassoon 4-polish`*
*Schema: family-canonical PLAN.md (matches Phase 2.1–2.4 + Stage 3 precedent)*
