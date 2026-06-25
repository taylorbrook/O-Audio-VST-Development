# Stage 2 (DSP) — VERIFICATION

**Plugin:** O-simpleSubtractive · **Stage:** 2 of 4 (DSP) · **Date:** 2026-06-25
**Verdict:** ✅ **PASS** — every Stage-2 success criterion is met with objective evidence.

## Goal-backward: success criteria → evidence

| # | Stage-2 criterion | Evidence | Verdict |
|---|-------------------|----------|---------|
| 1 | Loads as an instrument, MIDI routes, 16-voice poly, no crash | auval (Test MIDI PASS), pluginval s10 SUCCESS, harness `poly-both` (A & B both sound) | ✅ |
| 2 | 4 filter modes × 3 slopes audibly work; LP default sweeps cleanly | harness `mode-LP/HP/BP/Notch`, `slopes-steepen` (s6>s12>s24) | ✅ |
| 3 | Filter ADSR independent of amp; bipolar env opens/closes; keyTrack | harness `filter-env-bipolar` (hiPos≫hiNeg), `key-track` (cutoff C2 125 Hz → C6 2000 Hz), `legato-vs-mono` (amp env independent) | ✅ |
| 4 | Max resonance + no input → clean bounded sine at cutoff; in tune | harness `self-osc` (bounded peak 1.04, sustained), `self-osc-in-tune` (×2, ratio 2.016) | ✅ |
| 5 | Closed-form magnitude curve == measured filter (QUAL-02) | harness `curve-vs-measured` **maxErr 0.00 dB** across 4 octaves | ✅ |
| 6 | No high-key aliasing (QUAL-01, DSP-06) | harness `aa-highpitch` (alias/harm ratio 0.0007 at C7 saw) | ✅ |
| 7 | Poly/Mono/Legato + glide, no stuck notes | harness `poly-both`, `mono-last-note`, `legato-vs-mono`, `glide`; mode-switch hard-reset; pluginval fuzz clean | ✅ |
| 8 | Allocation-free processBlock; latency 0 | `setLatencySamples(0)`; pluginval allocation checks clean; MonoStack + viz tap are fixed/stack memory | ✅ |

## Technical validation
- **Render-harness:** 18/18 PASS (`O-simpleSubtractive-render-test`, offline, no DAW).
- **AU:** `auval -v aumu OSiS OuDv` → **AU VALIDATION SUCCEEDED** (20 params; MIDI; parameter ramping).
- **VST3:** `pluginval --strictness-level 10 --validate-in-process` → **SUCCESS**; grep of full log for fail/warning/error/alloc/leak/exception = none.
- **Build:** VST3 + AU + Standalone + render-test compile clean (sole warning: unused `processorRef` in the Stage-1 GenericAudioProcessorEditor placeholder — removed when Stage 3 swaps in the WebView).

## Architecture-contract adherence (ARCHITECTURE.md)
- Cytomic ZDF SVF core implemented verbatim (`Ω`, `g`, `k`, `a1/a2/a3`); Notch = src − k·BP; 1-pole 6 dB; cascade ×2 for 24 dB with resonance on stage 1. ✅
- PolyBLEP/polyBLAMP AA; no oversampling; zero latency. ✅
- Bipolar filter env in octaves (ENV_OCT=7); keyTrack 2^(keyTrack·(note−60)/12); fcEff clamped to [20, min(20k, 0.45·fs)]. ✅
- Dual independent `juce::ADSR`; voice lifetime keyed on amp env only. ✅
- Lock-free `VizRing` (copy-only audio thread); FFT/curve deferred to message thread. ✅
- 20-param APVTS unchanged from Stage 1 (contract frozen). ✅

## Focused critic review (DSP + RT-safety + architecture)
- **RT-safety:** `processBlock` has no allocation/lock/IO; `MonoStack` is a fixed 32-slot array; the stereo→mono viz buffer is a 4096-float stack chunk; param push is atomic loads. Confirmed by pluginval's in-process allocation checks. **No blockers.**
- **Mono/Legato path:** drives voice 0 directly in sample-accurate MIDI slices; mode switches call `allNotesOff` + clear the stack (no stuck/frozen voices); base-class voice bookkeeping is bypassed but lead-voice/lifetime logic uses `ampEnv.isActive()`, so nothing relies on stale base state. **No blockers.**
- **Self-oscillation:** soft-knee limiter is identity in the linear range (curve stays exact); negative-k bias is localized to res⁸; states NaN-scrubbed; `ScopedNoDenormals` guards decay tails. Limit cycle bounded (peak ≤ ~1.05 observed). **No blockers.**
- **Note (carry to Stage 4 polish, not a blocker):** Mono re-strike zeroes the envelope instantly, so a very fast amp attack can click on retrigger — expected "re-pluck" behavior; revisit only if a preset exposes it. Triangle/sub aliasing is unverified by probe (saw is the worst case and passes; triangle is 1/n²).

## Outstanding / risks for Stage 3
- The headline filter curve + spectrum + scope + dual-ADSR display exist as data (`SubVizAnalyzer`, display atomics) but are **not drawn** — that is Stage 3 (WebView). The audio-side contract they depend on (lead-voice cutoff/k atomics, env atomics, VizRing) is in place and validated.
- BinaryData namespace collision risk re-enters at Stage 3 when the UI-resources binary-data target is added (see project memory) — N/A now (no binary-data target yet).
