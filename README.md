# 0-Audio

[0-Audio](https://oaudio.io/) is an open-source audio software publisher releasing free VST3 and AU plugins for macOS and Windows. Instruments, effects, and utilities — built for DAWs and for notation software, with a particular focus on microtonal and physical-modeling instruments.

This repository holds the full source for the plugin catalog **and** the AI-assisted development system that builds it. Both are published together: the plugins you can install, and the workflow that produced them.

Every plugin is free and pay-what-you-want, and the entire catalog is open source under AGPL-3.0 — see [License](#license).

## Plugins

All plugins build as VST3 (macOS and Windows) and AU (macOS), and load in any compatible DAW.

**Instruments & Synths**

| Plugin | Description |
|--------|-------------|
| `O-Bells` | Physical-modeling bells |
| `O-Marimba` | Physical-model marimba |
| `O-Lyrica` | Physical-modeling harp |
| `O-Wind` | Physical-model flute |
| `O-Reed` | Physical-modeling reed wind |
| `O-Bowed` | Physical-model bowed string |
| `O-Contrabass` | Physical-model bowed bass (in development) |
| `O-Bassoon` | Physical-model bassoon (in development) |
| `O-Formant` | Physical-model vocal synth |
| `O-Prism` | Microtonal wavetable synth |
| `O-IntonationPad` | Wavetable pad with microtonal intonation |
| `O-MicrotonalSampler` | Microtonal sampler |
| `O-TextureForge` | Concatenative synthesis instrument |
| `O-Texture` | Neural texture synth (instrument/effect) |

**The `simple` series — pedagogical instruments**

Minimal, readable implementations of the classic synthesis methods, intended to be read as much as played.

| Plugin | Description |
|--------|-------------|
| `O-simpleSubtractive` | Subtractive synthesis |
| `O-simpleAdditive` | Additive + wavetable synthesis |
| `O-simpleFM` | Two-operator FM synthesis |
| `O-simpleGrain` | Granular synthesis |
| `O-simplePhysicalModelSynth` | Physical modeling |
| `O-simpleSampler` | Sampling |
| `O-simpleBeatmaker` | Step-sequencer drum machine |

**Effects & Processors**

| Plugin | Description |
|--------|-------------|
| `O-SimpleReverb` | Reverb |
| `O-DigiDelay` | Digital delay |
| `O-ReverseDelay` | Granular reverse delay |
| `O-Chorus` | Chorus |
| `O-Tremolo` | Tremolo |
| `O-Detune` | Detuning |
| `O-AnalogEQ` | Analog-voiced EQ |
| `O-AnalogSaturation` | Saturation |
| `O-Comp` | Compressor |
| `O-MultiBandCompressor` | Multiband dynamics |
| `O-Bass` | Bass enhancer |
| `O-Polystutter` | Beat repeater |
| `O-GrainScatter` | Granular stutter engine |
| `O-Freeze` | Granular freeze |
| `O-FreqPulse` | Spectral sequencer |
| `O-SpectralShaper` | Spectral transient shaper |
| `O-Orbit` | Spatial orbiter |

**Utilities**

| Plugin | Description |
|--------|-------------|
| `O-Gain` | Gain-staging utility |

Per-plugin versions, release state, and history live in the **[plugin registry](PLUGINS.md)**.

## Microtonal Dorico Integration

Dorico drives per-note microtonal playback in these plugins directly from the score — quarter-tones, just intonation, and custom tunings, with no MIDI pitch-bend workarounds and no per-track lane hacks. Write the accidental, and the plugin plays it.

### Per-Note Tuning (VST3 Note Expression)

Per-note tuning is provided by a shared module, `modules/tuning/note-expression` (v1.1.1). It owns the Note Expression Controller for `kTuningTypeID`, drains raw Note Expression events from a patched JUCE wrapper (`scripts/juce-patches/note-expression-juce-8.0.14.patch`), and applies the resulting per-note semitone offset at the voice call site. It composes with the `scala-tuning-engine` module, so a Dorico-driven delta and a plugin-side tuning table stack rather than conflict.

This is the VST3 path — the Note Expression symbols are VST3-only, so the capability requires a plugin's VST3 build. Dorico loads VST3 on both macOS and Windows.

Eleven plugins consume the module:

`O-Bassoon`, `O-Bells`, `O-Bowed`, `O-Contrabass`, `O-Formant`, `O-IntonationPad`, `O-Lyrica`, `O-MicrotonalSampler`, `O-Prism`, `O-Reed`, `O-Wind`

Activation is a one-time step. The installers — PKG on macOS, EXE on Windows — bundle `Ouaricon-VST3-NoteExpression.doricolib`. Import it once via **Dorico → Library → Library Manager → Import**, after which tuned pitches play back automatically in any project using these plugins as VST3 instruments.

### Playback Templates, Expression Maps, and Keyswitches

Two plugins ship full Dorico bundles — `O-MicrotonalSampler` and `O-Contrabass` — each under `plugins/<Name>/Resources/dorico/` with a playback template spec, endpoint configs, an install guide, and a smoke test.

The O-MicrotonalSampler bundle covers four instrument families — Strings, Winds, Brass, and a Generic fallback — in a single Playback Template, with Dorico routing each stave to the matching family map by the stave's instrument family. O-Contrabass ships the same shape with a single endpoint config.

A loose `.doricoexpmap` file is not picked up by Dorico's Library scanner, so the bundles use the multi-folder layout Dorico itself uses for user-saved templates, plus a `DefaultLibraryAdditions/` entry that registers the expression maps globally on launch.

Two O-MicrotonalSampler specifics worth knowing when setting it up:

- **Keyswitching is opt-in.** A fresh instance ships with `ks_enabled` off and forwards every note to the synth, so a library mapped into the low register loses nothing. Enable the toggle when you want the expression map's keyswitches to drive the technique cursor.
- **Dynamics can follow a continuous CC.** A `Dynamics Mode` parameter selects `Velocity` or `CC Crossfade`; in CC Crossfade mode the plugin crossfades recorded dynamic layers from the controller value, with a Dynamic Range control setting how far the crossfade travels.

### Further Reading

- [`research/microtonal-dorico-integration.md`](research/microtonal-dorico-integration.md) — the developer reference: module architecture, the canonical Dorico setup procedure, host-side behavior quirks, and a table of troubleshooting signatures.
- [`modules/tuning/note-expression/README.md`](modules/tuning/note-expression/README.md) — module API, JUCE patch management, and integration approach.
- [`plugins/O-MicrotonalSampler/Resources/dorico/INSTALL-DORICO.md`](plugins/O-MicrotonalSampler/Resources/dorico/INSTALL-DORICO.md) and [`plugins/O-Contrabass/Resources/dorico/INSTALL-DORICO.md`](plugins/O-Contrabass/Resources/dorico/INSTALL-DORICO.md) — per-plugin playback template install steps.

For help inside this repo, the `/dorico` command answers integration questions and troubleshoots playback for a given plugin.

## The Development System

An AI-assisted JUCE plugin development system that enables conversational creation of professional VST3 and AU audio plugins. Design and build custom audio processors through natural dialogue with Claude Code—no programming experience required. The interactive development loop is macOS-primary; cross-platform release builds (VST3 for macOS and Windows, AU for macOS) are produced through GitHub Actions CI via `/publish`.

## Why This Exists

To make the development of VST and AU plugins using Claude Code more achievable.

## What You Can Build

- **Effects**: Reverbs, delays, distortion, modulation, filters, dynamics processors
- **Synthesizers**: Subtractive, FM, wavetable, granular, additive
- **Utilities**: Analyzers, meters, routing tools, MIDI processors
- **Experimental**: Custom DSP algorithms, hybrid processors, generative tools

All plugins compile to native VST3 and AU formats compatible with any DAW (Ableton, Logic, Reaper, etc.). VST3 builds on both macOS and Windows; AU is macOS-only.

39 plugins have been built with this system so far — see the **[plugin registry](PLUGINS.md)** for the full catalog with versions and status.

## How It Works

### 1. Start (`/start`)

Brainstorm your plugin concept through conversation:
- **Creative brief** - Vision, sonic goals, UX principles
- **Parameter specification** - All controls, ranges, and mappings
- **Requirements** - Formal requirements with acceptance criteria (auto-extracted from brief)
- **UI mockups** - Visual design and layout

### 2. Plan (`/plan`)

Research and design the technical architecture:
- **DSP architecture** - Signal flow and processing strategy
- **Implementation plan** - Technical approach and complexity analysis

### 3. Implement (`/implement`)

Transform your specifications into a fully functional plugin through an automated workflow:

- **Build System Ready** (Stage 1): Project structure, CMake configuration, and all parameters implemented - validated automatically
- **Audio Engine Working** (Stage 2): DSP algorithms and audio processing complete - validated automatically
- **UI Integrated** (Stage 3): WebView interface connected to audio engine (or skip for headless plugins) - validated automatically with runtime tests
- After Stage 3 validation passes: Plugin complete, ready to install

Each stage follows the **GSD cycle**: discuss → research → plan → execute → verify. Documentation is automatically created at each phase, providing full traceability and easy resumption.

### 4. Deploy & Iterate

- `/install-plugin` - Install to system folders for DAW use
- `/test` - Run automated validation suite
- `/improve` - Add features or fix bugs (with regression testing)
- `/reconcile` - Reconcile state between planning and implementation

## Modern Interface Design

Plugins use web-based interfaces (HTML/CSS/JS) rendered via JUCE's WebView instead of traditional GUI frameworks. This enables:

- **Rapid prototyping**: See design changes instantly without rebuilding
- **Modern aesthetics**: Leverage contemporary web design patterns and animations
- **Interactive mockups**: Test and refine interfaces before implementation
- **Familiar tools**: Use web technologies many creators already understand
- **Responsive layouts**: Easily adapt UIs to different sizes and contexts

### GUI-Optional Workflow

Plugins can skip custom UI and ship as "headless" plugins using DAW-provided controls:

- **Faster iteration**: Test DSP immediately without waiting for UI implementation
- **Progressive enhancement**: Add custom UI later via `/improve`
- **Flexibility**: Decide when/if to build visual interface
- **Zero overhead**: Smaller binary, faster compile, all parameters exposed to DAW

## Key Features

### Automated Build Pipeline

7-phase build system (`scripts/build-and-install.sh`) handles validation, compilation, installation, and verification. No manual CMake commands or Xcode configuration required.

### Quality Assurance

- Automatic validation after each stage (compile-time + runtime tests)
- validation-agent runs pluginval automatically (VST3/AU validation)
- Validation is blocking - errors must be fixed before progressing
- Regression testing on modifications
- Backup verification before destructive operations
- Build failure detection and troubleshooting

### Knowledge Base

Dual-indexed troubleshooting database (`troubleshooting/`) captures solutions to build failures, runtime issues, GUI problems, and API misuse. The system learns from every problem encountered.

**Required Reading** (stage-specific pattern files) automatically prevents repeat mistakes by injecting proven patterns into subagent contexts.

### Template Library

Reusable code snippets and conceptual patterns (`.claude/templates/`) accelerate plugin development:

- **Code snippets**: Copy-paste ready patterns with variable substitution (relay setup, CMake configuration, processBlock skeleton)
- **Prose patterns**: Conceptual guides that agents interpret (LFO modulation, IIR filter chains, envelope followers)

Browse templates with `/templates` command or let subagents automatically discover relevant patterns for each stage.

**17 templates** across categories:
- Parameter binding (slider, toggle, combobox relays)
- CMake setup (WebView effects, synth/instruments)
- WebView initialization (lazy navigation, resource provider)
- DSP algorithms (LFO, filters, dynamics, saturation)
- Architecture (member order, thread safety)
- UI interaction (knob drag, VU meter animation)

### Graduated Research Protocol

3-level investigation system (`/research`) for complex problems:

- **Quick**: Single-agent targeted investigation (1-2 min)
- **Moderate**: Multi-agent parallel search (3-5 min)
- **Deep**: Comprehensive multi-level analysis (5-10 min)

### Version Management

- Semantic versioning on improvements
- Git-based state tracking
- Safe rollback capabilities
- Backup verification before destructive operations

### Workflow Modes

- **Manual mode** (default): Present decision menus at each checkpoint for full control
- **Express mode**: Auto-progress through implementation stages without intermediate menus
- **Configurable**: Set preferences in `.claude/preferences.json` or use `--express`/`--manual` flags
- **Safe**: Express mode drops to manual on any error, ensuring oversight when needed

### Lifecycle Management

- `/install-plugin` - Deploy to system folders
- `/uninstall` - Remove binaries (keep source)
- `/reset-to-ideation` - Roll back to concept stage
- `/destroy` - Complete removal with verified backup
- `/clean` - Interactive cleanup menu

## System Architecture

### Plugin-Local Planning (GSD-Integrated)

Every plugin has its own planning directory at `plugins/[Name]/.planning/`:

```
plugins/[Name]/.planning/
├── BRIEF.md                    # Vision, sonic goals, UX principles
├── REQUIREMENTS.md             # Formal requirements with acceptance criteria
├── STATUS.md                   # Current stage, progress, history
├── ROADMAP.md                  # Implementation strategy and phases
├── parameter-spec.md           # Complete parameter definitions
├── research/
│   └── ARCHITECTURE.md         # DSP design and signal flow
├── mockups/                    # Visual design references
└── stages/                     # GSD-style stage documentation
    ├── 1-foundation/
    │   ├── CONTEXT.md          # Discuss phase output
    │   ├── PLAN.md             # Execution plan
    │   ├── SUMMARY.md          # What was built
    │   └── VERIFICATION.md     # Verify phase output
    ├── 2-dsp/
    └── 3-ui/
```

**[GSD](https://github.com/open-gsd/gsd-core) Integration**: Each stage follows the discuss → research → plan → execute → verify cycle with explicit documentation at each phase.

**Zero drift**: All stages reference the same specs. No "telephone game" across workflows.

**Multi-plugin support**: Each plugin carries its own planning context—work on multiple plugins simultaneously without branch switching.

### Dispatcher Pattern

Each implementation stage runs in a fresh subagent context:

- `foundation-shell-agent` (Stage 1) - Project structure and parameter management
- `dsp-agent` (Stage 2) - Audio processing
- `gui-agent` (Stage 3) - WebView UI
- `validation-agent` (after each stage) - Automatic validation with runtime tests

**No context accumulation**: Clean separation prevents cross-contamination and keeps token usage minimal.

### Discovery Through Play

All features discoverable via:

- Slash command autocomplete (type `/` in Claude Code)
- Numbered decision menus at checkpoints
- Interactive skill prompts

**No manual required**: Learn by exploring, not reading docs.

### Checkpoint Protocol

At every completion point:

1. Auto-commit changes
2. Update state files (`STATUS.md`, `PLUGINS.md`)
3. Create stage documentation (`CONTEXT.md`, `SUMMARY.md`, `VERIFICATION.md`)
4. Present numbered decision menu
5. Wait for user response
6. Execute chosen action

**Never auto-proceeds**: You stay in control.

## Quick Start

### Prerequisites

- macOS (Sonoma or later recommended) for the interactive Claude Code development loop
- Claude Code CLI
- Windows is supported for VST3 release builds via GitHub Actions CI (triggered by `/publish`); local Windows builds use `scripts/build-and-install.ps1`

All other dependencies (Xcode Command Line Tools, JUCE, CMake, Python, pluginval) can be validated and installed via `/setup`.

MCPs active and suggested:
  ✅ Filesystem - @modelcontextprotocol/server-filesystem
  ✅ Context7 - @upstash/context7-mcp
  ✅ Playwright - @playwright/mcp
  ✅ Sequential Thinking - @modelcontextprotocol/server-sequential-thinking

### First-Time Setup

```bash
# Validate and configure your system dependencies
/setup

# The setup wizard will:
# - Detect your platform and installed tools
# - Offer to install missing dependencies automatically or guide manual installation
# - Save configuration for build scripts
# - Generate a system report
```

### Create Your First Plugin

```bash
# 1. Start the concept
/start

# Brainstorm your plugin idea through conversation
# Creates: creative brief, parameter spec, UI mockups

# 2. Plan the architecture
/plan

# Research and design the technical implementation
# Creates: DSP architecture, implementation plan

# 3. Build it
/implement

# Automated workflow builds the plugin

# 4. Install and test
/install-plugin YourPluginName

# Plugin now available in your DAW
```

### Improve an Existing Plugin

```bash
# Fix bugs or add features
/improve MyPlugin

# Describe the change
# System handles versioning, testing, and rollback safety
```

### Resume Interrupted Work

```bash
# Pick up where you left off
/continue MyPlugin

# System loads checkpoint and presents next steps
```

## Production Workflow Guide

This section provides a complete guide for using the Plugin Freedom System in production, including multi-plugin parallel development.

### Complete Single Plugin Workflow

The full journey from idea to installed plugin:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  Stage 0: Ideation & Planning                                               │
│  ┌─────────┐    ┌─────────┐                                                 │
│  │ /start  │ →  │ /plan   │  Creates: BRIEF.md, parameter-spec.md,         │
│  └─────────┘    └─────────┘           ARCHITECTURE.md, ROADMAP.md           │
└─────────────────────────────────────────────────────────────────────────────┘
                                    ↓
┌─────────────────────────────────────────────────────────────────────────────┐
│  Stages 1-3: Implementation (/implement)                                    │
│                                                                             │
│  Each stage runs: discuss → research → plan → execute → verify              │
│                                                                             │
│  Stage 1: Foundation    → CMakeLists.txt, PluginProcessor, parameters       │
│  Stage 2: DSP           → processBlock, audio algorithms                    │
│  Stage 3: GUI           → WebView UI, parameter bindings                    │
│                                                                             │
│  Quality gates block progression until verification passes.                 │
└─────────────────────────────────────────────────────────────────────────────┘
                                    ↓
┌─────────────────────────────────────────────────────────────────────────────┐
│  Stage 4: Deploy & Iterate                                                  │
│  ┌─────────────────┐    ┌─────────┐    ┌──────────┐                        │
│  │ /install-plugin │ →  │ /test   │ →  │ /improve │ (as needed)            │
│  └─────────────────┘    └─────────┘    └──────────┘                        │
└─────────────────────────────────────────────────────────────────────────────┘
```

**Step-by-step walkthrough:**

```bash
# 1. IDEATION: Explore your plugin idea
/start O-MyPlugin

# Interactive conversation captures:
# - Creative brief (vision, sonic goals, UX principles)
# - Parameter specification (all controls with ranges)
# - Requirements (auto-extracted with acceptance criteria)
# - UI mockups (optional, can add later)

# 2. PLANNING: Design the technical architecture
/plan O-MyPlugin

# Creates:
# - ARCHITECTURE.md (DSP design, signal flow)
# - ROADMAP.md (implementation strategy)
# - Complexity assessment

# 3. IMPLEMENTATION: Build the plugin
/implement O-MyPlugin

# Runs stages 1-3 automatically:
# - Stage 1: Project structure, CMake, parameters
# - Stage 2: Audio processing, DSP algorithms
# - Stage 3: WebView UI (or skip for headless)
#
# Each stage has quality gates that must pass.

# 4. TESTING: Validate the plugin
/test O-MyPlugin

# Runs pluginval (VST3/AU validation)
# Reports any issues found

# 5. INSTALLATION: Deploy to DAW
/install-plugin O-MyPlugin

# Copies to ~/Library/Audio/Plug-Ins/
# Clears AU cache for immediate availability

# 6. ITERATION: Improve as needed
/improve O-MyPlugin

# Add features, fix bugs with:
# - Semantic versioning
# - Regression testing
# - Rollback safety
```

### Multi-Plugin Parallel Development

Develop multiple plugins simultaneously using context isolation:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Context Switching Model                              │
│                                                                             │
│   Session A                    Session B                    Session C       │
│   ┌──────────────┐            ┌──────────────┐            ┌──────────────┐ │
│   │ O-PluginA    │            │ O-PluginB    │            │ O-PluginC    │ │
│   │ Stage 2: DSP │            │ Stage 1: Found│            │ Stage 3: GUI │ │
│   └──────────────┘            └──────────────┘            └──────────────┘ │
│         ↑                           ↑                           ↑          │
│   /plugin-focus              /plugin-focus              /plugin-focus      │
│   O-PluginA                  O-PluginB                  O-PluginC          │
│                                                                             │
│   Each plugin has isolated state in plugins/[Name]/.planning/               │
└─────────────────────────────────────────────────────────────────────────────┘
```

**Managing multiple plugins:**

```bash
# See all plugins and their status
/plugin-list

# Output example:
# ┌────────────────┬───────────┬─────────┬──────────────┐
# │ Plugin         │ Stage     │ Phase   │ Status       │
# ├────────────────┼───────────┼─────────┼──────────────┤
# │ O-PluginA      │ 2-dsp     │ execute │ in_progress  │
# │ O-PluginB      │ 1-found   │ verify  │ pending      │
# │ O-PluginC      │ 3-gui     │ discuss │ paused       │
# └────────────────┴───────────┴─────────┴──────────────┘

# Switch focus to a specific plugin
/plugin-focus O-PluginB

# Now all commands default to O-PluginB
/plugin-status          # Shows O-PluginB status
/implement              # Continues O-PluginB implementation

# Check detailed status
/plugin-status O-PluginA

# Output shows:
# - Current stage and phase
# - Completed stages with timestamps
# - Next recommended action
```

**Parallel development workflow:**

```bash
# Terminal 1: Working on O-PluginA
/plugin-focus O-PluginA
/implement

# Terminal 2: Working on O-PluginB (different Claude Code session)
/plugin-focus O-PluginB
/implement

# Each session operates independently with isolated state.
# No branch switching required—each plugin has its own .planning/ directory.
```

### Session Management (Pause/Resume)

Long-running development can span multiple sessions:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         Session Continuity                                  │
│                                                                             │
│   Session 1                    [Pause]                    Session 2         │
│   ┌──────────────┐            ┌──────────┐            ┌──────────────────┐ │
│   │ Working on   │     →      │ HANDOFF  │     →      │ Resume with full │ │
│   │ Stage 2 DSP  │            │ created  │            │ context restored │ │
│   └──────────────┘            └──────────┘            └──────────────────┘ │
│                                                                             │
│   /plugin-pause               Saves:                  /continue O-Plugin   │
│   O-Plugin                    - STATUS.md             Loads:               │
│                               - HANDOFF.json          - Last checkpoint    │
│                               - Stage docs            - Decision history   │
│                                                       - Next steps         │
└─────────────────────────────────────────────────────────────────────────────┘
```

**Pausing work:**

```bash
# When you need to stop mid-workflow
/plugin-pause O-MyPlugin

# Creates:
# - Updated STATUS.md with exact position
# - HANDOFF.json with context for resumption
# - Commit with checkpoint marker

# Output includes next command to run:
# "To resume: /continue O-MyPlugin"
```

**Resuming work:**

```bash
# In a new session (even days later)
/continue O-MyPlugin

# System:
# 1. Loads STATUS.md to find last position
# 2. Reads stage documentation for context
# 3. Presents summary of where you left off
# 4. Offers next action options

# You're back exactly where you stopped.
```

**Handling context window limits:**

```bash
# If context fills up during long workflow
# Claude Code will prompt: "Context limit approaching"

# Best practice:
/plugin-pause O-MyPlugin   # Save state
/clear                     # Clear context
/continue O-MyPlugin       # Resume with fresh context
```

### Quality Assurance Flow

Every stage has quality gates that must pass:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          Quality Gate Flow                                  │
│                                                                             │
│   Execute Phase                Gate Check                Verify Phase       │
│   ┌──────────────┐            ┌──────────┐            ┌──────────────────┐ │
│   │ Agent runs   │     →      │ Blocking │     →      │ Goal-backward    │ │
│   │ stage tasks  │            │ checks   │            │ verification     │ │
│   └──────────────┘            └──────────┘            └──────────────────┘ │
│                                     │                                       │
│                               ┌─────┴─────┐                                 │
│                               │  Pass?    │                                 │
│                               └─────┬─────┘                                 │
│                            Yes ↙         ↘ No                               │
│                     ┌──────────┐    ┌─────────────┐                        │
│                     │ Proceed  │    │ Fix issues  │                        │
│                     │ to next  │    │ then retry  │                        │
│                     │ stage    │    │ gate        │                        │
│                     └──────────┘    └─────────────┘                        │
└─────────────────────────────────────────────────────────────────────────────┘
```

**Gate checks by stage:**

| Stage | Gate Checks |
|-------|-------------|
| 1-foundation | CMake configures, builds without error, parameters registered, COMPAT requirements pass |
| 2-dsp | processBlock compiles, no real-time safety violations, audio flows, FUNC/DSP/PERF/QUAL requirements pass |
| 3-gui | WebView loads, parameters bind correctly, UI responds, UI requirements pass |

Each gate also verifies requirements from REQUIREMENTS.md where `verifiedAt` matches the current stage.

**Manual gate control:**

```bash
# Run gate check explicitly
/plugin-verify O-MyPlugin 2-dsp

# If gate fails, fix issues then retry
/plugin-execute O-MyPlugin 2-dsp   # Re-run execution
/plugin-verify O-MyPlugin 2-dsp    # Re-check gate

# Skip gate (use sparingly, requires justification)
/plugin-verify O-MyPlugin 2-dsp --skip "Known issue, will fix in Stage 3"
```

### Best Practices

**1. One plugin per focus**
```bash
# Always set focus before working
/plugin-focus O-MyPlugin

# Then run commands without specifying name
/implement
/plugin-status
/test
```

**2. Commit frequently**
```bash
# The system auto-commits at checkpoints, but you can also:
git status                    # Check what's pending
git add -A && git commit -m "WIP: Stage 2 progress"
```

**3. Use express mode for straightforward plugins**
```bash
# Skip intermediate menus
/implement O-MyPlugin --express

# Drops to manual mode on any error
```

**4. Pause before context fills**
```bash
# Don't wait for "context limit" warning
# Pause proactively after completing a stage
/plugin-pause O-MyPlugin
/clear
/continue O-MyPlugin
```

**5. Check status when returning**
```bash
# First command in any session
/plugin-list

# Then focus and continue
/plugin-focus O-MyPlugin
/plugin-status
/continue
```

**6. Use reconcile if state seems wrong**
```bash
# If STATUS.md and registry disagree
/reconcile O-MyPlugin

# Auto-repairs inconsistencies
```

**7. Document learnings**
```bash
# After solving a tricky problem
/doc-fix

# Adds to knowledge base for future reference
```

### Troubleshooting Common Issues

| Issue | Solution |
|-------|----------|
| "Plugin not found" | Run `/plugin-list` to see exact names |
| Stage won't progress | Run `/plugin-verify` to see gate failures |
| State seems corrupted | Run `/reconcile [Name]` to repair |
| Context too large | `/plugin-pause` then `/clear` then `/continue` |
| Build fails | `/research [error message]` for investigation |
| Plugin not in DAW | Run `/install-plugin` and restart DAW |

## Complete Command Reference

### Workflow Overview

The Plugin Freedom System uses a staged workflow with GSD (Get Stuff Done) phase cycles:

```
Stage 0: Ideation/Planning → Stage 1: Foundation → Stage 2: DSP → Stage 3: GUI → Stage 4: Polish
                                      ↓
                            Each stage cycles through:
                            discuss → research → plan → execute → verify
```

### Typical Workflow Example

```bash
/start O-NewPlugin          # Create idea/brief
/plan O-NewPlugin           # Stage 0: Architecture planning
/implement O-NewPlugin      # Stages 1-4: Build it
/test O-NewPlugin           # Validate with pluginval
/install-plugin O-NewPlugin # Deploy to DAW
/improve O-NewPlugin        # Later: add features
```

---

### Setup

| Command | Purpose |
|---------|---------|
| `/setup` | Validate and configure system dependencies (first-time setup) |

### Starting a Plugin

| Command | Purpose |
|---------|---------|
| `/start [Name?]` | Explore plugin ideas, create creative brief (no implementation) |
| `/plan [Name?]` | Stage 0 - Research DSP architecture, create ARCHITECTURE.md and ROADMAP.md |

### Context Management

Manage multiple plugins in parallel with explicit context switching:

| Command | Purpose |
|---------|---------|
| `/plugin-list` | Show all plugins with stage/phase status |
| `/plugin-focus [Name]` | Set active plugin context (default for other commands) |
| `/plugin-status [Name?]` | Detailed stage/phase breakdown |
| `/continue [Name?]` | Resume from last checkpoint |
| `/plugin-pause [Name?]` | Save progress, create handoff document |
| `/plugin-resume [Name?]` | Restore context from handoff and continue |

### Implementation (Stages 1-4)

| Command | Purpose |
|---------|---------|
| `/implement [Name?] [--express]` | Run full implementation workflow through stages |
| `/plugin-discuss [Name?] [Stage?]` | GSD discuss phase - gather context for current stage |
| `/plugin-research [Name?] [Stage?]` | GSD research phase - investigate approach |
| `/plugin-plan [Name?] [Stage?]` | GSD plan phase - create task breakdown |
| `/plugin-execute [Name?] [Stage?]` | GSD execute phase - run stage-specific agent |
| `/plugin-verify [Name?] [Stage?]` | GSD verify phase - validate stage goals achieved |
| `/plugin-critique [Name?] [Stage?]` | Run domain-specific critic on stage outputs for quality validation |
| `/plugin-handoff [Name?] [Stage?]` | Create handoff document for a stage transition |

Skip flags: `--skip-discuss`, `--skip-research`, `--skip-verify`

### Testing & Validation

| Command | Purpose |
|---------|---------|
| `/test [Name]` | Run pluginval and automated validation suite |
| `/show-standalone [Name]` | Open plugin UI in Standalone mode for visual inspection |

### Post-Completion

The review commands form a loop around `/improve`: a code review produces `CODE_REVIEW.md`, `/improve-review` resolves its findings, `/improve-review-info` sweeps the low-risk Info tier that pass skips, and `/improve-verify` confirms the fixes hold before the plugin is considered ship-ready. The two simplify commands sweep tiers of a plugin's `SIMPLIFICATION-AUDIT.md` in the same spirit.

| Command | Purpose |
|---------|---------|
| `/improve [Name]` | Add features, fix bugs (with versioning and regression testing) |
| `/improve-milestone [Name] [description?]` | Complex improvements run as GSD-style phase cycles |
| `/improve-review [Name] [findings?]` | Resolve code-review findings from a plugin's `CODE_REVIEW.md` |
| `/improve-review-info [Name] [findings?]` | Sweep the Info tier (IN-*) of `CODE_REVIEW.md` — the opt-out findings `/improve-review` skips |
| `/improve-verify [Name] [version?]` | Verify resolved code-review fixes hold with no regressions, then gate ship-readiness |
| `/simplify-phase2 [Name]` | Apply MEDIUM-risk HIGH-severity candidates from `SIMPLIFICATION-AUDIT.md` |
| `/simplify-phase3 [Name]` | Sweep the MEDIUM and LOW tiers of `SIMPLIFICATION-AUDIT.md` (high-volume, low-risk cleanup) |
| `/install-plugin [Name]` | Deploy to `~/Library/Audio/Plug-Ins/` |
| `/package [Name]` | Create signed PKG installer for distribution |
| `/build-installer [Name]` | Create a Windows EXE installer for VST3 distribution (Inno Setup) |
| `/publish [Name]` | Release via GitHub Actions CI/CD |

### Lifecycle Management

| Command | Purpose |
|---------|---------|
| `/uninstall [Name]` | Remove from system folders (keep source code) |
| `/reset-to-ideation [Name]` | Remove implementation, keep idea/mockups |
| `/destroy [Name]` | Complete removal with verified backup |
| `/clean [Name]` | Interactive cleanup menu (choose operation) |
| `/reconcile [Name]` | Fix out-of-sync state files |

### Module System (Code Reuse)

Reusable code components with versioning and dependency tracking:

| Command | Purpose |
|---------|---------|
| `/modules` | Manage shared modules (interactive) |
| `/module-list` | List all available modules by category |
| `/module-info [Name]` | Show detailed module information and dependents |
| `/module-add [Plugin] [Module]` | Add module dependency to plugin |
| `/module-remove [Plugin] [Module]` | Remove module dependency |
| `/module-create [Name] --from [Plugin]` | Extract code into reusable module |
| `/module-upgrade [Name]` | Upgrade module and rebuild all dependents |
| `/module-upgrade-all` | Batch upgrade all modules across all plugins, with preview |

### Research & Troubleshooting

| Command | Purpose |
|---------|---------|
| `/research [topic]` | Deep multi-agent investigation (3-level protocol) |
| `/dorico [Name] [question?]` | Dorico integration helper — microtonal playback, expression maps, playback templates, keyswitch routing, CC/PC technique triggers |
| `/doc-fix` | Document solved problem to knowledge base |
| `/add-critical-pattern` | Add current problem to Required Reading |

### System

| Command | Purpose |
|---------|---------|
| `/pfs` | Load Plugin Freedom System architecture context |
| `/templates` | Browse reusable code patterns and snippets |
| `/generalize-microtones` | Promote the VST3 Note Expression pattern into a shared module and propagate Dorico microtonal playback across pitched plugins |

## Project Structure

```
vst-development/
├── plugins/                          # Plugin source code
│   └── [PluginName]/
│       ├── .planning/                # GSD-style planning (plugin-local)
│       │   ├── BRIEF.md              # Creative vision
│       │   ├── REQUIREMENTS.md       # Formal requirements with acceptance criteria
│       │   ├── STATUS.md             # Current state and progress (phase-aware)
│       │   ├── ROADMAP.md            # Implementation strategy
│       │   ├── parameter-spec.md     # Parameter definitions
│       │   ├── modules.json          # Module dependencies
│       │   ├── research/
│       │   │   └── ARCHITECTURE.md   # DSP design
│       │   ├── mockups/              # UI mockup files
│       │   └── stages/               # Per-stage GSD docs (5 phases each)
│       │       ├── 0-ideation/
│       │       ├── 1-foundation/
│       │       │   ├── CONTEXT.md    # Discuss output
│       │       │   ├── RESEARCH.md   # Research output
│       │       │   ├── PLAN.md       # Execution plan
│       │       │   ├── SUMMARY.md    # What was built
│       │       │   └── VERIFICATION.md
│       │       ├── 2-dsp/
│       │       ├── 3-gui/
│       │       └── 4-polish/
│       ├── Source/                   # C++ implementation
│       └── CMakeLists.txt
├── docs/
│   └── codebase/                     # Shared architecture documentation
├── .claude/
│   ├── skills/                       # Specialized workflows
│   │   ├── plugin-workflow/          # Orchestrator (Build → DSP → GUI → Validation)
│   │   ├── plugin-planning/          # Research & design (Stage 0)
│   │   ├── plugin-ideation/          # Concept brainstorming
│   │   ├── plugin-improve/           # Versioned modifications
│   │   ├── ui-mockup/                # Visual design system
│   │   ├── plugin-testing/           # Validation suite
│   │   ├── plugin-lifecycle/         # Install/uninstall/destroy
│   │   ├── deep-research/            # 3-level investigation
│   │   ├── troubleshooting-docs/     # Knowledge capture
│   │   └── workflow-reconciliation/  # State consistency checks
│   ├── agents/                       # Implementation subagents
│   │   ├── research-planning-agent   # Research & Planning (Stage 0)
│   │   ├── foundation-shell-agent    # Build System Ready (Stage 1)
│   │   ├── dsp-agent                 # Audio Engine Working (Stage 2)
│   │   ├── gui-agent                 # UI Integrated (Stage 3)
│   │   ├── validation-agent          # Automatic validation (after each stage)
│   │   ├── ui-design-agent           # UI mockup design
│   │   ├── ui-finalization-agent     # UI implementation scaffolding
│   │   └── troubleshoot-agent        # Build failures
│   ├── commands/                     # Slash command prompts
│   ├── templates/                    # Reusable patterns library
│   │   ├── code-snippets/            # Copy-paste code with variables
│   │   ├── prose-patterns/           # Conceptual patterns
│   │   └── plugin-planning/          # Plugin .planning/ templates
│   └── hooks/                        # Validation gates
├── scripts/
│   ├── build-and-install.sh          # 7-phase build pipeline (macOS)
│   ├── build-and-install.ps1         # Windows VST3 build + install
│   ├── resolve-target.sh             # Resolve a plugin's CMake target name from its folder
│   ├── apply-juce-patches.sh         # Apply the vendored JUCE Note Expression patch (local dev)
│   ├── juce-patches/                 # Named JUCE patch files (e.g. note-expression)
│   ├── generate_placeholder_models.py # Placeholder ML models for ANIRA plugins
│   ├── add-agpl-headers.py           # Apply AGPL notice headers to source files
│   ├── regen-registry-used-by.sh     # Regenerate module registry used_by from disk truth
│   ├── verify-au-link.sh             # AU link/loading gate (auval)
│   ├── verify-suite-battery.sh       # Full-suite validation battery
│   └── verify-backup.sh              # Backup integrity checks
├── troubleshooting/                  # Dual-indexed knowledge base
│   ├── build-failures/
│   ├── runtime-issues/
│   ├── gui-issues/
│   ├── dsp-issues/
│   └── patterns/
│       └── juce8-critical-patterns.md  # Required Reading
└── PLUGINS.md                        # Plugin registry
```

## Philosophy

This system treats plugin development as a **creative conversation**, not a coding task.

You describe the sound, behavior, and appearance you want. The system handles the technical complexity—DSP implementation, parameter management, UI rendering, build configuration, validation, deployment.

**Focus on what matters**: Creating tools that serve your music.

## Feedback Loop

The complete improvement cycle:

```
Build → Test → Find Issue → Research → Improve → Document → Validate → Deploy
    ↑                                                                      ↓
    └──────────────────────────────────────────────────────────────────────┘
```

- **deep-research** finds solutions (graduated 3-level protocol)
- **plugin-improve** applies changes (with regression testing)
- **troubleshooting-docs** captures knowledge (dual-indexed for fast lookup)
- **ui-mockup finalization** auto-updates brief (treats mockup as source of truth)
- **plugin-lifecycle** manages deployment (cache clearing, verification)
- **Required Reading** prevents repeat mistakes (auto-injected into subagents)

Every problem encountered becomes institutional knowledge. The system learns and improves over time.

## Implementation Status

### Plugin Freedom System v1.0 (Complete)

All 35 requirements delivered across 7 phases:

| Phase | Deliverables | Status |
|-------|--------------|--------|
| 1. Agent Contracts | JSON schemas for all 9 agents, contract validation, gap analysis | ✓ Complete |
| 2. State Management | Registry v3, checkpoints, plugin isolation, session continuity | ✓ Complete |
| 3. Structured Handoffs | 4 handoff schemas, validation scripts, decision audit trails | ✓ Complete |
| 4. Verification Infrastructure | Critic agents (DSP/UI), token budget awareness | ✓ Complete |
| 5. Quality Gates | Blocking gates, code review integration, unified gate script | ✓ Complete |
| 6. Domain Specialization | Real-time safety rules, thread-safety patterns, quality standards | ✓ Complete |
| 7. Module System | Dependency tracking, semver, upgrade notifications | ✓ Complete |

### Milestone History

| Milestone | Theme | Shipped |
|-----------|-------|---------|
| v1.0 | Plugin Freedom System foundation | 2026-02-01 |
| v1.1 | Repository cleanup, planning workflow | — |
| v1.2 | Resource discovery & context injection | — |
| v1.3 | Opus 4.6 platform, agent teams, persistence | — |
| v1.4 | Domain refinements & template auto-selection | — |
| v1.5 | **Microtonal Shared Module & Suite Propagation** — `note-expression` v1.1.0 module + Dorico Library Manager distribution across 8 cohort plugins | 2026-04-27 |

**System status**: Production ready. 25 phases complete across 6 milestones (141 requirements satisfied).

## Requirements

### Software

**Required (interactive development — macOS-primary):**
- macOS 13+ (Sonoma recommended)
- Claude Code CLI

**Dependencies (validated/installed via `/setup`):**
- Xcode Command Line Tools (`xcode-select --install`)
- JUCE 8.0.14 (audio plugin framework)
- Python 3.8+ (build scripts)
- CMake 3.15+ (build system)
- pluginval (plugin validation tool)
- Git

**Cross-platform release builds:** VST3 for both macOS and Windows (plus macOS AU) are produced by GitHub Actions CI and triggered via `/publish`. Windows builds use the same JUCE 8.0.14 pin (see `.github/workflows/build-and-release.yml`); local Windows installs use `scripts/build-and-install.ps1`.

### Hardware

- Apple Silicon or Intel Mac
- 8GB RAM minimum (16GB recommended)
- 2GB free disk space per plugin

### Knowledge

- **Zero programming required**
- Basic understanding of audio plugin concepts (parameters, presets, DAW usage)
- Ability to describe sonic goals and UX preferences

## License

This repository — every plugin in it and the development system that builds them — is licensed under the **GNU Affero General Public License v3.0**. The full text is in [LICENSE](LICENSE).

JUCE is dual-licensed, and this project takes it under its **AGPLv3** option rather than the free Starter tier: the repository redistributes JUCE-owned source files, which the Starter EULA does not permit, and Starter's revenue cap counts pay-what-you-want income. The long-form rationale is in `PUBLIC-RELEASE-READINESS.md` §5.2.

Source files carry AGPL notice headers applied by `scripts/add-agpl-headers.py`.

Anyone distributing a derived plugin inherits the AGPL-3.0 obligations — including making the corresponding source available to users of that plugin.

## Acknowledgments

Built with:

- [JUCE](https://juce.com/) - Cross-platform audio application framework (used here under its AGPLv3 license option)
- [Claude Code](https://claude.com/claude-code) - AI-assisted development environment
- [Anthropic](https://anthropic.com/) - Claude AI models

---

**Start building**: `/start`
