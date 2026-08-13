# Stage 3 — GUI · Phase 3.1 (Two-screen shell, Room plan, musical parameters) — Research

**Plugin:** O-Octagon
**Stage:** 3 of 4 — GUI
**Phase:** 3.1 of 3
**GSD phase:** research
**Date:** 2026-08-12
**Branch:** `feat/o-octagon` @ `a47cef88` (Stage 2 phases 2.2 / 2.3 work uncommitted)
**Inputs:** `stages/3-gui/CONTEXT-3.1.md` (D1–D7, ten open questions), `ROADMAP.md` Phase 3.1,
`REQUIREMENTS.md` UI-02 (7 criteria), `research/ARCHITECTURE.md` §4.1 / §4.3 / §6.1 / §6.3 / §R7,
JUCE 8.0.14 in-tree source, `plugins/O-ReverseDelay/` (the WebView reference implementation).

---

## Entry Check — the four contracts

Standing obligation at every boundary (`pattern_promotion_checksum_pins_replaced_file`). Re-run
against the live files before anything else:

| Contract | SHA-256 measured now | STATUS frontmatter | Result |
|---|---|---|---|
| `BRIEF.md` | `697a4f32…f6b9fbd6` | `697a4f32…f6b9fbd6` | ✅ |
| `parameter-spec.md` | `b45f88dc…cbb9e02f` | `b45f88dc…cbb9e02f` | ✅ |
| `research/ARCHITECTURE.md` | `a8a358f4…9b6d4408` | `a8a358f4…9b6d4408` | ✅ |
| `ROADMAP.md` | `aec7d0ce…0137ee29` | `aec7d0ce…0137ee29` | ✅ |

**No drift. No contract is amended at this boundary.** Two re-pins are *identified* below (§8) and
both are deliberately scheduled for **3.3 discuss**, not taken here.

---

## Executive summary — what this research changes

Seven findings materially change what Phase 3.1 should build. Three of them are things the discuss
document could not have known because they live in JUCE's own source.

1. **The scene-write mechanism named in `ARCHITECTURE §6.3` — `setValueNotifyingHost` — is, on its
   own, an ungestured write.** `WebSliderParameterAttachment` routes every JS-side write through
   `ParameterAttachment::setValueAsPartOfGesture`, which calls `setValueNotifyingHost` with **no**
   `beginChangeGesture`/`endChangeGesture` around it. That is F2 below, it answers **Q2**, and it
   changes the puck drag in **3.1** — not just scenes in 3.3.
2. **`ParameterAttachment` silently skips a write whose value is unchanged.** A "scene wrote 8
   events" probe would fail on correct code. FUNC-06/1 already says *read back the 8 values* rather
   than count events — that wording is now load-bearing rather than stylistic. (F3)
3. **A per-`pointermove` `getNativeFunction` call for the metres readout is the wrong shape** — it
   is an async round trip whose promises can resolve out of order, so the readout can display a
   *stale* metre value while the puck is current. The fix is a pulled-and-cached venue envelope with
   a change signal, which still satisfies UI-02/5. (**Q4**, §2.4)
4. **The Room plan is portrait and height-bound: ~448 × 560 px inside a 1100 × 720 window.** The
   derived envelope's aspect is exactly 0.800 for the §OQ4 default venue. D7 survives, but the plan
   is narrower than "1100 px wide" suggests, and that sharpens D7's stated legibility cost. (**Q7**,
   §3)
5. **D5's `SIDES` rule is not derivable as written.** "Hull speakers off both axes" has no unique
   reading; a concrete, permutation-invariant predicate is proposed and measured against the default
   venue in §2.3. It is a 3.3 item, recorded now so 3.3 does not re-derive it.
6. **No new atomic is needed for the SAFE-mode banner**, and the venue-generation counter the
   solver's dirty check already uses is exactly the change signal the UI wants. Both answer with
   *existing* machinery. (**Q9**, **Q1**, §2.9 / §2.1)
7. **All 17 parameters are `AudioParameterFloat`.** One relay list, one attachment list. The
   relay-type split that bit O-ReverseDelay three separate times (`grainShape`, `freeze`,
   `sourceMode`) **cannot occur in this plugin**. Stated so plan does not budget for it.

Nothing found here blocks Phase 3.1. One UI-02 criterion (5) needs a specific three-part probe
design because the Venue screen it implies does not exist until 3.2 — §6.

---

## 1. The C++ integration surface — verified against JUCE 8.0.14 in-tree

### F1 — 17 slider relays, no type split

`parameter-spec.md` freezes 17 parameters, **all `juce::AudioParameterFloat`**, no Choice and no
Bool. Therefore:

```
kSliderIds  = srcX srcY srcZ width rolloff blur w1..w8 hullAtten airAmount outputGain   (17)
kComboIds   = ∅
kToggleIds  = ∅
```

O-ReverseDelay maintains three lists and its `PluginEditor.h` documents three separate incidents of
a parameter landing in the wrong one — a relay whose type does not match its parameter *attaches
without error* and produces a control that never updates. **That failure class is structurally
absent here**, and it stays absent only while the parameter set stays all-float (constraint 10).

The relay/attachment/webview member order and the 3-argument attachment are unchanged repo
convention (`juce8-critical-patterns` §11, §12) and are already written into CONTEXT constraints
2–3. The reference implementation to copy is `plugins/O-ReverseDelay/Source/PluginEditor.{h,cpp}`.

Relay construction reaches the parameters through `processorRef.getAPVTS().getParameter(id)`, which
returns `RangedAudioParameter*` — exactly what `WebSliderParameterAttachment`'s first argument
requires (`juce_ParameterAttachments.h:282`). **Q6 is answered: `getAPVTS()` is sufficient and no
new processor surface is needed for the relays.**

### F2 — the gesture finding (the most consequential thing in this document)

JUCE 8.0.14, `juce_ParameterAttachments.cpp:324`:

```cpp
void WebSliderParameterAttachment::sliderValueChanged (WebSliderRelay* slider)
{
    ...
    attachment.setValueAsPartOfGesture (slider->getValue());   // ← NO begin/endChangeGesture
}

void WebSliderParameterAttachment::sliderDragStarted (WebSliderRelay*) { attachment.beginGesture(); }
void WebSliderParameterAttachment::sliderDragEnded   (WebSliderRelay*) { attachment.endGesture(); }
```

and `juce_ParameterAttachments.cpp:76`:

```cpp
void ParameterAttachment::setValueAsPartOfGesture (float newDenormalisedValue)
{
    callIfParameterValueChanged (newDenormalisedValue, [this] (float f)
    {
        parameter.setValueNotifyingHost (f);   // gesture brackets come from the caller, not here
    });
}
```

**Consequence.** A JS `sliderState.setNormalisedValue(v)` that is *not* bracketed by
`sliderState.sliderDragStarted()` / `sliderDragEnded()` writes the parameter with **no gesture at
all**. The gesture brackets are what a host uses to open and close an automation-write region;
Logic's Touch and Latch modes key off them. An ungestured write moves the sound and may not be
recorded.

**Two places this bites, one of them in 3.1:**

- **3.1 — the puck.** `srcX` and `srcY` are two independent parameters driven by one gesture. The
  drag handler must call `sliderDragStarted()` on **both** at `pointerdown` and `sliderDragEnded()`
  on **both** at `pointerup`, with every intermediate `setNormalisedValue` inside that bracket. A
  puck that only writes values produces a position automation lane that a host in Touch mode may
  refuse to record — in a plugin whose headline gesture is automating position.
- **3.3 — scenes.** `ARCHITECTURE §6.3` says scenes write *"all 8 weight parameters at once via
  `setValueNotifyingHost`, so scenes record as ordinary automation."* Taken literally through the JS
  relay path, that produces eight ungestured writes. The correct idiom is
  `beginChangeGesture(); setValueNotifyingHost(v); endChangeGesture();` per parameter — which is
  exactly what `ParameterAttachment::setValueAsCompleteGesture` does
  (`juce_ParameterAttachments.cpp:59`). §6.3 is not wrong, it is *incomplete*; see §8.

### F3 — an unchanged write is silently skipped

`juce_ParameterAttachments.cpp:91`:

```cpp
template <typename Callback>
void ParameterAttachment::callIfParameterValueChanged (float newDenormalisedValue, Callback&& cb)
{
    const auto newValue = normalise (newDenormalisedValue);
    if (! approximatelyEqual (parameter.getValue(), newValue))   // ← skip if unchanged
        cb (newValue);
}
```

and the relay itself skips too (`juce_WebControlRelays.cpp:106`):

```cpp
if (! approximatelyEqual (std::exchange (value, valueChanged->newValue), valueChanged->newValue))
    listeners.call (...);
```

**Consequence.** From the default state (`w1..w8 = 1.0`), clicking `ALL` produces **zero** host
notifications, because every target value already equals the current value. Any probe of the form
*"assert 8 automation events were emitted"* fails on correct code.

FUNC-06/1 already reads *"Verified by reading back all 8 host-side parameter values."* **Keep that
wording exactly.** It is the only formulation that is both non-vacuous and correct.

### F4 — the JS→C++→JS echo, and why the puck must render optimistically

The round trip for one JS write is:

```
JS  setNormalisedValue(v)
 →  relay.handleEvent   (skips if unchanged — F3)
 →  attachment.setValueAsPartOfGesture → setValueNotifyingHost
 →  parameter change → WebSliderParameterAttachment::setValue (ignoreCallbacks = true)
 →  WebSliderRelay::setValue → emitEventIfBrowserIsVisible
 →  JS  handleEvent → scaledValue = snapped value → valueChangedEvent.callListeners()
```

Two things follow:

1. **The authoritative value arrives back asynchronously and is the *snapped* value.** A puck
   rendered directly from `getScaledValue()` on `valueChangedEvent` lags the pointer by a full
   round trip and will look rubbery at 60 fps. **Render the puck from a local optimistic position
   while a drag is active; render from `getScaledValue()` when it is not, and re-sync on
   `sliderDragEnded`.** (`w1..w8`, being ordinary controls, do not need this.)
2. **A `valueChangedEvent` listener must not write back.** `WebSliderParameterAttachment::setValue`
   sets `ignoreCallbacks` and `sliderValueChanged` contains a `jassertfalse` on that path
   (`juce_ParameterAttachments.cpp:326`). The echo terminates naturally because both the relay and
   the attachment compare-before-notify (F3), but a listener that *re-writes a rounded value* can
   ping-pong. Render on echo; never write on echo.

### F5 — build and resource deltas for 3.1

`CMakeLists.txt` already carries `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`,
`JUCE_WEB_BROWSER=1` and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` — all set at Stage 1
specifically so `createEditor` could be guarded *before* the Stage-3 swap. Verified: the guard is at
`Source/PluginProcessor.cpp:434`, and `tests/render-harness/CMakeLists.txt` builds with
`JUCE_WEB_BROWSER=0` and does not list an editor TU. **Constraint 1 is already satisfied and the
harness cannot break at 3.1 provided `PluginEditor.cpp` is never added to that target.**

What 3.1 adds:

```cmake
juce_add_binary_data(OuariconOctagon_UIResources
    NAMESPACE   UIBinaryData          # distinct namespace — constraint 7
    HEADER_NAME UIBinaryData.h
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/css/styles.css
        Source/ui/public/js/app.js
        Source/ui/public/js/roomplan.js         # NOT room-plan.js — constraint 6
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)
target_link_libraries(OuariconOctagon PRIVATE OuariconOctagon_UIResources)
```

`Source/ui/public/js/juce/index.js` is copied verbatim from
`/Users/taylorbrook/JUCE/modules/juce_gui_extra/native/javascript/index.js`; `check_native_interop.js`
sits beside it and **must** be served, or the page can hang (`juce8-critical-patterns` §13).

**Hyphen hazard, concretely (constraint 6).** `juce_add_binary_data` strips hyphens rather than
converting them: a file named `room-plan.js` yields the symbol `roomplan_js`. The safe habit — and
what the SOURCES list above does — is to author the filenames without hyphens in the first place.

**Resource provider** takes bare paths and matches by direct equality; never construct or strip a
scheme (`critical_webview_resource_provider_and_schemes`). Copy
`ReverseDelayEditor::getResource` (`PluginEditor.cpp:145-190`) shape verbatim, including
`charset=utf-8` on every text resource — which also disarms constraint 9 for anything served rather
than built in C++.

**Constraint 9 (`juce::String(const char*)` is ASCII-only) applies to C++-built strings only.** In
this editor that is a small surface — the negotiated-set name and any `detail` text. Build them with
`<<` or `+`, never `juce::String("… — …")`. UTF-8 that lives in `index.html` or `app.js` travels as
bytes and is unaffected.

---

## 2. The ten open questions

### 2.1 — Q1: `SCENES` and `setStateInformation` ordering

**Answer: `SCENES` needs no ordering treatment, no `AsyncUpdater`, and no `cancelPendingUpdate()`.**

`setStateInformation` (`Source/PluginProcessor.cpp:467-497`) is:

```
replaceState()  →  readVenueFromState()  →  venue.writeToState()  →  if (preparedYet) rebuildChannelMap()
```

`SCENES` is pure weight data. Nothing geometric is derived from it, no map is built from it, and the
audio thread never reads it. It rides `replaceState()` in exactly the way `VENUE` does and is read
lazily by the editor when it needs to populate the four user slots. The correct insertion point is
**after `replaceState()`, anywhere before the function returns** — and the simplest correct choice is
to not insert anything at all in the processor and let the editor read
`apvts.state.getChildWithName("SCENES")` on demand.

`pattern_asyncupdater_guard_flag_needs_cancel` does not attach here for the same reason it did not
attach at 2.2: **there is no deferred apply.** The processor deliberately uses a plain `preparedYet`
flag rather than an `AsyncUpdater` (`PluginProcessor.h:174-178`), so there is no queued work that
could stomp restored state and therefore nothing to cancel. Adding an `AsyncUpdater` for `SCENES`
would *create* the obligation the design has so far avoided. **Do not.**

One thing to mirror from the venue path: `setStateInformation` normalises a missing/partial `VENUE`
node by writing it back complete. Do the same for `SCENES` — a session written before 3.3 has no
`SCENES` child, and normalising once makes the next `getStateInformation` self-describing.

**A hazard found while answering this, which belongs to 3.2 but is decided by 3.1's editor
structure.** `modules/persistence/preset-manager` is the repo's preset module and ROADMAP 3.2 calls
for a musical preset store. Two facts about it:

- ✅ `applyPresetJson` (`OuariconPresetManager.h:299`) touches **parameters only**, plus an optional
  `customLoad` callback. It never calls `replaceState`. **FUNC-05's structural guarantee survives
  adoption of this module**, and the `customSave`/`customLoad` hook is exactly the mechanism by
  which a musical preset can carry `SCENES` (D6) without being able to reach `VENUE`.
- ⚠️ `setStateFromXml` (`OuariconPresetManager.h:580`) calls `parameters.replaceState(...)` and
  **nothing else**. If O-Octagon's `setStateInformation` were to delegate to it wholesale, the §4.1
  ordering would be lost: the venue would be restored into the tree but `readVenueFromState()` and
  `rebuildChannelMap()` would never run, so the geometry, the hull and the channel map would all
  still describe the *previous* venue. Silent, and it would pass every existing probe.

**Recommendation for 3.2, recorded now:** adopt the module for preset save/load and for the
`customSave`/`customLoad` `SCENES` hook, and **keep O-Octagon's own `getStateInformation` /
`setStateInformation` exactly as they are.** Do not route session state through the module.

### 2.2 — Q2: does an 8-parameter scene write coalesce in Logic?

**Answer: coalescing is not the risk. Two other things are, and both are inside JUCE rather than
inside Logic.**

Automation in Logic is per-parameter — eight parameters produce eight lanes, each receiving one
value. There is no cross-parameter coalescing mechanism for a host to apply. What actually decides
whether the write records is:

1. **Gesture brackets (F2).** Without `beginChangeGesture`/`endChangeGesture`, a write in Touch or
   Latch mode may be applied but not captured. **The scene write must bracket each parameter.**
2. **The unchanged-value skip (F3).** Parameters already at the target value emit nothing at all.

**Recommended implementation, and it is a C++ one.** Register a native function `applyScene` that
derives the membership from live geometry and writes all eight parameters in C++:

```cpp
for (int i = 0; i < 8; ++i)
    if (auto* p = apvts.getParameter ("w" + juce::String (i + 1)))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (target[i]));
        p->endChangeGesture();
    }
```

Deriving in C++ rather than in JS is the same argument O-ReverseDelay's `getWindowCurve` makes and
documents (`PluginEditor.cpp:283-300`): a JS reimplementation of a geometric rule is a **second
definition free to drift from the first**, and the drift would be invisible — the plan would keep
highlighting a plausible speaker set while no longer describing what the button writes. The relays
update the UI automatically because the attachments listen to the parameters.

FUNC-06/6 ("two scenes can be faded between") is satisfied by construction: writing parameters, not
applying a snapshot, is what makes a scene fadeable, and the 5 ms per-sample smoothers (§OQ3) do the
fade in audio.

**Residual, honestly stated:** whether Logic's Touch mode records a correctly-bracketed 8-parameter
burst as eight clean lane entries is a *host* behaviour that cannot be measured at a desk. It is a
one-line addition to the Stage 4 hall/Logic session, where the plugin is in Logic anyway (D2).

### 2.3 — Q3: geometry-derived scene sets, and the degenerate cases

**D5's `FRONT`/`REAR`/`LEFT`/`RIGHT` are unambiguous. `SIDES` is not, and needs a written
predicate.** "Hull speakers off both axes" admits several readings that disagree on real rigs.

**Proposed predicate — normalised lateral dominance:**

```
hx = (bbMaxX − bbMinX) / 2      hy = (bbMaxY − bbMinY) / 2
SIDES  =  { i : classify(i) != INTERIOR  ∧  |x_i − cx| / hx  >  |y_i − cy| / hy }
```

i.e. a speaker whose displacement from the centroid is **predominantly lateral**, measured in units
of each axis's own half-span so the rule is invariant to room proportions.

**Measured against the §OQ4 default venue** (centroid `(6.5000, 12.4625)`, `hx = 6.0`, `hy = 7.5` —
recomputed here from the coordinate table, and the same computation reproduces `rigScale = 7.93165`,
so the model agrees with `VenueModel`):

| Scene | Set | Note |
|---|---|---|
| `ALL` | 1–8 at 1.0 | — |
| `FRONT` (`y < cy`) | **1, 2, 3, 8** | |
| `REAR` (`y > cy`) | **4, 5, 6, 7** | |
| `LEFT` (`x < cx`) | **1, 6, 7, 8** | |
| `RIGHT` (`x > cx`) | **2, 3, 4, 5** | |
| `SIDES` (predicate above) | **3, 4, 7, 8** | the four mid-wall speakers |

**A near-tie worth knowing about.** Speakers 1 and 2 score `dx/hx = 1.0000` against
`dy/hy = 1.0617` — they miss `SIDES` by 6 %. A different hall, or a re-measurement that moves the
rear pair inward, flips the front pair into `SIDES`. That is not a defect in the rule; it is
precisely the situation **FUNC-06/3** exists for — *the plan shows which speakers a scene will
select before it is committed*. This measurement is the concrete evidence that the criterion earns
its cost.

**Ties and degeneracies.** Use strict comparisons throughout, and guard both half-spans with
`oo::plane::kMinSpan` — the same constant `VenueModel::normToMetres()` and `earHeight()` already
guard with, aliased rather than re-declared (`VenueModel.h:60`).

| Venue state | `FRONT`/`REAR` | `LEFT`/`RIGHT` | `SIDES` |
|---|---|---|---|
| Normal | as above | as above | as above |
| All 8 coincident | ∅ / ∅ | ∅ / ∅ | ∅ |
| All 8 collinear on one **side wall** (x span < `kMinSpan`) | works | ∅ / ∅ | ∅ (x ratio pinned to 0) |
| All 8 collinear across the **front** (y span < `kMinSpan`) | ∅ / ∅ | works | every off-centre speaker |
| Speaker exactly at `cy` | in neither `FRONT` nor `REAR` | — | — |

**"Empty" is the right answer, not a fallback.** FUNC-06/3 already requires an empty set to render
as empty and to be **non-writable**. Falling back to `ALL` would mean a `FRONT` button silently
becoming an `ALL` button on a degenerate rig — a scene mis-firing mid-concert is unrecoverable, and
a disabled button is legible while a lying button is not. `ALL` is never empty and remains the
escape hatch on any venue.

*(All of this is 3.3 work. It is written here so 3.3 does not re-derive it, and so §8's re-pin has
something concrete to pin.)*

### 2.4 — Q4: `getNativeFunction` cost during a puck drag

**Answer: do not call it per `pointermove`. Pull the envelope once, cache it, and refresh it on a
change signal. UI-02/5 is still satisfied — see §6.**

`getNativeFunction` is not a call, it is an **async round trip**
(`juce_gui_extra/native/javascript/index.js:73-92`): JS emits `__juce__invoke` with a promise id,
C++ runs the lambda on the message thread, and the result comes back as a separate `__juce__complete`
event that resolves the promise. Three problems at pointer rates:

1. **Out-of-order display.** Promises resolve independently. A readout that writes
   `readout.textContent` in each `.then()` can apply an older response after a newer one and show a
   *stale* metre value while the puck is current. There is no ordering guarantee to lean on.
2. **Message-thread traffic.** 60–120 round trips/second, each a JSON encode/decode across the
   WebKit message boundary, for a value that is a linear function of a quantity that changes only
   when the venue changes.
3. **It is the wrong dependency.** The metres value depends on `(srcX, srcY)` — which JS already
   holds, live, in the SliderState — and on the venue bbox, which is static between venue edits.

**Recommended shape.** One native function `getVenueGeometry` (§4) returns the envelope; JS caches
it and computes metres locally from the plugin's own numbers.

**This is not the failure `pattern_webview_knob_readout_scaled_value` warns about.** That pattern is
about a JS table of *transcribed constants* that drifts from the C++ `NormalisableRange`. Here the
numbers are the plugin's live geometry, fetched from the plugin, refreshed when the plugin's
geometry changes. The pattern's actual rule — *the UI must ask the processor* — is honoured.

**One detail that must not be lost in the move.** `VenueModel::normToMetres()` carries a zero-span
guard: a degenerate axis pins to its minimum rather than dividing by zero (`VenueModel.h:185-192`).
A naive JS `min + n·(max−min)` diverges from the C++ on exactly the degenerate venues 2.1 spent a
whole matrix on. **`getVenueGeometry` must return the degeneracy flags** (`degenerateX`,
`degenerateY`), computed in C++ against `plane::kMinSpan`, so JS branches on a flag rather than on a
transcribed threshold.

**Refresh triggers**, in order of when they land:

| Trigger | Phase | Mechanism |
|---|---|---|
| Editor opens | 3.1 | one `getVenueGeometry` at init |
| A venue edit made in the UI | 3.2 | the edit originates in JS, so JS invalidates its own cache |
| A venue change from *any* source (session restore, `.venue` load, host preset) | 3.1 | `venueGen` carried on the status poll — §2.9 |

The third is the one that would otherwise be a silent staleness hole, and it costs **nothing extra**:
the status poll already exists for the SAFE banner, and the generation counter already exists for the
solver's dirty check (`VenueSnapshot.h:80`, stamped by `publish()`, and `publishSnapshot()` is the
single funnel for every venue change). The UI compares the integer and re-fetches only when it moves.

### 2.5 — Q5: meter atomics, 30 Hz Timer vs `requestAnimationFrame`

**3.3 work; the design is settled here so 3.3 inherits it.** `meterPeak[8]` does not exist yet
(confirmed: no such member in `PluginProcessor.h`).

**Where read-and-zero happens: in the native function, on the message thread, with `exchange`.**

```cpp
// audio thread, after the map write (§4.3)
float prev = meterPeak[i].load (std::memory_order_relaxed);
if (pk > prev) meterPeak[i].store (pk, std::memory_order_relaxed);

// message thread, inside the native function JS polls
const float v = meterPeak[i].exchange (0.0f, std::memory_order_relaxed);
```

**Does zeroing race the max-store? Yes, benignly, and `exchange` is what bounds it.** Interleave the
audio thread's load–compare–store against the UI's exchange:

- UI exchange lands *between* audio's load and store → audio's store overwrites the zero with `pk`.
  Nothing is lost: `pk` is this block's real peak and it is simply attributed to the next UI frame.
- UI exchange lands elsewhere → it reads a complete value.

The worst case is a peak attributed to the adjacent 33 ms frame — invisible at these ballistics.
A plain `load` + `store(0)` from the UI is **not** equivalent and must not be used: it can drop a
peak written between the two.

**Timer vs rAF is not a choice — it is both, doing different jobs.** §4.3 and
`juce8-critical-patterns` §20 divide it: the **poll** (a native round trip, ~30 Hz) fetches new peak
targets; the **rAF loop** (60 fps) runs the ballistics
`current += (target − current) · speed` with attack 0.5 / decay 0.12, plus the 1.5 s peak-hold
releasing at 20 dB/s. Driving ballistics off the 30 Hz poll gives visibly stepped meters; polling at
60 Hz doubles the bridge traffic for no visual gain.

**Poll from JS, not `juce::Timer`.** Following O-ReverseDelay's `getGrainMeter` precedent
(`PluginEditor.cpp:24-38`), a JS-side interval keeps the editor free of a `juce::Timer` and — the
part that matters — keeps the ui-stub able to render the page, because the stub already models
`getNativeFunction` and would otherwise have to model `backend.addEventListener` as well.

**PERF-01 does not regress:** the audio-side addition is one relaxed load, one compare and one
conditional relaxed store per channel per block. No allocation, no lock. UI-03's criterion 4 re-runs
probe AO regardless.

### 2.6 — Q6: does `getAPVTS()` serve the relays?

**Yes. No new surface.** See F1. `getAPVTS().getParameter(id)` returns the `RangedAudioParameter*`
that `WebSliderParameterAttachment` requires. The accessor added unplanned at 2.2 execute does
exactly the job it was added for.

### 2.7 — Q7: does the Venue screen fit at 1100 × 720?

**Yes, with roughly 100 px of slack — but see §3, and read the caveat there before treating that as
settled.**

### 2.8 — Q8: is the ui-stub precedent rich enough?

**Yes, and O-Octagon's stub is *simpler* than O-ReverseDelay's, not richer — provided the design
stays pull-only.** See §5.

### 2.9 — Q9: which atomic reports SAFE mode?

**None exists, and none needs to be added — but adding one is still the better answer.**

SAFE mode is fully determined by the negotiated output set (`PluginProcessor.cpp:181`: mono or
stereo out). Two ways to surface it:

- **Derive it in the native function** from `getBus(false, 0)->getCurrentLayout()`, on the message
  thread. Zero new state, but it puts a second copy of "which sets count as SAFE" a long way from
  `isBusesLayoutSupported()`, where the first copy lives.
- **✅ Recommended — one `std::atomic<bool> safeMode`, written in `prepareToPlay()`** beside
  `preparedYet`. `prepareToPlay` is already the single site that knows the negotiated layout, the
  derivation rule stays adjacent to the predicate it mirrors, and the read is trivially thread-safe.

**The negotiated container *name* (constraint 13) is a separate matter and must not be an atomic.**
A `juce::String` written on one thread and read on another is a race. Resolve it in the native
function on the message thread —
`getBus (false, 0)->getCurrentLayout().getDescription()` — so no cross-thread string exists at all.
Build any surrounding text with `<<` (constraint 9).

**Recommended: fold all of this into one polled `getStatus`** (§4), which then also carries
`mapInvalid` for 3.2 and `venueGen` for §2.4. One poll channel, established at 3.1, extended twice.

### 2.10 — Q10: should darkened-Naturalist become a template?

**Not at 3.1. Decide at 3.3, and only if the aesthetic survives contact with the hall-legibility
question that 3.3 owns.**

`.claude/aesthetics/` holds five templates with a `manifest.json` + per-template
`aesthetic.md` + `metadata.json`, and every one has a `sourcePlugin` (or `null`) — the library's own
convention is that a template is *extracted from a shipped surface*, not authored ahead of one.
D3's split (brand chrome / mono technical data / single brass→pale-gold level ramp) is a genuinely
reusable idea for performance-oriented plugins, but it has never been rendered. Extracting it now
would pin a design that 3.3 may still change: CONTEXT D7 explicitly routes "unreadable at
performance distance" to **3.3 discuss**, and that is the same decision.

**Recommendation:** build it plugin-local at 3.1; revisit extraction at 3.3 verify via
`/ui-template-library`, when there is a rendered surface to extract *from*. No Stage-3 blocker
either way.

---

## 3. D7 — the 1100 × 720 budget, measured rather than asserted

### 3.1 The Room plan is portrait and height-bound

The envelope is derived, not stored (§6.2): speaker bbox + 15 % margin per axis, minimum 1.0 m.
For the §OQ4 default venue:

```
bbox        x [0.50, 12.50] → 12.00 m      y [4.50, 19.50] → 15.00 m
margin/side max(1.0, 0.15 · 12.00) = 1.80  max(1.0, 0.15 · 15.00) = 2.25
envelope    15.60 m × 19.50 m
aspect      15.60 / 19.50 = 0.800   (portrait)
```

Inside a 1100 × 720 window, allowing a ~56 px header (title + screen switcher) and a ~40 px footer,
the plan's available height is ~560 px, so:

```
plan  ≈  448 × 560 px        ← height-bound; widening the window does not enlarge it
```

**This is the single most useful number in this document for the designer.** The natural layout is
**plan left (~450 px), controls right (~610 px)** — not a wide plan with controls beneath it. It
also sharpens D7's stated cost: the accepted "no way to enlarge the plan for a distant read" is
really "the plan is 448 px wide", because the constraint is the 720 px height, not the 1100 px width.

Speaker spacing at that scale is comfortable — the 12 m front pair maps to ~345 px, the closest
adjacent pair (speakers 1↔8, 5.35 m) to ~154 px — so 8 numbered glyphs each carrying an in-plan
weight control fit without collision. The hull renders as a **hexagon**: vertices 1, 2, 4, 5, 6, 7,
with **3 and 8 `ON_EDGE`** on the two side walls, which is exactly what UI-02/2 asserts and is
confirmed by the coordinate table (three collinear speakers at `x = 12.50` and three at `x = 0.50`).

### 3.2 The Venue screen's 42 fields

Column budget at 1100 px with 24 px page padding (1052 usable):

| # | X (m) | Y (m) | Z (m) | Trim (dB) | Channel label | Class | Σ |
|---|---|---|---|---|---|---|---|
| 32 | 110 | 110 | 110 | 110 | 150 | 110 | **732** + gaps |

Row budget:

```
header/switcher    56
placeholder banner 40      (§R8 — the default venue must be unmistakably labelled)
table header       28
8 speaker rows    272      (8 × 34)
rake front/rear    40
negotiated set     28      (constraint 13)
save / load        44
verify-ping block 120      (3.2)
footer             40
                  ───
                  668  →  ~52 px slack in 720
```

**It fits — but this arithmetic is not the gate, and must not be treated as one.** This repo has a
dated scar for exactly this reasoning: `pattern_flex1_container_slack_invisible_to_row_sum` — a
`flex: 1` container's slack is invisible to arithmetic on the row sum, and O-ReverseDelay carried
93.5 px of phantom slack through *five releases* because every comment reasoned about rows and never
subtracted from the frame height. **The gate is a rendered measurement of the real page at exactly
1100 × 720**, which the ui-stub can do headlessly (§5). The table above is a feasibility check that
says "do not redesign D7 pre-emptively", nothing more.

Two follow-ons for 3.2: at 34 px rows with tabular numerals a 13–14 px mono comfortably renders
`12.50` in a 110 px field; and the 120 px verify-ping block is the least certain line in the budget,
because its layout is not yet designed. If 3.2 finds the Venue screen genuinely does not fit, D7 is
revisited at **3.2 discuss** — as CONTEXT already provides for.

---

## 4. Native-function inventory for Phase 3.1 — exactly three

Keeping this surface small is D1's whole argument (*"if the grep-diff gate is going to catch
something, it catches it against 17 bindings and not against 70"*). Three functions, and the count
is the thing `ui_frontend_check.js` asserts in both directions.

| # | Name | Returns | Consumers | Call pattern |
|---|---|---|---|---|
| 1 | `getParameterDefaults` | `{ id: engineeringValue }` × 17 | dblclick-reset | once at init |
| 2 | `getVenueGeometry` | envelope + speakers + hull, **one object** | plan proportions, glyphs, hull overlay, metres readout | init, then on `venueGen` change |
| 3 | `getStatus` | `{ safeMode, outputSetName, numOutputChannels, mapInvalid, venueGen }` | SAFE banner (3.1), `mapInvalid` (3.2), venue-cache invalidation | polled ~2 Hz |

**Why `getVenueGeometry` is one call and not three.** The plan draws the envelope, the 8 glyphs and
the hull overlay from the same geometry. Fetching them separately admits a **torn read** — an
envelope from venue A composited with glyphs from venue B — which is the identical hazard §7.2
addresses on the audio thread by acquiring one snapshot per control block. One call, one consistent
picture, and one fewer bridge surface to keep closed.

Payload shape:

```jsonc
{
  "envelope": { "minX": -1.30, "maxX": 14.30, "minY": 2.25, "maxY": 21.75,
                "degenerateX": false, "degenerateY": false },
  "centroid": { "x": 6.5000, "y": 12.4625 },
  "rigScale": 7.93165,
  "speakers": [ { "n": 1, "x": 0.50, "y": 4.50, "z": 4.50,
                  "label": "L", "class": "VERTEX" }, ... ],   // 8
  "hull":     [ { "x": 0.50, "y": 4.50 }, ... ],              // hullCount points, in order
  "hullCount": 6
}
```

Every field is read from live objects the processor already owns — `getVenue()` for the bbox,
centroid, `rigScale`, positions and labels; `getHull()` for `getHullPoints()`, `getNumHullPoints()`
and `classify(i)`. **The overlay therefore cannot drift from the behaviour it depicts**, because it
calls the same `ConvexHull2D` the solver calls; `classify()` was deliberately made a first-class
return value at 2.1 (P11) for exactly this.

`getStatus` is the only polled function. At 2 Hz it costs ~2 round trips/second and it is what makes
the SAFE banner correct when a host re-negotiates a track's output format with the editor open.

**3.2 and 3.3 extend this list; 3.1 must ship the grep-diff gate that keeps it closed** —
`pattern_webview_native_fn_bridge_gap`, asserted in **both** directions per UI-02/7.

---

## 5. The ui-stub — what O-Octagon needs beyond the precedent

`plugins/O-ReverseDelay/tests/ui-stub/` is 369 lines of `juce-stub.js` plus a 28-line
`serve-stub.sh`. The script's promise is the important part: it copies `Source/ui/public` to a temp
root and swaps **only** `js/juce/index.js` for the stub, so *the page under test is byte-identical to
production*. Copy that mechanism verbatim.

**O-Octagon's stub is smaller than O-ReverseDelay's**, and it is worth stating why so nobody budgets
for a richer one:

| Stub surface | O-ReverseDelay | O-Octagon 3.1 |
|---|---|---|
| `getSliderState` + ranges | 20 | **17** |
| `getComboBoxState` + choices | 4 | **0** |
| `getToggleState` | 1 | **0** |
| `getNativeFunction` whitelist | 13 | **3** |
| `window.__JUCE__.backend` shim | needed (preset-manager polls it) | not needed until 3.2 |
| `backend.addEventListener` | not modelled | **not needed** — pull-only (§2.5) |
| Canvas | n/a | **nothing to stub** — a real browser canvas *is* the thing under test |

Three requirements carried over from the precedent, each of which it learned the hard way:

1. **Ranges mirror `createParameterLayout()`.** The precedent's own comments record `delayTime`
   drifting after a v1.0.1 range change, and record the *trap of the neutral default* (`grainTilt`,
   `grainCount`, `tukeyTaper`, `driftRate` — four separate incidents where the range minimum was not
   the shipped default and the stub rendered a state the plugin never ships in). O-Octagon has two of
   these: **`w1..w8` default to 1.0, not 0.0** (range minimum), and **`blur` defaults to 0.10 and
   `airAmount` to 0.35**. All 17 skews are linear, so no `skewForCentre` is needed.
2. **An unknown native-function name must reject**, not resolve. That rejection is how a bridge gap
   surfaces in the stub instead of as a dead control in a DAW.
3. **A polled stub value must move.** The precedent makes `getGrainMeter` return a *walking* value
   specifically so the render gate can tell a live readout from a dead one. O-Octagon's `getStatus`
   should do the same for `venueGen`, and `getVenueGeometry` must be **mutable** — that mutability is
   what makes UI-02/5's non-vacuity probe possible (§6).

**Fidelity verdict: sufficient, and the constraint runs the other way.** The precedent is rich
enough because the design is pull-only. If 3.3 later reaches for `emitEventIfBrowserIsVisible` to
push meters, the stub grows a `backend.addEventListener` model — which is precisely the cost
O-ReverseDelay declined to pay. Keep the design pull-only unless something forces otherwise.

**Two gates the stub enables that a plugin build cannot:**

- **The TDZ gate (D4's mandatory mitigation).** With no browser-iteration phase, the first render of
  the page would otherwise be inside a plugin. `pattern_module_toplevel_init_tdz`'s prescription —
  a single `init()` call as the **last** statement of the module, so no top-level initializer can
  reach a not-yet-initialised binding — is statically checkable in `ui_frontend_check.js` and
  observable in the stub render.
- **The 1100 × 720 layout measurement (§3.2).** `ui_tooltip_clamp_check.js` is the model: it serves
  the real page, drives it with Playwright at the exact shipping viewport, and *measures rendered
  boxes*. Its own header records why — a static check cannot see a viewport-sensitive failure. Any
  O-Octagon layout gate must measure at exactly 1100 × 720, never compute.

---

## 6. Probing UI-02's seven criteria — including the one that does not fit at 3.1

| # | Criterion | Where it is proved | Notes |
|---|---|---|---|
| 1 | Plan aspect follows the derived envelope | Playwright at 1100 × 720: read the rendered canvas box, mutate the stub's `getVenueGeometry` bbox, re-measure | asserts against the *returned* envelope, never a literal — `pattern_test_fixture_mirrors_drift_silently` |
| 2 | Hull overlay matches, 3 and 8 `ON_EDGE` | stub returns `hullCount: 6` + classifications; probe asserts 6 polygon vertices and that glyphs 3/8 carry the on-edge treatment | plus a C++ probe in the render harness asserting `classify(2) == classify(7) == ON_EDGE` on the default venue |
| 3 | Puck drag is relative-delta | static scan of `app.js` for the `lastX/lastY` frame-delta form, plus a Playwright drag that grabs the puck **off-centre** and asserts it does not jump | the off-centre grab is what makes this non-vacuous; a centred grab passes under absolute tracking too |
| 4 | Canvas explicit `width`/`height` + DPR backing store | static scan for `calc()` sizing and a `devicePixelRatio` backing-store multiply; Playwright reads `canvas.width` vs `getBoundingClientRect().width` at DPR 1 **and** 2 | `o-textureforge-cursor-bug` — the bug is invisible at one DPR |
| 5 | **Metres readout resolves against the live venue** | **three-part — see below** | the venue-edit path it implies does not exist until 3.2 |
| 6 | Rendered against the stub before integration | the stub render itself, asserting zero console errors and that every one of the 17 controls is present | `pattern_module_toplevel_init_tdz` |
| 7 | Bridge closure both directions | `ui_frontend_check.js` grep-diff: every `getNativeFunction` name in JS ↔ every `withNativeFunction` in C++, expected surface **exactly 3** | `pattern_webview_native_fn_bridge_gap` |

### Criterion 5 — the one that needs designing

UI-02/5's non-vacuity gate is *"with the puck stationary, editing a venue coordinate must change the
readout."* At 3.1 there is **no Venue screen** — coordinate editing lands at 3.2. Testing it as
literally written is impossible this phase, and passing it vacuously would be worse. Three parts,
each proving a different half, and together non-vacuous:

- **(a) JS half — stub, Playwright.** `getVenueGeometry` in the stub is mutable. Read the metre
  readout with the puck stationary; mutate the stub's bbox; trigger the refresh path; assert the
  readout changed. **A JS min/max map fails this.** This is the direct analogue of the criterion.
- **(b) C++ half — render harness.** Call `applyVenueEdit()` with one speaker moved, then read the
  same accessors `getVenueGeometry` reads (`getVenue().bbMinX()` etc.) and assert the envelope
  moved. Proves the C++ side is live without needing a WebView.
- **(c) Static.** Assert the `getVenueGeometry` lambda body reads `processorRef.getVenue()` and
  contains **no numeric literal** for any bbox bound — the only way the two halves could be
  independently correct and still wired to a constant.

**(a) + (b) + (c) closes at 3.1. The end-to-end version — type a coordinate on the Venue screen,
watch the Room readout move — is a 3.2 gate**, and 3.2 should re-run it there. Declare that split at
the 3.1 verify boundary rather than letting 3.2 discover it, per the Stage-2 discipline that produced
zero verify-time surprises.

---

## 7. New risks and pitfalls found at this boundary

| # | Risk | Severity | Disposition |
|---|---|---|---|
| N1 | **Ungestured parameter writes** — the puck (3.1) and scenes (3.3) both write through a path with no `beginChangeGesture`. Sound moves; the host may not record it. Silent, and invisible to build/auval/pluginval | **HIGH** | 3.1 plan task: bracket the puck drag on **both** `srcX` and `srcY`. Static probe asserts `sliderDragStarted`/`Ended` are paired around every `setNormalisedValue` in the drag handler |
| N2 | **Preset-manager `setStateFromXml` bypasses the §4.1 restore ordering** — the venue would restore into the tree while geometry, hull and map keep describing the previous venue | **HIGH** (3.2) | Recorded in §2.1. Adopt the module for presets only; never route session state through it |
| N3 | **`SIDES` has no derivable definition** — D5's phrasing admits several readings that disagree on real rigs | MEDIUM (3.3) | Predicate proposed and measured in §2.3; re-pin §6.3 at 3.3 discuss (§8) |
| N4 | **Puck echo lag** — rendering the puck from `getScaledValue()` on the async echo makes it trail the pointer | MEDIUM | Optimistic local position during drag; authoritative value on `sliderDragEnded` |
| N5 | **Relative-delta accumulator sticks at the range edge** — accumulating past 0 or 1 and then reversing leaves the puck unresponsive until the surplus is unwound | MEDIUM | Clamp the *written* value, keep the accumulator unclamped, or re-derive from the parameter each frame. Probe: drag well past an edge, reverse, assert immediate response |
| N6 | **Venue-cache staleness from a non-UI venue change** (session restore, `.venue` load) | MEDIUM | Closed for free by `venueGen` on the `getStatus` poll (§2.4) |
| N7 | **`getStatus` poll interacts with the stub render gate** — a stub returning a constant renders identically whether JS polls or not | LOW | Stub returns a walking `venueGen`, per the `getGrainMeter` precedent |

Carried, unchanged: MSVC C3493 and `SafePointer` init-capture in nested lambdas are **write-time
habits** (constraint 11) — neither has a call site in 3.1, since the `FileChooser` work is 3.2, but
the habits start now.

---

## 8. Contract dispositions

Neither gap found at discuss is an error; both are absences, and research adds a third. **None is
amended at this boundary.**

| Gap | Contract | Research finding | When to re-pin |
|---|---|---|---|
| FUNC-06 named-scene semantics undefined | `ARCHITECTURE §6.3` | Confirmed, and **worse than discuss assessed**: D5's `SIDES` is not derivable as written (§2.3) | **3.3 discuss** |
| `SCENES` has no storage location | `ARCHITECTURE §4.1` (2-node tree diagram) | Confirmed; D6's sibling placement is sound and needs no ordering treatment (§2.1) | **3.3 discuss** |
| **NEW** — §6.3 says scenes write via `setValueNotifyingHost`, which is an *ungestured* write | `ARCHITECTURE §6.3` | The stated mechanism does not deliver the stated outcome without gesture brackets (F2) | **3.3 discuss**, with the two above |

**Why 3.3 and not now.** All three describe scene behaviour, and scenes are built at 3.3. Pinning a
derivation rule two phases before anything implements it invites exactly the drift the pin exists to
prevent, and the boundary discipline says amendments happen *at* boundaries — 3.3 discuss is a
boundary. They are recorded here so 3.3 inherits the analysis instead of re-deriving it.

**`REQUIREMENTS.md` is not edited at this boundary.** FUNC-06/1's *"read back all 8 host-side
parameter values"* is confirmed correct as written (F3) and needs no change.

---

## 9. Handed to plan

The P-series continues from Stage 2's **P36** — Phase 3.1's first plan decision is **P37**.

**Must be plan tasks:**

1. `tests/ui-stub/` — `juce-stub.js` (17 ranges, 3 native fns, rejecting whitelist, mutable
   geometry, walking `venueGen`) + `serve-stub.sh`, built **before** any integration (UI-02/6, D4).
2. `Source/ui/public/` — `index.html`, `css/styles.css`, `js/app.js`, `js/roomplan.js` (**no
   hyphen**), `js/juce/index.js` + `check_native_interop.js` copied from JUCE 8.0.14.
3. `Source/PluginEditor.{h,cpp}` — 17 relays / webview / 17 attachments in that member order;
   `getResource` by bare-path equality; the three native functions; `setSize(1100, 720)`;
   `#if JUCE_WEB_BROWSER` untouched; `PluginEditor.cpp` **never** added to the render-harness target.
4. `CMakeLists.txt` — `juce_add_binary_data(... NAMESPACE UIBinaryData ...)` + link.
5. `PluginProcessor` — `std::atomic<bool> safeMode` written in `prepareToPlay()`; a
   `getVenueGeneration()` accessor over the existing publisher counter. **Nothing else.** No new
   parameters (constraint 10), no `AsyncUpdater` (§2.1).
6. `tests/ui_frontend_check.js` — static gates: bridge closure both directions (surface **= 3**),
   TDZ single-`init()`-last, readouts via `getScaledValue()`, gesture pairing (N1), no bbox literals
   (§6c), member order, harness exclusion, both WebView2 CMake flags.
7. `tests/ui_layout_check.js` — Playwright at **exactly 1100 × 720**: plan aspect vs returned
   envelope, hull vertex count, canvas DPR backing store at DPR 1 and 2, off-centre puck grab,
   edge-reversal (N5), metres non-vacuity (§6a).
8. Render-harness additions: `classify()` on the default venue (§6, criterion 2) and the
   `applyVenueEdit` → envelope-moved probe (§6b).

**Deliberately deferred, with destinations:** scenes and the `SIDES` predicate → 3.3; venue editing,
verify-ping, presets, `mapInvalid` surfacing, the negotiated-set name → 3.2; meters → 3.3; the three
§8 re-pins → 3.3 discuss; the aesthetic-template extraction question → 3.3 verify.

**Predicted outcome is unchanged from discuss: Phase 3.1 closes UI-02 ✅ and nothing else.** The one
qualification research adds is UI-02/5's three-part split (§6), which should be declared at the 3.1
verify boundary with 3.2 named as the destination for its end-to-end half.

---

## 10. References

**In-tree JUCE 8.0.14** (`/Users/taylorbrook/JUCE`)
- `modules/juce_audio_processors/utilities/juce_ParameterAttachments.cpp:59, 76, 91, 279-333` — gesture semantics, the unchanged-value skip, `WebSliderParameterAttachment`
- `modules/juce_audio_processors/utilities/juce_ParameterAttachments.h:273-307` — the 3-argument constructor and the drag-started/ended listener contract
- `modules/juce_gui_extra/misc/juce_WebControlRelays.cpp:45-109` — relay echo and compare-before-notify
- `modules/juce_gui_extra/misc/juce_WebBrowserComponent.h:304-333, 372, 592` — `withNativeFunction`, `withEventListener`, bare-path resource provider, `emitEventIfBrowserIsVisible`
- `modules/juce_gui_extra/native/javascript/index.js:73-92, 135-235` — the async promise transport and `SliderState`

**In-repo**
- `plugins/O-ReverseDelay/Source/PluginEditor.{h,cpp}` — the reference editor
- `plugins/O-ReverseDelay/tests/ui-stub/{juce-stub.js,serve-stub.sh}` — the stub precedent
- `plugins/O-ReverseDelay/tests/ui_frontend_check.js` (15 static sections) / `ui_tooltip_clamp_check.js` (Playwright at the shipping viewport) — the probe models
- `modules/persistence/preset-manager/cpp/OuariconPresetManager.h:299, 556, 580` — `applyPresetJson`, `getStateAsXml`, `setStateFromXml` (N2)
- `troubleshooting/patterns/juce8-critical-patterns.md` §3, §8, §11, §12, §13, §16, §20
- `plugins/O-Octagon/Source/{PluginProcessor.h,PluginProcessor.cpp,Data/VenueModel.h,Data/VenueSnapshot.h,DSP/ConvexHull2D.h}`

**Memory patterns applied**
`pattern_webview_native_fn_bridge_gap` · `pattern_module_toplevel_init_tdz` ·
`pattern_webview_knob_readout_scaled_value` · `pattern_js_state_updater_overwrites_html_labels` ·
`o-textureforge-cursor-bug` · `critical_webview_resource_provider_and_schemes` ·
`critical_binary_data_strips_hyphens` · `critical_dual_binary_data_namespace_collision` ·
`critical_juce_webview_namespace_vs_postmessage` · `critical_juce_string_char_ctor_is_ascii_only` ·
`critical_webview2_static_linking` · `pattern_render_harness_breaks_on_webview_editor` ·
`pattern_flex1_container_slack_invisible_to_row_sum` · `pattern_tooltip_clamp_gate_viewport_sensitive` ·
`pattern_test_fixture_mirrors_drift_silently` · `pattern_asyncupdater_guard_flag_needs_cancel` ·
`pattern_preset_apply_needs_reset_to_defaults` · `pattern_promotion_checksum_pins_replaced_file`

---

## Next Phase

Ready for: **plan** — `/plugin-plan O-Octagon 3-gui`

The three findings plan must not lose: **N1** (gesture brackets on the puck — a 3.1 code change, not
a 3.3 one), **§3.1** (the plan is 448 × 560, so the layout is plan-left / controls-right), and
**§6 criterion 5** (the three-part probe, with its end-to-end half declared as a 3.2 gate).
