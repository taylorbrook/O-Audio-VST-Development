# Failure Investigation Guide

## Overview

When tests fail, systematic investigation is required to identify root causes and apply fixes.

## Investigation Procedure

### Step 1: Read Troubleshooting Documentation

Check `references/troubleshooting.md` for known issues matching the failure:

1. Search by test name (e.g., "ParameterChangeTest")
2. Search by error message (e.g., "assertion failed")
3. Search by symptom (e.g., "crash on parameter change")

If documented solution exists:
- Present fix recommendation to user
- Offer to apply fix or show relevant code
- Link to troubleshooting doc for context

### Step 2: Code Inspection

If issue not documented, offer to inspect relevant code:

Common locations by failure type:
- **Parameter failures:** `PluginProcessor.cpp` → `getStateInformation`, `setStateInformation`, APVTS setup
- **DSP failures:** `PluginProcessor.cpp` → `processBlock`, `prepareToPlay`
- **GUI failures:** `PluginEditor.cpp` → constructor, `paint`, `resized`
- **Initialization failures:** `PluginProcessor.cpp` → constructor, destructor

Present code snippets to user with identified issues.

### Step 3: Delegate to Deep Research (Non-Trivial Issues)

If issue is complex or root cause unclear, delegate to `deep-research` skill.

**When to delegate:**
- Error message not in troubleshooting docs
- Multiple interconnected failures
- Requires JUCE API expertise
- Needs cross-file analysis

**Handoff protocol:**
```
Task: "Investigate [test_name] failure in [PluginName]

Test output:
[exact error message or failure log]

Plugin info:
- Type: [from creative-brief.md - e.g., delay effect, reverb, etc.]
- DSP features: [list from parameter-spec.md]
- Parameters: [count and types]

Relevant files:
- plugins/{PluginName}/Source/PluginProcessor.{h,cpp}
- plugins/{PluginName}/Source/PluginEditor.{h,cpp}
[add other relevant files]

Goal: Find root cause of test failure and provide actionable fix with code changes"
```

**IMPORTANT:** Wait for deep-research to complete before presenting results to user. Do NOT attempt to fix complex issues without delegation.

### Step 4: Present Findings

After investigation (manual or delegated), present:

1. **Root cause analysis:**
   - What is failing
   - Why it's failing
   - Where in the code

2. **Fix recommendations:**
   - Specific code changes required
   - Files to modify
   - Testing approach to verify fix

3. **Decision menu:**
   - Apply fix automatically (if straightforward)
   - Show code and let user fix manually
   - Document issue for later
   - Skip and continue (not recommended)

## Common Failure Patterns

### Pattern: Assertion Failures

**Symptom:** Test fails with "assertion failed" message

**Investigation:**
1. Check which assertion failed (line number in test file)
2. Inspect preconditions: what state is required?
3. Check if plugin meets preconditions

**Common causes:**
- APVTS not initialized properly
- Parameters out of range
- State information incomplete

### Pattern: Segmentation Faults

**Symptom:** Test crashes with segfault

**Investigation:**
1. Identify crash location (use debugger or stack trace)
2. Check for null pointer dereferences
3. Check for out-of-bounds array access

**Common causes:**
- Uninitialized pointers
- Deleted objects still referenced
- Buffer size mismatches

### Pattern: Timeout Failures

**Symptom:** Test exceeds time limit

**Investigation:**
1. Check if plugin is blocking (mutex deadlock)
2. Check if processing is too slow
3. Check for infinite loops

**Common causes:**
- Thread synchronization issues
- Inefficient DSP algorithms
- Resource contention

## Example Investigation

**Scenario:** Pluginval fails with "getStateInformation returned empty data"

**Step 1 - Check troubleshooting.md:**
Found entry: "Empty state data usually means APVTS not saved"

**Step 2 - Inspect code:**
```cpp
void PluginProcessor::getStateInformation(MemoryBlock& destData)
{
    // BUG: Not saving APVTS state!
}
```

**Step 3 - Fix recommendation:**
```cpp
void PluginProcessor::getStateInformation(MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}
```

**Step 4 - Present to user:**
"Found issue: `getStateInformation` is empty, not saving APVTS state. This is required for pluginval to pass. Would you like me to apply the fix?"
