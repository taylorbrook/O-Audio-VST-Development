# Stage 1 (Foundation) — RESEARCH

**Mode:** Express / non-interactive. Foundation patterns are fully precedented in sibling plugins; no novel research needed (the novel work — SequencerClock/playhead — is Stage 2).

## Reusable patterns (verified in-repo, JUCE 8.0.9)

### 1. CMake plugin shell — copy O-simpleSubtractive/CMakeLists.txt
- `juce_add_plugin(... IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE FORMATS VST3 AU Standalone ...)`.
- `PLUGIN_CODE OSiB`, `PRODUCT_NAME "O-simpleBeatmaker${OUARICON_DEV_SUFFIX}"`, `VERSION "1.0.0"`.
- WebView2 flags set at Foundation (so Stage 3 inherits correct cross-platform config):
  `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`,
  `JUCE_VST3_CAN_REPLACE_VST2=0`; CMake `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`.
  **No `juce_add_binary_data` target yet** (no WebView resources until Stage 3) → avoids the
  O-simpleGrain dual-namespace collision by simply not having a 2nd target this stage.
- Root CMake auto-discovers `plugins/*/CMakeLists.txt` via glob → no root edit needed.
- Render-harness `add_subdirectory(tests/render-harness)` is **deferred to Stage 2** (the dir does not exist yet; adding the guarded block now would be a foot-gun if toggled ON early).

### 2. APVTS layout — `createParameterLayout()` idiom (O-simpleSubtractive PluginProcessor.cpp:38)
- `std::vector<std::unique_ptr<juce::RangedAudioParameter>>`, `ParameterID{ id, 1 }` (version hint 1).
- Helpers: a normalized `unitRange()` (0–1) and a dB range. `AudioParameterChoice` for `patternLength`.
- IDs in a `namespace OSimpleBeatmaker::ParamIDs` (single source of truth), generated for the 6×6 voice block via a small helper so all 42 are consistent.

### 3. Custom PATTERN state — extends the standard APVTS save/restore
- Standard sibling save: `parameters.copyState().createXml()` → `copyXmlToBinary`.
- **Extension:** on save, append a fresh `ValueTree "PATTERN"` child to the *copied* state encoding the 6×32 `std::atomic<uint8_t>` grid (base64 blob via `juce::MemoryBlock::toBase64Encoding` — JUCE's own format, used symmetrically for save+restore so the non-standard encoding is internally consistent; this is NOT JS-interop base64, so the MicrotonalSampler gotcha does not apply).
- On restore: parse the tree, read the `PATTERN` child into the atomics, then `removeChild(PATTERN)` **before** `parameters.replaceState(tree)` so the live APVTS tree never carries PATTERN → no duplicate-child accumulation across save cycles.
- Grid access: flat `std::array<std::atomic<uint8_t>, 6*32>`, index `voice*32 + step`. UI writes (message thread) / audio reads (atomic load) — lock-free, no allocation.

### 4. Editor shell — `GenericAudioProcessorEditor` (O-simpleSubtractive PluginEditor)
- Thin `AudioProcessorEditor` hosting a `GenericAudioProcessorEditor` child filling the window so all 42 params are visible/testable in any host. Stage 3 swaps the body for WebView while keeping the class identity + CMake `target_sources` stable.

### 5. Silent processBlock
- `juce::ScopedNoDenormals`; `buffer.clear()`; ignore MIDI. `setLatencySamples(0)` in `prepareToPlay`.
- `isBusesLayoutSupported`: output-only (mono or stereo), input bus disabled.

## Pitfalls pre-checked (from memory / CLAUDE.md)

- ✅ `getLatencySamples()` non-virtual → use `setLatencySamples`.
- ✅ WebView2 static-linking flag pairing (set now, even though WebView is Stage 3).
- ✅ No 2nd `juce_add_binary_data` target this stage → no namespace collision.
- ✅ Pattern as custom ValueTree, never 384 APVTS params.
- ✅ `MemoryBlock` base64 used symmetrically (save+restore) — safe; the JS-`btoa` interop gotcha is irrelevant here.

## Decision: no `gsd-phase-researcher` spawn

Complexity for *this stage* is low and fully precedented; spawning a researcher would add latency with no new information. (Stage 2 will warrant deeper research for the playhead/timing work.)
