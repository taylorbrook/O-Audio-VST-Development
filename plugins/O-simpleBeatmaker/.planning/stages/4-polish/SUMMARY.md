# Stage 4 (Polish) — SUMMARY

**What was built:** the v1.0.0 ship pass — six concept-isolating factory presets
wired to the existing tour buttons, a playability decision, a restored DSP gate,
the full validation sweep, and CHANGELOG v1.0.0. No new DSP or UI mechanism; the
42-param APVTS contract stayed frozen.

## Tasks completed

### T1 — `Source/BeatPresets.h` (new)
`constexpr std::array<BeatPreset, 6>` table (RESEARCH §2, transcribed verbatim).
Each preset = name + patternLengthChoice + swing/humanize/quantize (0–1) + tempo +
a 6×16 velocity grid (row = Voice enum order). C++ source, **not** a binary-data
blob (no 2nd BinaryData namespace). Six presets: Straight · Backbeat + Accents ·
Ghost Notes · Triplet Swing (swing01 0.80) · Humanized (hum 0.70 / quant 0.25) ·
Quantize Demo (swing 0.60 / hum 0.85 / quant 0.50).

### T2 — `applyConceptPreset(int)` on the processor
`PluginProcessor.h` (decl) + `.cpp` (impl) + `#include "BeatPresets.h"`. Message
thread: sets the 5 timing-feel params via `prm->setValueNotifyingHost(prm->convertTo0to1(real))`
(so host automation + the two-way-bound JS knobs/combo update for free), then
`clearGrid()` + `setStep()` to stamp the grid. Bounds-checked; choice param
(patternLength) normalised via `convertTo0to1((float) choiceIndex)`.

### T3 — `applyPreset` native function (`PluginEditor.cpp`)
Added to the existing `withNativeFunction` chain: `applyPreset(index)` →
`processorRef.applyConceptPreset((int) args[0])`. Same `juce::var` int marshalling
as `setStep`.

### T4 — JS wiring (`ui/public/js/app.js`)
`initPresetTour()` now drives the six `.tour-btn` (DOM order == preset index):
`await applyPresetFn(index)` then `await refreshGridFromBackend()`. `applyPresetFn`
declared at module scope (line 296) and looked up in `boot()` with the other native
fns (null-guarded). Knobs + length combo auto-update via their attachments, so only
the grid needs an explicit repaint.

### T5 — Native-fn parity gate (PASS)
grep-diff: both sides have exactly `{applyPreset, clearGrid, getGrid, getSampleRate,
setStep}`. No bridge gap. Plus `node --check app.js` = SYNTAX OK (guards the silent
JS-death pitfall).

### T6 — Build + install (clean)
`ninja O-simpleBeatmaker_{VST3,AU,Standalone} O-simpleBeatmaker-render-test` — all
linked clean (only benign JUCE switch-enum warnings). CLAUDE.md cache-clear +
dual-variant sweep + `-dev` bundle install done; `auval -a` lists `aumu OSiB OuDv`.

### T7 — Playability (FUNC-08): no default change
Per RESEARCH §3, changed **no** default param values: the voice defaults were tuned
+ QUAL-01-verified in Stage 2, master 0 dB matches the sibling convention, and the
preset velocities are disciplined (no preset stacks all six voices at 127 on one
step — e.g. Preset 1's downbeat is Kick 127 + ClosedHat 100, snare lands on the
backbeat). Harness `high-rate-bounded` peak 1.10 is a pathological stress case;
normal single hits are ~0.5. The hands-on audible clipping check is flagged as a
DAW residual (I cannot listen in this environment).

### T8 — Validation sweep (all green)
- **Render-harness:** 11/11 probes PASS (the 6 ROADMAP probes — grid-accuracy
  maxNominalErr=0, swing-offset 3675, humanize bounded, **quantize-preserves-swing
  q=1 swing=3675 survives humanize→0 (DSP-04)**, block-boundary fires-once,
  viz-truth fifoAgrees=Y (QUAL-02)).
- **auval `aumu OSiB OuDv`:** AU VALIDATION SUCCEEDED (render / 1-ch / bad-max-frames
  / param set + ramp / MIDI).
- **pluginval `--strictness-level 10`:** SUCCESS for **both** VST3 and AU.

### T9 — UI render confirmation (screenshot)
Standalone screenshot (`scratchpad/beatmaker-stage4.png`): full UI renders — grid,
timing lane, MIDI readout, all timing-feel knobs at their values, all six voice
strips, master. **Not blank** → the JS boot path (incl. the new `applyPreset`
lookup) is healthy; no silent UI death. Preset-populates-grid + audible-concept
confirmation is the hands-on DAW item (QUAL-02 residual).

### T10 — `CHANGELOG.md` v1.0.0 (new)
First release entry covering the whole staged build (Foundation → DSP → GUI →
Polish): six synth voices, host-synced sample-accurate sequencer, swing/humanize/
quantize feel engine + DSP-04 invariant, WebView teaching UI, six concept presets,
cross-platform WebView2 config, validation results. Keep-a-Changelog style.

## Deviation (gap closed during execute) — render-harness build repair

**Found:** the render-harness target (`tests/render-harness/CMakeLists.txt`) was
**un-buildable since Stage 3**. It compiles `PluginEditor.cpp` with `JUCE_WEB_BROWSER=0`
and a comment claiming "the editor is a pure GenericAudioProcessorEditor shell" —
but Stage 3 replaced that shell with the WebView editor (`juce::WebBrowserComponent`
+ relays/attachments). Stage 3 never re-ran the harness (it touched no DSP), so the
breakage lay dormant until this stage's regression sweep.

**Fix (shipping path provably unchanged):**
- `PluginProcessor.cpp`: guarded `#include "PluginEditor.h"` and `createEditor()`
  with `#if JUCE_WEB_BROWSER`. The shipping plugin always builds `JUCE_WEB_BROWSER=1`,
  so its `createEditor()` is byte-for-byte the same (`new ...Editor`). Only the
  headless harness (`=0`) takes the `#else` fallback (`GenericAudioProcessorEditor`),
  which exists solely to give the linker a valid symbol — the harness never opens
  an editor.
- `tests/render-harness/CMakeLists.txt`: dropped `PluginEditor.cpp` from the target
  sources (WebView; needs `JUCE_WEB_BROWSER=1`), updated the stale comment.

This keeps the harness a true headless DSP gate (no WebKit / binary-data in the test
build) and restores the Stage-2 correctness gate for the v1.0.0 ship.

## Files

**Created:** `Source/BeatPresets.h`, `CHANGELOG.md`
**Modified:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`,
`Source/PluginEditor.cpp`, `Source/ui/public/index.html`,
`Source/ui/public/js/app.js`, `tests/render-harness/CMakeLists.txt`

## Requirements status

| ID | Status |
|----|--------|
| FUNC-05 (concept presets) | ✅ six presets implemented + wired |
| FUNC-08 (playability) | ✅ defaults unchanged (justified); preset velocities disciplined; audible check = DAW residual |
| COMPAT-01 (pluginval VST3+AU) | ✅ strictness-10 SUCCESS both formats |
| QUAL-02 (visible matches audible) | ✅ truth-by-construction + Probe 6; hands-on DAW audit residual |
| (validation sweep) | ✅ build + auval + pluginval ×2 + harness 6/6 |

## Residual (hands-on DAW, not goal failures)
- Audible playability check (clipping at full-kit downbeat) + the QUAL-02 audible-
  vs-visible audit + a screenshot with a preset loaded — all belong to the
  in-DAW pass at `/install-plugin` / DAW testing (I cannot listen here).
