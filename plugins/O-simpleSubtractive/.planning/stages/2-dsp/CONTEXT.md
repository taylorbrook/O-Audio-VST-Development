# Stage 2 (DSP) — CONTEXT

**Plugin:** O-simpleSubtractive
**Stage:** 2 of 4 (DSP)
**Source:** Auto-generated from existing contracts (BRIEF.md, parameter-spec, ARCHITECTURE.md, Stage 1 VERIFICATION.md) — express mode, no interactive session.
**Date:** 2026-06-25

## Goal

Turn the silent 20-parameter Foundation shell into an audible, polyphonic, MIDI-playable
subtractive synthesizer: **oscillator bank → multimode ZDF state-variable filter → VCA**,
shaped by **two independent ADSRs** (filter env → cutoff, amp env → VCA + voice lifetime),
with **Poly / Mono / Legato + glide** voicing and a **real-time-safe visualization tap**.

This is the highest-risk stage. The genuinely-new work (vs the O-simpleFM sibling) is the
**self-oscillating multimode SVF + its exact closed-form magnitude curve** and the **voice
modes**. Everything else (Synthesiser/Voice skeleton, dual ADSR, VizRing, render-harness) is
a near-verbatim sibling port.

## Phasing (per ROADMAP)

The DSP stage is built in three de-risking phases:

- **2.1 — Source + LINEAR filter + dual ADSR + VCA.** Audible subtractive core; self-osc deferred.
- **2.2 — Self-oscillation + gain compensation + magnitude-curve validation.** The "class whistle" + QUAL-02 foundation.
- **2.3 — Voice modes + glide + visualization tap.** Poly/Mono/Legato + portamento + audio-thread viz ring.

Build the **linear** filter + all 4 modes + 3 slopes + curve match FIRST, then add the tanh
self-oscillation — the architecture's mandated de-risking order (Risk #1).

## Inputs (immutable contracts)

- `ARCHITECTURE.md` — full DSP spec: Cytomic ZDF SVF core, PolyBLEP AA, dual-ADSR routing,
  bipolar filter-env in octaves, key-track, voice/glide manager, viz data path, closed-form
  magnitude derivation, preset recipes. **This is the load-bearing spec; Stage 2 implements it exactly.**
- `parameter-spec` / Stage 1 APVTS — 20 params, IDs/ranges/defaults already built and validated
  (auval: "20 Global Scope Parameters"). Stage 2 reads them, does NOT change the layout.
- Stage 1 `VERIFICATION.md` — foundation PASS (pluginval SUCCESS, AU VALIDATION SUCCEEDED, state round-trips).

## Constraints

- **No oversampling** — PolyBLEP/polyBLAMP band-limit at source (steady phase increment); **zero added latency** (`setLatencySamples(0)`). This is the key simplification vs O-simpleFM (which oversamples 2×).
- **Allocation-free audio thread** (PERF-01): no alloc / FFT / locks in `processBlock`; FFT stays on the message thread (Stage 3). Audio thread is copy-only into `VizRing`.
- **Block-push param pattern** (O-Bassoon/O-simpleFM): processor reads APVTS atomics once/block → `voice->setParams(...)`; voices never read APVTS directly.
- **`ScopedNoDenormals`** + block-level `std::isfinite` scrub — resonant filters + decaying envelopes stall on denormals; self-osc must be tanh-bounded + NaN-scrubbed.
- **Never reset oscillator/filter phase/state mid-note** — reset only on voice (re)allocation.
- APVTS layout, CMake plugin config, and the 20-param contract are FROZEN from Stage 1.

## Success Criteria (Stage 2 exit)

1. Loads in a DAW as an **instrument**, MIDI routes, plays 16-voice polyphonically (no crash).
2. All four `filterType` modes + all three `filterSlope` settings audibly work; LP default sweeps cleanly.
3. Filter ADSR sweeps cutoff independently of amp ADSR; bipolar `filterEnvAmount` opens (+) / closes (−); `keyTrack` raises cutoff with pitch.
4. Max resonance + no input → **clean sustained sine at cutoff** (no blow-up, no DC); `keyTrack=100%` → in tune.
5. Closed-form magnitude curve matches measured swept-sine response within tolerance (all modes/slopes) — render-harness.
6. **No aliasing/buzz at high notes** (saw/square) — render-harness aliasing probe (QUAL-01, DSP-06).
7. Poly = 16 independent voices; Mono = last-note priority; Legato = slur without retrigger; glide ramps pitch; `glide=0` instant; no stuck notes.
8. `processBlock` allocation-free; latency reported 0; pluginval + auval pass.

## Out of Scope (deferred)

- WebView UI / parameter binding / spectrum-scope-curve drawing → **Stage 3 (GUI)**.
- Factory presets / preset-manager module / preset tour → **Stage 4 (Polish)**.
- v1.1-deferred params (`velToFilterEnv`/`velToAmp` as params, PWM, master tune/octave, 2nd osc, LFOs).
