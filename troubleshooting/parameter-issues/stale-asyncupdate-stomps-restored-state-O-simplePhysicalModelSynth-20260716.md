---
plugin: O-simplePhysicalModelSynth
date: 2026-07-16
problem_type: state_management
component: juce_processor
symptoms:
  - "Restored damping/decay values intermittently overwritten right after session/preset load when the material macro was being automated"
  - "Bug only reproduces when host automation touches the macro param on the audio thread shortly before setStateInformation runs"
  - "restoringState guard present in both parameterChanged and handleAsyncUpdate, yet restored values still stomped"
root_cause: event_timing
juce_version: 8.0.9
resolution_type: code_fix
severity: moderate
tags: [asyncupdater, cancelpendingupdate, setstateinformation, state-restore, macro-parameter, race-condition, guard-flag]
---

# Troubleshooting: Stale AsyncUpdater Apply Survives a State-Restore Guard Flag

## Problem
A macro parameter (`material`) that re-derives two visible knobs (damping/decay) defers its apply to the message thread via `juce::AsyncUpdater` when automated from the audio thread. An update queued *before* `setStateInformation` ran would fire *after* the restore's `restoringState` guard was cleared, applying pre-restore macro targets over the freshly restored damping/decay. (CODE_REVIEW.md VR-01, residual of CR-03.)

## Environment
- Plugin: O-simplePhysicalModelSynth v1.0.1 → fixed in v1.0.2
- JUCE Version: 8.0.9
- Affected: `Source/PluginProcessor.cpp` — `setStateInformation` / `handleAsyncUpdate`, post-implementation (verify pass)
- Date: 2026-07-16

## Symptoms
- Restored damping/decay intermittently wrong immediately after session restore or preset load
- Requires the race: audio-thread `material` automation stashes `pendingDamping`/`pendingDecay` + `triggerAsyncUpdate()`, then `setStateInformation` runs before the update fires
- Both `parameterChanged` and `handleAsyncUpdate` correctly checked `restoringState` — the guard just wasn't up anymore when the queued callback finally ran

## What Didn't Work

**Attempted Solution 1 (CR-03, v1.0.1):** `restoringState` atomic flag set around `setStateFromXml`, checked in `parameterChanged` and `handleAsyncUpdate`.
- **Why it failed (partially):** It suppresses re-entry *during* the restore, but an AsyncUpdate queued *before* the restore fires on the message loop *after* the whole `setStateInformation` call — by then the flag is false again, so `handleAsyncUpdate` happily applies the stale targets.

## Solution

Cancel the pending update inside `setStateInformation` while the guard is still up — the destructor already used the same pattern for teardown.

**Code changes** (`Source/PluginProcessor.cpp`):
```cpp
// Before (broken):
restoringState = true;
presetManager.setStateFromXml (xml.get());
restoringState = false;

// After (fixed):
restoringState = true;
presetManager.setStateFromXml (xml.get());
// VR-01: a macro apply queued from audio-thread automation BEFORE this
// restore would fire after restoringState clears and stomp the restored
// damping/decay — kill it while the guard is still up (dtor does the same).
cancelPendingUpdate();
restoringState = false;
```

## Why This Works

1. **Root cause:** message-loop event timing. `triggerAsyncUpdate()` queues a callback that cannot run while `setStateInformation` occupies the message thread — it can only run *after* the function returns, i.e. after the guard flag has been cleared. A flag that brackets a synchronous section cannot suppress an asynchronous callback that outlives the bracket.
2. **Why the fix addresses it:** `cancelPendingUpdate()` removes the queued callback entirely, so nothing stale fires after the guard drops. Placing it *before* `restoringState = false` also kills any update queued by concurrent audio-thread automation during the restore itself.
3. **Underlying pattern:** any guard-flag + `AsyncUpdater` combination has this hole. The flag protects the synchronous window; `cancelPendingUpdate()` protects the asynchronous tail. You generally need both (the destructor needing `cancelPendingUpdate()` is the well-known half; state restore is the forgotten half).

## Prevention

- Whenever a class pairs an `AsyncUpdater` with a "suppress during X" flag, grep for every place the flag is set and ask: *can an update already be queued when we enter this section?* If yes, `cancelPendingUpdate()` while the flag is still up.
- Checklist for `AsyncUpdater` users: `cancelPendingUpdate()` in the destructor **and** in any state-restore / bulk-apply path that must win over queued deferred applies.
- Regression gate: render-harness `state-roundtrip` check (22/22 PASS post-fix), though note the harness can't easily reproduce the message-loop race itself — this class of bug needs code inspection, not just black-box tests.

## Related Issues

- Same category, different root cause: [ableton-preset-interference-state-restoration-JUCE-20251107.md](ableton-preset-interference-state-restoration-JUCE-20251107.md) (host program-list interference with state restore)
