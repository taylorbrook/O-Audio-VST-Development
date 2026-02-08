# Stage 4: Polish - Research

## Code Analysis Summary

Full code review of all 14 source files completed. Plugin is in excellent shape overall.
Stages 1-3 are verified with pluginval strictness 5 PASS.

## Issues Identified

### High Priority (Audio Artifacts)

**1. FreezeManager: No crossfade on release**
- `FreezeManager::release()` (line 29-33) sets `active = false` and `crossfadeCounter = 0` instantly
- Grains currently reading frozen buffer will suddenly switch to reading live delay buffer
- This causes an audible click at the crossover point
- Fix: Add a release crossfade that mirrors the 5ms engage crossfade
- The `isActive()` check in GrainPool needs to account for the release transition

**2. Output clipping with many simultaneous grains**
- GrainPool sums up to 64 voices with no output limiting
- At high density + high feedback, summed output easily exceeds +/- 1.0
- The feedback path has soft-clipping (tanh), but the final mix output does not
- Fix: Apply soft-clipping to wet signal before dry/wet mix

### Medium Priority (UI Polish)

**3. No double-click reset on knobs**
- Standard DAW convention: double-click a knob to reset to default
- Currently only mouse drag is supported
- Fix: Add `dblclick` event handler that resets to normalized default (0.5 for most params)

### Low Priority (Deferred)

**4. Timer runs when editor hidden** - Minor CPU waste, not user-visible
**5. No anti-aliasing on pitched reads** - Theoretical concern, rarely audible at typical pitch ranges
**6. Euclidean algorithm is approximation** - Produces correct results for common step/pulse combos

## Patterns from Other Plugins

- O-FreqPulse uses SmoothedValue for all gain crossfades (good pattern, already used here)
- O-AnalogEQ has proper WebView2 config (already matched in O-GrainScatter)

## Release Preparation Research

- CHANGELOG.md format: follows Keep a Changelog convention used across repo
- PLUGINS.md: needs status update to "Working" with version 1.0.0
- STATUS.md: needs Stage 4 completion markers
