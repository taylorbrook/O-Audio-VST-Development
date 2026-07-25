# Changelog — O-ReverseDelay

All notable changes to the O-ReverseDelay granular reverse delay.
Format loosely follows [Keep a Changelog]. **v1.0.0 is the first shipped product
version** — there is no earlier release track.

## [1.5.0] — 2026-07-25 — Grain Size to 4000 ms

Minor release. One requested change — `grainSize`'s ceiling raised from **500 ms
to 4000 ms** — plus the two things that change forces and one latent test-harness
defect it exposed.

### `grainSize` — 50 to 4000 ms, default 200, skew centred on 316 ms

The knob now spans the **same 50–4000 ms as `delayTime`, on the same taper**.
Sharing `delayTime`'s 316 ms skew centre is deliberate: the two long-throw time
knobs sit next to each other on the panel, so a given knob angle now reads as
roughly the same duration on both.

The default stays **200 ms** and every shipped sound is unchanged — a new
instance renders identically to v1.4.0.

Measured off the rendered UI, not derived:

| knob | grain size |
|------|-----------|
| 0 % | 50 ms |
| 25 % | 68 ms |
| 50 % | 316 ms |
| 75 % | 1339 ms |
| 100 % | 4000 ms |

The taper is steep at the bottom — the old 50–500 ms working range now lives in
roughly the lower 55 % of the throw, and the top quarter buys 1339 → 4000 ms.
That is the cost of reaching 4 s on one knob, and it is exactly the cost
`delayTime` has always had.

### The capture ring had to grow — 6.0 s to 13.0 s

This is the load-bearing half of the change, and it is not optional.

A grain spawned at output sample `s` reads source `(s − gD − n)` at its own
sample `n`, so its **last** read lands at `(s − gD − G)` while the write head has
itself advanced to `(s + G)`. The ring must therefore span `gD_max + 2·G_max`,
not `gD_max + G_max`:

```
gD_max = kDelayTimeMaxMs + kDelayScatterMaxMs = 4.0 + 0.5 =  4.5 s
G_max  = kGrainSizeMaxMs                                  =  4.0 s   (was 0.5)
      -> 4.5 + 2·4.0                                      = 12.5 s required
```

Shipping the wider range against the old 6.0 s ring would **not** have faulted.
Long grains would simply have wrapped onto material the writer had already
overwritten — no NaN, no discontinuity (the ring is contiguous), every existing
probe still green, and the only symptom "the long settings sound a bit crunchy".

Cost: **~5.0 MB** stereo at 48 kHz (was ~2.3 MB), ~10 MB at 96 kHz, ~20 MB at
192 kHz. Allocated once in `prepareToPlay()`, never on the audio thread.

**The invariant is now a `static_assert`, not a comment.** Every prose statement
of this requirement was already correct at v1.4.0 and none of them stopped this
release from silently invalidating it — comments do not fail the build. Any
future move of `kDelayTimeMaxMs`, `kDelayScatterMaxMs` or `kGrainSizeMaxMs` that
outgrows the ring now stops the compiler.

### User presets are migrated; sessions need nothing

Two storage formats, opposite treatment — the same split v1.0.1 documented:

- **Sessions** (APVTS) store *denormalised* milliseconds. A session saved at
  350 ms recalls 350 ms under the new range with no migration, and rescaling one
  would actively corrupt it. Untouched.
- **Preset JSON** stores *normalised fractions*, which shift meaning when the
  range moves. A v1.4.0 preset at the old default 200 ms holds ~0.573 — which
  under the new curve would read back as **~1450 ms**.

`migrateUserPresets()` gains a `grainSize` arm. The two arms carry **different
version gates**, which is the part that was easy to get wrong:

| parameter | moved at | gate |
|-----------|----------|------|
| `delayTime` | v1.0.1 | `version < 1.0.1` |
| `grainSize` | v1.5.0 | `version < 1.5.0` |

Reusing the existing `!= "1.0.0"` gate would have migrated v1.0.0 presets and
silently left every v1.1–v1.4 preset — the bulk of any real library — holding a
fraction against the old curve. The failure is quiet: the preset still loads, it
just recalls the wrong grain size.

`grainSize` is also the harder rescale of the two. `delayTime` kept its skew
centre and moved only its max; `grainSize` moved **both** (max 500 → 4000, centre
158 → 316), so the curves differ in shape as well as extent and no scale factor
does the job — only reconstructing the old range and round-tripping through
milliseconds. The gate is per-file, so migration is idempotent even if a previous
pass was interrupted before the sentinel was written.

Factory presets need no edits: they are authored in engineering units and
re-converted through the new range on the `.factory-version` bump. All eight
re-seed and recall at `worst=0.0000` tolerance.

### Fixed — render-harness version string had drifted two releases

`tests/render-harness/CMakeLists.txt` pinned `JucePlugin_VersionString="1.2.0"`
while the plugin shipped 1.3.0 and 1.4.0. The file's own comment warns that this
value is load-bearing rather than cosmetic — both the factory-preset and
user-preset sentinels key off it — so probes N and R spent two releases auditing
v1.2.0's stale on-disk presets. Now 1.5.0, and the re-seed is visible in the run.

### Verified

All **108** render-harness probes pass, including:

- **`ring-cover-maxgrain`** (new) — at `D = G = 4000 ms` the reversed burst is
  absent before 4 s and present from 5–9 s, ratio `8.6e-8`. This is the assertion
  an undersized ring fails: a 6 s ring would wrap `(s − 8 s)` forward onto recent
  material and leak the burst into the early window.
- **`grainsize-preset-migration`** (new) — worst recall error `0.000061 ms`
  against a 0.01 ms parameter step, and the un-migrated drift is at least 9.0 ms
  (70 ms would recall 61.0 ms), so the probe has teeth rather than passing on a
  migration that did nothing.
- `blocksize-invariance` and `scatter-blocksize-invariance` — still bit-identical
  (`max|512−4096| = 0.000000000`), with W1 now driving `grainSize` at the new
  4000 ms maximum.
- `count-default-identity`, `window-default-identity`, `window-loopnorm-identity`
  — all still exactly `0.000000000`.
- `decay-count-fb100` / `ceiling16-loop-bounded` — decay still negative and
  monotone; the raised range does not disturb the v1.3.0 loop trim.
- Full-range parameter sweep now covers 50–4000 ms with no click or NaN.

### Not affected

Recorded because each looks adjacent and is not:

- `GrainScheduler::kMaxSpawnsPerBlock` — its bound is
  `overlapMax · kDelayTimeMinMs / kGrainSizeMinMs`, which keys off grainSize's
  **minimum**. Unmoved, so the cap's 8× margin is intact.
- `GrainPool`'s 32 slots — a grain lives `G` samples and the spawn interval is
  `G/overlap`, so concurrent grains ≈ overlap regardless of `G`.
- `loopCountTrim` — a function of overlap only.
- `ReverseGrain` — latches an `int G`; it owns no buffer to resize.
- `sizeRandom` — already clamps to `kGrainSizeMaxMs`, so it follows the new
  ceiling automatically.
- The WebView readout — reads `SliderState.getScaledValue()`, so it tracks the
  C++ range with no JS change.

## [1.4.0] — 2026-07-25 — Continuous Tukey taper + window-shape display

Minor release. Two requested changes: unfreeze Tukey's shape parameter, and show
the windowing function on screen.

### The range, corrected

The request was for a range of 0.01–9.9. Tukey's shape parameter is its **taper
fraction α**, hard-coded at `0.5` in `WindowLut.h` since v1.2.0, and it is
mathematically bounded to **[0, 1]** — 0 is the rectangular window, 1 is exactly
Hann, and there is nothing above 1 to reach. A 9.9 maximum would have clamped
from 1.0 upward, leaving roughly 90 % of the knob's travel rendering an identical
window. Confirmed with the user and shipped as **[0.01, 1.00]**.

### `tukeyTaper` — 0.01 to 1.00, step 0.01, default 0.50

- **0.01** — very nearly rectangular. Fast grain edges, an open/gated character.
- **0.50** — the shipped window. The default, so nothing existing changes.
- **1.00** — exactly Hann, reached rather than approached.

Rendered with **no new table and no new transcendental in the grain loop**,
because Tukey's taper is literally a Hann half. With `taperEnd = α/2`:

```
Tukey_taper(φ) = 0.5(1 − cos(πφ / taperEnd))
Hann(x)        = 0.5(1 − cos(2πx))
            ->   Tukey_taper(φ) = Hann(φ / (2·taperEnd))
```

so the whole window is one phase remap into the existing Hann table, saturating
at the flat top (`Hann(0.5)` is exactly 1.0f, so a grain's plateau is a true
unity plateau):

```
u = min(φ, 1 − φ)                 distance to the nearest edge
r = min(u / taperEnd, 1) · 0.5    [0, 0.5], flat top at exactly 0.5
w = Hann_table(r)
```

Continuous in α, zero extra memory, and one per-grain flag rather than a
quantised bank of tables.

**The step is load-bearing.** α changes the window's duty cycles, so the level
and feedback normalisations must track it — and `WindowLut`'s rule is that those
constants are integrated from the real window, never hand-derived. Integrating
2048 points per block on the audio thread is not available, so the stats are
precomputed per α in the constructor; a 0.01 step over [0.01, 1.00] makes that a
100-entry grid on which **every reachable α lands exactly**, so the stats are
exact rather than interpolated and the α = 0.5 entry reproduces v1.3.0's
constants bitwise. `taper-default-grid-exact` asserts that.

### ⚠ Not bitwise for Tukey — measured, and confined

The remap deviates from v1.3.0's stored Tukey table by up to **2.4e-6
(−112.5 dB)**. That is the Hann table's linear-interpolation error, not a change
of shape, and it cannot be avoided: reading a 2048-point table at an arbitrary
phase is not the same operation as evaluating `cos` there.

Stated properly rather than waved at — 2.4e-6 is roughly **20× a 24-bit LSB**, so
it is not "below the noise floor" as an absolute envelope error. What makes it
inaudible is *where* it occurs: the worst case is at φ ≈ 0.999, at the very end of
the taper where the window value is itself almost zero, so the error multiplies a
sample being faded out. Against a typical source level it lands near −124 dB.

The blast radius is Tukey only, and the cross-version diff confirms it rather
than asserting it: of 93 shared probes, **92 are byte-for-byte identical** and
the one that moved is `window-live-Tukey` (0.075537 → 0.075538). Hann, Gaussian,
Triangular and Expo-Decay never take this path, all eight factory presets are on
Hann, and a default session is bitwise unchanged. At α = 1.0 the remap lands on
the table's own points and the deviation drops to 4.2e-7.

### α needed two normalisation corrections, not one

Third release running where the output and feedback paths required *different*
constants for the same control — after shape (v1.2.0) and overlap (v1.3.0). The
split the engine has carried since v1.1.0 earns its keep again:

| | duty at α=0.01 | at α=1.0 | swing |
|---|---|---|---|
| Power (output path) | 0.994 | 0.375 | 4.2 dB |
| Amplitude (loop path) | 0.995 | 0.500 | 6.0 dB |

An α-aware output norm alone would have left ~1.8 dB of per-generation error in
the loop — "taper" heard as "tail length". Results:

| Measurement | Result |
|---|---|
| Wet level across α (probe AJ) | **0.010 dB** spread |
| Decay at fb 60, α ≥ 0.1 (AK) | **0.030 dB/s** vs default |
| Decay at fb 100, α ≥ 0.1 (AK) | **0.056 dB/s** vs default |
| Tilt power-invariance, all 100 α × 5 tilts | **exactly 1.0f** |

**α = 0.01 is a documented exception**, not a regression: 0.240 dB/s at fb 60 and
0.791 at fb 100. A near-rectangular window has crest factor ~1.0 against Hann's
1.63 and overlaps to something close to a constant; neither is removable by a
linear duty constant, which is the same statement `WindowLut.h` already makes
about Expo-Decay. It is bounded and printed separately rather than excused.

On the click risk the low end implies — the engine removed a 2 % Gaussian
pedestal at v1.2.0 for exactly this reason — the answer is measured and reassuring:
at the 50 ms grain minimum the grain-edge first difference is **0.0112 at α = 0.01
against 0.0058 at α = 0.5**, i.e. twice as fast and nowhere near the 0.25 click
threshold. A fast edge is the point of the low end, not a defect.

### The window display, inside the WINDOW panel

It draws the live envelope with shape, tilt and taper composed, a dashed midpoint
guide so tilt is legible, and a filled area so a near-rectangular taper reads as
"more window" at a glance.

It sits **inside the WINDOW panel**, beneath the three controls that shape it, so
the panel reads top-to-bottom as Shape → Tilt/Taper → the resulting window — cause
then effect, the same ordering the panel already used for Shape over Tilt. (It
was first built as a panel of its own in the reserved SPACE slot; putting it with
its controls reads better and leaves that slot reserved, so the row-3 / MORE-page
decision is still one release away rather than due now.)

Fitting it cost ~73 px the panel did not have spare, bought back by shrinking that
panel's own controls: knobs 56→46 px, select padding 7→4 px, gaps 12→9 and 7→5.
**Every one of those rules is scoped to `.group-window`** — `.knob`, `.knob-cell`,
`.select-cell` and `.division-select` are shared by all eight panels, so an
unscoped edit would have resized the whole interface while looking correct in the
one screenshot anyone checks. The frontend check now asserts the scoping.

The height budget is measured, not estimated, and lands with 1 px spare in a
158 × 213 body — the same zero-slack discipline the row geometry uses:

```
select-cell   28 + 6 + 10           =  44
knob-cell     46 + 5 + 10 + 5 + 12  =  78
env-cell      6 + 58 + 6 + 2 border =  72
2 row gaps at 9                     =  18
                              total =  212 of 213
```

The knob-stem is scaled with the knob (24→20 px). It is the one part of the knob
that is a fixed pixel height rather than a percentage gradient, so leaving it
would have given the smaller dial a pointer that overshot its own edge.

The curve is **fetched from C++** (`getWindowCurve`, 128 points) rather than
recomputed in JavaScript, and that is the design rather than an implementation
detail: a JS copy of the window would be a second definition free to drift from
the first, and a graph has no units to reveal it when it does — unlike a knob
readout, which is why the same rule already keeps readouts on `getScaledValue()`.
It is pulled on change (coalesced at 40 ms), not polled.

Verified by canvas hashing rather than by eye, which produced the nicest result
in this release: rendering Hann, Tukey α=0.50, α=0.01, α=1.00 and Expo-Decay
gives **4 distinct canvases from 5 renders** — and the single collision is
Tukey α=1.00 against Hann, byte-identical, because α=1 *is* Hann. The display
proves the mathematical identity visually.

`tukeyTaper` is **inert unless Tukey is selected**: the cell dims and sets
`aria-disabled`, but stays relay-bound and adjustable, so a value set beforehand
is honoured. Hiding it would make the panel jump as Shape changes.

### Row 2 still has a reserved slot

RANDOM | WINDOW | COUNT | SPACE — unchanged from v1.3.0. Because the display went
inside WINDOW rather than taking SPACE, the chassis v1.1.0 sized for v1.2–v1.6
keeps one free panel, and the row-3 / MORE-page decision (v1.0.0 review, section
D) is still ahead rather than forced now. No panel width, position or height
changed, so the tooltip edge-clamp geometry is the geometry v1.3.0 verified.

### Verification

| Check | Result |
|-------|--------|
| v1.3.0's shared probe result lines vs v1.4.0's | **92 of 93 byte-for-byte identical** (the one delta is the documented Tukey remap) |
| Offline render harness | **106/106 probes PASS, exit 0** (93 + 13 new) |
| `ui_frontend_check.js` | **ALL CHECKS PASSED** |
| `ui_tooltip_clamp_check.js` @ 940×743 | **ALL CHECKS PASSED**, 20/20 anchors, clamp fired on 4, 15/15 knobs bound |
| WINDOW panel height budget | **212 of 213 px**, `scrollHeight == clientHeight` (no overflow) |
| Envelope display, canvas-hashed | **4 distinct renders / 5**, collision = Tukey α=1 ≡ Hann |
| `pluginval --strictness-level 10` VST3 | **exit 0 ×3** |
| `pluginval --strictness-level 10` AU | **exit 0 ×3** |
| `auval -v aufx ORvD OuDv` | **AU VALIDATION SUCCEEDED** |
| AU component version | **66560** (= 1.4.0) |

New probes: `taper-remap-is-tukey`, `taper-alpha1-is-hann`,
`taper-duty-closed-form`, `taper-default-grid-exact`,
`taper-tilt-power-invariant`, `level-flat-taper`, `decay-taper-fb60/fb100`,
`taper-live-1/25/100`, `taper-inert-off-tukey`, `taper-edge-report`.

Two harness fixes found on the way: probes Z4 and AF compare each measurement
against the reference configuration's *as they go*, which works only because
their reference happens to be first in their sweep. The new taper probe's
reference (α = 0.5) sits third, and a single-pass version reported a whole decay
rate as the delta — a 9.7 dB/s "failure" that was the probe. It now collects
first and compares after. The `boundReadouts` assertion tightened at v1.3.0 also
caught the knob count again (15, not 14), which is the argument for keeping it
exact rather than `>=`.

### Not done

Human DAW sign-off. The taper is verified offline and by both validators, but
whether α near 0.01 is musically useful or merely edgy is a listening call.

## [1.3.0] — 2026-07-25 — Grain count / overlap ceiling

Minor release implementing **section B2** of the v1.0.0 review
(`improvements/2026-07-24-v1.1-review.md`): grain count was never directly
controllable, only inferable from density and grain size. Fills the second
reserved panel of the chassis v1.1.0 framed — markup plus one CSS block, no
resize.

### What changed

- **`grainCount`** — an explicit overlap ceiling, 2–16, step 1, **default 8**.
  The density map becomes `overlap = 2 + density·(ceiling − 2)`, replacing the
  hard-coded `2 + d·6`. Density 0 still gives overlap 2 at any ceiling, so
  nothing previously reachable became unreachable.
- **Spawn cap 32 → 128**, and no longer silent: dropped requests and pool
  refusals are counted separately and exposed on the processor.
- **COUNT panel** — the Count knob plus a live **Active / Overlap** readout.

### Why the ceiling is its own parameter and not a wider density knob

Density is stored **denormalised** in session state: a session saved at 60 %
recalls 60 %. Widening the density knob's own span to reach overlap 16 would
therefore have made every existing session ~2.3× denser at the same knob
position, and there is no migration available — APVTS sessions and preset JSON
need opposite treatment, and rescaling a session tree corrupts the ones already
correct (`critical_apvts_denormalised_vs_preset_normalised`).

A separate parameter sidesteps it entirely. Absent from any v1.0–v1.2 session or
preset, `grainCount` resolves to its default, and `(8 − 2)` is exactly `6.0f` —
so the new expression is the same three float operations on the same values that
v1.0.1 shipped. Bitwise, not "equivalent": written as
`min + d·(ceiling − min)` and deliberately not as the algebraically identical
`min·(1−d) + ceiling·d`, which lands an ulp away. Third release running where the
no-op default is a specific number rather than zero (v1.1's four → 0, v1.2's
`grainTilt` → 0.5, this → 8).

### The gain correction landed on the opposite path from the one predicted

The review expected `grainGain`'s `1/sqrt(overlap)` to under-correct as overlap
rose — overlapping grains read the same reversed material, so summing should be
partially coherent, the real rise should sit between √N and N, and the error
should grow with the ceiling. Sound reasoning, wrong path.

**Output path: no correction needed, measured.** The grains read the same
material but not at the same *time*. At a fixed output sample, grain *k* reads
source `2kH − D − t` for spawn interval *H*, so every pair in the sum is
separated by a multiple of `2H` — decorrelated for broadband input. Probe AA
sweeps overlap 2 → 16 and holds the wet level inside **0.07 dB** with no
correction term, against the ±1 dB budget probes D and Z2 use.

> The first version of that probe measured a **2.5 dB non-monotonic** spread,
> which is exactly what a coherence error looks like. It was the harness. The
> shared excitation generator has ±0.077 autocorrelation at lags 600–2400
> samples, which is precisely where the spawn interval sits, so each overlap
> setting summed a different amount of correlation *in the test signal* — the
> +1.45 dB outlier at overlap 10 sat on the generator's correlation peak at lag
> 960, that setting's own interval. A murmur3 finaliser (`whiteNoiseAt`,
> max|acf| 0.0024) drops the spread to 0.07 dB. Added alongside the old
> generator, not replacing it, so pre-v1.3.0 probe numbers stay diffable.

**Feedback path: a clipping defect.** What recirculates *is* self-similar, and
there `1/sqrt(N)` leaves √N of excess loop gain per generation. Harmless while N
stopped at 8; doubling the ceiling spent the whole margin:

| ceiling | decay @ fb 100, before the fix |
|---------|-------------------------------|
| 8       | −0.29 dB/s (shipped)          |
| 10      | **+0.87 dB/s** (growing)      |
| 12      | +0.87 dB/s                    |
| 14      | +0.67 dB/s                    |
| 16      | +0.46 dB/s → 90 s peak **1.28, clipped** |

Positive dB/s is self-oscillation. The `tanh` bounds the loop to ±1 per sample,
but the wet output is a near-coherent sum of 16 grains each reading loop content
at the limiter's ceiling, and `1/sqrt(16)` does not bound that.

Fixed by `loopCountTrim` = `(N/8)^−0.5` — the fully-coherent amplitude law,
used as derived rather than tuned — anchored at the legacy ceiling and exactly
`1.0f` at or below it, so the shipped decay stays bitwise the shipped decay. It
rides on the output/loop gain split v1.2.0 built for `gainRandom`. After:
decay spread across all ceilings **0.020 dB/s**, worst-case peak **0.28**.

### The spawn cap

The old 32 matched `GrainPool::kMaxGrains` on the reasoning that "excess spawns
would only steal grains anyway" — which died at v1.1.0, when the pool started
*refusing* instead of stealing. A drop and a refusal became different events, and
only the refusal is a design choice. The bound is now derived and sample-rate
independent: `overlapMax · kDelayTimeMinMs / kGrainSizeMinMs` = 16 · 50/50 = **16
nominal against a cap of 128**. Note it is grain size's *minimum*, not the
ceiling, that this cap is most sensitive to — below ~6 ms it would reach 128 at
ceiling 16, and probe AB is what will notice.

### The UI, and a reversed decision

The readout **reverses Stage 3's decision D10** ("no visualization, no Timer, no
C++→JS polling bridge"), deliberately: `GrainPool::countActive()` shipped in
Stage 2 and was called by nothing until v1.1's probe Y, which is how density
stayed an abstract percentage. It is a *pull* — JS polls a native function at
15 Hz — so there is no `juce::Timer` and no event-listener plumbing, which keeps
the whole bridge inside the surface the ui-stub already models. The native-fn
count goes 11 → 12.

The panel that was labelled MOTION is now **COUNT**: 276 px, unchanged in width,
position and height, so the frame geometry every tooltip clamp was verified
against is the same geometry. The label had to move regardless — "Motion"
describes delay-time drift (review B4 #6), which is not this, and now waits in
SPACE.

### Verification

| Check | Result |
|-------|--------|
| v1.2.0's 80 shared probe result lines vs v1.3.0's | **byte-for-byte identical** |
| Offline render harness | **93/93 probes PASS, exit 0** (81 + 12 new) |
| `ui_frontend_check.js` | **ALL CHECKS PASSED** |
| `ui_tooltip_clamp_check.js` @ 940×743 | **ALL CHECKS PASSED**, 18/18 anchors, clamp fired on 4 |
| `pluginval --strictness-level 10` VST3 | **SUCCESS ×3**, exit 0 |
| `pluginval --strictness-level 10` AU | **SUCCESS ×3**, exit 0 |
| `auval -v aufx ORvD OuDv` | **AU VALIDATION SUCCEEDED** |
| AU component version | **66304** (= 1.3.0) |
| Grain-steal refusal at ceiling 16 | **holds** — `ceiling16-pool-clickfree`, maxStep 0.0088 vs 0.177 threshold, peak 24/32 grains |

The AU pluginval run emits one pre-existing `!!! WARNING: Current program is
−1` from the JUCE AU wrapper. Not a failure; exit code 0.

New probes: `level-flat-count`, `spawncap-headroom-512/4096`,
`count-default-identity`, `ceiling16-pool-clickfree`, `count-live-2/12/16`,
`count-meter-live`, `decay-count-fb60/fb100`, `ceiling16-loop-bounded`.

### Not done

Human DAW sign-off. The ceiling raise is verified offline and by both
validators, but "does overlap 16 actually sound like a smoother wash" is a
listening judgement no probe makes.

## [1.2.0] — 2026-07-24 — Grain window shape + tilt

Minor release implementing **section B1** of the v1.0.0 review
(`improvements/2026-07-24-v1.1-review.md`), which called window tilt "the
highest-value single change in this document". Fills the WINDOW panel that
v1.1.0 framed and reserved — markup only, no resize, exactly as promised.

This is a *reverse* delay, and through v1.1.0 every grain played its source
backwards under a **symmetric** Hann: each one swelled in and out identically,
so the effect smeared but never bloomed. A window whose peak sits late produces
the backwards-swell-into-a-transient shape the effect is bought for.

### The compatibility guarantee, and how it was verified

Both new parameters default to the **shipped window**, and the no-op is not zero
for both: `grainShape` defaults to index 0 (Hann) and `grainTilt` to **0.5**
(symmetric). 0 is a hard peak-early tilt, so "new parameter, default it to 0"
would have re-voiced every existing session and all eight factory presets.

The defaults are the engine's *exact* no-op rather than approximately it, and
that is by construction (see the tilt design below). Measured the same way
v1.1.0 measured its own: the v1.1.0 harness was rebuilt from commit `8fa3646`
and run head-to-head.

| Check | Result |
|-------|--------|
| v1.1.0's 63 probe result lines vs v1.2.0's | **byte-for-byte identical** |
| Offline render harness | **81/81 probes PASS, exit 0** (63 + 18 new) |
| `ui_frontend_check.js` | **ALL CHECKS PASSED** (sections 1–15) |
| `ui_tooltip_clamp_check.js` @ 940×743 | **ALL CHECKS PASSED**, 16/16 anchors, clamp fired on 4 |
| `pluginval --strictness-level 10` VST3 | **SUCCESS ×3**, zero failures |
| `pluginval --strictness-level 10` AU | **SUCCESS ×3**, zero failures |
| `auval -v aufx ORvD OuDv` | **AU VALIDATION SUCCEEDED** |
| AU component version | **66048** (= 1.2.0) |

### Added

- **`grainShape` (Hann / Tukey / Gaussian / Triangular / Expo-Decay, default
  Hann)** — `WindowLut.h` was hard-coded Hann and had noted since Stage 2 that
  it was trimmed from O-simpleGrain's 5-shape `WindowLuts.h`. All five tables are
  built in the constructor, never on the audio thread, and indexed at spawn.
  Hann's table is bit-for-bit the expression v1.0.0 shipped.
  - The **Gaussian deliberately deviates** from O-simpleGrain's copy: at σ = 0.18
    the raw curve reads 0.021 at both ends rather than 0. In a one-shot granular
    texture that 2 % step is inaudible; here it would be a step at every grain
    boundary, overlapping 2–8 deep, inside a loop that re-reverses it every
    generation. The pedestal is subtracted and the result renormalised.
- **`grainTilt` (0–1, default 0.5)** — moves the window's peak within the grain.
  0 = peak early (a plucked, decaying reverse grain), 0.5 = the symmetric Hann
  as shipped, 1 = peak late (slow swell into a fast cut). Displayed as a signed
  percentage centred on "Centre"; the parameter keeps its 0–1 range because 0.5
  is the value whose warp is exactly neutral.
- **WINDOW panel**, with tooltip copy, dblclick reset and keyboard/wheel
  adjustment on the same footing as every other control.

### The tilt is a two-segment linear phase warp

`q = min(p, t)·(0.5/t) + max(p − t, 0)·(0.5/(1 − t))`, mapping `[0, t] → [0, ½]`
and `[t, 1] → [½, 1]`. Chosen over the review's suggested `read(pow(phase, k))`
(which puts a transcendental back in the grain loop) and over a family of
pre-tilted LUTs (~2.6 MB for a quantised approximation of a continuous control).
Two properties are exact rather than approximate:

- **At t = 0.5 it is the bitwise identity.** Both coefficients are exactly
  `1.0f`, so `q = min(p, 0.5) + max(p − 0.5, 0)`; for p ≥ 0.5 Sterbenz's lemma
  makes `p − 0.5` exact and `0.5 + (p − 0.5)` rounds to exactly `p`. Asserted
  over a 4097-point phase sweep, not assumed.
- **It is power-invariant for symmetric windows.** The segments' Jacobians are
  2t and 2(1−t), so the warped mean square is `t·mLo + (1−t)·mHi` — independent
  of t whenever the halves match. Tilt therefore cannot move the level or the
  loop's duty cycle for four of the five shapes, by construction rather than by
  a compensating constant.

### Two normalisations, because the two paths sum differently

The review warned that `grainGain = 1/sqrt(overlap)` assumes Hann's power duty
and that a shape change would read as a volume *and* a feedback change. Both
halves were real, and they needed **different** constants — which the first
implementation of this release got wrong in an instructive way.

- **Output path — power.** A pass over broadband input has each grain reading a
  different stretch of the ring, so contributions are decorrelated and sum in
  power. `shapeNorm = √(m_hann / m_shape)` folded into `grainGain` holds all five
  shapes inside **0.147 dB** (probe Z2). Tukey's mean square is 0.687 against
  Hann's 0.375, so uncompensated it would have landed +2.6 dB.
- **Feedback tap — amplitude.** What recirculates is the wash the engine just
  made: self-similar material read by overlapping grains at nearby offsets, so it
  sums closer to *coherently*, and a coherent sum follows the window's **mean**,
  not its mean square. Power-only normalisation left the decay rate spanning
  **4.40 dB/s** at feedback 100, ranked exactly by amplitude duty — "window
  shape" audible as "how long the tail lasts". `getLoopNorm()` multiplies the
  loop tap gains only.

The engine could express this because it has carried separate output and
feedback-tap gains since v1.1.0, where the split was built so `gainRandom` could
sit *after* the feedback tap. The same split, used in the other direction,
carries this. Neither constant may cross over: `loopTrim` on the output would
undo the power normalisation; `gainRandom` in the loop would make the decay rate
stochastic. Measured, worst deviation from Hann in dB/s:

| | feedback 60 | feedback 100 |
|---|---|---|
| power-only, all five shapes | 6.216 | 4.400 |
| power-only, excluding Expo-Decay | 1.318 | 1.330 |
| **+ loop trim, all five** | **1.848** | **0.175** |
| **+ loop trim, excluding Expo-Decay** | **0.042** | **0.042** |

Expo-Decay's residual is **not** a normalisation error and no linear constant
removes it: its crest factor is 3.10 against Hann's 1.63, so at equal loop energy
its peaks hit the loop's `tanh` harder and it genuinely loses more per
generation. It is largest at feedback 60 — mid-knee, where a limiter's
incremental gain is most level-dependent — rather than at 100, where everything
is deep enough into limiting for the differences to wash out. Probe Z4 bounds the
four low-crest shapes at 0.35 dB/s and Expo-Decay separately, so a regression to
power-only normalisation still fails even though it would sit inside any single
bound wide enough for Expo-Decay.

### Changed

- **`.shape-select` is 120 px, not `.division-select`'s 82 px.** "Expo-Decay"
  measures 94 px with padding and the arrow and rendered clipped. Visible only in
  a browser render — build, `auval`, `pluginval` and the static checks all pass a
  clipped select. Still inside the 190 px panel's content box, so nothing moves.
- **`bindDivisionCombo` generalised to `bindSelectCombo(juce, paramId)`** and
  called twice. The grainShape select needs identical behaviour — options built
  from live `properties.choices`, rebuilt if they arrive late, index refreshed on
  both events — and a second copy would be a second place for that to rot.
- **Factory presets** carry both new keys explicitly at the shipped window. The
  CMake `VERSION` bump is what makes those edits reach disk; at a frozen version
  the preset table is a silent no-op.

### Testing

Harness **63 → 81 probes**. New: `window-warp-identity`, `window-norm-identity`,
`window-default-identity`, `window-duty-report`, `window-loopnorm-identity`,
`level-flat-shape`, `level-flat-tilt-{Hann,Expo-Decay}`,
`decay-shape-fb{60,100}`, `window-live-{Tukey,Gaussian,Triangular,Expo-Decay}`,
`window-live-tilt{0,100}`, plus `sweep-grainTilt` / `sweep-grainShape` in the
all-parameter sweep and two new columns in the factory-preset audit.

`level-flat-tilt` runs for **Hann and Expo-Decay** specifically: for a symmetric
window the warp is power-preserving by construction and `getTiltNorm()` returns
exactly `1.0f`, so Hann tests the *warp*; Expo-Decay is the only asymmetric shape
and therefore the only one where the tilt normalisation arithmetic actually runs.
Testing Hann alone would leave it entirely unexercised.

`window-live-*` is the mirror of probe T: every other new probe asserts a *must
not change*, and a control wired to nothing satisfies all of them perfectly.

Two harness corrections fell out of the first run, both worth recording because
each looked like a DSP regression and neither was:

- **`setBaseline()`/`setDefaults()` now reset the window parameters.** Probe M
  sweeps them and leaves them where its triangle ended; probes P, Q and V run
  afterwards and inherited a tilted, non-Hann window. Probe Q's
  constant-overlap-add flatness read 0.3718 instead of 1.0000. Resetting at the
  source makes the leak impossible rather than making it every future probe's job
  to remember.
- **`decay-shape-fb60` needed its own measurement windows.** At ~9.8 dB/s, probes
  S and X's `[5–10 s]` vs `[20–25 s]` pair spans ~133 dB and the later window is
  reading the denormal floor. Four of five shapes still agreed to 0.04 dB/s while
  Expo-Decay read 1.5 dB/s off — which looks exactly like a normalisation failure
  and is not one.

### Notes

- No new RNG draws. The two xorshift streams' consumption per spawn is unchanged
  from v1.1.0, so probes T (zero-determinism) and W2 (block-size invariance) stay
  valid without re-tuning.
- Both parameters are latched per grain at spawn. Smoothing a window *shape* is
  not merely unnecessary but meaningless — two windows disagree at every phase,
  so any crossfade still steps a live grain's envelope.
- A symmetric window's two halves are canonicalised at construction when their
  power agrees to within 1e-6 relative. `std::cos`/`std::exp` evaluated at
  mirrored arguments disagree in the last ulp, which would turn "exactly 1.0f at
  every tilt" into "1.0f ± 5e-8" — inaudible, but it would cost the ability to
  assert power invariance as an *exact* property, and an invariant checkable only
  to a tolerance can rot by a real amount unnoticed. Mirroring the tables instead
  was not available: Hann's must stay bit-identical to v1.0.0's.

---

## [1.1.0] — 2026-07-24 — Grain randomisation + UI chassis

Minor release implementing **section B3** of the v1.0.0 review
(`improvements/2026-07-24-v1.1-review.md`): the four grain randomisations that
close the gap between "many reverse delays" and "a granular cloud". Also
expands the editor chassis **once**, sized for the controls planned through
v1.6, so later releases drop into space that already exists.

Builds on v1.0.1's grown capture ring, as the review required.

### The compatibility guarantee, and how it was verified

All four new parameters default to **0**, which is the exact no-op in the
engine — not a small value, a genuine no-op. Every randomisation is gated on
`amount > 0` and draws **nothing** from the RNG when off, so the pan sequence
is untouched and existing work renders identically.

This is measured, not asserted. The v1.0.1 harness was rebuilt from commit
`78af47b` and run head-to-head with v1.1.0:

| Check | Result |
|-------|--------|
| v1.0.1's 49 probe result lines vs v1.1.0's | **byte-for-byte identical** |
| Offline render harness | **63/63 probes PASS, exit 0** (49 + 14 new) |
| `ui_frontend_check.js` | **ALL CHECKS PASSED** (sections 1–15) |
| `ui_tooltip_clamp_check.js` @ 940×743 | **ALL CHECKS PASSED**, 14/14 anchors |
| `pluginval --strictness-level 10` VST3 | **SUCCESS ×3**, zero failures |
| `pluginval --strictness-level 10` AU | **SUCCESS ×3**, zero failures |
| `auval -v aufx ORvD OuDv` | **AU VALIDATION SUCCEEDED** |
| AU component version | **65792** (= 1.1.0) |

Because nothing is renamed, removed, re-ranged or re-typed, and no existing
session or preset changes value or sound, this is MINOR rather than MAJOR.

### Added

- **`jitter` (0–100 %, default 0)** — randomises the grain **spawn interval**,
  `interval · (1 ± 0.9·jitter·u)`. Through v1.0.1 the scheduler was a strictly
  periodic countdown, and a fixed interval against a fixed grain length is a
  comb — the reason sustained material read as metallic rather than as a cloud.
  The deviation is symmetric, so the *mean* interval, and with it the average
  overlap and the feedback loop's duty cycle, are exactly unchanged. Capped at
  ±90 % rather than ±100 % so the low tail cannot approach a zero-length
  interval (i.e. a spawn every sample).
- **`delayScatter` (0–500 ms, default 0)** — randomises each grain's latched
  delay by ±this. Thickens the smear without moving the rhythmic anchor,
  because the mean delay is unchanged. This is the parameter that required
  v1.0.1 first: it can push a grain's latched delay 500 ms *past* the delayTime
  maximum, so the worst-case read span became 4.5 + 2·0.5 = **5.5 s** — which
  v1.0.1's 5.5 s ring met by a single sample. The ring is now **6.0 s**
  (+192 KB stereo at 48 kHz) for a real margin.
- **`sizeRandom` (0–100 %, default 0)** — randomises each grain's latched
  length, clamped back into `grainSize`'s own range. Jitter alone leaves a
  residual periodicity because every grain still shares one envelope length;
  this removes it. Clamping to the parameter's own endpoints means a randomised
  grain is never longer than one the user could dial in by hand, which is what
  keeps the ring bound above true.
- **`gainRandom` (0–100 %, default 0)** — randomises per-grain gain for depth
  and shimmer, applied **after** the feedback tap (see below). Power-normalised
  by `1/sqrt(1 + dev²/3)`, so it changes spread and not level.
- **RANDOM panel** holding the four new knobs, with tooltip copy, dblclick
  reset and keyboard/wheel adjustment on the same footing as every other knob.
- **`getActiveGrainCount()`** — exposes the live concurrent-grain count.
  `GrainPool::countActive()` had existed since Stage 2 and was called by
  nothing; the harness now reports peak concurrency as a measured number.

### Changed

- **`GrainPool::obtain()` refuses the spawn when no slot is free, instead of
  stealing the oldest grain.** v1.0 overwrote the oldest slot in place, which
  cut a live Hann envelope from mid-window to zero in one sample — a click, not
  a crossfade. It was unreachable in v1.0 steady state (max overlap 8 against
  32 slots), but all four randomisations raise the transient concurrent-grain
  peak, so it had to be safe *before* they landed. Refusing costs one
  contributor out of a wash of 8–32 and is inaudible.
- **The wet path is now accumulated twice** — once with per-grain random gain
  (the output) and once without (the feedback tap). This is what keeps
  `gainRandom` downstream of the loop: a randomised gain inside a recirculating
  path compounds every generation, so the knob would control *how long the tail
  lasts* rather than how it shimmers, and at feedback = 100 would make the
  decay rate itself stochastic. Costs two extra mul-adds per grain-sample.
- **`rngState` seeds from a per-instance hash** rather than the shared literal
  `0x12345678`. v1.0 gave every instance the same seed, so two instances on two
  tracks produced identical pan sequences — and would have produced identical
  grain randomisation too, correlating exactly where a wide cloud is wanted.
  The seed is fixed for the *lifetime of the instance*, not re-rolled per
  `prepareToPlay`, so one instance still reproduces across prepare/reset cycles.
  Under `OUARICON_RENDER_HARNESS=1` it collapses back to v1.0's literal.
- **Editor 940 × 484 → 940 × 743.** A second panel row (RANDOM | WINDOW |
  MOTION | SPACE) sharing row 1's pinned width contract (190 | 190 | 276 |
  190), so the two rows align column-for-column. `215 + 14 + 245 = 474`
  consumes the height increase exactly — row 1 and the footer do not move.
  Capacity is ~27 knob-cell slots against the ~26 controls planned through
  v1.6. WINDOW / MOTION / SPACE are framed and labelled but empty, carrying a
  dimmed fleuron; filling one in a later release is an HTML change with no
  resize and no re-verification.
- `.botanical-overlay` height pinned at 340 px instead of `70%`. Under a
  percentage the resize scaled the plate to ~520 px and it began reading as
  clutter behind two rows of translucent panels.
- `kDelayTimeMinMs` named, replacing the `50.0f` literals in the parameter
  range and the tempo-sync clamp — the same single-definition discipline A1
  established for the maximum.

### Fixed

- **The ui-stub's `delayTime` range was stale at 50–2000 ms**, missed when
  v1.0.1 widened it to 50–4000. Any browser render of the page — which is the
  gate for the failure classes C++ builds cannot see — was showing a readout
  that disagreed with the plugin.

### Verification added

Fourteen new render-harness probes (T–Y) and two new frontend sections:

- **T `random-live` / `random-zero-determinism`** — each randomisation
  measurably changes the render (no dead controls), and two independent
  all-zero renders are bit-identical.
- **U `level-flat`** — wet RMS within ±1 dB across {0, 50, 100 %} for all four,
  the same budget probe D holds density to. Catches a character knob that is
  really a loudness knob.
- **V `jitter-breaks-grid`** — at density 0 a regular spawn grid overlap-adds
  perfectly flat (probe Q), so flatness reads grid regularity directly:
  1.0000 at jitter 0, 0.0339 at jitter 100. Asserts *both* ends, so a dead
  jitter fails rather than passing quietly.
- **W `scatter-ring-worst-case` / `scatter-blocksize-invariance`** — the 5.5 s
  read span against the 6.0 s ring, and 512-vs-4096 bit equality with all four
  randomisations on.
- **X `gainrandom-loop-neutral`** — loop decay at feedback 100 with gainRandom
  0 vs 100: −2.493 vs −2.527 dB/s, delta 0.034. The single assertion behind
  "applied after the feedback tap".
- **Y `pool-pressure-clickfree`** — grainSize swept under maximum
  randomisation; peak concurrency reported (14/32), click-freedom asserted.
- **`ui_frontend_check.js` §15** — four-way knob closure across
  `createParameterLayout` / `kSliderIds` / `KNOB_IDS` / the `knob-*` and
  `val-*` elements, plus a FORMAT entry and a ui-stub range for each, plus the
  four defaults pinned at 0. A knob wired in three of the four places is a
  silently dead control.
- **`ui_tooltip_clamp_check.js`** (new file) — drives the real page in a
  browser at the real 940 × 743 and measures every tooltip rectangle. The
  static check can prove the clamp *code* is correct but not that it *fires*,
  because that depends entirely on viewport width
  (`pattern_tooltip_clamp_gate_viewport_sensitive`). It asserts both edges, not
  just width, and fails if the clamp never engages at all.

### Two block-size bugs caught during this work

Both were found by the new probes, and both would have shipped silently:

1. **Jitter draws batched per pass.** The scheduler consumes its RNG inside a
   per-sample countdown while the spawn handler consumes after the whole pass
   is scheduled. Sharing one stream interleaved them differently at 512 than at
   4096 samples, so an offline bounce would not match what was monitored. Fixed
   by splitting into two streams, each consumed a fixed number of times per
   spawn — making consumption a function of spawn *index*, which is block-size
   invariant.
2. **The scatter clamp was derived from `passLen`.** A2 bounds each engine pass
   to `D`; negative scatter can put a grain's latched delay below that, and the
   obvious repair — clamp the latched delay up to `passLen` — makes the latched
   value itself depend on the host block size. Fixed by keying both the pass
   bound and the clamp off `grainDelayFloor`, a function of the parameters
   alone.

Per-grain randomisation values are also now drawn *before* the pool slot is
requested, so a refused spawn consumes exactly what a granted one does and RNG
consumption cannot depend on pool occupancy.

---

## [1.0.1] — 2026-07-24 — DSP correctness

Patch release fixing the three defects found in the v1.0.0 read-only review
(`improvements/2026-07-24-v1.1-review.md`, sections A and C). **No new
parameters, no UI change, no window resize.** All three were invisible to auval,
pluginval-10 and the shipped 41-probe harness, which is why they survived Stage 4
— the harness gained 8 probes that each fail on v1.0.0 and pass here.

Validated at release:

| Gate | Result |
|------|--------|
| Offline render harness | **49/49 probes PASS, exit 0** (41 shipped + 8 new) |
| Same harness vs v1.0.0 DSP | **12 FAIL** — the new probes are not vacuous |
| `pluginval --strictness-level 10` VST3 | **SUCCESS ×3**, zero failures |
| `pluginval --strictness-level 10` AU | **SUCCESS ×3**, zero failures |
| `auval -v aufx ORvD OuDv` | **AU VALIDATION SUCCEEDED** |
| `ui_frontend_check.js` | **ALL CHECKS PASSED, exit 0** |
| AU component version | **65537** (= 1.0.1) |

### Fixed

- **A1 — tempo sync silently clamped across this plugin's own tempo range.**
  `delayTime` maxed at 2000 ms and sync-derived times were clamped into it, so
  `1/1` collapsed below 120 BPM, `1/2D` below 90 and `1/2` below 60. In the
  70–100 BPM band the brief targets, the UI named a division the engine was not
  playing and two divisions landed on the same delay with no indication.
  **Root cause:** a literal `2000.0` in the sync clamp
  (`PluginProcessor.cpp:323`) duplicating the parameter's max instead of
  referencing it. `delayTime` max is now **4000 ms** (covers `1/1` at 60 BPM),
  the clamp reads the same `kDelayTimeMaxMs` constant, and the capture ring grows
  **3.5 s → 5.5 s** to cover `Dmax + 2·Gmax` = 4.0 + 1.0 s (≈ 2.1 MB stereo at
  48 kHz). The skew centre deliberately stays at **316 ms**, so short delay times
  keep their knob resolution.
- **A2 — grains read unwritten capture at large block sizes.** A grain spawned at
  block offset `i` latched `readAbs = blockStart + i − D` and rendered *before*
  the block's capture write, so reads were already-written only while `i < D`.
  With `D` bottoming out at 2205 samples (50 ms at 44.1 kHz), every 2048- or
  4096-sample buffer — routine in offline bounce and high-latency live rigs — had
  its late grains reading a full ring lap of stale audio, or silence early on.
  **Fix:** each engine pass is now bounded to `D` samples, so `i < D` holds by
  construction at any host block size. Chosen over clamping `D ≥ numSamples`
  (which would silently lengthen the delay at large buffers) and over
  write-input-first (which still drops the block's own feedback regeneration).
  The engine is now block-size **invariant**: a 4096-sample render is
  bit-identical to a 512-sample one (`max|Δ| = 0.000000000`, probe O). At the
  shipped 512-sample block with `D ≥ 2400`, the code path is a single pass and
  bit-identical to v1.0.0 — the fix costs nothing where it was already correct.
- **A3 — the bottom ~14 % of Density was a full-depth tremolo.** `overlap` mapped
  to `1 + density·7`, so at low density the hop equalled the grain length and
  Hann grains **abutted** — the wet output amplitude-modulated to true silence at
  every boundary (a 5 Hz, 100 %-depth gate at `grainSize = 200 ms`). Hann reaches
  constant-overlap-add at hop `G/2`, i.e. `overlap ≥ 2`. Remapped to
  **`overlap = 2 + density·6`**: same maximum (8), whole travel now a genuine
  smooth→dense sweep. Measured envelope min/max at `density = 0` goes
  **0.0000 → 1.0000** (probe Q).
- **C — `processBlock`'s oversized-block bail left the extra output channel
  unwritten.** The v1.0.0 bare `return` did already pass channel 0 dry through
  (the review's "bails to total silence" reading is wrong), but in a mono→stereo
  layout channel 1 is never written by this plugin and carried stale host memory.
  Dry is now explicitly duplicated to any unfilled output channel before bailing.
- **C — no `AudioProcessor::reset()` override.** Hosts calling `reset()` left the
  capture ring, grain pool, scheduler countdown and filter states populated, so a
  stale reverse tail survived a host-level reset. Now cleared, alloc-free, with
  the RNG re-seeded to the same fixed value `prepareToPlay` uses.

### Changed

- **Factory presets re-authored and re-seeded.** The A3 remap changes what a
  given `density` value means, so every preset's density is rewritten to
  `(7·d_old − 100)/6` — the value that reproduces its **shipped** overlap exactly
  (60→53.3, 55→47.5, 70→65, 30→18.3, 90→88.3, 65→59.2, 80→76.7). All eight
  presets therefore render as they did at v1.0.0; only the knob's scale moved.
  The `VERSION 1.0.0 → 1.0.1` bump is what invalidates the `.factory-version`
  sentinel and lets these edits actually reach
  `~/Library/O-ReverseDelay/Presets/Factory` — at a static version they would
  have been a silent no-op.
- **Feedback decay re-measured at `feedback = 100`** (probe S), since overlap
  sets both the spawn hop and `grainGain = 1/√overlap`, i.e. the loop's duty
  cycle. At the overlap-matched density (5.20) the decay is **−2.955 dB/s**
  against v1.0.0's **−2.958 dB/s** at the same overlap — unchanged to 0.003 dB/s.
  At the *same knob position* (density 60, overlap now 5.6) it is **−2.493 dB/s**,
  i.e. ~0.46 dB/s more sustain. No shipped preset moves, because all eight are
  overlap-matched.
- **Dead code removed:** `GrainScheduler::sampleRate` was stored in `prepare()`
  and never read. (`CaptureBuffer::readAbs()` is still uncalled and deliberately
  kept — it is the entry point for the planned stereo-source mode.)

### Migration Notes

The two persistence formats needed **opposite** treatment, and the review's
premise that both recall by normalised fraction is only half right:

- **Sessions need no migration.** APVTS stores each `PARAM`'s *denormalised*
  value — literal milliseconds — and JUCE restores it through
  `setDenormalisedValue()`, which re-normalises against whatever range is
  current. A v1.0.0 session saved at 1400 ms recalls 1400 ms under the 4000 ms
  range. Rescaling it would have **corrupted** it. Probe P asserts the round trip
  directly, and it passes against both the v1.0.0 and v1.0.1 DSP.
- **User presets do need migration.** `OuariconPresetManager::createPresetJson`
  stores `RangedAudioParameter::getValue()` — the normalised 0–1 fraction — so a
  v1.0.0 preset saved at 1400 ms would have read back as **2450.5 ms** under the
  wider range. `migrateUserPresets()` rewrites the `delayTime` fraction of every
  `"version": "1.0.0"` file in `Presets/User/` through the reconstructed v1.0.0
  range, then re-stamps the file. One-shot, guarded by a
  `.user-migration-version` sentinel mirroring the factory one (without it, every
  processor construction would re-read every preset on the message thread and
  concurrent constructions would race). **Known limit:** a v1.0.0 preset restored
  from a backup *after* the sentinel is stamped will not be migrated.

Automation, parameter IDs, ranges of the other nine parameters, and the state
format are all unchanged — this is not a breaking release.

## [1.0.0] — 2026-07-24 — first release

First shipped version: granular reverse-delay engine, Ouaricon Naturalist WebView
editor, 8 factory presets, preset bar and hover help.

**DSP is frozen as verified in Stage 2.** Stage 4 (Polish) shipped **zero** audio
changes — the D11 feedback-tap makeup constant was auditioned in Standalone and
**explicitly declined**: the wash decays as intended at `feedback = 100`, so the
topology's inherent ≈ −7.3 dB/generation pre-damping loss (−4.3 dB Hann² duty
+ −3.0 dB pan→mono-sum round trip) stands as the shipped character.

Validated at release:

| Gate | Result |
|------|--------|
| Offline render harness | **41/41 probes PASS, exit 0** (33 Stage-2 + 8 factory-preset audits) |
| `pluginval --strictness-level 10` VST3 | **SUCCESS ×3**, zero failures |
| `pluginval --strictness-level 10` AU | **SUCCESS ×3**, zero failures |
| `auval -v aufx ORvD OuDv` | **AU VALIDATION SUCCEEDED** |
| `ui_frontend_check.js` | **76/76 PASS, exit 0** |
| AU component version | **65536** (= 1.0.0) |

pluginval strictness 10 covers Editor, Open editor whilst processing, Automation,
Editor Automation, Plugin state, Plugin state restoration, Parameter thread
safety and Fuzz parameters.

Windows is **deferred to CI** — the CMake already carries `NEEDS_WEBVIEW2` and
`JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, and the two `FileChooser`
completions hoist their `SafePointer` to a local rather than init-capturing it in
a nested lambda, which is what MSVC rejects.

### Added — Stage 4: Polish

- **Factory presets:** 8 presets seeded to
  `~/Library/O-ReverseDelay/Presets/Factory/` on first run — Reverse Bloom,
  Guitar Swell, Vocal Halo, Slow Wash, Tight Smear, Dark Cavern, Near-Infinite,
  Rhythmic Reverse. Authored in **engineering units** (ms / % / Hz / choice index)
  and converted skew-safe through each parameter's own `NormalisableRange` via
  `convertTo0to1`; `delayTime`, `grainSize`, `lowCut` and `highCut` are skewed, and
  a hand-written normalised fraction on any of them would recall 10–30× wrong.
  Harness probe N audits all eight through the **shipping** `loadPreset()` — the
  measured round-trip error is **0.0000 on every parameter of every preset**.
  *Near-Infinite* runs `feedback = 100` and renders 30 s in the harness as a
  preset-driven stability statement. *Rhythmic Reverse* is the one tempo-synced
  preset (1/8 dotted), audited against a 120 BPM playhead.
- **Preset bar:** window grows 940 × 440 → **940 × 484** for a 44 px band under
  the header carrying `◀ ▶ [ name ] Save Load Delete`. The band and the height
  increase are the same 44 px, so panel heights and the footer are untouched.
  Styling reuses the page's own `.segment` / `.division-select` vocabulary rather
  than importing a dark chrome strip that would give the page a second title bar.
  Delete uses a **two-click inline confirm**, never the browser confirm dialog,
  which is a silent no-op or a throw in some JUCE WebView backends.
- **OuariconPresetManager v1.0.5** integrated via CMake include (header-only, no
  vendored copy). Session state now routes through it, so the current preset name
  survives a save/reload; pre-Stage-4 APVTS sessions still load unchanged.
- **Tooltips** on all 10 controls, authored as `data-tip-title` / `data-tip` in
  `index.html`. Hover only — no toggle, no persisted state. The tip measures its
  width at `left: 0` and **pins it before placing**, so the right-most control
  (`mix`) gets a full 230 px tip instead of a shrink-wrapped ribbon.

### Added — Stage 3: GUI

- Ouaricon Naturalist WebView editor: four framed group panels in signal-flow
  order (TIME | GRAIN | FEEDBACK | OUTPUT), all 10 parameters bound two-way
  through `Web*Relay` / `Web*ParameterAttachment`.
- Sync/Free control swap on a shared fixed-size slot — both controls stay
  relay-bound at all times, so neither is ever a dead control.
- Readouts and knob angles come exclusively from `SliderState.getScaledValue()` /
  `getNormalisedValue()`; the C++ `NormalisableRange` is the only source of range
  and skew. Double-click resets to the engineering default fetched from C++.

### Added — Stage 2: DSP

- Reverse grain engine over a 3.5 s stereo capture ring (reverse read offset
  D + 2n), Hann-windowed grains from a 32-slot preallocated pool with per-grain
  parameter latching for click-free changes.
- Feedback loop through the shared capture buffer: wet → gain → high-pass →
  low-pass → `tanh` → non-finite guard. 2nd-order Butterworth damping filters
  updated in place with `ArrayCoefficients` (never `Coefficients::makeXXX` on the
  audio thread), cutoffs clamped to 0.49·fs.
- Tempo sync across a 13-entry note-division table with a no-BPM fallback; width
  spread via an RT-safe xorshift32 with alternating pan sign; custom equal-power
  dry/wet mix (zero latency).

### Added — Stage 1: Foundation

- JUCE 8 plugin shell, VST3 + AU + Standalone, `PLUGIN_CODE ORvD`.
- Bus layouts mono→mono, mono→stereo, stereo→stereo.
- APVTS with the 10-parameter contract: `delayTime`, `syncMode`, `noteDivision`,
  `grainSize`, `density`, `feedback`, `lowCut`, `highCut`, `width`, `mix`.

### Notes

- Preset library location is `~/Library/O-ReverseDelay/Presets/{Factory,User}/`
  (**not** `~/Library/Application Support/`). The name is hardcoded without the
  dev suffix, so dev and release builds share one library.
- Factory presets only re-seed when `JucePlugin_VersionString` changes. While the
  version is frozen at 1.0.0, editing the factory table is a silent no-op until
  `~/Library/O-ReverseDelay/Presets/Factory` is removed. (v1.0.1 bumps the
  version, which re-seeds them.)
