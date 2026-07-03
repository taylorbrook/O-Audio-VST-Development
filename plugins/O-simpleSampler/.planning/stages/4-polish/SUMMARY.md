# Stage 4: Polish — Execution Summary

**Date:** 2026-06-26
**Plugin:** O-simpleSampler
**Stage:** 4 (Polish) — final stage, closes v1.0.0
**Result:** ✅ Complete — all 8 tasks done; full validation sweep green; installed `-dev` v1.0.0.

## What shipped

### 1. Render-harness re-armed (QUAL-01 gate) ✓
- Dropped `Source/PluginEditor.cpp` from `tests/render-harness/CMakeLists.txt` `target_sources` (it gained WebView types under `JUCE_WEB_BROWSER=0`; `createEditor()` now resolves via the `#else GenericAudioProcessorEditor` branch). Updated the stale comment.
- **Result: builds + 9/9 PASS** (makes-sound, repitch-tuning, stretch-pitch-tracks, stretch-time-independence, loop-seam-continuity fwd+pingpong, region-end-declick, vintage-clean-at-zero, aa-uptranspose-stable, stress-bounded). Re-asserts QUAL-01 + DSP-01…04 **after** the RT-safety + content edits.

### 2+3. RT-safety backlog closed (3 items) ✓
- **Items 1+2 — RT-safe source-swap handoff** (mirrors O-TextureForge, adapted): audio thread reads a raw `std::atomic<juce::AudioBuffer<float>*> sourceForAudio` (acquire) once per block — **never a `shared_ptr`**, so it can never own the last ref and free a buffer in the render path. Owning `currentSource` + one-generation `retiredSource` are guarded by a `juce::CriticalSection sourceMutex` taken **only off the audio thread**.
  - **Deviation from the plan's literal wording (justified):** the plan said the off-audio readers need "no atomic." But `decodeAndPublish` runs synchronously inside `prepareToPlay` (a host/audio-setup thread, **not** the message thread, unlike O-TextureForge which only mutates its corpus in an async message-thread callback). Dropping to a bare `shared_ptr` would have reintroduced a `prepareToPlay`-writer vs editor-reader data race that the original `atomic_load/store` was guarding. The `CriticalSection` closes **both** RT items (raw-ptr on audio thread; deprecated C++20 `atomic_load/store(shared_ptr)` helpers removed) with **no** new race and **no** audio-thread locking.
  - Readers updated: `processBlock` → raw ptr; `getSourceThumbnail` + `computeZeroCrossSnaps` → copy `currentSource` under the lock.
- **Item 3 — deferred fresh-instance root seed**: `prepareToPlay` no longer calls `seedRootForSource` (→ `setValueNotifyingHost`) directly; it sets `pendingRootSeed` + `triggerAsyncUpdate()`, and `handleAsyncUpdate()` (guaranteed message thread) consumes it. Validated end-to-end by the harness (message loop pumped → seed fires → `repitch-tuning` PASS at piano root 48).

### 4. Seven preset values authored (FUNC-07) ✓
- **CRITICAL central fix:** after the default-reset (which forces `rootKey → 60`) and before branch dispatch, `applyFactoryPreset` re-applies the active source's recorded root (`seedRootForSource(builtInIndexForIdentity(currentSourceIdentity))`). Keeps presets source-agnostic; **no preset plays octave-flat**.
- Filled all 7 branches with values from RESEARCH §4 (Raw One-Shot, Tuned Across the Keyboard, Looped Pad, Reversed Swell, Repitch vs Stretch A/B, SP-1200 Crunch, Filtered & Enveloped), using symbolic `ParamIDs` string IDs via the existing `setReal`/`setChoice`/`setBool` helpers. Removed the `ignoreUnused` stub. Stale "Stage-4 TODO" doc comments updated.

### 5. Multi-built-in set wired — FULL set (FUNC-02) ✓
- **USER DEPENDENCY resolved at execute time:** the user delivered `cello.aif`, `Hit02_s.WAV`, `string pizz.aif` to `Source/samples/` (confirmed: "wire all 4"). Renamed to clean JUCE symbols (`hit.wav`, `pizz.aif`).
- Roots probed via YIN f0 → nearest MIDI: **piano 48** (C3), **cello 69** (A4), **pizz 69** (A4), **hit 60** (percussive/neutral).
- Edits, consistent across all sites: CMake `O-simpleSampler_Samples` SOURCES (append only — dual NAMESPACE untouched); `builtInBlob()` cases 1/2/3 → real `BinaryData::{cello_aif,pizz_aif,hit_wav}`; `kBuiltInNames` + `kBuiltInRoot`; `sourceSample` Choice StringArray; `parameter-spec.md` built-in table; doc comments. No app.js change (combo populates from the APVTS Choice param).

### 6. Validation sweep — all green ✓
| Check | Result |
|-------|--------|
| Render-harness | **9/9 PASS** (post-refactor) |
| `auval -v aumu OsSm OuDv` | **SUCCEEDED — 21 Global Scope Parameters** (contract frozen); Component Version 1.0.0 |
| pluginval strictness 5 — VST3 | **exit 0** |
| pluginval strictness 5 — AU | **exit 0** |
| native-fn bridge | **8 JS ≡ 8 editor ≡ 8 processor, 0 orphans** (`applyFactoryPreset` + source-load fns intact) |

### 7. Version + CHANGELOG ✓
- `CMakeLists.txt` `VERSION "0.1.0"` → **`"1.0.0"`**.
- Created `CHANGELOG.md` at **v1.0.0** (full feature set + Stage-4 closeout + validation + the curated built-in set delivered, not piano-only-pending).

### 8. Local install ✓
- `./scripts/build-and-install.sh O-simpleSampler` rebuilt at v1.0.0 and installed `-dev` VST3 + AU (12M each, with the 4 embedded samples). Dual-variant sweep ran; **no alternate-variant orphan** warning. AU cache cleared. Post-install auval confirms Component Version 1.0.0 + 21 params.

## Constraints honored
- 21-param APVTS contract **FROZEN** (auval confirms 21).
- Dual binary-data NAMESPACE untouched (samples = `BinaryData`, UI = `UIBinaryData`); SOURCES appended only.
- Sample loading off the audio thread (PERF-01) — strengthened by the raw-ptr handoff.
- macOS install hygiene — dual-variant sweep, no orphans.

## Carry-outs / not in v1.0
- **COMPAT-02 (Windows):** WebView2 wiring is in code; **runtime verification on a Windows host/DAW is the user's** (per CONTEXT decision) — not a CI gate for v1.0.
- Per-preset audible isolation was authored to spec + validated structurally (build/auval/pluginval/round-trip); a final by-ear pass on each preset button in a DAW is recommended but not blocking.
- Render-harness `JucePlugin_VersionString` left at "0.1.0" — test-only metadata, not shipped; intentionally not bumped to avoid a no-op harness recompile.
