---
phase: stage-4-polish
reviewed: 2026-10-06
version_reviewed: 1.0.0
depth: deep
files_reviewed: 37
files_reviewed_list:
  - plugins/O-simpleWavetable/Source/BankFactory.cpp
  - plugins/O-simpleWavetable/Source/BankFactory.h
  - plugins/O-simpleWavetable/Source/BitQuantizer.h
  - plugins/O-simpleWavetable/Source/BuiltInBanks.cpp
  - plugins/O-simpleWavetable/Source/BuiltInBanks.h
  - plugins/O-simpleWavetable/Source/CycleView.cpp
  - plugins/O-simpleWavetable/Source/CycleView.h
  - plugins/O-simpleWavetable/Source/MipmapBuilder.cpp
  - plugins/O-simpleWavetable/Source/MipmapBuilder.h
  - plugins/O-simpleWavetable/Source/MonoStack.h
  - plugins/O-simpleWavetable/Source/PluginEditor.cpp
  - plugins/O-simpleWavetable/Source/PluginEditor.h
  - plugins/O-simpleWavetable/Source/PluginProcessor.cpp
  - plugins/O-simpleWavetable/Source/PluginProcessor.h
  - plugins/O-simpleWavetable/Source/PositionLfo.h
  - plugins/O-simpleWavetable/Source/PositionSmoother.h
  - plugins/O-simpleWavetable/Source/PresetRecipes.h
  - plugins/O-simpleWavetable/Source/TestHooks.h
  - plugins/O-simpleWavetable/Source/VizPayload.h
  - plugins/O-simpleWavetable/Source/WavetableBank.h
  - plugins/O-simpleWavetable/Source/WavetableImporter.cpp
  - plugins/O-simpleWavetable/Source/WavetableImporter.h
  - plugins/O-simpleWavetable/Source/WtRead.h
  - plugins/O-simpleWavetable/Source/WtSynthesiser.h
  - plugins/O-simpleWavetable/Source/WtVoice.h
  - plugins/O-simpleWavetable/Source/ui/public/index.html
  - plugins/O-simpleWavetable/Source/ui/public/js/i18n.js
  - plugins/O-simpleWavetable/Source/ui/public/js/juce/index.js
  - plugins/O-simpleWavetable/Source/ui/public/js/juce/check_native_interop.js
  - plugins/O-simpleWavetable/tests/bank-check/main.cpp
  - plugins/O-simpleWavetable/tests/dsp-check/main.cpp
  - plugins/O-simpleWavetable/tests/import-check/main.cpp
  - plugins/O-simpleWavetable/tests/mod-check/main.cpp
  - plugins/O-simpleWavetable/tests/perf-check/main.cpp
  - plugins/O-simpleWavetable/tests/state-check/main.cpp
  - plugins/O-simpleWavetable/tests/viz-check/main.cpp
  - plugins/O-simpleWavetable/tests/ui-probes/wheel-gesture-probe.mjs
findings:
  critical: 0
  warning: 10
  info: 21
  total: 31
status: resolved
open_findings: none
sources: .planning/stages/2-dsp/VERIFICATION.md (critic W1-W5, notes 1-8);
  .planning/stages/3-gui/VERIFICATION.md (critic W1-W5, N1-N13);
  .planning/stages/4-polish/RESEARCH.md §B §5 (verdicts and wording)
---

# O-simpleWavetable v1.0.0: Code Review Report

**Reviewed:** 2026-10-06
**Depth:** deep (Stage 2 DSP + architecture critic, Stage 3 UI + architecture + DSP-on-touched-code critic, Stage 4 fix-or-log research)
**Status:** resolved. Every finding is fixed, accepted by design, or reviewed and logged. No finding is open.

## Summary

This file consolidates the two critic reviews run during development into the suite
format. Neither review found a blocker. The Stage 2 review (DSP and architecture)
raised 5 warnings and 8 notes; the Stage 3 review (UI, architecture and the DSP it
touched) raised 5 warnings and 13 notes.

ID mapping:
- **WR-01 … WR-05** = Stage 2 critic W1–W5
- **WR-06 … WR-10** = Stage 3 critic W1–W5
- **IN-01 … IN-08** = Stage 2 critic notes 1–8
- **IN-09 … IN-21** = Stage 3 critic N1–N13

File and line anchors point at the code before the fix: the critic's own anchor where it
gave one, otherwise the Stage 4 plan-time anchor (`b71b7888`).

## Warnings

### WR-01 (Stage 2 W1): A voice steal clicks
`WtVoice.h:279-283`: `stopNote (..., false)` reset the amp envelope, so a stolen voice
dropped to 0 in one sample. Confirmed on the installed VST3: 16 held sines plus a 17th
note gave a max |Δy| of 8.7× steady state (no-steal control 1.5×). QUAL-01.

### WR-02 (Stage 2 W2): The velocity gain steps on a reused voice
`WtVoice.h:418-422`: `velGain = v²` was set instantly. Confirmed in Mono: a vel 127 note
released, then retriggered at vel 38, stepped 0.489 → 0.042 in one sample, |Δy| 10×
steady state. Not in Poly (JUCE starts a fresh voice). QUAL-01.

### WR-03 (Stage 2 W3): The first Mono note after Poly → Mono can play bent
An idle voice 0 never saw the pitch wheel return to centre (`WtVoice.h:412-416`,
`PluginProcessor.cpp:384-389`), so a wheel move in Poly followed by a mode switch bent
the first Mono note.

### WR-04 (Stage 2 W4): A big upward legato jump aliases for about 5 ms
The frozen crossfade source was the old, richer mip level read at the new pitch
(`WtVoice.h:604-610`). The critic estimated −21 dB for a 60 → 96 jump; Stage 4 measured
−24.3 / −26.0 dB (Sine→Saw / Drive, 48 kHz). QUAL-02 edge (transient).

### WR-05 (Stage 2 W5): ARCHITECTURE §17 is wrong about bank reads
Row 17 still said the message thread may read the Imported bank pointer directly. Since
Amendment 8 the import worker and `setStateInformation` also publish, so UI reads must
go through `getImportedBankSnapshot()`. Documentation only.

### WR-06 (Stage 3 W1): `uiReady` reads its counters after it emits
`PluginEditor.cpp:230-235`. If an import completed in that window, its `done` status was
swallowed: the page stayed `importBusy`, and Import and drop were disabled until a
reload.

### WR-07 (Stage 3 W2): A 96 MB drop freezes the message thread
`index.html:1144` + `PluginProcessor.cpp:1121-1126`. The base64 string (about 128 M
characters) was decoded synchronously into an unsized `MemoryBlock`, freezing the message
thread for seconds, while the importer keeps only 524,288 samples.

### WR-08 (Stage 3 W3): Wheel steps at least once per event
`index.html:1484`, `:2358`. The stepped knobs and the bank stack stepped at least once
per wheel event, so a trackpad flick jumped `bit_depth` from Full to 3. The continuous
knobs had the same per-event behaviour (W3b, added at plan sign-off).

### WR-09 (Stage 3 W4): A stale import error reappears
`index.html:2250` / `:2263` + the `uiReady` order. A stale `tooShort` error reappeared on
a built-in bank after the editor reopened.

### WR-10 (Stage 3 W5): A UI-held note sticks when the editor closes
`index.html:2563-2582`, `PluginEditor.cpp:360-363`. A note held from the on-screen
keyboard stuck if the host closed the editor mid-hold: no keyup arrived, `blur` did not
fire on teardown, and the destructor only called `stopTimer()`.

## Info

### IN-01 (Stage 2 note 1): Poly ↔ Mono switch hard-stops voices
User-initiated; accepted.

### IN-02 (Stage 2 note 2): Interpolation-Off frame-step ticks
Frame steps tick on Formant and Imported frames. The accepted CONTEXT decision.

### IN-03 (Stage 2 note 3): MIDI scratch buffer growth path
The MIDI scratch buffers are pre-sized and could reallocate on the audio thread past
about 3,600 events per block (`PluginProcessor.cpp:444-475`).

### IN-04 (Stage 2 note 4): No guard for `processBlock` before `prepareToPlay`
`PluginProcessor.cpp:355-377`.

### IN-05 (Stage 2 note 5): Mono ignores sustain
`renderMono` handles note on / off, all-notes / sound-off and the wheel only
(`PluginProcessor.cpp:511-551`).

### IN-06 (Stage 2 note 6): Import jobs can't be cancelled mid-decode
The UI has no control to stop an import job that is decoding.

### IN-07 (Stage 2 note 7): Save / restore hold `bankStateLock` while decoding
Flagged: save and restore decode while holding the lock (`restoreImportedBank` /
`writeImportedBank`, `PluginProcessor.cpp:841-891`). Stage 4 found the premise does not
match the code (see the resolution log).

### IN-08 (Stage 2 note 8): `#if OSIW_TEST_HOOKS` tested without the defining include
Headers tested the macro without including the file that defines its default, so `-Wundef`
warned (pre-fix: WtVoice 2, PositionSmoother 1, PositionLfo 5).

### IN-09 (Stage 3 N1): The FileChooser is not a member
A double click opened two dialogs (`PluginEditor.cpp:241-262`).

### IN-10 (Stage 3 N2): Display atomics can be torn for one tick
`PluginProcessor.cpp:577-595` (writer), `:628-667` (reader).

### IN-11 (Stage 3 N3): `getBankThumbnails` reads `importedOwner` under the lock
`PluginProcessor.cpp:686-691`; allowed by Amendment 18.

### IN-12 (Stage 3 N4): `releaseResources` does not clear the display state
`PluginProcessor.cpp:340-345`; the `disp*` reset lived only in `prepareToPlay`
(`:329-335`). After a held note and `releaseResources()` the lamp froze "sounding".

### IN-13 (Stage 3 N5): A pending auto-select can override a lesson
A lesson clicked while an import's auto-select was pending lost its bank to Imported.

### IN-14 (Stage 3 N6): A lesson clicked during Mono playback hard-stops the voice
2 ms tail; by design.

### IN-15 (Stage 3 N7): `sanitiseName` keeps C1 and bidi characters
`WavetableImporter.cpp:173-178`: `if (c >= 0x20 && c != 0x7f) clean += c;`.

### IN-16 (Stage 3 N8): Two gesture owners on `position`
The hero knob and the bank stack could each open a host gesture on `position`.

### IN-17 (Stage 3 N9): A newer-format session shows "import failed"
`renderImportError` had no `unsupported` branch (`index.html:2225-2232`).

### IN-18 (Stage 3 N10): The lesson highlight goes stale
The click-time `.active` toggle (`index.html:2439`) never cleared on edits, loads or
restores.

### IN-19 (Stage 3 N11): `#octDown` and `#octUp` have no tooltip
The two octave buttons were missing from TIP_BINDINGS.

### IN-20 (Stage 3 N12): `fmtDb` uses ASCII `-inf`
`index.html:1098`.

### IN-21 (Stage 3 N13): One automation gesture per step on stepped knobs
A stepped-knob drag wrote one begin / end pair per detent (`index.html:1414-1498`).

## Resolution log

> | Finding | Status | Where |
> |---------|--------|-------|
> | WR-01 | ✅ Resolved in the Stage 2 gap closure (`850df9b9`) | `WtVoice.h` `startTail` / `renderTail`: a hard stop with the amp envelope active decays the last output sample over a 2 ms raised cosine (ARCHITECTURE Amendment 13). G-STEAL 1.076× (negative control 4.99×); installed VST3 0.86× (was 8.7×). |
> | WR-02 | ✅ Resolved in the Stage 2 gap closure (`850df9b9`) | `setVelocityGain`: a start on a still-sounding voice ramps `velGain` over 3 ms; a start from idle stays instant (G-VEL unchanged, −11.905 dB). G-RETRIG-VEL 0.589× (negative control 35.8×); installed VST3 0.74× (was ~10×). |
> | WR-03 | ✅ Resolved in **v1.0.0** | `wheelNow` tracks every pitch-wheel event in the MIDI chunk fill; the Poly → Mono switch seeds voice 0 with it. **G-MONO-WHEEL** 0.00 / −0.00 / 0.00 c (negative control +199.98 / +100.00 c). R-NULL `w3_poly_to_mono` +199.98 c → 0.000 c. |
> | WR-04 | ✅ Resolved in **v1.0.0** | The frozen cycle keeps the phase and rate it was heard at (`xfPhase` / `xfInc`); the fold reads it phase-aligned. **G-LEGATO-XF** ≤ 7.1e-8 (negative control ≥ 0.544), **G-LEGATO-ALIAS** −102.0 / −107.2 dB = the no-jump floor (negative control −24.3 / −26.0), **G-LEGATO-CLICK** 1.000 (negative control 1.532). Residual: see the reviewed rows below. |
> | WR-05 | ✅ Resolved in the Stage 2 gap closure (`850df9b9`) | ARCHITECTURE row 17, §12, the Threading table and the Visualization Data Path route every UI read of Imported through `getImportedBankSnapshot()`; `importedOwner` is a `shared_ptr` (Amendment 12). |
> | WR-06 | ✅ Resolved in **v1.0.0** | `uiReady` assigns `lastBankGeneration`, `lastBankIndex`, `lastImportVersion`, `lastPresetRevision` and `lastPresetModified` first, then emits in `timerCallback` order. A transition inside the window re-sends next tick (a duplicate, never a loss). Code inspection + Task 14 hands-on (the editor TU is not in a harness). |
> | WR-07 | ✅ Resolved in **v1.0.0** | Drop cap lowered to 16 MiB on both sides (`WavetableImporter::kMaxMemoryBytes`, page `DROP_MAX_BYTES`); message-thread cost ≈ 170 ms (was ≈ 1.03 s at 96 MiB); the Import button streams from disk and has no cap. `import.err.tooLarge` → "too large to drop — use the Import button"; a specific processor error is kept for the drop. **G-DROP-CAPSYNC** (negative control: page at 96 → mismatch), G-DROP[b] (22,369,625 chars), UI state 7. |
> | WR-08 | ✅ Resolved in **v1.0.0** | One burst-aware wheel accumulator for the stepped knobs, the bank stack and the continuous knobs (W3b): the first event of a burst steps once; later pixel events step every `WHEEL_PX_PER_NUDGE` (100 px), at most 4 per event. **G-S3W3-WHEEL** 6/6 (negative control: the Stage 3 page fails 5 arms). |
> | WR-09 | ✅ Resolved in **v1.0.0** | Processor-side `pollBankForImportStatus()` on the 250 ms timer clears an `error` status when the bank leaves its last value (`lastStatusBank` seeded in the ctor and after `setStateInformation`); works with the editor closed. **G-S3W4-ERRCLEAR** (control, restore and sensitivity arms). |
> | WR-10 | ✅ Resolved in **v1.0.0** | The processor tracks `uiHeld[128]` in `handleUiMidi`; the editor destructor calls `releaseUiHeldNotes()` before `stopTimer()`; the page sends all-notes-off on `visibilitychange` → hidden. **G-S3W5-UIHELD** (negative control: still sounding). |
> | IN-01 | Accepted (by design) | **Poly↔Mono switch ends sounding notes.** Changing Voice Mode hard-stops every voice; each voice's last sample decays over a 2 ms raised-cosine tail (W1, Amendment 13), so the switch is click-free but held notes do not carry into the new mode. Accepted: the switch is user-initiated, and a carried note has no single correct owner (16 Poly voices → 1 Mono voice). Gate: G-SWITCH-TAIL 0.656× (negative control 20.0×). |
> | IN-02 | Accepted (by design) | **Interpolation Off steps the timbre once per cycle.** With Interpolation Off the frame index latches at each cycle wrap (ARCHITECTURE row 15), so modulated Position steps cycle by cycle; on the sine-phase built-ins the step lands at a zero crossing, on Formant and Imported frames it can tick. Accepted: the stepped sound is the lesson the switch teaches; Interpolation On removes it. |
> | IN-03 | ✅ Resolved in **v1.0.0** | Byte-capped MIDI chunk fill: a chunk ends before an event that would overflow the reserve, so the copy never grows the buffer. **G-MIDI-FLOOD** (6,000 events in one block, then a +2 st wheel) −0.024 c; `--alloc-check` 0 with the flood blocks (negative control: guard off → allocations, 1 / 1 arms). |
> | IN-04 | ✅ Resolved in **v1.0.0** | `std::atomic<bool> prepared`, set as the last statement of `prepareToPlay`; `processBlock` renders only when it is set. Not cleared in `releaseResources` (a host that processes after release keeps sound). **G-UNPREPARED** arms a / b / c (negative control: guard off → SIGSEGV). |
> | IN-05 | Reviewed, not changed | **Mono ignores the sustain pedal.** Mono is true-legato last-note priority and handles note, all-notes and pitch-wheel messages only; Poly honours CC64 through the JUCE Synthesiser. Pedal semantics in Mono are a design choice deferred to a later version. **Side note:** CC64 pressed in Mono never reaches the Synthesiser, so after a Mono → Poly switch with the pedal held, Poly reads the pedal as up until it is pressed again. |
> | IN-06 | Reviewed, not changed | **Imports cannot be cancelled from the UI.** A newer import or a state restore supersedes an in-flight job, and the destructor cancels it. A full 256-frame import costs about 20 ms on the worker (measured), so there is nothing for a cancel to save. |
> | IN-07 | Reviewed, not changed | **Save/restore and `bankStateLock`.** Restore decodes before taking the lock and locks only to publish (microseconds); the audio thread never takes this lock. |
> | IN-08 | ✅ Resolved in **v1.0.0** | `Source/TestHooks.h` holds the `OSIW_TEST_HOOKS` default and is included first by every header that tests it. **R-UNDEF** 0 lines (negative control, pre-fix: WtVoice 2 / PositionSmoother 1 / PositionLfo 5). |
> | IN-09 | ✅ Resolved in **v1.0.0** | Editor member `std::unique_ptr<juce::FileChooser> importChooser` + `importDialogInFlight`; the callback clears only the flag, first. Task 14 hands-on (double-click opens one dialog). |
> | IN-10 | Reviewed, not changed | **Display atomics can tear for one tick.** The lead-voice snapshot is 8 independent relaxed atomics; the 30 Hz reader may combine two consecutive blocks for one 33 ms frame. The next tick corrects it; there is no audio effect. A seqlock is not worth an audio-thread protocol for a one-frame cosmetic glitch. |
> | IN-11 | Reviewed, not changed | **`getBankThumbnails` reads `importedOwner` directly under `bankStateLock`.** Intended (Amendment 18, D-S): the bank and its filename must come from one lock scope; `getImportedBankSnapshot()` takes the same lock and returns the same `shared_ptr`. |
> | IN-12 | ✅ Resolved in **v1.0.0** | `resetDisplayState (fs)` called from both `prepareToPlay` and `releaseResources`. **G-VIZ-RELEASE**: after `releaseResources()` sounding false, note −1, hz 0, amp 0, silent payload (negative control: still sounding). |
> | IN-13 | ✅ Resolved in **v1.0.0** | The single apply core clears `pendingAutoSelect` under `bankStateLock` before it sets any parameter. **G-S3N5** (sensitivity arm: without the apply the bank goes to Imported). |
> | IN-14 | Accepted (by design) | **A lesson clicked during Mono playback ends the note through the 2 ms tail.** Every lesson resets unlisted parameters to their defaults, including Voice Mode → Poly, so it is a Poly↔Mono switch (IN-01). Accepted for the same reason. |
> | IN-15 | ✅ Resolved in **v1.0.0** | `WavetableImporter::isStrippedNameChar` strips C0, DEL, C1, the 12 `Bidi_Control` characters, U+2028 / U+2029, U+200B / U+2060 / U+FEFF, lone surrogates, U+FFFE / U+FFFF, > U+10FFFF and tag characters; keeps ZWJ / ZWNJ. Covers the file, JS and restored-state names. **G-SANITISE** (per-class negative control: the legacy predicate keeps each stripped code point). |
> | IN-16 | ✅ Resolved in **v1.0.0** | Page `gestureBegin (id)` / `gestureEnd (id)` per-id refcount used by `bindKnob`, `nudge`, `resetToDefault` and `bindBankDrag`. **G-S3W3-WHEEL** N8 arm (`position` drag-started / ended strictly alternate). |
> | IN-17 | ✅ Resolved in **v1.0.0** | `renderImportError` `unsupported` branch + `import.err.unsupported` in 3 languages. UI state 6; check-i18n. |
> | IN-18 | ✅ Resolved in **v1.0.0** | New `presetState { name, id, factory, modified }` event; the page lights a lesson iff its id matches and the preset is unmodified. **G-PRESET-STATE**; UI states 1 / 2 / 8; Task 14 hands-on. |
> | IN-19 | ✅ Resolved in **v1.0.0** | TIP_BINDINGS `#octDown`, `#octUp`. boot-all-uis `--strict-tips` 0 DEAD / 0 late; check-i18n. |
> | IN-20 | ✅ Resolved in **v1.0.0** | Page-only `'−inf'` (U+2212); the host text `"-inf"` and its parser stay ASCII (state-check P0 unchanged). check-i18n. |
> | IN-21 | ✅ Resolved in **v1.0.0** | Processor `stepKnobGesture (id, phase, index)` + `closeStepKnobGestures()`; the page brackets a pointer drag as one begin / move / end; the editor destructor closes open gestures. **G-S3N13** 1 begin + 1 end (negative control: 3 pairs); the probe's N13 arm. |
> | WR-04 residual | Reviewed, not changed | If a second crossfade trigger (bank, interp, band-limit or another level change) lands within 5 ms of a level-crossing pitch change, the unfinished part of the first fade continues at the newest pitch (the pre-v1.0.0 behaviour for that remainder). Measured not worse than before (magnifier −24.3 vs −24.5 dB; click ratio 1.000 vs 1.765). |
> | IN-03 adjacent | Reviewed, not changed | UI keyboard notes are merged into the host-owned MIDI buffer by `MidiMessageCollector`; that buffer's capacity belongs to the host or wrapper. |

## Upstream (not plugin code; not called)

Defects in the suite module `modules/persistence/preset-manager` v1.0.9. The plugin uses
the module only for the Factory / User folders, `savePreset (name)`, `deletePreset (name)`,
`initializeFactoryPresets` with its `.factory-version` sentinel, and `preset-manager.js`.
It keeps its own `getStateInformation` / `setStateInformation` and its own apply core. A
static check (R-STATIC S10) confirms none of the paths below is called. `modules/` is
outside this plugin's commit scope, so these are logged here, not fixed.

| Location | Defect | Why it does not reach this plugin |
|----------|--------|-----------------------------------|
| `OuariconPresetManager.h:617-644` (`setStateFromXml`) | A stale `<CustomState>` child can shadow a later reopen (suite memory `critical_preset_manager_stale_customstate_child`). | The module's XML state pair is never called. |
| `OuariconPresetManager.h:360-376` (JSON apply) | The apply resets every parameter to its default and then sets every key in the file, including `output_level`. Suite presets must never set Output Level. | The plugin applies user JSON through its own core, which skips `output_level` and ignores unknown keys (G-PRESET-USER); factory presets come from `PresetRecipes.h`, never the disk JSON. |
| `OuariconPresetManager.h:217` | `std::map` used without `#include <map>`; it relies on JUCE's transitive include. | Compiles today; a JUCE include change would break every consumer. |
| Module-header warnings (R-DBG) | 0 | — |
