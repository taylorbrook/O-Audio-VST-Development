# Old bank review: the 64 factory presets (1.2.0)

**Milestone:** preset-bank-expansion (R4) · **Researched:** 2026-11-14 · read-only, no file in the repo changed
**Inputs:** `Source/FactoryPresets.h` (lines below), `Source/GiParamIds.h` (defaults), `tests/render-harness/preset_gates_4.h` (FP0), `tests/render-harness/factory_gen.cpp` (recipes of the 19 generated sources), `Resources/factory/manifest.json`, `.planning/stages/4-polish/evidence-r2/r2-levels.txt` + `R2.full.txt` (the last recorded level table and FP2 line; 1.1.0 and 1.2.0 left all 64 renders bitwise, so they still hold).

## Method

A script parsed every spec, expanded `s*_` wildcards as `expandSpecValues` does, applied the `GiParamIds` defaults, and checked for these no-ops:
- a parameter on a disabled stream (literal keys only; wildcards reach disabled streams by design)
- filter parameters, or a route to the cutoff, with `filter_type` Off
- `fb_*` parameters with feedback 0
- reverb size, damp or position with `reverb_mix` 0
- ring parameters with `ring_mix` 0
- `crush_prob` with crush 0
- `window_skew` with window 0
- free size or density under Tempo, or synced size or density under Free
- a route with depth 0, or a route to a disabled stream
- LFO or mod-envelope settings with no route reading them
- an MPE source without MPE
- glide in Poly mode
- `output_gain`

It also listed every `pitch_rand`, `root_fine` and pitch route. Each description was read against its values by hand. The engine was checked where a value's effect was in doubt: head run target at `Engine.cpp:4040`, scan wrap at `ScanPlayhead.h:139`, preset exclusions at `PluginProcessor.cpp:120`.

**Clean results:**
- No preset sets `output_gain`.
- No dead route, no route to a disabled stream, no filter parameter under Off, and no depth-0 route.
- No unused LFO or mod-envelope settings, and no tempo or free mismatch.
- No Keys preset wobbles: `pitch_rand` is 0, there is no pitch route, and the plugin has no unison or chorus. All 8 Keys presets pass the FP0 Keys rule.
- The root parameters are **not** excluded from preset load. Only `output_gain` and `rec_arm` are excluded. So a preset that omits the root resets it to 48 / 0, and one that writes `sN_root_*` (Velvet Strings) does apply it.

## 1. Defects

Severity:
- **M**: breaks a project rule, or matters for this milestone.
- **L**: wording only.
- **I**: informational; the recommendation is no change.

Description-only fixes are free. Descriptions are documentation: they are not written to the preset JSON and are not hashed by `bankHash`, so no render or bank stamp moves.

| # | Sev | Preset | Line | Evidence | Proposed fix |
|---|-----|--------|------|----------|--------------|
| D1 | M | Pads · Velvet Strings | L729, L731 | The title names a real instrument, but the values are `s1_root_fine -8`, `s2_root_fine +8` with `s2_pitch +12`. The octave stream's fundamental lands 16 ct off stream 1's 2nd harmonic, and every coinciding partial pair beats; at C3 the beat is about 2.4 Hz. This is the "Oct layer on a coinciding partial with detune" trap from the realism rule (memory: feedback_realism_titled_presets_must_not_wobble). FP0 gates wobble only in Keys, so this was never gated. | Set both `root_fine` to 0 (delete the two keys). Reword "detuned against each other". Do it together with re-voice V4 (bowed). Changes the render. |
| D2 | M | Textures · Ember Field | L879 (level), L874 | Quietest preset in the bank: FP2 gated −30.0 dB, the floor of the 7.00 dB spread; level table −29.30 dB against a median of −26.15. `s1_level` is already **+6.0, the range ceiling**, so `sN_level` cannot trim it up. It is not a D72 outlier, because D72 compares versions, not the bank. | Re-voice to `fire` (V5) and re-measure. If it is still low, raise density or reverb, since level has no headroom. |
| D3 | I | Pads · Fifth Bloom | L642 | Loudest preset: FP2 gated −23.0 dB, level table −22.04 dB, +3.9 dB over the median. Together with D2 it sets the 7.00 dB spread, which leaves **2.0 dB** of the 9 dB FP2 budget for the 96 new presets. | No change. Trim all three streams by about 1 dB (OS1 rule) only if the new bank's spread gets tight; that would move the preset's render. |
| D4 | I | Drones · Frozen Field | L168, L171 | `freeze 1` parks every head: the run target needs `! gp.freeze` (Engine.cpp:4040). So `s*_feedback 40` and `s*_fb_diffusion 70` never act. The description already says "the feedback stays parked". The values exist to meet the FP0 Drones rule (feedback 30–80 with diffusion). | No change. It is documented, and dropping the values would break FP0. |
| D5 | I | Drones · Deep Well, Drones · Night Engine | L150, L218 | `s2_fb_enable 0` with `s2_feedback 0` is redundant, because a head at feedback 0 parks anyway. It records the intent "the noise bed stays out of the loop". | No change. Removing it moves the bank hash without changing the sound. |
| D6 | L | Keys · Glass Mallet | L350 vs L355–357 | "Bell with a pluck an octave above, low-passed …", but only s1 (bell) has the 6 kHz LP. The octave pluck (s2) is unfiltered. | Reword, for example "Bell, low-passed, with a bright pluck an octave above". This is superseded if V8 is taken. |
| D7 | L | Motion · Orbit Lines | L484 vs L492 | The description says "per-voice LFOs pan and shift the reads", but only LFO 1 is per-voice. `lfo2_mode` is absent, so LFO 2 is Global, and it is the one that moves the s2 read. | Reword: "a per-voice LFO pans, a slow global LFO shifts the vowel read". |
| D8 | L | Motion · Crush Tides | L552 vs L561 | "per-voice LFOs move the crush and the read": LFO 1 is per-voice (crush), but LFO 2 is Global (position). | Reword the same way as D7. |
| D9 | L | Textures · Ember Field | L875 | "Tiny dust grains", but `s1_size` is 110 ms. Glitter Haze, at 35 ms, is the one with tiny grains. | Reword to "Dust grains …". This is superseded if V5 is taken (the description is rewritten anyway). |
| D10 | L | Textures · Rust Cloud | L906 vs L913 | "Noise and dust, driven hard and crushed on every grain", but only s1 has crush, at `crush_prob` 100. s2 (dust) has drive 40 and no crush. | Reword to "the noise crushed on every grain, the dust driven". |
| D11 | L | Rhythmic · Gated Pad | L794 vs L797 | "Long pluck grains on 1/8", but `s1_size_sync` 8 = 1/16 D is only one step over the 1/16 default, shorter than the 1/8 grid. | Reword: "Pluck grains on 1/8 …". Lengthening `size_sync` to 10 (1/8) would change the sound; not recommended. |

**Not defects:**
- Motion · Reverse River routes LFO 2 to `s1_pitch_rand`. That is a pitch wobble, but Motion is not a realism title.
- Pads Pentatonic Mist, Just Glow, Nineteen and Seventeen Glass use `pitch_rand` over a scale. That is by design, and they are not realism titles.
- Expressive · Bend Choir: the bend is player-driven, not wobble.
- Motion · Loop Weaver: the Env 1 route uses the default mod envelope (10 / 500 / 0). That is a deliberate onset bump.
- The descriptions of the Live templates match their values.

## 2. Re-voice proposals (ranked)

The ranking is by how clearly the **name** calls for the new source.

**Effects of any re-voice:**
- It moves that preset's bitwise render, its loudness and the bank hash. Slot names are in `customState`, and `customState` is hashed.
- After a source change, re-trim the level with `sN_level` (FP2, `--preset-level-table`).
- The 56 presets not taken stay bitwise.

None of the 8 recommended sources is off-C3. They are all 48 / 0 in the manifest, so **no `root_note` / `root_fine` writes are needed**, and the R2 root row does not touch them.

**Watch the level headroom.** Kalimba, Glass Mallet, Rain Glass and Ember Field already sit at `s1_level` +6.0, the ceiling. If the new source is quieter, it cannot be trimmed up with `sN_level`.

### Recommended (8)

| Rank | Preset | Line | Change | Why | Companion changes / caveats | Verdict |
|------|--------|------|--------|-----|-----------------------------|---------|
| V1 | Keys · Kalimba | L372 | s1 `pluck` → `kalimba` | The name *is* the source: a tine with an overtone near 5.4×, plus a thumb noise. | Lane name "Kalimba". Description becomes "Short kalimba-tine grains under a soft low-pass…". Keys FP0 is unaffected. Both sources are 3 s, and the current read already wraps at 2.5 s (scan 120). Level is at the ceiling: check that it is not quieter. | **Accept** (named) |
| V2 | Drones · Organ Loft | L153 | s1 + s2 `harmonic` → `organ` | The name is an instrument the bank now has: drawbars 8′…1′, with a tremulant that moves amplitude only (no pitch wobble). | Lane names "Organ". Optional: `sN_loop_start` about 2 % to keep the key click at the start of the tape (`addStrike` at frame 0) out of the ping-pong reads. The Drones FP0 rule is unaffected. Re-trim; s1 is at −13.0 and s2 at −12.2, so there is headroom. | **Accept** (new find) |
| V3 | Pads · Warm Choir | L630 | s1 + s2 `vowel` → `choir` | The title says Choir. The choir tape drifts "ah" → "oo", so the description "an LFO walks the vowel" still holds. | `choir` is stereo. It has a 200 ms fade-in at the start of the tape, which positions 30 / 50 already avoid. **Taylor's call:** the source has a built-in ensemble detune of ±7–9 ct and about 6 ct of vibrato per voice. That is a real choir's character, and the preset itself adds none, but the realism rule should accept it explicitly. Re-trim (headroom: −5 / −10). | **Accept** (named) |
| V4 | Pads · Velvet Strings | L725 | s1 + s2 `harmonic` → `bowed`, **plus D1** (both `root_fine` → 0) | The title says Strings: bowed has 1/k harmonics with body resonances and a bow swell. | Description becomes "Two bowed streams an octave apart under a soft low-pass". The source's vibrato is 7 ct at 5.3 Hz and **fades in over 1.6 s**. At position 0 with spray 20, most grains fall before that, which reads as a calm ensemble. Set `sN_position` to about 5–10: there is a 150 ms fade-in at the start of the tape, and position 0 reads into it. Pads FP0 is unaffected. Re-trim (headroom). | **Accept** (named) |
| V5 | Textures · Ember Field | L874 | s1 `dust` → `fire` | "Ember" names the crackle recording. Fire is 11.5 s, with a seamless loop. This also addresses D2, the bank's quietest preset. | Description is rewritten (fixes D9). Level is at the ceiling: measure before and after. If it is still the floor of the spread, raise density or reverb, not level. Textures FP0 (density ≥ 70, spray 100) is unaffected. | **Accept** (new find) |
| V6 | Drones · Hollow Wind | L174 | s1 `noise_bed` → `wind` | Same pink-noise family, with its own wandering resonance (250 Hz–1.6 kHz) and gusts. The name asks for wind. | Keep the BP at 650 Hz / res 35 and the S&H on the cutoff. The two resonances stack, so listen for whistle. Description: "…on the wind…". Re-trim (s1 −0.3 → headroom to +6). | **Accept** (new find) |
| V7 | Textures · Rain Glass | L829 | s1 `dust` → `rain_synth`; s2 `noise_bed` stays | "Rain" is now a source: about 450 drops/s plus a hiss. The randomised band-pass stays as the "glass". | Optional, Taylor's call: s2 → `glass`. That would add pitched pings at the played key and change the character a lot, so it is not recommended. s1 is at the ceiling: measure. | **Accept s1** (named) |
| V8 | Keys · Glass Mallet | L349 | s1 `bell` → `glass`; s2 `pluck` → `marimba_bar` optional | The name is a mallet on glass. `glass` is a struck glass with partials 2.32 / 4.25 / 6.63, and `marimba_bar` supplies the mallet thump. | **Wrap caveat:** `glass` is 2.0 s and `marimba_bar` is 2.5 s. Under Retrigger at scan 100 the read wraps (`ScanPlayhead.h:139`), so a held note re-strikes every 2 s; at `amp_sustain` 45 that is audible. Lower `amp_sustain` to about 15–20, or accept the re-strike. Reword per D6. s1 is at the ceiling: measure. | **Accept s1**; s2 optional (named) |

### Judged and not recommended

These are listed so Taylor can pull any of them in.

| Preset | Line | Candidate | Reason |
|--------|------|-----------|--------|
| Expressive · Slide Glass | L237 | `bell` → `glass` | The name fits, but the preset sustains (sustain 50, release 1.5 s) for MPE slides. Glass dies in about 0.5 s (τ 0.55) and wraps at about 3.3 s (scan 60), and the bell carries the slide better. Optional, only together with a sustain cut. |
| Textures · Static Choir | L840 | `vowel` → `choir` | The name's anchor is "Static" (radio static). Crushed, band-passed vowel already reads that way. Optional. |
| Expressive · Bend Choir | L260 | s1 `vowel` → `choir` | Plausible: Slide → position would still morph ah → oo. But the description is "a close vowel", and the Expressive split is unaffected either way. Optional, lower than V1–V8. |
| Textures · Wind Harp | L883 | `noise_bed` → `wind` | The "harp" is a key-tracked BP at res 70 on a neutral noise. Wind's own wandering resonance would fight that tracking. Reject. |
| Rhythmic · Triplet Rain | L781 | s2 `pulse` → `pulse_triplet` | The content would match the 1/16 T grid. But "Rain" is the image, and the grid already holds. Optional. |
| Textures · Glitter Haze | L895 | `vowel` → `shimmer` | Shimmer is already C6–C8, and with `s1_pitch +24` it lands at C8–C10. It needs pitch 0, which turns it into a different preset. Reject; better used in a new preset. |
| Textures · Rust Cloud | L905 | s2 `dust` → `metal_grate` | "Rust" is a stretch for a grate recording. Optional. |
| Motion · Crush Tides | L551 | s1 `noise_bed` → `waves` | "Tides" fits loosely, but the waves would be crushed and band-passed beyond recognition. Reject. |
| Motion · Reverse River | L502 | `drone` → `stream` | The concept is the drone read backwards. A field recording reversed loses the tonal centre. Reject. |
| Motion · Clockwork | L609 | `pulse` → `clicks` | `clicks` is irregular (exponential gaps), and clockwork is regular. Reject. |
| Motion · Phase Garden | L588 | → `garden` | The concept is two harmonic streams a fifth apart drifting in and out of phase. Reject. |
| Keys · Soft Rhodes | L361 | `harmonic` → `fm_bell` | `fm_bell` is a 1 : 3.5 ratio, which is inharmonic and bell-like, not a Rhodes tine (1 : 1 / 1 : 14). Reject. |
| Keys · Celesta, Keys · Bell, Keys · Toy Box | L331, L340, L393 | `fm_bell` / `glass` / `kalimba` | The bell family is right for a celesta and a music box. Reject. |
| Keys · Grain Piano, Keys · Harp Cascade | L322, L382 | — | There is no piano source, and a pluck *is* a plucked string. Reject. |
| Expressive · MPE Breath | L225 | `vowel` → `breath` | "Breath" names pressure-as-breath control, and the vowel carries the formant morph that the routes move. Reject. |
| Expressive · Mono Lead | L274 | → `saw_stack` | `saw_stack` is 7 detuned saws (a built-in chorus), wrong for a "vibrato-free" mono lead. Reject. |
| Expressive · Glide Bass | L284 | → `sub` | `sub` is a near-pure sine at C1 (it would need root 24 / 0), so the drive and LP character goes. Reject. |
| Drones Iron Sky, Deep Well, Frozen Field, Low Orbit, Tape Halo, Night Engine | — | metal_plate / sub / contrabassoon / field | The names are images and `drone` fits all of them. Field recordings belong in the new Field category. Reject. |
| Remaining Pads and Rhythmic presets | — | — | Concept presets (tunings, intervals, grids). Reject. |

**R1 coverage effect.** If V1–V8 are taken, kalimba, organ, choir, bowed, fire, wind, rain_synth and glass (plus marimba_bar if V8's s2 is taken) each already appear in one old preset. The 96 new presets then need only one more of each to meet "every source in at least 2 presets".

## 3. Source usage of the old bank

Cells read lanes / presets. A preset that lists the same source twice counts 2 lanes and 1 preset. The 8 Live templates list no slots. None of the 33 sources new in 1.2.0 is used anywhere.

| Category | harmonic | pluck | bell | vowel | noise_bed | dust | drone | pulse | lanes |
|----------|----------|-------|------|-------|-----------|------|-------|-------|-------|
| Drones | 4 / 3 | – | – | – | 3 / 3 | – | 5 / 5 | – | 12 |
| Expressive | 3 / 3 | – | 2 / 2 | 4 / 4 | – | – | – | – | 9 |
| Keys | 1 / 1 | 5 / 4 | 4 / 4 | – | – | – | – | – | 10 |
| Live | – | – | – | – | – | – | – | – | 0 |
| Motion | 4 / 3 | 1 / 1 | 1 / 1 | 2 / 2 | 1 / 1 | 1 / 1 | 1 / 1 | 1 / 1 | 12 |
| Pads | 8 / 6 | – | – | 5 / 4 | – | – | 4 / 4 | – | 17 |
| Rhythmic | – | 5 / 5 | – | – | – | – | – | 5 / 5 | 10 |
| Textures | – | – | – | 4 / 4 | 4 / 4 | 4 / 4 | – | – | 12 |
| **Total** | **20 / 16** | **11 / 10** | **7 / 7** | **15 / 14** | **8 / 8** | **5 / 5** | **10 / 10** | **6 / 6** | **82** |

`harmonic` dominates: 20 of the 82 lanes, and 6 of the 8 Pads. `dust` and `pulse` are the least used. After V1–V8, `harmonic` drops to 15 lanes, `vowel` to 13, `dust` to 3 and `noise_bed` to 7.

## Reference: level context

The last recorded table is `evidence-r2/r2-levels.txt`, 0–10 s, dB:

| | Value | Preset |
|---|-------|--------|
| Median | −26.15 | |
| Low | −29.30 | Ember Field |
| | −28.04 | Glide Bass |
| | −27.72 | Keys · Bell |
| High | −22.04 | Fifth Bloom |
| | −23.09 | Slow Tide |
| | −23.13 | Dotted Echo |

FP2 gated, from `R2.full.txt`: −30.0 (Ember Field) to −23.0 (Fifth Bloom), a spread of 7.00 dB; max peak 0.560.

Thirteen presets have `s1_level` at the +6.0 ceiling:

| Line | Preset |
|------|--------|
| L310 | Velo Shards |
| L327 | Grain Piano |
| L336 | Celesta |
| L345 | Bell |
| L354 | Glass Mallet |
| L377 | Kalimba |
| L387 | Harp Cascade |
| L398 | Toy Box |
| L540 | Ring Spinner |
| L777 | Dotted Echo |
| L833 | Rain Glass |
| L854 | Sand Drift |
| L879 | Ember Field |

The plucked and struck sources sit low in loudness at the same setting, so new Keys presets will hit the same ceiling. Plan the level calibration loop to adjust density or `vel_to_amp`, not only `sN_level`.
