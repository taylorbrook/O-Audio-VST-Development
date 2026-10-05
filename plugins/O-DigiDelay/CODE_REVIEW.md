---
plugin: O-DigiDelay
version: 1.7.0
reviewed: 2026-09-30
verified: 2026-09-30T14:19:17-0700  # v1.8.1 (IN-01..09, IN-11, IN-13..15) — /improve-verify PASS
depth: deep
files_reviewed: 8
files_reviewed_list:
  - plugins/O-DigiDelay/Source/PluginProcessor.cpp
  - plugins/O-DigiDelay/Source/PluginProcessor.h
  - plugins/O-DigiDelay/Source/PluginEditor.cpp
  - plugins/O-DigiDelay/Source/PluginEditor.h
  - plugins/O-DigiDelay/Source/OuariconPresetManager.h
  - plugins/O-DigiDelay/Source/ui/public/index.html
  - plugins/O-DigiDelay/Source/ui/public/js/i18n.js
  - plugins/O-DigiDelay/Source/ui/public/modules/preset-manager.js
findings:
  critical: 1
  warning: 10
  info: 16
  total: 27
status: partially_resolved  # open: IN-10, IN-12, IN-16
supersedes: v1.2.9 review (2026-07-01, 11 findings, all resolved in v1.2.10–v1.2.12; see git history of this file)
---

# O-DigiDelay: Full-Plugin Code Review Report (v1.7.0)

**Reviewed:** 2026-09-30
**Depth:** deep (DSP → parameter binding → WebView UI → preset system)
**Status:** issues_found

## Summary

The audio path is still sound on the basics. It allocates nothing, takes no
locks, applies the 0.95 feedback clamp, guards against NaN recirculation, keeps
the WR-01 buffer headroom and checks the bus layout. None of the 11 v1.2.9
findings has regressed. All 8 relay IDs match `createParameterLayout`, all 13
`getNativeFunction` names are registered, and every i18n key has an en, fr and
zh-Hans entry.

What this review turns up:

- **One use-after-free.** The Save dialog outlives the editor.
- **A documented feature that does not exist.** NOTES.md advertises
  "Spillover", but bypass cuts the tail.
- **Two visible UI defects.** The knob arcs draw 90° at minimum and a closed
  ring at maximum. The last row of the preset dropdown is clipped outside the
  196px frame.
- **Three automation-gesture bugs.** Wheel changes send no gesture, a
  sync-mode TIME drag touches the wrong lane, and the preset name goes stale
  when the host restores state.

## Resolution Log

- **v1.8.0 (2026-09-30):** resolved CR-01 and WR-01..WR-10 (WR-01 resolved by
  implementing spillover, not by removing the claim). See CHANGELOG [1.8.0].
- **v1.8.1 (2026-09-30):** Info-tier sweep, which resolved IN-01..IN-03, IN-07..IN-09,
  IN-11, IN-13 and IN-15 in code, and IN-04, IN-05, IN-06 and IN-14 by documentation
  (NOTES.md, Known Issues). See CHANGELOG [1.8.1].
- **Outstanding:** IN-10, IN-12, IN-16 (feature-sized, deferred).

---

## Critical

### CR-01: Save dialog callback uses the editor after it is freed

**File:** `Source/PluginEditor.cpp:79-100`

`savePresetWithDialog` creates a heap `AlertWindow`. The editor does not own it,
and it runs through `enterModalState(true, callback, deleteWhenDismissed)`. The
callback captures a raw `this` and the `complete` callback of the WebView
(`juce_WebBrowserComponent.cpp:317` captures `[this, resultId]`).

**Failure:** press SAVE so the name prompt opens, then close the plugin window
or the project. Pressing Save or Cancel afterwards reads
`this->processorRef` from the freed editor, and `complete()` calls into the
destroyed WebBrowserComponent, so the host crashes. The Load path is safe
because `fileChooser` is a member, and destroying it cancels its callback.

**Fix:**
- Hold the dialog in a `std::unique_ptr<juce::AlertWindow>` member or a
  `SafePointer`, and dismiss it in `~Editor()`.
- In the callback, capture `juce::Component::SafePointer<Editor>` and return
  early if it is null.

---

## Warnings

### WR-01: "Spillover — delay tail continues on bypass" is advertised but not implemented

**Files:** `NOTES.md` (Key Features), `Source/PluginProcessor.h`

There is no `processBlockBypassed` override and no `getBypassParameter()`.
JUCE's default `processBlockBypassed` passes the input through untouched, and
the delay lines stop.

**Failure:** bypass the plugin in a VST3 host and the repeats stop dead instead
of ringing out. In Logic's AU bypass the plugin is not called at all.

**Fix:**
- Add an `AudioParameterBool` "bypass", returned from `getBypassParameter()`.
- While it is on, keep running the delay with no new input fed in, and ramp the
  dry signal to unity.
- Return the same handling from `processBlockBypassed`.
- Alternatively, remove the claim from NOTES.md.

### WR-02: Smoothers aren't seeded from the parameters, so the first block ramps from the constructor defaults

**File:** `Source/PluginProcessor.cpp:157-164` (with the defaults at `PluginProcessor.h:116-121`)

`SmoothedValue::reset(numSteps)` snaps the current value to the *target*. On
the first `prepareToPlay` the target is still the constructor literal (dry 1.0,
wet 0.3, feedback 0.3, time 500), not the restored parameter value. The same
failure class is in memory as `pattern_gain_ramp_without_seeding_fades_in_from_silence`.

**Failure:** a session saved with Dry 0 / Wet 100 (a send or bus delay) leaks a
dry signal that fades from full level to silence over the first 20ms of the
first playback or bounce after load.

**Fix:** in `prepareToPlay`, call `setCurrentAndTargetValue(param->load() / 100)`
for each smoother, and resolve the delay time through the sync logic.

### WR-03: The knob arcs draw 90° at minimum and a full closed ring at maximum

**File:** `Source/ui/public/index.html:998` (and the same line on all 6 knobs), with `1205`, `1232`, `1364` and `1417`

The markup is `stroke-dasharray="157.08"` (dash and gap both 157.08) combined
with `strokeDashoffset = ARC_LENGTH - norm*ARC_LENGTH`, where `ARC_LENGTH`
is 117.81. The visible dash is therefore 39.27 + 117.81·norm, which runs from 90°
to 360°. This was confirmed with a headless render at 700×196.

**Failure:**
- SPREAD and MOD default to 0% but draw a quarter arc.
- DRY defaults to 100% and draws a closed ring, so nothing marks where the 270°
  sweep ends.

**Fix:**
- Set `stroke-dasharray="117.81 314.16"`.
- Check the start rotation: `rotate(-135deg)` starts at 10:30, and the
  conventional 7:30 start is `rotate(135deg)`.

### WR-04: The last row of the preset dropdown sits outside the editor frame

**File:** `Source/ui/public/index.html:314-328`

The dropdown uses `top: 100%; margin-top: 4px; max-height: 180px`, so it spans
y 38–218 in a 196px frame. `.container` has `overflow: hidden`.

**Failure:** with the 12 factory presets installed, "Triplet Feel" sits at
y 196–216 even when the list is scrolled to the bottom. The only way to reach
it, or any user preset that sorts after it, is ◀▶.

**Fix:** use `max-height: 150px`, or compute it from
`innerHeight - rect.bottom - 8` in `showPresetDropdown`.

### WR-05: Mouse-wheel changes on a knob send no automation gesture

**File:** `Source/ui/public/index.html:1392-1397`, `1470-1485`

The wheel handler calls `setNormalisedValue` without
`sliderDragStarted()`/`sliderDragEnded()` around it.
`WebSliderParameterAttachment` then calls `setValueNotifyingHost` with no
gesture.

**Failure:** in Touch or Latch automation (Logic, Cubase, Pro Tools), wheel
moves are either not written or are overwritten by existing automation.

**Fix:** start a gesture on the first wheel event and end it after about
150ms of wheel inactivity.

### WR-06: A sync-mode TIME drag opens a gesture on `time` but changes `division`

**File:** `Source/ui/public/index.html:1279-1284`, `1446-1466`

The drag calls `timeState.sliderDragStarted/Ended`, but the values go through
`divisionState.setChoiceIndex`.

**Failure:** in Touch or Latch, the whole drag touches the Time lane, so the
current Time value overwrites existing Time automation. Division gets one
separate complete gesture for each step.

**Fix:** skip the time gesture while `isSyncMode`.

### WR-07: The preset name goes stale when the host restores state while the editor is open

**Files:** `Source/ui/public/modules/preset-manager.js:159-176`, `Source/OuariconPresetManager.h:557`

`setStateFromXml` rewrites `currentPresetName`, but C++ never pushes an event
and JS never polls for it.

**Failure:** after a host preset recall, an A/B compare or an undo, the knobs
update but the bar shows the old name. The `.active` dropdown row is wrong, and
◀▶ steps from a name the user cannot see.

**Fix:**
- Keep a revision counter on the preset manager and check it in the 30Hz
  editor timer.
- When it changes, fire `emitEventIfBrowserIsVisible("presetChanged", …)`,
  and have JS call `refresh()`.

### WR-08: The output meter reads 6dB low in a mono layout

**Files:** `Source/PluginEditor.cpp:248-250`, `Source/PluginProcessor.cpp:291-299`

The meter computes `(rmsLeft + rmsRight) * 0.5f`. `isBusesLayoutSupported`
accepts mono, and in mono `rmsMeterRight` is never written. It stays at 0, or
at its last stereo value after a layout change.

**Failure:** a mono track meters exactly half its level. After a switch from
stereo to mono, the meter is frozen partly on a stale right-channel value.

**Fix:**
- Store 0 to `rmsMeterRight` when there is no right channel.
- Meter with `max(L, R)`, or average over the active channel count.

### WR-09: Hover tips come back during a knob drag

**File:** `Source/ui/public/index.html:2008-2013`, `2022`

`pointerdown` hides the tip and clears `active`. The next `pointerover`, for
example crossing the arc stroke or the caption, shows it again, because the
handler doesn't check `dragState.isDragging`.

**Failure:** while TIME is being dragged, the tip follows the cursor over the
neighbouring captions and readouts.

**Fix:** return early from `pointerover`/`pointermove` while a drag is active.

### WR-10: The native Save dialog is English-only

**File:** `Source/PluginEditor.cpp:79-84`

The strings "Save Preset", "Enter a name for this preset:", "Save" and "Cancel"
are hard-coded. check-i18n only scans the page, so it cannot see them.

**Failure:** a fr or zh-Hans user presses "Enreg" and gets an English dialog.

**Fix:** pass localized strings from JS as arguments to `savePresetWithDialog`,
or keep a small C++ table keyed on `uiLanguage`.

---

## Info

### IN-01: The mod LFO sweeps 440→0.3Hz over the first 50ms after instantiation **Resolved in v1.8.1**
`PluginProcessor.cpp:153-155`. `dsp::Oscillator`'s frequency smoother starts at
440Hz, and `setFrequency(0.3f)` without `force` ramps down from there over 50ms.
With MOD above 0, a short delay time and audio in the first 50ms, the result is
a brief FM warble. Fix with `lfo.setFrequency(0.3f, true)`.

### IN-02: The triplet factors are rounded **Resolved in v1.8.1**
`PluginProcessor.h:153-155` uses `0.667 / 0.333 / 0.167` where the exact values
are 2/3, 1/3 and 1/6. At 1/16T that is 0.2% off: repeats drift off the grid by
about 0.2ms per repeat at 120BPM, and the error accumulates over long feedback
tails. Use `2.0f/3.0f` and so on.

### IN-03: FEEDBACK above 95% does nothing, and the tail estimate is not a decay time **Resolved in v1.8.1**
- The DSP clamps feedback to 0.95 (`PluginProcessor.cpp:244`), but the
  parameter and readout go to 100%, so the top 5% of knob travel is dead.
- `getTailLengthSeconds` (`:302-320`) always uses 2s rather than the actual
  time, and `2/(1-fb)` is not a −60dB decay: 95% feedback needs about
  135 repeats. The 30s cap hides most of this.
- Either rescale the range so 100% maps to 0.95, or show the clamp.

### IN-04: A large time change plays the buffer at up to 100× speed **Resolved in v1.8.1** (by documentation, NOTES.md)
The 20ms linear ramp on `smoothedTimeMs` moves the read head across the
buffer. A 2000→1ms jump, or a division change in sync mode, plays about 2s of
history in 20ms and produces a loud chirp. That may be intended (tape-like),
but for a "transparent" digital delay a crossfade between two taps is the
usual approach.

### IN-05: Sync silently clamps to 2000ms **Resolved in v1.8.1** (by documentation, NOTES.md)
`PluginProcessor.cpp:222`. 1/4D below 45BPM, or 1/4 below 30BPM, exceeds
2000ms and falls off the grid with no UI indication.

### IN-06: The UI shows a division that the DSP isn't using **Resolved in v1.8.1** (by documentation, NOTES.md)
`index.html:1424-1428` vs `PluginProcessor.cpp:213-227`. With SYNC on and no
BPM from the host (Standalone, or a host with no transport), the DSP uses the
free TIME value while the readout and echo spacing show the division.

### IN-07: Preset apply gaps **Resolved in v1.8.1**
`OuariconPresetManager.h:294-326`:
- `applyPresetJson` doesn't reset parameters that the JSON omits, so a
  partial or older preset inherits the previous values.
- It sets values with no begin/end gesture.
- It casts `prop.value` without validating it.

The Save dialog also returns the *unsanitized* name (`PluginEditor.cpp:93`)
while the manager stores the sanitized one, so "A/B" shows in the bar but the
list holds "A_B".

### IN-08: The readouts flash "1 ms / 0%" when the editor opens **Resolved in v1.8.1**
`index.html:1366-1368`, `1417`, `1440`. Setup writes `formatFn(0)` before the
backend's first update arrives, so the arcs sweep up from 0 on every open.
Skip the write while `parameterIndex === -1`.

### IN-09: The dropdown list and ◀▶ disagree **Resolved in v1.8.1**
`index.html:1605`, `preset-manager.js:331`. The dropdown uses the list cached
at init or the last save, while ◀▶ fetch a fresh list from C++. Call
`refresh()` when the dropdown opens.

### IN-10: No modified indicator, and preset loads fail silently
`preset-manager.js:193-203`. The bar keeps showing the preset name after a
knob moves. A failed `loadPreset` (file deleted on disk) gives no feedback.

### IN-11: The meter rewrites all 14 LED segments at 30Hz whether or not the level changed **Resolved in v1.8.1**
`index.html:1311-1316`. Cache `activeCount`. The editor timer also keeps
calling `evaluateJavascript` while the window is hidden.

### IN-12: No double-click reset or fine-drag modifier, and a fixed 2% wheel step
A trackpad flick can sweep the full range. Scale the wheel step by `deltaY`
and add Shift for fine steps.

### IN-13: Stale comments **Resolved in v1.8.1**
`PluginEditor.cpp:171-173` and `PluginProcessor.cpp:342` say "anything not
'fr' → 0" and "en/fr", but `languageIndex` also maps zh-Hans to 2. The code is
correct.

### IN-14: Cross-thread state access **Resolved in v1.8.1** (by documentation, NOTES.md)
- `getStateInformation` calls `parameters.state.setProperty` (`:344`) from
  whatever thread the host uses, while the message thread may touch the tree.
- `currentPresetName` (a `juce::String`) is written on the message thread and
  read inside `getStateAsXml`.
- This is the suite-wide pattern and has low practical risk.

### IN-15: Delay lines allocate channels they never use **Resolved in v1.8.1**
`PluginProcessor.cpp:135,147`. Each `DelayLine` is prepared with
`numChannels = 2`, but only channel 0 of each is used, so twice the needed
memory is allocated (about 780k floats per line at 192kHz). Prepare each with
`numChannels = 1`.

### IN-16: Accessibility (R7, known open)
- The knobs have no keyboard or ARIA support.
- SYNC (`index.html:987`) is a `<div>` with no `tabindex` or `role="switch"`.
- `#presetName` (`:963`) is a span that cannot receive focus, so the preset
  list is mouse-only.

---

## Resolved

| Version | Commit | Findings |
|---------|--------|----------|
| v1.8.0 | aeebc2a2 | CR-01, WR-01..WR-10 |
| v1.8.1 | 87326721 | IN-01..IN-09, IN-11, IN-13..IN-15 (IN-04/05/06/14 by documentation) |

Open: IN-10, IN-12, IN-16.
