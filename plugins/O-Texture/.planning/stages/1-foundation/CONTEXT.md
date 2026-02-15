# Stage 1: Foundation - Context

## Discussion Summary

**Date:** 2026-02-14
**Participants:** User, Claude

## Requirements Confirmed

- **Full Stage 1 scope:** CMake project with ANIRA/ONNX Runtime linking, model loading infrastructure, all 10 APVTS parameters, and Processor/Editor skeletons
- **10 parameters locked in** as specified in parameter-spec-draft.md (SOURCE, MODE, X, Y, CHARACTER_A, CHARACTER_B, EVOLVE, FREEZE, BRIGHTNESS, MIX)
- **Plugin classified as IS_SYNTH TRUE** (instrument) -- Generate mode creates audio from nothing, Transform mode uses audio input
- **Models embedded as binary resources** via `juce_add_binary_data` -- compiled into plugin binary for simpler distribution
- **Tiny placeholder ONNX models** for development -- match expected I/O shapes (32-dim latent -> 4096-sample audio) to validate full ANIRA pipeline without real ML quality
- **ANIRA via CMake FetchContent** -- downloaded at build time, simplest dependency management

## Constraints Identified

- **Phase 0 (PyTorch training) NOT complete** -- working with dummy/placeholder ONNX models during Stage 1
- **ANIRA is novel for this codebase** -- first ML inference integration, no existing reference
- **Binary size will be large** -- ONNX Runtime static lib (~50MB) + models (~31MB when real) + plugin binary
- **48kHz only** -- models trained at 48kHz, no resampling in v1
- **IS_SYNTH TRUE implications:**
  - DAWs will present it as an instrument (not effect)
  - Transform mode requires audio input routing (sidechain or bus configuration)
  - Must configure input bus even though Generate mode doesn't need it

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Stage 1 scope | Full (CMake + ANIRA + Model Loading + APVTS) | Validates entire build pipeline early, identifies ANIRA integration issues before Stage 2 |
| ANIRA dependency | CMake FetchContent | Simplest setup, auto-downloads at build time, matches ROADMAP suggestion |
| Plugin type | IS_SYNTH TRUE (instrument) | Generate mode creates audio from nothing -- synth classification is more natural for DAW workflows |
| Model storage | Binary resources (juce_add_binary_data) | Models compiled into binary -- simpler distribution, no external file path resolution needed |
| Placeholder models | Tiny ONNX models | Create minimal ONNX models matching expected I/O shapes to validate full inference pipeline |
| Parameters | 10 params as specified | Locked in: SOURCE, MODE, X, Y, CHARACTER_A, CHARACTER_B, EVOLVE, FREEZE, BRIGHTNESS, MIX |

## Stage 1 Sub-tasks

### 1.1: CMake + ANIRA + ONNX Runtime Integration (3-5 days)
- Create `plugins/O-Texture/CMakeLists.txt`
- Add ANIRA via FetchContent (with ONNX Runtime backend)
- Configure `juce_add_plugin()` with IS_SYNTH TRUE, NEEDS_WEBVIEW2 TRUE, WebView2 static linking
- Link ANIRA and ONNX Runtime to plugin target
- Generate tiny placeholder ONNX models (Python script) for encoder/decoder/prior shapes
- Embed placeholder models via `juce_add_binary_data`
- Verify: Plugin compiles, ANIRA links without missing symbols

### 1.2: Model Loading Infrastructure (3-5 days)
- Implement `ModelManager` class:
  - Load ONNX models from binary resources
  - Create ANIRA InferenceHandler sessions (encoder, decoder, prior)
  - Handle model switching when SOURCE parameter changes (lazy loading)
  - Atomic pointer swap when new model ready (thread-safe for audio thread)
- Model loading on message thread (not audio thread)
- "Loading..." state management (for UI feedback when model swaps)
- Error handling: missing model -> fallback to default, invalid ONNX -> log and disable

### 1.3: APVTS Parameter Structure (1-2 days)
- Create APVTS with all 10 parameters:
  - SOURCE (Choice 0-5, default 0): Rain/Metal/Wind/Crowd/Synth/Organic
  - MODE (Choice 0-1, default 0): Generate/Transform
  - X (Float 0.0-1.0, default 0.5): Latent X position
  - Y (Float 0.0-1.0, default 0.5): Latent Y position
  - CHARACTER_A (Float 0.0-1.0, default 0.5): Third latent dimension
  - CHARACTER_B (Float 0.0-1.0, default 0.5): Fourth latent dimension
  - EVOLVE (Float 0.0-1.0, default 0.3): Random walk rate
  - FREEZE (Bool, default Off): Halt evolution
  - BRIGHTNESS (Float -1.0 to 1.0, default 0.0): Tilt EQ
  - MIX (Float 0.0-1.0, default 1.0): Dry/wet blend
- Parameter listeners for SOURCE changes (trigger model reload)
- APVTS persistence verification (save/restore in DAW)

### 1.4: Processor + Editor Skeletons
- PluginProcessor with:
  - Input/output bus configuration (stereo in for Transform, stereo out)
  - Empty processBlock (passes audio through or generates silence)
  - getLatencySamples() reporting ~6144 samples
  - State save/restore via APVTS
- PluginEditor with:
  - Basic WebView setup (empty page or placeholder)
  - Correct window size (800x600)
  - WebView resource provider configuration

## Open Questions

- **ANIRA version pinning:** Which ANIRA git tag/commit to pin to? Need to check latest stable release.
- **ONNX Runtime version:** ANIRA bundles its own ONNX Runtime -- need to verify which version and whether it supports all required ops.
- **Input bus behavior in IS_SYNTH mode:** How DAWs handle audio input routing for synth plugins varies. May need testing across Ableton, Logic, and Reaper.
- **Placeholder ONNX generation:** Need a Python script to generate tiny ONNX models. This requires Python + ONNX/PyTorch installed on dev machine. Confirm availability.

## Next Phase

Ready for: **research** phase (investigate ANIRA CMake integration specifics, ONNX model generation, IS_SYNTH input bus patterns)
