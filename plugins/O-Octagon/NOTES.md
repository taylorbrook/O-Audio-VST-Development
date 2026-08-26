# O-Octagon Notes

## Status
- **Current Status:** 📦 Installed — stage-4 roll-up re-verify ✅ VERIFIED 2026-08-14, all four
  stages complete; dev-branded build (`O-Octagon-dev`), not yet released
- **Version:** 1.4.0 (dev build installed; not released)
- **Type:** Audio Effect (8-Channel DBAP Spatializer)
- **Build target:** `OuariconOctagon` (folder `plugins/O-Octagon`) — `PLUGIN_CODE OuOc`
- **Complexity:** 5.0 (capped; raw 13.0) — staged implementation

## Lifecycle Timeline

- **2026-08-10:** Prior research completed — `research/logic-pro-multichannel-octaphonic-dbap.md`
  establishes the locked architecture (Logic's 10 named surround formats, `aufx` single-bus
  constraint, the `AudioChannelSet` bitset trap, DBAP 2011-04-14 revised equations)
- **2026-08-10 (Ideation):** Creative brief and 30 requirements documented
- **2026-08-11 (Stage 0):** Research & Planning complete — ARCHITECTURE.md and ROADMAP.md
  documented (Complexity 5.0, staged). All 5 open questions resolved; parameter-count
  discrepancy resolved to 17.
- **2026-08-11 (Stage 1):** Foundation shell — CMake, APVTS (17 parameters), 8-channel transport
  validated.
- **2026-08-12 (Stage 2, phases 2.1–2.3):** Geometry core (channel map, convex hull, audience
  plane), DBAP solve, source shaping and outside-hull processing.
- **2026-08-12 (Stage 3, phases 3.1–3.3):** WebView UI — room plan and puck, venue editor and
  `.venue` I/O, verify ping, preset rail, scenes, field visualisation, eight meters.
- **2026-08-12 (Stage 4, phase 4.1):** Machine gates. Per-commit CI added; first MSVC compile; six
  factory presets; `COMPAT-04` closed 3 of 3; `COMPAT-01` re-confirmed on the final binary.
  95 probes, 0 failures.
- **2026-08-13/14 (Stage 4, phase 4.2):** Host-and-ear against a frozen binary (`378fb4cd`). Desk
  gates, then the Logic Pro 12.3 / BlackHole 64ch session — all 14 session gates. `COMPAT-02`
  closed 3 of 3; the bounce order measured; `QUAL-01`'s audible clause concluded. Ledger 30/0/0
  with the completion signal finally agreeing.
- **2026-08-14 (Stage 4 verify + install):** Stage-4 roll-up re-verify ✅ VERIFIED (commit
  `388fa335`) — every machine-checkable figure re-measured, both transcribed-figure gates
  re-derived second-person, auval PASS. Installation FORMALISED rather than re-copied: the
  installed `-dev` bundles were measured byte-identical to freeze `378fb4cd` (VST3 `928cd447…`,
  AU `cc54db02…`), the same binaries every Block C gate validated, so no rm/copy and no cache
  clear was performed — nothing changed on disk that a cache could go stale against.
- **2026-08-20 (v1.1.0):** Speaker→output assignment, prompted by in-space testing on an
  8-channel rig: the reported permutation (3→5, 4→6, 7→3, 8→4) matched the measured CoreAudio
  `Emagic_Default_7_1` device order exactly — correct role routing, not a defect. Added the Room
  plan's double-click output popover (swap semantics), per-glyph `→k` badges, and the Venue
  rail's `Direct 1–8` / `Roles` one-click label sets. All label edits ride
  `applyVenueEditChecked`; the device-order table lives in `Source/Data/OutputOrder.h`. Bridge
  surface 18 → 20; both UI gates pass (42 + 28 sections); Playwright interaction probe green.
- **2026-08-26 (v1.4.0):** Per-speaker alignment delay — the HIGH/small gap from
  `.planning/FEATURE-REVIEW.md`, now closed. 0–50 ms per speaker stored in the venue beside the
  trims, applied post-solve on the eight output lanes by eight separate mono `juce::dsp::DelayLine`
  instances (one instance per speaker is mandatory — JUCE keeps the delay time per INSTANCE and only
  the buffers per channel). Read at a 5 ms-smoothed fractional position through `popSample`, targets
  set on the 64-sample control grid, so QUAL-03 holds. `Derive` fills all eight from align-to-farthest
  against the audience-plane centroid; a ms/metres toggle converts by the speed of sound the payload
  carries. `.venue` schema 1 → 2, additive — pre-v1.4.0 sessions and files load with delays at 0 and
  render BIT-IDENTICALLY, since a zero delay is bypassed rather than run through a zero-length line.
  Venue is 50 values, not 42. Bridge surface 22 → 23 (`applySuggestedDelays`). The ms→samples
  conversion is done in double: the float spelling puts 10 ms at 48 kHz one ulp above 480 samples,
  which makes `popSample` interpolate and lowpasses every sample (found by probe CS). All four gates
  green — unit 49/0, render harness 53/0, frontend 43 sections, layout 31 sections; auval PASS.
- **2026-08-20 (v1.2.0):** Hover-help tooltips — "?" toggle in the header, 49 controls annotated
  across both screens, ported from O-Contrabass v1.7.0 (measure-then-pin placement). Preference
  persists as a root XML attribute in get/setStateInformation and is PULLED by the page at init.
  Bridge surface 20 → 22 (`setTooltipsEnabled` / `getTooltipsEnabled`); both UI gates pass
  (42 + 28 sections). UI state only — no parameter, no DSP change.
- **2026-08-20 (v1.3.0):** The flat-field audibility fix — srcZ, rolloff, width and blur made
  audibly effective. Root cause: the ~3 m speaker-to-ear vertical offset compresses DBAP's
  distance ratios (8.5 dB max spread at defaults) and Σv²=1 removes level cues. srcZ gains a
  ±6 dB proximity cue from the un-normalised 1/k field (bit-transparent at z = 0); rolloff range
  3–6 → 3–12 dB/2x; kBlurScale 0.5 → 1.5 (blur = 1 = true wash; default and factory blur values
  rescaled to keep authored radii); width max 6 → 12 m with kFadeFraction 0.15 → 0.05. Preset
  migration hook remaps < 1.3 user presets. Oracle fixture re-anchored to the new spec. Probes:
  unit 45, harness 50 (pow budget now 32 through GainStage), UI 42 + 28 — all green.
- **2026-08-25 (three-level review, propose-only — no code changed):** Six parallel subsystem
  reviewers plus an adversarial verification pass produced `CODE_REVIEW.md` (1 Critical,
  5 Warnings, 34 Info; 3 findings refuted, 2 Criticals downgraded),
  `.planning/SIMPLIFICATION-AUDIT.md` (16 candidates: 1 HIGH / 7 MEDIUM / 8 LOW) and
  `.planning/FEATURE-REVIEW.md` (capability inventory, competitive landscape, 10 ranked gaps).
  Headline defect: **CR-01 breaks all eight meters on every platform, macOS included** —
  `transform-origin: center` without `transform-box` resolves against the viewBox in both Chromium
  and WebKit, so every arc renders 507 px off its glyph (measured on the real page in both engines).
  It shipped because `tests/ui_layout_check.js:1245` asserts DOM parentage and attributes but never
  measures a rendered position — it passes identically with the arcs off-glyph, so any fix must add
  a real rendered-geometry assertion. Verification also disproved a
  repo-wide premise: a JUCE WebView does **not** drop native-fn completions when the *editor* is
  hidden (the gate is the web view's own `isVisible()` flag, and O-Octagon never clears it) —
  three findings died with it and the repo memory note was corrected.

- **2026-08-25 (v1.3.1):** CR-01 resolved — the eight level meters actually render on their
  speakers now. `.meter-arc` / `.meter-peak` `transform-origin: center` → `0 0`: with the SVG
  default `transform-box: view-box`, `center` resolved to the plan viewBox centre (224 280) in each
  glyph's own translated space, so every arc rotated about a point 507.1 px away and the peak tick
  orbited 28–696 px off-plan — identically in Chromium 141 and WebKit 26.5, so the shipped macOS
  build was broken too, not just Windows. `0 0` names the glyph-local origin, is the SVG initial
  used value, and needs no `transform-box`; `fill-box` was rejected because the tick's own
  fill-box centre is (0, −15) and it would spin in place. Section 23 of `ui_layout_check.js`
  rewritten to measure RENDERED geometry (`getScreenCTM` + `getBoundingClientRect`, seven clauses
  × 8 speakers, radii derived from the tick's own `y1`/`y2` and angles from its live `rotate()`
  attribute, plus a > 90°-of-sweep non-vacuity clause) — the old clauses read attributes only and
  returned ALL SECTIONS PASS on the broken build. Negative control run: CSS reverted with the new
  gate in place fails 5 clauses / exit 5 while the three original clauses stay green. Both UI
  gates green (42 + 28). UI/CSS only — no parameter, no DSP, no state change.

- **2026-08-25 (v1.3.2 — CODE_REVIEW WR-01…WR-05):** the five Warning findings from the v1.3.0
  review, resolved. **WR-01** — `setStateInformation()` published the venue snapshot TWICE
  microseconds apart (`readVenueFromState()` then `rebuildChannelMap()`), and the 2-slot publisher's
  second write lands in the slot a live `processBlock()` is holding by reference: a data race on
  ~276 bytes, reachable on any host preset switch or session restore with the transport rolling.
  Closed with `readVenueFromState (bool publish = true)` and `! preparedYet` at the call site —
  suppressed only when the rebuild that follows will run. The seqlock the review proposed as the
  durable fix was **deliberately declined**: its stated trigger ("a dragged rake control committing
  venue edits at mouse-move rate") describes code that does not exist, `setStateInformation` was the
  only reachable double-publish path, and a seqlock changes the audio thread's read path for no
  live caller. The reasoning is recorded at the function so a future second caller adds it.
  **WR-02** — the DPR watch was hooked to `resize`, which cannot fire on a fixed-size editor when
  only the backing scale changes; replaced with a re-arming `matchMedia("(resolution: Xdppx)")`.
  **WR-03** — `readFloat()` tested presence only, so `rakeFront="tall"` loaded 0.0 (against the
  header's "never zeros") and `x="nan"` loaded a real NaN that `toValueTree()` wrote back out as
  `"nan"` and re-read forever; now validated as complete numeric text plus `std::isfinite`.
  **WR-04** — the footer metres readout followed only the puck, so every other way the source moves
  left it showing a plausible wrong position; subscribed to the `srcX`/`srcY` echo.
  **WR-05** — a C++-rejected venue commit left the typed text on screen while the model held the
  old value, unbounded in SAFE mode where no commit can ever succeed; `paintFields()` now runs in
  the `ok: false` branch.
  Gates: render harness 51 probes / 0 failures (`CR setstate-publishes-once` added), geometry target
  46 / 0 (`P2 venue-nonnumeric-attrs` added), `ui_layout_check` 31 sections (29 DPR, 30 metres echo,
  31 rejected commit added), `ui_frontend_check` 42 sections, `auval` PASS. Six negative controls
  run by file swap — two of them changed the work: **NC3** showed section 31 passing with the fix
  reverted (a pending `resetVenue` refresh was repainting for the wrong reason — a quiesce
  precondition was added), and the WR-04 fix showed section 14's metres clause had been measuring
  the staleness rather than the edit (the source is now parked off the bbox rail, where a min-rail
  move can actually reach the readout).
  `ui_frontend_check` section 25's "no state in a completion" rule was **narrowed, not dropped**:
  assigning `committed` in a `.then()` is still forbidden; repainting from already-established state
  is allowed only inside the `ok: false` refusal branch.

- **2026-08-26 (v1.3.3 — SIMPLIFICATION-AUDIT HIGH-01):** the mini-plan stopped carrying its own
  copy of the Room plan's drawing code. `roomplan.js`'s Q8 charter is *a second view, never a second
  projection*, and section 19 of the static gate enforced the projection half — but only
  `metresToPx`, `fitBox` and `makeView` were actually shared. The six lines that build the hull's
  SVG `points` string and the three `classList.toggle` calls carrying `VERTEX`/`ON_EDGE`/`INTERIOR`
  had been copy-pasted into `venue.js`'s `drawMini()`, so a fourth classification class or any
  change to the points format had to be made twice. Extracted as `hullPoints(hull, view)` and
  `placeGlyph(g, s, view)` in `roomplan.js`; both plans call them, and `venue.js` no longer calls
  `metresToPx` at all — section 19 is **widened**, one fewer call site that could stop routing
  through the one projection. The helpers had to land in `roomplan.js` and not the other way:
  section 32 bans the classification vocabulary from the scene module.
  **The v1.1.0 output badge deliberately stayed in `roomplan.js`** — the mini has no `gout-N` node,
  and sharing the badge would mean a null-node branch on the mini's behalf.
  Byte-identity was measured, not asserted: `#plan-geometry` + `#mini-geometry` `outerHTML` dumped
  from the ui-stub across four venue states hashes to the same sha256 `fd73cf31…649b8b` before and
  after. Because the stub's fixture only ever produces `VERTEX` and `ON_EDGE`, a second probe
  compared the new exports against a verbatim copy of the pre-change inline code from
  `backups/O-Octagon/v1.3.2/` over all three classes plus an unknown one, on fresh *and* pre-dirtied
  glyphs (the on→off toggle direction the fixture never reaches), with untidy floats — 297
  comparisons, all identical, negative-controlling to 48 mismatches when one toggle's literal is
  mistyped. `ui_frontend_check` 42 sections, `ui_layout_check` 31 sections, `auval` PASS. JS only —
  no C++, no parameter, no DSP, no render-golden exposure.

- **2026-08-26 (v1.3.4 — SIMPLIFICATION-AUDIT Phase 3, Batch A):** six LOW-tier candidates applied,
  one reverted. **LOW-01** `commitScenes()` folds the `writeToState` + `++scenesGeneration` pair —
  one invariant, three hand-kept copies — leaving the constructor's seed write at
  `PluginProcessor.cpp:178` alone, since it has no cache to invalidate and bumping there would be a
  behaviour change (the audit missed that fourth site). **LOW-02** deleted a no-op
  `text-transform: none`. **LOW-03** `sliders.set(id, { state })` — the stored `input`/`value` nodes
  were dead payload. **LOW-04** `FIELD_INPUT_IDS` spreads `WEIGHT_IDS`. **LOW-05** `setMeters()` uses
  its own file's `clamp01`. **LOW-07** three `while (firstChild) removeChild` loops became
  `replaceChildren()`.
  **LOW-06 was reverted as a false positive** — the seven "meaningless" alias consts in `venue.js`
  are what satisfy `ui_frontend_check` §6's textContent *receiver-name* whitelist, which guards
  `pattern_js_state_updater_overwrites_html_labels`; deleting them failed the gate, and passing it
  again would mean loosening a deliberately short whitelist for seven lines of cosmetics.
  Verification: a rendered-DOM + computed-style snapshot over five page states hashes identically
  before and after (`69227ed4…d0ae47`, 0 console errors), shown deterministic across two baseline
  runs and negative-controlled to 80 changed style lines; a 126-comparison clamp probe covers the
  pathological inputs the all-zero stub meters never reach. Gates: `ui_frontend_check` 42,
  `ui_layout_check` 31, render 51/0, geometry 46/0, `auval` PASS.
  **Coverage gap found, not caused:** removing `++scenesGeneration` entirely leaves all 51 render
  probes green — nothing observes `getScenesGeneration()`. See Known Issues.
- **2026-08-26 (v1.3.5 — SIMPLIFICATION-AUDIT MEDIUM-03):** the audit's only behaviour correction.
  `getFieldGrid`'s `readParam` helper took a transcribed fallback per call — `4.0f` / `0.1f` /
  `1.0f` — and `blur`'s had drifted: the live default moved `0.10 → 0.03` in v1.3.0 when
  `kBlurScale` tripled, and the copy in `PluginEditor.cpp` never followed. `readParam` now takes
  only an id and derives the fallback from the parameter itself,
  `getParameter (id)->convertFrom0to1 (getDefaultValue())` — the same derivation
  `getParameterDefaults` already uses for the dblclick-reset payload — so the default is stated
  once, in the APVTS layout. All three literals are gone.
  **Reachability:** the fallback fires only on a null atomic (impossible for a valid id) or a
  non-finite one (host wrote NaN), so no reachable path changes value; on the NaN path the field
  backdrop now falls back to 0.03 rather than 0.10. Verified the derivation is bit-exact for all
  three parameters (linear ranges, no skew): `rolloff` 4.0f and `hullAtten` 1.0f are unchanged
  to the bit and only `blur` moves. Gates: `ui_frontend_check` 42, `ui_layout_check` 31,
  geometry 46/0, zero compiler warnings, `auval` registers.

## Known Issues

**No test observes `scenesGeneration` (found 2026-08-26, v1.3.4).** Deleting `++scenesGeneration`
from `commitScenes()` leaves all 51 render-harness probes and all 46 geometry probes green. CK and
CL round-trip the scene *state* but neither reads `getScenesGeneration()`, so the counter that tells
the page its cached slots are stale is uncovered. A probe asserting the generation advances across
`captureScene`, the `SCENES` custom-state callback and `setStateInformation` — and does NOT advance
across the constructor's seed write — would close it. Predates v1.3.4; the sweep only surfaced it.

**Deliberately deferred at v1.3.2** (from `CODE_REVIEW.md`):

- **The venue-snapshot seqlock.** WR-01's reachable path is closed by suppressing the double
  publish, but `VenueSnapshotPublisher` itself is still a 2-slot buffer that does not know which
  slot a reader holds. It is safe today because every publisher publishes exactly once per
  message-thread call — a property of the *callers*, not of the publisher. **Anyone adding a second
  publish inside one call must add the seqlock (or a 3-slot reader-claimed-index buffer) with it.**
  The argument is written out at `readVenueFromState()` in `PluginProcessor.cpp`.
- **No plausibility clamp in `readFloat()`.** WR-03's fix rejects non-numeric and non-finite text,
  but not an absurd-but-finite coordinate (`x="1e9"`). `readFloat()` is generic and cannot know
  whether it is reading a metre, a decibel or a rake, so per-field rails stay where they already
  are — `kVenueTrimClampDb` and the geometry limits in `publishSnapshot()`. The audio path is
  covered; the UI would draw a very large room.
- **The 34 Info findings (`IN-01` … `IN-34`) are untouched.** `/improve-review` resolves the
  Critical and Warning tiers only. Five of them rest on the hidden-editor premise that verification
  disproved and are annotated as such in the review rather than deleted.

**Registered risks** — see
`.planning/stages/0-ideation/CONTEXT.md` for the full register:

- **R1 (CRITICAL):** the speaker→buffer channel map fails *silently*. A wrong map does not crash,
  does not produce NaN, and passes `auval`, `pluginval` strictness 10 and every test that does not
  specifically look for it — it is audible only in the hall. Aggravated by the fact that for
  `create7point1()` the JUCE enum-bit order coincidentally equals the initializer-list order, so a
  hardcoded 0..7 map *appears correct today*. Mitigated by a three-layer test strategy.
- **R2 (HIGH):** Logic may negotiate 7.1 (SDDS) rather than plain 7.1 —
  `kAudioChannelLayoutTag_Emagic_Default_7_1` (`juce_CoreAudioLayouts_mac.h:117`) maps to the SDDS
  channel-type membership. Mitigated by accepting all three 8-channel containers. Settled at Stage 4.

## Additional Notes

**Concept.** A Logic Pro-native 8-channel spatializer rendering a mono/stereo source to eight
discrete speaker feeds using Distance-Based Amplitude Panning over an irregular, non-flat,
user-measured speaker array. Target venue: Roy Barnett Recital Hall, UBC — 255 seats, deep
rectangular plan, steeply raked seating, speakers mounted high on the side walls.

**Why DBAP and not VBAP.** VBAP normalises speaker positions to unit direction vectors and discards
distance entirely, serving one sweet spot. The Barnett array is three pairs down the side walls plus
an inboard rear pair — non-equidistant and mounted above a rake. DBAP weights every speaker by its
actual distance and shows lower variance across listener positions, which is the metric that matters
for an audience distributed through a hall.

**Transport.** mono/stereo in → `AudioChannelSet::create7point1()` out. 7.1 is used purely as an
8-channel carrier; its L/C/R/LFE semantics are meaningless here and all real geometry lives in the
DSP. Logic exposes only 10 named surround formats and no arbitrary discrete N-channel bus, and
effects (`aufx`) get exactly one output bus.

**Parameters.** 17 musical (automatable, APVTS) + 42 venue values (separate `ValueTree`, never
written by a musical preset). The headline gesture is *spatial orchestration* in the Acousmonium
sense — automating the 8 per-speaker weights to move a sound between speaker subsets at constant
perceived level, since `Σ v_i² = 1` means dropping weights redistributes rather than reduces.

**Relationship to O-Orbit.** Complementary, not redundant. O-Orbit is the general-purpose VBAP
*orbiter* with a motion engine across many surround formats. O-Octagon has no motion engine, one
locked 8-channel transport, and DBAP distance weighting for a specific irregular non-flat rig.
O-Octagon does **not** fork O-Orbit and must **not** link SAF.

**Presets store `blur` (0–1), not metres — and that is deliberate.** The blur radius is resolved
against the venue's `rigScale`, so the same `blur` gives a proportionally different radius on a
differently-sized rig. Factory presets are therefore venue-portable *by construction*: a patch means
the same musical thing in a different hall. On the default venue `rigScale` is 7.9317 m, so
`blur = 0.55` resolves to `r_s = 2.18 m` **there and only there**. If a preset's audible diffusion
changes after a venue edit, that is the design working, not drift.

**A preset never moves the source or the scene.** The six values a factory preset carries are room
character (`width`, `rolloff`, `blur`, `hullAtten`, `airAmount`, `outputGain`). The other eleven —
`srcX`, `srcY`, `srcZ` and the eight weights — are snapshotted and restored around the load
(`oo::presets::loadPreserving`). This is not the same as omitting them from the preset: the shared
preset manager resets *every* parameter to its default before applying anything, so omission alone
would re-centre the source and clear the scene mid-cue.

**Three channel orders coexist, all measured — never conflate them (Stage 4, 2026-08-14).**
The plugin's *buffer* order is `create7point1()` enum-bit order `L R C Lfe Lss Rss Lrs Rrs` — an
identity against the venue table. Logic's realtime *device* order is `Emagic_Default_7_1`
(`L R Ls Rs C LFE Lc Rc`; measured by probe CT as buffer→device `1,2,5,6,7,8,3,4`). A Logic
*bounce* writes the canonical WAVE channel-mask order `FL FR FC LFE BL BR SL SR` (measured by
CR-a as buffer→file `1,2,3,4,7,8,5,6`). Three different answers to three different questions —
which channel a sample occupies depends on which boundary you are looking at.

**Logic instantiation (COMPAT-02, the quiet failure):** insert O-Octagon via the slot's
**Stereo → 7.1** channel-configuration entry. Clicking the plugin *name* takes Logic's default
pick, **multi-mono** — eight independent mono instances, each correctly raising the SAFE and MAP
banners. If both banners are up on a 7.1 track, check for the multi-mono control bar first.

**Deferred to v1.1+:** VBAP A/B mode; binaural/stereo fold-down; quadraphonic variant; internal
diffuse reverb; motion engine; multiple simultaneous sources.

**v1.1 tool-maintenance register (from the Block C close, 2026-08-14).** Four
`tests/tools/analyse_bounce.py` / runbook defects, one root pattern — constants baked in before
measurement and overtaken by it. None was fixed mid-phase (editing graded assertions after the
fact was refused twice), and none affects a recorded result:
1. ping mode hard-codes expected sequence `1..8` (falsified by CT; CT stands as a
   transcribed-figure gate by the Block C close decision)
2. order mode's N13 guard refuses only the literal identity — the real bypass permutation is now
   `1,2,3,4,7,8,5,6`, and the refusal message quotes the stale `2,3,4,5,6,7,8,1`
3. `--emit-json` dedupes on `(label, mode)` with `--label` limited to CR-a/CR-b, so the manifest
   holds exactly one `lfe` run and a second silently evicts the first
4. the runbook's NC4 lacks its `dHull > 0` precondition — as spelled the control cannot fire
   (air is structurally inert at the mandated centre position)

**Planning artifacts:**
- `.planning/BRIEF.md` — creative brief
- `.planning/REQUIREMENTS.md` — 30 requirements with acceptance criteria and stage traceability
- `.planning/parameter-spec-draft.md` — parameter draft
- `.planning/research/ARCHITECTURE.md` — binding DSP architecture contract
- `.planning/ROADMAP.md` — phased implementation plan
- `.planning/stages/0-ideation/CONTEXT.md` — Stage 0 findings, decisions, risk register

**Related research:**
- `research/logic-pro-multichannel-octaphonic-dbap.md` — the locked architecture
- `research/juce8-multichannel-spatial-audio.md` — `AudioChannelSet` reference, bus negotiation
- `research/spatial-audio-per-grain-spatialization.md` §1 — VBAP math, for the deferred v1.1 mode

**Installation Locations (dev branding):**
- VST3: `~/Library/Audio/Plug-Ins/VST3/O-Octagon-dev.vst3`
- AU: `~/Library/Audio/Plug-Ins/Components/O-Octagon-dev.component`
- Installed binaries byte-identical to freeze `378fb4cd` (VST3 `928cd447…`, AU `cc54db02…`)
