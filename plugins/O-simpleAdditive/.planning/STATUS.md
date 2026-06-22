---
plugin: O-simpleAdditive
stage: 3
status: in_progress
last_updated: 2026-06-22
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
stage_2_mode: incremental_dsp_builds
next_action: begin_stage_4_validation
next_stage: 4
ready_for_implementation: true
contract_checksums:
  brief: sha256:2270ef614ef3fbac3778fc7157e8c2ce62d2d04a8dcc0b1348b07278b2803435
  parameter_spec: sha256:1a05abe8a73cb8fc9678b3d3eb570fb79aae09f28d17b8efb473c1056f98c26c
  architecture: sha256:76bcd9f438a6a694a7ad9a8c686b63f3eb8e6da69ded3b9639dc094e3a429a00
  roadmap: sha256:b8377a47e3fe71191749afaad537f7c72ed183d23c7f233117c55b4f5303288c
---

# O-simpleAdditive Status

## Current Position

Stage: 3 of 4 (GUI) — ✓ COMPLETE. Next: Stage 4 (Validation / Polish).
Status: WebView "Additive Field Guide" UI built, installed, **auval SUCCEEDED**, Standalone render verified. All 33 params two-way bound; 16 drawbars double as the live spectrum; oscilloscope, tooltips, 6 lesson presets, on-screen keyboard all wired.
Progress: [##################..] 90%

## Stage 3 (GUI) — ✓ COMPLETE (single coherent pass, build directly + show running plugin)

| Phase | Status | Artifact / Gate |
|-------|--------|-----------------|
| 3.1 Layout + drawbars + controls + cross-platform wiring | ✓ | stages/3-gui/SUMMARY.md — build clean (VST3+AU) |
| 3.2 Live drawbar-spectrum + oscilloscope | ✓ | drawbarSpectrumUpdate (active-spectrum snapshot) + scopeUpdate |
| 3.3 Tooltips + lesson preset tour | ✓ | tooltips on every control; 6 C++ snapshot lessons via applyFactoryPreset |

**Stage 3 result:** Field Guide WebView UI (sibling of O-simpleFM). 31 `WebSliderRelay` (16 drawbars + 15 knobs) + 2 `WebComboBoxRelay` (frameBSource, bitDepth), two-way bound. Drawbars = brass set-level + green live-glow (morphed+decayed active-spectrum snapshot; QUAL-02). 30 Hz Timer drives scope (`AdditiveVizAnalyzer`) + drawbar spectrum. Processor gained on-screen-keyboard MIDI (`MidiMessageCollector` + `handleUiMidi` + drain), `getCurrentSampleRate()`, `isSounding()`. CMake `juce_add_binary_data`. VST3+AU clean; **auval SUCCEEDED** (incl. MIDI). Default patch unchanged (pure sine) — no DSP regression. **Deferred to Stage 4:** persistent OuariconPresetManager save/load bar.

## Stage 2 (DSP) Phase Progress — incremental builds (build + auval per sub-phase) — ✓ COMPLETE

| Phase | Status | Artifact / Gate |
|-------|--------|-----------------|
| 2.1 Core additive voice | ✓ | stages/2-dsp/SUMMARY-2.1.md — build clean + **auval SUCCEEDED** |
| 2.2 Scan/morph + mod-env + LFO | ✓ | stages/2-dsp/SUMMARY-2.2.md — build clean + **auval SUCCEEDED** |
| 2.3 Spectral-decay + bit-depth + viz tap | ✓ | stages/2-dsp/SUMMARY-2.3.md — build clean + **auval SUCCEEDED** |

**Phase 2.3 result:** Spectral-decay macro (per-partial `D_k = exp(−rate·k·tau)`, control-rate `tau` ramp 0→1 over the note, `kDecayRateMax=0.35`), read-time bit-depth quantizer (`{Off,12,10,8,6,4,2}`, mid-tread), and the lock-free viz tap — `VizRing` (lifted from O-simpleFM `FmVizAnalyzer.h` → new `AdditiveVizAnalyzer.h`) fed by the post-gain mono sum + a 16-element atomic active-spectrum snapshot (primary = newest sounding voice by `noteAge`). VST3 + AU clean; `auval` SUCCEEDED incl. MIDI test. Default patch (spectralDecay=0, bitDepth=Off) bit-identical to 2.2 — no regression. Fixed an accidental `isVoiceActive` override of a JUCE virtual (renamed `isAmpActive`). `getVizRing()` + `readActiveSpectrum()` exposed for the Stage-3 editor.

**Phase 2.1 result:** `AdditiveVoice.h` (new) — 16-partial band-limited single-cycle table (2048-pt), per-note `Kmax` band-limit + raised-cosine taper, headroom-normalized sum, amp ADSR + velocity, control-rate dirty-refill. Synthesiser wired (16 voices). VST3 + AU clean; `auval` SUCCEEDED incl. MIDI test at 11k–192k Hz.

**Phase 2.2 result:** Wavetable dimension. Frame B presets (Sine/Saw/Square/Odd) + per-partial spectral morph `lerp(A_k, B_k, scan)` in `refillTable`. Per-voice mod-env (2nd ADSR) + global sine scan LFO; scan = `clamp(scanPosition + lfo·depth + modEnv·envAmount)` → 20 ms smoother → control-rate refill (refill only when scan moves > 1e-4). VST3 + AU clean; `auval` SUCCEEDED incl. MIDI test. Default patch (scan=0) bit-identical to 2.1 — no regression. `spectralDecay`/`bitDepth`/`velToDecay`/viz APVTS params present but not yet consumed (2.3).

## Stage 1 Phase Progress (express mode) — ✓ complete

| Phase | Status | Artifact |
|-------|--------|----------|
| discuss | ✓ | stages/1-foundation/CONTEXT.md |
| research | ✓ | stages/1-foundation/RESEARCH.md |
| plan | ✓ | stages/1-foundation/PLAN.md |
| execute | ✓ | stages/1-foundation/SUMMARY.md (foundation-shell-agent) |
| verify | ✓ | stages/1-foundation/VERIFICATION.md — **PASS** |

**Stage 1 result:** VST3 + AU build clean; `auval` SUCCEEDED (aumu/OSiA/OuDv); **33 Global Scope Parameters**; zero latency (`setLatencySamples(0)`); state persistence wired; `GenericAudioProcessorEditor` placeholder. Port of O-simpleFM minus oversampling/voices/viz/preset.

## Completed So Far

**Ideation:** ✓ Complete
- Core concept (pedagogical 16-partial additive + wavetable scan/morph; sibling to O-simpleFM)
- Requirements extracted (22) with acceptance criteria
- Parameter draft extracted

**Stage 0 (Research & Planning):** ✓ Complete
- Plugin type confirmed: Synth (Additive + light Wavetable), 16-voice poly, WebView UI
- Professional examples researched: Hammond drawbars, PPG/Waldorf wavetable, Harmor/Razor/Alchemy
- JUCE 8 modules identified & verified against local source (8.0.9): `Synthesiser`, `SynthesiserVoice`, `ADSR`, `dsp::LookupTableTransform`, `dsp::FFT`, `WindowingFunction`, `SmoothedValue`, `AudioParameterChoice` — confirmed NO native additive/wavetable class (engine is custom)
- All 9 open research questions resolved (render strategy, anti-aliasing, morph, Frame B editability, spectral-decay curve, bit-depth typing, polyphony, velocity routing, viz handoff)
- Complexity score: 5.0 (raw 12.0) → staged implementation
- ARCHITECTURE.md and ROADMAP.md documented; decisions folded into parameter-spec-draft.md

## Next Steps

1. **Stage 3: GUI (3 phases)** — Phase 3.1 layout + 16 drawbars + controls + cross-platform WebView wiring; Phase 3.2 live drawbar spectrum (from `readActiveSpectrum`) + oscilloscope (from `getVizRing` → `AdditiveVizAnalyzer`); Phase 3.3 tooltips + preset tour. → `/clear` then `/implement O-simpleAdditive`
2. Stage 4: Validation/Polish (pluginval, preset sweep, aliasing audit, changelog)
3. Pause here

## Context to Preserve

**Key decisions (Stage 0):**
- Render: precompute band-limited single-cycle wavetable per note (2048 pts), read by phase — NOT per-sample sum-of-sines
- Anti-aliasing: exact band-limit (omit partials k>Kmax=floor(0.5·fs/f0)) + boundary taper — NO oversampling, zero latency
- Morph: linear *spectral* (per-partial lerp), zipper-free
- Frame B: preset-only 4-way choice (sine/saw/square/odd) — NOT a second drawbar set (stays 33 params)
- Spectral-decay: `D_k = exp(−rate·k·tau)` per-partial exponential tilt over the note
- Bit depth: discrete choice {off,12,10,8,6,4,2}, "off" = clean
- Polyphony: 16 voices (matches O-simpleFM)
- Velocity: amp always-on; `velToDecay` opt-in
- Viz: reuse O-simpleFM `VizRing` + `FmVizAnalyzer`; bars from active-spectrum snapshot
- LFO sine-only/global; mod-env→scan only (decay routing deferred to v1.1)
- **Lower-risk than O-simpleFM** (no feedback loop, no oversampling); most infrastructure inherited from O-simpleFM

**Reference model:** O-simpleFM (`plugins/O-simpleFM`) — voice model, dual ADSR, `fastSine`, `VizRing`/`FmVizAnalyzer`, WebView CMake. Reuse near-verbatim.

**Strategy:** Staged (3 DSP phases + 3 GUI phases).

## Files Created / Updated
- plugins/O-simpleAdditive/.planning/research/ARCHITECTURE.md (new)
- plugins/O-simpleAdditive/.planning/ROADMAP.md (new)
- plugins/O-simpleAdditive/.planning/stages/0-ideation/CONTEXT.md (new)
- plugins/O-simpleAdditive/.planning/parameter-spec-draft.md (Resolved Decisions folded in)
- plugins/O-simpleAdditive/.planning/STATUS.md (updated)
- PLUGINS.md (registry row → 🚧 Stage 0)
