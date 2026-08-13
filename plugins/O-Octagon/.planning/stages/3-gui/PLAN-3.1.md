# Stage 3 — GUI · Phase 3.1 (Two-screen shell, Room plan, musical parameters) — Plan

**Plugin:** O-Octagon
**Stage:** 3 of 4 — GUI
**Phase:** 3.1 of 3
**GSD phase:** plan
**Date:** 2026-08-12
**Branch:** `feat/o-octagon` @ `a47cef88` (Stage 2 phases 2.2 / 2.3 work uncommitted)
**Inputs:** `CONTEXT-3.1.md` (D1–D7), `RESEARCH-3.1.md` (F1–F5, N1–N7, §2–§9),
`ROADMAP.md` Phase 3.1, `REQUIREMENTS.md` UI-02 (7 criteria),
`research/ARCHITECTURE.md` §3.1.6 / §4.1 / §4.3 / §6.1 / §6.3 / §R7 / §R8.

---

## Entry Check — contract checksums

Standing obligation at every boundary (`pattern_promotion_checksum_pins_replaced_file`). Recomputed
against the live files before anything else:

| Contract | SHA-256 measured at plan | STATUS frontmatter | Result |
|---|---|---|---|
| `BRIEF.md` | `697a4f32…f6b9fbd6` | `697a4f32…f6b9fbd6` | ✅ |
| `parameter-spec.md` | `b45f88dc…cbb9e02f` | `b45f88dc…cbb9e02f` | ✅ |
| `research/ARCHITECTURE.md` | `a8a358f4…9b6d4408` | `a8a358f4…9b6d4408` | ✅ |
| `ROADMAP.md` | `aec7d0ce…0137ee29` | `aec7d0ce…0137ee29` | ✅ |

**All four byte-exact. No pin moves at 3.1.** The three re-pins identified by research (§8 — §6.3's
`SIDES` derivation, §6.3's ungestured `setValueNotifyingHost`, §4.1's two-node tree) all describe
**scene** behaviour and are scheduled for **3.3 discuss**. This plan does not touch them.

`REQUIREMENTS.md` is **not edited at this boundary either** — unlike 2.3, where Task 1 corrected a
method QUAL-01 prescribed. UI-02's seven criteria as written at Stage 3 discuss are all executable,
including criterion 5, whose three-part probe (P45) satisfies the criterion as *written* rather than
requiring it to be reworded.

---

## Goal

**The performance surface works.** A WebView editor replaces the generic one; the Room screen draws
the room from the plugin's own geometry; all 17 musical parameters bind two-way; the source puck
drags with correctly-bracketed gestures; and every failure class this repo has scar tissue for is a
gate that runs, not a comment that hopes.

**`UI-02` closes ✅ and nothing else.** FUNC-02 / FUNC-04 / FUNC-05 / UI-01 are 3.2; FUNC-06 and
UI-03 / UI-04 / UI-05 are 3.3.

### What "works" means concretely at 3.1

| Delivered | Not delivered until |
|---|---|
| WebView shell, Room / Venue screen switcher (the Venue pane is an empty placeholder) | Venue screen contents — 3.2 |
| Room plan: derived envelope, hull overlay, 8 numbered glyphs, draggable puck | Level-gradient backdrop, elevation strip — 3.3 |
| 8 in-plan weight controls, sited at their speakers | Meters — 3.3 |
| All 17 `WebSliderParameterAttachment` bindings, `srcZ` as a bare slider | Named scenes, 4 user slots, `SCENES` node — 3.3 |
| Metres readout resolved against live geometry | Venue coordinate editing, `.venue` store, verify-ping, presets — 3.2 |
| SAFE-mode banner | `mapInvalid` surfacing, negotiated-set name display — 3.2 |
| `tests/ui-stub/`, `ui_frontend_check.js`, `ui_layout_check.js` | — |

### The three findings this plan must not lose

Research named them in its own handoff, and each is a task obligation below:

1. **N1 — gesture brackets are a 3.1 change, not a 3.3 one.** `WebSliderParameterAttachment`
   routes JS writes through `setValueAsPartOfGesture`, which is `setValueNotifyingHost` with **no**
   `beginChangeGesture`. The puck must bracket **both** `srcX` and `srcY`. → **P39**, Task 4, gate §12.
2. **The plan is 448 × 560, height-bound.** Layout is plan-left / controls-right, not a wide plan
   with controls beneath. → **P47**, Task 1.
3. **UI-02/5 needs a three-part probe**, and its end-to-end half is declared **now** as a 3.2 gate.
   → **P45**, Tasks 5 / 9 / 11.

---

## Requirement staging — read this before writing the verify report

Declared here so verify discovers nothing plan did not predict. This is the discipline that produced
zero verify-time surprises across all three Stage-2 phases.

| Requirement | Verdict predicted at 3.1 | Evidence |
|---|---|---|
| **UI-02** | ✅ **complete** | 7 criteria; see the mapping below |
| FUNC-02, FUNC-04, FUNC-05, UI-01 | untouched — 3.2 | — |
| FUNC-06, UI-03, UI-04, UI-05 | untouched — 3.3 | — |
| All 18 Stage-2 rows | must not regress | 62 probes re-run at 3.1 verify, plus the new BK–BM |

**Declared partial: none.** Per CONTEXT's prediction, 3.1's single requirement either closes or it
does not; if a criterion cannot be met it is declared at the 3.1 verify boundary **with a named
destination**.

### UI-02's seven criteria → where each is proved

| # | Criterion | Proved by | Task |
|---|---|---|---|
| 1 | Plan proportions follow the **derived envelope**, asserted against the plugin's computed envelope, never a hardcoded aspect | `ui_layout_check.js` §2 — read the rendered plan box at 1100 × 720, mutate the stub's `getVenueGeometry` bbox, re-measure | 5 |
| 2 | Hull overlay matches, **speakers 3 and 8 render `ON_EDGE`, not vertices** | `ui_layout_check.js` §3 (SVG `polygon.points.numberOfItems == hullCount`, on-edge class on glyphs 3/8) **+ C++ probe BK** (`classify(2) == classify(7) == ON_EDGE`, `getNumHullPoints() == 6`) | 5, 11 |
| 3 | Puck drag is **relative-delta**, not absolute cursor tracking | `ui_frontend_check.js` §12 (static form) **+** `ui_layout_check.js` §4 — an **off-centre** grab that must not jump, and §5's edge-reversal (N5) | 5, 9 |
| 4 | Canvas explicit `width`/`height` in `calc()` **+ DPR backing store** | `ui_layout_check.js` §6 at **DPR 1 and DPR 2** — the bug is invisible at one DPR | 5 |
| 5 | Metres resolved against the **live venue**; non-vacuity gate | **Three parts (P45):** (a) stub-mutation in Playwright, (b) C++ probe **BL** (`applyVenueEdit` → envelope accessors move), (c) static no-bbox-literals scan. End-to-end half → **3.2 gate, declared now** | 5, 9, 11 |
| 6 | **Rendered against the stub before C++ integration** | Task 5 runs before Task 6 exists; `ui_layout_check.js` §1 asserts zero console errors and all 17 controls present | 2, 3, 4, 5 |
| 7 | Bridge closure **both directions**, zero gaps | `ui_frontend_check.js` §3 — grep-diff, expected surface **exactly 3** | 9 |

### ROADMAP Phase 3.1 test criteria not carried by UI-02

`ROADMAP` lists two phase criteria that no UI-02 line covers. Both close here, and the split is
stated rather than left to verify:

- **"SAFE-mode banner appears on a stereo track and only there."** C++ half: probe **BM** negotiates
  mono / stereo / 7.1 / 7.1-SDDS / 5.1.2 and asserts `safeMode` tracks. JS half: `ui_layout_check.js`
  §9 drives the stub's `getStatus` through both states and asserts the banner appears and disappears.
- **"Every control moves its parameter; host automation moves every control."** The JS→parameter
  direction is proved by `ui_layout_check.js` §10 (drive each of the 17 controls, assert the stub
  records the write) plus `ui_frontend_check.js` §16's four-way closure. The parameter→JS direction is
  proved in stub-land by §10's echo half. **The in-host confirmation is folded into the Stage 4
  session** (D2) — declared here, not discovered at verify. Task 12's Standalone launch is the
  cheapest real-WebView evidence available this phase and is **not** D5.

---

## Plan Decisions

The P-series continues from Stage 2's **P36**. Phase 3.1 is **P37–P50**.

---

### P37 — `Source/PluginEditor.{h,cpp}`, class `OctagonEditor`, **one** relay list *(F1)*

`parameter-spec.md` freezes 17 parameters, all `juce::AudioParameterFloat`. Therefore:

```
kSliderIds = { srcX, srcY, srcZ, width, rolloff, blur,
               w1, w2, w3, w4, w5, w6, w7, w8,
               hullAtten, airAmount, outputGain }     // 17
kComboIds  = ∅        kToggleIds = ∅
```

One `std::vector<std::unique_ptr<juce::WebSliderRelay>>`, one attachment vector, no combo or toggle
machinery at all. **The relay-type split that bit O-ReverseDelay three separate times cannot occur
here** and the plan does not budget for it — a relay whose type does not match its parameter attaches
without error and produces a control that never updates, and that failure class is structurally
absent while the set stays all-float (constraint 10).

Member order is **relays → webView → attachments** and it is a destruction-order requirement, not
style (juce8-critical-patterns §3): C++ destroys in reverse, so an attachment declared last is
destroyed first, while the WebView it calls into is still alive. Attachment constructor takes
**three** arguments, the third `nullptr`.

`kSliderIds` is **not hand-written**. It is built from `oo::params::id(i)` over `0 … kCount`, which
is already the single mapping between the enum and the APVTS ids (`GainStage.h:75`). A second
transcribed list is `pattern_test_fixture_mirrors_drift_silently` with a `static_assert` two files
away that would not fire.

---

### P38 — The native surface is exactly **THREE**, and `getVenueGeometry` is **one** call *(§4)*

| # | Name | Returns | Called |
|---|---|---|---|
| 1 | `getParameterDefaults` | `{ id: engineeringValue }` × 17 | once at init |
| 2 | `getVenueGeometry` | envelope + centroid + rigScale + speakers + hull, **one object** | init, then on `venueGen` change |
| 3 | `getStatus` | `{ safeMode, outputSetName, numOutputChannels, mapInvalid, venueGen }` | polled at **2 Hz** |

Three, and the count is what `ui_frontend_check.js` §3 asserts **in both directions**. Keeping it at
three is D1's entire argument: *if the grep-diff gate is going to catch something, it catches it
against 17 bindings and not against 70.*

**`getVenueGeometry` is one call because three calls admit a torn read** — an envelope from venue A
composited with glyphs from venue B. That is the identical hazard §7.2 addresses on the audio thread
by acquiring one snapshot per control block, and P16 fixed at 2.2 by stamping the generation *inside*
the payload. One call, one consistent picture.

Fixed payload shape (the stub and the C++ must agree; §9's four-way closure asserts it):

```jsonc
{
  "envelope": { "minX": -1.30, "maxX": 14.30, "minY": 2.25, "maxY": 21.75,
                "degenerateX": false, "degenerateY": false },
  "centroid": { "x": 6.5000, "y": 12.4625 },
  "rigScale": 7.93165,
  "speakers": [ { "n": 1, "x": 0.50, "y": 4.50, "z": 4.50,
                  "label": "L", "class": "VERTEX" }, … ],   // 8
  "hull":     [ { "x": 0.50, "y": 4.50 }, … ],              // hullCount points, in order
  "hullCount": 6
}
```

Every field reads a live object the processor already owns — `getVenue()` for bbox / centroid /
`rigScale` / positions / labels, `getHull()` for `getHullPoints()`, `getNumHullPoints()` and
`classify(i)`. **The overlay therefore cannot drift from the behaviour it depicts**, because it calls
the same `ConvexHull2D` the solver calls; `classify()` was made a first-class return value at 2.1
(P11) for exactly this.

**`degenerateX` / `degenerateY` are computed in C++ against `oo::plane::kMinSpan`** and returned as
flags. `VenueModel::normToMetres()` pins a degenerate axis to its minimum rather than dividing by
zero (`VenueModel.h:185-192`); a naive JS `min + n·(max−min)` diverges from the C++ on exactly the
degenerate venues 2.1 spent a whole matrix on. JS branches on the flag, never on a transcribed
threshold.

---

### P39 — N1: the puck brackets **both** `srcX` and `srcY` *(RESEARCH F2 — the most consequential finding)*

JUCE 8.0.14, `juce_ParameterAttachments.cpp:324` → `:76`: `WebSliderParameterAttachment::
sliderValueChanged` calls `setValueAsPartOfGesture`, which calls `setValueNotifyingHost` with **no**
`beginChangeGesture` / `endChangeGesture`. The brackets come from `sliderDragStarted` /
`sliderDragEnded`, which the JS side must send.

**Rule, written once and generalised so 3.3 inherits it:** every pointer interaction that writes a
parameter opens with `sliderDragStarted()` on **every parameter it will write** and closes with
`sliderDragEnded()` on **every one of them**, with all intermediate `setNormalisedValue` calls inside.

- **The puck is the only two-parameter gesture at 3.1.** `pointerdown` → `srcX.sliderDragStarted()`
  **and** `srcY.sliderDragStarted()`; `pointerup` / `pointercancel` → both `sliderDragEnded()`.
- The 8 weight controls and the 7 ordinary sliders each bracket their own single parameter.
- **`pointercancel` and `lostpointercapture` must close the bracket too.** A drag interrupted by a
  window-focus change that never closes leaves the host in an open write region.

**Why this is not cosmetic:** the brackets are how a host opens and closes an automation-write
region. Logic's Touch and Latch modes key off them. An ungestured write **moves the sound and may not
be recorded** — in a plugin whose headline gesture is automating position. It is invisible to `ninja`,
to `auval` and to `pluginval`, which is why §12 of `ui_frontend_check.js` asserts the pairing
statically.

---

### P40 — The puck renders **optimistically during a drag**, authoritatively otherwise *(F4, N4)*

The JS→C++→JS round trip returns the **snapped** value asynchronously
(`RESEARCH-3.1` F4). A puck rendered from `getScaledValue()` on `valueChangedEvent` lags the pointer
by a full round trip and looks rubbery at 60 fps.

- **While `dragging`:** render from the local accumulator. Write through `setNormalisedValue` on
  every `pointermove`.
- **When not dragging:** render from `getScaledValue()` on `valueChangedEvent` — this is what makes
  host automation move the puck.
- **On `sliderDragEnded`:** discard the local position and re-sync from `getScaledValue()`.

**Never write back from a `valueChangedEvent` listener.** `WebSliderParameterAttachment::setValue`
sets `ignoreCallbacks` and `sliderValueChanged` carries a `jassertfalse` on that path
(`juce_ParameterAttachments.cpp:326`). The echo terminates naturally because both the relay and the
attachment compare-before-notify (F3), but a listener that re-writes a rounded value can ping-pong.
**Render on echo; never write on echo.**

`w1..w8` and the seven ordinary sliders do **not** need optimistic rendering — they are native range
inputs whose thumb the browser moves.

---

### P41 — The relative-delta accumulator is clamped **at the accumulator** *(N5)*

Research offered three mitigations for the sticky-edge failure. This plan picks the one that cannot
be got subtly wrong:

```js
acc.x = Math.min (1, Math.max (0, acc.x + dx / planWidthPx));   // clamp, then STORE BACK
srcX.setNormalisedValue (acc.x);
```

Clamping only the *written* value while letting the accumulator run past 1.0 is precisely the bug:
drag 200 px past the right edge, reverse, and the puck sits still for 200 px while the surplus
unwinds. Clamping the accumulator itself makes reversal immediate by construction.

**Non-vacuity:** `ui_layout_check.js` §5 drags well past an edge, reverses by one small step, and
asserts the puck's rendered `left` changes on that step. A surplus-carrying implementation fails it.

---

### P42 — The venue envelope is **cached and invalidated by `venueGen`**; no native call in a pointermove handler *(Q4, N6)*

`getNativeFunction` is not a call, it is an **async round trip**
(`juce_gui_extra/native/javascript/index.js:73-92`) whose promises can resolve **out of order**. A
readout that writes `textContent` inside each `.then()` can apply an older response after a newer one
and display a *stale* metre value while the puck is current. At 60–120 pointer events/second it is
also 60–120 JSON encode/decode round trips across the WebKit boundary for a value that is a linear
function of a quantity that changes only when the venue changes.

**Shape:** one `getVenueGeometry` at init → cache → compute metres locally from the plugin's own
numbers → refetch when `venueGen` moves on the `getStatus` poll.

| Trigger | Phase | Mechanism |
|---|---|---|
| Editor opens | 3.1 | one `getVenueGeometry` at init |
| A venue change from **any** source (session restore, `.venue` load, host preset) | 3.1 | `venueGen` on the 2 Hz `getStatus` poll |
| A venue edit made in the UI | 3.2 | the edit originates in JS, so JS invalidates its own cache |

Row 2 is the staleness hole this closes, and it costs **nothing extra**: the poll already exists for
the SAFE banner, and the counter already exists for the solver's dirty check
(`VenueSnapshot::getGeneration()`, and `publishSnapshot()` is the single funnel for every venue
change).

**This is not the failure `pattern_webview_knob_readout_scaled_value` warns about.** That pattern is
about a JS table of *transcribed constants* drifting from the C++ `NormalisableRange`. Here the
numbers are the plugin's live geometry, fetched from the plugin, refreshed when the plugin's geometry
changes. The pattern's rule — *the UI must ask the processor* — is honoured, and P45(c) statically
proves there is no constant hiding behind it.

`ui_frontend_check.js` §14 asserts no `getNativeFunction` result is awaited inside a `pointermove`,
`mousemove` or `requestAnimationFrame` handler.

---

### P43 — `std::atomic<bool> safeMode` in `prepareToPlay()`; the negotiated-set **name** is never an atomic *(Q9)*

Two ways to surface SAFE mode; this plan takes the second:

- Derive it in the native function from `getBus(false, 0)->getCurrentLayout()` — zero new state, but
  it places a second copy of *"which sets count as SAFE"* a long way from
  `isBusesLayoutSupported()` (`PluginProcessor.cpp:181`), where the first copy lives.
- ✅ **One `std::atomic<bool> safeMode`, written in `prepareToPlay()`** beside `preparedYet`.
  `prepareToPlay` is already the single site that knows the negotiated layout, so the derivation
  stays adjacent to the predicate it mirrors.

**The negotiated container *name* must NOT be an atomic.** A `juce::String` written on one thread and
read on another is a race. It is resolved **inside the native function on the message thread** —
`getBus (false, 0)->getCurrentLayout().getDescription()` — so no cross-thread string ever exists.
Any surrounding text is built with `<<`, never `juce::String("… — …")` (constraint 9: the ASCII-only
`const char*` constructor, with no compiler warning).

`getVenueGeneration()` is added alongside, forwarding `venuePublisher.getGeneration()`. **Nothing else
changes in the processor** — no new parameters (constraint 10), no `AsyncUpdater` (§2.1: the plain
`preparedYet` flag means there is no queued apply to cancel, and adding one would *create* the
obligation the design has avoided).

---

### P44 — The stub's 17 ranges are **asserted against `createParameterLayout()`**, not trusted

The stub must carry a range table — it is JS with no plugin behind it. The precedent's own comments
record that table drifting: `delayTime` after a v1.0.1 range change, plus **four separate incidents**
(`grainTilt`, `grainCount`, `tukeyTaper`, `driftRate`) where the range minimum was not the shipped
default and the stub rendered a state the plugin never ships in.

**So the table is written, and then a static gate parses `createParameterLayout()` out of
`PluginProcessor.cpp` and diffs it.** This is the fix for the mirror-drift class rather than a repeat
of it (`pattern_test_fixture_mirrors_drift_silently`).

Two neutral-default traps specific to O-Octagon, both of which the gate pins:

- **`w1..w8` default to 1.0, not the range minimum.** A stub defaulting to 0.0 renders eight silent
  speakers — a state the plugin never ships in, and one in which every UI-02 criterion still passes.
- **`blur` defaults to 0.10 and `airAmount` to 0.35.**

All 17 skews are linear, so no `skewForCentre` modelling is needed.

---

### P45 — UI-02/5 closes on a three-part probe; its end-to-end half is declared **now** as a 3.2 gate

UI-02/5's non-vacuity gate is *"with the puck stationary, editing a venue coordinate must change the
readout."* **At 3.1 there is no Venue screen** — coordinate editing lands at 3.2. Testing it as
literally written is impossible this phase, and passing it vacuously would be worse than failing it.

| Part | Where | What it proves | Task |
|---|---|---|---|
| **(a)** JS half | `ui_layout_check.js` §7 — the stub's `getVenueGeometry` is mutable: read the readout with the puck stationary, mutate the bbox, trigger the refresh path, assert the readout changed | **A JS min/max map fails this.** The direct analogue of the criterion | 5 |
| **(b)** C++ half | Render-harness probe **BL** — `applyVenueEdit()` with one speaker moved, then read the same accessors `getVenueGeometry` reads and assert the envelope moved | The C++ side is live, without needing a WebView | 11 |
| **(c)** Static | `ui_frontend_check.js` §13 — the `getVenueGeometry` lambda body reads `processorRef.getVenue()` and contains **no numeric literal** for any bbox bound | The only way (a) and (b) could both be correct and still be wired to a constant | 9 |

**(a) + (b) + (c) closes UI-02/5 at 3.1.** The end-to-end version — type a coordinate on the Venue
screen, watch the Room readout move — **is a 3.2 gate, and this plan declares it now** so 3.2
inherits it rather than discovering it. It goes in `SUMMARY-3.1.md` and in STATUS frontmatter.

---

### P46 — Three layers — `<canvas>` backdrop / `<svg>` geometry / DOM controls — driven by **one** projection function

The alternative (draw everything on the canvas) makes UI-02 criteria 1, 2 and 3 **unmeasurable
without a debug back-channel**, because a canvas has no boxes to measure and no vertices to count.
In a plugin whose whole discipline is *measure, do not assert*, that is disqualifying.

| Layer | Contents at 3.1 | Why |
|---|---|---|
| `<canvas id="plan-backdrop">` | envelope fill | The raster surface. 3.3's level-gradient field needs per-pixel work; the DPR backing store (UI-02/4) is this element's property |
| `<svg id="plan-geometry">` | hull `<polygon>`, 8 speaker markers | `polygon.points.numberOfItems` is a **DOM fact** — UI-02/2's vertex count and the `ON_EDGE` class on glyphs 3 and 8 are readable by Playwright without instrumenting the app |
| DOM overlay | 8 weight controls, the puck, the numbered speaker labels | Hit targets Playwright can grab **off-centre** (UI-02/3); relay bindings the stub render gate can enumerate (UI-02/6) |

**All three are positioned by one exported `metresToPx()` in `roomplan.js`.** Three layers with three
coordinate derivations would be three things to drift; one function used by all three cannot.
`ui_frontend_check.js` §19 asserts no second coordinate mapping exists (no other
`(v - minX) / (maxX - minX)` form anywhere in the JS).

**The canvas is still sized with explicit `width`/`height` in `calc()` plus a DPR backing store** —
canvas is a CSS *replaced element*, so left+right does **not** stretch it, and the bug is invisible at
one DPR (`o-textureforge-cursor-bug`). §6 of the layout gate measures at DPR 1 **and** 2.

---

### P47 — Layout is **plan-left / controls-right**, and the 1100 × 720 gate **measures** rendered boxes *(§3, D7)*

The derived envelope for the §OQ4 default venue is **15.60 m × 19.50 m, aspect 0.800 — portrait**.
Inside 1100 × 720, with a ~56 px header and a ~40 px footer, the plan is **height-bound at ≈ 448 × 560
px**. Widening the window would not enlarge it.

```
┌─ 1100 ──────────────────────────────────────────────────────────┐
│ header 56   O-OCTAGON  ·  [ROOM] [VENUE]        SAFE banner     │
├──────────────────┬──────────────────────────────────────────────┤
│  plan  448×560   │  controls  ~580                              │
│  canvas+svg+DOM  │  srcX srcY srcZ · width rolloff blur         │
│  puck, 8 weights │  hullAtten airAmount outputGain              │
├──────────────────┴──────────────────────────────────────────────┤
│ footer 40   metres readout (mono, tabular)                      │
└─────────────────────────────────────────────────────────────────┘
```

**The row arithmetic is not the gate and must not be treated as one.** This repo has a dated scar for
exactly this reasoning: `pattern_flex1_container_slack_invisible_to_row_sum` — O-ReverseDelay carried
**93.5 px of phantom slack through five releases** because every comment reasoned about rows and never
subtracted from the frame height. **The gate is a rendered measurement of the real page at exactly
1100 × 720** (`ui_layout_check.js` §8: `document.documentElement.scrollWidth <= 1100` and
`scrollHeight <= 720`), which is also why `ui_tooltip_clamp_check.js` exists separately from its
static sibling in the precedent (`pattern_tooltip_clamp_gate_viewport_sensitive`).

**Aesthetic (D3), plugin-local at 3.1.** All tokens live in one `:root` block in `styles.css` so 3.3
has something to extract if `/ui-template-library` is invoked at 3.3 verify (Q10 — not decided here):

```
--ground  #1A1613   --panel #241E1A
--ink     #F0E8DC   headings serif, wide tracking; botanical watermark, low alpha
--data    mono, font-variant-numeric: tabular-nums
--level   brass → pale-gold ramp        (the interface's only hue)
```

**`font-variant-numeric: tabular-nums` on every metre and level readout is a gate, not taste**
(`ui_frontend_check.js` §18). UI-01 will require 42 values column-aligned at 3.2, and a mis-scanned
metre is a measurement error that propagates silently into the solve.

**`setSize(1100, 720)` and the CSS must agree**, asserted by §17 — the precedent records this pair
diverging across three separate resizes.

---

### P48 — `createEditor`'s two arms **diverge**; the generic editor survives only in the `#else`

Today both arms are identical and the `#if` is provably inert — deliberately so (G8), added at Stage 1
*before* the swap rather than after it, because added afterwards it is a build break in a target
nobody is looking at (`pattern_render_harness_breaks_on_webview_editor`). 3.1 is the phase it was
written for:

```cpp
   #if JUCE_WEB_BROWSER
    return new OctagonEditor (*this);
   #else
    // The render harness compiles this TU with JUCE_WEB_BROWSER=0, under which
    // WebBrowserComponent's types do not exist. 29 probes die silently without this arm.
    return new juce::GenericAudioProcessorEditor (*this);
   #endif
```

`PluginEditor.h` is included **only from inside the guard**. `PluginEditor.cpp` is added to
`target_sources(OuariconOctagon)` and **must never** be added to
`tests/render-harness/CMakeLists.txt` — that file already carries the warning, and
`ui_frontend_check.js` §11 asserts the absence.

The Stage-1 comment *"It is deleted at Phase 3.1; nothing may come to depend on it"* is honoured for
the plugin path and **corrected** for the harness path: the generic editor is not deleted, it is
demoted to the `#else` arm, where COMPAT-04's Standalone eyeball no longer needs it because the
Standalone build takes the `#if` arm.

---

### P49 — The Playwright gate **FAILS** when Playwright is missing. It must not SKIP

**Verified at plan: Playwright is not currently resolvable in this environment** (`npx playwright`
prompts to install; no `~/.cache/ms-playwright`). The precedent's resolver
(`ui_tooltip_clamp_check.js:172-201`) prints `SKIP` and **exits 0** when it cannot find one.

**That behaviour is wrong for 3.1 and this plan deviates from the precedent.** In O-ReverseDelay the
Playwright gate was *supplementary* — a static sibling already covered the mechanism, and the browser
run added viewport sensitivity. Here it is the **sole** evidence for UI-02 criteria **1, 3, 4 and 5**.
A SKIP that exits 0 would let 3.1 close UI-02 against nothing, which is the exact class of vacuity
this project has caught five times (NC6 at 2.3, NC3 at 2.2, the `width`-wired-to-nothing sweep, the
zipper probe with no liveness gate, the worktree with no tracked files).

**`ui_layout_check.js` prints the install command and exits non-zero.** `npx playwright install
chromium` is a Task 0 prerequisite, recorded in the plan rather than discovered at execute.

The **CI** consequence is unchanged and already a Stage-4 residual: these gates are local-only, like
the 62 C++ probes. Stage 3 *widens* that gap, as CONTEXT predicted.

---

### P50 — Probe accounting: two JS gate files, three new C++ probes **BK–BM**, series continues at BK

The C++ letter series runs `A … BJ` with no gaps — **62 probes** (33 unit + 29 render-harness),
verified at 2.3. 3.1 adds three, all in the **render harness** (they need a live processor):

| Probe | Asserts | Serves |
|---|---|---|
| **BK** | On the §OQ4 default venue: `getNumHullPoints() == 6` and `classify(2) == classify(7) == ON_EDGE` (0-based → speakers 3 and 8) | UI-02/2's C++ half |
| **BL** | `applyVenueEdit()` with one speaker moved → `getVenue()`'s bbox accessors move, by the expected sign and magnitude | UI-02/5(b) — P45 |
| **BM** | `safeMode` is `true` under mono and stereo output, `false` under 7.1 / 7.1-SDDS / 5.1.2 | ROADMAP's SAFE-banner criterion, C++ half |

**Total after 3.1: 65 C++ probes (33 unit + 32 harness), 0 failures required.**

The JS gates are a **separate family** and are counted separately — sections, not letters:
`ui_frontend_check.js` (20 static sections) and `ui_layout_check.js` (10 Playwright sections). Mixing
them into the letter series would make "62 probes, 0 failures" mean two different things in two
documents.

**Nothing in the unit target changes**, and Gate 11 re-verifies its link line still has no `juce_dsp`
and no `juce_gui_extra`.

---

## Tasks

Ordering is load-bearing: **Tasks 1–5 complete before Task 6 exists.** UI-02/6 requires the page to
render against the stub *before* C++ integration, and D4 removed the browser-iteration safety net that
would otherwise catch a top-level TDZ throw on the first reload
(`pattern_module_toplevel_init_tdz`).

---

### Task 0 — Prerequisite: Playwright *(P49)*

```bash
npx playwright install chromium
```

One command, ~2 min, once. **Not optional** — P49 makes `ui_layout_check.js` fail rather than skip,
and four UI-02 criteria rest on it. If it cannot be installed, that is a **blocker declared at
execute**, not a silently green phase.

---

### Task 1 — `Source/ui/public/` scaffolding: HTML, CSS, and the two JUCE files *(P46, P47)*

**Create:**

```
Source/ui/public/index.html
Source/ui/public/css/styles.css
Source/ui/public/js/juce/index.js                 ← copied VERBATIM
Source/ui/public/js/juce/check_native_interop.js  ← copied VERBATIM
```

Both JUCE files are copied unmodified from
`/Users/taylorbrook/JUCE/modules/juce_gui_extra/native/javascript/` (verified present at plan).
`check_native_interop.js` **must** be embedded and served, or the page can hang
(`juce8-critical-patterns` §13).

`index.html`:
- `type="module"` on **every** script tag.
- Header with the Room / Venue switcher; the Venue pane is an empty labelled placeholder at 3.1.
- The three plan layers per P46, in z-order: `<canvas id="plan-backdrop">`, `<svg id="plan-geometry">`,
  `<div id="plan-controls">`.
- All 17 controls present in the DOM with ids derivable from `oo::params::id()`.
- **Labels are HTML-authored and must never be written via `textContent`**
  (`pattern_js_state_updater_overwrites_html_labels` — a shared state updater writing `textContent`
  erases them; render the value into a dedicated `<span>`, keep the label in its own node).

`styles.css`:
- `html`, `body` and `.frame` at exactly **1100 × 720** (P47 §17 asserts this against `setSize`).
- The `:root` token block from P47.
- `font-variant-numeric: tabular-nums` on the metre/level readout classes (P47, §18).
- Canvas sized with explicit `width`/`height` in `calc()` — **never** `left`+`right`
  (`o-textureforge-cursor-bug`).

**No filename contains a hyphen** (`critical_binary_data_strips_hyphens`: `room-plan.js` →
symbol `roomplan_js`). Author them hyphen-free rather than remembering the transform.

---

### Task 2 — `tests/ui-stub/` *(P44, UI-02/6, D4)*

**Create** `tests/ui-stub/juce-stub.js` and `tests/ui-stub/serve-stub.sh`.

`serve-stub.sh` copies `Source/ui/public` to a temp root and swaps **only** `js/juce/index.js` for
the stub, so *the page under test is byte-identical to production*. Copy the precedent's mechanism
verbatim (`plugins/O-ReverseDelay/tests/ui-stub/serve-stub.sh`); O-Octagon has no
`preset-manager.js` line to carry at 3.1.

`juce-stub.js` models — and O-Octagon's stub is **smaller** than the precedent's, which is worth
stating so nobody budgets for a richer one:

| Surface | O-ReverseDelay | O-Octagon 3.1 |
|---|---|---|
| `getSliderState` + ranges | 20 | **17** |
| `getComboBoxState` / `getToggleState` | 4 / 1 | **0 / 0** |
| `getNativeFunction` whitelist | 13 | **3** |
| `backend.addEventListener` | not modelled | **not needed** — pull-only |
| Canvas | n/a | **nothing to stub** — a real browser canvas *is* the thing under test |

Three requirements the precedent learned the hard way:

1. **Ranges + defaults per P44** — including `w1..w8 = 1.0`, `blur = 0.10`, `airAmount = 0.35`.
2. **An unknown native-function name must REJECT, not resolve.** That rejection is how a bridge gap
   surfaces in the stub instead of as a dead control in a DAW.
3. **A polled stub value must move**: `getStatus` returns a **walking `venueGen`**, and
   `getVenueGeometry` is **mutable** — that mutability is what makes P45(a) possible.

The stub also models the **echo**: `setNormalisedValue` records the write and fires
`valueChangedEvent` with the snapped value, so §10 can prove both directions.

---

### Task 3 — `Source/ui/public/js/app.js` *(P38, P40, P42, P43)*

The shell: screen switcher, the 17 bindings, readouts, SAFE banner, the status poll, the venue cache.

- **Import the `Juce` ES-module namespace**, not `window.__JUCE__`
  (`critical_juce_webview_namespace_vs_postmessage` — panels go silently dead otherwise).
- **A single `init()` call as the LAST statement of the module.** No top-level initializer may reach
  a not-yet-initialised binding (`pattern_module_toplevel_init_tdz` — a TDZ throw kills every later
  initializer, and §2 of the static gate checks the form).
- Readouts come from `SliderState.getScaledValue()` and refresh on `propertiesChanged`. The format
  table carries **units only, no range constants**
  (`pattern_webview_knob_readout_scaled_value`).
- Dblclick-reset reads `getParameterDefaults`, never a JS default table.
- `getStatus` polled at **2 Hz** from a JS interval — **not** a `juce::Timer`. Following the
  `getGrainMeter` precedent, this keeps the editor free of a Timer and, the part that matters, keeps
  the ui-stub able to render the page without modelling `backend.addEventListener`.
- Venue cache + `venueGen` comparison per P42.
- SAFE banner from `getStatus().safeMode`.
- The roomplan module is initialised **from inside `init()`**, hoisted and `try`/`catch`'d, so a plan
  failure cannot take the 17 controls down with it.

---

### Task 4 — `Source/ui/public/js/roomplan.js` *(P39, P40, P41, P46)*

The plan: one exported `metresToPx()`, three layers drawn/positioned from it, the puck, the 8 in-plan
weight controls.

- **`metresToPx()` is the only coordinate mapping in the codebase** (P46, §19).
- Canvas: DPR backing store — `canvas.width = rect.width * devicePixelRatio`, likewise height, then
  `ctx.scale(dpr, dpr)`. Re-run on `devicePixelRatio` change.
- SVG: hull `<polygon>` with exactly `hullCount` points in order; 8 speaker markers, each carrying
  its `class` from the payload so `VERTEX` / `ON_EDGE` / `INTERIOR` are visible in the DOM.
- DOM: puck + 8 weight controls positioned via `metresToPx()`.
- **Puck gesture per P39** — `sliderDragStarted()` on **both** `srcX` and `srcY` at `pointerdown`,
  both `sliderDragEnded()` at `pointerup`, **and at `pointercancel` / `lostpointercapture`**.
- **Relative-delta with the accumulator clamped** per P41 — never absolute cursor tracking
  (juce8-critical-patterns §16).
- **Optimistic render during drag, authoritative otherwise** per P40. Render on echo; never write on
  echo.

---

### Task 5 — `tests/ui_layout_check.js` — Playwright at **exactly 1100 × 720** *(P45a, P47, P49)*

Runs **before** any C++ exists. Model: `plugins/O-ReverseDelay/tests/ui_tooltip_clamp_check.js`
(serve the real page, drive it at the shipping viewport, **measure rendered boxes**).

| § | Assertion | Serves |
|---|---|---|
| 1 | Page renders with **zero console errors**; all 17 controls present in the DOM | UI-02/6 |
| 2 | Plan box aspect matches the **returned** envelope; mutate the stub bbox, re-measure, aspect follows | UI-02/1 |
| 3 | `polygon.points.numberOfItems == hullCount`; glyphs 3 and 8 carry the on-edge treatment | UI-02/2 |
| 4 | **Off-centre** puck grab does not jump | UI-02/3 |
| 5 | Drag past an edge, reverse one step → puck moves immediately | N5 / P41 |
| 6 | `canvas.width == round(rect.width · dpr)` at **DPR 1 and DPR 2** | UI-02/4 |
| 7 | Puck stationary, mutate stub bbox, readout **changes** | UI-02/5(a) |
| 8 | `scrollWidth <= 1100` **and** `scrollHeight <= 720` — measured, not computed | P47 |
| 9 | `safeMode: true` → banner present; `false` → absent | ROADMAP |
| 10 | Each of the 17 controls writes its parameter; the stub echo moves each control | ROADMAP |

**Exits non-zero when Playwright is unavailable** (P49). Exit code = number of failed assertions.

**§2, §3 and §7 assert against the values the stub RETURNED**, never against a literal
(`pattern_test_fixture_mirrors_drift_silently`). §4's off-centre grab is what makes it non-vacuous —
a centred grab passes under absolute tracking too.

**Gate: this must pass before Task 6 begins.** That ordering *is* UI-02 criterion 6.

---

### Task 6 — `Source/PluginEditor.{h,cpp}` *(P37, P38, P43, P48)*

**Create.** Reference implementation to copy in shape:
`plugins/O-ReverseDelay/Source/PluginEditor.{h,cpp}`.

- Member order **relays → webView → attachments** (P37), with the destruction-order comment.
- 17 relays and 17 attachments built by looping `oo::params::id(i)`, reaching parameters through
  `processorRef.getAPVTS().getParameter(id)` (returns `RangedAudioParameter*` — exactly what the
  attachment's first argument requires). Attachment takes **three** arguments, third `nullptr`.
- `getResource` matches **bare paths by direct equality**. Never construct or strip a scheme
  (`critical_webview_resource_provider_and_schemes` — URL schemes differ per platform). `charset=utf-8`
  on every text resource.
- The **three** native functions of P38, and no fourth.
- `getStatus` resolves the negotiated-set name on the message thread per P43; text built with `<<`.
- `setSize (1100, 720)` — **the single `setSize` call**, and the CSS must agree (§17).
- The header comment records the native-fn count as **THREE** and why the count is load-bearing.
- **MSVC habits start now** (constraint 11): no non-static `constexpr` inside a lambda (C3493), no
  `SafePointer(this)` init-capture in a nested lambda. Neither has a call site at 3.1 — the
  `FileChooser` work is 3.2 — but both are write-time habits, not port-time fixes.

---

### Task 7 — `PluginProcessor` deltas *(P43, P48)*

**Modify** `Source/PluginProcessor.h` / `.cpp`. Exactly three changes, and **nothing else**:

1. `std::atomic<bool> safeMode { false };` beside `preparedYet`; written in `prepareToPlay()` from
   the negotiated output layout, adjacent to the `isBusesLayoutSupported()` rule it mirrors. Public
   reader `isSafeMode()`.
2. `std::uint32_t getVenueGeneration() const noexcept` forwarding `venuePublisher.getGeneration()`.
   Message thread / diagnostics — the comment must repeat that **the dirty check must not read it**
   (it reads `snapshot.generation`, which arrives with the geometry it belongs to — the H1 bug P16
   made unreachable).
3. `createEditor()`'s arms diverge per P48; `#include "PluginEditor.h"` inside the guard only.

**No new parameters** (constraint 10). **No `AsyncUpdater`** (§2.1). **No `SCENES` handling** — that
is 3.3.

---

### Task 8 — `CMakeLists.txt` *(F5)*

```cmake
juce_add_binary_data(OuariconOctagon_UIResources
    NAMESPACE   UIBinaryData          # distinct namespace — constraint 7
    HEADER_NAME UIBinaryData.h
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/css/styles.css
        Source/ui/public/js/app.js
        Source/ui/public/js/roomplan.js          # NOT room-plan.js — constraint 6
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)
target_link_libraries(OuariconOctagon PRIVATE OuariconOctagon_UIResources)
```

Plus `Source/PluginEditor.cpp` in `target_sources(OuariconOctagon)`, and the Stage-1 comment
*"No juce_add_binary_data target at Stage 1 — the WebView UI lands at Phase 3.1"* replaced.

`NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `JUCE_WEB_BROWSER=1` and
`JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` are **already present** — set at Stage 1 precisely so
this swap would be safe. Verified at plan; §10 of the static gate re-asserts both WebView2 flags
(`critical_webview2_static_linking` — without static linking the Windows WebView renders blank in DAW
hosts, silently).

**`tests/render-harness/CMakeLists.txt` is NOT modified.**

---

### Task 9 — `tests/ui_frontend_check.js` — 20 static sections *(P39, P42, P44, P45c, P46, P47)*

**Create.** Model: `plugins/O-ReverseDelay/tests/ui_frontend_check.js` (15 sections). Exit code =
number of failed assertions.

| § | Assertion | Pattern / decision |
|---|---|---|
| 1 | `app.js` and `roomplan.js` parse as ES modules; `type="module"` on every script tag | a SyntaxError kills the whole UI |
| 2 | Single `init()` call as the **last** statement | `pattern_module_toplevel_init_tdz` |
| 3 | **Bridge closure both directions; surface exactly 3** | `pattern_webview_native_fn_bridge_gap`, UI-02/7 |
| 4 | Readouts via `getScaledValue()`; the format table carries units only, **no range constants** | `pattern_webview_knob_readout_scaled_value` |
| 5 | Dblclick reset reads `getParameterDefaults`, never a JS table | — |
| 6 | HTML-authored labels never written via `textContent` | `pattern_js_state_updater_overwrites_html_labels` |
| 7 | The `Juce` ES-module namespace, not `window.__JUCE__` | `critical_juce_webview_namespace_vs_postmessage` |
| 8 | Resource provider hard-codes no scheme; matches bare paths | `critical_webview_resource_provider_and_schemes` |
| 9 | Three-way closure: HTML/JS refs == `getResource` entries == `juce_add_binary_data` SOURCES | — |
| 10 | Both Windows WebView2 CMake flags set | `critical_webview2_static_linking` |
| 11 | Member order relays → webView → attachments; **`PluginEditor.cpp` absent from the harness target** | `pattern_render_harness_breaks_on_webview_editor` |
| 12 | **Gesture pairing:** every `setNormalisedValue` in a pointer handler sits inside a `sliderDragStarted`/`Ended` pair, and the puck brackets **both** `srcX` and `srcY`, including on `pointercancel` | **N1 / P39** |
| 13 | `getVenueGeometry` reads `processorRef.getVenue()` and contains **no numeric literal** for a bbox bound | **P45(c)** |
| 14 | **No `getNativeFunction` awaited inside `pointermove` / `mousemove` / rAF** | **P42** |
| 15 | The stub's 17 ranges + defaults **match `createParameterLayout()`** parsed from source | **P44** |
| 16 | Four-way closure: `createParameterLayout` == `params::id()` == `kSliderIds` == DOM control ids, with a stub range each | a control wired in three of four places is silently dead |
| 17 | **1100 × 720 in `setSize` AND in `html`/`body`/`.frame`** | P47 |
| 18 | `font-variant-numeric: tabular-nums` on every metre/level readout class | **D3 / P47** |
| 19 | **One projection function** — no second `(v-min)/(max-min)` coordinate form | **P46** |
| 20 | MSVC habits: no non-static `constexpr` in a lambda; no `SafePointer(this)` init-capture in a nested lambda | `critical_msvc_constexpr_lambda_capture`, `critical_msvc_safepointer_init_capture_nested_lambda` |

---

### Task 10 — Re-run `ui_layout_check.js` against the integrated page

Same file, unchanged. It ran pre-integration against `Source/ui/public` + the stub; re-run it now that
`getResource` serves the same tree, to prove Task 6's resource list did not drop a file. §9's
three-way closure catches a missing SOURCES entry statically; this catches a `getResource` path that
does not match what the HTML asks for.

---

### Task 11 — Render-harness probes **BK–BM** *(P50)*

**Modify** `tests/render-harness/main.cpp`. Three probes, continuing the letter series.

- **BK** — default venue: `getNumHullPoints() == 6`; `classify(2)` and `classify(7)` both `ON_EDGE`;
  the other six `VERTEX`. UI-02/2's C++ half. Print the measured classifications so a future venue
  change reads as a diff rather than a bare FAIL.
- **BL** — `applyVenueEdit()` with one speaker moved outward; assert the bbox accessors
  `getVenueGeometry` reads move by the expected sign and magnitude, and that `getVenueGeneration()`
  advanced. UI-02/5(b) — **P45**.
- **BM** — negotiate mono, stereo, 7.1, 7.1-SDDS and 5.1.2 in turn; assert `isSafeMode()` is
  `true, true, false, false, false`. The harness already constructs bus layouts programmatically.

Probe count after 3.1: **65** (33 unit + 32 harness).

---

### Task 12 — Gates

Every gate is **run at execute and RE-RUN from scratch at verify**, not read out of `SUMMARY-3.1.md`.
This is the 2.3 discipline that caught four mis-attributions.

| # | Gate | Pass condition |
|---|---|---|
| 1 | Clean 3-format build + both test targets, forced full recompile | exit 0, **zero `warning:` / `error:` / `FAILED`** |
| 2 | `node tests/ui_frontend_check.js` | exit 0, 20 sections |
| 3 | `node tests/ui_layout_check.js` | exit 0, 10 sections — **and it must not SKIP** (P49) |
| 4 | Stub render (Task 5 §1) ran **before** integration | recorded in SUMMARY with its timestamp |
| 5 | `auval -v aufx OuOc OuDv` | **AU VALIDATION SUCCEEDED** |
| 6 | pluginval s10, VST3 ×3 / AU ×3 | all six exit 0, zero `FAILED` |
| 7 | Both C++ test targets | **65 probes, 0 failures**, exit 0 / exit 0 |
| 8 | `gen_dbap_reference.py --check` | exit 0 |
| 9 | 17 params vs `parameter-spec.md`, three sides | 17/17, none hand-transcribed |
| 10 | `createEditor` guard present; `PluginEditor.cpp` absent from the harness target | both ✓ |
| 11 | Unit-target link line has no `juce_dsp` **and no `juce_gui_extra`** | ✓ |
| 12 | Contract checksums | all four byte-exact, **no pin moved** |
| 13 | **Standalone launch, macOS — HUMAN, ~5 min** | the window opens **1100 × 720**, the plan draws, the puck drags, the 17 controls move. See below |

**Gate 13 is not D5.** D5 is the ~15 min Logic session for QUAL-01's audible clause, folded to Stage 4
by D2 and untouched here. Gate 13 is a **launch-and-look**: no Logic, no automation, no measurement.
It exists because **no automated gate in this repo can prove a WebView loads at all** — the stub
renders in Chromium, not in WKWebView, and the blank-WebView class
(`critical_webview2_static_linking`, `critical_webview2_runtime_gotchas_windows`) ships green through
build, `auval` and `pluginval`. D4 removed the browser-iteration safety net; this is the cheapest
replacement.

---

### Task 13 — `SUMMARY-3.1.md` + `STATUS.md`

`SUMMARY-3.1.md` records: the 14 plan decisions and how each landed, every deviation, all 13 gate
results with their measured values, the UI-02 criterion→evidence mapping as **run**, and the two
declarations below.

**Two things must appear in SUMMARY and in STATUS frontmatter, or 3.2 rediscovers them:**

1. **UI-02/5's end-to-end half is a 3.2 gate** (P45) — type a coordinate on the Venue screen, watch
   the Room readout move. 3.1 closes it on (a)+(b)+(c).
2. **N2 — do not route session state through `OuariconPresetManager::setStateFromXml`** at 3.2. It
   calls `parameters.replaceState(...)` and **nothing else**, which would bypass §4.1's
   `readVenueFromState()` → `rebuildChannelMap()` ordering and leave geometry, hull and map describing
   the **previous** venue. Silent, and it passes every existing probe. Adopt the module for **presets
   only**; keep O-Octagon's own `getStateInformation` / `setStateInformation` exactly as they are.

STATUS frontmatter updates: `stage_phase: "3.2"`, `probe_count: 65`, `stage_3_native_fn_surface_3_1: 3`
(verified, not predicted), `gesture_bracket_obligation: CLOSED_3_1_PUCK / OPEN_3_3_SCENES`,
`ui_gate_files: 2`, `playwright_required: true`.

---

## Execution Constraints

1. **`createEditor` stays guarded** — 29 harness probes die silently otherwise
   (`pattern_render_harness_breaks_on_webview_editor`).
2. **Member order relays → webView → attachments** — destruction order, not style.
3. **`WebSliderParameterAttachment` takes three arguments**, third `nullptr`.
4. **`check_native_interop.js` embedded and served**; `type="module"` everywhere.
5. **Resource provider receives bare PATHS** — never hard-code `juce://`.
6. **No hyphens in embedded filenames** — `juce_add_binary_data` **strips** them.
7. **The second binary-data target needs a distinct `NAMESPACE`** — `UIBinaryData`.
8. **Pass the `Juce` ES-module namespace**, not `window.__JUCE__`.
9. **`juce::String(const char*)` is ASCII-only** — build with `<<` or `+`. No compiler warning; the
   damage is visible only in rendered output. Small surface here (the negotiated-set name, any
   `detail` text); UTF-8 living in `index.html` or `app.js` travels as bytes and is unaffected.
10. **The 17 parameters are frozen.** Scenes, verify-ping and venue values are not parameters.
11. **MSVC hazards are authored now**, not fixed at port time (C3493; `SafePointer` init-capture).
12. **Windows CI is Stage 4** — but both WebView2 flags are asserted at 3.1.
13. **`tests/render-harness/CMakeLists.txt` is not modified** and `PluginEditor.cpp` never enters it.

---

## Non-goals for Phase 3.1 — must not appear

Anything here appearing in a 3.1 diff is a **deviation to be recorded**, not a bonus.

- Venue screen contents, coordinate editing, `.venue` save/load, `FileChooser` — **3.2**
- `VerifyPing`, `mapInvalid` surfacing, the negotiated-set **display** — **3.2**
  *(the string is fetched by `getStatus` at 3.1 for the SAFE banner; it is not laid out until 3.2)*
- Preset store, `OuariconPresetManager` adoption — **3.2**
- `meterPeak[8]`, meters, ballistics, peak-hold — **3.3**
- Named scenes, the `SIDES` predicate, the 4 user slots, the `SCENES` tree node — **3.3**
- Level-gradient backdrop, side-elevation strip — **3.3**
- The three §8 contract re-pins — **3.3 discuss**
- Aesthetic-template extraction — **3.3 verify** (Q10)
- Any `AsyncUpdater` anywhere — **never**, per §2.1
- Any new APVTS parameter — **never**, per constraint 10
- CI wiring for these gates — **Stage 4**

---

## Success Criteria

**Phase 3.1 is complete when all of the following hold, each measured rather than asserted:**

- [ ] All four contract checksums byte-exact; **no pin moved**
- [ ] Clean 3-format build + both test targets on a **forced full recompile**, zero compiler
      diagnostics
- [ ] `ui_layout_check.js` ran **against the stub before any C++ integration existed**, zero console
      errors, all 17 controls present *(UI-02/6)*
- [ ] `ui_frontend_check.js`: 20 sections, exit 0 — including **bridge surface exactly 3 in both
      directions** *(UI-02/7)*, **gesture pairing on both `srcX` and `srcY`** *(N1)*, **no bbox
      literals** *(UI-02/5c)*, **stub ranges == `createParameterLayout()`**
- [ ] `ui_layout_check.js` re-run post-integration: 10 sections, exit 0, **did not skip**
- [ ] Plan aspect follows the **returned** envelope under a stub mutation *(UI-02/1)*
- [ ] Hull polygon has exactly `hullCount` vertices; glyphs 3 and 8 render `ON_EDGE` *(UI-02/2)*,
      cross-checked by probe **BK** in C++
- [ ] **Off-centre** puck grab does not jump; edge-reversal responds immediately *(UI-02/3, N5)*
- [ ] `canvas.width == round(rect.width · dpr)` at **DPR 1 and 2** *(UI-02/4)*
- [ ] Metres readout changes under a stub bbox mutation with the puck stationary *(UI-02/5a)*;
      probe **BL** confirms the C++ half *(UI-02/5b)*
- [ ] `scrollWidth <= 1100` and `scrollHeight <= 720`, **measured on the rendered page**
- [ ] SAFE banner tracks `getStatus().safeMode`; probe **BM** confirms the C++ derivation
- [ ] **65 probes, 0 failures** (33 unit + 32 harness). **None of A–BJ regressed**
- [ ] `auval` SUCCEEDED; pluginval s10 ×3 VST3 + ×3 AU all exit 0
- [ ] Standalone launches at 1100 × 720, plan draws, puck drags, all 17 controls move *(Gate 13)*
- [ ] `UI-02` ticked ✅ **complete** in `REQUIREMENTS.md`, with each criterion's evidence named
- [ ] `SUMMARY-3.1.md` declares **UI-02/5's end-to-end 3.2 gate** and **N2's preset-manager
      restriction**

---

## Risks Active in This Phase

| # | Risk | Mitigation in this plan |
|---|---|---|
| **N1** | **Ungestured writes.** The puck moves the sound and Logic may not record it. Invisible to build / `auval` / `pluginval` | **P39** + `ui_frontend_check.js` §12. Generalised so 3.3's scenes inherit the rule |
| **R7** | Largest UI in the repo; **all of it silent when it breaks** | Every scar in the repo's memory is a numbered gate section, not a comment |
| **D4** | No browser-iteration phase — the first render would otherwise be inside a plugin | The stub is **mandatory** (Tasks 2, 5) and ordered before integration. Gate 13 adds the one thing the stub cannot prove: that a WKWebView loads at all |
| **P49** | **A skipped Playwright gate reads green** and four UI-02 criteria rest on it | The gate **fails** rather than skips. Task 0 installs Chromium up front |
| **§3.2** | **Row arithmetic that fits on paper and overflows on screen** (`pattern_flex1_container_slack_invisible_to_row_sum` — 93.5 px of phantom slack through five releases) | §8 measures `scrollWidth`/`scrollHeight` on the rendered page at exactly 1100 × 720 |
| **N4/N5** | Rubbery puck; sticky edge | **P40** (optimistic render) and **P41** (clamp the accumulator), each with a non-vacuous probe |
| **N6** | Venue cache goes stale on a non-UI venue change | `venueGen` on the existing 2 Hz poll — costs nothing extra |
| **P46** | Three layers could drift from each other | **One** `metresToPx()`, asserted by §19 |
| **D7** | The plan is **448 px wide** and may be unreadable at performance distance | Accepted, and routed: a legibility failure is a **3.3 discuss** finding, not a 3.1 plan change |
| **carried** | **D5 / QUAL-01's audible clause** — Stage 4. **CI gap** — Stage 4, and 3.1 *widens* it. **COMPAT-04**, **COMPAT-02** — Stage 4 | Unchanged. Restated so no Stage-3 verification is read as having settled them |

---

## Next Phase

Ready for: **execute** — `/plugin-execute O-Octagon 3-gui`

Task ordering is the part execute must not optimise away: **Tasks 1–5 complete, and Task 5 passes,
before Task 6 creates a single line of C++.** That ordering is UI-02 criterion 6, and it is the only
protection D4 left standing against a top-level TDZ throw first surfacing inside a plugin.
