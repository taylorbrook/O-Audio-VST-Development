# Stage 1: Foundation - Summary

**Date:** 2026-02-14
**Phase:** execute (complete)
**Duration:** Single session

---

## What Was Built

### CMakeLists.txt with ANIRA FetchContent
- ANIRA v2.0.3 integrated via CMake FetchContent
- ONNX Runtime 1.19.2 bundled automatically (ANIRA dependency)
- LibTorch and TFLite backends disabled
- Plugin target: `OuariconTexture` (code: OuTx)
- IS_SYNTH TRUE, NEEDS_MIDI_INPUT TRUE, NEEDS_WEBVIEW2 TRUE
- Two binary data targets: `ModelData` (ONNX models) + `UIResources` (WebView HTML)
- macOS post-build step embeds ANIRA + ONNX Runtime shared libraries in plugin bundle Frameworks/
- Versioned symlinks created for dynamic linker resolution

### PluginProcessor (TextureProcessor)
- 10 APVTS parameters with cached `std::atomic<float>*` pointers:
  - SOURCE (Choice: Rain/Metal/Wind/Crowd/Synth/Organic)
  - MODE (Choice: Generate/Transform)
  - X, Y (Float 0.0-1.0)
  - CHARACTER_A, CHARACTER_B (Float 0.0-1.0)
  - EVOLVE (Float 0.0-1.0, default 0.3)
  - FREEZE (Bool)
  - BRIGHTNESS (Float -1.0 to 1.0)
  - MIX (Float 0.0-1.0, default 1.0)
- Bus config: stereo output enabled, sidechain input disabled (future Transform mode)
- Latency: 6144 samples via `setLatencySamples()` in prepareToPlay
- processBlock: `ScopedNoDenormals`, clears buffer (silence for Stage 1)
- State save/restore via APVTS copyState/replaceState

### PluginEditor (TextureEditor)
- WebView with `webview2` backend
- Resource provider serving index.html from binary data
- Windows WebView2 user data folder: `OTexture_WebView` (temp directory)
- Native integration enabled
- Window size: 800x600
- Dark background (#1a1a2e)
- Member order: Relays -> WebView -> Attachments (prepared for Stage 3)

### Placeholder WebView
- Minimal dark-themed HTML page
- Lists all 10 parameters with default values
- "Stage 1 - Foundation" subtitle

### Binary Data
- 3 placeholder ONNX models embedded (decoder 129KB, encoder 131KB, prior 6.5KB)
- WebView index.html embedded

### Shared Library Distribution
- ANIRA (libanira.2.0.3.dylib, 383KB) embedded in plugin bundle
- ONNX Runtime (libonnxruntime.1.19.2.dylib, 55MB) embedded in plugin bundle
- Versioned symlinks (libanira.2.dylib, libanira.dylib, libonnxruntime.dylib)
- rpath set to `@loader_path/../Frameworks`
- anira's own onnxruntime reference patched via install_name_tool

---

## Build Verification

- CMake configure: OK (ANIRA + ONNX Runtime downloaded and linked)
- Ninja build: OK (VST3 + AU compiled without errors)
- ANIRA linking: OK (no missing symbols)
- Binary data: OK (ModelData and BinaryData namespaces accessible)
- Shared library embedding: OK (Frameworks/ directory populated with symlinks)
- AU registration: `aumu OuTx OuDv - Ouaricon Audio Development: O-Texture-dev`

---

## Files Created

| File | Description |
|------|-------------|
| `CMakeLists.txt` | Full CMake config with ANIRA FetchContent + library embedding |
| `Source/PluginProcessor.h` | Processor header with APVTS, 10 cached params |
| `Source/PluginProcessor.cpp` | Processor impl, empty processBlock, state save/restore |
| `Source/PluginEditor.h` | Editor header with WebView |
| `Source/PluginEditor.cpp` | Editor impl with resource provider |
| `Source/ui/public/index.html` | Placeholder WebView page |

---

## Issues Encountered

1. **`getLatencySamples()` is non-virtual in JUCE 8** — Used `setLatencySamples()` in `prepareToPlay()` instead of overriding
2. **ANIRA shared library distribution** — Solved with post-build CMake step that copies .dylib files into plugin bundle Frameworks/ directory with versioned symlinks

---

## Success Criteria Status

- [x] Plugin compiles without errors (CMake + Ninja)
- [x] ANIRA and ONNX Runtime link without missing symbols
- [x] Placeholder ONNX models embedded as binary data (ModelData namespace accessible)
- [x] Plugin loads as instrument (IS_SYNTH TRUE, aumu type)
- [x] All 10 parameters defined in APVTS
- [x] WebView placeholder page available (800x600)
- [x] State save/restore implemented (APVTS copyState/replaceState)
- [x] `auval -a | grep -i texture` shows AU registration
- [x] Shared libraries embedded in plugin bundle for distribution

---

## Notes for Stage 2

- ANIRA `InferenceHandler` and `InferenceConfig` not yet instantiated (Stage 2 task)
- Custom `PrePostProcessor` needed for latent vector input (not audio)
- ANIRA model loading may expect file paths — investigate `CreateSessionFromArray` compatibility
- `processorRef` unused warning in Editor is benign (will be used in Stage 3 for WebView messaging)
