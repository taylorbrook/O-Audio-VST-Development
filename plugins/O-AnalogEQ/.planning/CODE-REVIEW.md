---
phase: O-AnalogEQ-code-review
reviewed: 2026-09-25T00:00:00Z
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
  critical: 0
  warning: 0
  info: 0
  total: 0
status: issues_found
---

# O-AnalogEQ: Code Review Report

**Reviewed:** 2026-09-25
**Depth:** thorough
**Files Reviewed:** 10
**Status:** issues_found

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

### Artifact-path mismatch — read this before running `/improve-review`

`.claude/commands/improve-review.md` declares a **blocking** precondition on
`plugins/[PluginName]/CODE_REVIEW.md` — underscore, plugin root. Twenty-five plugins use
that path. O-AnalogEQ and six others (`O-DigiDelay`, `O-Freeze`, `O-Gain`,
`O-MultiBandCompressor`, `O-Polystutter`, `O-Prism`) use
`.planning/CODE-REVIEW.md` — hyphen, `.planning/` subdirectory. This file is pinned to
the hyphen path by the task that wrote it and has deliberately **not** been moved.

**Consequence:** `/improve-review O-AnalogEQ` will reject on its `<review_present>`
precondition ("No CODE_REVIEW.md found for O-AnalogEQ") even though this review exists.
`/improve-review-info O-AnalogEQ` rejects identically. Point either command at
`plugins/O-AnalogEQ/.planning/CODE-REVIEW.md` explicitly, or resolve the path split
repo-wide first. The frontmatter key set here matches what those commands parse
(`findings:` block for the severity menu, `CR-*`/`WR-*` default scope, `IN-*` opt-in),
so nothing but the path needs adapting.

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

<!-- gsd:write-continue -->


