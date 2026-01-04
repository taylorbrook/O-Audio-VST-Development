# File Generation Protocol

This document contains the complete 8-step generation sequence for Phase 5, including detailed verification checkpoints and commit procedures.

---

## Phase 5: Generate Files

<critical_sequence enforcement="strict" allow_reordering="false">
MUST execute steps in this exact order (no parallelization):
1. Read template
2. Generate prose (requires template)
3. Write aesthetic.md (requires prose)
4. Generate metadata.json (requires aesthetic ID)
5. Generate test previews (requires aesthetic.md completion)
6. Update manifest (requires metadata.json)
7. Commit changes to git (requires all files created)
8. Present confirmation (requires git commit complete)
</critical_sequence>

<progress_tracking>
Copy this checklist to track Phase 5 progress:

```
Phase 5 Progress:
- [ ] Step 1: Read aesthetic template
- [ ] Step 2: Generate prose from context
- [ ] Step 3: Write aesthetic.md file
- [ ] Step 4: Generate metadata.json
- [ ] Step 5: Generate test previews (if selected)
- [ ] Step 6: Update manifest.json
- [ ] Step 7: Commit to git
- [ ] Step 8: Present confirmation
```

**Critical**: Execute steps sequentially. Each step requires output from previous step.
</progress_tracking>

---

## Step 1: Read Template

Read template: /Users/lexchristopherson/Developer/plugin-freedom-system/.claude/skills/ui-template-library/assets/aesthetic-template.md

---

## Step 2: Generate Prose

<prose_generation_requirements>
Transform accumulated context into descriptive prose following template structure. Balance prose descriptions with concrete examples (specific colors, sizes, values).

See ui-template-library/assets/aesthetic-template.md for section structure. ui-template-library handles interpretation and application.
</prose_generation_requirements>

---

## Step 3: Write aesthetic.md

Write aesthetic.md to: .claude/aesthetics/[aesthetic-id]/aesthetic.md

<verification>
Before proceeding to Step 4:
- File exists at expected path
- File size >1KB (indicates content written, not empty file)
- All 12 template sections present in file
</verification>

---

## Step 4: Generate metadata.json

<instructions>
Generate metadata.json with: id, name, description, created timestamp, tags, isTemplateOnly: true, testPreviews array

**Tag Inference Rules**: Extract tags from accumulated context:
- Vibe keywords (modern, vintage, minimal, bold, etc.)
- Color family (dark, light, blue, warm, cool, etc.)
- Plugin type mentions (compressor, reverb, synth, etc.)
- Control style (flat, 3D, skeuomorphic, etc.)

Example: "Dark modern blue flat controls" → tags: ["dark", "modern", "blue", "flat"]
</instructions>

<verification>
Before proceeding to Step 5:
- metadata.json exists at .claude/aesthetics/[aesthetic-id]/metadata.json
- File contains valid JSON (parseable)
- Required fields present: id, name, description, created, tags, isTemplateOnly
</verification>

---

## Step 5: Generate Test Previews

<conditional_execution>
IF test previews selected in Phase 4 THEN:
  FOR EACH selected plugin type:
    - Read spec from assets/test-plugin-specs.json
    - Invoke ui-template-library skill "apply" operation
    - Save HTML to .claude/aesthetics/[id]/test-previews/[plugin-type].html
  END FOR
END IF
</conditional_execution>

<verification>
Before proceeding to Step 6:
IF test previews were selected:
- All selected preview HTML files exist in test-previews/ subdirectory
- Each file size >2KB (indicates content, not empty)
- ui-template-library invocations returned success status
</verification>

See test-preview-protocol.md for detailed invocation protocol.

---

## Step 6: Update Manifest

Read .claude/aesthetics/manifest.json, append new aesthetic entry, write updated manifest.

<verification>
Before proceeding to Step 7:
- manifest.json exists and is valid JSON
- New aesthetic entry present in manifest array
- Entry contains all required metadata fields
</verification>

---

## Step 7: Commit to Git

<instructions>
Commit aesthetic to git using conventional format.
</instructions>

<commit_format>
**Stage all aesthetic files:**

```bash
git add .claude/aesthetics/[aesthetic-id]/
```

**Commit with conventional format:**

```bash
git commit -m "feat(aesthetics): [aesthetic-name] - new aesthetic template

Created aesthetic with [N] test previews ([preview-types])

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>"
```

**Example:**

```bash
git add .claude/aesthetics/modern-professional-001/
git commit -m "feat(aesthetics): Modern Professional - new aesthetic template

Created aesthetic with 2 test previews (simple-compressor, complex-reverb)

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>"
```

**If no test previews selected:**

```bash
git commit -m "feat(aesthetics): [aesthetic-name] - new aesthetic template

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>"
```
</commit_format>

<verification>
Before proceeding to Step 8:
- Git status shows clean working directory (all files committed)
- Commit exists in git log with expected message format
- No uncommitted changes remain
</verification>

---

## Step 8: Present Confirmation

Present confirmation with file paths and preview open commands.
