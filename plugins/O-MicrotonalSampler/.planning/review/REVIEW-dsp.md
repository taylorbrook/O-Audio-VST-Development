# O-MicrotonalSampler DSP & Audio Thread Audit

## Executive Summary

1. **Critical bug in renderTailRamp (line 240-254)**: Early-return logic is inverted — zeros the buffers when it should render them. Guards are correct but `return;` executes during normal operation, silencing all voice-steal tails. This causes clicks/silence on rapid note-offs.

2. **Parameter null-pointer dereference in startNote (lines 486-489)**: Code reads ADSR parameters directly without null-checking `getRawParameterValue` return. If any parameter is missing from APVTS, dereferencing nullptr crashes audio thread. Should guard each access with null-checks or validate once during wiring.

3. **Ramp math division-by-zero risk in renderTailRamp (line 293)**: When `rampSamples <= 1`, denominator `(float) rampSamples` can underflow to a very small number, causing numerical instability in the ramp coefficient. Should clamp denominator or guard against rampSamples <= 1.

## Bugs (Severity-Ranked)

### 🔴 CRITICAL: renderTailRamp early-return logic is inverted
**File:** `/Users/taylorbrook/Dev/VST-development/plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp:240–254`

```cpp
if (rampSamples <= 0
    || variantLow == nullptr
    || stealTailBufferL.empty()
    || stealTailBufferR.empty())
{
    const int n = juce::jmin (rampSamples, ...);
    for (int i = 0; i < n; ++i)
    {
        stealTailBufferL[(size_t) i] = 0.0f;
        stealTailBufferR[(size_t) i] = 0.0f;
    }
    return;  // <-- BUG: returns in NORMAL case, not error case
}
```

**Why it matters:** Voice-steal tails should fade out over 5ms when a note is stolen. Instead, this code returns (silencing the tail) whenever all prerequisites are met. The guard condition catches error cases (bad pointers) but then zeros the buffers and returns anyway. On every voice steal, the tail ramp is skipped entirely, causing audible clicks or silence.

**Concrete fix:** Invert the condition OR restructure the guard. Example:
```cpp
if (rampSamples <= 0 || variantLow == nullptr || stealTailBufferL.empty() || stealTailBufferR.empty())
{
    // Clear partially (only up to buffer size)
    const int n = juce::jmin (rampSamples, ...);
    for (int i = 0; i < n; ++i) { ... }
    return;
}
// Render the tail ramp here (currently unreachable)
```
OR add `if (variantLow != nullptr && ...)` guard around the render loop.

---

### 🔴 CRITICAL: APVTS parameter null-pointer dereference in startNote
**File:** `/Users/taylorbrook/Dev/VST-development/plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp:486–489`

```cpp
if (parameters != nullptr)
{
    const float a = parameters->getRawParameterValue ("attack")->load();    // NO null check
    const float d = parameters->getRawParameterValue ("decay")->load();     // NO null check
    const float s = parameters->getRawParameterValue ("sustain")->load();   // NO null check
    const float r = parameters->getRawParameterValue ("release")->load();   // NO null check
    adsr.setParameters ({ a, d, s, r });
}
```

**Why it matters:** `getRawParameterValue` returns `nullptr` if the parameter ID is not found in APVTS. Calling `.load()` on nullptr crashes the audio thread. If an APVTS layout change or typo in parameter ID occurs, the plugin will crash on any note-on.

**Concrete fix:**
```cpp
if (parameters != nullptr)
{
    auto* ap = parameters->getRawParameterValue ("attack");
    auto* dp = parameters->getRawParameterValue ("decay");
    auto* sp = parameters->getRawParameterValue ("sustain");
    auto* rp = parameters->getRawParameterValue ("release");
    if (ap && dp && sp && rp)
        adsr.setParameters ({ ap->load(), dp->load(), sp->load(), rp->load() });
}
```

---

### 🟠 HIGH: Division-by-zero / numerical instability in renderTailRamp ramp coefficient
**File:** `/Users/taylorbrook/Dev/VST-development/plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp:293`

```cpp
for (int i = 0; i < rampSamples; ++i)
{
    const float ramp = lastEnv * (1.0f - (float) i / (float) rampSamples);
    // ...
}
```

**Why it matters:** When `rampSamples` is very small (e.g., 1 or 2 samples), the fraction `i / rampSamples` loses precision and can produce NaN or Inf with certain ADSR envelope values. The ramp becomes erratic instead of a smooth linear fade.

**Concrete fix:**
```cpp
const float rampDenom = juce::jmax (1.0f, (float) rampSamples);
for (int i = 0; i < rampSamples; ++i)
{
    const float ramp = lastEnv * (1.0f - (float) i / rampDenom);
    // ...
}
```

---

### 🟠 HIGH: Per-sample `std::pow` in voice startNote path blocks RT safety
**File:** `/Users/taylorbrook/Dev/VST-development/plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp:28`

```cpp
inline double referenceFrequencyForNote (int midiNote) noexcept
{
    return 440.0 * std::pow (2.0, (midiNote - 69) / 12.0);  // Called in startNote (line 400)
}
```

**Why it matters:** `startNote` is called from the audio thread (JUCE dispatcher). While `std::pow` is generally fast, it's transcendental and not guaranteed RT-safe on all platforms. Called once per note-on (not per-sample), so latency impact is negligible but belongs on message thread. Frequency lookup could use pre-computed 12-TET table + interpolation.

**Concrete fix:** Pre-compute 128 12-TET frequencies at initialization; look up with O(1) array access + optional fine-tuning from TuningEngine. Or accept it's acceptable since called only once per note-on (Phase 2.1 design choice).

---

## Audio Thread Violations

### ✓ SAFE: Atomic operations on RR counters (selectVariantIndex, lines 184–235)
- Uses `std::memory_order_relaxed` atomics correctly (no acquires/releases needed; counter is per-voice-per-cell).
- xorshift32 PRNG is pure integer arithmetic (RT-safe).
- **Status:** Correct pattern.

### ✓ SAFE: Shared-ptr snapshot in startNote (lines 355–359)
- Uses conditional `std::atomic_load` if C++20 available, fallback to plain copy.
- Both are RT-safe (no allocation, copy is refcount inc in atomic CPU op).
- **Status:** Correct pattern.

### ✓ SAFE: MIDI and parameter reads in processBlock (PluginProcessor.cpp:313–320)
- CC 11 (Expression) scanned per-block; `setValueNotifyingHost` is callback-safe.
- Parameter reads via `getRawParameterValue()->load()` atomics are RT-safe.
- **Status:** Correct pattern.

### ⚠ SUSPECT: TuningEngine::getFrequency called from audio thread (startNote)
- TuningEngine is global scope (`D-4` namespace); no explicit thread affinity documented.
- If TuningEngine holds locks or caches that aren't atomic, race is possible during state restore or UI Scala-file load.
- **Recommendation:** Verify TuningEngine is lock-free for `getFrequency()` reads during audio thread calls.

### ✓ SAFE: Background loader message-thread callbacks (SampleLoader.cpp:223, 237, 252, 313, 416)
- All callbacks dispatched via `juce::MessageManager::callAsync` (correct pattern).
- Audio thread never reads from loader's working buffers; only message thread updates `currentSampleMap` via atomic-store.
- **Status:** Correct pattern.

---

## DSP Correctness Concerns

### Cubic Hermite interpolation at loop boundaries (lines 49–102)
**Assessment:** Correct.
- Clamping reads outside buffer bounds (line 56: `juce::jlimit(0, N-1, idx)`).
- Cubic polynomial applied per sample (standard implementation).
- Loop fade uses 8-sample xfade LUT (line 96–101) with equal-power weights (line 31–35).
- **Minor note:** 8-sample fade is short; will have audible phase discontinuity if loop is not seamless. Acceptable for synthetic samples but watch for loop-region user edits.

### Pitch ratio math (lines 113–123)
**Assessment:** Correct.
- `playRateForVariant = (desiredFreq / cellRefFreq) × (cellSR / hostSR)`.
- Per-cell MIDI note maps to 12-TET baseline.
- Tuning engine delta applied post-baseline (line 405).
- **Status:** Correctly compensates for mismatched source sample rate and tuning.

### Velocity layer crossfade (lines 409–453)
**Assessment:** Mostly correct, minor inefficiency.
- Equal-power panning (line 444: `equalPowerWeights(x)`).
- Crossfade region computed per-note based on velocity and velocity_crossfade parameter.
- **Inefficiency:** Parameter read inline (line 412–413) on every note-on. Should be cached or updated per-block. Low-impact (once per note-on) but not optimal.

### ADSR envelope time-to-coefficient conversion
**Assessment:** Assumed correct (delegated to juce::ADSR).
- Voice calls `adsr.setParameters(...)` with time values (seconds) from APVTS.
- JUCE's ADSR converts to per-sample coefficient via `setSampleRate`.
- **Caveat:** If APVTS parameter range doesn't match JUCE ADSR expectations, envelope will sound wrong. Verify layout (0–10s range, 0.005s default attack).

### Frequency-to-cents / ratio math
**Assessment:** Not explicitly in voice code (delegated to TuningEngine + NoteExpression).
- Voice calls `tuningEngine->getFrequency(midiNote)` and applies `NoteExpression::applyPendingTuning()`.
- These are module-external; assume correct if modules pass their own tests.
- **Concern:** No validation of returned frequencies (could be 0, negative, or NaN if module buggy). Should guard: `if (currentFrequency > 0.0) { ... }`.

### Stereo panning law (lines 627–628, 314–315)
**Assessment:** Correct (equal-power, no pan parameter—just layer crossfade).
- `yL = (lLow * layerWeightLow + lHigh * layerWeightHigh) * env` (simple sum, not panning).
- Layer weights are equal-power (angle-based) per `equalPowerWeights`.
- No per-voice pan; only velocity-layer crossfade.
- **Status:** Appropriate for sampler use case.

### Denormal handling (line 528)
**Assessment:** Present.
- `juce::ScopedNoDenormals noDenormals;` guards renderNextBlock.
- renderTailRamp is called from startNote (message thread context), not from renderNextBlock—so denormals not guarded there. **Minor issue:** If renderTailRamp runs on audio thread (via startNote on voice allocation), denormals should be guarded. Verify call context.

---

## Efficiency Concerns

### Per-sample parameter reads (avoid)
- **Non-issue:** ADSR parameters read once per startNote (line 486–489), not per-sample. ✓

### Per-sample velocity-layer lookup (non-issue)
- layerWeightLow/High set in startNote, not updated per-sample. ✓

### Inline equal-power weights in startNote hot path (line 444)
- Called once per note-on on condition (velocity crossfade enabled + near layer boundary). Acceptable.

### Loop position wrapping (lines 104–111, 321, 325, 634, 638)
**Assessment:** Correct but simplistic.
```cpp
static inline void wrapLoopPosition (double& pos, int lpStart, int lpEnd) noexcept
{
    if (lpEnd <= 0) return;
    const int lpLen = lpEnd - lpStart;
    if (lpLen <= 0) return;
    while (pos >= (double) lpEnd)  // Linear loop, O(N) worst-case
        pos -= (double) lpLen;
}
```
**Efficiency:** If `playRate > loopLen`, the while loop can iterate many times (e.g., high-pitch shift on short sample). Standard approach but consider modulo for large jumps:
```cpp
if (pos >= lpEnd) {
    double shifted = pos - lpStart;
    shifted = shifted - lpLen * std::floor((shifted) / lpLen);
    pos = lpStart + shifted;
}
```
**Impact:** Low; loop samples are typically longer than pitch-shift playRate, so while-loop runs 0–1 times per sample. Acceptable.

### Sample map cell lookup (lines 142–162 in SampleMap.h)
**Assessment:** Linear scan, O(N) per findCell call.
- Called once per note-on (non-hot path).
- Num cells ~128 (typical), negligible latency.
- Could optimize with map/tree if library grows >1000 notes, but N=128 is fine.

### RR counter atomic operations (per note-on, not per-sample)
- xorshift32 + modulo per variant selection.
- Called once at startNote, not in render loop.
- ✓ Correct.

### Cubic interpolation in hot path (per sample, per voice)
- 4 buffer reads + 12 FLOPs per sample, per voice.
- Standard cubic-Hermite cost; unavoidable for resampling.
- Cache locality: reads 4 consecutive (or nearly) float* pointers; good.
- ✓ Acceptable for polyphony up to 16 voices @ 48kHz.

### Expression & output-gain smoothing (processBlock, lines 338–368)
**Assessment:** Efficient.
- LinearSmoothedValue ramp applied per-block via `applyGainRamp`.
- No per-sample parameter reads; smoothed target set once per block.
- ✓ Correct pattern.

---

## Voice Management

### Voice stealing with tail-ramp (lines 337–346, 531–551)
**Assessment:** Logic is sound, but renderTailRamp bug (line 240) breaks execution.
- On startNote, if voice is active (adsr.isActive()), captures tail ramp.
- Tail ramp rendered at ~5ms fade (line 150: `0.005 * sampleRate + 16`).
- Mixed into output next block via stealTailBufferL/R.
- **Bug:** renderTailRamp returns early (line 254), so tail is never written. Tail disappears.
- **Fix:** Correct renderTailRamp early-return logic (see bug #1).

### Voice-on/off and note expression race
**Assessment:** Safe.
- startNote and stopNote are serialized by JUCE synthesiser (no concurrent access).
- NoteExpression tuning table is drained/updated before synthesiser::renderNextBlock (line 306).
- ✓ No race.

### Voice limit enforcement (line 326–327)
**Assessment:** Correct.
- Polyphony APVTS parameter read atomic; passed to `synthesiser.setVoiceCap()` before MIDI dispatch.
- JUCE's voice stealer respects cap.
- ✓ Correct.

### Voice state isolation
**Assessment:** Correct.
- Per-voice state: posLow, posHigh, playRateLow, playRateHigh, layerWeightLow/High, variantLow/High, currentMidiNote.
- No shared mutable state except rrCounters (atomic) and currentSampleMap (shared_ptr, atomic-store).
- ✓ Correct.

---

## Sample Loader

### Memory bounds on library size
**Assessment:** Unbounded.
- SampleLoader allocates one juce::AudioBuffer per sample file (line 157: `std::make_shared<AudioBuffer>`).
- No cap on total library size or buffer count.
- Loading a 50GB folder will allocate 50GB of RAM (or OOM crash).
- **Risk:** High for user with large libraries. Consider:
  - Warn if library > 1GB, suggest streaming or truncation.
  - OR implement memory quota / eviction.
  - OR document "max 4GB library per instance".

### Background loader synchronization (SampleLoader::run)
**Assessment:** Correct.
- Loader thread enumerates folder, loads files, builds SampleMap.
- Completion dispatched via `MessageManager::callAsync` (line 416).
- Message thread atomically stores result into currentSampleMap (PluginProcessor.cpp:567).
- Voices snapshot shared_ptr (startNote, line 356–359); refcount inc is atomic.
- ✓ Correct pattern.

### Cancellation race (stopThread, lines 36, 56, 74)
**Assessment:** Safe.
- `stopThread(500)` called before starting new load (e.g., user drops new folder while loading old one).
- JUCE Thread::stopThread waits up to timeout for thread exit, then kills it.
- If loader completes after kill, message callback still fires but processor may ignore it (safe because loadFolder already called stopThread).
- ✓ Safe, but could be tighter: consider an atomic `loadGeneration` counter to ignore stale callbacks.

### Per-cell single-file load (loadSingleVariant, lines 50–79)
**Assessment:** Correct.
- Loads one file, populates one SampleVariant.
- Completion via callback (line 237–240).
- Used for Stage 3 loop-editor cell replace.
- ✓ Correct pattern.

### Duplicate detection (SampleLoader.cpp lines 361–381)
**Assessment:** Correct.
- Detects bare duplicates (same MIDI + layer, no rr/take/tk tokens).
- Stages map + returns AmbiguousDuplicate list to processor (line 415).
- Processor surfaces modal confirmation (PluginProcessor.cpp:442–443).
- User confirms via confirmRoundRobinLoad (PluginProcessor.cpp:589).
- ✓ Correct pattern.

### Loop-point defaults
**Assessment:** Correct.
- If sample >= 18 frames: loopEnd = N - 2 (slightly before EOF to avoid cubic context overshoot).
- If sample < 18 frames: loopMode = OneShot (too short for safe cubic context).
- ✓ Matches renderNextBlock cubic-interp headroom requirement.

---

## Frequency Conversion & Microtonal Math

### 12-TET baseline (referenceFrequencyForNote, line 26–29)
**Assessment:** Correct.
- `freq = 440 * 2^((note - 69) / 12)` is standard A4 = 440 Hz baseline.
- 69 = A4 MIDI note.
- ✓ Correct.

### TuningEngine frequency lookup (line 399–401, 405–406)
**Assessment:** Delegated; assumed correct if TuningEngine passes its tests.
- Voice queries tuning engine, applies note-expression delta.
- No validation of returned frequency (could be 0, negative, or NaN).
- **Recommendation:** Guard: `if (currentFrequency > 0.0 && currentFrequency < 20000.0) { playRate = ... } else { variantLow = nullptr; return; }`.

### Play rate computation for variant (lines 113–123)
**Assessment:** Correct.
- Accounts for cell MIDI note, source sample rate, desired tuning frequency, and host sample rate.
- Formula: `playRate = (desiredFreq / cellRefFreq) × (srcSR / hostSR)`.
- ✓ Correct.

---

## Summary Table

| Issue | File:Line | Severity | Category | Status |
|-------|-----------|----------|----------|--------|
| renderTailRamp early-return | Voice.cpp:240 | CRITICAL | DSP Correctness | Inverted guard |
| APVTS param null deref | Voice.cpp:486 | CRITICAL | Audio Thread | No guard |
| Ramp coeff division | Voice.cpp:293 | HIGH | Numerical | Underflow risk |
| std::pow in startNote | Voice.cpp:28 | HIGH | Efficiency | Per-note, acceptable |
| renderTailRamp denormals | Voice.cpp:238 | MEDIUM | Audio Thread | Missing guard |
| Frequency validation | Voice.cpp:399 | MEDIUM | DSP Correctness | No bounds check |
| Memory unbounded | SampleLoader.cpp | MEDIUM | Resource Mgmt | No cap |
| Loop wrap efficiency | Voice.cpp:104 | LOW | Efficiency | Linear loop, rare |
| SampleMap lookup | SampleMap.h:142 | LOW | Efficiency | O(N), acceptable |

---

## Recommended Actions (Priority Order)

1. **Fix renderTailRamp early-return** (line 240–254): Invert condition or restructure guard.
2. **Guard APVTS parameter reads** (line 486–489): Null-check each `getRawParameterValue` return.
3. **Clamp ramp denominator** (line 293): Prevent division by small numbers.
4. **Document memory limit**: Warn users about unbounded library size; suggest streaming or per-instance cap.
5. **Validate tuning frequencies**: Guard against 0, negative, or NaN from TuningEngine.
6. **Optional: Pre-compute 12-TET table** instead of per-note std::pow (Phase 2.1 optimization).

---

## Overall Assessment

**Audio Thread:** Clean. Atomic operations, shared-ptr, and message-thread callbacks are correct. Two critical null-pointer dereferences are the main risk.

**DSP:** Mostly sound. Cubic interpolation, equal-power panning, and loop math are correct. renderTailRamp bug breaks voice-steal functionality entirely.

**Performance:** Good. Per-sample work is minimal (cubic interp + mix + env). Per-block smoothing for gain parameters. Pre-allocated voice pool avoids allocation during playback.

**Scalability:** Library size unbounded; warn users. Voice limit enforced. Sample map lookup is O(N) but acceptable for typical 128-note libraries.
