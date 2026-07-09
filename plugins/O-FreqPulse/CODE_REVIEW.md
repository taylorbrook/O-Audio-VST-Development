---
phase: O-FreqPulse-v1.16.2
reviewed: 2026-07-08
verified: 2026-07-08
depth: deep
files_reviewed: 12
files_reviewed_list:
  - plugins/O-FreqPulse/Source/PluginProcessor.cpp
  - plugins/O-FreqPulse/Source/PluginProcessor.h
  - plugins/O-FreqPulse/Source/PluginEditor.cpp
  - plugins/O-FreqPulse/Source/PluginEditor.h
  - plugins/O-FreqPulse/Resources/ui/js/app.js
  - plugins/O-FreqPulse/Resources/ui/index.html
  - plugins/O-FreqPulse/Resources/ui/js/juce/index.js
  - plugins/O-FreqPulse/Resources/ui/js/juce/check_native_interop.js
  - plugins/O-FreqPulse/Resources/ui/modules/preset-manager.js
  - plugins/O-FreqPulse/CMakeLists.txt
  - modules/persistence/preset-manager/cpp/OuariconPresetManager.h
  - modules/persistence/preset-manager/js/preset-manager.js
findings:
  critical: 0
  warning: 11
  info: 14
  total: 25
status: issues_found
---

# O-FreqPulse v1.16.2: Code Review Report

**Reviewed:** 2026-07-08
**Depth:** deep (parallel three-subsystem review: DSP/RT-safety · WebView bridge/editor · presets/params/build)
**Files Reviewed:** 12
**Status:** issues_found — **no critical defects**

## Summary

O-FreqPulse is a 4-band **time-domain** multiband rhythmic gate: Linkwitz-Riley LR4 crossover tree
(binary split at c2 → c1/c3) → per-band step sequencer with per-step velocity, per-band Euclidean
generation, per-band rate/phase/step-count overrides, asymmetric attack/release envelopes → dry/wet
mix. WebView UI with a 2D frequency×time grid and an audio-thread-driven playhead. ~165 parameters
(128 of them step-velocity relays), shared `OuariconPresetManager` module for user presets.

**This plugin is in good shape.** The three-subsystem review found **zero critical issues**. The
audio thread is RT-clean, the preset round-trip is correct, and — unusually for this suite — several
of the codebase's highest-value recurring failure modes are handled *correctly* here:

- **NO engineering-units-vs-normalized preset bug.** `loadPreset()` applies `convertTo0to1()` to
  every non-`[0,1]`/skewed param (`steps`, `rate`, `attack`/`release` skew 0.4, `crossover_1/2/3`
  skew 0.3, euclidean params, `phase_offset`, band `steps`) at `PluginProcessor.cpp:777-810, 819-823`;
  factory presets store normalized `getValue()` captured *after* application (`:1154-1160`), so skew is
  baked in correctly and the round-trip is lossless. O-FreqPulse does **not** have the O-Bells CR-01
  class of bug. ✓
- **Preset reset-to-defaults present.** `applyPresetJson` sets every `RangedAudioParameter` to its
  default before applying the preset's keys (`OuariconPresetManager.h:292-294`) — no omitted-key stale
  leak on the JSON *load* path (`pattern_preset_apply_needs_reset_to_defaults` satisfied). ✓
- **Preset name `"/"` sanitized.** `sanitizePresetName` (`OuariconPresetManager.h:193-196`, replaces
  `/ \ :`) is applied in save/load/delete — not affected by `critical_preset_name_slash_path_separator`. ✓
- **processBlock RT-safety:** `ScopedNoDenormals` at the top (`PluginProcessor.cpp:440`); all params via
  cached `std::atomic<float>*` acquired in the constructor (`:184-218`); no heap alloc, no locks;
  `generateEuclidean` returns `std::array<bool,32>` **by value** (stack) and `std::rotate` is in-place, so
  even an audio-thread pattern regen is RT-safe; all per-block scratch is stack. ✓
- **JUCE-8 latency done right:** `setLatencySamples(0)` in `prepareToPlay` (`:278`); the non-virtual
  `getLatencySamples` is not overridden. Zero is correct for the IIR LR topology. ✓
- **Native-fn bridge parity CLEAN:** all 12 `withNativeFunction` registrations are consumed by JS and
  every JS `getNativeFunction` target is registered — no silently-dead control (except the *wrong-object*
  call in WR-01, which is a different bug class). ✓
- **ComboBox vs Slider channels correct** for all 5 choice params (`rate` + `band0..3_rate`); readouts use
  `getScaledValue()` for every skewed param; 128 step relays indexed consistently `band*32+step`;
  resource provider maps all 6 files by bare-path equality with MIME + fallback; editor teardown order
  correct (`stopTimer()` first). ✓
- **CMake Windows-safe:** `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` both
  present; single `juce_add_binary_data` target (no `BinaryData` namespace collision). ✓

The defects that remain are all **WARNING or below** and cluster in three areas: (a) the **tooltip
persistence feature is fully non-functional** in both directions (WR-01) — a wrong-object JS call plus a
one-shot restore race; (b) **factory-preset plumbing** hygiene — a hardcoded regeneration sentinel out of
sync with the version, a destructive first-run `deleteRecursively`, a stale step-velocity leak into
Euclidean bands, and module drift vs the shared preset-manager (WR-02/06/07/08); and (c) a few **DSP
robustness edges** that are currently unreachable but unguarded (Nyquist clamp WR-04, channel-count clamp
WR-05) plus one genuine DSP-topology transparency note (WR-09).

Two findings that would normally read as *critical* were investigated and downgraded: the FileChooser
`launchAsync` completions capture raw `this`/`complete` **but are NOT an active UAF here** (IN-01 — traced
through JUCE 8.0.9), and the `getLatencySamples`/latency reporting is **correct**, not the stale-46 ms the
docs claim (IN-02 is a docs-sync task, not a code bug).

---

## Critical Issues

**None.** No crash, RT-safety violation, broken-audio, or data-loss defect was found. See the two
"looks-critical-but-isn't" reconciliations in **IN-01** (FileChooser UAF — safe as written) and **IN-02**
(latency reporting — correct; docs are stale).

---

## Warnings

### WR-01: Tooltip persistence is fully broken in both directions (wrong-object native call + one-shot restore race)

**Files:** save path `Resources/ui/js/app.js:1067-1073`; restore path `Source/PluginEditor.cpp:333-340`
+ `Resources/ui/js/app.js:1127-1133`. C++ side is correct: `setTooltipsEnabled` registered
(`PluginEditor.cpp:102-108`), atomic persisted in APVTS XML (`PluginProcessor.cpp:687, 700-703`),
`getTooltipsEnabled` getter exists (`PluginProcessor.h:38`).

Two independent bugs jointly disable the feature:

1. **Save side** — the toggle handler calls
   `window.__JUCE__.backend.getNativeFunction('setTooltipsEnabled')(...)`. But `window.__JUCE__.backend`
   is the `Backend` class (`check_native_interop.js:117-142`), which exposes only
   `addEventListener/removeEventListener/emitEvent` — it has **no `getNativeFunction`**. That method lives
   only on the `Juce` ES-module namespace (`js/juce/index.js:73`). So the guard `if (...)` is always
   false and the native fn is never invoked → the C++ atomic stays at its default `false`. This is
   exactly the documented `critical_juce_webview_namespace_vs_postmessage` gotcha.
2. **Restore side** — `timerCallback` pushes `restoreTooltipState` exactly **once**, on the first 30 Hz
   tick (~33 ms after construction), then latches `tooltipStateSynced=true` and never retries
   (`PluginEditor.cpp:333-340`). A WebView cold start + ES-module load reliably exceeds 33 ms, so at that
   tick `window.restoreTooltipState` is usually still `undefined`; the JS guard no-ops and the state is
   never re-pushed.

**Failure scenario:** user enables tooltips → works this session but the preference never reaches C++ →
on reopen the saved value (still `false`) also never reaches the WebView. The setting silently resets
every session.

**Fix:** (1) mirror the working pattern used two lines up for `getPluginVersion` and by the preset
manager — `try { Juce.getNativeFunction('setTooltipsEnabled')(state.tooltipsEnabled); } catch (e) {}`
(`app.js:104, 1151`); (2) stop driving restore from a one-shot timer race — add a `getTooltipsEnabled`
native fn (wrap the existing getter) and have `initializeTooltips()` pull the state once the JS is ready,
or gate the C++ push on an explicit JS "page ready" call. Fix both together — they are the same feature.

### WR-02: Factory-init leaks stale step velocities into Euclidean-mode bands (baked into shipped preset JSON)

**File:** `Source/PluginProcessor.cpp:1143-1146` (init loops `loadPreset(i)` i=0..11 to *capture* each
preset to JSON). `loadPreset(int)` does not reset all params first, and `setStepVelocities()` is skipped
for bands running Euclidean mode (e.g. Dubstep Pulse case 3 bands 0/1, cases 5/6, …).

**Failure scenario:** "Dubstep Pulse" leaves bands 0/1 step-grids at the previously-loaded "Trance Gate"
velocities; those stale values are baked into the factory `.json` and recalled. Audibly harmless *while*
that band is Euclidean (the grid is ignored by `getTargetGainForBand`), but the wrong pattern appears the
instant a user switches that band to Manual.

**Fix:** in `loadPreset(int)`, reset every band's step grid (`setStepVelocities(b, empty)`) before
applying the per-preset overrides, or reset all step params up-front. (Note: the JSON *load* path is
safe — `applyPresetJson` resets-to-default first, `OuariconPresetManager.h:292-294` — this is purely a
factory-*capture* defect.)

### WR-03: `savePresetWithDialog` discards the directory the user navigates to

**File:** `Source/PluginEditor.cpp:160-186` (save at `:180`). The native save dialog opens in the User
dir but `savePreset(presetName)` always writes to `getUserPresetsDirectory()`, using only
`file.getFileNameWithoutExtension()`.

**Failure scenario:** user opens the save dialog, navigates to Desktop, types "MySound" → the file is
silently written to `~/Library/O-FreqPulse/Presets/User/MySound.json`, not the Desktop. The full
`saveMode` FileChooser implies a location choice that is ignored. (Load is asymmetric and correct —
`loadPresetFromFile(file)` honors the actual path.)

**Fix:** either write to the actually-chosen `file` path, or replace the dialog with a plain name-entry
prompt that does not imply directory navigation.

### WR-04: Crossover cutoff never clamped to Nyquist → filter blow-up / NaN below ~40 kHz sample rate

**File:** `Source/PluginProcessor.cpp:297-312` (`updateCrossoverFrequencies`); param range 20–20000 Hz
(`:60-76`).

`LinkwitzRileyFilter::setCutoffFrequency(f)` computes `g = tan(pi*f/sampleRate)`; JUCE only `jassert`s
`f < sampleRate/2` (debug). At `f == sampleRate/2`, `tan(pi/2)=inf` → `processSample` yields `inf*0 =
NaN`; just past Nyquist `g` goes negative → unstable.

**Failure scenario:** offline render at 22050 Hz (voice/podcast export) with `crossover_3` raised to
~11 kHz → NaN injected into the High band → propagates to output. At 44.1/48/96 kHz (Nyquist > 20 kHz >
max cutoff) this cannot occur, so it only bites sample rates below ~40 kHz.

**Fix:** clamp in `updateCrossoverFrequencies` before the sort/`setCutoffFrequency`:
`const float nyq = 0.49f * (float)currentSampleRate;` then `jmin` each of c1/c2/c3 against `nyq`.

### WR-05: Dry/wet block not clamped to the processed channel count (no `isBusesLayoutSupported`)

**File:** `Source/PluginProcessor.cpp:444` (`numChannels = jmin(getNumChannels(),2)`), `:601-602` and
`:667` (`AudioBlock(buffer)` wraps *all* channels and is passed whole to `pushDrySamples`/`mixWetSamples`).
DryWetMixer is prepared for 2 channels (`:239, 249`); no `isBusesLayoutSupported` anywhere in `Source/`.

**Failure scenario:** a host that hands the plugin a >2-channel buffer (surround slot, future bus change)
→ `pushDrySamples` does `getSubsetChannelBlock(0, drySamples.getNumChannels())` reading past the 2-wide
`bufferDry` → OOB / assert. Currently **unreachable** because the bus is fixed stereo (`:178-180`) and
mono is handled — but there is no guard.

**Fix (cheap, defensive):** `auto mixBlock = block.getSubsetChannelBlock(0, (size_t)numChannels);` and use
`mixBlock` for both `pushDrySamples`/`mixWetSamples`; optionally add `isBusesLayoutSupported` restricting
to mono/stereo.

### WR-06: Factory-preset regeneration sentinel is a hardcoded literal out of sync with the plugin version

**File:** `Source/PluginProcessor.cpp:1134` (guard `== "1.16.0"`) and `:1176` (write `"1.16.0\n"`). Plugin
`VERSION` is `1.16.2` (`CMakeLists.txt:8`).

The processor rolls its own factory-init (not the shared module's) and gates regeneration on a manually
maintained string literal instead of `JucePlugin_VersionString` (which the shared module already uses,
`OuariconPresetManager.h:563`).

**Failure scenario:** any future release that changes a preset definition or adds a parameter but forgets
to bump this literal ships stale factory `.json` files (the guard returns early before regen). Benign for
1.16.2 specifically (that release only added licensing — no param/preset change), so **latent, not
currently broken**.

**Fix:** compare against and write `JucePlugin_VersionString`.

### WR-07: First-run/version-change factory init `deleteRecursively` + concurrent-construction race

**File:** `Source/PluginProcessor.cpp:1137-1140` (`if (factoryDir.isDirectory())
factoryDir.deleteRecursively(); factoryDir.createDirectory();`).

This reimplements — more weakly — the write-race the shared module's sentinel was built to eliminate
(`OuariconPresetManager.h:555-564`): it deletes the whole Factory dir then writes 12 files.

**Failure scenario:** on genuine first-run/version-change, two processors constructing concurrently
(pluginval + session insertion, or two instances) both pass the sentinel and both delete/recreate; a
concurrent `getPresetList()` during the deleted-but-not-yet-repopulated window returns an empty list, and
interleaved writes can corrupt files. Low probability; correctness real.

**Fix:** overwrite the 12 files in place (no `deleteRecursively`), and/or delegate to
`presetManager.initializeFactoryPresets()` which already handles this safely.

### WR-08: `preset-manager.js` is behind the shared module (v1.0.3) — missing delete/init hardening

**File:** `Resources/ui/modules/preset-manager.js` vs `modules/persistence/preset-manager/js/preset-manager.js`.
The local copy lacks three shared-module fixes: (1) `promptDelete()` uses bare `confirm()` (`:317`) — the
shared version is `async`, adds an `onConfirmDelete` hook, wraps `window.confirm` in try/catch, and fails
**safe** (aborts) when no dialog is available; (2) `_waitForNative()` (`:124`) polls forever — the shared
version bounds it with `maxAttempts` and surfaces a console error instead of hanging `initialize()`; (3)
no `onConfirmDelete` plumbing.

**Failure scenario:** in a WebView backend where `confirm()` is a silent no-op/throw, preset delete never
fires or throws uncaught. **Latent today** — `initializePresetManager()` (`app.js:1145`) wires no delete
button and never calls `promptDelete` — but this is confirmed module drift.

**Fix:** resync `preset-manager.js` from the shared module (`module-upgrade`), or backport the three
fixes.

### WR-09: LR crossover binary tree has no allpass compensation → not magnitude-flat near c2

**File:** `Source/PluginProcessor.cpp:639-655` (the 3-stage split/sum); tree defined `PluginProcessor.h:91-96`.

A single LR4 low+high sum is an **allpass**, not identity. The tree splits at c2 → low/high halves, then
splits each half at c1/c3, so the full sum is `allpass_c1(lowHalf) + allpass_c3(highHalf)` — the two
halves carry different allpass phase and do not reconstruct flat at the c2 boundary.

**Failure scenario:** c1=400, c2=500, c3=600 (closely spaced), all bands open, mix=1 → output should equal
input but shows a phase-induced magnitude ripple/dip around c2. With the **default** well-separated
crossovers (120/500/4000, 4×/8× spacing) the error is negligible (each allpass has effectively completed
or not-yet-begun its rotation at the neighboring crossover). Audible severity is small; worst with
closely-spaced crossovers.

**Fix:** either allpass-compensate each path (run the low half through an allpass at c3 and the high half
through an allpass at c1) for exact reconstruction, or — given this is a creative rhythmic gate, not a
mastering multiband — **document** that unity-gain transparency is approximate for closely-spaced
crossovers. The doc route is the pragmatic call unless bit-transparency at rest is a stated requirement.

### WR-10: `freq_low` / `freq_high` params are created and UI-wired but never used by the DSP

**File:** params `Source/PluginProcessor.cpp:79-89`; UI relays/attachments `Source/PluginEditor.cpp:32-33,
254-257`. Confirmed by the DSP review: neither symbol is referenced in `processBlock`.

**Failure scenario:** the two frequency-boundary controls are automatable APVTS params that reset on
preset load and are captured in factory JSON, but change nothing audible. If they are intended purely as
grid-axis display bounds (JS-only), they should not be automatable DSP params; if they were meant to bound
the crossover range or scale the spectral display, that wiring is missing — a dead/half-wired control akin
to the O-Bells `material` case.

**Action (verify intended role before acting):** confirm whether `freq_low/high` should (a) drive the
grid's frequency-axis display only → move them out of APVTS or make them non-automatable UI state, or
(b) do something audible → wire them. Do not silently delete until the intent is confirmed.

### WR-11: Host native program menu is populated but dead by design

**File:** `Source/PluginProcessor.cpp:731-755`. `getNumPrograms()` returns 12 and `getProgramName()`
returns the factory names, so the DAW's built-in program dropdown is populated — but `setCurrentProgram()`
(`:741-748`) only stores the index and never loads the preset (intentional, commented).

**Failure scenario:** a user picking "Dubstep Pulse" from Logic/Cubase's native program list sees the name
change but hears nothing; only the WebView preset bar works. Confusing dead control.

**Fix:** either wire `setCurrentProgram` to `loadPreset(index)` (so the native menu works too), or return
`1` program to stop advertising a non-functional menu.

---

## Info

### IN-01: FileChooser `launchAsync` completions capture raw `this`/`complete` — NOT an active UAF here (reconciliation)

**Files:** `Source/PluginEditor.cpp:167-186` (savePresetWithDialog), `:195-213` (loadPresetFromFile).

Both completions capture `[this, complete]` and call `complete(...)` unconditionally, which pattern-matches
the documented `pattern_webview_launchasync_safepointer_no_complete` UAF. **It was traced through JUCE
8.0.9 and confirmed safe as written:** `fileChooser` is a member `unique_ptr` (`PluginEditor.h:119`), so on
editor teardown `~FileChooser()` sets `asyncCallback = nullptr` (`juce_FileChooser.cpp:130-133`); the
native completion is additionally weak-guarded on both platforms (macOS `SafePointer<Native>` +
null-check; Windows `std::weak_ptr<Native>::lock()`). The lambda cannot fire after the editor is gone, so
neither `processorRef` nor `complete` is dereferenced post-teardown.

**Recommendation (defense-in-depth, not a bug):** capture a `juce::Component::SafePointer` and bail with a
**bare `return`** on the null path (do not call `complete()`), so safety is explicit rather than dependent
on `fileChooser` remaining a member. Low priority.

### IN-02: Stale NOTES.md / REQUIREMENTS docs describe the removed FFT design + phantom "~46 ms latency"

**Files:** `NOTES.md:34-36` ("FFT-based (2048 samples, 4× overlap)… ~46ms latency (reported to DAW)"),
`.planning/REQUIREMENTS.md:52-53`, `.planning/STATUS.md:129`.

**The code is correct** — it is time-domain LR crossovers reporting `setLatencySamples(0)`
(`PluginProcessor.cpp:278`); IIR LR filters have no fixed reportable latency and the DryWetMixer delay is
0. The docs are the stale artifact of the original FFT design. Update NOTES/REQUIREMENTS to the LR +
zero-latency topology so nobody "restores" a phantom 46 ms.

### IN-03: WebView2 user-data folder points at the bare temp root, not a plugin-scoped child

**File:** `Source/PluginEditor.cpp:64-67` sets `withUserDataFolder(File::getSpecialLocation(tempDirectory))`.
The documented pattern uses `.getChildFile("OFreqPulse_WebView")`. Sharing the temp root invites
cross-plugin collision/lock contention when multiple Ouaricon WebView plugins run in the same host. Use a
plugin-scoped child folder.

### IN-04: `timerCallback` does unconditional per-frame work at 30 Hz

**File:** `Source/PluginEditor.cpp:326-330`. Every tick allocates a fresh `String::formatted(...)` and
calls `evaluateJavascript` regardless of whether any band step / `hasSignal` changed, and never idles when
audio is absent. Message-thread only (no RT concern); avoidable churn. Cache the last-sent
`(b0,b1,b2,b3,hasSignal)` tuple and skip the eval when unchanged.

### IN-05: `getTailLengthSeconds()==0.0` slightly under-reports LR ringing

**File:** `PluginProcessor.h:43`. For a multiplicative gate, output is 0 when input is 0 regardless of the
up-to-500 ms release (release only sets envelope slew, not a post-input decaying tail). The only true tail
is LR4 ringing (~a few ms). `0.0` is *almost* correct; a strict value would be a few ms. Negligible on
freeze/bounce; if desired return e.g. `0.05`.

### IN-06: `calculateCurrentStep` has no internal guard for `numSteps==0`

**File:** `PluginProcessor.cpp:393` (`... % numSteps`). Every current caller passes `jlimit(2,32,…)`
(`:451, :579`) so it is safe today, but the function itself is unguarded; a future caller passing 0 = UB.
Add `if (numSteps <= 0) return 0;` at entry.

### IN-07: Free-run fallback pins tempo to 120 BPM when transport is stopped / BPM absent

**File:** `PluginProcessor.cpp:553-555, 561-566`. When the host is not playing (or reports no BPM) but
audio flows, the sequencer free-runs at a hardcoded 120 BPM → a tempo discontinuity vs the project tempo
at the play/stop boundary. Acceptable design choice for live monitoring; flag only.

### IN-08: Per-sample recomputation of `calculateCurrentStep` ×4 bands (perf)

**File:** `PluginProcessor.cpp:613-623`. RT-safe (pure arithmetic) but ~4 calls/sample, each 2 double
divisions + modulo (~384k calls/s at 96 kHz). Could precompute per-band next-step-boundary PPQ and only
recompute on crossing. Optimization only.

### IN-09: JS *set-path* inverse-normalization uses hardcoded constants (latent drift)

**File:** `Resources/ui/js/app.js` — `(val-2)/30` steps (`:147`), `/1000` attack/release (`:201,220`),
`(v-1)/31` euc_steps/pulses (`:779,793`), `/31` euc_offset/phase (`:807,821`), `/32` band-steps (`:840`).
All are currently correct against the C++ ranges (`PluginProcessor.cpp:30-56, 128-153`). This is the
**write** side only — every *readout* correctly uses `getScaledValue()`, so the "2× wrong display" mode is
absent. Risk is future drift if a C++ range changes without updating these literals; prefer deriving from
`SliderState`/pushed properties.

### IN-10: `"steps"` param has version hint `2` while every other param uses `1`

**File:** `PluginProcessor.cpp:31`. Almost certainly a typo. Harmless (APVTS state keys on the string ID;
the version hint doesn't alter the VST3 param-ID hash). Set to `1` for consistency.

### IN-11: Factory-version-string metadata cosmetics (hardcoded `"1.0.0"` + duplicate sentinels)

**Files:** processor stamps `"version" = "1.0.0"` (`PluginProcessor.cpp:1163`) instead of
`JucePlugin_VersionString` (shared module already fixed this, `OuariconPresetManager.h:589`); and two
independent sentinels coexist — `.version` (processor's, actually used) and `.factory-version` (shared
module's `initializeFactoryPresets`, never called here). Cosmetic/duplicate-logic; prefer delegating to
the shared module (also resolves WR-06/WR-07).

### IN-12: Swing odd/even decision uses unadjusted `rawStep` → 1-sample step flicker at boundaries

**File:** `PluginProcessor.cpp:385-393`. Swing offset is decided from `rawStep = (int)(ppq/stepLength)` but
applied to `adjustedPpq`; near a step boundary the odd/even flag and the resulting step index can
momentarily disagree, briefly repeating/skipping a step. Cosmetic timing artifact; no crash.
(Negative/pre-roll PPQ is separately clamped, which is correct.)

### IN-13: Redundant first-block recompute + stale comment + dead variable

**Files:** caches init to 0 (`PluginProcessor.h:151-152`) ≠ default params, so the first `processBlock`
re-runs `updateCrossoverFrequencies()`/`updateEuclideanPatterns()` once even though both already ran in
`prepareToPlay` (`:246, :270`) — one harmless RT-safe recompute. Also: comment at `:626` says
"SmoothedValue (linear ramp)" but the code uses the custom `BandEnvelope`; `:346` computes an unused
`int globalSteps` in `updateEuclideanPatterns`. Nitpicks.

### IN-14: `applyPresetJson` fires ~330 host/listener notifications per preset switch

**File:** `OuariconPresetManager.h:292-301`. The reset-to-defaults pass (~165 `setValueNotifyingHost`)
followed by the per-preset apply (~165 more) is an acceptable WR-01-style correctness tradeoff, but on a
165-param plugin that is ~330 notifications per preset load — automation-lane spam in some DAWs. Consider
suppressing notifications for params that don't change, if it proves noisy.

---

## Suggested Resolution Order

1. **WR-01** — tooltip persistence broken both directions; only user-visible *functional* bug, clean
   two-part fix.
2. **WR-02** — factory step-velocity stale leak; wrong pattern surfaces on Euclidean→Manual switch.
3. **WR-03** — save-dialog silently ignores the chosen directory; data-in-wrong-place surprise.
4. **WR-04 / WR-05** — cheap defensive DSP clamps (Nyquist + channel count); close latent NaN/OOB edges.
5. **WR-06 / WR-07 / WR-08 / IN-11** — factory-preset plumbing hygiene; best resolved together by
   delegating factory init to the shared `OuariconPresetManager` and resyncing `preset-manager.js`
   (`module-upgrade`).
6. **WR-10** — verify `freq_low/high` intended role before acting (dead vs display-only).
7. **WR-09** — decide: allpass-compensate the LR tree (larger change) or document the approximate-unity
   tradeoff (recommended for a creative gate).
8. **WR-11** — wire the native program menu or return 1 program.
9. **IN-02 / IN-03** — docs sync + plugin-scoped WebView2 folder.
10. Remaining IN as capacity allows.

> Resolve via `/improve-review O-FreqPulse` (reads this file's CR/WR/IN IDs).
