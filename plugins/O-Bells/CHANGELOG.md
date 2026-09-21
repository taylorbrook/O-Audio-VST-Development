# O-Bells Changelog

All notable changes to O-Bells will be documented in this file.

## [4.8.0] - 2026-09-21

Step 5 (last) of `improvements/preset-differentiation-v4.6-v4.8.md`: the factory bank
re-voiced against the final engine and grown **25 → 40**. MINOR: no parameter, range
or state change — sessions load and sound as they did on 4.7.1. **What changes is the
factory bank**: the original 5 categories and 25 names recall different sounds, 15
presets are new, and the on-disk bank regenerates on first launch.

### Changed — the 25 existing presets

- **RC-3 — Damping was authored inverted.** The live law is higher = SHORTER
  (note-off release 3 s → 0.5 s, and the hum stage of partials 0–1); the v2.2.1
  bank was written against the deleted `decayRate` path and put Large Bells at
  0.88–1.0. Now: Large Bells 0–0.15, Ambient 0–0.3, Warm 0.35–0.55, Bright
  0.55–0.8, bars / plates / anvils 0.75–1.0. The parameter and its tooltip are untouched.
- **RC-4 — every preset uses the 4.7.0 axes.** Hum Level spans −24…+6 dB, Prime
  Level −12…+6 dB. Hum Follow is set only alongside high Damping or as a partial
  shortening on held notes — it is inaudible on a tap by itself (brief, Step 3 note 2).
- **Out of the 0.3–0.7 band.** Inharmonicity 0.0–1.0; Mallet 0.0–1.0; Strike
  0.05–1.0; Overtone 0.05–1.0; Body 350–5000 ms; Bloom to 1.0 with Fine bands
  staggered (Evolving Bronze Wash); LP filter down to 900 Hz.
- **RC-5 — every preset has its own FX signature.** Until now no preset named a
  chorus, delay, EQ or reverb-shape parameter, so all shared one FX chain apart from
  Reverb Mix. Across the 40: delay on 13 (7 PingPong; 60 ms slap → 1.2 s toll), EQ
  on 29, Reverb Shimmer on 5, chorus on 5 (stylised presets only — see below), and
  size / damp / pre-delay / mod voiced per preset (Clanging Steel Plate 0.2 size,
  0 ms → Distant Cathedral 1.0 size, 150 ms, 85 % wet).

### Added — 15 presets (3 per category), built on what the old engine could not reach

Partial Model across the 40: Classic 13, Tubular 8, Plate 7, Bowl 6, Glass 6.

- **Large Bells:** Temple Bowl Bell (Bowl), Steel Tubular Chime (Tubular, steel),
  Clangorous Brass Zvon (Classic toward gamelan ratios, tierce +25 c — an untuned bell).
- **Warm Bells:** Rosewood Tube Chime (Tubular), Glass Marimba (Glass), Felt Celesta
  (fully harmonic Classic, prime only).
- **Bright Bells:** Steel Glockenspiel (Tubular at full stretch), Wine Glass Ping
  (Glass), Bronze Finger Cymbal (Plate, prime −12 dB).
- **Metallic:** Anvil Strike (Plate, cast iron — the shortest preset), Brake Drum
  (Bowl, steel), Thunder Sheet (Plate, nonlinear 0.9).
- **Ambient:** Glass Halo, Bowed Bowl Drone, Worn Tape Chimes.

### Pitch wobble — presets whose title names a real instrument no longer wobble

Reported on the first pass of this version: "the pitch is too wobbly, sounds
unrealistic on presets that suggest realism in the title". Measured as the RMS
deviation (cents) of the instantaneous frequency in the prime and hum bands, 0.4–4 s
of a held C4. Unison 1 with no chorus reads **< 2 c**. The causes, in order:

1. **Wide Unison detune** — 14–31 c RMS at the voice (Massive Iron Bell 3 × 22 c:
   30.9; Dark Iron Resonance 16.3; Distant Cathedral 14.1; Deep Bronze Tower 14.0).
2. **Chorus** — Underwater Bell 29.7, Velvet Bronze Tone 30.0, Sparkling Aluminum 11.5.
3. **Reverb Mod ≥ 0.3** — about 10 c (Grand Cathedral Bell 12.8 with FX on).
4. The voice's own Shimmer parameter is nearly innocent (≤ 1 c at 0.3, Unison 1).
5. Found while fixing 1: **a Sub / Oct layer stacked on a lifted hum.** The layer's
   prime lands ON the fundamental layer's hum with a random ±10 c scatter. Three
   unison voices averaged that out; on ONE voice it is a slow beat at the hum and up
   to **8 dB of note-to-note level swing** (Massive Iron Bell, Sub 0.85: −12.5…−20.4
   dBFS over five notes; Sub 0: 1.4 dB). The scatter is the engine's and was left
   alone; the realism presets no longer stack the two — the hum IS a bell's sub-octave.

So every realism preset is now **Unison 1, no chorus, Reverb Mod ≤ 0.2**, and none
stacks Sub on a lifted hum: 28 of 40 presets, all reading ≤ 1.7 c voice-isolated
(≤ 7.7 c with FX on — a large static hall alone reads 7–8 c: dense reflections, not
modulation). Warm Aluminum Bars lost its 4.5 Hz chorus "motor": a vibraphone motor is
tremolo, and the chorus is vibrato. Twelve presets are declared STYLISED and keep some
motion on purpose — seven of Ambient, Beating Bronze Gong (2 × 16 c: the beating is in
the title), Dense Bronze Gamelan (2 × 14 c ombak), Shimmering Bell Tree (a cluster of
bells), Thunder Sheet, Velvet Bronze Tone — but were roughly halved (worst 46.7 → 24.4 c).

### No preset names Output Gain

It is the user's control, not a preset's (`report.py` gates it). Pushing parameters
to their extremes first spread the bank over 20 dB (−14.0…−33.8 dBFS RMS; C3 / C4 /
C5, vel 1, FX on, loudest 1-s window) and put one held note of Frozen Steel Shimmer
at −2.9 dBFS. The bank is balanced by VOICING instead — hum / prime level, strike
position, sub / oct blends (the layer norm divides by 1 + sub + oct), nonlinear
drive (its tanh divides by the drive), EQ boosts dropped from the loud presets, wet
mixes. Across the 40: **10.9 dB** spread (−18.5…−29.4; 80 % of the bank inside 7.4
dB; the old 25 spread 8.5), loudest single-note peak −7.0 dBFS. The quiet end is the
short percussive presets, which read low on unweighted RMS; the loud end is Steel
Tubular Chime (steel's 2× decay). Treat these as ±2–3 dB: per-note amplitude
randomisation moves a single dark preset that much between notes. Frozen Steel
Shimmer's level was the REVERB, not the voice: size 1 / damp 0 stacks +5.6 dB on a
sustained plate even at 35 % wet → size 0.8, damp 0.2, 30 % wet, softer mallet.

### Aliasing exposure (Classic has no Nyquist guard — brief, Step 3 note 1)

Not fixed here (it changes Classic renders), but the bank no longer leans on it:
Sparkling Aluminum moved to a guarded model (Tubular); Dense Bronze Gamelan keeps Oct
0 as in v2.2.1; Bright Clear Crotale keeps Oct at 0.25 (was 0.2); none of the 15 new
presets uses an Oct layer. grep token: `MODEL_NYQUIST_GUARD`

### Factory bank on disk

Sentinel `4.1.1` → `4.8.0`: existing installs rewrite
`~/Library/O-Bells/Presets/Factory` on first launch. All 25 old names are kept, so
every old file is overwritten in place — no orphaned JSON. User presets are untouched.

### Harness

`report.py` gains the bank gates and re-anchors to v4.8.0:

- every preset's nearest neighbour ≥ 8 dB, tap AND held, on BOTH seeds;
- tap-T40 category medians Large > Warm > Bright ≥ each short Metallic preset
  (Anvil Strike, Brake Drum, Clanging Steel Plate, Shimmering Bell Tree), and the
  five medians pairwise ≥ 0.25 s apart. T40 is read on a 12 s tap — on the 6 s window
  every low-damping bell saturates at 5.98 s and the ordering would be a tie;
- pitch wobble: realism presets ≤ 3 c voice-isolated and ≤ 10 c with FX on, stylised
  ≤ 30 c. The FX ceiling cannot see a MILD chorus (4.5 Hz / 35 % reads 5.1 c, under
  the hall floor), so the causes are gated as well: a realism preset names no
  chorus, Unison 1, Reverb Mod ≤ 0.2;
- no factory preset names `outputGain`.
- **Stale-bank guard.** The harness reads the bank from the installed plugin's
  `~/Library/O-Bells/Presets/Factory`, which is only rewritten when the sentinel
  changes — re-voicing twice under one sentinel would have gated the first bank.
  `report.py` now removes `.factory_version` before every run.

### Testing

Voice-isolated (FX bypassed, High Fidelity on), C4, vel 0.8, 48 kHz, seed 1:

| | tap v4.7.1 (25) → v4.8.0 (40) | held |
|---|---|---|
| self-noise | 2.5 → 2.4 | 2.4 → 2.5 |
| pair median | 13.3 → **26.4** | 14.7 → **27.1** |
| pair p10 | 8.0 → 19.6 | 9.3 → 20.3 |
| pair min = worst nearest neighbour | 3.5 → **9.9** | 3.6 → **9.5** |

- Nearest-neighbour gate (≥ 8 dB): PASS on both seeds, tap and held; worst cell over
  both seeds 9.5 (held, seed 1: Deep Bronze Tower). On 4.7.1, 15 of 25 presets had a
  neighbour inside 6 dB. Adding 15 presets did not cost separation: the pair median
  ROSE (25 presets, same voicing: 24.7 / 25.7).
- Tap T40 medians (12 s window): Large **9.08** > Warm **5.17** > Bright **3.94** ≥
  Anvil Strike 1.45 / Clanging Steel Plate 1.44 / Brake Drum 2.66 / Shimmering Bell
  Tree 2.69. Ambient 8.36, Metallic 3.04. On 4.7.1 Large Bells were the SHORTEST
  category (2.1–2.8 s) and the two steel bar / plate presets the longest (5.9 s).
- The distinct-medians gate failed twice in-step and was met by voicing both times,
  not by loosening: Bright 3.96 vs Metallic 4.01 (Dense Bronze Gamelan Damping 0.6 →
  0.75), then Ambient 8.36 vs Large 8.61 after the additions (three Large Bells
  Damping → 0–0.05).
- Negative controls. Nearest-neighbour: the same run with `NEAREST_MIN = 12` exits 1
  and names the sub-12 dB pairs (first voicing pass; that code is unchanged). Wobble:
  Massive Iron Bell with its old 3 × 22 c unison reads 27.8 c against the 3 c ceiling
  (0.1 c as shipped); the old chorus on Warm Aluminum Bars reads only 5.1 c with FX on
  — which is why the causes are gated structurally.
- The table in `PluginProcessor.cpp` is generated from the measured scratch table;
  the gate run through the real `preset=` path reproduces the scratch numbers to
  0.1 dB, so nothing drifted in transcription.
- Release build + install (VST3, AU, dev branding): `auval -v aumu OBls OuDv` PASS;
  pluginval strictness 5 SUCCESS. Harness fails any non-finite sample; none.
- Review history of this version, all before release: per-preset Output Gain
  (rejected — a preset must not move the user's gain), then the wobble report above.
- NOT verified by any gate: how it sounds. The hands-on listen in the Standalone is
  a human gate.

## [4.7.1] - 2026-09-20

Step 4 of `improvements/preset-differentiation-v4.6-v4.8.md`: the plugin window
gets controls for the four 4.7.0 parameters. PATCH: no new parameter, no state
change, no audio change — every session loads and sounds as it did on 4.7.0.

### Added

- **Partials** section on the Instrument tab, between Synthesis and Ensemble: one
  four-column row — **Model** (`partialModel`), **Hum Level**, **Prime Level**,
  **Hum Follow**. Model is a 5-way slider with a word readout, bound through a
  slider relay exactly as Material is (WR-03); its option words stay English on
  every arm (D-01 arms 1 and 3, five new `I18N_EXEMPT` rows). Level readouts are
  scaled dB (`0.0dB`, as Gain), Hum Follow a percentage. Double-click resets, as
  on every other slider.
- Four relays + attachments in `PluginEditor`; the four tips are bound to their
  `.param-control`, so caption, slider and readout all carry the tip.
- en / fr / zh-Hans rows for the section title, four captions and four tips
  (13 lint rows per arm, 255 → 268). French is drafted this step —
  `reviewed: false`, nine entries on the unreviewed worklist. zh-Hans is authored
  at `reviewed: 'mt'` (nine entries below the ship bar, counted not failed).
  Tip bodies were written against `BellVoice.cpp`, including the two things a
  user would otherwise trip on: Inharmonicity changes meaning on a model
  (stretches the one table), and Hum Follow is heard on held notes because the
  release after note-off is Damping's.

### Geometry

- The row is a `.param-row-4` (`flex: 1; min-width: 0`), so the four columns are
  174.5 px on every arm whatever the captions say — that is the width pin; no
  per-caption pin was needed. Widest caption on any arm: `Niveau bourdon`
  101.5 px (en `Hum Follow` 75.5, zh 44.1). Section height 65.97 px, caption box
  11 px, slider → readout gap 8 px, readouts 11 px `#000` — identical on en, fr
  and zh-Hans.
- Everything below Synthesis moves down by the new section's height on every
  arm alike. The tab already scrolls.

### Testing

- `check-ui-labels --plugin O-Bells`: ALL CHECKS PASSED, exit 0. Geometry diff
  [7] reports 0 moved non-label elements on fr and on zh-Hans in all 15 states —
  inside the new section as well as outside it. Visible labels 126 → 131.
- `i18n-fr-lint`: 268 rows, 0 findings. `i18n-zh-lint`: 268 rows, 0 findings,
  9 at `'mt'`. `check-i18n`: 44 PASS, 0 FAIL.
- `boot-all-uis --plugin O-Bells`: clean 1 / 1, 0 DEAD bindings. The 2 late
  bindings (`#ref-pitch-knob`, `#octave-stretch`) are the lazily mounted tuning
  panel's, not this section's.
- `tests/ui_tip_render_check.js`: 1522 PASS, 0 FAIL. Its pinned `SLIDER_COUNT`
  went 35 → 39; it failed on exactly that line before the pin was moved. All four
  new tips render inside the 800 × 600 frame on every arm (tallest: fr Partial
  Model, 260 × 219.9).
- Release build + install (VST3, AU, dev branding): `auval -v aumu OBls OuDv`
  PASS, the four parameters listed; pluginval strictness 5 SUCCESS. Neither opens
  the WebView editor, so the Standalone was built and launched once: the editor
  constructs with the four new attachments and quits clean, no crash report.
- NOT verified by any gate: the live readouts and drag behaviour in the real
  plugin. The generic UI stub seeds neutral values, so the readouts the headless
  gates saw are not the plugin's, and the section sits below the fold of the
  Standalone window. Hands-on in a DAW: Model steps through five words, the
  levels read −24.0 … 6.0dB with 0.0dB at default, double-click resets.

## [4.7.0] - 2026-09-20

Step 3 of `improvements/preset-differentiation-v4.6-v4.8.md` (RC-4): a wider
engine. MINOR: four new parameters, append-only; every existing session, preset
and automation lane loads and **sounds exactly as it did on 4.6.0** (bit-identical
render at the new parameters' defaults). Processor + voice only — the new
parameters have no controls in the plugin window yet (Step 4); they are
reachable from the host's generic parameter list / automation.

### Added

- **Partial Model** (`partialModel`: Classic / Tubular / Plate / Bowl / Glass,
  default Classic). Classic is the existing harmonic → bell → gamelan
  interpolation. The other four swap in one body's partial ratio AND amplitude
  table: a tubular chime (2 : 3 : 4.2 group over a virtual strike pitch), a free
  circular plate (dense, clangy), a singing bowl (1 : 2.77 : 5.18 : 8.12 …, steep
  fall) and a wine glass (weak second mode). Tables and their provenance:
  `research/idiophone-partial-models.md`. Each amplitude row is scaled to
  Classic's total, so switching model changes the spectrum, not the level
  (−39 … −32 dB vs Classic −36, first second, defaults).
  - Partial 1 is the tuned prime (ratio 1.0) in every model, so tuning tables,
    Scala files and Dorico note-expression behave identically across models.
  - On a model, **Inharmonicity** stretches the table about the prime
    (`ratio^(1 + 0.3·(inh − 0.5))`, partials 2–7; 0.5 = the table as published).
    Partial Tuning still moves partial 2.
  - Models drop any partial above 0.45 × sample rate at note-on (Bowl reaches
    19.6× the played pitch). Classic is untouched — see Known.
- **Hum Level** / **Prime Level** (`humLevel`, `primeLevel`: −24 … +6 dB, default
  0): level of partial 0 and partial 1, the two that dominate every O-Bells
  spectrum. The Sub / Oct layers inherit them.
- **Hum Follow** (`humFollow`: 0–1, default 0). The hum-stage time constant hangs
  off a fixed 2 s whatever Body Time says, so a 500 ms body still carried a
  multi-second tail. At 1 that 2 s becomes Body Time (Brilliance, Hum Sustain,
  Material and Damping keep their say); in between the two are blended in the log
  domain. Applies to every partial's hum stage. Body 500 ms, held: prime decays
  1.17 dB/s at 0, 4.62 dB/s at 1 (designed 4×); Body 4000 ms: 1.58 → 1.21 dB/s.
- The four parameters carry **version hint 2**, so AU lists them after every
  existing parameter and no automation index moves. `params.tsv` regenerated:
  65 → 69, the diff is the four new rows and the count.

### Known / left alone

- **Hum Follow does little to a short TAP.** After note-off the ring is the
  Damping-driven release (τ 1.25 s at the default), which Hum Follow does not
  touch: tap T40 3.9 → 2.9 s at Body 500 ms. A short struck-plate sound wants high
  Damping too. Preset work, Step 5.
- **Classic still has no Nyquist guard** (gamelan 9.5× on the Oct layer aliases
  above ~C6 at 48 kHz). Pre-existing; fixing it would change Classic renders, which
  this step promises not to do.
- The Glass ratio row is recalled from Rossing (1994) and cross-checked only against
  a published figure (within the measured glass-to-glass spread); all amplitude
  rows are design values. Graded row by row in the research file.
- Factory presets do not use the new parameters yet (Step 5). Preset recall resets
  them to default, so the bank is unchanged: `report.py` baseline gate PASS.

### Testing

- **Defaults reproduce 4.6.0 — bit-exact, not merely within self-noise:** the five
  `probes.py` identity hashes (recorded from the 4.5.2 tree, untouched by 4.6.0)
  are equal, and the 60-point random control (new parameters at default) reads
  11.3 tap / 10.2 held, as on 4.6.0.
- **Random-range gate, ≥ 16 dB (was 11.5):** the same 60 points with the four new
  parameters drawn from a separate generator (the 20 old dimensions are the
  original points exactly): **tap 16.4 / 16.3** (two seeds) — PASS, by 0.3–0.4 dB
  (seed sd 0.2). **Held 15.5 / 15.4** against its own 10.3 baseline: the same
  +5 dB, but it does NOT reach 16; it is gated at the equivalent gain (≥ 14.8).
- **Models pairwise ≥ 5 dB:** closest Plate ↔ Glass 19.9 / 19.3 dB; to Classic
  23.2–31.6 dB. Centroid at C4 223 Hz (Classic) → 336–548 Hz.
- **No allocation in the audio thread:** new `--alloc-check` on the render harness
  hooks libmalloc's `malloc_logger` (sees malloc / calloc / realloc, so
  `HeapBlock` and `operator new` alike), scoped to the thread calling
  `processBlock`, and proves the hook live on every run. 0 allocations: six configs
  (every model, Unison 4 + both octave layers) × C4 held / C8 / C0 tap / FX on.
  Two traps met on the way, both now handled in the harness: clang folds
  `armed = true; malloc(); armed = false` (flags must be `volatile`) — caught by the
  liveness check, which would otherwise have read as a clean pass — and JUCE's
  TimerThread mallocs during `processBlock` (hence the thread scope).
- C8 renders of every model are finite (harness non-finite check), Nyquist cull live.
- **pluginval strictness 10**, in-process, installed VST3 and AU: SUCCESS, 0
  failures. One AU warning, `Current program is -1` — a program-index notice,
  nothing this step touches.
- `auval -v aumu OBls OuDv`: AU VALIDATION SUCCEEDED, component version 4.7.0,
  69 parameters. Installed VST3 carries 0 `ForTesting` symbols.
- Not yet auditioned in a DAW.
- Regression baseline: `backups/O-Bells/v4.6.0/`.

## [4.6.0] - 2026-09-20

Step 2 of `improvements/preset-differentiation-v4.6-v4.8.md`: three voice bugs.
MINOR: no parameter, range, type or state format changed and every session
loads, but **the sound changes** — see the next section before updating a
project you need to recall exactly.

### ⚠ Saved sessions and presets WILL sound different

Nothing is migrated, because nothing was stored wrong — the stored values are
finally being honoured. A session sounds different if it uses any of:

- **Material = Brass, Steel or Aluminum.** All three have been playing as Cast
  Iron. They now play as themselves: Brass shorter (0.7× decay) and
  brighter, Steel much longer (2×) and brighter, Aluminum shortest (0.5×) and
  brightest. 11 factory presets. Bronze and Cast Iron are unchanged (bit-identical).
- **Sub or Oct layer > 0 together with Bloom Amount > 0.** The layer used to hold
  at a fixed level for as long as the key was down — it never decayed — and
  started quieter than intended. It now blooms and decays like the main layer:
  held notes no longer carry a static drone underneath, and the layer's attack
  is slightly louder. 24 factory presets (every preset has Bloom ≥ 0.05).
- **Unison ≥ 2.** The detune spread was one-sided: the lowest voice played at the
  HIGHEST voice's pitch. Unison 2 had no detune at all and sat `Detune / 2` sharp
  (12 c → +6 c); Unison 3 played 0 / +D / +D instead of −D / 0 / +D. The spread is
  now symmetric about the played pitch: real beating at Unison 2, a wider chorus
  at 3–4, and the note is no longer sharp. 17 factory presets.

To keep the old sound exactly: stay on 4.5.2 (`backups/O-Bells/v4.5.2/`, or
`git restore --source=<4.5.2 commit> -- plugins/O-Bells`). Unison 1 + Bronze or
Cast Iron + no Sub / Oct under Bloom renders bit-identically to 4.5.2.

### Fixed

- **RC-1 — Material was a 2-way switch.** `getMaterialProperties()` did
  `round(material * 4)`, written for the normalised 0–1 value; the processor
  passes `getRawParameterValue()`, which for a Choice is the index 0–4. Index 0 →
  Bronze, every other index clamped to 4 → Cast Iron. It now rounds the index.
- **RC-2 — Sub / upper-octave layers never decayed while Bloom > 0.**
  `startNote()` calls `initializeBloom()` on those partials (`bloomPhase = 0`) but
  their render loops never called `applyBloom()`, and `applyMultiStageDecay()`
  returns early while `bloomPhase < 1`. Both loops now call `applyBloom()`.
- **RC-2, secondary — `initialFraction` applied twice on those layers.** Their
  partials are copied from the fundamental layer AFTER its `initializeBloom()`,
  so the copied amplitude was already the bloom start level, then bloomed again.
  The copy now takes the pre-bloom level (`targetAmplitude`).
- **Unison voice 0 took the last voice's detune** (found while deciding the
  item below). `initializePartials()` always wrote into `fundamentalVoices[0]`
  and `startNote()` copied out of it, so each later unison pass re-initialised
  voice 0 after its frequency had been set. It now writes the voice it is
  initialising; the copy is gone.

### Decided — the brief's "noted, not planned" `startNote` frequency overwrite: DELETE

`startNote()` overwrites every fundamental-layer partial frequency with
`detune × table ratio`, discarding what `initializePartials()` computed: the
material inharmonicity offset, `noteVariationInharmonicity` and a ±10-cent
per-partial scatter. Not honoured: a 10-cent random scatter on the prime partial
defeats the tuning engine (Scala / Dorico microtonal playback), and it would have
moved every note of every session. The dead computation is removed; the
fundamental layer is exactly tuned by definition. The random draw it consumed is
kept so the per-voice RNG sequence — and therefore every unaffected render — is
unchanged. The Sub / Oct layers keep their own ±10-cent scatter (always live).
`MaterialProperties::inharmonicity` is now documented as not applied; since
partials 0–1 share one ratio in all three tables it would be tuning-safe to
honour, which makes it a candidate axis for Step 3, not a bug fix.

### Not changed

- **Damping semantics** — law, range, tooltip and every coefficient untouched
  (tap T40 at damping 0 / 0.5 / 1: 9.4 / 6.0 / 2.2 s, identical to 4.5.2).
- Observed, left alone: any Bloom > 0 also skips the strike-stage decay of every
  layer (the decay is parked until the bloom ends, by which time the strike
  window has passed), so Bloom 0 → 0.01 is a 6–8 dB level step (measured,
  defaults, 0.5–1.5 s). Pre-existing on the main layer; recorded in the brief.

### Testing

- `probes.py` gained four gates; all PASS on 4.6.0 and all FAIL on a harness
  built from the 4.5.2 tree (negative control):
  - **Materials pairwise ≥ 5 dB:** closest pair Bronze ↔ Cast Iron 6.3 dB (both
    seeds; 5.6 on a third). Others 8.2–19.8 dB. 4.5.2: Aluminum ↔ Cast Iron 1.5.
  - **Held sub layer, 2 s → 10 s**, read in 50–90 Hz (the layer's own hum partial;
    −95 dB without the layer): falls **12.3 dB at damping 1** and 9.1 dB at the
    default damping 0.7 — at every Bloom amount, equal to the Bloom-0 row to
    0.1 dB. 4.5.2: 0.0 dB at every Bloom > 0. The brief's "≥ 12 dB" is the hum-stage
    law's figure at damping 1; at default damping the law gives 9.1 with or
    without Bloom, so the default row is gated against the Bloom-0 reference
    rather than by bending the damping law to reach 12.
  - **Unison prime peaks at 50 c:** −25.8 / +25.4; −49.5 / +0.8 / +49.7;
    −37.6 / −12.5 / +12.4 / +36.8 (4.5.2: +25.4 only; +0.8 / +49.7 / +56.1).
  - **Bit-identity vs 4.5.2**, five configs the fixes cannot reach (defaults;
    Bronze + Bloom; Cast Iron; Humanize + Damping; Sub with Bloom 0): sha256
    equal. Constants were recorded from the 4.5.2 build, not this one.
- Random-parameter median 11.3 tap / 10.2 held — inside ±1 dB of the v4.5.1
  anchor, which therefore stays.
- `report.py` **re-anchored to v4.6.0** (the sound changed on purpose; held p10
  left the ±1 dB window at +1.1). tap self-noise / median / p10 / min 2.5 / 13.3 /
  8.0 / 3.5, held 2.4 / 14.7 / 9.3 / 3.6. Self-noise fell ~0.8 dB because the
  stray scatter on unison voice 0 is gone. The v4.5.1 rows are kept in the file's
  comment. Nearest-neighbour problem is essentially unmoved (min 3.5 / 3.6) —
  as the brief predicted, that is Steps 3–5.
- `auval -v aumu OBls OuDv`: AU VALIDATION SUCCEEDED, component version 4.6.0.
  Installed VST3 carries 0 `ForTesting` symbols.
- Not yet auditioned in a DAW.
- Regression baseline: `backups/O-Bells/v4.5.2/`.

## [4.5.2] - 2026-09-20

Step 1 of `improvements/preset-differentiation-v4.6-v4.8.md`: an in-repo,
processor-level offline render harness and the v4.5.1 baseline report. PATCH:
test infrastructure only. No parameter, range, type, state format or audio path
changed; the shipped binary differs from 4.5.1 by its version stamp and the
footer label.

### Added

- **`tests/render-harness/` — `O-Bells-render-test`** (behind
  `-DOUARICON_BUILD_TESTS=ON`). Constructs the real `OBellsAudioProcessor`,
  recalls a factory preset through the real preset manager, plays one note over
  MIDI and writes float32. Built through `ouaricon_add_processor_console` (the
  param-dump builder): `JUCE_WEB_BROWSER=0`, no editor TU, and every
  `JucePlugin_*` macro — `JucePlugin_VersionString` included — derived from the
  plugin target, so it tracks `VERSION` with no mirrored literal. Chorus / delay
  / reverb / EQ are bypassed and High Fidelity is on, so the render is the voice
  through the limiter and one-pole LP only (`--fx` renders the full chain).
- **`report.py`** — every factory preset, tap (0.25 s) + held (6 s) at C4:
  pairwise distance, self-noise, T40, centroid, nearest neighbour; exits 1 when
  a baseline quantity moves more than 1 dB. **`probes.py`** — the RC-1…RC-4
  measurements (material, sub-layer decay under bloom, damping law, 60 random
  parameter points), gated on the random-point median.
- **Test-only RNG seed hook** — `BellVoice::setRandomSeedForTesting`,
  `OBellsAudioProcessor::setVoiceSeedsForTesting`, both inside
  `#if OBELLS_TEST_HOOKS`, which only the harness target defines. The plugin
  never compiles them (0 `ForTesting` symbols in the installed VST3). Seeds are
  splitmix64-mixed: `juce::Random` is a 48-bit LCG and adjacent integer seeds
  give near-identical first draws.

### Findings while reproducing the baseline

- **One shared seed per run is common random numbers.** The first cut seeded
  every job identically; the per-note randomisation then cancelled BETWEEN
  presets and Deep Bronze Tower <-> Grand Cathedral Bell read 2.8 dB — below the
  3.9 dB self-noise of the same preset. Same-preset voice-only vs processor
  renders were indistinguishable (distance = self-noise), so the processor path
  was never the difference. Each job now draws from its own stream.
- **The brief's held `pair min` (4.1 dB) is not reproducible, including by the
  scratch harness that produced it** (re-runs: 2.7 / 2.8 / 3.5). It was one
  clock-seeded draw of an extreme statistic. This harness gives 2.7–3.5, mean
  3.2, over 8 seeds; the gate anchors that cell at 3.2 and the brief is
  annotated. Every other cell is the brief's value.

### Testing

- `report.py`: BASELINE GATE PASS. tap self-noise / median / p10 / min
  2.8 / 12.9 / 7.7 / 3.7 (brief 3.4 / 13.0 / 7.4 / 3.7); held 2.9 / 13.9 / 8.3 /
  3.1 (brief 3.2 / 14.1 / 8.2 / 4.1 -> 3.2, see above). Same seed re-renders
  bit-identically. 8-seed spread: median sd 0.2, p10 sd 0.2, min sd 0.3 dB.
- `probes.py`: RANGE GATE PASS — random median 11.4 tap / 10.1 held (brief 11.5 /
  10.3). Materials idx 1/2/3 sit 1.6 / 3.2 / 1.5 dB from Cast Iron (self-noise
  2.4); sub layer under bloom 0.05 holds −29…−35 dB over 12 s; damping 0 / 0.5 /
  1 -> tap T40 9.4 / 6.0 / 2.2 s.
- `auval -v aumu OBls OuDv`: PASS, component version 4.5.2.
- Regression baseline: `backups/O-Bells/v4.5.1/`.

## [4.5.1] - 2026-09-20

Visual polish. PATCH: CSS and markup only. No parameter, range, type or state
format changed, and no audio path was touched.

### Changed

- **The two shells are 3x larger, fainter and spread apart.** `snail.png` stacks
  both shells in one 472x876 plate, which v4.5.0 drew once, 427px tall, against
  the right edge at 0.25 opacity. The plate is now drawn twice at 213.75% of the
  frame height and `clip-path` keeps one shell per copy, so the upper shell
  bleeds off the top-right corner and the lower shell off the bottom-left with
  no second asset. Opacity 0.25 -> 0.11 (tuning tab 0.18 -> 0.07). The second
  copy is `alt="" aria-hidden`, so `alt.snail` is still announced once. The
  tuning tab no longer slides the overlay sideways; it only dims it.
- **Value readouts are 11px black** (were 9px `#5C4033`) — `.param-value`,
  `#effects-tab .knob-value` and the double-click edit field. Both readout rules
  gained an explicit `line-height: 1.0909` (12px box), which also closes the
  `line-height: normal` finding on the 16 effect readouts.
- **Slider readouts sit 8px under the track** (was 4px). The thumb hangs 5px
  below the 8px track, so at 4px the larger readout touched it; 8px leaves a 3px
  gap. The footer Gain readout is a row layout and keeps its 4px.

### Fixed

- **Save / Load and the Strike Type / Velocity Curve buttons centre their
  captions.** Root cause: all five are `<div>`s whose width is pinned or
  flex-stretched WIDER than the caption (the v4.2.0 geometry pins), and a div
  does not centre text the way a `<button>` does — so the French-sized slack all
  landed on the right. `text-align: center` on `.preset-action-btn` and
  `.choice-button`.
- **The footer version label read v4.3.2** through two releases; now v4.5.1.

### Testing

- `check-ui-labels --plugin O-Bells`: ALL CHECKS PASSED on the en / fr / zh-Hans
  arms (0 moved elements, no new label intersections).
- Centring probe over every visible button on all three tabs: caption centre
  within 1px of box centre on every button.
- Regression baseline: `backups/O-Bells/v4.5.0/`.

## [4.5.0] - 2026-09-05

Simplified Chinese joins English and French (task 260905-rwh, Stage 4 wave 4c).
MINOR: a third language, a repaired render gate, 26 line-height pins and two
width pins. No parameter, range, type or state format changed, and no audio path
was touched.

### Added

- **`zh-Hans` on every one of the 255 emitter rows** — 66 `I18N` entries (title
  and body) and 123 `LABELS` entries, the largest table in the wave. Authored at
  `reviewed: 'mt'`, committed there, and promoted to **`reviewed: 'bt'`** only
  after two independent blind reverse reads. `'native'` stays open on every row:
  this project has no native Chinese reader, and `i18n-zh-lint` rule R1 prints
  that quality level on every run rather than leaving it to a reader's memory.
- **The endonym `简体中文`**, written as numeric character references to match
  this file's existing convention for `Français`.
- **A three-way `languageCode` / `languageIndex` codec.** Anything that is
  neither `fr` nor `zh-Hans` degrades to English. **No Chinese character appears
  anywhere under `Source/`** — the C++ carries only the ASCII code.

### Changed

- **Two hover-help bodies lost an enumeration, in English and French alike.** The
  gear body counted the settings popover's controls and named the only one it
  found, which stopped being true when v4.4.0 added the hover-help switch —
  `check-i18n` assertion [16] *requires* that switch, which is precisely what made
  the clause false. The language body named the selector's options twice, once in
  prose and once as a trailing range clause. Both are deletions, not extensions,
  and the Chinese was authored from the corrected English after a re-read.
- **The tuning panel's 32 form-4 nodes now name a face.** A form control does not
  inherit `font-family`; it takes the UA stylesheet's, which here is bare Arial —
  a face with no Han glyphs. 13 `<button>`, 17 `<input>` and 2 `<select>` would
  have had their Chinese captions resolved by the document-language fallback
  instead of by anything the page names, and **no grep of any file in this repo
  can see that**, because the defect is an *absent* declaration. A computed-style
  census found all 32. Arial stays **first** so the Latin metrics of the English
  and French arms do not move. `index.html` already carried `font-family: inherit`
  at six sites; those are the main page's controls, they were read first, and they
  reach none of the 32 — a plugin given this repair on some of its controls is not
  a plugin that has had it.
- **`css/tuning-panel.css`'s bare `monospace` now names Menlo and carries the CJK
  tail.** It named no face at all, so under `zh-Hans` even the cents readouts'
  digits were resolved through the document language's mono face — a readout
  changing metrics with no translated string anywhere near it.
- **The CJK tail on the three declared stacks, placed BEFORE the trailing
  generic.** Chromium resolves a bare generic against the document's `lang`, so
  under `zh-Hans` the generic is already a Chinese face and a tail written after
  it is never consulted — and the page still looks right on macOS, which is what
  makes that mistake survive review.
- **26 `line-height` pins.** `line-height: normal` is not a number — it is
  whatever the resolved face reports, and the resolved face changes with the
  document language. 584 visible leaves resolve `normal` on this page, ten times
  the wave's median; 584 is the upper bound, not the worklist, because only
  Han-carrying leaves reach the screen and only some of those move. Each pin is
  derived from that leaf's own measured English box, unitless, never global.
- **Two width pins, both FLOORS, because the Chinese is NARROWER** — the half a
  clip check is blind to. `.footer-gain-label` (en 26.50, fr 26.50, zh 20.41) is a
  flex item in a `flex-shrink: 0` row, so 6.09 px lost there shrank the gain
  block, slid the slider and its readout, and dragged the whole 380 px keyboard
  block after it: 44 elements moved in all 14 states, from one caption. The
  rotation table's corner header (en 30.56, fr 30.56, zh 27.36) additionally takes
  `nowrap`: its two-character Chinese broke *between* the characters — a
  min-content width of one glyph in an auto-sized column — which grew the header
  row and reported all thirteen ASCII note-name headers beside it as wrap defects.
- **`tests/ui_tip_render_check.js` derives its language list at BOTH of its two
  language sites.** The joined-pair assertion was the loud half; the two
  hand-written sweep calls were the silent one, and no repo-wide census can see
  them because a call site spells no list to grep for. Guarded so a list that goes
  empty makes the gate exit non-zero rather than sweep nothing and report green.

### Deliberately unchanged, and recorded as checked

- **The two late tip bindings** (`#ref-pitch-knob`, `#octave-stretch`) are pinned
  by this gate's own `EXPECTED_LATE` assertion, which documents them as designed
  behaviour. They are the suite's `boot-all-uis` census control; a run reporting
  zero would mean the census had gone blind, not that anything had been fixed.
- **`js/tuning-panel.js`** is this plugin's own copy, not the shared
  `scala-tuning-engine` module, and already carries 45 `data-i18n` hooks. Its
  captions are part of the 255 rows and the file itself needed no edit.
- **One glossary collision, screened and ruled out on evidence.** 阻尼 is reached
  by both `label.damping` and `label.damp`, but the two controls live in mutually
  exclusive tab panels and can never be on screen together. Qualifying both sides
  would have made the reverb caption the widest in a narrow knob row — a geometry
  defect introduced to solve a collision that cannot occur.

### Verified

- `check-i18n` exit 0; `check-ui-labels` exit 0 with **0 FAIL on the English,
  French and Chinese arms across all 14 states**; `i18n-zh-lint` 0 findings and
  `BELOW SHIP BAR 0`; `i18n-fr-lint` exit 0 — French untouched.
- `measure-ui --mode box --report all` over 3219 measured nodes: `undeclared-font`
  **196 → 0**, `wrap-count` 0, `svg-font-attr` 0. `line-height-normal` reads 19,
  every one an evidenced non-mover whose English box equals its Chinese box.
- Zero Han under `Source/**/*.{h,cpp}`, positive control fired on the same run.
- The blind reverse read ran twice against **two different models**, each from a
  fresh session outside the repo with no tool access and repo access forbidden in
  the prompt, on separately salted batches and in chunks of 85 rows. Round one
  caught one caption whose Chinese named the wrong control on a tab where both
  were visible; round two, independently, read the correction back as the
  tooltip's own title.
- `auval -v` AU VALIDATION SUCCEEDED, triple read off `auval -a`.

## [4.4.1] - 2026-09-03

The French rendering of the hover-help surface changes suite-wide (task
260903-ukp; O-Gain 1.3.3 was the tracer). PATCH: French strings and source
comments only — no parameter, range, type or state format changed.

### Changed

- **The French caption is now `Infobulles`** (feminine plural). The superseded
  rendering named the ACTION — help on hover; *infobulle* is the noun French
  DAW and OS interfaces use for the surface itself. The glossary root moved
  with it, ROOT-ONLY: `scripts/i18n-fr-glossary.js` now reads
  `'hover help': ['infobulles']` and
  `'toggle hover help': ['activer ou désactiver les infobulles']`, with the old
  rendering REMOVED rather than kept as an accepted alternate — so a plugin
  drifting back is a red G1 gate, not a silent pass.
- **Every sentence re-agreed from feminine singular to feminine plural**, not
  substituted: `cette …` → `ces infobulles`, `l’…` → `les infobulles`,
  `de l’…` → `des infobulles`, `toute l’…` → `toutes les infobulles`,
  `Une fois désactivée` → `Une fois désactivées`; the distributive `chaque …`
  → `chaque infobulle` is the one place the new term stays singular.
  Bare back-references that carried no occurrence of the old phrase — clauses
  reading *le réglage de l’aide*, *l’état de l’aide*, *son affichage ou non*,
  and the pronouns in *Lorsqu’elle est désactivée … la réactiver* — were
  rewritten too. A regex pass would have left every one of them pointing at an
  antecedent that no longer exists.
- Every changed body was read by the developer at a blocking checkpoint
  *before* it was written, so each ships `reviewed: true` legitimately and the
  repo-wide unreviewed-French TOTAL stays at 0.


## [4.4.0] - 2026-09-03

A switch for the hover help. The tooltip layer this plugin already had could
not be turned off; twenty of the suite's forty-three plugins already carried
that switch and twenty-three did not. This closes one of the twenty-three.

### Added

- **A hover-help switch in the settings popover.** `#tips-toggle`, a second
  `.settings-row` under the language selector, on the newer `#tips-toggle` /
  `label.hoverHelp` convention rather than the older `#help-toggle` spelling.
  It gates the tooltip renderer's own `show()` — a delegated renderer has no
  bindings to unbind — and persists under the localStorage key
  `obells.tipsEnabled`.
- **`data-tip-always` on `#gear-btn` and on `#tips-toggle`, and on nothing
  else.** Those two controls are the ones that REACH and RESTORE the help
  layer, so they keep explaining themselves while it is off. `#lang-select`
  deliberately does not carry it: it is only reachable through the gear, which
  already explained itself on the way in.
- **Five i18n keys, four of them settled roots copied rather than authored.**
  `label.hoverHelp`, `ui.on`, `ui.off` and `aria.helpToggle` take the French
  glossary roots verbatim from `scripts/i18n-fr-glossary.js` — *Aide au survol
  / Marche / Arrêt / Activer ou désactiver l'aide au survol*. The fifth,
  `tip.tipsToggle`, is the tooltip's own title and body.

### Measured

- **The second row costs nothing.** With the popover forced open it occupies **y 47..112, 200 x 65 px — byte-identical in English and French** — inside an 800 x 600 frame. The switch face measures **42.00 x 18.00 px in both languages**.
- **The switch's face is a `min-width: 42px` floor, not a pinned width**, so a
  longer French face grows LEFTWARD into slack the popover already has. The row
  is `space-between` and the button is a `[data-i18n]` node, so nothing the
  geometry gate measures moves. `check-ui-labels` [7] reports **0 non-label
  elements displaced** between English and French, and the visible element set
  identical in both.
- Every declaration in `.settings-toggle` above the four switch-specific ones
  is **copied from this page's own language `<select>`** — its font stack, ink,
  plate, hairline and radius — so the two controls in the popover match by
  construction rather than by a second designer re-deciding them.

### Decided

- **Default is ON.** The previous version showed hover help unconditionally, so
  ON is the setting that leaves an existing user's plugin behaving exactly as
  it did. Default OFF would additionally have made `boot-all-uis --strict-tips`
  measure an empty tip surface and call it correct.

### Also driven

- `tests/ui_tip_render_check.js` [1c] **replaces a proxy with the property it
  stood for.** Until this version there was exactly one `.settings-row`, and the
  gate asserted that count as a stand-in for *closest() is right by construction,
  not by luck*. There are two rows now. The count is no longer the question: the
  binding resolves `#lang-select` by its unique id and then walks ANCESTORS with
  `closest()`, which cannot reach a sibling row however many exist. What must
  hold — and is now asserted directly — is that the resolved wrapper contains
  `#lang-select` and does NOT contain `#tips-toggle`.


## [4.3.2] - 2026-08-31

Defects found by reading the French against the code. Stage O of the repo-wide i18n rollout.

### Fixed

- **item 66 — Partial Tune tooltip:** the body said the control "detunes the
  upper partials". `BellVoice.cpp` `calculatePartialFrequency()` scales exactly
  one ratio — `partialIndex == 2`, the tierce (2.4× in `bellRatios`) — by
  `2^(cents/1200)`; the hum (0.5×), the prime (1×) and partials 3–7 never move.
  The body now names the third partial and says the others stay put, in both
  languages. Tip height at the 260 px cap: 103.2 → 153.2 px (en), 103.2 →
  153.2 px (fr); bottom clearance 96.6 → 46.6 px, still inside the frame.
- **item 66 — Damping tooltip:** the body said damping sets "how quickly the
  bell's partials give up their energy". The live reach of `damping` is two
  places: `updateMultiStageCoefficients()` — the hum-stage coefficients of
  partials 0–1 only (`humDecayTime *= 1 + (1 − 0.8·d)·2`, ×3 at 0 → ×1.4 at 1)
  — and `stopNote()`'s release (`jmap(damping, 3.0 s, 0.5 s)`). Strike- and
  body-stage decay of every partial, and hum-stage decay of partials 2–7, come
  from Strike Time, Body Time, Hum Sustain, Material and Acoustic Brightness.
  The body now says exactly that and names the two release endpoints, in both
  languages. Tip height: 103.2 → 153.2 px (en), 103.2 → 186.5 px (fr); bottom
  clearance 289.6 → 239.6 / 206.3 px.
- **`ModalPartial::decayRate` removed** (`BellVoice.h`, the write in
  `BellVoice.cpp` `startNote()`): computed from
  `jmap(damping, 0.5 s, 5.0 s) × DECAY_MULTIPLIERS × material × acoustic
  brightness × noteVariationDecay` and never read anywhere — the live decay is
  the three-stage coefficient table. Its damping mapping was the inverse of
  both live paths (higher damping = longer decay), a trap for the next reader.
  DSP-neutral by construction and proven: a scratchpad offline render of 75
  cases (notes 48/60/72 × damping 0/0.25/0.5/0.75/1 × Partial Tune
  −100/−30/0/+30/+100 ct, 4.5 s each at 48 kHz, RNG seed pinned) hashes to the
  same SHA-256 before and after (`55a76506…d7fe` over 129 600 000 bytes; a
  different seed gives a different digest, so the probe can fail).
  `noteVariationDecay` is still drawn in `startNote()` so the per-voice RNG
  sequence is unchanged; it now feeds nothing.
- Console banner read `v4.2.0` since 4.2.0; it now reads the shipped version.

The two rewritten French bodies are `reviewed: false` again for the developer
to re-read; the other 184 entries keep `reviewed: true`.

## [4.3.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed

- **72 French entries revised** against the suite glossary
  (`scripts/i18n-fr-glossary.js`) and its lint: 29 terminology, 56 typography,
  4 meaning, 2 idiom and register, 0 grammar or agreement. The visible ones are
  MAILLOCHE → **MAILLET** (a *mailloche* beats a bass drum, not a bell), DOSAGE
  → **MIX** on the three FX mix knobs and **QUANTITÉ** on the five Amount
  captions, RÉVERBE → **RÉVERB** (*réverbe* is not a word), TENUE BOURDON →
  **MAINTIEN BOURDON**, and the Tuning tab from GAMME to **ACCORD** — a *gamme*
  is a scale, and the library on that tab is full of them.
- **French typography throughout the tooltips**: 69 no-break spaces added
  before `%`, before a colon and before `; ! ?`, and between every number and
  its unit, so a range like `100 ms à 10,0 s` can no longer break across a line
  in the middle of a measurement.
- Two tooltips said something the English did not, and now do not: High
  Fidelity's body had dropped the *fidelity* it is named for, and the EQ Mid
  body called its peaking band *une cloche* on a plugin whose every other
  sentence uses that word for the instrument.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.

All French is still flagged `reviewed: false`: this pass is a second machine
reading against a glossary and a lint, not a native speaker's.

## [4.3.0] - 2026-08-31

### Added — hover-help, in both languages

Every control on the page now has a hover tooltip: a title and two or three
sentences saying what it does, when to reach for it, and the range and unit it
covers. **65 entries — 63 parameters plus the gear button and the language
selector — in English and French**, switching with the rest of the page.

**This needed a RENDERER as well as copy, and that is the whole point.** canon
v2's `applyI18n()` writes `data-tip-title` and `data-tip` attributes onto the
anchors named in `TIP_BINDINGS`, and stops there. The thing that reads them and
paints a surface is per-plugin code, and at v4.2.0 this plugin had none — no
`#tooltip` element, no `.tooltip` rule, no hover handler. Authoring the copy
alone would have shipped 65 invisible strings and three green gates.

- `index.html` — the `#tooltip` surface, its CSS in this page's own parchment
  vocabulary, and `setupTooltips()`, ported from O-simpleFM's delegated
  cursor-following renderer and called after `initI18n()` inside the same
  `try/catch`.
- `Resources/ui/js/i18n.js` — the 65 `I18N` entries and the 65 `TIP_BINDINGS`
  rows. All French is a machine draft, `reviewed: false` on all 65.
- `tests/ui_tip_render_check.js` — **the first runnable gate this plugin has.**
  1024 assertions, 0 failures. No existing gate can see a rendered tooltip:
  `check-i18n` reads the table statically, `check-ui-labels` has no tooltip
  awareness at all, and `boot-all-uis` counts `aria-label` and `title` and never
  `data-tip`. Both of those were measured here rather than assumed — the label
  gate's 14-state output is identical before and after except for one retired
  label, and `boot-all-uis` reads `text=147 aria=19 title=0` on both sides.

### Changed

- The High Fidelity switch's bespoke `:hover`-only note is **gone**. It was a
  second hover surface at `z-index: 100` and would have painted alongside the
  new tooltip on the same hover. Its sentence moved **verbatim, in both
  languages**, into the High Fidelity tooltip's body — no new prose invented,
  the same rule contract §4 applies to a native `title=`.

### Fixed

- Two defects found by the gate rather than by reading, both in the renderer as
  first written:
  - a drag that strayed out of the cell being turned opened the **neighbouring**
    control's tip over the one under the user's hand. None of this page's
    sliders, FX knobs or the A4 knob calls `setPointerCapture`, so every
    boundary event during a drag lands on whatever is under the cursor. Fixed
    with a `pointerHeld` flag cleared on `pointerup`/`pointercancel`.
  - double-clicking an FX knob's readout to type a value left a tooltip parked
    over the input. The first guard sat in `focusin` and passed every static
    check while failing the behavioural one: replacing the readout's text node
    with an `<input>` mutates the DOM under a stationary cursor and Chromium
    dispatches a fresh `pointerover`, which the focus latch has nothing to say
    about. The guard now sits in `show()`, where it covers every entry path.

### Known — reported, not fixed

- **Two parameters are host-reachable and page-unreachable.**
  `tuning_pitchBendRange` and `tuning_temperamentPreset` have no control
  anywhere in the served page, so they get no tooltip. Both are automatable and
  both reach the tuning engine. `tuning_temperamentPreset` has a complete
  native-function bridge on the C++ side (`setTemperamentPreset` /
  `getTemperamentPreset`) that nothing on the page ever calls. No control was
  added to satisfy a count — that is a feature change with a geometry cost.
- The two tuning-panel tooltips (**A4 Reference**, **Octave Stretch**) log one
  `i18n: tip target not found` warning each at load, because the panel is
  imported lazily and is not in the DOM when the first sweep runs. They bind on
  the panel's own re-apply immediately afterwards. The gate pins the warning set
  to exactly those two rather than relaxing the check.

## [4.2.0] - 2026-08-29

### Added — the PAGE speaks French

The whole interface is now bilingual (English / Français), selectable from a new
gear popover in the preset bar and remembered with the session. **122 label
entries over 123 keyed elements and 19 keyed accessible names.** No hover-help
copy: v4.1.5 had none, and authoring it is a separate piece of work.

- `Resources/ui/js/i18n.js` — the table, embedded in `CMakeLists.txt` and served
  from a `getResource()` branch **in this same commit**. A file that is one
  without the other 404s at runtime and presents as a dead panel with no other
  symptom.
- The canon v2 `applyI18n` / `setLabel` / `initI18n` block, byte-identical to
  `scripts/i18n-canon.js`, at the top of the inline module in `index.html`, with
  `initI18n()` called LAST so the first sweep sees the sixteen effects knobs and
  the popover, which do not exist until then.
- The C++ language pair (`getUiLanguage` / `setUiLanguage`) and persistence as a
  non-parameter property on the APVTS state tree — deliberately not an
  `AudioParameterChoice`, so it cannot appear in an automation lane and no preset
  can change which language somebody reads their plugin in. Read back with an
  `isVoid()` guard, because the XML round-trip rebuilds every property as a
  string `var` regardless of what was written.

### Added — the Tuning tab speaks it too

`Resources/ui/js/tuning-panel.js` is a **plugin-owned copy**, 279 lines diverged
from `modules/tuning/scala-tuning-engine/js/tuning-panel.js`, and O-Bells has no
`dependencies.json` listing the module, so `/module-upgrade` will not revert
these edits. Its 34 captions are localized and its four rebuilt subtrees
re-sweep through a new `window.__reapplyI18n()`. This deliberately widens an
already-large divergence from the module; the alternative was a page whose
Tuning tab stayed English while every other tab spoke French. The module file
itself is untouched.

`scripts/i18n-extract.js` skips that filename unconditionally, so none of those
34 strings came from a worklist — they were enumerated by hand and verified by
driving the panel in the browser rather than by trusting a static check.

### Fixed — pre-existing, in ENGLISH

- **"True Keys" wrapped to two lines inside its own button**, making the tuning
  panel's visualisation row 10px taller than the other four states and pushing
  everything below it down. Present at v4.1.5 with no French anywhere on the
  page: measured on the pre-image at 2 line boxes and a 34px row, against 1 line
  and 24px now. `.viz-btn` is `flex: 1`, which carries `min-width: auto`, so each
  button was floored by its longest WORD; `white-space: nowrap` with 4px of
  horizontal padding gives every caption a 45.6px box that fits both languages.
- **The version label in the header read `v4.0.0`** while the plugin shipped
  4.1.5.

### Changed — geometry

Twelve pins and one restructure, each reverted alone and confirmed to re-break
`check-ui-labels` assertion 7. French moved captions in BOTH directions here —
`MIX -> DOSAGE` is +21.02px and `FEEDBACK -> RÉINJ.` is −21.75px on adjacent
knobs — so a clip-only check would have certified the whole Effects tab.

- `.preset-action-btn` pinned to 62px (SAVE/LOAD are shorter than ENREG./OUVRIR,
  and `.preset-browser` is the right-hand child of a `space-between` header, so
  any width change walked five elements sideways).
- `.choice-group` given `width: 100%` so its three `flex: 1` buttons split a
  fixed total.
- `.lp-filter-toggle` 98px and `.hi-fi-toggle` 136px — shrink-wrapped chips whose
  captions grow.
- `.fx-title` 62px, `.fx-bypass-btn` 48px, `#effects-tab .knob-container` 62px
  and `.knob-label { width: 100% }` — `.fx-section` floats the knob row between
  two auto margins, so it moved by HALF of any width change on either side.
- `.tonic-label` and `.octave-stretch-label` given fixed flex bases, because the
  slider and the readout beside each of them start where the caption ends.
- The CPU-warning banner's live estimate moved to the FRONT of the sentence. No
  word choice can hold an interpolated number at a fixed x while the words in
  front of it change length; putting the readout first makes its position
  language-invariant and keeps its own bold styling.
- `.tonic-selector`'s gap went 8px → 5px. Its negative control PASSES, so it is
  labelled a design guard rather than claimed as a French fix: what it buys is
  the room the 42px caption pin costs, without which the row overflows its
  column by 7.82px identically in both languages.

### Added — test artifacts

- `tests/i18n-states.json` — 14 states driving the Tuning and Effects tabs, the
  three generator forms, all five visualisations and the settings popover.
- `tests/ui-stub/generic-overrides.json` — the headless stub infers a native
  function's return shape from its NAME, and that guess emptied the Tuning tab:
  `getOctaveStretch` returned `null` and threw out of `loadInitialState()` before
  three of its four update calls ran, `getEmbeddedTuningList` returned an array
  where the page does `JSON.parse()`, and `getTonicNote` rendered the string
  `undefined`. With the plugin's real defaults supplied, coverage went from 118
  to 123 measured elements.

All French is a machine draft: 122 of 122 entries `reviewed: false`. No native
speaker has read it.

## [4.1.5] - 2026-08-02

### Fixed

- **EQ blows up to Inf/NaN on any EQ parameter change.** Same bug fixed in O-IntonationPad
  v2.8.4: the CR-02 RT-safe coefficient update copied the **6 raw** values returned by
  `IIR::ArrayCoefficients::makeXXX` (`{b0,b1,b2,a0,a1,a2}`) over
  `Coefficients::getRawCoefficients()`, but `IIR::Coefficients` stores **5 normalised**
  values (each divided by a0, a0 dropped) — the feedback polynomial was mis-aligned and the
  shelf/peak filters went unstable to Inf on the first gain/freq change. Normal playback was
  unaffected (coefficients from `prepare()` are correct until the first update). Fix: assign
  through `Coefficients::operator=(std::array)`, which normalises by a0; still RT-safe —
  `prepare()` now also assigns via the array form so the coefficient Array's ≥8-slot storage
  is reserved up front and the audio-thread assignment never allocates.
  Verified with pluginval strictness-10 (Automation + Fuzz parameters clean).

## [4.1.4] - 2026-08-02

Licensing-only release. No audio, parameter, state, or UI behavior changes — output is
byte-for-byte identical to 4.1.3. Presets and saved sessions are unaffected.

### Changed — licensing

- **Relicensed under AGPL-3.0-or-later.** The repository elected the GNU Affero General
  Public License v3 for its JUCE dual-license obligation (rather than the free Starter
  tier), and this release is the first O-Bells build distributed under it.
- **SPDX notice headers added to all 25 Ouaricon-authored O-Bells sources** — C++
  (`Source/`, `Source/DSP/`) and WebView assets (`Resources/ui/` HTML, CSS, JS) — each
  carrying `SPDX-License-Identifier: AGPL-3.0-or-later` and the standard AGPL notice.

## [4.1.3] - 2026-07-21

Unblocks the first cross-platform release. The 4.1.2 tag built on Windows but the
newly-added CI **pluginval strictness-10** gate (which never ran before v4.0.0) failed
with `NaNs found in buffer` during the parameter-automation/fuzz sweep — a latent DSP
defect present in every prior release, exposed only now. No audio change under normal use.

### Fixed — NaN in the output buffer (pluginval strictness-10)

- **Strike-resonator cutoff exceeded Nyquist.** `calculatePartialFrequency` returns
  `fundamental × ratio` with no upper bound; a high note × partial ratio × inharmonicity
  pushes it past Nyquist, and the bandpass `StateVariableTPTFilter` maps cutoff through
  `g = tan(π·fc/fs)` → Inf/NaN as fc → fs/2, latching the resonator for the whole note.
  Now clamped to `[20 Hz, fs·0.49]`. (pattern_biquad_nan_guard_sticky_silence)
- **Air-absorption filter could divide 0/0.** A 0 air-absorption time made
  `elapsedSamples / totalSamples` NaN (`jlimit` does not sanitize NaN), latching the
  two-pole state. Divisor floored at 1 sample.
- **Voice + output NaN backstops.** The summed voice output is flushed to 0 if non-finite
  before it reaches the stateful air filter and the shared FX bus (whose delay/reverb
  feedback would otherwise latch a NaN indefinitely); the final soft-limiter flushes any
  remaining non-finite sample so the host never receives NaN/Inf. Normal finite audio is
  untouched.

## [4.1.2] - 2026-07-21

Windows-build fix ahead of cross-platform publishing. No audio, parameter, state,
or UI behavior changes — mac/AU output is byte-for-byte identical to 4.1.1.

### Fixed — Windows compile blocker

- **All 7 `FileChooser::launchAsync` completions hoisted `SafePointer(this)` to a
  local before the lambda.** The v4.1.1 CR-03 fix init-captured
  `safeThis = juce::Component::SafePointer<…>(this)` *inside* the async completion
  lambda, which itself lives inside a `withNativeFunction([this]…)` outer lambda.
  Apple Clang binds `this` to the editor there, but MSVC binds it to the enclosing
  closure — a Windows-only compile failure invisible on macOS. Each site now declares
  `juce::Component::SafePointer<OBellsAudioProcessorEditor> safeThis(this);` in the
  outer lambda (where `this` is unambiguous) and captures it by copy. Teardown
  semantics (bare `return` on null, never calling `complete()`) are unchanged.
  Sites: savePresetWithDialog, loadPresetFromFile, loadScalaFile, saveScalaFile,
  loadKBMFile, saveKBMFile, exportTuningHTML.
  (critical_msvc_safepointer_init_capture_nested_lambda)

### Fixed — Plugin binary reported version 1.0.0

- **`juce_add_plugin` used `PLUGIN_VERSION`, which JUCE does not recognize.** JUCE's
  only version keyword is `VERSION`; with none set it falls back to `PROJECT_VERSION`
  (`project(JUCEPlugins VERSION 1.0.0)`), so **every** prior O-Bells release (1.x–4.1.1)
  shipped VST3/AU bundles stamped 1.0.0 — DAWs saw no version metadata and could miss
  update detection. Changed to `VERSION 4.1.2`; the AU/VST3 now report 4.1.2 (AU
  component version `0x0040102`). No code path relied on the old key — the registry
  version scanner already falls back to `VERSION`.
  (Latent in siblings O-Marimba, O-MicrotonalSampler, O-Reed — same wrong keyword.)

## [4.1.1] - 2026-07-08

Resolves the deep code-review findings in `CODE_REVIEW.md` (3 critical, 12 warning,
13 info). Bug-fix release — no parameter IDs, ranges, or state format changed.

### Fixed — Critical

- **CR-01: Factory preset library was effectively broken.** Factory values were authored
  in engineering units but applied through JUCE's *normalized* `setValueNotifyingHost`,
  so every non-[0,1]-ranged param (airAbsorptionTime, unisonDetune, strikeTime, brilliance,
  bodyTime, humSustain, lpFilterCutoff, pitchEnvTime, partialTuning, material) recalled
  slammed to a rail — the ~25 curated presets collapsed toward identical maxed-out timbres.
  Root cause: no `convertTo0to1` anywhere. Fix: `initializeFactoryPresets` now stores
  `range.convertTo0to1(value)` (identity for [0,1] params) so the on-disk convention matches
  user presets; `unisonCount` (the one param authored normalized against a [1,4] range) was
  corrected to engineering units. A `.factory_version` sentinel forces regeneration of the
  cached (broken) JSON on upgrade. (pattern_factory_preset_normalized_ignores_skew)
- **CR-02: EQ recomputed IIR coefficients with heap-allocating factories on the audio thread.**
  `EQProcessor::process` called `Coefficients::makeXXX` (malloc+free per block) whenever a
  band gain/freq changed — a dropout risk while dragging/automating eqLowGain/eqMidGain/
  eqMidFreq/eqHighGain. Fix: `ArrayCoefficients::makeXXX` (stack `std::array<float,6>`,
  identical math) copied in place into the pre-allocated storage. (pattern_arraycoefficients_rt_safe_iir)
- **CR-03: FileChooser `launchAsync` completions captured raw `this` (7 sites) → UAF.** Closing
  the plugin window / switching tracks while a dialog was open fired the completion against a
  freed editor. Fix: each completion now captures a `Component::SafePointer` and bails with a
  **bare return** on teardown — never calling `complete()` on the null path, which is itself a
  UAF (complete is owned by the destroyed WebBrowserComponent Impl).
  (pattern_webview_launchasync_safepointer_no_complete)

### Fixed — Warnings

- **WR-01:** `applyPresetJson` now resets parameters to their defaults before applying a
  preset's keys, so partial presets no longer inherit stale FX/lpFilter state from the
  previous preset. Global tuning (`tuning_*`) is intentionally preserved across timbre-preset
  loads. (pattern_preset_apply_needs_reset_to_defaults)
- **WR-02:** Preset names are sanitized with `createLegalFileName` before use as a filename;
  a name containing "/" no longer silently fails to save. (critical_preset_name_slash_path_separator)
- **WR-03:** The `material` control was a ComboBox in C++ but a slider in the UI (mismatched
  JUCE channels) — the control was dead and the CPU-decay estimator was stuck at Bronze. Bound
  it with a slider relay/attachment (keeps the knob UI); both now work.
- **WR-04:** `outputGain` dB readout (main + footer) used a ~3× too-steep hand-coded slope
  (showed +36 dB at max vs the true +12). Now reads the scaled dB from the range.
- **WR-05:** `airAbsorptionTime` readout treated the normalized value as seconds (always showed
  ms, ignored skew). Now formats the real seconds.
- **WR-06:** `pitchEnvTime` readout linearly decoded a skew-0.5 range (~2× off mid-range). Now
  uses the scaled value.
- **WR-07:** `eqMidFreq` readout AND double-click-edit linearly decoded a skew-0.5 range
  (showed 4100 Hz where the true center is 2150 Hz; typing 1000 set ~282). Both now go through
  the scaled value / skew-aware inverse.
- **WR-08:** Tuning UI (A4 ref, octave stretch, temperament) drove the TuningEngine via native
  functions but never wrote the APVTS params — so those values were **lost on session reload**
  and didn't automate. Fix (minimal two-way bridge): the native setters now write the APVTS
  param (the existing listener forwards to the engine), and the A4 knob initializes from the
  backend. (Live DAW-automation still doesn't move the knob — see Known Limitations.)
- **WR-09:** The soft limiter ran *before* the effects chain; the EQ's +12 dB shelves could
  clip the output afterward. Added a second safety limiter after the FX chain (the pre-FX stage
  is unchanged, so normal-level material is unaffected).
- **WR-10:** `DelayProcessor` used a fixed 192000-sample max (2.0 s only at 96 kHz) and never
  called `setMaximumDelayInSamples`; a 2.0 s delay overran at 176.4/192 kHz. Now sized per
  sample rate in `prepare()` and clamped in `setTime`.
- **WR-11:** `getTailLengthSeconds()` returned 0 for a multi-second-decay synth + reverb tail
  (hosts could truncate on offline bounce). Now reports 15 s.
- **WR-12:** `ScaleGenerator::generateRank2` clamped the generator against the *un-clamped*
  period. Reordered so the period is clamped first.

### Fixed / Changed — Info

- **IN-01:** Migrated all WebView slider readouts to `SliderState.getScaledValue()` (the
  C++-range-and-skew-aware value) instead of re-deriving engineering units from hardcoded JS
  constants — the drift class behind WR-04..07. (pattern_webview_knob_readout_scaled_value)
- **IN-02:** Added a `getParameterDefaults` native fn and double-click-to-reset on the main
  sliders.
- **IN-03:** A4-REF knob now initializes from the backend (was hardcoded to 440 Hz and drifted
  after state recall); also fixed drag accumulation always restarting at 440.
- **IN-04:** Deleted the dead, never-imported `instrument-footer-panel.{js,css}` (and their
  binary-data entries).
- **IN-05:** Corrected the `tuning-panel.js` docstring to document the `Juce` ES-module
  namespace (not `window.__JUCE__`). (critical_juce_webview_namespace_vs_postmessage)
- **IN-07:** `TuningExporter::calculateETDeviation` guards `totalDegrees <= 0`.
- **IN-08:** `loadScalaFile` rejects a `<= 0` degree count instead of silently truncating.
- **IN-09:** KBM degree clamp uses `scaleSize - 1` so the period can't be selected as a degree.
- **IN-10:** `BellVoice` uses a per-voice `juce::Random` (seeded once) instead of the shared
  non-thread-safe `getSystemRandom()` / `rand()` on the audio thread.
- **IN-12:** Removed dead `ReverbProcessor` members (`prevSize`, `prevDamping`, `tankState[]`).
- **IN-13:** Added defensive `{}` initializers to `BellVoice` decay-coefficient arrays.

### Known Limitations (deferred with rationale)

- **IN-06:** Pitch bends are stored per-note (not per-channel); two simultaneous same-numbered
  notes on different MPE channels share one bend slot. Acceptable for the Dorico per-note
  expression use case; a limit only if true MPE is expected.
- **IN-11:** The air-absorption coefficient is recomputed per-sample per-voice. Correct and
  NaN-safe; left as-is to avoid any tonal change (perf-only).
- **WR-08 residual:** Live DAW automation of `tuning_*` params updates the engine but does not
  move the tuning UI knob (no APVTS→UI push). Persistence and recall work.

### Technical notes

- **Files modified:** `Source/OuariconPresetManager.h`, `Source/PluginProcessor.{h,cpp}`,
  `Source/PluginEditor.{h,cpp}`, `Source/DSP/EQProcessor.cpp`, `Source/DSP/DelayProcessor.{h,cpp}`,
  `Source/DSP/ReverbProcessor.{h,cpp}`, `Source/ScaleGenerator.cpp`, `Source/TuningEngine.cpp`,
  `Source/TuningExporter.cpp`, `Source/BellVoice.{h,cpp}`, `Resources/ui/index.html`,
  `Resources/ui/js/tuning-panel.js`, `CMakeLists.txt`.
- **Files removed:** `Resources/ui/modules/instrument-footer-panel.js`,
  `Resources/ui/css/instrument-footer-panel.css`.
- **Validation:** VST3 + AU build clean; `auval -v aumu OBls OuDv` PASS (render tests through
  192 kHz).
- **Version bump rationale:** PATCH (4.1.0 → 4.1.1) — review-finding fixes; no parameter or
  state-format changes.

## [4.1.0] - 2026-04-26

### Added

- **adds VST3 Note Expression microtonal support for Dorico.** O-Bells now responds to Dorico's per-note tuning messages (`kTuningTypeID` Note Expression events), enabling correct microtonal playback of quarter-tones, third-tones, and arbitrary tuning deltas authored in Dorico's tonality system. End users must set Microtonality to "VST3 Note Expression" on the assigned expression map (see O-Lyrica 2.3.0 for the procedure).
- **Shared `note-expression` module adoption.** O-Bells consumes the Ouaricon module at `modules/tuning/note-expression` (v1.0.0), same shape as O-Lyrica v2.3.0.

### Technical notes

- **Composition with TuningEngine.** `BellVoice::startNote` computes the fundamental via `TuningEngine::getFrequency(midi)` first, then applies the NE semitone delta via `Ouaricon::NoteExpression::applyPendingTuning(table, midi, freq)` before `calculateMultiStageCoefficients()`.
- **Files modified:** `Source/PluginProcessor.{h,cpp}`, `Source/BellVoice.{h,cpp}`, `CMakeLists.txt`.
- **Version bump rationale:** MINOR (4.0.0 → 4.1.0) — new user-visible feature, backward compatible, no preset impact.

## [4.0.0] - 2026-04-13

### Added
- **Effects tab** — Full effects chain with dedicated UI tab, replacing the simple reverb slider
  - **Reverb** — 8-channel FDN reverb with shimmer, ported from O-Lyrica. Controls: Size, Decay, Damping, Shimmer, Mix, Bypass
  - **Delay** — Stereo delay with ping-pong mode. Controls: Time, Feedback, Mix, Ping-Pong toggle, Bypass
  - **EQ** — 3-band parametric EQ (Low Shelf, Peak, High Shelf). Controls: Freq, Gain, Q per band, Bypass
  - **Chorus** — JUCE chorus processor. Controls: Rate, Depth, Mix, Bypass
  - Each effect has independent bypass toggle and SVG vine-arc styled knobs

### Changed
- **Simple reverb slider removed** — Replaced by the full Reverb processor in the Effects tab

### Technical Notes
- Domain: DSP + UI + CMake
- New source files: `DSP/DelayProcessor.cpp/.h`, `DSP/EQProcessor.cpp/.h`, `DSP/ReverbProcessor.cpp/.h`
- Effects chain applied post-voice-mix, pre-output gain
- All effects bypass-safe with no clicks or artifacts
- Preset compatibility: Old `reverbMix` parameter no longer used; presets will load with effects at defaults

## [3.2.1] - 2026-02-19

### Added
- **Licensing module integration** - Compile-flag gated licensing overlay (OUARICON_LICENSING, off by default for local development)
  - Native overlay hides WebView until license is validated
  - License manager lives on processor, persists across editor open/close
- **Version label** - Small version indicator (v3.2.1) in bottom-left corner of UI

### Technical Notes
- Domain: C++ + UI + CMake
- Licensing is fully gated behind `OUARICON_LICENSING` CMake flag — no impact on local builds
- No DSP changes; backward-compatible

## [3.1.1] - 2026-02-05

### Fixed
- **Tuning library missing octave scale degree** - Embedded tunings now correctly include the period (1200 cents for octave-based tunings) when loaded into the TuningEngine
  - Root cause: `loadEmbeddedTuning` passed `tuning->intervals` directly to `setCustomIntervals()` without appending the period, causing `scaleIntervals.back()` to return the wrong value as the octave boundary
  - 12-note-per-octave tunings (Historical, Just Intonation) showed 11 scale degrees instead of 12
  - EDO, World, and Non-Octave tunings were similarly affected
  - Scala file loading was unaffected because `.scl` files include the period as the last pitch line

### Technical Notes
- Domain: C++ (PluginEditor.cpp loadEmbeddedTuning native function)
- No parameter changes; backward-compatible

## [3.1.0] - 2026-02-05

### Added
- **TrueKeys interval reporting** - The True Keys visualization now displays real-time interval analysis between held notes, matching O-Lyrica's implementation
  - Shows note names with interval labels (e.g., `C3 → G3 (P5) 702.0¢`)
  - Calculates intervals from actual TuningEngine frequencies (not scale degree approximations)
  - Recognizes common interval names (m2, M2, m3, M3, P4, TT, P5, m6, M6, m7, M7, P8) within ±15¢ tolerance
  - Shows total span when 3+ notes are held
  - Works accurately with all tuning systems (12-TET, Scala, temperaments, EDOs)

### Technical Details
- C++ Processor: Added `getHeldNotesData()` — iterates active synthesiser voices, returns MIDI notes + frequencies from TuningEngine
- C++ Editor: Timer callback now polls held notes and sends JSON arrays to WebView via `window.updateHeldNotes(notes, freqs)`
- JavaScript: Rewrote `drawTrueKeys()` to use frequency-ratio cent calculation (`1200 * log₂(f₂/f₁)`) instead of scale-degree lookup
- CSS: Added `.tk-grid`, `.tk-cents`, `.tk-total` styles for proper interval display layout
- No parameter changes; backward-compatible

## [3.0.1] - 2026-02-04

### Changed
- **Tuning tab layout refined** - Circle and polar visualizations no longer have panel backgrounds; they appear directly against the paper background for a cleaner look (matching O-Lyrica's approach)
- **Pitch circle enlarged** - SVG viewBox increased from 188x188 to 320x320 with proportionally larger radii for better readability
- **Polar canvas enlarged** - Canvas increased from 180x180 to 300x300

### Added
- **Spoke highlighting on note play** - When a MIDI note is played (from DAW or GUI keyboard), the corresponding spoke on the pitch circle turns red with an enlarged dot, providing real-time visual feedback of which scale degrees are active
- **Note event forwarding** - C++ processor now tracks active MIDI notes via atomic bitfields and the editor forwards note-on/off events to the WebView tuning panel

### Technical Details
- UI + C++ change: CSS panel removal, SVG enlargement, JS note-highlighting, C++ note tracking
- No DSP changes; pluginval Level 5 validated

## [2.4.1] - 2026-02-04

### Changed
- **Moved Humanize slider to Output section** - Relocated from Advanced section to Output section
  - Now positioned between Reverb slider and output meters for better workflow
  - Output meters compressed slightly to accommodate the new layout
  - More logical grouping with final output stage controls

### Technical Details
- UI-only change (HTML/CSS), no DSP or parameter changes
- Output section now uses 28% width for Reverb/Humanize, 35% for compact meters

## [2.4.0] - 2026-02-04

### Added
- **Humanize parameter** (0-100%) - Per-note random variation for organic bell realism
  - Varies strike position (±5%) per note - simulates hitting different spots on bell
  - Varies mallet hardness (±10%) per note - simulates varying strike force
  - Varies decay time (±15%) per note - each ring is slightly different
  - Varies attack time (±20%) per note - soft mallet bounces vary
  - Varies inharmonicity (±3%) per note - bell shape micro-variations
  - Uses Gaussian distribution (Central Limit Theorem approximation) for natural variation
  - Default 30% humanization for subtle organic character
  - Set to 0% for deterministic/mechanical behavior (backwards compatible)

### Technical Details
- New APVTS parameter: `humanize` (0.0-1.0)
- Per-note variation state stored in BellVoice (calculated once per note-on)
- Variations applied multiplicatively to base parameter values
- UI slider added to Advanced section in WebView interface
- No impact on CPU when humanize = 0% (variations all equal 1.0)

### Research Sources
- [AAS Chromaphone](https://www.applied-acoustics.com/chromaphone-3/) physical modeling techniques
- [Noise Engineering humanization](https://noiseengineering.us/blogs/loquelic-literitas-the-blog/humanization-and-variation/) principles
- Gaussian distribution for natural variation (CLT approximation already existed in codebase)

## [2.3.0] - 2026-02-03

### Added
- **16-voice polyphony** - Increased from 8 to 16 simultaneous voices
  - Allows more complex chord voicings and sustained passages
  - Voice stealing now triggers at 17th note instead of 9th

## [2.2.1] - 2026-02-03

### Changed
- **Complete factory preset redesign** - 25 new presets with descriptive, evocative names
  - Replaced culturally-specific names with descriptive acoustic character names
  - Full utilization of v2.2.0 parameters: airAbsorption, airAbsorptionTime, acousticBrightness
  - All 5 material types exercised across preset library
  - Research-informed parameter values based on modal synthesis bell research

### New Preset Categories & Names

**Large Bells (5):** Deep, long-sustaining tones
- Deep Bronze Tower, Massive Iron Bell, Cavernous Brass, Grand Cathedral Bell, Slow Tolling Bell

**Bright Bells (5):** Clear, articulate tones
- Bright Clear Crotale, Crystalline Steel Chime, Sparkling Aluminum, Brilliant Bronze Plate, Crisp Steel Bar

**Warm Bells (5):** Mellow, soft attack
- Soft Mallet Bronze, Mellow Brass Bowl, Warm Aluminum Bars, Gentle Hand Bell, Velvet Bronze Tone

**Metallic (5):** Complex spectra with inharmonicity
- Dense Bronze Gamelan, Clanging Steel Plate, Beating Bronze Gong, Shimmering Bell Tree, Dark Iron Resonance

**Ambient (5):** Atmospheric, evolving textures
- Distant Cathedral, Underwater Bell, Evolving Bronze Wash, Frozen Steel Shimmer, Ethereal Chime Pad

### Research Sources
Based on `research/modal-synthesis-bells-academic-research.md`:
- Church bell partial ratios (Hum:Prime:Tierce:Quint:Nominal)
- Frequency-dependent damping: R_k = b_1 + b_3 * f_k^2
- Multi-stage envelope structures (strike → body → hum tail)
- Fletcher & Rossing bell physics data
- Risset bell inharmonicity ratios

### Technical Notes
- Domain: Preset data only (no DSP changes)
- Previous presets must be deleted manually: `rm -rf ~/Library/O-Bells/Presets/Factory/`
- New presets created on next plugin load after clearing old presets
- Preset compatibility: BREAKING - old preset files incompatible with new parameter format

## [2.2.0] - 2026-02-03

### Added
- **GUI Keyboard in footer** - 2-octave interactive keyboard (C3-B4) for auditioning sounds
  - Click or touch keys to play notes
  - QWERTY keyboard support (Z-M for C3-B3, Q-P for C4-B4)
  - Visual feedback on key press
  - Sends MIDI to synth engine via `sendMidiNote` native function

### Changed
- **Footer panel expanded** - New layout: Gain Fader | GUI Keyboard | Branding
  - Gain slider moved from Output section to sticky footer for quick access
  - Footer height increased from 40px to 55px
  - Output section now contains only Reverb slider and stereo meters

### Technical Notes
- Domain: UI + C++ (index.html, PluginEditor.cpp, PluginProcessor.cpp/.h)
- New C++ methods: `triggerNoteOn(int, float)`, `triggerNoteOff(int)`
- New native function: `sendMidiNote(note, velocity, isNoteOn)`
- Keyboard uses `juce::Synthesiser::noteOn()`/`noteOff()` (thread-safe)
- Tab content height adjusted: `calc(100% - 145px)`
- **Integration approach:** Inline CSS/JS additions rather than using standalone module files (see NOTES.md for details)

### Compatibility
- Preset compatibility: Fully compatible (no parameter changes)
- DAW session compatibility: Fully compatible

## [2.1.0] - 2026-02-03

### Added
- **Air Absorption parameter** - Time-varying lowpass filter simulating realistic acoustic propagation
  - Simulates progressive high-frequency loss as bell sound decays (air absorption, distance effect)
  - 0%: No filtering (transparent, preserves existing sound)
  - 50%: Subtle darkening over decay
  - 100%: Progressive HF rolloff from 18kHz → 2kHz as bell decays
  - Default: 0% (non-breaking, preserves all existing presets)
  - Located in Synthesis section, next to Inharmonicity slider

### Technical Notes
- Domain: DSP + UI (BellVoice.cpp, PluginProcessor.cpp, PluginEditor.cpp, index.html)
- Total parameters: 32 (was 31)
- Implementation: One-pole lowpass filter per voice, cutoff modulated by decay progress
- Filter coefficient: `coeff = 1 - exp(-2π * cutoff / sampleRate)`
- Cutoff formula: `cutoff = 18000 - (18000 - minCutoff) * decayProgress` where `minCutoff = 18000 - 16000 * airAbsorption`
- CPU overhead: Negligible (~2 multiplies + 1 exp per sample, only when parameter > 0)
- Research basis: [Modal synthesis frequency-dependent damping](https://nathan.ho.name/posts/exploring-modal-synthesis/), [bell damping studies](https://www.acoustics.asn.au/conference_proceedings/ICSVS-1997/pdf/scan/sv970230.pdf)

### Compatibility
- Preset compatibility: Fully compatible (new param defaults to 0%, no audible change to existing presets)
- DAW session compatibility: Sessions saved with older versions will load with Air=0%

## [2.0.0] - 2026-02-03

### Breaking Changes
- **Parameter ID renamed:** `brightness` → `overtoneBrightness`
  - Existing presets will NOT load brightness values correctly
  - DAW automation lanes referencing "brightness" will break
  - Users must re-save presets after loading in v2.0.0

### Added
- **Acoustic Brightness parameter** - Controls frequency-dependent decay rate (new)
  - Simulates air absorption and natural bell physics where higher frequencies fade first
  - 0%: Higher partials decay 4× faster (very warm, dark sustain)
  - 50%: Moderate HF decay (natural bell character)
  - 100%: Normal decay rates (bright, synthetic)
  - Default: 70% (slightly natural)
  - Research: Based on [Stanford CCRMA frequency-dependent damping](https://ccrma.stanford.edu/~jos/pasp/Frequency_Dependent_Damping.html) and [bell damping studies](https://www.acoustics.asn.au/conference_proceedings/ICSVS-1997/pdf/scan/sv970230.pdf)

### Changed
- **Brightness parameter split into two controls:**
  - **Overtone Brightness** (renamed from "Brightness"): Controls initial partial amplitudes
    - Expanded range: [0.1×, 2.0×] for highest partial (was [1.0×, 2.0×])
    - 0%: Dark attack (upper partials attenuated)
    - 50%: Neutral
    - 100%: Bright attack (upper partials boosted)
  - **Acoustic Brightness** (new): Controls how fast upper partials decay over time

### Technical Notes
- Domain: DSP + UI (BellVoice.cpp, PluginProcessor.cpp, index.html)
- Total parameters: 31 (was 30)
- Acoustic brightness formula: `acousticDecayMult = 1.0 - (1.0 - acousticBrightness) * partialRatio * 0.75`
- Applied to both standard decay and multi-stage body phase decay
- All factory presets updated with acousticBrightness=70% default

### Migration Notes
To migrate existing presets:
1. Load preset in v2.0.0 (brightness value will be lost)
2. Manually set "Overtone" slider to desired initial brightness
3. Adjust "Acoustic" slider for decay character
4. Re-save preset

## [1.6.0] - 2026-02-03

### Changed
- **Complete factory preset redesign** - All 25 presets remade with research-informed parameter values
  - New preset names reflecting real instruments and sonic characteristics
  - Full utilization of bloom, shimmer, multi-stage decay, and ensemble features
  - Acoustically accurate parameters based on bell physics research

### Research Sources
Presets designed using acoustic research on real bells:
- Church bell partial ratios: Hum(0.25):Prime(0.5):Tierce(0.6):Quint(0.75):Nominal(1.0)
- Gamelan inharmonicity derived from bronze metallophone spectra (sléndro tuning)
- Singing bowl beating frequencies (~2-3Hz monaural beats from asymmetric modes)
- Tubular bell strike pitch phenomenon (4th/5th/6th partials in 2:3:4 ratio)
- Steel pan harmonic generation through nonlinear vibration
- Vibraphone modal frequency ratios (1:2.76:5.4 for bar instruments)

### New Preset List

**Orchestral (5):**
- Westminster Chimes - Steel tubular bells with characteristic twangy brightness
- Crystal Glockenspiel - Pure, high steel bars with suppressed overtones
- Jazz Vibes - Warm aluminum vibraphone with motor-like shimmer
- Antique Crotales - Sustaining bronze discs with harmonic purity
- Nutcracker Celesta - Gentle hammered steel with wooden warmth

**Sacred (5):**
- Flemish Carillon - True-harmonic bronze with minor third partial
- Russian Zvon - Cast iron with intentionally beating partials
- Himalayan Bowl - Bronze singing bowl with monaural beat texture
- Temple Tam-Tam - Large gong with complex inharmonic bloom
- Sanctus Handbell - Clear brass with prominent octave partial

**World (5):**
- Javanese Saron - Extreme inharmonicity bronze bar (sléndro-informed)
- Balinese Bonang - Knobbed gong essential to gamelan tuning
- Trinidad Tenor Pan - Steel pan with nonlinear harmonic generation
- West African Balafon - Warm woody tone with subtle buzzing texture
- Temple Woodblock - Dry percussive with emphasis on attack

**Ambient (5):**
- Spectral Bloom - Slowly evolving texture with maximum bloom
- Ice Crystals - High shimmering harmonics with crystalline character
- Subterranean Drone - Deep, dark texture with extreme sub presence
- Wind Chimes - Delicate, spacious aluminum chimes
- Sunken Cathedral - Filtered, underwater-like bell atmosphere

**Cinematic (5):**
- Trailer Impact - Massive hit bell for epic moments
- Dread Toll - Dissonant, tense bell for suspense/horror
- Ascension - Ethereal, angelic bell for emotional peaks
- Harbinger - Dark, ominous bell with deep sub weight
- Victory Peal - Bright, triumphant celebratory bell

### Technical Notes
- Domain: Preset data only (no DSP changes)
- Previous presets must be deleted manually: `~/Library/O-Bells/Presets/Factory/`
- New presets will be created on next plugin load after clearing old presets
- All parameters now properly utilized (bloom, shimmer, multi-stage decay, reverb, attack)

## [1.5.4] - 2026-02-03

### Changed
- **UI reorganization** - Improved parameter grouping for better workflow
  - Renamed "Character" section to "Onsets" - better describes the parameters in this group
  - Moved "Strike" and "Mallet" sliders from Synthesis section to Onsets section
  - Onsets section now contains all onset-related parameters: Strike, Mallet, Attack Amount, Noise, Velocity

### Technical Notes
- Domain: UI only (no DSP changes, no parameter ID changes)
- Preset compatibility: Fully compatible (parameter IDs unchanged)

## [1.5.3] - 2026-02-03

### Fixed
- **No sound when Strike at 0% or 100%** - Strike position now produces sound across the full range
  - Root cause: Comb filter formula created zero-nodes at position extremes (`sin(0)=0`, `sin(nπ)=0`)
  - Fix: Replaced comb filter with spectral tilt model for more musical and audible results

### Changed
- **Strike parameter redesigned** - Now models physical strike position with clear tonal difference
  - 0% (center): Warm, fundamental-heavy tone - like striking the bell's center
  - 100% (edge): Bright, partial-rich tone - like striking near the rim
  - Smooth spectral transition across the full range

### Technical Notes
- Domain: DSP (BellVoice.cpp:calculateStrikePositionGain)
- Preset compatibility: Existing presets may sound slightly different (spectral balance changed)

## [1.5.2] - 2026-02-03

### Added
- **Attack Amount slider in UI** - Now visible in Character section, underneath the Noise (attack type) selector
  - Controls the level of the strike transient (noise, thud, ping)
  - 0% = minimal transient, pure tone
  - 50% = natural transient level (default)
  - 100% = exaggerated transient, percussive

### Changed
- **Parameter label renamed** - "Attack" → "Attack Amount" for clarity

### Technical Notes
- Domain: UI only (parameter already existed since v1.3.0, just wasn't visible in UI)
- Total UI controls: 30 sliders + 2 choice params
- Preset compatibility: Fully compatible (parameter ID unchanged)

## [1.5.1] - 2026-02-03

### Changed
- **Bloom Speed readouts now display milliseconds** - All 4 bloom speed controls show actual time values instead of percentages
  - Main Bloom Speed: 25-400 ms (mid-partial range)
  - Bloom Speed Low: 15-250 ms
  - Bloom Speed Mid: 25-400 ms
  - Bloom Speed High: 50-800 ms

### Technical Notes
- Domain: UI (parameter display only, no DSP changes)
- Preset compatibility: Fully compatible (internal values unchanged)

## [1.5.0] - 2026-02-03

### Added
- **Bloom Fine Controls** - Per-band (Low/Mid/High) independent control of bloom speed and amount
  - 6 new parameters: `bloomSpeedLow`, `bloomSpeedMid`, `bloomSpeedHigh`, `bloomAmountLow`, `bloomAmountMid`, `bloomAmountHigh`
  - Toggle parameter `bloomFineEnabled` enables Override Mode
  - When enabled, main Bloom Speed/Amount sliders are greyed out and per-band controls take full effect
  - Expandable UI section with clear visual distinction between basic and advanced controls
  - Default values: All at 50% speed, 0% amount (matches behavior when disabled)

### Technical Notes
- Domain: DSP + UI
- Total parameters: 30 (was 23)
- Band definitions: Low (partials 0-1), Mid (partials 2-4), High (partials 5-7)
- Preset compatibility: New params default to values that match current behavior

## [1.4.1] - 2026-02-03

### Changed
- **Bloom Speed range expanded** - Much wider duration ranges for more dramatic bloom effects
  - Low partials: 15-250ms (was 5-15ms)
  - Mid partials: 25-400ms (was 30-120ms)
  - High partials: 50-800ms (was 50-200ms)
- **Low partial bloom amount increased** - More audible bloom on foundation tones
  - Low partials: 0-40% reduction (was 0-5%)

## [1.4.0] - 2026-02-03

### Added
- **Bloom Speed parameter** - Independent control over bloom duration (0-100%)
  - Low partials: 5-15ms bloom time
  - Mid partials: 30-120ms bloom time
  - High partials: 50-200ms bloom time
- **Bloom Amount parameter** - Independent control over bloom intensity (0-100%)
  - Controls how much partials swell from initial to peak amplitude
  - Low partials: subtle effect (0-5% reduction)
  - Mid partials: moderate effect (0-60% reduction)
  - High partials: dramatic effect (0-90% reduction)

### Changed
- **Bloom parameter split** - The original single "Bloom" parameter is now two separate controls:
  - "Bloom Speed" controls how fast the swell occurs
  - "Bloom Amount" controls the intensity of the spectral swelling effect
  - Provides finer control over the organic "breath" effect on bell attacks

### Breaking Changes
- **Bloom parameter replaced** - Old presets with `bloom` parameter will not load correctly
  - Users should resave presets after loading in v1.4.0
  - Default values: Speed=50%, Amount=0% (bloom off by default)

### Technical Notes
- Domain: DSP + UI
- Total parameters: 23 (was 22)
- Pluginval: Passes Level 5 (VST3 and AU)

## [1.3.0] - 2026-02-03

### Added
- **Attack parameter** - New slider (0-100%) controls strike transient volume
  - 0% = minimal transient, pure tone
  - 50% = natural transient level (default)
  - 100% = exaggerated transient, percussive

### Changed
- **Shimmer quality improvement** - Wider LFO range (0.1-8 Hz) with better desynchronization
  - Replaced prime ratios with more spread values for organic metallic shimmer
  - No more audible LFO synchronization patterns at any setting
- **Material differentiation** - Exaggerated material properties for audible distinction
  - Bronze: Baseline warm church bell (1.0x decay)
  - Brass: Bright, short, jazzy (0.7x decay, +0.20 brightness)
  - Steel: Very bright, long sustain (2.0x decay, +0.25 brightness)
  - Aluminum: Very bright, short, thin (0.5x decay, +0.30 brightness)
  - Cast Iron: Dark, long, gamelan-like (1.5x decay, -0.25 brightness)
- **Attack noise overhaul** - Replaced filtered noise with impulse-driven resonant filter bank
  - 4 resonant filters tuned to first 4 partials
  - Q values based on strike character (Click=10, Thud=2, Ping=5)
  - Strike transients now sound like physical mallet impact

### Fixed
- **Bloom bug** - Bloom effect now produces audible amplitude swell
  - Fixed decay masking bloom by delaying decay until bloom completes
  - Added spectral bloom with staggered partial timing (low partials instant, high partials fade in)
- **Per-note variation** - Repeated strikes now sound subtly different
  - Added Gaussian-distributed pitch variation (±10 cents)
  - Added amplitude variation (±25%, clamped 50%-150%)

### Breaking Changes
- **Material parameter type changed** from continuous slider to discrete dropdown
  - Old presets will round material value to nearest discrete option
  - Users should resave presets after loading in v1.3.0

### Technical Notes
- Domain: DSP
- Milestone: acoustic-realism-v2
- Total parameters: 22 (was 21)
- Pluginval: Passes Level 5 (VST3 and AU)

## [1.2.0] - 2026-02-02

### Added
- **Bloom parameter** - Spectral swelling effect where partials swell from initial to peak amplitude before decay
  - Range: 0-100%, Default: 0% (off)
  - Creates organic "breath" to bell attacks
- **Shimmer parameter** - Frequency modulation that increases during decay for metallic shimmering effect
  - Range: 0-100%, Default: 20%
  - Prime-ratio LFOs prevent phase locking between partials
  - Intensity increases as bell decays for authentic bell shimmer
- **Mallet temporal spreading** - Soft mallets now have gradual 50ms attack, hard mallets remain instant
- **Stereo enhancement infrastructure** - Per-partial panning, slow pan LFOs, and Haas delay functions implemented

### Changed
- **Material system overhaul** - Replaced 4-material system with research-based 5-material acoustic model
  - Bronze (1.0x decay, neutral), Brass (0.9x, +0.05 brightness), Steel (1.4x, +0.10 brightness)
  - Aluminum (0.7x, +0.15 brightness), Cast Iron (1.2x, -0.10 brightness, gamelan-like)
  - Each material now affects decay, brightness, AND inharmonicity
- **Inharmonicity label** - UI now displays full word "Inharmonicity" instead of "Inharm"

### Removed
- **Decay Shape parameter** - Multi-stage decay envelope is now always active
  - Simplified UI with more predictable sound
  - Note: Presets with old `decayShape` entries will ignore them

### Technical Notes
- Domain: DSP
- Milestone: realism-overhaul
- CPU overhead: ~4.3% increase for new features
- Memory: ~23KB increase (8 voices with bloom/shimmer/stereo state)
- Preset compatibility: Existing presets load with bloom=0%, shimmer=20% defaults

## [1.1.1] - 2026-02-02

### Fixed
- **Output clipping at default settings** - Added proper gain staging normalization in DSP
  - Root cause: 8 partials summing to ~2.7x, plus unison voices and octave layers, caused signal to exceed 0 dBFS
  - Fix: Normalize signal for partial count (0.4x), unison voices (sqrt), and octave layer blend before output gain stage
  - Output gain at 0 dB now produces unity gain as expected

## [1.1.0] - 2026-02-02

### Added
- **Reverb control** - New "Reverb" slider in the Output section for adding spaciousness to bell sounds
  - Range: 0-100% wet/dry mix
  - Default: 30%
  - Uses JUCE's high-quality reverb with optimized settings for metallic/bell tones
  - Positioned to the left of the Gain slider as requested

### Technical Details
- Added `reverbMix` parameter to APVTS (ID: "reverbMix", version 1)
- Implemented `juce::dsp::Reverb` with bell-optimized settings (roomSize: 0.7, damping: 0.4, width: 1.0)
- WebView UI binding via WebSliderRelay/WebSliderParameterAttachment
- Total parameters: 19 (was 18)

## [1.0.0] - 2026-02-02

### Added
- Initial release of O-Bells physical modeling bell synthesizer
- 18 parameters across 5 sections: Synthesis, Ensemble, Character, Advanced, Output
- WebView-based UI with botanical aesthetic
- 25 factory presets across 5 categories: Orchestral, Sacred, World, Ambient, Cinematic
- Full preset management with save/load functionality
- Real-time output metering
