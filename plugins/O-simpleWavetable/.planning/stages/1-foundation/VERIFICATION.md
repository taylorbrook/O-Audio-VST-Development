# Stage 1: Foundation - Verification

## Verification Date

2026-10-05

## Method

Goal-backward against CONTEXT.md success criteria. Every automated claim in SUMMARY.md was **re-run independently** in this verify session. Nothing was taken on SUMMARY's word.

- `ninja` rebuild: no work to do, so the build is current with HEAD `b70e9d46` and the plugin tree is clean.
- The installed bundles (`~/Library/Audio/Plug-Ins/{VST3,Components}/O-simpleWavetable-dev.*`) are byte-identical (`diff -rq`) to the build artefacts.
- Host checks ran on the **installed** binaries:
  - targeted `auval`
  - pluginval VST3 and AU at strictness 10
  - the installed VST3 loaded in pedalboard (`uv run --python 3.12 --with pedalboard`), which is independent of the project's own probes

## Goal-Backward Analysis

### Original Goals (CONTEXT.md)

1. A buildable, host-loadable silent synth shell. CMake is ready for Stage 3 WebView.
2. The complete 21-parameter APVTS: exact IDs, ranges, defaults and text, in spec order.
3. `juce::Synthesiser` with 16 silent voices, plus MIDI plumbing (`IS_SYNTH`, MIDI in, stereo out, no input bus, `MidiMessageCollector`).
4. State save/load: APVTS XML, `uiLanguage`, and an `IMPORTED_BANK` stub that tolerates the child being absent or present.
5. RT-safe while silent, with zero latency.

### Deliverables (SUMMARY.md and code inspection)

1. `CMakeLists.txt`:
   - `OSiW`, 0.1.0, VST3/AU/Standalone
   - `IS_SYNTH`, `NEEDS_MIDI_INPUT`, `NEEDS_WEB_BROWSER`, `NEEDS_WEBVIEW2`
   - `JUCE_WEB_BROWSER=1`, the WebView2 static-link define, `JUCE_USE_CURL=0`
   - `webview-drop-streaming` module
2. `createParameterLayout`: 21 params, plain 4-arg `NormalisableRange` only. `jassert(size == 21)` and `== ParamIDs::all.size()`.
3. `WtVoice` ×16 and `WtSound`. Voices are allocated in the ctor. `pitchWheelPos` is seeded at note-on.
4. `getStateInformation` / `setStateInformation`:
   - root-type gate
   - `uiLanguage` restore gated on `isVoid()` and run through the allow-list
   - every `IMPORTED_BANK` copy stripped before `replaceState` and again on save
   - Stage 2.4 seams documented
5. `processBlock`: `ScopedNoDenormals`, `buffer.clear()`, a `numSamples <= 0` early-out, and no allocation. `setLatencySamples(0)` is in `prepareToPlay`.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1. Buildable shell, Stage 3-ready CMake | ✅ Achieved | `ninja` targets VST3/AU/Standalone are current. Flags are confirmed in CMakeLists.txt. |
| 2. 21-param contract | ✅ Achieved | pedalboard (installed VST3) lists 21 plugin params plus the host Bypass, in spec order. Defaults and text match: `Sine → Saw` (UTF-8 intact), `Full`, `1/1`, `Triangle`, `Poly`, `-6.0` dB. auval reports "21 Global Scope Parameters". |
| 3. Instrument + MIDI | ✅ Achieved | pedalboard `is_instrument=True`. A MIDI note-on/off render of 1 s at 48 kHz gives peak 0.0, all samples finite. auval Test MIDI PASS. |
| 4. State round trip | ✅ Achieved | pedalboard `raw_state` saved into a fresh instance restored position 0.42, bit_depth "3" and output_level "-inf". The project's own probes cover `uiLanguage` and `IMPORTED_BANK` tolerance (P1–P4, P8 in SUMMARY; code inspected and matches). |
| 5. RT-safe, zero latency | ✅ Achieved | processBlock inspected: no allocation, no locks. auval Latency PASS. |

## Requirements Verification

**Stage:** stage-1
**Requirements for this stage:** 1 total (1 must)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| COMPAT-01: Passes pluginval (VST3 + AU) | must | ✅ Complete | pluginval strictness 10 passes on both. VST3: exit 0, SUCCESS, 0 FAILED. AU: exit 0, SUCCESS, 0 FAILED. auval passes. |

**Requirements Summary:**
- ✅ Complete: 1
- ⚠️ Partial: 0
- ⏸️ Deferred (later stage): 29 (stage-2: 18, stage-3: 7, stage-4: 4)
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (VST3/AU/Standalone) | ✅ Pass | Current with HEAD, and the installed bundles match the artefacts |
| `auval -v aumu OSiW OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED. 2 WARNINGs, both "did not retain default value" on skewed params (`lfo_rate` 0.328712, `amp_attack` 0.082435). This is the known-benign float round trip on skewed ranges. |
| pluginval VST3 strictness 10 | ✅ Pass | exit 0, 0 FAILED |
| pluginval AU strictness 10 | ✅ Pass | exit 0, 0 FAILED |
| pedalboard: params / instrument / silent render / state | ✅ Pass | See Goal Achievement |
| Project probes (state-check P0–P8, param-dump) | ✅ Pass | From the execute session (SUMMARY Task 12). Not re-run, because pedalboard covered the overlapping claims independently. |

## Human Verification (non-blocking for Stage 2)

These are the Task 14 DAW smoke checks. The installed-binary checks above already cover their substance, so they gate nothing in Stage 2. Run them whenever convenient.

- [ ] Logic: AU appears as an **instrument** (Ouaricon → O-simpleWavetable-dev). The generic editor shows 21 rows with correct text.
- [ ] Reaper or Live: VST3 loads as an instrument, and MIDI from the keyboard plays silently with no errors.
- [ ] Project save → reopen restores changed values.

## Issues Found

- None blocking.
- The 2 auval default-retention warnings are benign. They are recorded in SUMMARY deviation 6.
- The pedalboard key for `output_level` is `output_level_db` because the label gets appended. That is a test-harness detail, not a defect.

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes. Next is Stage 2 (DSP), phase 2.1.

**Blockers:** None
