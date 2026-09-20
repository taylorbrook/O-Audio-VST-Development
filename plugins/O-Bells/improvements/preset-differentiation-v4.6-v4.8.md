# Preset Differentiation — Investigation Brief and Staged Plan

**Plugin:** O-Bells
**Baseline:** v4.5.1
**Date:** 2026-09-20
**Report:** "the presets are sounding too similar to one another"
**Shape:** five sequential `/improve` runs, v4.5.2 → v4.8.0. Order is load-bearing (see Sequencing).

---

## Measured baseline (voice-only offline render, C4, vel 0.8, 48 kHz)

Metric: RMS dB difference of level-normalised log-band energies (24 bands, 60 Hz–16 kHz)
over 7 log-spaced time frames (0–6 s), floor −60 dB. Processor one-pole LP applied in
analysis; reverb/chorus/delay/EQ not rendered (presets only ever set `reverbMix`).

| Quantity | tap (0.25 s hold) | held (6 s) |
|---|---|---|
| Self-noise (same preset twice — per-note ±25 % amp randomisation) | 3.4 dB | 3.2 dB |
| Preset pair median / p10 / min | 13.0 / 7.4 / 3.7 | 14.1 / 8.2 / 4.1 |
| 60 random points across the whole parameter space, pair median | 11.5 | 10.3 |

> **Step 1 correction (v4.5.2).** The held `min` of 4.1 does not reproduce — not on the in-repo
> harness (2.7–3.5 over 8 seeds, mean 3.2) and not on the scratch harness that produced it
> (re-runs 2.7 / 2.8 / 3.5). It was one clock-seeded draw of an extreme statistic. Read it as
> **3.2**; `report.py` gates on that. Every other cell reproduces within 1 dB. 8-seed spread:
> median sd 0.2, p10 sd 0.2, min sd 0.3 dB — budget for that when writing the Step 5 ≥ 8 dB gate.

- 15 of 25 presets have a nearest neighbour within 3.7–5.9 dB — inside ~1.5× self-noise.
  Worst: Deep Bronze Tower ↔ Grand Cathedral Bell 3.8; Soft Mallet Bronze ↔ Velvet Bronze Tone 3.7;
  Slow Tolling Bell ↔ Distant Cathedral 4.5.
- Tap T40 is inverted: Large Bells 2.2–3.2 s (shortest in the bank); Crisp Steel Bar 5.5 s,
  Clanging Steel Plate 5.9 s (longest), despite bodyTime 800 / 500 ms.
- Spectral centroid at C4 (262 Hz) sits at 150–630 Hz for every preset.
- Presets already span the engine's reachable range → preset re-voicing alone cannot fix this.

## Root causes

### RC-1 — Material is a 2-way switch (bug)
`PluginProcessor.cpp` loads `materialParam` raw (choice index 0–4) and passes it to
`BellVoice::updateParameters`. `BellVoice::getMaterialProperties()` does
`round(material * 4.0f)` then `jlimit(0,4)` — it expects the normalised 0–1 value.
Index 0 → Bronze; 1,2,3,4 → Cast Iron.
Measured: idx 1/2/3 are 1.6/2.1/2.7 dB from Cast Iron (self-noise 2.2). Fed the normalised
value instead, materials separate by 6–12.5 dB from Bronze.
9 presets choose Brass/Steel/Aluminum and play as Cast Iron (dark −0.25 tilt, 1.5× decay).
grep token: `std::round(material * 4.0f)`

### RC-2 — Sub / upper-octave layers never decay while Bloom > 0 (bug)
`startNote` calls `initializeBloom()` on sub/oct partials (→ `bloomPhase = 0`), but the sub and
oct loops in `renderNextBlock` never call `applyBloom()`, and `applyMultiStageDecay()` returns
early while `bloomPhase < 1`. The layer holds at `initialAmplitude` until note-off.
Measured (sub 0.6, held 12 s, 2-s steps): bloom 0 → −34…−51 dB (decays);
bloom 0.05 → −29, −33, −36, −35, −34, −34 dB (flat).
All 25 presets have bloomAmount ≥ 0.05; 22 use an octave layer.
Secondary: sub/oct partials are copied from the fundamental AFTER its `initializeBloom`, then
bloomed again, so `initialFraction` is applied twice.
grep token: `Skip decay during bloom phase`

### RC-3 — Presets authored against an inverted Damping law (preset data, not DSP)
Live reach of `damping`: `stopNote()` release `jmap(damping, 3.0 s, 0.5 s)` and the hum-stage
multiplier on partials 0–1 — higher damping = SHORTER. The v2.2.1 presets were written against
the since-deleted `decayRate` path (removed v4.3.2) where higher = longer.
Measured tap T40 (bodyTime 4000, hum 90): damping 0 → 9.5 s, 0.5 → 6.0 s, 1.0 → 2.2 s.
Large Bells sit at 0.88–1.0. **Do not flip the parameter** — tooltip (v4.3.2 item 66) documents
the live law; fix the preset values.

### RC-4 — Narrow engine range (design limit)
- 8 partials, base amplitude `1/(p+1)`; partials 0 and 1 (ratios 0.5, 1.0) are identical in all
  three ratio tables (`harmonicRatios`, `bellRatios`, `gamelanRatios`) and dominate every spectrum.
- One `inharmonicity` axis interpolates harmonic → bell → gamelan; tables differ only in p2–p7.
- Hum-stage time constant is `1/(0.5 + b3·f²)` ≈ 2 s × humExtension × material × damping term,
  independent of `bodyTime` → a 500 ms "plate" still carries a multi-second fundamental tail.
- No control over hum (p0) / prime (p1) level.

### RC-5 — Unused differentiation axis
No preset sets chorus*, delay*, eq*, reverbSize/Damp/Predelay/Mod/Shimmer. Preset apply resets
unset params to default first, so all 25 share identical FX apart from `reverbMix`.

### Noted, not planned
`startNote` lines ~231–235 overwrite every fundamental-layer partial frequency with
`calculatePartialFrequency(p, 1, currentInharmonicity)`, discarding the material inharmonicity
offset, `noteVariationInharmonicity`, and the ±10-cent per-partial randomisation computed in
`initializePartials()` (they survive only on sub/oct layers). Decide in Step 2 whether to honour
or delete; honouring it changes every note's tuning scatter.

---

## Sequencing (why this order)

1. Harness first — every later step needs a gate, and the numbers above need to be reproducible in-repo.
2. Bug fixes before any voicing — voicing against RC-1/RC-2 would have to be redone.
3. Engine extension before voicing — the presets must be able to use the new parameters.
4. Re-voice once, last, against the final engine.
5. New parameters are APPEND-ONLY with defaults that reproduce v4.6.x output bit-for-bit in
   character (Model = current interpolation, Hum Level / Prime Level = 0 dB, Hum Follow = 0),
   so no step after 2 changes an existing session. MINOR, not MAJOR.

## Steps

| # | Version | Type | Scope | Gate |
|---|---|---|---|---|
| 1 | 4.5.2 | PATCH | In-repo render harness + baseline report. No plugin binary change. | Reproduces the table above ±1 dB |
| 2 | 4.6.0 | MINOR | RC-1 material mapping, RC-2 octave-layer decay (+ double initialFraction). Audible change to saved sessions using Brass/Steel/Aluminum or Sub/Oct + Bloom — called out in CHANGELOG. | Materials pairwise ≥ 5 dB; sub layer falls ≥ 12 dB between 2 s and 10 s held |
| 3 | 4.7.0 | MINOR | RC-4 DSP: `partialModel` choice (Classic / Tubular / Plate / Bowl / Glass …), `humLevel`, `primeLevel` (dB), `humFollow` (0–1: hum-stage τ tracks bodyTime). Processor + voice only; generic defaults = old sound. | Default-param render within self-noise of v4.6.0; random-param pair median ≥ 16 dB (was 11.5) |
| 4 | 4.7.1 / 4.8.0-pre | MINOR | UI for the Step-3 params: controls, tooltips, en / fr / zh-Hans rows, width pins, `check-ui-labels`, `i18n-fr-lint`, `i18n-zh-lint`. | All three language arms pass; no moved elements outside the new section |
| 5 | 4.8.0 | MINOR | Re-voice all 25 presets (RC-3, RC-5 + new params); factory sentinel `4.1.1` → `4.8.0`. | Every preset's nearest neighbour ≥ 8 dB (tap AND held); tap T40 ordering Large > Warm > Bright ≥ Metallic-short; category medians distinct |

Step 3 and 4 may be merged if the UI work is small; keep them split if the new section needs layout work.

## Harness

`tests/render-harness/` (Step 1, v4.5.2): `O-Bells-render-test` (processor-level,
`JUCE_WEB_BROWSER=0`, no editor TU, identity macros derived from the plugin target), `report.py`
(preset bank + baseline gate) and `probes.py` (RC-1…RC-4 measurements + random-range gate). The
scratch seed harness it was promoted from is deleted.

    cmake -B build -G Ninja -DOUARICON_BUILD_TESTS=ON && ninja -C build O-Bells-render-test
    python3 plugins/O-Bells/tests/render-harness/report.py
    python3 plugins/O-Bells/tests/render-harness/probes.py

Traps, both handled in the harness: the voice RNG is seeded from clock ^ `this` (test-only seed
hook, harness target only); and seeding every job in a run IDENTICALLY is common random numbers —
the per-note randomisation cancels between presets and near-neighbour distances read ~1.5 dB low,
so each job is seeded from (seed, job index). The ±25 % per-partial amplitude randomisation is NOT
governed by `humanize`.
