# Stage 2 (DSP) — PLAN

**Plugin:** O-simpleBeatmaker · **Stage:** 2 DSP · **Date:** 2026-06-25 · **Mode:** express (staged)
**Executor:** a single `dsp-agent`, top-to-bottom, **stopping at each of the 3 sub-phase checkpoints**.

**Goal:** Implement the full immutable DSP spine — 6 synthesized 808/909 voices (MIDI-playable), a host-synced sample-accurate step sequencer, and the swing/humanize/quantize timing-feel engine — where the internal sequencer emits GM-mapped `MidiMessage` note-ons at sample-accurate offsets into the SAME `MidiBuffer` as host MIDI, and the viz tap reads those very messages (`appliedSampleInBar − nominalSampleInBar`) so QUAL-02 is true by construction. First audio happens here. UI is Stage 3.

**The hard part is timing accuracy, not signal complexity.** De-risk in order: voices first (2.1, conventional), then prove the straight-time grid is sample-accurate in the harness (2.2) **before** adding any feel math (2.3). The offline render-harness, built alongside, is the automated gate between sub-phases.

**Inputs:**
- `research/ARCHITECTURE.md` — immutable DSP spec (voice table §3, exact swing/humanize/quantize math §2, processing order, latency=0).
- `stages/2-dsp/RESEARCH.md` — verified JUCE 8.0.9 signatures, code sketches, sub-slice transcription, Mechanism Gaps, Fallback A.
- `stages/2-dsp/CONTEXT.md` — staged brief + the 6 render-harness probes (these ARE the success criteria).
- `parameter-spec.md` — locked 42-param APVTS contract (stored ranges/IDs immutable).
- `Source/PluginProcessor.{h,cpp}` — Stage 1 shell: 42-param APVTS, `ParamIDs`, `kGmNotes {36,38,39,42,46,45}`, `kVoicePrefix`, `voiceParamID()`, lock-free `grid` + `getStep`, currently-silent `processBlock`.
- **Primary code template:** O-simpleFM `Source/{Operator.h,FMVoice.h,FmVizAnalyzer.h,PluginProcessor.cpp}` + `tests/render-harness/{main.cpp,CMakeLists.txt}`.

---

## Files

| File | Action | Purpose |
|------|--------|---------|
| `Source/fastSine.h` | **create** | Copy O-simpleFM `Operator.h` `fastSine` into **`namespace OSimpleBeatmaker`** (rename namespace; do NOT keep `OSimpleFM`). 1024-pt `dsp::LookupTableTransform`, function-local static, **mandatory floor-modulo wrap** (`phase -= twoPi*floor(phase/twoPi)`) + `isfinite` guard. Sine body for kick/tom/snare. |
| `Source/DrumVoiceEngine.h` | **create** | The 5 voice structs + 6-voice container. Header-only (suite norm). Exp envelopes, per-voice decay-ms mapping, filtered noise, choke, clap multi-burst, velocity→loudness+timbre. |
| `Source/UnifiedTriggerRouter.h` | **create** | GM `noteToVoice[128]` reverse map; mute/solo gating; the sub-slice render driver (transcribed from `Synthesiser::processNextBlock`); buffer merge. |
| `Source/SequencerClock.h` | **create** | Host-transport → firing steps. `getPosition()` once/block (3 guards); step enumeration in the block's ppq window; commit-once / discontinuity resync; standalone free-run; `playheadStepPhase` atomic. |
| `Source/TimingFeelEngine.h` | **create** | Per-hit Δt + velocity. Exact `Δswing + Δhuman·(1−q)` math (terms NOT folded). Pre-seeded per-voice `juce::Random`. **Fallback A: late-only humanize** for v1.0. Fixed-array carry-over queue for late offsets crossing the block end. |
| `Source/VizAnalyzer.h` | **create** | `juce::AbstractFifo` + POD `VizEvent` ring; `std::atomic<float> playheadStepPhase`. Pushed in the SAME path as the sequencer `addEvent` (QUAL-02 by construction). |
| `Source/PluginProcessor.h` | **modify** | Own the 6-voice `DrumVoiceEngine`, `SequencerClock`, `TimingFeelEngine`, `UnifiedTriggerRouter`, `VizAnalyzer`, master `SmoothedValue`, member `sequencerMidi` MidiBuffer, cached APVTS atomic pointers. Add viz accessors + `#if OUARICON_BUILD_TESTS` emitted-MIDI test hook. |
| `Source/PluginProcessor.cpp` | **modify** | `prepareToPlay` (voice/filter prepare, RNG seed, `ensureSize`, smoother reset, `setLatencySamples(0)`). `processBlock` (read playhead+params once; clock enumerate; feel compose+emit; merge; sub-slice render; mixer; viz push; playhead store). `ScopedNoDenormals`. |
| `CMakeLists.txt` | **modify** | Add the new `Source/*.h` to `target_sources`; un-comment the `OUARICON_BUILD_TESTS` option + `add_subdirectory(tests/render-harness)` block (lines 80–85). |
| `tests/render-harness/main.cpp` | **create** | Port O-simpleFM harness. Add `FakePlayHead : juce::AudioPlayHead` (synthetic transport) + the 6 probes. |
| `tests/render-harness/CMakeLists.txt` | **create** | `juce_add_console_app`; reach into plugin sources + new `.cpp` (none if headers stay header-only); borrow include dirs; `JucePlugin_*` defs (PluginCode `0x4f536942` "OSiB", ManufacturerCode `0x4f756172` "Ouar", IsSynth=1, WantsMidiInput=1). **NO UIResources link, NO `JUCE_WEB_BROWSER=1`** (Stage-1 editor is a `GenericAudioProcessorEditor` shell — verified no WebView symbols). |

**No new APVTS params. No new external packages.** All dependencies are the vendored local JUCE 8.0.9 tree.

---

## Tasks (grouped by the 3 sub-phases + harness — execute top-to-bottom; commit + checkpoint after each sub-phase)

### ── Sub-phase 2.1 — DrumVoiceEngine (MIDI-playable, NO sequencer) ──
*Conventional, low-risk half. 6 voices triggered by incoming MIDI only (GM map).*

#### T1 — `fastSine.h` + the 5 voice structs (`DrumVoiceEngine.h`)
1. `fastSine.h`: copy O-simpleFM `Operator.h` verbatim; rename `namespace OSimpleFM` → `namespace OSimpleBeatmaker`. Keep the floor-modulo wrap + `isfinite` guard (LUT clamps out-of-range, does not wrap — high-pitch tones flat-line without it).
2. `DrumVoiceEngine.h`: define structs **`KickVoice`, `SnareVoice`, `ClapVoice`, `HatVoice`, `TomVoice`** in `namespace OSimpleBeatmaker`. **Never name a struct bare `Voice`** — it collides with the existing `enum Voice { Kick, ... }` in `PluginProcessor.h`; avoid any `juce::` type name (`Random`, `Synthesiser`, `SynthesiserVoice`) per the O-simpleSampler `SamplerVoice`/`SamplerSound` precedent. `HatVoice` holds BOTH the closed and open hat (shared noise→BP source, separate envelopes) plus an `applyChoke()` method.
3. Each voice exposes the uniform interface: `prepareToPlay(double fs)`, `setParams(float tuneSt, float decay01, float tone01, float levelDb)`, `trigger(juce::uint8 velocity)`, `render(float* L, float* R, int start, int n)` that **adds** into the buffer span and advances its tail.
4. Exponential envelopes (ARCHITECTURE §Algorithm Details): recompute on rate/decay change `ampCoef = std::exp(-1.0/(decaySec*fs))`; per sample `ampEnv *= ampCoef`; `ampEnv` seeded to `1.0f` on `trigger()`; flush `if (ampEnv < 1.0e-6f) ampEnv = 0.0f;` → voice idle (denormal protection on tails). **Per-voice decay-ms mapping (`mapDecay01ToSeconds`) lives HERE, NOT in the param range** — kick boom (hundreds of ms) vs hat tick (tens of ms) differ by an order of magnitude.
5. Synthesis per ARCHITECTURE §3 voice table (808/909 flavor RESOLVED):
   - **Kick (36, 808):** sine/triangle body via `fastSine` + a faster exp **pitch** env (`fInst = fBase + pitchEnv*pitchAmt`, pitchEnv decay ~10–30 ms, ≈ +1–2 oct → base 40–60 Hz) + amp env + short (1–2 ms) click transient. `tone` = pitch-env amount + click level.
   - **Tom (45, 808):** pitched sine/triangle body + mild pitch+amp env. `tone` = pitch-env amount.
   - **Snare (38, 808/909 hybrid):** two detuned tonal oscs (~180 + ~330 Hz) + band-passed noise burst. `tone` = body↔noise balance.
   - **Clap (39, 808):** 3–4 retriggered noise bursts ~10 ms apart → band-pass + short diffuse tail. Implement with `int burstsRemaining` + `int samplesToNextBurst` counters inside the voice; re-seed amp env to 1.0 on each burst (RT-safe, no allocation). `tone` = burst spread vs tail.
   - **Closed Hat (42) / Open Hat (46):** shared HP/BP filtered-noise source; closed = very short decay, open = long decay. `tone` = filter Q / brightness.
6. Filtered-noise voices use **`juce::dsp::StateVariableTPTFilter<float>`** per-sample form: `prepare({ fs, (uint32) maxBlock, 1 })` once in `prepareToPlay`; `setType`/`setCutoffFrequency`/`setResonance`; `processSample(0, noise)` per sample (NOT the block `process(context)` form). Noise = pre-seeded `juce::Random` (`nextFloat()*2-1`) or inline xorshift; **seed in `prepareToPlay`, never on the audio thread.**
7. Hi-hat choke: on a closed-hat trigger, call `openHat.applyChoke()` which swaps the open-hat active env coefficient to a fast release `chokeCoef = std::exp(-1.0/(0.003*fs))` (~3 ms) so the open tail fast-fades.
8. Velocity → loudness + timbre: `vel01 = velocity/127`; gain = primary (`levelDb` + vel curve); timbre scalar adds pitch-env on kick/tom, more HF/noise on snare/hat, slightly faster attack — **composes (multiplies) with the per-voice `tone` knob, does not replace it.** Identical path for host MIDI and (later) sequencer MIDI.

**Verify:** compiles into the plugin target (header-only); no `juce::` name collisions. Audio correctness gated by Probe 5 in T3.

#### T2 — `DrumVoiceEngine` container + `UnifiedTriggerRouter` (MIDI half) + Mixer; wire into processor
1. `DrumVoiceEngine`: hold the 6 voices in voice-row order (0 Kick, 1 Snare, 2 Clap, 3 ClosedHat, 4 OpenHat, 5 Tom). Methods: `prepareToPlay(fs, maxBlock)`, `setParams(...)` per voice from cached APVTS, `trigger(int voiceIndex, uint8 velocity)`, `renderAll(buffer, start, n)` (calls each active voice's `render`).
2. `UnifiedTriggerRouter` (MIDI half this phase): build `int noteToVoice[128]` (init −1, fill the six from `kGmNotes`). `handleTrigger(const MidiMessage& m)`: `if (!m.isNoteOn()) return; int v = noteToVoice[m.getNoteNumber()]; if (v < 0) return;` (out-of-map ignored, no error). Apply **mute/solo at trigger time**: if any solo active and this voice not soloed → skip; else if this voice muted → skip; closed-hat (v==3) trigger also calls `openHat` choke.
3. Mixer/master: sum 6 voices → stereo; `juce::SmoothedValue<float>` on output gain, `reset(fs, 0.02)` in prepare, `setTargetValue(juce::Decibels::decibelsToGain(outDb, -60.0f))` per block (`−60 dB ⇒ silence`).
4. Processor wiring (`prepareToPlay`): call `voices.prepareToPlay`; seed RNG; reset smoother; `setLatencySamples(0)` (already present — **never override the non-virtual `getLatencySamples()`**). Cache APVTS atomic pointers (mirror O-simpleFM `PluginProcessor.cpp:192` `auto get = [this](const char* id){ return parameters.getRawParameterValue(id)->load(); };`).
5. Processor `processBlock` (2.1 path — **MIDI-only, no sequencer yet**): `ScopedNoDenormals`; `buffer.clear()`; read all 42 params once/block → per-voice `setParams`; sub-slice render over **host `midiMessages` only** (use the T-router driver; full sub-slice loop lands in T5 but the MIDI-playable path needs spans now); apply master gain ramp; block-level `std::isfinite` scrub (suite-wide NaN insurance).

**Verify:** builds VST3 + AU + Standalone; loads as an instrument; MIDI note-ons fire voices.

#### T3 — Render-harness skeleton + Probe 5
1. `tests/render-harness/CMakeLists.txt`: `juce_add_console_app(O-simpleBeatmaker-render-test ...)`. `target_sources`: `main.cpp` + `../../Source/PluginProcessor.cpp` + `../../Source/PluginEditor.cpp` (so `createEditor()` resolves at link). `add_dependencies(... O-simpleBeatmaker)` + `target_include_directories(... $<TARGET_PROPERTY:O-simpleBeatmaker,INCLUDE_DIRECTORIES>)`. Replicate the `JucePlugin_*` defs block with PluginCode `0x4f536942`, ManufacturerCode `0x4f756172`, `JucePlugin_IsSynth=1`, `JucePlugin_WantsMidiInput=1`, `JucePlugin_ProducesMidiOutput=0`. Add `OUARICON_BUILD_TESTS=1`. **DIVERGES from FM template: do NOT link `*_UIResources`, do NOT set `JUCE_WEB_BROWSER=1`** (no binary-data target at Stage 2; editor is a generic shell). If a later link error proves `PluginEditor.cpp` pulls a WebView symbol, add `JUCE_WEB_BROWSER=1` (per RESEARCH assumption A4) — not expected.
2. `CMakeLists.txt` (plugin): add the new `Source/*.h` to `target_sources`; un-comment the stubbed block (lines 80–85): `option(OUARICON_BUILD_TESTS "Build O-simpleBeatmaker render-test harness" OFF)` + `if(OUARICON_BUILD_TESTS) add_subdirectory(tests/render-harness) endif()`.
3. `main.cpp`: port O-simpleFM `main.cpp` — `ScopedJuceInitialiser_GUI`, `setPlayConfigDetails(0,2,fs,block)`, `prepareToPlay`, block-loop `render()`, `setParam` via `convertTo0to1`, `rms`/`peakAbs`/`binAmplitude`/`allFinite` helpers, the `check(name, ok, detail)` + failure-count + `return failures==0?0:1` harness shell.
4. **Probe 5 — MIDI-playable + voices (DSP-06 / QUAL-01):** inject a host `noteOn` per GM note (36/38/39/42/46/45) → assert the correct voice produces non-trivial RMS; closed-hat (42) then open-hat (46) → open tail truncates (choke); velocity scales RMS; high-rate repeated hits → output finite + bounded peak + aliasing budget (reuse FM's inter-harmonic DFT probe: alias energy ≪ harmonic energy).

**◆ CHECKPOINT 2.1** — commit `stage: O-simpleBeatmaker Stage 2.1 (DrumVoiceEngine) complete`. Gate: builds VST3+AU+Standalone clean; **Probe 5 passes**. STOP for checkpoint review.

---

### ── Sub-phase 2.2 — SequencerClock + UnifiedTriggerRouter (host-synced grid, swing/humanize OFF) ──
*Prove the sample-accurate straight-time grid BEFORE any feel math.*

#### T4 — `SequencerClock.h` (host-synced enumeration + free-run + discontinuity safety)
1. Read the playhead **once per block, only inside `processBlock`** (calling `getPosition()` elsewhere is UB). Three independent guards: `getPlayHead()` may be null; `getPosition()` may be empty; every `PositionInfo` field is `juce::Optional<...>` (except `getIsPlaying()` which is a bare `bool`). Canonical read per RESEARCH §2.2: default `bpm = tempoParam`, `synced=false`; if playhead+position+isPlaying → take `getBpm()` (guard div-by-zero), `getPpqPosition()` (sets `ppqStart`, `synced=true`).
2. Step enumeration (RESEARCH §2.2): `samplesPerPpq = (60/bpm)*fs`; `samplesPer16th = samplesPerPpq*0.25`; `blockPpq = numSamples/samplesPerPpq`; `barLenPpq = patternLength*0.25`; `barStart = lastBarStartPpqOpt ? *opt : ppqStart - fmod(ppqStart, barLenPpq)`. For `k` in `0..patternLength`: `stepPpq = barStart + k*0.25` (also test `±barLenPpq` neighbour bar to catch wrap at window edges); if `stepPpq ∈ [ppqStart, ppqStart+blockPpq)` → `nominalOffset = (int) llround((stepPpq-ppqStart)*samplesPerPpq)`; `vel = grid[cellIndex(voice,k)].load(std::memory_order_relaxed)`; if `vel > 0` enqueue a firing `(voice, k, nominalOffset, vel)`. Straight time: `appliedOffset == nominal`.
3. Commit-once / discontinuity safety: ppq is NOT monotonic (loop/relocate/tempo automation). Track `lastBlockEndPpq` (or `lastEmittedAbsStepPpq` per voice); if `ppqStart < lastBlockEndPpq − epsilon` or it jumps forward by > one block → treat as discontinuity and **resync** (re-derive from new ppq, clear pending) rather than back-fill. Prevents double-fires / misses at block edges (Probe 4).
4. Standalone / stopped free-run: when `getPlayHead()==nullptr` OR position empty OR `getIsPlaying()==false` → integrate `phaseInSteps += numSamples / samplesPer16th` (from `tempoParam`); `while (phaseInSteps >= patternLength) phaseInSteps -= patternLength;` emit each integer step-boundary crossing within `[0, numSamples)` at its in-block offset. On transition free-run→host-synced, **snap** `phaseInSteps` to the host-derived step phase (no audible catch-up — Mechanism Gap #3).
5. `patternLength` (8/16/32, from the choice param) sets the wrap length. `playheadStepPhase` atomic updated once/block (`store((float) fractionalStepIndex, std::memory_order_relaxed)`) for the Stage-3 sweep.
6. **Accepted simplification (document):** bpm read once/block; sub-block tempo ramps are out of scope for v1.0 (Mechanism Gap #4).

#### T5 — Emit → merge → sub-slice render in `processBlock`; FakePlayHead + test hook; Probes 1 & 4
1. Sequencer emit: clear the member `sequencerMidi` (pre-`ensureSize`d in `prepareToPlay`); for each firing step emit `juce::MidiMessage::noteOn(1, kGmNotes[voice], (uint8) vel)` at `nominalOffset` via `sequencerMidi.addEvent(...)` (kept sorted, ties stable). **At 2.2 the feel engine is bypassed** → `appliedOffset == nominalOffset` (straight time).
2. Merge (sorted): `sequencerMidi.addEvents(hostMidi, 0, numSamples, 0)` (host stays merged + sorted into the same buffer). The merged `sequencerMidi` is the single stream feeding voices AND (in 2.3) the viz.
3. Sub-slice render — **transcribe `juce::Synthesiser::processNextBlock`** (RESEARCH §Sub-slice, verified `juce_Synthesiser.cpp:180–236`): iterate `merged.findNextSamplePosition(0)`; render all voices over each `[startSample, toNext)` span (advancing tails), then `handleTrigger(meta.getMessage())` at the event offset; drain the tail span. **Skip JUCE's `minimumSubBlockSize` coalescing branch** — 1-sample sub-blocks are fine (light CPU) and sample-exact offsets are the whole point of the gate.
4. `playheadStepPhase` store once/block (from clock).
5. Harness `FakePlayHead : juce::AudioPlayHead` (RESEARCH §Render-harness): override the single pure-virtual `getPosition()`; return a `PositionInfo` with `setBpm`/`setPpqPosition`/`setPpqPositionOfLastBarStart`/`setTimeInSamples`/`setIsPlaying(true)`/`setTimeSignature(4/4)`. Inject via `proc.setPlayHead(&fake)`; **advance `fake.ppq += numSamples/samplesPerPpq` after each block** so a multi-block bar renders (makes Probe 4 meaningful).
6. Emitted-MIDI test hook: `#if OUARICON_BUILD_TESTS` accessor on the processor returning a const ref to the last block's `sequencerMidi` (and/or the pushed `VizEvent`s in 2.3) so offsets are asserted **directly**, not only inferred from audio transients.
7. **Probe 1 — grid accuracy (straight time, swing=humanize=0, quantize=100):** for each ON step `k`, emitted offset == `round(k * samplesPer16th)` within the bar (±0 samples). Read via the test hook.
8. **Probe 4 — block-boundary independence:** choose a block size so a step's nominal onset lands near a block edge; assert it fires once, at the correct absolute sample, in the correct block (no snapping, no double-fire, no miss). Re-exercised with Δt-straddle in 2.3.
9. Confirm sequencer-emitted note 36 and host-played note 36 are indistinguishable to the voice layer (one MIDI stream).

**◆ CHECKPOINT 2.2** — commit `stage: O-simpleBeatmaker Stage 2.2 (SequencerClock) complete`. Gate: builds clean; **Probes 1 & 4 pass**; DAW transport-sync smoke test (Logic AU / Reaper or Ableton VST3 / Standalone free-run) per CLAUDE.md cache-clear + install. STOP for checkpoint review.

---

### ── Sub-phase 2.3 — TimingFeelEngine + VizAnalyzer (swing/humanize/quantize Δt + truthful viz tap — THE lesson) ──

#### T6 — `TimingFeelEngine.h` (Δt composition, Fallback A late-only, carry-over queue); Probes 2 & 3
1. Pre-seeded `juce::Random voiceRng[6]` (one `setSeed` per voice in `prepareToPlay`; `nextFloat()` on the audio thread, **never reseed**). `triangular(rng) = (rng.nextFloat()*2-1 + rng.nextFloat()*2-1)*0.5` → centre-weighted `[−1,1]` (ARCHITECTURE *recommends* triangular over flat uniform).
2. **Exact composition — DO NOT fold the terms** (RESEARCH §2.3, ARCHITECTURE §2):
   - `swing01 = swingParam` (stored 0–1, display 0–75%); `q = quantizeParam`; `T8 = (60.0/bpm)/2.0`.
   - **Swing (deterministic, NOT scaled by q):** `dSwing = (k % 2 == 1) ? (swing01/3.0)*T8 : 0.0` (off-beat/odd 16ths delayed; on-beat unmoved).
   - **Humanize (sampled ONCE per committed hit):** timing `dHumanT`, velocity `dHumanV = triangular(voiceRng[v]) * humanizeParam * 24.0` (±24).
   - **Compose (q scales ONLY the humanize terms):** `dtSec = dSwing + dHumanT*(1.0-q)`; `appliedOffsetSamples = (int) llround(dtSec*fs)`; `finalVel = jlimit(1, 127, (int) lround(stepVel + dHumanV*(1.0-q)))`; `finalOffset = nominalOffset + appliedOffsetSamples`; emit `noteOn(1, kGmNotes[v], finalVel)` at `finalOffset`.
   - **DSP-04 invariant (the entire lesson):** at `q=1`, `dHumanT*(1−q)=0` and `dHumanV*(1−q)=0` → humanize vanishes, **but `dSwing` survives**.
3. **ADOPT Fallback A — late-only humanize for the v1.0 gate** (RESEARCH Mechanism Gap #1, ARCHITECTURE Risk §Fallback A, blessed): timing humanize is one-sided positive — `dHumanT = triRand01(voiceRng[v]) * humanizeParam * 0.030` where `triRand01 = (nextFloat()+nextFloat())*0.5 ∈ [0,1]` (max +30 ms, musically "laid back"). This removes negative offsets and the cross-block lookahead from the critical path. **Velocity humanize stays symmetric** (`dHumanV ∈ [−24,+24]` — velocity is not a sample offset). Document full symmetric (±) timing handling as a **post-gate enhancement**, not v1.0.
4. Carry-over queue for late offsets crossing the block end: fixed-capacity `std::array<PendingHit, N>` on the audio thread (alloc-free). When a hit's `finalOffset >= numSamples`, stash it with an absolute target sample; each block, drain entries whose `absTarget ∈ [blockStartAbs, blockStartAbs+numSamples)` and emit at the local offset. With Fallback A this is strictly the one-sided **late** case (no negative/lookahead branch).
5. Wire into `processBlock` step 3 (per processing order): the feel engine consumes each firing step from the clock and emits into `sequencerMidi` BEFORE the merge, so the merged stream is already sample-accurate. Guard div-by-zero on bpm.
6. **Probe 2 — swing (swing=75% ⇒ `swing01=1`):** odd-`k` emitted offset == `nominal + round((1/3)*T8*fs)`; even-`k` unmoved. (Spot-check swing≈50% display → off-beat lands on the 8th-note triplet within tolerance.)
7. **Probe 3 — humanize + quantize (humanize=1, quantize=0):** measured timing spread > 0 and bounded (`|Δt| <= round(0.030*fs)`, late-only ⇒ `Δt ∈ [0, +30ms]`); velocity spread bounded by ±24. **Then quantize=1 → timing spread → ~0 AND velocity spread → ~0, BUT swing offsets remain** (DSP-04 gate).

#### T7 — `VizAnalyzer.h` (truthful Δt tap); Probe 6
1. POD `struct VizEvent { juce::uint8 voiceIndex; juce::int16 stepIndex; juce::int32 nominalSampleInBar; juce::int32 appliedSampleInBar; juce::uint8 velocity; juce::uint8 source; }` (`source`: 0=sequencer, 1=host MIDI). `juce::AbstractFifo fifo{capacity}` + `std::array<VizEvent,capacity> store`. Producer (audio): `const auto scope = fifo.write(1); if (scope.blockSize1 > 0) store[scope.startIndex1] = ev;` (RAII commit in the scope destructor — no manual `finishedWrite`). Memory-ordering discipline per O-simpleFM `VizRing`.
2. **Both sample fields are bar-relative and come from the SAME scheduling pass:** `nominalSampleInBar = round(stepPpqInBar * samplesPerPpq)`; `appliedSampleInBar = nominalSampleInBar + appliedOffsetSamples` (store **raw / unwrapped** per Mechanism Gap #2 decision — UI subtracts → correct Δt regardless; Stage-3 handles bar-position display modulo `samplesPerBar`).
3. **Push the `VizEvent` in the SAME code path that calls `sequencerMidi.addEvent`** (one emit = one push) so audio and viz can never diverge — `Δt = applied − nominal` IS the offset baked into the emitted `MidiMessage`, never recomputed from the swing/humanize formula (QUAL-02 by construction). Host-MIDI note-ons also push a `VizEvent` (`source=1`, `nominal==applied`) so the readout shows both streams.
4. Playhead is the separate `std::atomic<float> playheadStepPhase` (continuous, one store/block) — decoupled from the event ring.
5. Editor: no WebView yet (Stage 3). At Stage 2 just confirm the ring is fed (a frame/event counter advances) and the push path is allocation-free.
6. **Probe 6 — viz truth (QUAL-02):** for emitted hits, `appliedSampleInBar − nominalSampleInBar` == the `appliedOffsetSamples` baked into the corresponding emitted `MidiMessage` (compare the test-hook MidiBuffer offset against the pushed `VizEvent`).

**◆ CHECKPOINT 2.3** — commit `stage: O-simpleBeatmaker Stage 2.3 (TimingFeelEngine + VizAnalyzer) complete`. Gate: builds clean; **Probes 2, 3, 6 pass** (all 6 green); `processBlock` allocation-free + lock-free (PERF-01); pluginval VST3 **and** AU at repo strictness; DAW smoke (swing/humanize/quantize audibly behave, quantize leaves swing). STOP for Stage-2→3 handoff.

---

## Dependencies

Strictly sequential by sub-phase: **2.1 → 2.2 → 2.3.** Within-phase: T1 → T2 → T3 (voices before container before harness probe); T4 → T5 (clock before emit/render); T6 → T7 (feel math before the viz tap that reads its output). The harness is built incrementally: skeleton + Probe 5 at 2.1, FakePlayHead + test hook + Probes 1/4 at 2.2, Probes 2/3/6 at 2.3. Each sub-phase is one checkpoint commit gated by its probes.

**Processing order in `processBlock` (per block, ARCHITECTURE §Processing Order — order is load-bearing):**
1. Read playhead + all 42 params once (atomic loads).
2. SequencerClock enumerates firing steps (host-synced or free-run), reading the lock-free PATTERN grid.
3. TimingFeelEngine computes Δt + velocity (RNG sampled once each) → emit into `sequencerMidi`.
4. Merge `sequencerMidi` with host MIDI (sorted).
5. Sub-slice render: per `[event_i, event_{i+1})` span advance all voice tails, then apply `event_i`'s trigger (GM→voice, choke, velocity).
6. Mixer/master sums voices, applies `outputLevel` (smoothed).
7. VizAnalyzer pushed during step 3/4 (events) + once for the playhead.

*Why order matters: swing/humanize must be applied BEFORE the merge so the merged stream is already sample-accurate; sub-slicing AFTER merge is what makes Δt audible rather than block-snapped.*

## Build / validation (run after each checkpoint)

```
cmake -B build -G Ninja -DOUARICON_BUILD_TESTS=ON
ninja O-simpleBeatmaker-render-test            # gate: exit 0 iff all built probes pass
ninja O-simpleBeatmaker_VST3 O-simpleBeatmaker_AU O-simpleBeatmaker_Standalone
# AU cache clear + dual-variant sweep + install per CLAUDE.md (scripts/build-and-install.sh O-simpleBeatmaker)
pluginval --strictness-level <repo default> ...VST3   # 2.3 gate, VST3 + AU
auval -v aumu OSiB Ouar                        # after install; renders audio (catches NaN/crash)
```

## Success criteria (the 6 render-harness probes + builds + pluginval — these ARE the gate)

- [ ] **Probe 1 — grid accuracy:** straight time, each ON step `k` fires at exactly `round(k * samplesPer16th)` within the bar (±0 samples). *(2.2)*
- [ ] **Probe 2 — swing:** swing=75% → off-beat (odd) 16ths delayed by exactly `round((swing01/3)*T8*fs)` samples; on-beat (even) 16ths unmoved; swing≈50% display → off-beat on the 8th-note triplet. *(2.3)*
- [ ] **Probe 3 — humanize + quantize:** humanize=100/q=0 → timing spread > 0 bounded by +30 ms (late-only), velocity spread ≤ ±24; **q=100 → humanize spread → ~0 BUT swing offsets remain** (DSP-04 invariant). *(2.3)*
- [ ] **Probe 4 — block-boundary independence:** a step (± Δt) straddling a block edge fires once at the correct absolute sample (no snapping, no double-fire, no miss). *(2.2, re-exercised 2.3)*
- [ ] **Probe 5 — MIDI-playable + voices:** host note-on per GM note → correct voice, non-trivial RMS; closed-hat (42) chokes open-hat (46); velocity scales output; high-rate hits finite/bounded + aliasing budget (DSP-06/QUAL-01). *(2.1)*
- [ ] **Probe 6 — viz truth:** emitted `appliedSampleInBar − nominalSampleInBar` == the `appliedOffsetSamples` baked into the emitted `MidiMessage` (QUAL-02 by construction). *(2.3)*
- [ ] Builds clean: VST3 + AU + Standalone (no warnings beyond JUCE's recommended set).
- [ ] `pluginval` passes (VST3 + AU) at repo strictness; `auval` SUCCEEDS after install.
- [ ] `processBlock` allocation-free + lock-free (PERF-01): RNG pre-seeded in `prepareToPlay`; `sequencerMidi.ensureSize` + carry-over `std::array` preallocated; APVTS read once/block via cached `getRawParameterValue()->load()`; audio→UI strictly `AbstractFifo` + atomic. No dropouts (PERF-02).
- [ ] Zero added latency (`setLatencySamples(0)`); `getLatencySamples()` never overridden.
- [ ] No new APVTS params (42-param contract intact); grid stays lock-free atomics, not 384 params; Stage-1 state persistence still passes.
- [ ] No `juce::`-type name collisions (voices suffixed `KickVoice`/…/`TomVoice`; no bare `Voice`).

**Requirements covered:** FUNC-01/02/03/04/06/07, DSP-01/02/03/04/05, DSP-06, PERF-01/02, QUAL-01 (QUAL-02 wired here, surfaced in Stage 3 UI).

## Deviations from the contract (flagged)

- **Fallback A (late-only humanize) adopted for the v1.0 gate** — timing humanize is one-sided positive (`Δhuman_t ∈ [0,+30ms]`); removes the negative-offset cross-block lookahead. Explicitly blessed by ARCHITECTURE Risk §Fallback A + RESEARCH Mechanism Gap #1. Symmetric (±) timing humanize is a documented **post-gate enhancement**. Velocity humanize remains symmetric (±24).
- **Harness CMake diverges from the FM template:** no `*_UIResources` link and no `JUCE_WEB_BROWSER=1` at Stage 2 (no binary-data target yet; editor is a `GenericAudioProcessorEditor` shell — verified no WebView symbols). Revisit in Stage 3.
- **VizEvent sample fields stored raw/unwrapped** (Mechanism Gap #2): UI subtracts for Δt; bar-position display modulo handled Stage-3.
- **bpm read once/block** — sub-block tempo ramps out of scope for v1.0 (accepted simplification, Mechanism Gap #4).

## Out of scope (Stage 3 / 4 / later)

WebView UI, grid/playhead/timing-lane drawing, MIDI readout, tooltips (Stage 3). Factory concept presets (Stage 4, FUNC-05). Per-voice pan, 6-oscillator metallic hat source, 8th-note swing toggle, rimshot/cowbell, symmetric (early) humanize (v1.1+).
