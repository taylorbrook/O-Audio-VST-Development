# Research Detection Protocol (Phase 0.45)

**Purpose**: Detect if deep-research already investigated this issue and extract findings to avoid duplicate work.

**Enforcement**: MANDATORY - Always executes, never skip

## Contents

1. [Why This Matters](#why-this-matters)
2. [Detection Algorithm](#detection-algorithm)
3. [Extraction Logic](#extraction-logic)
4. [Decision Logic](#decision-logic)
5. [State Tracking](#state-tracking)
6. [Anti-Patterns](#anti-patterns)

---

## Why This Matters

Deep-research Tier 3 uses Opus with extended thinking (expensive, minutes per query). If findings already exist in conversation history, reusing them saves time and cost.

**Trigger**: After Phase 0.3 or Phase 0.4 completes, BEFORE Phase 0.5 investigation

---

## Detection Algorithm

<scan_conversation_history>
Look for these markers in conversation history (starting from most recent):

1. **deep-research skill invocation:**
   - Search for: "deep-research" in previous messages
   - Look for tier/level indicators: "Tier 1", "Tier 2", "Tier 3", "Level 1", "Level 2", "Level 3"
   - Check for completion markers: "Research complete", "Investigation findings", "Investigation Summary"

2. **Research output sections:**
   - "Root Cause Analysis"
   - "Root Cause:"
   - "Solutions Identified"
   - "Recommended Solution"
   - "Recommendations"
   - "Investigation Summary"
   - "Implementation Steps"
   - "Implementation Roadmap"

3. **Handoff signals:**
   - "Invoking plugin-improve with findings"
   - "Invoking plugin-improve skill"
   - "Handing off to plugin-improve skill"
   - "[deep-research → plugin-improve] handoff"
   - "Ready to implement?"

4. **Context clues:**
   - User mentioned "after researching"
   - User said "based on the investigation"
   - User references prior troubleshooting
   - User said "from the research findings"
</scan_conversation_history>

---

## Extraction Logic

When research markers detected in conversation history, extract:

- **Research Tier/Level:** Which investigation depth was used (1/2/3)
- **Problem Statement:** What issue was being investigated
- **Root Cause:** Technical explanation of underlying issue
- **Recommended Solution:** Primary approach suggested by research
- **Alternative Solutions:** Other valid approaches with trade-offs
- **Implementation Steps:** Ordered tasks to apply the solution
- **Affected Files:** Which source files need modification
- **Testing Notes:** How to verify the fix works

---

## Decision Logic

### If Research Detected

When research findings detected in conversation history:

1. **Display findings summary:**
   ```
   ✓ Research handoff detected from deep-research skill

   Investigation: Tier/Level ${tier} (${tierDescription})
   Problem: ${problemStatement}
   Root Cause: ${rootCause}
   Recommended Solution: ${recommendedSolution}

   Using existing research findings (skipping Phase 0.5 investigation).
   ```

2. **Skip Phase 0.5:**
   - Do NOT run investigation logic
   - Do NOT invoke deep-research again
   - Proceed directly to Phase 0.9 (Backup Verification) with findings

3. **Set context for implementation:**
   - RESEARCH_SOURCE = "deep-research handoff"
   - ROOT_CAUSE = extracted root cause
   - PROPOSED_SOLUTION = extracted recommended solution
   - IMPLEMENTATION_STEPS = extracted steps
   - Use these in improvement planning and CHANGELOG

4. **Present implementation approval:**
   ```
   Ready to implement this solution?

   1. Yes, proceed with recommended solution
   2. No, use alternative approach - Show me alternatives
   3. No, investigate further - Run fresh investigation (Phase 0.5)
   4. Other

   Choose (1-4): _
   ```

   **Handle responses:**
   - Option 1 → Proceed to Phase 0.9 (Backup Verification)
   - Option 2 → Show alternative solutions from research, ask which to use
   - Option 3 → Proceed to Phase 0.5 (Investigation) despite research existing
   - Option 4 → Collect free-form text, reassess

### If No Research Detected

When NO research findings detected in conversation history:

1. **Log detection completion:**
   ```
   No research handoff detected in conversation history.
   Proceeding to Phase 0.5 (Investigation).
   ```

2. **Continue to Phase 0.5:**
   - Run normal investigation logic (auto-tiered)
   - May invoke deep-research if Tier 3 detected (user informed)

3. **Detection was performed:**
   - Important: Even if nothing found, detection ran (MANDATORY)
   - This prevents false negatives from lazy evaluation
   - Phase 0.45 always executes, never skipped

---

## State Tracking

Document whether research was detected for audit trail:

```bash
# Create/append to improvement context file
echo "research_detection_performed: true" >> .improve-context.yaml
echo "research_handoff_detected: ${FINDINGS_DETECTED}" >> .improve-context.yaml
echo "research_tier: ${TIER:-none}" >> .improve-context.yaml
echo "detection_timestamp: $(date -u +%Y-%m-%dT%H:%M:%SZ)" >> .improve-context.yaml
```

---

## Anti-Patterns

<anti_pattern severity="CRITICAL">
❌ NEVER skip Phase 0.45 research detection
❌ NEVER assume no research exists without scanning conversation history
❌ NEVER re-run deep-research if findings already in conversation
❌ NEVER ignore handoff signals from deep-research skill
✓ ALWAYS scan conversation history in Phase 0.45
✓ ALWAYS extract findings if markers detected
✓ ALWAYS skip Phase 0.5 if research already complete (unless user overrides)
✓ ALWAYS document detection results in .improve-context.yaml
</anti_pattern>

---

## Integration with Phase Flow

```
Phase 0 (Specificity) → Phase 0.3 (Clarification) → Phase 0.4 (Decision Gate)
                                                        ↓
                                    Phase 0.45 (Research Detection) ← YOU ARE HERE
                                                        ↓
                                  [Research detected?] decision point
                                        ↙                    ↘
                    YES: Skip to Phase 0.9              NO: Continue to Phase 0.5
                    (Backup Verification)                (Investigation)
```

---

## Benefits

- Avoids duplicate investigation (user already ran /research or deep-research auto-invoked)
- Preserves expensive research context (Opus + extended thinking costs real time and money)
- Maintains separation of concerns: research finds solutions, improve implements them
- Prevents "re-investigate what we just investigated" anti-pattern
- Clear handoff: research outputs findings → improve extracts and uses them
