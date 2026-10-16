# O-simpleReverb - Requirements

---
version: 3.0.0
plugin: O-simpleReverb
created: 2026-10-15
lastUpdated: 2026-10-15
milestone: family-retrofit
---

## Overview

**Target Milestone:** v3.0.0 (O-simple family retrofit)
**Total Requirements:** 12
**Coverage:** must: 10 | should: 2 | nice: 0
**Source:** `.planning/improvements/family-retrofit/CONTEXT.md` success criteria + the mockup sign-off (RESEARCH.md M1–M5, Q2–Q5)

## Requirements

### Identity and compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Case-only rename to O-simpleReverb everywhere live (targets, PRODUCT_NAME, getName, folders, WebView data folder); `OuSr` / manufacturer unchanged | must | complete | execute T1, T9 |
| COMPAT-02 | v2.0.0 (and v1.14.0) sessions restore with identical parameter values and uiLanguage | must | complete | execute T8 (verify_3_state S0–S5, installed VST3) |
| COMPAT-03 | No preset-folder migration needed on case-insensitive volumes; guarded case-only rename on case-sensitive ones (Q4) | should | complete | execute T1 |
| COMPAT-04 | auval + pluginval strictness 10 pass for VST3 and AU; old-case bundles swept | must | complete | execute T9 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | DSP frozen: bit-exact null against v2.0.0 (48 presets × 3 stimuli × 44.1 / 96 k + automation) | must | complete | execute T2–T5, T9 (installed VST3) |
| DSP-02 | Meter payload `{inDb, wetDb, outPeakL, outPeakR, spanMs, live}` once per block, RT-safe, hash-gated at 30 Hz | must | complete | execute T4 (render-check payload gates, mutant frameNoReset) |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Family layout at 1180 × 780, resizable 0.6–1.5×, editorScale persisted | must | complete | execute T6–T8 (probes at 0.6 / 1.0 / 1.5×) |
| UI-02 | Four live wells from C++-sourced data; the decay well tracks the real tail within ±20 % | must | complete | execute T6 (render-check decay-well gate), T8 (frame-probe F1–F4) |
| UI-03 | 6 Spaces + Init + 4 Ideas lessons from `PresetRecipes.h`; 48 utility presets browsable by type; ◀/▶ in TYPE order then user (Q5) | must | complete | execute T5, T7, T8 (lesson-row-budget) |
| UI-04 | Bridge census: every `getNativeFunction` name has a `withNativeFunction` (20 = 20) | must | complete | execute T6 |
| UI-05 | en / fr / zh-Hans; family gates green (check-i18n, check-ui-labels, fr/zh lint, boot-all-uis, 4 probes); no "absorption" copy (Q3) | must | complete | execute T7, T8 |
| UI-06 | Preset bar: Save / Delete inline, Load file… in the gear; v2 save dialog refused (Q2) | should | complete | execute T6, T7 |

## Acceptance Criteria Details

The task-level verification criteria live in `.planning/improvements/family-retrofit/PLAN.md` (Tasks 0–10); the evidence lives in `SUMMARY.md` beside it. The verify phase re-runs them goal-backward and writes `VERIFICATION.md`.
