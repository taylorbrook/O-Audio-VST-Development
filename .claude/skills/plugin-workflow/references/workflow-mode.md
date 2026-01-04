# Workflow Mode Detection and Handling

## Purpose

Determine whether to auto-progress (express mode) or present decision menus (manual mode) at checkpoints.

## Mode Sources

**Priority order:**
1. Environment variables (set by /implement or /continue command)
2. .continue-here.md field (for resumed workflows)
3. Default to "manual"

## Environment Variables

```bash
WORKFLOW_MODE="express" | "manual"  # Default: "manual"
AUTO_TEST="true" | "false"          # Default: "false"
AUTO_INSTALL="true" | "false"       # Default: "false"
AUTO_PACKAGE="true" | "false"       # Default: "false"
```

## Mode Persistence

Store mode in .continue-here.md for resume scenarios:

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

  // Check .continue-here.md for resumed workflows
  const handoffPath = `plugins/${pluginName}/.continue-here.md`
  if (fileExists(handoffPath)) {
    const content = readFile(handoffPath)
    const yaml = parseFrontmatter(content)
    if (yaml.workflow_mode) {
      mode = yaml.workflow_mode  // Preserved from initial /implement
    }
  }

  // Validate mode value
  if (mode !== "express" && mode !== "manual") {
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
