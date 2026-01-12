# Dispatcher Pattern

## Overview
The orchestrator uses the dispatcher pattern to route implementation stages to specialized subagents. This ensures separation of concerns - the orchestrator handles workflow state and checkpoints, while subagents handle implementation.

## Dispatcher Logic (Pseudocode)

```javascript
async function dispatchStage(pluginName, stageNumber) {
  // Check preconditions
  const preconditionCheck = checkStagePreconditions(pluginName, stageNumber)

  if (!preconditionCheck.allowed) {
    console.log(`✗ BLOCKED: ${preconditionCheck.reason}`)
    console.log(`Action: ${preconditionCheck.action}`)
    return { status: 'blocked', reason: preconditionCheck.reason }
  }

  // PRE-STAGE SCAN: Check for anti-patterns BEFORE implementation
  const preScanResult = await runPreStageScan(pluginName, stageNumber)
  if (preScanResult.exitCode === 1) {
    console.log(`✗ BLOCKED: Anti-patterns detected before Stage ${stageNumber}`)
    console.log(`Fix issues in existing code before proceeding`)
    return { status: 'blocked', reason: 'pre-stage-scan-failed' }
  }
  if (preScanResult.exitCode === 2) {
    console.log(`⚠️ Warnings detected (non-blocking) - proceeding with caution`)
  }

  // ALWAYS invoke subagents via Task tool for stages 1-3
  switch(stageNumber) {
    case 1:
      // Invoke foundation-shell-agent subagent
      return await invokeSubagent('foundation-shell-agent', {
        pluginName,
        contracts: loadContracts(pluginName),
        requiredReading: 'stage-N-patterns.md'
      })
    case 2:
      // Invoke dsp-agent subagent
      return await invokeSubagent('dsp-agent', {
        pluginName,
        contracts: loadContracts(pluginName),
        requiredReading: 'stage-N-patterns.md'
      })
    case 3:
      // Invoke gui-agent subagent
      return await invokeSubagent('gui-agent', {
        pluginName,
        contracts: loadContracts(pluginName),
        requiredReading: 'stage-N-patterns.md'
      })
    default:
      return { status: 'error', reason: `Invalid stage: ${stageNumber}. Valid stages: 1-3` }
  }
}
```

## Pre-Stage Scan

Before dispatching to a subagent, run the proactive anti-pattern scanner:

```bash
python3 .claude/hooks/validators/pre-stage-scan.py $PLUGIN_NAME $STAGE_NUMBER
```

Exit codes:
- `0`: No issues, proceed with stage
- `1`: Critical issues found, BLOCK workflow until fixed
- `2`: Warnings found, proceed but inform user

The scanner checks stage-specific anti-patterns:
- **Stage 1**: CMake configuration, build system patterns
- **Stage 2**: DSP patterns, real-time safety, juce::dsp API usage
- **Stage 3**: WebView patterns, parameter binding, ES6 modules

## Key Principles

1. **Always delegate stages 1-3** - Use Task tool to invoke subagents
2. **Pass contracts and stage-specific patterns** - Subagents read stage-N-patterns.md themselves
3. **Run pre-stage scan first** - Check for anti-patterns BEFORE implementation starts
4. **Check preconditions** - Validate contracts exist before dispatching
5. **Run validation after each stage** - validation-agent runs automatically with enhanced runtime validation
6. **Block on validation failures** - If validation fails with continue_to_next_stage=false, workflow stops

## Integration

The dispatcher is called from the main workflow loop:
```javascript
const result = await dispatchStage(pluginName, currentStage)
```

After dispatch completes, the orchestrator runs the checkpoint protocol (see state-management.md).
