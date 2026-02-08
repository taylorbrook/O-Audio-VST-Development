# Stage 4: Polish - Execution Plan

**Plugin:** O-Chorus
**Date:** 2026-02-08
**Goal:** Fix all known issues from Stage 3 verification, run pluginval validation, update stale documentation, and create CHANGELOG.md for v1.0.0 release.

---

## Pre-Conditions

- Stages 1-3 verified complete
- 3 issues identified in Stage 3 VERIFICATION.md
- Discuss phase confirmed: no presets, keep -dev naming, fix all 3 issues

---

## Tasks

### 1. [ ] Fix documentation drift — update parameter count to 7
- **Files:**
  - `Source/PluginEditor.h` (edit line 16 comment: "6 total" → "7 total", add drive to list)
  - `.planning/parameter-spec.md` (add DRIVE parameter section, update totals from 6→7)
  - `.planning/BRIEF.md` (update Controls table from 6 to 7, add Drive row)
- **Changes:**
  - PluginEditor.h: Update comment on line 16 from "Parameters: 6 total (rate, depth, voices, width, tone, mix)" to "Parameters: 7 total (rate, depth, voices, width, tone, mix, drive)"
  - parameter-spec.md: Add DRIVE section (ID: drive, Float, 0.0-1.0, default 0.3, linear, 50ms smoothing), update totals (Float: 6, Total: 7), add Drive to Character group
  - BRIEF.md: Add Drive row to Controls table (0-100%, Analog saturation amount), update "6 parameters" heading to "7 parameters"
- **Depends on:** none

### 2. [ ] Fix LFO frame rate assumption — use timestamp-based animation
- **Files:** `Source/ui/public/index.html` (edit lfoLoop function)
- **Changes:**
  - Add `let lastTimestamp = 0;` variable before lfoLoop
  - Modify `lfoLoop(timestamp)` to calculate `deltaTime = (timestamp - lastTimestamp) / 1000`
  - Replace `/ 60` with `* deltaTime` for frame-rate-independent phase advance
  - Guard first frame (deltaTime > 0.1 means first frame, skip advance)
  - Update `lastTimestamp = timestamp` at end
- **Depends on:** none

### 3. [ ] Fix mouse wheel missing gesture brackets
- **Files:** `Source/ui/public/index.html` (edit wheel event handler)
- **Changes:**
  - Add debounced gesture brackets around wheel events
  - On first wheel event: call `sliderDragStarted()`
  - Set a 200ms timeout to call `sliderDragEnded()` after scrolling stops
  - Track active wheel state per knob to avoid nested start/end calls
  - Clear timeout on subsequent wheel events within the 200ms window
- **Depends on:** none

### 4. [ ] Create CHANGELOG.md for v1.0.0 release
- **Files:** `CHANGELOG.md` (new file in plugin root)
- **Content:**
  - v1.0.0 header with date
  - Initial release features: 8-voice BBD-style chorus, 7 parameters, Naturalist WebView UI, LFO ring animation
  - Cross-platform: VST3 + AU, WebView2 static linking for Windows
- **Depends on:** Tasks 1-3

### 5. [ ] Build and verify zero errors/warnings
- **Commands:** `ninja OuariconChorus_VST3 OuariconChorus_AU OuariconChorus_Standalone` in build dir
- **Verify:** Zero errors, zero warnings across all three formats
- **Depends on:** Tasks 1-3

### 6. [ ] Install and clear AU cache
- **Commands:**
  - Kill AudioComponentRegistrar, clear AU caches
  - Remove old plugin binaries from system folders
  - Copy fresh VST3 and AU to system plugin folders
  - Verify AU detection with `auval -a | grep -i chorus`
- **Depends on:** Task 5

### 7. [ ] Run pluginval validation (VST3 + AU)
- **Commands:**
  - `/Applications/pluginval.app/Contents/MacOS/pluginval --validate-in-process --strictness-level 5 --timeout-ms 120000 <VST3 path>`
  - `/Applications/pluginval.app/Contents/MacOS/pluginval --validate-in-process --strictness-level 5 --timeout-ms 120000 <AU path>`
- **Expected:** All tests pass at strictness level 5
- **Depends on:** Task 6

### 8. [ ] Update STATUS.md
- **Files:** `.planning/STATUS.md`
- **Changes:** Update stage to reflect polish execution complete, update progress to 95%, record pluginval results
- **Depends on:** Task 7

---

## Success Criteria

- [ ] Documentation reflects 7 parameters everywhere (parameter-spec.md, BRIEF.md, PluginEditor.h)
- [ ] LFO ring animation runs at correct speed on 60Hz, 120Hz, and 144Hz displays
- [ ] Mouse wheel changes create discrete DAW undo entries (gesture brackets working)
- [ ] CHANGELOG.md exists with v1.0.0 release notes
- [ ] Build: zero errors, zero warnings (VST3 + AU + Standalone)
- [ ] pluginval passes at strictness level 5 (VST3 and AU)
- [ ] AU detected in system after fresh install

---

## Files Summary

| File | Action | Estimate |
|------|--------|----------|
| `Source/PluginEditor.h` | Edit comment (line 16) | 1 line |
| `.planning/parameter-spec.md` | Add DRIVE section, update totals | +20 lines |
| `.planning/BRIEF.md` | Add Drive row, update count | +2 lines |
| `Source/ui/public/index.html` | Fix lfoLoop + wheel gestures | ~25 lines changed |
| `CHANGELOG.md` | New file | ~20 lines |
| `.planning/STATUS.md` | Update progress | ~10 lines |

**Total:** 6 files (1 new, 5 edited)

---

*Plan ready for execution.*
