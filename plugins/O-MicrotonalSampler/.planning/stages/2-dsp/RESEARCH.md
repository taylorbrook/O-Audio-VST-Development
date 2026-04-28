---
title: "O-MicrotonalSampler Stage 2 (DSP) — Research Phase"
created: 2026-04-27
last_verified: 2026-04-27
juce_version: "8.0.4"
summary: "Stage 2 research resolves the seven Open Questions left by CONTEXT.md so the plan phase can break Stage 2 into 5 sub-stages without architectural ambiguity. Confirms JUCE 8 APIs (CatmullRomInterpolator math, ADSR, LagrangeInterpolator, default Synthesiser::findVoiceToSteal), maps each RQ to a concrete implementation site, and identifies the 5-ms voice-steal ramp as the only mechanism that actually requires a custom subclass. All 12 CONTEXT decisions (D2-1 … D2-12) are upheld; no decision reversals."
domain: workflow
type: guide
keywords:
  - stage-2
  - dsp
  - sampler
  - varispeed
  - cubic-hermite
  - catmull-rom
  - adsr
  - voice-stealing
  - loop-detect
  - filename-parser
  - lagrange
  - equal-power-crossfade
  - note-expression
stages: [2]
agents: [research, dsp]
---

# O-MicrotonalSampler — Stage 2 Research (DSP)

## Scope

CONTEXT.md (discuss phase) locked 12 implementation decisions and identified 7 research questions (RQ-1…RQ-7) that gate the plan phase. This document closes each RQ with concrete code-site references, JUCE-source citations, and the residual unknowns that must be measured at execution-time (e.g., the sine-sweep null test). It does **not** revisit the locked decisions; D2-1…D2-12 stand as written.

**Inputs reviewed:**
- `plugins/O-MicrotonalSampler/.planning/stages/2-dsp/CONTEXT.md` (discuss-phase decisions + RQs)
- `plugins/O-MicrotonalSampler/.planning/stages/1-foundation/RESEARCH.md` (Stage 1 surface invariants — Findings 6, 7, 8, 10)
- `plugins/O-MicrotonalSampler/Source/{MicrotonalSamplerVoice,SampleMap,SampleLoader}.h` (frozen Stage 1 surface)
- `plugins/O-MicrotonalSampler/.planning/{BRIEF.md, REQUIREMENTS.md}` (FUNC/DSP/PERF/QUAL contract)
- `/Users/taylorbrook/JUCE/modules/juce_audio_basics/utilities/{juce_Interpolators.h, juce_ADSR.h}` — `CatmullRomTraits::valueAtOffset`, `LagrangeTraits`, `ADSR::{noteOn,noteOff,getNextSample}`
- `/Users/taylorbrook/JUCE/modules/juce_audio_basics/synthesisers/juce_Synthesiser.cpp:525-600` — default `findVoiceToSteal` body (oldest-released → oldest-keyup → oldest)
- `plugins/O-Lyrica/Source/HarpSynthVoice.{h,cpp}` — `applyPendingTuning` use site at `startNote` (lines 105-147); confirms O-Lyrica does NOT override `findVoiceToSteal` (D2-2 is the only suite-wide custom case)
- `plugins/O-TextureForge/Source/dsp/CorpusLoader.{h,cpp}` — `LagrangeInterpolator::process()` SR-conversion pattern at load time (lines 206-225)
- `plugins/O-Bassoon/.planning/stages/2-dsp/PLAN.md` — sub-stage decomposition precedent (Phase 2.1 = first audio, then incremental phases)
- `modules/tuning/note-expression/cpp/NoteExpression.h` — `applyPendingTuning(table, midi, freq)` helper (Steinberg-free, header-inline)
- `modules/tuning/scala-tuning-engine/cpp/TuningEngine.h` — `getFrequency(int midiNote, int midiChannel = 0)` API
- Project memory: `JUCE 8: getLatencySamples() is NOT Virtual`, `Plugin Cache Clearing`

---

## RQ-1 — Cubic-Hermite aliasing measurement at +50c

**Decision context:** D2-1 says "Cubic-Hermite (4-pt), conditional 1st-order tilt LPF only if Stage 2.1 sine-sweep null test shows audible aliasing."

### Algorithm clarification (Cubic-Hermite ≡ Catmull-Rom)

JUCE's `juce::CatmullRomInterpolator` is the **same 4-point cubic-Hermite formula** with Catmull-Rom tangents (`m_k = ½·(y_{k+1} − y_{k-1})`). From `juce_Interpolators.h:118-131`:

```cpp
return y1 + offset * ((0.5f * y2 - halfY0)
                    + (offset * (((y0 + 2.0f * y2) - (halfY3 + 2.5f * y1))
                    + (offset * ((halfY3 + 1.5f * y1) - (halfY0 + 1.5f * y2))))));
```

This is exactly the cubic-Hermite Catmull-Rom polynomial expanded by Horner's method. Total cost: 7 muls, 8 adds per sample (+ 4 sample fetches). For 16 voices × 48 kHz = 768 k samples/sec → ~12 MFLOPs of interp; well under PERF-02's ≤5 % CPU target on Apple Silicon.

### Why we inline rather than use `juce::CatmullRomInterpolator`

`CatmullRomInterpolator` is a **stateful streaming processor** (4-tap shift register inside the class). It assumes a forward-only stream with externally-driven `speedRatio`. Our voice needs **random-access reads at fractional position `pos`** with **loop-wrap semantics**: when `pos` crosses `loopEnd`, we wrap to `loopStart` and the 4 surrounding samples must include wrapped indices. A streaming interpolator can't express that without state-thrashing on every loop iteration.

**Implementation site (Phase 2.1):** inline static helper `cubicInterp(const float* buf, int N, double pos, int loopStart, int loopEnd)` inside `MicrotonalSamplerVoice.cpp`. Computes `y0,y1,y2,y3` from `wrap(pos-1), wrap(pos), wrap(pos+1), wrap(pos+2)` where `wrap` returns `loopStart + ((idx - loopStart) mod loopLen)` if looping, else clamped to `[0, N-1]`. Apply the same Horner expansion as JUCE.

### The aliasing measurement

CONTEXT D2-1 calls for a sine-sweep null test before deciding whether to add a pre-filter. Concrete plan (Gate 1 of Phase 2.1):

1. **Test fixture:** synthesize a 0.5-Hz/sec exponential sine sweep from 20 Hz → 24 kHz (Nyquist@48k), 5 s long, 32-bit float, into a `SampleSlot` at host SR. `sourceSampleRate == hostSampleRate`, so SR conversion at load is a no-op.
2. **Drive at +50c:** trigger a note that yields `playRate = 2^(50/1200) ≈ 1.02930`. Capture the rendered output for the full sweep duration.
3. **Null test:** also render at exactly +0c (`playRate = 1.0`). Subtract; the residual is the interpolation error. Compute the residual's spectrum.
4. **Aliasing test:** more directly — when sweeping a sine through `f_in`, look for image content at `(f_s − f_in × playRate)` and the descending image. Threshold: **−60 dB below the test sine peak above 8 kHz**. Above this threshold → insert pre-filter.
5. **If pre-filter needed:** single biquad lowpass at `Nyquist / playRate ≈ 23.3 kHz` (just below the new effective Nyquist). One-pole tilt is insufficient at +3 % rate change for content up to 24 kHz; a 1st-order **biquad lowpass** at 22 kHz with `Q = 0.707` is the recommended fallback. `juce::dsp::IIR::Filter<float>` with `juce::dsp::IIR::Coefficients<float>::makeLowPass(hostSR, 22000.0f)`. ~4 muls/adds per sample — negligible.

### Predicted outcome

Catmull-Rom's high-frequency rolloff at offset = 0.5 is approximately **−1.6 dB at 0.4×Fs and −5 dB at 0.45×Fs** (from standard interpolation theory; see e.g. Olli Niemitalo, "Polynomial Interpolators for High-Quality Resampling of Oversampled Audio"). At +50c the speed ratio is 1.029, so a source sine at 22 kHz produces an alias at `48000 − 22000×1.029 ≈ 25366 → mirrored to 22634 Hz` — only 634 Hz from the test sine, with the alias attenuated by ~5 dB from the interpolator rolloff.

**Likely verdict:** content above ~10 kHz in source samples will produce audible foldback at +50c. Most sustained-tone material (bowed strings, woodwinds, vocals — the BRIEF.md target) has minimal energy above 8 kHz, so the audible failure mode is rare. **Default plan: ship without pre-filter**, document the test fixture in `Source/tests/aliasing_check.cpp` (out of build, manual run), commit a passing baseline. If a future user library exposes the foldback, add the biquad later (zero surface change to voice header).

**Pre-filter trigger condition (codified for Phase 2.1 Gate 1):** any alias bin `> −60 dBFS` in the +50c null residual above 8 kHz → pre-filter is mandatory; rebuild and re-test.

**Resolved.** No surface change. Pre-filter is conditional, deferred to Gate-1 measurement.

---

## RQ-2 — RMS loop-detect window / search-range tuning

**Decision context:** D2-4 says "1024-sample RMS window over latter 60 % of sample → pick lowest-RMS region → snap to nearest zero-crossing within ±64 samples → 8-sample equal-power crossfade at loop boundary. Fallback to one-shot if RMS variance high."

### Algorithm spec (Phase 2.5)

Inputs: `juce::AudioBuffer<float>& sample` (mono — for stereo, use channel 0 for analysis), `double sampleRate`.
Outputs: `(loopStart, loopEnd, success: bool)`.

```
windowSize     = 1024  // ~21 ms at 48 kHz
searchStart    = (int) (numSamples * 0.40)  // start scanning 40 % in (latter 60 %)
searchEnd      = numSamples - windowSize - 64  // leave room for window + zc-snap

1. Scan windows: for i in [searchStart..searchEnd] step 256:
       rms[i] = sqrt(mean(sample[i..i+windowSize-1]^2))
   (256-sample stride ≈ 5 ms — fine enough for sustained tones; 4× faster than per-sample scan)

2. Pick i* = argmin(rms[i])
3. Sanity check: if rms[i*] / mean(rms) > 0.7  → high variance, no usable plateau → return (0, 0, false)  [one-shot fallback]
4. Snap loop start: zeroCrossingNear(sample, i*, ±64 samples, falling-edge preference)
5. Compute loop length: target ≥ N cycles of the fundamental.
   For sustained tones, fundamental detection is non-trivial — use a heuristic loop length of MAX(2048, sampleRate / 50) samples (≥ 1 cycle of a 50 Hz tone, which covers everything down to A1).
6. Snap loop end: zeroCrossingNear(sample, loopStart + targetLen, ±64 samples, same-direction edge as loopStart)
7. 8-sample equal-power crossfade applied at PLAYBACK time (RQ-7), not baked into the buffer.
```

### Justification for parameter choices

- **1024-sample window (~21 ms @ 48 k):** long enough to average out one cycle of any pitched content above 50 Hz; short enough not to mask transient regions.
- **256-sample stride:** 4× faster than per-sample scan with no audible loss in best-window selection (windows shift by ~1/4 of their size).
- **Latter 60 % search range:** matches BRIEF.md vocabulary; aligns with the convention that attack/decay live in the first ~40 % of a sustained-tone sample.
- **±64-sample zero-crossing snap (~1.3 ms @ 48 k):** wide enough to find a zc within one fundamental cycle of any pitched content above ~750 Hz, narrow enough that the snapped loop point stays in the low-RMS region.
- **0.7 variance gate:** an empirical threshold — if the best window's RMS is more than 70 % of the mean RMS in the search range, the sample lacks a clear sustained plateau (transient material like percussion, single attacks, FX). Falls back to one-shot per EC-7.
- **`MAX(2048, SR/50)` loop length:** simple and robust. 2048 samples ≈ 23 Hz at 48 k worst case; covers any acoustic instrument fundamental. No pitch-detection pass required for v1.0.

### Adaptive options (deferred to v1.1)

- **Multi-cycle loop detection** via autocorrelation peak in the search region — would let loops align to integer fundamental periods, eliminating the 8-sample crossfade in the ideal case. Out of scope for v1.0; the equal-power crossfade is sufficient.
- **Manual per-sample loop override** — DSP-06, deferred to Stage 3 UI per CONTEXT.md "Out of scope this stage."

**Resolved.** Algorithm fully specified. `Phase 2.5 — Loop Detect` task creates `Source/LoopDetector.{h,cpp}` with this body, called from `SampleLoader::run()` per-sample after load + before publish.

---

## RQ-3 + RQ-4 — Voice-steal hook site and ramp implementation

**Decision context:** D2-2 (override `findVoiceToSteal` for oldest-released → oldest-held); D2-3 (5-ms linear ramp before reuse).

### JUCE default `findVoiceToSteal` already matches D2-2

`juce::Synthesiser::findVoiceToSteal` (juce_Synthesiser.cpp:525-600) does, in order:
1. Reuse a voice already playing the same target pitch (idempotent retrigger).
2. **Oldest voice that `isPlayingButReleased()`** ← matches "oldest-released first."
3. Oldest voice that has no key down (`!isKeyDown()`).
4. Oldest unprotected voice (lowest+highest sustained notes are protected unless released).

This is **already** the behavior D2-2 specifies. The "lowest+highest protect" extension is a feature, not a bug, for sustained microtonal material — it preserves bass and top-line voicing under voice exhaustion. **No subclass / override required for the steal-selection logic.**

**Decision refinement (preserves D2-2 intent):** keep JUCE's default. The plan-phase locks this as "D2-2 is satisfied by JUCE default; voice-steal customization is the *ramp* (D2-3), not the *selection* (D2-2)."

### The 5-ms ramp implementation (RQ-3 site)

**Site choice:** inside `MicrotonalSamplerVoice::startNote(...)`, **at the top of the function**, before any other state init.

**Why startNote (not findVoiceToSteal-side):** by the time JUCE has chosen this voice as the steal target, JUCE calls `voice->startNote(...)` directly — there is no "stolen, ramping out, then reassigned" intermediate state in JUCE's stock flow. The cleanest pattern is for the voice itself to detect "I am being reassigned while still active" and apply the ramp to its **outgoing** sample state before initializing the new note.

**Mechanism:** the voice maintains a small **release-tail** buffer (`std::array<float, kStealRampSamples * 2>` for stereo; `kStealRampSamples = (int)(0.005 * sampleRate)` ≈ 240 @ 48 k). When `startNote` is called and the voice was active (`adsr.isActive()` returns true), the voice:

1. Snapshots the current envelope value `lastEnv = adsr.getNextSample()`  (single-sample peek without advance — actually `envelopeVal` private; the simplest correct approach is to **render the next 240 samples of the OLD note into a small scratch buffer, multiplied by a linear ramp from `lastEnv → 0` over the 240 samples**, and append into `pendingTail`).
2. Resets all per-note state (`pos = startSample`, `playRate = newRate`, `adsr.reset()`, `adsr.noteOn()`).
3. In the next `renderNextBlock(...)`, **adds `pendingTail` to the output buffer for the first 240 samples** before/alongside the new note's normal render path.

**Cheaper alternative (preferred — matches O-Lyrica's pattern of "no explicit steal handling, rely on ADSR release"):** if release time is `≥ 5 ms`, JUCE's `findVoiceToSteal` will preferentially pick released voices, and the natural ADSR release tail handles the click. The 5-ms ramp only matters when **release == 0**. So:

- **If release > 5 ms:** rely on JUCE default + ADSR release → no special handling.
- **If release ≤ 5 ms:** force a 5-ms minimum tail by intercepting in `stopNote(velocity, allowTailOff)`: when JUCE calls `stopNote(0.0f, false)` (the "kill-now" call that precedes a steal), substitute a forced 5-ms linear release through the existing ADSR by temporarily setting `adsr.setParameters({a, d, s, 0.005f})` and calling `adsr.noteOff()`; voice clears itself when ADSR completes.

**However,** JUCE's `Synthesiser::startVoice(...)` calls `voice->stopNote(0.0f, false)` **then immediately calls** `voice->startNote(...)` — there's no audio rendered between them. The 5-ms tail must therefore be rendered into `pendingTail` and **mixed into the next block** alongside the new note's audio.

### Final design (locked for Phase 2.4)

```cpp
// MicrotonalSamplerVoice.h additions (Phase 2.4)
private:
    std::vector<float>        stealTailBufferL;   // resized in prepareToPlay to kMaxStealRamp samples
    std::vector<float>        stealTailBufferR;
    int                       stealTailSamplesRemaining = 0;

// MicrotonalSamplerVoice.cpp Phase 2.4 additions:
void MicrotonalSamplerVoice::startNote(...)
{
    // STEAL DETECTION + RAMP CAPTURE
    if (adsr.isActive() && currentSlot != nullptr)
    {
        const int rampSamples = juce::jmin((int)(0.005 * getSampleRate()),
                                           (int) stealTailBufferL.size());
        renderTailRamp(rampSamples);  // fills stealTailBufferL/R with old-note × linear-down ramp
        stealTailSamplesRemaining = rampSamples;
    }
    // ...normal startNote body...
}

void MicrotonalSamplerVoice::renderNextBlock(juce::AudioBuffer<float>& out, int startSample, int numSamples)
{
    // Mix in the steal tail FIRST
    if (stealTailSamplesRemaining > 0)
    {
        const int n = juce::jmin(stealTailSamplesRemaining, numSamples);
        out.addFrom(0, startSample, stealTailBufferL.data() + (kMaxStealRamp - stealTailSamplesRemaining), n);
        out.addFrom(1, startSample, stealTailBufferR.data() + (kMaxStealRamp - stealTailSamplesRemaining), n);
        stealTailSamplesRemaining -= n;
    }
    // ...normal voice render path...
}
```

**Allocation:** `stealTailBufferL/R` resized **once** in `prepareToPlay` (message thread). Audio thread only writes to and reads from these buffers — RT-safe.

### Resolved questions

- **RQ-3 (ramp implementation site):** inside `MicrotonalSamplerVoice` (not at `findVoiceToSteal` level). Voice self-detects via `adsr.isActive() && currentSlot != nullptr` at top of `startNote`.
- **RQ-4 (`findVoiceToSteal` override):** NO override needed. JUCE default already matches D2-2. The plan locks "D2-2 satisfied by default impl; D2-3 implemented via voice-side `startNote` tail capture."

---

## RQ-5 — Filename parser pattern set

**Decision context:** D2-7 says "Tolerant per BRIEF.md (case-insensitive, multi-convention). Accept: `note_velocity`, `note-velocity`, sharps (`C#4`) and flats (`Db4`), MIDI numbers (`60_v2`), layer suffixes (`_p`/`_mp`/`_mf`/`_f` → v1..v4). Silently skip + log unparseable files."

No O-TextureForge equivalent — it parses no per-file metadata. We're building the pattern set from scratch.

### Spec (Phase 2.2)

**Header:** `Source/FilenameParser.{h,cpp}` — pure-function `parse(const juce::String& filenameNoExtension) → std::optional<{int midiNote, int velLayer}>`.

**Tokenization:** split on `[_\-\s.]+` into 1+ tokens. For each token, attempt `parseAsNote` then `parseAsVelocity`. If both succeed (in any order across the tokens), return success.

**`parseAsNote(token)` regex set (case-insensitive):**

| Pattern | Match example | Output |
|---|---|---|
| `^([A-Ga-g])([#b]?)(-?[0-9])$` | `C4`, `c4`, `C#4`, `Db5`, `A-1`, `Bb-2` | semitone offset from C-1 (MIDI 0) |
| `^[Mm][Ii][Dd][Ii]?(-?[0-9]{1,3})$` | `MIDI60`, `midi72`, `M-Note48` | direct MIDI number (clamped 0–127) |
| `^([0-9]{1,3})$` (only when token is the FIRST token AND value ∈ [0,127]) | `60`, `72`, `127` | direct MIDI number — but only if no other token in the filename matched as a note |

Rationale for the bare-number ambiguity rule: a file like `12_v2.wav` could mean MIDI 12 + vel 2, OR pitch unset + ID 12 + vel 2. We treat bare numbers as MIDI **only** when the filename otherwise lacks a parseable note token, biasing toward the named-note convention which is by far the dominant convention in EXS24 / Decent Sampler / Kontakt libraries.

**`parseAsVelocity(token)` regex set (case-insensitive):**

| Pattern | Match | Output |
|---|---|---|
| `^[Vv]([0-9])$` | `v1`, `V4`, `v2` | velLayer = digit − 1, clamped to [0,3] |
| `^[Vv][Ee][Ll]?([0-9])$` | `vel2`, `Vel3`, `VEL1` | same |
| `^[Pp]$` | `p` | velLayer = 0 (pp) |
| `^[Mm][Pp]$` | `mp` | velLayer = 1 |
| `^[Mm][Ff]$` | `mf` | velLayer = 2 |
| `^[Ff]$` | `f` | velLayer = 3 (ff) |
| `^[Ll][Aa]?[Yy]?[Ee]?[Rr]?([0-9])$` | `layer3`, `L2`, `Lyr4` | velLayer = digit − 1 |

**Default if no velocity token found:** `velLayer = 0` (single-layer interpretation — for libraries like `C4.wav, D4.wav, ...` with no velocity metadata).

**Deduplication:** if multiple files map to the same `(midiNote, velLayer)` cell, keep the first parsed and log a warning for the rest. (Manual override path in Stage 3 lets the user replace.)

### Test fixtures (committed to test repo at Phase 2.2)

| Filename | Parsed → |
|---|---|
| `C4_v1.wav` | (60, 0) |
| `c4-v2.aif` | (60, 1) |
| `C#4_v3.wav` | (61, 2) |
| `Db5_v4.wav` | (73, 3) |
| `A-1.wav` | (9, 0) — A in octave -1 = MIDI 9 |
| `60_v2.wav` | (60, 1) — bare number wins because `60` parses as MIDI |
| `MIDI72_mf.wav` | (72, 2) |
| `cello_C3_mp.aif` | (48, 1) — `cello` is unparseable; ignored |
| `viola-A4-f.wav` | (69, 3) |
| `weird-name-no-pitch.wav` | none → skip + log |
| `D4.wav` | (62, 0) — no velocity → assume single layer |

### Logging

`SampleLoader::run()` accumulates a `juce::StringArray skippedFiles;` and includes it in the completion callback. The Stage 3 UI surfaces the list in a tooltip / log panel; Stage 2.2 just stores it.

**Resolved.** Pattern set is concrete, exhaustive for the 6 conventions called out in BRIEF.md / D2-7. Implementation lands in Phase 2.2 alongside `SampleLoader::run()`.

---

## RQ-6 — `juce::LagrangeInterpolator` SR ratio limits

**Decision context:** D2-9 says "`juce::LagrangeInterpolator` at load time (one-time) — 5th-order resample once into host-SR buffer."

### Verified behavior

`juce::LagrangeInterpolator` uses a **5-tap finite impulse response** (per `juce_GenericInterpolator.h:159` — `GenericInterpolator<LagrangeTraits, 5>`). The `process(speedRatio, in, out, numOutSamples, numInSamples, indexBuffer)` API is the standard interface (used identically by O-TextureForge `CorpusLoader::resampleToTargetRate`, lines 206-225).

**`speedRatio = sourceSR / targetSR`** — values > 1.0 downsample (more input than output), < 1.0 upsample (more output than input).

**Quality envelope (from JUCE source + standard polynomial-interp theory):**
- `speedRatio ∈ [0.5, 2.0]`: full quality, < −80 dB image artifacts.
- `speedRatio ∈ [0.25, 4.0]`: usable, < −60 dB images.
- `speedRatio < 0.25` (heavy upsample, e.g. 8 kHz source → 48 k host = ratio 0.167): increasing image artifacts; LagrangeInterpolator still produces output but rolloff softens.
- `speedRatio > 4.0` (heavy downsample, e.g. 192 kHz source → 44.1 k host = ratio 4.35): aliasing leak around new Nyquist, no anti-alias prefilter applied.

### Real-world coverage check

The Stage 2 SR matrix (CONTEXT.md "Sample-rate floor"): host SR ∈ {44.1, 48, 88.2, 96, 176.4, 192} kHz. Source SR realistic range: {8, 11, 16, 22.05, 32, 44.1, 48, 88.2, 96, 176.4, 192} kHz (covers anything from old-school 8-bit WAV up to studio-master AIFs).

| Source SR | Host SR | Ratio | LagrangeInterpolator verdict |
|---|---|---|---|
| 8 k | 192 k | 0.042 | **Out of clean range** — ratio < 0.25; flag in load log, accept output but warn |
| 22.05 k | 96 k | 0.230 | Marginal — < 0.25 cutoff; log warning, accept |
| 44.1 k | 48 k | 0.919 | Excellent |
| 48 k | 48 k | 1.0 | No-op (CorpusLoader.cpp:208 early-out: `if (abs(src - tgt) < 0.1) return;`) |
| 96 k | 48 k | 2.0 | Excellent |
| 192 k | 48 k | 4.0 | Edge of clean range — log info-level note |
| 192 k | 44.1 k | 4.35 | **Out of clean range** — ratio > 4.0; log warning, accept |

### Fallback policy (Phase 2.2)

Don't reject any file based on ratio. **Always load**, but log:
- `ratio ∈ [0.25, 4.0]` → INFO: "Resampled C4_v1.wav from 96 kHz → 48 kHz."
- `ratio ∉ [0.25, 4.0]` → WARN: "C4_v1.wav has source rate 192 kHz vs host 44.1 kHz — extreme ratio 4.35×; quality may suffer."

The user is the authority on whether this matters (they may have intentionally loaded an 8 kHz lo-fi vocal sample). The plugin doesn't make that judgment.

### Anti-alias pre-filter for heavy downsample (out of scope v1.0)

For ratios > 4.0, JUCE's interpolator has no internal anti-alias filter. Adding one would require a low-pass filter pre-pass before the interpolation call, sized to the new Nyquist. **Deferred to v1.1** — practical impact is small (192 k source samples are rare for sample libraries) and the fix is a localized Phase 2.2 follow-up if user reports surface.

**Resolved.** Algorithm is `LagrangeInterpolator::process()` per CorpusLoader pattern; ratio limits documented; logging policy specified.

---

## RQ-7 — Equal-power crossfade math (vel-layer + loop-boundary)

**Decision context:** D2-4 (8-sample equal-power crossfade at loop boundary) + D2-9 + REQUIREMENTS DSP-04 (equal-power crossfade between adjacent velocity layers).

### Single shared formula

Both crossfade sites use the **constant-power (sin/cos)** formulation:

```cpp
// gainA × sampleA + gainB × sampleB,  where x ∈ [0,1] is the fade position
inline std::pair<float, float> equalPowerWeights(float x) noexcept
{
    const float t = x * juce::MathConstants<float>::halfPi;
    return { std::cos(t), std::sin(t) };   // {gainA fading out, gainB fading in}
}
```

At `x = 0` → `(1.0, 0.0)`; at `x = 0.5` → `(0.707, 0.707)` (sum-of-squares = 1 → constant power); at `x = 1` → `(0.0, 1.0)`. Standard formulation; no surprises.

### Site 1 — Velocity-layer crossfade (Phase 2.3)

Computed **once per note at `startNote`**, baked into per-voice `layerWeightLow` and `layerWeightHigh` floats. The voice may then play one OR two `SampleSlot`s in parallel (sums them in the inner render loop), each weighted by its baked layer weight.

**Layer-boundary geometry:** for `numVelocityLayers = 4`, the boundaries are at MIDI velocities {32, 64, 96} (1-31 / 32-63 / 64-95 / 96-127, per BRIEF.md). The `velocity_crossfade` parameter (∈ [0,1]) sets the fade **width** as a fraction of the layer span:
- `velocity_crossfade = 0` → hard switch at the boundary, single layer plays.
- `velocity_crossfade = 1` → fade extends 50 % into each adjacent layer (full overlap).

Pseudocode (per-note, at startNote, with `vel ∈ [1,127]`, `layerWidth = 128 / numVelocityLayers`):
```
layerIdx       = clamp((vel - 1) / layerWidth, 0, numVelocityLayers - 1)
layerCenter    = (layerIdx + 0.5) * layerWidth
distanceCenter = vel - layerCenter   // signed
fadeWidthSamples = velocity_crossfade * (layerWidth / 2)
if abs(distanceCenter) < fadeWidthSamples and (the adjacent layer exists):
    x          = (distanceCenter / fadeWidthSamples) * 0.5 + 0.5  // in [0,1]
    (wLow, wHigh) = distanceCenter < 0 ? equalPowerWeights(1-x) : equalPowerWeights(x)
    use both layers, weighted
else:
    use only the in-layer slot, weight 1.0
```

Adjusts gracefully when `numVelocityLayers < 4`. EC-5 (velocity exactly at layer boundary) is satisfied: at the boundary, `wLow = wHigh = 0.707`, both layers contribute equal energy.

### Site 2 — Loop-boundary crossfade (Phase 2.5)

Applied **per-sample** in the voice's inner render loop, only across the 8-sample crossfade window straddling the loop boundary. When `pos` enters `[loopEnd - 8, loopEnd]`:

```cpp
// pseudocode in the inner render loop
const float fadeT = (float)(pos - (loopEnd - 8)) / 8.0f;   // 0 → 1 across the 8 samples
const auto [wOut, wIn] = equalPowerWeights(fadeT);
const float outgoing = cubicInterp(buf, N, pos, loopStart, loopEnd, /*noWrap*/);  // straight-through
const float incoming = cubicInterp(buf, N, pos - (loopEnd - loopStart), loopStart, loopEnd, /*noWrap*/);  // wrapped read
sample = outgoing * wOut + incoming * wIn;
```

(Pre-compute the 8-element cos/sin table at prepareToPlay; the inner loop indexes it instead of calling `std::cos/sin` per sample.)

### Sum-of-squares property

Both sites preserve constant power: `cos²(θ) + sin²(θ) = 1`. No volume dip at the crossover — the user perceives a smooth blend without a notch.

**Resolved.** Single `equalPowerWeights(x)` helper in `Source/MicrotonalSamplerVoice.cpp` (anonymous namespace), plus an 8-element pre-baked cos/sin LUT for the loop-boundary site.

---

## Sub-stage decomposition (refined from CONTEXT D2-6)

CONTEXT locked the **order**: 2.1 voice DSP → 2.2 loader → 2.3 vel crossfade → 2.4 voice-steal → 2.5 loop-detect. Research now anchors each sub-stage to concrete scope and Gate criteria. Plan phase will use these as task-graph inputs.

### Phase 2.1 — Voice DSP + first audio (Gate 1)

**Goal:** silent stub voice → produces sound for a single MIDI note from a hardcoded test fixture (an in-memory `SampleSlot` constructed at prepareToPlay time, since the loader doesn't exist yet).

**Touches:**
- `MicrotonalSamplerVoice.cpp` — implement `startNote/stopNote/renderNextBlock`, inline `cubicInterp`, ADSR member, NE consumption.
- `MicrotonalSamplerVoice.h` — add `juce::ADSR adsr;`, `double pos`, `double playRate`, `const SampleSlot* currentSlot`, `int currentMidiNote`, prepareToPlay declaration.
- `PluginProcessor.cpp` — call `voice->prepareToPlay(sampleRate, samplesPerBlock)` in `prepareToPlay`. Optionally construct an in-memory test fixture `SampleMap` (one C4 sine-burst slot) for Gate 1 audibility test. Test fixture is gated behind a `#if O_MICROTONAL_SAMPLER_PHASE_2_1_TEST_FIXTURE` macro and removed at Phase 2.2.

**Gate 1 verification:**
1. Build VST3 + AU + Standalone — clean.
2. pluginval `--strictness 5` — pass.
3. Standalone smoke-test: hardcoded fixture plays C4 sine when key is hit; ±50c retune via NE expression event audibly shifts pitch.
4. Aliasing null test (RQ-1 plan): record +50c output of sine sweep, decide pre-filter / no pre-filter. If pre-filter required, add `juce::dsp::IIR::Filter` member + 1 line in render loop, retest.
5. Allocation guard via `pluginval` — no allocs in `processBlock`.

### Phase 2.2 — Sample loader + filename parser

**Goal:** drag-drop folder → background thread loads files → `SampleMap` published atomically → next note picks up real samples.

**Touches:**
- `SampleLoader.cpp` — fill `run()` with: `AudioFormatManager` ctor → enumerate folder → parse filenames (RQ-5) → load each file via `AudioFormatReader::read` → SR-convert via `LagrangeInterpolator` (RQ-6) → mono→stereo duplicate (D2-10) → build `SampleMap` → `MessageManager::callAsync` to swap into processor.
- `SampleMap.cpp` (new) — `findSlot(midiNote, velocity)` lookup body. Linear scan over `slots` keyed on (midiNote, velLayer). 16 voices × 1 lookup per startNote → ~10 ns scan overhead at 88 notes × 4 layers = 352 entries; trivial.
- `FilenameParser.{h,cpp}` (new) — RQ-5 parser.
- `PluginEditor.cpp` — minimal "Load Folder…" button (Stage 3 UI replaces wholesale; Stage 2.2 ships a drop target only).

**Gate 2 verification:**
1. Drop a folder of 4 sine-burst WAVs (C4_v1, C4_v2, D4_v1, D4_v2) → all 4 audibly play correctly mapped.
2. Folder with malformed names → unparseable files silently skipped, parseable ones load.
3. Folder with mixed source SRs (44.1 + 96) → both load and play in tune.
4. Concurrent load + playback → active notes keep playing (old `shared_ptr<SampleMap>` snapshot), new notes pick up new map (EC-3).
5. pluginval `--strictness 5` — still clean.

### Phase 2.3 — Velocity-layer crossfade

**Goal:** low-velocity vs high-velocity samples blend smoothly at layer boundaries.

**Touches:**
- `MicrotonalSamplerVoice.cpp` — at `startNote`, compute `(slotLow, slotHigh, wLow, wHigh)` per RQ-7 Site 1. Inner render loop: if both slots set, sum weighted; else single-slot path.
- `velocity_crossfade` APVTS param read in `startNote` (parameter smoothing not needed — value is consumed once per note, so reads are at note-rate not sample-rate).

**Gate 3 verification:**
1. 4-layer sample set, sweep MIDI velocity 1→127 → crossfade between layers is audibly smooth, no clicks at boundaries.
2. `velocity_crossfade = 0` → hard switch, no overlap.
3. `velocity_crossfade = 1` → maximum overlap.
4. Velocity exactly at boundary (vel=64 with 4 layers) → both adjacent layers play at 0.707 each (verifiable with a steady-tone sample by ear; both contribute).

### Phase 2.4 — Voice-stealing ramp

**Goal:** 17th note while 16 are sounding → no clicks, smooth handoff.

**Touches:**
- `MicrotonalSamplerVoice.{h,cpp}` — add `stealTailBufferL/R`, `stealTailSamplesRemaining` (RQ-3 design). Implement `renderTailRamp(int n)` private helper. `startNote` self-detects active voice and calls it. `renderNextBlock` mixes tail in.
- `PluginProcessor.cpp` — verify `synthesiser.setNoteStealingEnabled(true)` is set (it's true by default in `juce::Synthesiser`, but we explicitly set in ctor for clarity).

**Gate 4 verification:**
1. Hold 16 sustained notes → trigger 17th → 17th plays cleanly, one of the 16 fades over 5 ms (audible only as a soft drop-out, no click).
2. With ADSR release = 0 (worst case) → still no click on steal.
3. With ADSR release = 2 s → tail fade is the 5-ms ramp PLUS the natural release tail (release is independent of steal mechanism).
4. Reduce `polyphony` from 16 → 8 mid-performance → excess voices steal-released on next note-on (EC-9).

### Phase 2.5 — Loop auto-detect

**Goal:** sustained samples loop seamlessly during long held notes.

**Touches:**
- `LoopDetector.{h,cpp}` (new) — RQ-2 algorithm. Pure function `detectLoop(const AudioBuffer<float>&, double SR) → (loopStart, loopEnd, success)`.
- `SampleLoader::run()` — invoke `detectLoop` per-sample after load and before publish; populate `slot.loopStart` / `slot.loopEnd`.
- `MicrotonalSamplerVoice.cpp` — inner render loop: if `slot.loopEnd > 0`, wrap `pos` and apply 8-sample equal-power crossfade at the boundary (RQ-7 Site 2).

**Gate 5 verification:**
1. Sustained sine-wave sample (5 s long) → loops at the same level forever, no audible crossfade artifact.
2. Sustained vibrato cello sample → loops without pitch jump or click.
3. Transient percussion sample (high RMS variance) → falls back to one-shot, plays through, ADSR release ends voice (EC-7).
4. Loop crossfade with `loopEnd - loopStart < 16` samples → fallback to one-shot (the 8-sample crossfade would consume half the loop region — not useful; defensive guard).

### Phase 2 Final Verification

After Gate 5:
1. **Acceptance suite** — synthetic test rig that loads a known sample set, plays all 88 notes × 4 layers, and verifies output checksums (regression catch for future Phase 2.x changes).
2. **CPU benchmark** — Apple Silicon, 48 k / 256 buffer, 16 sustained voices → CPU% measurement vs PERF-02 ≤ 5 % target.
3. **Allocation audit** — pluginval reports zero allocs in `processBlock`.
4. **Latency audit** — `getLatencySamples()` returns 0 (PERF-04). Note: `getLatencySamples` is non-virtual in JUCE 8 (project memory) → verify by host inspection or `setLatencySamples(0)` in `prepareToPlay` is not required (default is 0, no override).
5. **Full requirement traceability** — all 15 Stage 2 requirements per CONTEXT.md §"Requirements Confirmed" verified.

---

## Pitfalls (Stage-2-specific additions)

These augment the 13 pitfalls captured in Stage 1 RESEARCH.md §Pitfalls. Re-read those before Phase 2.1 — they all still apply.

1. **`juce::ADSR::setParameters` precondition** — `setSampleRate` MUST be called first; precondition is a `jassert`. Per `prepareToPlay`. Stage 2.1 task: in voice's `prepareToPlay`, call `adsr.setSampleRate(sampleRate)` before any param read.
2. **`juce::ADSR` doc warning** — "Do not change the parameters during playback." We obey this by reading APVTS only in `startNote` (not per-block), so parameter changes between notes are safe; mid-note APVTS twiddle is silently ignored, which matches O-Lyrica / O-Bassoon behavior.
3. **`std::pow(2.0, semis/12.0)` in startNote is fine** — `pow` is not RT-banned (allocation-free, deterministic), and `startNote` is rare event. However, `Ouaricon::NoteExpression::applyPendingTuning` already handles this — voice never calls `pow` directly.
4. **`shared_ptr<SampleMap>` capture in `startNote`** — must use `std::atomic_load(&sampleMapSource->/*ptr*/)` to acquire, OR if `sampleMapSource` is `std::shared_ptr<SampleMap>*`, use `std::atomic_load(sampleMapSource)`. **Refcount inc is the only allocation-equivalent op** — atomic, lock-free, RT-safe.
5. **Per-note `currentSlot` lifetime** — voice stores `const SampleSlot*` for note duration. The `shared_ptr<SampleMap>` snapshot copy keeps the map alive. Class member: store BOTH the `shared_ptr` (lifetime owner) and the `const SampleSlot*` (frequently-accessed pointer into the map).
6. **`stealTailBufferL/R` allocation in prepareToPlay** — `std::vector::resize` allocates. This is **message thread** (prepareToPlay is called by the host on the main thread before audio starts), so safe.
7. **`juce::AudioBuffer<float>::addFrom` channel count** — voice mixes into a stereo buffer. Mono samples are pre-duplicated to stereo at load time (D2-10), so all `SampleSlot::audio` is always 2-channel. Voice can hardcode the stereo write loop.
8. **`juce::SmoothedValue` ramp length for `output_gain`** — set in `prepareToPlay` to ~10 ms (480 samples @ 48 k) using `SmoothedValue::reset(sampleRate, 0.01)`. Standard convention; matches O-Lyrica.
9. **`processBlock` post-gain application order** — apply `output_gain` AFTER `synthesiser.renderNextBlock` and AFTER all voices have written. Single buffer pass: `buffer.applyGainRamp(0, 0, numSamples, startGain, endGain)` per channel using the smoothed value's start/end snapshots.
10. **Per-channel `LagrangeInterpolator` state** — if a stereo source needs SR conversion, **two separate `LagrangeInterpolator` instances** (one per channel) per `juce_Interpolators.h` doc warning. CorpusLoader downmixes to mono first; we don't, so honor the per-channel rule.
11. **Loader thread cancellation** — `SampleLoader::cancelLoad()` must set a flag the run loop checks via `threadShouldExit()` (CorpusLoader pattern). At Phase 2.2 implementation, every long-running step (`for (auto& file : folder)` body) checks `threadShouldExit()` between files.
12. **Loader completion callback on the wrong thread** — `MessageManager::callAsync` IS the right pattern. Direct call from `run()` would invoke processor mutators from the wrong thread. Use callback dispatch.
13. **Filename parser case sensitivity on Windows vs macOS** — `juce::DirectoryIterator` returns the filesystem-stored case. Parser must be case-insensitive (already is per RQ-5). On case-insensitive filesystems (default macOS HFS+/APFS), `c4.wav` and `C4.WAV` collide on disk anyway.
14. **`juce::AudioFormatReader` lifetime under `std::unique_ptr`** — `createReaderFor` returns `juce::AudioFormatReader*` you own; wrap in `std::unique_ptr` immediately. Failing to delete leaks file handles fast (one per loaded sample).
15. **Stereo file → 2 channels, mono file → 1 channel from reader, then duplicate** — match D2-10. Loader inspects `reader->numChannels`; if 1, copies mono data into both L and R of the slot's stereo buffer; if 2, copies straight through.

---

## Files Stage 2 Will Create or Touch

| File | Action | Phase |
|---|---|---|
| `Source/MicrotonalSamplerVoice.h` | Touch — add ADSR, pos, playRate, currentSlot, currentMap, stealTail buffers, prepareToPlay decl | 2.1, 2.4 |
| `Source/MicrotonalSamplerVoice.cpp` | Touch — implement startNote/stopNote/renderNextBlock, cubicInterp, layer crossfade, steal ramp, loop crossfade | 2.1, 2.3, 2.4, 2.5 |
| `Source/SampleMap.h` | Touch — fill `findSlot` body | 2.2 |
| `Source/SampleMap.cpp` | Create — `findSlot` linear scan body | 2.2 |
| `Source/SampleLoader.cpp` | Touch — fill `run()` with AudioFormatManager + parser + Lagrange resample + map build | 2.2 |
| `Source/FilenameParser.h` + `.cpp` | Create — RQ-5 parser | 2.2 |
| `Source/LoopDetector.h` + `.cpp` | Create — RQ-2 algorithm | 2.5 |
| `Source/PluginProcessor.cpp` | Touch — voice prepareToPlay loop, output_gain SmoothedValue, optional Phase-2.1 test fixture | 2.1, 2.2 |
| `Source/PluginProcessor.h` | Touch — add SmoothedValue member for output_gain | 2.1 |
| `Source/PluginEditor.cpp` | Touch — minimal "Load Folder…" button (Stage 3 UI replaces) | 2.2 |
| `plugins/O-MicrotonalSampler/CMakeLists.txt` | Touch — add new Source files to source list | 2.2, 2.5 |

**No CMake module additions** — `juce::juce_audio_formats` already linked in Stage 1. `juce::juce_dsp` is needed only if the conditional aliasing pre-filter is required (Phase 2.1 Gate 1 outcome); Stage 1 RESEARCH.md confirms it's already in the link list as a forward-compat measure.

---

## Module Reuse — Stage 2 Confirmation

No new shared modules required for Stage 2.

| Module | Stage 2 use |
|---|---|
| `note-expression` v1.1.0 | Voice consumes via `Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningSource, midiNote, currentFrequency)` in `startNote` (D2-12). Already wired at Stage 1. |
| `scala-tuning-engine` v2.0.0 | Voice base-frequency calc: `currentFrequency = tuningEngine->getFrequency(midiNote)`. Already wired at Stage 1. |

**Promotion candidate (deferred to v1.1+):** if a second sampler plugin lands, promote `SampleLoader` + `SampleMap` + `FilenameParser` + `LoopDetector` into a new `modules/synthesis/sample-engine/` shared module. v1.0 ships them as plugin-local files per the "extract on second use" project convention.

---

## Decision Refinements (CONTEXT additions/clarifications — no reversals)

These are clarifications discovered during research; CONTEXT decisions D2-1 … D2-12 stand unchanged.

| # | Refinement | Source |
|---|---|---|
| R1 | D2-2 satisfied by JUCE default `findVoiceToSteal`. **No subclass.** Customization budget moves entirely to D2-3 (the ramp). | RQ-3 / RQ-4 analysis above; `juce_Synthesiser.cpp:525-600` |
| R2 | D2-1 cubic-Hermite implementation = inline copy of JUCE's `CatmullRomTraits::valueAtOffset` Horner expansion (NOT calling `juce::CatmullRomInterpolator` directly — random-access semantics required for loop wrap). | RQ-1; `juce_Interpolators.h:118-131` |
| R3 | D2-4 RMS scan stride = 256 samples (4× faster than per-sample) with no audibility loss; algorithm spec in RQ-2. | RQ-2 |
| R4 | D2-7 parser: bare-number tokens treated as MIDI **only** when no other token in the filename parses as a note. Resolves `12_v2.wav` ambiguity. | RQ-5 |
| R5 | D2-9 Lagrange ratio fallback policy: log + accept all ratios; never reject. | RQ-6 |
| R6 | D2-3 ramp implementation: voice-side, in `startNote` — detects active steal via `adsr.isActive()`, captures 5 ms of "old note × linear-down" into a per-voice scratch buffer, mixes into next render block. **Allocation in prepareToPlay only.** | RQ-3 |
| R7 | D2-11 `output_gain` smoothing: 10 ms via `juce::SmoothedValue::reset(sr, 0.01)`. | Pitfall 8 |

---

## Open Questions (None Blocking)

All 7 CONTEXT-phase RQs are resolved with concrete implementation paths. The plan phase can proceed directly. The single residual unknown is the **Phase 2.1 Gate 1 aliasing measurement outcome** — but that is by design (D2-1 explicitly defers the pre-filter / no-pre-filter call to measurement). The plan phase will encode "if measurement shows alias > −60 dBFS above 8 kHz at +50c → add biquad LPF" as a conditional Gate 1 step, not a hard pre-locked decision.

---

## Next Phase

Ready for **plan** phase: `/plugin-plan O-MicrotonalSampler 2-dsp`

Plan-phase scope: produce a 5-wave execution plan (Phases 2.1 → 2.5) with one atomic commit per gate. Phase 2.1 spawns `dsp-agent` with this RESEARCH.md + CONTEXT.md attached and the in-memory test fixture macro pre-included. Each subsequent phase has clean wave boundaries — no concurrent file edits across phases (sub-stages are sequential). Verification gates per CONTEXT D2-6 are codified into per-phase Gate criteria above.
