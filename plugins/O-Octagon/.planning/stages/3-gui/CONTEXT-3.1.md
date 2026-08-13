# Stage 3 — GUI · Phase 3.1 (Two-screen shell, Room plan, musical parameters) — Context

**Plugin:** O-Octagon
**Stage:** 3 of 4 — GUI
**Phase:** 3.1 of 3 — Two-screen shell, Room plan, musical parameters
**GSD phase:** discuss
**Date:** 2026-08-12
**Branch:** `feat/o-octagon` @ `a47cef88` (Stage 2 phases 2.2 / 2.3 work uncommitted)
**Participants:** Taylor Brook, Claude

This document carries **both** the stage-level decisions for Stage 3 (D1–D2) and the phase-level
context for 3.1 — the same shape as `CONTEXT-2.1.md`, which decided Stage 2's cycle structure while
also serving as phase 2.1's discuss.

---

## Entry Check — carried obligations from Stage 2

The standing obligation at every boundary: *"Re-verify all four checksums — a checksum that silently
points at the wrong file is worse than no checksum, because it reports green."*
(`pattern_promotion_checksum_pins_replaced_file`)

**Re-run at this boundary, before anything else. All four byte-exact on arrival:**

| Contract | SHA-256 on arrival | Result |
|---|---|---|
| `BRIEF.md` | `697a4f32…9fbd6` | ✅ matches STATUS frontmatter |
| `parameter-spec.md` | `b45f88dc…b9e02f` | ✅ matches |
| `research/ARCHITECTURE.md` | `a8a358f4…f0429b` | ✅ matches (the D2 re-pin issued at 2.3 discuss) |
| `ROADMAP.md` | `aec7d0ce…7ee29` | ✅ matches |

**No drift on arrival, and no contract is amended at this boundary.** `ARCHITECTURE.md` was re-pinned
at 2.2 discuss and again at 2.3 discuss; **Stage 3 opens with all four pins stable.** Nothing in
today's eight decisions requires editing a checksummed contract — the two contract *gaps* found
(scene semantics, scene-slot persistence) are absences, not errors, and are resolved by writing
`REQUIREMENTS.md` acceptance criteria rather than by amending `ARCHITECTURE.md`. If research or plan
finds that §6.3 must actually *state* the derivation rule, that is a re-pin and it happens at a
boundary, not mid-phase.

### Carried obligations, and their disposition here

| Carried from | Obligation | Disposition at this boundary |
|---|---|---|
| 2.3 verify | **D5 manual Logic gate — OPEN** | **Folded into the Stage 4 hall session (D2 below).** Not run at Stage 3 |
| 2.2 discuss → 2.3 verify | **`FUNC-06` + `UI-02..05` criteria owed at Stage 3 discuss, before Stage 3 plan** | ✅ **CLEARED — five sections written today** (see below) |
| 2.1 verify onward | CI gap — all 62 probes are local-only, `-DOUARICON_BUILD_TESTS=ON` | Unchanged. Stage 4 |
| 2.2 verify | `COMPAT-04` retroactive criteria | Unchanged. Stage 4. **Now the only summary row in the file without a criteria section** |

---

## Discussion Summary

Stage 2 closed **18/18 requirement rows complete, 0 partial, 0 failed**, 62 probes / 0 failures, with
every gate re-run at verify against a forced full recompile and eight negative controls run as new
work. Stage 3 inherits a DSP layer that is measured rather than asserted.

Stage 3 is a different kind of risk. Nothing in it is arithmetically hard; **all of it is silent when
it breaks.** `ARCHITECTURE §R7` names this the largest UI in the repo — two screens, a canvas plan
with a live gradient backdrop, 8 in-plan weight controls, 8 meters, a side-elevation strip, and a
42-field measurement table. The repo's own scar tissue is almost entirely about UI failures that pass
every automated gate: a top-level TDZ throw that kills every later initializer, a
`getNativeFunction` with no `withNativeFunction` behind it, a canvas that never stretches because it
is a replaced element, a shared JS state updater that erases HTML-authored labels. Every one of those
is now a written acceptance criterion rather than a hope.

The one requirement that carries real correctness weight is **FUNC-06**, and it turned out to be
undefined in both contracts — see "Contract gaps found at this boundary" below.

---

## Requirements Confirmed

Stage 3 verifies **FUNC-02, FUNC-04, FUNC-05, FUNC-06, UI-01..05** — nine rows, distributed across
the three phases by `ROADMAP.md`:

| Phase | Verifies | Also delivers |
|---|---|---|
| 3.1 | **UI-02** | WebView shell, screen switcher, Room plan, all 17 parameter bindings, SAFE banner |
| 3.2 | **FUNC-02, FUNC-04, FUNC-05, UI-01** | Venue screen, venue store, `VerifyPing`, `mapInvalid` surfacing |
| 3.3 | **FUNC-06, UI-03, UI-04, UI-05** | Meters, scenes, gradient backdrop, elevation strip |

### The criteria debt owed at this boundary is CLEARED

STATUS carried *"`FUNC-06` and `UI-02..05` — summary rows with no acceptance criteria, owed at Stage 3
discuss, before Stage 3 plan. This stage repaired the same defect three times; carry the habit."*

All five had a summary row and **no criteria section at all** — they would have been "verified" at
3.1/3.3 against nothing. Written now, before plan, derived from `ROADMAP.md` Phase 3.1/3.3,
`ARCHITECTURE.md` §3.1.6/§4.1/§4.3/§6.3/§OQ4, and today's decisions — **not invented**:
**FUNC-06 (6), UI-02 (7), UI-03 (4), UI-04 (4), UI-05 (4).**

Re-checked programmatically after the edit: **30 summary rows, 29 sections, zero sections without a
row, zero duplicate headings, zero sections with zero criteria.** The single row still lacking a
section is **`COMPAT-04`** — exactly the known dated debt (retroactive, Stage 4). No new gap; the
`### DSP-04` duplicate repaired at 2.2 stays repaired.

### Five criteria are non-obvious and deliberate

1. **FUNC-06/2 requires a permuted-numbering probe.** Against the traced layout, speakers 1 and 2
   *happen* to be front, so any `FRONT` implementation — derived or hardcoded — passes. The probe
   re-measures a venue with permuted speaker numbering and asserts `FRONT` still selects the
   physically-front speakers. **A fixed-index implementation must fail it.** This is the probe-O
   discipline from 2.1 and the R1 failure class.
2. **FUNC-06/5 re-runs FUNC-05's bit-compare after `SCENES` exists.** The guarantee "a preset
   physically cannot reach `VENUE`" was argued at §4.1 against a two-node tree. Adding a third node
   does not automatically preserve it; it is re-measured against the new tree shape.
3. **UI-02/5 is a non-vacuity gate, not a feature statement.** "Read out metres via
   `getNativeFunction`" is unfalsifiable as written — a JS min/max map produces metres too. The
   criterion is: *with the puck stationary, editing a venue coordinate must change the readout.*
4. **UI-03/2 must cross-check against verify-ping, not against `v_i`.** Metering the solve would
   light the correct speaker **even under a bypassed channel map** — literally the NC3 failure
   measured at 2.2 verify. §4.3 meters the written buffer for this reason, and the criterion says so.
5. **UI-04/2 asserts a recompute *counter*, not smoothness.** The field depends on speakers and
   weights, never on source position, so dragging the puck for N frames must leave the count
   unchanged. "No CPU spike" is an observation; the counter is the gate.

---

## Approach Decisions

| # | Decision | Choice |
|---|---|---|
| D1 | Stage 3 cycle structure | **Three full discuss→research→plan→execute→verify cycles**, 3.1 / 3.2 / 3.3, matching ROADMAP and the Stage-2 discipline |
| D2 | D5 manual Logic gate | **Folded into the Stage 4 hall session.** Not run during Stage 3 |
| D3 | Visual aesthetic | **Ouaricon Naturalist, darkened — split treatment: brand chrome, technical data** |
| D4 | Design workflow | **Designed inside 3.1 execute**, by `gui-agent`. No separate `/ui-mockup` pass |
| D5 | FUNC-06 named-scene semantics | **Derived from measured geometry, and shown on the plan before commit** |
| D6 | FUNC-06 user-slot persistence | **A `SCENES` child of `apvts.state`**, sibling to `VENUE`, carried by musical presets |
| D7 | Editor sizing | **Fixed size, non-resizable — ~1100 × 720** |

> **Numbering note.** The D-series restarts at Stage 3, as it did at each Stage-2 phase. The
> P-series (plan decisions) continues from Stage 2's **P36** — Stage 3's first plan decision is
> **P37**.

### D1 — three full cycles

Stage 2's per-phase cycle was decided at 2.1 discuss and repeatedly earned back: 2.3 alone ran
**eight negative controls**, four of which **corrected an attribution** that a results table had
recorded as green. Stage 3's failure modes are less arithmetic and more silent than Stage 2's, which
argues for more boundaries rather than fewer, not less.

The concrete win is that a WebView bridge gap cannot hide behind a later phase. 3.1 stands up the
shell and all 17 bindings and is verified *before* 3.2 adds ~50 more native-function surfaces for the
venue table, and before 3.3 adds meters and scenes. If the grep-diff gate is going to catch something,
it catches it against 17 bindings and not against 70.

### D2 — D5 folded into Stage 4, and what that costs

D5 is the ~15 min manual Logic session carried since 2.2: Task 12 (a) automate `srcX`, confirm the 8
lanes no longer move in lockstep; (b) `w3 = 0` → that lane silent, others compensate; plus 2.3's
width / air / trim observations and the H2 HF-rich hull crossing.

**Accepted, and named rather than hidden:** its only unique coverage is **QUAL-01 criterion 2's
*audible* clause**. QUAL-01 is already ticked ✅ complete at 2.3 — measurement bounded the D2 step to
**~15 % of an 8 kHz component as a one-sample step** and matched its own prediction to 0.000 %, but
measurement cannot conclude audibility. Folding to Stage 4 means **QUAL-01 carries an unverified
clause through the whole of Stage 3.**

The risk this accepts is specific: if the crossing ticks audibly, the fix is **RESEARCH-2.3's H3
lever — `fc(d_hull = 0)` toward Nyquist** — which re-tunes the Stage-2 musical curve *after* Stage 3
is built and verified. That is a DSP change under a finished UI. It is bounded (it touches
`hullproc::airCutoffHz()` and the DSP-07 curve criteria, not the UI), and Stage 4 is the hall
session where the material and the room are real rather than simulated at a desk. **Recorded here so
Stage 4 does not rediscover it as a surprise, and so the Stage-3 verifications are not read as having
settled it.**

### D3 — Naturalist darkened, split by layer

The `BRIEF.md` visual constraint (*"technical and legible over decorative — an instrument read at a
distance in a dark hall… no ornament that competes with the level field"*) and the Ouaricon Naturalist
brand aesthetic (warm aged paper, botanical illustration, serif, wide tracking) are in genuine
conflict. None of the five templates in `.claude/aesthetics/` resolves it: Naturalist is warm paper,
Swiss Minimal is pure white.

**Resolution: brand owns the chrome, function owns the data.**

```
ground    #1A1613   dark aged paper, warm
panel     #241E1A
headings  serif, wide tracking            (brand)
mark      botanical, low-alpha watermark  (brand)
── data layer ──
metres    mono, tabular numerals, #F0E8DC
hull      #F0E8DC   2px, explicit
level     brass → pale-gold ramp
speakers  numbered, mono, high-contrast
```

**Tabular numerals are load-bearing, not typographic taste.** UI-01 requires 42 values editable
without leaving the screen; a proportional serif does not column-align a 24-field coordinate grid,
and mis-scanning a metre value is a measurement error that propagates silently into the solve. The
level ramp is the only hue in the interface, so nothing competes with it — which satisfies the
BRIEF's actual constraint while the frame stays recognisably Ouaricon.

Handed to research: whether this darkened split should be saved back as a **new** aesthetic template
(a dark-technical sibling to Naturalist) for future performance-oriented plugins, or kept
plugin-local. Not decided here — it is a library question, not a Stage-3 blocker.

### D4 — designed inside 3.1 execute

No separate `/ui-mockup` pass. `gui-agent` designs and integrates in one phase.

**This raises the cost of a TDZ throw, and the mitigation is therefore mandatory rather than
advisory.** With a browser-iteration phase, a top-level throw shows up on the first reload; without
one, the first render is inside a plugin. `ROADMAP` Phase 3.1 already mandates rendering against
`tests/ui-stub/juce-stub.js` before integrating, and `plugins/O-ReverseDelay/tests/ui-stub/` is the
in-repo precedent (alongside `ui_frontend_check.js` and `ui_tooltip_clamp_check.js`, which are the
model for a headless UI probe). **Building that stub is 3.1 work, not optional polish** — it is
written into UI-02's criteria.

### D5 — named scenes derive from geometry

Neither `BRIEF.md` nor `ARCHITECTURE.md` §6.3 says what `FRONT` *is*. Both list the six names and
specify only the write mechanism.

`FRONT`/`REAR` split on the centroid's y, `LEFT`/`RIGHT` on x, `SIDES` selects hull speakers off both
axes, `ALL` sets 8 × 1.0. **The Room plan shows which speakers a scene will select before it is
committed**, so a surprising set on an odd rig is visible rather than silent.

The alternative — fixed indices `FRONT = {1,2}` — is trivially testable and **silently wrong the
moment a hall is measured with different speaker numbering.** That is the R1 failure class
(`critical_audiochannelset_is_a_bitset_not_an_order`) reproduced one layer up, in a plugin whose
entire premise is that the room is measured rather than assumed. A scene mis-firing mid-concert is
not recoverable, which is also why an **empty** derived set must be shown as empty and must not be
writable: all-zero weights are DSP-05's silence path.

### D6 — `SCENES` as a sibling of `VENUE`

The 4 user slots are 32 floats and no contract covers them. They go in a `SCENES` child of
`apvts.state`:

```
apvts.state  (root: "OOctagon")
├── PARAM × 17   ← automatable, musical presets write these
├── SCENES       ← NEW: 4 slots × 8 weights. Musical presets may carry these
└── VENUE        ← message-thread only, NEVER touched by a preset
```

Scenes are pure weight data and therefore **venue-portable** — the same argument that puts
`srcX`/`srcY` in normalised coordinates. They ride session state automatically through
`copyState()`. **§4.1's structural guarantee is preserved by construction:** `SCENES` is a sibling of
`VENUE`, so preset code still physically cannot reach venue geometry — but FUNC-06/5 re-measures it
rather than inheriting the claim.

### D7 — fixed 1100 × 720

No constrainer, no reflow. Every pixel authored once, which is the cheapest possible answer to §R7's
scale risk — reflow logic is where a two-screen UI of this size actually breaks.

**The accepted cost, stated:** no way to enlarge the plan for a distant read, and it will be tight on
a small laptop lid in a hall. If 3.1's Room plan proves unreadable at performance distance at a fixed
1100 × 720, that is a **3.3 discuss** finding (the phase that owns visual legibility), not a 3.1
plan change.

---

## Constraints Identified

1. **`createEditor` must stay guarded with `#if JUCE_WEB_BROWSER`.** The render harness built at 2.2
   depends on it and 29 harness probes die silently otherwise
   (`pattern_render_harness_breaks_on_webview_editor`).
2. **`std::unique_ptr` member order is relay → webview → attachment**, and it is a destruction-order
   requirement, not a style preference (juce8-critical-patterns §3).
3. **`WebSliderParameterAttachment` takes three arguments**, the third `nullptr`.
4. **`NEEDS_WEB_BROWSER TRUE`** in `juce_add_plugin`; `check_native_interop.js` in
   `juce_add_binary_data` and served by the explicit-URL resource provider; `type="module"` on every
   script tag.
5. **The resource provider receives bare PATHS** — never hard-code `juce://`
   (`critical_webview_resource_provider_and_schemes`). URL schemes differ per platform.
6. **`juce_add_binary_data` strips hyphens from filenames**
   (`critical_binary_data_strips_hyphens`) — a `room-plan.js` becomes `roomplan_js`.
7. **A second `juce_add_binary_data` target needs a distinct `NAMESPACE`**
   (`critical_dual_binary_data_namespace_collision`).
8. **Pass the `Juce` ES-module namespace, not `window.__JUCE__`**
   (`critical_juce_webview_namespace_vs_postmessage`) — panels go silently dead otherwise.
9. **`juce::String(const char*)` is ASCII-only.** Recorded as a finding at 2.2 execute specifically
   because it *"matters for Stage 3 UI strings"*: `detail = "… — …"` renders as `â`, `detail << "… — …"`
   is correct, and **there is no compiler warning**
   (`critical_juce_string_char_ctor_is_ascii_only`). This constraint arrives with a scar.
10. **The 17 parameters are frozen.** `parameter-spec.md` is pinned and Stage 3 adds none. Scenes,
    verify-ping and venue values are **not** parameters (§6.3).
11. **Windows CI is Stage 4, but MSVC hazards are authored in Stage 3:** C3493 (non-static
    `constexpr` in a lambda) and `SafePointer(this)` init-capture in nested lambdas. Both are
    write-time habits, not port-time fixes.
12. **The default venue must be labelled unmistakably as a placeholder** on the Venue screen (§R8).
13. **The negotiated container name must be surfaced on the Venue screen** — Stage 4's R2 test reads
    it off the UI (`logic_negotiated_container: create7point1` is the current expectation, not a
    confirmed fact).

---

## Open Questions for Research

1. **The `SCENES` node and `setStateInformation` ordering.** §4.1 fixes `replaceState()` →
   `readVenueFromState()` → `rebuildChannelMap()`. Where does reading `SCENES` land, and does it need
   the `AsyncUpdater` + `cancelPendingUpdate()` treatment
   (`pattern_asyncupdater_guard_flag_needs_cancel`)?
2. **Does a scene write of 8 parameters via `setValueNotifyingHost` record as 8 automation events in
   Logic, or coalesce?** FUNC-06/1 and /6 both depend on the answer, and 2.2's Q1 established that
   the call is synchronous but said nothing about host-side recording.
3. **Geometry-derived scene sets on degenerate venues.** All 8 collinear, all 8 coincident — 2.1's
   degeneracy matrix. Which sets go empty, and is "empty" the right answer or should it fall back?
4. **`getNativeFunction` round-trip cost for the metres readout during a drag.** UI-02/5 demands a
   live venue resolve; is a per-mousemove native call acceptable, or does it need a cached
   bbox pushed on venue change?
5. **Meter atomics and the 30 Hz Timer vs. `requestAnimationFrame`** — where does the read-and-zero
   actually happen, and does zeroing from the UI thread race the audio thread's max-store?
6. **Does the existing `getAPVTS()` accessor** (added unplanned at 2.2 execute, recorded because it
   widens the public API) **serve the WebView relays**, or do the relays need a different surface?
7. **Fixed 1100 × 720 vs. the 42-field table.** Does the Venue screen actually fit, at the tabular
   type size legibility requires, without scrolling? If not, D7 needs revisiting at 3.2 discuss.
8. **The ui-stub's fidelity.** What does `O-ReverseDelay/tests/ui-stub/juce-stub.js` actually stub,
   and is it sufficient for a canvas-heavy two-screen UI with native-function calls, or does
   O-Octagon need a richer one?
9. **SAFE-mode banner atomic** — which existing atomic reports SAFE mode, or does the processor need
   a new one?
10. **Whether the darkened-Naturalist split should be saved as a new aesthetic template** (D3's
    deferred library question).

---

## Confirmed available — do not rebuild

- **`getAPVTS()` is public** (2.2 execute, deviation 3) — the accessor name used by 12+ sibling
  plugins, added partly *because* Stage 3's WebView relays would need it.
- **The render harness exists and works** — 29 probes, `createEditor` already guarded.
- **`hull::isInside()` / `hull::project()` are free functions over raw storage** — the Room plan's
  hull overlay and the puck's inside/outside state can call the same code the solver does, so the
  overlay cannot drift from the behaviour it depicts.
- **`VenueModel` publishes bbox, centroid, `rigScale` and the classification per speaker** — UI-02's
  envelope and the `VERTEX`/`ON_EDGE`/`INTERIOR` readout (3.2) both read existing outputs.
- **`meterPeak[8]` is specified in §4.3** but **not yet implemented** — it is 3.3 work; the spec is
  written, the atomics are not there.
- **`plugins/O-ReverseDelay/tests/ui-stub/`** — the TDZ-stub precedent, plus `ui_frontend_check.js`
  and `ui_tooltip_clamp_check.js` as headless UI-probe models.

---

## Predicted Outcomes — declared here, not to be discovered at verify

**Phase 3.1 closes: UI-02 ✅.** Nothing else. FUNC-02/04/05 and UI-01 are 3.2; FUNC-06 and UI-03/04/05
are 3.3.

**Declared partial in advance — none.** 3.1's single requirement either closes or it does not. If a
UI-02 criterion cannot be met at 3.1 it will be declared at the 3.1 verify boundary **with a named
destination**, per the Stage-2 discipline that produced zero verify-time surprises across three
phases.

### Residuals Stage 3 does not close, stated now

1. **D5 / QUAL-01's audible clause** — Stage 4 (D2 above).
2. **CI gap** — Stage 4. Stage 3 will *add* to it: UI probes will also be local-only.
3. **`COMPAT-04`** retroactive criteria — Stage 4. Now the only summary row without a section.
4. **`COMPAT-02`** (Logic Pro) — Stage 4, unchanged.
5. **UI-04 / UI-05 descope to v1.1** — **deliberately NOT decided here.** `ROADMAP` defines the
   descope path and both are `nice` priority. The decision belongs at **3.3 discuss**, when real
   schedule information exists. UI-03 is `should` and is **not** descopable — §R7 names it a defence
   on R1.

---

## Contract Gaps Found at This Boundary

Neither is an error in a contract; both are **absences**, which is why neither triggers a re-pin.

1. **FUNC-06's named-scene semantics were undefined.** `BRIEF.md` and `ARCHITECTURE.md` §6.3 both
   list `ALL / FRONT / REAR / LEFT / RIGHT / SIDES + 4 user slots` and specify only the write
   mechanism. Neither says what `FRONT` resolves to. Resolved by **D5** and written into FUNC-06's
   criteria. Research should decide whether §6.3 needs to state it — that would be a re-pin at the
   3.1 research or plan boundary.
2. **The 4 user slots had no storage location.** §4.1's tree diagram has exactly two children.
   Resolved by **D6** (`SCENES` sibling). Same disposition: if §4.1's diagram should show three
   nodes, that is a re-pin at a boundary.

**`REQUIREMENTS.md` was edited at this boundary** — five criteria sections added. It is not one of
the four checksummed contracts (it is the living traceability document), and the edit is the
scheduled discharge of a debt dated to this exact boundary.

---

## Next Phase

Ready for: **research** — `/plugin-research O-Octagon 3-gui`

Ten questions above. The highest-value ones are Q2 (does a scene write coalesce in Logic — FUNC-06
rests on it), Q7 (whether D7's fixed size survives the 42-field table), and Q8 (whether the ui-stub
precedent is rich enough, given D4 removed the browser-iteration safety net).
