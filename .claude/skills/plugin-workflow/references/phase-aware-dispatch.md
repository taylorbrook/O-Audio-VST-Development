# Phase-Aware Dispatch

Complex plugins (complexity ≥3) require phased implementation for Stages 2-3. This document describes the phase detection, execution loop, and prompt construction algorithms.

## Contents

- [When to Use Phase-Aware Dispatch](#when-to-use-phase-aware-dispatch)
- [Phase Detection Algorithm](#phase-detection-algorithm)
- [Routing Decision](#routing-decision)
- [Prompt Construction](#prompt-construction)
- [Error Prevention](#error-prevention)
- [Integration with Checkpoint Protocol](#integration-with-checkpoint-protocol)

## When to Use Phase-Aware Dispatch

Phase-aware dispatch is triggered when:
1. **Stage** is 2 (DSP) or 3 (GUI)
2. **Complexity score** (from plan.md YAML frontmatter) ≥3
3. **plan.md contains phase markers**: `### Phase 2.X` or `### Phase 3.X`

If all three conditions true: Loop through phases one at a time.
Otherwise: Invoke agent once for entire stage.

**Applies to:** Stages 2 and 3 only. Stage 1 remains single-pass.

## Phase Detection Algorithm

**Determine if phased implementation is needed:**

```typescript
// 1. Read plan.md to check for phases
const planContent = readFile(`plugins/${pluginName}/.ideas/plan.md`);

// 2. Extract complexity score
const complexityMatch = planContent.match(/\*\*Complexity Score:\*\*\s+([\d.]+)/);
const complexityScore = complexityMatch ? parseFloat(complexityMatch[1]) : 0;

// 3. Check for phase markers based on current stage
const stagePhasePattern = currentStage === 2
  ? /### Phase 2\.\d+/g
  : /### Phase 3\.\d+/g;

const hasPhases = stagePhasePattern.test(planContent);

// 4. Determine execution strategy
const needsPhasedImplementation = complexityScore >= 3 && hasPhases;

console.log(`Complexity: ${complexityScore}, Has phases: ${hasPhases}`);
console.log(`Execution mode: ${needsPhasedImplementation ? "PHASED" : "SINGLE-PASS"}`);
```

## Routing Decision

**Based on detection results, route to appropriate handler:**

### Single-Pass Condition

**IF complexity < 3 OR no phase markers found:**

1. Invoke subagent ONCE for entire stage
2. Use prompt template from reference file single-pass section:
   - Stage 2: `references/stage-2-dsp.md` lines 45-87 (single-pass implementation)
   - Stage 3: `references/stage-3-gui.md` lines 83-135 (single-pass implementation)
3. Checkpoint after stage completes (standard 6-step checkpoint)
4. Present decision menu (continue to next stage, pause, test, etc.)

### Phased Implementation Condition

**IF complexity ≥3 AND phase markers found:**

**Phase parsing:**
```typescript
// Extract all phases for current stage from plan.md
const phasePattern = currentStage === 2
  ? /### Phase (2\.\d+):\s*(.+?)$/gm
  : /### Phase (3\.\d+):\s*(.+?)$/gm;

const phases = [];
let match;
while ((match = phasePattern.exec(planContent)) !== null) {
  phases.push({
    number: match[1],        // e.g., "2.1" or "3.1"
    description: match[2]    // e.g., "Voice Architecture" or "Layout and Basic Controls"
  });
}

console.log(`Stage ${currentStage} will execute in ${phases.length} phases:`);
phases.forEach(phase => {
  console.log(`  - Phase ${phase.number}: ${phase.description}`);
});
```

**Phase execution loop:**
```typescript
for (let i = 0; i < phases.length; i++) {
  const phase = phases[i];

  console.log(`\n━━━ Stage ${phase.number} - ${phase.description} ━━━\n`);

  // Invoke subagent for THIS PHASE ONLY
  const phaseResult = Task({
    subagent_type: currentStage === 2 ? "dsp-agent" : "gui-agent",
    description: `Implement Phase ${phase.number} for ${pluginName}`,
    prompt: constructPhasePrompt(phase, pluginName, currentStage, phases.length)
  });

  // Parse subagent report
  const phaseReport = parseSubagentReport(phaseResult);

  // Validate phase completion
  if (!phaseReport || phaseReport.status === "failure") {
    console.log(`✗ Phase ${phase.number} (${phase.description}) failed`);
    presentPhaseFailureMenu(phase, phaseReport);
    return; // BLOCK progression - do not continue to next phase
  }

  // Phase succeeded
  console.log(`✓ Phase ${phase.number} complete: ${phase.description}`);
  console.log(`  - Components: ${phaseReport.outputs.components_this_phase?.join(", ") || "N/A"}`);

  // CHECKPOINT: Commit phase changes
  commitPhase(pluginName, phase, i + 1, phases.length);

  // CHECKPOINT: Update handoff file
  updateHandoff(
    pluginName,
    currentStage,
    phase.number,
    `Phase ${phase.number} complete: ${phase.description}`,
    i < phases.length - 1
      ? [`Continue to Phase ${phases[i + 1].number}`, "Review phase code", "Test", "Pause"]
      : [getNextMilestoneAction(currentStage), "Review completed work", "Test current state", "Pause workflow"]
  );

  // CHECKPOINT: Update plugin status
  updatePluginStatus(pluginName, `🚧 Stage ${phase.number}`);

  // CHECKPOINT: Update plan.md with phase completion timestamp
  const timestamp = new Date().toISOString();
  updatePlanMd(pluginName, phase.number, timestamp);

  // CHECKPOINT: Verify all steps completed
  verifyPhaseCheckpoint(pluginName, phase.number);

  // DECISION MENU: Present between phases (BLOCKING)
  if (i < phases.length - 1) {
    console.log(`
✓ Phase ${phase.number} complete

Progress: ${i + 1} of ${phases.length} phases complete

What's next?

1. Continue to Phase ${phases[i + 1].number} (recommended)
2. Review Phase ${phase.number} code
3. Test current implementation
4. Pause here
5. Other

Choose (1-5): _
    `);

    const choice = getUserInput(); // BLOCKING WAIT
    if (choice === "4" || choice.toLowerCase() === "pause") {
      console.log(`\n⏸ Paused between phases. Resume with /continue ${pluginName}`);
      return; // Exit workflow, state saved for resume
    }
    // Other choices handled by menu router
  }
}

console.log(`\n✓ All ${phases.length} phases complete for Stage ${currentStage}!`);
```

**After ALL phases complete:**
1. Commit final stage state (if not already committed)
2. Update handoff to next stage
3. Update plugin status to reflect stage completion
4. Present stage completion decision menu
5. WAIT for user response

## Prompt Construction

**For each phase invocation, construct phase-specific prompt:**

```typescript
function constructPhasePrompt(phase, pluginName, currentStage, totalPhases) {
  // Read Required Reading (MANDATORY for all subagents)
  const criticalPatterns = readFile("troubleshooting/patterns/juce8-critical-patterns.md");

  // Read contracts
  const architectureMd = readFile(`plugins/${pluginName}/.ideas/architecture.md`);
  const parameterSpecMd = readFile(`plugins/${pluginName}/.ideas/parameter-spec.md`);
  const planMd = readFile(`plugins/${pluginName}/.ideas/plan.md`);

  // Stage-specific additional contracts
  const creativeBriefMd = currentStage === 3
    ? readFile(`plugins/${pluginName}/.ideas/creative-brief.md`)
    : null;

  const mockupPath = currentStage === 3
    ? findLatestMockup(pluginName)
    : null;

  return `CRITICAL PATTERNS (MUST FOLLOW):

${criticalPatterns}

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Implement Phase ${phase.number} for plugin at plugins/${pluginName}.

**Current Phase:** ${phase.number} - ${phase.description}
**Total Phases:** ${totalPhases}
**Plugin Name:** ${pluginName}

**Contracts:**
- architecture.md:
${architectureMd}

- parameter-spec.md:
${parameterSpecMd}

- plan.md (Phase ${phase.number} section):
${planMd}

${currentStage === 3 ? `
- creative-brief.md:
${creativeBriefMd}

- UI Mockup: ${mockupPath}
` : ''}

**Tasks for Phase ${phase.number}:**
1. Read plan.md and extract Phase ${phase.number} components ONLY
2. Read architecture.md for component specifications
${currentStage === 2 ? `3. Add member variables for Phase ${phase.number} DSP components
4. Implement Phase ${phase.number} components in processBlock()
5. Build on existing code from previous phases (do NOT remove previous work)
6. Connect Phase ${phase.number} parameters only
7. Ensure real-time safety (no allocations, use juce::ScopedNoDenormals)` : `3. Implement UI elements for Phase ${phase.number}
4. Add parameter bindings for Phase ${phase.number} controls
5. Build on existing UI from previous phases (do NOT remove previous work)
6. Verify member order (Relays → WebView → Attachments)
7. Ensure all Phase ${phase.number} parameter IDs match HTML element IDs`}
8. Update plan.md with phase completion timestamp
9. Return JSON report with phase_completed: "${phase.number}"

**CRITICAL:** Implement ONLY Phase ${phase.number} components. Preserve all code from previous phases.

Build verification handled by workflow after agent completes.
`;
}
```

**Key differences from single-pass prompt:**
- Explicitly states current phase number and description
- Emphasizes "THIS PHASE ONLY" - not all phases
- Reminds to preserve previous phase code
- Includes phase completion timestamp requirement
- JSON report must include phase_completed field

## Error Prevention

### Anti-Pattern: "Implement ALL phases" (CRITICAL)

**The problem this solves:**

❌ **NEVER send** "Implement ALL phases" to subagent
- Causes compilation errors from attempting too much
- Led to DrumRoulette Stage 3 failure (3 phases → single invocation → build errors)
- Violates incremental implementation principle

✓ **ALWAYS invoke** subagent once per phase with phase-specific prompt
- One phase at a time, sequential execution
- Checkpoint after EACH phase
- User confirmation between phases
- Incremental testing and validation

### Enforcement Rules

**Phase-aware dispatch is MANDATORY for Stages 2-3 when:**
1. Complexity score ≥3 (from plan.md)
2. Phase markers exist in plan.md (### Phase 2.X or ### Phase 3.X)

**Phase-aware dispatch is SKIPPED for Stages 2-3 when:**
1. Complexity score <3 (simple plugin, single-pass sufficient)
2. No phase markers in plan.md (plan didn't define phases)

**Phase-aware dispatch DOES NOT APPLY to:**
- Stage 1 (Foundation) - always single-pass

**The orchestrator MUST:**
- Read plan.md to detect phases BEFORE invoking subagent
- Parse ALL phases for the stage
- Loop through phases sequentially
- Present decision menu after EACH phase
- WAIT for user confirmation before next phase

**The orchestrator MUST NOT:**
- Skip phase detection (this is mandatory control flow)
- Invoke subagent with multiple phases at once
- Auto-proceed between phases without user confirmation
- Reference stage-2-dsp.md or stage-3-gui.md reference files for control flow (those are documentation/templates only)

## Integration with Checkpoint Protocol

**Phase checkpoints are identical to stage checkpoints:**

After each phase completes:
1. ✓ Commit phase changes (git commit with phase number)
2. ✓ Update handoff (.continue-here.md with current phase)
3. ✓ Update plugin status (PLUGINS.md with phase emoji)
4. ✓ Update plan.md (phase completion timestamp)
5. ✓ Verify checkpoint succeeded (all files updated)
6. ✓ Present decision menu (BLOCKING)

This mirrors the standard checkpoint protocol but executes after EACH phase instead of EACH stage.

**Why this matters:**
- User can pause between phases (not just between stages)
- State is saved incrementally (phase-level granularity)
- Build failures isolated to single phase (easier to debug)
- /continue can resume mid-stage at specific phase
