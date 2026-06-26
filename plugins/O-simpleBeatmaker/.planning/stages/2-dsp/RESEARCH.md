# Stage 2 (DSP) — RESEARCH

> **Scope:** implementation mechanics for the genuinely-new / highest-risk pieces of Stage 2.
> The DSP math, voice table, swing/humanize/quantize formulas, and the verified API list are
> already locked in `research/ARCHITECTURE.md` and `parameter-spec.md` — this document does **not**
> re-derive or re-litigate them. It nails down *how* to wire them in JUCE 8.0.9, with signatures
> and code sketches verified against local source at `/Users/taylorbrook/JUCE/modules/` and the
> O-simpleFM sibling.
> **Date:** 2026-06-25 · **JUCE:** 8.0.9 (local) · **Confidence:** HIGH (all core APIs read from source).

---

## User Constraints (from CONTEXT.md / locked contracts)

### Locked Decisions
- **Params:** stored ranges/IDs LOCKED (`parameter-spec.md`). swing/humanize/quantize/tone/decay stored 0–1; tune ±12 st; levels −60..0 dB; `quantizeStrength` default **1.0**; `patternLength` default 16 (idx 1).
- **Grid is NOT APVTS** — read lock-free via the existing `std::array<std::atomic<uint8_t>, 6*32> grid` (0=off, 1–127=on@velocity). Never add 384 params.
- **Per-voice decay-ms mapping** lives in `DrumVoiceEngine`, NOT in the param range (kick boom vs hat tick differ by an order of magnitude).
- **Latency:** zero — `setLatencySamples(0)` in `prepareToPlay`. **Never override the non-virtual `getLatencySamples()`.**
- **Quantize must NOT touch swing** — compute `Δswing` and `Δhuman` as separate terms; `q` scales only `Δhuman`. Single most important correctness invariant (DSP-04).
- **Sequencer emits GM MIDI into the SAME `MidiBuffer` as host MIDI**, sorted by sample position; voices sub-slice render on event offsets. The viz tap reads the same emitted messages (QUAL-02 by construction).
- **Thread safety:** APVTS read once/block via cached `getRawParameterValue()->load()`; RNG pre-seeded in `prepareToPlay`, never reseeded on the audio thread; audio→UI strictly `AbstractFifo` + `std::atomic`. `processBlock` allocation-free + lock-free (PERF-01).

### Claude's Discretion (this stage)
- Internal voice struct layout, envelope coefficient storage, sub-slice loop structure, the render-harness test-hook surface, and the carry-over mechanism for negative humanize offsets (see §Mechanism Gaps).
- Triangular vs flat RNG distribution for humanize (ARCHITECTURE *recommends* triangular).

### Deferred / OUT OF SCOPE
- WebView UI (Stage 3), factory presets (Stage 4), per-voice pan, 6-oscillator metallic hat source, 8th-note swing toggle, rimshot/cowbell.

---

## Phase Requirements (ROADMAP traceability)

| ID | Description | Research support |
|----|-------------|------------------|
| FUNC-01/02/03/04/06/07 | Synth voices, MIDI-playable, host-synced sequencer, grid playback, transport | §2.1 voices, §2.2 clock, §Sub-slice |
| DSP-01/02/03 | Voice synthesis, envelopes, filtering | §2.1 |
| DSP-04 | swing preserved under quantize | §2.3 + §Pitfalls |
| DSP-05 | sample-accurate sub-step Δt | §2.2 §Sub-slice + §Mechanism Gaps |
| DSP-06 / QUAL-01 | aliasing budget on high-rate hits | §2.1 voices + §Render-harness Probe 5 |
| PERF-01/02 | alloc-free / lock-free processBlock | §Pitfalls, §Thread boundaries |
| QUAL-02 | viz shows the applied Δt | §2.3 VizAnalyzer |

---

## Summary

The conventional half (six one-shot percussion voices, GM trigger map, mixer) ports cleanly from
O-simpleFM's voice/envelope/`fastSine`/viz-ring patterns — those are read-from-source and reusable
verbatim. The genuinely-new and highest-risk half is **timing infrastructure**: reading
`AudioPlayHead::PositionInfo` (every field `juce::Optional`), enumerating step boundaries in the
block's ppq window, and **sub-slicing the voice render on per-event sample offsets exactly the way
`juce::Synthesiser::processNextBlock` does** (loop verified at `juce_Synthesiser.cpp:180–236`). The
render-harness gate hinges on a tiny synthetic `AudioPlayHead` subclass (override one method:
`getPosition()`) injected via `AudioProcessor::setPlayHead()`, plus a test hook exposing the emitted
`MidiBuffer` so offsets are asserted directly.

**Primary recommendation:** Build the sub-slice loop as a direct transcription of
`Synthesiser::processNextBlock` (own voices, not `juce::Synthesiser`). For Phase 2.3's negative
humanize offsets, **ship Fallback A (late-only humanize) for the v1.0 gate** and treat the symmetric
absolute-timeline carry-over as an optional enhancement — the ARCHITECTURE explicitly blesses this and
it removes the single fiddliest mechanism (the cross-block lookahead) from the critical path.

---

## Architectural Responsibility Map

| Capability | Primary owner | Mechanism |
|------------|---------------|-----------|
| Host transport → firing steps | `SequencerClock` (audio thread) | `getPlayHead()->getPosition()` once/block |
| Per-hit Δt + velocity | `TimingFeelEngine` (audio thread) | pre-seeded `juce::Random`, sampled once per committed hit |
| Voice synthesis | per-voice structs (audio thread) | `fastSine` LUT, `juce::Random` noise, `dsp::StateVariableTPTFilter`, exp envelopes |
| Trigger delivery + sub-slice | `UnifiedTriggerRouter` (audio thread) | merge two `MidiBuffer`s sorted; iterate; render in spans |
| Audio→UI handoff | `VizAnalyzer` (audio writer / Timer reader) | `juce::AbstractFifo` ring + `std::atomic<float>` playhead |
| Knob state | APVTS | `getRawParameterValue()->load()` once/block |
| Pattern state | custom atomic array | already built in Stage 1 (`getStep`) |

---

## Phase 2.1 — DrumVoiceEngine (MIDI-playable, no sequencer)

### Reusable verbatim from O-simpleFM
- **`fastSine(float phase)`** — `Source/Operator.h`. 1024-point `juce::dsp::LookupTableTransform`, function-local-static init, **mandatory floor-modulo wrap** (`phase -= twoPi*floor(phase/twoPi)`) because the LUT *clamps* out-of-range inputs rather than wrapping. Copy this file into Beatmaker `Source/` (or share via a small header) for kick/tom/snare sine bodies. `[VERIFIED: O-simpleFM/Source/Operator.h]`
- **Lock-free viz ring shape** — `Source/FmVizAnalyzer.h` `VizRing` (power-of-two + bitmask). The *event* ring in §2.3 differs (it carries POD structs via `AbstractFifo`, not a float overwrite ring), but the memory-ordering discipline (relaxed stores on the producer, `release`/`acquire` on the position) is the template. `[VERIFIED: O-simpleFM/Source/FmVizAnalyzer.h]`
- **Per-block param push pattern** — `PluginProcessor.cpp:192` `auto get = [this](const char* id){ return parameters.getRawParameterValue(id)->load(); };` then one `setParams(...)` per voice. Mirror this. `[VERIFIED: O-simpleFM/Source/PluginProcessor.cpp]`

### Voice structs — NOT `juce::Synthesiser`
These are one-shot, monophonic-retrigger percussion with no note-off semantics, so a custom struct per
voice is correct (ARCHITECTURE §3). Each exposes: `prepareToPlay(double fs)`, `setParams(tune, decay01,
tone01, level)`, `trigger(uint8 velocity)`, and `render(float* L, float* R, int start, int n)` that
*adds* into the buffer span and advances its tail.

**Exponential envelope (per ARCHITECTURE §Algorithm Details):**
```cpp
// In prepareToPlay / setParams — recompute on rate or decay change:
const double decaySec = mapDecay01ToSeconds (decay01);     // per-voice ms range lives HERE
ampCoef = std::exp (-1.0 / (decaySec * fs));               // per-sample multiplier
// Per sample in render():
ampEnv *= ampCoef;                                          // ampEnv seeded to 1.0f on trigger()
if (ampEnv < 1.0e-6f) ampEnv = 0.0f;                        // flush denormal tail -> voice idle
```
For the **kick/tom pitch envelope**, use a second, faster exp env that drives instantaneous frequency
(`fInst = fBase + pitchEnv * pitchAmt`), `pitchEnv` decaying ~10–30 ms. Phase-accumulate at `fInst`,
feed `fastSine`.

**Filtered-noise voices (snare body+noise, clap, hats)** — `juce::dsp::StateVariableTPTFilter<float>`.
Verified public API (`juce_StateVariableTPTFilter.h`):
```cpp
enum class Type { lowpass, bandpass, highpass };
void setType (Type);
void setCutoffFrequency (SampleType hz);
void setResonance (SampleType q);
void prepare (const juce::dsp::ProcessSpec&);   // call in prepareToPlay
void reset();
SampleType processSample (int channel, SampleType in);   // <-- per-sample, RT-safe, use this
```
Use `processSample(0, noise)` per sample (NOT the block `process(context)` form — sub-slicing makes
per-sample cleaner and avoids constructing `ProcessContext` objects in the hot loop). `prepare` once in
`prepareToPlay` with `{ fs, (uint32) maxBlock, 1 }`. `[VERIFIED: juce_dsp/processors/juce_StateVariableTPTFilter.h]`

Noise source: a pre-seeded `juce::Random` per voice, `nextFloat()*2-1`, or an inline xorshift for speed.
**Seed in `prepareToPlay`, never on the audio thread.**

### Hi-hat choke (closed 42 → open 46)
Open and closed hats share a noise→BP source but have separate envelopes. On a **closed-hat trigger**,
force the open-hat voice into a fast release: set a one-pole release coefficient
`chokeCoef = exp(-1/(0.003*fs))` (≈3 ms) and multiply the open-hat env by it each sample until it
reaches the new (faster) target. Implement as: when closed fires, `openHat.applyChoke()` which swaps the
open voice's active `ampCoef` to `chokeCoef`. `[CITED: ARCHITECTURE.md §3 + §Algorithm Details]`

### Clap multi-burst
3–4 retriggered noise bursts ~10 ms apart → BP → short diffuse tail. Implement with a small
`int burstsRemaining` + `int samplesToNextBurst` countdown inside the voice; on each burst re-seed the
amp env to 1.0. RT-safe (no allocation; counters only).

### Velocity → loudness + timbre
Single path for host MIDI and sequencer MIDI (they are indistinguishable). `vel01 = velocity/127`.
Gain = primary (`level_dB` + `vel01` curve); timbre scalar adds pitch-env on kick/tom, more HF/noise on
snare/hat, slightly faster attack. Compose with the per-voice `tone` knob (they multiply, not replace).

### `UnifiedTriggerRouter` — MIDI half (this phase)
GM map already in `PluginProcessor.h`: `kGmNotes {36,38,39,42,46,45}`. Build a reverse lookup
`noteToVoice[128]` (init −1, fill the six). On `isNoteOn()`: `int v = noteToVoice[msg.getNoteNumber()];
if (v < 0) return; // out-of-map ignored`. Apply mute/solo at trigger time: if any solo active and this
voice not soloed → skip; if muted → skip. `[VERIFIED: PluginProcessor.h kGmNotes]`

### Mixer / Master
Sum 6 voices → stereo; `juce::SmoothedValue<float>` on output gain (`reset(fs, 0.02)` in prepare,
`setTargetValue(juce::Decibels::decibelsToGain(outDb, -60.0f))` per block; `-60 dB ⇒ silence`).

---

## Phase 2.2 — SequencerClock (host-synced grid, swing/humanize OFF)

### `AudioPlayHead::PositionInfo` — verified JUCE 8.0.9 (`juce_AudioPlayHead.h`)
`getPlayHead()` returns `AudioPlayHead*` (may be **null** — standalone / no host). The accessor is:
```cpp
virtual Optional<PositionInfo> getPosition() const = 0;   // CALL ONLY FROM processBlock()
```
`Optional<PositionInfo>` is truthy via `explicit operator bool()` / `.hasValue()` (`juce_Optional.h:143`).
**Every** `PositionInfo` field is itself `juce::Optional<...>` and engaged only if the host supplies it:

| Getter | Returns | Notes |
|--------|---------|-------|
| `getBpm()` | `Optional<double>` | guard div-by-zero; fall back to `tempo` param |
| `getPpqPosition()` | `Optional<double>` | quarter-notes; bar-relative math source |
| `getPpqPositionOfLastBarStart()` | `Optional<double>` | bar align; if unset use `fmod(ppq, patternLen*0.25)` |
| `getTimeInSamples()` | `Optional<int64_t>` | discontinuity detection |
| `getTimeSignature()` | `Optional<TimeSignature{numerator,denominator}>` | default 4/4 |
| `getIsPlaying()` | `bool` (NOT optional) | gate host-synced vs free-run |
| `getIsLooping()` / `getLoopPoints()` | `bool` / `Optional<LoopPoints{ppqStart,ppqEnd}>` | resync hint |

`[VERIFIED: juce_audio_basics/audio_play_head/juce_AudioPlayHead.h:333–445,590]`

Canonical read (RT-safe, no allocation):
```cpp
double bpm = tempoParam;            // free-run default
double ppqStart = 0.0; bool synced = false;
if (auto* ph = getPlayHead())
    if (auto pos = ph->getPosition())            // Optional<PositionInfo>
    {
        if (pos->getIsPlaying())
        {
            if (auto b = pos->getBpm())          bpm = *b;
            if (auto p = pos->getPpqPosition())  { ppqStart = *p; synced = true; }
        }
    }
```

### Step enumeration
```cpp
const double samplesPerPpq  = (60.0 / bpm) * fs;
const double samplesPer16th = samplesPerPpq * 0.25;          // 1 step = 0.25 ppq
const double blockPpq       = numSamples / samplesPerPpq;
const double barLenPpq      = patternLength * 0.25;          // 8/16/32 steps
double barStart = lastBarStartPpqOpt ? *lastBarStartPpqOpt
                                     : ppqStart - std::fmod (ppqStart, barLenPpq);
for (int k = 0; k < patternLength; ++k)
{
    const double stepPpq = barStart + k * 0.25;              // absolute ppq of step k
    // also test stepPpq +/- barLenPpq for the neighbouring bar (wrap at window edges)
    if (stepPpq >= ppqStart && stepPpq < ppqStart + blockPpq)
    {
        const int nominalOffset = (int) std::llround ((stepPpq - ppqStart) * samplesPerPpq);
        const int vel = grid[cellIndex(voice,k)].load(std::memory_order_relaxed);  // lock-free
        if (vel > 0) emit(voice, k, nominalOffset, vel);     // straight time: appliedOffset == nominal
    }
}
```

### Commit-once / discontinuity safety
ppq is **not monotonic** (loop, relocate, tempo automation). Do **not** extrapolate across blocks.
Track `lastEmittedAbsStepPpq` per voice (or a `lastBlockEndPpq`); if `ppqStart < lastBlockEndPpq -
epsilon` or jumps forward by > one block, treat as a discontinuity and **resync** (clear pending,
re-derive from the new ppq) rather than back-filling. This prevents double-fires at block edges.
`[CITED: ARCHITECTURE.md §Algorithm Details — SequencerClock]`

### Standalone / stopped free-run fallback
When `getPlayHead()==nullptr`, `getPosition()` empty, or `getIsPlaying()==false`: integrate an internal
`double phaseInSteps`:
```cpp
phaseInSteps += numSamples / samplesPer16th;          // samplesPer16th from tempoParam
while (phaseInSteps >= patternLength) phaseInSteps -= patternLength;
// a step k fires this block when the integer step boundary is crossed within [0,numSamples)
```
Compute each crossing's in-block offset the same way and emit. `[CITED: ARCHITECTURE.md §SequencerClock]`

### `playheadStepPhase` atomic
Update once/block: `playheadStepPhase.store((float) (fractional step index 0..patternLength),
std::memory_order_relaxed);` Editor Timer reads it for the smooth sweep (decoupled from the event ring).

---

## Sub-slice render — mirror `Synthesiser::processNextBlock` (THE pattern)

Verified at `juce_Synthesiser.cpp:180–236`. We do **not** use `juce::Synthesiser` (one-shot voices), but
we transcribe its loop exactly. The key primitive is `MidiBuffer::findNextSamplePosition(int)` →
`MidiBufferIterator`; each `*it` is a `MidiMessageMetadata { data, numBytes, samplePosition }`.

```cpp
// merged = sequencer buffer + host buffer, kept sorted (see merge below)
auto it = merged.findNextSamplePosition (0);
int startSample = 0, numSamples = buffer.getNumSamples();
for (; numSamples > 0; ++it)
{
    if (it == merged.cend()) { renderAllVoices (buffer, startSample, numSamples); break; }

    const auto meta = *it;
    const int toNext = meta.samplePosition - startSample;

    if (toNext >= numSamples) { renderAllVoices (buffer, startSample, numSamples);
                                handleTrigger (meta.getMessage()); break; }
    if (toNext > 0) { renderAllVoices (buffer, startSample, toNext);
                      startSample += toNext; numSamples -= toNext; }
    handleTrigger (meta.getMessage());   // GM->voice, choke, velocity (zero-length span ok)
}
// drain any remaining events at/after the final span (Synthesiser does std::for_each tail)
```
`renderAllVoices(buf, start, n)` calls each active voice's `render(L,R,start,n)` (adds into the span and
advances tails). `handleTrigger` routes note-ons; note-offs are ignored except the closed→open choke.
`[VERIFIED: juce_audio_basics/synthesisers/juce_Synthesiser.cpp:180–236]`

> JUCE adds a `minimumSubBlockSize` coalescing branch (`setMinimumRenderingSubdivisionSize`) — for us a
> sub-block of 1 sample is fine (percussion, light CPU), so **skip the coalescing branch**; sample-exact
> offsets are the whole point of the gate.

### Merging the two buffers (sorted)
`MidiBuffer::addEvent(msg, sampleNumber)` keeps the buffer **sorted by sample position; ties keep
insertion order** (verified `juce_MidiBuffer.h:196–203`). Two clean options:
```cpp
// Member buffers, cleared each block (capacity preallocated in prepareToPlay via ensureSize):
sequencerMidi.clear();
// ... TimingFeelEngine emits into sequencerMidi via addEvent(noteOn, finalOffset) ...
sequencerMidi.addEvents (hostMidi, 0, numSamples, 0);   // merge host in, stays sorted
// iterate sequencerMidi as `merged`
```
`addEvents(other, startSample, numSamples, sampleDeltaToAdd)` verified at `juce_MidiBuffer.h:241`.
Preallocate with `MidiBuffer::ensureSize(bytes)` in `prepareToPlay` to keep `processBlock` alloc-free.
`[VERIFIED: juce_audio_basics/midi/juce_MidiBuffer.h]`

---

## Phase 2.3 — TimingFeelEngine + VizAnalyzer (the lesson)

### Δt composition (exact contract — do not fold terms)
```cpp
const double swing01 = swingParam;                 // stored 0..1 (display 0..75%)
const double q       = quantizeParam;              // 0..1
const double T8      = (60.0 / bpm) / 2.0;         // one 8th in seconds
// Swing: odd step indices only, deterministic, NOT scaled by q:
double dSwing = (k % 2 == 1) ? (swing01 / 3.0) * T8 : 0.0;
// Humanize: sampled ONCE per committed hit from the per-voice pre-seeded RNG:
double r = triangular (voiceRng[voice]);           // in [-1,1], sum-of-two-uniforms
double dHumanT = r * humanizeParam * 0.030;        // +/-30 ms
double rv = triangular (voiceRng[voice]);
double dHumanV = rv * humanizeParam * 24.0;        // +/-24 velocity
// Compose — q scales ONLY the humanize terms:
double dtSec = dSwing + dHumanT * (1.0 - q);
int appliedOffsetSamples = (int) std::llround (dtSec * fs);
int finalVel = juce::jlimit (1, 127, (int) std::lround (stepVel + dHumanV * (1.0 - q)));
int finalOffset = nominalOffset + appliedOffsetSamples;
sequencerMidi.addEvent (juce::MidiMessage::noteOn (1, kGmNotes[voice], (juce::uint8) finalVel), finalOffset);
```
**Invariant check (DSP-04):** at `q=1`, `dHumanT*(1−q)=0` and `dHumanV*(1−q)=0` → humanize vanishes, but
`dSwing` survives. This is the entire quantize-vs-feel lesson in one line. `[CITED: ARCHITECTURE.md §2]`

`triangular(rng)` = `(rng.nextFloat()*2-1 + rng.nextFloat()*2-1) * 0.5` — centre-weighted, range [−1,1].
`juce::Random` API: `setSeed(int64)` in `prepareToPlay` (one seed per voice), `nextFloat()`/`nextInt()`
on the audio thread (no reseed). `[CITED: ARCHITECTURE.md §JUCE refs]`

### VizAnalyzer — `juce::AbstractFifo` POD ring (QUAL-02 by construction)
```cpp
struct VizEvent {
    juce::uint8 voiceIndex; juce::int16 stepIndex;
    juce::int32 nominalSampleInBar; juce::int32 appliedSampleInBar;
    juce::uint8 velocity; juce::uint8 source;   // 0=sequencer, 1=host MIDI
};
// Producer (audio): juce::AbstractFifo fifo {capacity}; std::array<VizEvent,capacity> store;
const auto scope = fifo.write (1);
if (scope.blockSize1 > 0) store[scope.startIndex1] = ev;        // single contiguous slot
// (scope finishes the commit in its destructor — RAII, no manual finishedWrite)
```
**Both** sample fields are **bar-relative** and come from the *same* scheduling pass:
`nominalSampleInBar = round((stepPpqInBar) * samplesPerPpq)`, `appliedSampleInBar = nominalSampleInBar +
appliedOffsetSamples` (wrap into `[0, samplesPerBar)` if needed). UI computes `Δt = applied − nominal` —
**the displayed offset IS the applied offset** because it is read off the same numbers that produced
`finalOffset`, never recomputed from the swing/humanize formula. `[CITED: ARCHITECTURE.md §6, QUAL-02]`

Push the `VizEvent` in the **same** code path that calls `addEvent` (one emit = one push), so audio and
viz cannot diverge. Playhead is the separate `std::atomic<float> playheadStepPhase` (continuous, one
store/block). `[VERIFIED: AbstractFifo present — juce_core/containers]`

---

## Render-harness (Stage-2 gate)

Port O-simpleFM `tests/render-harness/{main.cpp,CMakeLists.txt}` (both read; they are the template). The
FM harness already shows: direct processor instantiation, `setPlayConfigDetails(0,2,fs,block)`,
`prepareToPlay`, block-loop rendering, `setParam` via `convertTo0to1`, single-bin DFT / RMS / peak
helpers, `ScopedJuceInitialiser_GUI`. Beatmaker reuses all of that. `[VERIFIED: O-simpleFM/tests/render-harness/]`

### Two Beatmaker-specific additions

**1. Synthetic `juce::AudioPlayHead` subclass.** `getPosition()` is the only pure-virtual; override it,
return a known transport so probes assert *exact* sample offsets:
```cpp
struct FakePlayHead : juce::AudioPlayHead
{
    double bpm = 120.0, ppq = 0.0;  bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo pos;
        pos.setBpm (bpm);
        pos.setPpqPosition (ppq);
        pos.setPpqPositionOfLastBarStart (std::floor (ppq / 1.0) * 1.0); // 4/4: bar = 1.0 ppq*4 -> adjust
        pos.setTimeInSamples ((juce::int64) std::llround (ppq * (60.0/bpm) * 44100.0));
        pos.setIsPlaying (playing);
        pos.setTimeSignature (PositionInfo::TimeSignature{}); // 4/4 default
        return pos;
    }
};
// inject: proc.setPlayHead (&fake); advance fake.ppq by blockPpq between blocks.
```
`setPlayHead(AudioPlayHead*)` is public on `AudioProcessor` (`juce_AudioProcessor.h:1204`). The setters
(`setBpm`, `setPpqPosition`, …) engage the matching `Optional` flag. `[VERIFIED: juce_AudioPlayHead.h, juce_AudioProcessor.h:1204]`

> The harness drives the processor block-by-block, **incrementing `fake.ppq += numSamples/samplesPerPpq`
> after each block**, so a multi-block bar is rendered and Probe 4 (block-boundary) is meaningful.

**2. Emitted-MIDI test hook.** Expose the sequencer's emitted `MidiBuffer` (or a captured `std::vector<VizEvent>`)
so offsets are asserted **directly**, not only inferred from audio transients. Cheapest:
`#if OUARICON_BUILD_TESTS` guarded accessor on the processor returning a const ref to the last block's
`sequencerMidi` (or a copy of the pushed `VizEvent`s). Probes 1/2/4/6 read this; Probe 5 still checks
audio (voice correctness + aliasing). Audio transient cross-check (peak-detect per voice band) validates
that the hook agrees with what was rendered.

### CMakeLists changes vs FM template
- `juce_add_console_app(O-simpleBeatmaker-render-test ...)`; `target_sources` reaches into
  `../../Source/PluginProcessor.cpp` + `PluginEditor.cpp` **plus the new DSP source files** (voice
  headers are header-only if you keep them `.h`; any `.cpp` must be listed).
- `add_dependencies(... O-simpleBeatmaker)` + borrow `$<TARGET_PROPERTY:O-simpleBeatmaker,INCLUDE_DIRECTORIES>`.
- Replicate the `JucePlugin_*` compile-defs block (Name/Code/IsSynth=1/WantsMidiInput=1, **PluginCode
  `0x4f536942` = "OSiB"**, ManufacturerCode `0x4f756172` = "Ouar").
- **Stage 2 has no `juce_add_binary_data` target yet** (per CMake comment — WebView resources arrive in
  Stage 3). So, unlike the FM harness, **do NOT link a `*_UIResources` target or set `JUCE_WEB_BROWSER=1`**
  unless `PluginEditor.cpp` references WebView symbols. Beatmaker's Stage-1 editor is a
  `GenericAudioProcessorEditor` shell → link only the juce modules. Revisit when Stage 3 adds the editor.
- Un-comment the wiring already stubbed in `CMakeLists.txt`:
  ```cmake
  option(OUARICON_BUILD_TESTS "Build O-simpleBeatmaker render-test harness" OFF)
  if(OUARICON_BUILD_TESTS)
      add_subdirectory(tests/render-harness)
  endif()
  ```
`[VERIFIED: O-simpleBeatmaker/CMakeLists.txt:80–85, O-simpleFM/tests/render-harness/CMakeLists.txt]`

### Probe assertions (from ROADMAP, with mechanism)
1. **Grid accuracy** — straight time: emitted offset of step k == `round(k * samplesPer16th)` within bar, ±0.
2. **Swing** — swing=75% (`swing01=1`): odd-k offset == nominal + `round((1/3)*T8*fs)`; even-k unmoved.
3. **Humanize+quantize** — humanize=1,q=0: offset spread >0, `|Δ|<=round(0.030*fs)`, vel spread `<=24`. q=1: spread→0 **but swing offsets remain**.
4. **Block-boundary** — choose block size so a swung step's `finalOffset` would exceed `numSamples`; assert it appears in the *next* block at the correct absolute sample (see Mechanism Gaps).
5. **MIDI-playable + voices** — inject host noteOn per GM note → correct voice non-trivial RMS; closed-hat (42) then open-hat (46) → open tail truncates; velocity scales RMS; high-rate hits → bounded peak/finite + aliasing budget (reuse FM's inter-harmonic DFT probe).
6. **Viz truth** — `appliedSampleInBar − nominalSampleInBar` == the `appliedOffsetSamples` baked into the emitted `MidiMessage`.

---

## Mechanism Gaps — places ARCHITECTURE specifies intent but not the concrete mechanism

1. **Negative humanize offset / cross-block lookahead (HIGH).** ARCHITECTURE says "+1-block lookahead so
   negative offsets remain representable" but does not give the data structure. A `MidiBuffer` sample
   offset **cannot be negative**, so a hit whose `finalOffset < 0` (early humanize) or whose nominal is
   in the *next* block but pulled early into *this* one needs a carry mechanism. **Recommended concrete
   mechanism:** a small fixed-capacity `std::array<PendingHit, N>` "carry-over" queue on the audio
   thread. When a step's `finalOffset >= numSamples` (late, runs past block end) **or** a look-ahead step
   resolves to `finalOffset < 0` relative to next block, stash it with an absolute target sample; each
   block, drain entries whose `absTarget` falls in `[blockStartAbs, blockStartAbs+numSamples)` and emit
   at the local offset. This is alloc-free (fixed array) and makes Probe 4 pass for both signs.
   **v1.0 GATE recommendation: ship Fallback A (late-only humanize, `dHumanT ∈ [0,+MAX]`)** — drop the
   negative case entirely so no look-ahead/carry is needed; symmetric humanize becomes a post-gate
   enhancement. ARCHITECTURE explicitly blesses this (Risk §Fallback A) and it is musically defensible
   ("laid-back" feel). The carry-over queue is then only needed for **late** offsets that cross the
   block end — a strictly simpler one-sided case.

2. **Bar-relative wrap for VizEvent int32 fields (MEDIUM).** `appliedSampleInBar` can exceed
   `samplesPerBar` when a late hit on the last step spills past the bar. Define the convention now:
   store the *raw* `nominalSampleInBar` and `appliedSampleInBar = nominal + appliedOffsetSamples`
   **without** wrapping (UI subtracts → correct Δt regardless), and let the UI handle bar-position
   display modulo `samplesPerBar`. Pick one and document it so Probe 6 and Stage-3 agree.

3. **Free-run ↔ host-synced phase continuity (LOW).** When the host starts/stops mid-pattern, decide
   whether free-run `phaseInSteps` resets to the host bar phase on resync or continues. Recommend:
   on transition to host-synced, **snap** `phaseInSteps` to the host-derived step phase (no audible
   "catch-up" drift). Document in the SequencerClock.

4. **`samplesPerStep` recompute on tempo automation (LOW).** Tempo can change mid-block in some hosts;
   ARCHITECTURE reads bpm once/block, which is fine (sub-block tempo ramps are out of scope for v1.0).
   State this explicitly as an accepted simplification.

---

## Don't Hand-Roll

| Problem | Don't build | Use | Why |
|---------|-------------|-----|-----|
| Sub-slice render on MIDI offsets | a per-sample trigger-flag scan from scratch | transcribe `Synthesiser::processNextBlock` + `findNextSamplePosition` | proven, sorted-iterator semantics handled, exactly what hosts expect |
| Sorted event merge | manual insertion sort | `MidiBuffer::addEvent` / `addEvents` | kept sorted, ties stable, alloc-free with `ensureSize` |
| Band-clean sine | raw `std::sin` per sample | O-simpleFM `fastSine` LUT | already band-clean + wrap-safe; reuse verbatim |
| Band/high-pass shaping | biquad coeff math by hand | `dsp::StateVariableTPTFilter::processSample` | TPT-stable, RT-safe per-sample form |
| Audio→UI handoff | a mutex-guarded queue | `juce::AbstractFifo` + atomics | lock-free SPSC, PERF-01 |
| Transport math | parsing host time yourself | `AudioPlayHead::PositionInfo` Optionals | the only correct cross-host source |

---

## Common Pitfalls (verified against project memory + sibling lessons)

1. **Class-name shadowing of `juce::` types.** O-simpleSampler had to rename `SamplerVoice`/`SamplerSound`
   → `SampleVoice`/`SampleSound` because `JuceHeader.h`'s `using namespace juce` makes them ambiguous.
   **Do NOT name a voice `Voice` (clashes with the local `enum Voice` in `PluginProcessor.h`!), `Synthesiser`,
   `Random`, or anything in `juce::`.** Use `KickVoice`, `SnareVoice`, `DrumVoiceEngine`, `SequencerClock`,
   `TimingFeelEngine` — all clear. Note the **existing `enum Voice { Kick, ... }`** in namespace
   `OSimpleBeatmaker` — a class `Voice` would collide; keep struct names suffixed. `[VERIFIED: MEMORY.md, PluginProcessor.h:32]`
2. **ParamID shadowing free functions.** O-simpleSampler's `end` ParamID was ambiguous with `juce::end`
   under `using namespace ParamIDs`. Beatmaker's IDs (`swing`, `humanize`, …) are safe, but if you add a
   local symbol, avoid `begin`/`end`/`size`. `[VERIFIED: MEMORY.md]`
3. **`getLatencySamples()` is non-virtual in JUCE 8 — never override.** Call `setLatencySamples(0)` in
   `prepareToPlay`. `[VERIFIED: MEMORY.md + ARCHITECTURE.md §Latency]`
4. **Denormals on tails.** `juce::ScopedNoDenormals` at top of `processBlock` (FM does this) **and** flush
   each voice env to 0 below ~1e-6 — resonant hat BP filters are the worst offenders. `[VERIFIED: O-simpleFM/PluginProcessor.cpp:223]`
5. **RT-safety.** No allocation/lock in `processBlock`: preseed RNG in `prepareToPlay`; `ensureSize` the
   sequencer `MidiBuffer`; the carry-over queue is a fixed `std::array`; no `juce::String`/heap in the hot
   path. APVTS read once/block via cached `getRawParameterValue()->load()`. `[VERIFIED: O-simpleFM pattern + ARCHITECTURE §Thread Boundaries]`
6. **LUT clamp, not wrap.** `fastSine` already wraps phase; if you write a second oscillator, replicate the
   floor-modulo wrap or high-pitch tones flat-line. `[VERIFIED: O-simpleFM/Operator.h]`
7. **`getPlayHead()` may be null AND `getPosition()` may be empty AND each field optional** — three
   independent guards. Calling `getPosition()` outside `processBlock` is UB. `[VERIFIED: juce_AudioPlayHead.h:580]`
8. **Two `juce_add_binary_data` targets must use distinct `NAMESPACE`** (O-simpleGrain collision) — N/A
   this stage (no binary data until Stage 3) but flagged for the harness CMake (don't link UIResources). `[VERIFIED: MEMORY.md]`

---

## Standard Stack (all in-tree, no new packages)

| Component | JUCE class | Verified location |
|-----------|------------|-------------------|
| Transport | `juce::AudioPlayHead::PositionInfo` | `juce_audio_basics/audio_play_head/juce_AudioPlayHead.h` |
| MIDI scheduling | `juce::MidiBuffer` / `juce::MidiMessage` / `MidiBufferIterator` | `juce_audio_basics/midi/juce_MidiBuffer.h` |
| Sub-slice reference | `juce::Synthesiser::processNextBlock` | `juce_audio_basics/synthesisers/juce_Synthesiser.cpp:180` |
| Sine | `juce::dsp::LookupTableTransform` (via `fastSine`) | O-simpleFM/Operator.h |
| Filtering | `juce::dsp::StateVariableTPTFilter<float>` | `juce_dsp/processors/juce_StateVariableTPTFilter.h` |
| RNG | `juce::Random` | `juce_core/maths/` |
| Audio→UI | `juce::AbstractFifo` + `std::atomic` | `juce_core/containers/` |
| Smoothing | `juce::SmoothedValue<float>` | `juce_audio_basics/` |
| State | `juce::AudioProcessorValueTreeState` + grid atomics | already in Stage 1 shell |

**Package Legitimacy Audit:** N/A — no external packages installed; all dependencies are the
already-vendored local JUCE 8.0.9 tree.

---

## Environment Availability
N/A beyond the existing build chain (CMake + Ninja + local JUCE 8.0.9 at `/Users/taylorbrook/JUCE`),
all confirmed present (Stage 1 built VST3+AU+Standalone, pluginval strictness 8 SUCCESS).

---

## Validation Architecture

| Property | Value |
|----------|-------|
| Framework | Custom offline console render-harness (no DAW), gated by `-DOUARICON_BUILD_TESTS=ON` |
| Location | `plugins/O-simpleBeatmaker/tests/render-harness/{main.cpp,CMakeLists.txt}` (NEW) |
| Build | `cmake -B build -DOUARICON_BUILD_TESTS=ON && cmake --build build --target O-simpleBeatmaker-render-test` |
| Run | execute the built console app; exit 0 iff all 6 probes pass |

**Phase → probe map:** 2.1 → Probe 5 · 2.2 → Probes 1,4 · 2.3 → Probes 2,3,6. Each sub-phase is a
checkpoint commit; the harness is the automated gate between them. DAW smoke-test (Logic AU, Ableton/
Reaper VST3, Standalone) after 2.2 and 2.3 for transport-sync robustness (per CLAUDE.md cache-clear +
install sequence).

---

## Assumptions Log

| # | Claim | Section | Risk if wrong |
|---|-------|---------|---------------|
| A1 | Triangular humanize distribution is acceptable for the gate (ARCHITECTURE *recommends*, not mandates) | §2.3 | LOW — flat uniform still passes bounds; only feel differs |
| A2 | Late-only humanize (Fallback A) is acceptable for v1.0 gate | §Mechanism Gaps #1 | LOW — explicitly blessed by ARCHITECTURE; symmetric is a post-gate enhancement |
| A3 | `samplesPerStep` recomputed once/block (no sub-block tempo ramp) is acceptable | §Mechanism Gaps #4 | LOW — matches read-once-per-block contract |
| A4 | Harness needs no WebView/UIResources link at Stage 2 (shell editor only) | §Render-harness | LOW — verify when un-commenting; if `PluginEditor.cpp` pulls WebView symbols, add `JUCE_WEB_BROWSER=1` |

*(No assumed package names — nothing installed.)*

---

## Sources

### Primary (HIGH — read from local source this session)
- `/Users/taylorbrook/JUCE/modules/juce_audio_basics/audio_play_head/juce_AudioPlayHead.h` — PositionInfo getters/setters, `getPosition()`, TimeSignature/LoopPoints.
- `/Users/taylorbrook/JUCE/modules/juce_audio_basics/synthesisers/juce_Synthesiser.cpp:180–236` — sub-slice loop (the pattern to mirror).
- `/Users/taylorbrook/JUCE/modules/juce_audio_basics/midi/juce_MidiBuffer.h` — `addEvent`/`addEvents`/`findNextSamplePosition`, sorted-insert + tie semantics.
- `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_StateVariableTPTFilter.h` — filter public API.
- `/Users/taylorbrook/JUCE/modules/juce_audio_processors/processors/juce_AudioProcessor.h:1204` — `setPlayHead`.
- `/Users/taylorbrook/JUCE/modules/juce_core/containers/juce_Optional.h:143` — `operator bool`/`hasValue`.
- O-simpleFM `Source/{Operator.h,FMVoice.h,FmVizAnalyzer.h,PluginProcessor.cpp}` + `tests/render-harness/{main.cpp,CMakeLists.txt}` — voice/envelope/viz/harness template.
- O-simpleBeatmaker `Source/PluginProcessor.h`, `CMakeLists.txt` — existing shell + test-wiring stub.

### Secondary (project memory / sibling lessons)
- `MEMORY.md` — class-name & ParamID shadowing, `getLatencySamples` non-virtual, BinaryData namespace collision, WebView flags.
- `research/ARCHITECTURE.md`, `parameter-spec.md`, `ROADMAP.md` — immutable DSP spec + probe definitions.

---

## Metadata
- **Standard stack:** HIGH — every API read from local 8.0.9 source.
- **Sub-slice mechanism:** HIGH — transcribed from `Synthesiser::processNextBlock`.
- **Transport sync:** HIGH on API, MEDIUM on cross-host robustness (no DAW test yet this session — covered by Stage-2 DAW smoke test).
- **Negative-humanize carry-over:** MEDIUM — recommend Fallback A for the gate (see Mechanism Gaps #1).
- **Research date:** 2026-06-25 · **Valid until:** ~2026-07-25 (stable JUCE pin; no fast-moving deps).
</content>
</invoke>
