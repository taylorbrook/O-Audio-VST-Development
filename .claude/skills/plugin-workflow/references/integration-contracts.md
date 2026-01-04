# Integration Contracts

Complete contract definitions for plugin-workflow skill interactions with subagents and system components.

## Contents

- [foundation-shell-agent Contract](#foundation-shell-agent-contract)
- [dsp-agent Contract](#dsp-agent-contract)
- [gui-agent Contract](#gui-agent-contract)
- [validator Contract](#validator-contract)
- [build-automation Contract](#build-automation-contract)
- [context-resume Contract](#context-resume-contract)
- [/implement Command Contract](#implement-command-contract)
- [Data Formats](#data-formats)

## Contract Overview

All subagents follow a consistent contract pattern:

**Input:** Prompt with contracts + Required Reading + stage-specific instructions
**Output:** JSON report with status, completionStatement, filesCreated, issues
**Error Handling:** Return error status with recovery suggestions

---

## foundation-shell-agent Contract

**When:** Stage 1 (Foundation) implementation

**Sends via Task tool:**
```
Implement Stage 1 (Foundation) for [PluginName].

**Contracts:**
- creative-brief.md: [full content]
- architecture.md: [full content]
- plan.md: [full content]

**Required Reading:**
[juce8-critical-patterns.md content]

**Directive:** Create foundation files (PluginProcessor.h/cpp, PluginEditor.h/cpp, CMakeLists.txt).
Return JSON report when complete.
```

**Expects:** JSON report conforming to `.claude/schemas/subagent-report.json`:
```json
{
  "agent": "foundation-shell-agent",
  "status": "success",
  "outputs": {
    "plugin_name": "PluginName",
    "source_files_created": [
      "Source/PluginProcessor.h",
      "Source/PluginProcessor.cpp",
      "Source/PluginEditor.h",
      "Source/PluginEditor.cpp",
      "CMakeLists.txt"
    ]
  },
  "issues": [],
  "ready_for_next_stage": true
}
```

**Error handling:**
- Missing contract → foundation-shell-agent returns status: "failure", orchestrator blocks progression
- File creation error → present retry menu to user
- Schema validation failure → log error, block progression

**Contract:** foundation-shell-agent creates files, returns report. Orchestrator validates schema, commits changes, updates state.

---

## dsp-agent Contract

**When:** Stage 2 (DSP) implementation

**Sends via Task tool:**
```
Implement Stage 2 [Phase X] for [PluginName].

**Contracts:**
- architecture.md: [full content]
- parameter-spec.md: [full content]
- plan.md: [full content]

**Required Reading:**
[juce8-critical-patterns.md content]

**Directive:** Implement DSP components and processing chain [for Phase X].
Return JSON report when complete.
```

**Expects:** JSON report conforming to `.claude/schemas/subagent-report.json`

**Error handling:**
- Missing DSP component → dsp-agent reports in issues array
- Real-time safety violation → warn in checkpoint menu
- Phase incomplete → ready_for_next_stage: false, orchestrator blocks

**Contract:** dsp-agent implements DSP, returns report. Orchestrator handles phase checkpoints and state updates.

---

## gui-agent Contract

**When:** Stage 3 (GUI) implementation

**Sends via Task tool:**
```
Implement Stage 3 (GUI) for [PluginName].

**Contracts:**
- parameter-spec.md: [full content]
- Mockup files: [list all v[N]-*.* files]

**Required Reading:**
[juce8-critical-patterns.md content]

**Directive:** Integrate WebView UI with parameter bindings.
Return JSON report when complete.
```

**Expects:** JSON report conforming to `.claude/schemas/subagent-report.json`

**Error handling:**
- Missing mockup → gui-agent returns error, cannot proceed
- Relay/attachment mismatch → validate counts match parameters
- WebView integration failed → present recovery menu

**Contract:** gui-agent integrates UI, returns report. Orchestrator validates relay/attachment counts match parameter-spec.md.

---

## validator Contract

**When:** Automatic validation after each stage completion

**Sends via Task tool:**
```
Validate Stage [N] completion for [PluginName].

**Stage:** [N]
**Plugin:** [PluginName]
**Contracts:**
- [stage-specific contracts]

**Expected outputs for Stage [N]:**
[stage-specific expected outputs]

Return JSON validation report.
```

**Expects:** JSON report conforming to `.claude/schemas/validator-report.json`

**Error handling:**
- Validation failure → include in checkpoint menu, let user decide
- Override file exists → validator handles suppression
- Schema validation failure → log warning, continue anyway (validation is advisory)

**Contract:** Validator runs automatically after each stage. Orchestrator presents validation results in checkpoint menu. User makes final decision.

---

## build-automation Contract

**When:** After stage completion to verify compilation

**Sends via Skill tool:**
```json
{
  "plugin_name": "PluginName",
  "build_type": "Debug",
  "operation": "build"
}
```

**Expects:**
```json
{
  "success": true,
  "build_log": "logs/PluginName/build-timestamp.log",
  "binary_path": "builds/PluginName/Debug/PluginName.vst3",
  "warnings": 0,
  "errors": 0
}
```

**Error handling:**
- Build failure → present recovery menu (retry, investigate, manual fix)
- Missing CMakeLists.txt → check foundation stage completion
- Compiler not found → delegate to system-setup skill

**Contract:** Orchestrator NEVER builds directly. Always delegates to build-automation skill.

---

## context-resume Contract

**Receives:**
- Plugin name (string)
- Stage number (integer 0-3)
- Phase (string or null)
- Orchestration mode (boolean, must be true)
- Handoff context (full .continue-here.md content)

**Returns:**
- Resumes at specified stage/phase
- Executes remaining workflow
- Presents checkpoint menus at each stage
- Updates state files throughout

**Contract:** context-resume is READ-ONLY. Orchestrator owns all state updates. context-resume only loads and routes.

---

## /implement Command Contract

**Receives:**
- Plugin name (string)
- Entry stage (typically Stage 1, or wherever workflow paused)

**Returns:**
- Executes workflow from entry stage through Stage 3
- Presents checkpoint menus at each stage
- Automatic validation after each stage
- Updates PLUGINS.md to ✅ Working when complete

**Contract:** Command expands to prompt that invokes this skill. Skill handles full workflow.

---

## Data Formats

Subagent reports conform to `.claude/schemas/subagent-report.json`.

**Required fields**: agent, status, outputs, issues, ready_for_next_stage

For complete schema definitions, see `.claude/schemas/`.
