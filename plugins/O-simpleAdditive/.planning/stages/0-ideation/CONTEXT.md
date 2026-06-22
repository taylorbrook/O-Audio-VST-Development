# O-simpleAdditive — Stage 0 Context (Discuss Findings)

**Date:** 2026-06-22
**Stage:** 0 (Ideation / Research & Planning) — complete

This file captures the discuss-phase decisions, constraints, and rationale that shaped `research/ARCHITECTURE.md` and `ROADMAP.md`. It is the narrative companion to those documents and the record of how the 9 open research questions were resolved.

---

## What this plugin is

The **additive sibling of the shipped O-simpleFM** — a deliberately simple **16-partial additive synthesizer with a wavetable scan/morph dimension, built for teaching**. Pedagogy is the north star (gesture → visible/audible consequence within five minutes, no manual). The engine is *always additive*; the wavetable is an added dimension, not a switchable mode. Most of the hard infrastructure (voice model, dual ADSR, lock-free viz, WebView, CMake) is **inherited from O-simpleFM**; the genuinely new work is the additive render + spectral morph + spectral-decay macro.

---

## Resolved research questions (the 9 the param-draft asked Stage 0 to settle)

1. **Render strategy → precompute a band-limited single-cycle wavetable per note, read by phase.** Not per-sample sum-of-16-sines (256 sine evals/sample across 16 voices, and aliases at high keys). When the spectrum changes (note-on, scan, decay, drawbar move), additively fill one 2048-pt single-cycle table from the 16 current amplitudes (partials above Nyquist omitted), then read it back with linear interpolation per sample. This is the CCRMA-standard additive→wavetable optimization, is real-time safe (fixed pre-allocated buffer, bounded control-rate refills), and is *pedagogically faithful*: the table IS the summed-partials waveshape, and morph/decay are applied in the spectrum before the fill, so the bars and scope always match what is heard (QUAL-02).

2. **Anti-aliasing → per-note harmonic-count cap with a boundary taper.** `Kmax = floor(0.5·fs/f0)`; partials `k > Kmax` are simply not written into the table (exact band-limit, zero filter cost). Raised-cosine taper on the top ~2 surviving harmonics so a partial crossing the Nyquist boundary at the top of the keyboard does not click. (DSP-02.)

3. **Morph interpolation → linear *spectral* (per-partial amplitude lerp), NOT waveform crossfade.** `active_k = lerp(A_k, B_k, scan)`. Keeps all partials phase-coherent (no inter-table beating), zipper-free with a smoothed scan, and shows the truthful "the spectrum morphs" picture. (DSP-03.)

4. **Frame B editability → preset-only (sine / saw / square / odd), NOT a second drawbar set.** A B drawbar set would add +16 params (33→49) and a second spectrum surface that fights the single-page, one-readable-spectrum pedagogy. Frame B is a 4-way `AudioParameterChoice`; each preset isolates a concept (pure sine, 1/k saw, odd-only square, hollow odds). **An editable "capture current drawbars as B" button is deferred to v1.1** (no new persistent params).

5. **Spectral-decay macro → per-partial exponential tilt over the note.** `D_k = exp(−rate·k·tau(t))`, `k=0..15` (fundamental never decays), `rate = spectralDecay·RATE_MAX`, `tau` an internal one-shot 0→1 ramp over note length so the spectrum visibly tilts darker over the note. At 0, all `D_k=1` (balance holds). Applied to the morphed spectrum before the table fill → visible in the bars and audible as darkening. (DSP-04.)

6. **Bit depth → discrete `AudioParameterChoice {off, 12, 10, 8, 6, 4, 2}`, "off" = clean sentinel.** Discrete bits make the lesson concrete; "off" = passthrough (clean reference). `q = round(x·L)/L`, `L=2^(bits−1)`, applied at table-read time (so the scope shows the staircase), no dither. (DSP-05.)

7. **Polyphony → 16 voices** (matches O-simpleFM; trivial CPU per voice = one table read + envelope). (FUNC-05.)

8. **Velocity routing → velocity→amp always-on; velocity→spectral-decay opt-in (`velToDecay`, default 0).** Baseline vel→amp is expected; opt-in vel→decay is a small expressive bonus without complicating the default mental model.

9. **Visualization handoff → reuse O-simpleFM `VizRing` + `FmVizAnalyzer` verbatim.** Audio thread copy-only into a lock-free power-of-two atomic overwrite ring; FFT on the editor Timer (30 Hz). Addition: the **drawbar spectrum** display is driven from a lock-free snapshot of the 16 *active* (morphed + decayed) amplitudes — so the bars are exact, and the FFT overlay merely confirms the match. (PERF-01, QUAL-02.)

---

## Parameter changes vs the draft (explicit)

The draft left several types/counts open; Stage 0 resolves them:
- **Frame B:** `frameBSource` is **a 4-way Choice (preset-only)** — NO second 16-drawbar set. (Param count stays 33, not 49.)
- **Bit Depth:** typed as **`AudioParameterChoice {off,12,10,8,6,4,2}`** (was "choice/float off / 2–16"). "off" is the clean sentinel.
- **New param adopted:** `velToDecay` (0–100%, default 0) — velocity → spectral decay, opt-in.
- **LFO:** **sine shape only** in v1.0 (no shape selector param); global (one LFO, all notes morph in phase).
- **Mod-env routing:** **scan only** in v1.0 (mod-env→spectral-decay deferred to v1.1).
- **Final v1.0 core count: 33 params** (16 drawbars + frameBSource + scanPosition + scanLfoRate + scanLfoDepth + scanEnvAmount + spectralDecay + bitDepth + velToDecay + amp ADSR×4 + mod ADSR×4 + outputLevel).

---

## Key architectural notes (and why O-simpleAdditive is *lower-risk* than O-simpleFM)

- **No feedback loop.** Additive is feed-forward — O-simpleFM's single hardest risk (DX7 feedback Nyquist limit-cycle/NaN stability) **does not exist here**.
- **No oversampling.** Band-limiting is exact (omit partials > Kmax), so there is no aliasing to oversample away → lower CPU and **zero added latency** (`setLatencySamples(0)`). O-simpleFM's HIGH anti-aliasing risk is downgraded to MEDIUM.
- **Bit-depth grit is intentional distortion**, not an artifact to suppress — leave it raw (no dither).
- **Display fidelity is the QUAL-02 lever:** the bars must read the *active* (post morph/decay) amplitudes, not the raw drawbar params — hence the active-spectrum snapshot to the UI.
- **Reuse:** `juce::Synthesiser` + custom `AdditiveVoice : SynthesiserVoice`; `fastSine` from O-simpleFM `Operator.h`; `VizRing`+`FmVizAnalyzer` from O-simpleFM; APVTS/relays/CMake from O-simpleFM.

---

## Constraints carried into implementation

- **PERF-01:** zero allocations in `processBlock`; lock-free ring + active-spectrum snapshot feed the viz; table refills at control rate (bounded 16-sine writes).
- **COMPAT-02:** `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` + `withUserDataFolder(tempDir)` on Windows, or the UI blanks (silent IE fallback).
- **QUAL-01:** no clicks/aliasing/zipper across the full keyboard — exact band-limit + boundary taper + smoothed scan + control-rate refill must hold.
- **QUAL-02:** spectrum bars + scope must match the audio — drive bars from the active-spectrum snapshot.
- **JUCE 8:** `SynthesiserVoice` has no virtual `prepareToPlay`; `getLatencySamples()` is non-virtual (use `setLatencySamples(0)`).
- **CMake:** `IS_SYNTH TRUE` + `NEEDS_MIDI_INPUT TRUE` or the plugin is silent in the DAW (critical-pattern #22).

---

## Scope decisions (locked at planning, 2026-06-22)

1. **Frame B → PRESET-ONLY in v1.0** (4-way choice). Editable "capture as B" button → deferred to v1.1.
2. **LFO → SINE-ONLY, global, in v1.0.** Shape selector + per-voice retrigger → deferred to v1.1.
3. **Mod-env → SCAN ONLY in v1.0.** Mod-env→spectral-decay → deferred to v1.1.
4. **`velToDecay` → ADOPTED in v1.0** (opt-in, default 0).
5. Honoring BRIEF/REQUIREMENTS Out-of-Scope: no per-partial envelopes, no >2 frames, no FFT resynthesis from recorded audio, no effects, no non-sine partials.

### Still open (non-blocking — confirm at mockup)
- **Band-limit visibility** — surface "high notes drop upper partials" to the student (itself teachable) or keep silent? Recommendation: optional annotation in the spectrum.
- **Frame A "load 1/k into drawbars" presets** — whether the Sawtooth/Square presets set the drawbars directly (so students see the bars fill) or use Frame B. Recommendation: set the drawbars (more pedagogically vivid).

None of these block Stage 1. The mockup-finalized `parameter-spec.md` remains the eventual authority over the 33-param set.

---

## Status

- ARCHITECTURE.md: complete (`research/ARCHITECTURE.md`) — 11 sections, all 9 research questions resolved.
- ROADMAP.md: complete, complexity 5.0 (raw 12.0), staged (3 DSP + 3 GUI phases).
- Ready for Stage 1 (Foundation).
