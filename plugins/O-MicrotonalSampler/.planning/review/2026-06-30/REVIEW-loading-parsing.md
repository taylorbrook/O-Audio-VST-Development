---
phase: review-loading-parsing
reviewed: 2026-07-01T02:19:34Z
depth: deep
files_reviewed: 5
files_reviewed_list:
  - plugins/O-MicrotonalSampler/Source/SampleLoader.cpp
  - plugins/O-MicrotonalSampler/Source/SampleLoader.h
  - plugins/O-MicrotonalSampler/Source/FilenameParser.cpp
  - plugins/O-MicrotonalSampler/Source/FilenameParser.h
  - plugins/O-MicrotonalSampler/Source/TriggerMapping.h
findings:
  critical: 1
  warning: 4
  info: 5
  total: 10
status: issues_found
---

# Phase review-loading-parsing: Code Review Report

**Reviewed:** 2026-07-01T02:19:34Z
**Depth:** deep
**Files Reviewed:** 5
**Status:** issues_found

## Summary

Reviewed the sample-loading and filename-parsing domain of O-MicrotonalSampler: `SampleLoader` (background worker thread + per-file decode/resample + sample-map construction), `FilenameParser` (tolerant pitch/velocity/RR/technique inference), and `TriggerMapping.h` (CC/PC/precedence tables).

Cross-file threading verified: the message thread publishes maps via `std::atomic_store` on `shared_ptr` (`atomicStore`, PluginProcessor.cpp:39) and the voice snapshots via `std::atomic_load(sampleMapSource)` (MicrotonalSamplerVoice.cpp:424), holding the `shared_ptr` for the note's duration. That half of the "free-while-audio-reads" contract is **correct** — a cell swap mid-note keeps the old buffer alive transitively. `TriggerMapping.h` is clean (band math is contiguous and covers 0..127; precedence and jlimits are sound).

The defects concentrate in the loader's thread lifecycle and per-file robustness. The BLOCKER is the force-kill of the worker thread when a load overruns the 500 ms `stopThread` timeout — realistic for the large libraries this plugin targets (the project memory cites 3–5 s streaming for 250 MB). The WARNINGs cover an untrusted-header allocation that silently aborts the whole folder load, a 1-sample resampler over-read, and int64→int truncation. Parser findings are mostly heuristic mis-map edge cases (INFO).

## Critical Issues

### CR-01: `loadFolder`/`loadSingleVariant`/`cancelLoad` can force-kill the worker mid-load (pthread_cancel) → leaked reader, half-mutated state, "locks in silly states"

**File:** `plugins/O-MicrotonalSampler/Source/SampleLoader.cpp:36, 57, 74` (and dtor `:27`)
**Issue:** Every public entry point restarts the worker with `stopThread (500)`. `juce::Thread::stopThread` signals exit, waits the timeout, and if the thread is still running **force-kills it** — on macOS this is `pthread_cancel` (JUCE `Threads_mac.mm:172`; the JUCE source itself warns "very bad karma… locks and events left in silly states"). The worker only checks `threadShouldExit()` at folder-loop boundaries (`SampleLoader.cpp:282`, `:327`); there is **no cancellation checkpoint inside `processOneFile`** — the `reader->read()` of one large file and the `LagrangeInterpolator::process` of one long file have no way to bail. A single multi-hundred-MB file (or a slow network/USB volume) easily exceeds 500 ms. If the user triggers a second load, a per-cell edit, or closes the project during that window, the in-flight worker is cancelled at its next syscall, which:
- leaks the `std::unique_ptr<AudioFormatReader>` and the in-flight `AudioBuffer`s (destructors never run),
- leaves the `skippedFiles` `StringArray` / `loaded` vector partially mutated,
- **never fires the completion or failure callback**, so the UI's load spinner hangs forever with no feedback,
- can abandon a JUCE internal lock held inside the format reader's I/O path.

The destructor's `stopThread (2000)` (`:27`) has the same hazard with a longer fuse.

**Fix:** Do not rely on force-kill. Add cooperative cancellation and let the previous worker drain:
```cpp
// In processOneFile, chunk the read/resample and bail cooperatively:
//   pass a std::function<bool()> shouldExit (or the SampleLoader*) and check it
//   between blocks of reader->read(...) and interp.process(...).
// In the public entry points, prefer a longer, non-killing join, or queue the
// next request and start it from run()'s tail rather than force-stopping:
void SampleLoader::loadFolder (...)
{
    signalThreadShouldExit();          // ask the current run() to unwind
    if (! stopThread (5000))           // generous; still returns false if it had to kill
        jassertfalse;                  // telemetry: we should never reach a force-kill
    ...
}
```
At minimum, thread `threadShouldExit()` checks through `processOneFile`'s read/resample loops so a large-file load is actually interruptible, and raise the timeout above the realistic worst-case decode time.

## Warnings

### WR-01: Untrusted `lengthInSamples` drives an unbounded allocation; a throw aborts the ENTIRE folder load with no callback and no skip entry

**File:** `plugins/O-MicrotonalSampler/Source/SampleLoader.cpp:108, 117, 143`
**Issue:** `srcSamples`/`srcChannels` come straight from the file header, then `juce::AudioBuffer<float> sourceBuf (srcChannels, srcSamples)` and `std::make_shared<juce::AudioBuffer<float>>(2, outNumSamples)` allocate proportionally with **no upper bound**. A corrupt header claiming a huge length, or a genuinely multi-GB file, throws `std::bad_alloc`. JUCE's `threadEntryPoint` wraps `run()` in `try/catch(...)` with only `jassertfalse` (Threads.cpp:108–114) — in Release the exception is **silently swallowed**, the worker exits, and neither `completionCallback` nor `failureCallback` ever fires. Because `processOneFile` is called inside the folder loop with no per-file `try`, one bad file kills the whole batch (every already-decoded sample in `loaded` is discarded) and the UI receives zero feedback — not even a "skipped" toast.
**Fix:** Wrap the per-file work so a throw becomes a skip, and cap the size:
```cpp
try {
    if (srcSamples > kMaxSamplesPerFile) { outSkipReason = "file too large: " + displayName; return false; }
    // ... existing allocate/read/resample ...
} catch (const std::exception& e) {
    outSkipReason = juce::String ("decode error: ") + e.what() + " (" + displayName + ")";
    return false;
}
```
so folder mode records the skip and continues, and single-variant mode reports the failure via its callback.

### WR-02: `int64 → int` truncation of `lengthInSamples` mis-sizes very long files

**File:** `plugins/O-MicrotonalSampler/Source/SampleLoader.cpp:108`
**Issue:** `const int srcSamples = (int) reader->lengthInSamples;`. `lengthInSamples` is `int64`. A file with >2^31 samples (or a corrupt header) truncates: values that wrap to a small positive number pass the `srcSamples <= 0` guard (`:111`) and cause a silent under-read (only the low-order sample count is decoded); values that wrap negative are rejected as "invalid header" even though the file is real. Either way the sample is mis-handled without a clear reason.
**Fix:** Keep the count as `int64`, validate against a sane ceiling before narrowing: `if (reader->lengthInSamples <= 0 || reader->lengthInSamples > kMaxSamplesPerFile) { skip; }` then `const int srcSamples = (int) reader->lengthInSamples;`.

### WR-03: Resampler output length uses `ceil`, causing a ~1-sample heap over-read of the input buffer

**File:** `plugins/O-MicrotonalSampler/Source/SampleLoader.cpp:131, 151-163`
**Issue:** `outNumSamples = (int) std::ceil ((double) srcSamples / srcRatio)` where `srcRatio = srcSR/targetSR`. `LagrangeInterpolator::process` consumes ≈ `srcRatio * outNumSamples` input samples. Because of the `ceil`, that product can exceed `srcSamples` by up to ~1 sample (e.g. any odd-length down-conversion such as 44.1 kHz → 48 kHz), so the interpolator reads one element past the end of `sourceBuf.getReadPointer(...)`, which holds exactly `srcSamples` valid samples. A small but real out-of-bounds heap read on the majority of real-world (SR-mismatched) loads.
**Fix:** Either allocate the source buffer with one guard sample of zero padding, or bound the requested output so input consumption never exceeds `srcSamples` (e.g. `outNumSamples = (int) std::floor ((double) srcSamples / srcRatio);` and accept the ≤1-sample tail loss, matching JUCE's own resampler examples).

### WR-04: RR split-form can double-consume a bare-integer note token, fabricating a round-robin index

**File:** `plugins/O-MicrotonalSampler/Source/FilenameParser.cpp:497-522`
**Issue:** The RR scan runs over **all** tokens with no exclusion of the token already claimed as the note. For a filename like `take_60.wav`, `"60"` is chosen as the bare-integer MIDI note, and then the split-form branch (`lc == "take"`, next token `"60"`) evaluates `parseAsRrIndex("take60")` → `rrIndex = 59`. The same `"60"` token is used both as the pitch and as an RR index. The sample lands in cell (60, 0, 0) tagged with a spurious explicit `rrIndex = 59`; harmless as a lone variant, but it suppresses the ambiguous-duplicate modal for genuinely-duplicated takes and skews variant ordering.
**Fix:** In the split-form branch, require the digit token to not be the note token: `if (i + 1 != noteTokenIndex && (i + 1) < tokens.size()) { ... }` (and likewise skip `i == noteTokenIndex`).

## Info

### IN-01: Folder enumeration is non-recursive — nested sample folders are silently ignored

**File:** `plugins/O-MicrotonalSampler/Source/SampleLoader.cpp:277-280`
**Issue:** `RangedDirectoryIterator(pendingFolder, /*recursive*/ false, wildcards, findFiles)`. Samples in subdirectories are neither loaded nor added to `skippedFiles`, so a user who drops a library organized into per-articulation subfolders gets a partial/empty map with no explanation.
**Fix:** Either recurse (`true`) or, if flat-only is intentional, count subdirectories and surface a "N subfolder(s) ignored" note in the completion payload.

### IN-02: Groups mixing an explicit-RR file and a no-token file are silently merged (ambiguity suppressed)

**File:** `plugins/O-MicrotonalSampler/Source/SampleLoader.cpp:424-444`
**Issue:** `anyExplicitRr` is true if *any* file in the (midi, layer, technique) group carries an RR token. A group of `{C3_v1_rr1.wav, C3_v1.wav}` is therefore treated as an intentional RR set and merged without the confirmation modal, even though the second file was probably an accidental duplicate. The `-1` file sorts last (`1000 + i`) and becomes a silent variant.
**Fix:** Flag as ambiguous when the group contains a mix of explicit-RR and no-token files (or when any two files share the same resolved RR index), so the user confirms.

### IN-03: Pre-note single-letter dynamics produce false-positive velocity layers

**File:** `plugins/O-MicrotonalSampler/Source/FilenameParser.cpp:167-170, 471-481`
**Issue:** With no post-note velocity token, the pre-note tier accepts bare `p`/`f`/`mp`/`mf` anywhere before the note. A file whose name abbreviates the instrument to a single delimited letter — e.g. `F-C3.wav` (Flute) or `P_C3.wav` (Piano) — is read as forte (layer 3) / piano (layer 0). `F` in particular silently maps the whole file to the top dynamic layer. This is an inherent heuristic tradeoff (documented), but it is a silent mis-map.
**Fix:** Consider requiring a two-plus-char dynamic (`mp`/`mf`) or an explicit `v`/`vel`/`L`/`layer` form in the pre-note tier, keeping bare `p`/`f` post-note only; or gate single-letter dynamics behind a "trust filename dynamics" option.

### IN-04: Single-variant *replace* path does not extend `lowestNote`/`highestNote` (cross-file)

**File:** `plugins/O-MicrotonalSampler/Source/PluginProcessor.cpp:1541-1552` (consumes loader output for `SampleMap.h` metadata)
**Issue:** When a per-cell single load places a note outside the current range, the rebuilt map copies `lowestNote`/`highestNote`/`numVelocityLayers` from the previous map unchanged. `findCell*` uses linear scans (not these fields), so playback is unaffected, but any UI/keyboard-range consumer that trusts `lowest/highestNote` shows a stale range. (Included as a deep cross-file note; the fix lives outside the 5 reviewed files.)
**Fix:** Recompute the range from `next->cells` after placement, or `jmin/jmax` the new note into the copied bounds.

### IN-05: `outNumSamples < 1` clamp is dead for the down-convert case and masks empty inputs

**File:** `plugins/O-MicrotonalSampler/Source/SampleLoader.cpp:132-133`
**Issue:** `if (outNumSamples < 1) outNumSamples = 1;` only runs inside `needsResample`. Because `srcSamples >= 1` is already guaranteed (`:111`) and `srcRatio > 0`, `ceil(srcSamples/srcRatio)` is always ≥ 1, so the clamp is unreachable. Minor dead code; harmless but signals the length math wasn't fully reasoned about (see WR-03).
**Fix:** Remove the dead clamp, or fold it into the WR-03 length fix.

---

_Reviewed: 2026-07-01T02:19:34Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
