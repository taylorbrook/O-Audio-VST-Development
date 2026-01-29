# Claude-assisted VST Development

An AI-assisted JUCE plugin development system that enables conversational creation of professional VST3 and AU audio plugins for macOS. Design and build custom audio processors through natural dialogue with Claude Code—no programming experience required.

#VST building system based upon **[TÂCHES](https://youtube.com/tachesteaches)**

## Why This Exists

To make the development of VST and AU plugins using Claude Code more achievable.

## What You Can Build

- **Effects**: Reverbs, delays, distortion, modulation, filters, dynamics processors
- **Synthesizers**: Subtractive, FM, wavetable, granular, additive
- **Utilities**: Analyzers, meters, routing tools, MIDI processors
- **Experimental**: Custom DSP algorithms, hybrid processors, generative tools

All plugins compile to native VST3/AU formats compatible with any DAW (Ableton, Logic, Reaper, etc.).

## How It Works

### 1. Start (`/start`)

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

**GSD Integration**: Each stage follows the discuss → research → plan → execute → verify cycle with explicit documentation at each phase.

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

- macOS (Sonoma or later recommended)
- Claude Code CLI

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

Skip flags: `--skip-discuss`, `--skip-research`, `--skip-verify`

### Testing & Validation

| Command | Purpose |
|---------|---------|
| `/test [Name]` | Run pluginval and automated validation suite |
| `/show-standalone [Name]` | Open plugin UI in Standalone mode for visual inspection |

### Post-Completion

| Command | Purpose |
|---------|---------|
| `/improve [Name]` | Add features, fix bugs (with versioning and regression testing) |
| `/install-plugin [Name]` | Deploy to `~/Library/Audio/Plug-Ins/` |
| `/package [Name]` | Create signed PKG installer for distribution |
| `/publish [Name]` | Release via GitHub Actions CI/CD |

### Lifecycle Management

| Command | Purpose |
|---------|---------|
| `/uninstall [Name]` | Remove from system folders (keep source code) |
| `/reset-to-ideation [Name]` | Remove implementation, keep idea/mockups |
| `/destroy [Name]` | Complete removal with verified backup |
| `/clean [Name]` | Interactive cleanup menu (choose operation) |
| `/reconcile [Name]` | Fix out-of-sync state files |
| `/clear-cache [Name]` | Clear validation cache |

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

### Research & Troubleshooting

| Command | Purpose |
|---------|---------|
| `/research [topic]` | Deep multi-agent investigation (3-level protocol) |
| `/doc-fix` | Document solved problem to knowledge base |
| `/add-critical-pattern` | Add current problem to Required Reading |

### System

| Command | Purpose |
|---------|---------|
| `/pfs` | Load Plugin Freedom System architecture context |
| `/templates` | Browse reusable code patterns and snippets |

## Project Structure

```
vst-development/
├── plugins/                          # Plugin source code
│   └── [PluginName]/
│       ├── .planning/                # GSD-style planning (plugin-local)
│       │   ├── BRIEF.md              # Creative vision
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
│   ├── build-and-install.sh          # 7-phase build pipeline
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

## Acknowledgments

Built with:

- [JUCE](https://juce.com/) - Cross-platform audio application framework
- [Claude Code](https://claude.com/claude-code) - AI-assisted development environment
- [Anthropic](https://anthropic.com/) - Claude AI models

---

**Start building**: `/start`
