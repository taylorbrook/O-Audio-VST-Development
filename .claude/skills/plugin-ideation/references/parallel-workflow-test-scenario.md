# Parallel Workflow Test Scenario

**Purpose:** Validate two-phase parameter specification implementation.

## Test Workflow

### Phase 1: Creative Brief Creation

```
User: /dream
System: What would you like to build?

User: A simple compressor with threshold, ratio, attack, and release controls.
System: [Adaptive questioning Phase 1-4]
User: [Finalize]

System creates creative-brief.md
```

### Phase 2: Quick Parameter Capture (NEW PATH)

```
System: ✓ Creative brief complete: SimpleComp

What's next?
1. Quick params + parallel workflow (18 min faster)
2. Full UI mockup first (traditional workflow)
3. Start implementation directly
4. Research similar plugins
5. Save for later
6. Other

User: 1

System: Quick Parameter Capture (for Stage 0 planning)

Ready to capture parameters? (y/n): _
User: y

System: [Parameter ID question via AskUserQuestion]
User: threshold

System: [Type question]
User: Float

System: [Range question]
User: -60 to 0 dB

System: [Default question]
User: -20

System: [DSP Purpose question]
User: Compressor threshold level for determining when compression begins

System: Add another parameter?
User: Yes

[Repeat for ratio, attack, release]

System: ✓ Draft parameters captured: 4 parameters

Files created:
- parameter-spec-draft.md (4 parameters)

What's next?
1. Start Stage 0 research now (recommended)
2. Design UI mockup now
3. Do both in parallel
4. Other

User: 3
```

### Phase 3: Parallel Execution

**Session 1 - Stage 0 Research:**
```
User: /plan SimpleComp
System: [Checks for parameter spec]
System: ℹ Using draft parameters. Full spec needed before Stage 1.
System: [Invokes research-planning-agent with draft params]
System: [Stage 0 completes - creates architecture.md]
```

**Session 2 - UI Mockup (parallel):**
```
User: /dream SimpleComp
System: [ui-mockup skill]
System: [Phase 0-5 - design iteration]
User: [Finalize mockup]
System: [Phase 6-10 - implementation files]
System: [Phase 10 - validates against draft]

IF mismatch:
  System: ⚠️ Parameter mismatch between draft and mockup

  Draft specified but missing from mockup:
  - attack (Float, 0-100 ms)

  Resolution: [menu]

ELSE:
  System: ✓ Consistency validated
  System: [Generates parameter-spec.md from mockup + draft merge]
```

### Phase 4: Merge Point - Stage 0 Planning

```
User: /plan SimpleComp
System: [Checks preconditions]
System: ✓ parameter-spec.md exists (full spec)
System: ✓ architecture.md exists
System: [Calculates complexity, creates plan.md]
System: ✓ Planning complete
```

### Phase 5: Implementation - Stage 1 Block Test

**Test Case 1: Only draft exists (should BLOCK)**
```
# Simulate: Delete parameter-spec.md, keep draft
User: /implement SimpleComp
System: ✗ Draft parameters found, but full specification required for implementation.

Next step: Complete UI mockup workflow to generate parameter-spec.md

Run: /dream SimpleComp → option 2
```

**Test Case 2: Full spec exists (should PROCEED)**
```
# After mockup finalized
User: /implement SimpleComp
System: ✓ parameter-spec.md found
System: [Proceeds to Stage 1 via foundation-shell-agent]
```

## Validation Checklist

### Interactive Capture Works
- [ ] plugin-ideation presents parallel workflow option in Phase 8
- [ ] AskUserQuestion tool captures parameters interactively
- [ ] parameter-spec-draft.md generated with correct format
- [ ] Status updated to "💡 Ideated (Draft Params)"
- [ ] Git commit includes draft file

### Stage 0 Accepts Draft
- [ ] plugin-planning checks for draft OR full spec
- [ ] Stage 0 proceeds with draft parameters
- [ ] Warning logged: "Using draft parameters. Full spec needed before Stage 1."
- [ ] architecture.md created successfully

### Stage 0 Accepts Draft
- [ ] plugin-planning uses draft for complexity calculation
- [ ] plan.md generated with correct complexity score
- [ ] Warning logged about needing full spec

### UI Mockup Validates Consistency
- [ ] Checks for existing draft when generating full spec (Phase 10)
- [ ] Detects parameter mismatches (if IDs differ)
- [ ] Presents conflict resolution menu
- [ ] Merges or reconciles differences based on user choice
- [ ] Generates parameter-spec.md with union or corrected set

### Stage 1 Blocks Without Full Spec
- [ ] Precondition check requires parameter-spec.md
- [ ] Clear error message if only draft exists
- [ ] Guides user to complete mockup workflow
- [ ] Proceeds when full spec available

### Parallel Execution Enabled
- [ ] Can start Stage 0 immediately after draft captured
- [ ] UI mockup can proceed independently
- [ ] Both workflows merge successfully at Stage 0
- [ ] Time saved verified (parallel execution vs. sequential)

### Backward Compatibility
- [ ] Existing sequential workflow still works (option 2)
- [ ] Plugins without draft proceed normally
- [ ] No breaking changes to existing contracts
- [ ] Traditional path (mockup → full spec → Stage 0) unchanged

## Time Comparison

**Sequential Workflow (Traditional):**
```
Creative Brief (5 min)
  ↓
UI Mockup (18 min)
  ↓
parameter-spec.md generated
  ↓
Stage 0 Research (25 min)
  ↓
Stage 0 Planning (3 min)

Total: 51 minutes
```

**Parallel Workflow (New):**
```
Creative Brief (5 min)
  ↓
Quick Param Capture (2 min)
  ↓
  ├─ Stage 0 Research (25 min) ────┐
  └─ UI Mockup (18 min) ────────────┤
                                     ├→ Stage 0 Planning (3 min)
                                    ┘

Total: 33 minutes (18 min saved, 35% reduction)
```

## Expected Outcomes

**Success Criteria:**
1. Quick capture workflow integrated into plugin-ideation (Phase 8.1)
2. Stage 0 accepts either draft or full parameter spec
3. Stage 0 accepts either draft or full parameter spec
4. UI mockup validates draft consistency and merges (Phase 10)
5. Stage 1 blocks until full spec available
6. Parallel execution demonstrably works (both workflows proceed simultaneously)
7. Time savings achieved (18 minutes through parallelization)
8. No regression in sequential workflow
9. Interactive parameter capture UX is clear and efficient

**Rollback Plan:**
- Revert commit 7cc817a
- Remove draft parameter handling from plugin-planning
- Remove parallel workflow option from plugin-ideation
- Delete parameter-spec-draft-template.md
- Revert to sequential workflow only

## Notes

- Draft spec is OPTIONAL (user chooses workflow path)
- Full spec is REQUIRED for Stage 1 (implementation)
- Validation at merge point prevents mismatches
- Clear error messages guide users through process
- Both workflows remain functional (parallel is opt-in)
