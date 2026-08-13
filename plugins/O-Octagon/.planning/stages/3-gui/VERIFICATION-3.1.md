# Stage 3 — GUI · Phase 3.1 (Two-screen shell, Room plan, musical parameters) — Verification

**Plugin:** O-Octagon
**Stage:** 3 of 4 — GUI · **Phase 3.1 of 3**
**GSD phase:** verify
**Date:** 2026-08-12
**Branch:** `feat/o-octagon` @ `a47cef88` (2.2 / 2.3 / 3.1 work uncommitted)
**Verifies:** `SUMMARY-3.1.md` against `PLAN-3.1.md`, `CONTEXT-3.1.md`, `REQUIREMENTS.md`

---

## Verdict

**✅ VERIFIED.** `UI-02` closes — 7/7 criteria, each re-measured at this boundary.
**Ready for Phase 3.2: yes, no blockers.**

**Every gate was RE-RUN from scratch, not read out of `SUMMARY-3.1.md`** — on a forced full
recompile (`rm -rf build/plugins/O-Octagon`, **128 ninja steps**). All 13 pass. **65 C++ probes
(33 unit + 32 harness), 0 failures. 30 JS gate sections, 0 failures.** All four contract checksums
recomputed byte-exact; **no pin moved at 3.1.**

**Two negative controls were run as NEW work at verify** — one per gate file — and both fired,
so neither new gate file is vacuous. The tree was proved **byte-identical** afterwards.

**One issue found at verify**, in `REQUIREMENTS.md` rather than in delivered code, and **fixed here**
(§Issues). Nothing in the shipped implementation was wrong.

---

## Goal-backward analysis

### What 3.1 set out to achieve (CONTEXT-3.1.md, PLAN-3.1.md)

1. A two-screen shell (ROOM / VENUE) at a fixed, non-resizable 1100 × 720 (D7).
2. A Room-screen top-down plan proportioned to the **derived envelope**, with an explicit convex
   hull overlay and a draggable source puck.
3. All 17 musical parameters bound through WebView relays, with host automation echoing back.
4. Rendered against `tests/ui-stub/juce-stub.js` **before** any C++ existed (D4's consequence).
5. Close **`UI-02` and nothing else.** Zero partials declared in advance.

### What was delivered, and whether it matches

| Goal | Status | Evidence re-measured at verify |
|---|---|---|
| 1 — two-screen shell at 1100 × 720 | ✅ Achieved | `ui_frontend_check.js` §17 (`setSize`, css html/body/.frame); `ui_layout_check.js` §8 measured `scrollWidth 1100 ≤ 1100`, `scrollHeight 720 ≤ 720` **on both screens**; Standalone window **1102 × 778** observed |
| 2 — plan / hull / puck | ✅ Achieved | Plan box **448.0 × 560.0 px**, aspect **0.8000** == returned envelope; hull polygon **6** points == `hullCount` 6; speakers **3 and 8** dashed `is-onedge`; puck relative-delta 40.00 px for 40 px |
| 3 — 17 params bound, echo both ways | ✅ Achieved | §10: 17/17 controls wrote, 17/17 readouts moved on echo, gesture brackets **21 opened / 21 closed**; §16's four-way closure; independent spec↔C++ diff **17/17** |
| 4 — stub-first ordering | ✅ Achieved | `ui_layout_check.js` §0 recorded the pre-integration run at `2026-08-12T14:22:05Z` with `Source/PluginEditor.cpp` absent. **This is a historical record and cannot be re-created at verify** — see §Method note |
| 5 — UI-02 only, zero partials | ✅ Achieved | UI-02 `complete`; the other 8 stage-3 rows remain `pending` for 3.2 / 3.3. Zero partials declared, zero found |

**Delivery matches goals. No goal was quietly narrowed, and no goal was exceeded in a way that
pulled unplanned work forward.**

---

## Requirements verification

**Stage:** 3-gui, phase 3.1
**Requirements verified at this phase:** 1 (`UI-02`, `should`)

| Requirement | Priority | Status | Acceptance criteria |
|---|---|---|---|
| UI-02: Room screen plan | should | ✅ **Complete** | **7/7**, each with named measured evidence |

### UI-02's seven criteria, re-measured at verify

| # | Criterion | Verify-phase measurement |
|---|---|---|
| 1 | Proportions follow the **derived envelope**, never a hardcoded aspect | `ui_layout_check.js` §2 — box **448.0 × 560.0 px**, rendered aspect **0.8000** == returned envelope **0.8000**; stub mutated to a landscape venue → **3.0128 vs 3.0000**, box changed shape. **NC8 re-run at verify: FIRES twice** |
| 2 | Hull overlay matches, incl. **3 and 8 `ON_EDGE`** | §3 — `polygon.points.numberOfItems` **6** == `hullCount` **6**; `glyph-3`/`glyph-8` `is-onedge`, other six `is-vertex`. C++ probe **BK**: `hullCount 6, 1:V 2:V 3:E 4:V 5:V 6:V 7:V 8:E`. **Observed as dashed rings in WKWebView at Gate 13** |
| 3 | Relative-delta drag, not absolute tracking | §4 — grabbed **8 px off-centre**: **0.00 px** jump on pointerdown, **40.00 px** travel for a 40 px move. §5 — after a 420 px overshoot a 4 px reversal moved the puck **4.00 px immediately** (clamp at the accumulator) |
| 4 | `calc()` sizing + DPR backing store | §6 — DPR 1 → **448 × 560**; DPR 2 → **896 × 1120**; both `== round(rect·dpr)` and the two **differ**. §17 asserts `calc()` and the absence of left+right |
| 5 | Metres resolved against the **live** venue | **(a)** §7 — puck stationary to 1e-6, readout `12.36 × 12.00 m` → `129.65 × 220.00 m` after a stub venue edit. **(b)** probe **BL** — `bbMaxX` 12.500→15.500, `bbMinY` 4.500→2.500, gen 3→4. **(c)** §13 — **zero** float literals in the `getVenueGeometry` body. **NC2 re-run at verify: FIRES** (`found 1.30f`) |
| 6 | Rendered against the stub **before** C++ | `ui_layout_check.js` §0's recorded pre-integration run, `2026-08-12T14:22:05Z`, `PluginEditor.cpp` absent, 10/10, 17/17 controls, 0 console errors. Post-integration re-run at verify: **10/10** |
| 7 | Bridge closure in **both** directions | §3 — surface **exactly 3**. **Independently re-derived at verify by hand**: the three `nativeFn("…")` call sites in `app.js`/`roomplan.js` (`getParameterDefaults`, `getStatus`, `getVenueGeometry`) == the three `withNativeFunction` registrations in `PluginEditor.cpp`. **Zero gaps either way** |

**Requirements summary:** ✅ Complete **1** · ⚠️ Partial **0** · ❌ Failed **0** · ⏸️ Deferred to a
later phase **8** (`FUNC-02/04/05/06`, `UI-01/03/04/05` — all stage-3, scheduled 3.2 / 3.3).

### The two ROADMAP criteria UI-02 does not carry — both re-confirmed

- **SAFE banner on a stereo track and only there.** Probe **BM**: `mono:SAFE stereo:SAFE
  7.1:REAL 7.1-SDDS:REAL 5.1.2:REAL`. `ui_layout_check.js` §9 drove `getStatus` through both states —
  banner appeared and disappeared. **Observed live in WKWebView at Gate 13**: the Standalone
  negotiated stereo and the banner read `SAFE  Stereo fold — not the 8-channel rig`, which also
  proves the 2 Hz poll runs inside a real WebView and not only in Chromium.
- **Every control moves its parameter; host automation moves every control.** §10 (17/17 written,
  17/17 echoed, 21/21 brackets) + §16's four-way closure. **The in-host confirmation remains folded
  into the Stage 4 session (D2)** — unchanged, and not claimed here.

---

## Automated checks — all 13 gates re-run

| # | Gate | Verify-phase result |
|---|---|---|
| 1 | Clean 3-format build + both test targets, **forced full recompile** | ✅ **128 steps, zero `warning:`, zero `error:`, zero `FAILED`** |
| 2 | `node tests/ui_frontend_check.js` | ✅ exit **0**, **20 sections**, all PASS |
| 3 | `node tests/ui_layout_check.js` | ✅ exit **0**, **10 sections**, **`skip` count 0** — the gate did not skip |
| 4 | Stub render **before** integration | ✅ §0's recorded run `2026-08-12T14:22:05Z`; post-integration re-run 10/10 |
| 5 | `auval -v aufx OuOc OuDv` | ✅ **AU VALIDATION SUCCEEDED** |
| 6 | pluginval s10, VST3 ×3 / AU ×3 | ✅ **6/6 exit 0, zero `FAILED` lines** |
| 7 | Both C++ test targets | ✅ **33 + 32 = 65 probes, 0 failures**, exit 0 / exit 0 |
| 8 | `gen_dbap_reference.py --check` | ✅ **102 cases OK**, no fixture drift |
| 9 | 17 params vs `parameter-spec.md` | ✅ **17/17** — re-derived at verify by an **independent** parse of both documents; ids, ranges and defaults all identical |
| 10 | `createEditor` guard; `PluginEditor.cpp` absent from harness | ✅ arms diverge, generic editor demoted to `#else`; `PluginEditor` **absent from both** `tests/*/CMakeLists.txt` |
| 11 | Unit-target link line: no `juce_dsp`, no `juce_gui_extra` | ✅ resolved from `build.ninja`: `juce_audio_basics`, `juce_core`, `juce_data_structures`, `juce_events` — **no `juce_dsp`, no `juce_gui_extra`** |
| 12 | Contract checksums | ✅ all four **byte-exact**, **no pin moved** |
| 13 | **Standalone launch, macOS (WKWebView)** | ✅ **re-discharged at verify with a fresh screenshot** — see below |

### Contract checksums, recomputed at this boundary

| Contract | Measured at verify | STATUS frontmatter | Result |
|---|---|---|---|
| `BRIEF.md` | `697a4f32…f6b9fbd6` | `697a4f32…f6b9fbd6` | ✅ |
| `parameter-spec.md` | `b45f88dc…cbb9e02f` | `b45f88dc…cbb9e02f` | ✅ |
| `research/ARCHITECTURE.md` | `a8a358f4…9b6d4408` | `a8a358f4…9b6d4408` | ✅ |
| `ROADMAP.md` | `aec7d0ce…0137ee29` | `aec7d0ce…0137ee29` | ✅ |

**All four byte-exact. No pin moved at 3.1**, as P37–P50 predicted. The three §8 re-pins remain
scheduled for **3.3 discuss**, untouched.

### Gate 13 — re-discharged at verify, not inherited

The Standalone was launched fresh from the verify build and photographed. **Window measured
1102 × 778** (= 1100 × 720 + Standalone chrome), matching `SUMMARY-3.1.md` exactly. Visible in the
capture:

- The ROOM / VENUE two-screen shell, ROOM active.
- The plan drew — 8 numbered speaker glyphs, hull polygon, source puck at centre.
- **Speakers 3 and 8 render as dashed rings; the other six are solid** — UI-02/2's visual half,
  in WKWebView rather than Chromium.
- All **8 weights at `1.00`** (the P44 neutral-default trap: not the range minimum).
- The 9 column controls at their **declared** defaults — `Source X 0.500`, `Source Y 0.500`,
  `Source Z 0.00 m`, `Width 0.00 m`, `Rolloff 4.00 dB/2x`, `Blur 0.10`, `Hull Atten 1.00 dB/m`,
  `Air 0.35`, `Output 0.0 dB` — each matching `parameter-spec.md`.
- Footer `SOURCE 6.50 × 12.00 m · ENVELOPE 15.60 × 19.50 m`.
- **SAFE banner live**: `SAFE  Stereo fold — not the 8-channel rig`.
- **D-2's fix confirmed visually**: the caption reads `Default (placeholder — NOT measured)` with a
  **correct em-dash**, not the three mangled bytes `String(const char*)` produced.
- Tabular numerals on every readout (P47's `tabular-nums` gate, as rendered).

**Gate 13 is NOT D5.** D5 remains folded into the Stage 4 hall session, untouched.

---

## Negative controls — 2 run as NEW work at verify, 2 fired

`SUMMARY-3.1.md` reports 8 negative controls at execute. Verify re-ran **one per gate file**, chosen
so that a pass proves the *file* non-vacuous rather than one assertion:

| # | Injected defect | Result |
|---|---|---|
| **NC2** | `PluginEditor.cpp:229` — wire `envelope.minX` to the literal `-1.30f` | ✅ **FIRED** — `ui_frontend_check.js` §13 `FAIL: no float literal in the getVenueGeometry body — found 1.30f`, **exit 1** |
| **NC8** | `roomplan.js:142` — hardcode `const aspect = 0.800` | ✅ **FIRED twice** — `ui_layout_check.js` §2 `FAIL: the aspect FOLLOWED — 0.8000 vs 3.0000` **and** `FAIL: the box actually changed shape`, **exit 2** |

**Tree proved byte-identical to baseline afterwards** — 32 files, `shasum -a 256`, zero drift — and
**both gates returned exit 0** on the restored tree. Neither new gate file is vacuous.

---

## Issues found at verify

### 1. `REQUIREMENTS.md` — UI-02 criterion 7's evidence line was orphaned into the UI-03 section (FIXED)

**The defect.** UI-02's seventh criterion (the `getNativeFunction`/`withNativeFunction` grep-diff)
was checked `[x]` but carried **no evidence line**. Its evidence — the `ui_frontend_check.js` §3
text — had been written **after** the `### UI-03` heading and its blockquote, so it appeared to be
the lead line of UI-03's criteria list instead.

**Why it matters, and why it is not cosmetic.** UI-02 is the requirement this phase exists to close.
Read as written, UI-02 had **six** substantiated criteria and one bare tick, while UI-03 — a
`pending` stage-3.3 row — appeared to carry a stray piece of already-measured evidence. That is
exactly the shape of a partial being read as complete at the next boundary, and it is the document
the 3.2 and 3.3 cycles inherit.

**Fixed at verify**, not merely reported: the evidence line was moved under its own criterion, and
the independent grep-diff performed at this verification was appended to it. Re-checked
programmatically — UI-02 now has **7 checked criteria and 7 evidence arrows**; UI-03 has **zero**
evidence arrows and its **4** unchecked criteria intact.

**Nothing in delivered code was wrong.** The bridge closure itself was correct, and verify
re-derived it by hand independently of §3.

### 2. `SUMMARY-3.1.md` Gate 11's module enumeration is incomplete (recorded, not fixed)

SUMMARY reports the unit-target link line as "`juce_audio_basics`, `juce_core`,
`juce_data_structures` only". Resolved from `build.ninja` at verify, it is those three **plus
`juce_events`** (a transitive dependency of `juce_data_structures`). **The gate's substantive claim
holds exactly** — no `juce_dsp`, no `juce_gui_extra` — so this is an incomplete enumeration in a
prose summary, not a gate failure or a link-line regression. Recorded so 3.2 does not read the
three-module list as the asserted invariant.

### 3. Cold-configure warning count — unchanged, re-recorded so it is not read as new

The forced full rebuild emitted **zero compiler diagnostics** (`warning:` 0, `error:` 0, `FAILED` 0
across 128 steps). The repo-wide `JUCE_BUNDLE_ID` CMake messages noted at 2.2 and 2.3 are unchanged
and none are O-Octagon's.

---

## Method note — what verify could and could not re-create

**UI-02 criterion 6 is a historical claim.** "Rendered against the stub *before* C++ integration"
is discharged by `ui_layout_check.js` §0's recorded run at `2026-08-12T14:22:05Z`, taken when
`Source/PluginEditor.cpp` did not exist. That file exists now, so **verify cannot re-create the
pre-integration state without deleting delivered work.** What verify did instead: re-ran the gate
post-integration (10/10) and confirmed §0 still cross-checks `setSize` against the shipped page.
The ordering claim rests on the execute-phase record, and this is stated rather than presented as
a verify-phase re-measurement.

Every other criterion was measured fresh at this boundary.

---

## Human verification

- [x] Standalone launches on macOS and the WebView **renders** (Gate 13, re-discharged at verify)
- [x] Hull, puck, weights, controls and both banners visible and correct in WKWebView
- [x] Em-dash renders correctly in the venue caption (D-2)
- [ ] **In-host confirmation that host automation moves every control** — folded into the **Stage 4**
      session (D2), unchanged
- [ ] **D5 / QUAL-01's *audible* clause** — the ~15 min Logic hall session, **Stage 4** (D2)

---

## Residuals — unchanged, restated so none is read as settled

| Residual | Destination |
|---|---|
| **D5 / QUAL-01's *audible* clause** — the ~15 min Logic session. **Gate 13 is not D5** | **Stage 4** hall session (D2) |
| **The CI gap** — the 65 C++ probes and both JS gates are local-only. **3.1 widens it**, as CONTEXT predicted | **Stage 4** |
| `COMPAT-02` — Logic on a surround track, 8 discrete channels | **Stage 4** |
| `COMPAT-04` — retroactive criteria debt (the only requirement row without a criteria section) | **Stage 4** |
| **UI-04 / UI-05 descope decision** | **3.3 discuss** |
| **D7's legibility cost** — the plan is **448 px wide**, measured not estimated | **3.3 discuss** |
| The three §8 contract re-pins (all scene-related) | **3.3 discuss** |

## Two declarations 3.2 inherits — carried forward from SUMMARY-3.1.md

1. **UI-02/5's end-to-end half is a 3.2 gate**, declared at plan (P45): type a coordinate on the
   Venue screen, watch the Room readout move. 3.1 closed the criterion on (a)+(b)+(c); the
   end-to-end version was not testable because the Venue screen is 3.2 work.
2. **N2 — do NOT route session state through `OuariconPresetManager::setStateFromXml`.** It calls
   `parameters.replaceState(...)` and nothing else, which would bypass ARCHITECTURE §4.1's
   `readVenueFromState()` → `rebuildChannelMap()` ordering and leave geometry, hull and channel map
   all describing the **previous** venue. It is silent and it passes every existing probe.
   **Adopt the module for PRESETS ONLY**; keep O-Octagon's own `getStateInformation` /
   `setStateInformation` exactly as they are.

---

## Stage verdict

**Status:** ✅ **VERIFIED** (Phase 3.1)

**Ready for next phase:** **Yes** — Stage 3 Phase 3.2 (Venue screen: FUNC-02 / FUNC-04 / FUNC-05 +
UI-01).

**Stage 3 is NOT complete** — 3.1 of 3. Eight stage-3 requirement rows remain `pending` by design.

**Blockers:** none.
