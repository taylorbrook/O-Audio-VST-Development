---
phase: O-GrainScatter-v2.4.0
reviewed: 2026-07-08
depth: deep
files_reviewed: 20
files_reviewed_list:
  - plugins/O-GrainScatter/Source/PluginProcessor.cpp
  - plugins/O-GrainScatter/Source/PluginProcessor.h
  - plugins/O-GrainScatter/Source/PluginEditor.cpp
  - plugins/O-GrainScatter/Source/PluginEditor.h
  - plugins/O-GrainScatter/Source/dsp/GrainPool.h
  - plugins/O-GrainScatter/Source/dsp/GrainScheduler.h
  - plugins/O-GrainScatter/Source/dsp/GrainTrajectory.h
  - plugins/O-GrainScatter/Source/dsp/FreezeManager.h
  - plugins/O-GrainScatter/Source/dsp/EuclideanGenerator.h
  - plugins/O-GrainScatter/Source/dsp/TempoTracker.h
  - plugins/O-GrainScatter/Source/dsp/ScaleQuantizer.h
  - plugins/O-GrainScatter/Source/dsp/DelayBuffer.h
  - plugins/O-GrainScatter/Source/dsp/TripleBuffer.h
  - plugins/O-GrainScatter/Source/dsp/LagrangeInterpolation.h
  - plugins/O-GrainScatter/Source/dsp/SpatialEncoder.h
  - plugins/O-GrainScatter/Source/dsp/BinauralDecoder.h
  - plugins/O-GrainScatter/Source/dsp/HRIRData.h
  - plugins/O-GrainScatter/Source/ui/public/index.html
  - plugins/O-GrainScatter/Source/ui/public/js/app.js
  - plugins/O-GrainScatter/CMakeLists.txt
findings:
  critical: 2
  warning: 12
  info: 15
  total: 29
status: issues_found
---

# O-GrainScatter v2.4.0: Code Review Report

**Reviewed:** 2026-07-08
**Depth:** deep (parallel three-subsystem review: [A] granular engine DSP — pool/scheduler/freeze/trajectory/euclidean/tempo/scale/delay/triple-buffer/Lagrange · [B] spatial/binaural DSP + audio processor · [C] editor/WebView bridge + UI + build)
**Files Reviewed:** 20 substantive (HRIRData.h is a pure data table; the two `js/juce/` files are vendored)
**Status:** issues_found

## Summary

O-GrainScatter is a 64-voice granular stutter effect: a Lagrange-3rd delay/freeze buffer feeds a
grain pool driven by a free/beat-synced scheduler with Euclidean gating, scale quantization, pitch
ladders, and a HOA3 → binaural spatialization stage. **The core granular DSP is well-built** — the
lock-free `TripleBuffer` viz handoff is provably race-free, all fractional buffer reads are
bounds-safe, `ScopedNoDenormals` covers the whole block, all 36 params are cached as
`std::atomic<float>*` (no per-block string lookups), `getLatencySamples()` is correctly left
un-overridden, the bus layout is locked to stereo, and **the v2.0.2 stack-overflow fix (heap
`binauralL/R` + correctly-sized `hoaBus`) is present and complete.**

The defects cluster in four areas: **(1) a dead flagship control** — the v2.4.0 "Scan" knob has a
param, DSP, DOM, and JS binding but no editor relay/attachment, so it is uncontrollable from the UI
*and* from host automation; **(2) audio-thread allocation / RT hazards** — `reset()` reallocates the
delay+freeze buffers, freeze-engage does a ~176k-element copy on the RT thread, and the spawn buffer
can overflow its reserve on large blocks; **(3) two structurally-broken audio features** — spatial-mode
feedback is a block-held constant and swing silently drops every off-beat subdivision; and **(4) the
usual UI/version drift** — hardcoded JS ranges/defaults and a `CMakeLists` version pinned three
minors behind the shipped build.

Recurring failure modes from this codebase that are handled **correctly** in O-GrainScatter:

- **`ScopedNoDenormals` at the top of `processBlock`** (PluginProcessor.cpp:385) — covers the main
  loop *and* the spatial post-loop; feedback/LPF recursion runs under FTZ/DAZ. ✓
- **All params cached as `std::atomic<float>*`** in the constructor and read via cached pointers in
  `processBlock` — no per-block hashed-string lookups. Every cached ID resolves non-null. ✓
- **`getLatencySamples()` NOT overridden** — the direct-form FIR binaural decode is genuinely
  zero-latency; correct for JUCE 8. ✓
- **v2.0.2 buffer-overflow fix holds** — `binauralL/R` are heap `std::vector` sized to
  `samplesPerBlock`; `hoaBus` is `kHOA3Channels × samplesPerBlock`; every index into
  `kBinauralFilters[16][2][128]` is provably bounded. Only the block-size *assumption* remains
  (WR-07). ✓
- **`isBusesLayoutSupported` locks stereo-in/stereo-out** (PluginProcessor.cpp:371) — `processBlock`'s
  channel-0/1 pointers can never be out of range. ✓
- **`TripleBuffer` is genuinely lock-free & race-free** (verified permutation invariant + acquire/
  release ordering); the Euclidean pattern is *copied* into the snapshot, not shared across threads. ✓
- **Both Windows WebView flags present** — `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`;
  `withUserDataFolder()` set to a temp dir. No blank-UI-on-Windows. ✓
- **Resource provider compares BARE paths** (`url == "/"`), not stripped URLs — no "Frame load
  interrupted." ✓
- **Correct `Juce` (ES module) vs `window.__JUCE__` split** — `getSliderState` et al. from the module;
  `window.__JUCE__.backend.addEventListener` only for receiving emitted viz events. ✓
- **Single `juce_add_binary_data` target** — no `BinaryData` namespace collision. **No `FileChooser`**
  in the editor (no `launchAsync` UAF surface). Editor member order (relays → webView → attachments)
  reverse-destructs correctly. ✓
- **Fractional reads NaN-safe** — `LagrangeInterpolation`, `ScaleQuantizer`, `GrainTrajectory`,
  `DelayBuffer`/`FreezeManager` reads cannot produce NaN/Inf from finite inputs; grain-length
  divide-by-zero is impossible (all paths `jmax(1, …)`). Stereo-mode feedback is `tanh`-bounded
  below unity gain even at 100%. ✓

---

## Critical (2)

### CR-01 — The v2.4.0 flagship "Scan" knob is completely dead (uncontrollable from UI *and* host automation)
- **File:** `Source/PluginEditor.cpp` / `Source/PluginEditor.h` (no `scan_position` relay/attachment — 35 relays + 35 attachments for **36** params) vs `Source/PluginProcessor.cpp:132` (param), `:32,411,541,544` (cached + live DSP), `Source/ui/public/index.html:366-367` (knob), `Source/ui/public/js/app.js:188` (binding)
- **Severity:** Critical
- **What:** `scan_position` is a real APVTS parameter that drives the grain read position through the
  0–2 s delay/freeze buffer — the entire v2.4.0 feature (`gp.positionInSamples = scanPositionSamples + spreadOffset`,
  PluginProcessor.cpp:541-544). It has a DOM knob (index.html:366) and a JS binding
  (`setupKnob('scan_position', Juce.getSliderState('scan_position'), …)`, app.js:188). But the editor
  registers **no `WebSliderRelay("scan_position")`, no `.withOptionsFrom`, and no
  `WebSliderParameterAttachment`** — verified: `grep -in scan Source/PluginEditor.*` → 0 hits; the
  editor has exactly 35 relays/attachments while `createParameterLayout` defines 36.
- **Failure scenario:** The user turns "Scan" → the indicator rotates and the readout changes % (the
  JS reads its *local* slider state), but no `valueChangedEvent` ever reaches the processor, so the
  grain read position never moves — most damaging in Freeze mode, where sweeping the frozen 2 s buffer
  is the intended musical gesture. Host **automation** of `scan_position` also can't move it (no
  attachment). The newest release ships with its marquee control inert (`pattern_webview_native_fn_bridge_gap`
  / UI-param drift).
- **Root cause:** When `scan_position` was added in v2.4.0 the param, DSP, DOM, and JS were all wired,
  but the editor relay/attachment triplet was never added.
- **Fix:** Add, mirroring any existing slider exactly: (a) `std::unique_ptr<juce::WebSliderRelay> scanPositionRelay;`
  + `std::unique_ptr<juce::WebSliderParameterAttachment> scanPositionAttachment;` in `PluginEditor.h`;
  (b) `scanPositionRelay = std::make_unique<juce::WebSliderRelay>("scan_position");` + `.withOptionsFrom(*scanPositionRelay)`
  on the WebView options; (c) `scanPositionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*audioProcessor.parameters.getParameter("scan_position"), *scanPositionRelay, nullptr);`.

### CR-02 — `reset()` reallocates the delay + freeze buffers on a thread the host may run in real time
- **File:** `Source/PluginProcessor.cpp:343,354` (`reset()`); `Source/dsp/DelayBuffer.h:9-15`, `Source/dsp/FreezeManager.h:10-19`
- **Severity:** Critical (RT heap allocation)
- **What:** `reset()` calls `delayBuffer.prepare(currentSampleRate, 2.0f)` (:343) and
  `freezeManager.prepare(...)` (:354). Both `prepare()` bodies call `juce::AudioBuffer::setSize(...)`
  with the default `avoidReallocating = false`, which **always frees and re-allocates** the heap
  storage — a 2 s × 2 ch delay buffer *and* a 2 s × 2 ch freeze buffer (≈ 6 MB of churn at 192 kHz).
- **Failure scenario:** Hosts are permitted to call `AudioProcessor::reset()` from the audio/render
  thread (transport start/stop, loop wrap, bypass toggle in several AU/VST3 hosts). Each such call
  triggers two large reallocations on the RT thread → allocator lock / priority inversion → xrun.
- **Root cause:** `reset()` reuses the heavyweight `prepare()` path to *zero* buffer contents; the
  intent (see the "// Clear delay buffer contents" comment at :342) only needs zeroing, not resizing.
  The same function already does the right thing for the spatial buffers — `hoaBus.clear()` +
  `std::fill(binauralL/R…)` at :366-368 are alloc-free.
- **Fix:** Add lightweight `clear()` methods to `DelayBuffer`/`FreezeManager` that call `buffer.clear()`
  and reset `writePosition`/`captureLength`/`active`/crossfade flags (no `setSize`), and call those
  from `reset()` instead of `prepare()`. Mirror the alloc-free `hoaBus.clear()` pattern already at :366.

---

## Warning (12)

### WR-01 — Spatial-mode feedback is a one-block-late, block-held constant → broken feedback + block-rate buzz [HIGH]
- **File:** `Source/PluginProcessor.cpp:519` (delay write consumes `feedbackL/R`), `:626` (stereo update gated `spatialMode==0`), `:684-686` (spatial feedback updated only in the post-loop)
- **Severity:** Warning (high audible impact — a shipped feature is audibly wrong)
- **What:** The per-sample main loop does `delayBuffer.pushSample(inputL + feedbackL, inputR + feedbackR)`
  every sample (:519). In stereo mode `feedbackL/R` are updated per-sample (:636-637). But in spatial
  mode (`spatialMode > 0`) they are updated **only in the post-processing loop** (:684-686), which runs
  *after* every sample has already been pushed. So for the entire spatial block, `feedbackL/R` hold the
  single value computed at the **last sample of the previous block**, and that constant is added to
  *every* input sample of the current block.
- **Failure scenario:** Spatial mode (Scatter or Trajectory) with `feedback > 0`: the feedback term
  becomes a per-block sample-and-hold — near-DC across each block, updated once per block, one block
  late. Audible symptoms: feedback that doesn't track the signal, a DC offset pumped into the delay
  buffer, and a stair-step artifact at block rate (`sampleRate/blockSize` Hz), worsening as feedback
  approaches its `±0.95` clamp. The stereo path is correct per-sample; spatial mode is not.
- **Root cause:** The binaural wet signal (that feedback is derived from) only exists after the sample
  loop, so the recursion was moved to the post-loop — but the delay write that *consumes* feedback
  stayed in the main loop, breaking the per-sample recursion.
- **Fix:** Make spatial feedback per-sample — decode the binaural output one sample at a time inside the
  main loop and feed the delay from the previous sample's binaural result; or, if block-latency is
  acceptable, at minimum ramp `feedbackL/R` across the block (don't hold one constant) and document the
  1-block latency.

### WR-02 — Swing silently drops every off-beat subdivision *and* desyncs the Euclidean step
- **File:** `Source/dsp/GrainScheduler.h:72-96` (swing gate at :78-87, euclidean advance at :95)
- **Severity:** Warning
- **What:** The swing logic runs only inside the integer-subdivision crossing block (`if (currentDiv > prevDiv)`,
  :72). At that instant `ppqAtSample` has just barely passed `currentDiv*subdiv`, but the swung target
  is `swungBoundary = currentDiv*subdiv + swingRatio*subdiv`, which is later. So `swungBoundary > ppqAtSample`
  is essentially always true → `continue` (:86). Because `currentDiv` won't increment again until the
  next integer boundary, no later sample re-enters the block — the swung grain is **never spawned**
  (dropped, not delayed). Worse, `euclideanStep` (:95) advances only *after* the swing gate, so swung
  subdivisions also skip the Euclidean advance → the pattern desynchronizes whenever swing is engaged.
- **Failure scenario:** Set `euclidean_swing > ~50.05%`. `swingRatio*subdiv` always exceeds one
  sample's PPQ, so the reject condition (:85) always fires on odd (off-beat) subdivisions → half the
  grains vanish instead of shuffling, and the Euclidean pattern drifts out of phase.
- **Root cause:** Swing reuses the integer-boundary crossing event as the swung-boundary event; the two
  are different points in time, and the test can only reject, never fire later.
- **Fix:** Detect the swung boundary with its own crossing test — shift the grid by the swing offset for
  odd divisions and compare `floor((ppqPrev - swingOffset)/subdiv)` vs `floor((ppqNow - swingOffset)/subdiv)`
  — and advance `euclideanStep` once per subdivision regardless of the swing gate so the pattern stays
  phase-locked.

### WR-03 — Freeze engage copies up to 2 s of audio element-by-element on the audio thread → click/xrun on toggle
- **File:** `Source/dsp/FreezeManager.h:21-30` (`engage`), `Source/dsp/DelayBuffer.h:44-54` (`copyRegion`)
- **Severity:** Warning
- **What:** `engage()` is called from `processBlock` (PluginProcessor.cpp:465) on the freeze off→on
  edge and synchronously runs `copyRegion` for `len` up to the full freeze buffer
  (`jmin(grainSize*4, numSamples)` — grain size 500 ms × 4 ≈ 88 200 samples). `copyRegion` (DelayBuffer.h:48-53)
  does per-element `getSample`/`setSample` with a modulo per element for both channels — ≈ 176 k calls
  + modulos inside a single sample slot of one block.
- **Failure scenario:** Toggle Freeze on while playing at a small buffer (64–128 samples): that one
  block must do ~176 k element ops on top of normal processing → deadline miss / audible click.
- **Root cause:** Whole-buffer capture done in one lump on the RT thread via the slow element-wise
  accessor rather than `memcpy`, and not amortized.
- **Fix:** Replace the element loop with `juce::AudioBuffer::copyFrom` (two contiguous `memcpy`s handling
  the ring wrap split) — ~176 k calls → 2–4 block copies.

### WR-04 — `spawnRequests` can exceed its `reserve(128)` → heap realloc on the audio thread (large/offline blocks)
- **File:** `Source/PluginProcessor.cpp:311` (`spawnRequests.reserve(128)`); `Source/dsp/GrainScheduler.h:36,101,105-110`
- **Severity:** Warning (RT allocation; narrow trigger, self-limiting after first growth)
- **What:** `spawnRequests` is reserved to 128, but `processBlockSync` pushes `1 + (repeats-1)` per
  subdivision crossing (repeats up to 16) and `processBlockFree` pushes one per interval, with no cap.
  For a large block the count exceeds 128 and `std::vector::push_back` reallocates on the audio thread.
- **Failure scenario:** Offline bounce / large host buffer. Sync mode at 1/32, `repeats=16`: any block
  > ~22 k samples produces > 128 requests; free mode @100% density overflows at > ~56 k samples. During
  a live render this is an xrun; during an offline render the deadline is soft (no dropout) and the
  vector never shrinks, so it self-limits after the first overflow — but it is still an audio-thread
  allocation and violates the zero-alloc rule.
- **Root cause:** Fixed reserve decoupled from the real worst case (`crossings × repeats`) for large
  `numSamples`.
- **Fix:** Cap the request count in the scheduler (`if (outRequests.size() >= kMaxSpawnsPerBlock) break;`)
  — a hard cap near `GrainPool::MaxVoices` (64) is musically sufficient since excess requests only steal
  voices. Add a `jassert(spawnRequests.size() <= capacity)` to catch regressions.

### WR-05 — No NaN/Inf guard on feedback or distance-LPF state (sticky NaN → permanent silence/noise)
- **File:** `Source/PluginProcessor.cpp:636-637,685-686` (feedback), `:677-678` (`distanceLpfState`)
- **Severity:** Warning
- **What:** None of the recursive state (`feedbackL/R`, `distanceLpfState[0/1]`) is checked for
  finiteness. `std::tanh` saturates *magnitude* but does not sanitize NaN (`tanh(NaN)==NaN`), and
  `std::min/max` don't filter NaN either.
- **Failure scenario:** A single non-finite sample from an upstream plugin (or a denormal edge that
  escapes) enters the feedback loop → `feedbackL = tanh(NaN·…) = NaN` → stored → added to the delay
  buffer next block → NaN grains → **permanent silence/noise until re-instantiated.** The recursive
  LPF latches NaN identically.
- **Root cause:** Recursive feedback + 1-pole LPF with no periodic sanitization.
- **Fix:** After computing `feedbackL/R` and after each LPF update, flush non-finite to zero:
  `if (! std::isfinite(feedbackL)) feedbackL = 0.0f;` (both channels, both LPF states) — the established
  `pattern_biquad_nan_guard_sticky_silence` reset pattern.

### WR-06 — Trajectory mode recomputes SH encoding (4+ trig calls) per active voice per sample → CPU/xrun risk
- **File:** `Source/PluginProcessor.cpp:584-615` (`setTarget` at :609, Doppler `sin/cos` at :599); `Source/dsp/SpatialEncoder.h` (`encodeSH16`, coeff-smoothing comment at ~:87)
- **Severity:** Warning
- **What:** `SmoothedSHCoeffs` exists precisely so trig runs only when the target changes ("Smoothing the
  16 coefficients linearly avoids per-sample trig"). But in Trajectory mode (`spatialMode==2`, non-Static
  trajectory), `setTarget()` → `encodeSH16` (2 `sin` + 2 `cos`) is invoked **every sample for every
  active voice**, plus another `sin`+`cos` for Doppler at :599.
- **Failure scenario:** 64 grains in Orbital/Spiral/Random trajectory ≈ 64 voices × ~6 transcendental
  calls × `numSamples` (e.g. 64×6×512 ≈ 196 k trig ops/block) *on top of* the 16-ch × 128-tap binaural
  convolution → xruns on modest hardware. (Scatter mode forces `trajectoryType=0` and is unaffected.)
- **Root cause:** Per-sample target recomputation defeats the coefficient-smoothing optimization.
- **Fix:** Update trajectory targets at a control rate (once per block, or every N samples); the existing
  one-pole smoothing already interpolates between updates click-free.

### WR-07 — `numSamples > samplesPerBlock` is not clamped → OOB write in HOA/binaural buffers
- **File:** `Source/PluginProcessor.cpp:325,327-328` (sizing), `:623,674-699` (writes indexed by `i < numSamples`)
- **Severity:** Warning (latent; defensive, given the file's overflow history)
- **What:** `hoaBus` and `binauralL/R` are sized to `samplesPerBlock` in `prepareToPlay`; nothing clamps
  `numSamples` to that in `processBlock`. If a host renders a block larger than the declared maximum,
  every write indexed by `i < numSamples` runs past the allocation → heap corruption.
- **Failure scenario:** A host that under-declares max block size, or a graph/bypass path that passes an
  oversized buffer. Per JUCE contract `numSamples <= samplesPerBlock`, but not all hosts honor it — and
  this is the exact buffer class v2.0.2 already had to fix.
- **Root cause:** Implicit trust in the block-size contract.
- **Fix:** Guard at the top of `processBlock`: `jassert(numSamples <= hoaBus.getNumSamples());` +
  defensive `numSamples = juce::jmin(numSamples, hoaBus.getNumSamples());`.

### WR-08 — Distance LPF cutoff recomputed per block with no smoothing → zipper on `distance`/`dist_lpf` automation
- **File:** `Source/PluginProcessor.cpp:420,424,669-671`
- **Severity:** Warning
- **What:** `distanceNorm`/`distLpfAmt` are read once per block and drive `lpfCutoff`/`lpfCoeff` (:669-670),
  which filter every sample; the values aren't smoothed, so per-block jumps step the coefficient
  discontinuously. (`dryWet`/`feedback` *are* smoothed via `SmoothedValue` — this path is the exception.)
- **Failure scenario:** Automating Distance or Distance LPF produces audible zipper/stepping on the
  binaural output.
- **Root cause:** No parameter smoothing on the distance-derived cutoff.
- **Fix:** Wrap `distanceNorm` (or `lpfCoeff`) in a `SmoothedValue` advanced per sample, matching the
  smoothing already applied to the other continuous params.

### WR-09 — `TempoTracker` accepts `bpm <= 0` verbatim → Sync mode silently stops scheduling
- **File:** `Source/dsp/TempoTracker.h:38-47`; `Source/dsp/GrainScheduler.h:63-66`
- **Severity:** Warning (host-dependent)
- **What:** `if (bpmOpt.hasValue() && ppqOpt.hasValue())` accepts `info.bpm = *bpmOpt` with no
  lower-bound check and sets `gotPosition = true`, so a host reporting `bpm == 0` (while stopped/scanning)
  never falls through to the 120 BPM fallback. `ppqPerSample = bpm/(60·sr) = 0` → in `processBlockSync`
  every sample has `currentDiv == prevDiv` → **no subdivision boundary is ever crossed → no grains
  scheduled.** (Note: this is a dropout, not a divide-by-zero — the `subdiv*60*sr/bpm` division at
  GrainScheduler.h:104 is unreachable when no crossing fires, so there is no UB.)
- **Failure scenario:** A host reports 0 BPM with a valid ppq → the Sync-mode granular engine goes
  silent until a valid tempo arrives.
- **Root cause:** No sanitization/fallback for invalid host tempo.
- **Fix:** In `TempoTracker::update`, treat `*bpmOpt <= 0` as "no position" and fall through to the
  existing 120 BPM manual fallback. (Also add a `jmax(1.0, bpm)` at GrainScheduler.h:104 to harden the
  pathological tiny-but-nonzero case — see IN-06.)

### WR-10 — `spatial_smooth` double-click reset default is wrong (ignores the 0.4 skew)
- **File:** `Source/ui/public/js/app.js:227` (`setupKnob('spatial_smooth', …, 0.1)`) vs `Source/PluginProcessor.cpp:286`
- **Severity:** Warning
- **What:** The hardcoded normalized default `0.1` doesn't match the C++ default of 5.0 ms on the skewed
  range `NormalisableRange(1, 200, 0.1, 0.4)`. The true normalized default is
  `convertTo0to1(5) = ((5-1)/199)^0.4 ≈ 0.209`.
- **Failure scenario:** Double-click "Smoothing" to reset → it snaps to ≈ 1.6 ms instead of 5 ms (~3×
  low). Every other knob's reset default is numerically correct; only this one drifts
  (`pattern_webview_knob_readout_scaled_value`).
- **Root cause:** JS-side hand-computed default that ignored the 0.4 skew.
- **Fix:** Set the default to `((5-1)/199)**0.4` ≈ `0.209` — or eliminate hand-coded defaults entirely
  via a `getParameterDefaults` native fn (see WR-11).

### WR-11 — Knob readouts + reset defaults re-implement C++ ranges/skew in JS instead of using the SliderState
- **File:** `Source/ui/public/js/app.js:114-178` (formatters hardcode ranges/skew), `:186-231` (per-knob normalized defaults)
- **Severity:** Warning (latent drift class)
- **What:** Displayed values are derived by re-implementing each C++ `NormalisableRange` (including skew)
  in JS, and reset defaults are hardcoded normalized constants. They *currently* match — but this is the
  documented fragile anti-pattern; WR-10 is the first crack (spatial_smooth already wrong). Any future
  range/skew/default change in `createParameterLayout` silently desyncs the readout and the reset with no
  build/auval error.
- **Root cause:** No use of `state.getScaledValue()` (JUCE pushes the true range/skew via
  `propertiesChanged`) and no `getParameterDefaults` native fn.
- **Fix:** Drive readouts from `state.getScaledValue()` (format only units/precision in JS) and add a
  `getParameterDefaults` native function for skew-correct double-click resets. Reusable `ui_frontend_check.js`
  shipped with O-MicrotonalSampler v1.23.7.

### WR-12 — `CMakeLists` VERSION (2.1.0) drifts from the shipped version (2.4.0)
- **File:** `CMakeLists.txt:11` (`VERSION 2.1.0`)
- **Severity:** Warning
- **What:** NOTES.md, CHANGELOG.md, and PLUGINS.md all state **2.4.0** (the scan_position/size_random/
  amp_random features are present in code), but the build metadata is pinned three minors behind.
- **Failure scenario:** Built VST3/AU report `2.1.0` to the DAW; installers, the plugin "about" string,
  and any host-side version gating are wrong. Users on 2.4.0 features see a 2.1.0 plugin.
- **Root cause:** Version bumps updated docs/CHANGELOG but not `CMakeLists.txt`.
- **Fix:** Set `VERSION 2.4.0` at CMakeLists.txt:11 (bump to the resolved patch version at ship time).

---

## Info (15)

### IN-01 — `FreezeManager::getCrossfadeGain()` is dead; freeze crossfade is not actually a gain ramp
- **File:** `Source/dsp/FreezeManager.h:67-72` — never called; `crossfadeDirection`/`crossfadeSamples` only delay `active=false` by ~5 ms. The real crossfade is implicit (grains switch source at spawn, PluginProcessor.cpp:557). Wire it in or delete it to avoid implying a ramp that isn't applied.

### IN-02 — `TempoTracker::lastPpq` is dead
- **File:** `Source/dsp/TempoTracker.h:20,45,60,69` — written in both branches, never read. Remove.

### IN-03 — Euclidean generator is not Bjorklund (produces a rotation of the canonical pattern)
- **File:** `Source/dsp/EuclideanGenerator.h:15` — the multiplicative `((i*pulses)%steps) < pulses` rule yields valid maximally-even patterns, but for some (steps,pulses) they are a *rotation* of the canonical Bjorklund result (e.g. E(5,8): gaps 2,2,1,2,1 vs Bjorklund 2,1,2,1,2). Edge cases (`steps<=0`, `pulses>steps`, `pulses=0/=steps`, `steps>16`) are all handled correctly. Note only — not a bug.

### IN-04 — `isEvenSubdiv` is misnamed (it is true for *odd* divisions)
- **File:** `Source/dsp/GrainScheduler.h:76` — `isEvenSubdiv = (divIndex % 2) != 0`. The intent (swing the off-beats) is correct; rename to `isOffBeat` for clarity.

### IN-05 — `GrainPool` has no `reset()`/`clearVoices` called from `prepareToPlay`
- **File:** `Source/dsp/GrainPool.h`; `Source/PluginProcessor.cpp:298` — voices are deactivated only in `GrainScatterProcessor::reset()` (:339), not in `prepareToPlay`. On a sample-rate/block change without a host `reset()`, active grains persist with stale `grainLengthSamples`/`readPosition`. Low impact (delay buffer is re-cleared → they read zeros and decay), but a `clearVoices()` in `prepareToPlay` is cleaner.

### IN-06 — `repeatIntervalSamples` divide is safe but should be hardened
- **File:** `Source/dsp/GrainScheduler.h:104` — `bpm==0` is unreachable here (no crossing fires), but a pathological tiny-but-nonzero `bpm` could overflow the `static_cast<int>` (UB). Add `jmax(1.0, syncInfo.bpm)`. Companion to WR-09.

### IN-07 — Repeat grains double-trigger (grain count inflates beyond `repeats`)
- **File:** `Source/dsp/GrainScheduler.h:105-110` — the `repeats-1` extra grains are scheduled at future subdivision offsets within the same block; those future subdivisions are then *also* detected as their own crossings and spawn again. May be intended "stutter," but it compounds WR-04 — confirm against design intent.

### IN-08 — Grain envelope never reaches phase 1.0
- **File:** `Source/dsp/GrainPool.h:184-185` — the final processed sample uses `samplesRemaining=1` → `phase = 1 - 1/L`, so the Hann/Blackman window never tapers fully to zero on the last sample. Negligible for typical L (≈5e-7 at L=4410). Noted for completeness.

### IN-09 — `hoaBus.setSample()` in the inner per-sample loop
- **File:** `Source/PluginProcessor.cpp:622-623` — writing via `setSample(ch, i, …)` incurs a debug bounds-check + per-call overhead each sample; cache `hoaBus.getWritePointer(ch)` outside the `i` loop.

### IN-10 — Distance has split semantics (grain gain frozen at spawn vs live LPF)
- **File:** `Source/dsp/GrainPool.h` (`distGain` uses `v.distance` frozen at spawn) vs `Source/PluginProcessor.cpp:669` (LPF uses the live block value) — moving Distance affects the LPF immediately but only affects grain gain for newly spawned grains. Document or unify.

### IN-11 — `reset()` does not reset `TempoTracker` or re-snap `SmoothedSHCoeffs`
- **File:** `Source/PluginProcessor.cpp:336-369` — `manualPpq` keeps counting and the SH coeff smoothers aren't re-snapped. Harmless (all voices are deactivated at :339), but a completeness gap vs the "clear all DSP state" intent.

### IN-12 — `releaseResources()` is empty
- **File:** `Source/PluginProcessor.cpp:334` — acceptable (JUCE re-`prepareToPlay`s before reuse), but note feedback/LPF/buffer state is only cleared in `prepareToPlay`/`reset`, not on release.

### IN-13 — Doppler uses the smoothed `shCoeffs.current[1]` as a previous-azimuth proxy
- **File:** `Source/PluginProcessor.cpp:598` — a documented heuristic, not a defect, but it lags the true previous target and under/over-estimates angular velocity right after a target change.

### IN-14 — Unused CSS class `.dimmed-spatial`
- **File:** `Source/ui/public/index.html:302-305` — the spatial gate (`app.js:274-283`) dims via inline `style.opacity`/`pointerEvents` on individual elements and never applies `.dimmed-spatial`. Dead rule; either remove it or switch `setupSpatialGate` to toggle one class for consistency with the pitch gate (`.dimmed`).

### IN-15 — Viz JSON rebuilt every timer tick regardless of visibility; rAF redraws unconditionally
- **File:** `Source/PluginEditor.cpp:194-234` (`timerCallback` builds both JSON strings before the visibility-gated emit); `Source/ui/public/js/app.js:580-585` (`renderLoop` calls `draw()` every frame). Both are low-priority CPU/alloc churn only — the timer is `stopTimer()`'d in the destructor (:181) and rAF pauses when hidden, so there's no teardown race. Optional: early-return in `timerCallback` when `!webView->isShowing()`, and gate `draw()` on a dirty flag.

---

## Recommended resolution order

1. **CR-01** (dead flagship "Scan" knob — highest user-facing impact) — add the relay/attachment triplet.
2. **CR-02** (RT reallocation in `reset()`) — add alloc-free `clear()` methods; stop calling `prepare()`.
3. **WR-01** (spatial feedback structurally broken) + **WR-05** (feedback/LPF NaN guard) — fix the spatial
   feedback recursion and add the finiteness flushes together (same code region).
4. **WR-02** (swing drops off-beats + Euclidean desync) — independent swung-boundary crossing test.
5. **WR-03** (freeze-engage `memcpy`) + **WR-04** (spawn cap) + **WR-06** (control-rate trajectory trig) —
   the RT-safety sweep.
6. **WR-07** (block-size clamp), **WR-09/IN-06** (BPM guard) — cheap defensive guards.
7. **WR-08** (distance-LPF smoothing), **WR-10/WR-11** (skew-correct readouts + `getParameterDefaults`),
   **WR-12** (CMake version) — UI/automation polish.
8. Info items as a cleanup sweep (IN-01/IN-02 dead code first).

Resolve via `/improve-review O-GrainScatter` (this file is the completed investigation — root causes and
fixes are prescribed and verified against source; confirm each against the current tree, then apply).
