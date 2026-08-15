# Stage 3 — GUI · Phase 3.3 (Scenes, meters, gradient, elevation) — Context

**Plugin:** O-Octagon
**Stage:** 3 of 4 — GUI
**Phase:** 3.3 of 3 — weight scenes, live meters, DBAP gradient backdrop, side-elevation strip
**GSD phase:** discuss
**Date:** 2026-08-12
**Branch:** `feat/o-octagon` @ `a47cef88` (Stage 2 phases 2.2 / 2.3 and Stage 3 phases 3.1 / 3.2 uncommitted)
**Participants:** Taylor Brook, Claude

---

## Entry Check — carried obligations from Phase 3.2

The standing obligation at every boundary: *"Re-verify all four checksums — a checksum that silently
points at the wrong file is worse than no checksum, because it reports green."*
(`pattern_promotion_checksum_pins_replaced_file`)

**Re-run at this boundary, before anything else. All four byte-exact on arrival:**

| Contract | SHA-256 on arrival | Result |
|---|---|---|
| `BRIEF.md` | `697a4f32…f6b9fbd6` | ✅ matches STATUS frontmatter |
| `parameter-spec.md` | `b45f88dc…cbb9e02f` | ✅ matches |
| `research/ARCHITECTURE.md` | `a8a358f4…9b6d4408` | ✅ matches |
| `ROADMAP.md` | `aec7d0ce…0137ee29` | ✅ matches |

**No drift on arrival.** `research/ARCHITECTURE.md` **is then amended at this boundary** — the three
§8 re-pins scheduled since 3.1 research, plus a fourth found here. See §Contract amendments. The
other three contracts are **not** touched and their pins are unmoved.

### Carried obligations, and their disposition here

| Carried from | Obligation | Disposition at this boundary |
|---|---|---|
| 3.1 research (N3) / 3.1 plan | **§6.3's `SIDES` is not derivable as written** — re-pin scheduled for 3.3 discuss | **TAKEN — amendment 3.** Measured predicate adopted (D16) |
| 3.1 research (N1) / 3.2 summary | **§6.3's gesture mechanism is incomplete** — `setValueNotifyingHost` opens no gesture; scenes must bracket `w1..w8` | **TAKEN — amendment 4.** D18. Closes `gesture_bracket_obligation` |
| 3.1 discuss (D6/D7) / 3.1 research | **§4.1's tree is two-node; `SCENES` makes it three** | **TAKEN — amendment 1.** D17 |
| 3.2 verify (Issues 2) | **`ui_layout_check.js` §0 emits no timestamp** — the ordering claim rests on transcription. *"3.3 should land that one line"* | **Accepted as 3.3 work.** D27 |
| 3.2 verify (Issues 1) | **`VerifyPing`'s RNG is unseeded** — figures are not reproducible. Seeding is *"a decision, not a default"* | **Decided: stays unseeded.** D28 |
| 3.2 verify / summary (D-1) | **Q11's main-column comparison inverts** — the venue rail's mini-plan is 170 px, the venue main column's slack is 251 px | **Decided: NO action.** D24 — and the premise that 3.3 spends that budget is **corrected** below |
| 3.2 verify / summary (D-2) | **The `[coarse]` rail assertion is not the guard** — the fitted-box-vs-stage assertion is | **Inherited as a construction rule.** D25 — and the vacuous shape is **confirmed present** on 3.3's own target column, measured below |
| 3.2 verify | **Gate 13's Venue/modal half** — UI-01/3(c), ~8 min human launch-and-look | **Unchanged, still open.** Rolls into 3.3's own Gate 13, which the new screens extend |
| 3.1 discuss (D2) | D5 / QUAL-01's *audible* clause — the ~15 min Logic hall session | Unchanged. **Stage 4.** Not run at 3.3 |
| 2.1 verify onward | CI gap — all probes and both JS gates are local-only | Unchanged. **Stage 4.** 3.3 widens it a third time |
| 2.2 verify | `COMPAT-04` retroactive criteria | Unchanged. Stage 4. Still the only summary row without a criteria section |
| 3.2 verify | **N8 — `mapInvalid` is AUDIBLE**, not "the retained map": speaker 1 gets L, speakers 2–8 all get R at unity | **Inherited and it now has a second consumer.** See D19's note on the meters |

### Numbering — the D-series continues at D15

Stage 3 does not restart the D-series (`CONTEXT-3.2.md`, §Numbering note). 3.1 held **D1–D7**, 3.2
held **D8–D14**. **3.3's decisions are D15–D28.** The P-series continues from **P68** — 3.3's first
plan decision is **P69**. C++ probe letters continue from **BZ**; 3.3's first probe is **CA**.

---

## Discussion Summary

3.1 built the shell and closed `UI-02`. 3.2 turned the plugin from a renderer of state into an editor
of it and closed `FUNC-02`, `FUNC-04`, `FUNC-05`, `UI-01`. **3.3 is the phase that makes the plugin
readable across a dark hall** — and it is the last phase of Stage 3, so every remaining stage-3 row
closes here or does not close at all.

Four requirements, and they are not four of a kind:

- **`UI-03` (meters)** is a **safety** feature wearing a visualisation costume. §4.3 meters the
  *written buffer, post-map and post-trim* rather than the solved `v_i` precisely so that a
  channel-map error shows up as the **wrong speaker lighting on the plan**. It is the second, human
  line of defence on R1, the highest risk in the project, and §R7 names it explicitly **not
  descopable**.
- **`FUNC-06` (scenes)** is the phase's only *write* path, and it writes to the eight parameters whose
  all-zero state is DSP-05's silence. It carries the last open third of the gesture-bracket
  obligation and it introduces the plugin's **first and only** `setCustomStateCallbacks`
  registration — which changes the shape of `FUNC-05`'s strongest assertion.
- **`UI-04` (gradient)** and **`UI-05` (elevation)** are both `nice`, and §R7 named them the descope
  path to v1.1. **Both ship** (D15).

Three things were measured or read in source at this boundary rather than assumed, and each one moved
a decision:

1. **The ROOM controls column has 278 px of vertical slack, at 582 px wide** — measured on the
   rendered page at 1100 × 720, not computed. This is 3.3's real layout budget. See §Measurement.
2. **The 251 px figure carried in from 3.2 is the *venue* main column's slack, not this one.**
   `VERIFICATION-3.2.md`'s residual reads *"3.3 adds meters and an elevation strip to that same
   rail"* — it does not. The meters go on the ROOM plan and the elevation strip goes in the ROOM
   controls column. **3.3 does not touch the venue rail at all.** Correcting this is what makes D24
   a decision rather than a deferral.
3. **`getVenueGeometry` already carries everything `UI-05` needs** — per-speaker `z`, `rake.front`,
   `rake.rear`, `bbox.minY/maxY` and the centroid, all landed by 3.2's P55 (`PluginEditor.cpp:277-365`).
   **The elevation strip requires no new native function**, which is a large part of why it is
   affordable alongside the gradient.

The risk profile shifts once more. 3.1's hazards were silent-when-broken *rendering*; 3.2's were
silent-when-broken *state*. **3.3's are silent-when-broken *correspondence*** — a meter that lights
the wrong speaker, a scene that selects the wrong four, a gradient that draws a field the solver does
not actually produce. Each of them looks entirely plausible on screen. That is why every one of them
is asserted against a value the C++ side **returned**, never against a JS re-derivation.

---

## Requirements Confirmed

Four rows, all closing at this phase. Criteria as written in `REQUIREMENTS.md`; nothing is narrowed
here and nothing is added.

| Requirement | Priority | Criteria | Notes at this boundary |
|---|---|---|---|
| `FUNC-06` — Weight scenes | should | 6 | Criterion 2's **permutation probe** is the non-vacuity guard; criterion 5 **re-runs** FUNC-05's bit-compare against the new tree shape |
| `UI-03` — Live per-speaker level indicators | should | 4 | **Not descopable** (§R7). Criterion 4 re-runs probe **AO** with metering live — PERF-01 must not regress |
| `UI-04` — DBAP gradient backdrop | nice | 4 | **Ships** (D15). Criterion 1 asserts against the shipping `DbapSolver`, never a JS re-derivation |
| `UI-05` — Side-elevation strip | nice | 4 | **Ships** (D15). Needs no new native function |

**Zero partials are declared in advance**, as at 3.1 and 3.2. Closing these four completes Stage 3:
`UI-02` (3.1), `FUNC-02`/`FUNC-04`/`FUNC-05`/`UI-01` (3.2), these four (3.3) — nine rows.

---

## Measurement taken at this boundary

The layout budget was **measured on the rendered page** at the shipping 1100 × 720, against the
ui-stub tree, rather than derived from row arithmetic — `pattern_flex1_container_slack_invisible_to_row_sum`
is the reason, and this project has already paid for that lesson twice in Stage 3.

| Box | Measured |
|---|---|
| `.content` | 1100 × 624 |
| `.plan-column` / `#plan-stage` / `#plan-layers` | 470 × 592 / 470 × 560 / **448 × 560** |
| `.controls-column` | **582 × 592**, `flex column`, `gap 12px` |
| `#group-position` / `#group-solve` / `#group-space` / `#group-output` | 89 / 63 / 63 / 63 px tall, last one ending at y = 386 |
| **Vertical slack below `#group-output`** | **278.0 px**, full 582 px width |

**278 px at 582 px wide is 3.3's budget on the ROOM screen**, and both new components must live
inside it. It is enough — but it is not generous, and it is the number research and plan must build
against instead of re-deriving one.

**The vacuous-assertion shape is already present on this column.** The same measurement returned
`controls.scrollHeight === controls.clientHeight === 592`. That is *exactly* D-2's signature: a
`flex` container whose overflowing child does not propagate into its `scrollHeight`. An assertion of
the form `controlsScrollHeight <= controlsClientHeight` would therefore report green over an
arbitrarily large overflow, **on this specific column, today**. See D25.

---

## Approach Decisions

| # | Decision | Choice | Rationale |
|---|---|---|---|
| **D15** | `UI-04` and `UI-05` descope | **Both ship in 3.3.** No descope | Both already carry complete, measurable acceptance criteria written at the Stage 3 boundary — the expensive part is done. The gradient costs **zero layout** (`#plan-backdrop` already exists as a separate canvas layer in `index.html:119`) and the elevation strip needs **no new native function**. Descoping later costs a flag, exactly as UI-04/4 requires; re-scoping costs a refactor. §R7's fallback stays available at plan if the 278 px does not hold |
| **D16** | `SIDES` semantics (§8 re-pin) | **Adopt the measured predicate**: `classify(i) != INTERIOR ∧ \|x−cx\|/hx > \|y−cy\|/hy` → `{3,4,7,8}` on the default venue | N3 proved *"hull speakers off both axes"* admits several readings that disagree on real rigs. The half-span normalisation is what makes it meaningful on a rectangular hall. Speakers 1 and 2 miss by **6 %** — a property of this hall, not a defect — and FUNC-06/3's show-before-commit is what makes a 6 % margin **visible instead of silent**. Rejected: widening the threshold (an untunable constant with no principled value) and dropping `SIDES` (loses the only scene that is not an axis split) |
| **D17** | `SCENES` storage (§8 re-pin) | **Third child of `apvts.state`, sibling of `VENUE`**, reached through the **first and only** `setCustomStateCallbacks` registration | Sibling, never child, is what makes FUNC-05 structural rather than disciplined. The registration is the single path by which a preset can reach non-parameter state, so §27's assertion **changes shape**: from *"the symbol appears in zero of 24 files"* to *"exactly one registration exists and its body touches only `SCENES`"*. Only the **four user slots** persist — named scenes are derived on demand, so there is nothing to go stale when the venue moves |
| **D18** | Scene gestures (§8 re-pin) | **`beginChangeGesture()` → `setValueNotifyingHost()` → `endChangeGesture()` on each of `w1..w8`** — eight brackets per scene click | N1: `setValueNotifyingHost` opens no gesture, so without brackets Logic's Touch/Latch may move the sound and **not record it** — invisible to build, `auval` and `pluginval` alike. Third and final site; closes `gesture_bracket_obligation` after the 3.1 puck and the 3.2 preset load |
| **D19** | Scene membership derivation | **Computed in C++, returned to the page. The UI performs no speaker arithmetic** | Directly inherits D14's rule for the ping readout (*the indicated speaker is `getPingState().speaker` and nothing else*). A JS re-derivation is a mirrored fixture (`pattern_test_fixture_mirrors_drift_silently`) over R1, the highest-risk component in the project. It is also what makes FUNC-06/2's permutation probe meaningful: a fixed-index implementation must **fail** it |
| **D20** | Empty-set scenes | **Rendered as empty and NOT writable** — the control is disabled and refuses the commit | All-zero weights are DSP-05's silence path. Reaching it by a mis-derived scene click mid-concert is unrecoverable, and a degenerate venue can legitimately empty a named scene |
| **D21** | Scene preview interaction | **Pointer hover or keyboard focus lights the resolved set on the plan; click commits** | Zero extra gestures in a concert, which a two-step arm/commit would double. Keyboard focus gives the identical preview, so the preview is not pointer-only and FUNC-06/3 is not satisfied by a hover the user may never perform. Rejected: always-on 8-dot swatches — too small at hall distance and they duplicate the plan |
| **D22** | User-slot store gesture | **Armed two-step**: a `STORE` control arms, the next slot click captures, and it **auto-disarms** after one capture | Deliberately asymmetric with D21. Recalling a scene is reversible; **overwriting a slot is not**, and it is the one gesture where a mid-concert mis-click destroys data the user measured. *This is a judgement call made here rather than asked — flagged so plan can overturn it cheaply* |
| **D23** | Meter form | **A concentric arc outside the speaker glyph**, filling −60..0 dBFS, peak-hold as a tick on that arc | The glyph's own stroke already renders hull classification (dashed vs solid — speakers 3 and 8 are dashed today), so the meter must not reuse it or the two readings collide. A concentric outer ring keeps *"the speaker that lights is the speaker that sounds"* literal **at the glyph**. Rejected: bars beside the glyphs (8 of them clutter a 448 × 560 plan and overlap the hull at the corners) and glyph brightness (not a readable scale, and the peak-hold marker has nowhere to live) |
| **D24** | Venue-screen mini-plan (D-1's inverted comparison) | **No action. Closed here, routed to the v1.1 backlog** | The comparison genuinely does invert — the rail's mini-plan is 170 px against the venue main column's 251 px of slack. But it is a layout change with **no requirement behind it**, the shipped 170 × 213 renders correctly and passes the guard, and **3.3's layout budget is on the ROOM screen, not the venue rail**. Correcting `VERIFICATION-3.2.md`'s premise (*"3.3 adds meters and an elevation strip to that same rail"*) is what turns this from a deferral into a decision |
| **D25** | The new column's layout assertion | **Fitted-box-against-its-stage, not `scrollHeight <= clientHeight`** | D-2 is not inherited as advice — the vacuous shape was **re-measured as present on this exact column** at this boundary (`592 === 592`). Any 3.3 assertion of that form would report green over an arbitrary overflow. A negative control must make the new assertion fire **while** the coarse one still passes, the same asymmetry NC3 demonstrated |
| **D26** | Elevation-strip orientation | **Depth horizontal (front left → rear right), height vertical, with `FRONT` / `REAR` labelled at the ends** | Conventional for a side elevation, and the only orientation that fits a 582 × ~160 box. But the plan's depth axis runs **top-to-bottom** while the strip's runs **left-to-right** — rotated 90° — so the ends must be labelled explicitly rather than relying on a shared orientation the two views do not have |
| **D27** | `ui_layout_check.js` §0 | **Print an ISO-8601 UTC timestamp from inside the gate**, one line | 3.2 verify found the ordering claim rests on a stamp transcribed from console output — a repo-wide search for `14:22:05` hits only planning prose, never a machine-produced artifact. One line makes the claim self-evidencing for every phase after it. It cannot repair 3.2's record and does not pretend to |
| **D28** | `VerifyPing`'s RNG seed | **Stays unseeded** (`juce::Random rng;`) | No 3.3 probe needs a reproducible ping stream. FUNC-04/3 holds **by construction** — `VerifyPing.cpp:258` hard-clamps with `jlimit` at `kPeakCeilDb` and the ping is a post-write overwrite — so reproducibility buys nothing a band assertion does not already have. Member-ownership already kills stream interleaving, which is the property that mattered. Recorded as a decision so it is not re-litigated as an oversight |

---

## Contract amendments — the §8 re-pins, taken at this boundary

Precedent: 2.2 discuss and 2.3 discuss both amended `ARCHITECTURE.md` and re-pinned at the discuss
boundary (`architecture_checksum_superseded` in STATUS frontmatter). 3.3 follows it.

**Three amendments were scheduled since 3.1 research. A fourth was found here.**

| # | Section | Amendment | Source |
|---|---|---|---|
| **1** | §4.1 | The tree is **three-node** — `SCENES` added as a sibling of `VENUE`, with the `setCustomStateCallbacks` consequence stated and FUNC-05's changed assertion shape spelled out | D6/D7 (3.1 discuss), scheduled |
| **2** | §4.3 | **Found at this boundary.** *"The UI reads and zeroes at ~30 Hz on a Timer"* — the rate is right, **the mechanism is not what this plugin uses**. Corrected to the fixed-interval JS pull, with N4's never-settling-promise hazard, the in-flight guard, C++-side `exchange(0)`, and the **frame basis** of the 0.5 / 0.12 coefficients | New |
| **3** | §6.3 | `SIDES` (and all six named scenes) stated as **derivable predicates**, with the default-venue result, the 6 % margin, the C++-side-derivation rule and the empty-set rule | N3 (3.1 research), scheduled |
| **4** | §6.3 | **Gesture brackets** added to the scene row and specified — `setValueNotifyingHost` alone opens no gesture | N1 (3.1 research), scheduled |

Amendment 2 is the one worth naming separately: it was not on anyone's list. §4.3's *"on a Timer"*
would have been read at plan as a contract requirement, and honouring it literally would have
undone 3.1's deliberate choice to keep `PluginEditor` `Timer`-free so `tests/ui-stub/` can render the
whole UI — which is what makes the pre-integration half of every layout gate possible.

### Checksum re-pin

| Contract | Superseded | New pin |
|---|---|---|
| `research/ARCHITECTURE.md` | `a8a358f4…9b6d4408` | **`32a85018…81d85273`** |

**The other three contracts are untouched and re-measured byte-exact after the edit:** `BRIEF.md`
`697a4f32…f6b9fbd6`, `parameter-spec.md` `b45f88dc…cbb9e02f`, `ROADMAP.md` `aec7d0ce…0137ee29`.
This is the **third** architecture re-pin (2.2 D2, 2.3 D2, 3.3 discuss).

---

## Constraints Identified

1. **The meters must measure the written buffer, never the solve.** §4.3 is explicit and the reason
   is R1. A probe that meters `v_i` would light correctly under a **bypassed channel map** — the
   exact NC3 failure caught at 2.2. The metering read sits after `GainStage` writes, indexed through
   `speakerToBuffer[i]`.
2. **N8 now has a second consumer.** `mapInvalid` sends `GainStage` to its `else` arm, which writes
   `out[ch][n] = ch == 0 ? sL : sR` — speaker 1 gets L, speakers 2–8 all get R at unity. The meters
   will therefore show seven channels lit at once in that state. **That is correct behaviour and must
   not be "fixed"**: it is the fold being visible, which is the whole point of metering the output.
   Any 3.3 assertion about what an invalid map "retains" must be made against the **snapshot**, never
   the output buffer.
3. **`PERF-01` must not regress.** UI-03/4 requires probe **AO** — the replaced global `operator new`
   family — re-run with metering live, still reading **0 allocations** across `processBlock`.
   `buffer.getMagnitude()` allocates nothing, but the assertion is a measurement, not an argument.
4. **The gradient must never recompute per frame.** The field depends on speaker positions and
   weights, **not** on source position, so dragging the puck must leave the recompute count
   unchanged. UI-04/2 requires this asserted **by a counter, not by eye** — which means the counter
   lives in C++ where a probe can read it.
5. **The gradient must be compared against the shipping `DbapSolver`.** UI-04/1: 20 grid points to
   1e-3. A JS re-derivation of the solve is a mirrored fixture and would drift silently.
6. **Canvas is a replaced element.** `o-textureforge-cursor-bug`: explicit `width`/`height` with a DPR
   backing store, never `left` + `right` stretch. `#plan-backdrop` is an existing canvas and the
   layout gate already runs at DPR 2 (§ line 407).
7. **No UI state may depend solely on a promise resolving** (N4). Completion is gated on
   `owner.isVisible()` and silently dropped when hidden — in Release there is no error and no
   rejection. This binds the 30 Hz meter poll hardest, because a `poll().then(poll)` recursion stops
   permanently the first time the editor is hidden.
8. **Every HTML-authored label stays authored in HTML.** `pattern_js_state_updater_overwrites_html_labels`:
   scene button labels are rendered, never written by a shared state updater; state goes to
   `data-*` + `aria-pressed`. `ROADMAP.md` Phase 3.3 names this as a test criterion in its own right.
9. **The page-module registry is derived, not enumerated.** P51 derives `PAGE_MODULES` from
   `Source/ui/public/js/*.js`; 3.3's new modules must land in it automatically. The enumeration hole
   has been caught **six** times in this project — the seventh will not be free.
10. **The native-function surface grows again.** 13 registrations today, grep-diffed in both
    directions by `ui_frontend_check.js` §3 against the C++ registrations, the derived call sites and
    the stub whitelist. Every new function must land in all three or §3 fails loudly.
11. **`FUNC-05`'s guarantee must be re-measured, not inherited.** The `SCENES` node changes the tree
    shape the guarantee holds over. FUNC-06/5 re-runs the 42-value bit-compare after the node exists.
12. **278 px, at 582 px wide.** Both new ROOM components fit inside it or the phase reports a
    deviation. Research measures; it does not compute.

---

## Open Questions for Research

| # | Question | Why it matters |
|---|---|---|
| **Q1** | Can `DbapSolver` be called on the **message thread** for the field sample, or does the field need its own instance? It carries instrumentation counters and is configured for the audio path | Determines whether UI-04 is a new solver call site or a second instance — and whether the recompute counter can be the existing instrumentation or must be new |
| **Q2** | What **grid resolution** does the backdrop use, and is it one native call returning the whole field or a streamed set? A 32 × 40 grid is 1280 solves × 8 `pow` | UI-04/2 forbids per-frame recompute but says nothing about the cost of the one recompute. `pow` budget is §3.3.5's concern |
| **Q3** | Does the field render `max_i v_i²` (ROADMAP) or something else? Confirm against the paper's figs 1–3 and against what `DbapSolver` actually returns | The gradient must be the field the solver produces, and the ROADMAP phrasing has not been checked against the shipped solver |
| **Q4** | What is the **exact shape of the meter payload** — linear peak or dB, one call or folded into `getStatus`? And does the C++ side do `exchange(0)` per speaker per read? | Determines whether the 2 Hz `getStatus` poll and a 30 Hz meter poll are two intervals or one. Two polls at different rates on one bridge is a new pattern here |
| **Q5** | Confirm N4's hidden-editor behaviour **empirically** for a 30 Hz interval — does the in-flight guard actually recover on re-show, or do requests queue? | D20's whole design rests on it. The N4 finding is read from JUCE source; nothing has run it at 30 Hz |
| **Q6** | What is the **probe shape for FUNC-06/6 (fade between two scenes)**? It is a property of the host, not of the plugin | The criterion says *"the gate that catches an implementation that latches"* — that needs an operational definition before plan can task it |
| **Q7** | What **venue fixture** does FUNC-06/2's permutation probe use, and what fixture makes a named scene resolve **empty** for D20? | Both criteria are vacuous without a fixture that distinguishes them from the traced layout, where 1 and 2 happen to be front |
| **Q8** | Does the elevation strip fit **582 × ~160** with the rake legible? Rake rise across 19.5 m is small — does the height axis need exaggeration, and if so is exaggerated height still honest? | UI-05/1 requires that changing `rakeRear` alone **visibly** changes the rear of the line. A true-scale strip may render that change as one pixel |
| **Q9** | Where do the **10 scene buttons** sit inside 278 px — one row of 10 at ~52 px, or two rows of 5? And does the `STORE` arm control (D22) need its own row? | Directly determines how much of the 278 px is left for the strip |
| **Q10** | Does `getVenueGeometry`'s existing payload need the **hull-classification** value for the `SIDES` predicate on the JS side, or is membership returned whole per D19? | D19 says whole; Q10 confirms no partial derivation sneaks back in |
| **Q11** | Is there a **negative control per gate family** available for 3.3's new sections, and specifically one that makes D25's fitted-box assertion fire **while** the coarse one passes? | The asymmetry is the proof the new assertion is not redundant. NC3 set the precedent |

---

## Non-goals for 3.3

Named so they do not drift in at execute:

- **Moving the venue-screen mini-plan** (D24). v1.1 backlog.
- **Repairing 3.2's Gate 4 ordering record** (D27 lands the timestamp; it cannot create a past record).
- **Seeding `VerifyPing`** (D28).
- **CI wiring.** Stage 4. 3.3 widens the gap a third time and says so.
- **The Logic hall session / D5 / `QUAL-01`'s audible clause.** Stage 4.
- **`COMPAT-02`, `COMPAT-04`.** Stage 4.
- **Any change to the 17 musical parameters or the 42 venue values.** `parameter-spec.md` is pinned
  and unmoved; scenes write existing parameters and store nothing new in `VENUE`.

---

## Risks specific to this phase

| Risk | Signature if it lands | Mitigation named here |
|---|---|---|
| **A meter lights the wrong speaker** | Entirely plausible on screen; only a ping cross-check catches it | §4.3's post-map metering + UI-03/2's cross-check against verify-ping stepping 1→8 |
| **A scene selects the wrong four** | Plausible on screen, and on the traced layout a fixed-index implementation looks **correct** | FUNC-06/2's permuted-numbering probe, which a fixed-index implementation must fail |
| **The gradient draws a field the solver does not produce** | Beautiful and wrong | UI-04/1 — 20 grid points against the shipping solver to 1e-3 |
| **The 278 px does not hold** | The strip overflows and `scrollHeight` still reports green | D25's fitted-box guard + a negative control with NC3's asymmetry |
| **The meter poll dies silently when the editor is hidden** | Meters frozen at their last value; no error anywhere | D20's fixed interval + in-flight guard; Q5 verifies it empirically |
| **Stage 3 runs long on two `nice` rows** | The hall test slips | §R7's descope path stays live at plan; UI-04's separate draw layer keeps the cost at a flag |

---

## Next Phase

**Ready for:** `research`

Eleven open questions, of which **Q1, Q5 and Q8** are the ones that can change the plan's shape:
whether the solver can be sampled off the audio thread, whether a 30 Hz pull survives a hidden
editor, and whether a true-scale elevation strip renders `rakeRear` visibly.

**Verification target for 3.3:** `FUNC-06`, `UI-03`, `UI-04`, `UI-05` — four rows, 18 criteria,
**zero partials declared in advance**. Closing them completes Stage 3.
