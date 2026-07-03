---
group: GROUP-A-processor-voice
reviewed: 2026-07-01T14:21:34Z
depth: deep
files_reviewed: 4
files_reviewed_list:
  - plugins/O-Formant/Source/PluginProcessor.cpp
  - plugins/O-Formant/Source/PluginProcessor.h
  - plugins/O-Formant/Source/FormantVoice.cpp
  - plugins/O-Formant/Source/FormantVoice.h
findings:
  critical: 1
  warning: 2
  info: 4
  total: 7
status: issues
---

# O-Formant Group A (Processor + Voice) — Code Review

**Reviewed:** 2026-07-01T14:21:34Z
**Depth:** deep
**Files Reviewed:** 4
**Status:** issues_found

## Summary

The real-time audio path is largely solid. The two hottest concerns I looked
for — heap allocation and unsynchronized shared state on the audio thread — mostly
check out:

- `ScopedNoDenormals`, `buffer.clear()` first, additive voice render, tanh soft-clip,
  and a brickwall limiter are all present and correct.
- `TuningEngine::getFrequency()` reads a lock-free `std::array<std::atomic<double>,128>`
  frequency table (verified in `TuningEngine.cpp`); the message thread rebuilds it under
  `intervalMutex` and the audio thread never touches `scaleIntervals`. No race there.
- `LyricsEngine::advanceAndGet()` uses a non-blocking `ScopedTryLock` + atomics — RT-safe.
- The Note-Expression `PendingTuningTable` is atomic and drained on the audio thread only;
  `applyPendingTuning`'s `exchange(0.0)` slot-consumption is correct and matches the doc.
- `setLatencySamples(0)` (not an illegal `getLatencySamples` override) is the correct
  JUCE 8 idiom; voice APVTS pointers are cached once in `setAPVTS`.

The material finding is that **pitch bend is completely non-functional** — the per-note
pitch dimension of MPE and the standard bend wheel produce no audible pitch change, and a
code comment actively claims otherwise. Two state-restore gaps and some minor items follow.

---

## Critical Issues

### CR-01: Pitch bend (standard MIDI + MPE per-note) is silently ignored

**File:** `plugins/O-Formant/Source/FormantVoice.cpp:331-334`, `660-665`; `dsp/PitchGlide.h:47-58`
**Issue:**
`notePitchbendChanged()` is an empty stub whose comment claims *"Pitchbend handled
per-sample via getCurrentlyPlayingNote().getFrequencyInHertz()"* — but nothing in the
render path ever reads `getFrequencyInHertz()` per sample. The sounding pitch is driven
entirely by:

```cpp
float baseF0 = pitchGlide.getNextFrequency();   // line 661
```

and `pitchGlide`'s target is set **only once** at note-on (`FormantVoice.cpp:207-213`):

```cpp
pitchGlide.setTarget (f0);  // or snapTo — f0 = tunedF0 from the tuning engine
```

`PitchGlide::getNextFrequency()` just ramps `currentFreq → targetFreq`; it has no pitch-bend
input. Because `notePitchbendChanged()` never calls `pitchGlide.setTarget()` (nor
`tuningEngine.setPitchBend()`), moving the bend wheel or an MPE per-note glide has **zero
effect** on the emitted pitch.

**Failure scenario:** Play a note, move the pitch-bend wheel (legacy mode is configured for
±2 st at `PluginProcessor.cpp:704`) or send MPE per-note pitch bend from a Roli/Linnstrument.
Expected: pitch slides. Actual: pitch stays fixed at the note-on frequency. Note the plugin's
*primary* microtonal path (TuningEngine + VST3 Note Expression from Dorico) is unaffected —
only continuous bend is dead — but MPE pitch is explicitly an advertised feature.

**Fix:** Re-read note pitch per sample (or per coeff-update block) and fold it into the glide
target. Minimal version, in `renderNextBlock` before/at line 661:

```cpp
// Compose tuned base with live MPE/standard pitch bend.
// Tuning engine gives the untuned-to-tuned ratio at note-on; apply the bend
// multiplicatively so microtonal + bend stack correctly.
const float midiHz   = (float) getCurrentlyPlayingNote().getFrequencyInHertz(); // includes bend
const float noteOnHz = (float) currentlyPlayingNote.getFrequencyInHertz();      // bend-free ref
const float bendRatio = (noteOnHz > 0.0f) ? midiHz / noteOnHz : 1.0f;
pitchGlide.setTarget (tunedF0 * bendRatio);
float baseF0 = pitchGlide.getNextFrequency();
```

and drive `pitchGlide.setTarget(...)` from `notePitchbendChanged()` as well (or simply set the
target every block). Delete the misleading comment at line 333 once implemented. Verify the
composition order matches the intended TuningEngine → NE → bend → glide stacking.

---

## Warnings

### WR-01: State round-trip loses master-tune, octave-stretch, tuning-mode, and built-in temperament when no editor is open

**File:** `plugins/O-Formant/Source/PluginProcessor.cpp:856-926`
**Issue:** `getStateInformation` persists `tuningEngine.getBuiltInPreset()` into the `"preset"`
property (line 872), but `setStateInformation` **never reads it back** — there is no
`setBuiltInPreset` call in the restore path (only `setCustomIntervals` + `setTonicNote`,
lines 908-913). Worse, the TuningEngine's `masterTune`, `octaveStretch`, `pitchBendRange`,
`mode`, and built-in preset are only pushed into the engine from WebView native functions on
the **message thread / editor** (`PluginEditor.cpp:314-353`). When a project is reloaded
headless — offline bounce/render with the editor never opened — the restored APVTS values and
saved tuning properties never reach `TuningEngine`, so it renders with its defaults
(A=440, stretch=1.0, Equal 12-TET).

**Failure scenario:** Save a session using Werckmeister III at A=442. Reopen the project and
bounce offline without opening the plugin UI → the render comes out in 12-TET at A=440. Custom
`.scl` intervals survive (restored directly at line 909) but named temperaments and master
tune do not.

**Fix:** In `setStateInformation`, after `replaceState`, push the restored values straight into
the engine instead of relying on the editor:

```cpp
tuningEngine.setBuiltInPreset (static_cast<TuningEngine::BuiltInPreset> (
    (int) tuningState.getProperty ("preset", 0)));
tuningEngine.setMasterTune   (parameters.getRawParameterValue ("tuning_masterTune")->load());
tuningEngine.setOctaveStretch(parameters.getRawParameterValue ("tuning_octaveStretch")->load());
tuningEngine.setPitchBendRange(parameters.getRawParameterValue ("tuning_pitchBendRange")->load());
// then rebuild happens inside those setters
```

(Apply built-in preset *before* custom intervals so a custom `.scl` still wins when present.)

### WR-02: `onVst3RawEvent` can heap-allocate on the audio thread under dense Note Expression

**File:** `modules/tuning/note-expression/cpp/NoteExpression.h:167-173` (invoked from the audio path via `PluginProcessor.cpp:750 drainAndUpdate`)
**Issue:** `onVst3RawEvent` does `blockEvents.push_back(e)` and is documented as being called
on the audio thread just before `processBlock`. The buffer is `reserve(64)`'d, but a block that
delivers more than 64 raw VST3 events (dense Dorico microtonal chords/clusters, each NoteOn
plus its kTuningTypeID NE, at small buffer sizes) will trigger a `std::vector` reallocation —
i.e. a heap allocation on the audio thread, exactly what RT code must avoid.

**Failure scenario:** A thick divisi passage in Dorico with >32 simultaneous notes (2 events
each) at a 64-sample buffer overruns the reserve and reallocates mid-`process`, risking a
dropout under load.

**Fix:** Bound the buffer and drop overflow (NE beyond capacity degrades gracefully to no
retune) rather than reallocate:

```cpp
void onVst3RawEvent (const Vst3RawEvent& e) override {
    if (blockEvents.size() < blockEvents.capacity())
        blockEvents.push_back (e);   // capacity fixed by reserve(); never reallocates
}
```

or raise the reserve to a hard worst-case (128 notes × 2) and assert on overflow. This is module
code shared across plugins — fix upstream.

---

## Info

### IN-01: Variable shadowing of `midiNote` in `noteStarted`

**File:** `plugins/O-Formant/Source/FormantVoice.cpp:187` vs `225`
**Issue:** Line 187 declares `int midiNote = currentlyPlayingNote.initialNote;`. Inside the Rd
block at line 225 a second `float midiNote = static_cast<float>(...)` shadows it. Both happen to
hold the same note number so behavior is correct, but the shadow is a readability/maintenance
hazard.
**Fix:** Rename the inner one (e.g. `midiNoteF`) or reuse the outer via a cast.

### IN-02: `processBlock` does ~25 string-keyed APVTS lookups per block instead of caching pointers

**File:** `plugins/O-Formant/Source/PluginProcessor.cpp:759-816`
**Issue:** The effects/output section calls `parameters.getRawParameterValue("...")->load()`
repeatedly every block. `getRawParameterValue(StringRef)` is `noexcept` and does not allocate
(no `juce::String` is constructed from the literals), so this is not an RT violation — but it is
an O(log n) map lookup per parameter per block and is inconsistent with `FormantVoice::setAPVTS`,
which correctly caches every `std::atomic<float>*` once.
**Fix:** Cache the effect/output `std::atomic<float>*` pointers in `prepareToPlay` (or the ctor)
and `->load()` them, matching the voice pattern.

### IN-03: NaN/Inf guard resets filters but not the excitation sources

**File:** `plugins/O-Formant/Source/FormantVoice.cpp:739-747`
**Issue:** The `!std::isfinite(sample)` guard resets `filterBank`, `cascadeBank`,
`nasalPoleZero`, and `consonantEngine`, but not `glottalSource`, `aspirationNoise`, or
`pitchGlide`. If a non-finite value originates upstream (e.g. `finalF0` going Inf from a bad
`std::pow`), the source can keep re-injecting NaN on subsequent samples even though the filters
were cleared.
**Fix:** Also `glottalSource.reset()` / `aspirationNoise.reset()` in the guard, and/or clamp
`finalF0` to a sane range before `glottalSource.setFrequency(finalF0)` at line 665.

### IN-04: Include-path case mismatch (`DSP/` vs on-disk `dsp/`)

**File:** `plugins/O-Formant/Source/PluginProcessor.h:16-18`
**Issue:** These include `"DSP/DelayProcessor.h"`, `"DSP/EQProcessor.h"`,
`"DSP/ReverbProcessor.h"` (uppercase), while the git-tracked files live under `Source/dsp/`
(lowercase, confirmed via `git ls-files`) and every other include uses `dsp/`. This compiles on
case-insensitive macOS/Windows filesystems but would fail to resolve on a case-sensitive
filesystem (e.g. some Linux CI).
**Fix:** Normalize to lowercase `dsp/` to match the tracked paths and the rest of the codebase.

---

_Reviewed: 2026-07-01T14:21:34Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
