# Execution Notes for Claude

Implementation notes, critical requirements, and anti-patterns for system-setup skill.

## Critical Requirements

When executing system-setup skill, you MUST:

1. **Check before installing** - NEVER install if dependency already exists and meets version requirement
2. **Validate versions** - Don't assume installed version meets minimum requirement
3. **Test functionality** - Verify executables actually run, not just that files exist
4. **Get confirmation** - Always get user confirmation before automated installation
5. **Provide fallback** - If automation fails, fall back to guided mode with manual instructions
6. **Use absolute paths** - Expand `~`, validate existence before saving to config
7. **Respect MODE** - Honor user's choice (automated/guided/check-only) throughout entire session
8. **Append test mode** - Pass `--test=$SCENARIO` to ALL system-check.sh invocations if TEST_MODE set

## Anti-Patterns to Avoid

**NEVER**:
- Install without checking if already present
- Assume version meets minimum without verification
- Proceed when critical dependencies are missing
- Use relative paths in configuration file
- Skip testing if executables actually run
- Auto-proceed without user confirmation at decision gates
- Use AskUserQuestion tool instead of inline numbered menus
- Forget to append test mode to system-check.sh calls
- Change MODE variable after initial selection (MODE must persist for Python, Build Tools, CMake, JUCE, and pluginval throughout entire session. If user requests mode change, restart skill with new mode from beginning.)

## Mode-Specific Behavior

### Automated Mode
- Attempt installation with user confirmation
- Fall back to guided mode if automation fails
- Present installation menus for missing dependencies
- Wait for user choice at each decision gate

### Guided Mode
- NEVER attempt automated installation
- Display manual instructions from references/ immediately
- Wait for user to complete manual steps
- Verify installation after user confirms completion

### Check-Only Mode
- NEVER offer installation (automated or guided)
- Report each dependency status (found/missing/wrong-version)
- Continue through all dependencies regardless of failures
- Generate report at end showing what needs attention

## Path Handling

**All paths in config must be absolute**:
- Convert `~` to actual home directory (`/Users/username`)
- Validate paths exist before saving to config
- Check JUCE path contains expected files (`modules/juce_core/juce_core.h`)

**Example**:
```json
{
  "juce_path": "/Users/lex/JUCE",  // ✅ Absolute
  "python_path": "/usr/local/bin/python3"  // ✅ Absolute
}
```

**NOT**:
```json
{
  "juce_path": "~/JUCE",  // ❌ Relative with ~
  "python_path": "python3"  // ❌ No path
}
```

## macOS-Specific Notes

1. **Xcode CLI Tools installation requires GUI interaction**
   - Running `xcode-select --install` launches a system dialog
   - Claude cannot complete this - must wait for user to click through GUI
   - Display message: "⏳ Please complete the Xcode installation dialog."
   - Wait for user to press Enter when done

2. **Homebrew installation may require user interaction**
   - Homebrew installer may prompt for sudo password
   - Some systems require accepting Xcode license
   - May need to confirm installation to /usr/local

3. **Some installations need sudo**
   - Installing to `/usr/local/bin` typically requires sudo
   - Always offer alternative: install to `~/bin` instead
   - Get permission before running sudo commands

4. **Permission model**
   - `/usr/local/bin` may need sudo for writing
   - `~/bin` never needs sudo (user-owned)
   - Homebrew installations to `/opt/homebrew` (Apple Silicon) don't need sudo

## State Management

**Variables that persist across entire setup session**:

| Variable | Initialized | Used | Example Values |
|----------|-------------|------|----------------|
| `MODE` | At skill entry (user menu choice) | All dependency validations | `"automated"`, `"guided"`, `"check-only"` |
| `TEST_MODE` | At skill entry (from `--test=` arg) | All system-check.sh calls | `"missing-cmake"`, `"old-python"`, or empty |

**These variables MUST persist**:
- MODE determines behavior for Python, Build Tools, CMake, JUCE, and pluginval
- Changing MODE mid-session would create inconsistent UX
- TEST_MODE must pass to every system-check.sh invocation

## Verification Best Practices

**After each installation**:
1. Re-run detection command
2. Verify version meets minimum requirement
3. Test executable actually runs (not just file exists)
4. Save to config only if all verifications pass

**Example verification sequence**:
```bash
# After installing Python
python3 --version  # Verify it runs
# Parse version, compare to 3.8 minimum
# If passes: save to config
# If fails: report error, offer recovery
```

## Menu Display Format

**Always use inline numbered menus** (NOT AskUserQuestion tool):

```
[Context about current state]

Would you like to:
1. [Primary option] (recommended)
2. [Secondary option]
3. [Alternative option]

Choose (1-3): _
```

**After displaying menu**: WAIT for user input, do NOT auto-proceed.

## Configuration File Format

`.claude/system-config.json` structure:

```json
{
  "platform": "darwin",
  "platform_version": "14.0",
  "arch": "arm64",
  "python_path": "/usr/local/bin/python3",
  "python_version": "3.11.5",
  "xcode_path": "/Library/Developer/CommandLineTools",
  "cmake_path": "/usr/local/bin/cmake",
  "cmake_version": "3.27.4",
  "juce_path": "/Users/lex/JUCE",
  "juce_version": "8.0.3",
  "pluginval_path": "/usr/local/bin/pluginval",
  "pluginval_version": "1.0.3",
  "validated_at": "2025-11-12T18:30:00Z"
}
```

**Optional fields** (if dependency skipped or has warnings):
```json
{
  "python_version": "3.7.1",
  "python_version_warning": true,  // Version < 3.8
  "pluginval_path": null,
  "pluginval_skipped": true  // User chose to skip
}
```

## Common Pitfalls

1. **Installing without checking first**
   - Always run detection before offering installation
   - Found + correct version = skip installation

2. **Not validating versions**
   - Just because `python3` exists doesn't mean it's 3.8+
   - Parse version string and compare numerically

3. **Proceeding when critical missing**
   - Python, Build Tools, CMake, JUCE are REQUIRED
   - Must warn user if attempting to skip these

4. **Using relative paths**
   - Config file must have absolute paths
   - Expand `~` before saving

5. **Not testing executables**
   - `which cmake` shows path, but does it run?
   - Always test: `cmake --version`

6. **Auto-proceeding without confirmation**
   - Every decision gate must wait for user input
   - No "smart defaults" that skip user choice

7. **Using AskUserQuestion for menus**
   - Use inline numbered lists instead
   - AskUserQuestion is for questions, not action menus

8. **Forgetting test mode**
   - If TEST_MODE set, append to ALL system-check.sh calls
   - Format: `system-check.sh --check-python --test=$TEST_MODE`

## Integration with Other Skills

**This skill is STANDALONE**:
- Does NOT update PLUGINS.md
- Does NOT create .continue-here.md
- Does NOT invoke plugin-workflow or plugin-planning
- Does NOT create or modify plugin directories

**This skill ONLY**:
- Creates/updates `.claude/system-config.json`
- Validates system dependencies
- Adds system-config.json to .gitignore
- Offers to invoke plugin-ideation after completion (via final menu)

**After setup completes**:
- Configuration stored in .claude/system-config.json
- NO state file updates needed
- NO plugin workflow initiated
- User chooses next action from final menu

## Completion Criteria

This skill is complete when:

1. ✓ Platform detected and confirmed by user
2. ✓ All 5 dependencies validated (or user explicitly skipped)
3. ✓ Configuration saved to .claude/system-config.json
4. ✓ Configuration file added to .gitignore
5. ✓ Final system report displayed
6. ✓ User selected action from final menu (exit, start new plugin, resume existing)

DO NOT auto-proceed to next action after final menu - user decides what's next.
