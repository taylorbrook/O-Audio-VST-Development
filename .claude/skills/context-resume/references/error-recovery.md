# Error Recovery and Advanced Features

**Context:** This file is part of the context-resume skill.
**Invoked by:** When errors occur or advanced resume features needed
**Purpose:** Handle missing/corrupt handoffs, stale context, and advanced resume scenarios

---

## Error 1: No Handoff Found

**Scenario:** `.continue-here.md` doesn't exist in any of the 2 locations.

**Response:**

```
No resumable work found for [PluginName].

Let me check the plugin status...
```

**Check PLUGINS.md:**

```bash
grep -A 10 "^### $PLUGIN_NAME$" PLUGINS.md
```

**Check git log:**

```bash
git log --oneline plugins/$PLUGIN_NAME/ -5
```

**Present findings:**

```
Last commits:
- abc1234 feat: TapeDelay Stage 2.2 - modulation system (2 hours ago)
- def5678 feat: TapeDelay Stage 2.1 - core delay (4 hours ago)

Best guess: Stage 2 was in progress but handoff file was deleted or lost.

How would you like to proceed?

1. Start from Stage 3 (GUI) - assume Stage 2 complete
2. Review Stage 2 code to verify completion
3. Restart Stage 2 from scratch (re-implement DSP)
4. Check if Stage 3 validation is more appropriate
5. Other

Choose (1-5): _
```

**If plugin NOT found in PLUGINS.md:**

```
[PluginName] not found in PLUGINS.md.

This plugin may not exist yet or was never registered.

Options:
1. Create new plugin with this name (/dream [PluginName])
2. List all existing plugins
3. Check if directory exists but not registered
4. Other

Choose (1-4): _
```

## Error 2: Corrupted Handoff File

**Scenario:** File exists but YAML is invalid or structure is wrong.

**Detection:**

```bash
# Try to parse YAML frontmatter
# If parse fails → corrupted
```

**Response:**

```
Handoff file exists but has invalid format.

Error: [Specific YAML parse error]

Attempting to reconstruct state from git history...
```

**Parse git log to infer state:**

Look for stage markers in commit messages, infer likely stage.

**Present reconstruction:**

```
Reconstructed state from git commits:

Inferred stage: Stage 2 (based on commit messages)
Last commit: feat: TapeDelay Stage 2.2 - modulation system
Commit date: 2 hours ago

Warning: This is a best guess. Handoff file was corrupted.

Options:
1. Proceed with Stage 3 (next stage after inferred position)
2. Review code to manually verify current state
3. Recreate handoff file manually (I'll help)
4. Other

Choose (1-4): _
```

**If user chooses option 3 (recreate):**

Analyze code, check build status, read contracts, create new handoff file based on findings and user confirmation.

## Error 3: Stale Handoff (Old Timestamp)

**Scenario:** Handoff file exists but `last_updated` is > 2 weeks ago.

**Warning:**

```
Resuming [PluginName]...

⚠️ Note: This handoff is 3 weeks old.
Context may be stale. Recent git activity detected.

Last handoff: 2025-10-20 (3 weeks ago)
Recent commit: 2025-11-08 (2 days ago)

The code may have changed since this handoff was written.

Options:
1. Use handoff anyway (may be outdated)
2. Check git log first (verify recent changes)
3. Review code to assess current state
4. Recreate handoff from current code state

Choose (1-4): _
```

## Advanced Feature 1: Time-Travel Resume

Support resuming from older handoff file if multiple backups exist:

```bash
# If .continue-here.md.backup files exist
find plugins/$PLUGIN_NAME -name ".continue-here.md*"
```

**Present options:**

```
Multiple handoff files found (including backups):

1. Current: Stage 3.2 (2 hours ago)
2. Backup: Stage 3.1 (1 day ago)
3. Backup: Stage 2 complete (2 days ago)

Which version would you like to resume from?

Choose (1-3): _
```

## Advanced Feature 2: Diff Between Sessions

Show what changed since last session:

```bash
# Get timestamp from handoff
last_updated="2025-11-10 12:00:00"

# Git log since that time
git log --since="$last_updated" plugins/$PLUGIN_NAME/ --oneline

# Git diff since that time
git diff "HEAD@{$last_updated}" HEAD -- plugins/$PLUGIN_NAME/
```

**Present summary:**

```
Changes since last session (2 hours ago):

Commits:
- abc1234 feat: Added modulation LFO (1 hour ago)
- def5678 fix: Fixed feedback calculation (30 min ago)

Files changed:
- PluginProcessor.cpp: +45 lines, -12 lines
- PluginEditor.h: +23 lines, -5 lines

Continue from current state? (y/n): _
```

## Advanced Feature 3: Cross-Session Notes

Display notes from handoff file markdown:

```markdown
## Notes for Next Session

- Remember to test wow/flutter at extreme values
- Check if feedback parameter needs nonlinear scaling
- UI mockup v3 might need iteration before implementing GUI
```

Display these prominently in summary.

---

**Return to:** Main context-resume orchestration in `SKILL.md`
