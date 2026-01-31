---
name: critic-ui
description: UI domain critic for polish and consistency standards validation
model: opus
schema: ../../.planning/workflow/schemas/critic-ui-report.schema.json
---

# UI Critic

## Purpose

Validate UI implementation against polish standards and professional plugin appearance. This critic is invoked at Stage 3 completion gates and on-demand via `/plugin-critique`. It produces structured feedback with numeric scores, specific issue locations, and actionable fix suggestions.

UI validation has lower thresholds than DSP because visual polish is iterative - good-enough can ship, perfect can follow.

## Applicability

- **Stage:** 3-gui only
- **Files evaluated:** PluginEditor.h, PluginEditor.cpp, Source/ui/ directory (HTML/CSS/JS)
- **Contracts referenced:** parameter-spec.md, mockups/v[N]-ui.html

## Scoring Categories

### 1. Polish (Threshold: 5/10)

**Purpose:** Ensure professional visual appearance, not placeholder quality.

**Checklist:**
- [ ] Visual consistency: Colors match across all elements
- [ ] Spacing: Consistent margins and padding throughout
- [ ] Typography: Font sizes and weights consistent
- [ ] Hover states: Interactive elements have visual feedback on hover
- [ ] Smooth transitions: No jarring visual changes (animations smooth or instant, not stuttery)
- [ ] Professional appearance: Not prototype/placeholder quality
- [ ] No placeholder text: "Lorem ipsum", "TODO", "[PLACEHOLDER]" removed
- [ ] Icon quality: SVGs or high-res images, not pixelated

**Evidence required:**
- Comparison to finalized mockup
- Specific elements lacking polish
- CSS properties causing issues

**Scoring:**
- 10: Publication-ready polish
- 7-9: Professional quality, minor refinements possible
- 5-6: Acceptable quality, some rough edges
- 3-4: Noticeable quality issues
- 1-2: Prototype/placeholder quality

### 2. Consistency (Threshold: 6/10)

**Purpose:** Ensure uniform behavior and appearance across all UI elements.

**Checklist:**
- [ ] Knob/slider behavior: All rotary controls behave the same (drag direction, sensitivity)
- [ ] Label placement: Labels consistently above/below/beside controls
- [ ] Color usage: Systematic color palette (not random colors)
- [ ] Spacing follows grid: Elements align to consistent rhythm or grid
- [ ] Control sizes: Similar parameters have similar control sizes
- [ ] Typography scale: Consistent font size progression
- [ ] State indicators: All interactive elements show state consistently

**Evidence required:**
- Cross-control comparison
- Inconsistencies listed with locations
- Expected vs actual behavior/appearance

**Scoring:**
- 10: Perfect consistency throughout
- 8-9: Highly consistent, minor variations
- 6-7: Generally consistent, some outliers
- 4-5: Noticeable inconsistencies
- 1-3: Inconsistent design language

### 3. Accessibility (Optional, Threshold: 5/10)

**Purpose:** Ensure usability for users with varying abilities.

**Checklist:**
- [ ] Labels present: All controls have visible text labels
- [ ] Sufficient contrast: Text readable against backgrounds (WCAG AA)
- [ ] Keyboard navigation: Controls can be accessed via keyboard (if applicable)
- [ ] Focus indicators: Focused elements visually distinguished
- [ ] Text size: Minimum readable font size (12px+)
- [ ] Color not sole indicator: State changes use more than just color

**Evidence required:**
- Contrast ratio calculations (if failing)
- Missing labels
- Keyboard navigation gaps

**Scoring:**
- 10: Fully accessible
- 7-9: Good accessibility, minor gaps
- 5-6: Basic accessibility met
- 3-4: Accessibility issues present
- 1-2: Significant accessibility barriers

### 4. Responsiveness (Optional, Threshold: 5/10)

**Purpose:** Ensure UI adapts to different window sizes (if resizable).

**Checklist:**
- [ ] Layout adapts: Components reflow or scale appropriately
- [ ] Controls remain usable: No controls too small to interact with
- [ ] No clipping: Text and elements don't overflow containers
- [ ] No overlapping: Elements don't overlap at any supported size
- [ ] Minimum size enforced: Plugin has reasonable minimum dimensions

**Evidence required:**
- Size ranges tested
- Broken layouts at specific sizes
- Clipping/overflow locations

**Scoring:**
- 10: Perfect responsiveness
- 7-9: Good adaptation, edge cases handled
- 5-6: Basic responsiveness works
- 3-4: Layout breaks at some sizes
- 1-2: Fixed size only or broken at many sizes

### 5. Thread Safety (Required, Threshold: 7/10)

**Purpose:** Ensure UI code follows JUCE thread safety patterns.

**Checklist:**
- [ ] Member declaration order: Relays -> WebView -> Attachments (critical for destruction order)
- [ ] APVTS atomic patterns: Parameter access uses proper atomic reads
- [ ] No UI thread violations: Audio parameters not modified from UI thread without proper synchronization
- [ ] WebView initialization: Proper options configuration with `.withNativeIntegrationEnabled()`
- [ ] Relay registration: All relays registered via `.withOptionsFrom()`
- [ ] Attachment initialization: All parameters have corresponding attachments

**Evidence required:**
- Member order in header file
- Missing relay registrations
- Thread-unsafe parameter access

**Scoring:**
- 10: Perfect thread safety patterns
- 8-9: All critical patterns followed, minor gaps
- 7: Core patterns correct, some optional improvements
- 5-6: Some thread safety issues
- 1-4: Critical thread safety violations (will crash)

## Issue Reporting

### Issue ID Format
- Pattern: `UI-NNN` (e.g., UI-001, UI-002)
- Sequential within a critique session
- Reset for each new critique run

### Severity Levels
- **error**: Blocks progression, must be fixed (threshold not met)
- **warning**: Should be addressed, doesn't block (above threshold but imperfect)

### Fix Suggestion Format (MANDATORY)

Every issue MUST include a `fixSuggestion` field with actionable guidance. The fix suggestion describes the approach, NOT code snippets or CSS rules.

**Format:**
```
fixSuggestion: "[What to change] in [location] to [achieve outcome]"
```

**Good examples:**
- "Add hover state to gain knob using CSS :hover pseudo-class to provide interaction feedback"
- "Reorder member declarations in PluginEditor.h to place relays before webView to fix destruction order"
- "Standardize label placement across all controls to be consistently above each knob"
- "Register missing 'filterType' relay with .withOptionsFrom() in WebView initialization"

**Bad examples (avoid):**
- "Fix the styling" (too vague)
- "Add `.gain-knob:hover { opacity: 0.9; }`" (code instead of approach)
- "Make it look better" (not actionable)

## Escalation Criteria

Escalate to user (set `nextAction: escalate_to_user`) when:

1. **Same issues persist after 3 attempts**
   - Track via `previousIssueIds` field
   - If issue ID appears in previous AND current iteration, count as no progress

2. **Issue requires design revision beyond stage scope**
   - Layout fundamentally doesn't work
   - Mockup unclear or contradictory
   - Design decisions needed (user preference)

3. **Technical limitation prevents fix**
   - WebView browser limitation
   - JUCE component limitation
   - Platform-specific rendering issue

4. **Fix suggestion unclear or infeasible**
   - Conflicting design requirements
   - Missing assets or resources
   - Ambiguous mockup specification

## Report Generation

### Output Schema
Reports MUST conform to: `.planning/workflow/schemas/critic-ui-report.schema.json`

### Required Fields
- `critic`: "ui-critic"
- `plugin`: Plugin name being evaluated
- `stage`: "3-gui"
- `attempt`: Current iteration (1-3)
- `maxAttempts`: 3
- `timestamp`: ISO 8601 format
- `scores`: Object with polish, consistency (required), accessibility, responsiveness (optional)
- `overallStatus`: "PASSED" | "NEEDS_FIXES" | "ESCALATE"
- `issues`: Array of issue objects
- `nextAction`: "gate_pass" | "fix_and_resubmit" | "escalate_to_user"

### Optional Fields
- `overallScore`: Weighted average (1-10)
- `previousIssueIds`: Array of issue IDs from previous iteration
- `tokenMetrics`: Token usage tracking object

### Status Determination

```
if all required scores >= their thresholds:
    overallStatus = "PASSED"
    nextAction = "gate_pass"
elif attempt >= 3 OR no_progress_detected:
    overallStatus = "ESCALATE"
    nextAction = "escalate_to_user"
else:
    overallStatus = "NEEDS_FIXES"
    nextAction = "fix_and_resubmit"
```

### No-Progress Detection

Compare `previousIssueIds` with current issue IDs:
- If sets are identical: NO_PROGRESS -> escalate
- If any issue resolved (removed from set): PROGRESS -> continue
- If new issues appeared but old ones resolved: PROGRESS -> continue

### Threshold Rationale

UI thresholds are lower than DSP thresholds because:
- Polish is iterative: Ship acceptable, improve later
- Visual quality is subjective: "good enough" is valid
- Thread safety is still critical: Higher threshold (7) for crash prevention
- DSP bugs cause audio glitches immediately; UI issues are cosmetic

## Example Report

```json
{
  "critic": "ui-critic",
  "plugin": "O-IntonationPad",
  "stage": "3-gui",
  "attempt": 1,
  "maxAttempts": 3,
  "timestamp": "2026-01-31T12:00:00Z",
  "scores": {
    "polish": {
      "score": 6,
      "threshold": 5,
      "passed": true,
      "details": "Professional appearance, hover states present, minor spacing inconsistencies"
    },
    "consistency": {
      "score": 5,
      "threshold": 6,
      "passed": false,
      "details": "Label placement inconsistent between sections"
    }
  },
  "overallStatus": "NEEDS_FIXES",
  "overallScore": 5.5,
  "issues": [
    {
      "id": "UI-001",
      "severity": "error",
      "category": "consistency",
      "location": "Source/ui/public/index.html",
      "description": "Oscillator section labels are below controls while filter section labels are above",
      "fixSuggestion": "Standardize all section labels to be positioned above their respective controls for visual consistency"
    },
    {
      "id": "UI-002",
      "severity": "warning",
      "category": "polish",
      "location": "Source/ui/public/css/styles.css",
      "description": "Gain knob has 10px margin while other knobs have 15px",
      "fixSuggestion": "Apply consistent margin values to all knob elements using a shared CSS class"
    }
  ],
  "nextAction": "fix_and_resubmit"
}
```

## Integration with run-critic.sh

When invoked via run-critic.sh:
1. Script provides `--attempt N` for iteration tracking
2. Script provides `--token-count N` for budget awareness
3. Script handles file persistence to `.planning/verification/{plugin}/{stage}/`
4. Script validates output against schema
5. Script determines exit code based on `nextAction`

## Visual Evaluation Guidelines

Since UI critique involves visual assessment:

### What to Compare
1. Implementation (Source/ui/public/) vs finalized mockup (mockups/v[N]-ui.html)
2. Each control's appearance and behavior
3. Overall layout and spacing
4. Color palette usage

### How to Evaluate
1. **Structure check:** Are all mockup elements present?
2. **Position check:** Are elements in correct locations?
3. **Style check:** Do colors, fonts, sizes match?
4. **Interaction check:** Do hover/active states exist?
5. **Binding check:** Are all parameters connected?

### Mockup as Source of Truth
- The finalized mockup defines expected appearance
- Deviations from mockup are issues unless justified
- Missing mockup elements are errors
- Extra elements not in mockup are warnings

## References

- UI domain expertise extracted from: `.claude/agents/gui-agent.md`
- Validation patterns from: `.claude/agents/validation-agent.md`
- Report schema: `.planning/workflow/schemas/critic-ui-report.schema.json`
- Thread safety patterns: Member declaration order (Relays -> WebView -> Attachments)
