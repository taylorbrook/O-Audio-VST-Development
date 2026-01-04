#VST building system based upon **[TÂCHES](https://youtube.com/tachesteaches)**

# Plugin Freedom System

An AI-assisted JUCE plugin development system that enables conversational creation of professional VST3 and AU audio plugins for macOS. Design and build custom audio processors through natural dialogue with Claude Code—no programming experience required.

## Why This Exists

To make the development of VST and AU plugins using Claude Code more achievable.

## What You Can Build

- **Effects**: Reverbs, delays, distortion, modulation, filters, dynamics processors
- **Synthesizers**: Subtractive, FM, wavetable, granular, additive
- **Utilities**: Analyzers, meters, routing tools, MIDI processors
- **Experimental**: Custom DSP algorithms, hybrid processors, generative tools

All plugins compile to native VST3/AU formats compatible with any DAW (Ableton, Logic, Reaper, etc.).

## How It Works

### 1. Dream (`/dream`)

Brainstorm your plugin concept through conversation:
- **Creative brief** - Vision, sonic goals, UX principles
- **Parameter specification** - All controls, ranges, and mappings
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

**Required Reading** (`juce8-critical-patterns.md`) automatically prevents repeat mistakes by injecting proven patterns into all subagent contexts.

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

### Contracts (Single Source of Truth)

Every plugin has immutable contracts in `plugins/[Name]/.ideas/`:

- `creative-brief.md` - Vision, sonic goals, UX principles
- `parameter-spec.md` - Complete parameter definitions
- `architecture.md` - DSP design and signal flow
- `plan.md` - Implementation strategy
- `ui-mockups/` - Visual design references

**Zero drift**: All stages reference the same specs. No "telephone game" across workflows.

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
2. Update state files (`.continue-here.md`, `PLUGINS.md`)
3. Present numbered decision menu
4. Wait for user response
5. Execute chosen action

**Never auto-proceeds**: You stay in control.

## Quick Start

### Prerequisites

- macOS (Sonoma or later recommended)
- Claude Code CLI

All other dependencies (Xcode Command Line Tools, JUCE, CMake, Python, pluginval) can be validated and installed via `/setup`.

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
# 1. Dream the concept
/dream

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

## Complete Command Reference

### Setup

- `/setup` - Validate and configure system dependencies (first-time setup)
  - Detects platform, checks for required tools
  - Offers automated installation or guided manual setup
  - Saves configuration to `.claude/system-config.json`

### Development Workflow

- `/dream` - Brainstorm concept, create creative brief, parameter spec, and UI mockups
- `/plan` - Research and design DSP architecture and implementation strategy
- `/implement [Name]` - Build plugin through automated 3-stage workflow with continuous validation
- `/continue [Name]` - Resume paused workflow
- `/improve [Name]` - Modify completed plugin (with regression testing)

### Quality Assurance

- `/test [Name]` - Run automated validation suite
- `/research [topic]` - Deep investigation (3-level protocol)
- `/doc-fix` - Document solved problems for knowledge base
- `/add-critical-pattern` - Add current problem to Required Reading

### Deployment

- `/install-plugin [Name]` - Install to system folders
- `/uninstall [Name]` - Remove binaries (keep source)
- `/show-standalone [Name]` - Preview UI in standalone mode

### Lifecycle

- `/clean` - Interactive cleanup menu (uninstall/reset/destroy)
- `/reconcile [Name]` - Reconcile state between planning and implementation
- `/clear-cache [Name]` - Clear validation cache
- `/reset-to-ideation [Name]` - Remove implementation, keep idea/mockups
- `/destroy [Name]` - Complete removal (with verified backup)

## Project Structure

```
plugin-freedom-system/
├── plugins/                          # Plugin source code
│   └── [PluginName]/
│       ├── .ideas/                   # Contracts (immutable during impl)
│       │   ├── creative-brief.md
│       │   ├── parameter-spec.md
│       │   ├── architecture.md
│       │   ├── plan.md
│       │   └── ui-mockups/
│       ├── Source/                   # C++ implementation
│       ├── WebUI/                    # HTML/CSS/JS interface
│       └── CMakeLists.txt
├── .claude/
│   ├── skills/                       # Specialized workflows
│   │   ├── plugin-workflow/          # Orchestrator (Build → DSP → GUI → Validation)
│   │   ├── plugin-planning/          # Research & design (Research Complete)
│   │   ├── plugin-ideation/          # Concept brainstorming
│   │   ├── plugin-improve/           # Versioned modifications
│   │   ├── ui-mockup/                # Visual design system
│   │   ├── plugin-testing/           # Validation suite
│   │   ├── plugin-lifecycle/         # Install/uninstall/destroy
│   │   ├── deep-research/            # 3-level investigation
│   │   ├── troubleshooting-docs/     # Knowledge capture
│   │   └── workflow-reconciliation/  # State consistency checks
│   ├── agents/                       # Implementation subagents
│   │   ├── research-planning-agent/  # Research Complete (Stage 0)
│   │   ├── foundation-shell-agent/   # Build System Ready (Stage 1)
│   │   ├── dsp-agent/                # Audio Engine Working (Stage 2)
│   │   ├── gui-agent/                # UI Integrated (Stage 3)
│   │   ├── validation-agent/         # Automatic validation (after each stage)
│   │   ├── ui-design-agent/          # UI mockup design
│   │   ├── ui-finalization-agent/    # UI implementation scaffolding
│   │   └── troubleshoot-agent/       # Build failures
│   ├── commands/                     # Slash command prompts
│   └── hooks/                        # Validation gates
├── scripts/
│   ├── build-and-install.sh          # 7-phase build pipeline
│   └── verify-backup.sh              # Backup integrity checks
├── troubleshooting/                  # Dual-indexed knowledge base
│   ├── build-failures/
│   ├── runtime-issues/
│   ├── gui-issues/
│   ├── dsp-issues/
│   └── patterns/
│       └── juce8-critical-patterns.md  # Required Reading
├── PLUGINS.md                        # Plugin registry
└── .continue-here.md                 # Active workflow state
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

- ✓ **Phase 0**: Foundation & Contracts
- ✓ **Phase 1**: Discovery System
- ✓ **Phase 2**: Workflow Engine
- ✓ **Phase 3**: Implementation Subagents
- ✓ **Phase 4**: Build & Troubleshooting System
- ✓ **Phase 5**: Validation System
- ✓ **Phase 6**: WebView UI System
- ✓ **Phase 7**: Polish & Enhancement

**System status**: Production ready.

## Requirements

### Software

**Required:**
- macOS 13+ (Sonoma recommended)
- Claude Code CLI

**Dependencies (validated/installed via `/setup`):**
- Xcode Command Line Tools (`xcode-select --install`)
- JUCE 8.0.0+ (audio plugin framework)
- Python 3.8+ (build scripts)
- CMake 3.15+ (build system)
- pluginval (plugin validation tool)
- Git

### Hardware

- Apple Silicon or Intel Mac
- 8GB RAM minimum (16GB recommended)
- 2GB free disk space per plugin

### Knowledge

- **Zero programming required**
- Basic understanding of audio plugin concepts (parameters, presets, DAW usage)
- Ability to describe sonic goals and UX preferences

## Contributing

This system is designed for personal use and learning. If you build something interesting with it, share your creative briefs and mockups—the process is the valuable part, not the compiled binaries.

## License

MIT - Use freely, modify as needed, share what you learn.

## Acknowledgments

Built with:

- [JUCE](https://juce.com/) - Cross-platform audio application framework
- [Claude Code](https://claude.com/claude-code) - AI-assisted development environment
- [Anthropic](https://anthropic.com/) - Claude AI models

---

**Start building**: `/dream`
