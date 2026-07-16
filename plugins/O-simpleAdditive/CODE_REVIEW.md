---
phase: O-simpleAdditive-v1.0.2
reviewed: 2026-07-15
verified: 2026-07-15T17:30:00-04:00
depth: deep
files_reviewed: 12
files_reviewed_list:
  - plugins/O-simpleAdditive/Source/PluginProcessor.cpp
  - plugins/O-simpleAdditive/Source/PluginProcessor.h
  - plugins/O-simpleAdditive/Source/PluginEditor.cpp
  - plugins/O-simpleAdditive/Source/PluginEditor.h
  - plugins/O-simpleAdditive/Source/AdditiveVoice.h
  - plugins/O-simpleAdditive/Source/AdditiveVizAnalyzer.h
  - plugins/O-simpleAdditive/Source/ui/public/index.html
  - plugins/O-simpleAdditive/Source/ui/public/css/styles.css
  - plugins/O-simpleAdditive/Source/ui/public/js/app.js
  - plugins/O-simpleAdditive/Source/ui/public/js/juce/index.js
  - plugins/O-simpleAdditive/CMakeLists.txt
  - plugins/O-simpleAdditive/CHANGELOG.md
findings:
  critical: 0
  warning: 6
  info: 6
  total: 12
status: issues_found
---

# O-simpleAdditive v1.0.2: Code Review Report

**Reviewed:** 2026-07-15
**Depth:** deep (cross-file: C++↔JS WebView bridge, APVTS wiring, RT audio-thread safety)
**Files Reviewed:** 12
**Status:** issues_found

## Summary

O-simpleAdditive is a pedagogical 16-voice additive/wavetable-scan synth (16 drawbars →
band-limited single-cycle table per note, Frame A→B spectral morph, spectral-decay tilt,
bit-depth quantizer) with an "Additive Field Guide" WebView UI.

The suite's recurring failure modes are handled **correctly** here:

- **WebView bridge is complete.** Every JS `Juce.getNativeFunction` call has a matching
  `withNativeFunction` registration: `uiMidi` (app.js:453 ↔ PluginEditor.cpp:101) and
  `applyFactoryPreset` (app.js:420 ↔ PluginEditor.cpp:112). All 33 params traced end to
  end: 31 `WebSliderRelay` + 2 `WebComboBoxRelay` (PluginEditor.cpp:70-86) match the JS
  `DRAWBAR_IDS`/`KNOB_IDS`/`COMBO_IDS` inventory (app.js:16-24), the HTML `knob-*` /
  `combo-*` elements, and `OSimpleAdditive::ParamIDs` exactly. No dead controls. ✓
- **`Juce` ES-module namespace** used for state/native functions; `window.__JUCE__` used
  only for `backend.addEventListener` (correct low-level use, app.js:340-348). ✓
- **Knob/value readouts use `SliderState.getScaledValue()`** with `propertiesChangedEvent`
  re-render (app.js:104-118) — no hardcoded JS min/max map, skew-safe. ✓
- **Factory presets** reset ALL params to defaults first, then author values in
  engineering units through `convertTo0to1` (PluginProcessor.cpp:363-372) — neither the
  skewed-normalized-fraction bug nor the partial-preset stale-state bug applies. ✓
- **Resource provider** compares bare paths by direct equality (PluginEditor.cpp:40-59). ✓
- **CMake:** single `juce_add_binary_data` target (no BinaryData namespace collision);
  `NEEDS_WEBVIEW2 TRUE` paired with `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`
  (CMakeLists.txt:23, 92); Windows `withUserDataFolder` set (PluginEditor.cpp:118-126). ✓
- **Editor member order** relays → webView → attachments with the reverse-destruction
  rationale documented (PluginEditor.h:43-60). ✓
- **RT safety otherwise sound:** pre-allocated `monoScratch`, lock-free `VizRing`
  (release/acquire on writePos), atomic 16-element spectrum snapshot, `ScopedNoDenormals`,
  block-level `isfinite` scrub, no locks/allocation/logging in `processBlock` or voice
  render, FFT confined to the editor Timer. No `launchAsync`/FileChooser, no IIR
  coefficient updates. ✓
- **Vendored `js/juce/index.js`** is byte-identical (md5 `01ff05e3…`) to the suite-majority
  copy shipped in 24 plugins (incl. O-simpleFM) — stock, unmodified. ✓

No blockers found. Six warnings: one bounded RT-safety violation (lazy sine-LUT init on
the audio thread), one live-control dead zone in the spectral-decay logic, missing
automation gestures around the lesson-preset writes, two pointer/keyboard interaction
leaks in the UI (stuck drag gesture, stuck notes), and a source↔registry version drift
(source still says 1.0.0 while PLUGINS.md says 1.0.2).

## Warnings

### WR-01: `fastSine` sine-LUT lazily constructed on the audio thread (lock + heap allocation at first note-on)

**File:** `plugins/O-simpleAdditive/Source/AdditiveVoice.h:53-70`
**Issue:** `fastSine` uses a function-local `static const SineTable table;`. The first call
site is `refillTable()` (AdditiveVoice.h:432), reached via `startNote → refillTable` —
i.e. **on the audio thread**. First-ever note-on therefore executes: the C++ magic-static
guard (a mutex acquire under contention, `__cxa_guard_acquire`), a
`LookupTableTransform::initialise` heap allocation, and 1024 `std::sin` calls — all inside
`processBlock`. It is one-time and bounded (~tens of µs), but it is a real allocation +
potential lock on the RT thread, the exact class of defect PERF-01 exists to prevent, and
it lands at the most audible moment (the first note the user plays).
**Failure scenario:** Under a small buffer (64 samples @ 96 kHz ≈ 0.7 ms budget) on a
loaded session, the first note-on can overrun the callback → audible click/dropout on
first play. Debug RT-safety tooling (TSan/RTSan, pluginval RT checks in future versions)
will also flag it.
**Fix:** Force the one-time init off the audio thread — e.g. in
`AdditiveVoice::prepareToPlay` (or the processor constructor):
```cpp
void prepareToPlay (double sr, int /*maxBlockSize*/)
{
    (void) OSimpleAdditive::fastSine (0.0f);   // touch the LUT: one-time init off the RT path
    ...
}
```
(`prepareToPlay` runs on the message/host thread before rendering starts, so the static is
fully constructed before any `renderNextBlock`.)

### WR-02: Spectral Decay / Vel→Decay knob changes are silently ignored once `tau` saturates (and turning decay off mid-note never restores brightness)

**File:** `plugins/O-simpleAdditive/Source/AdditiveVoice.h:295-307` (dirty logic), `:186-191` (setParams)
**Issue:** `setParams` stores `spectralDecay`/`velToDecay` without setting `spectrumDirty`;
the only place a rate change re-dirties the table is the render-loop branch
`if (currentDecayRate > 0.0f && tau < 1.0f)`. Two live-control dead zones follow:
1. After a note has been held ≥ `kTauRampSeconds` (2 s), `tau == 1.0` → the branch never
   runs again → moving the **Spectral Decay** knob (or any change to the effective rate)
   produces **no audible change** on that voice — the table is never refilled with the new
   `D_k = exp(-rate·k·tau)`.
2. Turning the knob **down to 0** mid-note takes the `rate > 0` condition false → the table
   stays frozen at its last *decayed* (dark) state instead of restoring the undecayed
   spectrum, until the scan pointer happens to move or the note is retriggered.
The setParams comment ("A change in either source … re-dirties the table while tau<1")
documents the mechanism but not these gaps. Because `refillTable` reads
`currentDecayRate` fresh each call, only the dirty flag is missing.
**Failure scenario:** The headline pedagogical gesture — hold a chord, sweep Spectral
Decay to *hear* the tilt — goes dead 2 seconds into the note. Drawbar live-glow also
freezes (the snapshot is only updated inside `refillTable`).
**Fix:** Dirty the table whenever the effective rate itself changes:
```cpp
// renderNextBlock, after currentDecayRate = effectiveDecayRate();
if (! juce::exactlyEqual (currentDecayRate, lastRenderedDecayRate))
{
    lastRenderedDecayRate = currentDecayRate;
    if (tau > 0.0f) spectrumDirty = true;   // rate change is audible once tau has advanced
}
```
(add `float lastRenderedDecayRate = 0.0f;` to the voice state). This also covers the
rate→0 restore case.

### WR-03: `applyFactoryPreset` calls `setValueNotifyingHost` without begin/endChangeGesture

**File:** `plugins/O-simpleAdditive/Source/PluginProcessor.cpp:359-372`
**Issue:** Both the reset-to-defaults loop (`:363-364`) and the `setReal`/`setChoice`
lambdas (`:366-372`) call `setValueNotifyingHost` bare. JUCE documents that this call must
be bracketed by `beginChangeGesture()`/`endChangeGesture()`; the VST3 wrapper maps these
to `beginEdit`/`performEdit`/`endEdit`, and a `performEdit` without `beginEdit` is out of
spec. Hosts that gate automation recording on gestures (Logic touch/latch, Cubase) may
drop or mis-record the 33 writes; some hosts assert or ignore un-gestured edits.
**Failure scenario:** User arms automation write, clicks a lesson preset → some/all of the
33 parameter moves are not recorded, or the host's undo/automation state desyncs.
**Fix:**
```cpp
for (auto* p : getParameters())
{
    p->beginChangeGesture();
    p->setValueNotifyingHost (p->getDefaultValue());
    p->endChangeGesture();
}
// and in setReal / setChoice:
p->beginChangeGesture();
p->setValueNotifyingHost (p->convertTo0to1 (real));
p->endChangeGesture();
```

### WR-04: Knob/drawbar drags have no pointer capture and no `pointercancel` handling — stuck drag + host gesture left open

**File:** `plugins/O-simpleAdditive/Source/ui/public/js/app.js:143-166` (knob), `:219-239` (drawbar)
**Issue:** Drag state is ended only by a `pointerup` delivered to `window`. Two escapes:
(1) release the mouse button **outside the plugin window** — WKWebView/WebView2 do not
deliver `pointerup` for events outside the view without capture, so `dragging` stays
`true`, the `pointermove` listener stays attached (the knob keeps following the cursor
with no button held), and — worse — `sliderDragEnded()` never fires, leaving the host
automation gesture (`beginChangeGesture`) **open indefinitely**; (2) a `pointercancel`
(touch interruption, OS gesture) fires neither `pointerup` nor cleanup.
**Failure scenario:** Drag a knob, release over the DAW timeline → knob glued to cursor;
host stuck in touch-automation "writing" on that parameter until the plugin window is
clicked again.
**Fix:** Capture the pointer and treat cancel as up:
```js
knob.addEventListener("pointerdown", (e) => {
  knob.setPointerCapture(e.pointerId);        // guarantees pointerup/cancel delivery
  ...
});
window.addEventListener("pointercancel", onUp);
```
(apply identically to the drawbar track handlers; remove the `pointercancel` listener in
`onUp` alongside the others).

### WR-05: On-screen keyboard can leave stuck (hung) notes — no blur/cancel release, pointerup lost outside the view

**File:** `plugins/O-simpleAdditive/Source/ui/public/js/app.js:491-517`
**Issue:** Note-off depends on `window` receiving `pointerup` (mouse) or `keyup` (QWERTY).
Both are droppable: releasing the mouse outside the plugin window, a `pointercancel`, or —
for QWERTY play — clicking away to the DAW mid-keypress (the `keyup` goes to the DAW, not
the WebView). `heldNotes` then never clears, `noteOff` is never sent, and the synth
sustains the note indefinitely (5 s max release only applies after a note-off arrives).
**Failure scenario:** Press `A` to audition, click the DAW transport while holding it →
note drones forever; the UI key stays green; only re-pressing and releasing the same key
recovers.
**Fix:** Add a panic path on focus loss and pointer cancellation:
```js
function releaseAll() { [...heldNotes].forEach(noteOff); pointerNote = null; }
window.addEventListener("blur", releaseAll);
document.addEventListener("visibilitychange", () => { if (document.hidden) releaseAll(); });
window.addEventListener("pointercancel", () => {
  if (pointerNote != null) { noteOff(pointerNote); pointerNote = null; }
});
```

### WR-06: Version drift — source builds 1.0.0 while the suite registry records 1.0.2

**File:** `plugins/O-simpleAdditive/CMakeLists.txt:17`, `plugins/O-simpleAdditive/CHANGELOG.md:6`
**Issue:** `juce_add_plugin(... VERSION "1.0.0")`, the CHANGELOG's newest entry is
`[1.0.0] — 2026-06-22`, and `.planning/STATUS.md` says `version: 1.0.0` — but `PLUGINS.md`
lists O-simpleAdditive at **1.0.2** (2026-06-25), and this review was commissioned as
v1.0.2. Git history for the plugin ends at the v1.0.0 Stage-4 commit (`942f1fa`); no
1.0.1/1.0.2 source change or changelog entry exists. Either the registry was bumped
without a source version bump, or two point releases were never recorded in-source. This
is the exact drift class that has previously caused stale-install confusion in this suite
(host shows a plugin version that doesn't match the registry/installer expectations, and
`/publish`-style tooling keys off the CMake VERSION).
**Failure scenario:** A rebuilt binary reports 1.0.0 to the DAW; anyone debugging against
PLUGINS.md ("1.0.2 is installed") chases a phantom stale-cache problem. Packaging/publish
scripts that read the CMake VERSION would tag a release as 1.0.0.
**Fix:** Reconcile the three sources of truth: if 1.0.1/1.0.2 shipped, set
`VERSION "1.0.2"` in CMakeLists.txt, add the missing CHANGELOG entries, and update
STATUS.md; if they did not, correct PLUGINS.md back to 1.0.0.

## Info

### IN-01: `getSampleRate` native function registered but never called from JS

**File:** `plugins/O-simpleAdditive/Source/PluginEditor.cpp:108-110`
**Issue:** Registered with a comment "reserved for future frequency-axis labels"; no
`Juce.getNativeFunction("getSampleRate")` exists in app.js. Harmless dead bridge endpoint.
**Fix:** Keep (documented as deliberate) or remove until a caller exists — either is fine;
just don't let it drift into an assumed-working API.

### IN-02: `currentNote` voice member is written but never read

**File:** `plugins/O-simpleAdditive/Source/AdditiveVoice.h:203, 456`
**Issue:** `currentNote = midiNote;` in `startNote` is the only reference; nothing consumes
it (the base class already tracks `getCurrentlyPlayingNote()`). Dead state.
**Fix:** Delete the member and assignment.

### IN-03: `computeKmax` edge — a fundamental at/above Nyquist is still synthesized

**File:** `plugins/O-simpleAdditive/Source/AdditiveVoice.h:344-349`
**Issue:** When `f0 ≥ 0.5·fs` (e.g. MIDI 127 ≈ 12.5 kHz at sample rates below ~25 kHz),
`floor(nyquist/f0) == 0` but `jlimit(1, 16, 0)` forces `Kmax = 1`, so the fundamental is
written above Nyquist and aliases — the one hole in the otherwise-exact band-limit.
Unreachable at standard rates (44.1 kHz+); noted for completeness.
**Fix:** `if (k < 1) { Kmax = 0; }` and have `refillTable`/render treat `Kmax == 0` as
silence (or clamp `nyquistGain(1, 0)` to 0).

### IN-04: `uiMidi` native function trusts its JS arguments (note number / velocity unvalidated)

**File:** `plugins/O-simpleAdditive/Source/PluginProcessor.cpp:345-352`, `PluginEditor.cpp:101-106`
**Issue:** `handleUiMidi` builds `MidiMessage::noteOn(1, noteNumber, ...)` from the raw
bridge int. The current JS clamps to 48–72, but the native boundary itself accepts any
int (out-of-range hits a debug `jassert` inside JUCE and produces a malformed message in
release) and a NaN velocity passes through `jlimit` (NaN comparisons are false). The NaN
would be scrubbed downstream, but the boundary should be defensive.
**Fix:** `noteNumber = juce::jlimit (0, 127, noteNumber);` and
`if (! std::isfinite (velocity)) velocity = 0.8f;` at the top of `handleUiMidi`.

### IN-05: `midiCollector` is only reset in `prepareToPlay` — messages queued before first prepare hit an unreset collector

**File:** `plugins/O-simpleAdditive/Source/PluginProcessor.cpp:133-144, 155`
**Issue:** If the editor's on-screen keyboard fires before the host's first
`prepareToPlay` (possible in Standalone startup or an editor opened on a suspended
plugin), `MidiMessageCollector::addMessageToQueue` runs against an un-`reset()` collector
(debug assertion; undefined timestamp base). Hosts normally prepare first, so this is a
belt-and-braces item.
**Fix:** `midiCollector.reset (44100.0);` in the constructor.

### IN-06: `role="slider"` elements never publish `aria-valuenow`/`aria-valuemin`/`aria-valuemax`

**File:** `plugins/O-simpleAdditive/Source/ui/public/js/app.js:131-132, 183`
**Issue:** Knobs and drawbars declare `role="slider"` + `tabindex="0"` (good — keyboard
nudge works) but no ARIA value attributes, so screen readers announce a valueless slider.
**Fix:** In `updateKnobVisual` / the drawbar `update()`, set
`el.setAttribute("aria-valuenow", st.getScaledValue())` (plus static min/max once, and
`aria-valuetext` using the existing `FORMAT` map).

---

_Reviewed: 2026-07-15_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
