# Stage 4: Polish - Research

## Research Date

2026-02-14

## Research Scope

Investigating pluginval strictness levels, cross-DAW testing approach, CHANGELOG conventions, code cleanup targets, and version tagging for O-Texture v0.1.0 pre-release.

---

## 1. Pluginval Strictness Levels (5-10)

### Level 5 (Baseline — Previously Passed)
- Basic call coverage: load/unload, processBlock, parameter iteration
- Parameter automation (single pass)
- State save/restore with checksum validation
- Subnormals logged as **warnings** (not errors)
- Editor open/close (single cycle)

### Level 6-7 (Enhanced)
- **Subnormals become ERRORS** (not just warnings) — critical for ONNX decoder output
- **EditorAutomationTest**: iterations increase from 100 to **1000 loops** with 10ms sleep between cycles
- **AUvalTest** (macOS): adds `-stress 20` flag (20 simulated seconds of multi-threaded audio I/O)
- **VST3validator**: adds `-e` (extended) flag for stricter Steinberg validation

### Level 8+ (Binary-Exact State)
- **PluginStateTestRestoration**: binary-exact state matching required
- Every byte of serialized state must match after randomization + restore cycle
- Not just checksum — full byte comparison

### Level 9-10 (Maximum)
- Same tests as level 8 but with increased iteration counts and timeouts
- All tests at maximum strictness

### O-Texture Specific Risks

| Risk | Level Triggered | Impact | Mitigation |
|------|----------------|--------|------------|
| Subnormal output from ONNX decoder | 6+ | ERROR (fail) | `ScopedNoDenormals` already in processBlock — verify it covers decoder output path |
| 1000-iteration editor open/close | 6+ | WebView crash risk | Known JUCE WebView issue — test empirically, may need `--skip-gui-tests` |
| Binary-exact state mismatch | 8+ | evolve_seed from `Time::currentTimeMillis()` may cause non-determinism | Investigate: seed is saved/restored, so roundtrip should be exact |
| Synchronous ONNX inference timing | 5+ | Real-time safety check may flag | Placeholder models are ~1ms, should be fine |
| Optional sidechain bus enable/disable | 5+ | Bus config test cycles sidechain on/off | Already handle missing input gracefully |
| Output buffer allocation in runDecoder | 5+ | `std::vector<float> outputBuffer(4096)` allocated per call | Pre-allocate to member |

### Recommended Testing Strategy

**Phase 1**: Strictness 5 baseline (should pass — previously passed at Stage 2)
```bash
pluginval --strictness-level 5 --validate /path/to/O-Texture-dev.vst3 --timeout-ms 30000 --verbose
pluginval --strictness-level 5 --validate /path/to/O-Texture-dev.component --timeout-ms 30000 --verbose
```

**Phase 2**: Strictness 10 without GUI (isolate DSP from WebView issues)
```bash
pluginval --strictness-level 10 --skip-gui-tests --validate /path/to/O-Texture-dev.vst3 --timeout-ms 60000 --verbose
```

**Phase 3**: Strictness 10 with GUI (full validation)
```bash
pluginval --strictness-level 10 --validate /path/to/O-Texture-dev.vst3 --timeout-ms 60000 --verbose
```

### Known WebView Issue
- JUCE WebView plugins can crash during high-iteration editor automation tests (JUCE Forum: "WebView crashes pluginval")
- Particularly affects macOS/M1 with WebKit ProcessThrottler
- If GUI tests fail at strictness 10, document as known issue and validate DSP separately
- This is a JUCE framework limitation, not an O-Texture bug

---

## 2. Code Review Findings (Cleanup Targets)

### PluginProcessor.cpp

1. **runDecoder() allocates per call** (line 175):
   ```cpp
   std::vector<float> outputBuffer(static_cast<size_t>(kBlockSize));
   ```
   Should pre-allocate as member variable. This is a heap allocation on the audio thread.
   **Severity**: Medium — real-time safety concern at high strictness.

2. **Logger calls in audio path** (lines 200, 332-333):
   - `Logger::writeToLog()` in `runDecoder()` catch block is fine (error path only)
   - `Logger::writeToLog()` in `prepareToPlay()` is fine (not real-time)
   - No logger calls in hot processBlock path — good

3. **Console.log statements in JavaScript** (main.js lines 29-31, 80-85):
   - Multiple `console.log()` calls for debugging (relay initialization, property changes)
   - Should remove or guard for production
   **Severity**: Low — cosmetic, no functional impact

4. **data-parameter-index attributes in HTML**:
   - HTML elements have `data-parameter-index` attributes that are unused by JavaScript
   - These are dead attributes (the JS uses relay names, not indices)
   **Severity**: Very low — cosmetic only

### PluginEditor.cpp

5. **paint() fills with color behind WebView** (line 107-110):
   ```cpp
   void TextureEditor::paint(juce::Graphics& g) { g.fillAll(juce::Colour(0xff1a1a2e)); }
   ```
   This is painted behind the WebView and never visible. Harmless but unnecessary.
   **Severity**: Very low — no impact

### DSP Headers

6. **OverlapAddProcessor::addDecodedBlock** uses shared `windowedBlock` member:
   - Single `std::array<float, 4096> windowedBlock` used for both L and R channels
   - Called sequentially (L then R in processBlock), so no race condition
   - But could be confusing — worth a comment
   **Severity**: None — correct as-is

7. **No TODO/FIXME/HACK comments** found in codebase — clean

### Summary of Code Cleanup Tasks

| File | Issue | Priority |
|------|-------|----------|
| PluginProcessor.cpp | Pre-allocate `outputBuffer` in runDecoder | High |
| main.js | Remove debug console.log statements | Low |
| index.html | Remove unused data-parameter-index attrs | Very low |

---

## 3. Cross-DAW Testing Approach

### Available DAWs for Testing

1. **Logic Pro** (macOS) — AU + VST3
2. **Ableton Live** (macOS) — AU + VST3
3. **Reaper** (macOS) — AU + VST3
4. **Standalone** — Direct launch

### Test Matrix

| Test | Logic | Ableton | Reaper | Standalone |
|------|-------|---------|--------|------------|
| Plugin loads without crash | | | | |
| UI renders (not blank) | | | | |
| Parameters respond to automation | | | | |
| State save/restore (close/reopen project) | | | | |
| Audio output (not silent) | | | | |
| XY pad drag interaction | | | | |
| Source selector changes | | | | |
| Mode toggle works | | | | |
| Freeze toggle works | | | | |
| No CPU spikes | | | | |

### Known Cross-DAW Gotchas for Synth Plugins

1. **Logic Pro**: IS_SYNTH=TRUE requires MIDI track, not audio track. Users must create a Software Instrument track.
2. **Logic Pro**: Sidechain bus may not be exposed for synths. Transform mode may not work in Logic.
3. **Ableton Live**: WebView plugins need `withKeepPageLoadedWhenBrowserIsHidden()` — already set.
4. **Ableton Live**: Plugin window focus/unfocus can cause WebView reload — test tab switching.
5. **Reaper**: Generally most permissive, good baseline test.

---

## 4. CHANGELOG Convention

### Format (from existing plugins)

- [Keep a Changelog](https://keepachangelog.com/en/1.0.0/) format
- [Semantic Versioning](https://semver.org/spec/v2.0.0.html)
- Sections: Added, Changed, Fixed, Removed, Performance, Technical Notes
- Example header: `## [0.1.0] - 2026-02-14`
- Bullet points describe user-facing changes with technical context

### v0.1.0 Content Plan

```markdown
## [0.1.0] - 2026-02-14

### Added
- Neural texture synthesis engine (1D CNN VAE, 32-dim latent space)
- 10 real-time parameters: Source, Mode, X, Y, Character A/B, Evolve, Freeze, Brightness, Mix
- 6 source categories: Rain, Metal, Wind, Crowd, Synth, Organic
- Generate and Transform modes
- XY pad with orbital trail animation
- Evolve modulation via 28-channel Perlin noise
- Freeze mode (halts latent evolution)
- Brightness tilt filter (1-pole, 800 Hz pivot)
- Stereo decorrelation via latent offset
- Ouaricon Naturalist WebView UI (aged paper, botanical motifs)
- State serialization including evolve noise seed and cursor positions
- ONNX Runtime decoder inference (direct C++ API)
- Overlap-add crossfading (4096-block, 2048-hop, Hann window)

### Technical Notes
- Uses placeholder ONNX models — real trained models will be integrated in a future version
- ANIRA v2.0.3 + ONNX Runtime 1.19.2 embedded in plugin bundles
- Plugin registers as AU instrument (aumu OuTx OuDv)
- JUCE 8.0.4, C++20
```

---

## 5. Version Tagging

### Current State
- CMakeLists.txt line 30: `VERSION 1.0.0`
- Plugin name: `O-Texture-dev` (development suffix from OUARICON_DEV_SUFFIX)

### Decision
- Update VERSION to `0.1.0` to reflect pre-release status
- Keep `-dev` suffix (controlled by build system, not version number)
- Version 0.x.y signals "not yet production-ready" per SemVer

### Note on VERSION field
- JUCE uses `VERSION` from `juce_add_plugin()` for the plugin's reported version
- Some DAWs display this version in plugin info
- Important to match CHANGELOG version

---

## 6. Build Verification

### Clean Build Test
- Delete `build/` directory entirely
- Re-run CMake configure + Ninja build
- Confirms no stale object files or cached paths

### Install Verification
- Follow CLAUDE.md sequence: clear AU cache, remove old binaries, install fresh
- Verify AU registration: `auval -a | grep -i texture`
- Verify VST3 presence: `ls ~/Library/Audio/Plug-Ins/VST3/O-Texture-dev.vst3`
- Verify AU presence: `ls ~/Library/Audio/Plug-Ins/Components/O-Texture-dev.component`

---

## 7. Code Signing

### Local Development (Ad-hoc)
```bash
codesign --deep --force --sign - path/to/O-Texture-dev.vst3
codesign --deep --force --sign - path/to/O-Texture-dev.component
```

### Verification
```bash
codesign --verify --verbose=2 path/to/O-Texture-dev.vst3
```

### Note
- Ad-hoc signing is sufficient for local testing
- Production signing uses Developer ID certificates via CI/CD (already configured in build-and-release.yml)

---

## 8. State Serialization Analysis

### Current Implementation (PluginProcessor.cpp lines 429-479)

**getStateInformation():**
1. Copies APVTS state tree
2. Adds `evolve_seed` (int from PerlinNoise1D seed)
3. Adds `evolve_cursors` (comma-separated float string with 6 decimal places)
4. Serializes to XML -> binary

**setStateInformation():**
1. Parses binary -> XML -> ValueTree
2. Replaces APVTS state
3. Restores evolve seed
4. Restores evolve cursors

### Binary-Exact State Concern (Level 8+)

The evolve_seed is derived from `Time::currentTimeMillis()` in the constructor, but **it is serialized and restored** via state. So the pluginval roundtrip test should produce identical bytes:
1. pluginval calls `getStateInformation()` → captures state
2. pluginval randomizes parameters
3. pluginval calls `setStateInformation()` with captured state
4. pluginval calls `getStateInformation()` again → compares

The seed value will be restored to the same value, and cursors to the same positions. **This should pass binary-exact matching.**

However, the cursor values are stored with `String(value, 6)` — 6 decimal places. If `setCursors()` re-evaluates noise (which it does via `evaluateNoise()`), the cached values will be recomputed from the same cursor positions + same seed, producing identical results. **Should be deterministic.**

**Verdict**: State serialization should be robust for pluginval level 8+. No changes needed.

---

## 9. Pre-allocate ONNX Output Buffer

### Current Issue

In `runDecoder()` (PluginProcessor.cpp line 175):
```cpp
std::vector<float> outputBuffer(static_cast<size_t>(kBlockSize));
```

This allocates 4096 floats (16 KB) on the heap every time the decoder runs. While it only runs once per hop (every 2048 samples, ~42ms at 48kHz), this is still a heap allocation on the audio thread.

### Fix

Add a member variable:
```cpp
std::vector<float> decoderOutputBuffer;  // Pre-allocated in prepareToPlay
```

Initialize in `prepareToPlay()`:
```cpp
decoderOutputBuffer.resize(static_cast<size_t>(kBlockSize), 0.0f);
```

Use in `runDecoder()`:
```cpp
Ort::Value outputTensor = Ort::Value::CreateTensor<float>(
    memoryInfo,
    decoderOutputBuffer.data(),
    ...
);
```

This eliminates the per-call allocation and improves real-time safety.

---

## 10. Summary of Research Findings

### Must Do (High Priority)
1. Pre-allocate ONNX output buffer (real-time safety)
2. Run pluginval at strictness 5 (baseline regression after Stage 3)
3. Run pluginval at strictness 10 (target for v0.1.0)
4. Create CHANGELOG.md (v0.1.0)
5. Update VERSION from 1.0.0 to 0.1.0

### Should Do (Medium Priority)
6. Remove console.log debug statements from main.js
7. Cross-DAW testing (Logic, Ableton, Reaper, Standalone)
8. Clean build verification (delete build/, rebuild)
9. Ad-hoc code signing

### Nice to Have (Low Priority)
10. Remove unused data-parameter-index HTML attributes
11. Verify `isBusesLayoutSupported()` handles all pluginval bus test variations

### Known Limitations (Document, Don't Fix)
- WebView may crash during high-iteration editor automation (JUCE framework issue)
- Placeholder models produce noise, not real textures
- Transform mode limited in Logic Pro (IS_SYNTH sidechain limitations)
