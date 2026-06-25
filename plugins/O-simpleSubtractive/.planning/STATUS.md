---
plugin: O-simpleSubtractive
stage: 4
status: complete
phase: verify
workflow_mode: express
last_updated: 2026-06-25
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: install_plugin
next_stage: done
ready_for_implementation: true
contract_checksums:
  brief: sha256:f9e5eaaafc94bf1f3289806766fe24f609ef2ec9db1ad43cef7295b27694a58c
  parameter_spec: sha256:0ba50c5956d8db980db071f6f9982de71151dd2f84526407e255e91b0f5f8603
  architecture: sha256:90af107beec7bbd0eb0e6bc9db5533d6f0d97951a45dc422ae24254a0c22fea3
  roadmap: sha256:a713331a2b77ca7cf8b2dd274a895d6fa22684966604c145297f6c298f97859e
---

# O-simpleSubtractive Status

## Current Position

Stage: 4 of 4 (Polish) — ✅ complete — **v1.0.0 ready to ship**
Status: All 4 stages complete. Stage 4 filled the 8-preset concept tour (FUNC-06) and confirmed playability (FUNC-07) with ZERO DSP/parameter change. Build clean (VST3+AU), AU VALIDATION SUCCEEDED, pluginval s10 SUCCESS. CHANGELOG v1.0.0 written. Only manual UAT (on-screen look/feel of preset loads) remains before distribution.
Progress: [####################] 100%

## Stage 4 (Polish) — ✅ Complete

| Phase | Status | Artifact |
|-------|--------|----------|
| discuss | ✓ | stages/4-polish/CONTEXT.md (express auto-gen) |
| research | ✓ | stages/4-polish/RESEARCH.md (8-snapshot value derivation + osc-mix constraint) |
| plan | ✓ | stages/4-polish/PLAN.md (T1–T6) |
| execute | ✓ | stages/4-polish/SUMMARY.md |
| verify | ✓ | stages/4-polish/VERIFICATION.md (PASS — auval SUCCEEDED, pluginval s10 SUCCESS) |

- **FUNC-06:** 8 concept presets wired live — Saw Sweep · Pluck · Brass Stab · Sweep Pad · Acid Bass · Square Bass · Noise Wind · Self-Oscillation. `applyFactoryPreset` fills the snapshots via `setValueNotifyingHost` (host API → relays/attachments → UI; no DOM poking, no DSP touched). 3-way key parity (HTML/JS/C++) verified.
- **FUNC-07:** playable as bass/lead/pluck/pad; default state usable; no param-default change.
- **Load-bearing constraint:** main osc can't be silenced (`main + subLevel·sub + noiseLevel·noise`) → Self-Oscillation uses Sine + `cutoff 261.6 Hz` + `keyTrack 1.0` (in-tune whistle); Noise Wind rejects the Sine carrier below a 1.5 kHz band-pass.
- Modified: `Source/PluginProcessor.cpp` (preset bodies), `Source/ui/public/index.html` + `js/app.js` (3 new buttons/captions/tooltips), **new** `CHANGELOG.md`.

## Stage 3 (GUI) — ✅ Complete

| Phase | Status | Artifact |
|-------|--------|----------|
| discuss | ✓ | stages/3-gui/CONTEXT.md |
| research | ✓ | stages/3-gui/RESEARCH.md (sibling-pattern extraction: O-simpleFM editor/JS/CSS + O-simpleGrain combos) |
| plan | ✓ | stages/3-gui/PLAN.md (15 tasks T1–T15 across 3 sub-phases + 9-pt checklist) |
| execute | ✓ | stages/3-gui/SUMMARY.md (gui-agent; VST3+AU build clean) |
| critic | ✓ | UI + Foundation critics — NO BLOCKERS (3 native-fns / 4 events / 20 params parity; DSP-untouched invariant holds) |
| verify | ✓ | stages/3-gui/VERIFICATION.md (PASS — auval SUCCEEDED, pluginval s10 SUCCESS) |

- New: `Source/ui/public/{index.html, css/styles.css, js/app.js, js/juce/index.js, js/juce/check_native_interop.js}`
- Modified: `Source/PluginEditor.{h,cpp}` (relays→WebView→attachments, 30 Hz Timer emitting 4 viz events), `CMakeLists.txt` (+1 `juce_add_binary_data` UI target, default `BinaryData` namespace — single target, no collision), `Source/PluginProcessor.{h,cpp}` (**T6: `applyFactoryPreset` wiring-only stub** — no DSP touched).
- **Native fns:** `uiMidi`→handleUiMidi, `getSampleRate`, `applyFactoryPreset` (stub). **Events:** filterCurveUpdate / spectrumUpdate / scopeUpdate / envUpdate.
- **QUAL-02 by construction:** headline curve = closed-form of the running SVF (`getCurve()`), overlaid on `getSpectrum()` on identical 256-bin log-f axis.
- Verify polish: **N1 fixed** (QWERTY keydown skipped while a `<select>` is focused).
- **Carried to Stage 4:** W1 — preset *content* (8 concept snapshots in `applyFactoryPreset`, FUNC-06); name-parity scaffold already in place.

## Stage 2 (DSP) — ✅ Complete

| Phase | Status | Artifact |
|-------|--------|----------|
| discuss | ✓ | stages/2-dsp/CONTEXT.md |
| research | ✓ | stages/2-dsp/RESEARCH.md |
| plan | ✓ | stages/2-dsp/PLAN.md |
| execute | ✓ | stages/2-dsp/SUMMARY.md (3 phases: 2.1 source+linear filter+ADSR+VCA, 2.2 self-osc+curve, 2.3 voice modes+glide+viz) |
| verify | ✓ | stages/2-dsp/VERIFICATION.md (PASS — 8/8 criteria) |

- New: `Source/OscillatorBank.h`, `SvfZDF.h`, `SubVoice.h`, `SubVizAnalyzer.h`, `tests/render-harness/`
- Modified: `Source/PluginProcessor.{h,cpp}` (Synthesiser + MonoStack + no-oversampling processBlock + viz tap), `CMakeLists.txt`
- **No oversampling** (PolyBLEP) → zero latency. Closed-form filter curve matches the audio filter at **0.00 dB** (QUAL-02 by construction).
- Self-osc: soft-knee limiter + localized negative-k bias → clean bounded sine, in tune at keyTrack=100%.

## Stage 1 (Foundation) — ✅ Complete

20-param silent shell; pluginval SUCCESS + AU VALIDATION SUCCEEDED; state round-trips. (See git + stages/1-foundation/.)

## Next Steps

1. ✅ **Stage 4: Polish — DONE.** 8-preset tour (FUNC-06) + playability (FUNC-07) shipped; auval SUCCEEDED, pluginval s10 SUCCESS, CHANGELOG v1.0.0. **Next: `/install-plugin O-simpleSubtractive`** (or `/show-standalone` for UAT).
2. **Manual UAT before ship:** eyeball the 8 preset loads + live UI (`/show-standalone O-simpleSubtractive` or in a DAW) — controls/visuals snap on preset load; Self-Oscillation whistles in tune; Noise Wind is noise-dominant; headline curve tracks sweeps, scope morphs, dual-ADSR independent, tooltips projector-readable. (Wiring/math/validation are objectively green; only the on-screen look/feel is unverifiable by automation.)
3. The audio→UI contract is in place and validated: lead-voice `displayCutoffHz`/`displayK`/`displayType`/`displaySlope` atomics, `filterEnvValue`/`ampEnvValue` atomics, `VizRing`, `getCurrentSampleRate()`.
4. UI uses ONE `juce_add_binary_data` target (`O-simpleSubtractive_UIResources`, default `BinaryData` namespace). If Stage 4 embeds presets as a 2nd binary-data target, give it a **distinct NAMESPACE** (O-simpleGrain lesson).

## Context to Preserve

**Key Stage-2 decisions (see SUMMARY.md):**
- Self-osc needs a soft-knee limiter (NOT plain per-sample tanh, which damps) + slightly-negative k at the knob top; nonlinearity gated by `k < 0.6` so the magnitude curve stays exact in the normal range.
- Mono retrigger resets envelopes to 0 (JUCE `ADSR::noteOn()` doesn't zero); Legato suppresses retrigger; Mono/Legato drive voice 0 directly (sample-accurate slices), bypassing Synthesiser note-allocation.
- ENV_OCT=7 for the bipolar filter env; resonanceToK = `2·(1−res)^0.6 − 0.18·res^8`.

**Sibling references:** O-simpleFM (primary template — Synthesiser/Voice/VizRing/render-harness), O-Prism (SVF), O-Bassoon, O-simpleAdditive.

**Files created (Stage 2):**
- plugins/O-simpleSubtractive/Source/{OscillatorBank,SvfZDF,SubVoice,SubVizAnalyzer}.h
- plugins/O-simpleSubtractive/tests/render-harness/{main.cpp,CMakeLists.txt}
- plugins/O-simpleSubtractive/.planning/stages/2-dsp/{CONTEXT,RESEARCH,PLAN,SUMMARY,VERIFICATION}.md
