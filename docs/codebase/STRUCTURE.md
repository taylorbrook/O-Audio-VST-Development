# Codebase Structure

**Analysis Date:** 2026-01-22

## Directory Layout

```
VST-development/
├── CMakeLists.txt                           # Root CMake - auto-discovers plugins
├── plugins/                                 # All plugin projects
│   ├── OuariconLyrica/                      # Physical modeling harp synthesizer (Stage 6)
│   │   ├── Source/
│   │   │   ├── PluginProcessor.cpp|h        # Main audio processor
│   │   │   ├── PluginEditor.cpp|h           # UI editor with WebView
│   │   │   ├── HarpSynthVoice.cpp|h         # Per-note synthesis voice
│   │   │   ├── HarpSynthSound.h             # Sound object definition
│   │   │   └── DSP/
│   │   │       ├── WaveguideString.cpp|h    # Bidirectional waveguide model
│   │   │       ├── PluckExciter.cpp|h       # Pluck impulse generator
│   │   │       ├── StringMaterial.cpp|h     # Material properties (Nylon, Gut, Wire, etc.)
│   │   │       ├── StiffnessFilter.cpp|h    # Allpass cascade for inharmonicity
│   │   │       ├── BodyResonance.cpp|h      # Modal body filtering
│   │   │       ├── SympatheticResonance.cpp|h # Acoustic coupling engine
│   │   │       ├── TuningEngine.cpp|h       # MIDI-to-frequency conversion
│   │   │       ├── GlissandoController.cpp|h # Pitch glissando (scale-locked or free)
│   │   │       └── VOICE_MANAGEMENT.md      # Voice allocation documentation
│   │   ├── Resources/ui/
│   │   │   ├── index.html                   # Main UI layout
│   │   │   ├── css/styles.css               # Web UI styling
│   │   │   ├── js/app.js                    # Main JavaScript controller
│   │   │   ├── js/juce/index.js             # JUCE↔WebView bridge
│   │   │   ├── js/juce/check_native_interop.js # Interop diagnostic
│   │   │   └── images/                      # UI graphics assets
│   │   ├── CMakeLists.txt                   # OuariconLyrica build config
│   │   ├── CHANGELOG.md                     # Version history
│   │   ├── .bugs/                           # Known issues tracking
│   │   ├── .contracts/                      # Contract specs
│   │   ├── .ideas/                          # Design ideas and brainstorms
│   │   └── improvements/                    # Planned improvements backlog
│   │
│   ├── OuariconMarimba/                     # Physical modeling marimba (Stage 6)
│   │   ├── Source/
│   │   │   ├── PluginProcessor.cpp|h
│   │   │   ├── PluginEditor.cpp|h
│   │   │   ├── MarimbaVoice.cpp|h
│   │   │   ├── MarimbaSound.h
│   │   │   ├── TuningEngine.h
│   │   │   ├── BodyResonance.h
│   │   │   ├── PresetManager.h
│   │   │   └── [other DSP]
│   │   └── CMakeLists.txt
│   │
│   ├── OuariconTremolo/                     # Tremolo effect (Stage 6)
│   │   ├── Source/
│   │   │   ├── PluginProcessor.cpp|h
│   │   │   ├── PluginEditor.cpp|h
│   │   │   └── ui/
│   │   └── CMakeLists.txt
│   │
│   ├── OuariconSaturationModeling/          # Saturation effect (Stage 6)
│   ├── Ouaricon Digital Delay/              # Delay effect (Stage 6)
│   ├── OuariconSimpleReverb/                # Reverb effect (Stage 6)
│   ├── OuariconAnalogEQ/                    # Parametric EQ (Stage 6)
│   ├── OuariconComp/                        # Compressor (Stage 6)
│   ├── OuariconPolystutter/                 # Beat repeater effect (Stage 6)
│   │
│   └── tache_plugins/                       # Third-party plugin collection
│       ├── AngelGrain/                      # Granular delay
│       ├── AutoClip/                        # Hard clipper
│       ├── DriveVerb/                       # Reverb variant
│       ├── MinimalKick/                     # Drum synth
│       └── [15+ additional plugins]
│
├── modules/                                 # Shared plugin components
│   ├── persistence/
│   │   └── preset-manager/cpp/
│   │       ├── OuariconPresetManager.h|cpp  # Generic preset save/load
│   │       └── [JSON serialization helpers]
│   │
│   ├── core/
│   │   └── webview-relay-manager/cpp/
│   │       └── WebViewRelayManager.h        # WebView↔APVTS bridge setup
│   │
│   ├── tuning/
│   │   └── scala-tuning-engine/cpp/
│   │       ├── OuariconTuningEngine.h       # Scala file parsing, tuning management
│   │       └── [tuning utilities]
│   │
│   ├── effects/
│   │   ├── analog-eq-unit/cpp/
│   │   │   └── AnalogEQUnit.h|cpp           # Parametric EQ DSP
│   │   └── compressor-unit/cpp/
│   │       └── CompressorUnit.h|cpp         # Dynamic range compression
│   │
│   ├── metering/
│   │   └── vu-meter/cpp/
│   │       └── VUMeterBridge.h              # Level metering for UI
│   │
│   ├── cmake/
│   │   └── OuariconModules.cmake            # CMake utilities for module inclusion
│   │
│   └── registry.yaml                        # Module registry and dependencies
│
├── build/                                   # CMake build directory
│   ├── plugins/OuariconLyrica/
│   │   ├── OuariconLyrica_artefacts/Release/
│   │   │   ├── VST3/OuariconLyrica.vst3/    # VST3 plugin (macOS)
│   │   │   ├── AU/OuariconLyrica.component/ # AU plugin (macOS)
│   │   │   └── Standalone/OuariconLyrica.app/ # Standalone executable
│   │   └── [build artifacts]
│   └── [other plugin builds]
│
├── research/                                # Background research and references
│   ├── microtonality-*.md                   # Scala/tuning system research
│   ├── physical-modeling-*.md               # Waveguide and modal synthesis
│   ├── reverb-comprehensive-research.md    # Reverb algorithm research
│   ├── delay-effects-comprehensive-guide.md
│   └── stutter-effects/                     # Beat repeater research
│
├── logs/                                    # Build and debug logs
│   └── [PluginName]/build_TIMESTAMP.log
│
├── backups/                                 # Plugin version backups
│   ├── OuariconLyrica/v1.11.1/              # Snapshot of v1.11.1 binary
│   └── [other versions]
│
├── troubleshooting/                         # Debug notes and fixes
├── scripts/                                 # Build/utility scripts
├── test-tunings/                            # Test Scala files and temperaments
├── .planning/                               # GSD planning documents
│   └── codebase/                            # Architecture analysis (generated)
│       ├── ARCHITECTURE.md                  # This analysis
│       ├── STRUCTURE.md                     # Structure documentation
│       ├── STACK.md                         # Technology stack (if generated)
│       ├── CONVENTIONS.md                   # Code conventions (if generated)
│       └── TESTING.md                       # Testing patterns (if generated)
│
├── .claude/                                 # Claude workspace state
├── .git/                                    # Git version control
├── PLUGINS.md                               # Plugin registry and status table
├── README.md                                # Project overview
└── CHANGELOG.md                             # Global version history
```

## Directory Purposes

**plugins/ (Plugin Projects):**
- Purpose: Individual VST3/AU/Standalone plugin implementations
- Contains: Processor, editor, DSP components, web UI resources per plugin
- Key files: CMakeLists.txt (build config), Source/ (C++ code), Resources/ui/ (web assets)

**plugins/OuariconLyrica/ (Reference Plugin):**
- Purpose: Most complex plugin - harp synthesizer with physical modeling, custom tuning, sympathetic resonance
- Contains: Full DSP layer (waveguide, body resonance, sympathetic coupling), tuning system, preset management
- Key files: HarpSynthVoice.h (voice architecture), WaveguideString.h (core DSP), TuningEngine.h (frequency mapping)

**plugins/OuariconLyrica/Source/DSP/ (Physical Modeling Layer):**
- Purpose: Specialized DSP modules for harp synthesis
- WaveguideString: Bidirectional digital waveguide with bridge/nut filters, loop damping, stiffness (inharmonicity) filter
- PluckExciter: Velocity-based impulse generation at pluck position
- StringMaterial: Material properties (damping, brightness multipliers) for 8 material types
- BodyResonance: Modal filtering of string output through body resonant modes
- SympatheticResonance: Acoustic coupling between voices based on frequency relationships
- TuningEngine: MIDI-to-frequency conversion supporting 12-TET, Scala files, preset temperaments, pitch bend
- GlissandoController: Smooth pitch transitions with scale-locked or free modes

**modules/ (Shared Components):**
- Purpose: Plugin-agnostic reusable code used by multiple plugins
- Contains: Preset manager, WebView relay bridges, effects units, metering utilities
- Usage: Included via include_directories() and target_link_libraries() in plugin CMakeLists.txt

**modules/persistence/preset-manager/ (Preset System):**
- Purpose: Generic JSON-based preset save/load with factory and user presets
- Location: ~/Library/Application Support/{PluginName}/Presets/ (macOS)
- Contains: Factory/ (read-only) and User/ (user-created) subdirectories
- Custom state: Callbacks for plugin-specific data (tuning files, convolution IR, etc.)

**modules/core/webview-relay-manager/ (UI Bridge):**
- Purpose: Relay managers for connecting WebView controls to APVTS parameters
- Usage: Reduces boilerplate for creating WebSliderRelay, WebComboBoxRelay instances
- Pattern: Relays created first (no dependencies), WebBrowserComponent second, attachments last

**modules/effects/analog-eq-unit/ and /compressor-unit/ (Effects):**
- Purpose: Reusable effects DSP (parametric EQ, compression) used in multiple plugins
- Location: Linked to OuariconMarimba, OuariconPolystutter, and effect chain plugins
- Interface: process() method taking AudioBuffer and block size; parameter setters for real-time control

**modules/tuning/scala-tuning-engine/ (Tuning System):**
- Purpose: Scala file parsing and tuning management (separate from plugin-level TuningEngine)
- Usage: Shared tuning utilities and file format handlers
- Future: MTS-ESP protocol support

**build/ (CMake Output):**
- Purpose: Generated build artifacts (object files, linked libraries, plugin bundles)
- Directories: plugins/[PluginName]/OuariconLyrica_artefacts/Release/{VST3,AU,Standalone}
- Installation: VST3 → ~/Library/Audio/Plug-Ins/VST3/, AU → ~/Library/Audio/Plug-Ins/Components/
- Note: build/ is in .gitignore; regenerated on each cmake build

**research/ (Reference Documentation):**
- Purpose: Background material for DSP implementation decisions
- Contains: Scala format specification, waveguide synthesis techniques, reverb algorithms, stutter effects
- Usage: Research-only; doesn't affect runtime code

**logs/ (Build Artifacts):**
- Purpose: Build logs from build-automation skill runs
- Location: logs/[PluginName]/build_TIMESTAMP.log
- Usage: Debugging build failures

**.planning/codebase/ (GSD Planning Documents):**
- Purpose: Generated architecture/structure/convention documentation
- ARCHITECTURE.md: Pattern, layers, data flow, entry points
- STRUCTURE.md: Directory layout, file purposes, naming conventions
- STACK.md: Technology stack and external dependencies
- CONVENTIONS.md: Coding style, naming patterns, error handling
- TESTING.md: Test framework, patterns, coverage

## Key File Locations

**Entry Points:**

- `plugins/OuariconLyrica/Source/PluginProcessor.cpp`: Creates parameter layout, initializes synthesiser and DSP engines
- `plugins/OuariconLyrica/Source/PluginProcessor.h`: AudioProcessor class definition; owns APVTS, TuningEngine, SympatheticResonanceEngine
- `plugins/OuariconLyrica/Source/PluginEditor.cpp`: Creates WebView, relays, parameter attachments; editor UI lifecycle
- `plugins/OuariconLyrica/Source/PluginEditor.h`: Editor class; defines relay and attachment members (critical destruction order)

**Configuration:**

- `CMakeLists.txt` (root): Discovers and adds all plugin subdirectories
- `plugins/OuariconLyrica/CMakeLists.txt`: OuariconLyrica build config; source files, include paths, JUCE modules
- `modules/cmake/OuariconModules.cmake`: CMake utilities for standard module inclusion
- `modules/registry.yaml`: Module dependency graph and inclusion rules

**Core Logic:**

- `plugins/OuariconLyrica/Source/DSP/WaveguideString.h|cpp`: Bidirectional waveguide implementation (~400 lines)
- `plugins/OuariconLyrica/Source/DSP/TuningEngine.h|cpp`: Frequency calculation and tuning system (~800 lines)
- `plugins/OuariconLyrica/Source/DSP/SympatheticResonance.h|cpp`: Acoustic coupling (~400 lines)
- `plugins/OuariconLyrica/Source/DSP/BodyResonance.h|cpp`: Modal filtering (~200 lines)
- `plugins/OuariconLyrica/Source/HarpSynthVoice.h|cpp`: Per-note rendering (~300 lines)

**Testing:**

- Not detected: No dedicated test directory structure
- Note: Testing done via manual plugin instantiation in DAW; some plugins have `.bugs/` tracking known issues

**Plugin Resources:**

- `plugins/OuariconLyrica/Resources/ui/index.html`: WebView layout (HTML5)
- `plugins/OuariconLyrica/Resources/ui/css/styles.css`: Web UI styling
- `plugins/OuariconLyrica/Resources/ui/js/app.js`: UI logic (parameter binding, tuning circle, keyboard visualization)
- `plugins/OuariconLyrica/Resources/ui/js/juce/index.js`: JUCE↔WebView JavaScript bridge
- Embedded in binary via `juce_add_binary_data()` in CMakeLists.txt

**Preset/Persistence:**

- `modules/persistence/preset-manager/cpp/OuariconPresetManager.h|cpp`: Generic preset manager
- User presets: `~/Library/Application Support/{PluginName}/Presets/User/`
- Factory presets: Shipped with plugin bundle

**Plugin Status/Documentation:**

- `PLUGINS.md`: Registry of all plugins with version and status
- `plugins/OuariconLyrica/CHANGELOG.md`: Version history and release notes
- `plugins/OuariconLyrica/.bugs/`: Known issues and fixes
- `plugins/OuariconLyrica/.ideas/`: Design ideas and brainstorms

## Naming Conventions

**Files:**

- `PluginProcessor.cpp|h`: Main audio processor (JUCE convention)
- `PluginEditor.cpp|h`: UI editor class (JUCE convention)
- `[VoiceName]Voice.cpp|h`: Per-note voice implementation (SynthesiserVoice subclass)
- `[VoiceName]Sound.h`: Sound definition (no .cpp; typically header-only)
- `[DspName].cpp|h`: Specialized DSP component (e.g., WaveguideString, TuningEngine)
- `[ManagerName]Manager.h|cpp`: Manager/coordinator class (e.g., PresetManager)

**Directories:**

- `Source/`: C++ source code (always under plugin root)
- `DSP/`: Physical modeling and signal processing modules (under Source/)
- `Resources/ui/`: Web UI assets (HTML, CSS, JS, images)
- `build/`: CMake output (generated, not committed)
- `modules/[category]/[module]/cpp/`: Shared module source (category = persistence, effects, tuning, etc.)

**Classes:**

- `Ouaricon[PluginName]AudioProcessor`: Main processor class
- `Ouaricon[PluginName]AudioProcessorEditor`: Editor class
- `[Instrument]Voice`: Voice class (HarpSynthVoice, MarimbaVoice)
- `[Instrument]Sound`: Sound class
- `[ComponentName]Engine`: Major DSP engine (TuningEngine, SympatheticResonanceEngine)

**Parameters (APVTS ParameterID):**

- camelCase format (e.g., "masterVolume", "stringMaterial", "pluckPosition")
- Descriptive names matching UI labels

**Member Variables:**

- camelCase (e.g., `currentFrequency`, `stringModel`, `sympatheticEngine`)
- Private members (audio thread): no prefix (per JUCE convention)
- Pointers to shared state: suffixed with Engine or Manager (e.g., `tuningEngine`, `presetManager`)

## Where to Add New Code

**New Feature (Parameter + DSP):**

Example: Adding "resonant body pre-filter" to OuariconLyrica

1. **Parameter Definition:**
   - Edit: `plugins/OuariconLyrica/Source/PluginProcessor.cpp` → createParameterLayout()
   - Add: `AudioParameterFloat` for new parameter (e.g., "bodyPreFilterQ")
   - Format: Use ParameterID with version suffix, NormalisableRange, default value

2. **Voice Implementation:**
   - Edit: `plugins/OuariconLyrica/Source/HarpSynthVoice.cpp` → startNote() or renderNextBlock()
   - Read parameter: `parameters->getRawParameterValue("bodyPreFilterQ")->load()`
   - Apply to DSP: Call setter on relevant component (e.g., `bodyResonance.setPreFilterQ()`)

3. **DSP Component:**
   - Edit or Create: `plugins/OuariconLyrica/Source/DSP/BodyResonance.h|cpp`
   - Add method: `void setPreFilterQ(float Q)` and member to store Q
   - Implement: Apply Q change in prepare() or process()

4. **UI Binding:**
   - Edit: `plugins/OuariconLyrica/Source/PluginEditor.h` (add relay and attachment)
   - Pattern: Create WebSliderRelay, then WebSliderParameterAttachment (order matters!)
   - Edit: `plugins/OuariconLyrica/Resources/ui/js/app.js` (add slider HTML and event handler)

5. **Testing:**
   - Adjust parameter in DAW, listen for sonic change
   - Verify preset save/load preserves value

**New Component/Module:**

Example: Adding "frequency analyzer" DSP module

1. **Header File:**
   - Create: `plugins/OuariconLyrica/Source/DSP/FrequencyAnalyzer.h`
   - Include JUCE DSP utilities, define class with prepare(), processSample(), getResult() methods

2. **Implementation:**
   - Create: `plugins/OuariconLyrica/Source/DSP/FrequencyAnalyzer.cpp`

3. **Integration:**
   - Edit: `plugins/OuariconLyrica/Source/HarpSynthVoice.h` → add member `FrequencyAnalyzer analyzer;`
   - Edit: HarpSynthVoice.cpp → initialize in prepare(), call processSample() in renderNextBlock()
   - Edit: CMakeLists.txt → add FrequencyAnalyzer.cpp to target_sources()

**Shared Module (Used by Multiple Plugins):**

Example: New preset serialization format

1. **Create Module Directory:**
   - `modules/persistence/new-format/cpp/`

2. **Header:**
   - `modules/persistence/new-format/cpp/NewFormatPresetManager.h`

3. **Implementation:**
   - `modules/persistence/new-format/cpp/NewFormatPresetManager.cpp`

4. **Include in Plugin:**
   - Edit: `plugins/OuariconLyrica/CMakeLists.txt`
   - Add: `target_include_directories(OuariconLyrica PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../modules/persistence/new-format/cpp)`
   - Add: Source file to target_sources()

5. **Dependency Registration:**
   - Edit: `modules/registry.yaml` → add new module entry

**Utilities (Helper Functions):**

- **Plugin-Local Utilities:**
  - Create: `plugins/[PluginName]/Source/Utilities.h|cpp`
  - Include in PluginProcessor.h

- **Shared Utilities:**
  - Create: `modules/utilities/common/cpp/Utilities.h`
  - Link via CMakeLists.txt

## Special Directories

**build/ (CMake Generated):**
- Purpose: Compilation artifacts and linked plugin bundles
- Generated: Running cmake --build build
- Committed: No (.gitignore)
- Regenerated: Each build

**Resources/ui/ (Web Assets):**
- Purpose: HTML, CSS, JavaScript for WebView UI
- Generated: No (hand-authored)
- Committed: Yes
- Embedded: Via juce_add_binary_data() into plugin binary

**modules/cmake/ (Build System):**
- Purpose: CMake helper functions and macros
- Generated: No (hand-authored)
- Committed: Yes
- Usage: Included by plugin CMakeLists.txt files

**.planning/codebase/ (GSD Documentation):**
- Purpose: Architecture and structure analysis for future development
- Generated: Yes (by /gsd:map-codebase command)
- Committed: Yes (reference documents)
- Usage: Loaded by /gsd:plan-phase to guide implementation planning

**research/ (Background Material):**
- Purpose: DSP algorithm research and format specifications
- Generated: No (hand-authored reference material)
- Committed: Yes
- Usage: Reference only; doesn't affect runtime

**logs/ (Build Output):**
- Purpose: Build system logs from CMake and compiler
- Generated: Yes (automated build script)
- Committed: No (.gitignore)
- Cleaned: Periodically

---

*Structure analysis: 2026-01-22*
