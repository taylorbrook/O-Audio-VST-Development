# Phase 1 Agent Audit

**Plan:** 01-01 (Audit and Inventory)
**Date:** 2026-01-30
**Status:** Complete

---

## Agent Inventory

### Core Orchestration Agents (9)

| Agent | Purpose | Inputs | Outputs | Tools |
|-------|---------|--------|---------|-------|
| plugin-workflow | Orchestrates stages 1-4 with GSD phase cycles | plugin_name, start_stage, skip_phases, express_mode | CONTEXT.md, RESEARCH.md, PLAN.md, SUMMARY.md, VERIFICATION.md per stage | Task, Bash, Read, Write, Edit, AskUserQuestion |
| build-automation | Orchestrates builds and installation via build script | plugin_name, stage, invoker, build_flags | Build logs, installation status, success/failure report | Bash, Read, Edit, Write |
| plugin-ideation | Adaptive brainstorming for new plugins and improvements | User description (free-form) | BRIEF.md, PLUGINS.md entry | Read, Write, Bash |
| plugin-planning | Stage 0 research and planning orchestration | BRIEF.md, parameter-spec.md | ARCHITECTURE.md, ROADMAP.md, CONTEXT.md | Read, Write, Edit, Bash, Task, WebSearch, Grep, Glob |
| plugin-testing | Manual testing validation (automated testing now in workflow) | plugin_name, test_mode (1-3) | Test logs, PLUGINS.md test_status | Read, Bash, Task |
| plugin-improve | Fix bugs, add features to completed plugins | plugin_name, improvement_request | CHANGELOG.md, updated source files, version bump | Read, Write, Edit, Bash, Task |
| ui-mockup | Orchestrates WebView UI mockup creation | plugin_name, parameter_spec | v[N]-ui.yaml, v[N]-ui-test.html, 5 implementation files | Read, Task, AskUserQuestion |
| plugin-lifecycle | Manage plugin installation/uninstallation/reset/destroy | plugin_name, mode (1-4) | System folder installations, PLUGINS.md updates | Bash, Read, Edit, Write |
| deep-research | Multi-level autonomous investigation for complex problems | topic, context, starting_level | Research reports, recommendations | Read, Grep, Task, WebSearch |

### Supporting Skills (11)

| Skill | Purpose | Invoked By |
|-------|---------|------------|
| context-resume | Load plugin context from STATUS.md to resume work | /continue command, plugin-workflow |
| aesthetic-dreaming | Create aesthetic templates without first creating a plugin | /aesthetic command, manual |
| system-setup | Validate and configure all development dependencies | /setup command, new user onboarding |
| plugin-packaging | Create branded PKG installers for distribution | /package command, manual |
| plugin-publishing | Cross-platform releases via GitHub Actions CI/CD | /publish command, manual |
| troubleshooting-docs | Capture solved problems as categorized documentation | deep-research, plugin-improve, /doc-fix |
| ui-template-library | Manage aesthetic templates - save/apply/list/delete | ui-mockup, aesthetic-dreaming |
| workflow-reconciliation | Reconcile workflow state files when out of sync | /reconcile command, skill detection |
| module-system | Manage reusable code modules across plugins | /module:* commands |
| plugin-phases | GSD-style phase commands for granular control | /plugin-discuss, /plugin-research, etc. |
| plugin-context | Core plugin context management commands | /plugin-list, /plugin-focus, etc. |

### Stage-Specific Subagents (6)

These agents are invoked by plugin-workflow during specific stages:

| Agent | Stage | Purpose | Model |
|-------|-------|---------|-------|
| foundation-shell-agent | Stage 1 | Create JUCE project structure and APVTS parameters | Sonnet |
| dsp-agent | Stage 2 | Implement audio processing and DSP algorithms | Sonnet/Opus (complexity-dependent) |
| gui-agent | Stage 3 | Integrate WebView UI with JUCE C++ code | Sonnet |
| polish-agent | Stage 4 | Presets, optimization, edge cases, release prep | Sonnet |
| research-planning-agent | Stage 0 | DSP research and architecture planning | Sonnet |
| validation-agent | Post-stage | Semantic validation and pluginval testing | Opus |

---

## Input/Output Analysis

### plugin-workflow

**Actual Inputs:**
- `plugin_name` (string, required) - Name of plugin to implement
- `start_stage` (int, optional, default 1) - Stage to begin at
- `skip_phases` (array, optional) - Phases to skip: ["discuss", "research", "verify"]
- `express_mode` (bool, optional) - Auto-advance without menus

**Actual Outputs:**
- `plugins/[Name]/.planning/stages/[N]-[stage]/CONTEXT.md` - Per-stage context
- `plugins/[Name]/.planning/stages/[N]-[stage]/RESEARCH.md` - Per-stage research
- `plugins/[Name]/.planning/stages/[N]-[stage]/PLAN.md` - Per-stage task breakdown
- `plugins/[Name]/.planning/stages/[N]-[stage]/SUMMARY.md` - Per-stage completion summary
- `plugins/[Name]/.planning/stages/[N]-[stage]/VERIFICATION.md` - Per-stage verification
- `plugins/[Name]/.planning/STATUS.md` - Updated workflow state
- `.claude/plugin-registry.json` - Updated registry entry
- `PLUGINS.md` - Updated plugin table

**State Changes:**
- Registry stage/phase updated at each transition
- STATUS.md phase progress table updated
- Git commits at phase and stage boundaries

---

### build-automation

**Actual Inputs:**
- `plugin_name` (string, required) - Plugin to build
- `stage` (int, optional) - Current stage (0, 2, 3, 4, 5, or null)
- `invoker` (string, optional) - "plugin-workflow" | "plugin-improve" | "plugin-lifecycle" | "manual"
- `build_flags` (array, optional) - ["--no-install"] | ["--dry-run"] | []

**Actual Outputs:**
- Build log: `logs/[PluginName]/build_TIMESTAMP.log`
- VST3 binary: `~/Library/Audio/Plug-Ins/VST3/[Product].vst3`
- AU binary: `~/Library/Audio/Plug-Ins/Components/[Product].component`
- Success/failure report to user

**State Changes:**
- Git commit on successful build (if invoked from workflow)
- No direct state file updates (invoker handles STATUS.md, PLUGINS.md)

---

### plugin-ideation

**Actual Inputs:**
- User description (free-form text)
- `plugin_name` (string, optional) - If improving existing plugin

**Actual Outputs:**
- `plugins/[Name]/.planning/BRIEF.md` - Creative vision document
- `PLUGINS.md` - New plugin entry with status "Ideated"
- `plugins/[Name]/.planning/STATUS.md` - Initial status file

**State Changes:**
- New plugin directory created
- Plugin added to registry with status "Ideated"

---

### plugin-planning

**Actual Inputs:**
- `plugin_name` (string, required) - Plugin to plan
- `BRIEF.md` (file, required) - Creative vision from ideation
- `parameter-spec.md` OR `parameter-spec-draft.md` (file, required) - Parameter definitions
- `mockups/*.yaml` (files, optional) - UI mockup files

**Actual Outputs:**
- `plugins/[Name]/.planning/research/ARCHITECTURE.md` - DSP architecture specification
- `plugins/[Name]/.planning/ROADMAP.md` - Implementation plan with complexity score
- `plugins/[Name]/.planning/stages/0-ideation/CONTEXT.md` - Stage 0 discuss output
- `plugins/[Name]/.planning/STATUS.md` - Updated with Stage 0 progress

**State Changes:**
- Status updated to "Planning Complete"
- Registry updated with Stage 0 completion

---

### plugin-testing

**Actual Inputs:**
- `plugin_name` (string, required) - Plugin to test
- `test_mode` (int, optional) - 1: Automated, 2: Build+Pluginval, 3: Manual DAW

**Actual Outputs:**
- Test log: `logs/[PluginName]/test_[timestamp].log`
- `plugins/[Name]/.planning/STATUS.md` - Updated test status
- `PLUGINS.md` - Test status column updated

**State Changes:**
- Test results recorded in PLUGINS.md
- STATUS.md stage updated to "testing_complete"

---

### plugin-improve

**Actual Inputs:**
- `plugin_name` (string, required) - Plugin to improve
- `improvement_request` (string, required) - What to fix/add
- `version_bump` (string, optional) - "patch" | "minor" | "major"

**Actual Outputs:**
- `plugins/[Name]/CHANGELOG.md` - New version entry
- Modified source files (implementation changes)
- `PLUGINS.md` - Updated version and last_updated
- `plugins/[Name]/NOTES.md` - Updated status and timeline
- Backup: `backups/[PluginName]-v[X.Y.Z]-[timestamp]/`
- Git tag: `v[X.Y.Z]`

**State Changes:**
- Version bumped in CMakeLists.txt
- Backup created before changes
- Git tag created for release

---

### ui-mockup

**Actual Inputs:**
- `plugin_name` (string, required) - Plugin to design UI for
- `parameter_spec` (file, required) - Parameter definitions
- `aesthetic_id` (string, optional) - Aesthetic template to apply
- Iteration feedback (string, optional) - Refinements for new version

**Actual Outputs:**
- `plugins/[Name]/.planning/mockups/v[N]-ui.yaml` - Design specification
- `plugins/[Name]/.planning/mockups/v[N]-ui-test.html` - Browser-testable mockup
- `plugins/[Name]/.planning/mockups/v[N]-ui.html` - Production HTML (after finalization)
- `plugins/[Name]/.planning/mockups/v[N]-PluginEditor.h` - C++ header boilerplate
- `plugins/[Name]/.planning/mockups/v[N]-PluginEditor.cpp` - C++ implementation boilerplate
- `plugins/[Name]/.planning/mockups/v[N]-CMakeLists.txt` - WebView build config
- `plugins/[Name]/.planning/mockups/v[N]-integration-checklist.md` - Implementation steps
- `plugins/[Name]/.planning/parameter-spec.md` - Generated on v1 finalization

**State Changes:**
- STATUS.md updated with mockup version
- PLUGINS.md marked as "UI designed" (if in workflow)

---

### plugin-lifecycle

**Actual Inputs:**
- `plugin_name` (string, required) - Plugin to manage
- `mode` (int, required) - 1: Install, 2: Uninstall, 3: Reset to Ideation, 4: Destroy

**Actual Outputs:**
- Mode 1: System folder installations at `~/Library/Audio/Plug-Ins/`
- Mode 2: Removed binaries from system folders
- Mode 3: Implementation removed, idea/mockups preserved
- Mode 4: Complete removal with backup
- `PLUGINS.md` - Status updated
- `plugins/[Name]/NOTES.md` - Status and timeline updated

**State Changes:**
- Plugin status updated to match operation
- System folder contents modified

---

### deep-research

**Actual Inputs:**
- `topic` (string, required) - Research question/problem
- `context` (object, optional) - {plugin_name, stage, error}
- `starting_level` (int, optional) - 1, 2, or 3 (default 1)

**Actual Outputs:**
- Research report at level completion
- Recommendations with confidence assessment
- Source references
- Decision menu for user action

**State Changes:**
- None directly (read-only advisory skill)
- Invokes plugin-improve via Skill tool if user selects "Apply solution"

---

## Workflow Coverage Analysis

### Full Lifecycle Trace

| Stage | Agent | Inputs | Outputs | Notes |
|-------|-------|--------|---------|-------|
| Ideation | plugin-ideation | User description | BRIEF.md, PLUGINS.md entry | Entry point, creates plugin |
| UI Design | ui-mockup | parameter-spec | v[N]-ui.*, parameter-spec.md | Optional but recommended before planning |
| Planning (Stage 0) | plugin-planning → research-planning-agent | BRIEF.md, parameter-spec.md | ARCHITECTURE.md, ROADMAP.md | Subagent handles research |
| Foundation (Stage 1) | plugin-workflow → foundation-shell-agent | Contracts | CMakeLists.txt, PluginProcessor.*, PluginEditor.* | Initial build system |
| DSP (Stage 2) | plugin-workflow → dsp-agent | ARCHITECTURE.md | processBlock implementation | Audio processing |
| GUI (Stage 3) | plugin-workflow → gui-agent | v[N]-ui.html | WebView integration | UI binding |
| Polish (Stage 4) | plugin-workflow → polish-agent | All contracts | Presets, optimization | Final refinements |
| Installation | plugin-lifecycle | plugin_name, mode=1 | System folder binaries | Optional, can skip |
| Distribution | plugin-packaging / plugin-publishing | plugin_name | PKG installer / GitHub release | Optional distribution |

---

## Gaps Identified

### 1. Music Theory / Tuning Agent (HIGH priority)

**Workflow stage affected:** Stage 2 (DSP) for tuning-related plugins

**Current handling:** Ad-hoc implementation in dsp-agent with research-planning-agent assistance

**Impact:**
- JI (Just Intonation) calculations are scattered
- Temperament math requires deep research each time
- Scala file parsing is duplicated across plugins
- No specialized knowledge of tuning systems

**Recommended:** New `tuning-theory-agent` specification OR enhanced dsp-agent specialization mode

**Rationale:** Multiple plugins (O-IntonationPad, O-Lyrica, etc.) need sophisticated tuning math. Currently relies on deep-research or manual implementation.

---

### 2. UI/UX Design Agent (MEDIUM priority)

**Workflow stage affected:** UI mockup generation (ui-mockup skill)

**Current handling:** ui-design-agent and ui-finalization-agent exist but are lightweight

**Impact:**
- Visual design quality varies
- No systematic aesthetic reasoning
- Layout decisions are basic (parameter count heuristics)
- No accessibility considerations

**Recommended:** Enhanced ui-design-agent with design system knowledge

**Rationale:** Current agents generate functional UIs but lack sophisticated design reasoning. aesthetic-dreaming skill partially addresses this.

---

### 3. Performance Profiling Agent (LOW priority)

**Workflow stage affected:** Stage 4 (Polish)

**Current handling:** polish-agent handles optimization but lacks deep profiling

**Impact:**
- CPU profiling is manual
- Memory analysis is ad-hoc
- No automated hotspot detection

**Recommended:** Could be addressed by enhanced polish-agent or separate profiling-agent

**Rationale:** Performance optimization is important but current manual approach works for most plugins.

---

### 4. Cross-Plugin Integration Agent (LOW priority)

**Workflow stage affected:** Post-implementation

**Current handling:** module-system skill handles code reuse

**Impact:**
- No agent specifically handles multi-plugin coordination
- Module extraction is manual
- Dependency tracking is basic

**Recommended:** Current module-system skill is sufficient; could be enhanced later

---

## Overlaps Identified

### 1. plugin-improve vs plugin-workflow

**Both can:** Trigger builds, update state files, commit changes

**Resolution:** Clear separation exists:
- `plugin-workflow` is for initial implementation (Stages 1-4)
- `plugin-improve` is for post-completion changes to working plugins
- Preconditions enforce this: plugin-improve requires status "Working" or "Installed"

**Action:** Document in BOUNDARIES.md, no merge needed

---

### 2. plugin-testing vs validation-agent

**Both can:** Run pluginval, check plugin quality

**Resolution:** Clear separation exists:
- `validation-agent` is a subagent invoked automatically during workflow (Stage 1-4 verification)
- `plugin-testing` is a manual skill invoked via /test command for additional testing
- validation-agent does semantic + runtime checks; plugin-testing is user-facing testing modes

**Action:** Document in BOUNDARIES.md, no merge needed

---

### 3. deep-research vs gsd-phase-researcher

**Both can:** Research technical topics, search documentation

**Resolution:** Different scopes:
- `gsd-phase-researcher` is lightweight research within a workflow phase (research phase in GSD cycle)
- `deep-research` is heavyweight multi-level investigation for complex/novel problems

**Action:** Document in BOUNDARIES.md, consider unifying research approach

---

### 4. context-resume vs workflow-reconciliation

**Both can:** Load state, understand workflow position

**Resolution:** Different purposes:
- `context-resume` is for user-initiated resume of paused work (/continue command)
- `workflow-reconciliation` is for detecting and fixing state drift

**Action:** Document in BOUNDARIES.md, no merge needed

---

## Prioritized Gap List

| Priority | Gap | Rationale |
|----------|-----|-----------|
| 1 | Music theory agent | Multiple plugins need JI/temperament math; current ad-hoc approach is inefficient |
| 2 | UI/UX design enhancement | Visual quality could improve; aesthetic-dreaming partially addresses |
| 3 | Performance profiling | Would help Stage 4 but current manual approach is functional |
| 4 | Cross-plugin integration | module-system skill covers most needs |

---

## Agent Invocation Map

```
User Commands
    │
    ├── /start → plugin-ideation
    │               └── [creates BRIEF.md]
    │
    ├── /mockup → ui-mockup
    │               ├── → ui-design-agent [via Task]
    │               └── → ui-finalization-agent [via Task]
    │
    ├── /plan → plugin-planning
    │               └── → research-planning-agent [via Task]
    │
    ├── /implement → plugin-workflow
    │               ├── Stage 1 → foundation-shell-agent [via Task]
    │               ├── Stage 2 → dsp-agent [via Task]
    │               ├── Stage 3 → gui-agent [via Task]
    │               ├── Stage 4 → polish-agent [via Task]
    │               ├── Post-stage → validation-agent [via Task]
    │               └── Build → build-automation [via Skill]
    │
    ├── /continue → context-resume
    │               └── → [appropriate skill based on state]
    │
    ├── /improve → plugin-improve
    │               ├── → plugin-ideation [if vague request]
    │               ├── → deep-research [Tier 3 investigation]
    │               ├── → build-automation [build phase]
    │               ├── → plugin-testing [test phase]
    │               └── → plugin-lifecycle [install phase]
    │
    ├── /test → plugin-testing
    │               └── → deep-research [on failures]
    │
    ├── /research → deep-research
    │               └── → plugin-improve [on "Apply solution"]
    │
    ├── /install-plugin → plugin-lifecycle (mode 1)
    ├── /uninstall → plugin-lifecycle (mode 2)
    ├── /reset-to-ideation → plugin-lifecycle (mode 3)
    ├── /destroy → plugin-lifecycle (mode 4)
    │
    ├── /package → plugin-packaging
    ├── /publish → plugin-publishing
    ├── /setup → system-setup
    ├── /reconcile → workflow-reconciliation
    │
    └── /doc-fix → troubleshooting-docs
```

---

## Summary

**Core Orchestration Agents (9):** plugin-workflow, build-automation, plugin-ideation, plugin-planning, plugin-testing, plugin-improve, ui-mockup, plugin-lifecycle, deep-research

**Supporting Skills (11):** context-resume, aesthetic-dreaming, system-setup, plugin-packaging, plugin-publishing, troubleshooting-docs, ui-template-library, workflow-reconciliation, module-system, plugin-phases, plugin-context

**Stage Subagents (6):** foundation-shell-agent, dsp-agent, gui-agent, polish-agent, research-planning-agent, validation-agent

**Gaps Identified:** 4 (1 HIGH, 1 MEDIUM, 2 LOW priority)

**Overlaps Identified:** 4 (all resolved with clear boundaries, no merges needed)

**Ready for Plan 02:** Schema creation can proceed using this inventory as source of truth.
