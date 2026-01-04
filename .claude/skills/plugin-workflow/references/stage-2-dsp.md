# Stage 2: DSP

**Context:** This file is part of the plugin-workflow skill.
**Invoked by:** Main workflow dispatcher after Stage 1 completion
**Purpose:** Implement audio processing where parameters control DSP

---

**Goal:** Implement audio processing, parameters control DSP

**Preconditions:**

- Stage 1 complete (parameters implemented)
- architecture.md exists (DSP component specifications)
- plan.md exists (complexity score, phase breakdown)

**Actions:**

### 1. Read Complexity Score from plan.md

Determine if phased implementation is required:

```typescript
const planContent = readFile(`plugins/${pluginName}/.ideas/plan.md`);

// Extract complexity score
const complexityMatch = planContent.match(/Complexity Score:\*\*\s*([\d.]+)/);
const complexityScore = complexityMatch ? parseFloat(complexityMatch[1]) : 0;

// Check if plan specifies phases
const hasPhases =
  planContent.includes("### Phase 3.1") ||
  planContent.includes("## Stage 2: DSP Phases");

console.log(
  `Complexity: ${complexityScore} (${hasPhases ? "phased" : "single-pass"})`
);
```

### 2a. Single-Pass Implementation (Complexity ≤2)

**If complexity ≤2 OR no phases defined:**

Invoke dsp-agent once for complete DSP implementation with minimal prompt:

```typescript
const dspResult = Task({
  subagent_type: "dsp-agent",
  description: `Stage 2 - ${pluginName}`,
  prompt: `
You are dsp-agent implementing Stage 2 for ${pluginName}.

**Plugin:** ${pluginName}
**Stage:** 3 (DSP Implementation)
**Complexity:** ${complexityScore} (single-pass)
**Your task:** Implement ALL DSP components from architecture.md

**Contracts (read these files yourself):**
- creative-brief.md: plugins/${pluginName}/.ideas/creative-brief.md
- architecture.md: plugins/${pluginName}/.ideas/architecture.md
- plan.md: plugins/${pluginName}/.ideas/plan.md
- parameter-spec.md: plugins/${pluginName}/.ideas/parameter-spec.md
- Required Reading: troubleshooting/patterns/juce8-critical-patterns.md

**CRITICAL: Read Required Reading BEFORE implementation.**

**Implementation steps:**
1. Read all contract files listed above
2. Read Required Reading (MANDATORY)
3. Add DSP member variables to PluginProcessor.h
4. Implement prepareToPlay() - initialize DSP at sample rate
5. Implement processBlock() with all DSP components
6. Connect parameters to DSP controls
7. Return JSON report with components list and real-time safety status

Build verification handled by orchestrator after you complete.
  `
});

// Parse and validate report (same as Stage 1/3)
const report = parseSubagentReport(dspResult);

if (report.status === "success") {
  console.log(`✓ Audio Engine Working`);
  console.log(`   Your DSP processing is implemented and functional`);
  console.log(`  `);
  console.log(
    `  Components: ${report.outputs.dsp_components_implemented.length}`
  );
  console.log(
    `  Parameters connected: ${report.outputs.parameters_connected.length}`
  );
  console.log(
    `  Real-time safe: ${report.outputs.real_time_safe ? "Yes" : "No"}`
  );

  // Invoke validation-agent for semantic review
  console.log("\nInvoking validation-agent for semantic review...");
  const validationResult = invokeValidationAgent(pluginName, 3);
  const validation = parseValidationReport(validationResult);

  if (validation.status === "FAIL" && !validation.continue_to_next_stage) {
    presentValidationFailureMenu(validation);
    return; // Block until user resolves
  }

  console.log(`✓ Validation ${validation.status}: ${validation.recommendation}`);

  // Continue to auto-test (Step 5)
}
```

### 2b. Phased Implementation (Complexity ≥3)

**If complexity ≥3 AND plan.md defines phases:**

Parse phase breakdown from plan.md:

```typescript
// Extract phases from plan.md
const phasePattern = /### Phase (4\.\d+):\s*(.+?)\n/g;
const phases = [];
let match;

while ((match = phasePattern.exec(planContent)) !== null) {
  phases.push({
    number: match[1], // e.g., "4.1"
    description: match[2], // e.g., "Core Processing"
  });
}

console.log(`Stage 2 will execute in ${phases.length} phases:`);
phases.forEach((phase) => {
  console.log(`  - Phase ${phase.number}: ${phase.description}`);
});
```

**Execute each phase sequentially (with minimal prompts):**

```typescript
for (let i = 0; i < phases.length; i++) {
  const phase = phases[i];

  console.log(`\n━━━ Stage ${phase.number} - ${phase.description} ━━━\n`);

  // Determine model based on complexity
  const model = complexityScore >= 4 ? "opus" : "sonnet";

  const phaseResult = Task({
    subagent_type: "dsp-agent",
    description: `Stage 2.${i+1} - ${pluginName}`,
    prompt: `
You are dsp-agent implementing Phase ${phase.number} for ${pluginName}.

**Plugin:** ${pluginName}
**Stage:** 3 (DSP Implementation)
**Phase:** ${phase.number} - ${phase.description}
**Complexity:** ${complexityScore} (phased implementation)
**Your task:** Implement Phase ${phase.number} components only

**Contracts (read these files yourself):**
- creative-brief.md: plugins/${pluginName}/.ideas/creative-brief.md
- architecture.md: plugins/${pluginName}/.ideas/architecture.md
- plan.md: plugins/${pluginName}/.ideas/plan.md
- parameter-spec.md: plugins/${pluginName}/.ideas/parameter-spec.md
- Required Reading: troubleshooting/patterns/juce8-critical-patterns.md

**CRITICAL: Read Required Reading BEFORE implementation.**

**Phase implementation steps:**
1. Read all contract files listed above
2. Read Required Reading (MANDATORY)
3. Extract Phase ${phase.number} components from plan.md
4. Add member variables for this phase's DSP components
5. Implement this phase's components in processBlock()
6. Build on existing code from previous phases (preserve all)
7. Connect this phase's parameters only
8. Return JSON report with phase_completed: "${phase.number}"

**CRITICAL:** Implement ONLY Phase ${phase.number} components, preserve all previous phase code.

Build verification handled by orchestrator after you complete.
    `
  });

  const phaseReport = parseSubagentReport(phaseResult);

  if (!phaseReport || phaseReport.status === "failure") {
    console.log(`
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
✗ Stage ${phase.number} Failed
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Phase ${phase.number} (${phase.description}) failed.

Error: ${phaseReport?.outputs.error_message || "Unknown error"}

What would you like to do?
1. Investigate
2. Show me the code
3. Show me the build output
4. I'll fix it manually (say "resume automation" when ready)

Choose (1-4): _
    `);

    // Handle failure (same 4-option menu as Stage 1/3)
    return; // Stop phased execution on failure
  }

  // Phase succeeded
  console.log(`✓ Phase ${phase.number} complete: ${phase.description}`);
  console.log(
    `  - Components: ${
      phaseReport.outputs.components_this_phase?.join(", ") || "N/A"
    }`
  );

  // Git commit for this phase
  await bash(`
git add plugins/${pluginName}/Source/
git add plugins/${pluginName}/.ideas/plan.md
git add plugins/${pluginName}/.continue-here.md

git commit -m "$(cat <<'EOF'
feat: ${pluginName} Stage ${phase.number} - ${phase.description}

Phase ${i + 1} of ${phases.length} complete
Components implemented this phase

🤖 Generated with Claude Code

Co-Authored-By: Claude <noreply@anthropic.com>
EOF
)"
  `);

  console.log(`✓ Committed: ${await bash('git log -1 --format="%h"')}`);

  // Update plan.md with phase timestamp
  const timestamp = new Date().toISOString();
  updatePlanMd(pluginName, phase.number, timestamp);

  // Update handoff
  updateHandoff(
    pluginName,
    phase.number, // e.g., "4.1"
    `Phase ${phase.number} complete: ${phase.description}`,
    i < phases.length - 1
      ? [
          `Phase ${phases[i + 1].number}: ${phases[i + 1].description}`,
          "Review current phase",
          "Test",
          "Pause",
        ]
      : ["Auto-test Stage 2 output", "Review complete DSP", "Pause"],
    complexityScore,
    true // phased
  );

  // Decision menu after each phase
  if (i < phases.length - 1) {
    // Not last phase
    console.log(`
✓ Phase ${phase.number} complete

Progress: ${i + 1} of ${phases.length} phases complete

What's next?
1. Continue to Phase ${phases[i + 1].number} (${
      phases[i + 1].description
    }) (recommended)
2. Review Phase ${phase.number} code
3. Test current state in DAW
4. Pause here
5. Other

Choose (1-5): _
    `);

    const choice = getUserInput();
    if (choice === "4" || choice === "pause") {
      console.log(
        "\n⏸ Paused between phases. Resume with /continue to continue Phase ${phases[i+1].number}."
      );
      return;
    }
    // Other choices: review, test, etc., then re-present menu
  }

  // Last phase - continue to auto-test
}

console.log(`\n✓ All ${phases.length} phases complete!`);
```

### 3. Handle Non-Phased Success

**After single-pass success:**

```typescript
// Update state
updateHandoff(
  pluginName,
  4,
  "Stage 2: DSP - Audio processing implemented",
  ["Auto-test Stage 2", "Review DSP code", "Test audio manually"],
  complexityScore,
  false
);

updatePluginStatus(pluginName, "🚧 Stage 2");
updatePluginTimeline(
  pluginName,
  4,
  `DSP complete - ${report.outputs.dsp_components_implemented.length} components`
);

// Git commit
bash(`
git add plugins/${pluginName}/Source/
git add plugins/${pluginName}/.continue-here.md
git add PLUGINS.md

git commit -m "$(cat <<'EOF'
feat: ${pluginName} Stage 2 - DSP

Audio processing implemented
${report.outputs.dsp_components_implemented.length} DSP components
All parameters connected to audio engine
Real-time safe implementation

🤖 Generated with Claude Code

Co-Authored-By: Claude <noreply@anthropic.com>
EOF
)"
`);
```

### 4. Handle Failure (Single-Pass or Any Phase)

**4-option failure menu:**

```typescript
if (report.status === "failure") {
  console.log(`
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
✗ Stage 2 Failed: DSP Implementation
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Error: ${report.outputs.error_message}
Type: ${report.outputs.error_type}

Issues:
${report.issues.map((i) => `  - ${i}`).join("\n")}

What would you like to do?
1. Investigate (invoke troubleshoot-agent)
2. Show me the code (PluginProcessor.cpp)
3. Show me the build output
4. I'll fix it manually (say "resume automation" when ready)

Choose (1-4): _
  `);

  // Handle choice (same as Stage 1/3)
}
```

### 5. Auto-Invoke plugin-testing Skill

**After Stage 2 succeeds (all phases if phased):**

```typescript
console.log("\n━━━ Running automated tests ━━━\n");

const testResult = Task({
  subagent_type: "general-purpose", // Or invoke plugin-testing skill directly
  description: `Test ${pluginName} after Stage 2`,
  prompt: `
Run automated tests for ${pluginName} after Stage 2 DSP implementation.

Use the plugin-testing skill to run the 5 automated tests:
1. Build test (already passed)
2. Load test (plugin loads without crash)
3. Process test (audio processing works)
4. Parameter test (parameters affect audio)
5. State test (save/load works)

Report results.
  `,
});

console.log(testResult);

// Parse test results
const testsPassed =
  testResult.includes("✓ All tests passed") || testResult.includes("PASS");

if (!testsPassed) {
  console.log(`
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
✗ Stage 2 Tests FAILED
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Audio processing implementation completed, but automated tests failed.

Test results:
${testResult}

Cannot integrate UI until audio engine is stable.

What would you like to do?
1. Investigate test failures - Find root cause
2. Show me the DSP code - Review implementation
3. Show me the test output - See detailed errors
4. I'll fix it manually (say "resume automation" when ready)

Choose (1-4): _
  `);

  // STOP - Do not proceed to Stage 2 with failing tests
  return;
}

console.log("✓ All Stage 2 tests passed");
```

### 6. Invoke validation-agent for Complexity ≥4 Plugins

**For complexity ≥4 plugins, run semantic validation:**

````typescript
if (complexityScore >= 4) {
  console.log("\n━━━ Running semantic validation (complexity ≥4) ━━━\n");

  // Read contracts
  const paramSpecContent = readFile(
    `plugins/${pluginName}/.ideas/parameter-spec.md`
  );
  const architectureContent = readFile(
    `plugins/${pluginName}/.ideas/architecture.md`
  );
  const planContent = readFile(`plugins/${pluginName}/.ideas/plan.md`);

  const validationResult = Task({
    subagent_type: "validation-agent",
    description: `Validate ${pluginName} Stage 2`,
    prompt: `
Validate Stage 2 completion for ${pluginName}.

**Stage:** 4
**Plugin:** ${pluginName}
**Contracts:**
- parameter-spec.md: ${paramSpecContent}
- architecture.md: ${architectureContent}
- plan.md: ${planContent}

**Expected outputs for Stage 2:**
- All DSP components from architecture.md implemented
- processBlock() contains real-time safe audio processing
- Parameters modulate DSP components correctly
- prepareToPlay() allocates buffers
- No heap allocations in processBlock
- ScopedNoDenormals used
- Edge cases handled (zero-length buffers)

Return JSON validation report with status, checks, and recommendation.
    `,
  });

  // Parse JSON report (robust handling)
  let validationReport;
  try {
    // Try extracting JSON from markdown code blocks first
    const jsonMatch =
      validationResult.match(/```json\n([\s\S]*?)\n```/) ||
      validationResult.match(/```\n([\s\S]*?)\n```/);

    if (jsonMatch) {
      validationReport = JSON.parse(jsonMatch[1]);
    } else {
      // Try parsing entire response as JSON
      validationReport = JSON.parse(validationResult);
    }
  } catch (error) {
    console.log(`
⚠️ Warning: Could not parse validation-agent report as JSON

Raw validation-agent output:
${validationResult}

Continuing workflow (validation is advisory, not blocking).
    `);
    validationReport = null;
  }

  // Present findings if report parsed successfully
  if (validationReport) {
    const { status, checks, recommendation, continue_to_next_stage } =
      validationReport;

    console.log(`
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
${status === "PASS" ? "✓" : "✗"} Validator ${status}: Stage 2 Review
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    `);

    // Group checks by severity
    const errors = checks.filter((c) => c.severity === "error");
    const warnings = checks.filter((c) => c.severity === "warning");
    const info = checks.filter((c) => c.severity === "info");

    if (errors.length > 0) {
      console.log("\n❌ Errors:");
      errors.forEach((e) => console.log(`  - ${e.message}`));
    }

    if (warnings.length > 0) {
      console.log("\n⚠️  Warnings:");
      warnings.forEach((w) => console.log(`  - ${w.message}`));
    }

    if (info.length > 0 && info.length <= 5) {
      console.log("\nℹ️  Info:");
      info.forEach((i) => console.log(`  - ${i.message}`));
    }

    console.log(`\nRecommendation: ${recommendation}`);

    // Present decision menu with validation findings
    console.log(`

What's next?
1. Integrate UI - Connect your interface to the audio engine ${
      continue_to_next_stage ? "(recommended by validation-agent)" : ""
    }
2. Address validation-agent ${errors.length > 0 ? "errors" : "warnings"} first
3. Review validation-agent report details
4. Test audio manually in DAW - Listen to your processing
5. Review DSP code
6. Other

Choose (1-6): _
    `);

    // User decides next action based on findings
    // (Advisory layer - user makes final call)
  }
}
````

### 7. Decision Menu (After Tests Pass) - GUI Decision Gate

**CRITICAL: This is the GUI-Optional Flow decision gate. User chooses custom UI OR headless.**

```
✓ Audio Engine Working
   Your DSP processing is implemented and functional

Plugin: [PluginName]
DSP components: [N]
Parameters connected: [N]
Tests: All passed
Status: Ready for UI integration

What type of interface do you want?

1. Add custom UI - Create WebView interface with mockup
2. Ship headless - Use DAW controls only (fast path to v1.0.0)
3. Test audio in DAW - Listen to your processing
4. Review DSP code - See implementation details
5. Pause workflow - Resume anytime
6. Other

Choose (1-6): _
```

**Routing logic:**

```typescript
const choice = getUserInput();

if (choice === "1" || choice === "add_gui" || choice === "custom ui") {
  // Option 1: Custom WebView UI path (existing Stage 2 behavior)
  handleGuiPath(pluginName, complexityScore);
} else if (choice === "2" || choice === "headless" || choice === "ship headless") {
  // Option 2: Headless path (NEW)
  handleHeadlessPath(pluginName, complexityScore);
} else if (choice === "3") {
  // Test in DAW
  console.log("\nInstructions for DAW testing:\n1. Build plugin\n2. Load in DAW\n3. Test parameters\n\nReturn here when ready.");
  // Re-present decision menu
} else if (choice === "4") {
  // Review code
  const processorCode = readFile(`plugins/${pluginName}/Source/PluginProcessor.cpp`);
  console.log(processorCode);
  // Re-present decision menu
} else if (choice === "5" || choice === "pause") {
  // Pause workflow
  updateHandoff(pluginName, 3, "Stage 2: DSP complete, paused at GUI decision gate",
    ["Add custom UI", "Ship headless", "Test in DAW"], complexityScore, false);
  console.log("\n⏸ Paused at GUI decision gate. Resume with /continue");
  return;
}
```

**Helper function: handleGuiPath()** (existing Stage 2 logic):

```typescript
function handleGuiPath(pluginName: string, complexityScore: number) {
  // Check for finalized mockup
  const mockupPath = findLatestMockup(pluginName);

  if (!mockupPath) {
    console.log(`
No finalized UI mockup found for ${pluginName}.

Would you like to:
1. Create mockup now - Run ui-mockup skill
2. Use existing mockup - I'll tell you which version
3. Skip GUI for now - Return to decision menu
4. Other

Choose (1-4): _
    `);

    const mockupChoice = getUserInput();

    if (mockupChoice === "1") {
      // Invoke ui-mockup skill
      Skill({ skill: "ui-mockup" });

      // After mockup finalized, read path and continue
      const finalizedMockupPath = findLatestMockup(pluginName);

      if (!finalizedMockupPath) {
        console.log("✗ Mockup creation failed or was cancelled. Returning to decision menu.");
        // Re-present GUI decision gate
        return;
      }

      mockupPath = finalizedMockupPath;
    } else if (mockupChoice === "3") {
      // Return to main decision gate
      // Re-present GUI decision gate from section 7
      return;
    }
  } else {
    console.log(`✓ Found existing mockup: ${mockupPath}`);
  }

  // Mockup exists - proceed to Stage 2 (GUI) as normal
  console.log("\n━━━ Integrating WebView UI ━━━\n");

  // Continue to Stage 2 GUI implementation (existing behavior)
  // This will be handled by main workflow orchestration
  currentStage = 4;
}
```

**Helper function: handleHeadlessPath()** (NEW):

```typescript
function handleHeadlessPath(pluginName: string, complexityScore: number) {
  console.log(`
━━━ Generating minimal editor for ${pluginName} ━━━

Creating headless plugin interface:
- Minimal PluginEditor.h/cpp (simple window with plugin name)
- DAW will provide parameter controls automatically
- Plugin will be fully functional without custom UI
- Can add custom UI later via /improve ${pluginName}
  `);

  // Check if mockup exists (user might have created one during ideation)
  const mockupPath = findLatestMockup(pluginName);
  if (mockupPath) {
    console.log(`
Note: Existing UI mockup found (${mockupPath}) but not used.
Mockup preserved for future use (add UI via /improve later).
    `);
  }

  // Generate minimal editor from template
  generateMinimalEditor(pluginName);

  // Update state files
  updateHandoff(
    pluginName,
    4,
    "Stage 2: GUI - Minimal editor (headless, DAW controls only)",
    ["Run validation tests", "Install plugin", "Add custom UI later"],
    complexityScore,
    false,
    "headless" // NEW: gui_type field
  );

  updatePluginStatus(pluginName, "🚧 Stage 2");
  updatePluginTimeline(
    pluginName,
    4,
    "GUI complete - Minimal editor (headless, uses DAW controls)"
  );

  // Git commit
  bash(`
git add plugins/${pluginName}/Source/PluginEditor.h
git add plugins/${pluginName}/Source/PluginEditor.cpp
git add plugins/${pluginName}/.continue-here.md
git add PLUGINS.md

git commit -m "$(cat <<'EOF'
feat(${pluginName}): Stage 2 - Minimal Editor (Headless)

Generated minimal PluginEditor for headless deployment
Plugin uses DAW-provided parameter controls
Can add custom WebView UI later via /improve command

🤖 Generated with Claude Code

Co-Authored-By: Claude <noreply@anthropic.com>
EOF
)"
  `);

  console.log(`✓ Committed: ${bash('git log -1 --format="%h"')}`);

  // Present decision menu
  console.log(`
✓ Minimal editor created

Files generated:
- Source/PluginEditor.h
- Source/PluginEditor.cpp

Plugin is now headless (no custom UI):
- DAW will provide sliders/knobs automatically
- All parameters are exposed for automation
- Can add custom UI later via /improve ${pluginName}

What's next?

1. Complete validation - Run tests and finalize v1.0.0 (recommended)
2. Test in DAW - See parameter controls in action
3. Add custom UI now - Create WebView interface after all
4. Review code - See minimal editor implementation
5. Pause workflow
6. Other

Choose (1-6): _
  `);

  const nextChoice = getUserInput();

  if (nextChoice === "1") {
    // Proceed to Stage 3 (validation)
    currentStage = 5;
    // Return to main workflow orchestration
  } else if (nextChoice === "3") {
    // User changed mind - invoke GUI path
    handleGuiPath(pluginName, complexityScore);
  } else if (nextChoice === "2") {
    // Test in DAW
    console.log("\nInstructions for DAW testing:\n1. Build plugin\n2. Load in DAW\n3. Test parameters with generic UI\n\nReturn here when ready.");
    // Re-present decision menu
  } else if (nextChoice === "5" || nextChoice === "pause") {
    console.log("\n⏸ Paused after headless editor generation. Resume with /continue");
    return;
  }
  // ... handle other choices
}
```

**Helper function: generateMinimalEditor()** (NEW):

```typescript
function generateMinimalEditor(pluginName: string) {
  const templatePath = "plugins/TEMPLATE-HEADLESS-EDITOR";
  const targetPath = `plugins/${pluginName}/Source`;

  // Read template files
  const headerTemplate = readFile(`${templatePath}/PluginEditor.h`);
  const cppTemplate = readFile(`${templatePath}/PluginEditor.cpp`);

  // Replace [PluginName] placeholders
  const header = headerTemplate.replaceAll("[PluginName]", pluginName);
  const cpp = cppTemplate.replaceAll("[PluginName]", pluginName);

  // Write files
  writeFile(`${targetPath}/PluginEditor.h`, header);
  writeFile(`${targetPath}/PluginEditor.cpp`, cpp);

  console.log(`✓ Generated minimal editor for ${pluginName}`);
}
```

**Helper function: findLatestMockup():**

```typescript
function findLatestMockup(pluginName: string): string | null {
  const mockupsDir = `plugins/${pluginName}/.ideas/mockups`;

  if (!fileExists(mockupsDir)) {
    return null;
  }

  // Look for finalized mockup (highest version number with -ui.html suffix)
  const mockupFiles = listFiles(mockupsDir, "*.html");

  // Filter for UI mockup files (v1-ui.html, v2-ui.html, etc.)
  const uiMockups = mockupFiles.filter(f => f.match(/v\d+-ui\.html$/));

  if (uiMockups.length === 0) {
    return null;
  }

  // Sort by version number and return latest
  const sorted = uiMockups.sort((a, b) => {
    const versionA = parseInt(a.match(/v(\d+)/)[1]);
    const versionB = parseInt(b.match(/v(\d+)/)[1]);
    return versionB - versionA; // Descending order
  });

  return `${mockupsDir}/${sorted[0]}`;
}
```

**CRITICAL: Do NOT proceed to Stage 3 if tests fail.**

---

**Return to:** Main plugin-workflow orchestration in `SKILL.md`
