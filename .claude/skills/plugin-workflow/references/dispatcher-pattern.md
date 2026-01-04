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

  // ALWAYS invoke subagents via Task tool for stages 1-3
  switch(stageNumber) {
    case 1:
      // Invoke foundation-shell-agent subagent
      return await invokeSubagent('foundation-shell-agent', {
        pluginName,
        contracts: loadContracts(pluginName),
        requiredReading: 'juce8-critical-patterns.md'
      })
    case 2:
      // Invoke dsp-agent subagent
      return await invokeSubagent('dsp-agent', {
        pluginName,
        contracts: loadContracts(pluginName),
        requiredReading: 'juce8-critical-patterns.md'
      })
    case 3:
      // Invoke gui-agent subagent
      return await invokeSubagent('gui-agent', {
        pluginName,
        contracts: loadContracts(pluginName),
        requiredReading: 'juce8-critical-patterns.md'
      })
    default:
      return { status: 'error', reason: `Invalid stage: ${stageNumber}. Valid stages: 1-3` }
  }
}
```

## Key Principles

1. **Always delegate stages 1-3** - Use Task tool to invoke subagents
2. **Pass contracts and Required Reading** - Every subagent receives architecture.md, plan.md, and juce8-critical-patterns.md
3. **Check preconditions first** - Validate contracts exist before dispatching
4. **Run validation after each stage** - validation-agent runs automatically with enhanced runtime validation
5. **Block on validation failures** - If validation fails with continue_to_next_stage=false, workflow stops

## Integration

The dispatcher is called from the main workflow loop:
```javascript
const result = await dispatchStage(pluginName, currentStage)
```

After dispatch completes, the orchestrator runs the checkpoint protocol (see state-management.md).
