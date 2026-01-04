# Stage 3: GUI

**Context:** This file is part of the plugin-workflow skill.
**Invoked by:** Main workflow dispatcher after Stage 2 completion
**Purpose:** Integrate WebView UI with parameter bindings

---

**Goal:** Integrate WebView UI with parameter bindings

**Preconditions:**

- Stage 2 complete and tests passed (DSP operational)
- Finalized UI mockup exists (v[N]-ui.html)
- parameter-spec.md exists

**Actions:**

### 1. Locate Finalized UI Mockup

**Scan for finalized mockup version:**

```bash
cd plugins/${PLUGIN_NAME}/.ideas/mockups/

# Find highest version number
LATEST_MOCKUP=$(ls -1 v*-ui.html 2>/dev/null | sort -V | tail -1)

if [ -z "$LATEST_MOCKUP" ]; then
  echo "
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
✗ BLOCKED: Cannot proceed to Stage 3
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Missing required contract: Finalized UI mockup

Stage 3 requires a finalized UI mockup (v[N]-ui.html) to integrate.

WHY BLOCKED:
Without a finalized UI mockup, gui-agent cannot know:
- UI layout and styling
- Parameter control types (sliders, knobs, toggles)
- Visual design and branding
- Dimensions and layout

HOW TO UNBLOCK:
1. Run /mockup to create UI mockup
2. Design UI through iterative refinement
3. Finalize a design version (marks it as v[N])
4. Then resume Stage 3 with /continue ${PLUGIN_NAME}

Current status: No finalized mockup found in .ideas/mockups/

Cannot proceed without this contract.
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  "
  exit 1
fi

echo "✓ Found finalized mockup: $LATEST_MOCKUP"
```

**If no mockup found:** STOP IMMEDIATELY, exit skill, wait for user to create mockup.

### 2. Read Complexity and Check for Phases

Same as Stage 2 - check if phased implementation needed:

```typescript
const planContent = readFile(`plugins/${pluginName}/.ideas/plan.md`);
const complexityScore = extractComplexityScore(planContent);
const hasPhases =
  planContent.includes("### Phase 4.1") ||
  planContent.includes("## Stage 2: GUI Phases");

console.log(
  `Complexity: ${complexityScore} (${hasPhases ? "phased" : "single-pass"})`
);
```

### 3a. Single-Pass Implementation (Complexity ≤2)

**If complexity ≤2 OR no phases defined:**

Invoke gui-agent once for complete UI integration with minimal prompt:

```typescript
const mockupPath = findLatestMockup(pluginName);

const guiResult = Task({
  subagent_type: "gui-agent",
  description: `Stage 3 - ${pluginName}`,
  prompt: `
You are gui-agent implementing Stage 3 for ${pluginName}.

**Plugin:** ${pluginName}
**Stage:** 4 (GUI Integration)
**Complexity:** ${complexityScore} (single-pass)
**Your task:** Integrate WebView UI with ALL parameter bindings

**UI Mockup:** ${mockupPath}

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
3. Read finalized UI mockup from ${mockupPath}
4. Create ui/public/ directory and copy mockup
5. Download JUCE frontend library to ui/public/juce-framework.js
6. Add relay members to PluginEditor.h (CRITICAL ORDER: Relays → WebView → Attachments)
7. Implement parameter bindings in PluginEditor.cpp
8. Add WebView initialization in constructor
9. Update CMakeLists.txt to enable JUCE_WEB_BROWSER=1
10. Return JSON report with binding count and member order verification

⚠️ CRITICAL: Member declaration order (Relays → WebView → Attachments) prevents 90% of release build crashes.

Build verification handled by orchestrator after you complete.
  `
});

// Parse and validate report
const report = parseSubagentReport(guiResult);

if (report.status === "success") {
  console.log(`✓ UI Integrated`);
  console.log(`   Your interface is connected and responding to parameter changes`);
  console.log(`  `);
  console.log(`  Mockup version: ${report.outputs.ui_mockup_version}`);
  console.log(`  Bindings: ${report.outputs.binding_count}`);
  console.log(
    `  All parameters bound: ${
      report.outputs.all_parameters_bound ? "Yes" : "No"
    }`
  );
  console.log(
    `  Member order correct: ${
      report.outputs.member_order_correct ? "Yes" : "No"
    }`
  );

  // Invoke validation-agent for semantic review
  console.log("\nInvoking validation-agent for semantic review...");
  const validationResult = invokeValidationAgent(pluginName, 4);
  const validation = parseValidationReport(validationResult);

  if (validation.status === "FAIL" && !validation.continue_to_next_stage) {
    presentValidationFailureMenu(validation);
    return; // Block until user resolves
  }

  console.log(`✓ Validation ${validation.status}: ${validation.recommendation}`);

  // Continue to auto-test (Step 5)
}
```

### 3b. Phased Implementation (Complexity ≥3)

**If complexity ≥3 AND plan.md defines UI phases:**

Parse phase breakdown (similar to Stage 2):

```typescript
// Extract Phase 4.1, 5.2, etc. from plan.md
const phasePattern = /### Phase (5\.\d+):\s*(.+?)\n/g;
const phases = [];
let match;

while ((match = phasePattern.exec(planContent)) !== null) {
  phases.push({
    number: match[1], // e.g., "5.1"
    description: match[2], // e.g., "Layout and Basic Controls"
  });
}

console.log(`Stage 3 will execute in ${phases.length} phases:`);
phases.forEach((phase) => {
  console.log(`  - Phase ${phase.number}: ${phase.description}`);
});

**Execute each phase sequentially (with minimal prompts):**

// Execute each phase sequentially
for (let i = 0; i < phases.length; i++) {
  const phase = phases[i];

  console.log(`\n━━━ Stage ${phase.number} - ${phase.description} ━━━\n`);

  const mockupPath = findLatestMockup(pluginName);

  const phaseResult = Task({
    subagent_type: "gui-agent",
    description: `Stage 3.${i+1} - ${pluginName}`,
    prompt: `
You are gui-agent implementing Phase ${phase.number} for ${pluginName}.

**Plugin:** ${pluginName}
**Stage:** 4 (GUI Integration)
**Phase:** ${phase.number} - ${phase.description}
**Your task:** Implement Phase ${phase.number} UI components only

**UI Mockup:** ${mockupPath}

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
4. Implement this phase's UI elements only
5. Build on existing code from previous phases (preserve all)
6. Connect this phase's parameters only
7. Maintain member order (Relays → WebView → Attachments)
8. Return JSON report with phase_completed: "${phase.number}"

Build verification handled by orchestrator after you complete.
    `
  });

  const phaseReport = parseSubagentReport(phaseResult);

  if (!phaseReport || phaseReport.status === "failure") {
    console.log(`✗ Phase ${phase.number} failed`);
    // 4-option menu, stop on failure
    return;
  }

  // Phase succeeded - git commit, update plan, handoff, decision menu
  console.log(`✓ Phase ${phase.number} complete`);

  // Git commit for this phase
  bash(`
git add plugins/${pluginName}/Source/
git add plugins/${pluginName}/ui/
git add plugins/${pluginName}/CMakeLists.txt
git add plugins/${pluginName}/.ideas/plan.md
git add plugins/${pluginName}/.continue-here.md

git commit -m "feat: ${pluginName} Stage ${phase.number} - ${phase.description}"
  `);

  // Decision menu between phases
  if (i < phases.length - 1) {
    console.log(`
✓ Phase ${phase.number} complete

Progress: ${i + 1} of ${phases.length} phases complete

What's next?
1. Continue to Phase ${phases[i + 1].number} (recommended)
2. Review Phase ${phase.number} code
3. Test UI in DAW
4. Pause here
5. Other

Choose (1-5): _
    `);

    const choice = getUserInput();
    if (choice === "4" || choice === "pause") {
      console.log(`\n⏸ Paused between UI phases. Resume with /continue.`);
      return;
    }
  }
}

console.log(`\n✓ All ${phases.length} UI phases complete!`);
```

### 4. Handle Non-Phased Success

**After single-pass success:**

```typescript
// Update state
updateHandoff(
  pluginName,
  5,
  "Stage 2: GUI - WebView UI integrated, all parameters bound",
  ["Auto-test Stage 3", "Test UI in DAW", "Review bindings"],
  complexityScore,
  false
);

updatePluginStatus(pluginName, "🚧 Stage 3");
updatePluginTimeline(
  pluginName,
  5,
  `GUI complete - ${report.outputs.binding_count} parameter bindings`
);

// Git commit
bash(`
git add plugins/${pluginName}/Source/
git add plugins/${pluginName}/ui/
git add plugins/${pluginName}/CMakeLists.txt
git add plugins/${pluginName}/.continue-here.md
git add PLUGINS.md

git commit -m "$(cat <<'EOF'
feat: ${pluginName} Stage 3 - GUI

WebView UI integrated from ${report.outputs.ui_mockup_version}
${report.outputs.binding_count} parameter bindings created
Member order: Relays → WebView → Attachments
CMakeLists.txt updated for WebView support

🤖 Generated with Claude Code

Co-Authored-By: Claude <noreply@anthropic.com>
EOF
)"
`);
```

### 5. Handle Failure

**4-option failure menu:**

```typescript
if (report.status === "failure") {
  // Special handling for binding_mismatch error
  if (report.outputs.error_type === "binding_mismatch") {
    console.log(`
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
✗ Stage 3 Failed: Parameter Binding Mismatch
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Error: Not all parameters from parameter-spec.md have UI bindings

Expected: ${report.outputs.expected_count} parameters
Implemented: ${report.outputs.actual_count} relay/attachment pairs
Missing: ${report.outputs.missing_bindings.join(", ")}

This is a ZERO-DRIFT violation. All parameters must be bound to UI.

What would you like to do?
1. Investigate (why were bindings missing?)
2. Show me the code (PluginEditor.h)
3. Show me the parameter-spec.md
4. I'll fix it manually (say "resume automation" when ready)

Choose (1-4): _
    `);
  } else {
    // Other errors (build failure, member order wrong, etc.)
    console.log(`
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
✗ Stage 3 Failed: UI Integration
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Error: ${report.outputs.error_message}
Type: ${report.outputs.error_type}

Issues:
${report.issues.map((i) => `  - ${i}`).join("\n")}

What would you like to do?
1. Investigate
2. Show me the code (PluginEditor.h/cpp)
3. Show me the build output
4. I'll fix it manually (say "resume automation" when ready)

Choose (1-4): _
    `);
  }

  // Handle choice (same pattern as Stages 1-3)
}
```

### 6. Auto-Invoke plugin-testing Skill

**After Stage 3 succeeds (all phases if phased):**

```typescript
console.log("\n━━━ Running automated tests (including UI validation) ━━━\n");

const testResult = Task({
  subagent_type: "general-purpose",
  description: `Test ${pluginName} after Stage 3`,
  prompt: `
Run automated tests for ${pluginName} after Stage 3 UI integration.

Use the plugin-testing skill to run all 5 tests plus UI-specific checks:
1. Build test (already passed)
2. Load test (plugin loads without crash)
3. Process test (audio processing works)
4. Parameter test (parameters affect audio)
5. State test (save/load works)
6. UI test (WebView renders, parameters sync)

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
✗ Stage 3 Tests FAILED
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Stage 3 UI integration completed, but automated tests failed.

Test results:
${testResult}

CANNOT proceed to Stage 3 (validation) with failing UI tests.

What would you like to do?
1. Investigate test failures
2. Show me the UI code
3. Show me the test output
4. I'll fix it manually (say "resume automation" when ready)

Choose (1-4): _
  `);

  // STOP - Do not proceed to Stage 3 with failing tests
  return;
}

console.log("✓ All Stage 3 tests passed (including UI validation)");
```

### 7. Invoke validator for Complexity ≥4 Plugins

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
    description: `Validate ${pluginName} Stage 3`,
    prompt: `
Validate Stage 3 completion for ${pluginName}.

**Stage:** 5
**Plugin:** ${pluginName}
**Contracts:**
- parameter-spec.md: ${paramSpecContent}
- architecture.md: ${architectureContent}
- plan.md: ${planContent}

**Expected outputs for Stage 2:**
- Member declaration order correct (Relays → WebView → Attachments)
- All parameters from spec have UI bindings
- HTML element IDs match relay names
- UI aesthetic matches mockup
- Visual feedback (knobs respond to parameter changes)
- WebView initialization includes error handling
- Binary data embedded correctly

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
⚠️ Warning: Could not parse validator report as JSON

Raw validator output:
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
${status === "PASS" ? "✓" : "✗"} Validator ${status}: Stage 3 Review
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
1. Complete validation - Final polish and testing ${
      continue_to_next_stage ? "(recommended by validator)" : ""
    }
2. Address validator ${errors.length > 0 ? "errors" : "warnings"} first
3. Review validator report details
4. Test UI manually in DAW - Try out your interface
5. Review UI code
6. Other

Choose (1-6): _
    `);

    // User decides next action based on findings
    // (Advisory layer - user makes final call)
  }
}
````

### 8. Decision Menu (After Tests Pass)

```
✓ UI Integrated
   Your interface is connected and responding to parameter changes

Plugin: [PluginName]
UI: WebView integrated from [mockup version]
Bindings: [N] parameters bound to UI
Tests: All passed (including UI sync)
Status: Ready for final validation

What's next?
1. Complete validation - Final polish and testing (recommended)
2. Test UI in DAW - Try out your interface
3. Review UI code - See implementation details
4. Refine styling - Polish visual design
5. Pause workflow - Resume anytime
6. Other

Choose (1-6): _
```

**CRITICAL: Do NOT proceed to Stage 3 if tests fail.**

---

**Return to:** Main plugin-workflow orchestration in `SKILL.md`
