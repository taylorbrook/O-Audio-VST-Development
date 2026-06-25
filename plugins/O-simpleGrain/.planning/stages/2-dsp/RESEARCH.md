# O-simpleGrain — Stage 2 (DSP) Research

**Researched:** 2026-06-24
**Type:** EXTRACTION research (architecture is LOCKED — see `research/ARCHITECTURE.md`). This document mines shipped in-repo plugins for copy-adaptable reference code so the dsp-agent implements the locked contract without reinventing JUCE 8 APIs.
**Confidence:** HIGH (all source cited from local files; JUCE 8.0.9 signatures verified against `/Users/taylorbrook/JUCE/modules/`).

> ⚠️ **Do not redesign.** All DSP decisions are pinned in ARCHITECTURE.md / CONTEXT.md / ROADMAP.md. This is a "where's the code, what's the exact signature, what's the gotcha" reference.

---

## 1. Summary — reuse vs. new, and the 3-sub-phase map

| Layer | Source (verbatim-ish) | New integration work |
|-------|----------------------|----------------------|
| Voice model (`Synthesiser`+custom `SynthesiserVoice`, amp `ADSR`, `setParams` block-push, non-virtual `prepareToPlay`, voice lifetime) | **O-simpleFM `FMVoice.h` / `PluginProcessor.cpp`** | Marry the voice to a per-voice grain pool + scheduler |
| Grain pool (preallocated array, round-robin/steal-oldest spawn, equal-power pan, overlap-add) | **O-GrainScatter `GrainPool.h`** | Read from a **static source buffer + global playhead** instead of a live `DelayBuffer` (the key delta — see §2) |
| Scheduler (per-sample countdown) | **O-GrainScatter `GrainScheduler.h`** | Period = `fs/density` (g/s) instead of GrainScatter's exponential density%; scatter jitters the period |
| Fractional read | **O-GrainScatter `LagrangeInterpolation.h`** (`lagrangeInterpolate`, copy verbatim) | Random-access read into source buffer + per-grain rate-tracking one-pole AA (§5) |
| Window LUTs | `GrainPool::computeEnvelope` shape math (adapt 5 shapes) | Precompute into 5× `std::array<float,2048>` (currently GrainScatter computes per-sample with transcendentals) |
| Freeze | **O-GrainScatter `FreezeManager.h`** (pin + 5 ms crossfade gain) | Adapt from "capture-region-of-delay-buffer" to "pin the global playhead position" (§4) |
| Viz: samples→scope/FFT | **O-simpleFM `FmVizAnalyzer.h`** (`VizRing`+`FmVizAnalyzer`, copy verbatim) | Lift unchanged |
| Viz: grain events | **O-GrainScatter `TripleBuffer.h`** (copy verbatim) | Define `GrainCloudFrame` struct (§7) |
| Sample loading: embed+decode+resample, atomic hot-swap, drag-drop streaming + Base64 | **O-MicrotonalSampler `PluginProcessor.cpp` / `webview-drop-streaming.js`** | `juce_add_binary_data` target (no in-repo audio-embed precedent — §6) |

**Sub-phase mapping (ROADMAP order):**
- **2.1** — voice model (§3) + grain pool/scheduler/overlap-add (§2) + window LUTs (§2.4) + amp ADSR + key resample + **single embedded `fire.wav` decode** (§6 minimal). Processor advances a position-only playhead.
- **2.2** — global read head scan/freeze (§4) + per-voice RNG spray/scatter + anti-aliasing one-pole (§5) + `velToDensity` + `SmoothedValue` on scan/position/output.
- **2.3** — full sample loading: embed all 4 + decode/resample + atomic hot-swap + drag-drop streaming/picker (§6) + the three viz taps (§7).

**Primary recommendation:** Copy `LagrangeInterpolation.h`, `TripleBuffer.h`, `FmVizAnalyzer.h` **verbatim** into `Source/dsp/`. Copy the *structure* of `GrainPool.h` spawn/steal-oldest and `FMVoice.h` voice skeleton, but **rewrite the per-grain read loop** because O-GrainScatter reads a live `DelayBuffer` (effect), whereas O-simpleGrain reads a static source buffer via a global playhead (synth) — this is the single most important delta.

### ⚠️ Surprise / risk surfaced during extraction
**O-GrainScatter is a granular *effect* (live-input delay-line granulator), NOT a sample-playback synth.** Its `GrainPool::processSample(const DelayBuffer&, const FreezeManager&, ...)` reads from a circular delay buffer of incoming audio, computing a self-cancelling delay tap (`positionOffset + elapsed - readPosition`). O-simpleGrain instead reads a **fixed source `AudioBuffer<float>` at `grain.readPos` relative to a global playhead**. So: the spawn/steal-oldest allocation logic, the equal-power pan, the envelope-by-phase, and the overlap-add summation transfer directly; the **read addressing does NOT** — the dsp-agent must implement `readSourceLagrange(readPos)` against the source buffer, not `delayBuf.readSample(...)`. ARCHITECTURE.md §Core Components already specifies the correct grain loop (`readPos += rate; phase += 1/lengthSamples`) — follow the architecture's loop, not GrainScatter's `samplesRemaining` countdown read.

---

## 2. Grain engine reference

### 2.1 Grain struct fields (O-simpleGrain needs)
ARCHITECTURE.md's grain loop drives these fields. Note GrainScatter uses a `samplesRemaining` countdown + `phase = 1 - remaining/length`; the architecture specifies a forward `phase += 1/lengthSamples`. **Use the architecture's forward-phase model** (cleaner for a sample-read synth):

```cpp
struct Grain {
    bool   active        = false;
    float  readPos       = 0.0f;   // absolute position in SOURCE buffer (samples), fractional
    float  rate          = 1.0f;   // combined read increment = voiceRate * 2^((grainPitch+spray)/12)
    float  phase         = 0.0f;   // window phase 0..1
    float  phaseInc      = 0.0f;   // = 1.0f / lengthSamples
    float  lengthSamples = 0.0f;   // grainSize ms * fs
    float  pan           = 0.5f;   // 0=L .. 1=R (equal-power)
    int    shape         = 4;      // window LUT index (rect/tri/Welch/Gauss/Hann)
    int    age           = 0;      // ++ per sample — for steal-oldest
    // AA one-pole state (per grain — §5):
    float  aaState       = 0.0f;
};
```

`std::array<Grain, kMaxGrainsPerVoice /*24*/>` per voice — **preallocated, never resized** (constants already in `PluginProcessor.h`: `kMaxGrainsPerVoice=24`, `kGlobalGrainCap=192`).

### 2.2 Pool allocation: round-robin / steal-oldest
**Source:** `O-GrainScatter/Source/dsp/GrainPool.h:111-165` — copy the *shape* of this verbatim (find inactive slot, else steal oldest). For O-simpleGrain "oldest" = highest `age` (or, equivalently, largest `phase`):

```cpp
// O-GrainScatter GrainPool::spawnGrain (lines 111-138), adapted to age-based steal:
int spawnGrain(/* init params */) {
    int target = -1, oldest = 0, maxAge = -1;
    for (int i = 0; i < kMaxGrainsPerVoice; ++i) {
        int idx = (nextGrain + i) % kMaxGrainsPerVoice;
        if (!grains[idx].active) { target = idx; break; }
        if (grains[idx].age > maxAge) { maxAge = grains[idx].age; oldest = idx; }
    }
    if (target < 0) target = oldest;          // steal oldest — never allocates, never xruns
    auto& g = grains[target];
    g.active = true; g.age = 0; g.phase = 0.0f;
    g.readPos = grainReadStart;               // playheadPos + positionSprayRand (§4)
    g.rate = combinedRate; g.lengthSamples = lenSamp; g.phaseInc = 1.0f / lenSamp;
    g.pan = grainPan; g.shape = windowShape; g.aaState = 0.0f;
    nextGrain = (target + 1) % kMaxGrainsPerVoice;
    return target;
}
```
This is the PERF-02 guarantee: a bounded array + steal-oldest means `processBlock` never allocates and high density × size × poly thins the cloud instead of xrunning.

### 2.3 Per-grain read + overlap-add loop (the rewritten core)
**Architecture-specified** (ARCHITECTURE.md §Core Components / §Processing Chain). Equal-power pan + envelope math from `GrainPool::processSample` (lines 184-228); the source read replaces `delayBuf.readSample`:

```cpp
// Per active grain, per sample:
float env = windowLut[g.shape].read(g.phase);          // §2.4 LUT, linear-interp
float src = readSourceLagrange(g.readPos);             // §2.5 — into the source buffer
src = aaOnePole(src, g.rate, g.aaState);               // §5 — band-limit if rate>1
float s = src * env;
float panL = std::cos(g.pan * juce::MathConstants<float>::halfPi);   // GrainPool.h:213-214
float panR = std::sin(g.pan * juce::MathConstants<float>::halfPi);
outL += s * panL;  outR += s * panR;                   // overlap-add summation
g.readPos += g.rate;  g.phase += g.phaseInc;  ++g.age;
if (g.phase >= 1.0f) { g.active = false; }              // grain done
```
Voice output: `(outL,outR) * ampEnv.getNextSample()`; voice lifetime gated on `ampEnv.isActive()`.

### 2.4 Window LUTs (5 shapes, precomputed 2048-pt)
**Source for the shape math:** `GrainPool::computeEnvelope` (lines 70-109). GrainScatter computes per-sample with transcendentals; **O-simpleGrain precomputes** into LUTs (`kWindowLutSize=2048` already declared). ARCHITECTURE.md §Window LUTs locks the 5 class shapes (note: these differ from GrainScatter's 6 — use the architecture's set):

```cpp
// phase φ ∈ [0,1]; precompute each into std::array<float, 2048> at construction:
rect : 1.0f                                                        // clicks — INTENDED artifact (DSP-03)
tri  : 1.0f - std::abs(2.0f*φ - 1.0f)
welch: 1.0f - (2.0f*φ - 1.0f)*(2.0f*φ - 1.0f)                      // parabolic
gauss: std::exp(-0.5f * ((φ-0.5f)/σ)*((φ-0.5f)/σ)), σ≈0.18f         // normalize to 1.0 at center
hann : 0.5f * (1.0f - std::cos(twoPi * φ))                         // GrainPool.h:76 (default)
```
Read with linear interpolation by grain phase — no per-sample transcendental in the grain loop (the O-simpleAdditive table rationale). The window-inset (UI-03, Stage 3) draws the selected LUT.

### 2.5 Lagrange fractional read — `LagrangeInterpolation.h` (COPY VERBATIM)
**Source:** `O-GrainScatter/Source/dsp/LagrangeInterpolation.h:6-14`. 4-point (3rd-order), random-access, stateless — exactly correct for grains reading arbitrary source positions (the stateful `juce::LagrangeInterpolator` streaming class is **unsuitable** — see §8):

```cpp
inline float lagrangeInterpolate (float ym1, float y0, float y1, float y2, float frac) {
    float c0 = y0;
    float c1 = y1 - (1.0f/3.0f)*ym1 - 0.5f*y0 - (1.0f/6.0f)*y2;
    float c2 = 0.5f*(ym1 + y1) - y0;
    float c3 = (1.0f/6.0f)*(y2 - ym1) + 0.5f*(y0 - y1);
    return ((c3*frac + c2)*frac + c1)*frac + c0;
}
```
Read helper (architecture pattern; mirror `FreezeManager::readSample` lines 44-64 for the index/frac/clamp shape, but **clamp** at source bounds rather than wrap, since a grain that runs off the end should taper out via its window):

```cpp
float readSourceLagrange(const float* src, int len, float pos) {
    int i0 = (int) pos;  float frac = pos - (float) i0;
    int im1 = juce::jlimit(0, len-1, i0-1);
    int ip0 = juce::jlimit(0, len-1, i0);
    int ip1 = juce::jlimit(0, len-1, i0+1);
    int ip2 = juce::jlimit(0, len-1, i0+2);
    return lagrangeInterpolate(src[im1], src[ip0], src[ip1], src[ip2], frac);
}
```

### 2.6 Scheduler — `GrainScheduler.h` countdown (adapt period formula)
**Source:** `O-GrainScatter/Source/dsp/GrainScheduler.h:22-39` (`processBlockFree`). Reuse the **per-sample countdown** mechanic; **replace** GrainScatter's exponential density%→ms mapping with the architecture's `fs/density` (g/s) + scatter jitter. ARCHITECTURE.md §Granular Voice gives the exact form:

```cpp
// per voice, per sample (scheduler lives in the voice, not the processor):
float baseInterval = sampleRate / effectiveDensity;               // effectiveDensity = density·velToDensity, clamped [1,200]
float jitter = (scatter/100.f) * baseInterval * (rng.nextFloat()*2.f - 1.f);   // scatter → async
int nextInterval = juce::jmax(1, (int)(baseInterval + jitter));
if (--samplesUntilNextGrain <= 0) { spawnGrain(...); samplesUntilNextGrain = nextInterval; }
```
GrainScatter's `samplesUntilNextGrain` countdown + `juce::Random rng` member (lines 31-38, 138) transfer directly. **Per-voice `juce::Random`** — no shared RNG, no alloc, no lock (real-time safe — §8).

### 2.7 Processor wiring (scheduler→spawn→overlap-add)
**Reference:** O-GrainScatter `PluginProcessor.cpp:475-570` shows the block structure (compute spawn requests → per-sample loop → spawn at offset → process). But for O-simpleGrain the loop lives **inside each `GrainVoice::renderNextBlock`** (since it's a `Synthesiser` synth, not an effect), and the **processor only**: (a) reads APVTS once/block → `voice->setParams(...)`, (b) **advances the global playhead per sample** (§4), (c) calls `synth.renderNextBlock`. Mirror O-simpleFM `PluginProcessor.cpp:189-209` (`pushParamsToVoices`) and `:220-341` (`processBlock` skeleton — but **without** the FM oversampler; O-simpleGrain is zero-latency).

---

## 3. Voice model reference (FMVoice → GrainVoice)

**Source:** `O-simpleFM/Source/FMVoice.h` (full) + `PluginProcessor.cpp:106-209`.

### 3.1 JUCE 8 no-virtual-prepareToPlay (the load-bearing pattern)
`juce::SynthesiserVoice` has **NO virtual `prepareToPlay`** in JUCE 8. O-simpleFM declares a **non-virtual** custom `prepareToPlay(double, int)` and the processor dispatches it via `dynamic_cast` (`FMVoice.h:48-63`, `PluginProcessor.cpp:156-159`):

```cpp
// FMVoice.h:48 — non-virtual; setSampleRate MUST precede ADSR setParameters:
void prepareToPlay (double sr, int /*maxBlockSize*/) {
    setCurrentPlaybackSampleRate (sr);     // SynthesiserVoice base method
    sampleRate = sr;
    ampEnv.setSampleRate (sr);             // ADSR setSampleRate BEFORE setParameters (§8 gotcha)
    ampEnv.setParameters (ampParams);
    // ... reset SmoothedValues at sr here ...
}

// PluginProcessor.cpp:156-159 — processor dispatches via dynamic_cast:
synth.setCurrentPlaybackSampleRate (rate);
for (int v = 0; v < synth.getNumVoices(); ++v)
    if (auto* gv = dynamic_cast<GrainVoice*> (synth.getVoice (v)))
        gv->prepareToPlay (rate, samplesPerBlock);
```

### 3.2 Sound + voice canPlaySound
`FMVoice.h:27-44` — copy verbatim, rename to `GrainSound` / `GrainVoice`:
```cpp
class GrainSound : public juce::SynthesiserSound {
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};
// in GrainVoice:
bool canPlaySound(juce::SynthesiserSound* s) override { return dynamic_cast<GrainSound*>(s) != nullptr; }
```

### 3.3 setParams block-push
`FMVoice.h:67-94` — block-push pattern: processor reads APVTS once/block, calls `voice->setParams(...)`. Voices **never** touch APVTS. For O-simpleGrain push: grainSize, density, windowShape, pitchSpray, positionSpray, scatter, grainPitch, panSpray, velToDensity, and the `juce::ADSR::Parameters` amp struct. Smooth where needed via `SmoothedValue` (`FMVoice.h:238-241` shows the member pattern, `:58-62` the reset).

### 3.4 startNote / stopNote / voice lifetime
`FMVoice.h:97-131` + `:139-197`. Key points for O-simpleGrain:
- `startNote`: compute `voiceRate = 2^((midiNote - kRootNote/*60*/)/12)`; **clear all grains** (`active=false`); reset scheduler countdown; capture velocity; `ampEnv.noteOn()`.
- `stopNote(allowTailOff)`: `allowTailOff ? ampEnv.noteOff() : (clearCurrentNote(); ampEnv.reset())` (`FMVoice.h:117-131`).
- **Voice lifetime gated on `ampEnv.isActive()` ONLY** (`FMVoice.h:139-140, 192-196`): `renderNextBlock` early-returns if `!ampEnv.isActive()`, and calls `clearCurrentNote()` when the amp env goes inactive. Freeze sustains by pinning the global playhead while the note is held — the voice stays alive because the note is held (amp env in sustain).
- Velocity→amp: fold `velLevel` into the per-sample output (`FMVoice.h:184` — `sample * ampEnvVal * velLevel`).

### 3.5 Synthesiser setup (processor ctor + prepare)
`O-simpleFM PluginProcessor.cpp:112-117` (ctor) and `:154-160` (prepare):
```cpp
// ctor — preallocate all voices, no audio-thread alloc later:
for (int i = 0; i < kMaxVoices /*8*/; ++i) synth.addVoice(new GrainVoice());
synth.addSound(new GrainSound());
synth.setNoteStealingEnabled(true);          // note-stealing = Synthesiser default (steal quietest/oldest)
```
`isBusesLayoutSupported` for a synth (output-only, mono/stereo, no input bus): copy `O-simpleFM PluginProcessor.cpp:173-187` verbatim. CMake already needs `IS_SYNTH TRUE` + `NEEDS_MIDI_INPUT TRUE` (Stage 1 / Stage 3.1) — without them the plugin is silent.

---

## 4. Read head / freeze reference

**Source:** `O-GrainScatter/Source/dsp/FreezeManager.h` (full) — but **adapt**, don't copy verbatim. GrainScatter's FreezeManager captures a region of the live delay buffer into a separate `freezeBuffer` and grains read *that*. O-simpleGrain's source is **already** a static buffer, so freeze is simpler: **pin the global playhead's velocity to 0** and crossfade the transition.

### 4.1 Global playhead (processor-owned)
Per ARCHITECTURE.md §Source Buffer + Read Head — advanced **in the processor**, per sample, shared by all voices:
```cpp
// processor, per sample (computed in processBlock BEFORE/around synth.renderNextBlock,
// or exposed to voices via a pointer/atomic the voices read at spawn time):
float vel = freezeActive ? 0.0f : (scan/100.f) * 1.0f;   // realtime-relative; scan ∈ [-200,+200]%
playheadVelocity.setTargetValue(vel);                     // SmoothedValue — no hard jump (QUAL-01)
playheadPos += playheadVelocity.getNextValue();
// wrap to [0, sourceLen):
if (playheadPos >= sourceLen) playheadPos -= sourceLen;
if (playheadPos < 0.0f)       playheadPos += sourceLen;
```
`position` sets the **resting point** (`positionAbsolute = position/100 * sourceLen`); grains spawn at `grainReadStart = positionAbsolute (blended w/ playhead) + positionSprayRand`. Grains read **offsets relative to the playhead** — the playhead is captured at *spawn* time into `grain.readPos`, then each grain advances independently by its `rate`.

### 4.2 Freeze pin + crossfade (the QUAL-01 click-free pattern)
**Reuse the crossfade-gain mechanic** from `FreezeManager.h:15, 67-85`:
- `crossfadeSamples = (int)(sampleRate * 0.005)` (~5 ms) — `FreezeManager.h:15`.
- On engage/disengage, ramp a gain 0→1 / 1→0 via `getCrossfadeGain()` (`FreezeManager.h:67-72`) and `advanceCrossfade()` (`:74-85`).
- For O-simpleGrain the simplest robust approach: **`SmoothedValue<float>` on `playheadVelocity`** (snaps to 0 on freeze, ramps back on unfreeze) + never reset grain phase mid-grain. ARCHITECTURE.md §Read head explicitly: "smoothed crossfade on engage/disengage so freeze→unfreeze is click-free."
- `freeze` is a **bool param** read via `getToggleState` on the UI side (Stage 3); in DSP it's `freezeParam->load() > 0.5f`.

---

## 5. Anti-aliasing reference

**Two parts** (ARCHITECTURE.md §Anti-Aliasing, decision #3, DSP-08):

1. **4-point Lagrange fractional read** (§2.5) — handles the fractional-position reads cleanly. Covers most aliasing for moderate transposition.

2. **Per-grain rate-tracking one-pole low-pass** — applied **only when `rate > 1`** (up-transposition); bypassed at `rate ≤ 1` (no down-transposition aliasing). Concrete coefficient formula (ARCHITECTURE.md §Algorithm Details → Anti-aliasing):

```cpp
// fc ≈ 0.5*fs / rate  (Nyquist of the SOURCE scaled by playback rate).
// One-pole LP:  y += g*(x - y),  g = 1 - exp(-2π*fc/fs).
// Substituting fc = 0.5*fs/rate:
float aaOnePole(float x, float rate, float& state) {
    if (rate <= 1.0f) { state = x; return x; }              // bypass — no up-transposition
    float fc = 0.5f * sampleRate / rate;                    // tracks playback rate
    float g  = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * fc / sampleRate);
    state += g * (x - state);
    return state;
}
```
`state` is the per-grain `aaState` field (§2.1), reset to 0 (or to first sample) on spawn. **Fallback** (documented, do NOT implement unless §9 dullness appears): interpolation-only, or whole-engine 2× oversampling (adds latency — would then require `setLatencySamples(N)`).

> Note: `g = 1 - exp(-2π·fc/fs)` is the accurate one-pole coefficient. The cheaper `g = 2π·fc/fs` (clamped <1) is acceptable for the modest cutoffs here, but `exp` is fine off the hot path's tightest budget (one `exp` per active grain per sample is ~192 `exp/sample` worst case — acceptable; if profiling flags it, precompute g vs. rate into a small LUT or use the cheap approximation).

---

## 6. Sample loading reference

### 6.1 Embedded built-ins — `juce_add_binary_data` (Stage 2.3; minimal `fire` in 2.1)
**No in-repo plugin embeds *audio*, but `juce_add_binary_data` accepts any file type** — confirmed by ONNX-model and HTML/JS embedding in `O-Texture/CMakeLists.txt:86-116`. Exact CMake pattern (mirror O-Texture's binary-data target + link order):

```cmake
# AFTER juce_generate_juce_header(${PROJECT_NAME}) — order matters:
juce_add_binary_data(${PROJECT_NAME}_Samples
    NAMESPACE BinaryData                          # default; yields BinaryData::fire_wav etc.
    HEADER_NAME BinaryData.h
    SOURCES
        Source/samples/fire.wav
        Source/samples/voice.wav
        Source/samples/water.wav
        Source/samples/piano.wav)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_Samples)
```
**Resulting symbols:** `BinaryData::fire_wav` (`const char*`), `BinaryData::fire_wavSize` (`int`), and likewise `voice_wav`, `water_wav`, `piano_wav`. (Filename `fire.wav` → identifier `fire_wav`.) `[VERIFIED: O-Texture/CMakeLists.txt + JUCE 8.0.9 juce_add_binary_data]`

**Asset facts (verified via `afinfo`):** all 4 are mono / 44100 Hz / 24-bit PCM, 2.2–3.0 s — within `kMaxSourceSeconds=10`. They decode through `WavAudioFormat`.

### 6.2 Decode + resample path
```cpp
juce::AudioFormatManager fmt;
fmt.registerBasicFormats();                       // WAV/AIFF/FLAC  [VERIFIED: juce_AudioFormatManager.h:80]

// createReaderFor(unique_ptr<InputStream>) overload  [VERIFIED: juce_AudioFormatManager.h:149]
std::unique_ptr<juce::AudioFormatReader> reader(
    fmt.createReaderFor(std::make_unique<juce::MemoryInputStream>(
        BinaryData::fire_wav, (size_t) BinaryData::fire_wavSize, /*keepInternalCopy=*/false)));
// MemoryInputStream(const void*, size_t, bool)  [VERIFIED: juce_MemoryInputStream.h:61]

if (reader != nullptr) {
    int nCh  = (int) reader->numChannels;          // public field [VERIFIED: juce_AudioFormatReader.h:243]
    int nSmp = (int) reader->lengthInSamples;      //              [VERIFIED: :240]
    double srcRate = reader->sampleRate;           //              [VERIFIED: :234]
    juce::AudioBuffer<float> tmp(nCh, nSmp);
    reader->read(&tmp, 0, nSmp, 0, true, true);    // read(AudioBuffer<float>*, ...) [VERIFIED: :156]
    // resample tmp (srcRate) -> engineRate into the published source buffer (§6.3)
}
```
**Resample to engine rate** (do this OFF the audio thread, at construction / on selection). Recommended: `juce::LagrangeInterpolator::process(speedRatio, in, out, numOut)` per channel — `speedRatio = srcRate / engineRate`, `numOut = nSmp * engineRate / srcRate`, capped to `kMaxSourceSeconds * engineRate`. `[VERIFIED: juce_GenericInterpolator.h:101 — int process(double speedRatio, const float* in, float* out, int numOut)]`. `LagrangeInterpolator` is the `using` alias for `Interpolators::Lagrange` `[VERIFIED: juce_Interpolators.h:200]`. (Offline ratio-resample is the *streaming* resampler used here in a one-shot — correct usage, unlike per-grain random access where it would be wrong; see §8.)

> **Note for 2.1:** the four .wav files are 44.1 kHz; if the engine runs at 48 kHz the source must be resampled. Decode the single default (`fire`) at construction in 2.1; the full 4-source embed + `sourceSample` choice + hot-swap lands in 2.3.

### 6.3 Atomic hot-swap (publish source buffer to audio thread)
**Source:** `O-MicrotonalSampler/Source/PluginProcessor.cpp:32-42` — copy-on-write via `std::atomic_load`/`std::atomic_store` on a `std::shared_ptr`:

```cpp
// O-MicrotonalSampler atomicLoad/atomicStore wrappers (PluginProcessor.cpp:32-42):
template <class T> std::shared_ptr<T> atomicLoad(const std::shared_ptr<T>& s) noexcept
    { return std::atomic_load(&s); }
template <class T> void atomicStore(std::shared_ptr<T>& s, std::shared_ptr<T> v) noexcept
    { std::atomic_store(&s, std::move(v)); }

// Publish (message/background thread, after decode+resample):
auto newSrc = std::make_shared<juce::AudioBuffer<float>>(/* resampled */);
atomicStore(currentSource, newSrc);            // old buffer reaped when last shared_ptr drops

// Audio thread, once per processBlock — snapshot, then read all block long:
auto src = atomicLoad(currentSource);          // keeps the buffer alive for the whole block
// ... grains read src->getReadPointer(0) ...   never touches a half-built buffer
```
The audio thread holds the `shared_ptr` for the block's duration, so a swap mid-block can't free the buffer it's reading. `[VERIFIED: O-MicrotonalSampler PluginProcessor.cpp:32-42, 341, 982, 1082]`

### 6.4 Drag-drop content-streaming + the Base64 gotcha
**Source:** `O-MicrotonalSampler PluginProcessor.cpp:2494-2528` (decode) + `ui/public/modules/webview-drop-streaming.js` + `PluginEditor.cpp:1013-1023`.

**The bridge — 4 NativeFunction names the JS expects** (Stage 2 must register these C++ handlers; the JS surface lands in Stage 3, but the names are fixed by the shared module `webview-drop-streaming.js:12-13, 248-366`):

| NativeFunction | Role |
|----------------|------|
| `dropSessionStart(sessionId[, folderName])` | open a session temp dir |
| `dropSessionAddFile(sessionId, filename, base64)` | stream one file's bytes (base64) |
| `dropSessionCommitFolder(sessionId, ...)` | finalize a folder drop → load |
| `dropSessionCommitFile(sessionId, filename, base64)` | finalize a single-file drop → load |

For O-simpleGrain (single source, not a multi-sample folder), the **single-file path** suffices: `dropSessionStart` → `dropSessionAddFile`/`dropSessionCommitFile`. Register them in the editor's `WebBrowserComponent::Options::withNativeFunction(...)` registry (Stage 3.1 wires the JS; Stage 2.3 provides the C++ decode+publish those handlers call).

**🔴 CRITICAL Base64 gotcha (project memory + verified in-repo):** the JS sends standard `btoa()` base64. Decode it with **`juce::Base64::convertFromBase64(OutputStream&, StringRef)`** — NOT `juce::MemoryBlock::fromBase64Encoding` (JUCE's proprietary `<size>.<altAlphabet>` format, which silently rejects `btoa()` output). Exact decode (`O-MicrotonalSampler PluginProcessor.cpp:2494-2528`):

```cpp
juce::MemoryBlock raw;
{ juce::MemoryOutputStream mos(raw, false);
  if (!juce::Base64::convertFromBase64(mos, base64)) return false; }   // <-- the load-bearing call
// then: wav.createReaderFor(new juce::MemoryInputStream(raw, /*keepInternalCopy=*/true), true)
//       -> reader->read(...) -> resample -> atomic publish (§6.3)
```
`[VERIFIED: juce_Base64.h:56 static bool convertFromBase64(OutputStream&, StringRef); + O-MicrotonalSampler PluginProcessor.cpp:2501]`

**JS gotchas to honor (from `webview-drop-streaming.js:32-43`):** (1) pass the `Juce` ES-module namespace (not `window.__JUCE__`) to the module; (2) chunk `btoa()` at 32 K (big files crash without chunking); (3) the C++ side must use `convertFromBase64`. These are Stage 3 concerns but the C++ decode (Stage 2.3) must match the standard-base64 contract.

**Picker fallback:** `juce::FileChooser` (async, message thread) — always works where drag-drop doesn't. Truncate loaded files to `kMaxSourceSeconds * engineRate` (UI notice if truncated).

---

## 7. Viz taps reference

**Three right-sized lock-free primitives** (ARCHITECTURE.md §Visualization Tap). Audio thread is **copy-only, no alloc, no FFT**. FFT/cloud/meter build on the Stage-3 message-thread Timer.

### 7.1 Output scope/spectrum — `VizRing` + `FmVizAnalyzer` (COPY VERBATIM)
**Source:** `O-simpleFM/Source/FmVizAnalyzer.h` (full) — lift unchanged into `Source/`. `VizRing` (lines 30-58): power-of-two ring, `write(data,n)` on the audio thread (relaxed atomics), `readLatest(dest,n)` on the message thread. `FmVizAnalyzer` (lines 61-131): 4096 FFT / Blackman-Harris on the message thread; **scope copied BEFORE the in-place FFT** (`FmVizAnalyzer.h:82-94` — the FFT clobbers its buffer). Audio-thread write is mono-sum post-gain (`O-simpleFM PluginProcessor.cpp:317-340`):

```cpp
// processBlock tail (audio thread), post-gain mono sum -> ring (copy-only):
if (numCh == 1) vizRing.write(buffer.getReadPointer(0), numSamples);
else { /* sum to a stack mono[] in <=4096 chunks, then vizRing.write(...) — see :323-339 */ }
```

### 7.2 Grain events — `TripleBuffer<GrainCloudFrame>` (COPY `TripleBuffer.h` VERBATIM)
**Source:** `O-GrainScatter/Source/dsp/TripleBuffer.h` (full) — lock-free SPSC, copy verbatim. API: audio thread fills `getWriteBuffer()` then `publish()`; message thread `read()` (`TripleBuffer.h:12-27`). GrainScatter frames grain events at `PluginProcessor.cpp:705-745` (`auto& snap = vizBuffer.getWriteBuffer(); ...; vizBuffer.publish();`).

**Define `GrainCloudFrame`** — the data each grain tap carries for the Stage-3 visuals (cloud scatter UI-01 + waveform playheads UI-02). Per ARCHITECTURE.md §Thread Boundaries ("Grain events (pos,size,pitch,pan,time)"):
```cpp
struct GrainEvent {
    float readPosNorm;    // grain.readStart / sourceLen  ∈ [0,1]  — cloud X / waveform playhead
    float sizeMs;         // grain length                          — cloud dot size
    float pitchSemis;     // (grainPitch + spray) relative          — cloud Y / color
    float pan;            // 0..1                                   — cloud lateral
    int   spawnSample;    // sample offset within block             — cloud time axis
};
struct GrainCloudFrame {
    static constexpr int kMax = 256;        // >= one block's spawns across 8 voices
    std::array<GrainEvent, kMax> events {};
    int  count = 0;
    float playheadNorm = 0.0f;              // global playhead / sourceLen — the live playhead line
    float positionNorm = 0.0f;              // resting point — for the shaded spray range
    float positionSprayNorm = 0.0f;         // ± shade width
    bool  frozen = false;                   // freeze-point marker
};
```
Write events at spawn time (audio thread); `publish()` once per block.

### 7.3 Active grain count — `std::atomic<int>` (CPU/overlap readout, UI-05)
```cpp
std::atomic<int> activeGrainCount { 0 };   // increment on spawn, decrement on grain-done, publish per block
```
Editor reads it on its 30 Hz Timer + derives overlap = `(grainSizeMs/1000)·density`, displays `Grains: N/192`, `Overlap: X.X×`, coarse CPU bar (`activeGrains/192` proxy). Cheap relaxed atomic.

---

## 8. JUCE 8 API confirmations (verified against `/Users/taylorbrook/JUCE/modules/`, v8.0.9)

| API | Signature / fact | Source |
|-----|------------------|--------|
| `SynthesiserVoice::canPlaySound` | `virtual bool canPlaySound(SynthesiserSound*) = 0` | juce_Synthesiser.h:129 |
| `SynthesiserVoice::startNote` | `virtual void startNote(int, float velocity, SynthesiserSound*, int pitchWheel) = 0` | :134 |
| `SynthesiserVoice::stopNote` | `virtual void stopNote(float velocity, bool allowTailOff) = 0` | :154 |
| `SynthesiserVoice::isVoiceActive` | `virtual bool isVoiceActive() const` | :160 |
| `SynthesiserVoice::renderNextBlock` | `virtual void renderNextBlock(AudioBuffer<float>&, int startSample, int numSamples)` | :198 |
| `SynthesiserVoice::setCurrentPlaybackSampleRate` | `virtual void setCurrentPlaybackSampleRate(double)` | :215 |
| **`SynthesiserVoice` has NO virtual `prepareToPlay`** | confirmed — use non-virtual + `dynamic_cast` (§3.1) | (absent) |
| `Synthesiser::addVoice/addSound/setNoteStealingEnabled` | present | :381 etc. |
| `Synthesiser::renderNextBlock` | `void renderNextBlock(AudioBuffer<float>&, const MidiBuffer&, int startSample, int numSamples)` | :535 |
| `ADSR::setSampleRate` | `void setSampleRate(double) noexcept` — **call BEFORE setParameters** | juce_ADSR.h:115 |
| `ADSR::setParameters` | `void setParameters(const Parameters&)` (jassert sampleRate>0) | :92 |
| `ADSR::Parameters` | `{ float attack, decay, sustain, release }` + 4-arg ctor | :68-83 |
| `ADSR::noteOn / noteOff / getNextSample / isActive` | `noteOn()`, `noteOff()`, `float getNextSample()`, `bool isActive() const` | :130/149/170/108 |
| `AudioFormatManager::createReaderFor(File)` | `AudioFormatReader* createReaderFor(const File&)` | juce_AudioFormatManager.h:135 |
| `AudioFormatManager::createReaderFor(stream)` | `AudioFormatReader* createReaderFor(std::unique_ptr<InputStream>)` | :149 |
| `AudioFormat::createReaderFor(ptr,bool)` | `createReaderFor(InputStream*, bool deleteWhenDestroyed)` (used in §6.4) | juce_AudioFormat.h:119 |
| `AudioFormatManager::registerBasicFormats` | `void registerBasicFormats()` | :80 |
| `AudioFormatReader::read` | `bool read(AudioBuffer<float>*, int startSampleInDest, int numSamples, int64 readerStartSample, bool useLeftChan, bool useRightChan)` | juce_AudioFormatReader.h:156 |
| `AudioFormatReader` fields | `double sampleRate; unsigned int bitsPerSample; int64 lengthInSamples; unsigned int numChannels;` | :234/237/240/243 |
| `MemoryInputStream` ctor | `MemoryInputStream(const void* sourceData, size_t dataSize, bool keepInternalCopy)` | juce_MemoryInputStream.h:61 |
| `Base64::convertFromBase64` | `static bool convertFromBase64(OutputStream&, StringRef)` — **use this, not MemoryBlock::fromBase64Encoding** | juce_Base64.h:56 |
| `Base64::toBase64` | `static String toBase64(const void*, size_t)` | :59 |
| `LagrangeInterpolator` | `using LagrangeInterpolator = Interpolators::Lagrange;` | juce_Interpolators.h:200 |
| `…Interpolator::process` | `int process(double speedRatio, const float* in, float* out, int numOut)` (streaming/random — see note) | juce_GenericInterpolator.h:101 |
| `SmoothedValue` | `setTargetValue(float)`, `getNextValue()`, `reset(double sr, double seconds)`, `setCurrentAndTargetValue(float)`, `skip(int)` | (in-use O-simpleFM FMVoice.h:58-62, PluginProcessor.cpp:161-163) |
| `juce::Random` real-time | per-voice instance member; `nextFloat()` no alloc/lock | (O-GrainScatter GrainScheduler.h:138; ARCHITECTURE §Special Considerations) |
| `getLatencySamples()` | **non-virtual in JUCE 8 — do NOT override**; use `setLatencySamples(0)` in prepare | (project memory; ARCHITECTURE §Latency) |

**Why NOT the stateful `juce::LagrangeInterpolator` for grain reads:** it is a *streaming* resampler (advances an internal read pointer at a fixed ratio). Grains read **arbitrary random positions** each spawn, so the stateless 4-point `lagrangeInterpolate` (§2.5) is required. The stateful `LagrangeInterpolator::process` is correct only for the **one-shot source resample** at load (§6.2), where it streams the whole buffer once.

---

## 9. Pitfalls / gotchas

| # | Pitfall | Avoidance |
|---|---------|-----------|
| 1 | **Allocation/lock in `processBlock`** (PERF-01) | Preallocated `std::array<Grain,24>`/voice + steal-oldest; per-voice `juce::Random` member; `std::vector` spawn lists are O-GrainScatter's effect pattern — O-simpleGrain's per-voice scheduler spawns inline (no vector). No `new`/`malloc`/lock anywhere in the render path. |
| 2 | **Base64 decode silently fails** | `juce::Base64::convertFromBase64`, NEVER `MemoryBlock::fromBase64Encoding` (§6.4). The wrong call returns garbage/empty with no error. |
| 3 | **ADSR `setSampleRate` ordering** | `setSampleRate(sr)` MUST precede `setParameters(...)` (jassert fires otherwise; values wrong). Do it in the non-virtual `prepareToPlay` (§3.1). |
| 4 | **`getLatencySamples()` non-virtual** | Do NOT override. `setLatencySamples(0)` in `prepareToPlay` (zero added latency in v1.0). |
| 5 | **Reading a half-loaded source buffer** | Atomic `shared_ptr` swap (§6.3); audio thread snapshots once per block and holds the ref for the block. Never build/resize the buffer on the audio thread. |
| 6 | **Headroom / clipping on dense clouds** | Overlapping grains sum; peak grows with overlap. Normalize the voice sum by a function of overlap (or a fixed headroom factor), soft-safety, + block-level `std::isfinite` scrub (O-simpleFM PluginProcessor.cpp:310-315), then `outputLevel` trim (`SmoothedValue`, dB→lin, 20 ms). |
| 7 | **Zipper noise** (QUAL-01) | `SmoothedValue` on `scan`, `position`, `playheadVelocity`, `outputLevel`; crossfade on freeze toggle (§4.2); never hard-jump the playhead or reset grain phase mid-grain. The ONLY intended click is the **rectangular window** per-grain artifact (DSP-03 — a feature). |
| 8 | **Up-transposition aliasing** (DSP-08) | 4-pt Lagrange + per-grain one-pole `fc=0.5fs/rate` when `rate>1` (§5); bypass at `rate≤1`. |
| 9 | **Wrong read addressing copied from GrainScatter** | GrainScatter reads a live `DelayBuffer` with a self-cancelling delay tap. O-simpleGrain reads the **static source at `grain.readPos`** advanced by `rate`. Follow ARCHITECTURE §Core Components, not `GrainPool::processSample`'s `delaySamples` math (§1 surprise). |
| 10 | **Scheduler density model mismatch** | Don't copy GrainScatter's exponential density%→ms (`GrainScheduler.h:26`). O-simpleGrain density is **grains/sec**: `period = fs/density`, scatter jitters it (§2.6). |
| 11 | **Denormals** | `juce::ScopedNoDenormals` at top of `processBlock` (O-simpleFM PluginProcessor.cpp:223); the AA one-pole state + ADSR tails warrant it (no feedback loop, so otherwise low exposure). |
| 12 | **Sample-rate mismatch on embedded .wav** | The 4 built-ins are 44.1 kHz; resample to engine rate at load (§6.2). Grain lengths (`grainSize·fs`), periods (`fs/density`), AA cutoffs (`0.5fs/rate`) all use current `fs` — recompute on `prepareToPlay`. Window LUTs are sample-rate-independent (phase-indexed). |

---

## 10. Per-sub-phase implementation checklist

### Phase 2.1 — Core grain engine + overlap-add + window LUTs + amp ADSR + key resample
**New files:** `Source/dsp/LagrangeInterpolation.h` (copy verbatim), `Source/GrainVoice.h` (+ `GrainSound`), `Source/dsp/GrainPool.h` or inline pool in the voice, `Source/dsp/WindowLuts.h`.
- [ ] `GrainSound` + `GrainVoice : juce::SynthesiserVoice` skeleton from `FMVoice.h:27-44, 97-131` (rename).
- [ ] Non-virtual `prepareToPlay(double,int)` — `setCurrentPlaybackSampleRate` + `ampEnv.setSampleRate` before `setParameters` (§3.1). Processor dispatch via `dynamic_cast` (O-simpleFM PluginProcessor.cpp:156-159).
- [ ] Preallocated `std::array<Grain,24>` + round-robin/steal-oldest `spawnGrain` (§2.2, GrainPool.h:111-165 shape).
- [ ] Per-sample scheduler countdown; `period = fs/density` (§2.6).
- [ ] 5× 2048-pt window LUTs precomputed at construction (§2.4); linear-interp read.
- [ ] Per-grain overlap-add loop: `lagrangeInterpolate` source read + equal-power pan + `readPos+=rate; phase+=phaseInc` (§2.3).
- [ ] `voiceRate = 2^((note - kRootNote)/12)` at `startNote`; combine with `grainPitch` (§3.4).
- [ ] Amp `juce::ADSR`; voice lifetime = `ampEnv.isActive()` (§3.4); velocity→amp.
- [ ] Processor: ctor preallocates 8 voices + 1 sound + note-stealing (§3.5); `isBusesLayoutSupported` synth (O-simpleFM:173-187); `processBlock` = `ScopedNoDenormals` + `pushParamsToVoices` + `synth.renderNextBlock` + `outputLevel` ramp + `isfinite` scrub (NO oversampler).
- [ ] Decode **single default `fire.wav`** at construction (§6.2, minimal); resample to engine rate; publish via atomic `shared_ptr` (§6.3, even for one source — sets up 2.3).
- [ ] Cache APVTS atomics in ctor (slots already declared `PluginProcessor.h:126-143`).

### Phase 2.2 — Read head (scan/stretch/freeze) + spray/scatter + anti-aliasing + velToDensity
- [ ] Processor-owned global playhead: `playheadPos += playheadVelocity` per sample, wrapped (§4.1); `position` = resting point; `scan` = velocity (`SmoothedValue`).
- [ ] `freeze` (bool): pin velocity→0 + smoothed crossfade engage/disengage (§4.2, FreezeManager.h:67-85 mechanic). Held note sustains the frozen instant.
- [ ] Per-voice `juce::Random` spray: position spray (`grainReadStart += U·positionSpray%·sourceLen`), pitch spray (`±U·pitchSpray` st into rate), scatter (period jitter §2.6), pan spray (`pan = 0.5 + U·panSpray%·0.5`).
- [ ] AA one-pole per grain: `fc=0.5fs/rate` when `rate>1`, bypass otherwise (§5); `aaState` field.
- [ ] `velToDensity`: `effectiveDensity = density·(1 + velToDensity·(vel−0.5)·2)`, clamp [1,200].
- [ ] `SmoothedValue` on `scan`, `position`, `outputLevel` (QUAL-01).

### Phase 2.3 — Sample loading (embed 4 + drag-drop + hot-swap) + viz taps
**New files:** `Source/dsp/TripleBuffer.h` (copy verbatim), `Source/FmVizAnalyzer.h`→`VizAnalyzer.h` (copy verbatim from O-simpleFM), `Source/dsp/GrainCloudFrame.h`.
- [ ] CMake: `juce_add_binary_data(${PROJECT_NAME}_Samples ...)` for all 4 .wav + `target_link_libraries(... PRIVATE ${PROJECT_NAME}_Samples)`, AFTER `juce_generate_juce_header` (§6.1).
- [ ] Decode all 4 via `createReaderFor(MemoryInputStream(BinaryData::xxx_wav, xxx_wavSize, false))` + resample (§6.2); `sourceSample` `AudioParameterChoice` selects; rebuild + atomic-publish on change (§6.3).
- [ ] Drag-drop C++ handlers: register `dropSessionStart` / `dropSessionAddFile` / `dropSessionCommitFile` NativeFunctions; decode with `juce::Base64::convertFromBase64` (§6.4); `FileChooser` picker fallback; truncate to 10 s.
- [ ] `VizRing` write (post-gain mono sum) at processBlock tail (§7.1, O-simpleFM:317-340).
- [ ] `TripleBuffer<GrainCloudFrame>`: fill `getWriteBuffer()` with grain events at spawn + playhead/position/spray/freeze fields; `publish()` per block (§7.2).
- [ ] `std::atomic<int> activeGrainCount` increment/decrement (§7.3).
- [ ] Public accessors on the processor for the editor (Stage 3): `getVizRing()`, the `TripleBuffer&`, `getActiveGrainCount()`, `getCurrentSampleRate()` (mirror O-simpleFM PluginProcessor.h:97-99).

---

## Sources

### Primary (HIGH confidence — local source, cited inline)
- `O-GrainScatter/Source/dsp/{GrainPool,GrainScheduler,LagrangeInterpolation,TripleBuffer,FreezeManager}.h` + `PluginProcessor.cpp:475-745`
- `O-simpleFM/Source/{FMVoice.h,FmVizAnalyzer.h,PluginProcessor.cpp,PluginProcessor.h}` + `CMakeLists.txt`
- `O-MicrotonalSampler/Source/PluginProcessor.cpp:32-42,2462-2528` + `PluginEditor.cpp:1013-1023` + `ui/public/modules/webview-drop-streaming.js`
- `O-Texture/CMakeLists.txt:86-116` (`juce_add_binary_data` precedent)
- `/Users/taylorbrook/JUCE/modules/` v8.0.9 headers (all signatures in §8)
- `O-simpleGrain/Source/samples/*.wav` (verified mono/44.1k/24-bit/2.2–3.0s via `afinfo`)
- `O-simpleGrain/.planning/research/ARCHITECTURE.md`, `.../stages/2-dsp/CONTEXT.md`, `.../ROADMAP.md` (locked contract)
