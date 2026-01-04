# Stage 1 Blocked: Missing Contracts

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
✗ BLOCKED: Cannot proceed to Stage 1
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Missing implementation contracts:

Required contracts:
✓ creative-brief.md - exists
[✓/✗] parameter-spec.md - [exists/MISSING (required)]
[✓/✗] architecture.md - [exists/MISSING (required)]

## Why Blocked

Stage 1 planning requires complete specifications to prevent implementation drift. These contracts are the single source of truth.

## How to Unblock

1. **parameter-spec.md**: Complete ui-mockup two-phase workflow
   - Run: `/dream ${PLUGIN_NAME}`
   - Choose: "Create UI mockup"
   - Design UI and finalize (Phase 4.5)
   - Finalization generates parameter-spec.md

2. **architecture.md**: Create DSP architecture specification
   - Run Stage 0 (Research) to generate architecture.md
   - Document DSP components and processing chain
   - Map parameters to DSP components
   - Save to plugins/${PLUGIN_NAME}/.ideas/architecture.md

Once both contracts exist, Stage 1 will proceed.
