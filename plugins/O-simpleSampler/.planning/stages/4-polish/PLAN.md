# Stage 4: Polish — Execution Plan

**Date:** 2026-06-26
**Plugin:** O-simpleSampler
**Stage:** 4 (Polish) — final stage, closes v1.0
**Inputs:** CONTEXT.md (4 decisions + 1 USER DEPENDENCY), RESEARCH.md (5 areas, execution-ordered)

## Goal

Close out v1.0 of O-simpleSampler: re-arm the offline correctness gate (render-harness), land the deferred content (curated built-in found-sounds + 7 preset param values), retire the 3 documented RT-safety backlog items, then run the full validation sweep and local install. **No new DSP, no parameter changes — the 21-param APVTS contract is FROZEN.**

Tasks are ordered for execution. The render-harness fix is **Task 1** because it is the QUAL-01 re-assertion gate and is currently un-buildable.

## Constraints (do NOT regress)

- **21-param APVTS contract FROZEN** — no param add/remove/rename. Presets + built-ins work within existing 21.
- **Dual binary-data NAMESPACE** — samples = `NAMESPACE BinaryData`/`HEADER_NAME BinaryData.h`; UI = `NAMESPACE UIBinaryData`/`UIBinaryData.h`. Append SOURCES only; never touch the namespaces (`dual_binary_data_namespace_collision`).
- **Sample loading off the audio thread** (PERF-01) — atomic-publish pattern; never block `processBlock`.
- **macOS install hygiene** — sweep both `-dev` and unsuffixed variants before install (`dev_release_variant_shadowing`).
- **USER DEPENDENCY (FUNC-02):** curated found-sound files not yet delivered (`Source/samples/` has piano.wav only, confirmed). Execute proceeds **piano-only**; wiring is written to accept whatever set exists at execute time; CHANGELOG annotates the gap.

## Tasks

### 1. [ ] Fix the render-harness (QUAL-01 gate — do FIRST)
- **Files:** `tests/render-harness/CMakeLists.txt`
- **Edit:** Remove line 25 `${CMAKE_CURRENT_SOURCE_DIR}/../../Source/PluginEditor.cpp` from `target_sources`. Update the stale comment (lines 23–24) — `createEditor()` now resolves inside `PluginProcessor.cpp` via the `#else GenericAudioProcessorEditor` branch (guard already done, `PluginProcessor.cpp:996`), so the editor TU is no longer needed at link time.
- **Why:** harness compiles `PluginEditor.cpp` under `JUCE_WEB_BROWSER=0`; the Stage-3 editor gained WebView types → un-buildable (`render_harness_breaks_on_webview_editor`).
- **Verify:** `cmake -S . -B build -G Ninja -DOUARICON_BUILD_TESTS=ON && cmake --build build --target O-simpleSampler-render-test && ./build/.../O-simpleSampler-render-test` → **ALL PASS** (9/9 at Stage 2.3). Re-asserts QUAL-01 + DSP-01…04.
- **Depends on:** none

### 2. [ ] RT-safety items 1+2 — raw-pointer + retiredSource source-swap handoff
- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Pattern:** mirror O-TextureForge (`PluginProcessor.{h:109,cpp:272}`).
  - Add `std::atomic<juce::AudioBuffer<float>*> sourceForAudio { nullptr };` — audio thread reads this once/block (`memory_order_acquire`). **No shared_ptr on the audio thread.**
  - Message thread keeps `std::shared_ptr<AudioBuffer<float>> currentSource` + add **one** `std::shared_ptr retiredSource` slot. On publish (cpp:435):
    ```cpp
    retiredSource = std::move (currentSource);          // keep old alive one generation
    currentSource = std::move (resampled);
    sourceForAudio.store (currentSource.get(), std::memory_order_release);
    ```
  - Old buffer freed on the **message thread** at the next publish — never on the audio thread.
  - Audio-thread readers (`processBlock` cpp:831) → read `sourceForAudio.load(acquire)`, null-check.
  - Message-thread readers (`getSourceThumbnail` cpp:458, `computeZeroCrossSnaps` cpp:781) → read `currentSource` directly (no atomic needed).
  - **Remove** the deprecated `atomicLoad`/`atomicStore` helpers (h:215–223, `std::atomic_load/store(shared_ptr*)`) — item 2 closes as a side-effect.
- **Why:** audio-thread snapshot can become last ref → `~AudioBuffer` (free) in render path (PERF-01 violation); deprecated C++20 atomics are forward-compat hygiene.
- **Note:** retired-depth-1 is safe — source swaps are user-paced via AsyncUpdater (one per message-thread turn), can't race inside one audio block.
- **Verify:** render-harness still ALL PASS; pluginval@5 (Task 6).
- **Depends on:** Task 1 (harness buildable to re-validate)

### 3. [ ] RT-safety item 3 — defer prepare-time root seed to AsyncUpdater
- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Edit:** Add `std::atomic<bool> pendingRootSeed { false };`. In `prepareToPlay` (cpp:302), replace the direct `seedRootForSource(...)` call with `pendingRootSeed = true; triggerAsyncUpdate();` (keep the `!stateWasRestored && !rootSeeded` gate). In `handleAsyncUpdate()` (cpp:728, guaranteed message thread), consume the flag → call `seedRootForSource(...)`.
- **Why:** `setValueNotifyingHost` inside `prepareToPlay` is discouraged (prepare can run off-message-thread / during scans, before the editor exists → notification re-enters / dropped / dirties project). Reuses the proven AsyncUpdater path (already seeds on user source change, cpp:738).
- **Verify:** fresh instance still seeds piano root (48) on load; pluginval@5.
- **Depends on:** none (independent of Task 2; do together as the RT-safety batch)

### 4. [ ] Author 7 preset param values (FUNC-07)
- **Files:** `Source/PluginProcessor.cpp` (`applyFactoryPreset`, cpp:507)
- **CRITICAL central fix:** after the default-reset loop (cpp:511) and **before** the branch dispatch, re-apply the active source root:
  ```cpp
  seedRootForSource (builtInIndexForIdentity (currentSourceIdentity));
  ```
  The default-reset sets `rootKey → 60`; piano's recorded root is 48 → every preset would play an octave flat. This keeps presets source-agnostic; no preset branch touches `rootKey`.
- **Fill the 7 stubbed branches** (cpp:529–565) using `setReal`/`setChoice`/`setBool` (string IDs). Each branch sets only the non-default params that isolate its concept. Starting values (audition in execute, keep consistent with `PRESET_LESSONS` prose, app.js:417):
  | Preset | Key params |
  |--------|-----------|
  | Raw One-Shot | loopMode=0; start=0,end=100; ampAttack=0.002,ampDecay=0.0,ampSustain=1.0,ampRelease=0.08; pitchMode=0 |
  | Tuned Across the Keyboard | pitchMode=0; loopMode=0; full region; defaults (relies on central root re-seed) |
  | Looped Pad | loopMode=1; loopStart=20,loopEnd=80; loopCrossfade=120; ampAttack=1.5,ampSustain=1.0,ampRelease=2.0 |
  | Reversed Swell | reverse=on; loopMode=0; ampAttack=1.5,ampDecay=0.0,ampSustain=1.0,ampRelease=1.5 |
  | Repitch vs Stretch A/B | pitchMode=1; loopMode=1; loopCrossfade=80; ampAttack=0.01,ampSustain=1.0,ampRelease=0.3 |
  | SP-1200 Crunch | vintage=80; filterCutoff≈12000; ampAttack=0.002,ampDecay=0.15,ampSustain=0.8,ampRelease=0.15 |
  | Filtered & Enveloped | filterCutoff≈1000,filterResonance≈40; ampAttack=0.3,ampDecay=0.6,ampSustain=0.5,ampRelease=0.6 |
- **Storage conventions:** 0–100 raw % (start/end/loop*/vintage/filterResonance/velToAmp); loopCrossfade ms (0–500); filterCutoff Hz (log); ADSR seconds (0–5); fine cents (±100); ampSustain 0–1; rootKey/tune INT; Choice indices per parameter-spec.
- **Remove** the `juce::ignoreUnused (setReal, setChoice, setBool)` line (cpp:527) once branches use them.
- **Verify:** each preset button audibly isolates its concept on the piano source; round-trips (relay resync); no octave-flat playback.
- **Depends on:** Task 1 (harness/build green)

### 5. [ ] Multi-built-in embedding + selector (FUNC-02) — conditional on asset delivery
- **Files:** `CMakeLists.txt` (samples target ~line 113), `Source/PluginProcessor.cpp` (`builtInBlob`, cpp:347–355), and IF names differ: `Source/PluginProcessor.h` (`kBuiltInNames` h:373, `kBuiltInRoot` h:379), `Source/ui/.../app.js:68`, `parameter-spec.md`
- **If assets delivered to `Source/samples/`:**
  1. Append the new files to `O-simpleSampler_Samples` SOURCES (currently piano.wav only; `TODO(Stage 2.3)` comment marks the spot).
  2. Replace the `default → piano` fallback cases 1/2/3 in `builtInBlob()` with real `BinaryData::<name>_wav` / `<name>_wavSize`.
  3. Per delivered sample: set the matching `kBuiltInRoot[i]` (probe f0 → nearest MIDI, or take the user's suggested Root Key).
  4. If names ≠ piano/vocal/flute/vinyl: update `kBuiltInNames`, `kBuiltInRoot`, app.js combo labels (app.js:68), parameter-spec built-in table — all 4 sites consistently (Choice count stays 4; `embedded:<name>` identity must match across all sites or state-restore breaks). JUCE mangles `vocal.wav` → `BinaryData::vocal_wav`.
- **If NOT delivered:** proceed piano-only. `default → piano` fallback keeps every selection audible (never silent). CHANGELOG (Task 7) notes the curated set is pending. No code change required.
- **Why:** decode/resample/publish path is generic — no new DSP. NAMESPACE split already correct; do not touch it.
- **Verify:** each built-in selects/decodes/plays; selecting one seeds its root; state-restore round-trips the `embedded:<name>` identity.
- **Depends on:** Task 2 (publish path touched by RT-safety refactor — write builtInBlob against the final publish API)

### 6. [ ] Validation sweep
- **Files:** none (verification commands)
- **Steps (all must pass):**
  1. **Render-harness** — ALL PASS, re-run **after** Tasks 2–4 (allocation/RT discipline check catches the source-swap refactor).
  2. **auval** — `auval -v aumu OsSm OuDv` → SUCCEEDED, **21 Global Scope Parameters** (contract intact).
  3. **pluginval strictness 5** — VST3 **and** AU, exit 0 (editor crash-guard held).
  4. **native-fn bridge grep** — JS `getNativeFunction` ≡ editor `withNativeFunction` ≡ processor methods, 0 orphans (current 8≡8≡8). Confirm `applyFactoryPreset` + source-load fns survive (`webview_native_fn_bridge_gap`).
- **Depends on:** Tasks 1–5

### 7. [ ] Version bump + CHANGELOG v1.0.0
- **Files:** `CMakeLists.txt:20`, `CHANGELOG.md` (create — none exists)
- **Edit:** `VERSION "0.1.0"` → `"1.0.0"`. Create `CHANGELOG.md` at v1.0.0 documenting: preset tour content (7 presets), RT-safety close-out (3 items), curated built-in set status (full set delivered vs piano-only-pending — match Task 5 outcome), render-harness re-arm.
- **Depends on:** Task 6 (sweep green before stamping v1.0.0)

### 8. [ ] Local install (finish line)
- **Files:** none (`./scripts/build-and-install.sh O-simpleSampler`)
- **Why:** Phase 4 of the script does the dual `-dev`/unsuffixed variant sweep automatically (`dev_release_variant_shadowing`; emits `⚠ Sweeping ALTERNATE-variant` on an orphan) and clears the AU cache before install.
- **Verify:** plugin loads in a DAW as `-dev`; no orphan variants.
- **Note:** **COMPAT-02 (Windows)** runtime verification is the **user's** on a Windows host/DAW (per CONTEXT decision) — wiring is in code, not a CI gate for v1.0.
- **Depends on:** Task 7

## Success Criteria

- [ ] Render-harness builds and ALL PASS (QUAL-01 + DSP-01…04 re-asserted) — both at Stage-4 start and after the RT-safety/preset edits
- [ ] No alloc/free on the audio thread on source swap (RT-safety items 1+2); deprecated `atomic_load/store(shared_ptr)` removed (item 2)
- [ ] Prepare-time root seed deferred to AsyncUpdater (item 3); fresh instance still seeds piano root 48
- [ ] All 7 preset buttons set values that audibly isolate their concept; none plays octave-flat (central root re-seed in place)
- [ ] Built-ins: full curated set embedded + selectable IF delivered; otherwise piano-only with audible fallback on every selection + CHANGELOG annotation
- [ ] auval SUCCEEDED with **21** Global Scope Parameters (contract frozen)
- [ ] pluginval strictness 5 exit 0 on **both** VST3 and AU
- [ ] native-fn bridge: 0 orphans (JS ≡ editor ≡ processor)
- [ ] `VERSION` = 1.0.0; CHANGELOG.md created at v1.0.0
- [ ] Installed locally as `-dev` with no orphan variant bundles

## Open items carried to execute

- **[USER DEPENDENCY]** built-in audio files → `Source/samples/` (+ suggested Root Key per sample). NOT yet delivered — execute proceeds piano-only; Task 5 wiring written against whatever exists at execute time; CHANGELOG annotates.
- **Preset ↔ source coupling** — default is params-only (leave source as-is). Revisit only if a delivered found-sound makes a specific pairing obviously better.
