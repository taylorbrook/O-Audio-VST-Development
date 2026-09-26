---
phase: O-AnalogEQ-code-review
reviewed: 2026-09-25T00:00:00Z
verified: 2026-09-26T00:00:00Z   # /improve-verify O-AnalogEQ v1.5.3 — PASS (v1.5.2 also PASS)
depth: thorough
files_reviewed: 10
files_reviewed_list:
  - plugins/O-AnalogEQ/Source/PluginProcessor.cpp
  - plugins/O-AnalogEQ/Source/PluginProcessor.h
  - plugins/O-AnalogEQ/Source/PluginEditor.cpp
  - plugins/O-AnalogEQ/Source/PluginEditor.h
  - plugins/O-AnalogEQ/Source/ui/public/index.html
  - plugins/O-AnalogEQ/Source/ui/public/js/i18n.js
  - plugins/O-AnalogEQ/Source/ui/public/modules/preset-manager.js
  - plugins/O-AnalogEQ/tests/ui_tip_render_check.js
  - plugins/O-AnalogEQ/tests/i18n-states.json
  - plugins/O-AnalogEQ/CMakeLists.txt
findings:
  critical: 1
  warning: 6
  info: 4
  total: 11
status: all_resolved
---

# O-AnalogEQ: Code Review Report

**Reviewed:** 2026-09-25
**Depth:** thorough
**Files Reviewed:** 10
**Status:** issues_found
**Findings:** 1 critical, 6 warning, 4 info — **11 open**

> The eleven are **all new**. The prior review's ten are every one of them FIXED and are
> recorded in the re-adjudication table below; a finding adjudicated FIXED or NOT-A-BUG
> does **not** count toward these totals. Eleven here against ten there is not a
> regression — it is a first pass over four surfaces that had never been reviewed.

## Summary

This review covers **v1.5.0** (CMakeLists `VERSION "1.5.0"`). The prior review was
`depth: standard` against **v1.1.7** on 2026-06-30 — **thirteen versions elapsed**
(1.1.8 → 1.1.11, then the four minor versions 1.2.0, 1.3.0, 1.4.0, 1.5.0 plus their
patches 1.3.1 and 1.4.1). Four whole surfaces shipped in that window and **have never
been reviewed**: the i18n canon and language-persistence path, the tooltip / hover-help
renderer and its `oaeq.tipsEnabled` switch, the Simplified Chinese (`zh-Hans`) table,
and the settings popover. This review is the first pass over all four.

**Prior findings: all ten are closed.** Every one of CR-01, WR-01..04 and IN-01..05 was
resolved in the v1.1.8–v1.1.11 window (commits `1cef1017`, `2f973a53`) and **none has
regressed** in the thirteen versions since. The re-adjudication table below records the
evidence in current code, per finding. The counts in this document's frontmatter
therefore describe **new** findings only — they are not a regression against the prior
review's ten, and a zero in a tier here does not mean a prior finding was dropped.

### Resolution status — updated 2026-09-26

**CR-02 and WR-05..WR-10 were resolved in v1.5.1** (commit `75b97ff5`, 2026-09-25).
**IN-06..IN-09 were resolved in v1.5.3** (this commit, 2026-09-26) by `/improve-review-info O-AnalogEQ`. **The open list is now empty.**

**v1.5.2 (2026-09-26)** closed two residual defects found while re-verifying those
fixes. Neither is a finding in this document; both were introduced or left open by the
v1.5.1 fixes themselves, and both are recorded here because this file is the record of
what the review's prescriptions actually produced:

- **WR-07's prescribed fix was incomplete as written.** Swapping `savePreset(name)` for
  `savePresetToFile(file)` — which is what §WR-07's *Fix* block says to do, verbatim —
  drops the `isFactoryPreset()` early-return that only the first API carries. The result
  reported success while writing a preset that `loadPreset()` can never reach, and let
  the dialog overwrite the factory bank. v1.5.2 re-imposes the guard at the call site
  (`Source/PresetSaveGuard.h`), **location-aware** rather than name-only so the
  arbitrary-path export WR-07 exists to enable is not re-broken. Gated by render-harness
  G4, verified non-vacuous against both a neutered guard and a name-only one.

- **WR-10 part 1 landed but did not deliver its stated scope.** §WR-10's fix says
  widening the scan means "[11] and [13] cover it for all 43 plugins." It did not: the
  served-set regex `[A-Za-z0-9._/-]+\.js` excludes `$`, `{` and `}`, so a
  `${CMAKE_SOURCE_DIR}/…` SOURCES entry resolved under the *plugin* root, missed, and
  was dropped with no `missing[]` report. O-AnalogEQ was covered (its SOURCES are all
  plugin-relative); **O-ReverseDelay, O-Contrabass, O-Marimba and O-MicrotonalSampler
  were not**, and printed green while embedding unscanned modules — the same shape the
  finding was opened over, one scan lower down. Fixed in `scripts/check-i18n.js`, with
  the phantom-extension and trailing-comment traps closed alongside, since both become
  false failures once unresolvable paths are reported rather than swallowed.

### Artifact path — resolved 2026-09-25

This review was originally written to `plugins/O-AnalogEQ/.planning/CODE-REVIEW.md`
(hyphen, `.planning/` subdirectory), which `/improve-review` could not find: the command
declares a **blocking** `<review_present>` precondition on
`plugins/[PluginName]/CODE_REVIEW.md` (underscore, plugin root).

That split has since been resolved. This file now lives at the canonical path, together
with the four other plugins that were on the non-standard one (`O-DigiDelay`, `O-Freeze`,
`O-Gain`, `O-Polystutter`). `/improve-review O-AnalogEQ` and `/improve-review-info
O-AnalogEQ` both resolve normally — the frontmatter key set here already matches what
they parse (`findings:` block for the severity menu, `CR-*`/`WR-*` default scope, `IN-*`
opt-in).

Three plugins (`O-MultiBandCompressor`, `O-Octagon`, `O-Prism`) keep a second, older
review under `.planning/` **deliberately** — those are retained archives, cross-linked by
`supersedes` / `superseded_by` / `previous_review` frontmatter keys from the current
review at the root path. O-Octagon's in particular still holds 34 open Info findings that
were never re-filed. Do not treat them as duplicates to clean up.

## Prior Findings: Re-adjudication

All ten prior findings were re-checked against **current v1.5.0 source**, locating each
construct **by name** — the v1.1.7 line numbers in the prior report have all drifted.

| ID | Title | Verdict | Evidence in current code |
|----|-------|---------|--------------------------|
| CR-01 | Filter coefficients heap-allocated on the audio thread every block | **FIXED** | `PluginProcessor.cpp:370-380` now calls `ArrayCoeffs::makeLowShelf` / `makePeakFilter` / `makeHighShelf` (`juce::dsp::IIR::ArrayCoefficients<float>`, aliased `PluginProcessor.h:100`), which returns a `std::array` by value — no `new`. Rebuilds are additionally gated behind `lfMoving`/`lmfMoving`/`hmfMoving`/`hfMoving` (`:331-334`); the steady-state branch at `:355-360` touches no coefficient at all. Commit `1cef1017` (v1.1.9). Matches `pattern_arraycoefficients_rt_safe_iir`. |
| WR-01 | Frequency readouts show wrong Hz (skew ignored in JS) | **FIXED** | `index.html:1172-1173` defines `FREQ_SKEW = 0.3` and `toHz = (v,min,max) => min + (max-min) * Math.pow(v, 1/FREQ_SKEW)`; all four `*_freq` formatters at `:1192-1198` route through it. Commit `1cef1017`. |
| WR-02 | No parameter smoothing — zipper noise on automation | **FIXED** | Eight `juce::SmoothedValue<float, Linear>` members (`PluginProcessor.h:118-121`), `reset(sampleRate, kSmoothingSeconds=0.03f)` and `setCurrentAndTargetValue` seeding in `prepareToPlay` (`:258-269`), and a `kSmoothingBlock = 32`-sample chunked rebuild loop in `processBlock` (`:363-383`). Commit `1cef1017`. |
| WR-03 | HF frequency not clamped to Nyquist | **FIXED** | `PluginProcessor.cpp:321-322` computes `nyquist = currentSampleRate * 0.5f` and applies `clampFreq(hz) = jmin(hz, nyquist * 0.99f)` to **all four** cutoffs at `:371, :374, :377, :380`, not just HF. Commit `1cef1017`. |
| WR-04 | FileChooser async callbacks capture `this` — use-after-free | **FIXED** | `PluginEditor.cpp:115` and `:152` construct `juce::Component::SafePointer<OuariconAnalogEQAudioProcessorEditor> safeThis(this)`; both callback **bodies** capture `[safeThis, complete]` (`:118`, `:155`), test `if (safeThis == nullptr) return;` (`:119-120`, `:156-157`), and dereference through `safeThis->audioProcessor` (`:131`, `:167`) rather than a raw `this`. Commit `1cef1017`. See the note below on the bail path. |
| IN-01 | `output_gain` processed but has no UI control | **FIXED** | Resolved by the "document the decision" arm of the prescribed fix: `PluginProcessor.cpp:87-91` now carries an explicit `NOTE (IN-01)` recording that the output knob was removed in the v1.0.5 UI simplification, that the parameter is kept for host automation and preset fidelity, and that a relay must **not** be added. Mirrored in `i18n.js:115` and `:853`. Commit `2f973a53` (v1.1.10). |
| IN-02 | Double-click reset uses 0.5, not the parameter default | **FIXED** | `index.html:1174` adds the forward map `toNorm(hz,min,max) = Math.pow((hz-min)/(max-min), FREQ_SKEW)`, and `DEFAULT_VALUES` (`:1179-1188`) is now built from the real C++ Hz defaults — `toNorm(100,30,500)`, `toNorm(500,100,2000)`, `toNorm(2000,500,8000)`, `toNorm(8000,2000,20000)` — matching `createParameterLayout` (`:43, :53, :66, :79`). Commit `1cef1017`. |
| IN-03 | Dead variable `currentParamName` in `setupDualKnob` | **FIXED** | `grep -n currentParamName plugins/O-AnalogEQ/Source/ui/public/index.html` returns **zero** matches in v1.5.0. Commit `2f973a53`. |
| IN-04 | `_waitForNative` polls indefinitely with no timeout | **FIXED** | `preset-manager.js:149-168`: `_waitForNative(maxAttempts = 100, intervalMs = 50)` bounds the poll at 5 s, logs `JUCE backend unavailable after 5s` and resolves rather than looping. Commit `2f973a53`. **Scope:** shared module preset-manager v1.0.8 (~30 consumers). |
| IN-05 | `promptDelete` relies on unreliable `confirm()` | **FIXED** | `preset-manager.js:65-69` adds an `options.onConfirmDelete` injection hook; `promptDelete` (`:352-365`) prefers it, falls back to a guarded `window.confirm()`, and **fail-safes to abort with a log** when neither is available. Commit `2f973a53`. **Scope:** shared module preset-manager v1.0.8 (~30 consumers). |

**Verdict split: 10 FIXED, 0 STILL OPEN, 0 SUPERSEDED, 0 NOT-A-BUG.**

### Notes on individual verdicts

**WR-04 — the bail path is correct, and deliberately so.** Both SafePointer null paths
`return` **without** calling `complete`. That looks like
`pattern_webview_launchasync_safepointer_no_complete`'s hung-promise trap and is not:
that same pattern file records the non-obvious gotcha that calling `complete` on the
dead-editor path is what UAFs, because JUCE 8's native-function completion holds a raw
pointer into the destroyed `WebBrowserComponent::Impl`. The editor and its page are gone
together, so the unresolved promise is moot. The current code matches the pattern's
prescription exactly. **No finding.**

**IN-05 — still an unreached path, as the prior review noted.** `grep -n
'deleteButton\|onConfirmDelete' index.html` returns zero matches, so O-AnalogEQ supplies
neither a delete button nor the confirm hook. The module-level fix is nonetheless the
right resolution: it removes the latent fragility for the ~30 consumers that do wire a
delete button.

**IN-04 / IN-05 are shared-module findings, not plugin-local.** `diff
modules/persistence/preset-manager/js/preset-manager.js
plugins/O-AnalogEQ/Source/ui/public/modules/preset-manager.js` returns **zero lines** —
the plugin's copy is byte-identical to the canonical module at `modules/registry.yaml:140`
version **1.0.8**. Any future fix in this file must land in `modules/persistence/` and
propagate, never in the plugin's copy; a plugin-local edit forks the module for one
consumer and silently diverges the other ~30.

**Stale-`<CustomState>`-child bug class: NOT-APPLICABLE.** `grep -rn
'setCustomStateCallbacks\|CustomState' plugins/O-AnalogEQ/Source/` returns **nothing**.
`CustomState` exists only in `modules/persistence/preset-manager/cpp/OuariconPresetManager.h:605,631`
and O-AnalogEQ never registers custom-state callbacks, so
`critical_preset_manager_stale_customstate_child` (the stale child shadowing a second
reopen) cannot fire here. Recorded explicitly rather than left unstated.

**Planner lead disproved: `label.level` is not an `output_gain` control.**
`i18n.js:618` `'label.level'` is consumed at `index.html:1076` by
`<div class="vu-meter-label" data-i18n="label.level">` — the **VU meter caption**, a
read-only output-level display. It is not a widget, has no relay and touches no
parameter. IN-01's resolution stands.

## New Findings

Eleven new findings, none of them a restatement of an adjudicated prior finding. IDs
continue the existing numbering per tier (`CR-02`, `WR-05..10`, `IN-06..09`).

### Gate Results

All four gates were run read-only at HEAD and **all four exit 0**. A green gate
CONSTRAINS what may be claimed here, and two of the findings below are precisely about
what these gates do *not* measure.

| Gate | Command | Exit | Result |
|------|---------|------|--------|
| check-i18n | `node scripts/check-i18n.js --plugin O-AnalogEQ` | **0** | `ALL CHECKS PASS — 1 localized plugin(s)`. 0/35 French entries unreviewed. canon v2. Assertion [16] examined the language selector → hover-help switch pairing and passed. **Assertion [12] reports "1 module(s)" — see WR-09.** |
| i18n-fr-lint | `node scripts/i18n-fr-lint.js --plugin O-AnalogEQ --verbose` | **0** | `CLEAN`. 49 rows, 0 findings across T1-T7/G1/C1/F1. 2 straight `fr === en` copies, both covered. |
| i18n-zh-lint | `node scripts/i18n-zh-lint.js --plugin O-AnalogEQ --verbose` | **0** | `GATE PASSED`. 49 rows / 35 zh entries, 0 findings across Z1-Z8/F1/R1. `BELOW SHIP BAR — entries at reviewed:'mt': 0`. Note Z6 is inert on 549 of 552 glossary terms (3 carry a measured budget), so a Chinese width overflow is NOT excluded by this zero. |
| check-ui-labels | `node scripts/check-ui-labels.js --plugin O-AnalogEQ --verbose` | **0** | `ALL CHECKS PASSED` on all three arms (en/fr/zh-Hans). 0 clipped, 0 moved, 0 intersecting; document scroll extent 920×220 on every arm. **Coverage: 11 of 11 `[data-i18n]` elements visible in at least one state — but only in one of their two states; see WR-08.** |

**What the gates constrain.** No finding below claims an i18n key is missing, a French or
Chinese string is mistyped or untranslated, a label is clipped at the shipping frame, or a
localized string reaches `innerHTML`. Those are all excluded by the four zeros above. The
UI findings are about the **states and modules the gates never enter**.

## Critical Issues

### CR-02: `isBusesLayoutSupported` is not overridden — a stereo-in/mono-out negotiation null-derefs every filter

**Label:** PLAUSIBLE (crash mechanism CONFIRMED in JUCE source; host reachability is the open question)
**File:** `plugins/O-AnalogEQ/Source/PluginProcessor.h:34-60` (no override), `Source/PluginProcessor.cpp:228-243, 337, 344-347`

`grep -rn 'isBusesLayoutSupported\|BusesLayout' plugins/O-AnalogEQ/Source/` returns
**nothing**. The base `juce::AudioProcessor::isBusesLayoutSupported` returns `true`
unconditionally, so the wrappers advertise every layout they enumerate — including
asymmetric ones — even though `BusesProperties` declares stereo in / stereo out
(`PluginProcessor.cpp:102-104`).

**Failure scenario (inputs/state → crash).** A host negotiates **2 in / 1 out**.

1. `prepareToPlay` sets `spec.numChannels = getTotalNumOutputChannels()` = **1**
   (`PluginProcessor.cpp:235`), so each `ProcessorDuplicator` allocates exactly **one**
   mono filter (`juce_ProcessorDuplicator.h`, `prepare`).
2. JUCE sizes the audio buffer at `max(totalIn, totalOut)` = **2** — the buffer width is a
   max over both directions and says nothing about which channels are outputs
   (`pattern_stereo_in_mono_out_buffer_width_lies`, proven on O-Octagon v1.11.0 against
   auval's own `(2,1)` config).
3. `processBlock` wraps the whole buffer: `juce::dsp::AudioBlock<float> block(buffer)`
   (`:337`), and `processChunk` builds a `ProcessContextReplacing`, where input and output
   block are the **same 2-channel block** (`:342`).
4. `ProcessorDuplicator::process` computes
   `numChannels = jmin(inputBlock.getNumChannels(), outputBlock.getNumChannels())` = **2**
   and loops `processors[(int) chan]->process(...)`. Its two `jassert`s are **debug-only**.
   In a Release build `processors[1]` is out of range on an `OwnedArray`, which returns
   **`nullptr`**, and the loop dereferences it. **Null-pointer crash on the audio thread,
   at the first block, in the host.**

All four bands (`lfFilter`, `lmfFilter`, `hmfFilter`, `hfFilter`, `:344-347`) take this
path; `saturation` and `outputGain` are channel-agnostic and do not.

**Why it has not been seen:** `auval -v aufx OuAE OuDv` passed at v1.5.0. That does not
clear this — auval's pass says the configs auval actually rendered did not crash, not that
`(2,1)` was among them. Reachability is the one unproven link; the crash itself is read
straight out of `juce_ProcessorDuplicator.h`.

**Fix** (closes it regardless of which layouts a host offers):
```cpp
bool isBusesLayoutSupported (const BusesLayout& layouts) const override
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;   // no asymmetric negotiation
}
```
Declare it in `PluginProcessor.h` beside the other overrides. Then drive the `(2,1)` and
`(1,2)` configs in the render harness to prove the rejection — an assertion that only
exercises `(2,2)` is vacuous here (`pattern_gate_stimulus_below_threshold_is_vacuous`).

## Warnings

### WR-05: `output_gain` is not smoothed — `dsp::Gain`'s default ramp is zero seconds

**Label:** CONFIRMED
**File:** `plugins/O-AnalogEQ/Source/PluginProcessor.cpp:317` / `:242, :249`, `Source/PluginProcessor.h:108`

The prior review's WR-02 text asserted that "`output_gain` is also applied via `Gain`
(which smooths internally)" and excluded it from the smoothing fix on that basis. **That
assertion is false.** `juce::dsp::Gain` declares `double sampleRate = 0,
rampDurationSeconds = 0;` (`juce_dsp/widgets/juce_Gain.h:151`) and `reset()` calls
`gain.reset (sampleRate, rampDurationSeconds)` (`:87-90`). With a ramp of **0**, the
internal `SmoothedValue` has zero steps to target and `setGainDecibels` → `setGainLinear`
→ `setTargetValue` **jumps instantly**. `setRampDurationSeconds` is never called anywhere
in this plugin — `grep -rn 'setRampDurationSeconds' plugins/O-AnalogEQ/Source/` is empty.

So the v1.1.9 smoothing work covered the eight band frequency/gain values
(`PluginProcessor.h:118-121`) and left the one remaining gain stage stepping per block.

**Failure scenario.** Audio is playing. The user loads the **"Surgical Cut"** factory
preset, whose `output_gain` is `0.542` (`PluginProcessor.cpp:220`) against the `Default`
preset's `0.5` (`:121`) — normalised `0.5 → 0.542` over a `NormalisableRange<float>(-12,
12, 0.1)` is **0 dB → +1.008 dB**, a ~12 % linear step applied at the block boundary in a
single sample. Audible click. Every band around it ramps over 30 ms
(`kSmoothingSeconds`, `PluginProcessor.h:127`) while this one does not, so the click is
the only discontinuity in an otherwise smooth preset change. Host automation of "Output
Gain" — the only other way to reach it, since IN-01 established it has no UI control —
zippers for the same reason.

**Fix:** one line in `prepareToPlay`, beside the existing `outputGain.prepare(spec)`:
```cpp
outputGain.setRampDurationSeconds (kSmoothingSeconds);   // 30 ms, matching the bands
outputGain.prepare (spec);
```
Order matters: `setRampDurationSeconds` calls `reset()` internally, and `prepare` sets the
sample rate, so set the duration and then prepare (or set it after `prepare` and before
`reset`). Verify with a render-harness null test that steps `output_gain` mid-buffer and
asserts the per-sample first difference stays under a zipper threshold
(`pattern_zipper_gate_absolute_step_is_gain_staging_dependent` — the threshold is
gain-staging dependent, so derive it rather than copying one).

### WR-06: Band on/off is a hard bypass over stale filter state — a click on toggle, a transient burst on re-enable

**Label:** CONFIRMED
**File:** `plugins/O-AnalogEQ/Source/PluginProcessor.cpp:344-347`, `:288, :293, :298, :302`

```cpp
if (lfOn)  lfFilter.process(context);
if (lmfOn) lmfFilter.process(context);
if (hmfOn) hmfFilter.process(context);
if (hfOn)  hfFilter.process(context);
```

The four `*_on` booleans are read once per block (`:288, :293, :298, :302`) and used as a
raw branch. Two distinct defects follow, and neither is covered by the WR-02 smoothing
work, which smooths only frequency and gain:

1. **Discontinuity on the falling edge.** With `lf_gain` at, say, +9 dB, toggling
   `lf_on` off removes the whole shelf between one block and the next — an instantaneous
   spectral change, no crossfade, no ramp. On sustained low-frequency content that is an
   audible click, and it is fully automatable from a DAW lane.
2. **Stale state on the rising edge — the worse half.** A bypassed band's
   `ProcessorDuplicator` is **not** reset and is **not** fed. Its biquad delay elements
   `z⁻¹`/`z⁻²` retain the last samples they saw before the bypass, which may be from
   minutes earlier and at a completely unrelated level. Re-enabling the band convolves
   those stale states into the current signal, producing an impulse-like transient whose
   amplitude is set by whatever was playing at bypass time — loud if the band was
   disabled during a loud passage and re-enabled during a quiet one.

**Failure scenario.** Automate `hf_on` off during a loud chorus and back on during a
quiet intro. The first block after re-enable is driven by `z⁻¹`/`z⁻²` holding chorus-level
samples against intro-level input: a burst well above the surrounding material, on every
re-enable, deterministically reproducible.

**Fix:** smooth the bypass rather than branching on it. Give each band a
`SmoothedValue<float>` wet/dry mix driven from the boolean (0 → 1 over
`kSmoothingSeconds`), run the filter **unconditionally** into a scratch block and
crossfade — which fixes both halves at once, because a band that is always fed never
holds stale state. The cheaper partial fix — `lfFilter.reset()` on the rising edge —
removes the burst but leaves the click, and a `reset()` is only safe on the control
thread's behalf if it is done from `processBlock` on an edge detect (it writes no memory,
so it is RT-safe here). Prefer the crossfade. Whichever is chosen, gate it in the render
harness by toggling mid-buffer and asserting the sample-to-sample first difference —
note `pattern_pointer_moved_is_not_audio_changed`: assert on the rendered audio, not on
the flag.

### WR-07: "Save Preset" dialog silently ignores the directory the user chose

**Label:** CONFIRMED
**File:** `plugins/O-AnalogEQ/Source/PluginEditor.cpp:106-138` (specifically `:129-131`)

```cpp
auto file = results.getFirst();
auto presetName = file.getFileNameWithoutExtension();
bool success = safeThis->audioProcessor.presetManager.savePreset(presetName);
```

The handler takes the user's chosen `juce::File`, **discards everything but its base
name**, and calls `savePreset(name)`, which writes to
`getUserPresetsDirectory().getChildFile(sanitizePresetName(name) + ".json")`
(`OuariconPresetManager.h:406-407`) — i.e. always to
`~/Library/O-AnalogEQ/Presets/User/`, never to the chosen path.

The shared module already exposes the correct API and has since v1.0.8:
`savePresetToFile(const juce::File&)` is documented "Save the current state to an
arbitrary file path (Save As / export). Unlike savePreset(), …"
(`OuariconPresetManager.h:138-142`) and handles the extension and parent-directory
creation itself (`:423-440`). The editor simply does not call it.

**Failure scenario.** The user picks **Save Preset**, navigates to `~/Desktop`, types
`Mix Bus EQ`, and confirms. The dialog closes, the JS receives `{success: true, name:
"Mix Bus EQ"}`, and the preset name display updates — so every signal says it worked.
`~/Desktop/Mix Bus EQ.json` **does not exist**. The user cannot find the file to share
it, and nothing anywhere reported a problem. The name-collision case is worse: saving to
`~/Desktop/Default.json` silently **overwrites** the existing user preset `Default` in
the library directory, which the user never navigated to and does not expect to have
touched.

**Fix:** call the API that exists.
```cpp
auto file = results.getFirst();
bool success = safeThis->audioProcessor.presetManager.savePresetToFile(file);
auto presetName = file.getFileNameWithoutExtension();
```
If writing into the library directory rather than the chosen one is genuinely the intent,
then the dialog is the wrong control and it should be a name prompt — but the current
combination (a directory picker whose directory is discarded) cannot be the intent.
Note that `loadPresetFromFile` at `:167` already does use the arbitrary-path API
correctly, so load and save are currently asymmetric.

### WR-08: The v1.4.0 hover-help switch is entirely ungated — no test drives `tipsEnabled === false`

**Label:** CONFIRMED
**File:** `plugins/O-AnalogEQ/tests/ui_tip_render_check.js` (1014 lines, `:432`, `:893`), `plugins/O-AnalogEQ/tests/i18n-states.json`, against `Source/ui/public/index.html:1720-1745, 1815-1820`

`grep -n 'tips-toggle\|tipsEnabled\|data-tip-always\|ui.off' tests/ui_tip_render_check.js`
returns **exactly one line**: `:422`, where `#tips-toggle` appears in the `anchorsOpen`
array — i.e. the gate hovers the switch to read *its* tooltip and never presses it.
`grep -n 'click' tests/ui_tip_render_check.js` finds two `page.click` calls (`:432`,
`:893`) and **both are `#gear-btn`**. `tests/i18n-states.json` declares two states, and
both are also opens — `#gear-btn` and `#presetName`.

So the entire v1.4.0 feature ships behind zero behavioural assertions:

- the show gate `if (!tipsEnabled && !el.hasAttribute('data-tip-always')) return;`
  (`index.html:1817`),
- the `data-tip-always` bypass that keeps `#gear-btn` and `#tips-toggle` explaining
  themselves while help is off,
- the `hideTip()` call in `applyTipsEnabled` (`:1726`) that pulls a standing tip down at
  the instant the switch goes Off,
- and the `localStorage` round-trip through `oaeq.tipsEnabled` (`:1739-1744`).

**Failure scenario.** Delete line `index.html:1817` — the show gate itself. The switch
becomes decorative: it flips `aria-pressed`, relabels itself and persists a value that
nothing reads, while hover-help keeps appearing. `ui_tip_render_check.js` still reports
**415 PASS**, `check-i18n` still exits 0 (the keys are all still present and referenced),
`check-ui-labels` still exits 0 (the surface is `pointer-events: none` decoration it
never counts as a label), and `i18n-fr-lint`/`i18n-zh-lint` never look at behaviour at
all. A user who turns hover-help off gets it anyway, and no gate in the repo says so.
This is the shape `pattern_gate_passes_because_of_a_different_bug` names: everything
green, because the thing measured is not the thing that broke.

**Fix:** add a tips-off arm to `ui_tip_render_check.js`, derived rather than spelled — it
already derives its language list from the table's own export behind a derive-or-abort
guard (`:228-233`), and the switch arm should follow the same discipline. Minimum
assertions, each of which fails on a distinct single-line deletion:
1. after `page.click('#gear-btn')` then `page.click('#tips-toggle')`, hovering a **normal**
   anchor (e.g. `.dual-knob-container[data-param-outer="lf_freq"]`) yields **no** visible
   `#tooltip`;
2. in that same state, hovering `#gear-btn` **does** — proving `data-tip-always` is live,
   not that the whole renderer died;
3. with a tip standing open, pressing the switch hides it within a frame — proving the
   `hideTip` coupling (`:1726`, `:1888`) rather than just the show gate;
4. reload with `localStorage['oaeq.tipsEnabled'] = 'false'` pre-seeded and assert the
   switch comes up reading **Off** with `aria-pressed="false"`.

Assertion 2 is the one that matters most: without it, a fix that disables the renderer
wholesale passes assertion 1.

### WR-09: The switch's **Off** label is never geometry-measured, in any of the three languages

**Label:** CONFIRMED
**File:** `plugins/O-AnalogEQ/tests/i18n-states.json`, against `Source/ui/public/index.html:704-718` and `Source/ui/public/js/i18n.js:674-675`

`check-ui-labels` drives the page through the states declared in `tests/i18n-states.json`.
That file holds two entries — `click: "#gear-btn"` and `click: "#presetName"` — so the
hover-help switch is measured **only in its `ui.on` state**. Its coverage line reads
"11 of 11 `[data-i18n]` elements were VISIBLE in at least one state", which is true and
is exactly the blind spot: a two-state control counted once.

The two states are not the same width. `i18n.js:674-675`:

| key | en | fr | zh-Hans |
|-----|----|----|---------|
| `ui.on` | `On` | `Marche` | `开` |
| `ui.off` | `Off` | `Arrêt` | `关` |

and `.settings-toggle` (`index.html:704-718`) is `flex: 0 0 auto` with
**`min-width: 42px`** — a **floor, not a cap**, so the box grows with its content and the
row grows with the box. `pattern_language_width_pins_content_sized_boxes_move` is the
governing trap: a min-width pin is a floor, and "already pinned" is not "safe".

**Failure scenario.** French `Arrêt` at `font-size: 9px` is wider than `Marche`'s
measured box is not — but nothing in this repo has ever measured either. A future copy
revision to the Off rendering (`Désactivé` is the suite's own glossary phrasing for the
adjacent `toggle hover help` root, per the v1.4.1 CHANGELOG entry) pushes past the
popover's width; the row reflows or the caption clips; `check-ui-labels` exits 0 because
it never rendered that word; `i18n-fr-lint` exits 0 because the string is typographically
correct; the user sees a broken panel. The Chinese arm is additionally unprotected by
Z6, which the gate itself reports inert on 549 of 552 glossary terms.

**Fix:** add a third state to `tests/i18n-states.json` that reaches the Off arm. The file's
schema supports only a single `click`, so this needs either a click **array** or a second
entry that the harness applies cumulatively — check `scripts/check-ui-labels.js`'s state
reader before choosing, and extend the schema if it takes one click only:
```json
{
  "name": "settings popover open, hover-help OFF (ui.off on the switch)",
  "click": ["#gear-btn", "#tips-toggle"]
}
```
Assertions [4]/[5]/[6]/[7]/[8] then cover the Off rendering on all three arms with no
further change.

### WR-10: `check-i18n`'s JS scan sees one module; the page ships two — five untranslated `title=` attributes sit in the blind half

**Label:** CONFIRMED
**Scope:** shared module preset-manager v1.0.8 (~30 consumers)
**File:** `modules/persistence/preset-manager/js/preset-manager.js:411-427` (identical copy at `plugins/O-AnalogEQ/Source/ui/public/modules/preset-manager.js`), against the `check-i18n` [12] output

`check-i18n` assertion [12] reports, verbatim:

> `PASS: [12] there is shipped page JS to scan — 1 module(s): the inline <script type="module"> in index.html`

and assertion [11] then passes on that basis:

> `PASS: [11] zero native title= remain, in the MARKUP AND IN THE JS (contract §4 — a native title renders a second, untranslated OS tooltip competing with the measure-then-pin renderer; a JS-written one is invisible to the markup scan)`

But `index.html:1146-1156` imports **three** ES modules, and the resource provider serves
all three (`PluginEditor.cpp:337-342, :344-349, :373-378`): `./js/juce/index.js`
(upstream, correctly excluded), `./js/i18n.js`, and **`./modules/preset-manager.js`** —
first-party shipped page JS that assertion [12] does not count.

Inside that blind half, `createPresetBar` writes a static markup template
(`preset-manager.js:419-427`) carrying **five hardcoded English `title=` attributes**:
`title="Previous preset"`, `"Next preset"`, `"Load preset from file"`, `"Save preset"`.
These are precisely what contract §4 deletes rather than localizes, and they are
JS-written, which is the exact case assertion [11]'s own message says it exists to catch.

**Failure scenario.** O-AnalogEQ does not call `createPresetBar` today — it constructs
`new PresetManager({...})` directly (`index.html:1510-1523`) — so the five are currently
dead *on this page*. They are not dead code: the module exports the function and assigns
`window.createPresetBar` unconditionally (`preset-manager.js:445`). Any consumer among the
~30 that adopts the factory — or a future O-AnalogEQ refactor that does — ships five
untranslated OS tooltips that render **on top of** the measure-then-pin renderer this
plugin just built, in a language unrelated to the page's, with `check-i18n` still exiting
0 and reporting "zero native title= remain". The gate's green is a statement about one
file, printed as a statement about the page.

**Fix (two parts, and the first alone is not enough).**
1. **Widen the scan.** Teach `scripts/check-i18n.js` assertion [12] to enumerate shipped
   page JS from the **resource provider's served set** (the `getResource` branches in
   `PluginEditor.cpp`), excluding `js/juce/` as upstream, rather than from the inline
   `<script type="module">` alone. Then [11] and [13] cover it for all 43 plugins. Expect
   this to turn up findings in other plugins' served modules; that is the point.
2. **Remove the five `title=`** from `createPresetBar`'s template in
   `modules/persistence/preset-manager/js/preset-manager.js` and re-propagate the module.
   **This must land in `modules/persistence/`, never in the plugin's copy** — `diff`
   against the canonical file is currently zero lines, and a plugin-local edit forks
   preset-manager for one consumer and silently diverges the other ~30. Replace the
   `title=` attributes with `aria-label`, which is what the suite's contract keys and what
   the renderer does not compete with.

## Info

### IN-06: The settings-popover contract comments still describe a one-row panel, two minor versions after it grew a second row — **Resolved in v1.5.3**

**Label:** CONFIRMED
**File:** `plugins/O-AnalogEQ/Source/ui/public/index.html:1082-1083` and `:1652-1653`

Two comment blocks assert a panel shape the markup twenty-five lines below them
contradicts. At `:1082-1083`, immediately above `.settings-cluster`:

> `STILL ONE ROW AT v1.3.0, and #gear-btn's tip says so. This plugin now HAS hover-help but no on/off TOGGLE for it: adding one means a second control here…`

and at `:1652-1653`, heading the popover's JS section:

> `ONE row, at v1.3.0 as at v1.2.0: the language selector alone. The hover-help this version adds is always on and has no toggle here, and tip.settings' body is written to promise exactly this one control and nothing else`

v1.4.0 added exactly that second row — `<div class="settings-row">` holding
`label.hoverHelp` and `#tips-toggle` (`index.html:1114-1121`). The second comment also
misreports `tip.settings`, whose body was corrected in v1.5.0 and now reads "Opens the
settings panel above this button. It closes again on a click outside it or on Escape."
(`i18n.js:434-445`) — it no longer promises one control, so the comment describes a
constraint that was deliberately lifted.

**Failure scenario.** This is the `/improve-review-info` "class-contract comment three
releases stale" shape, and its cost is the next executor's model of the file. The
`:1082-1083` block reads as an argument **against** adding the toggle ("adding one means a
second control here, a preference persisted through C++ and a data-tip-always bypass") —
so an executor adding a third row would reasonably conclude the work is unbuilt,
re-derive the `data-tip-always` bypass that already exists, and re-litigate a decision
that shipped. No behaviour changes; the risk is entirely the compiler's.

**Fix:** rewrite both blocks against v1.5.0. The `:1082` block should record that the
panel holds **two** rows and that `data-tip-always` is carried by `#gear-btn` and
`#tips-toggle` and nothing else (which the `:1104-1112` comment already states correctly —
keep that one). The `:1652` block should drop the `tip.settings` claim entirely, since
tying a comment to a copy string is what made it go stale.

### IN-07: 16 `getRawParameterValue(StringRef)` lookups per block — a per-block ordered-map walk, not an allocation — **Resolved in v1.5.3**

**Label:** CONFIRMED (and deliberately **not** escalated — see below)
**File:** `plugins/O-AnalogEQ/Source/PluginProcessor.cpp:286-305`

`processBlock` opens with sixteen `parameters.getRawParameterValue("…")->load()` calls.
Each resolves through `AudioProcessorValueTreeState::getParameterAdapter`, which does
`adapterTable.find (paramID)`
(`juce_AudioProcessorValueTreeState.cpp:338-341`).

**This is not an RT-safety violation, and the distinction is load-bearing.**
`adapterTable` is declared
`std::map<StringRef, std::unique_ptr<ParameterAdapter>, StringRefLessThan>`
(`juce_AudioProcessorValueTreeState.h:662`) — keyed on `StringRef`, not `juce::String`.
A `find` with a `StringRef` therefore constructs **no** `juce::String` and allocates
nothing; it walks a red-black tree doing string comparisons. The tempting reading — "a
string-keyed map lookup on the audio thread must be allocating" — is wrong here, and
recording *why* it is wrong is the point of this entry: a future reviewer should not
re-raise it as a CR-01-class blocker.

What it does cost is roughly `16 × log₂(16) ≈ 64` string comparisons per block, every
block, forever, to re-derive sixteen pointers that are **fixed for the processor's
lifetime**. At a 64-sample buffer and 48 kHz that is ~48 000 comparisons per second of
pure overhead.

**Failure scenario:** none — this cannot produce wrong output or a dropout on its own. It
is a measurable waste on a plugin instantiated across many tracks, and it is the one place
left where `processBlock` does work proportional to something other than the audio.

**Fix:** cache the sixteen `std::atomic<float>*` once and read through them.
```cpp
// PluginProcessor.h, private:
std::atomic<float>* pLfFreq = nullptr;  /* …one per parameter… */
// PluginProcessor.cpp, in the constructor (after `parameters` is constructed):
pLfFreq = parameters.getRawParameterValue ("lf_freq");
// processBlock:
const float lfFreq = pLfFreq->load();
```
The constructor, not `prepareToPlay` — the adapter table is built with the APVTS and the
pointers are stable for the object's lifetime, so there is nothing to re-resolve on a
sample-rate change. Verify with `pattern_malloc_logger_alloc_gate_two_traps` only if the
cache is combined with other `processBlock` work; for this change alone a before/after
comparison count is the honest measurement.

### IN-08: A second dialog launch destroys the first `FileChooser` mid-flight — **Resolved in v1.5.3**

**Label:** PLAUSIBLE
**File:** `plugins/O-AnalogEQ/Source/PluginEditor.cpp:107-111` and `:146-150`, `Source/PluginEditor.h` (`fileChooser` member)

Both `savePresetWithDialog` and `loadPresetFromFile` assign to the **same**
`std::unique_ptr<juce::FileChooser> fileChooser` editor member:
```cpp
fileChooser = std::make_unique<juce::FileChooser>(…);
```
A second assignment destroys the first `FileChooser` while its native dialog may still be
presented and its `launchAsync` completion still pending.

**Failure scenario.** The page calls `savePresetWithDialog` (dialog opens), and before it
is dismissed the page calls `loadPresetFromFile`. The save chooser is destroyed
underneath its own open dialog; depending on backend, the pending save completion either
never fires — leaving that JS promise unresolved with the editor still **alive**, which is
the genuine hung-promise case WR-04's bail path is *not* — or fires against a destroyed
`Pimpl`.

**Why PLAUSIBLE rather than CONFIRMED:** reaching it requires the page to issue a second
native dialog call while the first is open. Both entry points are buttons inside the
WebView, and a platform-modal dialog normally prevents the second click — but
"normally" is per-backend, and nothing in this code enforces it.

**Fix:** guard the entry, which is cheaper than proving each backend's modality:
```cpp
if (fileChooser != nullptr) { complete (juce::var (false)); return; }   // one at a time
fileChooser = std::make_unique<juce::FileChooser> (…);
```
and clear `fileChooser` at the end of each completion body (via `safeThis`, inside the
non-null branch, so the dead-editor path still touches nothing — see WR-04).

### IN-09: The page's init log still announces v1.3.1 — **Resolved in v1.5.3; class closed in v1.5.4**

> **v1.5.4 — the fix was scoped to the instance, not the class.** This finding names
> *the page's* init log, so the v1.5.3 sweep deleted the page's literal and wrote check
> [10] to scan `index.html` alone. The identical hand-maintained literal on the other
> announcing surface — the render-harness banner, printing `(v1.5.2)` against a 1.5.3
> build — was never in scope and survived, with the new gate printing green over it for
> a full version. v1.5.4 drops that copy in favour of `JucePlugin_VersionString` and
> widens [10] to the announcing call on both surfaces. Adjudicating a duplicated-literal
> finding: enumerate every surface that announces the value before writing the gate,
> because a gate written to the one instance the review happened to find is indistinguishable
> from a gate that works.

**Label:** CONFIRMED
**File:** `plugins/O-AnalogEQ/Source/ui/public/index.html:1527`

```js
console.log('OuariconAnalogEQ v1.3.1 UI initialized');
```

`CMakeLists.txt:12` declares `VERSION 1.5.0`, and `PLUGINS.md:34` records
`O-AnalogEQ | 📦 Installed | 1.5.0`. The string is two minor versions stale — it predates
the hover-help switch (1.4.0), the French caption revision (1.4.1) and the entire Chinese
rollout (1.5.0).

**Failure scenario.** A developer debugging a shipped binary opens the WebView inspector,
reads `v1.3.1`, and concludes they are running a stale build — then re-runs the whole
cache-clear-and-reinstall sequence from `CLAUDE.md` chasing a staleness that is only in
this string. The suite has been bitten by the mirror-image of this before
(`critical_plugin_version_keyword_ignored_by_juce`: a version keyword that JUCE ignores,
found because someone trusted a printed number).

**Fix:** do not hand-maintain a second copy of the version. Either delete the line, or
source it from the one place that is authoritative. `CMakeLists.txt` is the version truth
for this repo, and it already flows into `BinaryData`/`JucePlugin_VersionString`; the
cheapest correct form is to drop the literal:
```js
console.log('OuariconAnalogEQ UI initialized');
```
A hand-edited version string in page JS has no gate and will go stale again.

## Surfaces Reviewed and Cleared — No Finding

Recorded explicitly so a later reviewer knows these were examined rather than skipped. An
unstated clearance is indistinguishable from an omission.

### Surface 1 — i18n canon and language persistence (`i18n.js`, `index.html:1620-1700`)

`LANGUAGES = ['en', 'fr', 'zh-Hans']` (`i18n.js:125`) is the single declaration; the
three-way `languageCode` / `languageIndex` codec (`PluginProcessor.h:92-93`) is pure ASCII
and clamps anything that is neither `"fr"` nor `"zh-Hans"` to English, so a hand-edited
session or a hostile `setUiLanguage` argument degrades rather than storing an
unvalidated index. **No finding.**

The persistence round-trip is correct in a way worth recording, because the obvious
reading of it is wrong. `getStateInformation` writes `uiLanguage` as a **string** into
`parameters.state` **before** `getStateAsXml()` snapshots the tree
(`PluginProcessor.cpp:414-427`), and `setStateInformation` reads it **after**
`setStateFromXml` — which calls `replaceState()` and rebuilds the whole tree — guarding
with `isVoid()` and reading with `toString()` (`:429-453`). That is the only correct pair:
`critical_valuetree_xml_roundtrip_loses_type` establishes that
`NamedValueSet::setFromXmlAttributes` rebuilds every property as `var(value)` over the
attribute string, so a bool or int written there would not survive as its own type. The
code says so in its own comment and the comment is accurate.

The apparent leak — a **user preset** JSON saved while the UI is French carrying
`uiLanguage="fr"` into someone else's session — does **not** occur as a behaviour change:
nothing reads `uiLanguage` at preset-load time (only `setStateInformation` does), and the
next `getStateInformation` unconditionally overwrites the property from the
`std::atomic<int>` (`:421-423`), which no preset path can write. The header's stated
contract — "a preset must not be able to change which language somebody reads their plugin
in" (`PluginProcessor.h:76-79`) — **holds**. **No finding.**

Copy paths are `textContent` throughout: `applyLabel` writes `el.textContent`
(`index.html:1578`), the tooltip renderer uses `createElement` + `createTextNode`
(`:1819-1829`), and the one `innerHTML` on the page is `presetDropdown.innerHTML = ''`
(`:1462`) — a **clear**, not a write, with the items that follow built via
`document.createElement` and `item.textContent = name` (`:1478-1480`). Preset names
therefore cannot open a markup path even though they originate on the filesystem. This is
threat **T-r5c-03**, and it is **mitigated**; `check-i18n` assertion 9 passed.

### Surface 2 — tooltip / hover-help renderer (`index.html:1807-1953`, TIP_BINDINGS `i18n.js:865-879`)

Fourteen bindings for fifteen of sixteen parameters, with both gaps documented in the
table rather than left silent (the four dual knobs are one anchor each by construction —
`.knob-outer` and `.knob-inner` are both `pointer-events: none`, so only
`.dual-knob-container` receives a pointer event — and `output_gain` has no control at
all, per IN-01). Delegation is on `document` rather than a setup-time
`querySelectorAll('[data-tip]')`, which is required here because no anchor carries
`data-tip` until `applyI18n()` has run. `pointerout` ignores child-boundary moves within
the same anchor. **No finding.**

Clamping is genuinely four-edged: flip, re-clamp the flipped result, then a
`Math.max(M, …)` floor on both axes (`:1838-1874`). `pattern_fixed_tooltip_shrink_to_fit_edge`
and `pattern_tooltip_clamp_gate_viewport_sensitive` are both satisfied, and the comment
correctly identifies the floor — not the two `if`s above it — as the line that actually
ships the placement, which is the kind of claim that usually drifts and here does not.

Keyboard reach and ARIA are present and correct: `focusin`/`focusout` arms behind an
explicit last-input-device latch (`:1930-1944`), `role="tooltip"` on the surface with
`aria-hidden` flipped by the renderer rather than left `true` (`:1135`), and `Escape`
hiding. The latch's use of a device flag rather than `:focus-visible` is deliberate and
documented — Chromium reports `:focus-visible` false for a programmatic `.focus()`, so a
gate driving focus directly would measure "no tip" and record a false pass. **No finding.**
What the switch layered on top of this renderer is *not* gated — that is WR-08.

### Surface 3 — the `zh-Hans` table

35 entries across 49 emitter rows, `reviewed: 'bt'` throughout with **0 at `'mt'`**, which
`i18n-zh-lint`'s R1 rule confirms and prints on every run. Coverage against the English
key set is complete — `check-i18n` assertion 15 passes in both directions (every key
referenced exists, every `LABELS` key is referenced by an element or a `setLabel` call), so
there is no dead Chinese string and no key rendering as its own name. No row claims a
level above `'bt'`; `reviewed: 'native'` stays open and is printed as the standing
worklist. Geometry is pinned per leaf from that leaf's own measured English box
(`index.html:832-865`), unitless and scoped, and `check-ui-labels` reports 0 moved and 0
clipped on the `zh-Hans` arm. **No finding** — with the caveat already recorded in the
gate table that Z6 is inert on 549 of 552 glossary terms, so this zero is a statement
about the rules that fired, not about Chinese width in general.

### Surface 4 — the settings popover and `oaeq.tipsEnabled`

The `settings-popover` element (`index.html:1096-1122`) carries `role="dialog"`, a keyed
`aria-label` via `data-i18n-aria`, and `hidden` as its closed state;
`setSettingsPopoverOpen` keeps `#gear-btn`'s `aria-expanded` in agreement on every
transition (`:1665-1670`). Dismissal is on `pointerdown` rather than `click` —
deliberate, because this page starts a knob drag on `pointerdown` and a `click`-based
dismiss would leave the panel standing over a gesture already underway — plus `Escape`,
which returns focus to the gear. The panel opens upward because the gear sits 8 px from
the bottom of a 220 px frame inside an `overflow: hidden` container. **No finding.**

The `#tips-toggle` control is a `<button>` inside a plain `<div class="settings-row">`
and deliberately **not** inside a `<label for=…>`: a `<label>` wrapping a labelable
element re-dispatches the click and the switch would toggle twice. It names itself with
`aria-label` instead. **No finding.**

The three `localStorage` questions D-02 asks are each answered correctly in code
(`index.html:1736-1745`), and this is threat **T-r5c-04**, **mitigated**:

- **A value that is neither `"true"` nor `"false"`.** The parse is
  `applyTipsEnabled(stored !== 'false')` — a single negative test against one literal, so
  every other value, including a corrupted one, resolves to **ON**. That is the correct
  fail-direction: the pre-1.4.0 behaviour was unconditional hover-help, so ON is the
  setting that leaves an existing user's plugin unchanged.
- **`localStorage` throws.** `getItem` is wrapped in `try/catch` with `stored = null` on
  throw (`:1739`), which then takes the same `!== 'false'` path to ON. The `setItem` on
  click is separately wrapped and swallows (`:1742-1743`) — in private mode the switch
  still works, it just forgets, which the comment states.
- **`aria-pressed` and the label staying in agreement across a language change.** They do,
  and by construction rather than by a second code path. `applyTipsEnabled` sets
  `aria-pressed` and calls `setLabel(btn, 'ui.on')` or `setLabel(btn, 'ui.off')`
  (`:1729-1733`), and `setLabel` writes `el.dataset.i18n`. A later language change runs
  `applyI18n`, which re-renders **from** `dataset.i18n` for every `[data-i18n]` element
  (`:1606`) — so the button re-localizes into whichever state it is in, and
  `aria-pressed`, which is not language-dependent, is left alone. The two cannot
  disagree. The `if`/`else` form rather than a ternary inside the `setLabel` argument is
  also required: `check-i18n` assertion 13 rejects a conditional there, because an
  inflection decided in JS is a string no translator can see. **No finding** — what is
  missing here is coverage, not correctness (WR-08, WR-09).

### Surface 5 — `setupDualKnob` against WR-01's skew question, and `presetDropdown`

`setupDualKnob` (`index.html:1210-1290`) resolves outer-vs-inner from the cursor's
distance from centre at `pointerdown` (`INNER_THRESHOLD 0.60`), and its double-click reset
writes `DEFAULT_VALUES[…]` through `outerState.setNormalisedValue` (`:1264, :1266`) —
values now derived via `toNorm` from the real C++ Hz defaults, which is IN-02's fix.
Readouts go through `paramRanges[…].format(state.getNormalisedValue())`, which is the
skew-corrected `toHz`, satisfying `pattern_webview_knob_readout_scaled_value` and
`critical_lambda_normalisable_range_invisible_to_webview_slider_frontend`: the C++ range
is a plain `NormalisableRange` with a numeric skew of 0.3, **not** a lambda range, so the
JS can and does reconstruct it exactly. **No finding.**

`presetDropdown` (`:1461-1490`) is covered under Surface 1 for the `innerHTML` question.
Its empty state is written with `setLabel(empty, 'label.noPresets')` rather than a literal
— so the node becomes a `[data-i18n]` element the language sweep owns, and there is no
second code path to go stale. **No finding.**

### Surface 6 — native bridge completeness and the resource provider

Every `getNativeFunction` name requested by JS has a matching `withNativeFunction`
registration, in both directions and with nothing orphaned:

| JS `getNativeFunction` | C++ `withNativeFunction` |
|---|---|
| `savePreset`, `savePresetWithDialog`, `loadPreset`, `loadPresetFromFile`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`, `deletePreset`, `isFactoryPreset` (`preset-manager.js:108-117`) | `PluginEditor.cpp:100-202` — all ten present |
| `getUiLanguage`, `setUiLanguage` (`index.html:1630-1631`) | `PluginEditor.cpp:210-225` — both present |

`pattern_webview_native_fn_bridge_gap` (a JS call with no C++ registration hangs a promise
forever) does not apply. Every native function invokes `complete` on **every** path
including the argument-count bails (`:102-104`, `:141-143`, `:193-195`, `:199-201`); the
only two paths that deliberately do not are WR-04's dead-editor bails, which is correct
per pattern memory. The page reaches the bridge through the `Juce` namespace import
(`index.html:1147-1152`) rather than raw `window.__JUCE__`, satisfying
`critical_juce_webview_namespace_vs_postmessage`. This is threat **T-r5c-06**,
**mitigated**. **No finding.**

`getResource` (`PluginEditor.cpp:308-383`) is threat **T-r5c-01**, **mitigated**: seven
exact `url == "…"` string equalities with no wildcard, no prefix match, no substring test
and no filesystem access of any kind — every branch returns a `BinaryData` blob with an
explicit MIME type, and anything unmatched logs and returns `std::nullopt`. There is no
path to escape because there is no path construction. `critical_webview_resource_provider_and_schemes`
is satisfied. **No finding.**

Threat **T-r5c-02** (JS-origin preset name → filesystem) is **mitigated in the shared
module**: `sanitizePresetName` replaces `/`, `\` and `:` with `_`
(`OuariconPresetManager.h:247-250`) and is applied consistently at every site that turns a
name into a filename (`:295, :406, :449, :509, :510, :669`), so the same name round-trips
and `critical_preset_name_slash_path_separator` — where `getChildFile` silently drops a
file whose name contains a separator — cannot fire. `..` is not a separator after
sanitization and `getChildFile("..")` cannot escape a parent in JUCE. **No finding.**
The *directory* defect in the save path is WR-07, which is a different bug.

### Surface 7 — the stale-`<CustomState>` bug class, and factory-preset skew

`critical_preset_manager_stale_customstate_child` is **NOT-APPLICABLE**, already recorded
above with its grep evidence.

`pattern_factory_preset_normalized_ignores_skew` and
`critical_apvts_denormalised_vs_preset_normalised` were checked against all twelve factory
presets. The presets store **normalised** values and the constructor's comment states the
mapping explicitly (`PluginProcessor.cpp:109-113`), including
`Freq: pow((hz - min) / (max - min), 0.3) due to NormalisableRange skew`. Spot-checked
against the declared ranges: `Default`'s `lf_freq = 0.577` → `30 + 470 × 0.577^(1/0.3)` =
**100.0 Hz**, matching the APVTS default at `:43`; `lmf_freq = 0.627` → **500 Hz** (`:53`);
`hmf_freq = 0.617` → **2000 Hz** (`:66`); `hf_freq = 0.710` → **8000 Hz** (`:79`). All four
agree to the displayed precision. The skew is respected. **No finding.**

### Surface 8 — RT-safety of `processBlock`, beyond CR-01

Re-audited end to end at `PluginProcessor.cpp:277-397`. `juce::ScopedNoDenormals` is the
first statement (`:279`), so denormals are handled for the whole block. There is no
`new`, no `malloc`, no container growth, no lock, no `juce::String` construction and no
file or logging call on the path — `ArrayCoefficients` returns a `std::array` by value,
`getRawParameterValue` resolves through a `StringRef`-keyed map that allocates nothing
(IN-07), and `getSubBlock` is a view. `qValues[lmfQ]` / `qValues[hmfQ]` index a
3-element `constexpr` array (`PluginProcessor.h:128`) with a value that
`AudioParameterChoice` constrains to 0..2, so the index is in range by construction. No
unbounded loop: the chunk loop is `numSamples / kSmoothingBlock` iterations.

Latency reporting is correct by omission — the four IIR bands, the waveshaper and the gain
are all zero-latency, and `setLatencySamples` is correctly absent rather than set to a
wrong constant (`getLatencySamples()` is non-virtual in JUCE 8, so the only correct form
would be a `setLatencySamples(N)` in `prepareToPlay`, and `N` here is 0).

NaN origination: the one path that could produce a NaN coefficient is
`currentSampleRate == 0`, which would make `nyquist` 0 and `clampFreq` return 0 for every
band. `currentSampleRate` is initialised to `44100.0` (`PluginProcessor.h:110`) and only
ever assigned from `prepareToPlay`'s argument, so this requires a host to pass 0 — no
JUCE-hosted format does. Were it to happen, the NaN would be sticky in the biquad state
(`pattern_biquad_nan_guard_sticky_silence`) and `SmoothedValue::reset()` would not clear
it (`critical_smoothedvalue_reset_does_not_clear_a_nan`). **No finding** at the current
threshold; a `jassert(sampleRate > 0)` in `prepareToPlay` would be free insurance if one
is ever added for another reason.

`pattern_conditional_coeff_update_leaks_enabled_flag` — the trap where a cached
coefficient guard skips a rebuild and leaves an enable flag stale — does **not** apply:
the `*Moving` gate (`:330-334`) keys only on smoother motion, the discrete-Q edge and the
`coeffsInitialised` force flag, and the band-enable booleans are read fresh every block
and used only at the `process` call site (`:344-347`), never folded into the coefficient
cache. The `lastLmfQ`/`lastHmfQ` sentinels are reset to `-1` in `prepareToPlay` (`:273`)
so a sample-rate change rebuilds the bells. `coeffsInitialised` is set only when
`numSamples > 0` (`:386-387`), so a zero-length block cannot consume the forced first
build. All three of those are the right edge cases and all three are handled.

## Suggested Resolution Order

For a later `/improve-review` run. Grouped by where the fix lands, because three of these
are not plugin-local and one must not be fixed in this repository's plugin tree at all.

| Order | Findings | Lands in | Note |
|-------|----------|----------|------|
| 1 | **CR-02** | `Source/PluginProcessor.h` + `.cpp` | Four lines, closes a crash path. Do it first and alone. |
| 2 | **WR-05**, **WR-06** | `Source/PluginProcessor.cpp` | Both are audio-thread; one rebuild, one render-harness pass covers both. |
| 3 | **WR-07**, **IN-08** | `Source/PluginEditor.cpp` | Same two functions; fix together. |
| 4 | **WR-08**, **WR-09** | `tests/` + `tests/i18n-states.json` | Coverage, not behaviour. Land **before** anyone edits the switch. |
| 5 | **WR-10** | `modules/persistence/preset-manager/` **and** `scripts/check-i18n.js` | **Shared module + repo-wide gate.** Not a plugin-local fix. Expect it to surface findings in other plugins. |
| 6 | **IN-06**, **IN-07**, **IN-09** | `Source/` | `/improve-review-info` tier. Comment and cache work; risk is the compiler's. |

**Do not fix anything in this pass.** This document is read-only output; every fix above
is a later `/improve-review` run, and each one needs its own verification.

---

## Resolved

Every finding in this document is closed. Recorded here so a later reviewer can tell a
closed finding from a dropped one, and so the two cases where a prescribed *Fix* block
was unsafe as written are not lost.

| Findings | Release | Commit | Note |
|----------|---------|--------|------|
| CR-02, WR-05..WR-10 | v1.5.1 (2026-09-25) | `75b97ff5` | The defect tiers, per §Suggested Resolution Order. |
| — (two residual defects **introduced by** the v1.5.1 fixes) | v1.5.2 (2026-09-26) | `6b9e426b` | WR-07's prescribed swap dropped the `isFactoryPreset()` guard; WR-10 part 1 did not deliver its stated scope. Neither is a finding here. |
| IN-06, IN-07, IN-08, IN-09 | v1.5.3 (2026-09-26) | this commit | The Info tier, via `/improve-review-info`. |

**Two of this document's own *Fix* blocks were unsafe as literally written**, and both
were caught only by executing them and re-verifying. Recorded because the pattern is the
finding:

- **§WR-07** prescribed swapping `savePreset(name)` for `savePresetToFile(file)`. Correct
  about the discarded directory, silent about the `isFactoryPreset()` early-return that
  only the first API carries. Applying it verbatim removed factory-preset overwrite
  protection. Closed in v1.5.2 with a **location-aware** guard — a name-only reject would
  have re-broken the arbitrary-path export WR-07 exists to enable.
- **§IN-08** prescribed guarding on `fileChooser != nullptr` and clearing `fileChooser`
  "at the end of each completion body". That destroys the `FileChooser`, and the
  `std::function` then executing, from inside its own `launchAsync` callback; and the
  guard cannot be adopted *without* the clear, because then the first dialog latches it
  shut for the editor's lifetime. Closed in v1.5.3 with a `bool presetDialogInFlight`
  instead — releasing a bool touches no owner.

A prescription in a review is a hypothesis about the fix, not the fix.

### Gates added while resolving

Neither finding's tier implied a gate; both were added because the change is invisible to
the existing ones. Each was **seen to fail** before acceptance.

| Gate | Guards | Negative control |
|------|--------|------------------|
| `tests/render-harness/` G4 (v1.5.2) | `oaeq::presetSaveRefusal` | Guard neutered → the four refusal arms fail; name-only guard → the export arm fails **alone**. |
| `tests/check-param-cache.js` (v1.5.3) | IN-07's member↔ID mapping, cache completeness against the layout, no lookup outside the constructor | Swapped `pHmfFreq`/`pHmfGain` → [3] fails; lookup reintroduced in `processBlock` → [5] fails; assignment dropped → [2] and [4] fail. |
| `ui_tip_render_check.js` [10] (v1.5.3, widened v1.5.4) | no `vX.Y.Z` literal in the announcing call on either surface — `console.*` in `index.html`, `printf` in `tests/render-harness/main.cpp` | Restoring the v1.3.1 string turns [10] red; so does restoring the harness's `(v1.5.2)` banner; both at once names both files in one verdict. A `vX.Y.Z` in a non-call comment line does **not** trip it, which is what keeps the history comments legal. |

IN-07's mapping is gated by **name rather than by audio** on purpose: G1–G4 all still pass
with two same-band parameter reads exchanged, so the render harness cannot see the one
defect that change can introduce.

---

_Reviewed: 2026-09-25_
_Reviewer: Claude (GSD quick task 260925-r5c)_
_Depth: thorough — 10 files, v1.5.0, 13 versions since the prior standard review_





