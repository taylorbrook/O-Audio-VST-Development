# Stage 3 — GUI · Phase 3.2 (Venue screen, venue store, verify-ping) — Plan

**Plugin:** O-Octagon
**Stage:** 3 of 4 — GUI
**Phase:** 3.2 of 3
**GSD phase:** plan
**Date:** 2026-08-12
**Branch:** `feat/o-octagon` @ `a47cef88` (2.2 / 2.3 / 3.1 work uncommitted)
**Inputs:** `CONTEXT-3.2.md` (D8–D14), `RESEARCH-3.2.md` (N4–N8, Q2/Q3/Q5/Q6/Q8/Q11, §3, §4, §7),
`ROADMAP.md` Phase 3.2 (13 test criteria), `REQUIREMENTS.md` FUNC-02 / FUNC-04 / FUNC-05 / UI-01
(3 criteria each), `research/ARCHITECTURE.md` §4.1 / §4.4 / §6.3 / §7.2 / §7.4 / §F9 / §OQ2 / §R1 /
§R8, `PLAN-3.1.md` (P37–P50).

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

**All four byte-exact. No pin moves at 3.2.** The three §8 re-pins are all scene-related and stay
scheduled for **3.3 discuss**. Q6's `.venue` foreign-version policy is an **absence** in §4.1, not an
error in it — it is resolved by **P56**, a plan gate, and not by a re-pin. Recorded here so 3.3 does
not read it as settled contract.

**`REQUIREMENTS.md` is NOT edited at this boundary** (P68). All four of 3.2's rows have carried three
acceptance criteria each since Stage 0; no criteria debt is dated here. Re-checked at discuss:
30 summary rows, 29 criteria sections, `COMPAT-04` the only row still without one — the known
Stage-4 debt, unchanged.

### Task 0 is discharged at plan, unlike 3.1

**Playwright is resolvable.** `P49`'s gate does not skip, and 3.1 had to install it as Task 0. At
this boundary the gate's own resolver finds it at
`~/.npm/_npx/6f4879659183bc49/node_modules/playwright`, with browsers in
`~/Library/Caches/ms-playwright`. **Verified by running the resolver, not by checking `~/.cache`** —
that path does not exist on this machine and a naive check would have reported a false blocker.
3.2 has **no Task 0**; execute re-runs the resolver once and proceeds.

---

## Goal

**The plugin stops being a renderer of state and becomes an editor of it.** A user types 42 measured
values, stores them in a named file, loads a musical preset without disturbing them, and confirms
every speaker by ear from a button — and every write path that could half-apply, silently reroute or
quietly rewrite the room is a gate that runs.

**`FUNC-02` ✅, `FUNC-04` ✅, `FUNC-05` ✅ and `UI-01` ✅ close — all four `must`, the heaviest
requirement load of any Stage-3 phase.** FUNC-06 and UI-03 / UI-04 / UI-05 are 3.3.

### What "works" means concretely at 3.2

| Delivered | Not delivered until |
|---|---|
| Venue screen: 8-row table (label · x · y · z · trim · class), 2 rake fields, negotiated set name | Meters, level-gradient backdrop, elevation strip — 3.3 |
| Live mini-plan in the rail, height-bound, through the **same** `metresToPx` | Named scenes, `SIDES` predicate, 4 user slots, `SCENES` node — 3.3 |
| `.venue` save / load through `FileChooser::launchAsync` | — |
| Musical preset store (save · load · list · current), `OuariconPresetManager` **header only** | Scene-carrying presets and the one custom-state callback — 3.3 |
| `VerifyPing`: manual latch, auto-cycle 1→8, 120 s self-stop, fixed ceiling | — |
| `mapInvalid` banner **with its reason**, frame-level, on both screens | — |
| UI-02/5's inherited end-to-end gate, closed | — |

### The three findings this plan must not lose

Research named them in its own handoff, and each is a task obligation below:

1. **N8 — `mapInvalid` is AUDIBLE, and it is not the retained map.** `mappedOutputAvailable()` false
   sends `GainStage` to the `else` arm, which writes `out[ch][n] = ch == 0 ? sL : sR` with
   `numWrite 8` (`GainStage.cpp:408, 461`). **Speaker 1 gets L; speakers 2–8 all get R at unity.**
   So `setVenue` validates the label set **before** `applyVenueEdit()`. → **P52**, Task 11, probe BP,
   NC6.
2. **Two gates enumerate their JS inputs BY NAME** — `ui_frontend_check.js` §3 scans
   `[S.appJs, S.roomJs]` (`:165, :167`), §19 tests `S.appJs` alone (`:649`). A new `venue.js` is
   invisible to both; **they pass by not looking.** Sixth time this vacuity class has been caught.
   → **P51**, Task 1 (**first**, before the file exists), NC1 / NC2.
3. **N4 — a native function's `complete()` is dropped when the browser is hidden.** No error, no
   rejection, the promise never settles, and `withKeepPageLoadedWhenBrowserIsHidden()` keeps the page
   alive to have awaited it. **No UI state may depend solely on a promise resolving.** → **P64**,
   §25.

---

## Requirement staging — read this before writing the verify report

Declared here so verify discovers nothing plan did not predict. This is the discipline that produced
zero verify-time surprises across three Stage-2 phases and 3.1.

| Requirement | Verdict predicted at 3.2 | Evidence |
|---|---|---|
| **FUNC-02** | ✅ **complete** | 3 criteria; mapping below |
| **FUNC-04** | ✅ **complete** | 3 criteria; mapping below |
| **FUNC-05** | ✅ **complete** | 3 criteria; mapping below |
| **UI-01** | ✅ **complete** | 3 criteria; mapping below |
| UI-02 | already ✅ at 3.1 — **must not regress**, and its inherited end-to-end gate closes here | layout §14 |
| FUNC-06, UI-03, UI-04, UI-05 | untouched — 3.3 | — |
| All 18 Stage-2 rows + UI-02 | must not regress | 65 existing probes re-run, plus BN–BZ |

**Declared partial: none.** Fifth consecutive phase under that discipline. If a criterion cannot be
met it is declared at the 3.2 verify boundary **with a named destination**.

### FUNC-02 — measured venue entry

| # | Criterion | Proved by | Task |
|---|---|---|---|
| 1 | All 24 coordinate fields and both rake heights accept typed metre values and **reject** non-numeric input | `ui_layout_check.js` §13 — type `abc`, assert the field marks and **reverts on blur**; type a valid metre, assert it commits. Testable only because **D12** chose `type="text"` (`type="number"` makes "typed `abc`" indistinguishable from "cleared the field") | 8 |
| 2 | Saving then reloading a venue reproduces **all 42 values exactly** | **Probe BN** — bit-compare all 42 through the same `juce::File`-taking functions the chooser completion calls | 15 |
| 3 | The DBAP solve uses the entered values, verified by a **changed gain vector** after a coordinate edit | **Probe BZ** — drive `applyVenueEditChecked()` (the function `setVenue` calls) with one coordinate moved, assert the solved gain vector changed. Distinct from AQ, which drives `applyVenueEdit` directly and therefore does not exercise 3.2's path | 15 |

### FUNC-04 — verify-ping

| # | Criterion | Proved by | Task |
|---|---|---|---|
| 1 | Ping plays from **exactly one** speaker at a time, all others silent | **Probe BQ** — on a **non-identity** map, render each of the 8 targets and assert energy in exactly one buffer lane and **exact zero** in the other seven. Also carries ROADMAP orphan 5: change a label row, re-ping, assert the sounding lane followed | 15 |
| 2 | Manual step advances 1→8 and auto-cycle completes all 8 **unattended** | **Probe BS** — auto-cycle measured in SAMPLES: 8 × (1.2 s on + 0.4 s gap) = **12.8 s**, order 1→8, at 48 kHz = 614 400 samples. Carries ROADMAP orphan 3 | 15 |
| 3 | Ping level is bounded by a fixed conservative ceiling regardless of `outputGain` | **Probe BR** — measure RMS and peak at `outputGain = +12 dB` **and** `trim = +6 dB`. Carries ROADMAP orphan 2, which the criterion's *"`outputGain`"* wording does not reach: the trims are a separate multiply (FUNC-07, live since 2.3) | 15 |

### FUNC-05 — preset separation

| # | Criterion | Proved by | Task |
|---|---|---|---|
| 1 | Loading a musical preset leaves all 42 venue values **bit-identical** | **Probe BW** (bit-compare across a preset load) **+ §27**, the one greppable assertion: `setCustomStateCallbacks` appears **nowhere** in O-Octagon's source, and `preset-manager.js` is not vendored. `applyPresetJson` iterates `processor.getParameters()` only and can never walk `apvts.state`'s children — the criterion holds **by construction** and the probe measures it anyway | 12, 14, 15 |
| 2 | A preset saved under venue A recalls correctly under venue B, **position resolved against venue B's bbox** | **Probe BX** — save under the default venue, apply a venue whose bbox differs, load the preset, assert `srcX/srcY` are unchanged **normalised** and the resolved metres followed venue B | 15 |
| 3 | Session state round-trips **both stores together** | **Probe BY** — `getStateInformation` / `setStateInformation` across a modified venue **and** modified parameters, bit-compare both | 15 |

**N2 stands and is honoured:** session state keeps O-Octagon's own `getStateInformation` /
`setStateInformation` exactly as they are. `OuariconPresetManager::setStateFromXml` calls
`replaceState()` and nothing else, which would bypass §4.1's `readVenueFromState()` →
`rebuildChannelMap()` ordering.

### UI-01 — venue measurement screen

| # | Criterion | Proved by | Task |
|---|---|---|---|
| 1 | All 42 venue values are **viewable and editable without leaving the screen** | `ui_layout_check.js` §12 — all 42 fields present, editable and inside the viewport at exactly 1100 × 720; **§11** proves the rail itself does not overflow. `ui_frontend_check.js` §22 closes the 42 ids four ways | 8, 14 |
| 2 | Verify-ping is reachable from this screen and its **active speaker is visually indicated** | `ui_layout_check.js` §18 — Start, then assert the indicator follows the **returned** `getPingState().speaker`; **§26** asserts statically that no JS timer re-derives the step (**D14**) | 8, 14 |
| 3 | Venue save/load **round-trips through a named file** | **Three parts (P57)** — (a) probe **BN** through the same `juce::File`-taking functions, (b) **§29**'s static assertion that the completion calls exactly those and no parallel serialisation path exists, (c) **Gate 13**, a Standalone launch-and-look for the native modal itself | 15, 14, 19 |

**UI-01/3 cannot be closed by any existing gate class** — `FileChooser::launchAsync` opens a native
modal Playwright cannot drive and the render harness has no editor. Declared at discuss, designed
here, so it is not a verify-time surprise on a `must` row.

### The seven ROADMAP orphans — each a named gate

`ROADMAP.md` Phase 3.2 lists thirteen test criteria; seven are carried by no requirement-row line.
3.1's plan named two such orphans for UI-02 rather than letting verify find the gap; the list is
longer here and the discipline is the same.

| # | ROADMAP criterion | Gate | Task |
|---|---|---|---|
| 1 | Latched ping **self-stops at 120 s** | **Probe BT** — sample-counted, `120 s × sampleRate`; no FUNC-04 line mentions the timeout at all | 15 |
| 2 | Ping ceiling holds at **`trim = +6 dB`** | folded into **probe BR** | 15 |
| 3 | Auto-cycle completes 8 in **12.8 s** | folded into **probe BS** | 15 |
| 4 | Duplicate/missing label **surfaces the warning** and does not reroute | **Probe BP** (the guard rejects before apply) + `ui_layout_check.js` §16 (banner with reason, **both screens**) | 15, 8 |
| 5 | A label-row change moves audio, **confirmed by ping** | folded into **probe BQ**'s second half. AJ closed the rendered-tone half at 2.2; this is the ping-confirmed half | 15 |
| 6 | The **negotiated set name** is shown on the Venue screen | `ui_layout_check.js` §17. 3.1 constraint 13; **Stage 4's R2 reads it off this screen** | 8 |
| 7 | Per-speaker **hull classification** readout | `ui_layout_check.js` §17 — the CLASS column renders `VERTEX` / `ON_EDGE` / `INTERIOR` from the payload, not from a JS guess | 8 |

**Items 4 and 5 attach to `FUNC-03`, which is already ✅ complete** — nothing in `REQUIREMENTS.md`
goes yellow if they are dropped, which is exactly why they are named here.

---

## Plan Decisions

The P-series continues from 3.1's **P50**. Phase 3.2 is **P51–P68**.

---

### P51 — The page-module enumeration is **DERIVED FROM DISK**, not widened by hand *(RESEARCH §4)*

Research's sharpest item: `ui_frontend_check.js` §3 scans `[S.appJs, S.roomJs]` and §19 tests
`S.appJs` alone. A `venue.js` absent from both arrays makes both sections **pass by not looking**.

The obvious fix is to add `S.venueJs` to two arrays. **This plan does not do that**, because it
leaves the same hole open for 3.3's next module and would have to be remembered a third time. It
takes the form the rest of this project already uses for the same class of problem — `kSliderIds` is
built from `oo::params::id(i)` and never transcribed (P37); the DBAP fixture is derived from its
generator; the Layer-2 golden is parsed out of JUCE:

```js
// The page modules, DERIVED. Every .js under Source/ui/public/js/ EXCEPT the
// two verbatim JUCE files. A new module is covered the moment it exists.
const PAGE_MODULE_DIR = path.join(publicDir, 'js');
const PAGE_MODULES = fs.readdirSync(PAGE_MODULE_DIR)
    .filter(f => f.endsWith('.js'))
    .map(f => ({ name: `js/${f}`, src: fs.readFileSync(path.join(PAGE_MODULE_DIR, f), 'utf8') }));
```

**Sections 1, 3, 6, 12, 14 and 19 all iterate `PAGE_MODULES`** instead of naming files. A new
**§21** asserts three things, and the third is what makes the derivation trustworthy:

1. `PAGE_MODULES.length >= 3` — a directory read returning nothing must fail, not pass vacuously;
2. every module is in `juce_add_binary_data` SOURCES (§9's closure already requires the converse);
3. the derived set **equals** the set §9 computes from CMake, so a module that exists on disk but is
   never embedded fails here rather than 404-ing at runtime.

**Task 1 does this FIRST, before `venue.js` exists.** Research said *"widen both enumerations in the
same task that creates the file"*; deriving them one task earlier is strictly stronger, and it means
Task 5 cannot forget. **NC1 and NC2 demonstrate both halves fire.**

---

### P52 — `setVenue` **validates before it applies**, and the predicate is the same function the backstop uses *(N8)*

`mapInvalid` is not a quiet retention. `mappedOutputAvailable()` false → `GainStage`'s `else` arm →
`out[ch][n] = ch == 0 ? sL : sR` with `numWrite = 8`. **Speaker 1 gets the left input at unity;
speakers 2 through 8 all get the right input at unity.** Under D8's commit-on-blur a label swap
holds that state for as long as the user takes to type the second label.

So a new processor method sits **in front of** the existing single apply path:

```cpp
/** Validates the label set against the NEGOTIATED output set, then applies. On failure the venue
    is NOT touched, applyVenueEdit() is NOT reached, and `whyNot` names the reason and the row.
    Message thread only. */
bool applyVenueEditChecked (const oo::VenueModel& newVenue, ochan::MapDiagnosis* whyNot = nullptr);
```

**The check builds the map into a SCRATCH array using `ochan::buildSpeakerToBuffer()` itself.** Not
a reimplementation of "is this label set valid" — literally the same function the audio path's
backstop calls, so the guard and the backstop cannot drift. On success it calls `applyVenueEdit()`,
which builds into the real array. One extra map build per commit, on the message thread, against a
`std::array<int,8>`: free.

Three properties this preserves, each of which a different reviewer will check:

- **3.2 adds no second venue-apply path.** `applyVenueEdit()` remains the only construction site;
  this is a guard in front of it (CONTEXT's shrinking finding, unchanged).
- **The backstop stays exactly where it is.** A session restored from a foreign venue, or a host
  re-negotiating to a set that no longer contains a stored label, still fails in
  `rebuildChannelMap()` and still raises `mapInvalid`. The guard removes the *transient*, not the
  safety net.
- **`applyVenueEdit()` stays public**, because probe BL calls it. The rule that keeps that from
  becoming a hole is checkable: **§22 asserts `PluginEditor.cpp` contains no `applyVenueEdit (`
  call site.** The editor reaches the venue through the checked entry point and nothing else.

**NC6:** remove the guard, and probe **BP** fires.

---

### P53 — The label column **holds and marks**; the numeric columns **revert** *(N8, and it contradicts D12 for exactly one column)*

D12 fixed *"reject means revert, not hold"* — a field left holding invalid text describes a room that
does not exist. That is right for a numeric field: reverting `abc` to the last good metre loses
nothing.

**It is wrong for the label column, and the reason is a reachability argument, not a taste one.**
Every route from `(L, R)` to `(R, L)` passes through a duplicate. If a label that would duplicate
reverts, **L↔R is unreachable** — the user cannot ever swap two speakers.

So the label column has one extra state:

| Column | Invalid input | Commit |
|---|---|---|
| x, y, z, trim, rake | mark, **revert to last committed on blur** | proceeds |
| **label** | mark **both colliding rows**, **hold the pending edit** | **BLOCKED until the set is a permutation again** |

While the label set is not a permutation, the page **does not call `setVenue` at all** — so
`applyVenueEditChecked` never sees it and the guard is a backstop for the guard. Both layers exist
because the page can be wrong (a bug) and the state can arrive from elsewhere (a session restore).

`ui_layout_check.js` **§15** drives the full swap: set row 1 to `R` while row 2 is `R` → both rows
marked, `setVenue` **not called**; then set row 2 to `L` → commit proceeds, and the map follows.
**NC4:** make the label column revert like the others, and §15 fires — L↔R becomes unreachable.

---

### P54 — `mapInvalid` carries a **reason and a speaker index**, and it needs **no new atomic** *(N7, P43 reused)*

`buildSpeakerToBuffer` already separates three failure modes and throws the distinction away in a
`bool` (`ChannelMap.cpp:49-50`, `:61-62`, `:67-68`). The same file already carries the idiom for
surfacing it — `verifyEnumBitOrder (set, juce::String* whyNot)` (`:78-86`).

```cpp
namespace ochan
{
    enum class MapFailure { none, notEightChannels, labelNotInSet, duplicateLabel };

    struct MapDiagnosis
    {
        MapFailure reason { MapFailure::none };
        int        speakerIndex { -1 };   // 0-based; -1 when the failure names no single row
    };

    bool buildSpeakerToBuffer (const juce::AudioChannelSet& outSet,
                               const std::array<juce::AudioChannelSet::ChannelType, kNumSpeakers>& labels,
                               std::array<int, kNumSpeakers>& out,
                               MapDiagnosis* whyNot = nullptr);   // ← default arg: every existing
}                                                                 //   call site compiles unchanged
```

`isPermutationOf0to7` is **not** changed — it is probed by name at 2.1. The duplicate's row is found
by a scan inside `buildSpeakerToBuffer` after `next` is computed.

**No new atomic, and this is P43's rule reused rather than re-argued.** `rebuildChannelMap()` runs on
the message thread and `getStatus` runs on the message thread, so the diagnosis is a plain member.
**`mapInvalid` itself stays exactly the `std::atomic<bool>` it is** (`PluginProcessor.cpp:293`),
because that one *is* read by the audio thread through `mappedOutputAvailable()`. The same reasoning
that keeps `outputSetName` off an atomic keeps the reason string off one: it is built inside
`getStatus`, on the message thread, from the enum.

**After N8, the banner is the only thing telling an operator why seven speakers just went mono.**
In a hall, *which row* is the actionable half.

---

### P55 — `getVenueGeometry` gains the **8 trims and 2 rake heights** — still **ONE** call *(RESEARCH §3, P38 unchanged)*

The only venue values not yet on the wire. Added to the existing payload rather than to a second
function, because P38's argument is untouched: three calls admit a torn read — an envelope from
venue A composited with glyphs from venue B, which is the §7.2 hazard P16 fixed on the audio thread.

```jsonc
{
  "envelope":  { … },                  // unchanged
  "bbox":      { …, "degenerateX": false, "degenerateY": false },   // unchanged
  "centroid":  { … },  "rigScale": 7.93165,  "venueName": "…",
  "speakers":  [ { "n":1, "x":0.50, "y":4.50, "z":4.50,
                   "label":"L", "class":"VERTEX",
                   "trimDb": 0.0 }, … ],          // ← NEW: trimDb, per speaker
  "rake":      { "front": 0.0, "rear": 0.0 },     // ← NEW
  "hull": [ … ], "hullCount": 6, "generation": 4
}
```

`trimDb` rides **inside** the speaker object rather than as a parallel `trims[8]` array, so a
consumer cannot index the two out of step. The rake is its own object because it is venue-scoped,
not per-speaker. **42 = 8 × 5 + 2 is now fully representable from one call**, which is what makes
the Venue table a rendering job and `setVenue` its exact inverse.

---

### P56 — `.venue` I/O is its own TU: **fresh model, forward version surfaced, malformed root rejected** *(Q6)*

`@schemaVersion` is written (`VenueModel.cpp:253`) and **read but never branched on** — the code says
so at `:212-214` — and every attribute falls back to the **existing** value. Right for session state,
where the tree came from this plugin. **Wrong for a file:** a `.venue` written by a future build
loads as a mixture of the file and whatever the model already held — a room that is partly measured
and partly placeholder, with nothing on screen distinguishing it from a correct load. That is
CONTEXT's *"a venue that half-applied"*, and it is unrecoverable in a hall.

**New TU: `Source/Data/VenueFile.{h,cpp}`, namespace `oo::venuefile`, no processor reference.**

```cpp
namespace oo::venuefile
{
    enum class LoadResult { ok, forwardVersion, malformedRoot, unreadable };

    bool save (const VenueModel& venue, const juce::File& target);

    /** Loads into `out`, which is default-constructed FRESH by the caller — never the live model.
        `fileVersion` receives @schemaVersion when the root parses. */
    LoadResult load (const juce::File& source, VenueModel& out, int* fileVersion = nullptr);
}
```

Three rules, all cheap:

1. **Load into a fresh `VenueModel`, never the live one.** A missing attribute then falls back to a
   *known §OQ4 default* rather than to a neighbouring venue's value, so the worst case is legible.
2. **`schemaVersion > kSchemaVersion` → load, but SURFACE it.** Refusing a forward file is worse
   than loading it — the operator has the numbers either way — but silent best-effort is the one
   option to rule out. It is reported in the same banner class D13 establishes.
3. **A root that is not `VENUE`, or that yields fewer than 8 `SPEAKER` children, is rejected
   outright** and the live venue is **not touched**.

**Why a separate TU and not a `VenueModel` member.** The unit target links `juce_audio_basics`,
`juce_core` and `juce_data_structures` and **no** `juce_audio_processors` / `juce_dsp` /
`juce_gui_extra` — that narrow link line is Gate 11 and is what makes "no `PluginProcessor.cpp`"
structural. `juce::File`, `ValueTree` and `createXml()` all live inside it, so `VenueFile.cpp` joins
the unit target **and Gate 11 still passes** — verified against `tests/unit/CMakeLists.txt:91-99` at
plan. That is what puts UI-01/3's probe (a) in the fast target rather than behind a plugin.

---

### P57 — UI-01/3 closes on **three parts**, and the shape is P45's, reused deliberately

*"Venue save/load round-trips through a named file"* runs through a native modal. Playwright cannot
drive it; the render harness has no editor. Left undeclared it is a verify-time surprise on a `must`
row.

| Part | Where | What it proves | Task |
|---|---|---|---|
| **(a)** | **Probe BN** — `venuefile::save()` then `venuefile::load()` into a fresh model, bit-comparing all 42 values | The round trip is exact, through **the same two functions the completion lambdas call** | 15 |
| **(b)** | `ui_frontend_check.js` **§29** — the two completion lambdas call exactly `venuefile::save` / `venuefile::load`, and **no parallel serialisation path exists** (no `createXml` / `writeTo` / `parseXML` anywhere outside `VenueFile.cpp`) | The only way (a) could be correct and the button still be wired to something else | 14 |
| **(c)** | **Gate 13** — Standalone launch-and-look: click Save, the native modal opens, name a file, reopen it | The one thing no automated gate in this repo can prove: that a native modal opens **at all** from a WebView button | 19 |

**(b) is what makes (a) non-vacuous**, and it is the same reasoning P45(c) used for the bbox
literals: two correct halves wired to different code is the failure this class of gate exists for.

---

### P58 — Adopt the preset manager's **C++ header only**; four native functions, not ten *(N6, D10)*

`OuariconPresetManager` is 663 lines of header-only C++ plus a 447-line JS side. **The FUNC-05
guarantee lives entirely in the header**: `applyPresetJson` iterates
`parameters.processor.getParameters()` and resolves via `parameters.getParameter(id)`
(`OuariconPresetManager.h:298-350`) — it never walks `apvts.state`'s children.

The JS side carries two costs O-Octagon will not pay:

1. **Ten native functions wired unconditionally in its constructor** (`preset-manager.js:107-119`).
   Any one C++ does not register becomes an **N4-class never-settling promise** the moment it is
   called — and it would blow the §3 bridge count in the direction that is hardest to reason about.
2. **`createPresetBar()` writes `container.innerHTML`** and then queries its own injected markup
   (`:411-434`) — `pattern_js_state_updater_overwrites_html_labels` by construction, and
   incompatible with D3's authored chrome.

**So: `#include` the header, construct one `OuariconPresetManager`, expose the FOUR functions the
rail needs** — `savePreset`, `loadPreset`, `getPresetList`, `getCurrentPreset` — and write
O-Octagon's own JS. `preset-manager.js` is **not vendored**; **§27** asserts its absence.

**Register NO custom-state callback at 3.2.** `setCustomStateCallbacks` is the *only* path by which
a preset can reach non-parameter state (`:100-105`, written `:287-289`, read `:346-349`). At 3.2 the
strongest form of FUNC-05's guarantee is available and it is trivially checkable: **the symbol
appears nowhere.** At 3.3 the callback becomes legitimate (D6 puts `SCENES` in musical presets),
which is exactly why **FUNC-06/5 re-runs FUNC-05's bit-compare** rather than inheriting it. The two
decisions were made a phase apart and they line up.

**§16's four-way parameter closure is undisturbed** — none of the four preset functions is a
parameter or uses a `ctl-` id, and 3.2 adds no parameter (constraint 8).

---

### P59 — Bracket the preset load with **17 gestures at O-Octagon's call site**; do **not** fork the module *(N5)*

`applyPresetJson` calls `setValueNotifyingHost` **directly on the parameter object**
(`OuariconPresetManager.h:325-341`), not through a `ParameterAttachment`. Two consequences, both
verified through the chain:

- **F3's unchanged-write skip does not apply.** `callIfParameterValueChanged` is a member of the
  *attachment* (`juce_ParameterAttachments.cpp:88-95`), and this path never goes through one. A
  preset load from an untouched default patch still emits **17 reset writes** of values the
  parameters already hold. (All 17 are `AudioParameterFloat` and none overrides `isMetaParameter()`,
  so the two-pass meta machinery is a no-op: pass 0 matches nothing, pass 1 matches all 17.)
- **There are no gesture brackets.** `setValueNotifyingHost` is `setValue` +
  `sendValueChangedMessageToListeners` and nothing else. The wrappers turn that into a bare
  `kAudioUnitEvent_ParameterValueChange` (AU, `AU_1.mm:1341-1360`) and a bare `paramChanged`
  (VST3, `VST3.cpp:1498-1501`) — up to **34 unbracketed host writes** per load.

**Fix at O-Octagon's call site:** `beginChangeGesture()` on all 17 before, `endChangeGesture()` on
all 17 after. ~6 lines in `PluginEditor.cpp`, changes no shared module, and it **generalises P39's
rule** — *open a gesture on every parameter you will write, and close every one of them* — from the
puck to the preset store.

**Do not edit `OuariconPresetManager.h`.** Four other plugins depend on it and this boundary is not
the place to move a shared module. **§28** asserts the bracketing; **NC5** removes it and §28 fires.

FUNC-05 criterion 1 is not threatened either way — the writes are confined to the 17 APVTS
parameters and cannot reach `VENUE`. What is threatened is the *musical* experience of loading a
preset in Logic with a lane in Latch or Touch.

---

### P60 — `Source/DSP/VerifyPing.{h,cpp}`: a **post-write overwrite**, no `reset()`, clocks counted in **samples** *(Q5, §OQ2, §7.2, §F9)*

§7.2 fixes the position unambiguously — *"Verify-ping override **after** the write, **before**
metering"* — and §OQ2 fixes the routing — *"injected at the channel map, all other channels
hard-zeroed"*. Together the ping is a **post-write overwrite of the eight mapped output pointers**,
not a term folded into the gain path. It sits at the end of `renderChunk`'s REAL arm, after the
sample loop and after the NaN guard, indexed through `snapshot.speakerToBuffer` — which is what makes
it test the map rather than the chain.

```cpp
if (ping.isActive())
    ping.overwrite (out, kNumSpeakers, start, count);   // out[i] is ALREADY speaker i's buffer
```

**No `reset()` is needed and none is added.** This preserves P23/P30's *one reset site, ever*:

- The smoothers are untouched. `GainStage`'s per-sample loop advances all 17 unconditionally
  (`GainStage.cpp:451-460`), so on ping stop the DBAP signal resumes from smoothers that never froze
  — there is nothing to teleport.
- §OQ2's **20 ms raised-cosine envelope** already owns the discontinuity at start and stop. This is
  2.3's H1 argument in a different place: an envelope that reaches zero makes a state reset
  unnecessary, and a reset would be the thing that *introduces* a click by discarding a partly-faded
  tail.

**Signal and level, per §OQ2:** member-owned `juce::Random` (**never** `getSystemRandom()` — §F9, and
`pattern_rng_stream_interleave_blocksize`), a fixed-coefficient pinking network, then two
`juce::dsp::FirstOrderTPTFilter` instances at **200 Hz HP** and **8 kHz LP**. Level: **−20 dBFS RMS**,
hard-clamped at **−6 dBFS** peak. **The RMS-normalising scalar is CALIBRATED, not guessed** — the
pink network's output RMS is a property of its coefficients, so execute measures it once and lands it
as a **named constant**, with probe **BR** asserting the resulting RMS. Stated at plan so execute
does not invent a number and so the probe is not written against the constant it is meant to check.

**Both clocks are counted in SAMPLES, not in `juce::Timer` ticks:** 1.2 s on / 0.4 s gap, and the
120 s latch. Three reasons and the third is decisive — sample counting is deterministic, it is
block-size invariant (chunks tile the block exactly), and **it is the only version probes BS and BT
can measure offline**. 12.8 s at 48 kHz is 614 400 samples; a message-thread timer is unmeasurable in
a render harness.

**D11's four stops, and one precondition:**

| Stop | Mechanism |
|---|---|
| Editor close | `~OctagonEditor()` gains a body calling `processorRef.stopVerifyPing()` — it is currently `= default` |
| **Bypass** | `processBlockBypassed()` is overridden to stop the ping, then do the default passthrough. A bypassed plugin still emitting noise is a genuinely confusing thing to debug on a stage: the first instinct is to bypass, and if that does not silence it the diagnosis goes somewhere wrong |
| 120 s latch | audio thread, sample-counted → **probe BT** |
| Explicit Stop | `stopPing` native fn |
| *(not a stop)* | **A tab switch.** An operator walking the hall needs the ping alive while they look at the Room plan — D11's asymmetry, deliberate |
| **Precondition** | **The ping REFUSES to start when `mappedOutputAvailable()` is false** (Q5). Pinging "speaker 5" on a stereo fold names a speaker that does not exist during the one procedure whose purpose is confirming speaker N is speaker N — **R1 reproduced inside its own diagnostic tool.** The UI says why. → **probe BU** |
| **Fifth stop, same reason** | If `mapped` goes false **while** pinging (the F3 flip, which can happen between blocks with no intervening `prepareToPlay`), the audio thread stops the ping. Same argument as the precondition, arriving from the other direction |

**The ping is not part of the bit-identity contract** (§F9) — it is a UI action, never automated, and
every QUAL-03 / bit-identity probe runs with it off. The member-owned `Random` is free and removes
the question.

---

### P61 — Ping indicator: a **100 ms poll while pinging only**. No push path at 3.2 *(Q3, D14)*

The sampling argument is arithmetic, not taste. §OQ2 fixes the auto-cycle at 1.2 s on / 0.4 s gap, so
the shortest state the indicator must resolve is **400 ms**. `STATUS_POLL_MS` is **500**
(`app.js:120`) — **longer than the gap** — so the existing poll can miss a gap entirely and can lag a
speaker change by half a period. A ping-only poll at **100 ms** resolves it with margin and costs
nothing when the ping is off, which is almost always.

**Push is rejected on three counts, and the third is disqualifying at this boundary:**

1. It would be the **first non-pull surface in this UI**, and 3.1 chose pull deliberately.
2. **The ui-stub does not model `backend.addEventListener`** (`juce-stub.js:33` records the absence
   as a design property). D4 made the stub mandatory, so a push path must be modelled there before
   anything can be rendered stub-first.
3. **N4 applies to `emitEvent` harder than to completions** — it *is* the same
   `emitEventIfBrowserIsVisible` call. A pushed indicator update during a hidden interval is dropped
   with no retry; **a poll self-heals on its next tick.** For an indicator whose entire job is to be
   trustworthy during a speaker-identification procedure, a transport that can silently skip a step
   is the wrong transport — D14's own argument applied to the mechanism rather than to the
   derivation.

**D14 stands: C++ is the authority.** The audio thread owns the cycle and publishes
`{ active, mode, speaker, elapsedMs, remainingMs }` through atomics; `getPingState` reads them on the
message thread. **JS never re-derives the step** — a drifted `setInterval` would name speaker 5 while
6 sounds. **§26** asserts statically that no JS timer computes a speaker index.

---

### P62 — A plan box is fitted to the **smaller of its two bounds**, and the rail gets **its own** overflow assertion *(Q11 — measured)*

Measured in Chromium at 1100 × 720, DPR 1, against the **real `styles.css`**:

| Measurement | Value |
|---|---|
| Content area after the frame and `.screen` padding | **1068 × 592** |
| The 8-row table (`#`, label, x, y, z, trim, class) | **752 × 277 px** — 8 rows at 32.5 px, **47 %** of available height |
| Rail | 300 px wide |
| Width-bound mini-plan at aspect 0.800 | 278 × 348 → rail stack **601 > 592** ✗ |
| **Height-bound mini-plan** | **270 × 337** → rail **592 == 592** ✓ |

**The 42-field table was never the constraint. 3.1's Q7 is answered; D7 and D9 both survive.**

**The mini-plan was, and it is D7's portrait consequence appearing a second time one layer down.**
The Room plan is height-bound at 448 × 560 for exactly this reason. So the rule is stated **once** and
exported **once**: `roomplan.js` gains a `fitBox(rectW, rectH, aspect)` that returns the box fitted to
the smaller bound, and `relayout()` is refactored to call it. Both plans use one implementation.

> ⚠️ **`document.scrollHeight` was 720 the whole time.** `ui_layout_check.js` §8 measures the
> *document*, so as written **it would have passed a 9 px rail overflow.** The rail needs its own
> assertion: **`railScrollHeight <= railClientHeight`**, which is §11. **NC3** width-binds the
> mini-plan and demonstrates §11 firing *while §8 still passes* — the exact shape of the finding.

**Two incidentals from the same render, recorded so execute does not rediscover them:** the
venue-name field truncates at 220 px (`Default (placeholder — NOT meas`) and simply needs to be
wider — the left column has ~300 px spare on that row; and the ~240 px of vertical slack under the
table is **not** a better home for the mini-plan, because height-bound there it would be ~240 px
wide, **narrower than the rail's 270**. The measurement confirms D9 rather than overturning it.

---

### P63 — Venue layout: **table-left / rail-right** *(D9)*

```
┌─ 1100 ──────────────────────────────────────────────────────────┐
│ header 56   O-OCTAGON · [ROOM] [VENUE]   SAFE │ MAP INVALID     │  ← D13: frame-level,
├──────────────────────────────────────────┬──────────────────────┤     BOTH screens
│  venue-main  (flex 1)                    │  venue-rail  300     │
│  venue name  ────────────────────────    │  SET: 7.1 Surround   │
│  # LABEL   X     Y     Z    TRIM  CLASS  │  ┌────────────────┐  │
│  1 [L  ] [0.50][4.50][3.20][+0.0] VERTEX │  │  mini-plan     │  │
│  …                              752×277  │  │  270 × 337     │  │
│  8 [Rrs] [   ][    ][    ][    ] ON_EDGE │  └────────────────┘  │
│  RAKE  front [0.00]  rear [0.00]         │  SAVE   LOAD         │
│                                          │  PRESET ▾ SAVE LOAD  │
│                                          │  ── PING ──────────  │
│                                          │  ▶1..8  AUTO  STOP   │
├──────────────────────────────────────────┴──────────────────────┤
│ footer 40   metres readout (mono, tabular)                      │
└─────────────────────────────────────────────────────────────────┘
```

`font-variant-numeric: tabular-nums` on every venue field and readout is a **gate, not taste**
(§18, already live) — UI-01 column-aligns 42 values here, and a mis-scanned metre is a measurement
error that propagates silently into the solve. **The row arithmetic above is not the gate**; §11 and
§12 measure rendered boxes (`pattern_flex1_container_slack_invisible_to_row_sum`).

**D13's `mapInvalid` banner is frame-level, beside the SAFE banner**, and therefore visible on the
Room screen too. R1's defining property is silence; putting its only warning behind a tab the user is
not looking at reproduces the failure it exists to break. `getStatus` already returns `mapInvalid`
(`PluginEditor.cpp:317`) — this is a rendering decision plus P54's reason, not new plumbing.

---

### P64 — **No UI state depends solely on a native-function promise resolving** *(N4)*

`emitCompletionEvent` does `jassert (owner.isVisible())` and then calls
`emitEventIfBrowserIsVisible`, which **drops the event** when the component is hidden
(`juce_WebBrowserComponent.cpp:336-344, 607-611`). In Release: no error, no rejection, no log — the
`await` simply never returns. And because `OctagonEditor` passes
`.withKeepPageLoadedWhenBrowserIsHidden()` (`PluginEditor.cpp:159-162`), **the page survives being
hidden**, so the promise is abandoned inside a page that is still running and will still be running
when the component is shown again. An invocation naming an unregistered function has the same shape
(`:306-312`: `jassertfalse; return;` — no completion).

**Rule for every write path 3.2 adds:** the promise may be used to render an *advisory* result
(a rejection reason, a chosen filename), but **the authoritative UI state converges on the
`venueGen` poll** that already exists. Concretely: `setVenue`, `loadVenue` and the preset load all
cause a venue or parameter change the existing 2 Hz `getStatus` poll observes, and the table
re-renders from `getVenueGeometry`. The chooser's own promise may be left pending — it is not the
thing the UI depends on.

**This is additive to the SafePointer rule, not a replacement.** Constraint 7 holds exactly as
written: `return` **bare** on a dead pointer, never `complete(false)`, which is itself a
use-after-free. N4 says that even on a *live* pointer the completion may not arrive. **§25** asserts
the convergence; **§29** asserts the SafePointer form.

Recorded so it is not read as new: `pollStatus()` (`app.js:355-359`) already leaks a pending promise
on every dropped completion and is harmless only because `setInterval` fires the next one regardless
(`:394`). That is why 3.1 never saw this.

---

### P65 — The native surface is exactly **THIRTEEN**, and the §3 count literal **fails loudly until it moves**

| # | Function | Direction | Notes |
|---|---|---|---|
| 1 | `getParameterDefaults` | read | unchanged |
| 2 | `getVenueGeometry` | read | **+8 trims, +2 rake heights** (P55). Still one call |
| 3 | `getStatus` | read | **+`mapInvalidReason`, +`mapInvalidSpeaker`** (P54) |
| 4 | `setVenue` | **write** | D8 — all 42 values, one call. **Validates the label set before applying** (P52) and returns `{ ok, reason, speaker }` |
| 5 | `saveVenue` | write | `FileChooser::launchAsync` → `venuefile::save` |
| 6 | `loadVenue` | write | `FileChooser::launchAsync` → `venuefile::load` into a **fresh** model → `applyVenueEditChecked` |
| 7–10 | `savePreset` · `loadPreset` · `getPresetList` · `getCurrentPreset` | write/read | the four the rail needs, **not the module's ten** (P58). The load site brackets 17 gestures (P59) |
| 11 | `startPing` | write | speaker index or auto. **Refuses when `mappedOutputAvailable()` is false** |
| 12 | `stopPing` | write | D11's explicit Stop |
| 13 | `getPingState` | read | D14's authority. Polled at **100 ms while pinging only** |

**`ui_frontend_check.js:178`'s `registered.size === 3` must become `=== 13`, and it will FAIL until
it does — which is correct.** A count that silently tracked whatever was registered would assert
nothing. Every one of the thirteen must appear in three places or §3 fails: the C++ registration, a
JS call site **in a file the derived registry covers** (P51), and the ui-stub whitelist.

The stub whitelist is safe by construction — §3 asserts `stubbed == registered` as a **set**, so a
missing entry fails rather than passes. Worth stating, because the **stub's** unregistered-name
behaviour is *stricter* than the plugin's: the stub returns
`Promise.reject(new Error("Unregistered native function: …"))` (`juce-stub.js:281-283`) where the
plugin hangs silently. That asymmetry is safe **only** because §3 catches the gap statically.

---

### P66 — Probe accounting: **BN–BZ**, thirteen new → **78**; JS gates **30 → 49 sections**

The C++ letter series runs `A … BM` with no gaps — **65 probes** (33 unit + 32 harness), verified at
3.1. 3.2 adds thirteen, continuing at **BN**.

**Unit target (three)** — no processor, and Gate 11's narrow link line survives:

| Probe | Asserts | Serves |
|---|---|---|
| **BN** | `venuefile::save` → `venuefile::load` into a fresh model, **bit-compare all 42 values** | FUNC-02/2, **UI-01/3(a)** |
| **BO** | forward `@schemaVersion` returns `forwardVersion` and still loads; a non-`VENUE` root and a <8-speaker file return `malformedRoot` and **leave `out` at its defaults** | Q6 / P56 |
| **BV** | the three `MapFailure` reasons are distinguished and carry the right speaker index | N7 / P54 |

**Render harness (ten)** — need a live processor:

| Probe | Asserts | Serves |
|---|---|---|
| **BP** | `applyVenueEditChecked` with a duplicate label **returns false, names the row, and leaves the venue untouched**; `mapInvalid` stays false because nothing was applied | **N8 / P52**, ROADMAP orphan 4 |
| **BQ** | ping on a **non-identity** map: energy in exactly one buffer lane, **exact zero** in the other seven, for all 8 targets. Then change a label row and re-ping: the sounding lane **followed** | FUNC-04/1, ROADMAP orphan 5 |
| **BR** | ping RMS ≈ **−20 dBFS** and peak ≤ **−6 dBFS** at `outputGain = +12 dB` **and** `trim = +6 dB` | FUNC-04/3, ROADMAP orphan 2 |
| **BS** | auto-cycle order 1→8, each 1.2 s on / 0.4 s gap, **12.8 s total measured in samples**; manual step advances 1→8 | FUNC-04/2, ROADMAP orphan 3 |
| **BT** | a latched ping **self-stops at 120 s**, sample-counted | ROADMAP orphan 1 |
| **BU** | `startPing` **refuses** when `mappedOutputAvailable()` is false, and a `mapped` → not-`mapped` flip **stops** a running ping | Q5 / P60 |
| **BW** | a preset load leaves all 42 venue values **bit-identical** | FUNC-05/1 |
| **BX** | a preset saved under venue A recalls under venue B: `srcX/srcY` unchanged **normalised**, resolved metres follow venue B's bbox | FUNC-05/2 |
| **BY** | `getStateInformation` / `setStateInformation` round-trip **both stores** — 42 venue values and 17 parameters | FUNC-05/3 |
| **BZ** | `applyVenueEditChecked` with one coordinate moved **changes the solved gain vector** | FUNC-02/3 |

**Total after 3.2: 78 C++ probes (36 unit + 42 harness), 0 failures required. None of A–BM may
regress.**

The JS gates stay a **separate family counted as sections** — mixing them into the letter series
would make "78 probes, 0 failures" mean two different things in two documents:

| File | 3.1 | 3.2 | New |
|---|---|---|---|
| `ui_frontend_check.js` (static) | 20 | **31** | §21–§31 |
| `ui_layout_check.js` (Playwright) | 10 | **18** | §11–§18 |
| | 30 | **49** | 19 new sections |

---

### P67 — **Six negative controls, declared at plan**, and each names the gate it must make fire

Not left for execute to invent. Every one targets a hazard this phase introduced or a gate this phase
widened.

| # | Mutation | Must fire | Guards |
|---|---|---|---|
| **NC1** | put a second `(v - min) / span` projection in `venue.js` | §19 | **RESEARCH §4** — the enumeration hole, half one |
| **NC2** | call `nativeFn("getPingStat")` (unregistered) from `venue.js` | §3 | **RESEARCH §4** — half two |
| **NC3** | width-bind the mini-plan | **§11 fires while §8 still PASSES** | **Q11** — the document check would not have caught it |
| **NC4** | make the label column revert like the numeric ones | §15 | **N8 / P53** — L↔R becomes unreachable |
| **NC5** | drop the 17 gesture brackets around `loadPreset` | §28 | **N5 / P59** |
| **NC6** | remove `applyVenueEditChecked`'s guard | **probe BP** | **N8 / P52** — the audible fold |

**NC3's pass/fail asymmetry is the point** and must be recorded as such in `SUMMARY-3.2.md`: a
negative control that makes the *new* section fire while the *old* section stays green is the
strongest available evidence that the new section was not redundant.

After every control, the tree is proved **byte-identical** (`shasum -a 256` over the touched files)
and both gates re-run to exit 0 — the 3.1 discipline, unchanged.

---

### P68 — `REQUIREMENTS.md` is not edited; the seven ROADMAP orphans are carried as **named gates**

3.1 edited `REQUIREMENTS.md` because a criteria debt was **dated to that exact boundary**. None is
dated here — all four of 3.2's rows have carried three criteria each since Stage 0, and editing
criteria for a `must` row without a debt would be rewriting a target at the moment of aiming at it.

The seven orphans are **not** a contract gap: `ROADMAP.md` and `REQUIREMENTS.md` are both complete
documents that enumerate at different granularities. They are a **traceability** gap, and the
disposition is the one 3.1's plan used for UI-02's two orphans — carry each as a named gate, tabled
above, and record the mapping in `SUMMARY-3.2.md`.

---

## Tasks

**Ordering is load-bearing twice over:**

1. **Task 1 comes before `venue.js` exists.** The derived registry (P51) must be in place before the
   file it needs to cover, or Task 5 has to remember — which is precisely what failed the previous
   five times.
2. **Tasks 2–8 complete, and Task 8 PASSES, before Task 9 writes a line of C++.** UI-02 criterion 6's
   stub-first ordering is not a 3.1-only discipline; D4 removed the browser-iteration safety net for
   the whole stage, and a top-level TDZ throw in `venue.js` would otherwise first surface inside a
   plugin (`pattern_module_toplevel_init_tdz`).

---

### Task 1 — `ui_frontend_check.js`: the **derived page-module registry** *(P51)*

**Modify** `tests/ui_frontend_check.js`, **before anything else in this phase**.

- Add `PAGE_MODULES`, read from `Source/ui/public/js/*.js`, excluding `js/juce/`.
- Rewire **§1** (parses as an ES module), **§3** (native-fn call sites), **§6** (`textContent`
  receivers), **§12** (gesture pairing), **§14** (no native call in a pointer handler) and **§19**
  (one projection) to iterate it.
- Add **§21**: `PAGE_MODULES.length >= 3`; every module appears in `juce_add_binary_data` SOURCES;
  the derived set **equals** the CMake set.

**Runs green on the unmodified tree** (two modules today, both already covered by name), which is the
point: the change is invisible until Task 5 adds the third, and then it is automatic.

---

### Task 2 — `tests/ui-stub/juce-stub.js`: the 13-function surface *(P65)*

**Modify.** The stub is D4's mandatory mitigation and every new page module is rendered against it
first.

- **Ten new entries** in `NATIVE_FNS`, exactly the names of P65's table. An unknown name still
  **rejects**.
- `setVenue(payload)` — accepts all 42 values, **validates the label set as a permutation of the
  negotiated set**, returns `{ ok, reason, speaker }`, and on success mutates the stub venue and
  advances `venueGen`. The stub must be able to *reject*, or §15 has nothing to drive.
- `getVenueGeometry` gains `trimDb` per speaker and `rake` (P55). `getStatus` gains
  `mapInvalidReason` / `mapInvalidSpeaker` (P54).
- A **ping state machine**: `startPing(n | "auto")` / `stopPing` / `getPingState`, with the stub
  owning the cycle so `__OCTAGON_STUB__.stepPing()` can advance it deterministically. Playwright must
  never race a wall clock.
- A **preset store**: a name list, a current name, save/load that records what was written.
- `__OCTAGON_STUB__` gains `setMapInvalid(reason, speaker)`, `stepPing()` and `presetWrites`.

**The stub does NOT model `backend.addEventListener`** — P61 keeps the design pull-only, and the
absence stays a recorded design property (`juce-stub.js:33`).

---

### Task 3 — `index.html`: the Venue screen *(P63, D9, D12, D13)*

**Modify.** Replace the 3.1 placeholder with the D9 layout.

- `.venue-main`: the venue-name field (**wider than 220 px** — Q11's truncation), the 8-row
  `<table class="vtable">` with columns `#` · LABEL · X · Y · Z · TRIM · CLASS, and the two rake
  fields.
- **All 40 speaker inputs and both rake inputs are `type="text" inputmode="decimal"`** — never
  `type="number"` (**D12**: on invalid content `.value` is `""` and `.valueAsNumber` is `NaN`, so
  "typed `abc`" and "cleared the field" are indistinguishable and FUNC-02/1 becomes untestable).
  Field ids are `vf-<n>-<x|y|z|trim>`, `vf-label-<n>`, `vf-rake-front`, `vf-rake-rear`.
- `.venue-rail`: negotiated set name (orphan 6), the mini-plan container, SAVE / LOAD, the preset
  bar, the ping card.
- **The `mapInvalid` banner goes in the `<header>`, beside the SAFE banner** (D13) — frame-level, so
  it is visible on the Room screen too.
- **Every label is HTML-authored and JS never writes it.** Values go into dedicated nodes; the CLASS
  column is its own `<span class="cell-value">`
  (`pattern_js_state_updater_overwrites_html_labels`).

---

### Task 4 — `css/styles.css`: the Venue screen *(P62, P63)*

**Modify.** Same `:root` token block; no new tokens unless a value is genuinely new.

- `#screen-venue` flex row, `.venue-main` `flex: 1 1 auto; min-width: 0`, `.venue-rail`
  `flex: 0 0 300px`.
- `.vtable` at the **measured** type scale — 11 px mono fields, 9 px column heads, rows at 32.5 px.
- `.vfield` and `.vfield.is-invalid` / `.vfield.is-colliding` — the two marks P53 distinguishes.
- `.miniplan` sized from `--mini-w` / `--mini-h`, written by `venue.js` from `fitBox`. **Never a
  literal aspect** (the same rule UI-02/1 enforces on the Room plan).
- Every venue value class carries `font-variant-numeric: tabular-nums` and `var(--mono)` — §18 already
  gates the class list and the new classes join it.

---

### Task 5 — `Source/ui/public/js/venue.js` *(P52, P53, P55, P61, P62, P64)*

**Create.** Hyphen-free (`critical_binary_data_strips_hyphens`). Exports `createVenueScreen(deps)`;
**no top-level call of its own** — `app.js` initialises it inside `init()`, inside a `try`/`catch`,
exactly as `roomplan.js` is (§2).

- **The 42-field table**, rendered from the `getVenueGeometry` payload.
- **D8's commit model:** blur / Enter commits, Escape reverts. On commit, **one `setVenue` call
  carrying all 42 values.** No per-field write surface exists — that would be P38's torn read on the
  write side, 42 async round trips whose promises may resolve out of order against a model that
  recomputes bbox, centroid, `rigScale` and hull on every one.
- **D12's explicit parse:** `Number.parseFloat` on a trimmed string, rejecting `NaN`, empty and
  anything with trailing junk. Numeric fields **revert on invalid**; the **label column holds and
  marks** (P53), and while the label set is not a permutation the commit is **blocked** and
  `setVenue` is not called at all.
- **The mini-plan** through `roomplan.js`'s exported `metresToPx` / `normToMetres` / `fitBox` /
  `makeView` — **a second `view`, not a second projection** (Q8). Height-bound (P62).
- **The ping card:** Start/Stop/step/auto, and a **100 ms poll started only while pinging** (P61).
  The indicated speaker is `getPingState().speaker` and **nothing else** — no JS timer re-derives it
  (D14).
- **The preset bar** — four functions, O-Octagon's own DOM, no `createPresetBar` (P58).
- **P64 everywhere:** no UI state waits on a promise. Every write's authoritative effect arrives via
  the `venueGen` refresh.

---

### Task 6 — `Source/ui/public/js/roomplan.js`: export the fit rule *(P62, Q8)*

**Modify, minimally.** `metresToPx` is untouched — it is already parameterised by `view`, so a
mini-plan is a second `view` and §19 is **widened, not weakened** (P46 honoured rather than argued
around).

- Extract `relayout()`'s fit arithmetic into an exported
  `fitBox(rectW, rectH, aspect) -> { w, h }` — the smaller-bound rule, stated once.
- Export `makeView(envelope, w, h)`.
- `relayout()` calls both. **Behaviour is unchanged and the 3.1 gates prove it**: §2 must still
  measure 448.0 × 560.0.

---

### Task 7 — `Source/ui/public/js/app.js`: wire it in *(P54, P61, P64)*

**Modify.**

- Create the Venue screen inside `init()`, hoisted, in its own `try`/`catch`, so a Venue failure
  cannot take the Room screen or the 17 bindings down.
- Render the **`mapInvalid` banner with its reason and row** from `getStatus` (P54, D13) — the poll
  and the banner precedent already exist.
- Render the negotiated set name (orphan 6).
- `init()` stays the **last statement in the file** and no module-level declaration follows it (§2).

---

### Task 8 — `tests/ui_layout_check.js`: sections 11–18, **run against the stub, before any C++** *(P62, P57, UI-01)*

**Modify.** Same resolver, same 1100 × 720 viewport, same fail-not-skip rule (P49).

| § | Assertion | Serves |
|---|---|---|
| 11 | On the Venue screen: **`railScrollHeight <= railClientHeight`**, and `scrollWidth <= 1100` / `scrollHeight <= 720` | **P62 / Q11** — the rail assertion §8 does not make |
| 12 | All **42** fields present, editable, inside the viewport; the mini-plan's rendered aspect follows the **returned** envelope | UI-01/1 |
| 13 | Type `abc` → field marks and **reverts on blur**; type `7.25` → commits, one `setVenue` call, 42 values in the payload | **FUNC-02/1**, D8, D12 |
| 14 | **UI-02/5's inherited end-to-end gate**: edit a coordinate on the Venue screen, blur, switch to Room, assert the **envelope / metres readout MOVED** | **P45's end-to-end half**, declared at 3.1 |
| 15 | Set row 1 = `R` while row 2 = `R` → **both rows marked, commit BLOCKED, `setVenue` NOT called**; complete the swap → commit proceeds | **N8 / P53**, ROADMAP orphan 4 |
| 16 | `setMapInvalid("duplicateLabel", 2)` → banner appears **with the reason and the row**, on **both** screens; cleared → disappears | **D13**, ROADMAP orphan 4 |
| 17 | The negotiated set name is on screen; the CLASS column shows the **payload's** classification per speaker | ROADMAP orphans 6, 7 |
| 18 | Start → indicator follows the **returned** `getPingState().speaker` across a stub-driven step; Stop clears it; a refusal renders its reason | **UI-01/2**, D14, Q5 |

**Gate: this must pass before Task 9 begins.** Exit code = number of failed assertions; it **fails**
rather than skips when Playwright is unresolvable.

**Every assertion compares against what the stub RETURNED**, re-read at assertion time — never a
mirrored table of expected numbers (`pattern_test_fixture_mirrors_drift_silently`).

---

### Task 9 — `Source/Data/VenueFile.{h,cpp}` *(P56)*

**Create.** Namespace `oo::venuefile`, **no processor reference** — it joins the unit target and Gate
11's narrow link line survives (verified against `tests/unit/CMakeLists.txt:91-99` at plan).

- `save (const VenueModel&, const juce::File&)` — `toValueTree()` → `createXml()` → write.
- `load (const juce::File&, VenueModel& out, int* fileVersion)` — parse, **reject a non-`VENUE` root
  or fewer than 8 `SPEAKER` children without touching `out`**, report `forwardVersion` when
  `@schemaVersion > kSchemaVersion`, and otherwise read into the caller's **fresh** model.
- **Every string is pure ASCII or built with `<<`** — `juce::String(const char*)` converts through
  `CharPointer_ASCII` and mangles anything above 127 with no compiler warning. This cost a real
  defect at 3.1 (D-2, the mangled em-dash, silent since Stage 2).

---

### Task 10 — `Source/DSP/VerifyPing.{h,cpp}` *(P60)*

**Create.** Not in the unit target — it needs `juce_dsp`, and Gate 11 forbids that there.

- Member-owned `juce::Random` (§F9). Fixed-coefficient pinking network, then two
  `FirstOrderTPTFilter` at 200 Hz HP / 8 kHz LP.
- **RMS scalar calibrated by measurement at execute**, landed as a **named constant** with the
  measured figure in the comment. Probe BR asserts the result.
- 20 ms raised-cosine in and out. **No `reset()` anywhere** — P23/P30's one-reset-site-ever holds.
- Both clocks in **samples**: 1.2 s on / 0.4 s gap / 120 s latch, all from the prepared sample rate.
- Commands in and state out through **atomics only**; no `juce::String` crosses a thread (P43).
- `overwrite (float* const* out, int numSpeakers, int start, int count)` — writes the target lane and
  **hard-zeroes the other seven**.

---

### Task 11 — `ChannelMap` + `PluginProcessor` deltas *(P52, P54, P60)*

**Modify** `Source/DSP/ChannelMap.{h,cpp}` — add `MapFailure` / `MapDiagnosis` and the **defaulted**
out-param, so every existing call site compiles unchanged. `isPermutationOf0to7` is **not** touched.

**Modify** `Source/PluginProcessor.{h,cpp}`:

1. `applyVenueEditChecked (const VenueModel&, ochan::MapDiagnosis*)` — **P52's guard**, scratch-array
   validation through `buildSpeakerToBuffer` itself.
2. `rebuildChannelMap()` captures the diagnosis into a plain member; `lastMapDiagnosis()` accessor.
   **`mapInvalid` stays the atomic it is.**
3. `VerifyPing` member; `startVerifyPing (int speakerOrAuto)` / `stopVerifyPing()` /
   `verifyPingState()`. `startVerifyPing` **returns false and a reason when
   `mappedOutputAvailable()` is false**.
4. `processBlockBypassed()` overridden — stop the ping, then the default passthrough (D11).
5. `prepare()` the ping in `prepareToPlay()`, beside the existing initialisation.

**Modify** `Source/DSP/GainStage.{h,cpp}` — the **post-write overwrite** at the end of the REAL arm,
after the NaN guard, indexed through the same `out[]` pointers. The ping is passed in by the
processor; `GainStage` does not own it and does not decide when it runs (P24's rule: this class does
not ask the processor anything).

**No new APVTS parameter** (constraint 8). **No `AsyncUpdater`** (§2.1). **No `SCENES` handling** —
3.3.

---

### Task 12 — `Source/PluginEditor.{h,cpp}`: the ten new native functions *(P52, P55, P57, P58, P59, P65)*

**Modify.**

- The three existing functions gain their new payload fields (P54, P55). `getVenueGeometry` stays
  **one** call.
- `setVenue` — parse 42 values out of the payload, build a `VenueModel`, call
  **`applyVenueEditChecked`** and return `{ ok, reason, speaker }`. **It must not call
  `applyVenueEdit` directly** — §22 asserts the absence.
- `saveVenue` / `loadVenue` — `FileChooser::launchAsync`. **The `SafePointer` is hoisted to a local
  before the completion lambda** (`critical_msvc_safepointer_init_capture_nested_lambda`: MSVC
  resolves `this` to the closure in a nested-lambda init-capture), and **on a dead pointer the
  completion `return`s bare, never `complete(false)`** — which is itself a use-after-free
  (constraint 7, §4.4).
- One `OuariconPresetManager` member; **four** native functions; **`setCustomStateCallbacks` is never
  called** (P58). The `loadPreset` site brackets **all 17** parameters (P59).
- `startPing` / `stopPing` / `getPingState`.
- `~OctagonEditor()` gains a body: `processorRef.stopVerifyPing()` (D11).
- The header comment's native-fn count moves from **THREE** to **THIRTEEN**, with the reason the
  count is load-bearing restated.
- **MSVC and ASCII habits apply to every line added** (constraints 6, 10).

---

### Task 13 — `CMakeLists.txt` and the two test targets

**Modify** `plugins/O-Octagon/CMakeLists.txt`:

- `target_sources(OuariconOctagon)` += `Source/Data/VenueFile.cpp`, `Source/DSP/VerifyPing.cpp`.
- `juce_add_binary_data` SOURCES += `Source/ui/public/js/venue.js` — **hyphen-free**, and §21 now
  requires it.
- `target_include_directories` += the preset-manager module's `cpp/` path.

**Modify** `tests/unit/CMakeLists.txt` — `+= Source/Data/VenueFile.cpp`. **The link line does not
change**, and Gate 11 re-verifies that: still no `juce_dsp`, still no `juce_gui_extra`.

**Modify** `tests/render-harness/CMakeLists.txt` — `+= Source/Data/VenueFile.cpp`,
`Source/DSP/VerifyPing.cpp`. **`PluginEditor.cpp` never enters it** — §11 asserts the absence, and
32 probes die silently otherwise.

---

### Task 14 — `ui_frontend_check.js`: sections 22–31 *(P52, P53, P57, P58, P59, P60, P61, P64)*

**Modify.** Exit code = number of failed assertions.

| § | Assertion | Decision / pattern |
|---|---|---|
| 22 | **One** `setVenue` call site carrying all 42; **no per-field write surface**; the 42 field ids close four ways (HTML == `venue.js` table == payload keys == `VenueModel`'s setters); **`PluginEditor.cpp` contains no `applyVenueEdit (` call** | **D8 / P52** |
| 23 | Every venue input is `type="text"` + `inputmode="decimal"`; **no `type="number"` on the Venue screen**; the parse is explicit | **D12** |
| 24 | The label column holds-and-marks, the numeric columns revert — asserted in source form | **N8 / P53** |
| 25 | No UI state depends solely on a promise: every native write's UI effect is also reachable from the `venueGen` refresh path | **N4 / P64** |
| 26 | The ping indicator reads `getPingState`; **no `setInterval` / `setTimeout` computes a speaker index** | **D14 / P61** |
| 27 | `setCustomStateCallbacks` appears **NOWHERE** in O-Octagon's source; `preset-manager.js` is **not vendored** | **FUNC-05 / D10 / N6** |
| 28 | The `loadPreset` call site brackets **all 17** with `beginChangeGesture` / `endChangeGesture` | **N5 / P59** |
| 29 | Chooser completions: **hoisted `SafePointer` local**, bare `return` on death, **never `complete(false)`**; they call exactly `venuefile::save` / `venuefile::load`; **no parallel serialisation path** outside `VenueFile.cpp` | constraint 7, **UI-01/3(b)** |
| 30 | The ping override sits **after the write, before metering**; **no new `reset()`** on any smoother or filter outside `prepare()` | **§7.2 / P23 / P30** |
| 31 | MSVC + ASCII habits extended to `VenueFile.cpp`, `VerifyPing.cpp` and the new `PluginEditor.cpp` lines | constraints 6, 10 |

**§3's count literal moves 3 → 13** (P65) and §9's embedded-file count 6 → 7.

---

### Task 15 — Probes **BN–BZ** *(P66)*

**Modify** `tests/unit/main.cpp` (BN, BO, BV) and `tests/render-harness/main.cpp`
(BP, BQ, BR, BS, BT, BU, BW, BX, BY, BZ), continuing the letter series.

Every probe **prints its measured values**, so a future change reads as a diff rather than a bare
FAIL — the 2.1 discipline, unchanged. **Probe count after 3.2: 78** (36 unit + 42 harness).

Three that need their non-vacuity stated in the probe itself:

- **BQ** must use a **non-identity** map, or a hardcoded `out[i]` passes (`FUNC-03`'s C1 argument,
  and the reason probe D was built that way at 2.1).
- **BR** must drive `outputGain = +12 dB` **and** `trim = +6 dB` **simultaneously**; either alone
  leaves the other multiply untested.
- **BS / BT** must assert against **sample counts derived from the prepared sample rate**, not
  against a transcribed 614 400 (`pattern_test_fixture_mirrors_drift_silently`).

---

### Task 16 — Re-run `ui_layout_check.js` against the integrated page

Same file, unchanged. It ran pre-integration against the stub; re-run it now that `getResource`
serves the same tree, to prove Task 13's resource list did not drop `venue.js`. §21 catches a missing
SOURCES entry statically; this catches a `getResource` path that does not match what the HTML asks
for.

---

### Task 17 — The six negative controls *(P67)*

NC1–NC6 per the table. Each is applied, the named gate is confirmed to **fire**, the mutation is
reverted, and the tree is proved **byte-identical** afterwards by `shasum -a 256`. Both gate files
must then return exit 0 on the restored tree.

**NC3's result is recorded with both halves**: §11 fires, **§8 passes**. That asymmetry is the
evidence that §11 was not redundant, and it is the finding Q11 measured.

---

### Task 18 — `REQUIREMENTS.md`: tick the four rows with named evidence

**Modify only the four rows' criteria lines and their evidence arrows.** No criterion is reworded
(P68).

**Each evidence line goes under its own criterion**, and the section is re-checked programmatically
afterwards — `[x]` count == `→ **` count, per section. 3.1 verify found UI-02 criterion 7's evidence
orphaned past the `### UI-03` heading, where it read as the lead line of a *pending* row
(`pattern_evidence_line_orphaned_past_next_heading`). That check is run here rather than being
rediscovered at verify.

---

### Task 19 — Gates

Every gate is **run at execute and RE-RUN FROM SCRATCH at verify**, never read out of
`SUMMARY-3.2.md`. This is the 2.3 discipline that caught four mis-attributions and the 3.1 discipline
that caught three more.

| # | Gate | Pass condition |
|---|---|---|
| 1 | Clean 3-format build + both test targets, **forced full recompile** | exit 0, **zero `warning:` / `error:` / `FAILED`** |
| 2 | `node tests/ui_frontend_check.js` | exit 0, **31 sections** |
| 3 | `node tests/ui_layout_check.js` | exit 0, **18 sections** — and it must **not SKIP** (P49) |
| 4 | Stub render (Task 8) ran **before** Task 9's C++ | recorded in SUMMARY with its timestamp |
| 5 | `auval -v aufx OuOc OuDv` | **AU VALIDATION SUCCEEDED** |
| 6 | pluginval s10, VST3 ×3 / AU ×3 | all six exit 0, zero `FAILED` |
| 7 | Both C++ test targets | **78 probes, 0 failures**, exit 0 / exit 0 |
| 8 | `gen_dbap_reference.py --check` | exit 0, 102 cases |
| 9 | 17 params vs `parameter-spec.md`, three sides | **17/17**, none hand-transcribed; **3.2 adds none** |
| 10 | `createEditor` guard present; **`PluginEditor.cpp` absent from the harness target** | both ✓ |
| 11 | Unit-target link line has **no `juce_dsp` and no `juce_gui_extra`** — re-checked because `VenueFile.cpp` joined it | ✓ |
| 12 | Contract checksums | all four byte-exact, **no pin moved** |
| 13 | **Standalone launch, macOS — HUMAN, ~8 min** | the Venue screen renders at 1100 × 720; typing a coordinate redraws the mini-plan; **SAVE opens a native modal and LOAD reads the file back** (UI-01/3c); the ping sounds and the indicator follows | 
| 14 | The six negative controls | **all six fired**; tree byte-identical afterwards |
| 15 | `node tests/tools/venue_layout_study.js` | **re-run**, and its numbers still agree with §11/§12 |

**Gate 13 is still not D5.** D5 is the ~15 min Logic session for QUAL-01's audible clause, folded to
Stage 4 by D2 and untouched here. Gate 13 grew from ~5 min to ~8 min because **UI-01/3(c) is now
part of it** — a native modal is the one thing no automated gate in this repo can open.

---

### Task 20 — `SUMMARY-3.2.md` + `STATUS.md`

`SUMMARY-3.2.md` records: the 18 plan decisions and how each landed, every deviation, all 15 gate
results with measured values, the criterion→evidence mapping **as run** for all four rows, the seven
orphans' dispositions, and the six negative controls with **NC3's asymmetry stated explicitly**.

**Three things must reach SUMMARY and STATUS frontmatter, or 3.3 rediscovers them:**

1. **N8's inheritance:** FUNC-06's scene work must not assume an invalid map merely "retains" — **the
   SAFE fold is what plays.** Any 3.3 probe asserting retention must assert it against the
   **snapshot**, not against the output buffer.
2. **The 3.3 half of the gesture obligation is still open** — scenes must bracket each of `w1..w8`
   (`gesture_bracket_obligation: CLOSED_3_1_PUCK__OPEN_3_3_SCENES`). P59 closed the preset half.
3. **FUNC-05's assertion changes shape at 3.3.** At 3.2 it is *"no custom-state callback exists at
   all"*; at 3.3 it becomes *"exactly one exists and its body touches only `SCENES`"*. That is why
   FUNC-06/5 re-runs FUNC-05's bit-compare rather than inheriting it (Q2).

STATUS frontmatter updates: `stage_phase: "3.3"`, `probe_count: 78`, `js_gate_sections: 49`,
`stage_3_native_fn_surface_3_2: 13`, `negative_controls: 19`, `ui_02_5_e2e_gate: CLOSED_3_2`,
`roadmap_orphans_3_2: CLOSED`, `stage_3_repins_scheduled: "3.3-discuss"` (unchanged).

---

## Execution Constraints

1. **`createEditor` stays guarded with `#if JUCE_WEB_BROWSER`** — 42 harness probes die silently
   otherwise (`pattern_render_harness_breaks_on_webview_editor`).
2. **Member order relays → webView → attachments** — destruction order, not style. The preset manager
   and the `VerifyPing` reference join in an order that does not disturb it.
3. **The resource provider receives bare PATHS** — never hard-code `juce://`
   (`critical_webview_resource_provider_and_schemes`).
4. **`juce_add_binary_data` STRIPS hyphens** — `venue.js`, never `venue-table.js`
   (`critical_binary_data_strips_hyphens`).
5. **Pass the `Juce` ES-module namespace**, not `window.__JUCE__`
   (`critical_juce_webview_namespace_vs_postmessage`).
6. **`juce::String(const char*)` is ASCII-only** — build with `<<` or `+`. No compiler warning; the
   damage is visible only in rendered output. **3.2 writes far more user-facing C++ text than 3.1
   did** (three `MapFailure` reasons, the forward-version warning, the ping refusal), and D-2 was a
   real defect (`critical_juce_string_char_ctor_is_ascii_only`).
7. **`FileChooser::launchAsync` completions capture a `SafePointer` and `return` BARE on a dead
   pointer — never `complete(false)`**, which is itself a use-after-free
   (`pattern_webview_launchasync_safepointer_no_complete`, §4.4). **And N4: even on a live pointer
   the completion may be dropped** (P64).
8. **The 17 parameters are frozen.** `parameter-spec.md` is pinned; 3.2 adds none. Venue values, the
   ping and the preset store are **not** parameters (§6.3).
9. **No file I/O of any kind in `processBlock`** (PERF-01, §4.4). Venue save/load and preset load are
   message-thread and user-initiated, always.
10. **MSVC hazards are authored now, not fixed at port time** — C3493 (non-static `constexpr` in a
    lambda) and `SafePointer(this)` init-capture in nested lambdas. **3.2 is the phase that finally
    has call sites for the second one.**
11. **Member-owned `juce::Random`, never `getSystemRandom()`** (§F9,
    `pattern_rng_stream_interleave_blocksize`).
12. **The ping is injected AT the channel map, after everything else, all other channels
    hard-zeroed** (§OQ2). Bypassing DBAP, weights, hull, trim and `outputGain` is the entire point —
    a ping failure then has exactly one possible cause.
13. **The default venue stays labelled unmistakably as a placeholder** (§R8) — and 3.2 is the phase
    where a user replaces it.
14. **`tests/render-harness/CMakeLists.txt` never receives `PluginEditor.cpp`.**
15. **Do not edit `modules/persistence/preset-manager/`.** Four other plugins depend on it.

---

## Non-goals for Phase 3.2 — must not appear

Anything here in a 3.2 diff is a **deviation to be recorded**, not a bonus.

- `meterPeak[8]`, meters, ballistics, peak-hold — **3.3**
- Named scenes, the `SIDES` predicate, the 4 user slots, the `SCENES` tree node — **3.3**
- **Any** `setCustomStateCallbacks` registration — **3.3**, and §27 fails on it here
- Level-gradient backdrop, side-elevation strip — **3.3**
- The three §8 contract re-pins — **3.3 discuss**
- UI-04 / UI-05's descope decision — **3.3 discuss**
- Aesthetic-template extraction — **3.3 verify**
- A push (`emitEvent` / `addEventListener`) transport — **rejected at 3.2** (P61); revisit only with
  a stub that models it
- Any `AsyncUpdater` anywhere — **never**, per §2.1
- Any new APVTS parameter — **never**, per constraint 8
- Any edit to `OuariconPresetManager.h` — **never at this boundary** (P59)
- A second venue-apply path — **never** (P52)
- CI wiring for these gates — **Stage 4**

---

## Success Criteria

**Phase 3.2 is complete when all of the following hold, each measured rather than asserted:**

- [ ] All four contract checksums byte-exact; **no pin moved**
- [ ] Clean 3-format build + both test targets on a **forced full recompile**, zero compiler
      diagnostics
- [ ] `ui_layout_check.js` ran **against the stub before any 3.2 C++ existed**, with its timestamp
      recorded *(the 3.1 ordering discipline, carried)*
- [ ] `ui_frontend_check.js`: **31 sections**, exit 0 — including the **derived page-module
      registry** *(P51)*, **bridge surface exactly 13 both directions** *(P65)*, **no
      `applyVenueEdit` call in the editor** *(P52)*, **`setCustomStateCallbacks` nowhere** *(FUNC-05)*,
      **17 gesture brackets around the preset load** *(N5)*
- [ ] `ui_layout_check.js` re-run post-integration: **18 sections**, exit 0, **did not skip**
- [ ] **`railScrollHeight <= railClientHeight`** on the Venue screen, and document scroll ≤ 1100 × 720
      on **both** screens *(P62)*
- [ ] All **42** fields present, editable and legible at 1100 × 720; the mini-plan's aspect follows
      the **returned** envelope *(UI-01/1)*
- [ ] Typing `abc` marks and **reverts**; typing a metre commits **one** `setVenue` carrying 42
      values *(FUNC-02/1, D8, D12)*
- [ ] A duplicate label **marks both rows, blocks the commit, and `setVenue` is never called**;
      completing the swap commits *(N8 / P53)*
- [ ] Editing a coordinate on the Venue screen moves the **Room** readout *(UI-02/5's inherited
      end-to-end gate — P45, closed here)*
- [ ] The `mapInvalid` banner shows **reason and row**, on **both** screens *(D13 / P54)*
- [ ] The negotiated set name and the per-speaker hull classification are on screen *(orphans 6, 7)*
- [ ] The ping indicator follows the **returned** `getPingState().speaker`; **no JS timer re-derives
      the step** *(UI-01/2, D14)*
- [ ] **78 probes, 0 failures** (36 unit + 42 harness). **None of A–BM regressed**
- [ ] `.venue` round-trips **all 42 values bit-identically** through the same functions the chooser
      calls *(FUNC-02/2, UI-01/3a — probe BN)*; a forward version is **surfaced** and a malformed
      root **rejected without touching the live venue** *(probe BO)*
- [ ] Ping: **exactly one lane sounds, seven are exactly zero**, on a **non-identity** map *(BQ)*;
      ceiling holds at `outputGain +12 dB` **and** `trim +6 dB` *(BR)*; auto-cycle completes 8 in
      **12.8 s** *(BS)*; the latch **self-stops at 120 s** *(BT)*; it **refuses to start in SAFE
      mode** *(BU)*
- [ ] A preset load leaves all 42 venue values **bit-identical** *(BW)*; recalls correctly under a
      second venue *(BX)*; session state round-trips **both stores** *(BY)*
- [ ] `auval` SUCCEEDED; pluginval s10 ×3 VST3 + ×3 AU all exit 0
- [ ] **Gate 13**: Standalone renders the Venue screen, **SAVE opens a native modal and LOAD reads
      the file back**, the ping sounds and the indicator follows *(UI-01/3c)*
- [ ] **All six negative controls fired**; tree byte-identical afterwards; **NC3's asymmetry recorded**
      (§11 fires while §8 passes)
- [ ] `FUNC-02`, `FUNC-04`, `FUNC-05` and `UI-01` ticked ✅ **complete** in `REQUIREMENTS.md`, each
      criterion with its own evidence line, verified by a per-section `[x]`-vs-`→ **` count
- [ ] `SUMMARY-3.2.md` declares **N8's inheritance for 3.3**, the **open 3.3 gesture obligation**, and
      **FUNC-05's changed shape at 3.3**

---

## Risks Active in This Phase

| # | Risk | Mitigation in this plan |
|---|---|---|
| **N8** | **A label-swap collapses seven speakers to one signal at unity** — and it is live in shipped Stage-2 code today | **P52**: validate before applying, using `buildSpeakerToBuffer` itself as the predicate. **P53**: the label column holds rather than reverts, or L↔R is unreachable. Probe **BP** + §15 + **NC6** |
| **§4** | **A new page module silently reduces two gates' coverage** — they pass by not looking. Sixth instance of this vacuity class | **P51**: derive the enumeration from disk, in **Task 1**, before the file exists. **NC1 / NC2** |
| **N4** | **A dropped completion leaves the UI stale forever**, with no error, no rejection and a page still running | **P64**: no UI state depends solely on a promise; everything converges on the `venueGen` poll. §25 |
| **N5** | **A preset load sweeps 17 lanes in an armed host** | **P59**: 17 gestures at O-Octagon's call site. **Never edit the shared module.** §28 + **NC5** |
| **Q11** | **A width-bound mini-plan overflows the rail invisibly** — `document.scrollHeight` was 720 the whole time and §8 would have passed | **P62**: height-bind; §11 asserts `railScrollHeight <= railClientHeight`. **NC3 demonstrates §11 firing while §8 passes** |
| **Q6** | **A forward-version `.venue` half-applies** into the live model — a room partly measured and partly placeholder, indistinguishable on screen from a correct load | **P56**: fresh model, surface the version, reject a malformed root. Probe **BO** |
| **Q5** | **Pinging "speaker 5" on a stereo fold** — R1 reproduced inside its own diagnostic tool | **P60**: refuse to start when `mappedOutputAvailable()` is false, and stop on a mid-ping flip. Probe **BU** |
| **§R1** | Still the highest risk in the project, and **3.2 is the phase that puts a label dropdown in front of a user** | Three independent layers: P52's pre-commit guard, `buildSpeakerToBuffer`'s existing rejection, and D13's frame-level banner **with P54's reason** |
| **UI-01/3** | **A `must` criterion no existing gate class can close** — a native modal Playwright cannot drive and the harness has no editor for | **P57**'s three parts, designed at plan rather than discovered at verify |
| **MSVC** | 3.2 finally has call sites for the `SafePointer` init-capture hazard | Constraint 10 + §29 + §31. Hoist to a local; authored now, not fixed at port time |
| **P49** | A skipped Playwright gate reads green, and **eight** more criteria now rest on it | Fails rather than skips. **Playwright verified resolvable at plan** — no Task 0 this phase |
| **carried** | **D5 / QUAL-01's audible clause** — Stage 4. **The CI gap** — Stage 4, and 3.2 widens it again. **`COMPAT-02`**, **`COMPAT-04`** — Stage 4. **D7's legibility cost**, **UI-04/05 descope**, **the three §8 re-pins** — 3.3 discuss | Unchanged. Restated so no 3.2 result is read as having settled them |

---

## Next Phase

Ready for: **execute** — `/plugin-execute O-Octagon 3-gui`

Two orderings execute must not optimise away:

1. **Task 1 runs first**, before `venue.js` exists. The derived registry is what makes the sixth
   instance of the enumeration-hole class the last one.
2. **Tasks 2–8 complete, and Task 8 passes, before Task 9 writes a line of C++.** D4 removed the
   browser-iteration safety net for the whole stage, not just for 3.1.

And one finding execute must not soften: **`mapInvalid` is audible.** `setVenue` validates the label
set *before* `applyVenueEdit()`, and the label column **holds and marks** where every other column
reverts — because every route from `(L, R)` to `(R, L)` passes through a duplicate.
