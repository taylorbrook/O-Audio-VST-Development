---
name: plugin-critique
description: Run domain-specific critic on plugin stage outputs for quality validation
arguments:
  - name: plugin
    description: Plugin name (e.g., O-IntonationPad)
    required: true
  - name: stage
    description: Stage to critique (2-dsp or 3-gui)
    required: true
  - name: attempt
    description: Attempt number (1-3)
    required: false
    default: "1"
---

# Plugin Critique

Run domain-specific critics to validate plugin implementation quality. This command invokes the appropriate critic template and generates a structured report conforming to the critic report schema.

## Usage

```
/plugin-critique O-IntonationPad 2-dsp
/plugin-critique O-IntonationPad 3-gui --attempt 2
```

## Workflow

### 1. Validate Arguments

**Check plugin exists:**
```bash
PLUGIN_DIR="plugins/$ARGUMENTS.plugin"
if [ ! -d "$PLUGIN_DIR" ]; then
    echo "ERROR: Plugin directory not found: $PLUGIN_DIR"
    exit 1
fi
```

**Check stage is valid:**
```bash
case "$ARGUMENTS.stage" in
    2-dsp|3-gui)
        # Valid stage
        ;;
    *)
        echo "ERROR: Invalid stage '$ARGUMENTS.stage'. Must be '2-dsp' or '3-gui'"
        exit 1
        ;;
esac
```

### 2. Determine Applicable Critic

| Stage | Critic | Template |
|-------|--------|----------|
| 2-dsp | dsp-critic | .claude/critics/critic-dsp.md |
| 3-gui | ui-critic | .claude/critics/critic-ui.md |

### 3. Load Files for Critique

**For Stage 2-dsp:**
- `plugins/$PLUGIN/Source/PluginProcessor.h`
- `plugins/$PLUGIN/Source/PluginProcessor.cpp`
- `plugins/$PLUGIN/.planning/parameter-spec.md`
- `plugins/$PLUGIN/.planning/research/ARCHITECTURE.md`

**For Stage 3-gui:**
- `plugins/$PLUGIN/Source/PluginEditor.h`
- `plugins/$PLUGIN/Source/PluginEditor.cpp`
- `plugins/$PLUGIN/Source/ui/` directory (index.html, css/, js/)
- `plugins/$PLUGIN/.planning/parameter-spec.md`
- `plugins/$PLUGIN/.planning/mockups/v*-ui.html` (latest version)

### 4. Apply Critic Rules

Load the critic template from `.claude/critics/critic-{dsp|ui}.md` and evaluate the source files against each scoring category:

**DSP Critic categories:**
1. Real-Time Safety (threshold: 8)
2. Buffer Handling (threshold: 7)
3. Parameter Integration (threshold: 6)
4. Numerical Stability (optional, threshold: 6)
5. Architecture Alignment (optional, threshold: 5)

**UI Critic categories:**
1. Polish (threshold: 5)
2. Consistency (threshold: 6)
3. Accessibility (optional, threshold: 5)
4. Responsiveness (optional, threshold: 5)
5. Thread Safety (required, threshold: 7)

### 5. Generate Scores

For each category:
1. Run through the checklist items
2. Count passing/failing checks
3. Calculate score (1-10)
4. Compare to threshold
5. Mark as passed/failed

### 6. Identify Issues

For each failed check or concerning pattern:
1. Assign issue ID (DSP-001, UI-001, etc.)
2. Determine severity (error if threshold-blocking, warning otherwise)
3. Provide specific location (file:line)
4. Write description of what's wrong
5. Write fix suggestion describing approach

### 7. Generate Report

Create report conforming to schema:

**DSP Report Schema:** `.planning/workflow/schemas/critic-dsp-report.schema.json`
**UI Report Schema:** `.planning/workflow/schemas/critic-ui-report.schema.json`

### 8. Invoke run-critic.sh

After generating the report, call the orchestration script:

```bash
.planning/workflow/scripts/run-critic.sh "$ARGUMENTS.plugin" "$ARGUMENTS.stage" \
    --attempt "$ARGUMENTS.attempt"
```

The script handles:
- Iteration tracking
- No-progress detection
- Failure report persistence
- Exit code determination

## Output Format

Display structured summary:

```
========================================
         CRITIC VALIDATION
========================================

Critic:       [dsp-critic|ui-critic]
Plugin:       [plugin name]
Stage:        [2-dsp|3-gui]

+------------------------------------+
|      Attempt [N] / 3               |
+------------------------------------+

Scores:
  realtime_safety:      9/10 (threshold: 8) [PASSED]
  buffer_handling:      7/10 (threshold: 7) [PASSED]
  parameter_integration: 4/10 (threshold: 6) [FAILED]

Issues:
  DSP-001 [error] parameter_integration
    Location: PluginProcessor.cpp:87
    Description: 'resonance' parameter not read in processBlock
    Fix: Add resonance readout and apply to filter Q

  DSP-002 [error] parameter_integration
    Location: PluginProcessor.cpp
    Description: 'spread' parameter not affecting stereo width
    Fix: Read spread parameter and modulate stereo processing

Overall: NEEDS_FIXES
Next Action: Fix issues and run /plugin-critique [plugin] 2-dsp --attempt 2

----------------------------------------
Report saved to: .planning/verification/[plugin]/[stage]/critic-report-attempt[N].json
```

## Status Determination

| Condition | Status | Next Action |
|-----------|--------|-------------|
| All required scores >= thresholds | PASSED | gate_pass |
| Some scores < threshold, attempt < 3 | NEEDS_FIXES | fix_and_resubmit |
| Attempt >= 3 OR no progress | ESCALATE | escalate_to_user |

## Integration with Stage Gates

This command can be invoked:

1. **Manually:** User runs `/plugin-critique` for on-demand validation
2. **At gates:** `stage-transition-gate.sh` invokes `run-critic.sh` (Phase 5 integration)
3. **In workflow:** Plugin-workflow skill triggers critique before stage advancement

All paths use the same critic templates and report format.

## Iteration Workflow

**First attempt:**
```
/plugin-critique O-IntonationPad 2-dsp
```
-> Returns issues list with fix suggestions

**After fixes:**
```
/plugin-critique O-IntonationPad 2-dsp --attempt 2
```
-> run-critic.sh compares with previous issues
-> Detects progress or no-progress
-> Returns updated scores

**If still failing after attempt 3:**
-> Escalates to user with accumulated context
-> User decides next action

## Token Budget Awareness

When invoked with token tracking:

```bash
.planning/workflow/scripts/run-critic.sh "$PLUGIN" "$STAGE" \
    --attempt "$ATTEMPT" \
    --token-count "$CUMULATIVE_TOKENS"
```

If cumulative tokens exceed 50K soft limit:
- Warning displayed but validation continues
- Suggestion to simplify or escalate

## Report Persistence

**On PASS:** No report saved (per CONTEXT.md decision)

**On FAIL/ESCALATE:** Report saved to:
```
.planning/verification/{plugin}/{stage}/critic-failure-{timestamp}.json
```

This creates audit trail for:
- Debugging recurring issues
- Tracking iteration history
- Analyzing common failure patterns

## Error Handling

**Plugin not found:**
```
ERROR: Plugin 'NonExistent' not found in plugins/ directory
Available plugins: [list of existing plugins]
```

**Invalid stage:**
```
ERROR: Stage '4-test' is not valid for critique
Valid stages: 2-dsp, 3-gui
```

**Missing source files:**
```
ERROR: Cannot critique stage 2-dsp - PluginProcessor.cpp not found
Expected: plugins/[plugin]/Source/PluginProcessor.cpp
```

**Missing contracts:**
```
WARNING: parameter-spec.md not found - parameter integration check limited
Continuing with available files...
```

## Examples

**Critique DSP implementation:**
```
/plugin-critique O-IntonationPad 2-dsp
```

**Second attempt after fixes:**
```
/plugin-critique O-IntonationPad 2-dsp --attempt 2
```

**Critique UI implementation:**
```
/plugin-critique O-IntonationPad 3-gui
```

**Check all plugins at stage 2:**
```bash
for plugin in plugins/*/; do
    name=$(basename "$plugin")
    /plugin-critique "$name" 2-dsp
done
```
