---
milestone: double-preset-library
plugin: O-Prism
domain: polish
execute_agent: general-purpose
version_bump: minor
base_version: 1.17.4
target_version: 1.18.0
created: 2026-05-06
---

# Plan — Double O-Prism Factory Preset Library

## Goal

Author 96 hand-crafted factory presets to grow `FactoryPresets.cpp` from 96 → 192. No DSP/UI/parameter changes. Tuning parameters never touched (architecturally enforced; D3 in CONTEXT.md).

## Wavetable Budget — Evidence-Based

Histogram of existing 96 presets (osc A + osc B usage):

| Wavetable | osc A | osc B | Total | Priority for new 96 |
|-----------|-------|-------|-------|---------------------|
| WT_VocalLead       | 1 | 0 | 1  | **CRITICAL** — primary osc-A in 2+ new |
| WT_SpectralTilt    | 1 | 0 | 1  | **CRITICAL** — primary osc-A in 2+ new |
| WT_FormantFilter   | 0 | 2 | 2  | **HIGH** — primary osc-A in 3+ new |
| WT_OddHarmonics    | 0 | 2 | 2  | **HIGH** — primary osc-A in 3+ new |
| WT_HarmonicStretch | 0 | 2 | 2  | **HIGH** — primary osc-A in 3+ new |
| WT_SyncSweep       | 2 | 0 | 2  | MEDIUM — primary osc-A in 2 new |
| WT_OrganSweep      | 3 | 1 | 4  | MEDIUM |
| WT_CombSweep       | 3 | 1 | 4  | MEDIUM |
| WT_Bitcrush        | 3 | 1 | 4  | MEDIUM |
| WT_ChoirPad        | 3 | 1 | 4  | MEDIUM |
| WT_Wind            | 3 | 1 | 4  | MEDIUM |
| WT_PluckedString   | 4 | 1 | 5  | LOWER |
| WT_PWMSweep        | 4 | 1 | 5  | LOWER |
| WT_Breath          | 2 | 3 | 5  | LOWER |
| WT_VowelMorph      | 4 | 2 | 6  | LOWER |
| WT_HarmonicSeries  | 3 | 3 | 6  | LOWER |
| WT_ChurchBell      | 4 | 2 | 6  | LOWER |
| WT_PrismSpectrum   | 4 | 3 | 7  | OK |
| WT_FilteredNoise   | 2 | 6 | 8  | OK |
| WT_FMMetallic      | 6 | 2 | 8  | OK |
| WT_Wavefold        | 5 | 4 | 9  | OK |
| WT_Supersaw        | 6 | 3 | 9  | OK |
| WT_Triangle        | 5 | 9 | 14 | minor |
| WT_Square          | 5 | 9 | 14 | minor |
| WT_FMBell          | 8 | 7 | 15 | well-covered |
| WT_Sine            | 6 | 17| 23 | well-covered |
| WT_Saw             | 11| 14| 25 | well-covered |

**Coverage rule:** every wavetable must reach ≥3 osc-A uses across the full 192-preset library after this milestone.

## Task Breakdown

Sequential, single-file work. Each task adds presets to one category in `FactoryPresets.cpp`, in the same style as the existing 96 (call `out.push_back(makePreset(...))` with category string + name + RawMap merged onto `completeBase()`).

### Pre-execute Tasks

| # | Task | Verification |
|---|------|--------------|
| 0a | Backup: `backups/O-Prism/v1.17.4/` (full plugin folder copy) | Backup folder exists, byte-equal to source |
| 0b | Confirm category constant strings used in code (read `FactoryPresets.cpp` per-category section comments to lock canonical names) | Match exact strings: `"Pads"`, `"Drone"`, `"Lead"`, `"Bass"`, `"Pluck"`, `"Keys"`, `"Sequence"`, `"FX"`, `"Percussion"` |

### Authoring Tasks (one per category)

Each task:
- Inserts new presets at the END of the existing category section in `FactoryPresets.cpp`
- Uses `makePreset(apvts, cat, "Name", merge(base, { ... }))` pattern
- Builds incrementally — Release build after every 3-4 tasks, not every preset
- Verifies wavetable budget via running histogram (script in `scripts/preset-histogram.sh` to be added)

| # | Task | New | Final | Wavetable Focus |
|---|------|-----|-------|-----------------|
| 1 | Pads — author 8 new | 18→26 | 26 | FormantFilter, ChoirPad, OrganSweep, Breath, ChurchBell, HarmonicStretch, SpectralTilt, VowelMorph |
| 2 | Drone — author 10 new | 12→22 | 22 | HarmonicSeries, OrganSweep, ChurchBell, HarmonicStretch, Wind, OddHarmonics, FilteredNoise, ChoirPad, PrismSpectrum, SpectralTilt |
| 3 | Lead — author 9 new | 15→24 | 24 | VocalLead (×2), FormantFilter, FMMetallic, Wavefold, OddHarmonics, CombSweep, HarmonicStretch, ChoirPad |
| 4 | Bass — author 10 new | 10→20 | 20 | PrismSpectrum, HarmonicStretch, FMMetallic, Wavefold, CombSweep, Bitcrush, FormantFilter, OddHarmonics, Supersaw, PluckedString-low |
| 5 | Pluck — author 10 new | 10→20 | 20 | ChurchBell, PluckedString, CombSweep, FMMetallic, FormantFilter, OddHarmonics, Wavefold, PrismSpectrum, ChurchBell+chorus, HarmonicSeries |
| 6 | Keys — author 12 new | 8→20 | 20 | OrganSweep, FormantFilter, FMBell+chorus, PluckedString, FMMetallic, HarmonicSeries, FMEPiano-warm, FormantFilter-organ, FMMetallic-toy, ChurchBell, FMBell-marimba, ChoirPad |
| 7 | Sequence — author 10 new | 12→22 | 22 | Bitcrush, FMMetallic, FormantFilter, PluckedString, Wavefold, VowelMorph, CombSweep, HarmonicSeries, PrismSpectrum, Breath |
| 8 | FX — author 12 new | 8→20 | 20 | Wind, FilteredNoise, Bitcrush, FormantFilter, ChurchBell, Breath, CombSweep, Wavefold, PrismSpectrum, HarmonicStretch, SpectralTilt, OrganSweep |
| 9 | Percussion — author 15 new | 3→18 | 18 | FilteredNoise (×4), Wind, Bitcrush (×2), Wavefold, ChurchBell, FMMetallic (×2), PluckedString (×2), OddHarmonics, Sine-sub (×2) |

**Subtotal:** +96 presets, final = 192. Final distribution: 26+24+22+22+22+20+20+20+18 = **194**. *Adjust during execute: trim Drone or Sequence by 2 to land at 192, or accept slight overshoot — confirm during build.*

> Correction: target sums to 192. Adjusting Sequence 22→22 stays, Drone 22→22 stays, Pluck 20→20 — actual: Pads 26 + Lead 24 + Drone 22 + Sequence 22 + Bass 20 + Pluck 20 + Keys 20 + FX 20 + Percussion 18 = **192**. ✓

### Post-execute Tasks

| # | Task | Verification |
|---|------|--------------|
| 10 | Wavetable histogram check across all 192 | Every wavetable ≥3 osc-A uses; no regression on existing-favored tables |
| 11 | Tuning-persistence verification: grep new code for any of the 7 excluded IDs | `grep -E '"(tuningPreset\|tonic\|masterTune\|octaveStretch\|pitchBendRange\|glideMode\|glideTime)"' FactoryPresets.cpp` returns 0 |
| 12 | Update `CMakeLists.txt` version 1.17.4 → 1.18.0 | Version string match |
| 13 | Update `CHANGELOG.md` with v1.18.0 entry | Entry present, lists categories + new count |
| 14 | Update `PLUGINS.md` row | Version column shows 1.18.0, date column shows 2026-05-06 |
| 15 | Build (Release VST3 + AU on macOS) via `./scripts/build-and-install.sh O-Prism` | Build succeeds, installer phase 4 sweeps cleanly |
| 16 | Pluginval level 5 | Pass |
| 17 | Manual DAW smoke test (verify phase) | Step through 5+ presets in any category with a non-12-TET tuning loaded; tuning unchanged |

## Tentative Preset Names (Approval Gate)

These are the 96 new names lined up for hand-authoring. Final wavetable assignments may flex during execution to maintain the histogram budget; **names will not change without re-confirmation.**

### Pads (+8 → 26 total)
1. Glass Curtain *(FormantFilter / ChoirPad)*
2. Stratosphere *(SpectralTilt + ChoirPad)*
3. Bone Cathedral *(ChurchBell + ChoirPad)*
4. Lichen Drift *(HarmonicStretch)*
5. Hollow Star *(OrganSweep + Breath)*
6. Salt Air *(VowelMorph + Wind)*
7. Lithium Glow *(VowelMorph + ChoirPad)*
8. Snowmelt *(Breath + ChoirPad)*

### Drone (+10 → 22 total)
1. Continental Plate *(HarmonicSeries + Sub)*
2. Glacier Breath *(SpectralTilt + Wind)*
3. Stone Choir *(OrganSweep + ChoirPad)*
4. Salt Ocean *(HarmonicStretch)*
5. Bell Forest *(ChurchBell + reverb)*
6. Iron Hum *(OddHarmonics + Sub)*
7. Cathedral Furnace *(PrismSpectrum + reverb)*
8. Coral Tide *(FilteredNoise + slow LFO)*
9. Volcanic Throat *(HarmonicStretch + drive)*
10. Fossil Bell *(ChurchBell slow attack)*

### Lead (+9 → 24 total)
1. Glass Throat *(VocalLead)*
2. Phantom Choir *(VocalLead + reverb)*
3. Ember Tongue *(FMMetallic)*
4. Static Halo *(Wavefold + HarmonicSeries)*
5. Iron Howl *(CombSweep)*
6. Mantis Cry *(OddHarmonics)*
7. Velvet Razor *(FormantFilter + ChoirPad)*
8. Solder Burn *(FMMetallic + Wavefold)*
9. Magma Wire *(HarmonicStretch + drive)*

### Bass (+10 → 20 total)
1. Iron Wolf *(PrismSpectrum + sub)*
2. Tar Pit *(HarmonicStretch + sub)*
3. Magnet Bass *(FMMetallic + sub)*
4. Bone Saw *(Wavefold)*
5. Black Tide *(CombSweep + sub)*
6. Stutter Gut *(Bitcrush + LFO)*
7. Velvet Floor *(FormantFilter + sub)*
8. Plasma Trench *(OddHarmonics + drive)*
9. Coral Reese *(Supersaw + Wavefold)*
10. Glass Hammer *(PluckedString-low + drive)*

### Pluck (+10 → 20 total)
1. Brass Bell *(ChurchBell short env)*
2. Wire Fence *(PluckedString + LFO)*
3. Gut String *(PluckedString warm)*
4. Comb Drop *(CombSweep short)*
5. Solder Drop *(FMMetallic short env)*
6. Vellum Harp *(FormantFilter pluck)*
7. Shell Click *(OddHarmonics very short)*
8. Frost Bell *(ChurchBell + chorus)*
9. Wax Pluck *(Wavefold short env)*
10. Spectral Spark *(PrismSpectrum + delay)*

### Keys (+12 → 20 total)
1. Cathedral Pipe *(OrganSweep + reverb)*
2. Reed Organ *(FormantFilter + slow attack)*
3. Vibraphone Cold *(FMBell + chorus)*
4. Mbira *(PluckedString + vibrato)*
5. Music Box *(FMMetallic small)*
6. Harmonium *(HarmonicSeries + vibrato)*
7. Wood Rhodes *(FMEPiano warm)*
8. Vox Continental *(FormantFilter + organ)*
9. Toy Piano *(FMMetallic short)*
10. Gamelan *(ChurchBell metallic)*
11. Glass Marimba *(FMBell pluck)*
12. Choir Organ *(ChoirPad slow attack)*

### Sequence (+10 → 22 total)
1. Crystal Gate *(Bitcrush LFO)*
2. Mantra Arp *(FMMetallic + delay)*
3. Beam Walk *(FormantFilter + LFO)*
4. Marble Step *(PluckedString + arp gate)*
5. Solder Loop *(Wavefold + LFO)*
6. Vowel Drift *(VowelMorph + slow LFO)*
7. Strobe Forest *(CombSweep + delay)*
8. Liquid Counter *(HarmonicSeries + filter LFO)*
9. Glass Telegraph *(PrismSpectrum + delay)*
10. Steam Cycle *(Breath + gate LFO)*

### FX (+12 → 20 total)
1. Reverse Surge *(Wind + reverse env)*
2. Solar Flare *(HarmonicStretch + filter sweep)*
3. Atom Split *(Bitcrush + delay storm)*
4. Comet Tail *(FilteredNoise + long verb)*
5. Storm Approach *(Wind + slow LFO)*
6. Bell Rain Backwards *(ChurchBell reverse)*
7. Vapor Trail *(Breath + tail)*
8. Mech Bloom *(CombSweep impact)*
9. Pressure Vent *(FilteredNoise slow open)*
10. Spectral Fold *(Wavefold + Spectral)*
11. Subspace Echo *(PrismSpectrum + ping delay)*
12. Static Bloom *(Bitcrush + reverb wash)*

### Percussion (+15 → 18 total)
1. Sub Boom *(Sine + sub very short)*
2. Glass Hat *(FilteredNoise short)*
3. Wood Knock *(PluckedString very short)*
4. FM Tom *(FMMetallic + envelope)*
5. Click Snap *(Bitcrush + Wavefold short)*
6. Ride Cymbal *(FilteredNoise + bell tail)*
7. Bell Tap *(ChurchBell short)*
8. Snare Shimmer *(FilteredNoise + harmonic ring)*
9. Bongo *(PluckedString warm short)*
10. Tribal Drum *(FormantFilter + low pluck)*
11. Synthetic Conga *(FMMetallic + body)*
12. Glitch Hit *(Bitcrush very short)*
13. Wind Burst *(Wind very short)*
14. Tom Floor *(Sine sub + body)*
15. Synth Clave *(Wavefold + short tone)*

## Risks & Mitigations

| Risk | Mitigation |
|------|------------|
| Single-file 1225 → ~2400 LOC concurrency conflicts | Sequential per-category authoring; commit after each category |
| Preset name collisions with existing | Pre-flight grep on each new name before writing |
| Wavetable budget drift | Run histogram script (task #10) before declaring done |
| Build breakage from RawMap typos | Build after every 3 categories — fail fast |
| Tuning param leak | Task #11 grep is the gate |
| UI category overflow | Browser already auto-paginates by category in `getPresetListWithCategories` — verify in DAW only at end |
| Preset count overshoot | Target 192 exactly; if a category overruns by 1-2 names, drop the weakest from that category |

## Verification Criteria (for verify phase)

Maps to CONTEXT.md success criteria:

1. ✅ `grep -c 'out.push_back (makePreset' FactoryPresets.cpp` = 192
2. ✅ Per-category counts match table above ±2
3. ✅ Wavetable histogram across all 192 — every wavetable ≥3 osc-A uses
4. ✅ Tuning persistence — manual test: load Bohlen-Pierce in standalone, step through 5+ new presets per category, tuning unchanged (architecturally guaranteed; manual test confirms)
5. ✅ Pluginval level 5 passes
6. ✅ CHANGELOG entry under v1.18.0

## Approval Gate

User must approve the **preset name list** before execute phase begins. Wavetable assignments may flex during execute to maintain histogram targets.

If user wants to:
- **Trim names:** strike them now and adjust per-category count
- **Add names:** call out which to add and from which category to drop
- **Rename:** edit the list before execute starts
- **Change category strategy:** edit CONTEXT.md D1, regenerate this plan

Default action on approval: enter execute phase (`/clear` → `/improve-milestone O-Prism`).
