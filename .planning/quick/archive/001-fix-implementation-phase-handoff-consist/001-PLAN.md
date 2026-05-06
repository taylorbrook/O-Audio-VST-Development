---
phase: quick-001
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - .claude/references/handoff-protocol.md
  - .claude/skills/plugin-workflow/SKILL.md
  - .claude/skills/plugin-phases/SKILL.md
autonomous: true

must_haves:
  truths:
    - "All handoff menus show /clear as mandatory first step"
    - "Express mode auto-advances phases within stage but stops at stage boundaries"
    - "Phase completion menus in plugin-phases match handoff protocol format"
  artifacts:
    - path: ".claude/references/handoff-protocol.md"
      provides: "Updated two-step handoff format"
      contains: "Step 1.*clear.*Step 2"
    - path: ".claude/skills/plugin-workflow/SKILL.md"
      provides: "Express mode behavior fix"
      contains: "STOP - do not auto-advance"
    - path: ".claude/skills/plugin-phases/SKILL.md"
      provides: "Phase completion menus with /clear"
      contains: "/clear"
  key_links:
    - from: ".claude/skills/plugin-phases/SKILL.md"
      to: ".claude/references/handoff-protocol.md"
      via: "format reference"
      pattern: "handoff-protocol"
---

<objective>
Fix implementation workflow handoff consistency

Purpose: Ensure all workflow transitions use a consistent two-step handoff format that makes /clear mandatory, not a footnote.

Output: Updated handoff protocol and skill files with consistent handoff menus
</objective>

<execution_context>
@/Users/taylorbrook/.claude/get-shit-done/workflows/execute-plan.md
@/Users/taylorbrook/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@.claude/references/handoff-protocol.md
@.claude/skills/plugin-workflow/SKILL.md
@.claude/skills/plugin-phases/SKILL.md
</context>

<tasks>

<task type="auto">
  <name>Task 1: Update handoff-protocol.md with two-step format</name>
  <files>.claude/references/handoff-protocol.md</files>
  <action>
Update the standard format section (lines 22-53) to show a clear two-step handoff:

Replace the current format that shows `/clear` as a footnote:
```
`/command [PluginName]`
<sub>`/clear` first → fresh context window</sub>
```

With an explicit two-step format:
```
**Step 1:** `/clear` — fresh context window
**Step 2:** `/command [PluginName]`
```

Update the anti-pattern section to include the footnote format as a BAD example.

Update the table showing handoff points to clarify this is a two-step process.
  </action>
  <verify>Read the updated file and confirm /clear appears as Step 1 in the standard format</verify>
  <done>Standard format shows mandatory two-step sequence, not footnote</done>
</task>

<task type="auto">
  <name>Task 2: Fix plugin-workflow.md stage completion and express mode</name>
  <files>.claude/skills/plugin-workflow/SKILL.md</files>
  <action>
Two changes:

1. Update Stage Completion Menu (lines 386-424) to use two-step format:
   Replace the `<sub>` footnote with explicit two-step:
   ```
   **Step 1:** `/clear`
   **Step 2:** `/implement [PluginName]`
   ```

2. Update Express Mode Output section (lines 429-456):
   - Remove line 349 behavior showing "→ Advancing to Stage 3"
   - Add explicit HANDOFF at stage boundary
   - Express mode should show stage complete, then present handoff (not auto-advance)

   Change express output to show:
   ```
   ━━━ Stage 2: DSP ━━━
     [auto] discuss → ...
     ...
     ✓ Stage 2 complete

   ---
   ## ▶ Next Up
   **Stage 3: GUI** — WebView UI integration

   **Step 1:** `/clear`
   **Step 2:** `/implement [PluginName]`
   ---
   ```

3. Add reference comment in express section pointing to handoff-protocol.md
  </action>
  <verify>Read updated file, confirm express mode section shows handoff format not auto-advance, confirm stage completion menu uses two-step format</verify>
  <done>Express mode stops at stage boundaries with proper handoff, stage completion uses two-step format</done>
</task>

<task type="auto">
  <name>Task 3: Add /clear to plugin-phases.md completion menus</name>
  <files>.claude/skills/plugin-phases/SKILL.md</files>
  <action>
Update all phase completion menus to include /clear instruction:

1. Lines 86-99 (discuss completion menu):
   Change:
   ```
   Next:
   1. /plugin-research O-IntonationPad 2-dsp (recommended)
   ```
   To:
   ```
   Next:
   **Step 1:** `/clear`
   **Step 2:** Choose one:
   - `/plugin-research O-IntonationPad 2-dsp` (recommended)
   - `/plugin-plan O-IntonationPad 2-dsp` (skip research)
   ```

2. Similarly update the research completion menu reference (line 131)

3. Similarly update the plan completion menu reference (line 165)

4. Similarly update the execute completion menu reference (line 192)

5. Update Express Mode Integration section (lines 335-352):
   - Change line 349 from "→ Advancing to Stage 3" to show HANDOFF
   - Add note: "Express mode auto-advances PHASES but presents handoff at STAGE boundaries"

6. Add reference: "See: `.claude/references/handoff-protocol.md`"
  </action>
  <verify>Read updated file, grep for "/clear" to confirm it appears in completion menu examples</verify>
  <done>All phase completion menus show /clear as mandatory first step</done>
</task>

</tasks>

<verification>
1. `grep -n "Step 1.*clear" .claude/references/handoff-protocol.md` - finds two-step format
2. `grep -n "Advancing to Stage" .claude/skills/plugin-phases/SKILL.md` - should NOT find auto-advance
3. `grep -n "/clear" .claude/skills/plugin-phases/SKILL.md` - finds /clear in completion menus
4. All three files reference handoff-protocol.md as the authoritative format
</verification>

<success_criteria>
- All handoff menus show `/clear` as explicit Step 1 (not footnote)
- Express mode documentation shows stage boundary handoffs
- plugin-phases completion menus include /clear instruction
- Consistent format across all three files
</success_criteria>

<output>
After completion, create `.planning/quick/001-fix-implementation-phase-handoff-consist/001-SUMMARY.md`
</output>
