# O-Octagon Changelog

## v1.10.0 (2026-08-27)

**Resolves the four Warnings of `CODE_REVIEW.md` (v1.8.0 review, 2026-08-27).** No Critical
findings existed. Every fix below was verified against the working tree before it was written —
all four defects were live at HEAD 99e7d206. The 30 Info findings are untouched
(`/improve-review-info O-Octagon`).

MINOR, not PATCH: WR-01 changes what a stored `motionSync` index *means* and WR-02 re-sorts the AU
parameter list once more. Both are user-visible on unreleased features; neither renames, removes
or re-ranges a parameter, so nothing breaks — but a dev session is not bit-identical across this
version and that is worth a minor number.

### Fixed

- **WR-01 — the Sync table ran 4× slower than its labels, and two menu pairs were bit-identical.**
  `kSyncMultipliers` (`MotionClock.h`) is cycles per beat and `cyclesAt` applies it with no hidden
  factor — Hz is exactly `bpm/60 · mult`. The v1.8.0 table had `1/4 = 0.25`, glossed in the header
  and both hover-help strings as "one cycle per bar at 4/4" — a defensible convention on its own,
  except that the same menu carried `1 Bar = 0.0625` (one cycle per **sixteen** beats) and
  `4 Bars = 1/64`. *Slow Orbit* ("4 Bars") orbited once every 32 s at 120 BPM. Separately the
  triplet entries used 4/3 instead of 3/2, so `1/16D ≡ 1/8T` and `1/8D ≡ 1/4T` to the bit.
  **Root cause:** the table was byte-copied from O-Orbit, whose table has the same two defects and
  no gloss at all; the gloss was written to fit the numbers rather than the numbers to the labels.
  **Fix:** the table is now written from the musical definitions — a division names the DURATION
  of one cycle, so `1/4` is one cycle per beat (1.0), `1 Bar` one per four beats (0.25), a triplet
  is 3/2 its parent's rate and a dotted note 2/3 of it. Header gloss and both hover-help strings
  (EN/FR) now say "1/4 is one cycle per beat, 1 Bar one cycle per four beats (4/4 assumed)".
  **Consequence for saved state:** a `motionSync` index keeps its *label* and gains its label's
  *meaning* — every synced session from v1.8.0/v1.9.0 now moves 4× faster than it did, which is
  what its menu always said it would do. *Slow Orbit* ("4 Bars") is unchanged as a preset and now
  orbits once every 8 s at 120 BPM.
  **Gate:** new geometry probe **MP9 `sync-table-musical-semantics`** asserts, from the
  definitions and not from the table, `1/4 = 1.0`, the three bar entries, the three (T, plain, D)
  triples, `1/2D`, and that no two entries are equal. Render probe **DD** was re-fixtured (verifier
  finding): its companion clause relied on index 8 being 0.25 to land at `sin t = 1`; it now uses
  index 12 ("1 Bar") at PPQ 1 and still holds bit-identity across both Z consumers.
  **Mirrored into O-Orbit v1.1.1** (same table, same fix, tooltip made explicit).

- **WR-02 — all 11 post-v1.4 parameters carried version hint 1, so the AU parameter list
  re-sorted and Logic automation lanes recorded on ≤v1.4 retargeted.** JUCE's AU wrapper sorts
  parameters by masked ID hash then stable-sorts by version hint; Logic keys lanes by index in
  that list. With every hint at 1 the eleven new hashes interleaved with the seventeen originals:
  `outputGain → motionHeight`, `hullAtten → decorr`, `airAmount → outputGain`, `rolloff →
  hullAtten` (indices 0–12 — the weights, position, width, blur — never moved). VST3 identifies
  by hash and was never affected.
  **Root cause:** `makeFloat`/`makeBool`/`makeChoice` hard-coded `ParameterID { id, 1 }`; the
  comment beside it ("mandatory in JUCE 8") and `GainStage.h`'s "APPENDED so every pre-existing
  index is untouched" both reason about the APVTS index, which the AU order does not use.
  **Fix:** the three factories take a leading `versionHint`; three named generations
  (`kHintV1_0 = 1` for the 17 originals, `kHintV1_5 = 2` for `decorr`, `kHintV1_8 = 3` for the ten
  `motion*`). This restores the ≤v1.4 AU order exactly and gives every future addition a monotone
  baseline. **Cost, stated plainly:** any v1.5–v1.9 *dev* Logic session with lanes on the nine
  parameters that had moved retargets once more. Nothing has shipped; after the first public
  release this becomes irreversible, which is why it lands now.
  **Gate:** new render probe **DO `version-hints-monotone`** walks the LIVE parameter objects and
  holds every id to its generation (28 seen, 0 off-generation); an id it does not know fails it.
  `ui_frontend_check` §15/§16's six layout-parsing regexes accept the new leading argument.

- **WR-03 — the elevation strip drew the live source at the ANCHOR's depth.** The audio thread
  shapes the source at `anchor.y + offset.y` and evaluates ear height there
  (`GainStage.cpp` / `SourceShaper.cpp heightAt`); the strip added only `offset.z` at the anchor's
  `srcY`. On any raked venue the "Source" readout was wrong by rake × offset.y (up to ~0.5 m on
  the default rig), "Ear" read a constant, and the section marker never swept front-to-back — the
  one thing a section view exists to show. `app.js` forwarded `offset[2]` alone.
  **Fix:** `elevation.setMotionZ(z)` → `setMotion(offset, running)`; the strip keeps `motionY` and
  `motionZ` and evaluates `earHeight(anchorDepth + motionY)`. **Gate:** `ui_layout_check` §22
  rakes the rear 4 m, runs an Orbit, and samples 12 ticks: "Ear" must take ≥ 2 distinct values
  and the marker's `cx` must span > 2 px — measured 12 distinct readings (1.51 → 1.61 m) and a
  64 px sweep. The previous gate could not see this: the stub's rake is flat.

- **WR-04 — five of the fifteen monitor-fold checks asserted a local Woodworth transcription,
  never `MonitorFold`; the class's ITD magnitude was asserted nowhere.** The v1.7.0 CHANGELOG's
  "31 smp vs 31.5 model max" was printed, not asserted, and `kMaxItdSeconds` is the class's own
  constant. The verifier proved it by mutation: θ-only, 0.5×, 2×, a 45° clamp and a naive 90°
  clamp all passed 15/15.
  **Fix:** the five lambda checks are renamed `woodworth-formula-*` and documented as model
  documentation, not class coverage. New section **1b** drives the class with an impulse from
  octagon speakers at 45°, 90° (the maximum AND the seam), 135° (the (π − θ) branch) and 180°
  (must be ZERO — a naive clamp reads 31.5) and asserts the far-ear onset lag in samples against
  the *published* formula `r/c · (θ + sin θ)` with c = 343 written in the test, ±2 samples for
  linear interpolation; plus a hard-left/hard-right mirror check. 20 checks, 0 failures.
  **Negative control, re-run this session:** 0.5× Woodworth fails 3 of the new checks (9/15/9
  smp), the naive 90° clamp fails 2 (31 smp at 135° and at 180°), θ-only fails 2 (9/19 smp) —
  while all five formula checks stay green under every mutation, which is exactly the point.
  `README.md` updated.

### A note on HEAD 99e7d206 (v1.9.0)

The v1.9.0 Stage-G commit was made by a concurrent session from the shared index while this work
was in flight, and it swept two of this version's edits into it: `app.js` (the
`elevation.setMotion` call) and the two `i18n.js` Sync hover-help strings. At 99e7d206 `app.js`
therefore calls a method `elevation.js` did not yet have — a `TypeError` on every meter poll. This
commit restores consistency; do not check out v1.9.0 on its own expecting a working elevation
strip. (`pattern_shared_checkout_index_race_between_sessions`.)

### Testing

- Backups: `backups/O-Octagon/v1.8.0/` (from HEAD 2ba236a1 — filled the gap IN-07 reported) and
  `backups/O-Octagon/v1.9.0/` (working tree before this session's edits). Both verified.
- monitor-fold standalone: **20/0** (was 15) · mutation control: 3 wrong models each fail ≥ 2.
- geometry harness: **58/0** (MP9 added) · render harness: **74/0** (DO added, DD re-fixtured; the
  v1.7.0 digest anchor DC and every bit-identity probe unchanged).
- `ui_layout_check` **31 sections, all pass** (2 new §22 checks) · `ui_frontend_check` **43/43** ·
  `check-i18n --strict-v2` repo-wide pass.
- `./scripts/build-and-install.sh O-Octagon` → `auval -v aufx OuOc OuDv` **PASSED**; installed
  `O-Octagon-dev.component` reports 1.10.0.

## v1.9.0 (2026-08-27)

**The PAGE speaks French, not only the hover help.** O-Octagon has offered a French/English
selector since v1.6.0, and its own `lang-select` tooltip told the user, in both languages, that
"the labels on the page itself do not change". That was true and it was the wrong thing to be
true. This version localizes the page: **84 captions, 89 accessible names and 12 script-written
strings**, on canon v2 — the same contract O-Tapestop, O-MultiBandCompressor, O-Bitrot and
O-ReverseDelay already carry. All five shipped plugins are now on canon v2, and
`check-i18n --strict-v2` passes repo-wide for the first time.

Numbers do not move: a readout is never a `[data-i18n]` element, and `ms`, `m` and every metre
value read identically in both languages.

### The mechanism

- **A label is owned by its element**, through `data-i18n="key"` on a LEAF element, with the
  authored English left in the markup as the fallback that renders if `applyI18n()` never runs.
- **`applyLabel()` writes `textContent` AND `dataset.label` together.** That mirror is the
  systemic answer to `pattern_js_state_updater_overwrites_html_labels`: an element carries its
  own record of what its caption should be, so "no state updater erased a label" becomes a
  checkable invariant rather than a convention.
- **A script-written caption declares its own key** through `setLabel(el, 'literal.key')` and
  becomes a `[data-i18n]` element from that moment, so the language sweep owns it. Two-state
  captions are two calls behind an `if`/`else`, never one call with a ternary.
- **`aria-label` is keyed** through `data-i18n-aria`. All 89 are keyed; **56 of them are
  generated from a template** rather than transcribed, because the attribute sweep has no `vars`
  argument and eight speaker rows × six accessible names is a list that goes stale in one row.
- Zero native `title=` attributes remain, and `I18N_EXEMPT` carries the six reasoned exclusions
  (the product name's two halves, `ms`, `m`, and the two language endonyms).

### `tests/ui_frontend_check.js` §6 was REWRITTEN, and the old shape was about to pass vacuously

§6 asserts that no `textContent` write erases an HTML-authored label. It did that with an explicit
whitelist of receiver identifiers, reviewed when it grew.

Canon v2 makes `applyLabel()` a `textContent` writer for **every** label on the page — and the
whitelist did not grow at all, because `applyLabel` takes its element as a parameter named `el`
and `el` was already whitelisted for the map-banner copy. The single largest `textContent` writer
ever added to this page walked straight through and the gate reported nothing.

Growing the whitelist by 84 entries was the alternative and is worse: it asserts "these
identifiers may write", which is not the property. §6 now understands the mirror. A write is
legitimate when it is one half of a `dataset.label` / `textContent` pair written from the SAME
expression, or an explicit mirror teardown (`delete el.dataset.i18n; delete el.dataset.label;`
before printing a raw C++ diagnostic code), or a freshly created node, or one of the remaining
dedicated value receivers. It also now asserts that no keyed element has element children —
`applyLabel` writes `textContent` and would delete them — which is why the Delay column head was
**split** into a keyed caption span beside its unit value span.

**The whitelist SHRANK by two.** `monitorCopy` and `monitorNode` were added at v1.7.0 only because
there was no route for a state-dependent caption; both are `setLabel()` calls now.

### Layout — measured at the shipping 1100 × 720, both languages

Before the CSS work, **four** non-label elements moved between English and French — the fewest of
the five plugins localized so far, which is what the roomiest frame in the suite should produce.
Driving the eight page states the gate can now reach (Motion tab, Drift path, settings popover,
the speaker→output popover, the three banners individually, the Venue screen) raised that to
**twelve** across all states. Every one traces to a shrink-to-fit container:

| Fixed | Measured cause |
|---|---|
| `.screen-tab { min-width: 84px }` | VENUE 80.4 → LIEU 64.5 slid the right-anchored nav 15.7 px |
| `.scene-btn { padding: 6px 3px }` | GAUCHE / DROITE 38.5 in a 36.2 px content box, in a 10-column 1fr grid with no cell to spare |
| `.cell-dense` caption track 44 → 52 px | MARCHE 45.4, SYNCHRO 49.7, VITESSE 45.1 all overran 44 |
| `.settings-unit` 36 → 44 px | MARCHE 39.7 in a 34 px content box |
| `.elev-readouts .cell-label { min-width: 46px }` | Ear 21.0 → Oreille 43.3 dragged the strip 22.4 px |
| `.caption-field { min-width: 50px }` | FIELD 40.1 → CHAMP 48.2 moved `#field-legend` 8.1 px |
| `#readout-label-envelope { min-width: 82px }` | ENVELOPE 71.5 → ENVELOPPE 79.6 |
| `.safe-banner` 184, `.map-banner` 396, `.monitor-banner` 400 px | the three banners are shrink-to-fit in the header flex, so a copy line that grows slides the nav AND the gear |
| `.venue-rake / .venue-delay > .cell-label` 42 → 70 px | RAKE 42.0 → INCLINAISON 69.4 |
| `.vcol-head > [data-i18n] { min-width: 38px }` | DELAY 30.7 → RETARD 36.8 slid the unit span |
| `.venue-head > .cell-label { min-width: 40px }` | VENUE 37.8 → LIEU 27.2 — a caption that gets SHORTER moves its neighbour too |
| `#btn-scene-store { min-width: 46px }` | STORE 44.1 → MÉM. 37.7; a keyed label, so the geometry diff excludes it by design and this was found by measuring |

Nine of these change ENGLISH geometry as well. That is the trade D-04 asks for: French is sized,
never shrunk, and the only box two languages can share is the wider one.

**Two French strings were shortened once, in the table, never at runtime.** `Trajectoire` (66.7 px,
the tooltip's wording) became **`Tracé`** as the Path caption — which is also what this plugin's own
English internals call it (`refreshTrace`, `TRACE_SHAPE_IDS`). The monitor's suppressed-render copy
went from 370.8 px to `Désactivé hors ligne — l'export est propre`, because the full form put the
banner at 461.7 px in a header that also carries the title, the tab pair and the gear.

### Fixed

- **The `lang-select` tooltip no longer lies.** In both languages it said the page labels do not
  change. It now says they do, and that numbers and unit symbols do not.
- **The preset tooltip said "17 parameters".** The real figure is 28 —
  `oo::params::kCount`, static-asserted in `DSP/GainStage.h:79`. It has been wrong since v1.8.0
  added the ten motion parameters. Corrected in both languages.

### Verification

`ui_frontend_check.js` 43 sections, `ui_layout_check.js` 31 sections, `check-i18n --plugin` (canon
v2), `check-i18n --strict-v2` repo-wide, `check-ui-labels --plugin` (84/84 labels measured across
9 states, zero geometry shifts), `boot-all-uis.js` (clean; `title=0 aria=89 i18n=84`),
`O-Octagon-geometry-test` 57 probes and `O-Octagon-render-test` 73 probes from a fresh
`-DOUARICON_BUILD_TESTS=ON` configure, `auval -v aufx OuOc OuDv`.

**Twenty-six negative controls, all twenty-six fired** — including one that did not, at first: the
new `venue.js` alias assertion matched only a bare identifier right-hand side, so
`const el = rows[0].labelInput;` slipped past it. Widened to match the whole RHS, then re-run.

**All 184 French entries are machine drafts, every one `reviewed: false`.** No native speaker has
read one. The language round-trip through the C++ APVTS has still never been executed by hand.

---

## v1.8.0 (2026-08-27)

**Motion engine — generative trajectories for the source puck.** Closes the last HIGH-priority gap
in `.planning/FEATURE-REVIEW.md`: hand-drawing Logic automation for a ten-minute rotation was the
most tedious task in the workflow. Six paths — Orbit, Figure-8, Sweep, Drift, Pendulum, Spiral —
produce an (x, y, z) offset in **venue metres** added downstream of `srcX/srcY/srcZ`, which stay
the anchor: the three position lanes are never written, existing automation keeps working and the
path travels with it.

**The primary constraint was reproducibility, and it is structural.** Phase is a pure function of
absolute position — the absolute 64-sample control grid in Free mode, the host PPQ extrapolated to
each grid boundary when synced — and nothing integrates per call. Two bounces of one session are
identical (Drift included, through a `Seed`), and a session renders the same at 64, 256, 1024 and
ragged block sizes. Both claims were **negative-controlled**: an accumulator in `cyclesAt`'s place
fails eight probes; deleting the dirty-check bypass fails DE and DG.

### Added

- **Ten parameters, group `motion`** (`kCount` 18 → 28): `motionOn` (the first Bool), `motionPath`
  and `motionSync` (the first Choices — a host lane reads "Figure-8", not 0.2), `motionRate`
  (0.01–4 Hz, the first skewed range, centre 0.3 Hz), `motionSize` (extent, 0–24 m),
  `motionRatio`, `motionAngle`, `motionHeight` (0–8 m, coupled to the same phase), `motionPhase`,
  `motionSeed` (1–64, Drift only). `motionOn` defaults **off**.
- **`Source/DSP/MotionPath.h`** — the single header-only, JUCE-free generator compiled by the audio
  thread, the editor's trace and the unit target. **`MotionClock.h`** — absolute-position phase
  derivation with the Free-mode re-base and the stopped/no-PPQ rules. **`PerlinNoise.h`** —
  byte-copy of O-Orbit's.
- **`SourceShaper::shapeAt()`** — steps 2–6 from a metres pair; `shape()` delegates to it. The
  v1.7.0 render digest did not move (probe DC).
- **Position | Motion tab pair** in the Position group. Measured first: a seventh group of any shape
  overflowed the fixed 1100 × 720 column (756 against 592, with 13 px in hand), so Motion is a
  second panel of nine dense cells — three rows, exactly Position's — behind a tab in the title row.
  Seed replaces Phase while Path is Drift.
- **Venue map:** the path **trace** (128 points from the same C++ generator via the new native
  function `getMotionTrace`, 26 → 27), the anchor as a hollow **ghost**, and the **live puck**
  riding the 30 Hz `getMeters` poll as three floats. Drift draws a 48-point tail instead of a
  trace. The elevation strip shows the live height.
- **Two factory presets:** *Slow Orbit* (4 Bars, 8 m, ratio 0.8, 1 m of height) and *Wander*
  (Drift, 0.05 Hz, 6 m, seed 7). The ten motion ids are **authored** (`kAuthored` 6 → 16) so motion
  travels in presets; the six original rows write no motion key and load motion-off.
- **Probes DC, DD, DE, DF, DG, DH, DI, DJ, DK, DL, DM, DN** in the render harness (61 → 73) and
  **MP1–MP8** in the geometry target (49 → 57); the harness gains `HarnessPlayHead`.
- EN + FR hover help for the tab pair and the ten controls.

### Compatibility

- **Motion off is bit-identical to v1.7.0.** The v1.7.0 `shape()` call and dirty-check predicate
  run verbatim on the off branch; probe DC holds the render against the v1.7.0 binary's own digest
  (`0xb8c5a2d0c7518204`, captured from commit `2e03020e` before any DSP edit), with
  `instr::motionSolves == 0` as the other half of the claim.
- **Pre-1.8.0 presets load motion-off** with the 18 restored bit-exact (probe DK). No migration
  hook — no range moved.
- **SAFE mode:** motion runs (the map animates) and is inaudible by construction — the mono/stereo
  fold never reads a position (probe DM).
- **Accepted:** a tempo change mid-block re-syncs at the next block start. A bounce started at a
  PPQ that is not a multiple of 64 samples from the session start evaluates the path at instants up
  to 63 samples shifted from a bounce from zero — the grid is aligned to the absolute sample
  counter, by design (`GainStage.h`), and the 5 ms ramps absorb it.

## v1.7.0 (2026-08-26)

**Binaural / stereo monitoring fold-down — the piece is finally audible away from the hall.**
Closes the HIGH-priority gap in `.planning/FEATURE-REVIEW.md`: the brief's own use case, "preparing
and revising a piece away from the venue", had no listening path at all, while SpatGRIS (BINAURAL),
L-ISA Studio, SPAT Revolution and dearVR all treat headphone monitoring as table stakes.

**The primary design constraint was not the DSP — it was that the monitor must never silently
contaminate a render.** Five independent guards enforce it, and no single mistake disables more
than one:

| # | Guard | What it kills |
|---|---|---|
| 1 | Not an APVTS parameter | No automation lane exists to record an arm and replay it |
| 2 | Not persisted in session state | A reopened session is *always* disarmed — the only guard covering a **realtime** bounce |
| 3 | `isNonRealtime()` structural bypass | Logic's Bounce and Bounce in Place |
| 4 | `processBlockBypassed()` disarms | A bypassed plugin that still folds |
| 5 | Editor destructor disarms | The fold cannot outlive its own warning banner |

Plus: refused in SAFE mode, refused on an unresolved monitor pair, and mutually exclusive with the
verify ping (each arms by disarming the other).

### Added

- **`Source/DSP/MonitorFold.h` / `.cpp`** — folds the eight solved feeds to a headphone pair using
  the venue's own measured geometry. Per speaker, about a listener at the audience centroid at
  `plane::earHeight()` on the raked plane: Woodworth inter-aural delay, constant-power lateral pan,
  `1/r` distance gain referenced to the mean speaker distance, and a head-shadow one-pole on the
  shadowed ear with a rear-darkening approximation of the pinna cue. **No new venue fields** — the
  venue schema stays at 2.
- **MONITOR banner** — a third frame-level banner beside SAFE and MAP, and deliberately the loudest
  of the three: *filled* with `--alert` rather than outlined, because the other two report what the
  host or the venue did, and this one reports something that could reach a delivered file. When the
  fold is suppressed for an offline render the copy says so, which is the reassuring half.
- **Monitor toggle** on the Venue rail, directly under Ping because the two are mutually exclusive.
- **`ochan::resolveMonitorSlots()`** — resolves the pair through `getChannelIndexForType()`, on the
  message thread, published as `VenueSnapshot::monitorSlot`. **Not speakers 1 and 2**: the monitor
  has to reach the physical outputs a headphone amp is plugged into, which under the measured
  CoreAudio device order are outputs 1–2 = `left`/`right`.
- **`tests/monitor-fold/`** — a standalone 15-check correctness harness for the fold, linking only
  three JUCE modules and the class under test.
- **Probes CY, CZ, DA, DB** in the render harness (61 total, all green).

### Fixed

- **The far ear was silent at ±90°, and it was found by the new harness rather than by ear.** An
  uncompressed constant-power pan drives the far ear to *exactly zero* at full lateral deflection,
  which took the inter-aural delay and the head shadow out of the signal path for precisely the
  lateral sources they matter most for — the fold had quietly degraded into a hard-panned stereo
  mix that happened to own eight delay lines. Fixed with `kPanDepth`, an ILD ceiling of ~17 dB (a
  real head is 15–20 dB at 90°, never infinite).

  Worth recording *how* it hid: every assertion that measured the **near** ear passed throughout.
  Only a check on the far ear could see it. The two assertions that had passed vacuously alongside
  it were then tightened from `>` comparisons to plausible **bands**, so neither the rail nor its
  absence can pass silently again.

### Notes

- **Latency is deliberately not reported.** The ITD lines add up to ~0.66 ms and
  `setLatencySamples()` is *not* called for them — reporting it would move host PDC and therefore
  change the render, which is the exact contamination the feature exists to be incapable of.
- **No HRTF convolution.** Scoped out as milestone-sized: it needs an embedded HRIR dataset
  (licence, megabytes of binary data, the `juce_add_binary_data` hyphen-stripping trap) and 16
  partitioned convolvers. A pure ITD/ILD model is front-back ambiguous by construction; the rear
  darkening is a hint, not a resolution.
- **Bit-identity with v1.6.0 is structural, not approximate.** Disarmed, `MonitorFold::isRunning()`
  is false and `GainStage` does not call `fold()` at all — nothing is clocked, no line is pushed,
  no filter advances. Probe CY asserts `monitorSamples == 0` across a full render, and probe CU's
  v1.4.0 digest still matches.

### Testing

- Render harness: **61 probes, 0 failures**.
- **Negative control run for CZ**, the primary-constraint probe: with guard 3 removed it fails with
  "OFFLINE RENDER WAS CONTAMINATED", while CY, DA and DB stay green — so CZ targets that guard
  specifically and is not decoration.
- `tests/monitor-fold`: 15 checks, 0 failures. ITD measures 31 samples @48 kHz for a source at 90°
  against a model maximum of 0.6558 ms (31.5 samples); ILD mirrors to ±16.9 dB.
- Static gates: `ui_frontend_check.js` 43/43, `ui_layout_check.js` 31/31, `check-i18n.js` all pass.
- `auval -v aufx OuOc OuDv` PASS · pluginval strictness 10 SUCCESS.
- **Not yet done:** hall/headphone listening, and the two manual render checks (bounce offline with
  the monitor armed; reload a session and confirm it comes back disarmed).

## v1.6.0 (2026-08-26)

**Hover help in English or French, and a settings gear to choose between them** — Stage C of the
repo-wide i18n rollout (quick task `260826-ieq`), and the plugin the rollout deliberately runs
second, because O-Octagon carries the strictest static gate in the suite: 43 sections, a native-fn
COUNT assertion, a ui-stub whitelist set-equality, and a module registry DERIVED from the contents
of `Source/ui/public/js/`. Adding a page module here is not a one-line change — it is designed not
to be. Everything below landed in one commit, because a partial one fails those gates on purpose.

### Added

- **`js/i18n.js` — all 53 hover-help strings, in English and French, in one place.** Every tooltip
  the plugin has ever shown lived as a `data-tip` / `data-tip-title` literal in `index.html`. The
  markup now carries **zero** copy; `applyI18n()` writes both attributes at page init out of a key
  table, and re-writes them on a language change. The measure-then-pin renderer is untouched — it
  still reads the same two attributes and does not know a table exists.

  The English was **moved, not rewritten**: every `en` entry is what v1.5.0 shipped, with its HTML
  entities decoded to the characters they named, because `setAttribute` + `textContent` do not
  decode entities.

- **A settings popover behind a gear in the header.** It carries a Language selector
  (English / Français) and the hover-help toggle. The `⚙` takes the exact 24 px circle the v1.2.0
  `?` button held, so the header silhouette is unchanged.

- **`getUiLanguage` / `setUiLanguage`** — the 24th and 25th native functions, exactly parallel to
  v1.2.0's tooltip pair. **PULLED once by the page at init, never pushed**: a push from the editor
  constructor or a poll tick fires before the page module has evaluated, so the preference would
  silently never arrive. No timer, no `poll().then(poll)`, no revision counter — the language is
  not preset content, and `OuariconPresetManager::loadPreset` walks `preset["parameters"]` only.

- **`uiLanguage` persists as a root XML attribute**, the idiom `tooltipsEnabled` already uses here.
  Held as `std::atomic<int>` behind a two-function codec, because `std::atomic<juce::String>` does
  not compile; persisted as the language CODE, so a session written today still means "French" if
  the codec ever gains a third entry. Restored through the same clamp, so a hand-edited or corrupt
  value degrades to English. Pre-1.6.0 sessions have no attribute and open in English.

### Changed

- **The hover-help toggle MOVED into the popover. It is not duplicated.** `#help-toggle` is gone;
  the control is now a segmented On / Off pair inside the panel — **two buttons, not one that
  relabels itself**, the same idiom the Venue screen's ms/m unit toggle uses, so both captions stay
  HTML-authored and this module still makes no `textContent` write at all (section 6).

- **The toggle's own tooltip is now ONE key covering both states**, where v1.2.0 had a single
  sentence and other plugins in the suite had a pair swapped on click. A state-dependent string
  written outside the table would be stranded in the previous language the moment the selector
  fires — and `window.__setLanguage()`, which drives the language from a test harness, raises no
  `change` event at all, so no listener workaround covers both paths. The caption and `aria-pressed`
  carry the state instead.

- **`initI18n()` is called from INSIDE `init()`.** Section 2 requires `init();` to remain the
  literal last statement of `app.js` with no module-level declaration after it, so the hoisted
  `import` is the only new top-level form. It is wrapped in its own `try/catch` like every other
  initialiser: a table typo must not take the 18 parameter bindings down, which is exactly what an
  eager top-level tooltip init did to a sibling plugin (`pattern_module_toplevel_init_tdz`).

- **Three gate literals moved, and all three are supposed to.** Section 3's native-fn count 23 → 25;
  section 9's embedded-file count 11 → 12; section 9's import scan now accepts single quotes as well
  as double, because the canonical i18n import line is single-quoted repo-wide and a double-only
  scan reported `i18n.js` as embedded-but-never-referenced — a false failure describing a page that
  does import it. `tests/ui-stub/juce-stub.js` learned the two new names in the same commit, which
  is what section 3's `setsEqual(stubbed, registered)` requires.

### Not changed, deliberately

- **`.tooltip { max-width }` stays at 240 px.** Raising it makes tips wider, which pushes the
  horizontal clamp toward failure while barely helping the vertical one. French wraps taller inside
  the existing cap instead, and the flip logic already handles taller tips — measured below.

### Verified

- All **43 sections** of `ui_frontend_check.js` pass (377 assertions), including section 2 with
  `init();` still last, section 3 at 25 registered functions with the stub whitelist equal to the
  C++ surface, and sections 9 and 21 with `js/i18n.js` in the derived set, in SOURCES and in a
  `getResource()` branch.
- All **31 sections** of `ui_layout_check.js` pass on the rendered page at 1100 × 720 — the gear did
  not disturb the layout it asserts.
- `node scripts/check-i18n.js` passes, 45 keys × {en, fr}.
- **Every anchor hovered and MEASURED in both languages** on both screens: 57 tips per language,
  all fully inside 1100 × 720 with an 8 px margin, the horizontal clamp engaging 18 times in each.
  French costs exactly **one** extra vertical flip (12 → 13) and no extra clamp. Nothing exceeds the
  240 px cap.
- The popover was proved **clickable, not merely present**: `elementFromPoint` at the centre of the
  gear, the selector and both toggle halves returns each control itself.
- Both CI probe targets green — geometry 49/49, render 57/57. `auval` PASS.
- `UIBinaryData::i18n_jsSize` == `wc -c js/i18n.js` == 30719.

### All 45 French entries are machine-drafted and flagged `reviewed: false`

No native speaker has read them. `node scripts/check-i18n.js` prints the worklist. The terms most
likely to want judgement are `rake` ("Inclinaison"), `hullAtten` ("Atténuation hors enveloppe"),
`rolloff` ("Atténuation") and `decorr` ("Décorréler").

### Known, and left alone

- The `preset-list` tooltip still says "the 17 parameters". v1.5.0 made it 18. The copy was moved
  verbatim by design — this release moves English, it does not rewrite it — so the stale count came
  with it. It is a one-word fix for whoever next touches that string.

## v1.5.0 (2026-08-26)

**A mono decorrelator behind the Width control** — the MEDIUM-value/small-effort gap from
`.planning/FEATURE-REVIEW.md`, and the limitation the v1.3.0 changelog named as a deliberate future
improvement. Width moved two sub-points apart in space but fed them the SAME SIGNAL: on a mono input
bus `in1` *is* `in0`, so the two feeds were bit-for-bit identical and two identical copies arriving
from two directions comb rather than widen. Most fixed-media stems are effectively mono, so this was
the common case, not the corner one. Width was doing geometry with no signal diversity behind it.

### Added

- **`decorr` — "Decorrelate", 0–1, default 0, in the Position group directly under Width.** The
  18th musical parameter. Each sub-point feed passes through its own network of four Schroeder
  all-pass sections with different, mutually incommensurate delays, so the two carry the same
  spectrum with unrelated phase. Measured cross-correlation at full depth: **0.06** (probe CV).

  **All-pass rather than velvet noise, for a reason specific to this plugin.** DBAP's
  constant-intensity claim (Σv² = 1, verified against an independent oracle to 1e-7) is a claim
  about the gain vectors, and it survives only while the stage in front of them leaves each feed's
  energy alone. An all-pass is unity-magnitude at every bin, exactly; a velvet-noise FIR is
  unity-power in expectation over its length. Measured chain gain: 0.9992 / 0.9998.

- **Depth scales the network's delay lengths, not a dry/wet mix and not the feedback.** A dry/wet
  blend of a signal with an all-passed copy of itself is a comb filter — infinitely deep nulls at
  50% — which is the defect this feature exists to remove. Scaling feedback turns each section from
  a diffuser into a slap. Delay length is the axis that behaves: dispersion runs 0 → ~22 ms (left)
  / ~26 ms (right), and at the bottom of the control both chains clamp to the same one-sample floor
  and **converge to the same filter**, so the effect fades out to a common phase colour rather than
  to a comb. Probe CV asserts that convergence bit-for-bit.

- **The delay reads are INTEGER, and that was a correction during development, not the first
  design.** The first implementation read at a fractional position with linear interpolation,
  copying the v1.4.0 alignment delay one class over. That destroys the all-pass property the entire
  design rests on: a linear interpolator is a two-tap FIR whose magnitude reaches `|1−2f|` at
  Nyquist, and inside a feedback loop it attenuates the recirculating signal and moves the pole.
  Measured chain gain was exactly unity at depth 1.0 — where `base × depth` lands on the integers
  the bases already are — and **3.2 to 4.6 dB down at every other depth**, with the summed pair
  reaching −6.94 dB against a coherent sum where incoherent addition predicts −3.01. Not
  decorrelation: cancellation, plus a lowpassed feed. Integer reads have no interpolator, so every
  section is exactly all-pass at every depth (measured 0.00 dB at all nine tested). The cost is
  that a delay steps by a sample as depth sweeps rather than gliding — inaudible here, unlike the
  alignment delay, because the jump is on the recirculating signal inside an all-pass rather than
  on an output lane at unity gain, and the delays are 56–521 samples long.

### Behaviour worth knowing about

- **Turning Decorrelate up costs about 3 dB where the two feeds overlap.** Measured −2.76 dB at
  full depth, −2.78 at half, −2.88 at quarter, against the −3.01 dB incoherent addition predicts.
  That is decorrelation doing its job, not a bug: where both sub-points feed the same speaker, its
  contribution goes from a coherent sum (`x`) to an incoherent one (`≈x/√2`). It is deliberately
  **not** compensated — compensating would boost the case where the feeds *don't* overlap. What you
  are trading is the combing.

- **It is inert at Width 0, and that is structural.** At `wEff == 0` the two sub-points coincide and
  their gain vectors are bit-for-bit identical (probe AY), which is what makes §3.4.3's degenerate
  path a clean mono sum. Decorrelating there would make that sum incoherent — 3 dB down and phasey —
  at the one setting where the plugin guarantees the arithmetic is transparent. So the gate is on
  the **effective** width, not on the parameter and not on `width`: the rFade collapse near the rig
  centroid drives the spread to zero without the parameter moving, and the decorrelator follows it
  down. Depth is scaled by the same spread, so the boundary is a fade rather than a switch.

### Compatibility

- **Every session and preset written before v1.5.0 renders bit-identically.** At `decorr = 0` the
  network is bypassed structurally — the inner loop reaches v1.4.0's literal expression, not an
  arithmetically equivalent one. Held two ways by probe **CU**: a render digest captured from the
  **v1.4.0 binary before a line of this feature existed** (`0xe25f022c8ce71dc9`), and an
  instrumentation counter asserting the network executed zero times. The digest is taken at width
  6 m off-centre with air up — every condition the decorrelator needs, with only the parameter at
  zero — so a gate wired backwards fails it.

- **No preset or session migration.** `decorr` is additive and joins `oo::presets::kPreserved`
  (11 → 12), so a factory preset load leaves it where the operator put it. All six factory presets
  are bit-unchanged. It describes the *material* — is this stem effectively mono? — not the room a
  preset is painting.

### UI

- Decorrelate sits directly under Width. That placement is the feature's discoverability: it does
  nothing on its own, so a user who widens a mono stem, hears combing rather than width, and goes
  looking for the cure finds it in the group they are already in.

- **The controls column tightened by 4 px per group.** The fifth Position cell opens a third grid
  row, and this column has no absorber — `.elev-stage` is `flex: none` at a fixed height and every
  group above it is content-sized — so the new row went straight into overflow (measured at
  scrollHeight 613 against clientHeight 592, both DPRs). Rather than take all 21 px out of the
  elevation strip, which is the one element here with legibility assertions against it, the cost is
  spread: 2 px off each group's top and bottom and 2 px off each column gap reclaims 34 px across
  six groups and five gaps. Horizontal rhythm is untouched.

### Tests

- **CU** — `decorr = 0` reproduces the v1.4.0 render digest exactly, and the network never runs.
- **CV** — the chains are all-pass **at nine depths** (worst 0.004 dB), decorrelating (r = 0.06),
  DC-transparent through a full depth sweep (3.0e-7), and convergent at depth 0 (bit-identical).

  **This probe's first draft measured depth 1.0 only and passed against the broken fractional
  read** — the bases are integers, so depth 1.0 is the single value in the range where that
  implementation needed no interpolation and was therefore correct. The defect was caught by
  measuring the *summed* level, not by the probe. The sweep now spans integer, half- and
  quarter-sample positions and asserts the gain at each; re-introducing the fractional read fails
  it at eight of the nine (verified by reverting and re-running, not assumed).
- **CW** — the wEff gate: at width 0, `decorr` 0 vs 1 is bit-identical; at width 6 m the same
  comparison must *differ*, or the control is wired to nothing.
- **CX** — QUAL-03 block-size invariance with the chains clocking and `decorr` swept across block
  boundaries at five block sizes.
- **CP** renamed and extended — the preserved set is twelve, and its liveness gate fired correctly
  on `decorr` sitting at its default the moment it joined.
- **AZ** — coverage accounting is now 17 of 18, with the eighteenth deliberately excluded: every
  section is an all-pass with `H(1) = 1` exactly, so AZ's DC construction would sweep `decorr` and
  measure a per-sample delta of zero — a vacuous pass dressed as coverage.

### Files

- `Source/DSP/Decorrelator.h` (new, no JUCE dependency so the narrow unit link line survives)
- `Source/DSP/GainStage.{h,cpp}`, `Source/DSP/DbapSolver.h` (instrumentation counter)
- `Source/PluginProcessor.cpp`, `Source/Data/PresetPolicy.h`, `Source/PluginEditor.{h,cpp}`
- `Source/ui/public/index.html`, `js/app.js`, `css/styles.css`
- `tests/render-harness/main.cpp`, `tests/unit/main.cpp`, `tests/ui_frontend_check.js`,
  `tests/ui_layout_check.js`, `tests/ui-stub/juce-stub.js`

## v1.4.0 (2026-08-26)

**Per-speaker alignment delay** — the HIGH-value/small-effort gap from `.planning/FEATURE-REVIEW.md`.
The venue model measured positions but the DSP only ever compensated LEVEL; on a deep hall with
three speaker pairs down the walls, arrival-time skew is audible, and every PA-world tool (L-ISA,
Soundscape, ordinary system processors) has this. It completes the venue-calibration story: 8 delay
lines and 8 new venue values.

### Added

- **A per-speaker alignment delay, 0–50 ms, stored in the venue beside the existing trims.** A new
  `Delay` column in the Venue table, applied post-solve on the eight output lanes. Venue-scoped, so
  a musical preset physically cannot reach it and no automation lane can touch it — the same
  guarantee the 42 measured values already had.

- **`Derive` — an auto-derived suggestion from the measured geometry.** Align-to-farthest against
  the audience-plane centroid:

  ```
  ref     = (centroid.x, centroid.y, earHeight(centroid.y))
  d_i     = |spk_i − ref|
  delay_i = (max(d) − d_i) / 343 m/s × 1000     → the farthest speaker gets 0 ms
  ```

  The reference seat is DERIVED, not stored, which is what keeps the schema addition purely
  per-speaker. Derive is a ONE-SHOT FILL, not a mode: it writes the eight stored values once and
  returns, and the operator then edits any of them freely. Moving a speaker later never silently
  rewrites a delay someone typed by hand.

  The arithmetic lives in `VenueModel::suggestedDelaysMs()` and is reached through one new native
  function (`applySuggestedDelays`, 22 → 23), shaped exactly like `applyOutputOrderPreset`. The page
  performs no speaker arithmetic — D19, and here it matters twice, because a JS copy of this law
  would be the same class of bug v1.3.5 just fixed.

- **A ms/metres toggle on the Delay column.** Storage is always milliseconds; the toggle converts
  what is displayed and what is parsed, in metres of PATH DIFFERENCE. The conversion divides by
  `speedOfSound` off the `getVenueGeometry` payload rather than a `343` written into the page — a JS
  literal would be a mirrored fixture over `oo::plane::kSpeedOfSoundMps`, free to drift until the
  column reads metres the Derive button disagrees with. The unit is a VIEW state and is deliberately
  not written to the `.venue` file: a room does not have a preferred unit, an operator does, and
  storing it would make two identical rooms compare unequal.

### Changed

- **`.venue` schema 1 → 2**, additive: `@delayMs` on each `SPEAKER`. The loader does not branch on
  the version and still does not need to — `readFromState()` defaults every absent attribute
  individually, so a schema-1 file, a session from any earlier build, or a `.venue` carrying no
  `@schemaVersion` at all yields `delayMs = 0` for all eight. **Those rooms render bit-identically
  to v1.3.5**, and that is structural rather than approximate: a zero delay is bypassed outright, so
  the write is the literal v1.3.5 expression with no delay line touched. Probe CP asserts it against
  a constructed schema-1 file, loaded into a model that already held nonzero delays.

- **The venue is 50 values, not 42** (8 × 6 + 2). Every count that names it moved with it: the
  session tree, the `.venue` file, `getVenueGeometry`, `setVenue`, the ui-stub, and the four gates.

### Technical

- **Eight separate mono `juce::dsp::DelayLine` instances, not one 8-channel instance.** The same
  trap `airL`/`airR` already document, in a different class: JUCE keeps `delay`, `delayInt` and
  `delayFrac` as PER-INSTANCE members and only the buffers per channel, so one 8-channel instance
  would carry eight histories sharing ONE delay time — silently correct while every speaker agrees
  and silently wrong the instant two differ, which is the only configuration this feature produces.

- **Read through `popSample (ch, d)` at a smoothed position, not `setDelay()` at the boundary.**
  Eight `SmoothedValue` ramps on the same 5 ms as the gains, targets set at the 64-sample control
  boundary off the absolute counter and `getNextValue()` called once per sample unconditionally in
  BOTH mode arms — so QUAL-03 block-size invariance holds by the same mechanism as the seventeen.
  Probe CT renders eight lanes delayed 1.50–24.25 ms at block sizes 512 and 4096 and requires a
  bit-identical memcmp.

- **The ms → samples conversion is done in double and cast once.** The obvious
  `ms * 0.001f * (float) sampleRate` is wrong by an ulp at exactly the values operators type: 10 ms
  at 48 kHz comes out 480.0000305, so `delayFrac` is 3.05e-5 instead of 0 and `popSample`
  interpolates — a lowpass and a sub-sample error on every sample, measured at 1e-5 against a signal
  whose per-sample slope is 0.33. Probe CS found it by failing a bit-compare that shift 480 otherwise
  matched to five decimal places. In double, every round millisecond at every standard rate lands on
  an exact sample count.

- **All eight lines clock together or none do.** A line pushed without being popped walks `writePos`
  away from `readPos` and its delay grows by a sample per sample, so "push always, pop sometimes" is
  not available. Clocking none until one delay is wanted is what makes a venue with no delays cost
  nothing; clocking all eight once any is wanted keeps the other seven warm, so every adjustment
  after the first glides instead of dropping out. The one cold start is preceded by a `reset()`.

- **The verify ping stays undelayed.** The delay lands before the NaN guard and before the ping's
  overwrite. The ping already bypasses DBAP, the weights, the hull trim, the air filter, the
  per-speaker trim and `outputGain` so that a ping from the wrong speaker has exactly ONE possible
  cause — the map. A delayed ping would add a second.

- **No `setLatencySamples()`.** The existing prohibition stands and now has a second reason: the
  delay is per-CHANNEL and host PDC is per-plugin, so no single number could be honest. Alignment
  delay is an acoustic correction, not a processing latency.

- **The 50 ms rail has one definition and three aliases.** `oo::plane::kMaxAlignDelayMs`, aliased by
  `VenueModel::kMaxSuggestedDelayMs` (the suggestion clamps to it), `GainStage::kMaxAlignDelayMs`
  (the lines are sized for it) and `OOctagonProcessor::kVenueDelayClampMs` (the funnel rails to it) —
  the move `VenueModel::kMinSpan` already makes. Three literals policed by `static_assert`s was the
  first attempt; it worked and emitted `-Wfloat-equal` on every build, because comparing two floats
  for equality is the smell the warning names even when they are constants. One definition needs no
  comparison.

- **Delays are sanitised at `publishSnapshot()`, the single funnel**, `sane()` before `jlimit()` as
  the trim is: NaN survives `jlimit` unchanged (both comparisons are false for NaN) and would reach
  `setTargetValue` and latch the smoother — RESEARCH-2.2's H2 latch through a third door. The lower
  rail is not decoration either: a negative delay would index before the write head, which
  `juce::dsp::DelayLine` does not range-check.

### Testing

- **Unit (49 probes, 0 failures).** New: **CP** schema-1 `.venue` loads with delays at zero and the
  other 42 values intact, with the file's lack of `@delayMs` asserted first so the probe cannot pass
  vacuously; **CQ** the align-to-farthest law including its DIRECTION — delay is monotonically
  decreasing in distance, which `d_i / c` fails on every pair; **CR** the suggestion scales linearly
  with the room and a collapsed rig yields eight exact zeros rather than eight NaNs.
- **Render harness (53 probes, 0 failures).** New: **CS** one delayed speaker moves EXACTLY one lane
  by EXACTLY 480 samples, bit-exactly, with a bit-silent head — and the other seven lanes stay
  bit-identical, which is what stops the zero-delay half passing with the feature deleted; **CT**
  block-size invariance with all eight lines clocking.
- **Frontend static (43 sections).** New §43: the page owns no speed of sound and no 50 ms literal,
  and the two conversions are one multiply and one divide. §30 gained the delay-before-ping ordering.
- **Layout, Playwright (31 sections).** §12 confirms all 50 fields are present, editable, populated
  and fully inside 1100 × 720 — the new column fits without touching the window size.

## v1.3.5 (2026-08-26)

**MEDIUM-03** from `.planning/SIMPLIFICATION-AUDIT.md` — the one item in that audit that corrects
behaviour rather than shape. Batch B's remaining four (MEDIUM-01, 02, 04, 05) stay skipped; Batch C
(MEDIUM-06, MEDIUM-07, LOW-08) stays deferred.

### Fixed

- **`getFieldGrid`'s `blur` fallback was the pre-v1.3.0 default.** `PluginEditor.cpp`'s field-backdrop
  handler read its three solve inputs through a `readParam (id, fallback)` helper whose fallbacks were
  transcribed literals — `4.0f` / `0.1f` / `1.0f`. `blur`'s live default moved `0.10 → 0.03` in v1.3.0
  when `kBlurScale` tripled (`0.5 → 1.5`, `DbapSolver.h`) so that `blur = 1` is a true wash; the copy
  in the editor did not follow it and had been stale for two minor versions.

  **Root cause:** a default stated in two places. `PluginProcessor.cpp:106` owns it in the APVTS
  layout; the editor kept a second copy with nothing tying them together. This is the repo's own
  `pattern_test_fixture_mirrors_drift_silently` firing in production code rather than in a fixture.

  **Fix:** `readParam` now takes only an id and derives the fallback from the parameter itself —
  `getParameter (id)->convertFrom0to1 (getDefaultValue())`, the identical derivation
  `getParameterDefaults` (line 284) already uses for the dblclick-reset payload. All three literals
  are gone and the default is stated once, in the layout.

  **Reachability:** the fallback fires only when `getRawParameterValue` returns null (impossible for a
  valid id) or the atomic is non-finite (host wrote NaN). No reachable path changes value — on the
  NaN path the field backdrop now falls back to `0.03` instead of `0.10`, which is a correction, and
  `rolloff` / `hullAtten` were already correct at `4.0` / `1.0` and are unchanged.

### Testing

- Zero-warning gate: `-Wshadow-uncaptured-local` was the live hazard here (the enclosing lambda
  already renames its loop local to `weightParam` for exactly this reason). The new `param` /
  `fallback` locals shadow nothing — the enclosing constructor scope declares only `options`.
- Render goldens untouched: `PluginEditor.cpp` is excluded from the offline harness, and no static
  gate parses these literals.

## v1.3.4 (2026-08-26)

Phase 3 sweep from `.planning/SIMPLIFICATION-AUDIT.md` — **6 of 7 approved LOW-tier candidates
applied**, one reverted as a false positive. Batch B (MEDIUM-01…05) was skipped by choice and
Batch C (MEDIUM-06, MEDIUM-07, LOW-08) deferred; all eight remain in the audit.

### Changed

- **LOW-01** — `commitScenes()`. `sceneStore.writeToState (apvts.state)` followed by
  `++scenesGeneration` was three hand-kept copies (`captureScene`, `scenesFromVar`,
  `setStateInformation`). The two lines are one invariant — `scenesGeneration` is the only signal
  telling the page its cached slots are stale — so a fourth scene-mutating path that wrote without
  bumping would leave the UI showing scenes the plugin no longer holds. **The constructor's write at
  `PluginProcessor.cpp:178` is deliberately NOT a call to it:** it seeds the `SCENES` node at birth
  (N13) so a pre-`prepareToPlay()` save carries a complete tree, there is no cache to invalidate
  yet, and `scenesGeneration` starts at 1 — bumping there would be a behaviour change, not a dedup.
  The audit missed that fourth site.
- **LOW-02** — deleted `.vfield-label { text-transform: none; }`. No ancestor of the label inputs
  sets a transform, so `none` was already the computed value.
- **LOW-03** — `sliders.set(id, { state })`. The `input` and `value` nodes stored alongside were
  dead payload suggesting a wider contract than exists; every consumer in all four files reads only
  `.state`, and `bindSlider`'s own closures still capture the nodes for rendering.
- **LOW-04** — `FIELD_INPUT_IDS = [...WEIGHT_IDS, "rolloff", "blur", "hullAtten"]`, replacing a
  transcription of the eight weight ids `WEIGHT_IDS` already declares 59 lines above.
- **LOW-05** — `setMeters()` now calls the `clamp01` its own file defines at line 197 instead of
  re-inlining `Math.min(1, Math.max(0, …))`.
- **LOW-07** — `replaceChildren()` for the three `while (firstChild) removeChild` loops
  (`venue.js` preset list, `elevation.js` axis and speaker groups).

### Reverted — LOW-06 was a false positive

The audit proposed deleting seven single-use alias consts in `venue.js` (`const value = pingStateNode;`
before `value.textContent = …`), rationale *"the aliases add a line per site and no meaning"*, test
impact *"None — purely mechanical"*. Both are wrong: **those aliases are the mechanism by which
`venue.js` satisfies `ui_frontend_check.js` section 6**, which guards
`pattern_js_state_updater_overwrites_html_labels` by whitelisting textContent *receiver identifier
names* and pairing each with a companion assertion about what it binds to. Removing them moved the
receivers to `classNode`, `venueNameNode`, `presetCurrentNode`, `pingStateNode` and `ooStateNode` —
all genuinely dedicated value nodes, none on the list — and section 6 failed.

Making it pass would mean either loosening a deliberately short whitelist or writing five new
companion assertions, in exchange for seven lines of nit-tier cosmetics. Reverted instead; the item
stays in the audit **reclassified as a false positive**, not as pending work.

### Verification

- **Rendered-DOM + computed-style snapshot, byte-identical.** `#plan-geometry`, `#mini-geometry`,
  `#elev-strip`, `#preset-list`, `#readout-metres`, `#readout-envelope`, `#ping-state`, the venue
  table, and the computed `text-transform` of all eight label inputs, captured from the ui-stub at
  1100 × 720 across five states (both screens, a venue edit, and a drive of
  srcX/srcY/srcZ/blur/rolloff). sha256 `69227ed4…d0ae47` before and after, 0 console errors. The
  harness was first shown **deterministic across two consecutive baseline runs**, so the diff means
  something.
- **Negative control on that snapshot.** Re-adding `.vfield-label { text-transform: uppercase; }`
  and dropping one `replaceChildren()` call changes 80 computed-style lines and grows the
  `#elev-axis` child list; restoring gives byte-identity again.
- **LOW-05 clamp equivalence, 126 comparisons.** The stub's peaks reset to 0 with no ping running,
  so the snapshot exercises `setMeters` only at zero. The clamp is proven separately over `NaN`,
  `±Infinity`, `-0`, `1e308`, out-of-range, strings, `null`/`undefined`, arrays and objects, on the
  full `Number(x?.[i]) || 0` expression — plus an assertion that the shipped `clamp01` declaration
  is character-for-character the expression that was deleted.
- `tests/ui_frontend_check.js` 42 sections PASS (exit 0) · `tests/ui_layout_check.js` 31 sections
  PASS · render harness 51 probes / 0 failures · geometry target 46 probes / 0 failures · `auval`
  PASS · VST3 + AU installed at 1.3.4.

**A coverage gap found while negative-controlling LOW-01, and NOT introduced by it:** deleting
`++scenesGeneration` from `commitScenes()` outright leaves all 51 render-harness probes green.
CK and CL round-trip the scene *state* but nothing observes `getScenesGeneration()`, so the
generation counter has no probe at all. LOW-01 is therefore verified by the call-site diff and the
compiler — the three sites now call one function holding the same two statements in the same order —
and not by a test. Worth a probe; filed as follow-up rather than folded into a cosmetic sweep.

## v1.3.3 (2026-08-26)

Applies **HIGH-01** from `.planning/SIMPLIFICATION-AUDIT.md`. No behaviour change: the rendered DOM
is byte-identical, proven rather than asserted (below).

### Changed — the mini-plan and the Room plan now share their hull and glyph drawing

`roomplan.js`'s charter since Phase 3.2 has been *a second view, never a second projection* (Q8),
and section 19 of the static gate enforces the projection half of it. Only the projection half was
actually true. `metresToPx`, `fitBox` and `makeView` were shared; the six lines that turn a hull
into an SVG `points` string and the three `classList.toggle` calls that carry
`VERTEX` / `ON_EDGE` / `INTERIOR` onto a glyph had been copy-pasted into `venue.js`'s `drawMini()`.
Two copies of one rule is the drift class this plugin's own fixtures have been caught by
(`pattern_test_fixture_mirrors_drift_silently`) — a fourth classification class, or any change to
the points format, had to be made twice and would have drifted the first time it was not.

Extracted into `roomplan.js` beside the three functions already shared:

- `hullPoints(hull, view)` — the points string.
- `placeGlyph(g, s, view)` — the `translate()` transform and the three class toggles.

`drawGeometryLayer()` and `drawMini()` both call them. `venue.js` no longer calls `metresToPx` at
all, so section 19 is **widened** by this rather than weakened: there is one fewer call site that
could stop routing through the one projection.

**The v1.1.0 output badge deliberately stays in `roomplan.js`.** `drawGeometryLayer()` also writes
`gout-N`, and the mini plan has no DOM node for it. Pulling the badge into the shared helper would
mean a null-node branch on the mini's behalf — one more thing to keep true, not one fewer — so the
shared part is scoped to hull + transform + toggles and the badge is rendered by the caller that has
somewhere to put it.

The helper had to land in `roomplan.js` and could not go the other way: section 32 of
`ui_frontend_check.js` bans the `VERTEX` / `ON_EDGE` / `INTERIOR` vocabulary from the module that
resolves a scene, and section 19 requires the single-projection module.

### Verification

Byte-identity of the DOM is the whole claim, so it was **measured on the rendered page**, not argued:

- **Rendered-DOM diff.** `#plan-geometry` and `#mini-geometry` `outerHTML` captured from the
  ui-stub at 1100 × 720 across four states — default venue, speaker 1 moved to x = −4.00, speaker 4
  driven to the origin, and the Room screen re-laid-out after both edits. Before and after the
  change the dump is byte-identical: sha256 `fd73cf31…649b8b` both times.
- **Old-vs-new equivalence probe, 297 comparisons, all identical.** The ui-stub's fixture carries
  fixed classifications and only ever produces `VERTEX` and `ON_EDGE`, so the rendered diff alone
  leaves the `INTERIOR` branch unobserved. The probe compares the new exports against a verbatim
  copy of the pre-change inline code from `backups/O-Octagon/v1.3.2/`, over all three classes plus
  an unknown one, on fresh *and* pre-dirtied glyphs (the on→off toggle direction a fixed fixture
  never exercises), and with hulls carrying negatives, `0.1 + 0.2`, `1e-7`, `-0.0` and `1e21` —
  float formatting is the thing under test, so the values are deliberately not tidy. It compares the
  full call *sequence*, not just the end state.
- **Negative control.** Mistyping one toggle's literal (`ON_EDGE` → `ONEDGE`) in the new
  `placeGlyph` turns those 297 comparisons into 48 mismatches. The probe fails when the thing it
  claims to check is broken.
- `tests/ui_frontend_check.js` — ALL SECTIONS PASS, 42 sections (exit 0).
- `tests/ui_layout_check.js` — ALL SECTIONS PASS, 31 sections.

No C++ changed and no render golden is exposed.

## v1.3.2 (2026-08-25)

Resolves the five Warning findings from the v1.3.0 code review (`CODE_REVIEW.md` WR-01 … WR-05).
Every one was re-read from disk and confirmed still present before it was touched, and every fix
carries a probe that was **negative-controlled** — reverted, re-run, and observed to fail.

### Fixed — WR-01: `setStateInformation()` raced the audio thread with a double publish

`VenueSnapshotPublisher` is a 2-slot double buffer. `publish()` always writes `1 - activeSlot` and
nothing tracks which slot a *reader* holds; `processBlock()` binds the active slot **by reference**
once and holds it for the whole callback. One publish inside that window is safe — it writes the
other slot. Two are not: the second computes `1 - (the slot the first just activated)` and lands
squarely in the slot the audio thread is reading. That is a data race on ~276 bytes of non-atomic
floats and ints.

`setStateInformation()` did exactly that: `readVenueFromState()` published, then
`rebuildChannelMap()` published again microseconds later — inside a ~10.7 ms block at 48 k/512. A
host preset switch or session restore with the transport rolling runs that concurrently with
`processBlock()`, and JUCE's VST3 and AU wrappers add no guard.

**Fix:** `readVenueFromState (bool publish = true)`. `setStateInformation()` passes
`! preparedYet` — suppressing its publish *only* when the `rebuildChannelMap()` at the bottom of the
function will run. The condition has to match that one: a host that restores before
`prepareToPlay()` skips the rebuild, and an unconditional `false` would leave the restored geometry
unpublished. `prepareToPlay()` passes `false` too, for symmetry; audio is suspended there, so that
pair was never the reachable one.

**The seqlock was considered and deliberately not taken.** The review proposed it as the durable
fix, on the reasoning that "any two publishes straddling one block still race — e.g. a dragged rake
control committing venue edits at mouse-move rate". That reachability claim is wrong, and
verification said so: `venue.js` has exactly one `setVenue` call site and it commits on blur/Enter,
not on drag; the Room-plan puck writes APVTS parameters and never the venue; every C++ UI entry
point funnels through `applyVenueEdit()`, whose `if (preparedYet) rebuildChannelMap(); else
publishSnapshot();` is an either/or by construction. `setStateInformation()` was the only reachable
double-publish path, so suppressing that publish closes it. A seqlock (or a 3-slot buffer with a
reader-claimed index) changes the audio thread's read path, and no second caller justifies that risk
today — the reasoning is recorded at `readVenueFromState()` so that whoever adds one adds the
seqlock with it.

**Probe:** render harness `CR setstate-publishes-once`. The property is a *count*, so the probe
counts it — `getVenueGeneration()` advances once per publish. Both branches are driven, because the
suppression is conditional: prepared expects exactly +1 *and* the venue to arrive; unprepared
expects exactly +1 for the opposite reason. Negative-controlled twice — with no suppression the
prepared branch reads `2 publish — RACY`; with unconditional suppression the unprepared branch reads
`0 publish — SUPPRESSED TOO FAR`.

### Fixed — WR-02: the DPR watch was hooked to an event that cannot fire for it

`roomplan.js`'s comment stated the requirement exactly — "a window dragged between a Retina and a
non-Retina display changes DPR without changing any CSS size" — and then hooked it to
`window.addEventListener("resize", …)`. The editor is a fixed 1100 × 720 non-resizable surface
(`setSize` is the only sizing call; there is no `setResizable` and no constrainer anywhere in
`Source/`), so the CSS viewport never changes, and a backing-scale change with an unchanged CSS
viewport dispatches no `resize` in either engine. `dprWatch` was the only post-construction writer of
`dpr`, and `resizeCanvas()` reads that module-level value — so every other relayout path re-ran with
the stale number and none of them healed it. Non-Retina → Retina left a 1× backing store upscaled 2×:
a visibly soft Room plan.

**Fix:** arm `matchMedia("(resolution: Xdppx)")` at construction and **re-arm it on every fire**. Such
a query is one-shot by nature — written against the current ratio, it stops matching the moment the
ratio moves and never fires again. The listener detaches before re-arming on both the modern and the
deprecated surface. The `resize` listener stays as a belt-and-braces path for genuine viewport
changes.

**Probe:** `ui_layout_check` section 29. Section 21 already measured DPR 1 and DPR 2, but in two
*separate browser contexts* — construction-time DPR only, which cannot see a live transition.
Section 29 substitutes `devicePixelRatio` and `matchMedia` before the page's own scripts run, fires
the query the way a UA does, and measures the rendered backing store. The clause that makes it
non-vacuous is `0 resize events dispatched in the whole run` — without it the old code passes too.
A third clause asserts the re-arm, which is the one-shot trap.

### Fixed — WR-03: `readFloat()` accepted garbage as 0.0 and `"nan"` as a real NaN

The guard tested **presence** only, and the conversion that followed had no numeric-validity and no
finiteness check. `<VENUE rakeFront="tall">` loaded `0.0` — a flat audience plane — instead of the
§OQ4 default of 1.10, directly against the header's "never zeros" contract, because
`String::getDoubleValue()` returns 0.0 with no leading number. And JUCE's parser recognises the
literal words `nan` and `inf`, so `<SPEAKER x="nan">` loaded a real NaN.

This is the *normal* conversion path, not an exotic one: `getStateInformation()` serialises via
`createXml()`, so on restore every property in the tree is a string var. Two doors carry
user-controlled text in — `setStateInformation()`, and `VenueFile::load()`, whose validation is
structural only, on a file format `VenueModel` explicitly supports hand-editing.

The audio thread was never at risk — `publishSnapshot()` sanitises every field it copies — but the
message-thread model feeds the UI readouts and the Room plan, and `toValueTree()` writes the value
straight back out. `juce::String(double)` renders a NaN as `"nan"`, which `readFloat()` then re-read
as a NaN: the corruption closed a loop across saves. The project had already shut the other three
doors with this exact idiom (`finiteOr()` on the UI write path, `isfinite`/`jlimit` on scene weights,
`publishSnapshot()` on the audio path); the file/session door was the one left open.

**Fix:** validate before converting. A string var must be *complete* numeric text — optional sign,
digits with at most one decimal point, optional exponent, nothing trailing — and the converted value
must be `std::isfinite`. Either failure returns the per-attribute fallback. No plausibility clamp is
applied here: `readFloat()` is generic and does not know whether it is reading a metre, a decibel or
a rake, so per-field rails stay where they already are.

**Probe:** geometry target `P2 venue-nonnumeric-attrs`, in three parts — garbage/`nan`/`inf`/trailing
garbage all fall back per attribute with the model still `allFinite`; a **negative control** that the
spellings a real file carries (leading whitespace, `1.5e1`, `-2.5E-1`, `+3`) are still *read*, so the
validator cannot silently become data loss; and a save/reload round trip, because a single read
cannot show stickiness. Negative-controlled: with the guard reverted the first and third clauses fail
and the second still passes.

### Fixed — WR-04: the footer metres readout only followed the puck

`renderMetres()` is a pure function of `srcX` and `srcY`, but its only live-update wiring was
`roomPlan`'s `onSourceMoved` callback — which `roomplan.js` calls from exactly one place, the puck's
`pointermove` handler. Dragging `ctl-srcX` / `ctl-srcY`, stepping them from the keyboard, their
dblclick reset, and host automation all moved the puck and updated `val-srcX` while the footer went
on showing a plausible **wrong** position in metres, on the plugin's only metres readout. `index.html`'s
Source X tooltip promises "the metres readout below is live"; dragging that exact slider was the case
where it was not. `elevation.js` already subscribes its marker to this echo — the footer was the one
module out of step with its siblings.

**Fix:** subscribe `renderMetres` to the `srcX` / `srcY` `valueChangedEvent`, the same idiom the
`FIELD_INPUT_IDS` loop directly above already uses. Render on the echo, never write on it.

**Probe:** `ui_layout_check` section 30, driven with a **key on the slider**, never the puck — a
pointer gesture on the puck is the one path that always worked. Non-vacuity is `val-srcX` moving,
which proves the stimulus reached the echo. Negative-controlled: with the subscription reverted the
readout reads `"12.50 × 15.10 m" -> "12.50 × 15.10 m"`.

**This fix also exposed a probe that was measuring the bug.** Section 14's metres clause read its
"before" value from a footer WR-04 had left stale, so the venue edit was simply the first thing to
recompute it — "the readout changed" was reporting the staleness, not the edit. With the footer live
the clause failed, correctly: the source sits at normalised 1.0, where `metres.x` *is* the bbox max
rail, and the edit moves a min rail. The mapping is `min + n·(max − min)`, so a min-rail move of Δ
shifts the readout by `(1 − n)·Δ`, which at `n = 1` is exactly zero. Section 14 now parks the source
in the interior first, where the mapping is sensitive to both rails, and restores it afterwards so
its fixture does not become a later section's hidden precondition.

### Fixed — WR-05: a plugin-rejected venue commit left the typed text on screen

`commit()` fires `setVenue` fire-and-forget and clears `pending` synchronously. When
`applyVenueEditChecked()` refused, the JS added an `is-colliding` mark and nothing else — the typed
text stayed in the input while `pending` no longer held it, so the model's value for that field was
the old committed one and the *next* commit would silently send it. Nothing healed the desync: a
rejected `setVenue` never reaches `applyVenueEdit()`, so it never publishes and never bumps
`venueGen`, and `paintFields()`'s only other caller is the `venueGen`-gated refresh. A tab switch
redraws the mini-plan only.

The unbounded case is the reachable one. In SAFE mode — mono or stereo output, a supported AU
configuration — `buildSpeakerToBuffer` fails with `notEightChannels` on *every* commit, the venue
inputs are not disabled, and `speaker` is `-1`, so not even the `is-colliding` mark appears. No
commit can ever succeed there, so the operator was reading room measurements the plugin does not
hold, with zero feedback, in a live-hall calibration tool.

**Fix:** `paintFields()` in the `ok: false` branch. It skips any field in `pending` and the focused
field by design, and `commit()` is only ever reached from a blur handler, so the field just edited is
no longer focused. This is the C++-reject path **only** — the JS-detected collision path returns
before `pending.clear()` and keeps its hold-and-mark semantics, because reverting there would make an
L ↔ R swap unreachable.

**Probe:** `ui_layout_check` section 31, with a new `rejectVenueWrites()` stub hook that arms the
backstop's refusal in its SAFE-mode shape (`speaker: -1`) — the stub's own label check agrees with
the page by construction, so that branch had no natural lever. Clauses: the page asked exactly once
and sent the typed value; the refusal moved nothing downstream; the field went back to what the
plugin holds; and with `speaker = -1` there is no mark to fall back on, so the repaint *is* the
feedback.

**The first version of this probe was decoration and the negative control caught it.** `resetVenue()`
in the preamble bumps `venueGen`, and the refresh it triggers arrives up to a poll period later — it
was landing *after* the rejected commit and repainting the table for entirely the wrong reason.
NC3 came back `ALL SECTIONS PASS` with the fix reverted. Section 31 now quiesces on the envelope and
the field before anything is typed, and asserts that quiescence as a stated precondition.

### Changed — the "no state in a completion" static rule was narrowed, not dropped

`ui_frontend_check` section 25 (N4/P64) rejected any `paintFields()` inside a `.then()`. The rule
conflated two different things. `committed = …` **establishes** state: if the completion is dropped
the page never learns the truth, which is the whole content of N4/P64. `paintFields()`
**re-renders** state the `venueGen` poll already established: if that completion is dropped the page
is left exactly where it already was. Only the first is a convergence hazard, and WR-05 needs the
second, because on a refusal the refusal is the only event there is.

The rule is now two checks: no `.then()` may assign `committed` (unchanged strength), and any
`.then()` that repaints must be inside the `ok: false` refusal branch. A `.then()` that repainted
unconditionally still fails.

### Verification

| Gate | Result |
|---|---|
| `tests/render-harness` | 51 probes, 0 failures (was 50) |
| `tests/unit` geometry target | 46 probes, 0 failures (was 45) |
| `tests/ui_layout_check.js` | 31 sections pass (was 28) |
| `tests/ui_frontend_check.js` | 42 sections pass |
| `auval -v aufx OuOc OuDv` | AU VALIDATION SUCCEEDED |

Six negative controls run, each by swapping the file back and rebuilding — never `git checkout`,
which would have taken the uncommitted fix with it:

| Control | Expected failure | Observed |
|---|---|---|
| NC1 `roomplan.js` reverted | §29 | 3 clauses fail, backing store `448 -> 448` |
| NC2 `app.js` reverted | §30 | readout unchanged across the nudge |
| NC3 `venue.js` reverted | §31 | field still shows `-7.25` |
| NC4 `readFloat` reverted | `P2` | guard + round-trip fail, neg-control half passes |
| NC5 suppression unconditional | `CR` | unprepared `0 publish — SUPPRESSED TOO FAR` |
| NC6 no suppression | `CR` | prepared `2 publish — RACY` |

---

## v1.3.1 (2026-08-25)

### Fixed — all eight level meters rendered 507 px off their speakers (CODE_REVIEW CR-01)

**Root cause (measured in both engines, not inferred):** `.meter-arc` and `.meter-peak` set
`transform-origin: center` with no `transform-box`. The default `transform-box` for an SVG element
is `view-box`, so `center` resolves to the centre of the *plan* viewBox — `224px 280px` — measured
in each glyph's own translated local space. Both meter elements sit inside a
`<g transform="translate(x y)">` and carry no `cx`/`cy`, so their local origin already *is* the
speaker centre; naming `center` pointed the rotation at a point half a plan away. Every arc was
rotated about that point and landed 507.1 px from its speaker, and the peak tick — rotated by
attribute from `js/roomplan.js` — orbited 28–696 px off-plan instead of sitting 15 px out on the
arc.

The original review filed this as Windows-only, on the theory that WebKit resolves SVG
`transform-origin` fill-box-like. It does not: re-measured on the real page under Playwright
WebKit 26.5 (the WKWebView engine), the numbers are byte-identical to Chromium's. **The shipped
macOS build was equally broken**, and UI-03 — which this plugin's own docs call the second human
line of defence on the channel map, against a top risk (R1) that fails *silently* — has been 100%
non-functional since Phase 3.3.

- **`.meter-arc` / `.meter-peak`: `transform-origin: center` → `0 0`.** `0 0` names the
  glyph-local origin outright, is the SVG initial used value, and needs no `transform-box`
  override, so it resolves identically in Chromium and WebKit. `transform-box: fill-box` is the
  other available cure for the arc but is *wrong* for the tick, whose own fill-box centre is
  (0, −15) — it would spin the tick on its own midpoint instead of sweeping it round the arc. One
  origin, stated once, for both rules.
- Measured after the fix, per glyph, in Chromium 141 **and** WebKit 26.5: arc rotation origin and
  arc ink both land on the speaker dot to 0.000 px, and all eight peak ticks orbit at exactly
  15.00 px.

### Fixed — the gate that certified the bug (`tests/ui_layout_check.js` section 23)

Section 23 is titled "eight meter arcs, at their glyph positions" and never measured a position.
It asserted DOM parentage, the `r` attributes and the `stroke-dasharray` attribute — all of which
were correct while the arcs rendered half a plan away — so it returned **ALL SECTIONS PASS** on
the broken build. This is the repo's own recorded *"a probe that passes BOTH ways is decoration"*
pattern, and it is why the defect shipped.

- Section 23 now measures **rendered geometry**: `getScreenCTM()` for where each arc's rotation
  origin and each tick's midpoint actually land, and `getBoundingClientRect()` for where the arc's
  ink actually lands. Seven new clauses, all eight speakers each.
- Expectations are derived from the page, never mirrored as literals: the tick's orbit radius
  comes from its own `y1`/`y2` midpoint (cross-checked against the arc's `r`), and its predicted
  landing point from the live `rotate()` attribute `js/roomplan.js` wrote.
- A **non-vacuity clause** guards the set: the eight ticks are driven to eight distinct peaks and
  must span > 90° of sweep, because a tick resting at 0° sits where it was authored and cannot be
  displaced by a wrong origin — a flat set would prove nothing.
- **Negative control run before the gate was accepted:** with `transform-origin: center` restored
  and the new gate in place, section 23 fails 5 clauses / exit 5, reporting the 507.1 px arc
  displacement and 65–706 px tick radii — while the three original attribute-only clauses still
  pass green, which is the decoration demonstrated rather than argued.

**Testing:** `ui_layout_check` 28 sections and `ui_frontend_check` 42 sections green with the fix;
5 failures with the CSS reverted. UI/CSS only — no parameter, no DSP, no state-format change;
presets and sessions are unaffected.

## v1.3.0 (2026-08-20)

### Changed — srcZ / rolloff / width / blur made audibly effective (the flat-field fix)

**Root cause (measured, not guessed):** the default rig hangs speakers at 4.50–5.40 m while the
source rides the 1.10–3.20 m ear plane, so every source→speaker distance carries a ~3 m constant
vertical offset. That offset compresses the distance ratios DBAP feeds on: even with the puck
parked next to a speaker, the max-to-min channel spread was only **8.5 dB** at defaults — and
Σv² = 1 normalisation removes every overall-level cue. All four reported controls only reshaped
that already-flat field: full blur sweep moved the spread ~2 dB, srcZ ~1–2 dB per channel (and
zero overall), width ≤ 2.5 dB between sub-point vectors (and 39% suppressed by the centroid fade
at the DEFAULT puck position), rolloff's exposed 3–6 dB/2x range mapped to exponents a = 0.5–1.0 —
the gentle half of DBAP's useful range.

- **Source Z — proximity level cue** (GainStage): each sub-point's gains are trimmed by
  `(invK_z / invK_0)^2.5`, clamped ±6 dB, where `invK` is the un-normalised DBAP field `1/k = √denom`
  the solver already computes and `invK_0` is the identical solve with the height offset stripped.
  At srcZ = 0 the two solves have identical inputs, the cue is exactly 1.0, and the pre-1.3.0
  arithmetic is preserved bit-for-bit. Rising toward the speaker plane now gets louder and sharper
  (+4.5 dB measured at the front-left puck); flying above the array recedes (−2 dB at +8 m; −3 dB
  sunk to −2 m). Costs one reference solve per sub-point at control rate only: the pow budget is
  now exactly 32 per solve pair (probes BJ/BL updated; probe AE's 16-per-direct-pair reuse claim
  unchanged).
- **Rolloff range 3–6 → 3–12 dB/2x** (default 4 unchanged): R = 12 (a ≈ 2) reaches a 25 dB spread —
  a real focus control. Presets saved under < 1.3.0 are migrated (see below).
- **Blur `kBlurScale` 0.5 → 1.5** (backstop `kMaxBlurMetres` 8 → 24 m): blur = 1 now reaches
  1.5 × the RMS rig radius and genuinely washes the source across the array (spread → 2.8 dB).
  Parameter default rescaled 0.10 → 0.03 and the factory-preset blur column ÷3, so every shipped
  patch keeps its authored radius. Python oracle (`gen_dbap_reference.py`) re-anchored to the new
  spec constants and the fixture regenerated — a spec change, not a silent re-record.
- **Width max 6 → 12 m**, and `kFadeFraction` 0.15 → 0.05 so the default centre puck (0.46 m from
  the centroid) no longer sits inside the width-collapse zone (was ~1.19 m, now ~0.40 m; the 180°
  axis-flip guard at the exact centroid is preserved). Width remains inherently subtle on MONO
  material — it separates the L and R feeds in space; a mono decorrelator is a possible future
  improvement, deliberately out of scope here.

**Preset migration** (preset-manager v1.0.6 hook, editor-side): user presets stamped < 1.3 have
rolloff ÷3, width ÷2 and blur ÷3 applied to their stored normalised fractions — each preset keeps
its audible meaning on the new ranges. Sessions are unaffected (APVTS stores denormalised values).
Factory .json files regenerate automatically via the WR-04 version sentinel.

**Testing:** unit 45/45, render-harness 50/50 (pow-budget probes updated 16 → 32 through the
GainStage path, Distant Field blur expectation 0.55 → 0.18), ui_frontend_check 42/42 (stub ranges
synced), ui_layout_check 28/28.

## v1.2.0 (2026-08-20)

### Added — hover-help tooltips ("?" toggle)

Ported from O-Contrabass v1.7.0, which carries the VERIFIED measure-then-pin tooltip placement
(pattern_fixed_tooltip_shrink_to_fit_edge) rather than the earlier shrink-to-fit variant.

- **"?" toggle in the header** (between the screen tabs and the banners): when lit, resting the
  pointer on a control for 350 ms shows a short description of what it does. 49 controls and
  readouts annotated across both screens — the puck, the 8 in-plan weights, the nine
  controls-column cells, scenes (named sets, U slots, STORE), the elevation strip, the SAFE and
  MAP banners, the screen tabs, the venue rail (files, presets, output order, ping, rake, output
  set) and the footer readouts.
- **The preference persists with the session** as a root XML attribute in
  get/setStateInformation — an attribute, not a ValueTree property, whose XML round-trip
  rebuilds properties as strings so an `isBool()` guard on restore would never fire
  (critical_valuetree_xml_roundtrip_loses_type). Pre-1.2.0 sessions restore with help OFF.
- **The page PULLS the stored state at init** via `getTooltipsEnabled` — never pushed from C++,
  which would fire before the page module evaluated and silently never arrive
  (pattern_webview_one_shot_state_push_stale_on_preset_load).
- The toggle's own tip carries `data-tip-always` and bypasses the enabled gate, so the one
  control that can turn help back on is never the one control unable to explain itself.
- Tooltip content is written via `textContent` only, into the surface's own created nodes —
  no authored label is ever touched (pattern_js_state_updater_overwrites_html_labels), and the
  surface is a sibling of `.frame`, keeping `#group-elevation` the controls column's last child
  (ui_layout_check §22 stays non-vacuous).
- New native functions `setTooltipsEnabled` / `getTooltipsEnabled` (bridge surface 20 → 22,
  closed three ways by `ui_frontend_check.js` §3; §6's textContent whitelist grew by the
  tooltip's three receivers with their own binding proofs).
- UI state only: no parameter, no automation lane, no preset membership. DSP untouched.

**Testing:** `ui_frontend_check.js` 42/42 and `ui_layout_check.js` 28/28 pass.

## v1.1.0 (2026-08-20)

### Added — speaker→output assignment (the in-space rig fix)

**Root cause this addresses:** the plugin publishes a 7.1 layout and writes channel ROLES; the
host decides which physical output each role reaches. Under CoreAudio (Logic, Standalone) the
measured device order is `Emagic_Default_7_1` — `L R Lrs Rrs C Lfe Lss Rss` — so with the factory
role-order labels, speakers 3–8 land on physical outputs 5, 6, 7, 8, 3, 4 (the exact permutation
reported from in-space testing on an 8-channel interface, and the one measured at Stage 4 Gate
16). That is correct role routing, not a defect; what was missing was a first-class way to say
"speaker n is WIRED to output n."

- **Double-click a speaker glyph on the Room plan** → a popover assigns that speaker's physical
  output (1–8). Swap semantics: the previous holder of the chosen output takes the vacated one,
  so the label set stays a permutation by construction and the venue guard can never see a
  duplicate from this path.
- **Output badges on the plan**: a glyph whose label reaches a different physical output than its
  own number shows `→k` beside it (`→?` for a label outside the 7.1 set). A stock rig shows
  nothing new; a remapped rig is legible at a glance.
- **Venue rail, "Output order" group**: `Direct 1–8` writes the whole device-order label set in
  one click — the single-click fix for a rig wired 1..8 in a CoreAudio host — and `Roles`
  restores the factory surround-role labels.
- Mechanism: all three are LABEL edits through the existing validated path
  (`applyVenueEditChecked` → `buildSpeakerToBuffer` → `getChannelIndexForType`). No DSP change,
  no parameter change, no buffer index anywhere; assignments persist with the venue, `.venue`
  files, and presets, exactly as labels always have.
- New native functions `assignSpeakerOutput` and `applyOutputOrderPreset` (bridge surface
  18 → 20, closed three ways by `ui_frontend_check.js` §3); per-speaker `output` rides the
  existing `getVenueGeometry` payload. The device-order table lives in `Source/Data/OutputOrder.h`
  and in C++ only — the page renders numbers it is handed (D19).

**Caveat, stated plainly:** the output numbering assumes the measured CoreAudio 7.1 device order.
A non-CoreAudio host may map roles differently; the verify ping remains the 60-second ground
truth in any host, and the popover says so.

**Testing:** `ui_frontend_check.js` — all 42 sections pass, including the widened §3 closure.
Default behavior unchanged: with factory labels the map, the solve, and the meters are untouched.

## v1.0.0 (unreleased — Stage 4 phase 4.2 Block C complete, 2026-08-14)

### Host validation (Logic Pro 12.3, BlackHole 64ch — phase 4.2)

- **Logic's canonical interleaved 7.1 bounce order measured: `1,2,3,4,7,8,5,6`** — i.e. a bounce
  file carries `L R C Lfe Lrs Rrs Lss Rss` on channels 1–8, the canonical WAVEFORMATEXTENSIBLE
  channel-mask order (`FL FR FC LFE BL BR SL SR`). Measured at 158.3 dB minimum isolation and
  confirmed by a permuted-venue bounce returning its before-the-bounce prediction exactly. This is
  **not** the realtime device order (`Emagic_Default_7_1`: `L R Ls Rs C LFE Lc Rc`) — the bounce
  and device paths order channels differently, and both are now measured.
- **The speaker→buffer map is consulted, not decorative:** an 8-cycle label permutation loaded into
  all eight instances shifted the whole bounce order by exactly the predicted derangement.
- **LFE slot confirmed an ordinary speaker on the bounce path** — byte-identical output against a
  reference speaker fed the same 31 Hz–16 kHz multitone; no bass management, no filtering. The
  positive control (air filter engaged at a hull excursion) matches the TPT filter model to
  0.00/0.03 dB at two operating points with the cutoff derived from venue geometry, not fitted.
- **The hull-crossing audible clause is concluded** (the last open clause): sample-resolution
  null between a hull-crossing gesture and a static render on commercial program material shows
  no discontinuity; operator listen passed. Monitoring path recorded (MacBook Pro speakers).
- **Instantiation constraint documented:** in Logic, insert via the slot's **Stereo → 7.1**
  channel-config entry. Clicking the plugin *name* yields Logic's default **multi-mono** pick —
  eight independent mono instances with both banners correctly raised.
- Session recall (save → quit → reopen), all 11 automation lanes (write + read-back), `auval`,
  and `User/`-preset non-pollution all verified in-host; full gate-by-gate record in
  `.planning/stages/4-polish/evidence/session-gates-4.2.txt`.

## v1.0.0 (unreleased — Stage 4 phase 4.1 complete, 2026-08-12)

Eight-channel DBAP spatializer for irregular concert arrays. First release.

### Added

- **DBAP panning across eight speakers**, solved from measured venue geometry rather than an assumed
  ring. DBAP and not VBAP because VBAP discards distance, which an irregular hang cannot afford.
- **Measured venue model** — 42 values (eight speaker positions with labels, plus the rake and
  bounding-box scalars), editable in the UI and saved to `.venue` files independently of presets.
- **Source position, width and blur** — a puck over the room plan, sub-point source widening, and a
  blur radius that scales with the rig rather than with metres, so a patch means the same thing in a
  different hall.
- **Outside-hull processing** — a dB/metre trim with a −24 dB floor, and an air filter whose cutoff
  falls with distance beyond the speaker hull.
- **Six named scenes plus four user slots** (FUNC-06), resolved against the measured geometry rather
  than fixed speaker indices, so they follow a re-hung rig.
- **Verify ping** — a per-speaker identification tone for checking the map at the desk.
- **Eight output meters** that follow the channel map, not the buffer order.
- **Six factory presets** — Dry Point, Concert Default, Chamber, Wide Hall, Distant Field,
  Enveloping. Room character only: a preset never moves the source or the scene.
- **SAFE mode** — instantiating on a mono or stereo output gives a defined, non-destructive fold with
  a banner in the UI rather than a refusal to load.
- **Per-commit CI** (`.github/workflows/ci-tests.yml`) building and running both C++ test targets on
  macOS, and building the VST3 under MSVC with pluginval strictness 10 on Windows.

### Technical Notes

- **95 offline probes** across two console targets — 45 geometry/unit, 50 render-harness. No unit-test
  framework: `juce_add_console_app` plus exit codes, matching the twelve existing harnesses in this
  repo.
- **The channel map is derived, never hardcoded.** `AudioChannelSet` is a bitset, so buffer order is
  enum-bit order and a hardcoded 0..7 map would silently scramble the speaker assignment while
  passing auval and pluginval. Layer 2 of the gate compares a golden generated from *parsed JUCE
  source* by a compile-time `static_assert`, so a JUCE release that reorders `ChannelType` fails the
  build rather than shipping a wrong map.
- **Three 8-channel containers are accepted** — 7.1, 7.1-SDDS and 5.1.2 — because those are the only
  8-channel formats Logic exposes. Anything else, including `octagonal()`, folds to SAFE mode and
  raises the banner. The predicate is written as the complement of the three real rigs
  (`Source/Data/RigPolicy.h`), which is what makes an unknown fourth container fold *safely* rather
  than silently pass as a rig nobody mapped.
- **Preset loads preserve the source and the scene.** The shared preset manager resets every
  parameter to its default before applying a preset (by design), so a room-character preset omitting
  the position keys would not leave them alone — it would re-centre the source and clear the scene.
  `oo::presets::loadPreserving` snapshots and restores those eleven parameters around the load, at
  O-Octagon's call site and never in the shared module.
- **Factory presets are initialized from the editor, not the processor**, which keeps all preset file
  I/O off the headless `auval`/pluginval scan path. Verified: six pluginval runs and an `auval` pass
  created no user preset directory at all.
- Factory preset values are authored in engineering units and converted through the live
  `NormalisableRange`, never as normalised literals.

### Validation

- pluginval strictness 10 — VST3 ×3 and AU ×3, all six exit 0.
- `auval -v aufx OuOc OuDv` — **AU VALIDATION SUCCEEDED**, with all six `AUChannelInfo` configs
  reported: `[1,1] [1,2] [1,8] [2,1] [2,2] [2,8]`.
- 95/95 probes, 0 failures, from a forced full recompile with zero compiler warnings.
- 69 JS UI-gate sections green.

### Not Yet Validated

- **Host testing: DONE as of 2026-08-14** (see the Block C entry above) — with two named residuals:
  the realtime-loopback LFE *delta* was not measured (reference channels lost to a monitoring
  feedback loop; the LFE device channel itself captured clean at the constructed level), and the
  audible-clause listen has not been repeated on revealing monitoring.
- **No hall.** Nothing has been heard on a real 8-speaker array; the physical-interface half of
  COMPAT-02/2 carries owner: none.
- **Windows UI correctness.** CI proves the code compiles under MSVC and that pluginval 10 opens the
  editor without a timeout. No human has seen the UI on Windows.
- **RT-safety beyond allocation.** Allocation is measured by replacing the global `operator new`
  family; locks and file I/O in `processBlock` remain grep plus inspection.
  `-fsanitize=realtime` is unsupported by Apple clang 17.0.0.
