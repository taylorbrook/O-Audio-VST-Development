---
phase: O-Chorus-whole-plugin-review
reviewed: 2026-06-30T00:00:00Z
depth: deep
files_reviewed: 8
files_reviewed_list:
  - plugins/O-Chorus/Source/DSP/ChorusEngine.cpp
  - plugins/O-Chorus/Source/DSP/ChorusEngine.h
  - plugins/O-Chorus/Source/PluginProcessor.cpp
  - plugins/O-Chorus/Source/PluginProcessor.h
  - plugins/O-Chorus/Source/PluginEditor.cpp
  - plugins/O-Chorus/Source/PluginEditor.h
  - plugins/O-Chorus/Source/ui/public/index.html
  - plugins/O-Chorus/Source/ui/public/modules/preset-manager.js
findings:
  critical: 0
  warning: 3
  info: 4
  total: 7
status: issues
---

# O-Chorus: Code Review Report

**Reviewed:** 2026-06-30
**Depth:** deep (cross-file: PluginProcessor ↔ ChorusEngine ↔ PluginEditor, WebView↔C++ bridge)
**Files Reviewed:** 8
**Status:** issues_found

## Summary

O-Chorus is a shipped v1.2.1 multi-voice BBD-style chorus. The WebView↔C++ native-function bridge is
complete (all 10 JS `getNativeFunction` calls have matching C++ `withNativeFunction` registrations — no
silent-dead-control gap), parameter reads in `processBlock` are correctly atomic, denormals are guarded,
and `prepareToPlay` correctly reallocates delay lines on sample-rate change.

The headline defect is a **DSP delay-time bug**: at moderate-to-high Spread the per-voice delay time goes
**negative**. This was originally filed as Critical (out-of-bounds read / corruption), but verification
against the JUCE `DelayLine` source **downgraded it to a Warning** — `popSample` ignores negative delays
(the `>= 0` sentinel branch skips `setDelay`) and `setDelay` clamps via `jlimit(0, maxDelay, …)`, so there
is no OOB read, NaN, or memory corruption. The real consequence is audible quality loss: high-Spread voices
collapse toward ~0 ms delay and stop modulating symmetrically. Two further correctness issues: the delay
line is pushed twice per sample for overlapping voices during a voice-count crossfade, and the tone filter
has no Nyquist clamp so it becomes unstable at sample rates ≤ ~40 kHz. No crash, memory-safety, or
silent-dead-control defects were found.

## Warnings

### WR-01: Per-voice delay time goes negative at high Spread → modulation collapses (originally filed CR-01)

**File:** `plugins/O-Chorus/Source/DSP/ChorusEngine.cpp:223-235`

**Severity note (verified):** Filed originally as Critical "out-of-bounds read." **Downgraded to Warning**
after reading JUCE's `DelayLine` (`juce_DelayLine.cpp`): `popSample` only applies the argument when
`delayInSamples >= 0` (a negative value hits the sentinel branch and is ignored — it never indexes the
buffer), and `setDelay` clamps `delay = jlimit(0, maxDelay, newDelay)`, so `delayInt = floor(delay)` is
never negative. **There is no out-of-bounds read, no NaN, and no memory corruption.** The defect is an
audible quality/correctness bug.

**Issue:** The per-voice base delay goes negative at high Spread, so the delay no longer tracks the intended
LFO modulation.

Trace (voice `v = 0`, `count > 1`):
- `voiceOffset = curSpread * spreadRangeMs * (2*0/(count-1) - 1) = -curSpread * 15ms`
- `voiceBaseDelayMs = baseDelayMs(10) + voiceOffset = 10 - curSpread*15`  (= −5 ms at spread 1.0)
- `modulatedDelayMs = voiceBaseDelayMs + lfoValue * effectiveDepth * delayRangeMs`  → swings roughly −10..0 ms

**Failure scenario:** Load the factory **"Ensemble"** preset (`voices=8`, `spread=1.0`) or push Spread past
~0.55 with nonzero depth. For voice 0, `modulatedDelaySamples` is negative for most of the LFO cycle. Because
`popSample` ignores negatives (reuses the last-set, clamped delay) and `setDelay` clamps to 0, voice 0's
delay collapses toward ~0 ms and stops modulating — it degrades to a near-dry mono passthrough, and the
intended symmetric LFO warble becomes asymmetric (only the positive half of the swing renders). Net effect:
thinner, lopsided chorusing at high Spread. No crash.

**Fix:** Clamp the modulated delay to a positive range so every voice keeps a valid, modulating delay:
```cpp
float modulatedDelaySamples = (modulatedDelayMs / 1000.0f) * static_cast<float>(sampleRate);
modulatedDelaySamples = juce::jlimit(1.0f,
                                     static_cast<float>(maxDelaySamplesAllocated),
                                     modulatedDelaySamples);
float delayed = voice.delayLine.popSample(0, modulatedDelaySamples);
```
Better: re-center the spread so voices stay positive — raise `baseDelayMs` above
`spreadRangeMs + delayRangeMs*maxDepthVariation` (≈ 21 ms), or scale `spreadRangeMs` down so the minimum
delay stays ≥ ~2 ms. This both stops the collapse and restores symmetric modulation.

### WR-02: Delay line pushed twice per sample for overlapping voices during voice-count crossfade

**File:** `plugins/O-Chorus/Source/DSP/ChorusEngine.cpp:255-259` (with the lambda at 235-238)
**Issue:** During a voice-count crossfade both `processVoices(currentVoiceCount, ...)` and
`processVoices(targetVoiceCount, ...)` run in the same sample iteration, and each call does
`popSample` **and** `pushSample` on `voices[v]`. For the overlapping voices (indices below
`min(current,target)`) this pushes `monoInput` into the same delay line **twice per audio sample**,
advancing the write pointer at 2× the real sample rate for the ~50 ms crossfade.

**Failure scenario:** Automate or turn the Voices knob (e.g. 4 → 6). For voices 0–3 the delay buffer now
receives two identical writes per sample while only one real sample of audio elapses; a fixed-sample delay
read therefore reads input from half the intended real-time span and the buffer holds duplicated samples,
producing an audible pitch/doubling artifact for the crossfade duration. It self-heals after 50 ms, so it's
a transient glitch rather than persistent corruption — but it fires on every voice-count change.

**Fix:** Separate the read/write so each delay line is pushed exactly once per sample regardless of
crossfade. E.g. compute both the "old-count" and "new-count" contributions from a single `popSample`, and
issue the `pushSample` for each voice once outside the two weighted accumulation passes:
```cpp
// pop + accumulate for each active voice with both gains, then push once per voice per sample
```

### WR-03: Tone filter has no Nyquist clamp — unstable coefficients at sample rates ≤ ~40 kHz

**File:** `plugins/O-Chorus/Source/DSP/ChorusEngine.cpp:100-108` (cutoff from `mapToneParamToCutoff`, max 20 kHz)
**Issue:** `updateToneFilter` computes `n = 1/tan(pi*cutoff/sampleRate)` with `cutoff` up to 20 kHz and no
clamp against Nyquist. When `cutoff` approaches or exceeds `sampleRate/2`, `tan(pi*cutoff/sampleRate)`
blows up (at Nyquist) or goes **negative** (above Nyquist), yielding degenerate/negative biquad
coefficients and an unstable filter.

**Failure scenario:** Host runs at 22.05 kHz or 32 kHz (valid, and exercised by pluginval's sample-rate
sweep). With Tone at maximum (cutoff = 20 kHz > Nyquist), `tan()` is negative → `n < 0` → filter poles move
outside the unit circle → the wet signal rings/blows up to NaN/Inf, corrupting output. Even at 44.1 kHz the
20 kHz cutoff sits at 0.9× Nyquist, near the accuracy edge of the bilinear transform.

**Fix:** Clamp the cutoff to a safe fraction of Nyquist before computing coefficients:
```cpp
float nyquist = static_cast<float>(sampleRate) * 0.5f;
cutoff = juce::jmin(cutoff, nyquist * 0.49f);
```

## Info

### IN-01: Stale "Stage 1 / Placeholder UI" comments in a shipped v1.2.1 plugin

**File:** `plugins/O-Chorus/Source/PluginEditor.cpp:8`, `plugins/O-Chorus/Source/PluginEditor.h:9-12`
**Issue:** Header comments still describe the editor as "Stage 1 (Foundation) - Placeholder WebView UI",
which is misleading for a fully-featured shipped release.
**Fix:** Update the comments to reflect the final feature set (8 params, preset bar, LFO ring).

### IN-02: Dead code in preset-manager.js (unused factory + globals + never-wired options)

**File:** `plugins/O-Chorus/Source/ui/public/modules/preset-manager.js:43-44, 111-113, 316-320, 351-387`
**Issue:** `createPresetBar`, the `window.OuariconPresetManager`/`window.createPresetBar` globals, and the
`deleteButton`/`menuButton` option branches are never used by O-Chorus (index.html constructs `PresetManager`
directly and passes no delete/menu buttons). `promptDelete()` uses `confirm()`, which the module's own
comment notes may not work in JUCE WebView. This is shared-module carryover, not a defect, but it is
unreachable in this plugin.
**Fix:** Acceptable as shared-module surface; if trimming per-plugin, drop the unused exports/branches.

### IN-03: `_waitForNative` polls forever with no timeout

**File:** `plugins/O-Chorus/Source/ui/public/modules/preset-manager.js:124-135`
**Issue:** `_waitForNative` recursively `setTimeout`s every 50 ms until `window.__JUCE__.backend` exists,
with no cap. If native integration never initializes (misconfiguration, WebView2 fallback to IE on Windows),
`initialize()` never resolves and the preset bar is silently inert with no diagnostic.
**Fix:** Add a bounded retry/timeout that rejects (or logs) after N attempts so the failure is observable.

### IN-04: Generic `savePreset` native fn forwards an unsanitized name to the preset manager

**File:** `plugins/O-Chorus/Source/PluginEditor.cpp:57-63`
**Issue:** The `savePreset` native function passes `args[0].toString()` straight to
`presetManager.savePreset(...)`. Per project memory, `OuariconPresetManager` uses the name verbatim as the
JSON filename, so a name containing `/` silently fails to save. The shipped UI's Save button routes through
`savePresetWithDialog` (name derived from a file chooser, so it's safe), leaving this path unused today — but
it is exposed to the WebView and would fail silently if called with a path-separator name.
**Fix:** Reject/sanitize names containing path separators before saving, and surface the failure to the caller.

---

_Reviewed: 2026-06-30_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
