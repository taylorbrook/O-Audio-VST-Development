---
status: issues-found
files_reviewed: 11
depth: standard
date: 2026-07-15
findings:
  critical: 2
  warning: 7
  info: 10
  total: 19
---

# O-Texture v0.1.0 — Code Review Report

**Reviewed:** 2026-07-15
**Depth:** standard
**Files Reviewed:** 11
**Status:** issues-found

Files reviewed:
- `Source/PluginProcessor.cpp` / `.h`
- `Source/PluginEditor.cpp` / `.h`
- `Source/DSP/PerlinNoise1D.h`
- `Source/DSP/TiltFilter.h`
- `Source/DSP/OverlapAddProcessor.h`
- `Source/DSP/HannWindow.h`
- `Source/ui/public/js/main.js`
- `Source/ui/public/index.html`
- `CMakeLists.txt`

## Summary

The overlap-add architecture, WebView relay wiring (member declaration order, resource-provider path matching, no `this`-capturing async callbacks), and Hann/COLA math are sound. However, the core inference path violates real-time safety at the deepest level — synchronous ONNX Runtime calls with per-call heap allocation and logging inside `processBlock` — and the missing bus-layout restriction allows out-of-bounds writes for >stereo output layouts. There are also genuine thread-safety races on the Perlin noise state between the host and audio threads, and the saved evolve state is wiped by `prepareToPlay` in typical host call order, making state persistence ineffective. Several UI controls (SOURCE, MODE) are presented but silently do nothing.

## Critical Issues

### CR-01: Synchronous ONNX inference and logging on the audio thread

**File:** `Source/PluginProcessor.cpp:155-203, 376-408`
**Issue:** `processBlock` calls `runDecoder()` directly, which:
1. Runs `decoderSession->Run(...)` synchronously — neural inference has unbounded, block-size-independent execution time (a full 4096-sample decode per hop) and ONNX Runtime internally allocates memory and may take locks. At small host buffer sizes this will miss the deadline and glitch; on any thread contention it can stall the audio callback.
2. Heap-allocates on every call: `Ort::MemoryInfo::CreateCpu(...)` and both `Ort::Value::CreateTensor(...)` wrappers construct ORT API objects per invocation.
3. On exception, calls `juce::Logger::writeToLog(...)` with `juce::String` concatenation (heap allocation, potential file I/O/lock) on the audio thread (line 200).

Ironically, CMakeLists.txt fetches and links ANIRA v2.0.3 — a library whose entire purpose is RT-safe neural inference (background inference thread + lock-free FIFO handoff) — but the processor bypasses it and calls ORT directly.

**Fix:** Move inference off the audio thread. Either use ANIRA's `InferenceHandler` (already linked), or a dedicated inference thread fed by a lock-free ring: audio thread pushes latent vectors and pops decoded blocks; output silence/last-block repeat when a block isn't ready. At minimum: hoist `Ort::MemoryInfo` to a member created once in `initDecoderSession()`, and remove the `Logger::writeToLog` from the audio-thread catch path (set an atomic error flag instead).

### CR-02: No `isBusesLayoutSupported` override — >2-channel output causes out-of-bounds writes in TiltFilter

**File:** `Source/PluginProcessor.cpp:41-44, 312-334`; `Source/DSP/TiltFilter.h:76-104`
**Issue:** `TextureProcessor` declares a stereo output bus but never overrides `isBusesLayoutSupported()`. The JUCE default returns `true` for any layout, so a host may activate the output bus as 5.1/7.1. `prepareToPlay` hardcodes `tiltFilter.prepare(sampleRate, 2)` (line 319) and `olaProcessor.prepare(2)`, but `TiltFilter::processBlock` iterates `buffer.getNumChannels()` and indexes `lpState[ch]` (line 96) — for any channel ≥ 2 this reads/writes past the end of a 2-element `std::vector` → undefined behavior / crash.
**Fix:** Add:
```cpp
bool isBusesLayoutSupported(const BusesLayout& layouts) const override
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && (layouts.getChannelSet(true, 0).isDisabled()
            || layouts.getChannelSet(true, 0) == juce::AudioChannelSet::stereo());
}
```
Defense-in-depth: clamp the channel loop in `TiltFilter::processBlock` to `std::min<size_t>(numChannels, lpState.size())`, and pass the real channel count in `prepareToPlay`.

## Warnings

### WR-01: Data races on PerlinNoise1D state between host thread and audio thread

**File:** `Source/PluginProcessor.cpp:429-479`; `Source/DSP/PerlinNoise1D.h:28-79`
**Issue:** `setStateInformation` (host/message thread) calls `evolveNoise.setSeed()` (rebuilds the 512-byte `perm` table, lines 461, 126-143 of PerlinNoise1D.h) and `evolveNoise.setCursors()` while the audio thread may concurrently be inside `constructLatentVectors` → `evolveNoise.advance()` / `getAllValues()`, which reads `perm`, and reads/writes `cursors`/`cachedValues`. `getStateInformation` likewise reads `getCursors()` and `getSeed()` unsynchronized against audio-thread writes. This is a data race (UB): torn permutation table mid-hop and torn cursor floats.
**Fix:** Stage restored state through atomics: store the pending seed/cursors in `std::atomic` slots (or a small double-buffered struct with an `std::atomic<int>` sequence/flag) set from `setStateInformation`, and apply them at the top of `processBlock` before the hop decode. Snapshot cursors for `getStateInformation` the same way (audio thread publishes to an atomic snapshot each hop).

### WR-02: Restored evolve cursors/seed are wiped by `prepareToPlay`

**File:** `Source/PluginProcessor.cpp:323-324, 458-477`
**Issue:** `setStateInformation` carefully restores `evolve_seed` and `evolve_cursors`, but `prepareToPlay` unconditionally calls `evolveNoise.reset()` (zeroing all cursors) and reseeds `inactiveRng` from wall-clock time. In the typical host sequence (setState during project load, prepareToPlay when audio starts) the restored evolution position is discarded, so the persisted state round-trip is dead on arrival — and every sample-rate/buffer reconfiguration also resets evolution mid-session.
**Fix:** Remove `evolveNoise.reset()` from `prepareToPlay` (or guard it with a `stateRestored` flag so a restore survives the next prepare). The Perlin state is sample-rate independent (hop-domain), so there is nothing to recompute there.

### WR-03: Inactive latent dims re-randomized every hop — FREEZE doesn't freeze, sound not reproducible

**File:** `Source/PluginProcessor.cpp:243-247`
**Issue:** `constructLatentVectors` assigns `inactiveRng.nextFloat() * 0.6f - 0.3f` to every inactive dim on every hop (~every 43 ms), including when `freeze == true`. Consequences: (1) FREEZE only freezes the evolve dims — the texture continues fluttering from inactive-dim jitter; (2) `inactiveRng` is reseeded from `currentTimeMillis()` in `prepareToPlay`, so the same project never reproduces the same output; (3) the per-hop discontinuity in 20 latent dims adds unintended timbral churn even at EVOLVE = 0.
**Fix:** Draw the inactive-dim values once (per seed / per prepare), store them in a member `std::array<float, 32>`, and reuse them each hop. If slow drift is desired, route them through additional `PerlinNoise1D` channels gated by `freeze` like the evolve dims.

### WR-04: Latency reported as 6144 samples, but the generator has no algorithmic latency

**File:** `Source/PluginProcessor.cpp:321`; `Source/DSP/OverlapAddProcessor.h:91`
**Issue:** `setLatencySamples(OverlapAddProcessor::ACCUM_SIZE)` reports 6144 samples (~128 ms @ 48 kHz). But the OLA path outputs decoded audio starting at sample 0 of the first hop — there is no input-to-output delay in Generate mode (the plugin is a source; only a one-block window fade-in exists). Hosts will apply 6144 samples of PDC, shifting this track ~128 ms relative to others in bounces and MIDI-triggered alignment.
**Fix:** Report `setLatencySamples(0)` for the current Generate-only implementation. If/when Transform mode (input → encoder → decoder) lands, report the actual analysis/synthesis delay of that path and update it when the mode switches.

### WR-05: `M_PI` in HannWindow.h breaks MSVC builds

**File:** `Source/DSP/HannWindow.h:22`
**Issue:** `M_PI` is not defined by `<cmath>` on MSVC unless `_USE_MATH_DEFINES` is set before inclusion. This header only includes `<array>` and `<cmath>`, so the Windows build (which this plugin targets — `NEEDS_WEBVIEW2 TRUE`, static WebView2 linking flags are configured) fails to compile.
**Fix:** Replace with a literal or JUCE constant:
```cpp
constexpr float twoPi = 6.283185307179586f; // or juce::MathConstants<float>::twoPi
```

### WR-06: Double-click reset hardcodes 0.5 — wrong default for MIX and EVOLVE

**File:** `Source/ui/public/js/main.js:267-271, 375-379`
**Issue:** Both the slider and knob `dblclick` handlers call `setNormalisedValue(0.5)` as "reset to default". The C++ defaults are MIX = 1.0 and EVOLVE = 0.3 (PluginProcessor.cpp:292, 303), so double-clicking Mix halves the output level and double-clicking Evolve jumps to the wrong rate. Only Brightness/X/Y/Char A/Char B happen to default to 0.5 normalized. This is the known hardcoded-JS-defaults drift pattern (see O-MicrotonalSampler v1.23.7).
**Fix:** Pass the correct default per control (e.g., `bindKnob('mix', mixState, 1.0)`), or expose parameter defaults from C++ (native function or `properties` payload) and reset to those.

### WR-07: SOURCE and MODE parameters have no effect — six source buttons and a Generate/Transform toggle are dead controls

**File:** `Source/PluginProcessor.cpp:47-48, 347-414`; `Source/ui/public/index.html:14-17, 57-110`
**Issue:** `sourceParam` and `modeParam` are cached in the constructor but never read anywhere in the audio path. The UI presents six source models (Rain/Metal/Wind/Crowd/Synth/Organic) and a Generate|Transform mode toggle, all fully wired through relays and attachments — yet only the rain dim_map/decoder is ever loaded and only generation is implemented. Users (and host automation) can change these parameters with zero audible effect and no indication anything is missing. The declared "Sidechain" input bus for Transform is likewise unused.
**Fix:** For v0.1.0 either (a) disable/grey-out the unimplemented sources and Transform in the UI (single source button active, remove the mode toggle) and drop the unused parameters, or (b) wire SOURCE to select among the embedded models. Shipping automatable parameters that do nothing bakes them into the plugin's parameter contract (IDs are versioned) — better to remove them before first release than to strand them.

## Info

### IN-01: `decoderReady` is written but never read

**File:** `Source/PluginProcessor.cpp:390, 328`; `Source/PluginProcessor.h:109`
**Issue:** Dead state flag — set to `true` after a successful decode, reset in `prepareToPlay`, never consulted.
**Fix:** Remove, or use it to gate output during decoder warm-up.

### IN-02: Redundant copy in `runDecoder`

**File:** `Source/PluginProcessor.cpp:176-195`
**Issue:** The output tensor is bound to `decoderOutputBuffer`, then the result is `memcpy`d to `outputAudio`. Binding the tensor directly to `outputAudio` (the caller's `decodedBufferL/R`) eliminates the intermediate buffer and the 16 KB copy per hop.
**Fix:** `CreateTensor<float>(memoryInfo, outputAudio, kBlockSize, ...)` and delete `decoderOutputBuffer`.

### IN-03: Unused JS import and variable

**File:** `Source/ui/public/js/main.js:8, 232`
**Issue:** `getBackendResourceAddress` is imported but never used; `thumb` in `bindVerticalSlider` is queried but never used (the separate `updateVerticalSlider` re-queries it).
**Fix:** Remove both.

### IN-04: CMake hardcodes the macOS-arm64 ONNX Runtime dylib path

**File:** `CMakeLists.txt:152, 166-168`
**Issue:** `onnxruntime-1.19.2-macOS-arm64` is baked into the post-build copy and `install_name_tool` steps. An x86_64 or universal macOS build will silently copy the wrong (or missing) dylib.
**Fix:** Derive the path from ANIRA's resolved ONNX Runtime target/variable, or branch on `CMAKE_OSX_ARCHITECTURES`.

### IN-05: ANIRA is fetched and linked but its inference engine is unused

**File:** `CMakeLists.txt:8-17, 61`; `Source/PluginProcessor.cpp:122-153`
**Issue:** The full ANIRA dependency (FetchContent, shared-lib embedding, C++20 requirement) is pulled in solely to obtain ONNX Runtime headers/libs, while inference goes through raw `Ort::Session`. This is both dead weight and the missed fix for CR-01.
**Fix:** Either adopt ANIRA's `InferenceHandler` for the decode path (resolves CR-01), or depend on ONNX Runtime directly and drop ANIRA.

### IN-06: `encoder.onnx` and `prior.onnx` are embedded but never loaded

**File:** `CMakeLists.txt:86-94`; `Source/PluginProcessor.cpp:122-131`
**Issue:** Only `decoder_onnx` and the dim map are read from `ModelData`. The encoder and prior models inflate the binary for no runtime benefit in v0.1.0.
**Fix:** Remove from the binary-data target until Transform mode needs them.

### IN-07: Plugin declares MIDI input but ignores all MIDI

**File:** `Source/PluginProcessor.h:34`; `CMakeLists.txt:32`; `Source/PluginProcessor.cpp:347`
**Issue:** `acceptsMidi() = true` and `NEEDS_MIDI_INPUT TRUE`, but `processBlock` discards `midiMessages`. Hosts will route MIDI to the plugin (and some classify it as an instrument requiring note input) with no effect.
**Fix:** If MIDI triggering is planned, keep and document; otherwise set both to false.

### IN-08: Dead fallback branch in the processBlock read loop

**File:** `Source/PluginProcessor.cpp:399-400`
**Issue:** `getSamplesUntilNextHop()` returns `HOP_SIZE - readPosition` with `readPosition ∈ [0, HOP_SIZE)`, so it is always ≥ 1; the `samplesToRead <= 0` fallback is unreachable — and if it ever did execute it would read across a hop boundary without decoding a fresh block.
**Fix:** Remove the branch, or replace with `jassert(samplesToRead > 0)`.

### IN-09: Perlin cursors grow unboundedly — evolve rate degrades in very long sessions

**File:** `Source/DSP/PerlinNoise1D.h:59-63, 120-124`
**Issue:** `cursors[ch] += stepPerHop` forever. Once a cursor exceeds ~2048–16384 (days of playback at low EVOLVE), float ULP approaches/exceeds small step sizes and the noise quantizes, then stalls. Since `hashAt` masks position with `& 0xFF`, the noise is periodic with period 256 — the cursor can be wrapped losslessly.
**Fix:** After incrementing: `if (cursors[ch] >= 256.0f) cursors[ch] -= 256.0f;`

### IN-10: Canvas sized once at DOMContentLoaded; no resize/late-stylesheet handling

**File:** `Source/ui/public/js/main.js:30-34`
**Issue:** `canvas.width/height` are computed from `container.clientWidth/Height` at DOMContentLoaded, which can fire before the external stylesheet finishes applying in a cold WebView, yielding a mis-sized backing store; there is also no `ResizeObserver`, so any future resizable-editor work will render a stretched canvas (canvas is a CSS replaced element — the known O-TextureForge gotcha).
**Fix:** Size the canvas in a `ResizeObserver` on the container (also covers the initial layout settling), and consider DPR-aware backing (`clientWidth * devicePixelRatio`).

---

_Reviewed: 2026-07-15_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
