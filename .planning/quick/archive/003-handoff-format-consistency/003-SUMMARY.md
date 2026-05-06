# Quick Task 003: Summary

## Completed

Updated handoff presentation formats across 3 skill files to use consistent GSD-style formatting:

### Files Modified

1. **`.claude/skills/context-resume/references/context-parsing.md`**
   - Updated Step 3 "Build Summary" section
   - Added `━━━` separator lines for visual structure
   - Added explicit two-step format: `/clear` + command
   - Added "Also available" alternatives section
   - Emphasized CRITICAL: do not auto-proceed

2. **`.claude/skills/plugin-workflow/SKILL.md`**
   - Updated Phase Completion Menu (Manual Mode)
   - Added `━━━ PHASE COMPLETE ━━━` header
   - Added proper separators and boxed format
   - Explicit Step 1/Step 2 with `/clear` instruction

3. **`.claude/skills/plugin-phases/SKILL.md`**
   - Updated 5 handoff menus:
     - discuss → research (Step 8)
     - research → plan (Step 7)
     - plan → execute (Step 7)
     - execute → verify (Step 8)
     - verify → next stage (Step 9)
   - All now use `━━━ PHASE/STAGE COMPLETE ━━━` header
   - All include explicit `/clear` + command two-step format
   - All include "Also available" alternatives

## Format Standard

All handoffs now follow this structure:

```
━━━ [PHASE/STAGE] COMPLETE ━━━

**[PluginName]** — [context description]

[Status/artifacts info]

━━━━━━━━━━━━━━━━━━━━━━━━━━━

## ▶ Next Up

**[Next Phase/Stage]** — [description]

**Step 1:** `/clear` — fresh context window
**Step 2:** `/[command] [PluginName]`

━━━━━━━━━━━━━━━━━━━━━━━━━━━

**Also available:**

- Alternative commands
- Other options

━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## Success Criteria

- [x] All stage completion messages use `━━━` separators
- [x] All "Next Up" sections use explicit Step 1/Step 2 format
- [x] Plugin name always included in commands
- [x] No prose-style "Next:" instructions remain in modified sections
