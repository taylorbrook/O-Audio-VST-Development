# Stage 1: Foundation + Shell

**Context:** This file is part of the plugin-workflow skill.
**Invoked by:** Main workflow dispatcher after Stage 0 completion
**Purpose:** Create plugin structure with build system AND implement ALL parameters with APVTS in single pass

---

**Goal:** Create plugin structure that compiles with full parameter system

**Preconditions:**

- Stage 0 complete (plan.md exists)
- creative-brief.md exists
- architecture.md exists
- parameter-spec.md exists (from finalized UI mockup)

## Actions

### 1. Precondition Check: parameter-spec.md REQUIRED

**BLOCKING check before proceeding:**

```bash
if [ ! -f "plugins/${PLUGIN_NAME}/.ideas/parameter-spec.md" ]; then
  echo "Cannot proceed to Stage 1 - parameter-spec.md missing"
  echo "Complete UI mockup workflow first (/mockup)"
  exit 1
fi
```

**If parameter-spec.md missing:** STOP IMMEDIATELY, exit skill, wait for user to create contract via ui-mockup skill.

### 2. Construct Minimal Prompt

Orchestrator does NOT read contracts - subagent will read them from files.

### 3. Invoke foundation-shell-agent via Task Tool

Call foundation-shell-agent subagent with minimal prompt:

```typescript
const foundationShellResult = Task({
  subagent_type: "foundation-shell-agent",
  description: `Stage 1 - ${pluginName}`,
  prompt: `
You are foundation-shell-agent implementing Stage 1 for ${pluginName}.

**Plugin:** ${pluginName}
**Stage:** 1 (Foundation + Shell)
**Your task:** Create build system and implement ALL parameters

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
3. Create CMakeLists.txt with JUCE 8 integration
4. Create Source/PluginProcessor.{h,cpp} with APVTS
5. Implement ALL parameters from parameter-spec.md
6. Return JSON report with status and file list

Build verification handled by orchestrator after you complete.
  `
});
```

**What foundation-shell-agent implements:**

- Build system (CMakeLists.txt)
- Skeleton source files (PluginProcessor, PluginEditor)
- APVTS with ALL parameters from parameter-spec.md
- Parameter IDs matching spec exactly (zero-drift)
- State management (save/load)
- processBlock stub (no DSP yet)

### 4. Parse JSON Report with Error Handling

**Implement robust JSON parsing:**

```typescript
function parseSubagentReport(rawOutput: string): object | null {
  try {
    // Strategy 1: Extract from markdown code blocks
    const codeBlockMatch = rawOutput.match(/```json\n([\s\S]*?)\n```/);
    if (codeBlockMatch) {
      return JSON.parse(codeBlockMatch[1]);
    }

    // Strategy 2: Find JSON object via brace matching
    const braceStart = rawOutput.indexOf("{");
    const braceEnd = rawOutput.lastIndexOf("}");
    if (braceStart !== -1 && braceEnd !== -1 && braceEnd > braceStart) {
      const jsonCandidate = rawOutput.substring(braceStart, braceEnd + 1);
      return JSON.parse(jsonCandidate);
    }

    // Strategy 3: Try parsing entire output
    return JSON.parse(rawOutput);
  } catch (error) {
    console.error("JSON parsing failed:", error);
    return null;
  }
}

const report = parseSubagentReport(foundationShellResult);

if (!report) {
  console.log(`
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
✗ Stage 1 Error: Could not parse foundation-shell-agent report
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

The foundation-shell-agent subagent completed but returned malformed output.

Raw output:
${foundationShellResult}

What would you like to do?
1. Retry foundation-shell-agent dispatch
2. Show full subagent output
3. Report bug (subagent should return valid JSON)
4. Manual intervention (I'll fix and say "resume automation")

Choose (1-4): _
  `);

  // Wait for user response, handle accordingly
  return;
}
```

### 5. Validate Required Fields

**Check JSON structure:**

```typescript
function validateFoundationShellReport(report: any): {
  valid: boolean;
  error?: string;
} {
  if (!report.agent || report.agent !== "foundation-shell-agent") {
    return { valid: false, error: "Missing or wrong 'agent' field" };
  }

  if (!report.status || !["success", "failure"].includes(report.status)) {
    return { valid: false, error: "Missing or invalid 'status' field" };
  }

  if (!report.outputs || typeof report.outputs !== "object") {
    return { valid: false, error: "Missing 'outputs' object" };
  }

  if (!report.hasOwnProperty("ready_for_next_stage")) {
    return { valid: false, error: "Missing 'ready_for_next_stage' field" };
  }

  // Validate parameter outputs
  if (report.status === "success") {
    if (!report.outputs.parameter_count || typeof report.outputs.parameter_count !== "number") {
      return { valid: false, error: "Missing 'parameter_count' in outputs" };
    }

    if (!report.outputs.parameters_implemented || !Array.isArray(report.outputs.parameters_implemented)) {
      return { valid: false, error: "Missing 'parameters_implemented' array" };
    }
  }

  return { valid: true };
}

const validation = validateFoundationShellReport(report);
if (!validation.valid) {
  console.log(`
✗ Invalid report format: ${validation.error}

Report received:
${JSON.stringify(report, null, 2)}

What would you like to do?
1. Retry with fresh foundation-shell-agent
2. Report bug (malformed JSON structure)
3. Manual intervention

Choose (1-3): _
  `);
  return;
}
```

### 6. Handle foundation-shell-agent Success/Failure

**If status="success" (files created + parameters implemented):**

```typescript
if (report.status === "success" && report.ready_for_next_stage) {
  console.log(`✓ foundation-shell-agent complete: Build system + parameters`);
  console.log(`  - Plugin: ${report.outputs.plugin_name}`);
  console.log(`  - Files: ${report.outputs.source_files_created?.length || 0}`);
  console.log(`  - Parameters: ${report.outputs.parameter_count}`);
  console.log(`  - APVTS: ${report.outputs.apvts_created ? "✓" : "✗"}`);

  // Now invoke build-automation to verify compilation
  console.log(`\nInvoking build-automation to verify compilation...`);

  // Proceed to section 6a (Build Verification)
}
```

**If status="failure":**

Present 4-option failure menu with investigation options.

**Special handling for parameter_mismatch:**

If error_type="parameter_mismatch":
- Show missing_parameters list
- Show implemented_parameters list
- Highlight contract violation (zero-drift broken)

### 6a. Build Verification via build-automation

**CRITICAL: foundation-shell-agent does NOT verify builds. plugin-workflow does this via build-automation.**

**After foundation-shell-agent returns success, invoke build-automation skill:**

```typescript
console.log("✓ foundation-shell-agent complete: Build system + parameters");
console.log("  Files:", report.outputs.source_files_created);
console.log("  Parameters:", report.outputs.parameter_count);
console.log("");
console.log("Invoking build-automation to verify compilation...");

Skill({
  skill: "build-automation",
  context: {
    plugin_name: pluginName,
    stage: 2,
    flags: ["--no-install"],
    invoking_skill: "plugin-workflow",
    purpose: "Verify Stage 1 foundation+shell compiles successfully",
  },
});
```

**build-automation workflow:**

1. Validates plugin directory exists: `plugins/${pluginName}`
2. Validates CMakeLists.txt exists: `plugins/${pluginName}/CMakeLists.txt`
3. Runs build from root: `cd ~/Developer/plugin-freedom-system`
4. Executes: `./scripts/build-and-install.sh ${pluginName} --no-install`
5. Build script runs cmake from root (not plugin directory):
   - `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` (if build/ doesn't exist)
   - `cmake --build build --target ${pluginName}_VST3 --target ${pluginName}_AU`
6. If build succeeds: Display success message, return control to plugin-workflow
7. If build fails: Present 4-option failure protocol

**Build failure protocol (handled by build-automation):**
1. Investigate - Run troubleshooter agent
2. Show build log - Display last 100 lines
3. Show code - Display source files with errors
4. Wait - User fixes manually, says "retry build"

**Build directory location:**
- ✅ Correct: `~/Developer/plugin-freedom-system/build/`
- ❌ Wrong: `~/Developer/plugin-freedom-system/plugins/${pluginName}/build/`

**Why foundation-shell-agent doesn't verify builds:**
- Runs in fresh context with limited tools (no Bash)
- Doesn't know build system architecture

### 6b. Semantic Validation via validation-agent

**After build verification succeeds, invoke validation-agent for semantic review:**

```typescript
console.log("\nInvoking validation-agent for semantic review...");
const validationResult = invokeValidationAgent(pluginName, 2);
const validation = parseValidationReport(validationResult);

if (validation.status === "FAIL" && !validation.continue_to_next_stage) {
  presentValidationFailureMenu(validation);
  return; // Block until user resolves
}

console.log(`✓ Validation ${validation.status}: ${validation.recommendation}`);
```

**validation-agent checks for Stage 1:**
- CMakeLists.txt uses appropriate JUCE modules
- JUCE 8 patterns followed (ParameterID format, header generation)
- All parameters from parameter-spec.md implemented
- Parameter IDs match spec exactly (zero-drift)
- Code organization follows JUCE best practices

**Validation is advisory:** If status is FAIL, user can choose to continue anyway or fix issues.
- Would require understanding monorepo structure
- Separation of concerns: agents create files, build-automation compiles

**Note:** Build failures are handled entirely by build-automation skill. plugin-workflow just invokes it and awaits result.

### 7. Update State Files

```typescript
updateHandoff(
  pluginName,
  2,
  `Stage 1: Foundation + Shell - Build system + ${report.outputs.parameter_count} parameters`,
  [
    "Stage 2: Implement DSP",
    "Review build artifacts",
    "Test parameters in DAW",
  ],
  complexityScore,
  phased
);

updatePluginStatus(pluginName, "🚧 Stage 1");
updatePluginTimeline(
  pluginName,
  2,
  `Foundation + Shell complete - ${report.outputs.parameter_count} parameters`
);
```

### 8. Git Commit

```bash
git add plugins/[PluginName]/Source/
git add plugins/[PluginName]/CMakeLists.txt
git add plugins/[PluginName]/.continue-here.md
git add PLUGINS.md

git commit -m "$(cat <<'EOF'
feat: [PluginName] Stage 1 - foundation + shell

Build system created with JUCE 8 configuration
[N] parameters implemented with APVTS
State management (save/load) functional
Compilation verified (VST3, AU, Standalone)

🤖 Generated with Claude Code

Co-Authored-By: Claude <noreply@anthropic.com>
EOF
)"
```

### 9. Decision Menu

```
✓ Build System Ready
   Your plugin compiles successfully with all parameters implemented

Plugin: [PluginName]
Build artifacts: VST3, AU, Standalone
Parameters: [N] parameters implemented
APVTS: Created and functional
Status: Compiles successfully, loads in DAW (ready for DSP implementation)

What's next?
1. Implement audio processing - Make your DSP vision a reality (recommended)
2. Test parameters in DAW - Verify controls respond
3. Review build system - See generated artifacts
4. Review parameter code - Check APVTS implementation
5. Show parameter list - View all controls
6. Pause workflow - Resume anytime
7. Other

Choose (1-7): _
```

**Handle responses:**

- 1 or "continue": Proceed to Stage 2 (DSP)
- 2: Provide DAW testing instructions
- 3: Show build artifacts list
- 4: Display source files with parameter code
- 5: List all parameters with IDs, types, ranges
- 6 or "pause": Update handoff, exit
- 7 or "other": Ask "What would you like to do?" then re-present menu

---

**Return to:** Main plugin-workflow orchestration in `SKILL.md`
