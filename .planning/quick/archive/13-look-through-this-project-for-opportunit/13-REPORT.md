# Refactoring and Simplification Opportunities Report

## Executive Summary

The Ouaricon plugin codebase (23 plugins, ~10,741 PluginEditor.cpp lines) contains significant duplication in three main areas: preset native function registration (~1,400 lines across 14 plugins), resource provider URL mapping (~1,400 lines across 23 plugins), and WebView initialization boilerplate. A phased approach -- starting with the two highest-impact items -- could eliminate **~3,000+ lines** of copy-pasted code while improving consistency and reducing the surface area for bugs.

**Impact summary:**

| Priority | Opportunities | Lines Eliminable | Plugins Affected |
|----------|--------------|-----------------|-----------------|
| HIGH | 3 | ~3,200+ | 14-23 |
| MEDIUM | 6 | ~500-800 | 9-23 |
| LOW | 4 | Varies | 5-14 |

---

## HIGH PRIORITY -- Massive Duplication

### 1. Preset Native Function Boilerplate (~1,400 lines across 14 plugins)

**Description:** Every plugin with preset support copy-pastes ~80-112 lines of identical `withNativeFunction()` lambda registrations into the PluginEditor constructor. These lambdas for `savePreset`, `loadPreset`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`, `deletePreset`, `isFactoryPreset`, `savePresetWithDialog`, and `loadPresetFromFile` are functionally identical across all 14 plugins -- the only difference is the processor variable name (`processorRef` vs `processor`).

The existing `preset-manager` module at `modules/persistence/preset-manager/` provides the C++ `OuariconPresetManager` class and `preset-manager.js` JS module, but does NOT address the ~100-line native function registration block each editor must manually write.

**Affected plugins (14):**
O-AnalogEQ, O-Bass, O-Bells, O-Chorus, O-Comp, O-Detune, O-DigiDelay, O-FreqPulse, O-Lyrica, O-Marimba, O-Polystutter, O-SimpleReverb, O-SpectralShaper, O-Tremolo

**Lines per plugin:** 78-112 (measured: O-AnalogEQ ~93, O-Tremolo ~112, O-Comp ~101, O-Chorus ~78)

**Total lines eliminable:** ~1,300-1,400 (reduced to ~14-28 lines total, 1-2 per plugin)

**Scope:** Medium (1-2 days)
**Risk:** Low -- pure refactor, no behavior change. Each plugin calls the same preset manager API.

**Suggested approach:**
Create a helper function in the preset-manager module:
```cpp
// modules/persistence/preset-manager/cpp/PresetNativeFunctions.h
static juce::WebBrowserComponent::Options
registerPresetNativeFunctions(
    juce::WebBrowserComponent::Options options,
    OuariconPresetManager& presetManager,
    std::unique_ptr<juce::FileChooser>& fileChooser);
```
Each editor's constructor reduces from ~100 lines to:
```cpp
options = registerPresetNativeFunctions(options, processorRef.presetManager, fileChooser);
```

---

### 2. `getResource()` URL Mapping Duplicated in All 23 PluginEditors (~1,400 lines)

**Description:** Every Ouaricon plugin hand-writes a `getResource()` method containing the same `makeVector` lambda and identical URL mappings for core resources (`/`, `/index.html`, `/js/juce/index.js`, `/js/juce/check_native_interop.js`, `/modules/preset-manager.js`). Only the plugin-specific image resources (`/img/paper.jpg`, `/img/botanical.png`, etc.) differ between plugins.

The module registry declares a `resource-provider` module at `core/resource-provider`, but **this directory does not exist** -- it was never implemented. The average `getResource()` method is 50-65 lines per plugin, with ~30-40 lines being identical boilerplate.

**Affected plugins:** All 23 Ouaricon plugins

**Total lines eliminable:** ~700-900 (the shared core mappings, keeping only plugin-specific image entries)

**Scope:** Medium (1-2 days)
**Risk:** Low -- the mapping logic is mechanical and easily testable.

**Suggested approach:**
Create the `resource-provider` module at `modules/core/resource-provider/`:
```cpp
// ResourceProvider.h
class OuariconResourceProvider {
public:
    // Register a plugin-specific resource
    void addResource(const juce::String& path, const char* data, int size, const char* mimeType);

    // Resolve URL to resource (handles core + plugin-specific)
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);
};
```
Each plugin's `getResource()` reduces from ~60 lines to ~10-15 (only registering its unique images).

**Bonus fix:** This would also resolve the MIME type inconsistency discovered during audit -- 14 plugins use `text/javascript` while 10 use `application/javascript` for the same JS resources. A shared provider ensures consistency.

---

### 3. WebView Initialization Boilerplate (0 of 23 plugins use WebViewRelayManager)

**Description:** The `webview-relay-manager` module exists at `modules/core/webview-relay-manager/` and is registered in `registry.yaml`, but **zero plugins actually use it in source code**. O-Prism has it declared via `ouaricon_add_module` in its CMakeLists.txt and references it in planning documents, but the actual `PluginEditor.cpp` does not import or use `WebViewRelayManager.h`.

Every plugin manually writes the relay-create / webview-create / attachment-create pattern with identical `withBackend(juce::WebBrowserComponent::Options::Backend::webview2)`, `withWinWebView2Options(...)`, `withNativeIntegrationEnabled()`, `withResourceProvider(...)` chains. This is typically 15-25 lines of identical setup code.

**Affected plugins:** All 23 Ouaricon plugins

**Total lines eliminable:** ~350-575 (15-25 lines x 23 plugins)

**Scope:** Large (2-3 days) -- requires careful testing since WebView lifecycle and destruction order directly affect crash safety in release builds
**Risk:** Medium -- WebView destruction order bugs cause release-build crashes (the exact problem WebViewRelayManager was designed to solve). Changes need testing on both macOS and Windows.

**Suggested approach:**
Either adopt the existing `WebViewRelayManager` module (preferred) or create a builder/factory:
```cpp
auto [webView, relays] = OuariconWebViewBuilder()
    .withPresets(processorRef.presetManager, fileChooser)
    .withRelay("speed", speedRelay)
    .withRelay("depth", depthRelay)
    .withResourceProvider([this](auto& url) { return getResource(url); })
    .build();
```
**Prerequisite:** Opportunity #1 and #2 should be done first, as they remove the bulk of what gets passed into WebView options.

---

## MEDIUM PRIORITY -- Inconsistencies and Structural Issues

### 4. Ghost Module in Registry

**Description:** `modules/registry.yaml` registers `resource-provider` at path `core/resource-provider` (lines 87-98), but this directory does not exist. This creates confusion about what's available vs. what's aspirational.

**Affected files:** `modules/registry.yaml`

**Scope:** Small (10 minutes to remove, or part of Opportunity #2 to create)
**Risk:** None

**Suggested approach:** Either remove the entry until the module is built, or create the module as part of Opportunity #2. If removing, also update `used_by` references.

---

### 5. Module Adoption Gaps (preset-manager declared in 5 of 14 plugins)

**Description:** Only 5 plugins formally declare `ouaricon_add_module ... preset-manager` in their CMakeLists.txt (O-AnalogEQ, O-Chorus, O-Detune, O-SimpleReverb, O-Tremolo). The remaining 9 preset-using plugins (O-Bass, O-Bells, O-Comp, O-DigiDelay, O-FreqPulse, O-Lyrica, O-Marimba, O-Polystutter, O-SpectralShaper) have the preset manager code copy-pasted directly rather than using the module system.

Similarly, 8 plugins do not include `OuariconModules.cmake` at all: O-Bass, O-Freeze, O-IntonationPad, O-Lyrica, O-Marimba, O-MultiBandCompressor, O-Polystutter, O-SpectralShaper.

**Affected plugins:** 9 (preset-manager gap), 8 (OuariconModules.cmake gap)

**Scope:** Small-Medium (2-4 hours) -- mechanical CMakeLists.txt updates
**Risk:** Low -- adding module declarations does not change compiled output if the code is already present

**Suggested approach:** Add `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)` and `ouaricon_add_module(...  preset-manager)` to all 9 remaining preset plugins. This formalizes existing dependencies and enables future module-system features (like automatic native function registration from Opportunity #1).

---

### 6. CMakeLists.txt target_link_libraries Duplication

**Description:** All 23 plugins copy-paste the same `target_link_libraries` block with 13 JUCE modules. Hash analysis reveals ~8 distinct configurations, but the variation is small (most just add `juce_dsp` or `juce_cryptography` to the base set of 12). 12 of 23 plugins share an identical module list.

**Affected plugins:** All 23

**Scope:** Medium (half-day)
**Risk:** Low-Medium -- CMake changes need a full rebuild to verify

**Suggested approach:** Create a CMake function in `modules/cmake/OuariconModules.cmake`:
```cmake
function(ouaricon_link_standard_juce TARGET)
    target_link_libraries(${TARGET}
        PRIVATE juce::juce_audio_basics juce::juce_audio_devices ... [12 standard modules]
        PUBLIC juce::juce_recommended_config_flags ...)
endfunction()
```
Plugins needing extras (juce_dsp, juce_cryptography) add them separately. Reduces each CMakeLists.txt by ~15 lines.

---

### 7. Licensing Boilerplate in 9 Editors (~150+ lines)

**Description:** 9 plugins have identical `#if OUARICON_LICENSING_ENABLED` blocks covering: overlay creation in constructor (~8 lines), listener registration (~3 lines), destructor cleanup (~3 lines), resized() bounds (~3 lines), and `licenseStatusChanged()` callback (~15 lines). Total: ~30 lines per plugin, 270 lines across 9 plugins.

**Affected plugins:** O-AnalogEQ, O-AnalogSaturation, O-Bells, O-Chorus, O-Comp, O-Detune, O-DigiDelay, O-SimpleReverb, O-Tremolo

**Scope:** Medium (half-day)
**Risk:** Low -- the licensing overlay is a well-isolated pattern

**Suggested approach:** Create a `LicensingEditorHelper` class or mixin that handles overlay lifecycle. Could be integrated into a future `OuariconEditorBase` class if combined with opportunities #1-3.

---

### 8. Inconsistent Processor Variable Naming

**Description:** Three different names for the processor reference across 23 editors:
- `processorRef` -- 16 plugins (majority)
- `processor` -- 4 plugins (O-AnalogEQ, O-Chorus, O-Marimba, O-TextureForge)
- `audioProcessor` -- 1 plugin (O-GrainScatter)

This causes friction when reading code across plugins and makes grep/search-replace refactoring harder.

**Affected plugins:** 5 (the non-standard ones)

**Scope:** Small (1-2 hours)
**Risk:** Low -- find-and-replace within each plugin

**Suggested approach:** Standardize to `processorRef` (the majority convention). Update the 5 outlier plugins. If building an `OuariconEditorBase` class (from opportunity #3), this becomes automatic.

---

### 9. UI Directory Structure Inconsistency

**Description:** Two different directory layouts for WebView UI assets:
- `Source/ui/public/` -- 18 plugins (majority)
- `Resources/ui/` -- 5 plugins (O-Bells, O-FreqPulse, O-Lyrica, O-Orbit, O-SpectralShaper)

Both serve the same purpose. The inconsistency makes tooling and templates harder.

**Affected plugins:** 5 (the `Resources/ui/` variants)

**Scope:** Medium (half-day per plugin, but mechanical)
**Risk:** Medium -- requires updating CMakeLists.txt BinaryData paths and verifying all resources still resolve

**Suggested approach:** Standardize to `Source/ui/public/` (the 78% majority). Move files, update CMakeLists.txt `juce_add_binary_data` paths. Do one plugin at a time with build verification.

---

## LOW PRIORITY -- Nice to Have

### 10. `parentHierarchyChanged()` Not Universally Applied

**Description:** 9 plugins use the safer `parentHierarchyChanged()` pattern for deferred WebView navigation, while 14 navigate immediately in the constructor. The deferred pattern prevents crashes during DAW plugin scanning when the component isn't fully attached.

**Using `parentHierarchyChanged()` (safer):** O-Bass, O-Detune, O-DigiDelay, O-Freeze, O-IntonationPad, O-MultiBandCompressor, O-Orbit, O-SpectralShaper, O-Tremolo

**Navigating in constructor (riskier):** O-AnalogEQ, O-AnalogSaturation, O-Bells, O-Chorus, O-Comp, O-FreqPulse, O-GrainScatter, O-Lyrica, O-Marimba, O-Polystutter, O-Prism, O-SimpleReverb, O-Texture, O-TextureForge

**Scope:** Small (1-2 hours)
**Risk:** Low -- `parentHierarchyChanged()` is strictly safer; no behavior change for normally-loaded plugins

**Suggested approach:** Adopt `parentHierarchyChanged()` in all 14 remaining plugins. If building an `OuariconEditorBase` class, this becomes the default behavior.

---

### 11. Large PluginProcessor Files

**Description:** Three processors have significant complexity that could benefit from further DSP extraction:

| Plugin | Lines | DSP/ Files Already | Notes |
|--------|-------|--------------------|-------|
| O-Polystutter | 1,829 | 6 | Has RepeatLane, TriggerRouter DSP classes; processor still very large |
| O-Bells | 1,413 | 0 | No DSP extraction done; all modal synthesis inline |
| O-Lyrica | 1,070 | 22 | Already well-extracted; processor size is reasonable |
| O-FreqPulse | 982 | 0 | Spectral sequencer logic all in processor |
| O-Detune | 885 | 0 | Detuning algorithm in processor |

**Scope:** Large (1-2 days per plugin)
**Risk:** Medium -- DSP refactoring can introduce subtle audio bugs

**Suggested approach:** Extract DSP into `Source/DSP/` classes following the pattern established by O-Lyrica, O-IntonationPad, and O-Prism. Prioritize O-Bells (no DSP extraction at all) and O-FreqPulse.

---

### 12. tache_plugins Cleanup

**Description:** The `plugins/tache_plugins/` directory contains 17 entries:
- **10 building plugins** (have CMakeLists.txt): AngelGrain, AutoClip, DriveVerb, Drum808, DrumRoulette, FlutterVerb, GainKnob, LushPad, OrganicHats, Scatter + MinimalKick (stalled at Stage 5)
- **5 ideated-only** (no CMakeLists.txt): ClapMachine (4K), LushVerb (152K), PadForge (100K), Words (72K)
- **1 template**: TEMPLATE-HEADLESS-EDITOR (12K) -- still referenced by the plugin-workflow Stage 3-4 decision gate

**MinimalKick** is stuck at Stage 5 with a `.continue-here.md` file. It has a CMakeLists.txt so it's included in every full build.

The ideated-only plugins contain `.ideas/mockups/` directories with design exploration artifacts.

**Scope:** Small (1 hour)
**Risk:** None for ideated plugins; Low for MinimalKick

**Suggested approach:**
- Keep TEMPLATE-HEADLESS-EDITOR (actively used by workflow)
- Consider adding ideated plugins to `SKIP_PLUGINS` in the root CMakeLists.txt (ClapMachine doesn't build but the others with CMakeLists do) or leaving as-is since they don't have CMakeLists
- For MinimalKick: either resume Stage 5 or archive it to prevent build-time overhead
- Mockup directories (`.planning/mockups/`) in 7 Ouaricon plugins are development artifacts -- harmless but could be archived

---

### 13. MIME Type Inconsistency in getResource()

**Description:** Plugins split roughly evenly between `text/javascript` (14 plugins) and `application/javascript` (10 plugins) as the MIME type for `.js` files. Both work, but `application/javascript` is the current standard (RFC 9239). One plugin (O-Comp) uses both.

**Scope:** Small (30 minutes) -- or automatically resolved by Opportunity #2
**Risk:** None -- both MIME types work in all browsers/WebView implementations

**Suggested approach:** Resolve as part of Opportunity #2 (shared resource provider). If done independently, standardize to `application/javascript`.

---

## Recommended Execution Order

The opportunities chain naturally into phases:

**Phase A -- Quick Wins (1 day)**
1. Fix ghost module in registry (#4) -- 10 minutes
2. Standardize processor variable naming (#8) -- 1-2 hours
3. Add module declarations to remaining plugins (#5) -- 2-4 hours
4. Standardize `parentHierarchyChanged()` (#10) -- 1-2 hours

**Phase B -- High-Impact Deduplication (3-4 days)**
5. Create preset native function helper (#1) -- 1-2 days
6. Create resource-provider module (#2) -- 1-2 days (resolves #4 and #13)

**Phase C -- Infrastructure Consolidation (2-3 days)**
7. CMake `target_link_libraries` consolidation (#6) -- half-day
8. Licensing boilerplate consolidation (#7) -- half-day
9. WebView initialization consolidation (#3) -- 2-3 days

**Phase D -- Per-Plugin Improvements (ongoing)**
10. UI directory standardization (#9) -- half-day per plugin
11. DSP extraction for large processors (#11) -- 1-2 days per plugin
12. tache_plugins cleanup (#12) -- 1 hour

**Total estimated effort:** ~8-12 days for Phases A-C, plus ongoing Phase D work.

**Highest ROI:** Opportunities #1 and #2 together eliminate ~2,700+ lines of duplicated code across 23 plugins. They should be the first significant refactoring effort.
