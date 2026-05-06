---
milestone: double-preset-library
plugin: O-Prism
base_version: 1.17.4
created: 2026-05-06
---

# Double the O-Prism Factory Preset Library

## Goal

Grow the factory preset library from **96 → 192 presets** (96 new, hand-authored, curated).

## Current State (v1.17.4)

`plugins/O-Prism/Source/FactoryPresets.cpp` — 1,225 lines, 96 presets:

| Category    | Current Count | Notes |
|-------------|---------------|-------|
| Pads        | 18            | Largest existing bucket |
| Lead        | 15            | |
| Drone       | 12            | |
| Sequence    | 12            | |
| Bass        | 10            | |
| Pluck       | 10            | |
| Keys        | 8             | Under-served |
| FX          | 8             | Under-served |
| Percussion  | 3             | Severely under-served |

Each preset is a `OuariconPresetManager::FactoryPresetDef` with normalized [0,1] APVTS values, built via the `makePreset()` helper.

## Decisions (from Discuss Phase)

### D1 — Distribution: Expand AND Rebalance

Combine even expansion across all 9 categories with rebalancing weight toward the under-served buckets (Percussion, Keys, FX). No new categories — keep the existing taxonomy.

**Target distribution (96 → 192):**

| Category    | Current | New | Final | Δ  | Rationale |
|-------------|---------|-----|-------|----|-----------|
| Pads        | 18      | +8  | 26    | +8 | Already largest; modest expansion |
| Lead        | 15      | +9  | 24    | +9 | Modest expansion |
| Drone       | 12      | +10 | 22    | +10 | Strong O-Prism use case |
| Sequence    | 12      | +10 | 22    | +10 | Strong O-Prism use case |
| Bass        | 10      | +10 | 20    | +10 | Even doubling |
| Pluck       | 10      | +10 | 20    | +10 | Even doubling |
| Keys        | 8       | +12 | 20    | +12 | **Rebalance** — under-served |
| FX          | 8       | +12 | 20    | +12 | **Rebalance** — under-served |
| Percussion  | 3       | +15 | 18    | +15 | **Rebalance** — severely under-served |
| **Total**   | **96**  | **+96** | **192** | | |

> Numbers are a target; final allocation may shift ±2 per bucket during plan phase based on creative coherence. The rebalancing principle (Percussion/Keys/FX get the largest deltas) is fixed.

### D2 — Wavetable Coverage: Broaden to Under-Used Tables

Existing 96 presets lean heavily on `WT_Saw`, `WT_Square`, `WT_Triangle`, `WT_FMEPiano`, `WT_FMBell`, `WT_PWMSweep`, `WT_Supersaw`. The new 96 should prioritize the **under-used tables** to showcase the full 27-wavetable library:

- `WT_FMMetallic` (9), `WT_Wavefold` (10), `WT_Bitcrush` (11)
- `WT_VowelMorph` (12), `WT_ChoirPad` (13), `WT_VocalLead` (14), `WT_FormantFilter` (15)
- `WT_HarmonicSeries` (16), `WT_SpectralTilt` (17), `WT_OddHarmonics` (18)
- `WT_HarmonicStretch` (19), `WT_CombSweep` (20), `WT_PrismSpectrum` (21)
- `WT_Breath` (22), `WT_PluckedString` (23), `WT_ChurchBell` (24)
- `WT_OrganSweep` (25), `WT_Wind` (26), `WT_FilteredNoise` (27)

**Research-phase task:** audit the existing 96 to produce a wavetable-frequency histogram, then select a target distribution for the new 96 that fills the gaps.

### D3 — Microtonal: HARD CONSTRAINT — Presets Must Not Touch Tuning

**Critical rule from user:** *"Presets should not touch tuning ever — the tuning should be separate and persist as whatever it was set to before. In other words, if a tuning is selected the user should be able to move through presets while the same tuning persists."*

**Status:** Architecturally enforced. `PluginProcessor.cpp:474-477` registers 7 excluded IDs:

```cpp
presetManager.excludedParameterIds = {
    "tuningPreset", "tonic", "masterTune", "octaveStretch",
    "pitchBendRange", "glideMode", "glideTime"
};
```

`OuariconPresetManager.h:181, 214, 519` skip these IDs on save / load / applyFactoryDefinition.

**Verification criteria for execute & verify phases:**
1. No new preset's raw map may contain any of the 7 excluded IDs.
2. Manual DAW test: load tuning → step through new presets → confirm tuning unchanged.

Design implication: presets are 12-TET-first (existing model). The active Scala scale re-colors them at runtime.

### D4 — Authoring: Hand-Author Each Preset

All 96 new presets are **deliberate, hand-crafted parameter snapshots** with creative names, matching the existing 96. **No procedural generation, no A/B variation farming.**

This is the slowest path but matches the curation quality of the existing library. Each preset gets:
- A distinct creative name (no "Pad 2", "Bass Variant 3")
- A parameter snapshot that explores a specific sonic idea
- Mod matrix usage where it adds movement (existing presets average ~2 mod slots)
- Effects use that fits the category (Pads → reverb-forward, Bass → minimal FX, etc.)

## Out of Scope

- Adding new APVTS parameters (preset format unchanged)
- New categories (existing 9 remain)
- Tuning-aware preset design (D3 prohibits)
- New wavetables (use the existing 28)
- Mod matrix expansion (use the existing 4 slots)
- UI changes to the preset browser (it already auto-paginates by category)

## Success Criteria

1. `FactoryPresets.cpp` builds 192 presets total (verifiable by `grep -c 'makePreset' Source/FactoryPresets.cpp` returning 193, including the helper definition).
2. Final category counts match D1 target (±2 per bucket).
3. Wavetable histogram across all 192 presets covers all 27 wavetables with ≥2 presets each (D2).
4. Stepping through all 192 presets in Logic / Ableton with a non-12-TET tuning loaded leaves tuning unchanged (D3).
5. Pluginval passes (level 5).
6. CHANGELOG entry under MINOR version bump (1.17.4 → 1.18.0).

## Version Bump

**MINOR** (1.17.4 → 1.18.0) — preset library doubling is a substantive feature addition with no parameter schema changes, no breaking changes, full backward compatibility for user-saved presets.

## Affected Files

- `plugins/O-Prism/Source/FactoryPresets.cpp` (primary — adds ~1200 LOC for 96 new presets)
- `plugins/O-Prism/CMakeLists.txt` (version bump only)
- `plugins/O-Prism/CHANGELOG.md` (1.18.0 entry)
- `PLUGINS.md` (version row update)

No source changes required to `OuariconPresetManager`, `PluginProcessor`, `PluginEditor`, or any DSP file.

## Open Questions for Research Phase

1. Wavetable histogram of existing 96 — which tables are 0-use? Which are over-represented?
2. Mod matrix usage histogram — which (src, dst) pairs are unused but musically interesting?
3. Effects parameter coverage — are there reverb/delay/chorus configurations underexplored in the current 96?
4. Are the existing per-category sound-design templates (e.g. Pads start with `completeBase()` + reverb) consistent enough that we can document a per-category "starter sketch" for hand-authoring?
5. Is there a UI ceiling on preset count per category? (Browser pagination needs to be confirmed at 26 in Pads.)
