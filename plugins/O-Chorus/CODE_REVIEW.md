---
phase: O-Chorus-code-review
reviewed: 2026-09-30T00:00:00Z
depth: thorough
files_reviewed: 9
files_reviewed_list:
  - plugins/O-Chorus/Source/PluginProcessor.cpp
  - plugins/O-Chorus/Source/PluginProcessor.h
  - plugins/O-Chorus/Source/DSP/ChorusEngine.cpp
  - plugins/O-Chorus/Source/DSP/ChorusEngine.h
  - plugins/O-Chorus/Source/PluginEditor.cpp
  - plugins/O-Chorus/Source/PluginEditor.h
  - plugins/O-Chorus/Source/ui/public/index.html
  - plugins/O-Chorus/Source/ui/public/modules/preset-manager.js
  - plugins/O-Chorus/CMakeLists.txt
findings:
  critical: 1
  warning: 7
  info: 7
  total: 15
status: resolved
---

# O-Chorus: Code Review Report

**Reviewed:** 2026-09-30
**Depth:** thorough (processor ↔ engine ↔ editor ↔ WebView bridge, plus the shared preset-manager module)
**Tree reviewed:** working tree = committed v1.6.3 + the **uncommitted v1.7.0** R4/R5 UI pass
**Findings:** 1 critical, 7 warning, 7 info — **15 open**
**Method:** static read + analytic checks against JUCE 8.0.15 source. No renders were run. Every
number below comes from the code's own constants.

## Summary

The bridge is clean. All 8 relays have attachments, every `getNativeFunction` has a
`withNativeFunction`, parameter reads are atomic, and the v1.7.0 resource additions
(`ebgaramond_css`, three `woff2`) match the CMake embed list and the stylesheet's `../fonts/` URLs.
The factory-preset normalized values account for the Rate skew: they decode to 0.5 / 0.3 / 2.0 /
0.8 / 3.0 / 0.8 Hz.

The defects cluster in the DSP engine and in the bus layout:

- **Mono tracks get no chorus.** Mono→stereo inserts lose the dry right channel (CR-01).
- **Two of the 2026-06-30 fixes do not deliver what they claim.** The WR-03 Nyquist clamp is off by
  2×, so the upper Tone range is dead at 44.1/48 kHz (WR-01). The WR-01 negative-delay fix
  clamps to 1 sample, so the outer voices are still pinned flat at high Spread (WR-04).
- **The Voices control clicks.** The crossfade uses the old layer's LFO phases and pans, then snaps
  them at the end (WR-02). A second step during a fade drops the half-faded layer (WR-03). The
  mouse wheel does nothing on this knob (WR-06).
- **Drive steps by +2.4 dB at 1%.** It spans only +2.4 → +4.4 dB above that point (WR-05).

---

## Critical

### CR-01: Mono and mono→stereo layouts are accepted but broken **Resolved in v1.8.0**

**Files:** `Source/PluginProcessor.h` (no `isBusesLayoutSupported` override),
`Source/DSP/ChorusEngine.cpp:185`, `Source/PluginProcessor.cpp:89-90`

The processor declares stereo buses but does not override `isBusesLayoutSupported`. JUCE's default
returns `true` (`juce_AudioProcessor.h:1432`), so Logic and other hosts offer Mono and Mono→Stereo
instances.

- **Mono → mono:** `ChorusEngine::process` returns early when `numChannels < 2`. The plugin loads,
  shows its UI, responds to every knob, and passes the input through untouched.
- **Mono → stereo** (Logic's default for an effect on a mono guitar or vocal track, the most common
  chorus use): `processBlock` clears output channel 1 before the engine runs. The engine then reads
  `dryR = 0` and `monoInput = 0.5·L`. The dry signal comes out **hard left only**, and the wet path
  is fed at −6 dB.

**Fix:** Either restrict the plugin to stereo→stereo (the suite has 28 `isBusesLayoutSupported`
overrides to copy from), or accept mono and mono→stereo explicitly. For mono→stereo, copy ch0 into
ch1 before the engine, and use `monoInput = L` when the input is mono. The second option is better
for a chorus. Bump the version for Logic's per-version AU I/O cache
(critical_logic_caches_au_io_config_per_version).

---

## Warnings

### WR-01: Tone clamp is 0.245·fs, not near-Nyquist. The top of Tone is dead at 44.1/48 kHz **Resolved in v1.8.0**

**File:** `Source/DSP/ChorusEngine.cpp:122-123`

```cpp
const float nyquist = static_cast<float> (sampleRate) * 0.5f;
cutoff = juce::jmin (cutoff, nyquist * 0.49f);   // = 0.245 · fs
```

Clamp results by sample rate:

- **44.1 kHz:** the clamp is **10.8 kHz**. Tone maps 8 k→20 k over 0→+1, so everything above
  **+23%** is dead.
- **48 kHz:** the clamp is 11.8 kHz, so everything above **+31%** is dead.
- **96 kHz:** the full range is reachable. Tone therefore sounds different at each sample rate.

The Shimmer preset (+50%, meant to be about 14 kHz) plays at 10.8 kHz at 44.1 kHz. The
2026-06-30 WR-03 fix probably meant `nyquist * 0.98f` or `sampleRate * 0.49f`.

**Fix:** `cutoff = jmin (cutoff, 0.45f * (float) sampleRate)`, which gives 19.8 kHz at 44.1 kHz.
The biquad stays stable up to Nyquist, so a 0.45·fs ceiling is safe.

### WR-02: Voice-count crossfade runs the new layer on the old layout, then snaps LFO phases and pans **Resolved in v1.8.0**

**File:** `Source/DSP/ChorusEngine.cpp:98-112, 268-331`

`lfoPhaseOffset` and `panPosition` are written only by `setVoiceCount(currentVoiceCount)`. That
call happens in `prepare()` and at the **end** of a crossfade. During the 50 ms fade:

- Voices shared by both layers use the **old** count's phase and pan for their new-layer tap.
- Voices that exist only in the new layer use stale values: phase 0 and centre pan from
  construction, or whatever an earlier count left behind.

When the fade completes, `setVoiceCount(target)` rewrites every offset in one sample. At 4→8 voices,
voice 1's phase goes from π/2 to π/4, which moves its delay tap by up to
`depth·var·5 ms·|Δsin|`. That is about 1.9 ms instantly at depth 0.5, which is an audible click.
Its pan also jumps from 1/3 to 1/7.

**Fix:** Keep a second layout (phase and pan) per layer. Compute the new-layer tap with the target
count's phase, pan and spread, and let the fade hide the difference. Only relabel at the end, when
the old layer's gain is already 0.

### WR-03: Changing the voice count again during a fade drops the half-faded layer **Resolved in v1.8.0**

**File:** `Source/DSP/ChorusEngine.cpp:199-203`

```cpp
if (numVoices != targetVoiceCount) { targetVoiceCount = numVoices; crossfadeProgress = 0.0f; }
```

`currentVoiceCount` is not advanced. Take a 4→5 fade at 60%: the "5" layer is at gain 0.6. If the
count then changes to 6, the "5" layer disappears, the "4" layer jumps back to gain 1.0, and shared
voices switch spread taps in a single sample. A Voices drag or automation that steps through
several counts within 50 ms clicks at every step.

**Fix:** When a new target arrives mid-fade, either queue it until the current fade completes, or
promote whichever layer is louder to "current" and restart the fade from it.

### WR-04: High-Spread voices are still pinned flat. The 2026-06-30 WR-01 fix moved the pin to 1 sample **Resolved in v1.8.0**

**File:** `Source/DSP/ChorusEngine.cpp:250-266`

The comment says "Clamping keeps every voice modulating". It does not. The outer voice's base is
`10 − 15·s` ms, so at **Spread ≥ 0.667** it is ≤ 0 ms. With LFO excursion of at most
±5·depth·1.15 ms, the whole modulated range stays below 1 sample, and `jlimit(1, …)` holds the
voice at a static 1-sample delay. Between Spread 0.3 and 0.667 the LFO trough is flat-topped, which
gives a pitch-slope discontinuity every cycle.

Affected factory presets:

- **Ensemble** (Spread 1.0, 8 voices, depth 0.5): voice 0 (base −5 ms) and voice 1 (base −0.7 ms)
  are static near-dry copies.
- **Lush** (Spread 0.8, 6 voices): voice 0 (base −2 ms) is pinned, and voice 1 clips at its troughs.

A 1-sample copy of the mono sum adds to the dry signal in phase. That raises the level and gives an
unmodulated, slightly low-passed doubling, which undoes the chorus the preset is named for.

**Fix:** Keep every tap positive by construction. Make the spread one-sided (`0 … +15 ms`) or raise
`baseDelayMs` so that `base − spreadRange − maxDepthExcursion ≥ ~1 ms` (for example, base 22 ms).
Both options change the sound of existing presets, so this needs a MINOR bump and a listening pass.

### WR-05: Drive jumps +2.4 dB at 1% and then spans only 2 dB **Resolved in v1.8.0**

**File:** `Source/DSP/ChorusEngine.cpp:146-159`

`saturate()` passes the signal through unchanged below `drive < 0.01`. Above that it applies
`tanh((1+k)·x)/tanh(1+k)` with `k = drive·0.5`, a full tanh knee from the first step.

- **Small-signal gain:** 1.0 at drive 0.009. It jumps to 1.005/tanh(1.005) = **1.315 (+2.4 dB)**
  at 0.01, then reaches 1.5/tanh(1.5) = **1.66 (+4.4 dB)** at drive 1.0.
- **Audible effect:** the knob's real "drive" is the step at 1%. Automating or smoothing Drive down
  to 0 crosses that step mid-ramp and produces a level and timbre jump. The Vibrato preset
  (drive 0) is about 2.4 dB quieter in the wet path than it would be at drive 0.01.

**Fix:** Use a level-compensated curve that goes continuously to identity, for example
`tanh(g·x)/g` with `g = 1 + drive·G` (see pattern_level_compensated_drive_vs_makeup_gain), and
remove the `< 0.01` bypass branch. This changes the sound of every preset with drive > 0, so it
needs a MINOR bump.

### WR-06: The mouse wheel on the Voices knob does nothing **Resolved in v1.8.0**

**File:** `Source/ui/public/index.html:1272-1276`

The wheel step is ±0.02 normalized, and the value is then snapped with
`Math.round(norm·7)/7`. From any snapped value, ±0.02 is 0.14 of a step, so the result rounds back
to the same count. The wheel handler also brackets a gesture with no value change, which leaves an
empty undo step in some hosts.

**Fix:** When `p.isVoices`, step by `±1/7`. This amounts to `snapVoicesNorm(norm + sign/7)`.

### WR-07: Factory presets never refresh after the first install **Resolved in v1.8.0**

**File:** `Source/PluginProcessor.cpp:198-201`

```cpp
if (factoryDir.isDirectory() && factoryDir.getNumberOfChildFiles(juce::File::findFiles) > 0)
    return;
```

This check runs before `presetManager.initializeFactoryPresets()`. That module function has its own
version-stamped sentinel (`.factory-version`), which exists so that factory banks are rewritten
when the plugin version changes. The early return skips the sentinel check whenever any `.json`
already exists. As a result, any factory preset correction, including the ones the fixes for
WR-04 and WR-05 will need, will never reach an existing install
(pattern_factory_bank_sentinel_makes_preset_gates_read_stale_bank).

**Fix:** Delete the plugin-side guard and let the module's sentinel decide.

---

## Info

### IN-01: No `AudioProcessor::reset()` override, so `ChorusEngine::reset()` is dead code **Resolved in v1.9.0**

**Files:** `Source/PluginProcessor.h`, `Source/DSP/ChorusEngine.cpp:87-96`

A host `reset()` (transport jump, bypass-flush, offline render start) leaves up to 31 ms of old
audio in the delay lines and old state in the tone filters. That produces a burst of the previous
material at the next play. Fix: `void reset() override { chorusEngine.reset(); }`.

### IN-02: A knob drag sticks when the mouse is released outside the WebView **Resolved in v1.9.0**

**File:** `Source/ui/public/index.html:1181-1196, 1261-1269`

Dragging starts on `mousedown`, and `mouseup` is listened for on `document`. If the button is
released outside the plugin window, `mouseup` never arrives. The knob keeps following the pointer on
re-entry and the host gesture stays open. Fix: use pointer events with `setPointerCapture`.

### IN-03: The preset name display goes stale on host program changes and session restores **Resolved in v1.9.0**

**File:** `Source/ui/public/modules/preset-manager.js:173` (refresh runs only at init and after the
page's own operations)

With the editor open, changes made by `setCurrentProgram()` from the host's program menu, or by
`setStateInformation()`, update the knobs through the attachments. The preset name is never
re-pulled. Fix: re-pull `getCurrentPreset` on a low-rate timer, or push a revision counter
(pattern_webview_one_shot_state_push_stale_on_preset_load).

### IN-04: "Save…" ignores the folder the user picks **Resolved in v1.9.0**

**File:** `Source/PluginEditor.cpp:129-156`

The save dialog lets the user choose any location, but only `getFileNameWithoutExtension()` is used.
The preset is written to the user-presets directory, so a user who saves to Desktop finds nothing
there. Fix: lock the chooser to the user dir, or call `savePresetToFile(file)` and add the factory
guard that call lacks (pattern_savepresettofile_has_no_factory_guard).

### IN-05: The knobs cannot be operated from the keyboard **Resolved in v1.9.0**

**File:** `Source/ui/public/index.html:928-1045`

The `.knob` elements have no `tabindex`, `role="slider"` or `aria-value*`, and there is no arrow-key
handler. The v1.7.0 pass added `:focus-visible` styling that no knob can receive.

### IN-06: Stale "Stage 1 (Foundation) — Placeholder UI" header comment (IN-01 from 2026-06-30, still open) **Resolved in v1.9.0**

**File:** `Source/PluginEditor.h:30`

### IN-07: The Vibrato preset plays 3 dB below the input **Acknowledged in v1.9.0 — documented, not changed**

**File:** `Source/PluginProcessor.cpp:232-237`, `Source/DSP/ChorusEngine.cpp:277-281`

With mix 1.0 and one centred voice, equal-power panning gives 0.707 per side. A centred source
comes out at −3 dB, and bypass-comparing makes the effect sound weaker than it is. Fix:
compensate the wet gain by √2 when voices = 1, or accept the drop and note it in the preset.

---

## Re-adjudication of the 2026-06-30 review (`.planning/O-Chorus-CODE-REVIEW.md`)

| Prior | Title | Status now |
|-------|-------|------------|
| WR-01 | Negative per-voice delay at high Spread | **NOT FIXED.** The clamp moved the pin from "last value" to 1 sample. See **WR-04** |
| WR-02 | Double push during voice-count crossfade | FIXED (single pop-pop-push per sample, lines 290-318) |
| WR-03 | Tone filter lacks a Nyquist clamp | **FIXED BUT REGRESSED.** The clamp constant is 2× too low. See **WR-01** |
| IN-01 | Stale Stage 1 comments | OPEN. See **IN-06** |
| IN-02 | Dead code in preset-manager.js | Not re-audited (module-owned copy) |
| IN-03 | `_waitForNative` polls forever | FIXED (`maxAttempts = 100`) |
| IN-04 | Unsanitized name to `savePreset` | FIXED in module (`sanitizePresetName`) |

## Not findings (checked)

- The factory-preset normalized values decode to clean values through Rate's 0.35 skew, and the
  Voices int rounding lands on 2/6/4/8/1/3.
- The tone `IIR::Filter` default-constructs first-order. `prepare()` assigns biquad coefficients
  and then calls `reset()`, so the state reallocation happens off the audio thread. Per-sample
  `getRawCoefficients()` writes keep the filter at order 2, so there is no RT malloc
  (critical_iir_filter_default_ctor_is_first_order).
- Pop-then-push ordering on the delay lines is exact (critical_delayline_push_without_pop_shifts_delay).
- The v1.7.0 WebView resources match: all 4 embeds have `getResource` branches, and the binary
  names match `juce_add_binary_data` hyphen-stripping.

## Suggested grouping for `/improve-review`

- **PATCH:** CR-01 (with a version bump for the AU cache), WR-01, WR-06, WR-07, IN-01, IN-02, IN-06.
  None of these change the sound at stereo defaults, except that Tone now reaches its intended range.
- **MINOR (sound change + listening pass + preset re-check):** WR-02, WR-03, WR-04, WR-05, IN-07.

---

## Resolved

| Findings | Version | Commit |
|----------|---------|--------|
| CR-01, WR-01..07 | v1.8.0 | b4ebd225 |
| IN-01..06 | v1.9.0 | f5dd6a6a |
| IN-07 | v1.9.0 | acknowledged — documented in NOTES.md, not changed (equal-power pan law) |
