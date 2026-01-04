---
name: setup
description: Validate and configure system dependencies for the Plugin Freedom System
---

# /setup

Validates system dependencies required for JUCE plugin development and configures the environment.

## Behavior

When user runs `/setup` or `/setup --test=SCENARIO`, invoke the system-setup skill.

**Test Mode:**
If user provides `--test=SCENARIO`, pass the scenario to the skill.
Test mode uses mock data and makes no system changes. See [test-scenarios.md](../skills/system-setup/assets/test-scenarios.md) for available scenarios.

The skill will:
1. Detect current platform (macOS, Linux, Windows)
2. Check for required dependencies (Python, build tools, CMake, JUCE, pluginval)
3. Offer automated installation where possible or guide manual setup
4. Validate all installations are functional
5. Save configuration to `.claude/system-config.json`
6. Generate system report showing what was validated

## Preconditions

<preconditions>
  <required>None - this is the first command new users should run</required>

  <rationale>
    This command validates and installs system dependencies, so it cannot have dependency preconditions.
    Should be run BEFORE any plugin development commands (/dream, /plan, /implement).
  </rationale>
</preconditions>

## Output

Creates `.claude/system-config.json` with validated dependency paths:
```json
{
  "platform": "darwin",
  "juce_path": "/Users/username/JUCE",
  "cmake_path": "/usr/local/bin/cmake",
  "pluginval_path": "/usr/local/bin/pluginval",
  "python_version": "3.11.5",
  "validated_at": "2025-11-11T10:30:00Z"
}
```

This file is gitignored and used by build scripts to locate dependencies.
