# O-ParametricEQ Stage 3 (GUI), Phase 3.2: Research (interactive response display)

**Researched:** 2026-11-15
**Scope:** ROADMAP Phase 3.2 and Stage 3 CONTEXT U1, U3, U5 and open questions 5 and 6. It covers:
- the `eqSections` wiring between the editor-owned `DisplayModel` and the page;
- G9 (b), the page's JS evaluator against the C++ closed form (open question 5);
- the node interactions already in the page (create, drag, scroll Q, double-click, remove) and their host gestures (open question 6);
- typed entry at the range ends (UI-04, DSP-06);
- the 25th-band tip (U5);
- the rows carried from VERIFICATION-3.1 (Issue 1: double-click defaults; Issue 3: the Debug-assertion row with something emitted).

Out of scope, left to 3.3: analyzer, meter, fills, the listen state machine (U2), and the 60 fps row.

**Method:** two scratch benches, kept in `research-3.2/` (see its README). The scratch harness build and the dump live in the session scratchpad.

| Bench | What it is |
|---|---|
| `g9b-bench/make_dump_main.py` | A scratch copy of the render harness with two env switches: a G9 dump of all 260 XR-g9 draws (packets, header, the C++ unit-circle closed form) and a `DisplayModel` poll-cost timer |
| `g9b-bench/g9b.js`, `evalcost.js` | node 24 (V8). They lift `evalBand`, `magDb` and `composeSum` **verbatim** from `Source/ui/public/index.html` and score them against the dump |

- **[BENCH]:** measured on this machine (Apple M4 Max, macOS 26.6.2, Apple clang 21, JUCE 8.0.15, Release harness objects at `c77d014d`).
- **[ARGUED]:** from reading `Source/` at `c77d014d`, the page, or JUCE 8.0.15.

**Contracts read:** CONTEXT (Stage 3), ROADMAP § 3.2, REQUIREMENTS UI-01 / UI-02 / UI-04 / DSP-06, ARCHITECTURE "True-response display" and G9, BRIEF § Layout, VERIFICATION-3.1, SUMMARY-3.1, `DisplayModel.h/.cpp`, `ResponseEval.h`, `PluginEditor.cpp`, `index.html`, `i18n.js`, `scripts/check-i18n.js`.

## Summary

1. **Most of 3.2's UI already exists in the page.** The v1 page has the closed-form evaluator, the 2x2 composition, `sumView()` (U1), click-to-create (U3), drag with begin / end on both parameters, scroll-Q in wheel bursts, double-click gain reset on a node, Delete / Backspace and the strip's delete button, and typed entry. What 3.2 adds is the C++ side (`eqSections`), two page fixes (findings 3 and 4), the U5 tip, and the gates [ARGUED].
2. **G9 (b) passes by six orders of magnitude.** The page's own `evalBand` + `composeSum`, run under node on the processor's packets from all 260 XR-g9 draws (5 rates, stereo and mono): worst **1.4e-8 dB** against the C++ closed form over 148 956 scored points (gate 0.01 dB). No NaN below Nyquist [BENCH].
3. **Bug: on a mono layout the page draws the wrong curve.** `composeSum` and `sumView` read each band's placement from the relay. The engine and `DisplayModel` use the *effective* placement, which is Stereo on mono (`effectivePlacement`, `BandParams.h:264`). With Left and Right bands on a mono track, the page's curve is off by up to **59.5 dB** [BENCH]. Fix: compose from each packet's `placement` and `active` (P3).
4. **Gesture issue: a node drag opens a gain gesture even when the drag cannot write gain** (Low Cut / High Cut / Notch / Band Pass, or `gain_scale` ≤ 1 %). In Logic's Touch or Latch mode an open gesture writes the parameter's current value for its whole span, so dragging a cut's frequency overwrites that band's gain lane [ARGUED]. Fix: open the gain gesture only when the drag can write gain, decided at drag start (P5).
5. **The C++ side is cheap.** `poll()` + payload + `JSON::toString`: a one-band drag tick **≤ 53 µs**, a 24-band push **≤ 324 µs** (≤ 14 KB), an idle poll **0.2 µs** [BENCH]. A 60 Hz timer costs nothing measurable and halves the node-to-curve lag during a drag (P2).
6. **The page's curve maths cost 3.8 ms a frame** at worst (24 bands, 94 sections, 801 points; V8) [BENCH]. Fine at 30 Hz. It matters for 3.3's 60 fps row, because `draw()` re-evaluates every band on every frame. Measure in WKWebView in 3.2; cache per band in 3.3 only if the row asks (P10).
7. **Payload shape:** the page expects `{ sampleRate, bands: [ { band: 1 ... 24, … } ] }`. `DisplayModel` gives 0-based packets and a separate header. The editor adapts: `band + 1`, `sampleRate` from the header, `stereo` passed through (P1).
8. **U5 must ship all three languages now.** `check-i18n [1]` fails any key without `en`, `fr` and `zh-Hans` (`scripts/check-i18n.js:615`). CONTEXT U5's "French / zh-Hans drafted with Stage 4" is not possible. The new key gets machine drafts (`reviewed: false` / `'mt'`), like every other entry in the table today (P7).

No call for Taylor. Every item below is the plan's.

---

## Q1. `eqSections`: editor, payload, timing

### What exists [ARGUED]

- `DisplayModel` (Phase 2.6) is message-thread only. `poll()` returns the changed band indices; `packetFor (b)` gives `{ band 0 ... 23, type, placement (effective), active, sections, onePoles }`; `headerVar()` gives `{ sampleRate, stereo, rateKnown }`. It reads the parameter atomics, not anything `processBlock` writes, so the curve follows edits with the transport stopped (ARCHITECTURE; XR-dm polls with no render).
- `PluginEditor.cpp:390` registers `requestEqSections` as a no-op that completes. `timerCallback()` (`:638`) is gated on `pageIsReady` and sends nothing.
- The page (`index.html:1930`) expects `{ sampleRate, bands: [...] }` with `band` **1-based**, merges packets per band, and calls `requestEqSections` at init (`:1970`) and after a preset load (`:1995`).

### Wiring (P1)

- The editor owns `std::unique_ptr<DisplayModel>`, built in the constructor. Construction designs nothing; the first `poll()` reports all 24.
- `requestEqSections` sets `fullPushPending = true` and completes. The page calls it before `pageReady`, so the flag waits for the first tick. No `DisplayModel` API change is needed: on the flag, `poll()` and then emit all 24.
- `timerCallback()`: `changed = poll()`; if `fullPushPending`, every band. Emit nothing when the list is empty, so idle traffic stays 0 (3.1 acceptance row).
- Payload: `{ sampleRate, stereo, rateKnown, bands: [ packetFor (b) with band = b + 1 ] }`. Before the first prepare the model designs at 48 kHz with `rateKnown: false`; the page draws it as is (no label in v1).
- Host preset loads and automation need no extra path: `poll()` sees the changed bands.

### Hidden page [ARGUED]

`emitEventIfBrowserIsVisible` tests the WebView component's own `isVisible()` flag (`juce_WebBrowserComponent.cpp:609`), not whether the window is on screen. The editor never hides the WebView, so a `poll()` result is never consumed by a dropped emit. If a later change ever hides it, the timer must skip `poll()` while hidden, or set `fullPushPending` on show.

### Cost [BENCH]

`OPEQ_POLL_BENCH`, 400 random 24-band configurations, 20 one-band frequency edits each:

| Case | 48 kHz med / p99 / max | 192 kHz med / p99 / max |
|---|---|---|
| 24 bands changed: poll + payload + JSON | 209 / 301 / 324 µs; 9.5 / 13.0 / 14.0 KB | 203 / 288 / 296 µs |
| One band's frequency (a drag tick) | 6.3 / 29.8 / 40.4 µs | 5.9 / 29.2 / 52.8 µs |
| No change | 0.1 / 0.2 / 0.2 µs | 0.1 / 0.2 / 0.2 µs |

### Timer rate (P2)

- The node is drawn from the relay value on the next frame. The curve waits for the next poll. At 30 Hz the curve trails the node by up to 33 ms plus a frame during a drag; at 60 Hz by up to 17 ms.
- At the costs above, a 60 Hz tick is free. Recommended: `startTimerHz (60)` in `pageReady`. 3.3 can emit the analyzer every second tick; its ballistics already use the measured dt.
- The 3.2 editor bench should record node-to-curve lag (relay write → `eqSections` → painted) during a scripted drag, at the rate chosen.

### Debug assertion (VERIFICATION-3.1 Issue 3)

With `eqSections` live, re-run E-cycle in Debug (10 open / close cycles, 0 assertions). That makes R3's row non-vacuous.

---

## Q2. G9 (b): the JS evaluator against C++ (open question 5)

### Method [BENCH]

- The scratch harness's `OPEQ_G9_DUMP` runs the XR-g9 jobs unchanged (the processor's own `DisplayModel`, after the first block) and writes, per draw: the 24 packets via `JSON::toString`, the header, and a grid of the 192 G9 log points (10 Hz – min (30 kHz, 0.49 fs)) plus every active band's frequency below Nyquist. For each grid point it also writes `composeFromPackets (bands, e^{jω})`: LL, RR, MM, SS. That is the harness's own closed form, the one XR-g9 scores against the measured response.
- `g9b.js` lifts `evalBand`, `magDb` and `composeSum` from `index.html` by name with brace matching, so it runs the shipping text, not a copy. It feeds them the packets with 1-based band numbers, composes the `active` bands in index order, and scores each view.
- Floors are XR-g9's: points within 80 dB of the largest maximum among the four views, and at or above −120 dB.

### Results [BENCH]

| | Value |
|---|---|
| Draws | 260 (50 per rate × 5 rates stereo, 2 per rate mono) |
| Points scored | 148 956 |
| **Worst, scored** | **1.37e-8 dB** (192 kHz; 44.1 k 5.2e-9, 48 k 8.3e-10, 88.2 k 7.6e-10, 96 k 3.5e-10) |
| Worst at or above −120 dB, no 80 dB floor | 9.1e-7 dB |
| NaN below Nyquist | 0 |
| JS `magDb` floor hits (−200 dB, `|H|² < 1e-20`) | 23 296, all below −120 dB, all outside the ±30 dB display |

The page and C++ use the same algebra (`s = j tan (ω/2) / g`), and the JSON round trip costs ≤ 1.5e-8 dB (`DisplayModel.h`, XR-g9j). G9 (a) + (b) together stay inside UI-02's 0.1 dB with the whole (b) budget unused.

### The gate (P4)

- **Harness:** add a permanent `--dump-g9 <dir>` mode to the render harness. It is the scratch patch with the env switch replaced by the flag. It runs only `displayRowsXr`'s G9 jobs, in about 6 s.
- **Runner:** `tests/g9b/g9b.mjs`. It lifts the three functions from `Source/ui/public/index.html` (the shipping page, never a copy) and scores them at 0.01 dB with XR-g9's floors.
- **Guards against a vacuous pass:** each lifted function must be found exactly once; the scored count must be ≥ 100 000; every one of the 5 rates and mono must contribute points.
- **Negative controls,** each of which must exceed 0.05 dB (the G9 (a) set, applied in JS):
  - `k × 1.01` in every SVF section;
  - `onePoles` dropped;
  - the first L and M bands swapped in order;
  - Side composed with the Mid matrix (a one-token mutation of the lifted `composeSum`);
  - **new:** on the mono draws, a relay placement that differs from the packet's (finding 3). After P3 this must change nothing.
- **Order of steps:** dump → node run. Both go in the verify recipe and as one CTest / script entry. G9 (b) needs node, which the UI gates already require.

---

## Q3. Placement on mono: the page composes the wrong matrix (finding 3)

- **What the page does:** `composeSum` switches on `bandChoice (b, 'placement')` (`index.html:1504`), the relay value. `sumView()` picks the view from the selected band's relay placement (`:1535`). The page also chooses the bands to compose from the relays: enabled and not bypassed.
- **What the engine does:** on a mono layout the processor runs every band as Stereo (`effectivePlacement`). `DisplayModel` sends that effective placement, and XR-dm2 proves the packets carry Stereo on mono.
- **Measured** [BENCH, `g9b.js raw-mono`]: mono draws composed with relay placements (odd bands Left, even Right) are off by up to **59.5 dB** (`m000`, 1.72 kHz: page +18.4 dB, true −41.1 dB). Every band that is not Left drops out of the LL view.
- **P3:**
  - `composeSum` takes the placement from `bandPackets[b].placement`.
  - The bands it composes are those whose packet is `active`. `active` already excludes disabled, bypassed and sleeping bands, which is exactly what the engine runs.
  - `sumView()` takes the selected band's packet placement.
  - The page keeps its relay reads for everything it draws per band (nodes, washes, bypassed ghosts).
  - The node badge keeps the relay placement, so the user sees what they set. On mono the curve then shows the truth while the badge shows the setting. That matches the engine, which stores the parameter and ignores it on mono.
- Global bypass stays a page rule (the sum is drawn flat and dashed).
- The curve can lag the relays by one tick on a placement or enable edit. That is the same lag as any other edit.

---

## Q4. Node interactions and host gestures (UI-01, open question 6)

### What the page does today [ARGUED, `index.html:1801-1903`]

| Action | Writes | Gesture |
|---|---|---|
| Click empty plot (U3) | `type` Bell, `placement` Stereo, `bypass` off (each a complete gesture: combo / toggle attachments), `q` default, `freq`, `gain` (each `writeScaled` = begin / set / end), then `enabled` on, last | one per parameter |
| Drag a node (≥ 3 px) | `freq` and `gain` on every move | `sliderDragStarted` on **both** at threshold, `sliderDragEnded` on both on up / cancel / lost capture (idempotent) |
| Wheel on a node | `q`, log domain, Shift fine | one gesture per 250 ms burst |
| Double-click a node | `gain` → default (not within 450 ms of a create) | one |
| Delete / Backspace, strip delete | `enabled` off | complete gesture |
| 25th band click | nothing | — (U5 tip missing) |

Writing `enabled` last means the band appears at its new values.

### Host gestures [ARGUED]

- The relay's `sliderDragStarted` / `sliderDragEnded` become `beginChangeGesture` / `endChangeGesture` on that one parameter.
- JUCE forwards them per parameter: AU `kAudioUnitEvent_Begin/EndParameterChangeGesture` (`juce_audio_plugin_client_AU_1.mm:1352-1360`); VST3 `beginEdit` / `endEdit` (`juce_audio_plugin_client_VST3.cpp:1461-1467`). There is no shared gesture state, so two parameters open at once is a legal sequence in both formats.
- **Suite precedent:** O-Strata's terrain centre drag opens CX and CY together (`O-Strata/.../index.html:5538`). Its in-process "one pair each" rows pass. Its Logic undo row is still open (O-Strata Stage 3 VERIFICATION, human row 3), so no Logic evidence exists in the suite yet.
- **Recording in Logic is a hands-on row.** No automated path drives a hosted plugin's WebView with real pointer events.

### Gain gesture without a gain write (finding 4; P5)

- `onMove` opens the gain gesture for every drag. It writes gain only when `TYPE_HAS_GAIN[type] && scale > 0.01`. For the other types, and at `gain_scale` ≤ 1 %, the gesture is open with no writes.
- In Touch and Latch, Logic and most VST3 hosts write the parameter's current value while a gesture is open. A frequency drag on a Low Cut would therefore flatten that band's gain lane over the drag. That costs an automation edit, not audio. With `gain_scale` at 0 % on a Bell it costs real gain automation.
- **P5:** at the drag threshold, set `d.gain = TYPE_HAS_GAIN[type] && scale > 0.01`. Begin and end the gain gesture only when `d.gain` is true, and write gain only then. The value is decided once, so a type change by automation mid-drag cannot unbalance the pair.

### Bench rows for the plan (P6)

All rows run in the 3.1 editor bench (real editor, real page). Events are JS-dispatched `PointerEvent` / `WheelEvent` / `MouseEvent ('dblclick')`. `setPointerCapture` throws on a synthetic pointer id, and the page already catches that. Each row counts `parameterGestureChanged` per parameter:
- **Create:** a click at (x, y) creates the first free band: freq = `xf (x)`, gain = `round (yg (y) / scale, 0.1)`, Bell / Stereo / not bypassed / default Q; slope unchanged. With bands 1–23 enabled, band 24; with 24, nothing changes and the tip shows (U5).
- **Drag:** a Bell drag gives exactly one begin / end pair on `freq` and on `gain`, and values that follow the pointer. A Low Cut drag gives one pair on `freq` and **zero** on `gain` (P5). Cancel and lost capture end the pairs too.
- **Wheel:** a burst of 10 events gives one pair on `q`, and the value moves in the log domain.
- **Double-click (VERIFICATION-3.1 Issue 1, mutant E15):**
  - on a node: gain lands on its default in dB, within the float step;
  - on each strip field (freq, gain, Q) and each knob (`output_gain`, `gain_scale`): the host value equals `getDefaultValue()` converted;
  - a node created within 450 ms is not reset.
  - Re-run E15 against this row; it must now be caught.
- **Delete:** Delete key, and the strip button, set `enabled` off and select the nearest band.
- **Typed entry at the ends (UI-04, DSP-06):** "10", "30k", "30000", "-30", "30", "0.025", "40" and out-of-range inputs ("5", "99k", "-99", "0", "1e9") land exactly on start / end.
  - The page maps the clamped end through `scaledToNorm` to exactly 0 or 1; `NormalisableRange::convertFrom0to1` then gives start / end and `snapToLegalValue` clamps.
  - Check the host raw value equals the range end, not just "within tolerance".
- **Transport stopped:** with the processor prepared and no `processBlock` calls, a host-side `setValueNotifyingHost` on a band's freq produces an `eqSections` event for that band within 2 ticks, and the painted curve changes.
- **Gate:** the bench reports the gesture counts; mutating `onUp` to skip one `sliderDragEnded` must fail the drag row.

### Hands-on for Taylor (Logic AU, one VST3 host)

- In Touch mode, drag a Bell node: `freq` and `gain` lanes record together, and one ⌘Z undoes the drag.
- Drag a Low Cut node: only `freq` records.
- A wheel burst records one Q stretch.
- Click-to-create in Latch: the new band's values are written.

---

## Q5. Typed entry and the strip (UI-04)

- `parseEntry` accepts "2500", "2.5k", "2,5 k", a typographic minus; it clamps by the live relay start / end (`index.html:994`). Range ends are exact (Q4, P6).
- Not accepted: exponent forms ("1e3"). Not needed for v1.
- The strip's double-click reset and its 240 ms entry delay are in place (3.1). The double-click rows above gate them.
- Host text still has no units ("999.9998779"); that stays a Stage 4 item (VERIFICATION-3.1 Issue 4).

---

## Q6. The 25th-band tip (U5; P7)

- `createBand` returns 0 with 24 bands enabled. The plot handler then returns silently (`index.html:1860`). Nothing shows.
- The tooltip renderer anchors to an element's rect (`showTooltip`, `:2107`). A point tip can reuse its measure-pin-clamp body with a zero-size rect at the pointer.
  - **Refactor:** `placeTip (rect, title, body)`, used by both `showTooltip` and the new `showPointTip (x, y, key)`.
  - The edge clamp and shrink-to-fit rules come with it unchanged (suite memory: shrink-to-fit at the edge; viewport-sensitive clamp).
- **Behaviour:**
  - shows on the refused click, at the pointer, for about 1.6 s;
  - hides on the next `pointerdown`;
  - shows even when hover help is off (it is feedback, not help: the `data-tip-always` rule);
  - body only, no title.
- **i18n:** a new key `bands-full` with `en` "All 24 bands are in use", plus `fr` (`reviewed: false`) and `zh-Hans` (`'mt'`) machine drafts, because `check-i18n [1]` requires all three (Summary 8). It is not in `TIP_BINDINGS`: no DOM anchor carries it. Confirm `check-i18n` does not flag an unbound key. If it does, bind it to a hidden anchor, as O-Strata's `#terrain-notice` keys do.
- **Gates:** add a `tests/i18n-states.json` state, appended last (memory), that enables 24 bands and clicks the plot, so `check-ui-labels` measures the tip in all three languages at 800 × 500. `boot-all-uis --strict-tips` must stay 0.

---

## Q7. The curve maths per frame (P10)

`evalcost.js` on the draw with the most sections (24 bands, 94 sections), 801 points (a 1600 px plot): **3.83 ms median, 5.10 ms p99** per `draw()`'s maths in V8 [BENCH]. JavaScriptCore in WKWebView is untested here.

- At 30 Hz, and on relay-driven redraws, that is fine.
- During a drag the page redraws per animation frame, and `listenPulse` (3.3) redraws every frame. At 60 fps that is about a quarter of the frame before canvas work.
- **3.2:** the editor bench records `draw()` time in WKWebView on this draw at 1600 × 1000 (`performance.now()` around `draw`).
- **3.3:** cache each band's response, and re-evaluate only bands whose packet changed, only if the 60 fps row needs it. The grid includes every band's frequency, which moves during a drag, so a cache must hold the fixed pixel grid apart from the band points.

---

## Plan inputs (no call needed)

| ID | Item |
|---|---|
| P1 | Editor owns `DisplayModel`; `requestEqSections` sets `fullPushPending`; the timer polls and emits only changed bands (all after the flag); payload `{ sampleRate, stereo, rateKnown, bands: [band + 1 …] }`; nothing on an empty list |
| P2 | Timer at 60 Hz (cost ≤ 53 µs per drag tick); bench records node-to-curve lag. Debug E-cycle re-run with `eqSections` live (VERIFICATION-3.1 Issue 3) |
| P3 | Page: `composeSum` uses each packet's `placement` and composes `active` packets; `sumView()` uses the selected band's packet placement; badges and per-band drawing keep the relays |
| P4 | G9 (b) gate: render harness `--dump-g9 <dir>` + `tests/g9b/g9b.mjs` lifting the shipping functions; 0.01 dB; vacuity guards; five negative controls incl. mono relay placement |
| P5 | Page: gain gesture opened only when the drag can write gain, decided at drag start |
| P6 | Editor-bench rows: create, drag (gesture pairs, Low Cut zero gain pairs), wheel burst, double-click defaults on node / fields / knobs (kills E15), delete, typed range ends exact, transport-stopped curve |
| P7 | U5 tip: `placeTip` refactor, `showPointTip`, key `bands-full` in en / fr / zh-Hans (drafts flagged), i18n state appended last, five UI gates 0 |
| P8 | No processor, DSP or `DisplayModel` change. XR-lid stays SHA-equal; designer 150 / 0 / 18 and render 166 / 0 stay, plus whatever P4 adds |
| P9 | Version 0.8.0; CHANGELOG; ROADMAP 3.2 components unchanged (the gates fit the existing criteria) |
| P10 | `draw()` time in WKWebView recorded in 3.2; per-band cache deferred to 3.3 |

**CONTEXT note for the plan:** U5's "French / zh-Hans drafted with the Stage 4 localisation" is replaced by machine drafts in 3.2, which `check-i18n [1]` forces. Stage 4 reviews them with the rest.

## Calls

None. Findings 3 and 4 are correctness fixes inside the page, not design choices. The timer rate (P2) is a plan detail at no measurable cost.
