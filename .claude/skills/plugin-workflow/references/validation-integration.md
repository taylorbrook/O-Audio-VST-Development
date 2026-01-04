# Validation Agent Integration

Helper functions for validation-agent integration.

## invokeValidationAgent

```typescript
function invokeValidationAgent(pluginName: string, stage: number): string {
  const expectations = getStageExpectations(stage);

  const prompt = `
Validate Stage ${stage} completion for ${pluginName}.

**Stage:** ${stage}
**Plugin:** ${pluginName}

**Contracts (read these files yourself):**
- creative-brief.md: plugins/${pluginName}/.ideas/creative-brief.md
- architecture.md: plugins/${pluginName}/.ideas/architecture.md
- plan.md: plugins/${pluginName}/.ideas/plan.md
- parameter-spec.md: plugins/${pluginName}/.ideas/parameter-spec.md
- Required Reading: troubleshooting/patterns/juce8-critical-patterns.md

**Expected outputs for Stage ${stage}:**
${expectations}

Return JSON validation report with status, checks, and recommendation.
Max 500 tokens.
  `;

  return Task({
    subagent_type: "validation-agent",
    description: `Validate Stage ${stage} - ${pluginName}`,
    prompt: prompt
  });
}
```

## getStageExpectations

```typescript
function getStageExpectations(stage: number): string {
  const expectations = {
    2: `- CMakeLists.txt with JUCE 8 patterns
- PluginProcessor.{h,cpp} with APVTS
- All parameters from parameter-spec.md implemented
- Parameter IDs match spec exactly (zero-drift)
- JUCE 8 ParameterID format used
- juce_generate_juce_header() called correctly`,

    3: `- All DSP components from architecture.md implemented
- processBlock() contains real-time safe processing
- Parameters modulate DSP components correctly
- prepareToPlay() allocates buffers
- No heap allocations in processBlock
- ScopedNoDenormals used
- Edge cases handled`,

    4: `- Member declaration order correct (Relays → WebView → Attachments)
- All parameters from spec have UI bindings
- HTML element IDs match relay names
- UI aesthetic matches mockup
- Visual feedback works (knobs respond)
- WebView initialization includes error handling`
  };

  return expectations[stage] || "No expectations defined";
}
```

## parseValidationReport

```typescript
function parseValidationReport(rawOutput: string): object {
  try {
    // Extract JSON from markdown code blocks
    const jsonMatch = rawOutput.match(/```json\n([\s\S]*?)\n```/) ||
                      rawOutput.match(/```\n([\s\S]*?)\n```/);

    if (jsonMatch) {
      return JSON.parse(jsonMatch[1]);
    }

    // Try parsing entire output
    return JSON.parse(rawOutput);
  } catch (error) {
    console.warn("Could not parse validation report, treating as PASS");
    return {
      agent: "validation-agent",
      status: "WARNING",
      checks: [],
      recommendation: "Could not parse validation output (validation is advisory)",
      continue_to_next_stage: true
    };
  }
}
```

## presentValidationFailureMenu

```typescript
function presentValidationFailureMenu(validation: object) {
  const errors = validation.checks.filter(c => c.severity === "error");
  const warnings = validation.checks.filter(c => c.severity === "warning");

  console.log(`
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
✗ Validation ${validation.status}: Stage ${validation.stage}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Semantic validation found issues that should be addressed:
  `);

  if (errors.length > 0) {
    console.log("\n❌ Errors:");
    errors.forEach(e => console.log(`  - ${e.message}`));
  }

  if (warnings.length > 0) {
    console.log("\n⚠️  Warnings:");
    warnings.forEach(w => console.log(`  - ${w.message}`));
  }

  console.log(`\nRecommendation: ${validation.recommendation}`);
  console.log(`
What would you like to do?

1. Address issues - Fix validation errors before continuing
2. Continue anyway - Validation is advisory, proceed at your own risk
3. Show details - See full validation report
4. Pause workflow - I'll fix manually
5. Other

Choose (1-5): _
  `);

  // Wait for user input and handle accordingly
}
```
