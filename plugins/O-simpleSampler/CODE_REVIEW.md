---
phase: O-simpleSampler-v0.1.0-full-review
reviewed: 2026-07-16T00:00:00Z
depth: standard
files_reviewed: 9
files_reviewed_list:
  - plugins/O-simpleSampler/CMakeLists.txt
  - plugins/O-simpleSampler/Source/PluginProcessor.h
  - plugins/O-simpleSampler/Source/PluginProcessor.cpp
  - plugins/O-simpleSampler/Source/PluginEditor.h
  - plugins/O-simpleSampler/Source/PluginEditor.cpp
  - plugins/O-simpleSampler/Source/SampleSound.h
  - plugins/O-simpleSampler/Source/SampleVoice.h
  - plugins/O-simpleSampler/Source/dsp/LagrangeInterpolation.h
  - plugins/O-simpleSampler/Source/ui/public/modules/webview-drop-streaming.js
findings:
  critical: 2
  warning: 6
  info: 5
  total: 13
status: issues_found
---

# O-simpleSampler v0.1.0: Full Code Review Report

**Reviewed:** 2026-07-16
**Depth:** standard
**Files Reviewed:** 9
**Status:** issues_found

## Summary

Stage 2.1 core sampler (16-voice Repitch engine, embedded piano.wav, placeholder editor). The code is well-commented and most suite-recurring patterns were consciously handled. Two Critical findings remain: the retired source buffer can be freed on the audio thread when a source swap lands mid-block (the exact `pattern_retired_map_reaper_rt_free` / retired-list gap — o-simpleSampler is already listed as a backport candidate in project memory), and the three off-audio decode/publish paths (prepareToPlay on the host thread, AsyncUpdater on the message thread, setStateInformation on an arbitrary thread) mutate shared non-atomic state (`currentSourceIdentity`, `stateWasRestored`, `rootSeeded`) with no lock — the exact `pattern_texture_forge_swap_needs_lock_if_prepare_publishes` case.

**Suite-recurring patterns checked:**

| Pattern | Verdict |
|---|---|
| RT-safety of processBlock (no alloc/lock/log; ScopedNoDenormals) | VIOLATED — mid-block swap frees retired buffer on RT thread (CR-01); otherwise clean, ScopedNoDenormals present, NaN scrub present |
| Retired sample map freed on audio thread (reaper/retired-list) | VIOLATED (CR-01) |
| prepareToPlay publishes synchronously → CriticalSection off-audio | VIOLATED (CR-02) — no lock exists |
| JS getNativeFunction vs C++ withNativeFunction grep-diff | VIOLATED (WR-06) — 4 JS bridge fns, 0 C++ registrations (module is currently dead code) |
| Base64: convertFromBase64 vs fromBase64Encoding | N/A — no C++ decode side exists yet; JS module docs the correct API |
| launchAsync SafePointer / no complete() on null path | CLEAR — no FileChooser anywhere |
| Lagrange bounds at buffer edges | CLEAR — im1/ip0/ip1/ip2 all jlimit-clamped; but see WR-01 (float position precision) |
| Bare `end` param-ID shadowing juce::end | CLEAR — regionStart/regionEnd identifiers, string IDs unchanged |
| SampleVoice/SampleSound vs juce::SamplerVoice/Sound collision | CLEAR — renamed, documented in both headers |
| Dual juce_add_binary_data BinaryData namespace collision | CLEAR — single target today; correct UIBinaryData guidance committed in CMake comments for Stage 3 |
| Resource provider bare-path handling | N/A — no WebView editor yet |
| Activating-dead-param default timbre | CLEAR — vintage=0, cutoff=20 kHz, resonance=0, loopMode=Off are all no-op defaults |
| Factory-preset skew / knob-readout scaled-value | N/A — no presets/UI yet |
| Denormals / SR-change handling / ADSR order (setSampleRate before setParameters) | CLEAR |

## Critical Issues

### CR-01: Retired source buffer is freed on the audio thread when a swap lands mid-block

**File:** `plugins/O-simpleSampler/Source/PluginProcessor.cpp:378` (publish), `:442` (snapshot), `Source/PluginProcessor.h:139-152`
**Issue:** `processBlock` snapshots `currentSource` via `atomicLoad` (line 442) and holds the `shared_ptr` for the block. When `decodeAndPublish` runs concurrently (sourceSample change via AsyncUpdater, or `setStateInformation`), `atomicStore` at line 378 releases the publisher's reference to the old buffer immediately. The audio thread's block-local `src` is then the **last** reference; when it goes out of scope at the end of `processBlock`, the old `juce::AudioBuffer<float>` — up to `kMaxSourceSeconds × engineRate × nCh × 4B` ≈ 46 MB at 192 kHz stereo — is destructed and `free()`d **on the audio thread**. The header comment ("a swap mid-block can never free the buffer it is reading") is true for the read window but misses the end-of-block deallocation. This is the exact defect documented in `pattern_retired_map_reaper_rt_free` (O-MicrotonalSampler v1.23.2 W10), and project memory (`pattern_audition_preview_retired_list`) already lists o-simpleSampler as a backport candidate.
**Failure scenario:** User flips Source (or a session restore lands) while notes are playing → one-block race window per swap → multi-MB `free()` inside the RT callback → allocator lock/priority inversion → audible dropout, pluginval RT-violation, or watchdog kill in strict hosts.
**Fix:** Keep the old pointer alive off-audio: before `atomicStore`, push the outgoing `shared_ptr` onto a message-thread retired list (freed only when `use_count() == 1`, via a `juce::Timer` reaper), per the suite pattern:
```cpp
// publisher (message/host thread), under the CR-02 lock:
auto old = atomicLoad (currentSource);
atomicStore (currentSource, std::move (resampled));
if (old != nullptr)
    retiredSources.push_back (std::move (old));   // reaped by timer at use_count()==1
```

### CR-02: Unsynchronized concurrent decode/publish paths — data race on `currentSourceIdentity` and the restore flags

**File:** `plugins/O-simpleSampler/Source/PluginProcessor.cpp:234-242, 349-381, 417-429, 524-573`; `Source/PluginProcessor.h:215-220`
**Issue:** Three entry points call `loadBuiltInSource` → `decodeAndPublish`: `prepareToPlay` (host audio-setup thread — not guaranteed to be the message thread in VST3/AU hosts), `handleAsyncUpdate` (message thread), and `setStateInformation` (arbitrary host thread). `decodeAndPublish` writes `currentSourceIdentity` (line 379) — a non-atomic `juce::String` — and `setStateInformation` also writes it (line 538) plus `stateWasRestored` (line 550), which `prepareToPlay` reads (line 248). Concurrent `juce::String` assignment is a data race on its ref-counted internals (UB, potential use-after-free of the old string data), and a race between prepareToPlay and setStateInformation can publish buffer A with identity B (torn buffer/identity pair). C++17 has no `atomic<shared_ptr>`; the suite fix (`pattern_texture_forge_swap_needs_lock_if_prepare_publishes`, shipped for exactly this plugin family) is a `CriticalSection` taken only off-audio.
**Failure scenario:** Host restores session state on a worker thread while another host thread runs `prepareToPlay` (common at project-load: both fire back-to-back on different threads) → racing `String` assignment corrupts the refcount → crash or heap corruption; or the restored source identity is clobbered by the prepare-time reload, silently reverting the session's source.
**Fix:** Add `juce::CriticalSection sourcePublishLock;` and take it (ScopedLock) around the whole decode→publish→identity-update sequence in `decodeAndPublish`, and around the identity/flag reads-writes in `prepareToPlay` / `setStateInformation` / `handleAsyncUpdate`. Never take it in `processBlock` (the atomic shared_ptr snapshot stays lock-free on the audio thread).

## Warnings

### WR-01: `(float) readPos` quantizes the fractional read position — audible degradation on long sources

**File:** `plugins/O-simpleSampler/Source/SampleVoice.h:129, 170, 203-214`
**Issue:** `readPos` is correctly accumulated in `double`, but both call sites cast it to `float` before `readSourceLagrange`, and `frac` is computed in `float`. Float has a 24-bit mantissa: beyond ~2^20 samples (~21 s at 48 kHz) the representable step is 0.125 samples; at the 30 s cap at 192 kHz (5.76 M samples) it is **0.5 samples** — the Lagrange `frac` collapses to two values. Interpolation degrades to stair-stepped nearest-ish reads late in the sample: zipper noise, aliasing, and pitch wobble that worsen the further the read head travels.
**Failure scenario:** Play a note with Start=0 on a 30 s source at 96/192 kHz engine rate; the tail of the note gets progressively grittier/detuned — worst exactly where the pedagogical "long found-sound" content lives.
**Fix:** Keep double precision through the read: `static float readSourceLagrange (const float* src, int len, double pos)` with `const auto i0 = (int) pos; const float frac = (float) (pos - (double) i0);`. (The four sample values can stay float; only position/frac need double.)

### WR-02: Region-end hard cut — `ampEnv.reset()` at `endSamp` clicks

**File:** `plugins/O-simpleSampler/Source/SampleVoice.h:185-189`
**Issue:** When the read head reaches `endSamp`, the voice does `ampEnv.reset(); break;` — an instantaneous amplitude cut. Unless the sample content happens to be zero at the region end (it won't be for a user-set End < 100 %, and the End default computation already truncates), every note that plays to the region boundary ends with a discontinuity. The same instant cut fires when a live region edit (params are re-pushed every block) moves `endSamp` behind an in-flight `readPos`. Note-off itself is click-free (`noteOff()` release), but region-end is the *common* termination path for a one-shot sampler with short regions.
**Failure scenario:** Set End = 40 % on the piano source (mid-sustain), play any note and hold it → audible click/tick at region end on every voice; loop a MIDI clip and it clicks rhythmically.
**Fix:** Replace the reset with a fast release: on reaching `endSamp`, call `ampEnv.noteOff()` after temporarily forcing a short release (~5 ms), or apply a small precomputed linear fade over the last N samples before `endSamp`. Reserve `reset()` for the `stopNote(_, false)` steal path only.

### WR-03: `decodeAndPublish` — int truncation of `lengthInSamples` and unbounded pre-cap allocation

**File:** `plugins/O-simpleSampler/Source/PluginProcessor.cpp:364-372`
**Issue:** `const int nSmp = (int) reader->lengthInSamples;` truncates an `int64`. A >2^31-sample file wraps negative (rejected — fine) or, above 2^32, wraps to a small bogus positive length (silently loads garbage-length audio). Worse, the full decoded file is allocated at source rate (`juce::AudioBuffer<float> tmp (nCh, nSmp)`) *before* the 30 s `kMaxSourceSeconds` cap is applied in `resampleToEngineRate` — a 2 GB WAV means a ≥2 GB transient allocation, and `AudioBuffer` allocation failure throws `std::bad_alloc` out of an uncaught path (host crash). Harmless for the embedded piano.wav today, but `decodeAndPublish` is explicitly the generic Stage 2.3 user-file path.
**Failure scenario:** Stage 2.3 wires user file loading; user drops a 3 GB field recording → multi-GB spike or bad_alloc → host crash, or wrapped-length garbage playback.
**Fix:** Cap before allocating, in 64-bit:
```cpp
const juce::int64 maxSrcSamples = (juce::int64) (kMaxSourceSeconds * srcRate);
const int nSmp = (int) juce::jlimit ((juce::int64) 0, maxSrcSamples, reader->lengthInSamples);
```
(and keep the `nSmp <= 0` reject).

### WR-04: `resampleToEngineRate` clamp-up to 1 output sample can over-read the input

**File:** `plugins/O-simpleSampler/Source/PluginProcessor.cpp:331-341`
**Issue:** `numOut = jlimit (1, maxOut, numOut)` forces at least one output sample even when `floor (nSmp / ratio) == 0` (i.e. `nSmp < ratio`, e.g. a 2-sample WAV at 192 kHz decoded into a 44.1 kHz engine, ratio ≈ 4.35). `juce::LagrangeInterpolator::process` then consumes ~`ratio` input samples for that one output — more than the `nSmp` samples the read pointer actually has → heap over-read past `src`'s allocation. The invariant `numOut * ratio ≤ nSmp` holds for the floor result but is broken by the clamp-up.
**Failure scenario:** A degenerate/truncated user WAV (few samples, high sample rate) is decoded → OOB read; usually silent garbage, ASan/pluginval flags it, worst-case reads an unmapped page.
**Fix:** Bail instead of clamping up: `if (numOut < 1) { auto out = std::make_shared<juce::AudioBuffer<float>> (nCh, 1); out->clear(); return out; }` before running the interpolator (or copy the input into a `ratio`-padded temp before processing).

### WR-05: `seedRootForSource` calls `setValueNotifyingHost` from `prepareToPlay` (non-message thread)

**File:** `plugins/O-simpleSampler/Source/PluginProcessor.cpp:248-252, 396-401`
**Issue:** `AudioProcessorParameter::setValueNotifyingHost` is documented message-thread-only (it drives host `beginEdit/performEdit` callbacks). `prepareToPlay` is called by many VST3/AU hosts on a non-UI audio-setup thread, so the fresh-instance root seed at line 250 violates the threading contract — some hosts assert, drop the edit, or misbehave with automation recording armed. (The `handleAsyncUpdate` call site at line 428 is fine — that's the message thread.)
**Failure scenario:** Load a fresh instance in a host that prepares on a worker thread with automation write-armed → spurious/mis-threaded parameter edit; strictest hosts (and pluginval at high strictness) flag it.
**Fix:** Defer the prepare-time seed to the message thread:
```cpp
if (! stateWasRestored && ! rootSeeded)
{
    rootSeeded = true;
    const int idx = builtInIndexForIdentity (currentSourceIdentity);
    juce::MessageManager::callAsync ([this, idx] { seedRootForSource (idx); });
}
```
(Safe here because the processor outlives the message loop in practice, but prefer routing through the existing AsyncUpdater with a dedicated pending-seed flag to avoid lifetime questions.)

### WR-06: Vendored drop-streaming module has zero C++ counterparts — 4 unregistered bridge functions and a foreign commit signature

**File:** `plugins/O-simpleSampler/Source/ui/public/modules/webview-drop-streaming.js:248, 260, 286, 304, 331, 353, 363`
**Issue:** The module calls `getNativeFunction('dropSessionStart' / 'dropSessionAddFile' / 'dropSessionCommitFolder' / 'dropSessionCommitFile')`. A grep of the plugin's C++ finds **zero** `withNativeFunction` registrations (the editor is a placeholder — no WebView exists), and no HTML/JS imports the module, so it is dead code shipped two stages early. Per the suite's documented failure mode, an unregistered native fn fails *silently* when Stage 3 wires the UI. Additional contract hazards baked into the copy: (a) `dropSessionStart` is called with 2 args on the folder path (line 248: `sessionId, folderName`) but 1 arg on the single-file path (line 331) — the future C++ registration must tolerate both arities; (b) `dropSessionCommitFolder` (lines 286-293, 304-311) passes the O-MicrotonalSampler-specific 7-arg signature (`layer, mode, override, embedAudio, technique, overrideTechnique`) which has no meaning for a single-source pedagogical sampler; (c) `cellSelector`/`cellMidiVelExtractor` presume a MIDI-cell grid this plugin doesn't have.
**Failure scenario:** Stage 3 imports the module as-is; drops appear to work in JS (toasts fire) but every bridge call rejects/undefined-returns → "Drop session start failed" toast or silent dead drop-zone, with no build/auval/harness signal (the exact `pattern_webview_native_fn_bridge_gap`).
**Fix:** Either delete the file until Stage 3 (preferred — it will be re-vendored at the then-current module version) or leave a `TODO(Stage 3)` manifest in the file header listing the four required `withNativeFunction` names and prune the commit signature to this plugin's actual folder-load options before first use. When the C++ side lands, decode with `juce::Base64::convertFromBase64` (never `MemoryBlock::fromBase64Encoding`), as the module header already mandates.

## Info

### IN-01: Per-block `dynamic_cast` over all 16 voices

**File:** `plugins/O-simpleSampler/Source/PluginProcessor.cpp:477-482` (also `:220-222`)
**Issue:** `processBlock` runs 16 `dynamic_cast<SampleVoice*>` every block. RTTI casts are allocation-free but not free (string-compare fallbacks across DSO boundaries on macOS). The voice set is fixed at construction.
**Fix:** Cache `std::array<SampleVoice*, kMaxVoices>` in the constructor when calling `synth.addVoice`, and iterate that in `processBlock`/`prepareToPlay`.

### IN-02: `std::atomic_load/atomic_store` on `shared_ptr` — deprecated and not lock-free

**File:** `plugins/O-simpleSampler/Source/PluginProcessor.h:143-152`
**Issue:** The free-function atomic shared_ptr API is deprecated in C++20 and, on libc++, is implemented via a shared spinlock pool — the audio thread's once-per-block `atomicLoad` briefly spins against any concurrent publish. Acceptable within the suite's established pattern (bounded, once per block), but worth noting alongside the CR-01/CR-02 rework: if a `CriticalSection`-guarded publisher plus retired-list lands, consider the suite's try-lock/slot pattern so the audio-thread path is provably wait-free.
**Fix:** No action required for 2.1 beyond CR-01/CR-02; document the spinlock in the header comment.

### IN-03: Every `prepareToPlay` re-decodes and re-resamples the embedded WAV

**File:** `plugins/O-simpleSampler/Source/PluginProcessor.cpp:229-242`
**Issue:** Hosts call `prepareToPlay` frequently (buffer-size change, transport re-arm, bypass toggle). Each call runs a full WAV decode + Lagrange resample of up to 30 s of audio, and re-publishes even when neither the source nor the rate changed — wasted work and extra CR-01 swap windows.
**Fix:** Cache the decoded source-rate buffer (and the identity + rate it was published at); skip the reload when `identity` and `engineRate` are unchanged.

### IN-04: Stereo sources drop the right channel instead of summing

**File:** `plugins/O-simpleSampler/Source/PluginProcessor.cpp:369-372, 443`
**Issue:** The engine reads only channel 0 of the published buffer ("mono" snapshot) — a stereo user file (Stage 2.3) or a stereo built-in loses its right channel entirely rather than being summed to mono. Documented in comments, but discard is lossy and will read as a bug to users ("my sample sounds different in the plugin").
**Fix:** When `nCh > 1`, sum channels into channel 0 with 1/nCh (or −3 dB stereo) scaling during decode, before the resample.

### IN-05: JS module small defects — first-item-only drops, `file://localhost` URI mishandling

**File:** `plugins/O-simpleSampler/Source/ui/public/modules/webview-drop-streaming.js:139-141, 465-471`
**Issue:** (a) Only `items[0]` is inspected — a multi-file drop silently ignores every file after the first, with no toast. (b) `trimmed.slice('file://'.length)` on a `file://localhost/path` URI (emitted by some hosts) yields `localhost/path` — an invalid path pushed onto the fast-path list.
**Fix:** (a) Iterate `items` (or at least toast "only the first item was loaded"). (b) Strip an optional `localhost` authority: `trimmed.replace(/^file:\/\/(localhost)?/, '')` before `decodeURIComponent`.

---

_Reviewed: 2026-07-16_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
