---
phase: O-simpleGrain-full-plugin-review
reviewed: 2026-07-15T00:00:00Z
depth: standard
files_reviewed: 17
files_reviewed_list:
  - plugins/O-simpleGrain/Source/PluginProcessor.h
  - plugins/O-simpleGrain/Source/PluginProcessor.cpp
  - plugins/O-simpleGrain/Source/PluginEditor.h
  - plugins/O-simpleGrain/Source/PluginEditor.cpp
  - plugins/O-simpleGrain/Source/GrainSound.h
  - plugins/O-simpleGrain/Source/GrainVoice.h
  - plugins/O-simpleGrain/Source/VizAnalyzer.h
  - plugins/O-simpleGrain/Source/dsp/Grain.h
  - plugins/O-simpleGrain/Source/dsp/GrainCloudFrame.h
  - plugins/O-simpleGrain/Source/dsp/LagrangeInterpolation.h
  - plugins/O-simpleGrain/Source/dsp/TripleBuffer.h
  - plugins/O-simpleGrain/Source/dsp/WindowLuts.h
  - plugins/O-simpleGrain/Source/ui/public/index.html
  - plugins/O-simpleGrain/Source/ui/public/js/app.js
  - plugins/O-simpleGrain/Source/ui/public/modules/webview-drop-streaming.js
  - plugins/O-simpleGrain/tests/render-harness/main.cpp
  - plugins/O-simpleGrain/CMakeLists.txt
findings:
  critical: 2
  warning: 5
  info: 9
  total: 16
status: issues_found
---

# O-simpleGrain: Code Review Report

**Reviewed:** 2026-07-15
**Depth:** standard (full-plugin, all 17 listed sources)
**Files Reviewed:** 17
**Status:** issues_found

## Summary

Full adversarial review of the granular engine, WebView editor, JS UI, shared drop-streaming module, render harness, and CMake. The core DSP is in good shape: the grain pool is allocation-free and bounded, the AA one-pole precomputes its coefficient on spawn, the TripleBuffer and VizRing memory orderings are correct for their SPSC roles, the dual `juce_add_binary_data` namespace collision is correctly avoided (`UIBinaryData` vs `BinaryData`), and both Windows WebView2 flags (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`) are present. The two v1.0.1 CHANGELOG fixes (velToDensity ×0.01 scaling; restore-clobber via `cancelPendingUpdate`) are verified present in the code.

Two critical defects remain: (1) a user-dropped source is silently replaced by the "fire" built-in on the next `prepareToPlay` — a mid-session audible data-loss bug on the plugin's flagship "granulate your own sound" flow; (2) `currentSourceIdentity` / `currentSampleRate` are mutated without synchronization from both host-controlled threads (`prepareToPlay`, `get/setStateInformation`) and the message thread, a COW-`juce::String` race that can UAF. Additionally the on-screen keyboard — an advertised primary input path — is completely dead: the JS fetches a `uiMidi` native function that is never registered and has no processor-side handler (the exact `pattern_webview_native_fn_bridge_gap` failure documented for this suite).

## Critical Issues

### CR-01: Dropped source is silently replaced by the "fire" built-in on every `prepareToPlay`

**File:** `plugins/O-simpleGrain/Source/PluginProcessor.cpp:254-271` (with `dropSessionCommitFile` identity at `:535`)
**Issue:** `dropSessionCommitFile` publishes a dropped file with identity `"dropped:<name>"` (line 535) — a name only, no disk path (WKWebView strips paths, by design). But `prepareToPlay`'s reload logic only special-cases identities starting with `"embedded:"`; everything else goes down the "restored user file" branch:

```cpp
const juce::File f (currentSourceIdentity);   // "dropped:piano-lick.wav" — NOT an absolute path
if (f.existsAsFile()) { ... }
if (! loadedUserFile)
{
    currentSourceIdentity = "embedded:fire";
    loadBuiltInSource (0, sampleRate);        // ← clobbers the user's live source
}
```

**Failure scenario:** User drags in their own sound (works, plays). Host later re-prepares the plugin — buffer-size or sample-rate change, audio-engine stop/start (Logic does this routinely), Standalone device restart, offline bounce. `prepareToPlay` runs, `juce::File("dropped:...")` doesn't exist → the user's loaded sound is silently discarded and replaced by fire, mid-session, with no notice. Bonus defect: `juce::File(String)` with a non-absolute path fires `jassert` in debug builds (`parseAbsolutePath`), so debug hosts hit an assertion here too.

The existing fallback is intended only for *state restore* (name-only identities are documented to fall back). A **live** session where the resampled buffer is already published in `currentSource` must not be clobbered.

**Fix:** Treat `"dropped:"` identities explicitly in `prepareToPlay` (and guard the `juce::File` construction behind an absolute-path check):

```cpp
else if (currentSourceIdentity.startsWith ("dropped:"))
{
    // The dropped bytes are gone (no path), but the resampled buffer is live.
    // Keep it if the engine rate is unchanged; if the rate changed, re-resample
    // the currently-published buffer (retain its rate in a member), or at worst
    // keep it and accept the transposition — never clobber with a built-in.
    if (atomicLoad (currentSource) != nullptr)
        loadedUserFile = true;
}
else if (juce::File::isAbsolutePath (currentSourceIdentity)) { /* existing path branch */ }
```

The robust version retains the decoded-at-source-rate buffer (or the raw dropped bytes) in a member so a rate change can re-resample correctly. The same guard is needed in `setStateInformation` (line 959) to avoid the debug `jassert` on `juce::File("dropped:...")` — the restore *fallback* itself is fine there.

### CR-02: Data race on `currentSourceIdentity` / `currentSampleRate` between host threads and the message thread

**File:** `plugins/O-simpleGrain/Source/PluginProcessor.cpp:385, 204-206, 907-920, 951-975`; `PluginProcessor.h:286, 347`
**Issue:** `currentSource` is correctly protected with `std::atomic_load/store`, but the *sibling state written on the same code path is not*:

- `decodeAndPublish` (line 385) assigns `currentSourceIdentity` (a COW ref-counted `juce::String`). Its callers span threads: `prepareToPlay` (host-controlled thread — VST3 `setupProcessing` / AU init are not guaranteed to be the message thread), `handleAsyncUpdate` / native-fn callbacks / FileChooser completion (message thread), and `setStateInformation` (host-controlled).
- `getStateInformation` (line 915) *reads* `currentSourceIdentity` and can be called by hosts from arbitrary threads (VST3 `getState` during autosave).
- `currentSampleRate` (plain `double`, header line 347) is written in `prepareToPlay` and read concurrently by message-thread native fns (`dropSessionCommitFile`, `getSampleRate`, `loadSourceFromFileChooser`) and the editor timer.

**Failure scenario:** User drops a file (message thread runs `decodeAndPublish`, assigning the String) while the host re-prepares after a buffer-size change (`prepareToPlay` on the host thread assigns the same String), or while the host autosaves (`getStateInformation` reads it). Two unsynchronized COW-String ref-count operations on the same object → double-release / use-after-free → crash. This is the exact class the suite already fixed with a lock in O-simpleSampler v1.0.0 ("swap-publish needs a lock if prepareToPlay also publishes" — C++17 has no `atomic<shared_ptr>` member form, and Strings have nothing at all).

**Fix:** Guard `currentSourceIdentity` (and the identity read in `getStateInformation` / `prepareToPlay`) with a `juce::CriticalSection` taken only off the audio thread (audio never touches the String), matching the O-simpleSampler pattern:

```cpp
juce::CriticalSection sourceStateLock;   // member
// every read/write of currentSourceIdentity:
{ const juce::ScopedLock sl (sourceStateLock); currentSourceIdentity = identity; }
```

Make `currentSampleRate` a `std::atomic<double>` (relaxed) — it's read lock-free from several threads.

## Warnings

### WR-01: On-screen keyboard is completely dead — `uiMidi` native fn fetched in JS but never registered, no processor handler

**File:** `plugins/O-simpleGrain/Source/ui/public/js/app.js:902, 885, 892`; `Source/PluginEditor.cpp:117-172`; `Source/ui/public/index.html:219-222`
**Issue:** `setupKeyboard()` does `uiMidiFn = Juce.getNativeFunction("uiMidi")`, and `noteOn`/`noteOff` call it. The editor registers nine native functions — `uiMidi` is not one of them, and the processor has no UI-MIDI injection at all (no queue, nothing merged into `processBlock`'s `midiMessages`). `getNativeFunction` for an unknown name does not throw (it only `console.warn`s), so `uiMidiFn` is a live callable whose invocations vanish into the backend — the exact silent-failure mode documented in this repo's `pattern_webview_native_fn_bridge_gap` memory. The JS comment "the processor may not expose it yet" is a Phase-3.1 leftover; the plugin shipped without it.

**Failure scenario:** Any user without external MIDI clicks the keys or presses A–K as the UI instructs ("Play · click the keys or use your computer keyboard") — keys highlight, **zero sound**. For a pedagogical synth this is the primary play path; the feature is 100% non-functional. Side effect: each keypress spawns a never-settled promise (`uiMidiFn(...)` is not awaited/caught).

**Fix:** (a) Register the native fn in the editor and forward to a processor handler; (b) in the processor, push events into a lock-free FIFO drained at the top of `processBlock` into `midiMessages` (gate with nothing — merge unconditionally, or per suite memory, verify behavior when the host MIDI buffer is empty); (c) remove the "optional" hedge in app.js and `.catch(() => {})` the invocation:

```cpp
.withNativeFunction ("uiMidi", [this] (const juce::Array<juce::var>& args, auto complete) {
    if (args.size() >= 3)
        processorRef.handleUiMidi ((int) args[0], (bool) args[1], (float) (double) args[2]);
    complete (juce::var (true));
})
```

`handleUiMidi` writes `{note, on, vel}` into a `juce::AbstractFifo`-backed array; `processBlock` drains it into `midiMessages` before `synth.renderNextBlock`.

### WR-02: `decodeAndPublish` allocates and decodes the entire file before the 10 s cap — unbounded allocation + `int` truncation of `lengthInSamples`

**File:** `plugins/O-simpleGrain/Source/PluginProcessor.cpp:370-378`
**Issue:** The 10 s cap (`kMaxSourceSeconds`) is applied only in `resampleToEngineRate`, *after* the full file has been decoded:

```cpp
const int nSmp = (int) reader->lengthInSamples;      // int64 → int truncation
...
juce::AudioBuffer<float> tmp (nCh, nSmp);            // full-file allocation
reader->read (&tmp, 0, nSmp, 0, true, true);         // full-file decode
```

**Failure scenario:** (a) User drops a 45-minute podcast WAV — ~500 MB+ float allocation, full decode, then 99% is thrown away; multi-second message-thread stall plus a memory spike (and the base64 string in `dropAccumBase64` doubles it). (b) A malformed/hostile header claiming `lengthInSamples` near `INT32_MAX` → `tmp` attempts a multi-GB allocation → `std::bad_alloc` → crash. Values above `INT32_MAX` truncate to negative/zero and are caught, but the just-below range is not.

**Fix:** Clamp the decode length to the cap before allocating, and derive `truncated` from the comparison:

```cpp
const juce::int64 maxSrcSmp = (juce::int64) std::ceil (kMaxSourceSeconds * srcRate) + 4;
const juce::int64 len64     = reader->lengthInSamples;
if (len64 <= 0) return false;
const int nSmp = (int) juce::jmin (len64, maxSrcSmp);
const bool preTruncated = (len64 > maxSrcSmp);
```

(then OR `preTruncated` into the `truncated` flag).

### WR-03: Factory presets interact inconsistently with a user-loaded source

**File:** `plugins/O-simpleGrain/Source/PluginProcessor.cpp:787-788, 876-886`
**Issue:** `applyFactoryPreset` first resets every parameter to default, including `sourceSample`. Two inconsistent outcomes when a dropped/picked source is active:
1. If the user's last built-in choice was ≠ fire (e.g. "water"), the reset *changes* `sourceSample` → `parameterChanged` fires → the AsyncUpdater rebuilds the built-in → **the user's loaded source is silently discarded** by pressing any lesson button.
2. If the choice was already 0 (fire), APVTS suppresses the same-value change → no rebuild → the dropped source survives — which means **"Granular Fire" does not actually load the fire sample** (its `setChoice (sourceSample, 0)` is also a same-value no-op), contradicting its stated intent.

**Failure scenario:** User granulates their own recording, clicks "Smooth Cloud" to hear the concept applied to *their* sound — depending on invisible prior state, they either keep their source or lose it to "fire". Non-deterministic from the user's viewpoint.

**Fix:** Decide the contract and enforce it explicitly. Recommended: presets keep the current source (concepts apply to whatever is loaded) — skip `sourceSample` in the reset loop and drop the `setChoice (sourceSample, 0)` from "Granular Fire" (or make Granular Fire force-load fire via `loadBuiltInSource (0, currentSampleRate)` explicitly, bypassing the same-value suppression).

### WR-04: Version metadata drift — sources say 1.0.1, suite registry says 1.1.0

**File:** `plugins/O-simpleGrain/CMakeLists.txt:17`; `tests/render-harness/CMakeLists.txt` (`JucePlugin_VersionString "1.0.1"`, `JucePlugin_VersionCode 0x010001`); `CHANGELOG.md` (latest entry 1.0.1); vs `PLUGINS.md:62` ("O-simpleGrain … 1.1.0 … Installed")
**Issue:** The plugin's own version sources are unanimous at 1.0.1, but the suite registry (`PLUGINS.md`) records the installed version as 1.1.0, and no 1.1.0 CHANGELOG entry exists. Either a 1.1.0 was installed without bumping CMake/CHANGELOG (binaries then self-report 1.0.1 — exactly the stale-version confusion this repo has repeatedly debugged around the dev/release AU-registry shadowing), or `PLUGINS.md` is over-advanced. The version is also duplicated by hand in the harness CMake, guaranteeing future drift.

**Failure scenario:** Support/debug session checks the plugin's reported version in the DAW (1.0.1), concludes an old build is cached, and burns time on AU-cache sweeps for a binary that is actually current — the documented `pattern_stale_host_instance_vs_offline_repro` trap, self-inflicted.

**Fix:** Reconcile: bump `CMakeLists.txt` VERSION + harness `JucePlugin_VersionString/Code` + add the missing CHANGELOG entry, or correct `PLUGINS.md`. Longer term, derive the harness version macros from a single CMake variable.

### WR-05: `std::atomic_load/store` on `shared_ptr` is lock-based — the audio thread takes a shared spinlock/mutex each block

**File:** `plugins/O-simpleGrain/Source/PluginProcessor.h:190-199`; `PluginProcessor.cpp:613`
**Issue:** The free-function `std::atomic_load(&currentSource)` in `processBlock` is not lock-free: libc++ implements it via a global `__sp_mut` pool (bounded spin, then a real mutex). The audio thread therefore acquires a mutex once per block, shared with every `atomic_store` from the message/host threads during source publishes — and, because the pool is global and hashed by address, potentially shared with unrelated `shared_ptr` atomics in the same process.

**Failure scenario:** The message thread is preempted by the OS while holding the pool mutex mid-`atomic_store` (publishing a new source); the audio thread's `atomic_load` blocks behind it → priority inversion → xrun. Low probability, but it is a real lock on the RT path with a known suite-endorsed alternative.

**Fix:** Keep the shared_ptr ownership on the message side and give the audio thread a genuinely lock-free view — e.g. the retired-list pattern already shipped in O-MicrotonalSampler v1.24.0: an `std::atomic<juce::AudioBuffer<float>*>` (pointer + length packed, or pointer to an immutable struct) read by `processBlock`, with the owning `shared_ptr`s parked in a message-thread retired list freed only at `use_count()==1`. If the current pattern is retained as a deliberate suite convention, document the tradeoff at the `atomicLoad` helper.

## Info

### IN-01: Dead member — editor `fileChooser` is never used

**File:** `plugins/O-simpleGrain/Source/PluginEditor.h:48`
**Issue:** The editor declares `std::unique_ptr<juce::FileChooser> fileChooser` "reserved", but all picking goes through the processor's own chooser. Dead code.
**Fix:** Delete the member (and its comment).

### IN-02: `dynamic_cast` over all voices twice per block in `processBlock`

**File:** `plugins/O-simpleGrain/Source/PluginProcessor.cpp:692-693, 707-708` (also `prepareToPlay:216`)
**Issue:** 16 RTTI casts per block on the audio thread. Not an RT violation, but avoidable.
**Fix:** Cache a `std::array<GrainVoice*, kMaxVoices>` at construction (the voices are owned for the processor's lifetime) and iterate that.

### IN-03: Playhead rest-ease constant is sample-rate dependent

**File:** `plugins/O-simpleGrain/Source/PluginProcessor.cpp:660`
**Issue:** `kRestEase = 0.0008` per sample makes the Position-knob glide ~2× faster at 96 kHz than at 44.1 kHz.
**Fix:** Derive the coefficient from a time constant: `ease = 1 - exp(-1 / (tauSeconds * sampleRate))` computed in `prepareToPlay`.

### IN-04: `Grain::readPos` is `float` — position quantization near the end of long sources at high rates

**File:** `plugins/O-simpleGrain/Source/dsp/Grain.h:25`; `GrainVoice.h:229, 237`
**Issue:** A 10 s source at 96 kHz spans 960k samples; float ULP there is 0.0625 samples, so fractional read increments quantize near the tail (subtle pitch/interp jitter for grains reading late in the source). The processor keeps `playheadPos` as `double` but the voice truncates to `float` (`setPlayhead(float)`).
**Fix:** Make `readPos` (and the playhead handoff) `double`, or store readPos as int index + float frac.

### IN-05: JS duplicates C++ contract constants

**File:** `plugins/O-simpleGrain/Source/ui/public/js/app.js:764` (`GLOBAL_GRAIN_CAP = 192`), `:717-727` (window formulas + `GAUSS_SIGMA = 0.18`)
**Issue:** `kGlobalGrainCap`, the five window formulas, and the Gaussian σ are re-implemented in JS. Any C++ change silently desynchronizes the meter/inset (the window-inset JS recompute was an accepted design decision; the risk should still be pinned).
**Fix:** Push the cap once via a native fn or initialisation data; add a cross-reference comment on both sides for the window formulas.

### IN-06: `applyFactoryPreset` writes parameters without begin/end change gestures

**File:** `plugins/O-simpleGrain/Source/PluginProcessor.cpp:788, 791-801`
**Issue:** 18 `setValueNotifyingHost` calls with no `beginChangeGesture`/`endChangeGesture`. Hosts recording automation may log ungestured writes oddly (Logic/Live tolerate it; strict hosts warn).
**Fix:** Wrap each write in a gesture pair, or at minimum document the omission (mirrors O-simpleAdditive, so fix both together).

### IN-07: Out-of-range grain reads pin to the edge sample — constant-value segments under large position spray

**File:** `plugins/O-simpleGrain/Source/GrainVoice.h:305-307, 361-373`
**Issue:** `readPos = playheadPos ± up to 100% of sourceLen` can start well below 0 or beyond the end; `readSourceLagrange` clamps all four taps, so such a grain outputs a constant `src[0]`/`src[len-1]` (windowed DC-ish thump) until/unless the read climbs back in range. Documented as taper-by-window, but at 100% spray up to half the spawns are affected.
**Fix:** Wrap the spawn position into `[0, sourceLen)` (consistent with the playhead's own wrap), or clamp the sprayed spawn into range.

### IN-08: UI status/thumbnail refreshes race on fixed timers

**File:** `plugins/O-simpleGrain/Source/ui/public/js/app.js:386` (1200 ms after Load… click), `:268` (300 ms after sourceSample change)
**Issue:** The Load… flow reports truncation status and refetches the thumbnail 1.2 s after the *click*, not after the pick — a user who browses longer gets a stale "Source loaded" status and old waveform; a slow decode similarly outruns the 300 ms combo refetch.
**Fix:** Have the C++ side emit a `sourceChanged` WebView event after every successful `decodeAndPublish` (editor already owns a timer/event channel) and drive status + thumbnail refetch from that.

### IN-09: `prepareToPlay` re-decodes and re-resamples the source on every prepare, even at an unchanged rate

**File:** `plugins/O-simpleGrain/Source/PluginProcessor.cpp:247-271`
**Issue:** Every host re-prepare (transport engine restarts, buffer-size changes) re-runs WAV decode + Lagrange resample of up to 10 s of audio. Correctness-neutral for built-ins, but a needless stall — and it is the code path that triggers CR-01 for dropped sources.
**Fix:** Skip the reload when `sampleRate == currentSampleRate` and a source is already published (fold into the CR-01 fix).

---

## Review-lens notes (checked, no finding)

- **Bridge coverage:** all other JS `getNativeFunction` names (`dropSessionStart/AddFile/CommitFile`, `loadSourceFromFileChooser`, `wasLastLoadTruncated`, `getSampleRate`, `getSourceThumbnail`, `applyFactoryPreset`) are registered; all 18 relay IDs match `ParamIDs` and the JS lists; the resource provider serves every path the page requests (bare-path equality, correct per suite memory).
- **SliderState readouts:** knob values use `getScaledValue()` with `propertiesChangedEvent` listeners — no hardcoded JS ranges (correct per `pattern_webview_knob_readout_scaled_value`). Double-click-reset is absent but was an audit item, not a defect here.
- **FileChooser completion:** the null path bails with a bare `return` and never calls a WebView `complete()` (the native fn completes immediately) — matches `pattern_webview_launchasync_safepointer_no_complete`. Processor-owned chooser is cancelled by its dtor in JUCE 8.
- **TripleBuffer/VizRing:** acquire/release pairs correct; writer/reader thread assignments hold (audio writes, editor timer reads).
- **Destruction order:** relays → WebView → attachments declared in the documented safe order.
- **CMake:** dual binary-data targets use distinct `NAMESPACE`s; both Windows WebView2 flags present; harness compiles `PluginEditor.cpp` with `JUCE_WEB_BROWSER=1` and links `O-simpleGrain_UIResources`, so the WebView-editor link drift pattern does not apply.
- **v1.0.1 fixes verified in place:** `velToDensity * 0.01f` at the push site (`PluginProcessor.cpp:632`, harness gate 9 covers it); restore-clobber handled via `cancelPendingUpdate()` (`:980`).

---

_Reviewed: 2026-07-15_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
