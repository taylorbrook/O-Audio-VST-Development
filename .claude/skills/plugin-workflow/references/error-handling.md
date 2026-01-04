# Error Handling

Comprehensive error detection, recovery strategies, and user action patterns for plugin-workflow orchestration.

## Contents

- [Common Errors](#common-errors)
- [Graceful Degradation](#graceful-degradation)
- [Error Reporting Format](#error-reporting-format)

---

## Common Errors

### Subagent returns invalid JSON

**Detection:** JSON parse error when reading subagent output

**Recovery:** Use fallback parsing strategies (extract from code blocks, partial JSON)

**User action:** If all parsing fails, present raw output and offer manual recovery

---

### Schema validation failure

**Detection:** jsonschema.ValidationError when validating report

**Recovery:** Log specific validation error, block progression

**User action:** Present error details, offer to retry subagent or continue anyway (risky)

---

### Build failure after stage

**Detection:** build-automation returns success: false

**Recovery:** Present build log excerpt in checkpoint menu

**User action:** Choose from: retry build, investigate logs, manual fix, rollback stage

---

### State file corruption

**Detection:** YAML parse error or missing required fields in .continue-here.md

**Recovery:** Reconstruct from git log and PLUGINS.md

**User action:** Verify reconstructed state before continuing

---

### Checkpoint step failure

**Detection:** Git commit fails, file write fails, etc.

**Recovery:** Log specific step failure, attempt remaining steps

**User action:** Present partial checkpoint status, offer manual completion

---

## Graceful Degradation

### When validation-agent unavailable

- Skip validation step
- Log warning in checkpoint
- Continue to present checkpoint menu

### When build-automation unavailable

- Fall back to direct cmake/xcodebuild (legacy mode)
- Log warning about using legacy build

### When Required Reading file missing

- Log warning
- Continue subagent invocation without Required Reading
- May encounter known issues without pattern protection

---

## Error Reporting Format

All errors presented to the user include:

1. **What failed** - Specific stage/step that failed
2. **Why it failed** - Error message or root cause
3. **How to fix** - Actionable recovery steps
4. **Alternative** - Other ways to proceed (pause, rollback, manual fix)

### Example Error Message

```
ERROR: Stage 2 (Shell) failed

Reason: Parameter count mismatch (expected 5, found 3)
Missing parameters: delayTime, feedback

What should I do?

1. Retry Stage 1 - foundation-shell-agent will regenerate APVTS
2. Check parameter-spec.md - Verify parameter definitions
3. Manual fix - Edit PluginProcessor.cpp manually then continue
4. Pause workflow - Save progress and investigate

Choose (1-4): _
```

This format ensures users always have:
- Clear understanding of what went wrong
- Actionable paths forward
- Ability to pause and investigate manually if needed
