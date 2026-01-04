# Failure Protocol - Option Details

## Option 1: Investigate (Recommended)

Invoke troubleshooter agent via Task tool:

```
Task: Deep investigation of build failure for [PluginName]

Context:
- Build log: logs/[PluginName]/build_TIMESTAMP.log
- Error summary: [extracted error from log]
- Build flags used: [--no-install | none]
- Invoking stage: [Stage 0, 2-5 | plugin-improve | manual]

Instructions:
1. Analyze build log for root cause
2. Search codebase for similar issues
3. Check troubleshooting knowledge base
4. Provide fix recommendations with exact file/line changes

Return findings as structured report with:
- Root cause analysis
- Recommended fixes (ranked by confidence)
- Example code snippets
```

After troubleshooter returns:
- Display findings to user
- Ask if they want to apply suggested fix or choose another option
- Re-present failure menu

## Option 2: Show me the build log

Display full build output:

```
=== Build Log: logs/[PluginName]/build_TIMESTAMP.log ===

[full log contents]

=== End of Log ===

The error appears to be:
[highlighted error section]
```

Re-present failure menu after display.

## Option 3: Show me the code

Extract file/line references from error, display relevant code:

```
Error in: Source/PluginProcessor.cpp:45

=== Source/PluginProcessor.cpp (lines 40-50) ===
40: void PluginProcessor::processBlock(AudioBuffer<float>& buffer,
41:                                      MidiBuffer& midiMessages)
42: {
43:     ScopedNoDenormals noDenormals;
44:     auto totalNumInputChannels  = getTotalNumInputChannels();
45:     auto totalNumOutputChannels = getTotalNumOutputChannels();  // ERROR HERE
46:
47:     for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
48:         buffer.clear (i, 0, buffer.getNumSamples());
49: }
=== End of Code ===
```

Re-present failure menu after display.

## Option 4: Wait - I'll fix manually

```
Workflow paused. When ready:
- Make your changes to the source files
- Say "retry build" to rebuild with same flags
- Or say "continue [PluginName]" to resume workflow
```

Exit skill. Do NOT re-present menu. User will resume when ready.

## Option 5: Other

Capture user's custom action:
- If user says "retry": Re-invoke build with same flags
- If user says "skip": Continue to next stage (plugin-workflow only)
- If user says "abort": Exit workflow completely
- Otherwise: Execute user's specific instruction
