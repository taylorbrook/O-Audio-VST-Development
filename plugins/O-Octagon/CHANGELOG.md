# O-Octagon Changelog

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
