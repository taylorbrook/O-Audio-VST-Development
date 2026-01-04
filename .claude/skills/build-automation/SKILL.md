# build-automation

---
name: build-automation
description: Orchestrates plugin builds and installation via build script with comprehensive failure handling. Use when build or compile is needed, build fails, compilation errors occur, or during plugin installation. Invoked by plugin-workflow, plugin-improve, and plugin-lifecycle skills.
---

<!--
TABLE OF CONTENTS:
1. Success Criteria (lines 8-19)
2. Purpose (lines 21-34)
3. Context Mechanism (lines 36-58)
4. Build Workflow (lines 60-128)
5. Failure Protocol (lines 130-179)
6. Success Protocol (lines 181-247)
7. Integration Examples (line 249)
8. Error Handling Rules (lines 253-281)
9. Testing & Debugging (lines 283-285)
10. Common Issues (lines 287-289)

NOTE: Line numbers approximate, update after refactoring
-->

## Success Criteria

This skill succeeds when:
1. Build script executes and returns exit code
2. Build result communicated to user (success or failure message)
3. Log file path displayed
4. Context-appropriate decision menu presented
5. User choice captured and executed
6. Control returned to invoking skill

This skill fails when:
- Plugin directory doesn't exist
- Build script not found or not executable
- User chooses "Wait" and doesn't resume (partial success - workflow paused intentionally)

## Purpose

Orchestrates plugin builds via `scripts/build-and-install.sh` with comprehensive failure handling.

**Invokers:** plugin-workflow (Stages 2-5), plugin-improve (Phase 7), plugin-lifecycle (verification)
**Invokes:** build script, troubleshooter agent (on failure, user option 1)

**Key behaviors:**
- Context-aware build flags (Stage 2 uses `--no-install`, others full build)
- Structured 5-option failure menu (never auto-retry)
- Context-specific success menus (different per stage)
- Always returns control to invoker (never continues autonomously)

## Context Mechanism

When invoked, this skill receives context via invocation parameters:

```json
{
  "plugin_name": "PluginName",
  "stage": 0 | 2 | 3 | 4 | 5 | null,  // Stage numbers: 0=Planning, 2=Foundation, 3=DSP, 4=GUI, 5=Validation
  "invoker": "plugin-workflow" | "plugin-improve" | "plugin-lifecycle" | "manual",
  "build_flags": ["--no-install"] | ["--dry-run"] | []
}
```

**Context is preserved throughout skill execution:**
- Stored at skill entry (before entering Build Workflow)
- Reused on retry without re-prompting user
- Determines build flags, success menu, and return behavior

**Manual invocation detection:**
- When `invoker: "manual"` (user says "build PluginName" directly without workflow context)
- Prompt: "Use --dry-run to preview build commands? (y/n)"
- No stage-specific success menu (generic completion message only)

<workflow name="build_workflow">

## Build Workflow

When invoked, the build-automation skill follows this workflow:

### Build Workflow Progress Checklist

Copy this checklist to track your progress:

```
Build Progress:
- [ ] Step 1: Input validation
- [ ] Step 2: Determine build flags
- [ ] Step 3: Invoke build script
- [ ] Step 4: Monitor build output
- [ ] Step 5: Capture exit code
- [ ] Step 6: Execute success/failure protocol
```

### 1. Input Validation

- Verify plugin name provided
- Check plugin directory exists: `test -d "plugins/$PLUGIN_NAME"`
- Validate CMakeLists.txt present: `test -f "plugins/$PLUGIN_NAME/CMakeLists.txt"`

### 2. Determine Build Flags

Context-aware flag selection:

- **Stage 2 (Foundation)**: Always use `--no-install` flag (verify compilation only, no installation)
- **Stages 3-5 (DSP/GUI/Validation)**: Full build with installation (no flags)
- **plugin-improve**: Full build with installation (no flags)
- **Manual invocation** (`invoker: "manual"`): Ask if they want `--dry-run` to preview commands

### 3. Invoke Build Script

Execute build script with appropriate flags:

```bash
./scripts/build-and-install.sh [PluginName] [flags]
```

Display build progress in real-time using Bash tool.

### 4. Monitor Build Output

- Capture stdout and stderr
- Display progress messages to user
- Watch for error indicators
- Track build duration

### 5. Capture Exit Code

Check build script exit code:

- **Exit 0**: Build succeeded → proceed to Success Protocol
- **Exit 1**: Build failed → proceed to Failure Protocol

### 6. Log File Location

Always show log file path after build attempt:

```
Build log: logs/[PluginName]/build_TIMESTAMP.log
```

User can review full build output from log file if needed.

</workflow>

<decision_gate name="build_failure_handling" blocking="true">

## Failure Protocol

When build fails (exit code 1):

1. Extract error from log (last 50 lines or first error indicator)
2. Present structured menu (see menu structure below)
3. WAIT for user choice (NEVER auto-proceed)
4. Execute chosen option

**Iterative Debugging Loop**:
```
┌─────────────────────────────┐
│ Present failure menu        │
│ ↓                           │
│ Wait for user choice        │
│ ↓                           │
│ Execute option 1-3          │
│ ↓                           │
│ Return to menu (loop)       │ ← Continue until user chooses option 4 (Wait) or 5 (Other/abort)
└─────────────────────────────┘
```

Menu structure:
```
⚠️ Build failed

What would you like to do?
1. Investigate - Run troubleshooter agent (recommended)
2. Show me the build log - Display full build output
3. Show me the code - Open source files with error
4. Wait - I'll fix manually
5. Other
```

**Option Actions** (summary - full details in references/failure-protocol.md):

1. **Investigate**: Invoke troubleshooter agent via Task tool with build log context → display findings → re-present menu
2. **Show build log**: Display full log contents with highlighted error section → re-present menu
3. **Show code**: Extract file/line from error, display relevant code section (±5 lines) → re-present menu
4. **Wait**: Exit skill, preserve context for retry (user says "retry build" when ready) → do NOT re-present menu
5. **Other**: Capture custom action (retry, skip, abort, or execute specific instruction)

**Menu Loop**:
- Options 1-3: Execute action, then re-present menu (iterative debugging)
- Option 4: Exit skill cleanly (workflow paused)
- Option 5: Execute user's choice, behavior depends on instruction

</decision_gate>

<decision_gate name="build_success_handling" blocking="false">

## Success Protocol

When build succeeds (exit code 0), follow this workflow:

### 1. Verify Success

- Confirm build script exit code 0
- Check log contains "Phase 7: Verification" success message
- Extract installed plugin locations from script output

### 2. Display Success Message

Use this template:

```
✓ Build successful

Built and installed:
- VST3: ~/Library/Audio/Plug-Ins/VST3/[ProductName].vst3
- AU: ~/Library/Audio/Plug-Ins/Components/[ProductName].component

Build time: [duration]
Log: logs/[PluginName]/build_TIMESTAMP.log
```

For `--no-install` builds (Stage 2):

```
✓ Build successful (compilation verified, not installed)

Built artifacts:
- VST3: plugins/[PluginName]/build/[PluginName]_artefacts/Release/VST3/[ProductName].vst3
- AU: plugins/[PluginName]/build/[PluginName]_artefacts/Release/AU/[ProductName].component

Build time: [duration]
Log: logs/[PluginName]/build_TIMESTAMP.log
```

### 3. Commit Build Success

Before presenting decision menu, commit the successful build:

**If invoked from workflow** (plugin-workflow, plugin-improve):
```bash
git add .
git commit -m "chore: build [PluginName] successful"
```

**If manual invocation**: Skip commit (user manages their own git workflow)

### 4. Check Workflow Mode

Before presenting decision menu, check if auto-progression is configured:

1. **Check invoker type**: If `invoker: "plugin-workflow"`, read workflow mode from .claude/preferences.json
2. **Express mode**: Skip menu, exit immediately with SUCCESS status (auto-progression)
3. **Manual mode**: Present context-aware decision menu (Step 5)
4. **Manual invocation** (`invoker: "manual"`): Always present menu (no preferences check)

### 5. Context-Aware Decision Menu

Load context-appropriate menu from `assets/success-menus.md` based on `context.stage` parameter

<handoff_protocol name="return_to_invoker">

After user makes decision from success menu:

1. Identify invoking skill (stored at entry)
2. Return control using appropriate mechanism:
   - **plugin-workflow**: Exit with status SUCCESS, await stage dispatcher
   - **plugin-improve**: Exit with status SUCCESS, await improvement workflow
   - **Manual invocation**: Exit with final status message

**State update responsibility:**
- build-automation commits the successful build (Step 3 above)
- Invoking skill (plugin-workflow) handles .continue-here.md and PLUGINS.md updates
- Proper separation: build-automation handles build artifacts, orchestrator handles workflow state

NEVER continue workflow autonomously after success.
NEVER invoke next stage directly.
NEVER update .continue-here.md or PLUGINS.md (invoking skill's responsibility).
ALWAYS exit and let invoking skill/workflow orchestrate next action.

Exit the skill cleanly:
1. Do NOT invoke any other skills or agents
2. Do NOT continue to next stage
3. Do NOT update state files (.continue-here.md, PLUGINS.md)
4. Simply complete skill execution
5. The invoking skill/workflow will detect completion and proceed according to its own logic

</handoff_protocol>

</decision_gate>

## Integration Examples

See complete scenario walkthroughs in `references/integration-examples.md`

## Error Handling Rules

<critical_rule name="never_auto_retry" enforcement="blocking">

**NEVER automatically retry a failed build without explicit user decision.**

When build fails:
1. MUST present failure menu (lines 99-112)
2. MUST await user choice
3. MUST NOT execute any retry logic autonomously

Violation consequences: User loses control of workflow, unexpected builds consume resources, debugging becomes impossible.

</critical_rule>

<state_requirement name="retry_context_preservation">

When user requests build retry after manual fix:

MUST preserve from original invocation:
- Build flags (`--no-install`, `--dry-run`, or none)
- Invoking skill (plugin-workflow, plugin-improve, manual)
- Invoking stage (Stage 0, 2, 3, 4, 5, or N/A)
- Last decision point (for return navigation)

Context is already stored from skill entry (see "Context Mechanism" section).
Reuse stored context on retry without re-prompting user.

</state_requirement>

### Handle Missing Dependencies

If build script fails with dependency errors (CMake, Ninja, JUCE not found):

1. Display specific missing dependency
2. Provide installation command (e.g., "Install with: brew install ninja")
3. Suggest: "Run /setup command to validate full environment configuration"
4. After user installs, offer: "Retry build now?"

### Parse Errors Intelligently

Extract meaningful error information for troubleshooter:

- **CMake errors**: Configuration issues, missing modules, path problems
- **Compilation errors**: Syntax, type mismatches, missing declarations
- **Linker errors**: Missing symbols, library issues
- **Installation errors**: Permission issues, path conflicts

Pass full context to troubleshooter, not just error message.

## Testing & Debugging

See `references/testing-guide.md` for manual testing procedures

## Common Issues

See `references/troubleshooting.md` for common issues and solutions
