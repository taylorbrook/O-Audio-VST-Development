# Stage 2 (DSP) — RESEARCH

**Plugin:** O-simpleSampler
**Stage:** 2 of 4 — DSP
**Phase:** research → complete
**Date:** 2026-06-25
**Inputs:** stages/2-dsp/CONTEXT.md (2 decisions), research/ARCHITECTURE.md, ROADMAP.md, parameter-spec.md, Stage-1 PluginProcessor.{h,cpp}; verbatim source extraction from O-simpleGrain / O-simpleSubtractive; local JUCE 8.0.9 source; piano.wav probe.

> **Scope.** This research covers all of Stage 2 (Phases 2.1 / 2.2 / 2.3) but front-loads the **immediate execute target, Phase 2.1** (Repitch core + region + amp ADSR + piano decode → first audio), per CONTEXT D2 (execute checkpoints after 2.1). Every reusable component below was extracted **verbatim** from shipped siblings — most of Stage 2 is a port, not new invention. The genuinely-new code is small: the Repitch read head, the loop equal-power crossfade, Vintage, the `velToAmp` blend, and the Stretch time-axis wrapper around the grain engine.

---

## 0. Answers to the 6 open items (CONTEXT.md → "Open items for research phase")

| # | Open item | Resolution |
|---|-----------|------------|
| 1 | Lagrange + one-pole AA port surface | `lagrangeInterpolate(ym1,y0,y1,y2,frac)` (global, `dsp/LagrangeInterpolation.h:11`) + the `readSourceLagrange(src,len,pos)` clamp wrapper + `aaOnePole(x,coeff,engaged,state)`. Drop-in, zero-dependency. **§2.** |
| 2 | Off-thread decode→resample→atomic-publish | `std::atomic_load/atomic_store` on a `std::shared_ptr<juce::AudioBuffer<float>>`; `decodeAndPublish()` (AudioFormatManager + `createReaderFor(MemoryInputStream)` + `reader->read`) + `resampleToEngineRate()` via offline `juce::LagrangeInterpolator`; all on the message thread; audio thread snapshots once/block. **§4.** |
| 3 | `sourceSample` combo reconciliation | **KEEP the 4-entry `AudioParameterChoice` exactly as-is** (collapsing changes discrete-param normalization → breaks the frozen contract). Wire index 0→piano blob; indices 1–3 fall back to the piano blob (documented TODO) so no selection yields silence. Real vocal/flute/vinyl assets land Stage 4. **§5.** |
| 4 | piano.wav pitch → `rootKey` seed | Probe: **44.1 kHz, 24-bit, 3.00 s, f0 ≈ 131.25 Hz** = MIDI **48** (scientific C3 = 130.81 Hz; "C2" in the plugin's MIDI60=C3 label scheme). **Seed piano's default root = 48, NOT the placeholder 60** — 60 puts the whole keyboard an octave sharp of standard pitch. **§6.** |
| 5 | 2nd `juce_add_binary_data` wiring | Samples target `NAMESPACE BinaryData HEADER_NAME BinaryData.h`; UI target (Stage 3) `NAMESPACE UIBinaryData HEADER_NAME UIBinaryData.h`. Distinct NAMESPACE **and** HEADER_NAME — `HEADER_NAME` alone is insufficient (O-simpleGrain Stage-3.1 collision). Declare samples target after `juce_generate_juce_header`, link separately. **§7.** |
| 6 | `velToAmp` curve + click-free ramps | `velToAmp` blend is **net-new** (no sibling has it): `velLevel = (1 − v) + v·velocity`, `v = velToAmp·0.01`; default 50% → `0.5 + 0.5·velocity`. Click-free note-on/off comes from the amp ADSR (5 ms attack / release tail); output gain `SmoothedValue.reset(sr, 0.02)` (20 ms) + `applyGainRamp`. **§8 / §9.** |

---

## 1. Reuse map (where each Stage-2 component comes from)

| Component | Source (verbatim port) | Phase | New work |
|-----------|------------------------|-------|----------|
| `lagrangeInterpolate` 4-pt read | O-simpleGrain `dsp/LagrangeInterpolation.h` | 2.1 | none — copy file |
| `readSourceLagrange` clamp wrapper | O-simpleGrain `GrainVoice.h:390` | 2.1 | copy static helper |
| Rate-tracking one-pole AA (`aaOnePole`) | O-simpleGrain `GrainVoice.h:357,412` | 2.1 | copy; apply to Repitch read |
| Voice skeleton (`SynthesiserVoice`, non-virtual `prepareToPlay`) | O-simpleGrain `GrainVoice.h` / O-simpleSubtractive `SubVoice.h` | 2.1 | rename + strip to Repitch read head |
| Source decode/resample/atomic-publish | O-simpleGrain `PluginProcessor.cpp:336–405` | 2.1 | copy `decodeAndPublish`/`resampleToEngineRate`/`loadBuiltInSource` |
| Dual binary-data CMake + synth/webview flags | O-simpleGrain `CMakeLists.txt` | 2.1 | add samples target (UI target Stage 3) |
| Synth setup + per-block param push | O-simpleGrain `PluginProcessor.cpp:184,644` | 2.1 | adapt to `SamplerVoiceParams` |
| `sourceSample` listener → AsyncUpdater reload | O-simpleGrain `PluginProcessor.cpp:456–476` | 2.1 | copy; add per-source root seed |
| Amp `juce::ADSR` + VCA + lifetime gate | O-simpleSubtractive `SubVoice.h:62–282` | 2.1 | copy; **add `velToAmp` blend (net-new)** |
| Repitch read head (`readPos += keyRatio`) | — (net-new, trivial) | 2.1 | new |
| Loop engine (equal-power xfade + ping-pong + zero-cross snap) | net-new; idiom = O-simpleGrain pan `cos/sin` (`GrainVoice.h:339`); O-Freeze Hann/SmoothedValue | 2.2 | new |
| Stretch (synchronous-granular SOLA) | O-simpleGrain `GrainVoice.h` grain pool + `spawnGrain` + Hann LUT | 2.2 | wrap time-axis at 1× + hop = grainSize/2 |
| Vintage (S&H decimation + bit-crush) | net-new; O-simpleAdditive bit-depth lesson | 2.2 | new (bounded, bypass at 0) |
| Resonant LP filter (`StateVariableTPTFilter`) | **ARCHITECTURE-specified JUCE class**; curve math = O-simpleSubtractive `SubVizAnalyzer.h:71` | 2.2 | per-voice; map res%→Q |
| Lead-voice display atomics | O-simpleSubtractive `PluginProcessor.cpp:283–309` | 2.3 | copy (loudest-active) |
| Viz tap `VizRing` + analyzer | O-simpleGrain `VizAnalyzer.h` (verbatim) | 2.3 | rename `SamplerVizAnalyzer` |
| Render-harness | O-simpleGrain `tests/render-harness/` | 2.3 | adapt gates |

---

## 2. Anti-alias fractional read (DSP-02) — Phase 2.1

### 2.1 `lagrangeInterpolate` (drop-in, copy the whole file)
`O-simpleGrain/Source/dsp/LagrangeInterpolation.h:11` — global scope, stateless, header-only:
```cpp
inline float lagrangeInterpolate (float ym1, float y0, float y1, float y2, float frac)
{
    float c0 = y0;
    float c1 = y1 - (1.0f / 3.0f) * ym1 - 0.5f * y0 - (1.0f / 6.0f) * y2;
    float c2 = 0.5f * (ym1 + y1) - y0;
    float c3 = (1.0f / 6.0f) * (y2 - ym1) + 0.5f * (y0 - y1);
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}
```
`frac ∈ [0,1)` between `y0` and `y1`; `ym1`/`y2` are the neighbours. No bounds checking — the caller clamps.

### 2.2 `readSourceLagrange` — clamped random-access read (copy this static helper)
`GrainVoice.h:390` — independently `jlimit`-clamps all four indices to `[0,len-1]`, so reads off either region/loop boundary saturate to the edge sample (no OOB, no wrap unless you want one):
```cpp
static float readSourceLagrange (const float* src, int len, float pos) noexcept
{
    if (src == nullptr || len <= 0) return 0.0f;
    const int   i0   = (int) pos;
    const float frac = pos - (float) i0;
    const int   im1  = juce::jlimit (0, len - 1, i0 - 1);
    const int   ip0  = juce::jlimit (0, len - 1, i0);
    const int   ip1  = juce::jlimit (0, len - 1, i0 + 1);
    const int   ip2  = juce::jlimit (0, len - 1, i0 + 2);
    return lagrangeInterpolate (src[im1], src[ip0], src[ip1], src[ip2], frac);
}
```
**Repitch use:** `readPos` is the absolute fractional source position; `src = readSourceLagrange(srcPtr, srcLen, (float)readPos); readPos += keyRatio;` confined to `[startSamp, endSamp)`.

### 2.3 Rate-tracking one-pole AA (`fc ≈ 0.5·fs/rate`, engaged only when rate>1)
Coefficient computed once per note (Repitch) or per grain (Stretch) — `GrainVoice.h:357`:
```cpp
g.aaEngaged = (g.rate > 1.0f);
g.aaCoeff   = g.aaEngaged
            ? 1.0f - std::exp (-juce::MathConstants<float>::twoPi
                     * (0.5f * (float) sampleRate / g.rate) / (float) sampleRate)
            : 0.0f;
g.aaState   = readSourceLagrange (sourcePtr, sourceLen, g.readPos); // prime → settled (no attack transient)
```
Per-sample update + coherent bypass — `GrainVoice.h:412`:
```cpp
static float aaOnePole (float x, float coeff, bool engaged, float& state) noexcept
{
    if (! engaged) { state = x; return x; }   // keep state coherent across bypass→engage
    state += coeff * (x - state);             // y += g*(x - y)
    return state;
}
```
**Phase 2.1 caveat:** `keyRatio` is fixed per note, so the AA coefficient can be computed at `startNote` (cheaper than O-simpleGrain's per-grain compute). For Repitch, recompute on note-on only.

---

## 3. Sampler voice skeleton — Phase 2.1

**Class:** `SamplerVoice : juce::SynthesiserVoice` (custom — NOT `juce::SamplerVoice`). `canPlaySound` → `dynamic_cast<SamplerSound*>(sound) != nullptr`. Overrides: `startNote / stopNote / pitchWheelMoved / controllerMoved / renderNextBlock`.

**JUCE-8 prepare dispatch (load-bearing).** `SynthesiserVoice` has **no virtual `prepareToPlay`**. Declare a non-virtual `prepareToPlay(double sr, int blockSize)` and dispatch it from the processor via `dynamic_cast` (`PluginProcessor.cpp:220`):
```cpp
synth.setCurrentPlaybackSampleRate (sampleRate);
for (int v = 0; v < synth.getNumVoices(); ++v)
    if (auto* sv = dynamic_cast<SamplerVoice*> (synth.getVoice (v)))
        sv->prepareToPlay (sampleRate, samplesPerBlock);
```
Inside it: `setCurrentPlaybackSampleRate(sr); ampEnv.setSampleRate(sr); ampEnv.setParameters(ampParams);` — **`setSampleRate` BEFORE `setParameters`** (`SubVoice.h:62`, JUCE-8 gate). LP filter `prepare(spec)` here too (§9).

**Voice does NOT hold a `juce::AudioBuffer`.** It snapshots a raw `const float* sourcePtr` + `int sourceLen` per block via `setSource(const float*, int)` (`GrainVoice.h:118`); the processor holds the owning `shared_ptr` alive for the whole block. Null source → silence.

**`startNote` (Repitch):**
```cpp
voiceRate = std::pow (2.0f, (float)(midiNote - rootKey + tune + fine*0.01f) / 12.0f); // keyRatio
readPos   = reverse ? (double) endSamp : (double) startSamp;
velLevel  = (1.0f - vAmp) + vAmp * juce::jlimit (0.0f, 1.0f, velocity);  // §8 net-new blend
// (clear filter state; compute AA coeff from voiceRate; ampEnv.noteOn())
```
> **Note:** `keyRatio` uses the **runtime `rootKey`/`tune`/`fine`** pushed from APVTS, not the fixed `kRootNote` constant O-simpleGrain bakes in. `kRootNote=60` stays the APVTS *default* only; the live value drives tuning.

**`renderNextBlock`** mirrors `GrainVoice.h:186`: early-out if `!ampEnv.isActive()`; per-sample read → AA → (Vintage 2.2) → (filter 2.2) → `ampVal = ampEnv.getNextSample()·velLevel`; `addSample` to L/R (mono source duplicated); `clearCurrentNote()` when `ampEnv` finishes. Lifetime gated on `ampEnv.isActive()` (`SubVoice.h:221,277`).

---

## 4. Source loading — decode → resample → atomic publish (PERF-01) — Phase 2.1

**Publish primitive = atomic `shared_ptr` swap** (NOT `TripleBuffer`; that's for viz). `PluginProcessor.h:197`:
```cpp
template <class T> static std::shared_ptr<T> atomicLoad (const std::shared_ptr<T>& s) noexcept { return std::atomic_load (&s); }
template <class T> static void atomicStore (std::shared_ptr<T>& s, std::shared_ptr<T> v) noexcept { std::atomic_store (&s, std::move (v)); }
// member: std::shared_ptr<juce::AudioBuffer<float>> currentSource;
```
Old buffer is reaped automatically by refcount: the audio thread snapshots once per block and holds the ref for the whole block, so a mid-block swap can never free the buffer being read.

**Decode (shared sink for built-in + drop + picker)** — `PluginProcessor.cpp:364`:
```cpp
juce::AudioFormatManager fmt; fmt.registerBasicFormats();           // WAV/AIFF/FLAC
std::unique_ptr<juce::AudioFormatReader> reader (
    fmt.createReaderFor (std::make_unique<juce::MemoryInputStream> (data, numBytes, false)));
if (reader == nullptr) return false;                                // invalid → keep previous source
juce::AudioBuffer<float> tmp ((int) reader->numChannels, (int) reader->lengthInSamples);
reader->read (&tmp, 0, (int) reader->lengthInSamples, 0, true, true);
auto resampled = resampleToEngineRate (tmp, reader->sampleRate, engineRate, truncated);
atomicStore (currentSource, std::move (resampled));                 // ATOMIC PUBLISH
```

**Resample = offline `juce::LagrangeInterpolator`** (per channel, capped at `kMaxSourceSeconds`) — `PluginProcessor.cpp:336`:
```cpp
const double ratio = srcRate / engineRate;                          // speedRatio
int numOut = (int) std::floor ((double) nSmp / ratio);
truncated = (numOut > maxOut); numOut = juce::jlimit (1, maxOut, numOut);
juce::LagrangeInterpolator interp; interp.reset();
interp.process (ratio, src.getReadPointer(ch), out->getWritePointer(ch), numOut);
```
> `kMaxSourceSeconds` is already `30` in O-simpleSampler `PluginProcessor.h:119` (O-simpleGrain caps at 10 — sampler wants the larger 30 s; honoured).

**Audio-thread snapshot** in `processBlock` — `PluginProcessor.cpp:634`:
```cpp
auto src = atomicLoad (currentSource);
const float* srcPtr = (src && src->getNumSamples() > 0) ? src->getReadPointer (0) : nullptr;
int srcLen = src ? src->getNumSamples() : 0;
```
**Threading:** all decode/resample on the **message thread** (ctor / `AsyncUpdater` / picker — no `juce::Thread` needed for short found-sounds). The render-harness must pump messages for the AsyncUpdater path (§10).

---

## 5. Built-in decode + `sourceSample` combo reconciliation — Phase 2.1

**Index → blob** (`PluginProcessor.cpp:301`, `#include "BinaryData.h"`):
```cpp
struct BuiltInBlob { const char* data; int size; };
BuiltInBlob builtInBlob (int idx) noexcept {
    switch (idx) {
        case 0:  return { BinaryData::piano_wav, BinaryData::piano_wavSize };
        // Stage 2.1: vocal/flute/vinyl assets not yet embedded (CONTEXT D1) —
        // fall back to piano so no selection yields silence. Real blobs land Stage 4.
        default: return { BinaryData::piano_wav, BinaryData::piano_wavSize };
    }
}
bool loadBuiltInSource (int i, double rate) {
    i = juce::jlimit (0, kNumBuiltIns-1, i);
    return decodeAndPublish (builtInBlob(i).data, (size_t) builtInBlob(i).size, rate,
                             juce::String("embedded:") + kBuiltInNames[i]);
}
```
**Combo decision (open item #3): KEEP the 4-entry `AudioParameterChoice {piano,vocal,flute,vinyl}` unchanged.** Collapsing to 1 entry changes the discrete parameter's step count / `convertTo0to1` normalization → breaks the frozen Stage-1 APVTS contract + state checksums (CONTEXT: "prefer the lowest-risk option that keeps APVTS/state stable"). Indices 1–3 decode the piano blob as a documented fallback for Stage 2; identity strings still record the chosen name. (Stage 3 may visually present only "Piano" while the APVTS StringArray stays 4-wide.)

**Change → reload (never on audio thread)** — listener + AsyncUpdater (`PluginProcessor.cpp:456`):
```cpp
// ctor: apvts.addParameterListener (sourceSample, this);  (class : ...::Listener, AsyncUpdater)
void parameterChanged (const juce::String& id, float v) override {
    if (id == ParamIDs::sourceSample) { pendingBuiltInIndex.store ((int) v); triggerAsyncUpdate(); } }
void handleAsyncUpdate() override {
    const int idx = pendingBuiltInIndex.exchange (-1); if (idx < 0) return;
    loadBuiltInSource (idx, currentSampleRate);
    seedRootForSource (idx);          // §6 — net-new per-source root seed
}
```
**State** already wired in Stage 1 (`getStateInformation` writes `SOURCE/identity`; `setStateInformation` restores it). **Add to `setStateInformation`** (mirror `PluginProcessor.cpp:953`): after `replaceState`, re-decode the restored source and `cancelPendingUpdate()` so the choice-rebuild doesn't clobber a restored user file.

---

## 6. piano.wav pitch → `rootKey` seed (FUNC-01) — Phase 2.1

**Probe (autocorrelation, attack-settled window):**
```
afinfo: 1 ch, 44100 Hz, 24-bit, 3.000 s
f0 ≈ 131.25 Hz  →  MIDI 48  (scientific C3 = 130.81 Hz)
```
131.25 Hz = MIDI **48**. In the plugin's label scheme (MIDI 60 = "C3", Yamaha), that note is labelled "C2".

**Recommendation: seed piano's default root = 48** (not the placeholder 60). With root=48: pressing MIDI 48 → keyRatio 1 → 131 Hz (recorded pitch); MIDI 60 → 262 Hz (≈ middle C) → **keyboard in standard tune**. With root=60 (placeholder): pressing MIDI 60 → 131 Hz → the whole instrument plays an octave flat of standard pitch.

**Divergence to flag:** O-simpleGrain's harness uses `srcF0 = 130.81` with `root = kRootNote = 60` (it deliberately plays piano.wav an octave low — fine for a *texture*, wrong for a *tuned sampler*). Do not copy root=60.

**Per-source root table (engine metadata, overridable — corrects the spec placeholder):**
```cpp
static constexpr int kBuiltInRoot[kNumBuiltIns] = { 48 /*piano*/, 69 /*vocal*/, 72 /*flute*/, 48 /*vinyl*/ };
```
**Seeding mechanism (plan/execute decision — surfaced, not yet chosen):** the APVTS `rootKey` default stays 60 (frozen contract). Seed the live value via `rootKeyParam`'s `AudioParameterInt::operator=`/`setValueNotifyingHost` **on explicit `sourceSample` change** (in `handleAsyncUpdate`), and once for the default source after construction. Construction order `ctor → setStateInformation → prepareToPlay` makes a ctor/prepare-time seed safe: a restored session overwrites it, a fresh instance keeps 48. **Caveat:** writing params during construction is touchy in some hosts — prefer seeding in `prepareToPlay` (first call) guarded by a "state was not restored" flag, or only seed on user-driven source change and accept that a brand-new instance shows root=60 until first interaction. Recommend the prepare-time guarded seed.

---

## 7. CMake — 2nd binary-data target + synth/webview flags — Phase 2.1

Two `juce_add_binary_data` targets with **distinct `NAMESPACE` AND `HEADER_NAME`** (a distinct `HEADER_NAME` alone is NOT enough — both share `getNamedResource`/`namedResourceListSize` inside the namespace; O-simpleGrain Stage-3.1 + project memory `critical_dual_binary_data_namespace_collision.md`):
```cmake
# Samples (THIS stage). Declared AFTER juce_generate_juce_header(O-simpleSampler).
juce_add_binary_data(O-simpleSampler_Samples
    NAMESPACE BinaryData  HEADER_NAME BinaryData.h
    SOURCES Source/samples/piano.wav)
target_link_libraries(O-simpleSampler PRIVATE O-simpleSampler_Samples)

# UI resources (Stage 3) — second target, different namespace:
# juce_add_binary_data(O-simpleSampler_UIResources NAMESPACE UIBinaryData HEADER_NAME UIBinaryData.h SOURCES ...)
```
`juce_add_plugin` (already correct from Stage 1, confirm): `IS_SYNTH TRUE  NEEDS_MIDI_INPUT TRUE  NEEDS_WEB_BROWSER TRUE  NEEDS_WEBVIEW2 TRUE  FORMATS VST3 AU Standalone`. Compile defs: `JUCE_WEB_BROWSER=1  JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1  JUCE_USE_CURL=0`.
**Action 2.1:** `cp plugins/O-simpleGrain/Source/samples/piano.wav plugins/O-simpleSampler/Source/samples/piano.wav`, add the samples target + link, flip the Stage-1 binary-data TODO.

---

## 8. Amp ADSR + VCA + `velToAmp` (DSP-06) — Phase 2.1

**ADSR** (`SubVoice.h:62`): `juce::ADSR ampEnv; juce::ADSR::Parameters ampParams;` — `setSampleRate(sr)` then `setParameters(ampParams)` in prepare; per-block re-apply `ampEnv.setParameters(p.amp)`; VCA `out *= ampEnv.getNextSample() * velLevel`; lifetime `ampEnv.isActive()`. Defaults already in spec (A 0.005 / D 0.3 / S 1.0 / R 0.2). Push via a `SamplerVoiceParams.amp = juce::ADSR::Parameters{ ampAttack, ampDecay, ampSustain, ampRelease }`.

**`velToAmp` blend — NET-NEW** (neither O-simpleGrain nor O-simpleSubtractive has it; both use raw `velLevel = jlimit(0,1,velocity)`). Add per ARCHITECTURE:
```cpp
const float v = juce::jlimit (0.0f, 1.0f, velToAmp * 0.01f);   // param stored 0–100
velLevel = (1.0f - v) + v * juce::jlimit (0.0f, 1.0f, velocity);
```
v=0 → velocity-independent (level 1); v=1 → full velocity; default 50% → `0.5 + 0.5·velocity`. Capture in `startNote`.

---

## 9. Vintage, Filter, Output, Lead-voice — Phase 2.2 / 2.3

### 9.1 Vintage (DSP-04) — Phase 2.2, NET-NEW, bounded, bypass at 0
Per-voice, **before** the filter. Full bypass at `vintage==0` (bit-for-bit clean — acceptance). Bounded ops → no NaN; covered by the block `isfinite` scrub:
```cpp
if (vintage > 0.0f) {
    const float fsEff = juce::jmap (vintage, 0.f,100.f, (float)fs, 3000.f);   // S&H decimate
    phase += fsEff / (float) fs; if (phase >= 1.0f) { phase -= 1.0f; held = x; } x = held;
    const float bits = juce::jmap (vintage, 0.f,100.f, 24.0f, 8.0f);          // bit-crush
    const float L = std::pow (2.0f, bits); x = std::round (x * (L*0.5f)) / (L*0.5f);
}
```
(Idiom for the lerp/quantize from O-simpleAdditive bit-depth lesson; the S&H is the SP-1200 model from ARCHITECTURE.)

### 9.2 Resonant LP filter (DSP-05, QUAL-02) — Phase 2.2
**ARCHITECTURE specifies `juce::dsp::StateVariableTPTFilter<float>` (LP)** — NOT the O-simpleSubtractive custom `SvfZDF`. Confirmed JUCE 8.0.9 API (`juce_StateVariableTPTFilter.h`): `prepare(ProcessSpec)`, `setType(Type::lowpass)`, `setCutoffFrequency(Hz)`, `setResonance(Q)`. **`resonance` is Q-like** (default `1/√2 ≈ 0.707` = Butterworth; internally `R2 = 1/resonance`). Map the 0–100% param:
```cpp
filt.setCutoffFrequency (juce::jlimit (20.0f, 0.45f*(float)fs, filterCutoff));
const float Q = juce::jmap (filterResonance, 0.f,100.f, 0.707f, 12.0f);  // Butterworth → resonant
filt.setResonance (Q);
```
Per-voice instance (16× is CPU-light). Smooth cutoff/resonance with `SmoothedValue` (20 ms) to avoid zipper (QUAL-01).

**Closed-form curve (QUAL-02) — reuse O-simpleSubtractive `SubVizAnalyzer.h:71` `SubFilterCurve::magnitudeDb` verbatim.** It is exact for any TPT SVF LP, with `k = 1/Q = R2`:
```
g = tan(π·fcDisplay/fs);  Ω = tan(π·f/fs)/g;  |H_LP| = 1 / sqrt((1−Ω²)² + (kΩ)²)
```
So the lead voice publishes `displayCutoffHz = fc` and `displayK = 1/Q` (= `1/resonance`) and the message thread draws the same g/k the audio uses → matches by construction. (`SubVizAnalyzer.h` bundles `VizRing` + `SubFilterCurve` + the curve driver `updateCurve`.)

> **Fallback:** if profiling ever shows 16× `StateVariableTPTFilter` is too heavy, drop in the 12-line `SvfZDF.h` linear core (copy from O-simpleSubtractive) — its g/k feed the *same* `magnitudeDb`. Not expected to be needed.

### 9.3 Output + lead-voice atomics (DSP-07) — Phase 2.3
**Output gain** (`PluginProcessor.cpp:162`): `outputGain.reset(sampleRate, 0.02);` then per block `setTargetValue(decibelsToGain(outDb,-60))` + `buffer.applyGainRamp(0, n, g0, outputGain.skip(n))`. Final `std::isfinite` scrub after summing voices.

**Lead-voice** (`SubVoice.h:269` caches `lastFcEff`/`lastK`; `PluginProcessor.cpp:283` `updateDisplayFromLeadVoice`): O-simpleSubtractive picks the **loudest-active** voice (max amp-env), publishes `displayCutoffHz`/`displayK`/env atomics once/block.
> **Divergence:** ARCHITECTURE says "most-recently-triggered". **Recommend the proven loudest-active pattern** (represents the dominant heard timbre; copy verbatim). If last-triggered is truly wanted, add a monotonic `triggerSeq` counter on each `startNote` and pick the max — small change, but loudest-active is the safer default.

**Voice-stealing:** `synth.setNoteStealingEnabled(true)` + 16 voices (`PluginProcessor.cpp:133`). Default JUCE stealing (quietest/oldest) is sufficient (DSP-07).

---

## 10. Loop engine (DSP-03) — Phase 2.2, NET-NEW

No verbatim sibling for a sampler loop seam, but the pieces are all in-repo idioms:
- **Equal-power crossfade primitive** = O-simpleGrain pan (`GrainVoice.h:339`): `cos(θ·π/2)`, `sin(θ·π/2)`. Apply across the seam: when `readPos` enters the last `xfadeSamp = loopCrossfade·fs/1000` before `loopAbsEnd`, run a 2nd read head from `loopAbsStart`; `out = a·cos(θ) + b·sin(θ)`, `θ: 0→π/2`; at the seam the primary head jumps to `loopAbsStart + xfadeSamp`.
- **Ping-pong:** negate read direction at each boundary; crossfade across the turnaround the same way.
- **Zero-cross snap:** when any of start/end/loopStart/loopEnd change, snap markers to the nearest source sign-change off the audio thread; publish int indices the voice reads (secondary click defense so even `loopCrossfade=0` is reasonable; the crossfade is primary).
- **Reverse** negates the base increment independently of loop direction.
- **Loop bounds are % of the region** → moving start/end rescales the loop (UI shades loop *inside* the region, Stage 3).

---

## 11. Stretch (synchronous-granular SOLA, DSP-01) — Phase 2.2, HEADLINE

Reuse the O-simpleGrain grain machinery; the only new wrapper is the **1× time axis**:
- **Grain POD** (`dsp/Grain.h:22`) — forward-phase model: `readPos += rate; phase += phaseInc; ++age; done when phase>=1`. `std::array<Grain, kMaxGrainsPerVoice=4>` per voice.
- **Spawn** (`GrainVoice.h:308`) — find-inactive-else-steal-oldest, RT-safe (no alloc). AA coeff + Hann via `WindowLuts->read(shape=4, phase)` (`WindowLuts.h:86`).
- **Stretch substitution:** replace O-simpleGrain's *density-driven* interval with a fixed **`grainHop = grainSize·(1−overlap)` = grainSize/2** (2× Hann overlap, grain ≈ 60 ms = `kStretchGrainMs`). The **time-axis read head** (`timePos`) advances at **1× realtime** through the region/loop (`timePos += 1.0`); each spawned grain captures `srcStart = timePos` and reads resampled by `keyRatio` (`g.rate = keyRatio`). Held note → duration preserved (time axis 1×), pitch tracks the key (grain resample). The waveform playhead = `timePos` → makes Repitch (pitch-coupled rate) vs Stretch (constant 1×) **visible for free** (UI-02).
- `pitchMode` toggles Repitch (continuous read) ↔ Stretch (grain overlap-add). Internal grain config fixed/hidden.

---

## 12. Viz tap (PERF-01) — Phase 2.3

Copy `O-simpleGrain/Source/VizAnalyzer.h` **verbatim** (rename `GrainVizAnalyzer → SamplerVizAnalyzer`). Contract: **audio thread COPY-ONLY into a pre-allocated lock-free ring; NO alloc / NO FFT / NO locks; FFT on the message thread (editor Timer 30 Hz)**.
- `VizRing` (`VizAnalyzer.h:36`): `std::array<std::atomic<float>, 8192>` (power-of-two), `write()` (audio, `memory_order_release` on `writePos`) / `readLatest()` (message, `acquire`). SPSC overwrite ring.
- **Write site** = tail of `processBlock`, post-gain, mono-summed in ≤4096-sample **stack** chunks (`PluginProcessor.cpp:771`).
- **FFT site** = editor `timerCallback()` 30 Hz → `analyzer.process(ring, fs)` → `emitEventIfBrowserIsVisible(...)`. Scope copied **before** the in-place `performFrequencyOnlyForwardTransform` (it clobbers its buffer). Analyzer lives on the **editor** (FFT cost only when UI open).
- Playhead: `std::atomic<double> displayPlayhead` (lead-voice read/time pos); filter curve: `displayCutoffHz`/`displayK`. Static waveform peaks computed off-thread on load, pushed once (Stage 3).

---

## 13. Render-harness — Stage-2 correctness gate (Phase 2.3)

**Port `O-simpleGrain/tests/render-harness/`.** `juce_add_console_app`; `target_sources` the plugin's `PluginProcessor.cpp` **+ `PluginEditor.cpp`** directly (no static lib — `createEditor()` must resolve at link); `add_dependencies` on the plugin **and both binary-data targets**; borrow includes via `$<TARGET_PROPERTY:O-simpleSampler,INCLUDE_DIRECTORIES>`; hand-define the `JucePlugin_*` macros; add `JUCE_MODAL_LOOPS_PERMITTED=1` (needed for `runDispatchLoopUntil` to pump the AsyncUpdater sample-decode). Gate: `option(OUARICON_BUILD_TESTS OFF)` + `if(...) add_subdirectory(tests/render-harness)`. Exit code 0 = all gates pass (not registered with CTest).

`main()` pattern (`main.cpp:392`): `ScopedJuceInitialiser_GUI`; stack processor; `setPlayConfigDetails(0,2,fs,512)` + `prepareToPlay`; `setParam`/`resetDefaults`; a 512-sample `render()` loop building `MidiBuffer` (note-on @0, optional note-off); `check` lambda tallies failures; `pumpMessages()` after a source change so the async decode lands.

**Probes available verbatim:** `rms`, `peakAbs`, `allFinite`, `continuityFraction` (click/gap), `autocorrPitchHz` (pitch), `Spectrum`(centroid/peak/band/flatness), `octaveShift` (comb-immune), `analyze` (Hann FFT).

**Stage-2 gates (per ROADMAP):**
- Note-on → audio; note-off → release tail → silence; lifetime gated on amp env.
- **Repitch tuning:** measured f0 at the seeded root = 131 Hz; an octave up = 2× (use `autocorrPitchHz`; a plain Repitch voice has **no grain comb**, so a single-bin DFT also works, but autocorr is the safe gate).
- **Stretch pitch/time independence:** held note keeps duration while pitch tracks the key — **single-grain `autocorrPitchHz` probe** at low grain density (`main.cpp:462` pattern: one grain at a time eliminates the grain-rate comb; window the grain body). Project memory: spectral bin probes are confounded by the grain comb.
- **Loop-seam click absence:** `continuityFraction` across a forward/ping-pong loop with crossfade.
- **Vintage clean-at-zero:** `vintage=0` output bit-for-bit equal to the clean read; increasing adds measurable quantization.
- **Anti-alias budget:** high keys (and extreme Stretch) — no spurious partials above tolerance (`Spectrum`/`octaveShift`).

---

## 14. Risks & mitigations (Stage 2)

| Risk | Sev | Mitigation (this research) |
|------|-----|----------------------------|
| Repitch tuning wrong (root mis-seed) | HIGH→LOW | Probed root = **48** (§6); render-harness asserts f0 at root + octave. |
| Upward-transposition aliasing | HIGH→LOW | `aaOnePole` `fc=0.5fs/rate` engaged on rate>1 (§2.3); proven O-simpleGrain decision. |
| `velToAmp` is net-new (no reference) | LOW | Trivial blend (§8); render-harness can assert velocity scaling. |
| Stretch not clearly distinct from Repitch | HIGH | 1× time-axis wrapper + fixed 60 ms/2× Hann (§11); single-grain autocorr gate. Phase 2.2. |
| Dual binary-data symbol collision | MED→LOW | Distinct `NAMESPACE` **and** `HEADER_NAME` (§7); copy O-simpleGrain. |
| Source hot-swap touches half-loaded buffer | MED→LOW | `atomic_load/store` shared_ptr; snapshot once/block (§4). |
| Filter class mismatch (arch says TPT, sibling uses SvfZDF) | LOW | Use `StateVariableTPTFilter` per ARCHITECTURE; curve math (`k=1/Q`) ports from `SubVizAnalyzer.h` (§9.2); SvfZDF is the documented fallback. |
| Lead-voice semantics (loudest vs last-triggered) | LOW | Use loudest-active (proven); flag (§9.3). |
| Loop seam click | MED | Equal-power cos/sin xfade + zero-cross snap (§10); harness `continuityFraction`. Phase 2.2. |
| `rootKey` seed vs frozen APVTS default | LOW | Keep default 60; seed live value at prepare/source-change, guarded against state-restore clobber (§6). |

---

## 15. Phase 2.1 execute checklist (immediate target)

1. `cp` piano.wav into `Source/samples/`; add `O-simpleSampler_Samples` binary-data target (`NAMESPACE BinaryData`) + link; `#include "BinaryData.h"`.
2. Copy `dsp/LagrangeInterpolation.h` from O-simpleGrain (verbatim). Add `readSourceLagrange` + `aaOnePole` statics.
3. `SamplerSound` (trivial) + `SamplerVoice : juce::SynthesiserVoice` (Repitch read head, AA, amp ADSR + `velToAmp` blend, non-virtual `prepareToPlay`). `SamplerVoiceParams` push struct.
4. Processor: build `juce::Synthesiser` (16 `SamplerVoice` + 1 `SamplerSound`, `setNoteStealingEnabled(true)`); `prepareToPlay` dispatch via `dynamic_cast`; `decodeAndPublish`/`resampleToEngineRate`/`loadBuiltInSource` + `currentSource` atomic shared_ptr; `sourceSample` listener → AsyncUpdater; per-source root seed (piano=48); per-block APVTS read → `setParams`/`setSource` → `synth.renderNextBlock`; output `SmoothedValue` + `isfinite` scrub.
5. Extend `setStateInformation` to re-decode the restored source + `cancelPendingUpdate()`.
6. Build VST3+AU+Standalone; `auval -v aumu OsSm OuDv` SUCCEEDED; pluginval@5 SUCCESS. **STOP for DAW play-test (CONTEXT D2).**

**Defer to 2.2:** loop engine, Stretch, Vintage, filter. **Defer to 2.3:** viz tap, voice-stealing audit, AA hardening, render-harness.

---

## 16. Reuse files to copy (paths)

| File | From | Action |
|------|------|--------|
| `dsp/LagrangeInterpolation.h` | `O-simpleGrain/Source/` | copy verbatim (2.1) |
| `samples/piano.wav` | `O-simpleGrain/Source/samples/` | copy (2.1) |
| `Grain.h`, `WindowLuts.h` | `O-simpleGrain/Source/dsp/` | copy for Stretch (2.2) |
| `SvfZDF.h` | `O-simpleSubtractive/Source/` | fallback only (2.2) |
| `VizAnalyzer.h` (→ `SamplerVizAnalyzer.h`) | `O-simpleGrain/Source/` | copy + rename (2.3) |
| `tests/render-harness/{main.cpp,CMakeLists.txt}` | `O-simpleGrain/` | port + adapt gates (2.3) |
| `SubVizAnalyzer.h` `SubFilterCurve` | `O-simpleSubtractive/Source/` | curve math reuse (2.2/3) |

---
*Research complete 2026-06-25. Inputs: ARCHITECTURE/ROADMAP/parameter-spec + verbatim extraction from O-simpleGrain & O-simpleSubtractive (4 parallel passes) + JUCE 8.0.9 source + piano.wav autocorrelation probe. Next: plan phase.*

---
---

# Phase 2.2 — Deep Research (Tone Chain → Stretch) · 2026-06-25

**Phase:** research (re-entry for Phase 2.2) → complete
**Trigger:** Phase 2.1 SHIPPED + DAW gate cleared (CONTEXT D3). Discuss split Phase 2.2 → **2.2a tone chain** → DAW checkpoint → **2.2b Stretch SOLA** (CONTEXT D4/D5).
**Inputs:** stages/2-dsp/CONTEXT.md "Open items for research phase (Phase 2.2)" (6 items) · the **actual shipped** `Source/SampleVoice.h` (2.1 Repitch read head, no longer the speculative skeleton) · `Source/PluginProcessor.cpp` processBlock param-push · ARCHITECTURE Algorithm Details · **3 parallel verbatim-extraction passes** over O-MicrotonalSampler / O-simpleGrain / O-simpleSubtractive / O-simpleAdditive + JUCE 8.0.9 `StateVariableTPTFilter` source.

> **Why a second research pass.** §9–§11 above were written *before* 2.1 existed and treated the loop/Vintage/filter/Stretch blocks as forward-scope. Now 2.1 is real code and the extraction passes surfaced material corrections to that forward-scope — most importantly that **the loop crossfade is a port, not net-new** (O-MicrotonalSampler ships it), that the **region-end declick is mandatory and independent of the amp release** (the 0.2 s release is unreachable from the region-end path), that the **S&H decimator is net-new** (no in-repo idiom — only the bit-crush ports), and that **O-simpleSubtractive does NOT use `StateVariableTPTFilter`** (only its closed-form curve ports; the JUCE runtime filter is net-new wiring). This section supersedes §9.1/§9.2/§10/§11 where they conflict.

---

## P0. Answers to the 6 Phase-2.2 open items (CONTEXT.md)

| # | Open item | Resolution | Verbatim? | Pass |
|---|-----------|------------|-----------|------|
| 1 | Loop crossfade mechanics (2nd read head, ping-pong, zero-cross) | **PORT** the dual-read-head equal-power crossfade from `O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp:72-111` (`readVariantWithLoop` + `wrapLoopPosition`). Forward + fixed-8-sample there → generalize to configurable `xfadeSamp = loopCrossfade·fs/1000`. **Ping-pong, zero-cross snap, configurable xfade = net-new.** **§P3.** | partial | loop |
| 2 | Region-end declick ramp + reconcile with 0.2 s release | The 0.2 s amp release **does NOT cover** the region-end cut: `SampleVoice.h:185-189` calls `ampEnv.reset()` (instant idle), **not** `noteOff()` (release ramp) — the release tail is only reachable from MIDI key-up. A **dedicated short raised-cosine end-ramp is required.** Port idiom = `O-MicrotonalSampler/Source/PluginProcessor.cpp:489-510` (`0.5−0.5·cos(π·k/fadeSamps)`, `fadeSamps = min(region/4, 5 ms·fs)`). **§P2.** | idiom | loop |
| 3 | Stretch SOLA port surface (time-1× + per-grain keyRatio; pool lifetime; pitchMode switch) | The `Grain` POD (`O-simpleGrain/dsp/Grain.h:22-47`) already has **independent `readPos` (time) + `rate` (pitch)** — SOLA-native. The **only structural gap:** O-simpleGrain snapshots a **per-block** playhead; SOLA needs a **voice-local per-sample `timePos`** (`+= 1.0` for 1× realtime) captured as `g.readPos` *at each grain's spawn sample*. Pool=4 safe (≤2 active at 2× overlap). **Latch `pitchMode` at note-on** (mid-note switch clicks; drain pattern optional later). **§P6.** | port+edit | stretch |
| 4 | Vintage formulation (fsEff/bits lerp, S&H+quantize order, clean-at-0) | **Bit-crush = VERBATIM** from `O-simpleAdditive/Source/AdditiveVoice.h:335-346`: `qLevel = 2^(bits−1)` (= L/2), `round(s·qLevel)/qLevel`, gated by `quantOn` (bit-clean off-branch). **S&H decimation = NET-NEW** (no in-repo idiom — grep of whole suite confirms). Order: **decimate (S&H) → quantize (bit-crush)**, both gated `if (vintage>0)`. **§P4.** | bit-crush verbatim / S&H net-new | filter+vintage |
| 5 | Filter (`StateVariableTPTFilter` setup, res→Q, closed-form curve) | JUCE 8.0.9 verified: `setResonance(Q)` is **Q-like** (default `1/√2`, internally `R2=1/resonance`), **asserts `>0`** (cannot pass 0). Per-voice: `prepare({fs,block,1})`, `processSample(0,x)`, `reset()` on note-on. Closed-form curve = **VERBATIM** `O-simpleSubtractive/Source/SubVizAnalyzer.h:71-110` (`SubFilterCurve::magnitudeDb`) **fed `k = 1/Q = R2`** (the bridge). `res%→Q` map (`jmap 0.707→~12`) is **net-new** (the in-repo `resonanceToK` is a self-oscillation taper — do NOT reuse). **§P5.** | curve verbatim / wiring+map net-new | filter+vintage |
| 6 | Grain-tuning starting point | O-simpleGrain **ships 30 ms grains @ density 40 → ~1.2× overlap** (`PluginProcessor.cpp:57,64`), hop = `fs/density` (density-driven, NOT `grainSize/2`). That is **NOT the SOLA baseline.** Use **60 ms (`kStretchGrainMs`) + fixed `hop = grainSize/2` ⇒ 2× Hann overlap** as the 2.2b starting point; tune grain length against the 2.3 single-grain autocorr harness. **§P6.4.** | new defaults | stretch |

---

## P1. Integration map — where 2.2 bolts onto the shipped 2.1 voice

**Current 2.1 per-sample chain** (`SampleVoice.h:167-190`): `readSourceLagrange(readPos)` → `aaOnePole` → `VCA (ampEnv·velLevel)` → `addSample`; `readPos += voiceRate`; one-shot `if (readPos≥endSamp) { ampEnv.reset(); break; }`.

**Target 2.2 per-sample chain (both pitch modes share the downstream):**
```
  ── source-sample generation (pitchMode) ──
  Repitch (2.1, default): s = aaOnePole( readSourceLagrange(readPos) )      [continuous head]
  Stretch (2.2b):         s = Σ grains[ aaOnePole(lagrange(g.readPos))·hann ] [overlap-add, AA per-grain]
  ── shared downstream (2.2a, per voice) ──
  s = vintage(s)        // S&H decimate → bit-crush ; bypass if vintage==0   (DSP-04)
  s = lpFilter(s)       // StateVariableTPTFilter processSample(0,s)         (DSP-05)
  out = s · ampEnv.getNextSample() · velLevel · endRamp                      (DSP-06 + declick)
  ── read-head advance (2.2a) ──
  Repitch: readPos += voiceRate·dir ; loop-wrap OR one-shot-end+declick
  Stretch: timePos  += 1.0·dir       ; loop-wrap on timePos ; grains ride keyRatio
```
The **Vintage → Filter → VCA** tail is identical for both modes (insert between the source-sample generation and the existing VCA). Only the **read head** forks on `pitchMode`.

### P1.1 `SamplerVoiceParams` extension (push struct, `SampleVoice.h:48-57`)
Add (processor fills from the already-cached atomics — all 9 deferred params are live in the layout, currently inert):
```cpp
int   loopMode      = 0;     // 0 off / 1 forward / 2 ping-pong
int   loopStartSamp = 0;     // ABS source-frame index (processor: % of region, zero-cross-snapped)
int   loopEndSamp   = 0;     // ABS source-frame index (exclusive)
int   xfadeSamp     = 0;     // loopCrossfade ms → samples (loopCrossfade·fs/1000)
bool  reverse       = false;
int   pitchMode     = 0;     // 0 Repitch / 1 Stretch  (LATCHED per-voice at startNote)
float vintage       = 0.0f;  // 0–100
float filterCutoff  = 20000; // Hz, already 20..0.45·fs clamped + 20 ms smoothed processor-side
float filterQ       = 0.707f;// mapped from filterResonance% processor-side (net-new jmap)
```
> **Smoothing lives in the processor, not 16× per-voice.** `filterCutoff`/`filterResonance` are global (no per-note mod — `velToFilter` is v1.1-deferred), so smooth ONE `SmoothedValue` each (20 ms, like `outputGain`) in the processor and push the current scalar. Voices call `setCutoffFrequency`/`setResonance` **once per block** (one `tan` recompute — cheap) — not per sample. This avoids 16 redundant smoothers and matches the existing `outputGain` pattern (`PluginProcessor.cpp:488-491`).

### P1.2 processBlock param-push additions (`PluginProcessor.cpp:449-474`)
Alongside the existing `startSamp/endSamp` computation, add (all cheap, per block):
- `loopMode = (int) loopModeParam->load()`; `reverse = reverseParam->load() > 0.5f`; `pitchMode = (int) pitchModeParam->load()`; `vintage = vintageParam->load()`.
- **Loop bounds (% of region):** `loopAbsStart = startSamp + loopStart%·(endSamp−startSamp)`, `loopAbsEnd = startSamp + loopEnd%·(endSamp−startSamp)`, clamped `loopAbsStart < loopAbsEnd ≤ endSamp`. Then overwrite with the **zero-cross-snapped** indices published by the off-thread snap (§P3.4).
- `xfadeSamp = jlimit(0, (loopAbsEnd−loopAbsStart)/2, (int)(loopCrossfade·0.001·fs))`.
- Filter: push the processor-smoothed `filterCutoff` (Hz) + `filterQ` (`jmap(res,0,100, 0.707, 12)`, §P5.2).

---

## P2 (2.2a). Region-end declick — open item 2

**The problem, confirmed by code reasoning.** `SampleVoice.h:185-189`:
```cpp
if (readPos >= (double) params.endSamp) { ampEnv.reset(); break; }
```
`ampEnv.reset()` sets the envelope to **idle/0 instantly** (it is the same call the voice-steal "hard stop" uses at `:143`) — it does **not** start the release ramp; only `ampEnv.noteOff()` does, and that is reachable only from MIDI key-up (`stopNote(allowTailOff=true)`, `:137`). So a **held** note whose End was dragged down hits `endSamp` while the env is still at sustain ≈ 1.0 → instantaneous drop to 0 → **click**. The 0.2 s release is structurally unreachable here. **A dedicated end-ramp is required** (do NOT widen/abuse `ampRelease` — 0.2 s would overrun `endSamp` and is user-owned).

**Port idiom — raised-cosine tail** (`O-MicrotonalSampler/Source/PluginProcessor.cpp:489-510`):
```cpp
// when readPos enters the last `endRampSamp` samples before endSamp, taper:
const int endRampSamp = juce::jmin ((params.endSamp - params.startSamp) / 4,
                                    (int) std::floor (0.005 * getSampleRate())); // ≤5 ms
const double distToEnd = (double) params.endSamp - readPos;        // samples remaining
float endRamp = 1.0f;
if (distToEnd < (double) endRampSamp && endRampSamp > 0)
    endRamp = (float) (0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi
                       * juce::jmax (0.0, distToEnd) / (double) endRampSamp));
// multiply into the VCA:  outv = s * ampVal * endRamp;
```
- **Scope:** applies only on the **one-shot path** (`loopMode==off`, or after the final pass). When looping, the seam crossfade (§P3) handles continuity and this ramp is bypassed.
- **Reverse interaction:** when `reverse`, playback terminates at `startSamp` instead — apply the same taper to `readPos − startSamp`.
- **Harness gate (2.3):** `continuityFraction` across a sustained note with End lowered mid-hold — no discontinuity at the boundary.

---

## P3 (2.2a). Loop engine — open item 1 (PORT + net-new extensions)

### P3.1 Equal-power primitive (verbatim) — `O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp:31-35`
```cpp
static inline std::pair<float,float> equalPowerWeights (float x) noexcept {
    const float t = juce::jlimit (0.0f, 1.0f, x) * juce::MathConstants<float>::halfPi;
    return { std::cos (t), std::sin (t) };           // {fade-out, fade-in}
}
// mix:  out = outSample * w.first + inSample * w.second;   (cpp:101)
```
(Same primitive as O-simpleGrain pan, `GrainVoice.h:350-351`.) For a configurable `xfadeSamp` (vs MicrotonalSampler's fixed 8) either compute `cos/sin` per seam-sample (≤500 ms of fade is rare and the cost is trivial) or precompute a small LUT on `xfadeSamp` change. Recommend **direct cos/sin** — the seam is a tiny fraction of samples.

### P3.2 Dual-read-head forward crossfade (PORT) — `MicrotonalSamplerVoice.cpp:72-111`
The shipped mechanism, generalized from fixed-8 to `xfadeSamp`:
```cpp
// primary head reads readPos; when within xfadeSamp of loopEnd, mix a 2nd head from loopStart:
const double fadeStart = (double) loopEnd - xfadeSamp;
float s;
if (readPos < fadeStart) {
    s = readSourceLagrange (src, len, (float) readPos);          // plain read
} else {
    const float x   = (float) ((readPos - fadeStart) / (double) xfadeSamp);  // 0→1 across fade
    const auto  w   = equalPowerWeights (x);
    const float out = readSourceLagrange (src, len, (float) readPos);                       // outgoing
    const float in  = readSourceLagrange (src, len, (float)(readPos - loopLen + xfadeSamp));// incoming (≈loopStart)
    s = out * w.first + in * w.second;
}
// advance + wrap (subtract loopLen — preserves fractional phase, no hard jump):
readPos += voiceRate;
while (readPos >= (double) loopEnd) readPos -= (double) loopLen;     // loopLen = loopEnd - loopStart
```
This is `readVariantWithLoop` + `wrapLoopPosition` (`cpp:104-111`) with `8` → `xfadeSamp`. The wrap **subtracts loopLen** rather than resetting to `loopStart`, preserving sub-sample phase. AA one-pole runs on `s` exactly as in 2.1.

### P3.3 Ping-pong (NET-NEW) — confirmed nothing in the suite reflects a sample read head
Grep confirmed only **delay** "PingPong" routing exists (`O-Bells/DelayProcessor`, `O-Formant`), not a reflecting read head. Net-new: carry a per-voice `int dir = +1`; at each boundary negate `dir` and **reflect** position, crossfading across the turnaround with the same equal-power pair:
```cpp
readPos += voiceRate * dir;
if (dir > 0 && readPos >= loopEnd)   { readPos = 2.0*loopEnd   - readPos; dir = -1; } // reflect at end
if (dir < 0 && readPos <= loopStart) { readPos = 2.0*loopStart - readPos; dir = +1; } // reflect at start
// turnaround xfade: near a boundary, mix the reflected head (reading the opposite direction)
```
`reverse` composes by seeding `dir = −1` and `readPos = endSamp` at note-on (independent of loop direction — ARCHITECTURE §Loop "reverse negates the base increment independently").

### P3.4 Zero-crossing snap (NET-NEW, off-thread) — confirmed not in suite
No marker→sign-change snap exists (`PluginProcessor.cpp:60` self-documents it as "lands Stage 2"). The detection kernel (sign flip of consecutive samples) appears in `O-Formant/dsp/VibratoLFO.h:58-62` but is not a snap. **Implementation:** the processor is **already** a `Listener` + `AsyncUpdater` (for `sourceSample`). Extend it: a `parameterChanged` on `start`/`end`/`loopStart`/`loopEnd` → `triggerAsyncUpdate()` → on the message thread, scan ±W samples (e.g. ±256) around each nominal marker for the nearest sign change (`src[i-1] < 0 != src[i] < 0`) minimizing `|src[i]|`, store snapped indices in `std::atomic<int>` members the audio thread reads once/block.
> **Secondary defense only.** The equal-power crossfade is the primary seam smoother; zero-cross snap keeps even `loopCrossfade=0` reasonable. If 2.2a schedule is tight, the snap can ship as a thin ±W scan (the crossfade carries correctness) — but CONTEXT D4 lists it in 2.2a, so include it.

### P3.5 Loop ↔ region ↔ declick interaction
- Loop bounds are **% of region** → moving `start`/`end` rescales the loop (UI shades loop inside region, Stage 3). Computed processor-side each block (§P1.2).
- `loopMode==off` → existing one-shot path + **end-ramp** (§P2). `forward`/`ping-pong` → loop-wrap, **no** end-ramp (the note sustains until key-up → amp release).
- **Harness gate (2.3):** `continuityFraction` across forward + ping-pong seams with `loopCrossfade` 0 / 10 / 100 ms.

---

## P4 (2.2a). Vintage — open item 4 (bit-crush PORT + S&H NET-NEW)

Per-voice, **before** the filter (ARCHITECTURE: LP tames decimation aliasing — SP-1200-into-filter). **Full bypass at `vintage==0`** is a hard acceptance (DSP-04) — branch, do not rely on transparent endpoints.

### P4.1 Bit-crush (VERBATIM idiom) — `O-simpleAdditive/Source/AdditiveVoice.h:335-346`
```cpp
// precompute once per block (NOT per sample):
const bool  quantOn = (vintage > 0.0f);
const float bits    = juce::jmap (vintage, 0.f, 100.f, 16.0f, 8.0f);   // clean→8-bit (ARCH FS_MIN/BITS_MIN)
const float qLevel  = quantOn ? std::exp2 (bits - 1.0f) : 1.0f;        // = L/2 for L=2^bits
const float qInv    = quantOn ? 1.0f / qLevel : 1.0f;
// per sample (after S&H):
if (quantOn) s = std::round (s * qLevel) * qInv;     // mid-tread; std::round is symmetric ± (no sign branch)
```
`qLevel = 2^(bits−1)` ≡ your `L/2` (`L=2^bits`); `round(s·qLevel)/qLevel` ≡ `round(x·L/2)/(L/2)`. **Match.** No dither (the staircase IS the lesson). The `quantOn` gate is the bit-clean off-branch — replicate its structure.

### P4.2 Sample-and-hold decimation (NET-NEW) — no in-repo idiom (grep-confirmed)
Phase-accumulator S&H (fractional ratios; the SP-1200 grit), gated identically:
```cpp
// per voice state: float shPhase = 0.0f, shHeld = 0.0f;
if (vintage > 0.0f) {
    const float fsEff = juce::jmap (vintage, 0.f, 100.f, (float) fs, 3000.f);  // FS_MIN ≈ 3000 Hz
    shPhase += fsEff / (float) fs;
    if (shPhase >= 1.0f) { shPhase -= 1.0f; shHeld = s; }
    s = shHeld;                       // hold
}                                     // else: untouched — bit-for-bit clean
// then bit-crush (P4.1)
```
Reset `shPhase=0, shHeld=0` on note-on. Bounded ops (no transcendental, no NaN) — covered by the block `isfinite` scrub. **Order = decimate → quantize** (S&H first, then bit-reduce the held value).

### P4.3 Clean-at-zero gate
Both stages branch on `vintage > 0.0f`. At `vintage==0` the sample passes untouched (no `round`, no hold) — **bit-for-bit clean** (DSP-04 acceptance). **Harness gate (2.3):** `vintage=0` output == clean read sample-for-sample; `vintage=70` adds measurable quantization steps.

---

## P5 (2.2a). Filter — open item 5 (curve VERBATIM + JUCE wiring/map NET-NEW)

### P5.1 `juce::dsp::StateVariableTPTFilter<float>` — JUCE 8.0.9 verified
- API: `setType(Type::lowpass)` (only `lowpass/bandpass/highpass`), `setCutoffFrequency(Hz)`, `setResonance(Q)`, `prepare(const ProcessSpec&)`, `reset()`, `processSample(int channel, float)`.
- **`setResonance` is Q-like:** default `1/√2 ≈ 0.707` = Butterworth (12 dB/oct); internally `R2 = 1/resonance` (`.cpp:133`). **Asserts `resonance > 0`** (`.cpp:63`) — **never pass 0** (no self-oscillation, unlike the ZDF's negative-k trick — irrelevant here, LP is linear).
- **Per-VOICE mono:** in the voice's `prepareToPlay`, `filt.prepare({ fs, (uint32) blockSize, 1 })` (numChannels = **1**); per sample `s = filt.processSample(0, s)`; **`filt.reset()` in `startNote`** (clear state so a stolen voice doesn't carry the previous note's tail). State is `s1[ch]/s2[ch]` indexed by channel (`.cpp:110-111`).
- `setCutoffFrequency`/`setResonance` each call `update()` (one `tan`) — call **once per block** from the processor-pushed smoothed scalars (§P1.1), not per sample.
- Clamp cutoff `jlimit(20.0f, 0.45f*fs, filterCutoff)`.

### P5.2 Resonance% → Q (NET-NEW — do NOT reuse `resonanceToK`)
The in-repo `SubVoice.h:309-313 resonanceToK` is a **self-oscillation power-law taper** (drives k negative at the top) — wrong for a linear LP. Net-new linear-ish map:
```cpp
const float Q = juce::jmap (filterResonance, 0.f, 100.f, 0.707f, 12.0f);  // Butterworth → resonant
filt.setResonance (Q);     // never 0
```
(`filterResonance` default 0 → Q 0.707 = open Butterworth; QUAL: smooth with the 20 ms processor `SmoothedValue`.)

### P5.3 Closed-form curve (VERBATIM) — staged for Stage 3, atomics published in 2.2a
Copy `O-simpleSubtractive/Source/SubVizAnalyzer.h:71-110` `SubFilterCurve::magnitudeDb` **verbatim**. The bridge: it expects `k = 1/Q`, and JUCE's `R2 = 1/resonance` **is** that `k` numerically. LP path only:
```
g = tan(π·fcDisplay/fs);  W = tan(π·f/fs)/g;  |H_LP| = 1 / sqrt((1−W²)² + (k·W)²)   // k = 1/Q = R2
```
**2.2a action (CONTEXT D4 "stage the atomics now"):** the **loudest-active** voice publishes `displayCutoffHz = smoothedCutoff` and `displayK = 1/Q` once per block (O-simpleSubtractive lead-voice pattern, `PluginProcessor.cpp:410`). The curve **drawing** is Stage 3; only the atomics land in 2.2a so the audio/curve g/k match by construction (QUAL-02). `displayK` semantics = `1/Q` (== R2), matching `magnitudeDb`'s `k`.

---

## P6 (2.2b). Stretch SOLA — open items 3 + 6 (HEADLINE)

**Copy 3 files verbatim** (already partly present): `dsp/LagrangeInterpolation.h` (already in 2.1), `dsp/Grain.h`, `dsp/WindowLuts.h` (Hann = shape 4). The grain **render loop**, `readSourceLagrange`, and `aaOnePole` are already in `SampleVoice.h` (ported in 2.1) — reuse them per grain.

### P6.1 The Grain POD is SOLA-native (no struct change) — `Grain.h:22-47`
Two independent fields are exactly the time/pitch split: `float readPos` (absolute source position = **time axis**, advanced `readPos += rate`) and `float rate` (per-sample increment = **pitch axis**, set once at spawn). Per-grain AA (`aaCoeff/aaEngaged/aaState`) and Hann (`phase/phaseInc`) are already in the struct.

### P6.2 The ONE structural gap: voice-local per-sample `timePos`
O-simpleGrain snapshots a **per-block** playhead (`PluginProcessor.cpp:720 setPlayhead`) and all grains in a block share that `srcStart` — fine for a texture cloud, **wrong for SOLA** (each grain's `srcStart` must be the time pointer *at its spawn sample*). Fix: a per-voice `double timePos` advanced **per output sample** inside `renderNextBlock`:
```cpp
timePos += 1.0 * dir;          // 1× realtime (duration preserved regardless of key); dir for reverse
// loop-wrap timePos exactly like the Repitch readPos (§P3.2) — the time axis is what loops/reverses
```
At each grain spawn: `g.readPos = timePos;` `g.rate = voiceRate;` (= keyRatio; zero all O-simpleGrain spray — `positionSpray/pitchSpray/scatter/panSpray` = 0). The waveform playhead = `timePos` → Repitch (pitch-coupled rate) vs Stretch (constant 1×) is **visible for free** (UI-02).

### P6.3 Replace the density scheduler with a fixed SOLA hop — `GrainVoice.h:228-239`
O-simpleGrain's hop is **density-driven** (`baseInterval = fs/density` + scatter jitter). Replace the reload with a deterministic fixed hop; keep the `if (--samplesUntilNextGrain<=0)` countdown structure:
```cpp
const float lenSamp  = juce::jmax (2.0f, (kStretchGrainMs * 0.001f) * (float) fs);  // 60 ms
const float phaseInc = 1.0f / lenSamp;
// on fire:
spawnGrainSOLA (timePos, voiceRate, lenSamp, phaseInc);
samplesUntilNextGrain = juce::jmax (1, (int) (lenSamp * 0.5f));   // hop = grainSize/2 ⇒ 2× Hann overlap (NO jitter)
```
**Also drop the `√overlap` normalizer** (`O-simpleGrain/PluginProcessor.cpp:755-756`) — a texture-cloud power-law heuristic. A 2× Hann overlap-add is already unity-gain (COLA).

### P6.4 Grain-tuning baseline (open item 6)
O-simpleGrain ships **30 ms @ density 40 ≈ 1.2× overlap** — **NOT** the SOLA baseline. Start 2.2b at **60 ms (`kStretchGrainMs`) + `hop = grainSize/2` ⇒ 2× Hann overlap**, then tune grain length against the **2.3 single-grain autocorrelation probe** (project memory: the grain-rate comb confounds spectral probes). Levers if A/B isn't clean: longer grains (less transient smear, more latency-feel) vs shorter (tighter transients, more graininess); 3× overlap as a fallback (CONTEXT D5 — phase-vocoder stays v1.1-deferred).

### P6.5 Pool lifetime + pitchMode switch
- **Pool = 4 is SAFE.** Active grains = grainLen/hop = overlap = **2** at 2× overlap; pool 4 = 2× headroom (boundary frames + mode drain). `spawnGrain`'s find-inactive/steal-oldest (`GrainVoice.h:312-320`) never allocates even on a momentary 3rd grain (it steals the oldest = the one already fading). `kMaxGrainsPerVoice=4` is already a `PluginProcessor.h` constant — use it (O-simpleGrain's 24 is cloud-sized; 4 is correct for SOLA and saves the pool-scan cost).
- **Latch `pitchMode` at note-on** (cleanest for v1). A hard mid-note switch clicks (truncates in-flight grain windows). On note-on, the latched path is fixed for the voice; reset grain pool (`for g: g.active=false`), `samplesUntilNextGrain=0` (fire immediately), seed `timePos` at the read position so no jump. **Optional later:** O-simpleGrain's drain pattern (stop spawning old path, let active grains finish their Hann taper while the new path spawns) for click-free live toggling — defer unless DAW A/B demands it.

### P6.6 Stretch ↔ downstream
The overlap-add sum (AA already applied per grain) is the `s` fed to **Vintage → Filter → VCA** (§P1) — identical tail to Repitch. The end-ramp (§P2) and loop-wrap operate on `timePos` in Stretch (not the grain `readPos`).

---

## P7. Risks & mitigations (Phase 2.2)

| Risk | Sev | Mitigation (this research) |
|------|-----|----------------------------|
| Region-end click (held note, End lowered) | MED→LOW | Dedicated raised-cosine end-ramp (§P2) — the 0.2 s release is unreachable from this path; harness `continuityFraction`. |
| Loop seam click | MED→LOW | PORT the shipped equal-power dual-head crossfade (§P3.2) + zero-cross snap (§P3.4); never hard-jump (wrap subtracts loopLen). |
| Ping-pong / zero-cross are net-new | MED | Small, bounded edits (§P3.3/P3.4); harness seam probe on both modes. |
| Vintage not bit-clean at 0 | MED→LOW | Explicit `vintage>0` gate on BOTH S&H and bit-crush (§P4.3); S&H is net-new so its gate is mandatory (bit-crush gate is the verbatim `quantOn`). |
| Filter: `setResonance(0)` assert / Q-vs-k confusion | LOW | Map res%→Q `0.707..12` (never 0); curve fed `k=1/Q=R2` (§P5.2/P5.3) — verified against JUCE source. |
| Stretch not clearly distinct from Repitch (DSP-01) | **HIGH** | Voice-local 1× `timePos` (§P6.2) + fixed `grainSize/2` hop + 60 ms/2× baseline (§P6.4); drop √overlap normalizer; single-grain autocorr harness (2.3). |
| pitchMode mid-note click | MED→LOW | **Latch at note-on** (§P6.5); reset pool + seed timePos. Drain pattern deferred. |
| Grain pool overflow | LOW | Pool=4 ≥ 2× the 2-active worst case; steal-oldest never allocates (§P6.5). |
| 16× per-voice filter/Vintage CPU | LOW | Both are a few ops/sample; filter coeff update once/block; ARCHITECTURE §Special-Considerations confirms light. SvfZDF is the documented fallback. |

---

## P8. Execute checklists

### Phase 2.2a — Tone chain (lower risk, inherited patterns) → DAW checkpoint
1. **Region-end declick** (§P2) — raised-cosine end-ramp on the one-shot path; `SampleVoice.h:184-189`.
2. **Loop engine** (§P3) — extend `SamplerVoiceParams` (loopMode/loopAbs*/xfadeSamp/reverse); processor loop-bounds + xfade math (§P1.2); PORT dual-head equal-power crossfade (§P3.2); ping-pong reflect (§P3.3); reverse seed at note-on; off-thread zero-cross snap via the existing Listener+AsyncUpdater (§P3.4).
3. **Vintage** (§P4) — per-voice S&H decimate (net-new) → bit-crush (verbatim), both gated `vintage>0`, before the filter; reset S&H state on note-on.
4. **Filter** (§P5) — per-voice `StateVariableTPTFilter` LP; `prepare({fs,block,1})` in voice prepare; `reset()` on note-on; processor-smoothed cutoff + `jmap` Q pushed per block; `processSample(0,s)`; clamp cutoff `0.45·fs`. Stage `displayCutoffHz`/`displayK`(=1/Q) loudest-active atomics (curve drawing = Stage 3).
5. **Build + auval + pluginval@5 + install → STOP** for DAW play-test (loop sustains, no seam click; ping-pong/reverse; Vintage clean-at-0 → grit; filter shapes tone; no zipper).

### Phase 2.2b — Stretch SOLA (HEADLINE) → DAW A/B
6. Copy `dsp/Grain.h` + `dsp/WindowLuts.h` (Hann shape 4) verbatim.
7. Add per-voice `std::array<Grain,kMaxGrainsPerVoice>` + voice-local `double timePos` + `samplesUntilNextGrain` (§P6.2/P6.3). `spawnGrainSOLA`: `g.readPos=timePos; g.rate=voiceRate;` zero spray; per-grain AA from `g.rate`.
8. Fixed `hop = grainSize/2` (60 ms baseline); overlap-add render (reuse 2.1 `readSourceLagrange`/`aaOnePole`); drop √overlap normalizer.
9. `pitchMode` **latched at note-on** (§P6.5): reset pool, `samplesUntilNextGrain=0`, seed `timePos`. Stretch feeds the same Vintage→Filter→VCA tail; loop-wrap/end-ramp operate on `timePos`.
10. **Build + validate + install → STOP** for DAW A/B (toggle Pitch Mode on a sustained note: Repitch slows/shortens low notes; Stretch holds duration, pitch tracks key). Tune grain length against the 2.3 autocorr harness.

> 2.3 (AA hardening + viz taps + voice-stealing + RT-safety + offline render-harness) remains the subsequent Stage-2 pass — unchanged. The render-harness Stretch gate = **single-grain autocorrelation** (spectral bins confounded by grain comb — project memory).

---

## P9. Files to copy / create (Phase 2.2)

| File | From | Action | Pass |
|------|------|--------|------|
| `Source/SampleVoice.h` | — (extend shipped 2.1) | add loop/declick/Vintage/filter to render path + `SamplerVoiceParams` | 2.2a |
| `Source/PluginProcessor.{h,cpp}` | — | param-push loop bounds/xfade/Vintage/filter; processor `SmoothedValue` cutoff/Q; zero-cross-snap AsyncUpdater; loudest-active display atomics | 2.2a |
| equal-power dual-head crossfade idiom | `O-MicrotonalSampler/MicrotonalSamplerVoice.cpp:31-111` | port (generalize 8→xfadeSamp) | 2.2a |
| raised-cosine end-ramp idiom | `O-MicrotonalSampler/PluginProcessor.cpp:489-510` | port | 2.2a |
| bit-crush idiom | `O-simpleAdditive/AdditiveVoice.h:335-346` | port verbatim | 2.2a |
| `SubFilterCurve::magnitudeDb` + `updateCurve` | `O-simpleSubtractive/SubVizAnalyzer.h:71-110,173-182` | copy verbatim (feed k=1/Q) | 2.2a (atomics) / Stage 3 (draw) |
| `Source/dsp/Grain.h` | `O-simpleGrain/Source/dsp/Grain.h` | copy verbatim | 2.2b |
| `Source/dsp/WindowLuts.h` | `O-simpleGrain/Source/dsp/WindowLuts.h` | copy verbatim (Hann=4) | 2.2b |
| grain spawn/render/AA idiom | `O-simpleGrain/GrainVoice.h:228-263,308-422` | port + SOLA edits (§P6.2/P6.3) | 2.2b |

**Net-new (no port):** ping-pong reflection (§P3.3), zero-cross snap (§P3.4), S&H decimation (§P4.2), res%→Q jmap (§P5.2), voice-local `timePos` + fixed SOLA hop (§P6.2/P6.3).

---
*Phase 2.2 deep research complete 2026-06-25. Inputs: CONTEXT Phase-2.2 open items + shipped 2.1 SampleVoice.h/PluginProcessor.cpp + ARCHITECTURE + 3 parallel verbatim-extraction passes (O-MicrotonalSampler / O-simpleGrain / O-simpleSubtractive / O-simpleAdditive) + JUCE 8.0.9 StateVariableTPTFilter source. Supersedes §9.1/§9.2/§10/§11 where they conflict. Next: plan phase (Phase 2.2 — split 2.2a tone chain → 2.2b Stretch).*
