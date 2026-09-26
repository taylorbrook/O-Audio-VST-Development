---
plugin: O-Formant
stage: 4
status: complete
phase: verified
version: 1.32.1
last_updated: 2026-09-25
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: dorico_microtonal_smoke_test
next_phase: done
contract_checksums:
  brief: sha256:887e6d791af653926f6ceb139dae19cb6fcc89668d0193023f05aba3da64bd0b
  parameter_spec: sha256:4fb1124cb5c2b37036da9c6b8f0cc92cd29a77ae4f923f6ad83c65b7541c48f8
  architecture: sha256:2e1fae651a274f47508f13825aaf264db18e01f2bd34ab4a280780d47583e563
  roadmap: sha256:3e668584a37cb671c62136f655295c65876cfd79ee8e9adc0bf0ed7beeef010d
---

# O-Formant Status

## Current Position

Stage: 4 of 4 (Polish) -- VERIFIED COMPLETE
Status: All stages verified, pluginval level 10 PASSED (VST3 + AU)
Progress: [####################] 100%

## v1.32.1 -- IN-12 diffusion remainder (2026-09-25)

- Stage-0 diffusion ring wrapped below ~36 kHz (142 read, 128 buffer). Buffers now sized max(scaled, raw)+1. PATCH; no change at >=44.1 kHz.

## v1.32.0 -- CODE_REVIEW.md Info-tier sweep 2, audible (2026-09-25)

- Resolved IN-01, 02, 04, 06, 11, 12, 14a, 15. MINOR (audible: bend/glide, burst tail, reverb off 48 kHz, EQ sweeps). Reverb exact at 48 kHz (all float damping values). Tail now computed from settings. auval PASS, pluginval 10 PASS (VST3), UI/i18n gates PASS. No render null (no harness).
- Open: IN-10, IN-17 (fix local table), IN-18 (document), IN-21 nav, IN-24 (semantics+i18n chosen; interactions open). Listening pass on factory presets owed (v1.31.0 + v1.32.0).

## v1.31.1 -- CODE_REVIEW.md Info-tier sweep (2026-09-24)

- Resolved (whole or part) IN-03, 07, 09, 16, 19, 20, 21 (save), 22a/c, 23a/b/d/e, 24g/h/i/j/k. PATCH. Audio bit-identical to v1.31.0 at 44.1/48 kHz (seeded out-of-tree null test, 6 cases + state round-trip); 192 kHz delay now reaches 2 s. auval PASS, pluginval 10 PASS (VST3), UI gates PASS.
- Open: audible IN-01/02/06/10/11/12/14a/15/17 (MINOR + listening pass); IN-18, IN-04, IN-21 nav (decisions); IN-24a-e (a11y/i18n feature). Listening pass on the 16 factory presets from v1.31.0 still owed.

## v1.31.0 -- CODE_REVIEW.md wave 3, timbre (2026-09-24)

- Resolved CR-01 (LF alpha false root → pressed half re-rendered), WR-01 (Fant Rg + Ta clamp), WR-11 (MPE Rd bias centred, note-on seeds pressure/timbre), WR-15 (shimmer +12 st), WR-17 (hybrid vowel gains + Singer's boost, exact cascade peak gain, 0.45·sr clamp). MINOR — timbre change, no param/state change. auval PASS, pluginval 10 PASS (VST3). All CR-*/WR-* in CODE_REVIEW.md now resolved; IN-01..04, IN-06..24 remain (/improve-review-info).
- Known: pressed presets are +11..+19 dB louder than v1.30.1 (they were near-silent from CR-01). Listening pass on the 16 factory presets owed.

## v1.30.0 -- CODE_REVIEW.md wave 2 (2026-09-24)

- Resolved CR-03, CR-06, WR-03, WR-04, WR-06, WR-07, WR-08, WR-09, WR-14, WR-16, WR-18, WR-19, WR-20 (IN-13 as a side effect). MINOR — additive state (`lyricsEngine.syllables`, `tuningEngine.kbm`); 4 tuning_* params non-automatable (IDs unchanged). auval PASS, pluginval 10 PASS (VST3), check-i18n / fr-lint / zh-lint (O-Formant) / check-ui-labels / ui_tip_render_check PASS.
- Deferred: v1.31.0 timbre wave (CR-01 + WR-01, WR-11, WR-15, WR-17) — needs a listen pass on all 16 presets. IN tier via /improve-review-info.
- Needs hands-on: DAW listen for glides (Transition knob), steal declick at 17+ voices, bend range, preset switching, KBM load/clear/reopen, offline bounce of a lyrics session with the editor closed.

## v1.29.1 -- CODE_REVIEW.md wave 1 (2026-09-24)

- Resolved CR-02, CR-04, CR-05, CR-07, CR-08, CR-09, WR-02, WR-05 (module note-expression 1.1.1 → 1.1.2), WR-10, WR-12, WR-13, IN-05. PATCH — no param/state-format change. Details in CHANGELOG.md.
- **Known limitations (deferred):** wave 2 → v1.30.0: CR-03, CR-06, WR-03, WR-04, WR-06, WR-09, WR-14, WR-16, WR-18. Wave 3 → v1.31.0 (timbre re-render + preset listen pass): CR-01+WR-01, WR-11, WR-15, WR-17. Unscheduled: WR-07, WR-08, WR-19, WR-20, remaining IN-*.
- Other note-expression consumers (O-Bassoon, O-Bells, O-Bowed, O-Contrabass, O-IntonationPad, O-Lyrica, O-MicrotonalSampler, O-Prism, O-Reed, O-Strata) pick up WR-05 on their next rebuild.

## v1.25.0 -- Phase 24 propagation (2026-04-26)

- **Phase 24 wave 7 of 7 (final per-plugin propagation).** O-Formant adopts the shared `note-expression` module (modules/tuning/note-expression v1.0.0) for VST3 Note Expression microtonal playback in Dorico.
- **CMake delta**: O-Formant was the only Phase 24 plugin missing `include(OuariconModules.cmake)` — added at line 3 (immediately after `cmake_minimum_required` and BEFORE `juce_add_plugin`). Module call `ouaricon_add_module(O-Formant note-expression)` added after the `target_sources` block.
- **Per-call-site MPE composition** (different shape from O-Reed/O-Bowed helper-based pattern): NE applied at the single `tunedF0` assignment site in `FormantVoice::noteStarted()` immediately after `tuningEnginePtr->getFrequency(midi)` and BEFORE `pitchGlide.snapTo/setTarget(f0)`. Cast through `double` at helper boundary (`tunedF0` is `float`). `f0` re-read after NE composition (was a local copy of `tunedF0` before the NE step).
- **MPE pitch source for NE correlation**: `int midiNote = currentlyPlayingNote.initialNote`. Shared module's `updatePendingFromEvents` correlates by `noteId` regardless of MPE channel.
- **Composition correctness**: glottal source `LFGlottalSource` samples the correct fundamental from sample 0 (Pattern 2 — no attack zipper). Downstream `tunedF0` consumers in `renderNextBlock` (spectral tilt + source-filter coupling) all see the tuned value because NE updates `tunedF0` BEFORE `pitchGlide` consumption. ConsonantEngine articulation independent of pitched fundamental — remains intelligible at microtonal shifts.
- **Version bump**: 1.24.2 → 1.25.0 (MINOR — new user-visible feature, backward compatible, no preset impact).
- **Files modified**: 8 (CMakeLists.txt, PluginProcessor.{h,cpp}, FormantVoice.{h,cpp}, CHANGELOG.md, .planning/STATUS.md, modules/registry.yaml).
- **Build**: tri-format ninja exit 0; AU validates via `verify-au-link.sh O-Formant`; freshly installed per CLAUDE.md (cache cleared, dev-suffix + prod-named bundles).
- **Dorico 3-point smoke gate**: DEFERRED — batch validation pending (per user direction at orchestrator level for Phase 24 — gates for plans 24-02..24-07 are batch-validated together at end-of-phase rather than gating each plan inline).

## Completed So Far

- **Ideation:** Complete
  - Core concept defined (physical-model vocal synth, source-filter model)
  - 21 parameters specified with ranges and defaults
  - UI vision captured (2D XY vowel morph pad with formant peaks overlay)
  - Use cases identified (film/game, electronic, ambient, education)
  - Requirements extracted with acceptance criteria (26 requirements)
  - 6 research documents referenced

- **Stage 0:** Complete
  - Plugin type defined: Synth (MIDI Instrument)
  - Professional examples researched: 5 (Pink Trombone, VocalSynth 2, Cantor Digitalis, Plaits, Humanoid)
  - JUCE modules identified: juce_audio_basics, juce_audio_processors, juce_dsp, juce_core, juce_gui_basics, juce_gui_extra
  - DSP feasibility verified (all components implementable with JUCE 8 + custom code)
  - Parameter ranges researched and validated
  - Complexity score: 5.0 (raw 8.2)
  - Strategy: Phase-based implementation (3 DSP phases, 3 GUI phases)
  - ARCHITECTURE.md documented with 11 sections
  - ROADMAP.md documented with phased breakdown
  - CONTEXT.md captured key decisions and constraints

- **Stage 1:** Foundation complete - Build system operational, 21 parameters implemented
  - CMakeLists.txt: IS_SYNTH TRUE, NEEDS_MIDI_INPUT TRUE, juce_dsp linked
  - PluginProcessor: MPESynthesiser with 16 FormantVoice instances, enableLegacyMode()
  - FormantVoice: MPESynthesiserVoice skeleton with 21 cached parameter pointers, silent output
  - APVTS: All 21 parameters (20 Float + 1 Bool) with correct ranges, defaults, skew factors
  - State management: getStateInformation/setStateInformation via APVTS XML
  - GenericAudioProcessorEditor for Stage 1 UI
  - Output-only stereo bus (instrument, no input bus)

- **Stage 2 Phase 2.1:** Core Vocal Engine -- 9 DSP files created, 5 files modified
  - GlottalWavetable: Flat vector storage (128 Rd x 2048 samples x 10 mipmap levels, ~10MB shared)
  - GlottalTableGenerator: Fant 1995 regression, Newton-Raphson solvers, FFT mipmap generation
  - LFGlottalSource: Per-voice wavetable oscillator with bilinear interpolation (Rd + mipmap level)
  - AspirationNoise: Single-pole IIR LP at 4kHz, SmoothedValue breathiness, per-voice Random
  - VowelData: Csound bass voice formant tables (5 vowels, F1-F5, BW, gains as linear)
  - FormantBiquad: 32-byte DF2T biquad struct with inline processSample
  - FormantFilterBank: 5 parallel BPFs with shift/spread, makeBandPass coefficients
  - VowelMorpher: Shepard IDW interpolation, log-domain frequency blending
  - FormantVoice: Full per-sample loop with block-rate coefficient updates, ADSR envelope
  - PluginProcessor: Wavetable generation at construction, voice preparation in prepareToPlay
  - Architecture deviation: Mipmapped wavetable (not direct LF + PolyBLEP) per user decision

- **Stage 2 Phase 2.2:** Modulation & Expression -- 3 DSP files created, 2 files modified
  - VibratoLFO: Sine LFO with onset delay ramp, micro-jitter (+/-0.5% per cycle)
  - PitchGlide: One-pole exponential smoother for portamento (from O-Prism pattern)
  - ConsonantEngine: KLATT parallel noise (LP/HP crossfade + sibilance BP + plosive burst)
  - FormantVoice: Per-sample F0 chain (glide->vibrato->jitter), consonant mix, MPE expression
  - MPE integration: pressure->breathiness (additive), timbre->vowelY (offset), velocity->burst
  - 19 of 21 parameters now connected

- **Stage 3 Phase 3.1:** Layout + Controls + Binding -- WebView operational, 21 parameter controls bound
  - CMakeLists.txt: NEEDS_WEB_BROWSER TRUE, NEEDS_WEBVIEW2 TRUE, binary data target
  - PluginEditor: 20 WebSliderRelays + 1 WebToggleButtonRelay + WebBrowserComponent
  - index.html: Ouaricon Naturalist aesthetic, XY vowel morph pad, 18 seed knobs, 1 toggle
  - main.js: XY pad drag/automation, knob drag/automation, toggle binding
  - Resource provider: bare path matching (5 resources)
  - Build: VST3 + AU compiled, installed to system folders

- **Stage 3 Phase 3.2:** Visual Polish -- Formant overlay, cursor glow, ADSR visualization
  - Cursor glow: 28px radial moss-green gradient behind XY pad cursor
  - Formant dots: F1-F5 labeled dots in lower XY pad, Shepard IDW + shift/spread
  - ADSR canvas: DPR-aware, linear segments, reactive to 4 ADSR params
  - 7 new relay listeners (formantShift, formantSpread, vowelFocus, attack, decay, sustain, release)
  - No C++ changes, JS-only additions
  - Build: VST3 + AU compiled, auval PASS, pluginval level 5 PASS

- **Stage 4 Phase 4.1:** DSP Completion + Presets -- outputGain, stereoWidth, 16 factory presets, preset browser UI
  - outputGain: SmoothedValue 50ms, dB-to-linear, post-synth in processBlock
  - stereoWidth: Per-voice equal-power pan by MIDI note in FormantVoice
  - OuariconPresetManager integrated with 16 factory presets (4 categories)
  - Preset browser WebView UI (prev/next, category dropdown, save)
  - 10 native functions for preset communication
  - pluginval level 5 PASSED
  - State persistence: preset name survives DAW save/load

- **Stage 4 Phase 4.2:** Validation + Release -- pluginval level 10, CHANGELOG.md
  - pluginval level 10 PASSED on VST3 (seed: 0x59b5378) -- all tests including parameter thread safety, state restoration, non-releasing SR switch
  - pluginval level 10 PASSED on AU (seed: 0x5a23e58) -- all tests passed
  - auval PASSED (aumu OuFm OuDv)
  - CHANGELOG.md created (v1.0.0, Keep a Changelog format)
  - VST3 + AU installed to system folders
  - No code fixes needed -- level 10 passed clean on first run

## Next Steps

1. Install plugin to system folders (`/install-plugin O-Formant`)

## Context to Preserve

**Architecture files:**
- `plugins/O-Formant/.planning/research/ARCHITECTURE.md` -- DSP specification (immutable contract)
- `plugins/O-Formant/.planning/ROADMAP.md` -- Implementation plan with phases
- `plugins/O-Formant/.planning/stages/2-dsp/PLAN.md` -- Phase 2.1 task breakdown
- `plugins/O-Formant/.planning/stages/2-dsp/PLAN-2.2.md` -- Phase 2.2 task breakdown
- `plugins/O-Formant/.planning/stages/2-dsp/RESEARCH-2.2.md` -- Phase 2.2 research

**Key Decisions:**
- Voice framework: juce::MPESynthesiser + MPESynthesiserVoice with enableLegacyMode()
- Formant topology: Parallel (not cascade) for v1
- Glottal source: Mipmapped wavetable (deviation from ARCHITECTURE.md, user chose this)
- Custom biquad structs for formant filters (not juce::dsp::IIR::Filter)
- IS_SYNTH TRUE + NEEDS_MIDI_INPUT TRUE in CMakeLists.txt
- Output-only bus (no audio input)
- Per-voice pitch glide (deviation from architecture's monophonic last-note glide)
- MPE pressure additive above knob baseline (not multiplicative)

**Files Created (Phase 2.2):**
- plugins/O-Formant/Source/dsp/VibratoLFO.h
- plugins/O-Formant/Source/dsp/PitchGlide.h
- plugins/O-Formant/Source/dsp/ConsonantEngine.h

**Files Modified (Phase 2.2):**
- plugins/O-Formant/Source/FormantVoice.h
- plugins/O-Formant/Source/FormantVoice.cpp
