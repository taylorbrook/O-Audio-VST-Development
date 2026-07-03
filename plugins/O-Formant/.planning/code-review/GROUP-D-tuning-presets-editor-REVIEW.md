---
phase: GROUP-D-tuning-presets-editor
reviewed: 2026-07-01T00:00:00Z
depth: deep
files_reviewed: 11
files_reviewed_list:
  - plugins/O-Formant/Source/TuningEngine.cpp
  - plugins/O-Formant/Source/TuningEngine.h
  - plugins/O-Formant/Source/TuningExporter.cpp
  - plugins/O-Formant/Source/TuningExporter.h
  - plugins/O-Formant/Source/ScaleGenerator.cpp
  - plugins/O-Formant/Source/ScaleGenerator.h
  - plugins/O-Formant/Source/EmbeddedTunings.cpp
  - plugins/O-Formant/Source/EmbeddedTunings.h
  - plugins/O-Formant/Source/OuariconPresetManager.h
  - plugins/O-Formant/Source/PluginEditor.cpp
  - plugins/O-Formant/Source/PluginEditor.h
findings:
  critical: 4
  warning: 5
  info: 5
  total: 14
status: issues_found
---

# GROUP D: Tuning / Presets / Editor — Code Review Report

**Reviewed:** 2026-07-01
**Depth:** deep
**Files Reviewed:** 11
**Status:** issues_found

## Summary

Reviewed the microtonal tuning engine (Scala/KBM parsing, frequency lookup, scale
generators, embedded tunings, HTML export), the JSON preset manager, and the WebView
editor bridge. Cross-referenced all 36 JS `getNativeFunction` calls against the C++
`withNativeFunction` registrations — **every JS call is backed by a C++ registration**,
so there are no silently-dead controls from the bridge-mismatch class.

The audio-thread frequency path is **RT-safe**: `getFrequency()`
(TuningEngine.cpp:714, called from `FormantVoice.cpp:189` on the audio thread) reads
only the pre-computed `std::atomic<double>` frequency table and the atomic pitch-bend
array — no locks, no allocation. All `.scl`/`.kbm` parsing and table rebuilds happen
on the message thread. Good.

However there are **four Critical defects**: (1) an unbounded heap allocation driven by
a `.kbm` header field that crashes on a malformed file, (2) every embedded tuning loads
without its period, so all 24 factory tunings are mistuned, (3) preset names are used
verbatim as filenames — the known `/` path-separator bug plus a `..` path-traversal
write/read primitive, and (4) editing a single interval silently wipes any 12-entry
scale back to 12-TET.

## Critical Issues

### CR-01: Malformed .kbm map-size triggers unbounded allocation → crash

**File:** `plugins/O-Formant/Source/TuningEngine.cpp:533, 549, 562`
**Issue:** `newMapSize` is read straight from the file with `getIntValue()` and is never
range-validated (unlike firstNote/lastNote/middle/ref, which are `jlimit`ed at 542-545).
`mappingCount` is then set to `newMapSize` (549) and the fill loop at 562 pushes entries
until `newMapping.size() < mappingCount`. A `.kbm` whose first data line is e.g.
`2000000000` forces an attempt to allocate ~2 billion `int`s → `std::bad_alloc` / crash.
`.kbm` files are user-supplied and parsed on load, so this is a trivially reachable
parser crash / DoS.

```cpp
// After: int newMapSize = dataLines[0].getIntValue();
newMapSize = juce::jlimit(0, 512, newMapSize);   // KBM maps are tiny in practice
...
int mappingCount = (newMapSize > 0) ? newMapSize : 12;
mappingCount = juce::jlimit(1, 512, mappingCount);
// also clamp newOctaveDegree before use at line 575
```

### CR-02: Embedded tunings load without their period → all 24 factory tunings mistuned

**File:** `plugins/O-Formant/Source/PluginEditor.cpp:552-566` (with
`TuningEngine::setCustomIntervals`, TuningEngine.cpp:240-269)
**Issue:** `EmbeddedTuning::intervals` explicitly **excludes** the period
(EmbeddedTunings.h:30 — "Cents from unison (excluding period)"; the period is stored
separately in `EmbeddedTuning::period`). `loadEmbeddedTuning` calls
`setCustomIntervals(tuningData->intervals, tuningData->name)` and **never passes /
appends `tuningData->period`**. `setCustomIntervals` does not append a period either, so
`scaleIntervals.back()` ends up being the last real scale degree. Both the C++ frequency
math (`calculateCustomFrequency` uses `activeIntervals.back()` as the period,
TuningEngine.cpp:794) and the JS panel (tuning-panel.js:403/489/514 use
`intervals[last]` as period) then treat that last degree as the octave.

Example: `young1799` = `{0 … 1091.7}` (12 values). After load the "period" becomes
1091.7 c and a degree is lost, so the scale repeats every 1091.7 cents instead of 1200.
Non-octave tunings are catastrophically wrong — Bohlen-Pierce (period 1901.955) loads
with a ~1755 c period; Carlos Alpha (period 1248) loses its defining non-octave period
entirely. Note the built-in-preset path does this correctly (it `push_back`es the period
at TuningEngine.cpp:172-176) — the embedded path is inconsistent with it.

```cpp
.withNativeFunction ("loadEmbeddedTuning", [this] (const auto& args, auto complete) {
    if (args.size() >= 1) {
        auto* tuningData = EmbeddedTunings::getTuningById (args[0].toString().toStdString());
        if (tuningData != nullptr && ! tuningData->intervals.empty()) {
            std::vector<double> withPeriod = tuningData->intervals;
            withPeriod.push_back (tuningData->period);   // append period marker
            processorRef.tuningEngine.setCustomIntervals (withPeriod, tuningData->name);
            complete (juce::var (true));
            return;
        }
    }
    complete (juce::var (false));
})
```

### CR-03: Preset name used verbatim as filename — "/" data loss and ".." path traversal

**File:** `plugins/O-Formant/Source/OuariconPresetManager.h:229, 159, 253, 268, 289, 328`
(name source: `main.js:933` `prompt('Save preset as:')`, passed with only `.trim()`)
**Issue:** `savePreset` builds the path as
`getUserPresetsDirectory().getChildFile(presetName + ".json")` with no sanitization. This
is the documented O-Formant/O-simplePhysicalModelSynth gotcha and worse:
- **`/` in the name** (e.g. "Koto / Harp") → `getChildFile` treats it as a subdirectory
  separator; the `User/` subdir doesn't exist so `replaceWithText` fails → **save
  silently fails** while the UI shows the name as saved.
- **`..` in the name** (e.g. `../../../../tmp/evil`) → `getChildFile` resolves `..`,
  giving an **arbitrary-path file write** primitive on save and, symmetrically, an
  **arbitrary `.json` read** via `loadPreset` (268), `loadPresetFromCategory` (289,
  `category` also unsanitized), and `deletePreset` (328) which deletes arbitrary files.
  `isFactoryPreset` (159) is likewise bypassable.

Preset names originate from an untrusted `prompt()` string, so this is reachable from the
UI. Sanitize before touching the filesystem:

```cpp
static juce::String sanitizePresetName (const juce::String& n) {
    auto s = juce::File::createLegalFileName (n).trim();   // strips / \ .. : etc.
    return s.isEmpty() ? juce::String() : s;
}
// in savePreset/loadPreset/deletePreset/loadPresetFromCategory/isFactoryPreset:
auto safe = sanitizePresetName (presetName);
if (safe.isEmpty()) return false;
auto presetFile = getUserPresetsDirectory().getChildFile (safe + ".json");
```
Apply the same to `category` in `loadPresetFromCategory`.

### CR-04: Editing a single interval silently wipes any 12-entry scale to 12-TET

**File:** `plugins/O-Formant/Source/TuningEngine.cpp:279`
**Issue:** `setSingleInterval` reinitializes the whole scale to 12-TET whenever
`scaleIntervals.size() == 12`:

```cpp
if (scaleIntervals.size() < 2 || scaleIntervals.size() == 12)
{   // reset to 12-TET ...
```

A valid custom scale can legitimately hold exactly 12 stored values — e.g. an 11-EDO
scale (`generateEDO(11,…)` returns 12 entries), or any 12-value scale sent via
`setTuningIntervals`, or a 12-note embedded tuning like `young1799` (12 values). The
first time the user drags/edits one degree of such a scale (tuning-panel.js:322 →
`setSingleInterval`), the entire scale is **discarded and replaced with 12-TET**, then the
single edit is applied on top of 12-TET. Silent data loss on a normal UI action.

The `== 12` heuristic is trying to detect a "no period appended" legacy shape, but it is
indistinguishable from real 12-value scales. Track initialization with an explicit flag
instead of guessing from size:

```cpp
// Only initialize when the scale is genuinely uninitialized:
if (scaleIntervals.size() < 2)
{   /* seed 12-TET */ }
// remove the "|| scaleIntervals.size() == 12" branch entirely
```

## Warnings

### WR-01: Unescaped scale name/description enables HTML/DOM injection

**File:** `plugins/O-Formant/Source/TuningExporter.cpp:392, 398` (and the WebView path:
`getTuningName`/`getActiveTuningName` → tuning-panel.js `innerHTML` sinks, e.g. :297/:536)
**Issue:** `scaleName` derives from the first non-comment line of a `.scl` file
(TuningEngine.cpp:459) and is concatenated unescaped into the exported HTML `<title>` and
`<h1>`. A `.scl` description containing `<script>…</script>` or `"><img onerror=…>` yields
active markup in the exported document, and the same string reaches the plugin's own
WebView via `innerHTML` — where the page holds native functions that can write/delete
files (see CR-03). Escape scale-derived strings before injecting into HTML/DOM. In C++,
HTML-encode `scaleName` in `toHTML`; in JS, prefer `textContent` over `innerHTML` for
backend-supplied names.

### WR-02: generateRank2 clamps generator against the un-clamped period

**File:** `plugins/O-Formant/Source/ScaleGenerator.cpp:59-60`
**Issue:** `generatorCents` is clamped to `periodCents - 1.0` on line 59 **before**
`periodCents` itself is clamped on line 60. If a caller passes `periodCents` below the
100.0 floor (or absurdly large), the generator bound is computed from the wrong period.
Not a crash, but produces a scale that doesn't match the requested (clamped) period.
Clamp `periodCents` first, then clamp `generatorCents` against the clamped value.

### WR-03: calculateETDeviation divides by caller-supplied totalDegrees

**File:** `plugins/O-Formant/Source/TuningExporter.cpp:118`
**Issue:** `etCents = (degree / totalDegrees) * period` divides by `totalDegrees` with no
guard. Internal callers happen to pass `noteCount > 0` (the loop at toHTML:433 is skipped
when `noteCount == 0`), but this is a `public static` API; a `totalDegrees == 0` call
yields `inf`/`nan`. Add `if (totalDegrees <= 0) return 0.0;` at the top.

### WR-04: reduceAndSort loops forever if period <= 0

**File:** `plugins/O-Formant/Source/ScaleGenerator.cpp:87-90`
**Issue:** `while (c < 0.0) c += period;` / `while (c >= period) c -= period;` never
terminates if `period <= 0`. Current callers always pass a positive period (1200.0 hard-
coded, or the clamped `periodCents`), so it's presently unreachable — but the function is
one refactor away from an infinite loop / audible hang. Guard with
`if (period <= 0.0) period = 1200.0;` at entry.

### WR-05: KBM octave-degree read from file is not range-validated

**File:** `plugins/O-Formant/Source/TuningEngine.cpp:539, 575`
**Issue:** `newOctaveDegree` is read via `getIntValue()` and stored into
`kbmOctaveDegree` with only a `> 0` check. A wild value flows into KBM math and the
generated `.kbm` on export. Clamp it to a sane range (e.g. `jlimit(1, 512, …)`) alongside
the CR-01 fix.

## Info

### IN-01: Unused native-function registrations (no JS caller)

**File:** `plugins/O-Formant/Source/PluginEditor.cpp:250, 283, 325, 340, 352, 540`
**Issue:** `setTuningIntervals`, `setSingleIntervalEncoded`, `getMasterTune`,
`setTemperamentPreset`, `getTemperamentPreset`, and `getEmbeddedTuningCategories` are
registered but never called from the JS (verified against all 36 `getNativeFunction`
sites). Not dead controls (the reverse gap would be the dangerous one), just unused
backend surface. Remove or wire up.

### IN-02: setSingleIntervalEncoded is an exact duplicate of setSingleInterval

**File:** `plugins/O-Formant/Source/PluginEditor.cpp:271-293`
**Issue:** The two lambdas are byte-for-byte identical. Dead duplication; drop one.

### IN-03: Non-atomic reads of pitchBendRange / a4Frequency / octaveStretch

**File:** `plugins/O-Formant/Source/TuningEngine.cpp:896, 291-293`
**Issue:** `pitchBendRange`, `a4Frequency`, `octaveStretch` are plain scalars written on
the message thread. `applyPitchBend` reads `pitchBendRange` on the audio thread
(`getFrequency` → `applyPitchBend`). This is a benign data race (no torn read for aligned
scalars on the target ISAs; only a transient value glitch), but making them
`std::atomic` with relaxed ordering would match the rest of the class's discipline.

### IN-04: Transient frequency-table inconsistency during rebuild

**File:** `plugins/O-Formant/Source/TuningEngine.cpp:900-917`
**Issue:** `rebuildFrequencyTable` stores the 128 entries one at a time (each atomic, but
not as a group). A note started mid-rebuild can read a table where some notes are new and
some are old — momentary, self-correcting, not a crash. Acceptable for this use; noted for
completeness.

### IN-05: Unused member flags

**File:** `plugins/O-Formant/Source/TuningEngine.h:333-334`
**Issue:** `mtsSynthClientConnected` and `scalaFileLoaded` are written but never read for
any behavior. Dead state; remove or use.

---

_Reviewed: 2026-07-01_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
