# Stage 0 (Ideation / Research & Planning) — CONTEXT

**Plugin:** O-simpleSampler
**Date:** 2026-06-25
**Outcome:** Stage 0 complete — ARCHITECTURE.md + ROADMAP.md produced. Ready for Stage 1 (Foundation).

---

## What this plugin is

Pedagogical keyboard sampler — the sampler sibling of O-simpleFM / O-simpleAdditive / O-simpleGrain / O-simpleSubtractive. The irreducible spine: **source recording → start/end region (+ loop + reverse) → pitch engine → Vintage → resonant LP → amp VCA**, 16-voice poly. North star: a student loads one found sound and, within five minutes, makes it a melodic, tunable instrument — the "oh, that's how a sampler works" moment. Built for MUSC319 wk05-wed (musique concrète → Mellotron → Fairlight → MPC → SP-1200 → software sampler). WebView UI, JUCE 8.

---

## Resolved decisions (the discuss → research payoff)

### 1. Stretch algorithm — RESOLVED: synchronous-granular (SOLA), NOT a phase vocoder
The brief's headline open question. **Decision: reuse the O-simpleGrain / O-GrainScatter grain scheduler + overlap-add** as a synchronous-granular SOLA pitch shifter.
- **Repitch** = continuous fractional-read varispeed (`readPos += keyRatio`; pitch+time coupled — honest Mellotron/Fairlight behaviour).
- **Stretch** = time-axis read head at **1× realtime** (duration preserved) + per-grain resample by `keyRatio` (pitch tracks the key), Hann overlap-add at ~2× overlap. Internal grain (≈60 ms, pool 4) **fixed and hidden** behind the single toggle.
- **Why granular over phase vocoder:** in-repo + shipped + proven (O-simpleGrain v1.0.1 / O-GrainScatter v2.4.0); lower latency (no STFT frame); honest artifact profile (mild graininess vs phasiness); the offline render-harness **single-grain autocorrelation probe** already validates it (project memory); and the playhead visual (1× vs pitch-coupled) makes the lesson *visible* for free (UI-02). **Phase-vocoder "HQ Stretch" deferred to v1.1.**

### 2. Anti-aliasing on upward repitch (DSP-02 MUST) — RESOLVED
4-pt **Lagrange** random-access read (`Source/dsp/LagrangeInterpolation.h` from O-simpleGrain) + a **rate-tracking one-pole** low-pass (`fc=0.5·fs/rate`) when `rate>1`. O-simpleGrain decision #3 applied to both pitch modes. A teaching tool must not buzz on high keys. Fallback: 2× oversampling (adds latency) only if the harness budget fails.

### 3. Loop engine (DSP-03 MUST) — RESOLVED
Forward + ping-pong with an **equal-power (sin/cos) crossfade** across the seam (second read head from `loopStart`), plus **automatic zero-crossing snap** of the four markers (off-thread) as a secondary click defense. `loopCrossfade` 0–500 ms (default 10 ms).

### 4. Vintage (DSP-04 MUST) — RESOLVED
One macro = **sample-and-hold rate decimation** (`fsEff=lerp(fs,~3 kHz,vintage)`) + **bit-depth quantization** (`bits=lerp(clean,~8,vintage)`), SP-1200 lineage (26 kHz/12-bit). **Full bypass at `vintage=0`** (bit-for-bit clean). Per-voice, **before the filter** (LP tames decimation aliasing). Mirrors the O-simpleAdditive bit-depth lesson.

### 5. Signal path — RESOLVED: full per-voice chain
`source → region → pitch → Vintage → filter → VCA` is entirely per-voice, matching the brief's literal "single sampler voice" path and the UI's left-to-right teaching order. The live filter curve is driven from the **lead voice** (`displayCutoffHz`/`displayK` atomics) — one readable curve despite 16 voices (O-simpleSubtractive pattern).

### 6. Filter — RESOLVED
Per-voice `juce::dsp::StateVariableTPTFilter<float>` LP (linear — no self-oscillation needed). Same closed-form `Ω=tan(π·f/fs)/g` magnitude gives an **exact filter-response curve for free** (QUAL-02).

### 7. Voice — custom `SamplerVoice`, NOT `juce::SamplerSound`/`SamplerVoice`
JUCE's built-in Sampler does naive varispeed only (no Stretch / loop crossfade / Vintage / per-voice filter). Custom `SynthesiserVoice` reusing the O-simpleFM/O-simpleGrain skeleton. 16 voices, default stealing.

### 8. Sample loading — RESOLVED
- **Built-ins:** embedded `.wav` via a SECOND `juce_add_binary_data` target — **distinct `NAMESPACE BinaryData` + `HEADER_NAME BinaryData.h`** vs the UI-resources target's `UIBinaryData` (the O-simpleGrain Stage-3.1 duplicate-symbol lesson). Decode `createReaderFor(MemoryInputStream)` → resample off-thread → atomic publish. Per-sample default root key seeds `rootKey`.
- **Load-your-own:** reuse the shipped O-simpleGrain `webview-drop-streaming.js` content-streaming module + `dropSample*` native fns; decode with **`juce::Base64::convertFromBase64`** (NOT `MemoryBlock::fromBase64Encoding` — project memory); `FileChooser` fallback. Source-length cap 30 s.

### 9. Parameter resolution (draft open items)
- **KEEP** both `tune` (coarse st) and `fine` (cents) — distinct concepts, do NOT consolidate.
- **DEFER `velToFilter` to v1.1** (per-voice filter makes it addable later).
- **Key-tracking in Stretch:** pitch tracks the key via grain resample; time axis at 1× (held note keeps duration).
- **16 voices**; source cap **30 s**.
- **Final v1.0 core = 21 parameters** (see ARCHITECTURE.md Parameter Mapping). `Load…` is an action/native-fn + custom state, NOT an APVTS param.

---

## Complexity & strategy
- **Tier:** 5–6 (synth + MIDI + file I/O + load-your-own streaming + interactive waveform editor) → research depth DEEP.
- **Complexity score: 5.0** (capped; raw 12.0 = params 2.0 + 7 algorithms + 3 features). Matches O-simpleFM/O-simpleAdditive.
- **Strategy: phased** — Stage 2 DSP in 3 phases, Stage 3 GUI in 3 phases.
- **Highest risk:** Stretch engine (#1) + upward-transposition AA (#2) — both HIGH but de-risked by shipped O-simpleGrain DSP; load-your-own drag-drop (#3) HIGH-effort/LOW-residual (shipped pattern + picker fallback). No feedback loop → lower core-DSP risk than the FM/subtractive siblings.

## Constraints honored
- JUCE 8.0.9; CMake+Ninja; macOS VST3+AU + Windows VST3.
- `IS_SYNTH TRUE` + `NEEDS_MIDI_INPUT TRUE`; `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`.
- **Dual binary-data targets** (embedded samples + WebView UI) → distinct `NAMESPACE` (O-simpleGrain lesson).
- RT-safe processBlock (no alloc/lock/file-IO); sample loading off-thread + atomic pointer swap; lock-free viz handoff.
- `setLatencySamples(0)` (zero added latency, no oversampling); `getLatencySamples()` non-virtual.
- macOS WebView content-streaming drag-drop + `juce::Base64::convertFromBase64`.

## Stage-2 correctness gate
Port the O-simpleFM/O-simpleGrain offline DSP render-harness: Repitch tuning accuracy, **Stretch pitch/time independence (single-grain autocorrelation probe)**, loop-seam click absence, Vintage clean-at-zero, anti-alias budget on high notes, note lifecycle.

## Sibling references (read in-repo)
O-simpleGrain (PRIMARY reuse — Lagrange read, grain engine, sample loading, dual binary-data, drop module, viz, harness), O-simpleSubtractive (resonant LP + curve, ADSR, 16-voice, doc format), O-simpleFM/O-simpleAdditive (voice skeleton, WebView CMake, Additive bit-depth lesson), O-MicrotonalSampler (drag-drop + Base64 gotcha), O-GrainScatter/O-Freeze (overlap-add, loop crossfade).

## Files produced this stage
- `plugins/O-simpleSampler/.planning/research/ARCHITECTURE.md`
- `plugins/O-simpleSampler/.planning/ROADMAP.md`
- `plugins/O-simpleSampler/.planning/stages/0-ideation/CONTEXT.md` (this file)
- `plugins/O-simpleSampler/.planning/STATUS.md` (updated → stage 0 complete)
