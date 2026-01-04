# Adaptive Questioning Strategy

## Core Principle: Extract First, Then Fill Gaps

The adaptive questioning system is designed to minimize user effort while maximizing useful context. This is achieved through a two-phase approach:

### Phase 1: Free-Form Extraction
Allow users to provide information in whatever format is natural to them:
- Detailed written descriptions
- Short one-liners
- Bulleted lists
- Stream-of-consciousness thoughts
- Reference examples ("like Plugin X but with Y")

**Key insight:** Users often know what they want but may not know which details matter for implementation. Let them dump their thoughts, then parse systematically.

### Phase 2: Gap-Driven Questioning
After extracting all provided information:
1. Map user input to the tier system (Tier 1: Critical → Tier 4: Never Ask)
2. Identify which tiers have gaps
3. Prioritize Tier 1-2 gaps for questions
4. Generate exactly 4 questions per batch targeting highest-priority gaps
5. Present decision gate: "Ready to finalize?" vs "Ask more questions"
6. Repeat until user chooses to finalize

## Why This Works

### Avoids Interrogation Fatigue
Instead of asking 20 questions upfront, adaptive questioning:
- Lets users provide as much or as little detail as they want initially
- Only asks about what's actually missing
- Allows users to bail out early ("Yes - I'm ready to finalize") if they're satisfied

### Respects User Knowledge Levels
Different users have different levels of clarity:
- **Beginners:** Vague ideas → Need more Tier 1 foundational questions
- **Experts:** Detailed specs → Jump straight to Tier 2-3 refinement questions
- **Everyone:** Decision gate after each batch lets them control depth

### Prevents Scope Creep
By explicitly categorizing Tier 4 (Never Ask UI Details), the system:
- Keeps ideation focused on concept and functionality
- Defers visual design to the mockup phase
- Avoids bikeshedding over pixel dimensions during brainstorming

## Continuous Iteration Until Satisfaction

The decision gate after each question batch creates a natural checkpoint:
```
Ready to finalize the creative brief?
1. Yes - I'm ready to finalize
2. No - Ask me more questions
3. Let me add more thoughts
```

This allows:
- **Option 1:** Early exit if user feels they've provided enough context
- **Option 2:** System-driven refinement (Claude asks next 4 targeted questions)
- **Option 3:** User-driven refinement (user adds free-form thoughts, then system re-analyzes gaps)

**Result:** Creative brief contains exactly as much detail as the user wants to provide, no more, no less.

## Implementation Notes

### Gap Analysis Algorithm
1. Parse all previous user responses (Phase 1 free-form + all question batch answers)
2. Check Tier 1 coverage:
   - Type, Core concept, Unique value, Target user
   - If any missing → Prioritize for next batch
3. Check Tier 2 coverage:
   - Parameters, Audio routing, Presets, DSP approach
   - Fill remaining question slots with these gaps
4. Check Tier 3 coverage:
   - Workflow, Visual style, Advanced features
   - Only ask if Tiers 1-2 complete
5. Never ask Tier 4 (UI implementation details)

### Question Batch Composition
- Always generate exactly 4 questions (not 3, not 5)
- Use AskUserQuestion tool with options
- Balance question types (closed options vs open "Other" text input)
- Make each question independent (can be answered in any order)

### State Management
- Track all extracted information across conversation
- Never re-ask questions about known information
- Merge free-form additions (Option 3) with existing context before re-analyzing gaps
