# Codebase Structure

**Analysis Date:** 2026-01-29

## Directory Layout

```
vst-development/
├── CMakeLists.txt                  # Root CMake: auto-discovers plugins, links JUCE
├── README.md                        # Project overview and workflows
├── PLUGINS.md                       # Plugin registry with status
├── CLAUDE.md                        # Build requirements and critical patterns
├── build/                           # Build artifacts (generated, not committed)
│   ├── JUCE/                        # JUCE framework (CMake output)
│   ├── plugins/                     # Compiled binaries and objects
│   │   ├── O-Bass/
│   │   │   └── O-Bass_artefacts/
│   │   │       └── Release/
│   │   │           ├── VST3/O-Bass.vst3
│   │   │           └── AU/O-Bass.component
│   │   └── [other plugins]/
│   └── CMakeCache.txt, build.ninja
├── plugins/                         # Active plugin sources
│   ├── O-Bass/                      # Bass enhancement plugin (mature)
│   ├── O-Comp/                      # Compressor plugin
│   ├── O-MultiBandCompressor/       # Multiband compressor with spectrum
│   ├── O-AnalogSaturation/          # Saturation/distortion
│   ├── O-AnalogEQ/                  # Parametric EQ
│   ├── O-SimpleReverb/              # Reverb processor
│   ├── O-DigiDelay/                 # Digital delay
│   ├── O-Tremolo/                   # Tremolo modulation
│   ├── O-Marimba/                   # Synth instrument (polyphonic)
│   ├── O-Lyrica/                    # Lyrical synthesis plugin
│   ├── O-Polystutter/               # Polyphonic stutter effect
│   ├── O-IntonationPad/             # Pitch correction tool
│   └── tache_plugins/               # Teaching reference plugins
├── modules/                         # Reusable code components (shared)
│   ├── core/
│   │   ├── webview-relay-manager/   # WebView relay lifecycle (crash prevention)
│   │   └── resource-provider/       # HTML/CSS/JS resource serving
│   ├── persistence/
│   │   └── preset-manager/          # JSON preset save/load
│   ├── metering/
│   │   └── vu-meter/                # VU meter visualization bridge
│   ├── tuning/
│   │   └── scala-tuning-engine/     # Scala file parsing and MTS-ESP
│   ├── effects/
│   │   ├── analog-eq-unit/          # EQ filters
│   │   ├── compressor-unit/         # Compressor DSP
│   │   └── [other effects]/
│   ├── synthesis/                   # Sound generation
│   ├── ui/                          # UI components
│   ├── cmake/                       # CMake utility files
│   ├── scripts/                     # Module helper scripts
│   └── registry.yaml                # Module index and metadata
├── research/                        # Algorithm and domain research
│   ├── stutter-effects/
│   └── [other research]/
├── troubleshooting/                 # Knowledge base (indexed)
│   ├── build-failures/
│   ├── runtime-issues/
│   ├── gui-issues/
│   ├── dsp-issues/
│   └── patterns/
│       └── juce8-critical-patterns.md  # Required reading for subagents
├── .planning/                       # GSD project planning
│   ├── codebase/                    # Architecture/structure docs (THIS DIRECTORY)
│   │   ├── ARCHITECTURE.md
│   │   └── STRUCTURE.md
│   └── [will contain phase-level docs]
├── .claude/                         # AI assistant system files
│   ├── agents/                      # Specialized agent prompts
│   ├── skills/                      # Workflow automations
│   ├── commands/                    # Slash command definitions
│   ├── templates/                   # Code snippets and patterns
│   └── hooks/                       # Validation gates
├── scripts/                         # Build and utility scripts
│   ├── build-and-install.sh         # 7-phase build pipeline
│   └── verify-backup.sh
├── backups/                         # Version history (plugin snapshots)
│   ├── OuariconMarimba/
│   │   ├── v1.0.0/
│   │   ├── v1.5.0/
│   │   └── [versions]/
│   └── [other archived plugins]/
├── docs/                            # Project documentation
├── logs/                            # Build logs and CI output
└── test-tunings/                    # Scala tuning files for testing
```

## Directory Purposes

**vst-development/:**
- Purpose: Project root, contains all plugins and shared infrastructure
- Contains: CMake config, plugin sources, build system, documentation
- Key files: CMakeLists.txt (root CMake), README.md (overview), PLUGINS.md (status)

**build/:**
- Purpose: Build artifacts and compiled binaries
- Contains: Generated Ninja/CMake files, compiled plugin binaries (VST3/AU/Standalone), object files
- Generated: YES (cleared by `rm -rf build/`)
- Committed: NO (git-ignored)
- Structure: Mirrors plugin layout, each plugin gets `[PluginName]_artefacts/Release/` folder

**plugins/:**
- Purpose: All active plugin source code
- Contains: Individual plugin directories, each with Source/, CMakeLists.txt, .planning/
- Key files: `plugins/[PluginName]/CMakeLists.txt` (per-plugin CMake config)

**plugins/[PluginName]/:**
- Purpose: Single plugin's implementation
- Contains: Source/ (C++), CMakeLists.txt, .planning/ (design docs), optional .ideas/ (mockups)
- Structure:
  - `Source/PluginProcessor.h/cpp` - Audio engine and parameter management
  - `Source/PluginEditor.h/cpp` - WebView UI and relay management
  - `Source/DSP/` - Algorithm components (filters, generators, modulators)
  - `Source/ui/public/` - HTML/CSS/JS resources
  - `Source/ui/public/js/juce/` - JUCE parameter relay binding code
  - `Source/ui/public/modules/` - Shared JS modules (preset-manager.js)
  - `CMakeLists.txt` - Plugin build configuration
  - `.planning/` - GSD-style stage documentation

**plugins/[PluginName]/Source/:**
- Purpose: C++ implementation
- Key files:
  - `PluginProcessor.h` - AudioProcessor subclass, APVTS, parameter layout
  - `PluginProcessor.cpp` - Constructor, prepareToPlay, processBlock, state serialization
  - `PluginEditor.h` - AudioProcessorEditor, relay management
  - `PluginEditor.cpp` - WebView initialization, resource provider, parameter attachment
  - `OuariconPresetManager.h` - Preset persistence (included from modules/)
  - `DSP/[ComponentName].h` - Individual algorithm implementations

**plugins/[PluginName]/Source/DSP/:**
- Purpose: Audio processing algorithms and components
- Contains: One class per file (CrossoverFilter, HarmonicGenerator, EnvelopeFollower, etc.)
- Pattern: Each component has `prepare()`, `process()`, `reset()`, configuration setters

**plugins/[PluginName]/Source/ui/public/:**
- Purpose: WebView UI resources (HTML, CSS, JS, images)
- Contains:
  - `index.html` - Main UI layout and structure
  - `js/juce/index.js` - Parameter relay binding (JavaScript ↔ C++)
  - `js/juce/check_native_interop.js` - Validates native function availability
  - `js/app.js` - Optional custom JavaScript logic
  - `modules/preset-manager.js` - Preset UI functions
  - `img/` - Background images, logos, icons
  - `css/styles.css` - Optional stylesheet (some plugins use inline styles)

**modules/:**
- Purpose: Shared, reusable code components
- Contains: Core infrastructure, effects, synthesis, metering, UI components
- Registry: `modules/registry.yaml` lists all modules with versioning and metadata
- Usage: Plugins include modules via CMakeLists.txt `target_include_directories()` and `target_link_libraries()`
- Examples:
  - `modules/persistence/preset-manager/` - Used by all plugins with preset support
  - `modules/core/webview-relay-manager/` - Used by all WebView plugins
  - `modules/effects/compressor-unit/` - Used by O-Comp, O-MultiBandCompressor
  - `modules/tuning/scala-tuning-engine/` - Used by O-Marimba, O-Lyrica

**modules/core/:**
- Purpose: Foundation infrastructure for all plugins
- WebViewRelayManager: Manages relay lifecycle (prevents crash from destruction order)
- ResourceProvider: Serves embedded binary data to WebBrowserComponent

**modules/persistence/:**
- Purpose: State management and persistence
- PresetManager: JSON-based factory/user preset management

**modules/metering/:**
- Purpose: Audio level visualization
- VUMeterBridge: Cross-thread communication for metering updates

**modules/tuning/:**
- Purpose: Pitch and tuning systems
- ScalaTuningEngine: Parses Scala tuning files, generates note tables

**modules/effects/:**
- Purpose: DSP algorithm implementations
- CompressorUnit, AnalogEQUnit, etc. - Reusable processors

**research/:**
- Purpose: Algorithm and domain research documents
- Contains: Technical deep-dives on stutter effects, synthesis algorithms, etc.
- Not code: These are reference materials and design documents

**troubleshooting/:**
- Purpose: Knowledge base of solved problems
- Structure:
  - `build-failures/` - CMake, compilation, linker errors with solutions
  - `runtime-issues/` - Crashes, hangs, audio artifacts with solutions
  - `gui-issues/` - WebView, UI binding, relay lifecycle problems with solutions
  - `dsp-issues/` - Audio algorithm issues, numerical instability with solutions
  - `patterns/juce8-critical-patterns.md` - "Required Reading" injected into subagent contexts
- Auto-indexed: Subagents search this before attempting fixes

**.planning/codebase/:**
- Purpose: Architecture and structure documentation
- Contains: ARCHITECTURE.md, STRUCTURE.md (THIS FILE)
- Used by: GSD planning agents to understand existing patterns before generating code

**.claude/:**
- Purpose: AI system configuration and automation
- agents/ - Specialized prompts for foundation-shell, dsp, gui, validation agents
- skills/ - Complex workflow automations (plugin lifecycle, deep research, etc.)
- templates/ - Code snippets and patterns for reuse
- hooks/ - Validation gates triggered after each build phase

**scripts/:**
- Purpose: Automated build and maintenance
- `build-and-install.sh` - 7-phase pipeline: prepare → CMake → compile → validate → install → test → report

**backups/:**
- Purpose: Version history of archived plugins
- Structure: `backups/[PluginName]/v[version]/` with full source from each release
- Use: Rollback to previous versions, reference old implementations

## Key File Locations

**Entry Points:**
- `CMakeLists.txt`: Root build configuration (auto-discovers plugins via `file(GLOB)`)
- `plugins/[PluginName]/CMakeLists.txt`: Per-plugin build target (juce_add_plugin)
- `plugins/[PluginName]/Source/PluginProcessor.cpp`: Plugin constructor (DAW entry point)

**Configuration:**
- `.claudemD`: Build requirements and critical patterns (MUST READ before building)
- `CLAUDE.md`: Plugin cache clearing sequence (MUST RUN after each ninja build)
- `modules/registry.yaml`: Module metadata and dependencies
- `plugins/[PluginName]/CMakeLists.txt`: Plugin-specific format settings, resource embedding

**Core Logic:**
- `plugins/[PluginName]/Source/PluginProcessor.h`: Parameter definitions, DSP component members
- `plugins/[PluginName]/Source/PluginProcessor.cpp`: processBlock implementation, state management
- `plugins/[PluginName]/Source/DSP/[Component].h`: Algorithm implementations
- `plugins/[PluginName]/Source/PluginEditor.cpp`: WebView lifecycle, relay/attachment setup

**Testing & Validation:**
- `scripts/build-and-install.sh`: Automated validation pipeline
- `troubleshooting/patterns/juce8-critical-patterns.md`: Required pattern reading

**UI Assets:**
- `plugins/[PluginName]/Source/ui/public/index.html`: UI layout
- `plugins/[PluginName]/Source/ui/public/js/juce/index.js`: Parameter relay binding

## Naming Conventions

**Files:**
- `PluginProcessor.h/cpp` - Core audio processor (required by all plugins)
- `PluginEditor.h/cpp` - WebView editor (required by all plugins with UI)
- `[ComponentName].h/cpp` - DSP components in Source/DSP/ (e.g., CrossoverFilter.h)
- `[ComponentName]Bridge.h` - Wrapper exposing C++ to JavaScript (e.g., VUMeterBridge.h)
- `index.html`, `index.js` - Standard names for UI entry points
- `preset-manager.js` - Preset UI module (shared across plugins)

**Directories:**
- `Source/` - C++ implementation (mandatory in each plugin)
- `Source/DSP/` - Algorithm components (optional, created when needed)
- `Source/ui/` - WebView resources (required if WebView editor)
- `Source/ui/public/` - Binary-embedded resources
- `Source/ui/public/js/juce/` - JUCE interaction code
- `Source/ui/public/modules/` - Shared JS modules
- `Source/ui/public/img/` - Image resources
- `.planning/` - GSD planning (optional, created during development)
- `.ideas/` - Visual mockups and brainstorms (optional)

**Classes:**
- `O[FeatureName]AudioProcessor` - Plugin processor class (e.g., OBassAudioProcessor)
- `O[FeatureName]AudioProcessorEditor` - Plugin editor class
- `[AlgorithmName]` - DSP component (no O prefix, e.g., CrossoverFilter, HarmonicGenerator)
- DSP Component members follow pattern: `iir[Mode]`, `fir[Mode]`, `smoothed[Parameter]`

**Parameters:**
- `parameter_name` - Lowercase with underscores in parameter ID
- Parameter ID version suffix: `{ "parameter_name", 1 }` (increment for breaking changes)

**Constants:**
- `k[Name]` - Constant prefix (e.g., `kMinFreq`, `kMaxFreq`, `kNumPrecomputedFilters`)
- `FFT_SIZE`, `FFT_ORDER` - All-caps for compile-time constants

## Where to Add New Code

**New Feature (DSP Algorithm):**
- Primary code: `plugins/[PluginName]/Source/DSP/[NewComponentName].h/cpp`
  - Implement `prepare(ProcessSpec)`, `process(buffer)`, `reset()` methods
  - Use atomic operations for RT-unsafe config changes
- Register in processor: Add to `plugins/[PluginName]/Source/PluginProcessor.h` as member
- Call in processBlock: `plugins/[PluginName]/Source/PluginProcessor.cpp` in audio pipeline
- Update CMakeLists: Add `.cpp` file to `target_sources()` if new `.cpp` file created

**New Parameter:**
- Definition: `plugins/[PluginName]/Source/PluginProcessor.cpp` in `createParameterLayout()`
  - Choose type: `AudioParameterFloat`, `AudioParameterChoice`, `AudioParameterBool`
  - Set range, skewing, default, suffix
- Reading: In `processBlock()`, call `parameters.getRawParameterValue("param_id")->load()`
- Smoothing: If automation-sensitive, use `SmoothedValue<float>` for click-free transitions
- Binding to UI: In `PluginEditor::PluginEditor()`, create relay and attachment

**New Component/Module:**
- Implementation: `modules/[category]/[component-name]/cpp/[ComponentName].h`
  - Or `plugins/[PluginName]/Source/` if plugin-specific
  - Header-only optional: `.h` only if no compilation needed
- CMake integration: Register in CMakeLists or module registry if reusable
- Documentation: Update `modules/registry.yaml` with metadata

**UI Elements (WebView):**
- Layout: `plugins/[PluginName]/Source/ui/public/index.html` (HTML structure)
- Styling: `plugins/[PluginName]/Source/ui/public/js/app.js` or inline styles
- Parameter Binding: `plugins/[PluginName]/Source/PluginEditor.cpp`:
  - Create relay: `std::unique_ptr<juce::WebSliderRelay> newRelay`
  - Create attachment: `std::unique_ptr<juce::WebSliderParameterAttachment> newAttachment`
  - Follow member order: Relays FIRST, then WebView, then Attachments
- JavaScript: `plugins/[PluginName]/Source/ui/public/js/juce/index.js` (relay binding)

**Utilities (Shared Helpers):**
- Non-DSP shared code: `modules/core/` or category-appropriate folder
- Math helpers: In-line in DSP component or `modules/effects/`
- UI utilities: `modules/ui/`

**Tests:**
- Unit tests: Not currently in codebase (validation via pluginval)
- Integration tests: Manual DAW testing after `/install-plugin`
- Validation: Automated via `pluginval` in build pipeline

## Special Directories

**build/:**
- Purpose: Compiled binaries and build artifacts
- Generated: YES (created by CMake/Ninja)
- Committed: NO (git-ignored)
- Clean: `rm -rf build/` then rebuild
- Install location for plugins: `~/Library/Audio/Plug-Ins/VST3/` and `~/Library/Audio/Plug-Ins/Components/` (handled by CLAUDE.md script)

**backups/:**
- Purpose: Version snapshots for rollback
- Generated: NO (manually created by versioning commands)
- Committed: YES (git-tracked for history)
- Structure: `backups/[PluginName]/v[semver]/` contains full Source/ and CMakeLists.txt

**.planning/:**
- Purpose: GSD-style stage planning documentation
- Generated: YES (created by GSD agents during implementation)
- Committed: YES (git-tracked for knowledge preservation)
- Plugin-local planning: `plugins/[PluginName]/.planning/` (per-plugin docs)
- Shared codebase planning: `.planning/codebase/` (architecture/structure docs)

**troubleshooting/:**
- Purpose: Knowledge base for problem solving
- Generated: YES (new issues documented as encountered)
- Committed: YES (shared learning repository)
- Auto-indexed: Agents search here before attempting fixes
- Mandatory read: `patterns/juce8-critical-patterns.md` (injected into all subagent contexts)

---

*Structure analysis: 2026-01-29*
