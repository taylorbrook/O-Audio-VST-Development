# Stage 2 (DSP) — SUMMARY

**Plugin:** O-simpleGrain · **Stage:** 2 of 4 (DSP) · **Run mode:** express
**Phases:** 2.1 ✅ · 2.2 ✅ · 2.3 ✅ (this file appends per sub-phase)

---

## Phase 2.1 — Core grain engine + overlap-add + window LUTs + amp ADSR + key resample ✅

**Execute agent:** dsp-agent · **Build:** clean (VST3+AU+Standalone) · **auval:** SUCCEEDED · **Date:** 2026-06-24

### Files created
- `Source/dsp/LagrangeInterpolation.h` — 4-pt stateless `lagrangeInterpolate` (copied verbatim from O-GrainScatter).
- `Source/dsp/WindowLuts.h` — `WindowLuts`: five precomputed 2048-pt envelope tables (rect/tri/Welch/Gauss/Hann), `read(shape,phase)` linear-interp; no per-sample transcendental.
- `Source/dsp/Grain.h` — `Grain` POD (forward-phase model: active/readPos/rate/phase/phaseInc/lengthSamples/pan/shape/age/aaState).
- `Source/GrainSound.h` — `GrainSound : juce::SynthesiserSound`.
- `Source/GrainVoice.h` — preallocated `std::array<Grain,24>` pool + steal-oldest spawn + per-sample density scheduler + overlap-add render loop + key-tracked resample + amp ADSR + non-virtual `prepareToPlay` + `GrainVoiceParams` block-push struct.

### Files modified
- `Source/PluginProcessor.h` — `WindowLuts`, `juce::Synthesiser synth`, atomic `shared_ptr<AudioBuffer<float>> currentSource` (+atomicLoad/Store helpers), `outputGain` SmoothedValue, `positionAbsolute`, `loadDefaultSource()` decl.
- `Source/PluginProcessor.cpp` — ctor adds 8 voices + 1 sound + note-stealing, hands each voice the LUT; `prepareToPlay` dispatches voice prepare via `dynamic_cast`, resets gain, decodes default `fire.wav` (resampled to engine rate, capped 10 s, atomic-published); full `processBlock` (snapshot source, push params/source/playhead, `synth.renderNextBlock`, output gain + fixed 0.5 headroom, `isfinite` scrub). `setLatencySamples(0)` kept.
- `CMakeLists.txt` — 5 new headers added to `target_sources` (no `juce_add_binary_data` yet — Phase 2.3 TODO intact).

### Verification
- **Build:** VST3 + AU + Standalone link clean, no errors/warnings (beyond benign repo-wide BUNDLE_ID-spaces note).
- **auval:** `aumu OsGr OuDv` → **AU VALIDATION SUCCEEDED** (render tests at 22050/44100/48000/96000/192000 Hz × frame sizes 64–4096; 1-channel; bad-max-frames; parameter set/ramp; **Test MIDI PASS**).
- **RT-safety:** allocation/lock/IO-free render path (preallocated pool, steal-oldest, LUTs at construction, lock-free atomic source snapshot, ScopedNoDenormals + isfinite, per-voice `juce::Random`). Zero added latency.
- **Manual-listen (deferred to stage-end DAW pass):** separated→fused grains, buzz↔fragments, rect-click, MIDI transpose.

### Seams pre-wired for 2.2/2.3 (no hot-loop re-touch)
1. Atomic `currentSource` publish (2.3 swaps byte source only). 2. `setParams(GrainVoiceParams)` struct (2.2 adds spray/scatter fields). 3. Abstract per-block playhead value the voice reads at spawn (2.2 promotes to moving/freezable). 4. `aaOnePole` no-op pass-through at the call site (2.2 fills body). 5. Per-voice `juce::Random rng` member present.

### Deviations / notes
- `WindowLuts` uses a `std::vector<float>` sized once in its ctor (runtime `kWindowLutSize`) instead of `std::array<float,2048>` — functionally identical, allocation-free on the audio thread.
- Mono source read in 2.1 (default `fire.wav` is mono); stereo handling lands with 2.3 multi-source.
- Headroom is a fixed 0.5 factor (ARCHITECTURE's "fixed headroom factor" option), not overlap-aware normalization (acceptable for v1.0).
- `loadDefaultSource` locates `fire.wav` via a `__FILE__`-relative path (throwaway 2.1 path; replaced by `BinaryData` embed in 2.3). Resolves in local CMake dev builds.
- `std::atomic_load/store` on `shared_ptr` are C++20-deprecated (compile fine) — established O-MicrotonalSampler pattern.

---

## Phase 2.2 — Read head (scan / time-stretch / freeze) + spray & scatter + anti-aliasing + velToDensity ✅

**Execute agent:** dsp-agent · **Build:** ✅ clean VST3+AU+Standalone · **auval:** ✅ SUCCEEDED · **Date:** 2026-06-24
**Tasks:** T5 (global read head) · T6 (freeze) · T7 (spray/scatter + velToDensity) · T8 (AA one-pole). Deps held: `T5 → {T6,T7}; T7 → T8`. No new files. Phase 2.1 code preserved (only extended).

### Files modified
- `Source/PluginProcessor.h` — promoted the static `positionAbsolute` resting point to a **moving/freezable global read head**: `double playheadPos` + three `SmoothedValue<float>` (`scanSmoothed`, `positionSmoothed`, `playheadVelocity`). Removed the dead `positionAbsolute` member.
- `Source/PluginProcessor.cpp` — `prepareToPlay`: reset the 3 new SmoothedValues (~20 ms) and seed `playheadPos` at `position% × srcLen` after the source decodes. `processBlock`: read `scan`/`freeze`/`positionSpray`/`pitchSpray`/`scatter`/`panSpray`/`velToDensity` atomics into `GrainVoiceParams`; advance the global playhead **per sample** (freeze-pinnable velocity, rest-ease toward the resting point when scan≈0, bidirectional wrap); push the **block-start** playhead to voices via the existing `setPlayhead` seam (voice spawn signature unchanged).
- `Source/GrainVoice.h` — extended `GrainVoiceParams` with the 5 spray/scatter/velToDensity fields; `velToDensity` folded into `effectiveDensity`; `scatter` jitters the scheduler period per spawn (RNG); `spawnGrain` adds position spray (read-start), pitch spray (rate), pan spray (equal-power pan); filled the `aaOnePole` body.

### What each task added
- **T5 — global read head.** Processor advances `playheadPos += playheadVelocity.getNextValue()` per sample; velocity = `scan/100` (realtime-relative; ±200% range → ±2 samples/sample). `position` sets the resting point the playhead eases toward when `|velocity| < 1e-4` (Position knob stays live). Bidirectional `[0,srcLen)` wrap (negative scan = reverse). `SmoothedValue` (~20 ms) on scan/position/velocity, reset in `prepareToPlay`. Per ARCHITECTURE §Read head / RESEARCH §4.1.
- **T6 — freeze.** `freeze = freezeParam->load() > 0.5f`; engage targets `playheadVelocity → 0`, disengage ramps back to the scan velocity — **via the velocity `SmoothedValue` ramp** (RESEARCH §4.2 "simplest robust approach"), so engage/disengage is click-free and the playhead is **never hard-jumped** (QUAL-01). Held note sustains the frozen instant (voice alive in amp sustain; grains keep reading the pinned region ± position spray → a frozen pad). Freeze overrides scan (pins) per the freeze×scan interaction.
- **T7 — spray & scatter + velToDensity.** All via the per-voice `juce::Random rng` (no alloc/lock). Position spray: `readPos = playheadPos + U(−1,1)·positionSpray%·srcLen`. Pitch spray: `rate = voiceRate·2^((grainPitch + U(−1,1)·pitchSpray)/12)`. Pan spray: `pan = clamp(0.5 + U(−1,1)·panSpray%·0.5, 0,1)`. Scatter: `nextInterval = max(1, (int)(baseInterval + scatter%·baseInterval·U(−1,1)))`. velToDensity: `effectiveDensity = clamp(density·(1 + velToDensity·(velLevel−0.5)·2), 1, 200)` → `baseInterval = fs/effectiveDensity`.
- **T8 — AA one-pole.** Filled the 2.1 no-op: bypass at `rate ≤ 1` (keeps `state = x` coherent for the bypass→engage edge); else `fc = 0.5·fs/rate`, `g = 1 − exp(−2π·fc/fs)`, `state += g·(x − state)`. Inserted between `readSourceLagrange` and the envelope multiply (existing call site). `aaState` primed to the **first read sample** on spawn (settled start, no attack transient).

### Deviations / notes
- **Freeze mechanic:** used the `SmoothedValue<float>` velocity ramp (RESEARCH §4.2's explicitly-recommended "simplest robust approach") rather than the FreezeManager crossfade-gain — fewer moving parts, same click-free guarantee for a static-source playhead. No crossfade buffer needed.
- **Position + scan both live:** added a gentle per-sample ease (×0.0008/sample) toward the smoothed resting point when scan≈0 and not frozen, so the Position knob remains responsive between note re-triggers without a hard jump. (ARCHITECTURE: "`position` sets the playhead's resting point; `scan` is its velocity.")
- **Block-start playhead snapshot:** voices read a single per-block `setPlayhead` value (Sequencing Note 3 / PLAN T5 "push per block"); the processor still advances the true playhead per sample for read-head coherence + the Stage-3 viz playhead line. Block-granular spawn position is musically correct for granular (short, position-sprayed grains).
- **`aaState` primed to first sample** (not 0) — RESEARCH §5 permits either; first-sample priming avoids a spurious transient on up-transposed grains.
- No new files; `CMakeLists.txt` unchanged (Phase 2.3 `juce_add_binary_data` TODO intact).

### Success criteria: code-complete vs. manual-listen
- **Code-complete (this phase):** scan velocity + position resting point + bidirectional wrap (T5); freeze pin + click-free SmoothedValue ramp + held-note sustain (T6); per-voice position/pitch/pan spray + scatter period jitter + velToDensity (T7); rate-tracking AA one-pole, bypass at rate≤1 (T8); `SmoothedValue` on scan/position/velocity, no hard playhead jump, per-voice `juce::Random` only (no alloc/lock), `setLatencySamples(0)` retained.
- **Manual-listen (verify phase / DAW):** scan fwd/slow/reverse/×2; freeze pins click-free (DSP-06/FUNC-03/QUAL-01); pitch + position spray shimmer, no two grains identical (DSP-04); scatter 0%→sync (sidebands) vs high→async (noise) (DSP-05); high pitch-spray grains stay clean, no buzz (DSP-08); no zipper on scan/position automation (QUAL-01).
- **AUTO build gate (orchestrator):** clean VST3+AU+Standalone; AU registers (`auval`); pluginval ≥8; `setLatencySamples(0)`.

---

## Phase 2.3 — Sample loading (embed 4 + drag-drop + hot-swap) + viz taps ✅

**Execute agent:** dsp-agent · **Build:** ✅ clean VST3+AU+Standalone (CMake reconfigured for the binary-data target; bundle 4.7M→6.1M w/ embedded WAVs) · **auval:** ✅ SUCCEEDED · **Date:** 2026-06-24
**Tasks:** T9 (embed 4 + decode/resample + atomic hot-swap on `sourceSample`) · T10 (load-your-own: drag-drop streaming C++ handlers + Base64 + picker + 10 s cap) · T11 (three lock-free viz taps). Deps held: `T9 → {T10, T11}`. **Phase 2.1/2.2 code preserved — only extended** (grain read stays static-source forward-phase; the atomic `currentSource` publish seam from 2.1 swaps only the *byte source*, not the engine plumbing).

### Files created
- `Source/dsp/TripleBuffer.h` — **copied VERBATIM** from O-GrainScatter (lock-free SPSC triple buffer; `getWriteBuffer()`/`publish()`/`read()` API unchanged).
- `Source/VizAnalyzer.h` — **copied from** O-simpleFM `FmVizAnalyzer.h`; ring + FFT logic UNCHANGED, analyzer type renamed `FmVizAnalyzer → GrainVizAnalyzer` (all behavior identical). `VizRing` (audio-thread write / message-thread read) + `GrainVizAnalyzer` (message-thread 4096/Blackman-Harris FFT). **No FFT on the audio thread** — the audio thread only writes samples to the ring; the FFT is the Stage-3 editor Timer's job.
- `Source/dsp/GrainCloudFrame.h` — fixed-size POD `GrainEvent {readPosNorm, sizeMs, pitchSemis, pan, spawnSample}` + `GrainCloudFrame {events[256], count, playheadNorm, positionNorm, positionSprayNorm, frozen}`. No heap, no dynamic growth.

### Files modified
- `CMakeLists.txt` — added `juce_add_binary_data(O-simpleGrain_Samples NAMESPACE BinaryData HEADER_NAME BinaryData.h SOURCES Source/samples/{fire,voice,water,piano}.wav)` **AFTER** `juce_generate_juce_header` + `target_link_libraries(O-simpleGrain PRIVATE O-simpleGrain_Samples)`. Added the 3 new headers to `target_sources`. Removed the Stage-1 `# TODO(Stage 2.3)` marker.
- `Source/PluginProcessor.h` — now also inherits `private juce::AudioProcessorValueTreeState::Listener, private juce::AsyncUpdater`. Added: viz taps (`VizRing vizRing`, `TripleBuffer<GrainCloudFrame> grainCloudBuffer`, `std::atomic<int> activeGrainCount`); drag-drop session state (`dropSessionId/FolderName/AccumBase64/AccumFilename`); `std::unique_ptr<juce::FileChooser> fileChooser`; `lastLoadTruncated`/`pendingBuiltInIndex`/`suppressChoiceRebuild`; built-in name table; public accessors `getVizRing()/getGrainCloudBuffer()/getActiveGrainCount()/getCurrentSampleRate()/wasLastLoadTruncated()`; the 4 drag-drop handler decls + `loadSourceFromFileChooser()`; private decode helpers (`loadBuiltInSource`, `decodeAndPublish`, `resampleToEngineRate`, `builtInIndexForIdentity`, `rebuildSourceFromChoice`, `endDropSession`) + the listener/AsyncUpdater overrides. Removed `loadDefaultSource` decl.
- `Source/PluginProcessor.cpp` — `#include "BinaryData.h"`. Ctor registers the `sourceSample` parameter listener (dtor removes it + `cancelPendingUpdate()`). Replaced the throwaway 2.1 `__FILE__`-relative `loadDefaultSource` with the full Stage-2.3 decode suite (embedded `BinaryData` path + shared `decodeAndPublish`/`resampleToEngineRate`). `prepareToPlay` re-decodes the active source (built-in or restored user file) at the engine rate (R8) + zeroes `activeGrainCount`. `processBlock` fills the grain-cloud frame (reset `count=0`, wire voices, set read-head fields, `publish()`) and writes the post-gain mono sum to `vizRing` (copy-only, ≤4096-chunk stack mono, no alloc, no FFT). `setStateInformation` re-decodes the restored source (with the user-file/built-in fallback + `suppressChoiceRebuild` race guard).
- `Source/GrainVoice.h` — `#include "dsp/GrainCloudFrame.h"`; added per-block viz seams `setGrainCloudFrame()/setActiveGrainCount()`; `spawnGrain` gained a `spawnSample` arg and appends a bounded `GrainEvent` (drops extras at `kMaxEvents`, never grows); added `publishActiveCount()` (scans the pool O(24) → applies the delta to the shared atomic — robust vs. steal-oldest/grain-done/note-clear) called at render tail / hard-stop / inactive-drop; `prepareToPlay` forgets the count contribution.

### What each task added
- **T9 — embed 4 + decode/resample + atomic hot-swap.** CMake binary-data target → `BinaryData::fire_wav`/`fire_wavSize` etc. (filename `fire.wav` → identifier `fire_wav`; index order fire/voice/water/piano matches the `sourceSample` choice). `decodeAndPublish(data, bytes, engineRate, identity)` = `createReaderFor(MemoryInputStream(blob, size, false))` → `reader->read(&tmp,…)` (1–2 channels) → `resampleToEngineRate` (`juce::LagrangeInterpolator::process(srcRate/engineRate, in, out, numOut)` per channel, cap `kMaxSourceSeconds·engineRate`) → `atomicStore(currentSource, …)` + set `currentSourceIdentity = "embedded:<name>"`. `sourceSample` change → `parameterChanged` sets `pendingBuiltInIndex` + `triggerAsyncUpdate()` → `handleAsyncUpdate` decodes/publishes **on the message thread** (NEVER the audio thread).
- **T10 — load-your-own.** Registered the C++ methods for the shared `webview-drop-streaming.js` bridge (names below). Base64 decoded with **`juce::Base64::convertFromBase64(MemoryOutputStream&, StringRef)` ONLY** (R7/§9.2). Single-file commit decodes→resamples→publishes; identity = `"dropped:<filename>"`. `loadSourceFromFileChooser()` = async `juce::FileChooser` (message thread) → same decode path; identity = full path (re-readable on restore). Files > `kMaxSourceSeconds·engineRate` truncated; `wasLastLoadTruncated()` flag for the Stage-3 notice. `currentSourceIdentity` persists via the existing Stage-1 custom ValueTree state; on restore a missing path falls back to the default built-in.
- **T11 — three viz taps.** (1) **VizRing (samples):** post-gain mono sum at the `processBlock` tail (mirrors O-simpleFM). (2) **TripleBuffer<GrainCloudFrame> (grain events):** processor owns one frame/block, `count=0` reset, each voice appends a `GrainEvent` at spawn (bounded 256), processor sets `playheadNorm/positionNorm/positionSprayNorm/frozen`, `publish()` once/block. (3) **`std::atomic<int> activeGrainCount`:** per-voice pool scan delta (robust). Public accessors mirror O-simpleFM.

### NativeFunction handler names registered (for Stage 3's reference)
The C++ methods exist on the processor; **Stage 3 wires them into the WebView via `WebBrowserComponent::Options::withNativeFunction(name, …)`** (the WebBrowserComponent itself is created in Stage 3 — NOT instantiated here). Names are FIXED by `webview-drop-streaming.js` — do NOT rename:
| NativeFunction name | Processor method | Role |
|---------------------|------------------|------|
| `dropSessionStart` | `dropSessionStart(sessionId[, folderName])` → bool | open a session |
| `dropSessionAddFile` | `dropSessionAddFile(sessionId, filename, base64)` → bool | stream the file's base64 (accumulated) |
| `dropSessionCommitFile` | `dropSessionCommitFile(sessionId, filename, base64)` → bool | decode+resample+publish the single source |
| `dropSessionCommitFolder` | `dropSessionCommitFolder(sessionId)` → bool (false) | accepted no-op (single source — keeps the shared JS binding clean) |

Plus a direct (non-drag-drop) entry point Stage 3 calls from a **"Load…" button**: `loadSourceFromFileChooser()` (async picker). `dropSessionCommitFile` tolerates the base64 arriving on `AddFile`, on `Commit`, or both (falls back to the accumulated payload) so Stage 3's JS adaptation binds either way.

### Deviations / notes
- **`activeGrainCount` is a per-voice pool-scan delta**, not raw inc/dec. PLAN T11 said "increment on spawn, decrement when finished" — pure inc/dec is fragile under steal-oldest (a stolen-then-respawned slot stays active, no net change) and note-clears. Scanning the bounded 24-slot pool once per block (O(24)/voice, trivial) and `fetch_add(live − myLast)` is always exact. Same observable readout, robust.
- **Hot-swap dispatch via AsyncUpdater** (not a flag polled in `processBlock`): the parameter listener may fire off the message thread; `triggerAsyncUpdate()` guarantees the decode runs on the message thread (RT-safe; the audio thread never decodes). `pendingBuiltInIndex` is the carried selection.
- **State-restore race guard:** `replaceState()` fires the `sourceSample` listener; `suppressChoiceRebuild` prevents the AsyncUpdater clobbering a restored user/dropped file with the built-in choice. `setStateInformation` re-decodes the active source itself (a host can restore state after `prepareToPlay`).
- **Drag-drop is C++-only this stage** (Sequencing Note 5): the WebBrowserComponent + JS surface land in Stage 3.1. No `WebBrowserComponent` instantiated here.
- **Single-source drag-drop** uses the single-file path (`Start → AddFile → CommitFile`); `dropSessionCommitFolder` is a clean no-op so the shared multi-file module still binds.
- **Stereo handling:** `decodeAndPublish` decodes 1–2 channels generally; the engine read still snapshots channel 0 (the 2.1/2.2 mono read) — the generated built-ins are mono. A full stereo grain read is a v1.x refinement (out of scope; ARCHITECTURE keeps the read mono for the lesson).
- **`std::atomic_load/store` on `shared_ptr`** remain C++20-deprecated-but-compiling (established O-MicrotonalSampler pattern; unchanged from 2.1).
- No change to `setLatencySamples(0)`, `ScopedNoDenormals`, `isfinite`, per-voice `juce::Random`, the bounded pool, or the grain read addressing (R1 held).

### Success criteria: code-complete vs. manual-listen / Stage-3-dependent
- **Code-complete (this phase):** all 4 built-ins embedded + decoded/resampled/published; `sourceSample` hot-swap off-thread via AsyncUpdater (atomic publish, no audio-thread access to a half-loaded buffer); drag-drop C++ handlers + `convertFromBase64` decode + picker + 10 s truncation; `currentSourceIdentity` round-trips (built-in by name, user file by path, dropped by name with fallback); three lock-free taps fill without locks, FFT off the audio thread, `processBlock` allocation-free; `setLatencySamples(0)` retained.
- **Manual-listen (verify phase / DAW):** all built-ins granulate ("fire" worked example); a dropped `.wav` + the picker load and granulate; oversized truncated to 10 s; source swap glitch-free while holding a note (FUNC-04/05).
- **Stage-3-dependent (UI wiring):** the four visuals (cloud scatter UI-01, waveform playheads UI-02, output scope/spectrum UI-04, grain-count/CPU readout UI-05) consume `getGrainCloudBuffer()`/`getVizRing()`/`getActiveGrainCount()`; the drag-drop JS + "Load…" button + WebBrowserComponent + truncation/missing-file notices are wired in Stage 3.1.
- **AUTO build gate (orchestrator):** clean VST3+AU+Standalone (incl. the new binary-data target); AU registers (`auval`); pluginval ≥8; `setLatencySamples(0)`.

---

**Stage 2 (DSP) is functionally complete** — engine + read head + spray/scatter + AA + sample loading + viz taps all in place. Next: Stage 3 (GUI) consumes the taps + wires the drag-drop/picker/source-select UI.
