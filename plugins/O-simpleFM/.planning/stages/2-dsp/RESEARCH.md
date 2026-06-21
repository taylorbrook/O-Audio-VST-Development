# Stage 2 (DSP) — RESEARCH

**Plugin:** O-simpleFM · **Stage:** 2 DSP · **Date:** 2026-06-20 · **Mode:** express
**Method:** Explore agent extracted exact, copy-ready reference code from the Ouaricon suite. All call sites verified against current source. JUCE 8.0.9 APIs confirmed.

---

## 1. Voice architecture — O-Bassoon (excellent match)

**Sound:** `class FMSound : juce::SynthesiserSound { appliesToNote→true; appliesToChannel→true; }` (O-Bassoon `BassoonSound.h:15`).

**Voice:** `class FMVoice : juce::SynthesiserVoice`. `canPlaySound` = `dynamic_cast<FMSound*>(sound) != nullptr`.

**CRITICAL — JUCE 8 voice prepare (no virtual):** declare a **non-virtual** `void prepareToPlay(double sr, int maxBlock);` (NO `override`). Body calls `setCurrentPlaybackSampleRate(sr)` then **`adsr.setSampleRate(sr)` BEFORE any `adsr.setParameters(...)`** (JUCE 8 ADSR contract — `recalculateRates` uses stored SR). Ref: `BassoonVoice.cpp:31-48`.

**Processor dispatch (linchpin):** in `OSimpleFMAudioProcessor::prepareToPlay`:
```cpp
synth.setCurrentPlaybackSampleRate (sampleRate);          // (×OS factor — see §4)
for (int v = 0; v < synth.getNumVoices(); ++v)
    if (auto* fv = dynamic_cast<FMVoice*>(synth.getVoice(v)))
        fv->prepareToPlay (sampleRate, samplesPerBlock);
```
Ref: `O-Bassoon/PluginProcessor.cpp:156-184`.

**Voice allocation (ctor, pre-allocate — no audio-thread alloc later):**
```cpp
for (int i = 0; i < 16; ++i) { auto* v = new FMVoice(); v->setAPVTS(&parameters); synth.addVoice(v); }
synth.addSound (new FMSound());
```
Ref: `O-Bassoon/PluginProcessor.cpp:111-139`. A plain `juce::Synthesiser` member suffices (16 fixed voices); the `BassoonSynthesiser` `findFreeVoice` override is only for a *runtime* voice cap — not needed.

**Block param-push:** read APVTS atomics once per block via `parameters.getRawParameterValue("id")->load()`, push to each voice via a `setParams(...)` aggregate setter. O-Bassoon adds change-detection "shadow" members to skip dispatch when nothing moved (`PluginProcessor.cpp:204-326`); O-Marimba pushes unconditionally every block. **Decision: unconditional push** (16 voices × a few floats is trivial; change-detection is premature). Voices read APVTS directly only for one-shot note-on values if needed.

**renderNextBlock invariants (`BassoonVoice.cpp:192-263`):** early-return if `!ampEnv.isActive()`; per-sample loop; **`addSample` to BOTH channels** (sum, don't overwrite); after loop, `if(!ampEnv.isActive()){ clearCurrentNote(); /*reset feedback history etc*/ }`.

**startNote/stopNote:** `startNote` computes `fc` from `MidiMessage::getMidiNoteInHertz(note)` + pitch-wheel bend, sets velAmp, **resets feedback history (not phase)**, `ampEnv.noteOn(); modEnv.noteOn();`. `stopNote`: `if(allowTailOff){ampEnv.noteOff(); modEnv.noteOff();} else { clearCurrentNote(); ampEnv.reset(); modEnv.reset(); }`. Ref: `BassoonVoice.cpp:50-122`.

**MIDI:** fully handled by `synth.renderNextBlock(buffer, midi, 0, n)` — no manual iteration. (But see §4: MIDI sample offsets must be scaled when rendering oversampled.)

**Bus support (synth):** `O-Bassoon/PluginProcessor.cpp:191-202` — stereo out, reject any input bus. (O-simpleFM already has this from Stage 1.)

---

## 2. Lock-free viz ring — O-Marimba `WaveformFifo`

Fixed-size, **allocation-free** member (nothing to allocate in prepare):
```cpp
class VizRing {
    static constexpr int kSize = 8192;                 // power of two; ≥ FFT 4096 + scope window
    std::array<std::atomic<float>, kSize> buf {};
    std::atomic<int> writePos { 0 };
public:
    void write(const float* d, int n) noexcept {       // AUDIO THREAD, copy-only
        int w = writePos.load(std::memory_order_relaxed);
        for (int i=0;i<n;++i){ buf[w]=d[i]; w=(w+1)&(kSize-1); }
        writePos.store(w, std::memory_order_release);
    }
    void readLatest(float* dst, int n) const noexcept {// MESSAGE THREAD
        int w = writePos.load(std::memory_order_acquire);
        int start = (w - n) & (kSize-1);
        for (int i=0;i<n;++i) dst[i]=buf[(start+i)&(kSize-1)].load(std::memory_order_relaxed);
    }
};
```
Ref: `O-Marimba/PluginProcessor.h:19-54`, write at `.cpp:276-279`. O-Marimba uses modulo; power-of-two + bitmask is the cheap equivalent. Audio-thread write taps the **post-gain mono sum**.

---

## 3. FFT + window — O-MultiBandCompressor pattern (we run on Timer, not processBlock)

```cpp
static constexpr int kFftOrder = 12;                    // 4096 (spec: separate sidebands)
static constexpr int kFftSize  = 1 << kFftOrder;
juce::dsp::FFT fft { kFftOrder };
juce::dsp::WindowingFunction<float> window { kFftSize,
    juce::dsp::WindowingFunction<float>::blackmanHarris };   // spec: BH, not Hann
std::array<float, kFftSize*2> work {};                  // 2× for freq-only transform
// on message thread (editor Timer):
ring.readLatest(scope, kScopeWin);                      // COPY SCOPE WINDOW FIRST
ring.readLatest(work.data(), kFftSize);
window.multiplyWithWindowingTable(work.data(), kFftSize);
fft.performFrequencyOnlyForwardTransform(work.data());  // magnitudes in work[0..size/2]
```
Ref: `O-MultiBandCompressor/PluginProcessor.h:98-102`, `.cpp:518-573`. **Verified gotcha (ARCHITECTURE risk #4):** `performFrequencyOnlyForwardTransform` overwrites its work buffer in place → copy the scope window BEFORE running it. `blackmanHarris` enum confirmed present in `juce_dsp/maths/juce_Windowing.h`.

---

## 4. Oversampling — write IIR variant fresh; render synth at 2× then decimate

**Suite only uses `filterHalfBandFIREquiripple`** (`O-AnalogSaturation/PluginProcessor.cpp:71-89`, `TapeAge`). The spec's `filterHalfBandPolyphaseIIR` is a valid JUCE 8 enum (lower latency, non-linear phase) — use it fresh.

**Construction (prepare):**
```cpp
oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
    2 /*channels*/, 1 /*log2(2×)*/,
    juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
    true /*maxQuality*/, false /*no gain normalise*/);
oversampler->initProcessing ((size_t) samplesPerBlock);
oversampler->reset();
setLatencySamples ((int) std::round (oversampler->getLatencyInSamples()));
```

**CRITICAL synth-OS pattern (NOT the effect up→process→down):** a synth has no input — to actually band-limit the *generated* sidebands the voices must render at the oversampled rate, then decimate:
```cpp
buffer.clear();
juce::dsp::AudioBlock<float> base (buffer);
auto os = oversampler->processSamplesUp (base);          // returns internal 2N block (silence)
// wrap the internal block as an AudioBuffer the Synthesiser can render into:
float* ch[2] = { os.getChannelPointer(0), os.getChannelPointer(1) };
juce::AudioBuffer<float> osBuf (ch, (int) os.getNumChannels(), (int) os.getNumSamples());
osBuf.clear();
synth.renderNextBlock (osBuf, scaledMidi, 0, (int) os.getNumSamples());   // synth SR = fs×2
oversampler->processSamplesDown (base);                  // decimate → buffer
```
- **Synth playback SR must be `sampleRate × 2`** (set in prepare); voices' `prepareToPlay` also get `sampleRate × 2` so phase increments + ADSR + ceiling Nyquist are all computed at the oversampled rate.
- **MIDI offsets must be scaled ×2** into a reused member `juce::MidiBuffer` (clear()+addEvent in a loop — no alloc). Unscaled MIDI would place note-ons up to ~half a block early.
- `getLatencyInSamples()` is in the *base* rate domain — report directly.

---

## 5. fastSine — `LookupTableTransform` written fresh (ABSENT in suite)

Repo only has `dsp::Oscillator::initialise` sine tables (O-DigiDelay/O-Detune) — wrong tool (carries own phase). For PM we index a table by the *modulated* phase:
```cpp
struct SineLUT {
    juce::dsp::LookupTableTransform<float> t;
    SineLUT(){ t.initialise([](float x){ return std::sin(x); },
                            0.0f, juce::MathConstants<float>::twoPi, 1024); }
    inline float operator()(float phase) const noexcept {
        constexpr float twoPi = juce::MathConstants<float>::twoPi;
        phase -= twoPi * std::floor (phase / twoPi);     // MANDATORY: LUT clamps out-of-range
        return t (phase);
    }
};
```
1024 points / linear ≈ 97 dB SNR (spec). The floor-wrap is load-bearing: the PM argument `carPhase + I·modOut` swings to many ×2π at high index, and `LookupTableTransform` *clamps* (not wraps) inputs outside `[0,2π]`.

---

## Decisions locked from research

- **Direct orchestrator authoring** (not dsp-agent delegation) for execute — matches Stage 1 precedent; the dsp-agent cannot build, and tight build→fix iteration + full-context contract fidelity is lower-risk for DSP this dense. Noted as deviation in SUMMARY.
- **Synth renders at 2× rate; decimate via Oversampling** (the only way OS actually fights FM aliasing). MIDI scaled ×2 into a member buffer.
- **`filterHalfBandPolyphaseIIR`** as specced (fresh; suite uses FIR elsewhere).
- **FFT/scope on the editor 30 Hz Timer** (per ARCHITECTURE), not processBlock (the MBC deviation). Stage 2 editor stays Generic but gains the Timer + analyzer so Stage 3 only swaps the body for WebView + `emitEventIfBrowserIsVisible`.
- **Unconditional block param-push** (no change-detection — premature).
- **Power-of-two viz ring + bitmask** (cheaper than O-Marimba's modulo).
