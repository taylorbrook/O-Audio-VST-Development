# Workflow Mode Detection and Handling

## Purpose

Determine whether to auto-progress (express mode) or present decision menus (manual mode) at checkpoints.

## Mode Sources

**Priority order:**
1. Environment variables (set by /implement or /continue command)
2. .planning/STATUS.md field (for resumed workflows)
3. Default to "manual"

## Environment Variables

```bash
WORKFLOW_MODE="express" | "manual" | "auto"  # Default: "manual"
AUTO_TEST="true" | "false"          # Default: "false"
AUTO_INSTALL="true" | "false"       # Default: "false"
AUTO_PACKAGE="true" | "false"       # Default: "false"
```

## Mode Persistence

Store mode in .planning/STATUS.md for resume scenarios:

```yaml
---
plugin: PluginName
stage: 3
workflow_mode: express  # or "manual"
auto_test: false
auto_install: true
auto_package: false
orchestration_mode: true
---
```

## Implementation

```javascript
function getWorkflowMode(pluginName) {
  // Check environment variable first
  let mode = process.env.WORKFLOW_MODE || "manual"

  // Check .planning/STATUS.md for resumed workflows
  const handoffPath = `plugins/${pluginName}/.planning/STATUS.md`
  if (fileExists(handoffPath)) {
    const content = readFile(handoffPath)
    const yaml = parseFrontmatter(content)
    if (yaml.workflow_mode) {
      mode = yaml.workflow_mode  // Preserved from initial /implement
    }
  }

  // Validate mode value
  if (mode !== "express" && mode !== "manual" && mode !== "auto") {
    console.warn(`Invalid workflow_mode: ${mode}, defaulting to manual`)
    mode = "manual"
  }

  return mode
}

// Get mode at skill start
const workflowMode = getWorkflowMode(pluginName)
const autoTest = process.env.AUTO_TEST === "true"
const autoInstall = process.env.AUTO_INSTALL === "true"
const autoPackage = process.env.AUTO_PACKAGE === "true"

console.log(`Workflow mode: ${workflowMode}`)
```

## Express Mode Functions

### displayProgressMessage

```javascript
function displayProgressMessage(currentStage, nextStage) {
  const milestones = {
    0: "Research Complete",
    1: "Build System Ready",
    2: "Audio Engine Working",
    3: "UI Integrated",
    4: "Plugin Complete"
  }

  const currentMilestone = milestones[currentStage] || `Stage ${currentStage}`
  const nextMilestone = milestones[nextStage] || `Stage ${nextStage}`

  console.log(`\n✓ ${currentMilestone} → ${nextMilestone}...\n`)
}
```

**Example output:**
```
✓ Build System Ready → Audio Engine Working...
```

### checkForErrors

```javascript
function checkForErrors(result) {
  // Check subagent result for errors
  if (result.status === "error" || result.status === "failure") {
    return true
  }

  // Check for build failures (exit code check)
  if (result.buildFailed === true) {
    return true
  }

  // Check for validation failures
  if (result.testsFailed === true) {
    return true
  }

  return false
}
```

**Error detection:**
- Subagent returned error status
- Build failed (compilation, linking)
- Tests failed (pluginval errors)
- Installation failed

### handleError

```javascript
function handleError(workflowMode, errorType, errorMessage) {
  if (workflowMode === "express") {
    console.log("\n✗ Error detected - dropping to manual mode\n")
    console.log(`Error: ${errorMessage}`)

    // Override mode for remainder of workflow
    workflowMode = "manual"

    // Present error menu (blocking)
    presentErrorMenu(errorType, errorMessage)

    return workflowMode  // Now "manual"
  } else {
    // Already manual mode, just present error menu
    presentErrorMenu(errorType, errorMessage)
    return workflowMode
  }
}
```

**Error interruption:**
- Express mode drops to manual on ANY error
- User sees error details and investigation menu
- Workflow cannot continue without manual intervention

### presentErrorMenu

```javascript
function presentErrorMenu(errorType, errorMessage) {
  console.log(`\n✗ ${errorType}\n`)
  console.log(`Details:\n${errorMessage}\n`)

  console.log("What should I do?")
  console.log("1. Investigate error (deep-research)")
  console.log("2. Show full logs")
  console.log("3. Review code")
  console.log("4. Manual fix (pause workflow)")
  console.log("5. Other")
  console.log("\nChoose (1-5): ")

  // Wait for user input (blocking)
  const choice = getUserInput()
  handleErrorMenuChoice(choice, errorType)
}
```

## Auto Mode Behavior

Auto mode (`--auto`) generates all planning artifacts (CONTEXT.md, RESEARCH.md, PLAN.md) without user interaction. Unlike express mode which auto-advances between phases but still runs each phase normally, auto mode synthesizes planning documents directly from existing contracts.

### Auto-Generate Context (autoGenerateContext)

In auto mode, Claude generates CONTEXT.md from existing contracts instead of running interactive questioning. When auto mode is active, Claude should:

1. Read BRIEF.md, parameter-spec.md, research/ARCHITECTURE.md, and any previous stage VERIFICATION.md
2. Compile a CONTEXT.md that references these source documents, lists requirements extracted from contracts, and notes constraints (follow ARCHITECTURE.md, implement all parameters, real-time safe)
3. Mark the CONTEXT.md as "Auto-Generated from existing contracts (no interactive session)"

<!-- Illustrative: shows Claude what behavior to implement -->
```javascript
// Illustrative pseudocode — describes the behavior Claude should follow
function autoGenerateContext(pluginName, stage) {
  const brief = readFile(`plugins/${pluginName}/.planning/BRIEF.md`)
  const params = readFile(`plugins/${pluginName}/.planning/parameter-spec.md`)
  const arch = readFile(`plugins/${pluginName}/.planning/research/ARCHITECTURE.md`)
  const prevVerification = readFileSafe(`plugins/${pluginName}/.planning/stages/${prevStage}/VERIFICATION.md`)

  const context = {
    source: "Auto-Generated from existing contracts (no interactive session)",
    requirements: extractRequirements(brief, params, arch),
    constraints: ["Follow ARCHITECTURE.md", "Implement all parameters", "Real-time safe"],
    previousStageNotes: prevVerification ? summarize(prevVerification) : null
  }

  writeFile(`plugins/${pluginName}/.planning/stages/${stage}/CONTEXT.md`, formatContext(context))
}
```

### Auto-Generate Research (autoGenerateResearch)

In auto mode, Claude invokes the research phase non-interactively. The research agent reads CONTEXT.md and produces RESEARCH.md without asking clarifying questions. When dispatching the research agent in auto mode, append to the prompt:

> "Mode: Non-interactive (auto mode) -- do not ask clarifying questions. Produce RESEARCH.md directly from the provided CONTEXT.md and existing contracts."

<!-- Illustrative: shows Claude what behavior to implement -->
```javascript
// Illustrative pseudocode — describes the non-interactive research dispatch
function autoGenerateResearch(pluginName, stage) {
  const contextMd = readFile(`plugins/${pluginName}/.planning/stages/${stage}/CONTEXT.md`)

  invokeTask({
    subagentType: "gsd-phase-researcher",
    prompt: `
      Research implementation approach for ${pluginName} stage ${stage}.
      Context: ${contextMd}

      Mode: Non-interactive (auto mode) -- do not ask clarifying questions.
      Produce RESEARCH.md directly from the provided CONTEXT.md and existing contracts.
    `
  })
}
```

### Error Handling in Auto Mode

Auto mode drops to manual mode on ANY error, following the same fallback behavior as express mode. When an error occurs during auto mode:

1. Log the error with context (which phase, what failed)
2. Switch workflow mode to "manual" for the remainder of the workflow
3. Present the standard error menu for user intervention

This ensures that auto mode never silently continues past failures. The user always regains control when something goes wrong.
