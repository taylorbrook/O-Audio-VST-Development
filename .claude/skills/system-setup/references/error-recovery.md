# Error Recovery Procedures

This document describes error handling and recovery procedures for system-setup dependency validation.

## Automated Installation Failures

If any automated installation fails:

1. **Capture error output**
2. **Display error message**
3. **Fall back to guided mode**

**Example**:
```
✗ Failed to install CMake via Homebrew

Error: Homebrew is not installed and automatic installation failed.

Falling back to manual instructions...

[Display manual CMake installation steps from references/platform-requirements.md]

Press Enter to continue with manual installation...
```

**Recovery flow**:
- Display error with specific failure reason
- Automatically switch to guided mode for this dependency
- Show manual instructions from platform-requirements.md
- Wait for user to complete installation
- Re-run detection and validation

---

## Missing Critical Dependencies

If critical dependencies are missing and user skips them:

**Warning display**:
```
⚠ Warning: Critical dependencies are missing

The following required dependencies were not installed:
- CMake 3.15+ (required for building)
- JUCE 8.0.0+ (required for plugin development)

You will not be able to build plugins until these are installed.

Would you like to:
1. Go back and install missing dependencies
2. Save current configuration anyway (not recommended)
3. Exit without saving

Choose (1-3): _
```

**Recovery actions**:
- Option 1: Return to first missing dependency, retry validation
- Option 2: Save config with `"missing_critical": true` flag
- Option 3: Exit skill without creating `.claude/system-config.json`

**Critical dependencies** (cannot skip):
- Python 3.8+ (required for build scripts)
- Build Tools (required for C++ compilation)
- CMake 3.15+ (required for JUCE build system)
- JUCE 8.0.0+ (required for plugin framework)

**Optional dependencies** (can skip):
- pluginval (validation will be limited but builds still work)

---

## Version Too Old

If a dependency is found but version is too old:

**Example (CMake)**:
```
✗ CMake 3.10.2 found, but version 3.15+ is required

Would you like to:
1. Update CMake via Homebrew (automated)
2. Show manual update instructions
3. Continue anyway (build may fail)

Choose (1-3): _
```

**Recovery actions**:
- Option 1: Run `brew upgrade [dependency]`, verify version, save to config
- Option 2: Display manual upgrade instructions from platform-requirements.md
- Option 3: Save current version with `"version_warning": true` flag

**Version checking is strict for**:
- Python: Must be >= 3.8
- CMake: Must be >= 3.15
- JUCE: Must be >= 8.0.0

---

## Permission Errors

If installation fails due to permissions:

**Example (pluginval)**:
```
✗ Failed to install pluginval: Permission denied

This usually means you need sudo access.

Would you like to:
1. Retry with sudo (will prompt for password)
2. Install to ~/bin instead (no sudo required)
3. Show manual instructions

Choose (1-3): _
```

**Recovery actions**:

**Option 1** - Retry with sudo:
```bash
1. Confirm sudo usage with user
2. Run: sudo mv /tmp/pluginval.app/Contents/MacOS/pluginval /usr/local/bin/
3. Run: sudo chmod +x /usr/local/bin/pluginval
4. If success: Save path as "/usr/local/bin/pluginval"
5. If failure: Fall back to option 2 or 3
```

**Option 2** - Install to user directory:
```bash
1. Create: mkdir -p ~/bin
2. Move: mv /tmp/pluginval.app/Contents/MacOS/pluginval ~/bin/
3. Set permissions: chmod +x ~/bin/pluginval
4. Save path as "~/bin/pluginval" (expand ~ to full path in config)
5. Note: User may need to add ~/bin to PATH
```

**Option 3** - Manual instructions:
- Display instructions from platform-requirements.md
- Wait for user to complete installation
- Re-run detection

**Common permission issues**:
- `/usr/local/bin` requires sudo on some systems
- Homebrew installation may require sudo for first-time setup
- Xcode CLI Tools always require GUI interaction (no workaround)

---

## Homebrew Installation Failures

Homebrew is required for automated installation on macOS. If Homebrew installation fails:

**Failure scenario**:
```
✗ Failed to install Homebrew

Error: [specific error from Homebrew installer]

Homebrew is required for automated dependency installation.

Would you like to:
1. Try installing Homebrew manually, then continue
2. Switch to guided mode (show manual install instructions for all dependencies)
3. Exit setup

Choose (1-3): _
```

**Recovery actions**:
- Option 1: Show Homebrew installation instructions, wait for completion, retry
- Option 2: Switch MODE to "guided" for all remaining dependencies
- Option 3: Exit without saving config

---

## JUCE Custom Path Validation Failures

When user provides custom JUCE path, validation may fail. Retry logic:

**Attempt tracking**:
```
Attempt 1/3: User enters path → Validation fails → Offer retry
Attempt 2/3: User enters path → Validation fails → Offer retry
Attempt 3/3: User enters path → Validation fails → Force fallback
```

**After 3 failed attempts**:
```
Maximum attempts reached for custom JUCE path.

Falling back to automated installation...

Installing JUCE 8.0.3 to ~/JUCE
[... proceed with automated install ...]
```

**Validation failure reasons**:
- Path doesn't exist
- Path exists but missing `modules/juce_core/juce_core.h`
- Path has JUCE but version < 8.0.0
- Path is not readable (permission issue)

---

## Test Mode Error Handling

When running with `--test=SCENARIO`, errors are simulated:

**Test mode error display**:
```
[TEST MODE: missing-cmake]

✗ CMake not found (simulated)

[... normal error recovery menu ...]
```

**Key difference**: No actual system changes are made, but error flows are tested.

---

## General Error Recovery Principles

1. **Never fail silently** - Always display error with specific reason
2. **Always offer recovery path** - Automated → Manual → Skip
3. **Preserve user progress** - Don't restart entire setup on single failure
4. **Validate after recovery** - Re-run detection after manual installation
5. **Save partial config** - Even incomplete config is useful for troubleshooting
6. **Log errors** - Display full error output for debugging

---

## Integration with Main Workflow

Error recovery is integrated at each dependency validation decision gate. See [validation-workflow.md](validation-workflow.md) for where error recovery procedures are invoked.
