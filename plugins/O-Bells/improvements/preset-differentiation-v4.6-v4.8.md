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

> **FIXED v4.6.0.** The lookup rounds the index. Closest pair Bronze ↔ Cast Iron 6.3 dB, the rest
> 8.2–19.8. Count correction: **11** presets use Brass / Steel / Aluminum (4 / 4 / 3), not 9.

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

> **FIXED v4.6.0.** Both layer loops call `applyBloom()`; the copy takes `targetAmplitude`. Read in
> 50–90 Hz (the sub layer's own hum partial), 2 s → 10 s: 0.0 dB on v4.5.2 at every bloom > 0; now
> equal to the bloom-0 row to 0.1 dB. Count correction: **24** presets have Sub or Oct > 0 (18 / 17).
>
> **Gate amendment (2026-09-20).** "≥ 12 dB between 2 s and 10 s" is the hum-stage law's figure at
> damping 1 (12.3). At the default damping 0.7 the same law gives 9.1 dB — with or without bloom — so
> 12 is unreachable there without changing damping semantics, which Step 2 forbids. The gate asserts
> what the fix promises: every bloom row tracks the bloom-0 row (±0.5 dB) at damping 0.7 AND 1.0, and
> ≥ 12 dB at damping 1.0. Both clauses fail on v4.5.2.
>
> **Observed, not planned.** That same early-return parks decay for the whole bloom on EVERY layer, and
> the stage is chosen by absolute time since note-on — so any bloom > 0 skips the strike stage.
> Bloom 0 → 0.01 is a 6–8 dB level step (defaults, 0.5–1.5 s). All 25 presets sit on the bloom > 0
> side, so re-voicing is unaffected; decide before anyone ships a Bloom-0 preset.

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

> **EXTENDED v4.7.0.** Four append-only parameters (version hint 2), defaults bit-identical to v4.6.0:
> `partialModel` (Classic / Tubular / Plate / Bowl / Glass), `humLevel`, `primeLevel` (−24…+6 dB),
> `humFollow` (0–1). Random-range pair median, same 60 points + the four new axes: **tap 11.3 → 16.4,
> held 10.2 → 15.5** (seed 2: 16.3 / 15.4). Models 19–32 dB apart at defaults. Hum Follow 1 at Body
> 500 ms: prime decay 1.17 → 4.62 dB/s.
>
> **The tables were NOT in `research/`** — only the free-free bar series was. Written in-step:
> `research/idiophone-partial-models.md`, provenance-graded per row. Plate computed (Leissa's
> equation, reproduces his table), Bowl and Tubular quoted from measurements, **Glass recalled from
> Rossing 1994 and only cross-checked against a figure — re-verify if the paper turns up.** All four
> AMPLITUDE rows are design values, not measurements (except Glass slots 1–4, read off Jundt Fig. 3a).
>
> **Decisions.** Slot 1 stays the tuned prime (1.0) in every model — Tubular therefore carries a quiet
> SYNTHETIC prime at its virtual strike pitch. Plate / Bowl / Glass keep the engine's 0.5 hum at −18 dB
> so Hum Level is live everywhere. On a model, Inharmonicity stretches the one table
> (`ratio^(1 + 0.3·(inh − 0.5))`, slots 2–7) instead of morphing three. Hum Follow replaces the law's
> fixed 1/b1 = 2 s with Body Time on EVERY partial's hum stage, not only p0–1 — the multi-second tail
> was all eight partials. Models cull partials above 0.45·fs at note-on.
>
> **Held gate.** The Step-3 gate "≥ 16 (was 11.5)" is the tap column: +4.5 dB. Held has its own baseline
> (10.3) and is gated at the same gain (≥ 14.8); it reads 15.5, NOT 16. Say so rather than round it.
>
> **Observed, not planned.**
> 1. Classic has NO Nyquist guard: gamelan 9.5× on the Oct layer aliases above ~C6 at 48 kHz.
>    Pre-existing; left alone because fixing it changes Classic renders. grep token: `MODEL_NYQUIST_GUARD`
> 2. Hum Follow is inaudible on a TAP: the ring after note-off is the damping-driven release
>    (τ 1.25 s at default), T40 3.9 → 2.9 s only. A short "plate" preset needs high Damping as well —
>    Step 5 voicing, and one more reason RC-3 matters.
> 3. `.planning/parameter-spec.md` is the 22-parameter Stage-0 document (no `humanize`, no FX); it was
>    not extended. `.planning/params.tsv` (regenerated, 69 params, append-only diff) is the live record.

### RC-5 — Unused differentiation axis
No preset sets chorus*, delay*, eq*, reverbSize/Damp/Predelay/Mod/Shimmer. Preset apply resets
unset params to default first, so all 25 share identical FX apart from `reverbMix`.

### Noted, not planned — DECIDED v4.6.0: DELETE
> Not honoured. A ±10-cent random scatter on the prime partial defeats the tuning engine, and honouring
> it would have moved every note of every session. The dead computation is removed from
> `initializePartials()` (its RNG draw kept, so unaffected renders stay bit-identical); the fundamental
> layer is exactly tuned by definition. The sub / oct scatter stays. Partials 0–1 share one ratio in all
> three tables, so the MATERIAL inharmonicity offset would be tuning-safe to honour — a candidate axis
> for Step 3's `partialModel`, not a bug fix. grep token: `NOT applied (never was`
>
> **Found while deciding it — unison voice 0 (FIXED v4.6.0, user-approved as a third audible change).**
> `initializePartials()` always wrote `fundamentalVoices[0]`, so with Unison ≥ 2 each later pass
> re-initialised voice 0 after its frequency was set: voice 0 kept the LAST voice's detune (the one place
> the "dead" computation was live). Unison 2 @ 50 c measured one peak at +25 c; Unison 3: 0 / +50 / +56.
> Now −25.8 / +25.4 and −49.5 / +0.8 / +49.7. 17 of 25 presets use Unison ≥ 2.

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
| 2 | 4.6.0 ✅ | MINOR | RC-1 material mapping, RC-2 octave-layer decay (+ double initialFraction), unison voice-0 detune (found in-step). Audible change to saved sessions using Brass/Steel/Aluminum, Sub/Oct + Bloom, or Unison ≥ 2 — called out in CHANGELOG. | Materials pairwise ≥ 5 dB; sub layer tracks its bloom-0 decay and falls ≥ 12 dB between 2 s and 10 s held at damping 1 (amended, see RC-2); unison prime peaks on the designed detune; 5 untouched configs bit-identical to v4.5.2 |
| 3 | 4.7.0 ✅ | MINOR | RC-4 DSP: `partialModel` choice (Classic / Tubular / Plate / Bowl / Glass …), `humLevel`, `primeLevel` (dB), `humFollow` (0–1: hum-stage τ tracks bodyTime). Processor + voice only; generic defaults = old sound. | Default-param render within self-noise of v4.6.0; random-param pair median ≥ 16 dB (was 11.5) |
| 4 | 4.7.1 ✅ | PATCH | UI for the Step-3 params: controls, tooltips, en / fr / zh-Hans rows, width pins, `check-ui-labels`, `i18n-fr-lint`, `i18n-zh-lint`. | All three language arms pass; no moved elements outside the new section |
| 5 | 4.8.0 ✅ | MINOR | Re-voice all 25 presets (RC-3, RC-5 + new params); factory sentinel `4.1.1` → `4.8.0`. | Every preset's nearest neighbour ≥ 8 dB (tap AND held); tap T40 ordering Large > Warm > Bright ≥ Metallic-short; category medians distinct |

Step 3 and 4 may be merged if the UI work is small; keep them split if the new section needs layout work.

> **Step 4 DONE v4.7.1 (kept split, shipped as PATCH — no parameter or state change).** New **Partials**
> section between Synthesis and Ensemble, one `.param-row-4`: Model · Hum Level · Prime Level · Hum Follow.
> The width pin is structural — `.param-row-4` columns are `flex: 1; min-width: 0`, 174.5 px on every arm;
> widest caption `Niveau bourdon` 101.5 px. `check-ui-labels`: 0 moved on fr and zh-Hans in all 15 states.
> fr rows are `reviewed: false` (9), zh-Hans `'mt'` (9) — **both worklists are open going into Step 5.**
> Step 5 note: the tips tell the user Hum Follow is heard on HELD notes and that Inharmonicity stretches the
> table on a model — voice the presets so those statements stay true.

> **Step 5 DONE v4.8.0.** Same 5 categories, same 25 names (none stopped fitting). Voice-isolated: pair median
> 13.3 → 25.1 tap / 14.7 → 25.6 held; worst nearest neighbour 3.5 → **10.6** over both seeds (tap, seed 1: Deep
> Bronze Tower ↔ Massive Iron Bell) against the ≥ 8 gate. Tap T40 medians, 12 s window: Large 9.94 > Warm 5.56 >
> Bright 4.18 ≥ Clanging Steel Plate 1.45 / Shimmering Bell Tree 2.68; Ambient 8.37, Metallic 3.55.
>
> **Gate readings.** "Metallic-short" is two named presets, not the category — Metallic also holds the gong and the
> iron bowl (9.5 / 8.9 s). "Category medians distinct" is read as the five tap-T40 medians pairwise ≥ 0.25 s apart; it
> failed once (Bright 3.96 vs Metallic 4.01) and was met by voicing (Gamelan Damping 0.6 → 0.75). T40 needs the 12 s
> window: at 6 s every low-damping bell saturates at 5.98 and Large > Warm would be a tie.
>
> **Observed, not planned.**
> 1. The harness reads the bank from the INSTALLED plugin's `~/Library/O-Bells/Presets/Factory`, rewritten only on a
>    sentinel change — a second voicing pass under `4.8.0` would have gated the first bank. `report.py` now removes
>    the sentinel each run. Voicing itself was iterated through job overrides (defaults + overrides == preset apply),
>    and the real `preset=` path reproduces those numbers to 0.1 dB.
> 2. The re-voiced bank spread 20 dB in level (old bank 8.5), and Frozen Steel Shimmer hit −3 dBFS on one note. A first
>    pass fixed that with per-preset Output Gain — **rejected: a preset must not move the user's gain.** Balanced by
>    voicing instead (8.7 dB spread, peak −8.1 dBFS); `report.py` fails any factory preset that names `outputGain`.
>    Level levers that are NOT gain: the layer norm divides by 1 + sub + oct, the nonlinear tanh divides by its drive,
>    strike position sets the hum / prime gain, and a size-1 / damp-0 reverb stacks +5.6 dB on a sustained voice.
>    Still open: preset apply RESETS Output Gain to 0 dB on every load (only `tuning_*` is exempt) — same principle,
>    but it lives in the shared preset-manager module, so it was not changed here.
> 3. Classic's missing Nyquist guard (Step 3 note 1) is still open; the bank was steered off it (Sparkling Aluminum →
>    Tubular; Gamelan Oct 0; Crotale Oct 0.25). Any future Classic preset with a high Oct layer re-exposes it.
> 4. FX on (informational, not gated): worst neighbour 9.1 / 8.8 — reverb pulls dark long presets together.
> 5. The Bloom 0 → 0.01 level step (RC-2 note) is untouched: no preset ships at Bloom 0 (minimum 0.02).

## Harness

`tests/render-harness/` (Step 1, v4.5.2): `O-Bells-render-test` (processor-level,
`JUCE_WEB_BROWSER=0`, no editor TU, identity macros derived from the plugin target), `report.py`
(preset bank + baseline gate — **re-anchored to v4.8.0** in Step 5: tap 2.4 / 25.1 / 16.3 / 10.6, held
2.5 / 25.6 / 16.1 / 11.0, plus the Step-5 nearest-neighbour and tap-T40 bank gates; the v4.5.1 table above and the
v4.6.0 anchor are history, not the gate) and `probes.py` (RC-1…RC-4
measurements + random-range gate, still on its v4.5.1 anchor: 11.3 / 10.2 after Step 2; from v4.6.0
also the material, sub-layer, unison and bit-identity-vs-v4.5.2 gates; from v4.7.0 the model, Hum
Follow, extended-range and `--alloc-check` gates). The
scratch seed harness it was promoted from is deleted.

    cmake -B build -G Ninja -DOUARICON_BUILD_TESTS=ON && ninja -C build O-Bells-render-test
    python3 plugins/O-Bells/tests/render-harness/report.py
    python3 plugins/O-Bells/tests/render-harness/probes.py

Traps, both handled in the harness: the voice RNG is seeded from clock ^ `this` (test-only seed
hook, harness target only); and seeding every job in a run IDENTICALLY is common random numbers —
the per-note randomisation cancels between presets and near-neighbour distances read ~1.5 dB low,
so each job is seeded from (seed, job index). The ±25 % per-partial amplitude randomisation is NOT
governed by `humanize`.
