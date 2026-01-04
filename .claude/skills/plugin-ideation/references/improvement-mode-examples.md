# Improvement Mode: Question Batch Examples

This reference provides detailed examples of adaptive question generation for improvement brainstorming. These examples illustrate how to generate contextual questions based on the user's improvement request and identified gaps.

## Table of Contents

- [Generic Improvement Request](#generic-improvement-request)
- [Specific Feature Addition](#specific-feature-addition)
- [Bug Fix Request](#bug-fix-request)
- [UI Modification](#ui-modification)

---

## Generic Improvement Request

**User input**: "Improve the filters"

**Extracted**:
- Aspect: DSP (inferred)
- Current state: Unclear
- Proposed change: Unclear

**Gaps identified** (4 needed):
- What specific improvement? (Tier 1)
- Current behavior vs proposed? (Tier 1)
- Version impact? (Tier 2)
- Testing criteria? (Tier 2)

**Question Batch** (via AskUserQuestion):

```
Question 1:
  question: "Which aspect would you like to improve?"
  header: "Aspect"
  options:
    - label: "Audio processing (DSP)", description: "Change how it sounds"
    - label: "Parameters", description: "Add/modify/remove controls"
    - label: "User interface", description: "Layout or visual changes"
    - label: "Features/workflow", description: "Presets, MIDI, utilities"

Question 2:
  question: "What's the current behavior you want to change?"
  header: "Current state"
  options:
    - label: "It's broken", description: "Bug or error"
    - label: "It's limited", description: "Missing functionality"
    - label: "It's inefficient", description: "Performance issue"
    - label: "Other", description: "Different issue"

Question 3:
  question: "Version impact of this change?"
  header: "Version bump"
  options:
    - label: "Patch (bugfix)", description: "v1.0.0 → v1.0.1"
    - label: "Minor (new feature)", description: "v1.0.0 → v1.1.0"
    - label: "Major (breaking change)", description: "v1.0.0 → v2.0.0"
    - label: "Other", description: "Not sure"

Question 4:
  question: "How to verify success?"
  header: "Testing"
  options:
    - label: "A/B test audio", description: "Compare before/after sound"
    - label: "Check parameter behavior", description: "Test controls work"
    - label: "Visual inspection", description: "UI looks correct"
    - label: "Other", description: "Different testing approach"
```

---

## Specific Feature Addition

**User input**: "Add resonance parameter to the filter"

**Extracted**:
- Aspect: Parameters (specific) ✓
- Feature name: Resonance ✓
- Action: Add ✓
- Target: Filter ✓

**Gaps identified** (pad to 4 with Tier 3):
- Range and default value? (Tier 2)
- UI placement? (Tier 3)
- Version impact? (Tier 2)
- Backward compatibility? (Tier 3)

**Question Batch** (via AskUserQuestion):

```
Question 1:
  question: "What range for the resonance parameter?"
  header: "Range"
  options:
    - label: "0-1 (normalized)", description: "Standard VST range"
    - label: "0-100%", description: "Percentage style"
    - label: "0.1-10.0", description: "Q-factor style"
    - label: "Other", description: "Custom range"

Question 2:
  question: "Where should this appear in the UI?"
  header: "UI placement"
  options:
    - label: "Next to filter cutoff", description: "Grouped with filter controls"
    - label: "Separate filter section", description: "Dedicated area"
    - label: "Anywhere - user decides", description: "Don't specify"
    - label: "Other", description: "Different placement"

Question 3:
  question: "Version impact of this change?"
  header: "Version bump"
  options:
    - label: "Minor (new feature)", description: "v1.0.0 → v1.1.0"
    - label: "Major (breaking change)", description: "v1.0.0 → v2.0.0"
    - label: "Other", description: "Not sure"

Question 4:
  question: "Should old presets default to 0 resonance?"
  header: "Backward compatibility"
  options:
    - label: "Yes - safe default", description: "No change to existing sounds"
    - label: "No - use middle value", description: "New parameter applies to old presets"
    - label: "Other", description: "Different approach"
```

---

## Bug Fix Request

**User input**: "Filter clicks when adjusting cutoff"

**Extracted**:
- Aspect: DSP (specific) ✓
- Feature name: Filter ✓
- Action: Fix ✓
- Current state: Clicks when adjusting ✓
- Type: Bug ✓

**Gaps identified** (pad to 4 with Tier 3):
- Severity/frequency? (Tier 2)
- Reproduction steps? (Tier 2)
- Expected behavior? (Tier 3)
- Version impact? (Tier 2)

**Question Batch** (via AskUserQuestion):

```
Question 1:
  question: "How often does this clicking occur?"
  header: "Frequency"
  options:
    - label: "Always", description: "Every cutoff adjustment clicks"
    - label: "Sometimes", description: "Intermittent or context-dependent"
    - label: "Rarely", description: "Hard to reproduce"
    - label: "Other", description: "Different pattern"

Question 2:
  question: "How do you reproduce this?"
  header: "Reproduction"
  options:
    - label: "Move cutoff knob quickly", description: "Fast parameter changes"
    - label: "Automate cutoff parameter", description: "DAW automation"
    - label: "MIDI control cutoff", description: "External controller"
    - label: "Other", description: "Different method"

Question 3:
  question: "What should happen instead?"
  header: "Expected behavior"
  options:
    - label: "Smooth cutoff changes", description: "No audible clicks"
    - label: "Slight smoothing", description: "Minimal latency, mostly smooth"
    - label: "Other", description: "Different expectation"

Question 4:
  question: "Version impact of this fix?"
  header: "Version bump"
  options:
    - label: "Patch (bugfix)", description: "v1.0.0 → v1.0.1"
    - label: "Minor (behavior change)", description: "v1.0.0 → v1.1.0 if fix alters sound"
    - label: "Other", description: "Not sure"
```

---

## UI Modification

**User input**: "The plugin UI feels cramped"

**Extracted**:
- Aspect: UI (specific) ✓
- Current state: Cramped ✓
- Proposed change: Unclear

**Gaps identified** (4 needed):
- What specific cramping issue? (Tier 1)
- Which section/controls? (Tier 2)
- Proposed solution? (Tier 2)
- Version impact? (Tier 2)

**Question Batch** (via AskUserQuestion):

```
Question 1:
  question: "What makes the UI feel cramped?"
  header: "Issue"
  options:
    - label: "Controls too close together", description: "Need more spacing"
    - label: "Text too small", description: "Readability issue"
    - label: "Overall window too small", description: "Need larger default size"
    - label: "Other", description: "Different problem"

Question 2:
  question: "Which section feels most cramped?"
  header: "Location"
  options:
    - label: "Top parameter row", description: "Main controls"
    - label: "Bottom section", description: "Additional features"
    - label: "Entire plugin", description: "Everything needs more space"
    - label: "Other", description: "Different area"

Question 3:
  question: "Proposed solution?"
  header: "Fix"
  options:
    - label: "Increase default window size", description: "Make plugin larger"
    - label: "Redesign layout", description: "Rearrange controls"
    - label: "Add resizable window", description: "Let user adjust size"
    - label: "Other", description: "Different approach"

Question 4:
  question: "Version impact of this change?"
  header: "Version bump"
  options:
    - label: "Minor (UI enhancement)", description: "v1.0.0 → v1.1.0"
    - label: "Major (UI redesign)", description: "v1.0.0 → v2.0.0 if layout changes significantly"
    - label: "Other", description: "Not sure"
```

---

## Pattern Recognition

### Specific Improvement Strategy:
1. Extract known information (feature, action, criteria)
2. Identify remaining gaps (typically 1-2 gaps)
3. Pad question batch to 4 with Tier 3 nice-to-haves
4. Usually 1 question batch is sufficient

### Vague Improvement Strategy:
1. Start with foundational questions (aspect, current state)
2. Use answers to refine direction
3. Follow-up batches dig into specifics
4. Typically 2-3 question batches needed

### Common Trap to Avoid:
**Don't ask about what's already explicit.**
If user says "add resonance parameter to the filter," do NOT ask "Which aspect?" (already know: parameters) or "What feature?" (already know: resonance)
