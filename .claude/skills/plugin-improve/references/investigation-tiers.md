# Investigation Tier Protocols

**Purpose**: Detailed investigation procedures for each tier (1/2/3).

**Auto-detection**: plugin-improve automatically selects tier based on complexity indicators (see Phase 0.5 detection algorithm in SKILL.md).

---

## Tier 1: Basic Code Inspection (5-10 minutes)

**When to use:**
- Cosmetic changes, simple fixes, obvious issues
- Known patterns from troubleshooting/
- Single-file scope, UI-only changes

**Protocol:**

1. **Read relevant source files:**
   - PluginProcessor.cpp/h
   - PluginEditor.cpp/h
   - Relevant JUCE modules

2. **Check for:**
   - Obvious typos or errors
   - Known pattern matches (search troubleshooting/)
   - Simple logic issues
   - Missing null checks
   - Incorrect constants

3. **Present findings:**
   - What's wrong (one sentence)
   - Root cause (one sentence)
   - Fix (specific file and line number)

**Example:**
```
Problem: Button color doesn't change on hover
Root cause: Missing setColour() call in PluginEditor constructor
Fix: Add setColour(TextButton::buttonOnColourId, ...) at line 42
```

---

## Tier 2: Root Cause Analysis (15-30 minutes)

**When to use:**
- Logic errors, parameter issues, integration bugs
- Single component, requires code analysis
- Functional bug with specific symptom

**Protocol:**

1. **Trace logic flow:**
   - Start from symptom (what user sees)
   - Follow code path backward to cause
   - Check integration points between components

2. **Analyze deeper:**
   - Review parameter definitions (createParameterLayout)
   - Examine state management (getStateInformation/setStateInformation)
   - Check threading issues (processBlock vs GUI thread)
   - Verify APVTS usage (parameter access patterns)
   - Look for race conditions

3. **Test hypothesis:**
   - Identify suspected cause
   - Verify with code inspection
   - Check if fix would resolve symptom

4. **Present findings:**
   ```markdown
   ## Investigation Findings

   ### Problem Analysis
   [What's actually wrong and why it's happening]

   ### Root Cause
   [Technical explanation of the underlying issue]

   ### Affected Files
   - plugins/[Name]/Source/[File]:[Line]
   - plugins/[Name]/Source/[File]:[Line]

   ### Recommended Approach
   [How to fix it properly - not a workaround]

   ### Alternative Approaches
   [Other valid options with trade-offs explained]

   ### Backward Compatibility
   [Will this break existing presets/sessions?]

   Proceed with recommended approach? (y/n): _
   ```

**Example:**
```
Problem Analysis:
Reverb parameter changes don't apply until preset reloaded.

Root Cause:
PluginProcessor::parameterChanged() doesn't call reverb.updateParameters().
APVTS notifies processor of changes, but processor doesn't propagate to DSP.

Affected Files:
- plugins/LushVerb/Source/PluginProcessor.cpp:156 (parameterChanged callback)

Recommended Approach:
Add reverb.updateParameters() call in parameterChanged() callback.

Alternative Approaches:
1. Poll parameters in processBlock (worse performance)
2. Use AudioProcessorValueTreeState::Listener (more complex)

Backward Compatibility:
No breaking changes - pure bug fix.
```

---

## Tier 3: Deep Research (Delegate)

**When to use:**
- Complex bugs, performance issues, architectural problems
- Multi-component, requires deep investigation
- Crashes, "all plugins" issues, unclear root cause

**Protocol:**

1. **Invoke deep-research skill:**
   ```
   Complex issue detected. Invoking deep-research skill...
   ```

2. **Provide context:**
   - Problem description
   - Plugin name
   - Current stage/state
   - Symptoms observed

3. **deep-research performs:**
   - Graduated investigation (Levels 1-3)
   - JUCE documentation search
   - Pattern analysis across codebase
   - Returns structured findings with recommendations

4. **Use research output:**
   - Continue with Phase 0.5 "Present findings" format
   - Use recommended solution in implementation
   - Document research tier in CHANGELOG

**Note**: deep-research uses Opus with extended thinking for complex analysis. If research already performed (Phase 0.45 detection), skip this step.

---

## Escalation Protocol

**When to escalate:**
- Tier 1 investigation doesn't find obvious cause → Escalate to Tier 2
- Tier 2 investigation reveals multi-component complexity → Escalate to Tier 3
- Any tier reveals architectural issue → Escalate to Tier 3

**How to escalate:**
1. Log escalation: "Initial investigation insufficient, escalating to Tier [N]"
2. Apply higher-tier protocol
3. Document escalation in CHANGELOG (shows thoroughness)

---

## Planning Trigger Detection (Phase 0.5 Extension)

After investigation completes, check tier to determine if Phase 0.6 (Implementation Planning) should trigger.

### Tier 1: No Planning

**Characteristics:**
- Cosmetic changes, typo fixes, single-parameter adjustments
- Single-file scope
- Known patterns from troubleshooting/
- Quick fixes with obvious solutions

**Action:** Skip Phase 0.6 entirely. Proceed directly to Phase 0.9 (Backup Verification).

**Rationale:** Planning overhead not justified for simple fixes. Keep the fast path fast.

---

### Tier 2: Planning Suggested

**Characteristics:**
- Logic errors, multi-parameter changes, integration fixes
- 2-5 files affected
- DSP changes (but not architectural)
- Requires understanding of component interactions

**Keywords detected:** "change", "fix", "modify", "update", "adjust"

**Action:** Soft suggestion to user:
```
This looks moderately complex. Planning recommended.

Would you like me to create an implementation plan before starting?

1. Yes, create plan
2. No, proceed directly
3. Other

Choose (1-3): _
```

**Proceed to Phase 0.6** if user accepts, otherwise skip to Phase 0.9.

---

### Tier 3: Planning Required

**Characteristics:**
- Architectural changes, major refactors, multi-system improvements
- 5+ files affected
- New features touching multiple components
- High risk of rework without upfront planning

**Keywords detected:** "refactor", "overhaul", "redesign", "rewrite", "architecture"

**Action:** Strong suggestion to user:
```
This is a significant change. Planning recommended before starting.

Would you like me to create an implementation plan?

1. Yes, create plan (recommended)
2. No, proceed directly
3. Other

Choose (1-3): _
```

**Proceed to Phase 0.6** if user accepts, otherwise skip to Phase 0.9.

---

### Notes

**Tier boundary between 2 and 3 is Claude's discretion based on investigation findings.** Use judgment based on:
- Number of files affected
- Complexity of changes
- Risk of unintended side effects
- Whether changes touch DSP vs UI vs both

**User can decline planning suggestion or use --no-plan flag to bypass.** The suggestion is not a hard gate - users who prefer to work iteratively can skip planning even for Tier 3.

**See:** `references/implementation-planning.md` for complete Phase 0.6 protocol.
