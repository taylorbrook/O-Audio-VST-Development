# Stage 0 (Ideation / Research & Planning) — Context & Decisions

**Plugin:** O-simpleGrain — Pedagogical Granular Synthesizer (third sibling after O-simpleFM, O-simpleAdditive)
**Date:** 2026-06-24
**Outputs:** `research/ARCHITECTURE.md`, `ROADMAP.md` (this file records the discuss-phase reasoning behind them)

---

## What this plugin is

A deliberately minimal granular synth built for classroom teaching: **source buffer → windowed grain scheduler → overlap-add → per-voice amp ADSR → out.** MIDI-playable, polyphonic, with a Freeze mode. Every control maps to a hearable + visible granular concept. Four live visualizations + a CPU readout + tooltips + concept-isolating presets, consistent with the two shipped pedagogical siblings.

---

## Key architectural decisions (locked at planning)

1. **Engine = grain pool + scheduler + overlap-add, mined near-verbatim from the shipped O-GrainScatter.** `GrainPool.h` (preallocated `std::array<Grain,N>` + round-robin/steal-oldest `spawnGrain` + `computeEnvelope(phase,shape)` + equal-power pan + overlap-add `processSample`), `GrainScheduler.h` (per-sample countdown free mode), `LagrangeInterpolation.h` (4-pt random-access fractional read), `TripleBuffer.h` (lock-free grain-event handoff), `FreezeManager.h` (playhead pin). This is the single biggest de-risking finding.
2. **Infrastructure = inherited from O-simpleFM / O-simpleAdditive.** `juce::Synthesiser` + custom `SynthesiserVoice`, amp `juce::ADSR`, `setParams` block-push, `VizRing`+`FmVizAnalyzer` lock-free FFT/scope tap, WebView CMake + cross-platform flags, preset-manager module, pedagogical UI conventions (tooltips, concept presets).
3. **Feed-forward, no feedback loop, no spectral STFT in v1.0.** Granular overlap-add removes the hardest stability class (vs O-simpleFM's DX7 feedback). Spectral STFT deferred to a future sibling O-simpleSpectral (one-concept discipline; the title is "Grain").
4. **Global read head (one playhead), not per-grain free-running positions.** The processor owns `playheadPos`; `position` = resting point, `scan` = velocity, `freeze` = pin. Grains read offsets relative to it (± position spray). Keeps the freeze/stretch lesson legible on the waveform display.
5. **Real-time safety = preallocated bounded grain pool + steal-oldest.** No alloc, no locks in `processBlock`. 8 voices × `MaxGrainsPerVoice=24` = 192 global cap. High density × size × poly can never xrun — the cloud thins gracefully. The cap is also the pedagogical point (CPU-cost lesson).
6. **18 APVTS parameters** (17 draft + research-adopted `panSpray`, `velToDensity`). `Load…` is an action/native-function + custom state, not an APVTS param.

---

## Open research questions — RESOLVED (each with a concrete recommendation)

| # | Question | Resolution |
|---|----------|-----------|
| 1 | Density exposure (grains/sec vs period vs overlap)? | **grains/sec** (1–200), with a **derived live overlap readout** = `(grainSizeMs/1000)·density`. Control the concrete thing; display the conceptual thing. `overlap<1`=gaps, `≥1`=continuous. |
| 2 | MIDI-key → grain-pitch coupling? | **Key-tracked resample** (note transposes the read increment), combined **multiplicatively** with `grainPitch` + `pitchSpray`: `rate = 2^((note−60 + grainPitch + pitchSprayRand)/12)`. Root = C3. Gate-only rejected (un-playable). |
| 3 | Anti-aliasing on upward transposition? | **4-pt Lagrange** fractional read (random-access) + a **per-grain rate-tracking one-pole** low-pass (`fc≈0.5fs/rate`) applied only when `rate>1`. No global oversampling (zero latency). Fallback: interp-only, then 2× OS. |
| 4 | Polyphony? | **8 voices** (granular is heavier; suite norm is 16). Confirmed against grain budget. |
| 5 | Max grain cap + stealing? | **`MaxGrainsPerVoice=24`, steal-oldest** (O-GrainScatter pattern); global cap **192**. Bounded preallocated pool ⇒ no xrun. |
| 6 | Velocity routing? | **velocity→amp always-on**; **velocity→density opt-in** (`velToDensity`, default 0). |
| 7 | Per-grain pan spray? | **Included, minimal** (`panSpray`, 0–100%, default 0). Cheap equal-power pan; stereo cloud is far better in class. |
| 8 | Source-length cap? | **10 s** @ engine rate (built-ins are 1–3 s; loaded files truncated with a UI notice). |
| 9 | Built-in embedding? | **`.wav` via `juce_add_binary_data`** → `BinaryData::fire_wav` → `MemoryInputStream` → `AudioFormatManager::createReaderFor` → resample to engine rate off-thread. (No in-repo precedent for embedding audio, but the API supports it; both `createReaderFor` overloads verified locally.) |
| 10 | Read-head / freeze model? | Playhead velocity = `scan%·realtime`; `freeze` pins velocity to 0 with a smoothed crossfade (zipper-free). Negative scan = reverse. |
| 11 | Load-your-own (macOS WebView)? | **Content-streaming drag-drop** from O-MicrotonalSampler v1.0.4 (`webkitGetAsEntry`→FileReader→base64→`NativeFunction`→temp), decode with **`juce::Base64::convertFromBase64` (NOT `MemoryBlock::fromBase64Encoding`)**, + `FileChooser` picker fallback. |

---

## Project-specific gotchas folded into ARCHITECTURE.md (documented regressions)

- **No alloc / no locks in `processBlock`** (PERF-01): preallocated grain pool, steal-oldest, per-voice `juce::Random`.
- **Windows WebView2 flags mandatory** (COMPAT-02): `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (auto-defines `JUCE_USE_WIN_WEBVIEW2=1`); `withUserDataFolder(tempDir)` on Windows. Without both → blank UI.
- **macOS drag-drop base64**: `juce::Base64::convertFromBase64`, NOT `MemoryBlock::fromBase64Encoding` (silently rejects standard btoa()).
- **WebView JS namespace**: pass the `Juce` ES-module namespace (has `getNativeFunction`) to shared panels, NOT `window.__JUCE__`. Resource provider receives **bare paths** — compare directly.
- **Rectangular window click is an intended teaching artifact**, not a bug to remove.
- **Smooth params** (QUAL-01): freeze pin/unpin + scan zipper-free (`SmoothedValue` + crossfade).
- **Lock-free viz** (PERF-01): grain events via `TripleBuffer`, output samples via `VizRing`, grain count via `atomic<int>`; FFT on message thread; copy scope window before in-place FFT.
- **`getLatencySamples()` non-virtual in JUCE 8** — use `setLatencySamples(0)`.
- **Source hot-swap** via atomic pointer swap (double-buffer; reap old) — audio thread never touches a half-loaded buffer.

---

## Complexity & strategy

- **Complexity score: 5.0** (capped; raw **13.0** = params 2.0 + algorithms 8 + features 3). Tier **5–6** (MIDI synth + file I/O/load-your-own + four real-time visuals + FFT) → research depth **DEEP**.
- **Strategy: phased** (4 stages, Stage 2 DSP and Stage 3 GUI each split into 3 phases) — mirrors the siblings.
- **Net risk: MEDIUM-HIGH effort, LOWER core-DSP risk than O-simpleFM** (feed-forward; engine + infra are ports; the highest-effort item — macOS drag-drop — is a documented shipped pattern with a picker fallback).

---

## Requirements traceability (where each requirement lands)

- **Stage 1 (Foundation):** COMPAT-01 (pluginval), COMPAT-02 (WebView2 flags).
- **Stage 2 (DSP):** FUNC-01/02/03 (overlap-add, MIDI poly, freeze), FUNC-04/05 (source select/load — loading lands in Phase 2.3), DSP-01..08 (grain size, density/overlap, windows, spray, scatter, scan/stretch, ADSR, AA), PERF-01/02 (RT-safe, bounded grains), QUAL-01 (no unintended artifacts).
- **Stage 3 (GUI):** FUNC-06/07 (preset tour, tooltips), UI-01..06 (cloud, waveform+playheads, window inset, scope/spectrum, grain-count/CPU, single-page).
- **Stage 4 (Polish/Validation):** COMPAT-*, remaining acceptance audits.

---

## Pre-Stage-1 gate (IMPORTANT)

Only `parameter-spec-draft.md` exists (plan-before-mockup path). A full **`parameter-spec.md` MUST be produced at mockup finalization before Stage 1**, at which point the **mockup becomes the source of truth** for the final parameter set. The 18-param set is the planning contract to reconcile against the mockup.
</content>
</invoke>
