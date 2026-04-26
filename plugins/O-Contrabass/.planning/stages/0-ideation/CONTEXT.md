# Stage 0 — Research & Planning Context

**Stage:** 0 (Ideation / Research & Planning)
**Status:** Complete
**Date:** 2026-04-25
**Plugin:** O-Contrabass

---

## Phase Findings (Discuss)

This document captures the synthesis of Stage 0's discuss phase: what the plugin is, why these decisions were made, and what constraints carry forward into implementation.

### What O-Contrabass Is

A **bass-only physical-modeling bowed contrabass** (E1–G3, monophonic, 4-string EADG) purpose-built for two complementary territories on a single engine:

1. **Cinematic orchestral arco** — Spitfire Albion / CSS / VSL territory (sustained bowed bass for film, TV, game scoring).
2. **Sustained drone instrument** — Stephen O'Malley / Tony Conrad / Charlemagne Palestine territory (infinite sustain, sub-harmonics, slow-bow LFO, just-intoned drones).

The architecture intentionally specializes: every DSP decision is tuned for the bass register and sustained articulation. This is the inverse design of O-Bowed (general-purpose, configurable, presets across instruments).

### Why "Drone-First, Then Add Damping"

A core engineering insight from research §2: the waveguide is built to *want* to oscillate forever, then losses are added back in. Drone mode is the natural state of the engine; orchestral arco is achieved by *adding* damping. This inverts the typical synth-design framing and informs every component choice:

- Bridge filter `g` defaults to 0.999 (long sustain, brought down from drone-perfect 0.99995).
- Loop gain ceiling is hard-clamped at 0.9999999 (never 1.0) — defense in depth.
- All loop nonlinearities (saturator, friction) are bounded so they CAN'T amplify.

### Why Hyperbolic Friction (Not Elasto-Plastic)

Hyperbolic curve is monotonic, requires no Newton-Raphson, and validates well in O-Bowed production. Bass slow-attack character minimizes the regime where elasto-plastic memory effect matters (mostly violin/viola attack transients). Adding Newton convergence on top of E1+drone stability challenge multiplies risk without proportional sonic gain. Sub-harmonic period-doubling works equally well on hyperbolic. → Defer elasto-plastic to v1.1.

### Why 2x Oversampling (Not 4x)

Bass slow attacks reduce aliasing pressure compared to violin. 8th harmonic of E1 is 328 Hz (well below Nyquist). Polyphase IIR has minimum-phase response (preserves PERF-03 zero-latency). Sub-harmonic period-doubling produces broadband content but stays mostly in the audible band. 4x is the fallback if QUAL-01 reveals aliasing artifacts.

### Why Lagrange3rd (Not Thiran)

Thiran allpass is *stateful* and clicks under continuous detune/vibrato modulation (JUCE doc explicitly warns). Lagrange3rd is stateless FIR — `setDelay()` modulation is glitch-free. Critical for ±1200 cents detune automation and 5 Hz vibrato.

### Why 8-Mode Body Bank (Fixed Wood)

O-Contrabass is bass-only by design; morphable material is O-Bowed's territory. The 8 modes are anchored to published double-bass acoustics (Askenfelt KTH 1982, Rossing 2010): A0 air mode (60 Hz), T1/B1- main wood (98 Hz, also wolf seat), B1+ corpus (115 Hz), 3 cluster modes, bridge cluster (700 Hz), bridge hill (1.2 kHz). Body Size scales frequencies (1.83:1 span); Body Damping scales Q uniformly. Q does NOT scale with size (Bissinger/Hutchins: size-independent in real wood).

### Why Sub-Harmonics via Period-Doubling Bias (Not a Separate Generator)

Real bowed sub-harmonics are an Anomalous Low Frequencies (ALF) phenomenon documented by Hanson, Guettler, and Kawano (2025). The friction junction's stick-slip nonlinearity already produces them when biased into the upper-Schelleng-wedge regime (high `F_bow`, low `v_0`). Adding a separate octave-down generator would be artificial; biasing the existing junction is physically authentic and integrates naturally with everything else (vibrato, body, infinite sustain).

### Why Full Ouaricon Microtonal Convention

- VST3 Note Expression is REQUIRED for Dorico microtonal playback (FUNC-06).
- MTS-ESP is REQUIRED for MTS-ESP-aware hosts (Bitwig, Reaper, Pianoteq, Cubase 13+).
- Scala/TUN is the static/standalone tuning option.
- MPE pitch-bend is the fallback for non-microtonal MPE controllers.
- 12-TET is the always-available baseline.

Spike findings (3 spikes, 2026-04-22 → 2026-04-23) validated the JUCE-NE-PATCH pattern for VST3 Note Expression in Dorico. O-Lyrica is the production reference plugin.

### Why Slow-Bow LFO Must Be Schelleng-Aware

Naïve sub-audio modulation of bow speed/pressure can walk the trajectory outside the playable Schelleng wedge, producing raucous artifacts. Research §3.4 documents the per-block headroom calculation: depth is auto-clamped to 80% of remaining wedge headroom. Combined with 23° pressure phase-lag (mimics natural arm-motion delay), this produces musical "breathing" without ever leaving the playable zone.

---

## Constraints Carried Forward

### Build / Foundation
- `IS_SYNTH TRUE` + `NEEDS_MIDI_INPUT TRUE` in CMakeLists (juce8-critical-patterns #22).
- Output-only `BusesProperties` in constructor (synth pattern, juce8-critical-patterns #4).
- `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (Windows WebView).
- `setLatencySamples()` in `prepareToPlay` — `getLatencySamples()` is NOT virtual in JUCE 8.

### DSP / Audio Thread
- `juce::ScopedNoDenormals` at top of `processBlock` (mandatory).
- No allocations, locks, or file I/O in `processBlock` (PERF-01).
- 2x oversampling via `juce::dsp::Oversampling<float>` (`filterHalfBandPolyphaseIIR`).
- `juce::dsp::DelayLine<float, Lagrange3rd>` for all 4 strings (8192-sample buffer).
- All APVTS reads via atomic `getRawParameterValue()->load()`.
- Loop gain hard-clamped to 0.9999999 (never 1.0).

### Microtonal / MPE
- `JUCE-NE-PATCH` MUST be applied to `~/JUCE/` for Note Expression to work — build fails loudly without it.
- Voice-side tuning drain: `pendingTuningSemis[pitch].exchange(0.0)` BEFORE friction-junction trigger in `noteStarted`.
- Pre-configured `.doricoexpmap` file ships in installer.
- Tuning priority order: Note Expression > MTS-ESP > Scala/TUN > MPE pitch-bend > 12-TET.

### Module Extraction
- `ouaricon_bow_friction` (or registry-conformant name) shared module to be created mid-Phase 2.1.
- O-Bowed regression test required after extraction (~1 day cost).
- O-Contrabass adds `SubHarmonicBias::apply()` API that O-Bowed doesn't initially consume.

### Performance Budget
- Target: <5% CPU on M1 at 44.1 kHz / 256-sample block (PERF-02).
- Projected: ~3.2% — comfortable headroom for v1.1 features (Authentic Arco coupling, FFT body convolution).
- Memory: ~1.5 MB per voice (delay lines + LUTs + filter states).

---

## Research Sources Consulted

This Stage 0 task synthesized from existing deep-research documents (rather than running fresh web searches):

1. **`research/O-Contrabass-research-synthesis.md`** — top-level synthesis, primary reference (12 sections including signal flow, friction tuning, drone features, performance projection).
2. **`research/O-Contrabass-bass-waveguide-stability.md`** — long delay-line stability, dispersion (Rauhala/Välimäki), Lagrange3rd vs Thiran, denormal handling, oversampling, ±1200 cents detune.
3. **`research/O-Contrabass-body-acoustics.md`** — 8-mode bank tuning (Askenfelt KTH 1982, Rossing 2010), Body Size scaling, wolf-tone, bow noise spectral targets.
4. **`research/O-Contrabass-drone-and-subharmonics.md`** — sub-harmonic period-doubling (Hanson, Guettler, Kawano 2025), infinite sustain stability, slow-bow LFO, vibrato (Mick 2025), output protection.
5. **`research/bow-string-friction-models.md`** — friction theory (general reference, sibling plugin shared).
6. **`research/O-Bowed-research-synthesis.md`** — sibling plugin architecture context.
7. **`.claude/skills/spike-findings-VST-development/references/vst3-note-expression-dorico.md`** — Patterns 1–5 for VST3 Note Expression in Dorico (validated 2026-04-23).
8. **`troubleshooting/patterns/juce8-critical-patterns.md`** — 22 patterns checked for relevance to architecture decisions.

---

## Open Decisions (Resolved in ROADMAP)

The Stage 0 deep-research surfaced 6 open decisions. ROADMAP.md §"Open Decisions Resolved" gives detailed recommendations:

| # | Decision | Recommendation | Defer? |
|---|----------|----------------|--------|
| 1 | Friction tier (hyperbolic vs +elasto-plastic) | Hyperbolic only | Defer EP to v1.1 |
| 2 | Module extraction timing | Mid-Phase 2.1 | No — do during Stage 2 |
| 3 | Authentic Arco wolf toggle | Hide in v1.0 | Defer to v1.1 |
| 4 | Sub-harmonic max depth | 1 octave (f0/2) | Cap at 1 octave |
| 5 | Body Size physical span | 1/4 → 4/4 (full) | No — full span |
| 6 | Wood material variants | Single fixed wood | Defer 2-variant to v1.1 |

---

## Files Created During Stage 0

- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` — DSP architecture specification (immutable contract for Stages 1–4).
- `plugins/O-Contrabass/.planning/ROADMAP.md` — Implementation plan with phase breakdown, complexity score (5.0 capped), open-decision recommendations.
- `plugins/O-Contrabass/.planning/stages/0-ideation/CONTEXT.md` — This document.
- `plugins/O-Contrabass/.planning/STATUS.md` — Updated to Stage 0 complete.

---

## Next Up

Stage 1 — Foundation. Run `/implement O-Contrabass` to invoke `foundation-shell-agent` to:
- Create `CMakeLists.txt` with all required flags
- Create `Source/PluginProcessor.{h,cpp}` skeleton with 29 APVTS parameters
- Verify pluginval strictness 10 baseline (silent plugin pass)
- Confirm cross-platform build (macOS VST3+AU, Windows VST3)

**Estimated effort:** ~1 day for Stage 1.
