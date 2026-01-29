# Architecture

**Analysis Date:** 2026-01-29

## Pattern Overview

**Overall:** Layered processor-editor pattern with modular DSP components

This is a JUCE 8-based audio plugin framework following the standard VST3/AU architecture. Each plugin follows a strict separation between audio processing (real-time safe) and UI interaction (WebView-based). The architecture emphasizes:

- **Single-Responsibility**: Processor handles audio, Editor handles UI, DSP components handle algorithms
- **Real-Time Safety**: All audio work in `processBlock()` is RT-safe; UI communication via atomics
- **Parameter Management**: Centralized `AudioProcessorValueTreeState` with value tree-based state
- **WebView Integration**: HTML/CSS/JS UI communicates with C++ via parameter relays and native functions

**Key Characteristics:**
- Plugin inherits from `juce::AudioProcessor` with mandatory editor
- All audio parameters defined in `createParameterLayout()` and stored in APVTS
- Intermediate buffers pre-allocated in `prepareToPlay()` (zero allocation in processBlock)
- WebView UI rendered via `juce::WebBrowserComponent` with binary-embedded resources
- Thread-safe metering via `std::atomic<float>` for UI display
- Preset management through `OuariconPresetManager` (JSON-based persistence)

## Layers

**Audio Processing (Real-Time Safe):**
- Purpose: Process incoming audio samples in low-latency, lock-free manner
- Location: `plugins/[PluginName]/Source/PluginProcessor.cpp`, `Source/DSP/` subdirectory
- Contains: `processBlock()` implementation, DSP component processing, buffer management
- Depends on: JUCE audio modules (`juce_audio_processors`, `juce_dsp`), DSP components
- Used by: DAW's audio engine (triggered via `processBlock()` callback)

**DSP Components (Algorithms):**
- Purpose: Encapsulate reusable audio processing algorithms (filters, generators, modulators)
- Location: `plugins/[PluginName]/Source/DSP/` (e.g., `CrossoverFilter.h`, `HarmonicGenerator.h`)
- Contains: Stateful filter objects, envelope followers, pitch trackers, harmonic generators
- Depends on: JUCE DSP module (`juce_dsp`), standard C++ math libraries
- Used by: Processor's `processBlock()` and `prepareToPlay()`
- Pattern: Each component has `prepare(ProcessSpec)`, `reset()`, `process()`, configuration setters
- Thread Model: Stateless processing methods, configuration changes via atomic flags or smoothed values

**Parameter Management (State):**
- Purpose: Define all plugin parameters, manage state serialization, expose to DAW
- Location: `plugins/[PluginName]/Source/PluginProcessor.h` (APVTS member), `createParameterLayout()`
- Contains: Parameter definitions (Float, Choice, Bool), default values, ranges, skewing
- Depends on: `juce::AudioProcessorValueTreeState`, parameter types from JUCE 8
- Used by: Editor for binding, Processor for reading during audio processing, DAW for automation
- Format: XML serialization in DAW projects, JSON in factory/user presets

**UI/Editor (Non-Real-Time):**
- Purpose: Provide visual interface and handle user interaction
- Location: `plugins/[PluginName]/Source/PluginEditor.cpp/h`, `Source/ui/public/` (HTML/CSS/JS)
- Contains: WebView component, parameter relays, attachment lifecycle management
- Depends on: `juce_gui_extra` (WebBrowserComponent), preset manager, parameter relays
- Used by: DAW's host window system, user interaction events
- Pattern: Editor creates and destroys relays/attachments safely, WebView displays UI resources

**Preset/Session Persistence:**
- Purpose: Save/restore plugin state, manage factory and user presets
- Location: `modules/persistence/preset-manager/cpp/OuariconPresetManager.h` (shared module)
- Contains: JSON preset file I/O, APVTS parameter serialization, preset navigation
- Depends on: File system, APVTS
- Used by: Editor UI functions (savePreset, loadPreset via native calls), DAW session restore

**WebView Resource Management:**
- Purpose: Serve HTML/CSS/JS files to WebView, handle binary data embedding
- Location: CMakeLists.txt (juce_add_binary_data), `Source/ui/` directory
- Contains: index.html (UI layout), js/juce/index.js (relay binding), img/* (images), modules/* (preset-manager.js)
- Depends on: JUCE binary data embedding system
- Used by: Editor's WebBrowserComponent via resource provider pattern

## Data Flow

**Initialization (Plugin Load):**

1. DAW instantiates plugin (calls constructor)
2. `OuariconAudioProcessor()` constructor runs:
   - Creates `AudioProcessorValueTreeState` with parameter layout
   - Initializes `OuariconPresetManager`
   - Sets factory presets
3. DAW calls `prepareToPlay(sampleRate, blockSize)`:
   - Configure `ProcessSpec` with sample rate, block size, channel count
   - Pre-allocate intermediate buffers (low/high band splits, mono processing buffers)
   - Prepare all DSP components (`crossover.prepare()`, `cleanModeProcessor.prepare()`, etc.)
   - Initialize smoothed value objects with attack/release times
   - Report latency to host if applicable
4. DAW creates editor, calls `createEditor()`:
   - Editor constructor creates parameter relays
   - Editor creates WebView component
   - Editor creates parameter attachments (Relay → APVTS binding)
   - Editor triggers WebView navigation to "index.html"

**Audio Processing (Real-Time Loop):**

1. DAW calls `processBlock(audioBuffer, midiBuffer)` at regular intervals (typically every 5-512 samples)
2. Processor reads parameter values from APVTS:
   - `parameters.getRawParameterValue("param_name")->load()` (atomic read)
   - Parameters may be automated by DAW (envelope, LFO, etc.)
3. Smooth value changes to prevent clicks:
   - `smoothedEnhance.setTargetValue(newValue)` sets target
   - Processor iterates within block, smoothing from current to target
4. Process audio through DSP pipeline:
   - Split into bands: `crossover.process(stereoBuffer, lowBandBuffer, highBandBuffer)`
   - Process low band: `monoSummer.process()` to mix L/R to mono
   - Generate harmonics: `harmonicGenerator.process()` on mono low
   - Process high band as-is
   - Recombine bands back to stereo
5. Apply gain and limiting based on output level
6. Write processed audio back to audioBuffer (in-place or separate output)
7. Update metering atomics for UI display:
   - `outputLevelDB.store(calculateLevel(buffer))`
   - `limitIndicator.store(limitAmount)`
8. Return (DAW renders these samples)

**UI Interaction (Non-Real-Time):**

1. User moves slider in WebView
2. HTML/JS calls relay function: `juce_slider_id.valueAsString = newValue`
3. Relay pushes value change to C++ parameter (APVTS)
4. Attachment synchronizes parameter → UI if changed from DAW automation
5. Processor picks up new value on next `processBlock()` iteration

**Metering (Cross-Thread Communication):**

1. Processor thread calculates output level in `processBlock()`:
   - `float peakLevel = calculateMagnitude(audioBuffer)`
   - `outputLevelDB.store(juce::Decibels::gainToDecibels(peakLevel))`
2. Timer callback in Editor (runs on message thread, typically 30Hz):
   - `float currentLevel = processorRef.getOutputLevelDB()`
   - Calls JavaScript to update UI meter: `juce_call_evaluateJavascript("updateMeter(" + level + ")")`

**Preset Save/Load:**

1. User clicks "Save Preset" in UI
2. JavaScript calls native function: `savePreset(presetName)`
3. PresetManager captures current APVTS state, serializes to JSON:
   ```json
   { "crossover_freq": 0.25, "enhance": 0.5, "output": 0.5 }
   ```
4. Writes JSON to `~/Music/[PluginName] Presets/UserPresets/[name].json`
5. On load, reads JSON and calls `parameters.getReferenceToParameter().setValueNotifyingHost()`
   - Updates APVTS, triggers parameter attachments
   - Processor sees new values on next `processBlock()`

## Key Abstractions

**AudioProcessor (Plugin Interface):**
- Purpose: Primary plugin entry point, host communication
- Examples: `OBassAudioProcessor`, `OCompAudioProcessor`, `OAnalogSaturationAudioProcessor`
- Location: `plugins/[PluginName]/Source/PluginProcessor.h`
- Pattern:
  - Mandatory methods: `prepareToPlay()`, `processBlock()`, `releaseResources()`, `createEditor()`
  - Mandatory properties: `parameters` (APVTS), `presetManager`
  - Optional meters: `std::atomic<float>` meter values, getter methods
  - Helper: `createParameterLayout()` static method returns parameter definitions

**DSP Component (Algorithm Encapsulation):**
- Purpose: Reusable, configurable audio processing unit
- Examples: `CrossoverFilter`, `HarmonicGenerator`, `EnvelopeFollower`, `PitchTracker`
- Location: `plugins/[PluginName]/Source/DSP/[ComponentName].h`
- Pattern:
  - Lifecycle: `prepare(ProcessSpec spec)` → `process(buffer)` → `reset()`
  - Configuration: `setMode()`, `setCutoffFrequency()`, etc. (atomic setters if RT-unsafe)
  - State: Member variables for filter coefficients, oscillator phase, envelope state
  - No allocation in `process()` - all buffers pre-allocated

**Parameter Relay (UI↔Audio Bridge):**
- Purpose: Connect HTML slider/toggle to C++ parameter
- Types: `juce::WebSliderRelay`, `juce::WebToggleButtonRelay`
- Pattern:
  - Created in Editor constructor with options: `WebSliderRelay::Options::withRange(min, max)`
  - Attached to APVTS parameter via `WebSliderParameterAttachment` binding
  - HTML/JS reads/writes via element ID: `juce.slider_id.valueAsString = newValue`
  - Bidirectional: DAW automation updates UI, user input updates parameter

**PresetManager (State Persistence):**
- Purpose: Handle save/load/navigation of factory and user presets
- Pattern:
  - Factory presets initialized at plugin startup with predefined parameter values
  - User presets stored as JSON files in user's preset directory
  - Native functions exposed to JavaScript: `savePreset(name)`, `loadPreset(name)`, `getPresetList()`
  - Serialization: Captures normalized parameter values (0.0-1.0 range)

**WebView Resource Provider (Asset Serving):**
- Purpose: Serve embedded binary resources to WebBrowserComponent
- Pattern:
  - CMakeLists.txt uses `juce_add_binary_data()` to embed HTML/CSS/JS/images
  - Editor's `getResource()` method intercepts requests and returns BinaryData entries
  - MIME type mapping: "index.html" → "text/html", "*.js" → "text/javascript", "*.css" → "text/css"

## Entry Points

**Plugin Constructor:**
- Location: `plugins/[PluginName]/Source/PluginProcessor.cpp` constructor
- Triggers: DAW instantiates plugin (when plugin is scanned or loaded into session)
- Responsibilities:
  - Initialize `AudioProcessorValueTreeState` with parameter layout
  - Configure bus setup (stereo input/output, MIDI if applicable)
  - Initialize `PresetManager` with factory presets
  - Set initial DSP component configurations

**prepareToPlay():**
- Location: `plugins/[PluginName]/Source/PluginProcessor.cpp`
- Triggers: DAW determines session sample rate/block size (before first `processBlock()`)
- Responsibilities:
  - Pre-allocate intermediate buffers at determined block size
  - Prepare all DSP components with actual sample rate/block size
  - Initialize smoothed value objects with correct sample rate
  - Report plugin latency to host (if applicable)
  - Verify no allocation will happen in audio thread

**processBlock():**
- Location: `plugins/[PluginName]/Source/PluginProcessor.cpp`
- Triggers: Audio playhead advances (typically every 512 samples or variable block size)
- Responsibilities:
  - Read current parameter values (atomic reads from APVTS)
  - Process audio through DSP pipeline (in real-time safe manner)
  - Update metering atomics for UI display
  - Handle parameter changes mid-block (smoothing to avoid clicks)
  - Clear unused output channels

**createEditor():**
- Location: `plugins/[PluginName]/Source/PluginProcessor.cpp`
- Triggers: DAW opens plugin UI window
- Responsibilities:
  - Instantiate `AudioProcessorEditor` subclass
  - Editor constructor creates relays, WebView, attachments
  - Return editor for DAW to add to host window
  - Return nullptr if plugin is headless (DAW-provided controls only)

**PluginEditor Constructor:**
- Location: `plugins/[PluginName]/Source/PluginEditor.cpp`
- Triggers: `createEditor()` instantiates editor
- Responsibilities:
  - Create parameter relays in correct order (Relays → WebView → Attachments)
  - Create WebView with resource provider
  - Create parameter attachments binding relays to APVTS parameters
  - Set editor window size
  - Schedule WebView navigation (via `parentHierarchyChanged()`)

**getStateInformation() / setStateInformation():**
- Location: `plugins/[PluginName]/Source/PluginProcessor.cpp`
- Triggers: DAW saves session or saves plugin state
- Responsibilities:
  - Serialize APVTS state to binary block (getStateInformation)
  - Deserialize binary block and restore APVTS (setStateInformation)
  - Handle version compatibility for future plugin versions
  - PresetManager handles JSON presets separately (not DAW state)

## Error Handling

**Strategy:** Defensive programming with logging, fallback to defaults, assertion for internal errors

**Patterns:**
- **Parameter Out-of-Range**: Clamped by `NormalisableRange` in parameter definition, no crash
- **Missing Preset File**: PresetManager falls back to factory preset, logs warning
- **WebView Resource Not Found**: Returns empty resource, HTML/JS handles gracefully
- **Allocation Failure in prepareToPlay()**: Throws exception (safe before audio thread starts)
- **Allocation in processBlock()**: Assertion failure in debug builds only (would underrun in release)
- **NaN/Inf in Audio**: Checked via `juce::ScopedNoDenormals` (subnormal numbers converted to zero)
- **Release Build Crashes**: Typically caused by member destruction order (relays destroyed before WebView) or evaluateJavascript called on destroyed WebView (prevented via attachment order pattern)

## Cross-Cutting Concerns

**Logging:**
- Uses `DBG()` macro for debug output (output to IDE console, stripped in release)
- Critical errors written via `std::cerr` for troubleshooting
- Metering updates do NOT log (would spam audio thread)

**Validation:**
- Parameter ranges validated at definition time (NormalisableRange enforces bounds)
- DSP component state validated in `prepare()` (e.g., buffer sizes match)
- Audio buffer sizes asserted to match in `processBlock()`
- APVTS state integrity checked on load (corrupted presets fall back to defaults)

**Authentication:**
- Not applicable (single-user plugin, no network access)

**Real-Time Safety:**
- `juce::ScopedNoDenormals` used at start of `processBlock()` to prevent subnormal underflow
- Atomic operations for metering (lock-free cross-thread communication)
- SmoothedValue objects used for parameter changes to prevent clicks
- Buffers pre-allocated to prevent allocation in audio thread
- No blocking I/O, no mutex locks, no blocking allocations in processBlock()
- Parameter reads use `getRawParameterValue()->load()` for atomic access

---

*Architecture analysis: 2026-01-29*
