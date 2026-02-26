# O-IntonationPad Code Review & Simplification Backlog

**Created:** 2026-02-24
**Context:** Full code review pass across DSP, Processor, Editor, UI, and build config.
**Items 1-8 resolved separately. Remaining items below.**

---

## High Priority

### 9. 128 redundant mutex acquisitions in TuningEngine::rebuildFrequencyTable
- **File:** `TuningEngine.cpp`
- **Issue:** `rebuildFrequencyTable` calls `calculateCustomFrequency` 128 times in a loop, each acquiring/releasing `intervalMutex` individually. Intervals don't change during the loop.
- **Fix:** Create private `calculateCustomFrequencyUnlocked()`, lock once at the top of `rebuildFrequencyTable`.

### 10. dynamic_cast on audio thread always succeeds
- **File:** `PluginProcessor.cpp:475-494`
- **Issue:** All 8 synth voices are `WavetableVoice`. The `dynamic_cast` in `processBlock` always succeeds — the null check is dead code. Also applies to `getActiveNotes()` on the message thread (line 688).
- **Fix:** Replace with `static_cast`.

---

## Medium Priority

### 11. 4x copy-pasted JSON array builder in PluginEditor
- **File:** `PluginEditor.cpp:143-371`
- **Issue:** Same loop converting `vector<double>` to JSON string repeated in `generateEDO`, `generateHarmonicSeries`, `generateRank2`, and `getTuningIntervals` native functions.
- **Fix:** Extract a `static juce::String doubleVectorToJSON(const std::vector<double>& v)` helper.

### 12. Duplicate ProcessSpec in prepareToPlay
- **File:** `PluginProcessor.cpp:356-405`
- **Issue:** `filterSpec` and `fxSpec` are byte-for-byte identical structs.
- **Fix:** Use one `spec` variable.

### ~~13. Dead LFO phase increment in prepareToPlay~~ ✅ Fixed v1.15.8
- **File:** `PluginProcessor.cpp:368-373`
- **Issue:** `lfoPhaseIncrementA/B` computed in `prepareToPlay` but unconditionally overwritten every `processBlock`.
- **Fix:** Removed 4 dead lines (2 param reads + 2 assignments). Phase resets retained.

### ~~14. Wrong relay type for delayMode~~ ✅ Fixed v1.15.9
- **File:** `PluginEditor.h:75`, `PluginEditor.cpp:80,137,517-518`, `index.html:915,866-871,972`
- **Issue:** `delayMode` is `AudioParameterChoice` but wired to `WebSliderRelay`/`WebSliderParameterAttachment` instead of `WebComboBoxRelay`/`WebComboBoxParameterAttachment`.
- **Fix:** Changed to combo box relay/attachment in C++ and dropdown select in JS UI.

### ~~15. Dead getNextSample() in WavetableOscillator~~ ✅ Fixed v1.15.10
- **File:** `WavetableOscillator.h`
- **Issue:** Never called anywhere in the codebase. `processBlockStereo()` and `advancePhase()` are used exclusively.
- **Fix:** Removed the 35-line method entirely.

### ~~16. Dead tuning_tuningMode parameter~~ ✅ Fixed v1.15.11
- **File:** `PluginProcessor.cpp:53-58`, `parameterChanged:591-601`
- **Issue:** No UI relay exists for this parameter. The `temperamentPreset` already sets tuning mode implicitly via `setBuiltInPreset()`.
- **Fix:** Removed the parameter definition, listener registration, and parameterChanged handler case.

### ~~17. Local generateEDO lambda duplicates ScaleGenerator::generateEDO~~ ✅ Fixed v1.15.12
- **File:** `EmbeddedTunings.cpp:120-126`
- **Issue:** Local lambda reimplemented EDO generation without validation and with different period semantics (excludes period vs includes).
- **Fix:** Lambda now delegates to `ScaleGenerator::generateEDO()` and strips the trailing period to match `EmbeddedTuning` struct convention.

### ~~18. Dead intervalScaleSize variable in index.html~~ ✅ Fixed v1.15.13
- **File:** `index.html:1565, 1609-1612`
- **Issue:** Set on change detection but never read for any conditional behavior. `renderIntervalToggles` runs unconditionally.
- **Fix:** Removed variable declaration, assignment, and dead if-block.

### ~~19. 35+ document-level mousemove/mouseup listeners~~ ✅ Fixed v1.15.14
- **File:** `index.html:1396-1413`
- **Issue:** Each `setupKnob` call registers its own `mousemove`/`mouseup` pair on `document`. All 35+ handlers execute on every mouse move, checking `if (!isDragging)`.
- **Fix:** Single `knobDrag` state object with one pair of document-level listeners. Removed per-knob `isDragging`, `lastY`, `virtualNorm` locals.

### 20. Dead pitch-circle.js bundled in binary data
- **Files:** `Source/ui/public/modules/pitch-circle.js`, `CMakeLists.txt`, `PluginEditor.cpp:697`
- **Issue:** Bundled, served by resource provider, but never imported. Rendering is inlined in `tuning-panel.js`.
- **Fix:** Delete the file, remove from CMakeLists binary data, remove the resource-provider case.

### 21. Magic number 0.0001f repeated 4 times
- **File:** `WavetableVoice.cpp` inside `renderNextBlock`
- **Issue:** Silence threshold used in 4 separate checks with no named constant.
- **Fix:** `static constexpr float kSilenceThreshold = 0.0001f;`

### 22. Broken masterVolume formatter in index.html
- **File:** `index.html:1243-1248`
- **Issue:** Applies `20 * Math.log10(v)` to a value that's already in dB (range -60 to 6). Double-converts.
- **Fix:** If param is dB: `v => v.toFixed(1)`. If param is linear gain: fix the knob range.

---

## Low Priority (Cleanup)

### 23. Duplicate scrollbar CSS for #tuning-tab and #effects-tab
- **File:** `index.html:146-157, 550-561`
- **Fix:** Shared `.scrollable-tab` class.

### 24. Duplicate OSC A/B wavetable option lists
- **File:** `index.html:763-779, 841-857`
- **Fix:** Build both selects from a shared JS array. (May be addressed by #7 knob factory work.)

### 25. Repeated static_cast<size_t>(i) pattern
- **File:** `WavetableVoice.cpp` and `.h`, ~10 loops
- **Fix:** Use `size_t i` as loop variable.

### 26. Redundant juce_audio_formats and juce_events in CMakeLists
- **File:** `CMakeLists.txt`
- **Fix:** Remove both. No audio file I/O in plugin; `juce_events` is transitively linked.

### 27. Version annotation comments throughout index.html
- **File:** `index.html` (~18 locations)
- **Fix:** Remove — git log is the better record.

### 28. Triple iteration over notes in updateActiveNotes
- **File:** `index.html:1767-1830`
- **Fix:** Single-pass loop building `activeMidi`, `uniqueNotes`, and `groups` maps together.

### 29. DBG() calls in TuningEngine parameter-change paths
- **File:** `TuningEngine.cpp` — `setSingleInterval`, `setBuiltInPreset`, `loadScalaFile`, `loadKBMFile`
- **Fix:** Remove or consolidate to one summary DBG per operation.

### 30. Stale .planning/ docs
- **Files:** `.planning/STATUS.md`, `.planning/parameter-spec.md`
- **Issue:** Reflect v1.0.0 architecture, not current v1.15.1.
- **Fix:** Update or add a note that these are historical planning docs.
