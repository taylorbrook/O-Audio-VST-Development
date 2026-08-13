# Stage 3 — GUI · Phase 3.1 (Two-screen shell, Room plan, musical parameters) — Execute Summary

**Plugin:** O-Octagon
**Stage:** 3 of 4 — GUI · **Phase 3.1 of 3**
**GSD phase:** execute
**Date:** 2026-08-12
**Branch:** `feat/o-octagon` @ `a47cef88` (Stage 2 phases 2.2 / 2.3 work still uncommitted)
**Plan:** `PLAN-3.1.md` — 14 tasks (0–13), 14 plan decisions (P37–P50), 13 gates

---

## Outcome

**`UI-02` ✅ complete — all seven criteria, each with named measured evidence. Nothing else moved,
exactly as predicted at plan. Zero declared partials.**

- **13 of 13 gates pass.** Gate 13 (the Standalone launch-and-look) was discharged with a real
  WKWebView screenshot and window measurement rather than left open.
- **65 C++ probes, 0 failures** (33 unit + 32 render-harness). None of A–BJ regressed; BK / BL / BM
  are new.
- **30 JS gate assertions, 0 failures** — `ui_frontend_check.js` 20 sections + `ui_layout_check.js`
  10 sections, the latter run **twice** (pre- and post-integration).
- **8 negative controls, 8 fired.** Both new gate files are demonstrably non-vacuous.
- All four contract checksums byte-exact; **no pin moved at 3.1.**
- Clean 3-format build + both test targets on a forced full recompile: **zero `warning:`, zero
  `error:`, zero `FAILED`.**

Two deviations, both recorded below. One is a **real pre-existing defect that Phase 3.1 is the first
thing to make visible**, and it was fixed rather than rendered.

---

## Entry check — contract checksums

| Contract | SHA-256 measured at execute | STATUS frontmatter | Result |
|---|---|---|---|
| `BRIEF.md` | `697a4f32…f6b9fbd6` | `697a4f32…f6b9fbd6` | ✅ |
| `parameter-spec.md` | `b45f88dc…cbb9e02f` | `b45f88dc…cbb9e02f` | ✅ |
| `research/ARCHITECTURE.md` | `a8a358f4…9b6d4408` | `a8a358f4…9b6d4408` | ✅ |
| `ROADMAP.md` | `aec7d0ce…0137ee29` | `aec7d0ce…0137ee29` | ✅ |

**All four byte-exact. No pin moved.** The three §8 re-pins identified by research are scene work and
remain scheduled for **3.3 discuss**, untouched.

---

## How each of the 14 plan decisions landed

| P | Decision | Landed |
|---|---|---|
| P37 | `OctagonEditor`, ONE relay list, `kSliderIds` derived from `oo::params::id(i)` | ✅ **as planned, and stronger** — there is no `kSliderIds` symbol at all. The relay loop is bounded by `oo::params::kCount`; `ui_frontend_check.js` §16 asserts *both* that the derivation is present *and* that no hand-written list exists |
| P38 | Native surface exactly THREE; `getVenueGeometry` one call | ✅ **3/3**, diffed both directions + against the stub. **Payload shape deviated — see D-1** |
| **P39** | **N1 — the puck brackets BOTH `srcX` and `srcY`, incl. `pointercancel`** | ✅ Statically by §12, and *observed*: the stub counts brackets, and §10 measured **21 opened / 21 closed**, balanced, with every written parameter bracketed. **NC1 and NC4 both fire** |
| P40 | Optimistic render during drag, authoritative otherwise; render on echo, never write on echo | ✅ §12 asserts every `valueChangedEvent` listener only paints and that `renderPuck()` contains no `setNormalisedValue` |
| P41 | Accumulator clamped **at the accumulator** | ✅ §5 of the layout gate: a 420 px overshoot then a 4 px reversal moved the puck **4.00 px immediately**. **NC6** (clamp only at the write) produces **0.00 px** and fails |
| P42 | Envelope cached, invalidated by `venueGen`; no native call in a pointermove | ✅ §14 scans `pointermove`/`mousemove`/rAF bodies for `nativeFn(`/`await` — none. §7 proves the invalidation path end-to-end through the 2 Hz poll |
| P43 | `std::atomic<bool> safeMode` in `prepareToPlay()`; the set NAME never an atomic | ✅ Written as the **complement of the three REAL containers** rather than `== mono \|\| == stereo`, so a future fourth 8-channel container admitted by `isBusesLayoutSupported()` cannot silently start raising the banner. Probe **BM** drives all five |
| P44 | Stub's 17 ranges ASSERTED against `createParameterLayout()` | ✅ §15 parses the C++ — including the `w1..w8` **loop form**, which has no literal id in source — and diffs range, default and skew. **NC3** (w1 default 1.0 → 0.0) fires |
| P45 | UI-02/5 closes on (a)+(b)+(c) | ✅ All three. **The end-to-end half is declared as a 3.2 gate below** |
| P46 | Three layers, ONE `metresToPx()` | ✅ §19 asserts exactly one definition, that `app.js` does no projection, and that every dividing line touching a `min[XY]` bound lives inside it |
| P47 | Plan-left / controls-right; the 1100 × 720 gate MEASURES | ✅ **Measured 448.0 × 560.0 px**, which is RESEARCH §3.1's predicted figure to the pixel — and it is a *consequence* of a measured stage rect, not an input. `scrollWidth 1100 ≤ 1100`, `scrollHeight 720 ≤ 720`, on both screens |
| P48 | `createEditor`'s arms diverge; generic editor demoted to `#else` | ✅ §11 asserts the exact two-arm form, the guarded include, presence in `target_sources`, and **absence from the harness target** |
| **P49** | **The Playwright gate FAILS rather than SKIPs** | ✅ Implemented as specified. Task 0 installed Chromium; the resolver finds it in the npx cache. The no-Playwright path prints the install command and `exit 1` |
| P50 | Probes BK–BM → 65; JS gates counted separately as sections | ✅ 33 unit + 32 harness = **65**. JS: 20 + 10 **sections**, kept a separate family |

---

## Deviations

### D-1 — `getVenueGeometry`'s payload carries a `bbox` block that P38's fixed shape did not

**What changed.** P38 froze the payload at `envelope` / `centroid` / `rigScale` / `speakers` / `hull`
/ `hullCount`, with `degenerateX`/`degenerateY` inside `envelope`. The shipped payload adds a
sibling **`bbox`** block (`minX`/`maxX`/`minY`/`maxY`/`degenerateX`/`degenerateY`) and carries
`venueName` and `generation`.

**Why it is necessary, not cosmetic.** The envelope and the speaker bounding box are **different
boxes**, and the plan's own numbers say so: the envelope is `[−1.30, 14.30] × [2.25, 21.75]`, the
bbox is `[0.50, 12.50] × [4.50, 19.50]`. The envelope is what the plan *draws*. But
`VenueModel::normToMetres()` denormalises `srcX`/`srcY` against the **bbox**
(`VenueModel.h:185-192`), so the bbox is what the puck's position and the metres readout must resolve
through. With only the envelope on the wire, the page would have to **invert the 15 % margin rule** to
recover the bbox — a second derivation of a C++ rule, free to drift, and wrong by 1.80 m and 2.25 m on
the default venue in a way that looks entirely plausible. P38's own reasoning (`degenerateX` is a
*bbox* property, and it was already placed in the payload) points at the same conclusion.

**Cost of the deviation:** none to the gates. §13 still finds **zero float literals** in the lambda
body; every bound is an accessor and the degenerate test still references `oo::plane::kMinSpan` by
name. The bbox is read from the same `getVenue()` in the same pass, so the torn-read argument for one
call is untouched.

### D-2 — a one-line fix in `Source/Data/VenueModel.cpp`, which Task 7 did not list

**The defect.** `VenueModel.cpp:173` read `name = "Default (placeholder — NOT measured)";`. The only
`juce::String` assignment overloads take a `String`, so that literal is built through
`String::String(const char*)`, which converts via **`CharPointer_ASCII`**
(`juce_String.cpp:307-308`) and mangles every byte above 127. The em-dash is U+2014 — three UTF-8
bytes — so three garbage characters went into `apvts.state`'s `VENUE @name`, and would have gone into
every `.venue` file written from 3.2 on. There is no compiler warning; the damage is visible only in
output, which is exactly why it survived Stage 2 — **nothing rendered this string until Phase 3.1**
(`critical_juce_string_char_ctor_is_ascii_only`).

**Why it was fixed rather than reported.** Rendering it was the alternative, and the plan's own
constraint 9 names this hazard as a 3.1 **write-time** obligation. Fixed with
`juce::String (juce::CharPointer_UTF8 (...))`. Confirmed visually at Gate 13: the caption reads
`Default (placeholder — NOT measured)` with a correct em-dash.

**Scope check performed, not assumed.** A repo-wide scan for non-ASCII inside C++ string literals
found three occurrences. The other two — `ChannelMap.cpp:106` and `:120` — are built with `+`, which
takes the **UTF-8** path (`juce_String.cpp:773-777`) and were never affected. `ui_frontend_check.js`
§20 now asserts no non-ASCII string literal in `PluginEditor.cpp`.

### Two probe defects found and fixed *by the probes failing*, not by inspection

Recorded because each was a probe not measuring what it claimed:

1. **`ui_layout_check.js` §10** first reported **sixteen frozen readouts** on a page whose echo was
   working perfectly. One `ArrowRight` steps a range input by its `step` attribute — 0.0001
   normalised — which is **below the display resolution of every readout on the page** (0.0001 of
   `srcZ`'s 10 m range is 0.001 m against a 2-decimal readout). Changed to `PageUp`/`PageDown`, which
   Chromium steps by `max(step, range/10)`. A vacuous *failure* rather than a vacuous pass, but the
   same defect class.
2. **`ui_frontend_check.js` §7, §8, §16 and §18** all failed on correct code at first run: §7 and §8
   ban a token (`window.__JUCE__`, `juce://`) that also appears in the **comment explaining why it is
   banned**, in the file being scanned — a failure whose obvious "fix" is to delete the explanation.
   The gate now strips comments. §16's regex harvested a nineteenth parameter called `w` from the
   `makeFloat ("w" + juce::String (i), …)` loop; §18 anchored on `id=` while the markup writes `class`
   first. All four are gate fixes; the page was never wrong.

---

## UI-02's seven criteria → the evidence as RUN

| # | Criterion | Measured |
|---|---|---|
| 1 | Proportions follow the derived envelope | Plan box **448.0 × 560.0 px**, aspect **0.8000** == returned envelope **0.8000**. Stub mutated to a landscape venue → **3.0128 vs 3.0000**. **NC8** fires |
| 2 | Hull matches; **3 and 8 `ON_EDGE`** | `points.numberOfItems` **6** == returned `hullCount` **6**; `glyph-3`/`glyph-8` `is-onedge`, other six `is-vertex`. Probe **BK**: `1:V 2:V 3:E 4:V 5:V 6:V 7:V 8:E`. Visible as dashed rings in the Gate 13 screenshot |
| 3 | Relative-delta drag | **8 px off-centre** grab: **0.00 px** jump on pointerdown, **40.00 px** travel for a 40 px move. **NC5** fires at 33.00 px |
| 4 | `calc()` + DPR backing store | DPR 1 → **448 × 560**; DPR 2 → **896 × 1120**; both `== round(rect · dpr)` and they differ. **NC7** fires |
| 5 | Metres against the live venue | **(a)** puck stationary to 1e-6, readout `12.36 × 12.00 m` → `129.65 × 220.00 m`; **(b)** probe **BL** `bbMaxX` 12.500→15.500, `bbMinY` 4.500→2.500, gen 3→4; **(c)** **zero** float literals in the lambda. **NC2** fires |
| 6 | Rendered against the stub **before** C++ | **10/10 PASS at `2026-08-12T14:22:05Z`**, `Source/PluginEditor.cpp` absent — §0 recorded the pre-integration run. 17/17 controls, 17/17 readouts, 0 console errors |
| 7 | Bridge closure both directions | Surface **exactly 3**; JS-called == C++-registered == stub-whitelisted; unknown names REJECT |

### The two ROADMAP criteria UI-02 does not carry

- **SAFE banner on a stereo track and only there.** C++: probe **BM** — `mono:SAFE stereo:SAFE
  7.1:REAL 7.1-SDDS:REAL 5.1.2:REAL`. JS: §9 drove `getStatus` through both states, banner appeared
  and disappeared. **And observed live in WKWebView**: the Standalone negotiated stereo on the
  default device and the banner was present in the Gate 13 screenshot — which also confirms the 2 Hz
  poll is running inside a real WebView, not only in Chromium.
- **Every control moves its parameter; host automation moves every control.** §10: all 17 written
  (stub recorded every one), all 17 readouts moved on the echo, brackets balanced 21/21. §16's
  four-way closure. **The in-host confirmation remains folded into the Stage 4 session (D2).**

---

## Gate results

| # | Gate | Result |
|---|---|---|
| 1 | Clean 3-format build + both test targets, forced full recompile | ✅ **zero `warning:` / `error:` / `FAILED`** |
| 2 | `node tests/ui_frontend_check.js` | ✅ exit 0, **20 sections** |
| 3 | `node tests/ui_layout_check.js` | ✅ exit 0, **10 sections**, **did not skip** — Playwright resolved from the npx cache |
| 4 | Stub render **before** integration | ✅ **`2026-08-12T14:22:05Z`**, 10/10, `PluginEditor.cpp` did not exist |
| 5 | `auval -v aufx OuOc OuDv` | ✅ **AU VALIDATION SUCCEEDED** |
| 6 | pluginval s10, VST3 ×3 / AU ×3 | ✅ **6/6 exit 0, zero `FAILED` lines** |
| 7 | Both C++ test targets | ✅ **33 + 32 = 65 probes, 0 failures**, exit 0 / exit 0 |
| 8 | `gen_dbap_reference.py --check` | ✅ **102 cases OK** |
| 9 | 17 params vs `parameter-spec.md`, three sides | ✅ **17/17**; spec == `createParameterLayout` == `params::id()` == DOM `ctl-<id>`; ranges, defaults and skews all match; none hand-transcribed |
| 10 | `createEditor` guard; `PluginEditor.cpp` absent from harness | ✅ both, asserted by §11 |
| 11 | Unit-target link line: no `juce_dsp`, no `juce_gui_extra` | ✅ `juce_audio_basics`, `juce_core`, `juce_data_structures` only |
| 12 | Contract checksums | ✅ all four byte-exact, **no pin moved** |
| 13 | **Standalone launch, macOS** | ✅ window **1102 × 778** (= 1100 × 720 + Standalone chrome), plan drew, hull hexagon with 3/8 dashed, 8 weights at **1.00**, 9 column controls at declared defaults, footer `SOURCE 6.50 × 12.00 m · ENVELOPE 15.60 × 19.50 m`, SAFE banner live |

### Negative controls — 8 run, 8 fired

Both new gate files are non-vacuous, and every UI-02 criterion resting on the Playwright gate has a
demonstrated failure mode.

| # | Injected defect | Caught by |
|---|---|---|
| NC1 | Drop `srcY`'s `sliderDragStarted()` from the puck | §12 — *twice*: the balance count **and** the both-axes assertion |
| NC2 | Wire `envelope.minX` to the literal `-1.30f` | §13 — "found 1.30f" |
| NC3 | Drift the stub's `w1` default 1.0 → 0.0 | §15 — "w1(cpp 0..1 def 1 / stub 0..1 def 0)" |
| NC4 | Remove the `pointercancel` close | §12 |
| NC5 | Absolute cursor tracking instead of relative delta | layout §4 — 33.00 px for a 40 px move |
| NC6 | Clamp only at the write, accumulator runs past 1.0 | layout §5 — 0.00 px on reversal |
| NC7 | Drop the DPR backing store | layout §6 — *twice*: the DPR-2 identity **and** the differs-between-DPRs check |
| NC8 | Hardcode the plan aspect at 0.800 | layout §2 — *twice*: aspect did not follow, box did not change shape |

The unmodified tree passed both gates immediately after each control was reverted.

---

## Files

**Created**

```
Source/PluginEditor.h                        Source/ui/public/index.html
Source/PluginEditor.cpp                      Source/ui/public/css/styles.css
                                             Source/ui/public/js/app.js
tests/ui_frontend_check.js   (20 sections)   Source/ui/public/js/roomplan.js
tests/ui_layout_check.js     (10 sections)   Source/ui/public/js/juce/index.js             ← verbatim
tests/ui-stub/juce-stub.js                   Source/ui/public/js/juce/check_native_interop.js ← verbatim
tests/ui-stub/serve-stub.sh
```

Both JUCE bridge files are **byte-exact** copies of
`/Users/taylorbrook/JUCE/modules/juce_gui_extra/native/javascript/` (sha256 verified on copy).

**Modified**

```
CMakeLists.txt                  + juce_add_binary_data(OuariconOctagon_UIResources) NAMESPACE UIBinaryData
                                + Source/PluginEditor.cpp in target_sources
Source/PluginProcessor.h        + isSafeMode(), getVenueGeneration(), std::atomic<bool> safeMode
Source/PluginProcessor.cpp      + safeMode written in prepareToPlay(); createEditor arms diverge;
                                  guarded #include "PluginEditor.h"
Source/Data/VenueModel.cpp      D-2 — the CharPointer_UTF8 fix
tests/render-harness/main.cpp   + probes BK / BL / BM
.planning/REQUIREMENTS.md       UI-02 → complete, with per-criterion evidence
```

`tests/render-harness/CMakeLists.txt` and `tests/unit/CMakeLists.txt` are **unmodified**, as planned.

---

## Two declarations 3.2 must inherit

> **These are the two things PLAN-3.1 Task 13 says must appear here or 3.2 rediscovers them.**

### 1. UI-02/5's end-to-end half is a **3.2 gate**, declared now

3.1 closed UI-02 criterion 5 on P45's three parts: (a) a stub-mutation in Playwright, (b) probe BL in
C++, (c) a no-bbox-literals static scan. The **end-to-end** version — *type a coordinate on the Venue
screen and watch the Room readout move* — was not testable at 3.1 because the Venue screen is 3.2
work. It is a **3.2 gate**, not a 3.1 residual, and it is written down here rather than left for 3.2
to discover.

### 2. **N2 — do NOT route session state through `OuariconPresetManager::setStateFromXml`**

`setStateFromXml` calls `parameters.replaceState(...)` **and nothing else**. Routing O-Octagon's
session restore through it would bypass ARCHITECTURE §4.1's `readVenueFromState()` →
`rebuildChannelMap()` ordering, leaving **geometry, hull and channel map all describing the previous
venue**. It is silent, and it passes every existing probe.

**At 3.2: adopt the module for PRESETS ONLY.** Keep O-Octagon's own `getStateInformation` /
`setStateInformation` exactly as they are.

---

## Residuals — unchanged, restated so none is read as settled

| Residual | Destination |
|---|---|
| **D5 / QUAL-01's *audible* clause** — the ~15 min Logic session | **Stage 4** hall session (D2). Gate 13 is **not** D5 |
| **The CI gap** — these two JS gates are local-only, like the 65 C++ probes. **3.1 widens it**, exactly as CONTEXT predicted | **Stage 4** |
| `COMPAT-02` — Logic on a surround track, 8 discrete channels | **Stage 4** |
| `COMPAT-04` — retroactive criteria debt (the only requirement row without a section) | **Stage 4** |
| **UI-04 / UI-05 descope decision** | **3.3** |
| **D7's legibility cost** — the plan is **448 px wide**, now measured rather than estimated. A legibility failure is a **3.3 discuss** finding, not a 3.1 plan change | **3.3 discuss** |
| The three §8 contract re-pins (all scene-related) | **3.3 discuss** |

---

## Next

**verify** — `/plugin-verify O-Octagon 3-gui`

Every gate above is re-runnable from scratch and is **meant to be re-run at verify rather than read
out of this file** — the 2.3 discipline that caught four mis-attributions. Nothing here is a claim
that cannot be reproduced by running the command beside it.
