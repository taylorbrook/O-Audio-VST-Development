---
phase: O-simpleBeatmaker-v1.0.0
reviewed: 2026-07-15
depth: deep
files_reviewed: 21
files_reviewed_list:
  - plugins/O-simpleBeatmaker/Source/PluginProcessor.cpp
  - plugins/O-simpleBeatmaker/Source/PluginProcessor.h
  - plugins/O-simpleBeatmaker/Source/PluginEditor.cpp
  - plugins/O-simpleBeatmaker/Source/PluginEditor.h
  - plugins/O-simpleBeatmaker/Source/DrumVoiceEngine.h
  - plugins/O-simpleBeatmaker/Source/SequencerClock.h
  - plugins/O-simpleBeatmaker/Source/TimingFeelEngine.h
  - plugins/O-simpleBeatmaker/Source/UnifiedTriggerRouter.h
  - plugins/O-simpleBeatmaker/Source/VizAnalyzer.h
  - plugins/O-simpleBeatmaker/Source/BeatPresets.h
  - plugins/O-simpleBeatmaker/Source/BeatmakerIDs.h
  - plugins/O-simpleBeatmaker/Source/fastSine.h
  - plugins/O-simpleBeatmaker/Source/ui/public/index.html
  - plugins/O-simpleBeatmaker/Source/ui/public/css/styles.css
  - plugins/O-simpleBeatmaker/Source/ui/public/js/app.js
  - plugins/O-simpleBeatmaker/Source/ui/public/js/juce/index.js
  - plugins/O-simpleBeatmaker/CMakeLists.txt
  - plugins/O-simpleBeatmaker/CHANGELOG.md
  - plugins/O-simpleBeatmaker/tests/render-harness/CMakeLists.txt
  - plugins/O-simpleBeatmaker/tests/render-harness/main.cpp
findings:
  critical: 2
  warning: 3
  info: 9
  total: 14
status: resolved
resolved: 1.0.1 (CR-01, CR-02, WR-01, WR-02, WR-03); 1.0.2 (IN-01..IN-09)
verified: 2026-07-15
---

# O-simpleBeatmaker v1.0.0: Code Review Report

**Reviewed:** 2026-07-15
**Depth:** deep (cross-file: audio/message-thread interactions, WebView bridge, sequencer timing, presets/state, build config)
**Files Reviewed:** 21
**Status:** issues_found

## Summary

O-simpleBeatmaker is a pedagogical 6-voice synthesized step-sequencer drum machine
(custom atomic 6×32 grid + 42-param APVTS, sample-accurate host-synced sequencer with
a swing/humanize/quantize feel engine, lock-free VizAnalyzer, single-page WebView UI,
offline render-harness gate).

Several of this codebase's recurring failure modes are handled **correctly** here:

- **WebView bridge completeness** — grep-diff is clean: all 5 JS `getNativeFunction`
  calls (`setStep`, `getGrid`, `clearGrid`, `applyPreset`, `getSampleRate`,
  `app.js:574-578`) have matching `withNativeFunction` registrations
  (`PluginEditor.cpp:101-131`). Knob readouts use `SliderState.getScaledValue()`
  (`app.js:112`), never a hardcoded JS min/max map. The vendored
  `js/juce/index.js` is byte-identical to the JUCE 8.0.9 upstream.
- **Factory presets** — `applyConceptPreset` converts engineering-unit values through
  `convertTo0to1` (`PluginProcessor.cpp:451-464`); no raw-normalized authoring
  (though it applies partially — see WR-01).
- **Build config** — `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`
  (`CMakeLists.txt:23,97`), Windows `withUserDataFolder` (`PluginEditor.cpp:133-143`),
  single binary-data target (no namespace collision), and the render-harness correctly
  excludes `PluginEditor.cpp` with `JUCE_WEB_BROWSER=0` + a guarded `createEditor`
  fallback (`PluginProcessor.cpp:522-533`, `tests/render-harness/CMakeLists.txt:41`).
- **State round-trip** — grid persists via a `PATTERN` child, restore goes through the
  same atomics the audio thread reads, live APVTS tree kept PATTERN-free
  (`PluginProcessor.cpp:536-564`); the UI's 4 Hz `getGrid` poll picks up host restores.
- **RT-safety fundamentals** — no locks/logging in `processBlock`; fastSine LUT warmed
  and RNG/noise pre-seeded in `prepareToPlay`; viz handoff is a true SPSC
  `AbstractFifo`; editor destruction order (relays → webview → attachments) is correct.
- **Sequencer timing** — the stateless half-open ppq-window enumeration, carry-over
  queue, and discontinuity handling are sound; the harness's six probes genuinely
  exercise block-boundary and DSP-04 invariants.

The two Critical findings are in the voice engine: **unbounded float phase
accumulators** that audibly corrupt the tonal voices within minutes-to-tens-of-minutes
of sustained playback, and a **mono-bus double-add** (+6 dB) in `renderAll`.

## Critical Issues

### CR-01: Unbounded float phase accumulators corrupt Kick/Tom/Snare pitch during long sessions

**File:** `plugins/O-simpleBeatmaker/Source/DrumVoiceEngine.h:105-106` (Kick), `:155-156` (Tom), `:215-216` (Snare)
**Issue:** The tonal voices accumulate phase in a `float` that is never wrapped:

```cpp
phase += twoPi * fInst / (float) fs;      // KickVoice::render — no wrap, ever
```

`fastSine()` wraps its *input* for the LUT (`fastSine.h:42`), but the accumulator
itself grows monotonically for as long as the voice renders. Voices stay `active`
for ~13.8 decay time-constants per trigger (until `ampEnv < 1e-6`), so a snare on
2 & 4 (default decay tc ≈ 0.2 s → ~2.8 s active per hit) keeps its oscillators
running essentially continuously. Snare `ph2` advances at 2π·330 ≈ 2073 rad/s of
active time: after ~30-35 minutes it passes 4×10⁶, where the float ulp (0.5) is
**10× larger than the per-sample increment (0.047)** — increments round to 0 or
0.5 and the tonal body collapses into pitch garbage/silence. The kick (314 rad/s)
and tom (754 rad/s) follow within one long rehearsal/classroom session. The
render-harness cannot catch this (probes render < 20 s).

**Failure scenario:** Leave any pattern with kick/snare/tom playing for a set-length
session (this plugin's exact classroom use case). Snare timbre audibly detunes and
disintegrates after roughly half an hour; kick and tom follow later. Reopening the
project "fixes" it — a classic unreproducible-sounding regression.

**Fix:** Wrap the accumulator in the render loop (the increment is always ≪ 2π, so a
single conditional subtract suffices — no `fmod` needed):

```cpp
phase += twoPi * fInst / (float) fs;
if (phase >= twoPi) phase -= twoPi;
```

Apply to `KickVoice::phase`, `TomVoice::phase`, and `SnareVoice::ph1`/`ph2`.
(HatVoice/ClapVoice are noise-based and unaffected.)

### CR-02: Mono output bus double-adds every voice (+6 dB, likely clipping)

**File:** `plugins/O-simpleBeatmaker/Source/DrumVoiceEngine.h:437-444`; layout accepted at `plugins/O-simpleBeatmaker/Source/PluginProcessor.cpp:180-193`
**Issue:** `isBusesLayoutSupported` explicitly accepts a mono output bus, but
`renderAll` aliases `R` to `L` when there is one channel:

```cpp
float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : L;
```

Every voice then executes `L[start+i] += s; R[start+i] += s;` — with `R == L` that
is **two adds into the same sample**, doubling every voice's output (+6.02 dB) on
mono configurations. With six voices already summing toward full scale at default
0 dB levels, mono renders clip hard. auval's 1-channel test only verifies the plugin
renders, not its gain, so this shipped green.

**Failure scenario:** Instantiate on a mono track (Logic mono instrument strip, mono
bounce, or a host that negotiates mono for a narrow output) — the drum kit is 6 dB
hotter than the stereo instance and distorts on accented hits.

**Fix:** Skip the second add when the buffer is mono. Cheapest without touching every
voice's inner loop: keep the per-voice `L/R` writes but hoist an alias check, e.g.
change each voice's render loop body to

```cpp
L[start + i] += s;
if (R != L) R[start + i] += s;
```

(the branch is perfectly predicted), or have `renderAll` render into a stereo
scratch pair and sum-halve into mono. Add a mono render to the harness asserting
RMS parity with one stereo channel.

## Warnings

### WR-01: applyConceptPreset applies partially — stale mute/solo/level/tune state silently corrupts the lesson

**File:** `plugins/O-simpleBeatmaker/Source/PluginProcessor.cpp:439-473`
**Issue:** The lesson presets set only the 5 timing-feel params and the grid. The
other 37 params — per-voice tune/decay/tone/level and, critically, **mute/solo**
plus `outputLevel` — retain whatever the user last set. This is exactly the
partial-preset failure documented in the project memory pattern
"applyPresetJson must reset all params to defaults before applying."

**Failure scenario:** A student solos the kick (or drags snare Level to −60 dB) while
exploring, then clicks the "Ghost Notes" lesson. The preset loads but the snare —
the entire point of the lesson — is inaudible. Nothing in the UI indicates why; the
teaching moment fails.

**Fix:** In `applyConceptPreset`, before applying the preset values, reset all voice
params (or at minimum all 12 mute/solo toggles and the 6 voice levels + outputLevel)
to their defaults via the same `setValueNotifyingHost(convertTo0to1(default))` path:

```cpp
for (auto* p : getParameters())
    if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        rp->setValueNotifyingHost (rp->getDefaultValue());
```

then stamp the preset's own values on top. (If keeping voice timbre edits across
lessons is intended, at least clear mute/solo and restore levels, and document it.)

### WR-02: getTailLengthSeconds (3.0 s) underreports the max kick / open-hat tail — offline bounces truncate

**File:** `plugins/O-simpleBeatmaker/Source/PluginProcessor.h:55`; decay mappings at `plugins/O-simpleBeatmaker/Source/DrumVoiceEngine.h:79` (kick, tc up to 1.2 s) and `:335` (open hat, tc up to 0.7 s)
**Issue:** The envelopes are exponential with the *time constant* set by the decay
param, and a voice renders until `env < 1e-6` (≈13.8 tc). At max kick decay
(tc = 1.2 s) the level is still −21.7 dB at the reported 3.0 s tail
(e^(−3/1.2) ≈ 0.082); the −60 dB point is ≈ 8.3 s. Open hat at max decay is
similar (−60 dB ≈ 4.8 s).

**Failure scenario:** A region ends on a max-decay kick (the "50 ms .. 1.2 s boom"
the comment advertises). An offline bounce honors the 3 s tail and cuts the boom at
−22 dB — an audible truncation/click in the render that never happens live.

**Fix:** Return the worst-case audible tail: `return 9.0;` (13.8 × 1.2 s ≈ 16.5 s to
the internal floor is overkill; ~7 tc to −60 dB → 8.3 s, round up), or compute it
from the current max decay params if you want it tight.

### WR-03: sequencerMidi can reallocate on the audio thread when host MIDI exceeds the 4 KB reserve

**File:** `plugins/O-simpleBeatmaker/Source/PluginProcessor.cpp:134` (reserve), `:357` (merge)
**Issue:** `sequencerMidi.ensureSize (4096)` reserves ~4 KB (≈450 note-ons at ~9
bytes each), but `sequencerMidi.addEvents (midiMessages, 0, numSamples, 0)` copies
**every** host event — including CCs, and any SysEx. A single multi-KB SysEx dump
(patch librarian, MIDI-learn burst, controller firmware chatter) or a pathologically
dense block blows past the reserve and `MidiBuffer` grows its heap storage inside
`processBlock` — a violation of the plugin's own PERF-01 "zero alloc in
processBlock" contract.

**Failure scenario:** User's controller emits a SysEx identity dump while the
transport runs at a small buffer size → malloc on the audio thread → possible
priority-inversion glitch precisely when the host is busiest.

**Fix:** Filter the merge to the messages the router can consume (note-ons are all
`renderMerged`/`handleTrigger` ever act on):

```cpp
for (const auto meta : midiMessages)
    if (meta.getMessage().isNoteOn())
        sequencerMidi.addEvent (meta.getMessage(), meta.samplePosition);
```

and/or raise the reserve (e.g. 16384) as belt-and-braces.

## Info

### IN-01: Host-MIDI viz events are not gated by mute/solo (sequencer viz is)

**File:** `plugins/O-simpleBeatmaker/Source/PluginProcessor.cpp:342-356`
**Issue:** Sequencer hits are gated by `router.isVoiceAudible` *before* the viz push
(`:320`), but host note-ons push a `VizEvent` unconditionally; `handleTrigger`
then silently drops the muted trigger. The lane draws a dot and the MIDI readout
prints a row for a voice that makes no sound.
**Fix:** Either gate the host viz push with `router.isVoiceAudible(v)` for
consistency, or (if "show the incoming stream" is the pedagogical intent) mark
muted host events visually (e.g. a `muted` flag on the VizEvent → dimmed row).

### IN-02: dbToGain silence threshold (−59.5) disagrees with the UI's −∞ display threshold (−59.95)

**File:** `plugins/O-simpleBeatmaker/Source/DrumVoiceEngine.h:57-60` vs `plugins/O-simpleBeatmaker/Source/ui/public/js/app.js:54`
**Issue:** With the 0.1 dB step, values −59.9…−59.6 render as hard silence in C++
(`db <= -59.5f → 0`) while the knob readout shows a finite "−59.6 dB".
**Fix:** Align the thresholds (use −59.95 in `dbToGain`, matching
`Decibels::decibelsToGain (db, -60.0f)` semantics, or display −∞ from −59.5).

### IN-03: Free-run playhead phase transiently exceeds a shrunken pattern length

**File:** `plugins/O-simpleBeatmaker/Source/SequencerClock.h:127-129`
**Issue:** `freeRunStepPos` is wrapped *after* enumeration with the new
`patternLength`; on a 32→8 change `playheadPhaseOut` can report e.g. 30 on an
8-step grid for one block. The JS guards with a modulo (`app.js:374,453`), and
`renderGridColumns` drops the `playhead` class until the column next changes —
a one-step visual hiccup.
**Fix:** Wrap `startPos` against `barLenSteps` at the top of the free-run branch
(and reset `lastPhaseCol = -1` in `renderGridColumns`).

### IN-04: Sample rate snapshot is fetched once at boot and read non-atomically

**File:** `plugins/O-simpleBeatmaker/Source/PluginEditor.cpp:129-131`, `plugins/O-simpleBeatmaker/Source/ui/public/js/app.js:578`, `plugins/O-simpleBeatmaker/Source/PluginProcessor.h:82,144`
**Issue:** JS caches `getSampleRate()` at boot; if the host switches sample rate (or
the editor opens before the first `prepareToPlay`, reading the 44100 default) the
lane's Δt-in-steps scale is wrong until the editor reloads. `currentSampleRate`
is also a plain `double` read cross-thread (benign on arm64/x64, still untidy).
**Fix:** Include `sr` in the per-frame "frame" event (it already carries bpm), and
make `currentSampleRate` a relaxed `std::atomic<double>`.

### IN-05: Knobs have no double-click-to-default

**File:** `plugins/O-simpleBeatmaker/Source/ui/public/js/app.js:129-173`
**Issue:** Every other recent plugin in the suite grew a `getParameterDefaults`
native fn + dblclick reset (project pattern from O-MicrotonalSampler v1.23.7);
here a mis-dragged tempo/tune knob can only be restored by eye.
**Fix:** Register a `getParameterDefaults` native fn and add a `dblclick` handler
that sets the normalized default.

### IN-06: Synced step enumeration misses steps if one block spans more than a full pattern period

**File:** `plugins/O-simpleBeatmaker/Source/SequencerClock.h:80-99`
**Issue:** The candidate loop covers `bar ∈ {−1, 0, +1}` around `barStart`. If
`blockPpq > barLenPpq` (worst case: 8-step pattern = 2 ppq, host bpm ≫ 240 with a
16k+ buffer) steps past `barStart + 2·barLenPpq` fall inside the window but are
never enumerated. Unreachable with sane hosts, but the free-run path has no such
ceiling, so the asymmetry is silent.
**Fix:** Derive the upper bar bound from the window:
`const int barsSpanned = 1 + (int) std::ceil (blockPpq / barLenPpq);` and loop
`bar <= barsSpanned`.

### IN-07: Knob drag has no pointer capture / pointercancel handling

**File:** `plugins/O-simpleBeatmaker/Source/ui/public/js/app.js:152-171`
**Issue:** The drag relies on window-level `pointerup`; a `pointercancel` (pen/touch,
OS gesture interruption) never fires `onUp`, leaving `dragging = true` and
`sliderDragStarted` without its matching `sliderDragEnded` (an open host
automation gesture) until the next pointerup anywhere.
**Fix:** `knob.setPointerCapture(e.pointerId)` on pointerdown and register `onUp`
for `pointercancel` too.

### IN-08: 4 Hz grid poll can transiently revert a cell clicked mid-round-trip

**File:** `plugins/O-simpleBeatmaker/Source/ui/public/js/app.js:305-319,474-481`
**Issue:** If a `getGrid` request is dispatched and the user toggles a cell before
the response arrives, the stale snapshot overwrites the local `gridState` and
repaints the cell off; the next poll (≤250 ms later) flips it back. A brief
visible flicker; the C++ state is never wrong.
**Fix:** Track a `lastLocalEditTime` and skip applying poll results within ~300 ms
of a local edit (or version the grid with an edit counter native-side).

### IN-09: Host-MIDI viz loop constructs a MidiMessage for every event, including SysEx (post-1.0.2 finding)

**File:** `plugins/O-simpleBeatmaker/Source/PluginProcessor.cpp:346-348`
**Issue:** The viz readout loop calls `meta.getMessage()` before any filtering, so a
multi-KB SysEx constructs a heap-backed `juce::MidiMessage` on the audio thread —
the same allocation class WR-03 fixed in the merge loop directly below it.
**Fix:** Apply the WR-03 raw-byte gate before touching the message:
`meta.numBytes == 3 && (meta.data[0] & 0xF0) == 0x90 && meta.data[2] != 0`, then
read note/velocity from `meta.data[1]`/`meta.data[2]` directly (no `MidiMessage`
construction at all). Behavior for note-ons is unchanged — `isNoteOn()` already
excluded velocity-0.

---

_Reviewed: 2026-07-15_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
