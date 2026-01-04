# Error Handling and Edge Cases

## Phase 1: Free-Form Collection

### Edge Case: User provides no information
**Scenario:** User responds with "I don't know" or very minimal description

**Handling:**
- Acknowledge the starting point
- Proceed directly to Phase 2 gap analysis
- All tiers will show as gaps, generate questions covering Tier 1 critical elements

### Edge Case: User provides complete specification
**Scenario:** User's initial description covers all 3 tiers comprehensively

**Handling:**
- Proceed through Phase 2 gap analysis (will find minimal gaps)
- In Phase 3, generate 4 clarification questions (fine details, confirmations)
- Present decision gate as usual
- User will likely choose "finalize" immediately

## Phase 2: Gap Analysis

### Error: Failed to parse user context
**Scenario:** Accumulated context is malformed or missing

**Handling:**
- Log parsing error
- Default to treating all tiers as gaps
- Proceed with comprehensive question generation

### Error: No gaps identified (all tiers covered)
**Scenario:** Context already covers all visual elements

**Handling:**
- Generate 4 confirmatory questions ("Did I understand correctly that...")
- Allow user to refine or finalize at decision gate

## Phase 3: Question Batch

### Error: AskUserQuestion tool fails
**Scenario:** Tool invocation returns error or timeout

**Handling:**
- Retry once with same questions
- If retry fails, fall back to inline text questions with markdown formatting
- Collect free-form responses and proceed to Phase 3.5

### Edge Case: User skips all questions
**Scenario:** User types "skip" in "Other" for all 4 questions

**Handling:**
- Acknowledge skipped questions
- Proceed to decision gate
- User can choose "Add more context" to provide information differently

## Phase 3.5: Decision Gate

### Edge Case: User requests option not in menu
**Scenario:** User types custom response not matching 3 options

**Handling:**
- Clarify the 3 available options
- Re-present decision gate with same format
- If user persists, interpret intent and route to closest option

### Error: Decision gate tool fails
**Scenario:** AskUserQuestion fails at critical decision point

**Handling:**
- Fall back to inline numbered list
- Wait for user to type 1, 2, or 3
- Parse response and route accordingly

## Phase 3.7: Name Aesthetic

### Edge Case: User rejects all 3 suggested names
**Scenario:** User chooses "Other" but provides no custom name

**Handling:**
- Prompt again: "What would you like to call this aesthetic?"
- If still no response, generate default name: "[primary-color]-[vibe]-aesthetic"
- Example: "blue-modern-aesthetic"

### Error: Name collision (ID already exists)
**Scenario:** Generated slug matches existing aesthetic ID in manifest.json

**Handling:**
- Append numeric suffix: "-001", "-002", etc.
- Check manifest again until unique ID found
- Maximum 999 suffixes, then error and ask user for different name

## Phase 4: Test Preview Selection

### Edge Case: User selects 0 previews
**Scenario:** User doesn't select any test plugin types

**Handling:**
- Skip Step 5 in Phase 5 (test preview generation)
- Only generate aesthetic.md and metadata.json
- Update manifest with empty testPreviews array

### Edge Case: User requests preview type not in list
**Scenario:** User types custom plugin type in "Other"

**Handling:**
- Acknowledge request but explain only 4 pre-defined types available
- Suggest closest match from available types
- User can choose from 4 types or select none

## Phase 5: Generate Files

### Error: Template file not found
**Scenario:** aesthetic-template.md missing from ui-template-library/assets/

**Handling:**
- Log error: "Template file not found at [path]"
- Abort Phase 5 and report to user
- Suggest running system verification or reinstalling ui-template-library skill

### Error: Cannot write aesthetic.md
**Scenario:** Directory doesn't exist or permissions issue

**Handling:**
- Attempt to create directory: `.claude/aesthetics/[id]/`
- If creation fails, log error and report to user
- Abort Phase 5 (cannot proceed without writing aesthetic.md)

### Error: ui-template-library skill invocation fails
**Scenario:** Skill not available or returns error for test preview

**Handling:**
- Log warning for specific plugin type
- Continue with remaining preview types (don't abort entire phase)
- Report partial success in confirmation message: "Generated 3 of 4 test previews (complex-reverb failed)"

### Error: Manifest.json not found or malformed
**Scenario:** Cannot read or parse .claude/aesthetics/manifest.json

**Handling:**
- Create new manifest.json with single entry (current aesthetic)
- Log warning that previous aesthetics may not be indexed
- Proceed with Phase 5 completion

### Error: Manifest update fails (write error)
**Scenario:** Cannot write updated manifest.json

**Handling:**
- Log error but continue (aesthetic files still created)
- Report in confirmation: "Aesthetic created but manifest not updated"
- Aesthetic will still be functional, just not in centralized index

## Phase 6: Decision Menu

### Error: Test preview files missing for Option 1
**Scenario:** User chooses "Open test previews" but files don't exist

**Handling:**
- Check for file existence before running `open` commands
- Report which previews are missing
- Open only the available previews
- Return to decision menu

### Error: PLUGINS.md not found for Option 4
**Scenario:** User chooses "Apply to existing plugin" but PLUGINS.md missing

**Handling:**
- Log error: "PLUGINS.md not found at [path]"
- Report to user: "No plugins found. Create a plugin first with /dream or /plan"
- Return to decision menu with Option 4 disabled

### Error: ui-mockup skill not available for Option 4
**Scenario:** User chooses "Apply to existing plugin" but skill can't be invoked

**Handling:**
- Log error: "ui-mockup skill not found"
- Report to user: "Cannot invoke ui-mockup. Verify skill installation."
- Return to decision menu with Option 4 disabled

## General Error Handling

### Skill-level errors

**Principle:** Fail gracefully, never abort entire workflow unless critical files can't be written.

**Logging:**
- All errors logged with timestamp, phase ID, and error message
- Errors include suggested remediation steps

**User communication:**
- Clear, non-technical explanation of what went wrong
- Specific next steps to resolve
- Always offer path forward (retry, skip, or alternative route)

**Recovery strategy:**
- Phases 1-4: Non-critical, can continue with partial information
- Phase 5: Critical for file generation, must abort if template or write fails
- Phase 6: Non-critical, can repeat or exit gracefully
