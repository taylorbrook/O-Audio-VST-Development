# Breaking Change Detection Guide

## Detection Criteria

A change is **breaking** if it makes the plugin incompatible with:
- Existing automation data
- Saved presets/state
- Host integration
- User workflows

## Specific Indicators

### 1. Parameter Changes
**Breaking:**
- Parameter ID renamed (automation breaks)
- Parameter range changed (saved values invalid)
- Parameter removed (automation breaks)
- Parameter type changed (e.g., float → choice)

**Non-breaking:**
- Parameter display name changed
- Parameter default value changed
- Parameter added (new, doesn't affect existing)

### 2. State Format Changes
**Breaking:**
- Preset file format changed (old presets won't load)
- State version incompatible (getStateInformation/setStateInformation)
- XML structure changed (if using XmlElement)

**Non-breaking:**
- Added new state fields (backward compatible)
- Improved state validation

### 3. API Changes
**Breaking:**
- Public method signature changed
- Public method removed
- Class renamed
- Namespace changed

**Non-breaking:**
- Private method changes
- Implementation details
- Internal refactoring

### 4. Behavior Changes
**Breaking:**
- DSP algorithm fundamentally different (presets sound different)
- Feature removed
- Default behavior changed (affects existing projects)

**Non-breaking:**
- Bug fix (makes behavior correct)
- Performance improvement (same output)
- UI improvement (same functionality)

## Detection Process

When analyzing changes, check:

1. **Read diff:** Look for parameter ID changes in source
2. **Check APVTS:** Any createParameterLayout() modifications?
3. **Check state:** Any getStateInformation() changes?
4. **Check API:** Any public method changes in PluginProcessor.h?
5. **Check DSP:** Any algorithm changes affecting output?

## Warning Template

If breaking changes detected, warn user:

```
⚠️  BREAKING CHANGES DETECTED

The following changes are incompatible with existing projects:

- [Specific change 1]: [Impact on users]
- [Specific change 2]: [Impact on users]

This will require a MAJOR version bump (X.0.0).

Users will need to:
1. [Migration step 1]
2. [Migration step 2]

Proceed with MAJOR version bump? (y/n): _
```

## Common False Positives

**Not breaking:**
- Adding new parameters (backward compatible)
- Changing private implementation
- Refactoring without API changes
- Fixing bugs (restoring correct behavior)
- Improving performance (same output)

## Edge Cases

**Debatable:**
- Bug fix that changes output (is current behavior "correct"?)
- Adding validation (may reject previously accepted input)
- Tightening parameter ranges (may clip existing values)

**Resolution:** Treat as breaking if users rely on current behavior, even if technically a bug.
