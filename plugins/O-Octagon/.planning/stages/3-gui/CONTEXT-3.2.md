# Stage 3 — GUI · Phase 3.2 (Venue screen, venue store, verify-ping) — Context

**Plugin:** O-Octagon
**Stage:** 3 of 4 — GUI
**Phase:** 3.2 of 3 — Venue screen, venue store, verify-ping
**GSD phase:** discuss
**Date:** 2026-08-12
**Branch:** `feat/o-octagon` @ `a47cef88` (Stage 2 phases 2.2 / 2.3 and Stage 3 phase 3.1 uncommitted)
**Participants:** Taylor Brook, Claude

---

## Entry Check — carried obligations from Phase 3.1

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

**No drift on arrival, and no contract is amended at this boundary.** The three §8 re-pins identified
at 3.1 research are all scene-related and remain scheduled for **3.3 discuss**, untouched.

### Carried obligations, and their disposition here

| Carried from | Obligation | Disposition at this boundary |
|---|---|---|
| 3.1 plan (P45) / 3.1 summary | **UI-02/5's end-to-end half is a declared 3.2 gate** — edit a coordinate on the Venue screen, watch the Room readout move | **Accepted and scheduled.** D8's commit model is what makes it drivable; carried into plan as a named gate |
| 3.1 research (N2) / 3.1 summary | **Never route session state through `OuariconPresetManager::setStateFromXml`** | **Honoured and sharpened by D10.** Verified in module source at this boundary — see the finding below |
| 3.1 discuss (D2) | D5 manual Logic gate — OPEN | Unchanged. **Stage 4 hall session.** Not run at 3.2 |
| 2.1 verify onward | CI gap — all probes and both JS gates are local-only | Unchanged. Stage 4. **3.2 widens it again** |
| 2.2 verify | `COMPAT-04` retroactive criteria | Unchanged. Stage 4. Still the only summary row without a criteria section |
| 3.1 discuss (constraint 13) | The **negotiated container name** must be surfaced on the Venue screen — Stage 4's R2 reads it off the UI | **3.2 work.** Already returned by `getStatus` as `outputSetName`; 3.2 renders it |
| 3.1 verify / summary | D7's legibility cost — the plan is **448 px wide**, measured | Unchanged. **3.3 discuss** |

### Numbering note — the D-series CONTINUES at D8

Stage 2 restarted the D-series at each phase. **Stage 3 does not**, because `CONTEXT-3.1.md`'s
D1–D7 are *stage-level* decisions (cycle structure, the D5 fold, aesthetic, design workflow, scene
semantics, scene persistence, editor sizing) and are referenced by those numbers throughout
`STATUS.md`, `SUMMARY-3.1.md` and `VERIFICATION-3.1.md`. Restarting would make "D2" mean two
different things in one stage. **3.2's decisions are D8–D14.** The P-series continues from **P50** —
3.2's first plan decision is **P51**.

---

## Discussion Summary

3.1 delivered the shell and closed `UI-02` 7/7. **3.2 is where the plugin stops being a renderer of
plugin state and starts being an editor of it.** Every requirement it carries is a write path:
42 venue values typed by a user, a `.venue` file, a musical preset, and an audio generator commanded
from a button.

The risk profile shifts accordingly. 3.1's hazards were *silent-when-broken* rendering bugs. 3.2's
are **silent-when-broken state bugs**, and two of them are unrecoverable in a hall: a venue that
half-applied, and a preset that quietly rewrote the room. §R1 is still the highest risk in the
project, and 3.2 is the phase that puts a label-map dropdown in front of a user.

Two things were verified in source at this boundary rather than assumed, and both **shrink** 3.2:

1. **`applyVenueEdit()` already writes back to `apvts.state`** (`PluginProcessor.cpp:394-406`:
   `venue.writeToState (apvts.state)` → `hull.build()` → `rebuildChannelMap()` or
   `publishSnapshot()`). A `.venue` file load is therefore *parse → build a `VenueModel` →
   `applyVenueEdit()`*, and it reuses the single construction site. **3.2 adds no second venue-apply
   path**, which is what keeps §4.1's ordering hazard from reappearing.
2. **`OuariconPresetManager::applyPresetJson` cannot reach the VENUE node** — it iterates
   `parameters.processor.getParameters()` and resolves by `parameters.getParameter (id)`
   (`OuariconPresetManager.h:298-350`). It never walks `apvts.state`'s children. FUNC-05 criterion 1
   holds **by construction**, not by discipline. See D10 for the single exception that must be
   guarded.

The Venue screen's **read** path is also largely built already: `getVenueGeometry` returns per-speaker
`x`/`y`/`z`/`label`/`class`, and `getStatus` returns `outputSetName`, `mapInvalid`,
`numOutputChannels` and `venueGen`. **What 3.2 owns is the write path, the file store, the preset
store, and the ping.**

---

## Requirements Confirmed

3.2 verifies **FUNC-02, FUNC-04, FUNC-05, UI-01** — four rows, all **`must`** priority. This is the
heaviest requirement load of any Stage-3 phase, and the only one where every row is `must`.

| Requirement | Priority | Criteria | Owned by |
|---|---|---|---|
| FUNC-02 — Measured venue entry | must | 3 | The 42-field table + `.venue` store |
| FUNC-04 — Verify-ping | must | 3 | `Source/DSP/VerifyPing.{h,cpp}` |
| FUNC-05 — Preset separation | must | 3 | The musical preset store + session round-trip |
| UI-01 — Venue measurement screen | must | 3 | The Venue screen itself |

### There is NO criteria debt at this boundary

Unlike 3.1 — which discharged five empty criteria sections — **all four of 3.2's rows have carried
acceptance criteria since Stage 0.** The debt cleared at 3.1 discuss was `FUNC-06` + `UI-02..05`.
Re-checked at this boundary: **30 summary rows, 29 criteria sections**, and the single row still
without a section is `COMPAT-04` — exactly the known Stage-4 debt. **No new gap.**

**`REQUIREMENTS.md` is therefore NOT edited at this boundary.** 3.1's edit was the scheduled
discharge of a debt dated to that exact boundary; no such debt is dated here, and editing criteria
for a `must` row without one would be rewriting a target at the moment of aiming at it.

### Seven ROADMAP criteria are carried by NO requirement-row line — named here, not left for verify

`ROADMAP.md` Phase 3.2 lists thirteen test criteria. Seven of them have no corresponding line in
FUNC-02/04/05 or UI-01. 3.1's plan named two such orphans for UI-02 and carried them as gates rather
than letting verify discover the gap; **the same discipline applies, and the list is longer here.**

| # | ROADMAP criterion | Nearest row | Why it is not carried |
|---|---|---|---|
| 1 | **Latched ping self-stops at 120 s** | FUNC-04 | No FUNC-04 line mentions the timeout at all — and **D11 makes it a safety decision this boundary owns** |
| 2 | Ping ceiling holds **at `trim = +6 dB`** | FUNC-04/3 | The criterion says *"regardless of `outputGain`"* only. The trims are a **separate** multiply (FUNC-07, live since 2.3) |
| 3 | Auto-cycle completes 8 **in 12.8 s** | FUNC-04/2 | The row says *"completes all 8 unattended"* with no time bound; 12.8 s is what makes FUNC-04's headline *"under a minute"* true |
| 4 | Duplicate/missing label **surfaces the warning** and does not reroute | FUNC-03 (✅ closed 2.2) | The **rejection** half closed at 2.1 (probes E/F/G). The **surfacing** half is new UI work in an already-complete row |
| 5 | A label-map row change moves audio, **confirmed by ping** | FUNC-03/3 (✅ closed 2.2) | Closed by probe **AJ**, which used a rendered tone. The **ping-confirmed** half is new |
| 6 | The **negotiated set name** is shown on the Venue screen | UI-01 | UI-01's three criteria cover the 42 values, the ping and save/load. The set name is not a venue value — it is 3.1 constraint 13 and **Stage 4's R2 reads it off this screen** |
| 7 | Per-speaker **hull classification readout** (VERTEX / ON_EDGE / INTERIOR) | UI-01/1 | *"All 42 venue values"* — the classification is **derived**, not one of the 42 |

**Every one of these must appear in `PLAN-3.2.md` as a named gate.** Items 4 and 5 are the sharpest:
they attach to a requirement row that is already ✅ **complete**, so nothing in the requirements
document will ever go yellow if they are dropped.

### UI-01/3 cannot be closed by any existing gate class — declared now

*"Venue save/load round-trips through a named file"* runs through `juce::FileChooser::launchAsync`,
which opens a **native modal**. Playwright cannot drive it, and the render harness has no editor.
Left undeclared, this becomes a verify-time surprise on a `must` row.

**It closes on the P45 three-part shape**, declared here so plan does not have to invent it:

- **(a)** a C++ probe that saves and reloads through the **same `juce::File`-taking functions** the
  chooser's completion lambda calls, bit-comparing all 42 values;
- **(b)** a static assertion that the completion lambda calls **exactly those functions** and no
  parallel serialisation path exists;
- **(c)** a Gate-13-style Standalone launch-and-look for the chooser itself — the only thing that
  proves a native modal opens at all from a WebView button.

The same reasoning applies to the SafePointer rule: see open question Q4, which is a genuine hole in
the pattern rather than a restatement of it.

---

## Approach Decisions

| # | Decision | Choice |
|---|---|---|
| D8 | Venue field commit semantics | **Commit on blur / Enter; Escape reverts. One `setVenue` call carrying all 42 values, atomically** |
| D9 | Venue screen layout | **Table-left / rail-right, with a LIVE MINI-PLAN in the rail.** D7's 1100 × 720 survives |
| D10 | Musical preset store | **Adopt `OuariconPresetManager` for presets ONLY.** Session state stays O-Octagon's own — and **no custom-state callback is registered at 3.2** |
| D11 | Verify-ping lifetime | **Four independent stops: editor close, bypass, 120 s latch, explicit Stop. A tab switch is NOT one of them** |
| D12 | Numeric input element | **`type="text"` + `inputmode="decimal"` with an explicit parse — NOT `type="number"`** |
| D13 | `mapInvalid` surfacing | **A frame-level banner beside the SAFE banner, visible on BOTH screens** |
| D14 | Ping active-speaker indicator | **C++ is the authority. The UI never re-derives the step from a JS timer** |

---

### D8 — commit on blur / Enter, and the whole venue goes in one call

A field commits when it loses focus or `Enter` is pressed. `Escape` reverts it to the last committed
value. On commit the page sends **one `setVenue` native call carrying all 42 values**, and C++ builds
a complete `VenueModel` and hands it to the existing `applyVenueEdit()`.

**Two distinct problems are solved by one decision, and both are real:**

1. **Live-per-keystroke applies transiently degenerate venues.** Typing `12.5` passes through `1` and
   `12` — each a *real* venue that gets a real `hull.build()` and a real `rebuildChannelMap()`. A rig
   whose x-span collapses toward `plane::kMinSpan` mid-keystroke is the exact degeneracy matrix 2.1
   built probes for, reached by ordinary typing rather than by a pathological venue.
2. **A per-field write is a torn write.** This is **P38's argument, one layer up.** `getVenueGeometry`
   is a single call specifically so the page cannot composite an envelope from venue A with glyphs
   from venue B. A `setVenueField(i, "x", v)` surface reintroduces exactly that hazard on the write
   side: 42 independent async round trips whose promises may resolve out of order (3.1 research Q4),
   against a model that recomputes bbox, centroid, `rigScale` and the hull on **every** one.

**The accepted cost, stated:** the Room plan does not follow the digits as they are typed. **D9's
mini-plan is the compensation**, and it is why the two decisions are taken together rather than
separately.

**What this buys UI-02/5's inherited end-to-end gate:** a discrete, observable commit event is
exactly what a Playwright gate can drive and assert against — type into the Venue field, blur, then
read the Room screen's metre readout. A live-per-keystroke model has no commit boundary to hang the
assertion on.

---

### D9 — table-left / rail-right, with a live mini-plan

```
┌─ VENUE ─────────────────────────────────────────────────────────┐
│  #  LABEL     X       Y       Z      TRIM     CLASS  │  RAKE F/R │
│  1  L      │  0.50 │  4.50 │  3.20 │  +0.0 │ VERTEX  │  ──────── │
│  2  R      │ 12.50 │  4.50 │  3.20 │  +0.0 │ VERTEX  │  SET: 7.1 │
│  3  C      │  6.50 │  2.25 │  3.20 │  +0.0 │ ON_EDGE │  ──────── │
│  4  LFE    │                                          │ ┌───────┐│
│  …                                                    │ │ mini  ││
│  8  Rrs    │                                          │ │ plan  ││
│                                                       │ └───────┘│
│                                                       │ SAVE LOAD│
│                                                       │ ── PING ─│
└─────────────────────────────────────────────────────────────────┘
```

**This resolves 3.1 discuss open question Q7**, which 3.1 could not answer because it only built the
Room screen. The 42 values are one 8-row table (`label`, `x`, `y`, `z`, `trim` = 40) plus 2 rake
fields — **not** the 42-field grid the question feared. UI-01/1's *"without leaving the screen"* is
met by a single table, and D7's fixed 1100 × 720 survives.

**The mini-plan is not decoration.** It is the feedback D8 removes, restored in a form that is *more*
useful than live digits: a mis-typed `12` for `1.2` shows up as a rig that **changed shape**, which is
legible at a glance in a way a wrong number in a column is not. §R8 — *"the venue measurement never
happens"* — is a project risk about people not entering real numbers; seeing the room redraw as you
enter them is the cheapest possible counter-pressure.

**It reuses `roomplan.js`'s single `metresToPx()` (P46)**, which is what keeps the mini-plan from
becoming a second projection free to drift from the one the Room screen draws. Whether that
parameterises cleanly without breaking `ui_frontend_check.js` §19's *exactly one definition*
assertion is **Q8**, handed to research — the gate is real and must not be weakened to accommodate
this.

---

### D10 — adopt the module for presets only, and register NO custom-state callback

**Verified in module source at this boundary, not inferred.** `applyPresetJson`
(`OuariconPresetManager.h:298-350`) resets every parameter to its default and then applies the
preset's values, in both cases through `processor.getParameters()` and
`parameters.getParameter (id)`. **It never walks `apvts.state`'s children.** So FUNC-05 criterion 1 —
*"loading a musical preset leaves all 42 venue values bit-identical"* — is a structural property of
the module, exactly as §4.1 argued it would be for any parameter-scoped preset writer.

**There is precisely one hole, and it is worth naming loudly:**

> `setCustomStateCallbacks (CustomSaveCallback, CustomLoadCallback)` — `customSave()` is written into
> the preset as `customState`, and `customLoad()` is invoked on load
> (`OuariconPresetManager.h:100-104, 287-289, 346-348`). **This is the only path by which a preset
> can reach non-parameter state.** FUNC-05's whole guarantee reduces to a single greppable
> assertion: *O-Octagon registers no custom-state callback that can reach `VENUE`.*

**At 3.2, O-Octagon registers no custom-state callback at all.** That is the strongest form of the
assertion and it is trivially checkable. At **3.3** the callback becomes legitimate — D6 puts
`SCENES` in musical presets — and that is precisely why **FUNC-06/5 re-runs FUNC-05's bit-compare
after `SCENES` exists** rather than inheriting the claim. The two decisions were made a phase apart
and they line up.

**N2 stands unchanged and is the other half:** `setStateFromXml` calls `parameters.replaceState(...)`
and nothing else, so routing **session** restore through it would bypass §4.1's
`readVenueFromState()` → `rebuildChannelMap()` ordering. O-Octagon's own `getStateInformation` /
`setStateInformation` (`PluginProcessor.cpp:483-528`) are kept **exactly as they are**.

The module is header-only (`modules/persistence/preset-manager/cpp/OuariconPresetManager.h`, v1.0.5)
with a 447-line JS side. Whether the JS side assumes a DOM shape O-Octagon does not have, and what it
costs the bridge grep-diff, is **Q10**.

---

### D11 — four stops, and a tab switch is not one of them

**Stops the ping:** the editor destructor (window closed) · processor bypass · the 120 s safety latch
· an explicit Stop button.
**Does NOT stop the ping:** switching between the Room and Venue tabs.

The asymmetry is deliberate and it is about the hall. One operator walking the room needs the ping to
keep sounding while they look at the Room plan; the same operator closing the plugin window must not
leave pink noise in a PA nobody is watching. §OQ2 already specifies the 120 s timeout for *"a
forgotten ping running through a rehearsal"* — **D11 adds the two stops OQ2 does not name**, and
window-close is the one that actually happens.

**Bypass is included because a bypassed plugin still emitting noise is a genuinely confusing thing to
debug on a stage** — the first instinct is to bypass the plugin, and if that does not silence it the
diagnosis goes somewhere wrong.

**The 120 s timeout is ROADMAP criterion 1 in the orphan table above** — no FUNC-04 line carries it,
and D11 is the reason it must be gated rather than merely implemented.

---

### D12 — `type="text"` + `inputmode="decimal"`, not `type="number"`

FUNC-02 criterion 1 requires the fields to *"accept typed metre values and **reject** non-numeric
input."* **`<input type="number">` makes that criterion untestable, because the DOM discards the
rejected input before anything can observe it:** on invalid content `.value` returns the empty
string and `.valueAsNumber` returns `NaN`, so *"the user typed `abc`"* and *"the user cleared the
field"* are indistinguishable — and there is nothing left to show the user in order to reject it.
The commit path also cannot distinguish a deliberate clear from a typo.

`type="text"` with `inputmode="decimal"` keeps the numeric keypad on touch, keeps the typed string
intact, and makes the reject path an explicit, assertable branch: **parse → on failure, mark the
field and revert to the last committed value on blur.** It also sidesteps locale decimal-separator
behaviour and the spinner controls, which are wrong for a measured value in metres.

**Reject means revert, not hold.** A field left holding invalid text is a field whose displayed value
does not describe the room — the state D13 exists to make impossible to miss.

---

### D13 — `mapInvalid` gets a frame-level banner, on both screens

The `mapInvalid` warning renders as a banner in the **frame**, beside the existing SAFE banner, and
is therefore visible on the Room screen as well as the Venue screen.

**A duplicate or missing label assignment is R1 — the highest risk in the project — and R1's defining
property is that it is silent.** Putting its only warning behind a tab the user is not currently
looking at reproduces the failure mode the warning exists to break. The SAFE banner precedent is
already built, already frame-level, already polled at 2 Hz, and `getStatus` **already returns
`mapInvalid`** (`PluginEditor.cpp:317`) — so this is a rendering decision, not new plumbing.

Whether the banner needs the *reason* (duplicate / missing / unresolvable label) rather than a
boolean is **Q9**. In a hall, *"which row"* is the actionable half.

---

### D14 — the ping's active speaker is read from C++, never re-derived in JS

UI-01 criterion 2 requires the ping's *"active speaker is visually indicated."* The indicated speaker
**must be the one the audio thread is actually sounding**, read back from C++.

A JS-side `setInterval` stepping 1→8 on the same 1.2 s / 0.4 s schedule would look correct on every
screenshot and would be a **second implementation of the cycle**, free to drift from the first — and
drift here means the UI names speaker 5 while speaker 6 is sounding, during the one procedure whose
entire purpose is confirming that speaker N is speaker N. That is R1 reproduced inside its own
diagnostic tool.

**The transport is a genuine open question, not a decision.** `getStatus` polls at 2 Hz, but the
auto-cycle's gap is **0.4 s** — a 500 ms poll can miss a gap entirely and lags the indicator by up to
half a period. Raising the poll, adding a second faster poll only while pinging, or pushing from C++
via `emitEvent` are all defensible; this is **Q3**.

---

## Constraints Identified

Carried from 3.1 and still binding — 1–11 below are unchanged, and the WebView constraint set from
`CONTEXT-3.1.md` applies in full to every new page element:

1. **`createEditor` stays guarded with `#if JUCE_WEB_BROWSER`** — 32 harness probes die silently
   otherwise (`pattern_render_harness_breaks_on_webview_editor`). P48's two-arm form is asserted by
   `ui_frontend_check.js` §11.
2. **`std::unique_ptr` member order is relay → webview → attachment** — a destruction-order
   requirement (juce8-critical-patterns §3).
3. **The resource provider receives bare PATHS** — never hard-code `juce://`
   (`critical_webview_resource_provider_and_schemes`).
4. **`juce_add_binary_data` strips hyphens** — a `venue-table.js` becomes `venuetable_js`
   (`critical_binary_data_strips_hyphens`). The `UIBinaryData` namespace target already exists.
5. **Pass the `Juce` ES-module namespace, not `window.__JUCE__`**
   (`critical_juce_webview_namespace_vs_postmessage`); `ui_frontend_check.js` §7/§8 already ban both
   tokens outside comments.
6. **`juce::String(const char*)` is ASCII-only** — this cost a real defect at 3.1 (D-2, the mangled
   em-dash in the venue name, silent since Stage 2). §20 now asserts no non-ASCII string literal in
   `PluginEditor.cpp`; **every new C++ string 3.2 writes inherits that gate**
   (`critical_juce_string_char_ctor_is_ascii_only`).
7. **`FileChooser::launchAsync` completions capture a `juce::Component::SafePointer`, and on a dead
   pointer `return` **bare** — never `complete(false)`**, which is itself a use-after-free
   (`pattern_webview_launchasync_safepointer_no_complete`, ARCHITECTURE §4.4). See **Q4** — the
   WebView native-function context adds a wrinkle the pattern does not cover.
8. **The 17 parameters are frozen.** `parameter-spec.md` is pinned; 3.2 adds none. Venue values,
   verify-ping and the preset store are **not** parameters (§6.3).
9. **No file I/O of any kind in `processBlock`** (PERF-01, §4.4). Venue save/load and preset load are
   message-thread and user-initiated, always.
10. **MSVC hazards are authored now, not fixed at port time** — C3493 (non-static `constexpr` in a
    lambda) and `SafePointer(this)` init-capture in nested lambdas. 3.2 writes more lambdas than 3.1
    did, including the chooser completions.
11. **The default venue must stay labelled unmistakably as a placeholder** (§R8) — and 3.2 is the
    phase where a user replaces it.
12. **The ping is injected AT the channel map, after everything else, with all other channels
    hard-zeroed** (§OQ2). Bypassing DBAP, weights, hull, trim and `outputGain` is the entire point —
    it means a ping failure has exactly one possible cause.
13. **Member-owned `juce::Random`, never `getSystemRandom()`** (§F9) — the system random is a shared
    thread-local; an audio-thread RNG shared across phases also breaks block-size invariance
    (`pattern_rng_stream_interleave_blocksize`).
14. **The bridge grep-diff gate now runs against a much larger surface.** 3.1's was 3 native
    functions; 3.2 adds the venue write, the file store, the preset store and the ping. **This is
    precisely what D1's three-cycle structure was bought for** — the gate catches a gap against 3.2's
    surface, not against 3.3's combined one.

---

## Open Questions for Research

1. **Does `applyPresetJson`'s reset loop emit 17 spurious host automation writes?** It calls
   `setValueNotifyingHost (getDefaultValue())` on **every** parameter before applying the preset
   (`OuariconPresetManager.h:323-330`). In Logic with a lane in Latch or Touch, loading a preset while
   armed could record a full default sweep across all 17. FUNC-05/1's *clean load* rests on this, and
   3.1 research N1 already showed this repo's attachment layer has non-obvious gesture behaviour.
2. **Is `customState` the right home for `SCENES` at 3.3, and does registering the callback at all
   create a reachable path to `VENUE`?** D10 registers none at 3.2. Research should establish what
   3.3 will need so that FUNC-06/5's re-measurement has a defined target rather than an open one.
3. **Ping indicator transport (D14).** `getStatus` polls at 2 Hz; the auto-cycle gap is 0.4 s. Raise
   the poll rate globally, add a faster poll only while pinging, or push from C++ with
   `emitEvent` + `withEventListener`? Note that a push path is the first thing in this UI that is not
   pull-only — 3.1 research flagged that the ui-stub does **not** model `backend.addEventListener`,
   so a push design has a stub cost D4 made mandatory.
4. **A `FileChooser` launched from inside a native function leaves a JS promise unresolved if the
   editor dies.** The SafePointer rule says `return` bare on a dead pointer — but the page's
   `await nativeFn("loadVenue")` then never settles. Is a permanently pending promise acceptable
   (the page is being torn down anyway), or does the design need a one-shot guard or a timeout? The
   pattern as written does not cover the WebView-native-function context.
5. **Where exactly does the ping inject relative to `GainStage`'s smoothers and the map write**, and
   does starting or stopping it need a `reset()`? §OQ2 says *"at the channel map, all other channels
   hard-zeroed"*, but `GainStage` owns the write and P23/P30 established *one reset site, ever*.
   The 20 ms raised-cosine envelope may make a reset unnecessary — that is the H1 argument from 2.3
   research in a different place.
6. **`.venue` schema-version mismatch policy.** `kSchemaVersion = 1`. What does load do with a file
   written by a future version — reject, accept with a warning, or partial-load? FUNC-02/2's
   *"reproduces all 42 values exactly"* says nothing about a foreign file.
7. **The label-swap transient.** Swapping L↔R passes through a duplicate assignment, which
   `rebuildChannelMap()` **rejects**, retaining the last valid map (probe F) and raising
   `mapInvalid`. Is a transient-invalid state acceptable and legible under D13's banner, or does the
   UI need an explicit swap gesture? A user cannot reorder the map without passing through it.
8. **Does the mini-plan break `ui_frontend_check.js` §19's *exactly one `metresToPx` definition*?**
   D9 requires the same projection at a different box size. Parameterisation should hold, but the
   gate is real and **must not be weakened to accommodate the feature** — that would invert P46.
9. **Does `mapInvalid` need a reason code** (duplicate / missing / unresolvable label) rather than a
   boolean? In a hall, *which row* is the actionable half. `ChannelMap` already distinguishes these
   cases internally (probes E / F / G).
10. **What does the preset-manager's 447-line JS side assume?** Does it require a DOM shape O-Octagon
    does not have, does it add native-function surface the §3 grep-diff must track, and does adopting
    it disturb §16's four-way parameter closure (spec == `createParameterLayout` == `params::id()` ==
    DOM `ctl-<id>`)?
11. **Does the 42-field table plus rail actually fit at 1100 × 720 at the tabular type size legibility
    demands?** This is 3.1's Q7, now answerable because D9 fixes the layout. If it does not, D7 is
    revisited **here**, not at 3.3.

---

## Confirmed available — do not rebuild

- **`applyVenueEdit (const VenueModel&)` is public and complete** — writes to `apvts.state`, rebuilds
  the hull, runs the single channel-map construction site, publishes the snapshot. **The venue write
  path already exists**; 3.2 supplies the `VenueModel` and the surface that builds it.
- **`getVenueGeometry` already returns per-speaker `x`/`y`/`z`/`label`/`class`** plus `bbox`,
  `envelope`, `centroid`, `rigScale`, `venueName`, `hull` and `generation`. The Venue table's read
  path is a rendering job. **Missing from the payload: the 8 trims and the two rake heights** — the
  only venue values not on the wire.
- **`getStatus` already returns `outputSetName`, `mapInvalid`, `numOutputChannels` and `venueGen`.**
  D13's banner and ROADMAP orphan #6 are both rendering jobs.
- **`VenueModel` has the full setter surface** — `setSpeakerPosition`, `setSpeakerTrimDb`,
  `setSpeakerLabel`, `setRake`, `setName`, plus `toValueTree()` / `readFromState()` /
  `writeToState()`. The `.venue` file format needs no new serialisation code.
- **`venuePublisher.getGeneration()`** is the change signal the Room screen's cache already
  invalidates on — a venue edit on the Venue screen therefore reaches the Room screen through
  machinery that is **already built and already gated** (`ui_layout_check.js` §7).
- **`mapInvalid` is already raised and already atomic** (`PluginProcessor.cpp:293`), and the
  last-valid-map retention is already probed (E / F / G at 2.1).
- **The render harness (32 probes) and both JS gate files (20 + 10 sections) exist and are
  non-vacuous** — 8 negative controls fired at 3.1 execute, 2 more at verify.
- **`OuariconPresetManager` v1.0.5** at `modules/persistence/preset-manager/` — header-only C++ plus
  a JS side. Adopted for **presets only** (D10).
- **`tests/ui-stub/juce-stub.js`** — must be extended for 3.2's new native functions, and the
  stub-first ordering (UI-02 criterion 6, D4's mandatory mitigation) applies to every new page module.

---

## Predicted Outcomes — declared here, not to be discovered at verify

**Phase 3.2 closes: FUNC-02 ✅, FUNC-04 ✅, FUNC-05 ✅, UI-01 ✅.** All four `must`. Nothing else —
FUNC-06 and UI-03/04/05 are 3.3.

**Declared partial in advance — none.** This is the fourth consecutive phase to declare zero partials
at discuss; the three Stage-2 phases and 3.1 each produced zero verify-time surprises under it. If a
criterion cannot be met it will be declared **at the 3.2 verify boundary with a named destination**.

**Also closing at 3.2, though carried by no requirement row:** UI-02/5's inherited end-to-end gate
(P45) — type a coordinate on the Venue screen, watch the Room readout move — plus the seven ROADMAP
orphans tabled above.

### Residuals Stage 3 does not close — unchanged, restated so none is read as settled

1. **D5 / QUAL-01's *audible* clause** — Stage 4 hall session. Gate 13 was not D5 and neither is
   anything at 3.2.
2. **The CI gap** — Stage 4. **3.2 widens it again**: the ping probes, the venue round-trip probes
   and the new JS gate sections are all local-only.
3. **`COMPAT-04`** retroactive criteria — Stage 4. Still the only summary row without a section.
4. **`COMPAT-02`** (Logic Pro on a surround track) — Stage 4.
5. **UI-04 / UI-05 descope to v1.1** — **3.3 discuss**, unchanged. UI-03 is `should` and is **not**
   descopable.
6. **D7's legibility cost** — the Room plan is 448 px wide, measured. **3.3 discuss.** Note that
   **Q11 may return a second, independent legibility finding for the Venue table at 3.2**; if it
   does, it is answered here rather than deferred, because D9 is the decision that would change.
7. **The three §8 contract re-pins** (all scene-related) — **3.3 discuss**.

---

## Contract Gaps Found at This Boundary

**None.** Unlike 3.1, which found two absences in §6.3 and §4.1, everything 3.2 needs is specified:
§4.1 fixes the state ordering and the `.venue` format, §4.4 fixes the file I/O rule, §OQ2 fixes the
ping in full detail, §6.3 lists venue save/load as a non-parameter, and §F9 fixes the RNG ownership.

**No contract is amended and no checksum moves at this boundary.**

The seven ROADMAP criteria carried by no requirement row are **not** contract gaps — `ROADMAP.md` and
`REQUIREMENTS.md` are both complete documents that simply enumerate at different granularities. They
are a **traceability** gap, and the disposition is to carry them as named plan gates, which is what
3.1's plan did with UI-02's two orphans.

---

## Next Phase

Ready for: **research** — `/plugin-research O-Octagon 3-gui`

Eleven questions above. The highest-value ones are **Q1** (whether a preset load emits 17 automation
writes — FUNC-05/1 rests on it and 3.1's N1 already proved this attachment layer has surprises),
**Q4** (the `FileChooser` promise hole, which the SafePointer pattern does not cover and which sits
on a `must` row's only untestable-by-existing-gates criterion), and **Q3** (the ping indicator
transport, which may force the first push-based path in a deliberately pull-only design and therefore
carries a ui-stub cost).
