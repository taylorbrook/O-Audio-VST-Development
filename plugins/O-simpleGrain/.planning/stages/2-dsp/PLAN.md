# O-simpleGrain — Stage 2 (DSP) Execution Plan

**Plugin:** O-simpleGrain · **Stage:** 2 of 4 (DSP) · **Phase:** plan · **Date:** 2026-06-24
**Run mode:** express (auto-progress 2.1→2.2→2.3 via dsp-agent; stop with handoff at the Stage 2→3 boundary — CONTEXT D2)
**Builds on:** Stage 1 silent shell (`Source/PluginProcessor.{h,cpp}` — 18-param APVTS, cached atomic pointers, engine constants, `currentSourceIdentity`, `setLatencySamples(0)`, silent `processBlock`).

> **LOCKED CONTRACTS — do not redesign.** Every DSP decision is pinned in
> `research/ARCHITECTURE.md` (immutable), `ROADMAP.md` (3-phase breakdown), `stages/2-dsp/CONTEXT.md`
> (the two open decisions: procedural samples already generated; express mode), and
> `stages/2-dsp/RESEARCH.md` (extracted reference code §1–§10). This plan sequences the
> RESEARCH.md §10 checklist into ordered, executable tasks. The dsp-agent implements; it does
> not re-decide.

---

## Stage Goal

Fill the currently-silent `processBlock` with the full granular audio engine specified in
`ARCHITECTURE.md`: a polyphonic (8-voice) `juce::Synthesiser` of `GrainVoice`s, each owning a
preallocated bounded grain pool scheduled at the density-derived period, reading windowed grains
from a global-playhead read head over an embedded/loaded source buffer, overlap-added, anti-aliased
on up-transposition, shaped by a per-voice amp ADSR, with a processor-owned scan/freeze read head,
per-grain spray/scatter, source sample loading (embed 4 + drag-drop + atomic hot-swap), and the
three lock-free audio→UI taps the Stage 3 GUI will consume. Real-time-safe throughout (no
alloc/lock in `processBlock`, zero added latency).

**Phased delivery (matches ROADMAP "Stage 2: DSP Phases" — each independently buildable + committable):**

| Sub-phase | Headline | First audible result |
|-----------|----------|----------------------|
| **2.1** | Core grain engine + overlap-add + window LUTs + amp ADSR + key resample | The plugin makes its first granular sound; MIDI-playable |
| **2.2** | Read head (scan/stretch/freeze) + spray/scatter + anti-aliasing + velToDensity | The granular "moves" and stays clean |
| **2.3** | Sample loading (embed 4 + drag-drop + hot-swap) + viz taps | All sources selectable/loadable; UI data taps live |

---

## Build / Validation Strategy (after EACH sub-phase)

The orchestrator builds + validates after **every** sub-phase commit (2.1, 2.2, 2.3 are each a
green build + a git commit). For each:

1. **Build all three formats:**
   ```bash
   cmake --build build --target O-simpleGrain_VST3 O-simpleGrain_AU O-simpleGrain_Standalone --parallel
   ```
   (or `ninja O-simpleGrain_VST3 O-simpleGrain_AU O-simpleGrain_Standalone` from `build/`)
2. **CRITICAL macOS cache-clear + dual-variant sweep BEFORE install** (project CLAUDE.md — dev branding
   produces `O-simpleGrain-dev.{vst3,component}`; a stale unsuffixed `O-simpleGrain.{vst3,component}`
   on disk pins Logic's AU registry slot). Prefer the script, which does the Phase-4 dual sweep
   automatically:
   ```bash
   ./scripts/build-and-install.sh O-simpleGrain
   ```
   If installing manually: `killall -9 AudioComponentRegistrar`; `rm -rf ~/Library/Caches/AudioUnitCache/`
   and `com.apple.audiounits.cache`; remove BOTH `O-simpleGrain.{vst3,component}` AND
   `O-simpleGrain-dev.{vst3,component}` from the VST3/Components folders; then install the `-dev` variant.
3. **auval** (AU registration + static checks):
   ```bash
   auval -a | grep -i simplegrain        # appears
   auval -v aumu OsGr Ouar               # full validation (substitute real type/subtype/manufacturer)
   ```
4. **Allocation-free / RT-safety check** (see Real-Time-Safety Gates): pluginval at strictness ≥ 8 on
   the VST3, plus a Standalone smoke run holding a chord — no xrun, no allocation warning.

### Which test criteria are automatable vs. manual-listen

| Criterion class | How verified | Auto / Manual |
|-----------------|--------------|---------------|
| Builds VST3+AU+Standalone, no warnings | build command exit 0 | **AUTO** |
| AU registers / passes static auval | `auval -v` | **AUTO** |
| `processBlock` allocation-free; no xrun under high density×size×8 voices | pluginval ≥8 + Standalone chord-hold profile | **AUTO** |
| `setLatencySamples(0)` reported | pluginval latency check / `auval` | **AUTO** |
| Plugin loads as an **instrument**, MIDI routes, polyphonic, no crash | load in DAW + play | **MANUAL-LISTEN** (load is auto via pluginval; *playability* is manual) |
| Separated grains → fused continuous cloud as density rises (FUNC-01/DSP-02) | listen | **MANUAL-LISTEN** |
| Few-ms = pitched buzz; tens-of-ms = fragments (DSP-01) | listen | **MANUAL-LISTEN** |
| Rectangular window audibly clicks; Hann/Gauss do not (DSP-03) | listen | **MANUAL-LISTEN** |
| Held MIDI transposes the cloud; chords work (FUNC-02) | play | **MANUAL-LISTEN** |
| Scan moves read head fwd/slow/reverse/×2; freeze pins click-free (DSP-06/FUNC-03/QUAL-01) | listen | **MANUAL-LISTEN** |
| Spray shimmers; scatter sync→async (DSP-04/05) | listen + (Stage-3 spectrum later) | **MANUAL-LISTEN** |
| High pitch-spray grains stay clean — no buzz (DSP-08) | listen | **MANUAL-LISTEN** |
| All built-ins granulate; drag-drop + picker load a `.wav`; oversized truncated (FUNC-04/05) | drop a file + select sources | **MANUAL-LISTEN** (decode path is auto-testable; audible result manual) |
| Source swap glitch-free | switch source while holding a note | **MANUAL-LISTEN** |
| Viz taps fill without locks / FFT off audio thread | code review + pluginval | **AUTO** |

> The headline audible checks (separated→fused, buzz↔fragments, rect clicks, MIDI transpose, freeze,
> sync↔async, AA cleanliness) are **manual-DAW** and belong to the verify phase. The build / auval /
> allocation-free / zero-latency / no-lock checks are **automatable** and gate each sub-phase commit.

---

## Real-Time-Safety Gates (EVERY task must hold these)

These are invariants, not steps. Any task that violates one is wrong:

1. **No allocation, no lock, no I/O in `processBlock` / `renderNextBlock`.** No `new`/`malloc`/`std::vector`
   growth/`std::string`/file ops/mutex on the render path. (Pitfall §9.1.)
2. **Preallocated pools only.** `std::array<Grain, kMaxGrainsPerVoice/*24*/>` per voice, allocated at
   construction; steal-oldest when full (never resize). Window LUTs `std::array<float, kWindowLutSize/*2048*/>`
   built once at construction.
3. **`juce::ScopedNoDenormals` at the top of `processBlock`** (already present in the Stage 1 shell —
   keep it). Block-level `std::isfinite` scrub after summing voices.
4. **Per-voice `juce::Random`** as a member — never a shared/global RNG, never seeded on the audio thread
   in a way that allocates. `nextFloat()` only.
5. **`setLatencySamples(0)`** stays (already in `prepareToPlay`). NEVER override `getLatencySamples()`
   (non-virtual in JUCE 8 — pitfall §9.4).
6. **`SmoothedValue<float>`** on `scan`, `position`, `playheadVelocity`, and `outputLevel` (zipper-free —
   QUAL-01). Never hard-jump the playhead; never reset a grain's phase mid-grain. The ONLY intended click
   is the rectangular window per-grain artifact (DSP-03 — a feature).
7. **Source buffer published via atomic `shared_ptr` swap** (RESEARCH §6.3). The audio thread snapshots
   `atomicLoad(currentSource)` once per block and holds the ref for the whole block; it never touches a
   half-built buffer, never resizes it.
8. **ADSR `setSampleRate(sr)` BEFORE `setParameters(...)`** in the non-virtual `prepareToPlay` (pitfall §9.3).

---

## Known Risks (from ARCHITECTURE Implementation Risks + the RESEARCH §1 surprise)

| # | Risk | Mitigation (locked) |
|---|------|---------------------|
| **R1 — read addressing does NOT transfer from O-GrainScatter** | O-GrainScatter is a granular *effect* reading a live `DelayBuffer` with a self-cancelling delay tap (`positionOffset + elapsed − readPosition`). Copying its `GrainPool::processSample` read math would be **wrong**. | Implement `readSourceLagrange(src, len, readPos)` against the **static source buffer**; follow ARCHITECTURE §Core Components forward-phase loop (`readPos += rate; phase += phaseInc`), NOT GrainScatter's `samplesRemaining` countdown read. Only the spawn/steal-oldest logic, equal-power pan, envelope-by-phase, and overlap-add summation transfer. (RESEARCH §1 surprise, §9.9.) |
| **R2 — allocation/xrun under high density × size × poly** (PERF-01/02) | Dense clouds × 8 voices could exhaust a dynamic grain list. | Preallocated `std::array<Grain,24>`/voice + steal-oldest; global cap 192. At worst the cloud thins — never xruns. (Gate 2.) |
| **R3 — scheduler density-model mismatch** | GrainScatter uses an exponential density%→ms mapping. | O-simpleGrain density is **grains/sec**: `period = fs/density`; scatter jitters it (RESEARCH §2.6, §9.10). |
| **R4 — up-transposition aliasing** (DSP-08) | High pitch-spray / grainPitch grains buzz. | 4-pt Lagrange read + per-grain rate-tracking one-pole `fc=0.5fs/rate` when `rate>1`, bypass at `rate≤1` (RESEARCH §5). Documented fallback: interpolation-only, then whole-engine 2× OS (would then need `setLatencySamples(N)`) — do NOT implement unless dullness appears. |
| **R5 — freeze/unfreeze + scan click** (QUAL-01) | Hard playhead jump on toggle/automation. | `SmoothedValue` on `playheadVelocity`/`scan`/`position`; ~5 ms crossfade on freeze engage/disengage (FreezeManager mechanic). Never hard-jump. (RESEARCH §4.2.) |
| **R6 — audio thread reads a half-loaded source** | Hot-swap mid-block. | Atomic `shared_ptr` swap; snapshot once per block (RESEARCH §6.3, §9.5). |
| **R7 — Base64 decode silently fails** (drag-drop) | `MemoryBlock::fromBase64Encoding` rejects standard `btoa()` output with no error. | Decode with **`juce::Base64::convertFromBase64(OutputStream&, StringRef)`** ONLY (RESEARCH §6.4, §9.2; project memory). |
| **R8 — sample-rate mismatch on embedded .wav** | The 4 built-ins are 44.1 kHz; engine may run 48 kHz. | Resample to engine rate at load, off the audio thread (RESEARCH §6.2, §9.12). Recompute grain/period/AA math on `prepareToPlay`. |
| **R9 — headroom/clipping on dense clouds** | Overlapping grains sum; peak grows with overlap. | Overlap-aware normalization (or fixed headroom factor) + soft safety + block-level `isfinite` + `outputLevel` trim. (§9.6.) |

---

# Phase 2.1 — Core Grain Engine + Overlap-Add + Amp ADSR + Key Resample

**Goal (ROADMAP):** A polyphonic, MIDI-playable granular voice — a preallocated bounded grain pool
scheduled at the density-derived period, each windowed grain read from the embedded default source and
overlap-added, shaped by the amp envelope, transposed by MIDI key. The audible granular core.

### Files to create / modify

**Create:**
- `Source/dsp/LagrangeInterpolation.h` — **copy verbatim** from `plugins/O-GrainScatter/Source/dsp/LagrangeInterpolation.h` (the 4-pt stateless `lagrangeInterpolate(ym1,y0,y1,y2,frac)`).
- `Source/dsp/WindowLuts.h` — the five precomputed 2048-pt window LUTs + linear-interp `read(shape, phase)`.
- `Source/dsp/Grain.h` — the `Grain` POD struct (fields per RESEARCH §2.1).
- `Source/GrainSound.h` — `GrainSound : juce::SynthesiserSound` (always applies).
- `Source/GrainVoice.h` — `GrainVoice : juce::SynthesiserVoice` (inline grain pool + scheduler + amp ADSR + overlap-add loop).

**Modify:**
- `Source/PluginProcessor.h` — add `juce::Synthesiser synth;`, a `WindowLuts` instance, the atomic source `shared_ptr`, and accessors needed for prepare.
- `Source/PluginProcessor.cpp` — ctor preallocates 8 voices + 1 sound + note-stealing; `prepareToPlay` dispatches voice prepare via `dynamic_cast` + decodes the default `fire.wav`; `processBlock` does push-params + `synth.renderNextBlock` + output gain + `isfinite` scrub.
- `CMakeLists.txt` — add the new headers to `target_sources` (no binary-data target yet — that's 2.3).

### Numbered task breakdown

**Task 1 — Copy primitives + build the window LUTs.**
- Copy `LagrangeInterpolation.h` verbatim into `Source/dsp/`.
- Create `WindowLuts.h`: a class holding `std::array<std::array<float, kWindowLutSize>, 5>` (rect / tri / Welch / Gauss / Hann), filled in the constructor with the exact shape math from RESEARCH §2.4 (`rect=1.0`; `tri=1−|2φ−1|`; `welch=1−(2φ−1)²`; `gauss=exp(−0.5((φ−0.5)/0.18)²)` normalized to 1.0 at centre; `hann=0.5(1−cos(2πφ))`). Add `float read(int shape, float phase) const` with **linear interpolation** by `phase∈[0,1]` (clamp phase, index `phase*(N-1)`). No per-sample transcendental in the read.
- Adds: `WindowLuts` class, `read()`.

**Task 2 — `Grain` struct + `GrainSound`/`GrainVoice` skeleton + per-sample scheduler + spawn/steal-oldest.**
- `Grain.h`: the POD from RESEARCH §2.1 (`active, readPos, rate, phase, phaseInc, lengthSamples, pan, shape, age, aaState`).
- `GrainSound.h`: copy `FMVoice.h:27-44` `SynthesiserSound` shape, rename to `GrainSound` (both `appliesTo…` return true).
- `GrainVoice.h`: skeleton from `O-simpleFM/Source/FMVoice.h` (`canPlaySound` via `dynamic_cast<GrainSound*>`; member `std::array<Grain, OSimpleGrainAudioProcessor::kMaxGrainsPerVoice> grains;`, `int nextGrain=0;`, per-voice `juce::Random rng;`, `int samplesUntilNextGrain=0;`, `double sampleRate;`, `float voiceRate=1.0f;`, `float velLevel=1.0f;`).
- Implement `spawnGrain(...)` per RESEARCH §2.2 (find inactive slot, else steal max-`age`; init `active/age=0/phase=0/readPos/rate/lengthSamples/phaseInc/pan/shape/aaState=0`; advance `nextGrain`). Bounded → never allocates.
- Implement the per-sample scheduler countdown per RESEARCH §2.6: `baseInterval = sampleRate / effectiveDensity` (effectiveDensity = `density` for now — `velToDensity` is 2.2), `if (--samplesUntilNextGrain <= 0) { spawnGrain(...); samplesUntilNextGrain = nextInterval; }`. (Scatter jitter is 2.2 — keep `nextInterval = (int)baseInterval` here, clamped `≥1`.)
- Adds: `Grain`, `GrainSound`, `GrainVoice` (members + `canPlaySound` + `spawnGrain` + scheduler).
- **Depends on:** Task 1 (needs `WindowLuts` + the grain `shape` index meaning).

**Task 3 — Non-virtual `prepareToPlay`, `startNote`/`stopNote`/lifetime, and the overlap-add render loop.**
- `GrainVoice::prepareToPlay(double sr, int /*maxBlock*/)` **non-virtual** (RESEARCH §3.1): `setCurrentPlaybackSampleRate(sr); sampleRate=sr; ampEnv.setSampleRate(sr); ampEnv.setParameters(ampParams);` — **setSampleRate before setParameters** (gate 8). Reset any `SmoothedValue`s here.
- `setParams(...)` block-push (RESEARCH §3.3): receives `grainSize, density, windowShape, grainPitch` and the `juce::ADSR::Parameters` amp struct (spray/scatter/pan/velToDensity arrive in 2.2; pass-through-ignore for now or omit until 2.2). Voices never touch APVTS.
- `startNote(midiNote, velocity, …)` (RESEARCH §3.4): `voiceRate = std::pow(2.0f, (midiNote - kRootNote)/12.0f)`; clear all grains (`active=false`); reset `samplesUntilNextGrain=0`; `velLevel = velocity` (velocity→amp always-on); `ampEnv.noteOn()`. Grains inherit the current playhead (position-only this phase — see Task 4).
- `stopNote(vel, allowTailOff)`: `allowTailOff ? ampEnv.noteOff() : (clearCurrentNote(); ampEnv.reset())`.
- `renderNextBlock(buffer, start, num)`: early-return if `!ampEnv.isActive()`; per sample: run scheduler → spawn; loop active grains computing `env = lut.read(g.shape, g.phase); src = readSourceLagrange(srcPtr, srcLen, g.readPos); s = src*env;` equal-power pan (`panL=cos(g.pan*halfPi), panR=sin(...)`), `outL+=s*panL; outR+=s*panR; g.readPos+=g.rate; g.phase+=g.phaseInc; ++g.age; if (g.phase>=1) g.active=false;`. Multiply `(outL,outR)` by `ampEnv.getNextSample() * velLevel`, add into the buffer. (AA one-pole is a no-op pass-through here — it lands in 2.2.) Call `clearCurrentNote()` when `ampEnv` goes inactive. **R1: this is the rewritten static-source loop — do NOT use GrainScatter's delay-tap read.**
- Adds: `prepareToPlay`, `setParams`, `startNote`, `stopNote`, `renderNextBlock`, the overlap-add loop, `readSourceLagrange` helper (RESEARCH §2.5 — clamp at source bounds, not wrap).
- **Depends on:** Task 2.

**Task 4 — Processor wiring: synth setup, default-source decode, processBlock, output stage.**
- Header: add `juce::Synthesiser synth;`, `WindowLuts windowLuts;`, `std::shared_ptr<juce::AudioBuffer<float>> currentSource;` (+ the `atomicLoad`/`atomicStore` helpers from RESEARCH §6.3), a `juce::SmoothedValue<float> outputGain;`, and a `float positionAbsolute` resting point (read-head position-only for now).
- Ctor: `for (i<kMaxVoices) synth.addVoice(new GrainVoice()); synth.addSound(new GrainSound()); synth.setNoteStealingEnabled(true);`.
- `prepareToPlay`: keep `setLatencySamples(0)`; `synth.setCurrentPlaybackSampleRate(sr)`; loop voices `dynamic_cast<GrainVoice*>` → `prepareToPlay(sr, block)` (RESEARCH §3.1); reset `outputGain` (20 ms); **decode the single default `fire.wav`** off the audio thread (RESEARCH §6.2 — for 2.1 read it from `Source/samples/fire.wav` via `AudioFormatManager::createReaderFor(File)` OR a `MemoryInputStream` over a temporary read; the binary-data embed lands in 2.3), resample to engine rate, `atomicStore(currentSource, …)`.
- `processBlock`: keep `ScopedNoDenormals` + `buffer.clear()`; read APVTS atomics once; for each voice `dynamic_cast` → `setParams(grainSize, density, windowShape, grainPitch, ampParams)`; push the global `positionAbsolute = (position/100)*srcLen` to a member the voices read at spawn (a plain pointer/value snapshot — full scan/freeze in 2.2); `auto src = atomicLoad(currentSource);` snapshot; pass the source pointer/len to the voices (via `setSource(...)` member or a shared pointer the voices read); `synth.renderNextBlock(buffer, midi, 0, numSamples)`; apply `outputGain` (dB→lin from `outputLevel`, smoothed) with overlap-aware headroom factor; block-level `std::isfinite` scrub (mirror O-simpleFM `PluginProcessor.cpp:310-315`). **No oversampler.**
- `isBusesLayoutSupported` already correct (Stage 1).
- `CMakeLists.txt`: add the new headers to `target_sources`.
- Adds: synth + source + output members and the full block wiring.
- **Depends on:** Tasks 1–3.

### Dependencies
`T1 → T2 → T3 → T4` (strictly sequential; the engine is one connected subsystem). T1 is independent and can start immediately.

### Success criteria (ROADMAP Test Criteria — verify phase checks these)
- [ ] Plugin loads in DAW as an **instrument**, MIDI routes, plays polyphonically (no crash).
- [ ] Low density (period > grain size) = audibly separated grains; raising density to overlap fuses them into a continuous cloud (FUNC-01/DSP-02).
- [ ] Grain size at a few ms = pitched buzz; tens of ms = recognizable source fragments (DSP-01).
- [ ] All five window shapes selectable; **rectangular audibly clicks**, Hann/Gaussian do not (DSP-03).
- [ ] Held MIDI notes transpose the cloud to pitch; chords work (FUNC-02).
- [ ] `processBlock` allocation-free under profiler; high density × size × 8 voices does NOT xrun (grains thin via steal-oldest) (PERF-01/02).
- [ ] Amp ADSR shapes notes; no stuck/silent voices; no clicks except the intentional rectangular artifact.
- [ ] **Build gate:** VST3+AU+Standalone build clean; AU registers (`auval`); `setLatencySamples(0)`; pluginval ≥8 passes.

---

# Phase 2.2 — Read Head (Scan / Time-Stretch / Freeze) + Spray & Scatter + Anti-Aliasing

**Goal (ROADMAP):** The granular "moves" — move/freeze/stretch the read head, scatter the cloud
(sync↔async), spray each grain independently, and keep up-transposed grains clean.

### Files to create / modify

**Modify:**
- `Source/PluginProcessor.h` — add the global read-head state: `double playheadPos;`, `juce::SmoothedValue<float> playheadVelocity, scanSmoothed, positionSmoothed;`, a freeze-crossfade gain, and a struct/snapshot the voices read at spawn (`playheadPos`, `positionSpray`, `freezeActive`).
- `Source/PluginProcessor.cpp` — advance the global playhead per sample in `processBlock` (before/around `synth.renderNextBlock`); read `scan`/`freeze`/`positionSpray`/`pitchSpray`/`scatter`/`panSpray`/`velToDensity` atomics and push to voices.
- `Source/GrainVoice.h` — add spray/scatter into the scheduler + spawn (`positionSpray` read start, `pitchSpray` rate, `scatter` period jitter, `panSpray` pan), `velToDensity` into effective density, and the AA one-pole into the grain read loop.

> No global read head was a *processor-owned* object in 2.1 (we used a static `positionAbsolute`).
> 2.2 promotes it to a moving, freezable playhead per ARCHITECTURE §Source Buffer + Read Head.

### Numbered task breakdown

**Task 5 — Processor-owned global read head (scan / position) with smoothing.**
- Per ARCHITECTURE §Read head + RESEARCH §4.1: in `processBlock`, per sample compute
  `vel = freezeActive ? 0 : (scan/100)*1.0` (realtime-relative; `scan ∈ [−200,+200]%`),
  `playheadVelocity.setTargetValue(vel); playheadPos += playheadVelocity.getNextValue();` wrap to
  `[0, srcLen)`. `position` sets the resting point (`positionSmoothed`); grains spawn at
  `grainReadStart = playheadPos (+ positionSpray in Task 7)`.
- `SmoothedValue` on `scan`, `position`, `playheadVelocity` (reset in `prepareToPlay`, ~20 ms).
- Snapshot `{playheadPos, frozen}` into the per-block struct the voices read at spawn.
- Adds: global playhead advance + smoothing in the processor.
- **Depends on:** Phase 2.1 (T4).

**Task 6 — Freeze: pin velocity to 0 + click-free crossfade; held-note sustain.**
- `freeze` bool read `freezeParam->load() > 0.5f` (RESEARCH §4.2). On engage: target `playheadVelocity → 0`;
  on disengage: ramp back to the scan-derived velocity. Use the ~5 ms crossfade-gain mechanic from
  `FreezeManager.h:15,67-85` (or simply rely on the `SmoothedValue` ramp on `playheadVelocity`) so
  engage/disengage is click-free (QUAL-01). Never hard-jump `playheadPos`.
- Held note sustains the frozen instant: the voice stays alive (amp env in sustain); grains keep reading
  the pinned region (± position spray from Task 7) → a frozen pad rather than one buzzing grain.
- Adds: freeze pin + crossfade.
- **Depends on:** Task 5.

**Task 7 — Per-voice spray & scatter RNG (position / pitch / period / pan) + velToDensity.**
- All via the per-voice `juce::Random rng` (no alloc/lock — gate 4). At `spawnGrain`:
  - **Position spray:** `grainReadStart = playheadPos + (rng.nextFloat()*2−1) * (positionSpray/100) * srcLen`.
  - **Pitch spray:** per-grain `pitchSprayRand = (rng.nextFloat()*2−1) * pitchSpray` st →
    `rate = voiceRate * 2^((grainPitch + pitchSprayRand)/12)`.
  - **Pan spray:** `g.pan = 0.5 + (rng.nextFloat()*2−1) * (panSpray/100) * 0.5` (clamp `[0,1]`; equal-power already in the loop).
  - **Scatter** (scheduler period jitter, RESEARCH §2.6): `jitter = (scatter/100) * baseInterval * (rng.nextFloat()*2−1); nextInterval = max(1, (int)(baseInterval + jitter))`.
  - **velToDensity:** `effectiveDensity = density * (1 + velToDensity * (velLevel−0.5)*2)`, clamp `[1,200]`, then `baseInterval = sampleRate / effectiveDensity`.
- Push the new params via `setParams` (extend the 2.1 signature: add `positionSpray, pitchSpray, scatter, panSpray, velToDensity`).
- Adds: full spray/scatter into scheduler + spawn.
- **Depends on:** Task 5 (needs `playheadPos`).

**Task 8 — Anti-aliasing one-pole in the grain read loop.**
- Per RESEARCH §5: `aaOnePole(x, rate, state)` — if `rate ≤ 1` bypass (`state = x; return x;`); else
  `fc = 0.5*sampleRate/rate; g = 1 − exp(−2π*fc/sampleRate); state += g*(x − state); return state;`.
  Use the per-grain `aaState` field (reset to 0 / first sample on spawn). Insert into the grain read loop
  between `readSourceLagrange` and the envelope multiply (the no-op placeholder from 2.1 Task 3).
- Adds: `aaOnePole` + its call site.
- **Depends on:** Task 7 (rate now carries pitch spray).

### Dependencies
`T5 → {T6, T7}`; `T7 → T8`. T6 and T7 both depend on T5 (the playhead) and can proceed in either order;
T8 needs T7's rate. Whole phase depends on 2.1.

### Success criteria (ROADMAP Test Criteria)
- [ ] Scan moves the read head through the source (forward/slowed/reverse/double); position sets where (DSP-06).
- [ ] Freeze pins the playhead — texture does not drift; freeze→unfreeze is click-free (FUNC-03, QUAL-01).
- [ ] Pitch spray and position spray apply independent per-grain randomization (frozen texture shimmers, no two grains identical) (DSP-04).
- [ ] Scatter at 0% = synchronous (discrete sidebands/pitched); high scatter = asynchronous (smeared/noisy) (DSP-05). *(Spectrum view is Stage 3 — verify audibly here.)*
- [ ] High pitch-spray grains stay clean — no unintended buzz/aliasing (DSP-08).
- [ ] No zipper on scan/position automation; no allocation/locks introduced by the per-grain RNG (QUAL-01, PERF-01).
- [ ] **Build gate:** clean build all 3 formats; AU registers; pluginval ≥8 passes; still `setLatencySamples(0)`.

---

# Phase 2.3 — Sample Loading + Visualization Taps

**Goal (ROADMAP):** Source selection/loading (embed all 4 + drag-drop + atomic hot-swap) + the
audio-thread taps that feed all four Stage-3 visualizations and the CPU readout.

### Files to create / modify

**Create:**
- `Source/dsp/TripleBuffer.h` — **copy verbatim** from `plugins/O-GrainScatter/Source/dsp/TripleBuffer.h` (lock-free SPSC).
- `Source/dsp/GrainCloudFrame.h` — `GrainEvent` + `GrainCloudFrame` structs (RESEARCH §7.2).
- `Source/VizAnalyzer.h` — **copy verbatim** from `plugins/O-simpleFM/Source/FmVizAnalyzer.h` (`VizRing` + `FmVizAnalyzer`; rename type if desired but keep the ring/FFT logic unchanged).

**Modify:**
- `CMakeLists.txt` — add `juce_add_binary_data(O-simpleGrain_Samples …)` for all 4 `.wav` (AFTER `juce_generate_juce_header`) + `target_link_libraries(O-simpleGrain PRIVATE O-simpleGrain_Samples)`; add the new headers to `target_sources`. (Replaces the Stage-1 TODO comment.)
- `Source/PluginProcessor.h` — add `sourceSample` decode-on-change, the `VizRing`, `TripleBuffer<GrainCloudFrame>`, `std::atomic<int> activeGrainCount`, drag-drop session state, and public accessors for the editor (`getVizRing()`, the `TripleBuffer&`, `getActiveGrainCount()`, `getCurrentSampleRate()`).
- `Source/PluginProcessor.cpp` — decode all 4 embedded sources; hot-swap on `sourceSample` change; register drag-drop NativeFunction C++ handlers + `FileChooser` picker; fill the three taps at the `processBlock` tail / at spawn.
- `Source/GrainVoice.h` — increment/decrement `activeGrainCount` on spawn/done; write a `GrainEvent` to the frame on each spawn.

### Numbered task breakdown

**Task 9 — Embed all 4 built-ins + decode/resample + atomic hot-swap on `sourceSample`.**
- CMake (RESEARCH §6.1): `juce_add_binary_data(O-simpleGrain_Samples NAMESPACE BinaryData HEADER_NAME BinaryData.h SOURCES Source/samples/fire.wav voice.wav water.wav piano.wav)` then `target_link_libraries(O-simpleGrain PRIVATE O-simpleGrain_Samples)` — **after** `juce_generate_juce_header`. Yields `BinaryData::fire_wav` / `fire_wavSize`, etc.
- Replace 2.1's file-read decode with `createReaderFor(std::make_unique<juce::MemoryInputStream>(BinaryData::xxx_wav, (size_t)BinaryData::xxx_wavSize, false))` (RESEARCH §6.2); `reader->read(&tmp,…)`; resample `srcRate→engineRate` via `juce::LagrangeInterpolator::process(srcRate/engineRate, in, out, numOut)` per channel (off the audio thread), cap to `kMaxSourceSeconds*engineRate`; `atomicStore(currentSource, newBuf)`.
- `sourceSample` (`AudioParameterChoice`) change → rebuild + atomic-publish off the audio thread (a parameter listener or a flag checked on the message thread; **never decode on the audio thread**).
- Adds: binary-data target, 4-source decode, hot-swap on selection.
- **Depends on:** Phase 2.1/2.2 (the engine reads `currentSource`).

**Task 10 — Load-your-own: drag-drop streaming C++ handlers + Base64 decode + picker fallback + 10 s cap.**
- Register the C++ NativeFunction handlers the shared JS module expects (RESEARCH §6.4; the JS surface is Stage 3, but the C++ contract is fixed now): `dropSessionStart(sessionId[,folderName])`, `dropSessionAddFile(sessionId, filename, base64)`, `dropSessionCommitFile(sessionId, filename, base64)` (single-file path suffices for one source).
- **Decode base64 with `juce::Base64::convertFromBase64(MemoryOutputStream&, StringRef)` ONLY** (R7 / §9.2): `juce::MemoryBlock raw; { juce::MemoryOutputStream mos(raw,false); if(!juce::Base64::convertFromBase64(mos, base64)) return false; }` → `createReaderFor(new juce::MemoryInputStream(raw, true), true)` → read → resample → `atomicStore`. Reap the session temp dir after commit.
- `FileChooser` picker fallback (async, message thread) for the same decode→resample→publish path.
- Truncate loaded files to `kMaxSourceSeconds*engineRate` (set a flag the UI surfaces in Stage 3).
- Update `currentSourceIdentity` to the loaded path (persists via the existing Stage-1 custom state).
- Adds: 3 NativeFunction handlers, `convertFromBase64` decode, picker, truncation.
- **Depends on:** Task 9 (shares the decode/resample/publish path).

**Task 11 — The three lock-free viz taps.**
- Copy `TripleBuffer.h` and `FmVizAnalyzer.h`→`VizAnalyzer.h` verbatim; define `GrainEvent`/`GrainCloudFrame` (RESEARCH §7.2) in `Source/dsp/GrainCloudFrame.h`.
- **VizRing (samples):** at the `processBlock` tail, post-gain mono-sum → `vizRing.write(...)` (RESEARCH §7.1; mirror O-simpleFM `PluginProcessor.cpp:317-340` — sum stereo to a stack mono buffer in ≤4096 chunks). **No FFT on the audio thread** (FFT is the Stage-3 message-thread Timer).
- **TripleBuffer<GrainCloudFrame> (grain events):** the processor owns one frame per block: `auto& f = grainBuffer.getWriteBuffer();` reset `f.count=0`; each voice writes a `GrainEvent{readPosNorm, sizeMs, pitchSemis, pan, spawnSample}` at spawn (bounded by `kMax=256`); the processor sets `f.playheadNorm/positionNorm/positionSprayNorm/frozen`; `grainBuffer.publish()` once per block.
- **`std::atomic<int> activeGrainCount`:** increment on spawn, decrement on grain-done; publish per block.
- Public accessors for the editor (Stage 3): `getVizRing()`, `getGrainBuffer()`, `getActiveGrainCount()`, `getCurrentSampleRate()` (mirror O-simpleFM `PluginProcessor.h:97-99`).
- Adds: 3 taps + accessors; grain-event writes in the voice.
- **Depends on:** Task 9 (needs `srcLen` for `readPosNorm`); independent of Task 10.

### Dependencies
`T9 → {T10, T11}`. T10 (drag-drop) and T11 (taps) are independent of each other and may proceed in
either order once T9's decode/publish path exists. Whole phase depends on 2.1+2.2.

### Success criteria (ROADMAP Test Criteria)
- [ ] All built-in sources selectable and granulated; "fire" worked example reproducible (FUNC-04).
- [ ] Drag-drop a user `.wav` on macOS loads and granulates (content-streaming, correct base64 decode); picker fallback works; oversized files truncated to 10 s with notice (FUNC-05).
- [ ] Source swap is glitch-free (atomic publish; no audio-thread access to half-loaded buffer).
- [ ] `processBlock` remains allocation-free; viz ring + grain-event triple-buffer + atomic count fill without locks; FFT not on the audio thread (PERF-01).
- [ ] `activeGrainCount` and derived overlap update live (drives UI-05 in Stage 3).
- [ ] **Build gate:** clean build all 3 formats (incl. the new binary-data target); AU registers; pluginval ≥8 passes; still `setLatencySamples(0)`.

---

## Sequencing Concerns / Notes for the dsp-agent

1. **2.1 default-source decode is throwaway-ish.** 2.1 decodes `fire.wav` from `Source/samples/` (file
   read) to get audible quickly; 2.3 Task 9 replaces that with the embedded `BinaryData` path. Build the
   2.1 decode behind the **same `atomicStore(currentSource,…)` publish** so 2.3 only swaps the *source* of
   the bytes, not the engine plumbing. (Gate 7 holds from 2.1 onward.)
2. **`setParams` signature grows across phases.** 2.1 pushes `grainSize/density/windowShape/grainPitch/ampParams`;
   2.2 adds `positionSpray/pitchSpray/scatter/panSpray/velToDensity`. Define it generously in 2.1 (or accept
   a small struct) to avoid churn — but do not *use* the spray/scatter values until 2.2.
3. **The global read head changes shape between 2.1 and 2.2.** 2.1 uses a static `positionAbsolute` resting
   point (no motion); 2.2 promotes it to a moving/freezable `playheadPos`. Keep the voice's spawn-time read
   of "current playhead" abstract (a value/pointer the processor sets per block) so 2.2's promotion doesn't
   touch the voice's spawn signature.
4. **AA one-pole is a no-op placeholder in 2.1, live in 2.2.** Wire the call site in 2.1 Task 3 (returning
   `x` unchanged) so 2.2 Task 8 only fills the body — avoids re-touching the hot loop.
5. **Drag-drop is C++-only in Stage 2.** Register the NativeFunction handlers + decode path now (2.3 Task 10);
   the JS surface that calls them is Stage 3.1. The 4 handler *names* are fixed by the shared module — do not
   rename them.
6. **R1 is the one place to be careful.** The grain read loop is the *rewritten* static-source loop, NOT a
   copy of GrainScatter's delay-tap read. Everything else from GrainScatter (spawn/steal-oldest, equal-power
   pan, envelope-by-phase, overlap-add) transfers; the read addressing does not.
