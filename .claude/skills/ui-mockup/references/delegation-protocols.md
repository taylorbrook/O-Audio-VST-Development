# Delegation Protocols

## Overview

This skill uses pure orchestrator pattern with zero direct file generation. All files created by specialized subagents via Task tool.

## Phase A: Design Iteration (ui-design-agent)

### Purpose
Generate YAML specification and browser-testable HTML mockup for rapid iteration.

### Outputs
1. `v[N]-ui.yaml` - Machine-readable design specification
2. `v[N]-ui-test.html` - Browser-testable mockup (no JUCE required)

### Invocation Protocol

**Before invoking ui-design-agent:**

1. **Read context files in parallel:**
   - `creative-brief.md` (if exists)
   - Aesthetic template (if user selected one in Phase 0)
   - Previous version YAML (if iteration)

2. **Detect version number:**
   - List all `v*-ui.yaml` files in `.ideas/mockups/`
   - Extract version numbers, find highest
   - Increment for new version

3. **Construct comprehensive prompt:**
   - Plugin name
   - Creative brief summary (if exists)
   - User requirements (layout, style, controls from Phases 1-3)
   - Aesthetic template content (if selected)
   - Version number (v[N])
   - Refinement feedback (if iteration from Phase 5.5)

4. **Invoke via Task tool:**
   ```
   Task tool:
   - subagent_type: "ui-design-agent"
   - description: "Generate UI mockup v[N] for [PluginName]"
   - prompt: [constructed prompt with all context]
   ```

5. **Wait for JSON return message:**
   ```json
   {
     "status": "success" | "error",
     "phase": "design-iteration",
     "version": N,
     "filesCreated": ["v[N]-ui.yaml", "v[N]-ui-test.html"],
     "validationPassed": true | false,
     "stateUpdated": true | false,
     "commitHash": "abc123",
     "error": "..." (if status == "error")
   }
   ```

6. **Handle result:**
   - `status == "error"` → Present error menu (retry/manual/debug/cancel)
   - `validationPassed == false` → Agent already retried internally, present error menu
   - `stateUpdated == false` → Log warning, offer state recovery menu
   - `status == "success"` → Continue to Phase 5.5 decision menu

### Error Recovery Menu

```
✗ Design generation failed: [error message]

What would you like to do?

1. Retry - Invoke ui-design-agent again
2. Manual fix - I'll create files myself
3. Debug - Show agent output
4. Cancel - Stop workflow
5. Other

Choose (1-5): _
```

### Iteration Support

When user selects "Iterate" in Phase 5.5 menu:
- Collect refinement feedback
- Invoke **NEW** ui-design-agent instance (fresh context)
- Include previous version number for reference
- Increment version number (v2, v3, etc.)
- Pass refinement requirements in prompt

Why fresh context: After 4-5 iterations, accumulated context blocks progression. Fresh agent keeps each iteration lightweight.

## Phase B: Implementation Scaffolding (ui-finalization-agent)

### Purpose
Generate production files ONLY after Phase A design approved via Phase 5.5 decision menu.

### Outputs
3. `v[N]-ui.html` - Production HTML (copy-paste to plugin)
4. `v[N]-PluginEditor.h` - C++ header boilerplate
5. `v[N]-PluginEditor.cpp` - C++ implementation boilerplate
6. `v[N]-CMakeLists.txt` - WebView build configuration
7. `v[N]-integration-checklist.md` - Implementation steps

### Prerequisites

**MUST verify before invoking ui-finalization-agent:**
- User selected option 2 (Finalize) in Phase 5.5 decision menu
- Phase A files exist: `v[N]-ui.yaml` and `v[N]-ui-test.html`
- Finalization marker present in YAML (appended by orchestrator after user approval)

See `references/phase-b-enforcement.md` for guard implementation.

### Invocation Protocol

**Before invoking ui-finalization-agent:**

1. **Read finalized design files in parallel:**
   - `v[N]-ui.yaml` (finalized YAML specification)
   - `v[N]-ui-test.html` (finalized test HTML)
   - `parameter-spec.md` (if exists - for v2+ mockups)

2. **Detect parameter-spec.md generation needs:**
   - If `parameter-spec.md` does NOT exist → Must be generated
   - Check for draft: `parameter-spec-draft.md`
   - Flag for agent to generate spec from finalized YAML

3. **Construct comprehensive prompt:**
   - Plugin name
   - Version number (v[N])
   - Path to finalized YAML
   - Path to finalized HTML
   - Path to `parameter-spec.md` OR flag to generate it
   - Path to draft (if exists and spec generation needed)

4. **Invoke via Task tool:**
   ```
   Task tool:
   - subagent_type: "ui-finalization-agent"
   - description: "Generate implementation files for mockup v[N]"
   - prompt: [constructed prompt with contracts]
   ```

5. **Wait for JSON return message:**
   ```json
   {
     "status": "success" | "error",
     "phase": "implementation-scaffolding",
     "version": N,
     "filesCreated": [
       "v[N]-ui.html",
       "v[N]-PluginEditor-TEMPLATE.h",
       "v[N]-PluginEditor-TEMPLATE.cpp",
       "v[N]-CMakeLists-SNIPPET.txt",
       "v[N]-integration-checklist.md",
       "parameter-spec.md" (if v1 only)
     ],
     "parameterSpecGenerated": true | false,
     "draftConflictDetected": true | false,
     "stateUpdated": true | false,
     "commitHash": "abc123",
     "error": "..." (if status == "error")
   }
   ```

6. **Handle result:**
   - `draftConflictDetected == true` → Agent presented resolution menu to user
   - `status == "error"` → Present error menu (retry/manual/debug/cancel)
   - `stateUpdated == false` → Log warning, offer state recovery menu
   - `status == "success"` → Continue to Phase 10.7 completion menu

### Error Recovery Menu

```
✗ Implementation file generation failed: [error message]

What would you like to do?

1. Retry - Invoke ui-finalization-agent again
2. Manual fix - I'll create files myself
3. Debug - Show agent output
4. Cancel - Stop workflow
5. Other

Choose (1-5): _
```

## State Recovery Protocol

When agent returns `stateUpdated == false`:

```
⚠️ Agent did not update state files

What would you like to do?

1. Verify state - Check if update actually happened
2. Manual update - I'll fix .continue-here.md myself
3. Continue anyway - State not critical for this step
4. Other

Choose (1-4): _
```

## Delegation Rules (STRICT)

**This skill NEVER generates mockup files directly.**

### Enforcement

IF orchestrator attempts to generate files directly:
- STOP execution immediately
- DISPLAY error: "File generation must be delegated to subagents. Use Task tool."

### Correct Delegation Sequence

1. **Orchestrator:** Gather requirements (Phases 0-3.5)
2. **Orchestrator:** Invoke ui-design-agent via Task tool
3. **ui-design-agent:** Generate YAML + test HTML, commit, return JSON report
4. **Orchestrator:** Parse JSON report, present Phase 5.5 decision menu
5. **Orchestrator (if iterate):** Invoke NEW ui-design-agent instance with refinements
6. **Orchestrator (if finalize):** Update creative brief from mockup (Phase 5.6)
7. **Orchestrator:** Invoke ui-finalization-agent via Task tool
8. **ui-finalization-agent:** Generate 5 implementation files, commit, return JSON report
9. **Orchestrator:** Parse JSON report, present completion menu (Phase 10.7)

### Why Dispatcher Pattern

- **Context isolation:** Each subagent runs in fresh context
- **No accumulation:** Prevents context window bloat during iteration
- **Clean separation:** User interaction in orchestrator, file generation in agents
- **Scalability:** 4-5 iterations don't block progression from accumulated context

## Tool Usage by Phase

### Orchestrator (ui-mockup skill)
- **Read:** Load context files (creative-brief.md, aesthetics, previous versions)
- **Task:** Invoke subagents (ui-design-agent, ui-finalization-agent)
- **AskUserQuestion:** Gather requirements (Phases 1-3), internal routing
- **Inline menus:** Decision menus (Phase 5.5, Phase 10.7)

### Subagents (ui-design-agent, ui-finalization-agent)
- **Read:** Load specifications (YAML, HTML, parameter-spec.md)
- **Write:** Create mockup files (7 total files across both agents)
- **Edit:** Update state files (.continue-here.md, PLUGINS.md)
- **Bash:** Git commits, file operations

Orchestrator NEVER uses Write or Edit for mockup files.
