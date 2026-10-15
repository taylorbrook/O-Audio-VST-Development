# Context: O-simple family retrofit

**Plugin:** O-SimpleReverb (becomes O-simpleReverb)
**Milestone:** family-retrofit
**Created:** 2026-10-15
**Phase:** Discuss
**Brief:** `plugins/O-SimpleReverb/improvements/family-retrofit.md`. It holds the family conventions, the rename checklist and the must-not-change list. This file adds the discuss answers and does not repeat the brief.

---

## Original Request

> Rename to O-simpleReverb and retrofit the full O-simple family UI (Ambisonics-style), keeping both preset sets: lessons in a lesson row, the 48 utility presets in the browser. Milestone workflow, starting with a UI mockup for sign-off.

---

## Requirements Gathered

### Core Requirements

1. **Rename to O-simpleReverb**
   - Description: case-only rename of the folder, target, PRODUCT_NAME, getName, preset folder, WebView2 data folder, registry, PLUGINS.md and gate names. Follow the brief's checklist. `PLUGIN_CODE OuSr` and the manufacturer code stay.
   - Acceptance: a v2.0.0 session reopens on the renamed build with every parameter value intact (same VST3 id, AU `aufx OuSr`). `auval` passes. No `O-SimpleReverb` bundle is left installed.
   - Open for research: migrating the user preset folder (`O-SimpleReverb` → `O-simpleReverb`). APFS is case-insensitive, but Windows and the preset manager's own path compare may not be. Use the v1.5.0 rename precedent.

2. **Family layout at 1180×780**
   - Description: the same base size as Ambisonics, with a fixed aspect ratio, resizable 0.6–1.5×, `editorScale` persisted on the processor and `applyUiScale()`. Top to bottom: header (wordmark, preset bar, gear) → viz-band → sigpath stations → `main.controls` → two lesson rows → footer → `#tooltip`.
   - Acceptance: the boot-all-uis and resize probes pass at 0.6×, 1.0× and 1.5×, with no overflow at en, fr or zh-Hans.

3. **Four visual wells** (all chosen)
   - **Live decay well:** a scrolling dB-over-time plot. The input envelope is drawn in amber, the wet tail in sage, and the current type's RT60 slope (from the calibrated DECAY seconds) in brass. Data comes from C++ through `MeterFramePayload` + `TripleBuffer`, emitted at 30 Hz with `emitEventIfBrowserIsVisible`.
   - **Early-reflection taps, L/R:** a stem plot of time × gain, L above the axis and R below. It redraws on TYPE and SIZE. The tap data must come from `EarlyReflections.h` (a single source, pushed or fetched from C++) and is never duplicated in JS.
   - **Tone curve:** the wet-path frequency response, showing the Low Cut slope (when IN) plus Character's tilt or absorption. Research must decide between computing it in JS from the same coefficients and pushing it from C++, without drift from the DSP.
   - **Wet/dry output meter:** a small L/R peak meter in the output column, like Ambisonics' `out-meter`. It replaces the needle VU.
   - Acceptance: every well responds visibly within one frame of a gesture on the parameters it shows. Plot inks pass the contrast gates on the walnut wells.

4. **Controls in signal order**
   - Description: the groups are Input → Early / Pre-delay (if it is exposed by station only, there is no new parameter) → Engine (TYPE + SIZE + DECAY) → Tone (CHARACTER + Low Cut) → Mix (WET, DRY) → Output. Each control has `.group-route` hints and a knob, value line and meaning line.
   - **TYPE** is a segmented row of six buttons (Booth · Room · Hall · Spring · Plate · Ambient), with the engine named beneath them (FDN / Plate / Spring).
   - **Low Cut** is a LPFREQ seed knob with a small IN/OUT pill (LPON) above it. The knob dims while the pill is OUT.
   - The knobs are Family A seed knobs, and only `.knob-stem` rotates.

5. **Ten lessons in two rows, plus Init**
   - Row 1 (types): Booth, Room, Hall, Spring, Plate, Ambient.
   - Row 2 (concepts): Init, Size ≠ Decay, Character (absorption), Low Cut (mud), Insert vs Send.
   - `Source/PresetRecipes.h` is the single source (engineering units → `convertTo0to1`). Each lesson resets to defaults first, then applies its recipe. The buttons are `.tour-btn[data-preset]`. Each row's width budget is pinned at the widest language (French).
   - The 48 utility presets stay in the preset browser, unchanged.

6. **Tooltips rewritten to explain why**, in en, fr and zh-Hans. New fr and zh bodies are flagged unreviewed.

7. **Family plumbing:** the native functions listed in the brief (`getParameterDefaults`, `uiReady`, `applyFactoryPreset`, the full preset set including `deletePreset` and `getPresetCatalog`, and the `presetState` push), the CMake flags, the test tree (`ui-probes`, `ui-stub`, `ui-fixtures`, `pedalboard/verify_*.py`) and the refreshed `.planning/` docs (BRIEF, REQUIREMENTS, ROADMAP, STATUS, whose current frontmatter is stale at Stage 1).

### Scope Boundaries

**In scope:** the rename, the full UI rewrite, the payload + TripleBuffer for the wells, PresetRecipes.h, the lesson rows, the tooltip rewrite, the family test gates, the docs refresh and v3.0.0.

**Out of scope:** any parameter ID, range, skew, default or state-format change; any DSP change (the v2.0.0 engines are frozen); new parameters (no pre-delay knob); changes to the 48 utility presets; a git tag (that is `/publish` only).

---

## User Preferences

### Interaction Style
- [x] Real-time parameter response (live decay well, wells redraw on gesture)
- [x] Preset-based switching (lessons + utility browser)
- [x] Automation-friendly (IDs unchanged)

### Quality Priorities
1. Gesture → visible consequence (the family north star)
2. No audible or state change versus v2.0.0 (null against the old build where the RNG allows it)
3. Visual polish at every scale and language

### Breaking Changes
- [ ] OK to break existing presets
- [x] Must maintain preset compatibility
- [x] Must maintain automation compatibility

---

## Plugin Context

### Current State
- **Version:** 2.0.0, from the `reverb-engine-rewrite` milestone of 2026-10-01: FDN / Dattorro plate / dispersive spring, 126 render-check gates + 30 mutants.
- **UI today:** a 500×350 fixed window with paper.jpg + flora.png, vine-arc knobs, a `<select id="TYPE">`, a needle VU at 30 Hz, and 2211 lines in one `index.html`.
- **Native functions today:** loadPreset, getPresetList, getCurrentPreset, next and previous, savePresetWithDialog, loadPresetFromFile, getUiLanguage, setUiLanguage and getPluginVersion. The family set is missing.

### Known Limitations
- `Source/ModulationFx.h` sits beside the dsp/ split. Research must confirm what it feeds before a sigpath station is placed for it.
- Two of the release variants are installed (`O-SimpleReverb.vst3` and `-dev`), so the install sweep must remove both old names.

---

## Questions Answered

**Q1: Which visual wells?**
> All four: the live decay well, the L/R early-reflection taps, the tone curve and the wet/dry output meter.

**Q2: TYPE control?**
> A segmented row of six, with the engine labelled beneath.

**Q3: Lesson set?**
> Types + concepts, 10: six type lessons plus Size ≠ Decay, Character, Low Cut and Insert vs Send.

**Q4: Mockup sign-off?**
> It is a research output: a static mockup HTML in this folder, signed off before plan.

**Q5: Lesson layout?**
> Two rows. Types first, then Init + concepts.

**Q6: Low Cut?**
> A knob + an IN/OUT pill. The knob dims while OUT.

**Q7: Window?**
> 1180×780, like Ambisonics.

---

## Success Criteria

1. [ ] The mockup has been signed off before plan.
2. [ ] O-simpleReverb loads v2.0.0 sessions and presets with identical values. auval and pluginval 10 pass for VST3 + AU.
3. [ ] All four wells respond to their parameters live, and the decay well tracks the real wet tail.
4. [ ] Ten lessons + Init load from PresetRecipes.h, and 48 utility presets remain browsable.
5. [ ] The family gates are green: check-i18n, check-ui-labels, i18n-fr-lint, i18n-zh-lint, boot-all-uis, the ui-probes, render-check 126 + 30 mutants.
6. [ ] The render nulls against v2.0.0 (the DSP is untouched).
7. [ ] The build has 0 warnings, and the old bundle names are swept from the system folders.

---

## Next Phase

Research: map the Ambisonics files one-to-one to Reverb, settle the open questions (preset-folder migration, the ER tap source, the tone-curve source, ModulationFx, the payload fields) and **produce `mockup/index.html` for sign-off**.

---

*Generated by improve-milestone discuss phase*
