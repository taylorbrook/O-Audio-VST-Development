# Dependency Validation Workflow

This document describes the validation pattern used for all system dependencies.

## General Validation Algorithm

For each dependency, follow this pattern:

1. **Detection**: Run `system-check.sh --check-[dependency]`
2. **Parse JSON response**: Extract `found`, `version`, `path`, `meets_minimum`
3. **Validation**:
   - If found AND version OK → Save to config, continue to next dependency
   - If found AND version too old → Offer upgrade menu (automated/manual/skip)
   - If not found → Offer install menu based on MODE

4. **Installation Decision Gate** (if not found):
   - **Automated mode**: Present menu (install automatically / show manual / skip)
   - **Guided mode**: Show manual instructions immediately
   - **Check-only mode**: Report as missing, continue

5. **Post-installation**: Re-run detection to verify, save to config if successful

6. **Proceed**: Move to next dependency in sequence

## Dependency-Specific Variations

| Dependency | Order | Required | Min Version | Detection Command | Auto-Install Method | Special Handling |
|------------|-------|----------|-------------|-------------------|---------------------|------------------|
| Python | 1 | Yes | 3.8 | `system-check.sh --check-python` | `brew install python3` | None |
| Build Tools | 2 | Yes | N/A | `system-check.sh --check-xcode` (macOS) | `xcode-select --install` | Launches GUI - must wait for user completion |
| CMake | 3 | Yes | 3.15 | `system-check.sh --check-cmake` | `brew install cmake` | None |
| JUCE | 4 | Yes | 8.0.0 | `system-check.sh --check-juce` | Download from GitHub, extract to ~/JUCE | Custom path option with 3-attempt retry loop |
| pluginval | 5 | No | N/A | `system-check.sh --check-pluginval` | Download from GitHub, install to /usr/local/bin or ~/bin | Permission error recovery (sudo vs ~/bin fallback) |

## Standard Detection Response Format

All `system-check.sh` commands return JSON:

**When found**:
```json
{
  "found": true,
  "version": "3.11.5",
  "path": "/usr/local/bin/python3",
  "meets_minimum": true
}
```

**When not found**:
```json
{
  "found": false,
  "checked_paths": ["/usr/local/bin/python3", "/usr/bin/python3"]
}
```

## Installation Menus by Mode

### Automated Mode Menu Template

When dependency not found in automated mode:

```
✗ [Dependency] [version] not found
[Brief explanation of what this dependency does]

1. Yes, install automatically
2. No, show me manual instructions
3. Skip (build may fail)

Choose (1-3): _
```

### Guided Mode Behavior

No menu - immediately display manual instructions from `platform-requirements.md` and wait for user to complete installation.

### Check-Only Mode Behavior

No menu - report as missing and continue to next dependency.

## Version Upgrade Menu Template

When dependency found but version too old:

```
✗ [Dependency] [current_version] found, but version [min_version]+ is required

1. Update via Homebrew (automated)
2. Manual update instructions
3. Continue anyway (build may fail)

Choose (1-3): _
```

## JUCE Custom Path Validation Loop

JUCE has unique handling for custom installation paths:

**Retry logic** (max 3 attempts):
```
Attempt 1: Prompt for path → Validate → If invalid, offer retry
Attempt 2: Prompt for path → Validate → If invalid, offer retry
Attempt 3: Prompt for path → Validate → If invalid, force fallback to manual instructions
```

**Validation check**: Path must contain `modules/juce_core/juce_core.h`

**Menu after failed custom path**:
```
✗ Invalid JUCE installation at provided path
The path must contain JUCE modules (modules/juce_core/juce_core.h).

1. Try a different path
2. Install JUCE to default location (~/JUCE)
3. Show manual installation instructions

Choose (1-3): _
```

## pluginval Permission Error Recovery

pluginval installation may fail due to permissions on `/usr/local/bin`:

**Error recovery menu**:
```
✗ Failed to install to /usr/local/bin (permission denied)

1. Retry with sudo (will prompt for password)
2. Install to ~/bin instead (no sudo required)
3. Show manual instructions

Choose (1-3): _
```

**Recovery actions**:
- Option 1: Re-run install with sudo confirmation
- Option 2: Create `~/bin`, install there, save alternate path to config
- Option 3: Display manual instructions from `platform-requirements.md`

## Automated Installation Sequences

### Python (via Homebrew)
```bash
1. Check if Homebrew installed: which brew
2. If not installed: Run Homebrew installer with user confirmation
3. Run: brew install python3
4. Verify: python3 --version
5. If success: Save to config, continue to Build Tools
6. If failure: Fall back to manual instructions
```

### Xcode Command Line Tools
```bash
1. Run: xcode-select --install (launches GUI dialog)
2. Display: "⏳ Please complete the Xcode installation dialog."
3. Display: "Press Enter when installation is complete..."
4. WAIT for user input (GUI installation is manual)
5. Verify: xcode-select -p
6. If success: Save to config, continue to CMake
7. If failure: Fall back to manual instructions
```

### CMake (via Homebrew)
```bash
1. Check if Homebrew installed: which brew
2. If not installed: Run Homebrew installer with user confirmation
3. Run: brew install cmake
4. Verify: cmake --version
5. If success: Save to config, continue to JUCE
6. If failure: Fall back to manual instructions
```

### JUCE (download and extract)
```bash
1. Display: "Installing JUCE 8.0.3 to ~/JUCE..."
2. Download: curl -L -o ~/JUCE.zip "https://github.com/juce-framework/JUCE/releases/download/8.0.3/juce-8.0.3-osx.zip"
3. Extract: unzip -q ~/JUCE.zip && mv ~/juce-8.0.3-osx ~/JUCE
4. Cleanup: rm ~/JUCE.zip
5. Verify: system-check.sh --validate-juce-path ~/JUCE
6. If success: Save to config, continue to pluginval
7. If failure: Fall back to manual instructions
```

### pluginval (download and install)
```bash
1. Download: curl -L -o /tmp/pluginval.zip "https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_macOS.zip"
2. Extract: unzip -q /tmp/pluginval.zip
3. Install: sudo mv /tmp/pluginval.app/Contents/MacOS/pluginval /usr/local/bin/
4. Permissions: sudo chmod +x /usr/local/bin/pluginval
5. Cleanup: rm -rf /tmp/pluginval.zip /tmp/pluginval.app
6. Verify: pluginval --version
7. If permission error: Trigger error recovery menu
8. If success: Save to config, continue to config persistence
9. If failure: Fall back to manual instructions
```

## State Variables

These variables persist throughout the entire setup session:

| Variable | Set When | Scope | Values |
|----------|----------|-------|--------|
| `MODE` | User selects mode at skill entry | All dependencies | `"automated"`, `"guided"`, `"check-only"` |
| `TEST_MODE` | User provides `--test=SCENARIO` argument | All system-check.sh calls | `SCENARIO` name or empty |

## Progress Tracking

Display progress indicator at each dependency:

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Progress: Step 1 of 5 - Python
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

Increment step number for each dependency validation.
