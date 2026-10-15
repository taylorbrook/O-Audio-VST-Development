# O-SimpleReverb — O-simple family retrofit (brief)

**Date:** 2026-10-15
**Baseline:** v2.0.0
**Target:** v3.0.0 (rename + new UI; parameter IDs, ranges, `OuSr` code and state format unchanged)
**Source:** `/improve` Phase 0.5 investigation (Tier 3 → milestone). Gap analysis against the O-simple*
family; O-simpleAmbisonics (the only other effect) and O-simpleWavetable are the template.

## Decisions (user, 2026-10-15)

1. **Rename** O-SimpleReverb → **O-simpleReverb**.
2. **Full family retrofit**: Ambisonics-style UI.
3. **Keep both preset sets**: lessons in a lesson row, plus the 48 utility presets in the preset browser.
4. **Milestone workflow** (`/improve-milestone`). Start with a UI mockup for sign-off.

## Family conventions to adopt (evidence: O-simpleAmbisonics)

- **North star:** "the tight loop between gesture and visible consequence" (`O-simpleAmbisonics/.planning/BRIEF.md:14`).
  The aim is an "oh, THAT's how reverb works" moment within five minutes, with no manual.
- **Wordmark:** `O&#8202;&#8211;&#8202;simple<span class="title-accent">Reverb</span>`, i18n-exempt (`O-simpleAmbisonics/Source/ui/public/index.html:832`).
- **Footer:** "Ouaricon Audio · O-simpleReverb" | tour caption | technical note (`:1126-1130`).
- **Layout,** top to bottom (single `index.html` + `js/i18n.js` + `modules/preset-manager.js` + `img/insects.png`):
  header (title, preset bar, gear) → `viz-band` (canvas wells) → `sigpath` stations →
  `main.controls` (group columns in signal order, `.group-route` hints, knob + value + meaning line)
  → `preset-tour` lesson row → footer → `#tooltip` outside `#plugin`.
- **Look:** field-guide tokens (`--bg-paper`, `--brown-text`, `--text-muted #715D45`,
  `--green-text #4E6838`), EB Garamond, Family A seed knobs (only `.knob-stem` rotates), dark walnut
  plot wells with sage/amber/brass inks, insects.png overlay at opacity 0.14. Replaces paper.jpg,
  flora.png and the vine-arc knobs.
- **Window:** fixed aspect ratio, resizable 0.6x–1.5x. Uses `applyUiScale()`, `editorScale` persisted
  on the processor, `setResizeLimits` and `setFixedAspectRatio` (`O-simpleAmbisonics/Source/PluginEditor.cpp:426-449,563`).
- **Native functions:** `getParameterDefaults`, `getUiLanguage`, `setUiLanguage`, `uiReady`,
  `applyFactoryPreset`, the full preset set (incl. `deletePreset`, `getPresetCatalog`) and the
  `presetState` push (`O-simpleAmbisonics/Source/PluginEditor.cpp:219-373`).
- **Visual data:** JUCE-free `MeterFramePayload.h` (quantised ints, hash-gated emits) +
  `TripleBuffer<MeterFrame>`, sent with `emitEventIfBrowserIsVisible` at 30 Hz.
- **Lessons:** `Source/PresetRecipes.h` is the single source (Init + N lessons, engineering units,
  `convertTo0to1`). Each lesson resets to defaults, then applies its recipe. `.tour-btn[data-preset]`.
- **CMake:** `IS_SYNTH FALSE`, `EDITOR_WANTS_KEYBOARD_FOCUS FALSE`, `VST3_CATEGORIES "Fx" "Reverb"`,
  quoted VERSION.
- **Tests:** `tests/ui-probes/*.mjs`, `tests/ui-stub/generic-overrides.json`, `tests/ui-fixtures/`,
  `tests/pedalboard/verify_*.py`. Keep `tests/render-check` (126 gates + 30 mutants).
  Suite gates: check-i18n, check-ui-labels, i18n-fr-lint, i18n-zh-lint, boot-all-uis.
- **Docs:** `.planning/` BRIEF / REQUIREMENTS / ROADMAP / STATUS (YAML frontmatter, currently stale
  at "Stage 1 2026-01-13").

## Reverb-specific mapping (starting proposal for discuss)

- **Signal path:** Input → Pre-delay / Early reflections → Engine (FDN · Plate · Spring) →
  Tone (Character · Low Cut) → Wet / Dry → Output. Matches the `Source/dsp/` split.
- **Visual wells (candidates):**
  - energy-decay / RT60 curve per type (DECAY in seconds is already calibrated);
  - early-reflection tap plot, L vs R;
  - wet output level / VU (replaces the needle).
- **Lesson candidates:** one per type (Booth, Room, Hall, Spring, Plate, Ambient), Size vs Decay are
  different things, Pre-delay/Early reflections, Character (absorption), Low Cut (mud), Wet/Dry
  insert vs send.
- **Tooltips:** rewrite to explain why in en/fr/zh-Hans. New fr/zh bodies flagged unreviewed.

## Rename checklist (keep `PLUGIN_CODE OuSr` + manufacturer → VST3 id / AU aufx·OuSr survive)

- Folder: two-step `git mv` (case-only rename on a case-insensitive APFS).
- `CMakeLists.txt`: target, `PRODUCT_NAME "O-simpleReverb${OUARICON_DEV_SUFFIX}"`, binary-data names.
- `PluginProcessor.h:161` `getName`; `PluginProcessor.cpp:245` preset-manager folder;
  `PluginEditor.cpp:57` WebView2 data folder.
- `modules/registry.yaml:217`, `PLUGINS.md:36`, gate `--plugin` names, root CMake `add_subdirectory`.
- Install: sweep `O-SimpleReverb{,-dev}.{vst3,component}`, clear the AU cache.
- Precedent: v1.5.0 rename (`CHANGELOG.md:841-861`).

## Must not change

Parameter IDs (UPPERCASE `TYPE`, `CHARACTER`, …), ranges, skews, defaults, state format, DSP
(v2.0.0 engines), `OuSr`.
