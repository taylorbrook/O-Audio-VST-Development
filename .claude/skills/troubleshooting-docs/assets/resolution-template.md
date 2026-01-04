---
plugin: [PluginName or "JUCE" for generic]
date: [YYYY-MM-DD]
problem_type: [build_error|runtime_error|api_misuse|validation_failure|ui_layout|dsp_issue|state_management|performance|thread_violation]
component: [cmake|juce_processor|juce_editor|juce_dsp|juce_gui_basics|juce_audio_utils|apvts|pluginval|xcode|system|webview]
symptoms:
  - [Observable symptom 1 - specific error message or behavior]
  - [Observable symptom 2 - what user actually saw/experienced]
root_cause: [missing_framework|missing_module|wrong_api|thread_violation|missing_constraint|state_sync|memory_issue|config_error|version_incompatibility|logic_error|event_timing|url_protocol]
juce_version: [X.Y.Z - optional]
resolution_type: [code_fix|config_change|environment_setup|api_migration]
severity: [critical|moderate|minor]
tags: [keyword1, keyword2, keyword3]
---

# Troubleshooting: [Clear Problem Title]

## Problem
[1-2 sentence clear description of the issue and what the user experienced]

## Environment
- Plugin: [Name or "JUCE generic"]
- JUCE Version: [version if applicable]
- Affected: [Component, file, stage - e.g., "PluginEditor UI, Stage 3", "DSP processBlock, Stage 2"]
- Date: [YYYY-MM-DD when this was solved]

## Symptoms
- [Observable symptom 1 - what the user saw/experienced]
- [Observable symptom 2 - error messages, visual issues, unexpected behavior]
- [Continue as needed - be specific]

## What Didn't Work

**Attempted Solution 1:** [Description of what was tried]
- **Why it failed:** [Technical reason this didn't solve the problem]

**Attempted Solution 2:** [Description of second attempt]
- **Why it failed:** [Technical reason]

[Continue for all significant attempts that DIDN'T work]

[If nothing else was attempted first, write:]
**Direct solution:** The problem was identified and fixed on the first attempt.

## Solution

[The actual fix that worked - provide specific details]

**Code changes** (if applicable):
```cpp
// Before (broken):
[Show the problematic code]

// After (fixed):
[Show the corrected code with explanation]
```

**Configuration changes** (if applicable):
```cmake
# CMakeLists.txt change:
[Show what was changed]
```

**Commands run** (if applicable):
```bash
# Steps taken to fix:
[Commands or actions]
```

## Why This Works

[Technical explanation of:]
1. What was the ROOT CAUSE of the problem?
2. Why does the solution address this root cause?
3. What was the underlying issue (API misuse, configuration error, JUCE version issue, etc.)?

[Be detailed enough that future developers understand the "why", not just the "what"]

## Prevention

[How to avoid this problem in future plugin development:]
- [Specific coding practice, check, or pattern to follow]
- [What to watch out for]
- [How to catch this early]

## Related Issues

[If any similar problems exist in troubleshooting/, link to them:]
- See also: [another-related-issue.md](../category/another-related-issue.md)
- Similar to: [related-problem.md](../category/related-problem.md)

[If no related issues, write:]
No related issues documented yet.
