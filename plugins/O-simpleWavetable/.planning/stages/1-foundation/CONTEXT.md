# Stage 1 (Foundation) — CONTEXT

**Plugin:** O-simpleWavetable
**Stage:** 1 of 4 — Foundation / Shell
**Date:** 2026-10-05
**Source:** Manual discuss phase. Contracts: BRIEF.md, ARCHITECTURE.md, ROADMAP.md, parameter-spec.md (locked, 21 params), mockups/v1-*. The four open Stage 0 conflicts were put to the user and resolved below.

## Goal

A buildable, host-loadable **silent synth shell** with the structure of O-simpleAdditive: synth CMake (MIDI in, WebView2 flags ready for Stage 3), the complete 21-parameter APVTS, a `juce::Synthesiser` with 16 silent voices, and state save/load with an `IMPORTED_BANK` stub. No audio until Stage 2. No WebView UI until Stage 3: a `GenericAudioProcessorEditor` exposes the parameters for testing.

## Requirements addressed this stage

- **COMPAT-01 (build/validate):** VST3 + AU + Standalone on macOS; targeted `auval -v aumu OSiW <mfr>` passes (not `auval -a`, which SIGABRTs on this machine because of NI AU + TCC); pluginval passes.
- **COMPAT-02 prep:** CMake carries `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, the WebView2 static-link define and `JUCE_USE_CURL=0`, so Stage 3 needs no CMake rework.
- **Parameter contract:** all **21** params in parameter-spec.md order, with exact IDs, ranges, defaults and skews. `jassert (params.size() == 21)`. ROADMAP's "22" is the arithmetic slip that parameter-spec.md documents.
- **MIDI plumbing:** `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, one stereo output bus and no input bus. `MidiMessageCollector` is in place for the Stage 3 on-screen keyboard.
- **State:** APVTS `copyState()` XML round trip, plus `uiLanguage` property and `IMPORTED_BANK` child stub (written only when a bank exists, so Stage 1 writes nothing; the restore path tolerates its absence or presence).

## Resolved decisions (user sign-off 2026-10-05)

| # | Question | Decision |
|---|----------|----------|
| 1 | Bank = Imported with nothing imported | **Silence + UI prompt** (null bank; voices and envelopes still run) |
| 2 | ±2 st pitch bend (not in brief) | **Keep** (O-simpleSubtractive convention; feeds mip-level selection) |
| 3 | Bit quantizer | **Mid-rise**: 3 bits = exactly 8 levels, Full = bit-identical bypass |
| 4 | Imports < 2048 samples | **Reject with a message** (no single-cycle / pitch-synchronous path) |
| 5 | `lfo_div` default | 1/1 (index 2), as in parameter-spec.md (default taken, not asked) |
| 6 | Imported normalization | Per-frame, +24 dB cap (default taken, not asked) |

Decisions 1–6 shape Stage 2 DSP. Stage 1 only declares the parameters that carry them.

## Locked conventions

- `PLUGIN_CODE OSiW` (sibling family: OSiA Additive, OSiF FM, OSiS Subtractive; checked unique). Manufacturer code and company name are the suite's `OUARICON_*` variables. `PRODUCT_NAME "O-simpleWavetable${OUARICON_DEV_SUFFIX}"`.
- Class names: `OSimpleWavetableAudioProcessor` / `OSimpleWavetableAudioProcessorEditor`. The editor name must match the mockup template `v1-PluginEditor.{h,cpp}` so Stage 3 can drop it in.
- `ouaricon_add_module(O-simpleWavetable webview-drop-streaming)` as in O-simpleGrain (it copies the JS at configure time; the copied file is gitignored).
- Link juce_audio_utils, juce_audio_formats, juce_dsp, juce_gui_extra (plus the juce_audio_basics, juce_audio_processors and juce_core they pull in).
- Ranges: plain `NormalisableRange<float>{start, end, interval, skew}` only. **Never a lambda range**: the WebView slider frontend cannot see one. `output_level` text function shows `"-inf"` at ≤ −60 dB. `bit_depth` index 0 text is "Full".
- `setLatencySamples (0)` in `prepareToPlay` (`getLatencySamples` is non-virtual in JUCE 8).
- No param ID shadows a `juce::` free function. Every Choice has ≥ 2 entries.
- Voice/sound classes: `WtVoice` (16, silent `renderNextBlock`) and `WtSound` (applies to all notes and channels). Pitch-wheel plumbing can be stubbed.

## Out of scope for Stage 1

- Any DSP: banks, mips, oscillator, envelopes, LFO, crossfader, importer, reaper (Stage 2).
- WebView UI, relays, attachments, binary data, viz timer (Stage 3). The CMake flags are set, but `juce_add_binary_data` for the UI tree can wait until Stage 3.
- Presets, CPU work, Windows build (Stage 4).

## Constraints

- RT-safe even while silent: `ScopedNoDenormals`, clear the output, no allocation in `processBlock`.
- Follow ARCHITECTURE.md and parameter-spec.md exactly (immutable contracts).
- Build and install via `./scripts/build-and-install.sh O-simpleWavetable`, which does the AU-cache clear and the dev/release variant sweep.
- Commits are path-scoped to `plugins/O-simpleWavetable` and `PLUGINS.md`. The checkout is shared with other in-flight sessions.

## Success criteria

1. `ninja O-simpleWavetable_VST3 O-simpleWavetable_AU O-simpleWavetable_Standalone` builds clean.
2. `auval -v aumu OSiW <mfr>` passes. pluginval passes on the VST3 (and AU).
3. Loads as an **instrument** and accepts MIDI. Silent output is expected.
4. All 21 parameters appear in the generic editor, in spec order, with correct ranges, defaults and text ("Full", "-inf", choice strings).
5. Session save → reload restores parameter values and `uiLanguage`. A state containing an `IMPORTED_BANK` child loads without error.
