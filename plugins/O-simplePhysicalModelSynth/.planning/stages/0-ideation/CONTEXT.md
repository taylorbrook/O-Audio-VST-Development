# Stage 0 (Ideation / Research & Planning) — Context

**Plugin:** O-simplePhysicalModelSynth
**Date:** 2026-06-26
**Author:** research-planning-agent
**Outputs:** `research/ARCHITECTURE.md`, `ROADMAP.md`

This file captures the discuss-phase findings, the resolved open questions, and the binding constraints that Stages 1–4 must honor.

---

## What this plugin is

A pedagogical physical-modeling synth — the teaching sibling to O-simpleFM / O-simpleAdditive, and the teaching distillation of the production physical models O-Lyrica (KS), O-Bells (modal), O-Bowed (waveguide/friction). It makes ONE mental model playable: **excitation → resonator → material/damping**. Three exciters (Pluck/Strike/Bow) drive a two-way resonator switch (String = Karplus-Strong; Modal = resonant bank). 16-voice poly, velocity→brightness, WebView live visuals.

## Complexity

- **Tier 4 (synth w/ MIDI), with Tier-6 visualization** → research depth MODERATE-to-DEEP.
- **Complexity score = 5.0** (capped; raw 13.0 = 2.0 params + 8 algorithms + 3 features). Same cap as the other pedagogical siblings.
- **Strategy = staged** (two resonator engines + three exciters + one HIGH-risk friction component).

## Key decisions (binding)

1. **Modal = resonant filter bank, not triggered sinusoids.** A parallel bank of 8 resonant bandpass biquads driven by the exciter signal — so any exciter rings it (FUNC-04 falls out for free). Custom direct-form arrays (O-Bassoon precedent), NOT `juce::dsp::IIR::Filter` at 128-instance scale. This is the central architectural choice that makes the "excitation DRIVES a resonator" pedagogy honest across both engines.
2. **KS string is the v1.0 String engine; Waveguide is `nice`/deferrable.** Karplus-Strong is the canonical algorithm the class derives and the one the headline loop diagram draws. Excitation Position works in v1.0 via a comb on the exciter (O-Lyrica `PluckExciter` pattern); the dual-rail Waveguide (true pickup) is an optional Stage-2 phase reusing O-Lyrica/O-Bowed.
3. **Thiran all-pass fractional delay** for loop tuning (flat magnitude → damping comes only from the explicit Damping LPF). Validated in O-Bowed. Pitch fixed per note → Thiran's no-modulation caveat doesn't apply.
4. **Single global lead-voice viz tap** (most-recently-triggered) — one readable model is the pedagogy. O-simpleFM `VizRing` lock-free reuse; loop-energy scalar + modal-stem array via atomics.
5. **Bow is tiered + single-control.** Memoryless STK bow table (core); elasto-plastic enhancement out of scope; no second Bow control in v1.0.

## Resolved open questions (the 8 "Research Must Confirm" items)

| # | Question | Answer |
|---|----------|--------|
| 1 | Fractional-delay tuning | First-order all-pass via `DelayLine<float,Thiran>` + loop-filter group-delay compensation (O-Lyrica/O-Bowed). |
| 2 | Modal mode count + sets | N=8 modes; `f_k = f0·k·√(1+B·k²)` (B=Inharmonicity); per-mode decay from O-Bells `DECAY_MULTIPLIERS`. Bar≈low-B, bell≈high-B. |
| 3 | Material macro curve | steel↔nylon: log-frequency cutoff lerp (~10 kHz→~2 kHz) + linear feedback lerp (0.995→0.93); macro writes Damping+Decay so both knobs co-move. |
| 4 | Excitation design | Pluck=filtered noise burst; Strike=raised-cosine band-limited impulse (no DC click); Bow=memoryless STK friction (sustains). DC-blocker on sum. |
| 5 | Waveguide vs KS scope | KS = v1.0 must; Position via exciter comb; Waveguide = `nice` Stage-2 phase, deferrable without contract break. |
| 6 | Polyphony | Confirmed 16 voices (`juce::Synthesiser`). |
| 7 | Viz tap | Single global lead-voice tap; lock-free ring + atomics; no per-voice viz. |
| 8 | Bow 2nd control | No — `bowForce` only in v1.0. Bow Speed deferred to v1.1. |

## Highest risk

**Bow friction drive (HIGH).** ~50% of project risk. Mitigation: memoryless-first, validate a basic sustained bowed tone before refining, hard-clamp, epsilon-guard; fallback = sustained filtered-noise drive into the loop. (O-Bowed: friction is always the highest-risk component.)

## Binding repo constraints

- **CMake:** `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (COMPAT-02). Single `juce_add_binary_data` target (WebView UI) → no namespace collision; if embedded presets are ever binary-data, give the 2nd target a DISTINCT `NAMESPACE`.
- **Naming:** `StringVoice`/`ModalVoice`/`PhysicalModelVoice` — never `SamplerVoice`/`SamplerSound` (shadows `juce::`). No bare `end`/`begin` param-ID symbols under `using namespace`.
- **Render-harness (Stage-2 gate):** offline console app, no DAW. KS tuning verified by single-note **autocorrelation** pitch probe (NOT spectral — the loop comb fools spectral probes). Compile under `JUCE_WEB_BROWSER=0`, drop `PluginEditor.cpp` from harness sources, `#if JUCE_WEB_BROWSER` guard `createEditor`. Re-run at start of Stage 4.
- **JUCE 8.0.9**, CMake + Ninja, local JUCE at `/Users/taylorbrook/JUCE`.

## Direct in-house reuse (don't rebuild from scratch)

- O-Lyrica `WaveguideString.h` / `PluckExciter.h` — KS/waveguide loop, `OnePoleLPF`, tuning, group-delay comp, position comb, feedback-from-decay, shadow-filter crossfade.
- O-Bells `BellVoice.h` — modal bank, ratio/decay tables, `StrikeExciter`.
- O-Bowed `WaveguideString.h` / `BowedStringVoice` — dual Thiran rails, memoryless friction junction.
- O-simpleFM `FmVizAnalyzer.h` / `PluginProcessor.h` / `CMakeLists.txt` / `tests/render-harness/main.cpp` — `VizRing`, 16-voice synth structure, ParamIDs namespace, WebView2 flags, harness gate.

## Next step

Stage 1 (Foundation) — `/implement O-simplePhysicalModelSynth` or `/plugin-discuss O-simplePhysicalModelSynth` for Stage 1.
Note: the brief lists a UI mockup as an optional next step; a full `parameter-spec.md` (from mockup finalization) should replace the draft before Stage 1 hardens param ranges, but the architecture + roadmap are complete on the 17-param draft.
