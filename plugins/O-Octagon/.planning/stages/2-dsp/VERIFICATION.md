# Stage 2 — DSP — Verification

**Plugin:** O-Octagon
**Stage:** 2 of 4 — DSP
**Phases:** 2.1 (Geometry Core) · 2.2 (DBAP Solve + Gain Application) · 2.3 (Source Shaping +
Outside-Hull)
**GSD phase:** verify
**Date:** 2026-08-11
**Branch:** `feat/o-octagon`

> This is the **stage-level** report. Each phase carries its own: `VERIFICATION-2.1.md`,
> `VERIFICATION-2.2.md`, `VERIFICATION-2.3.md`. This document closes Stage 2 as a whole and does not
> restate their detail.

---

## Verdict

**✅ VERIFIED.** Ready for Stage 3 (GUI): **yes**, with no blockers.

**All fourteen requirements on Stage 2's traceability line close ✅ complete. Zero partial, zero
failed.**

Stage 2 ran **one full discuss → research → plan → execute → verify cycle per roadmap phase** — a
decision taken at 2.1 discuss because the channel map is R1 (CRITICAL, silent failure, audible only
in the hall) and deserved its own verify before any gain math existed to confuse a diagnosis. That
structure is what produced the stage's defining property: **every partial in this stage was declared
at a discuss boundary and closed at its named destination. Not one was discovered at verify.**

| | 2.1 | 2.2 | 2.3 | Stage |
|---|---|---|---|---|
| Probes | 21 | 46 | **62** | 33 unit + 29 render harness |
| Failures | 0 | 0 | **0** | — |
| Gates re-run at verify | 8 | 10 | **11** | all from scratch, forced full recompile |
| Negative controls | 1 | 4 | **8** | **13 total** |
| Requirements closed | 2 ✅ 2 ⚠️ | 10 ✅ 1 ⚠️ | **6 ✅** | **14 ✅ 0 ⚠️ 0 ❌** |

---

## Entry check — contract checksums at the stage close

| Contract | SHA-256 | Result |
|---|---|---|
| `BRIEF.md` | `697a4f32890d7420…` | ✅ **unchanged across all three phases** |
| `parameter-spec.md` | `b45f88dc5017ec2c…` | ✅ **unchanged across all three phases** |
| `research/ARCHITECTURE.md` | `a8a358f4be0ea183…` | ✅ matches the 2.3 discuss re-pin |
| `ROADMAP.md` | `aec7d0ce0db9ad6c…` | ✅ **unchanged across all three phases** |

**`ARCHITECTURE.md` was re-pinned twice, once per phase boundary, and never mid-phase.**

| Boundary | Change | Superseded hash |
|---|---|---|
| 2.2 discuss (D2) | `rigScale` **7.95 → 7.93165 m** in §OQ4, plus §3.3.2's blur table. Recomputed independently at discuss — the third derivation, and the first time the contract agreed | `bff8a83b…` |
| 2.3 discuss (D2) | §3.5.2's air skip condition → **`airAmount · d_hull == 0`**, making the shipping default patch bit-transparent | `cd881a10…` |

**No pin moved at 2.3** (P36). H2's correction to §3.5.2's accepted-cost figures — the quoted
"0.7 dB @ 10 kHz" is the *analog* one-pole's, and magnitude was the wrong quantity anyway, the phase
term dominating by up to 190× — is carried as an **erratum** in `SUMMARY-2.3.md` and in
`REQUIREMENTS.md`'s QUAL-01 note, both un-checksummed, rather than as a third re-pin.

---

## Goal-Backward Analysis

### What Stage 2 set out to achieve (`ROADMAP.md`, phases 2.1–2.3)

1. **Geometry and routing standing before a single gain is computed** — `VenueModel`,
   `ConvexHull2D`, `ChannelMap`, `VenueSnapshot`, with the channel-map suite in place.
2. **3-D DBAP per the 2011-04-14 revised equations**, solved on a control grid and applied through
   per-sample smoothers, real-time safe and block-size invariant.
3. **Source shaping and outside-hull processing** — stereo sub-points, hull trim, air-absorption LPF,
   per-speaker calibration trim.
4. **All fourteen Stage-2 requirements verified** against acceptance criteria that exist.

### What was delivered

| Component (`ROADMAP` §Algorithms) | Status | Where |
|---|---|---|
| 1. `VenueModel` — bbox, centroid, `rigScale`, sloped plane | ✅ | 2.1, + `VenueGeometry.h` free functions (P14) |
| 2. `ChannelMap` — `getChannelIndexForType()` + label layer | ✅ | 2.1 |
| 3. `ConvexHull2D` — monotone chain, classification, projection | ✅ | 2.1 |
| 4. `DbapSolver` — 3-D DBAP, revised equations | ✅ | 2.2 |
| 5. `SourceShaper` — denormalisation, sub-points, `rFade`, rake | ✅ | created 2.2 (P15/H3), made live 2.3 |
| 6. `HullProcessor` — trim + air LPF | ✅ | 2.3, header-only (P25) |
| 7. `GainStage` — control grid, 17 smoothers, trims, output gain | ✅ | 2.2, completed 2.3 |
| 8. `VerifyPing` | ⏸️ **Stage 3** | FUNC-04, correctly out of Stage 2's scope |

### Goal achievement

| Goal | Status | Evidence |
|---|---|---|
| 1 — geometry and routing first | ✅ Achieved | 2.1 closed COMPAT-03 and DSP-03 before any gain existed. The Layer-2 golden **fails the build** if JUCE's enum-bit order moves — proven by negative control at 2.1 verify |
| 2 — DBAP, solved and applied correctly | ✅ Achieved | Probe Y: **max \|impl − oracle\| = 1.0236e-7** over 102 committed fixture cases against an independent Python oracle. AA: **max \|Σv²−1\| = 3.259e-7** over 7686 solves. AO: **0 allocations** across 66 `processBlock` calls. AL/AM/AN/BI: bit-identical by `memcmp`, never a tolerance |
| 3 — shaping and outside-hull | ✅ Achieved | 2.3's three markers retired to zero; AY, AX, BA, AU, AV, BD, BE, BF, AW, BB, BC — all with non-vacuity or negative controls |
| 4 — fourteen requirements verified | ✅ Achieved | Table below. **And the criteria they are verified against were written**, which was not true when the stage began |

---

## Requirements Verification — the full Stage 2 line

| Requirement | Priority | Status | Closed at |
|---|---|---|---|
| FUNC-01 — 8-channel transport | must | ✅ Complete | 2.2 *(criterion 3; re-mapped from stage-1)* |
| FUNC-03 — label map | must | ✅ Complete | 2.1 + 2.2 |
| FUNC-07 — per-speaker calibration trim | should | ✅ Complete | **2.3** |
| DSP-01 — DBAP, 3-D, revised equations | must | ✅ Complete | 2.2 |
| DSP-02 — constant intensity | must | ✅ Complete | 2.2 |
| DSP-03 — convex hull | must | ✅ Complete | 2.1 |
| DSP-04 — sloped audience plane | must | ✅ Complete | 2.1 + 2.2 |
| DSP-05 — speaker weights | must | ✅ Complete | 2.2 |
| DSP-06 — stereo sub-point geometry | should | ✅ Complete | **2.3** |
| DSP-07 — outside-hull distance processing | should | ✅ Complete | **2.3** |
| DSP-08 — room-size-independent blur | should | ✅ Complete | **2.3** |
| PERF-01 — real-time safety | must | ✅ Complete | 2.2 |
| PERF-02 — solve scheduling and smoothing | should | ✅ Complete | 2.2 |
| COMPAT-03 — channel map | must | ✅ Complete | 2.1 |
| QUAL-01 — no audio artifacts | must | ✅ Complete | **2.3** |
| QUAL-02 — no NaN or Inf | must | ✅ Complete | 2.2 |
| QUAL-03 — block-size invariance | must | ✅ Complete | 2.2 |
| QUAL-04 — no zipper noise | should | ✅ Complete | 2.2 + **2.3** |

**Stage 2 summary:** ✅ Complete **18 of 18 rows** (14 distinct requirements on the traceability line
plus FUNC-01, DSP-04/3, FUNC-03/3 and QUAL-04/3 closing across phase boundaries) · ⚠️ Partial **0** ·
❌ Failed **0**.

### Every partial in this stage was declared, not discovered

| Partial | Declared at | Closed at | Verified as real when declared |
|---|---|---|---|
| DSP-04 criterion 3 | 2.1 plan | 2.2 (probe AK, both halves) | yes |
| FUNC-03 criterion 3 | 2.1 plan | 2.2 (probe AJ) | yes |
| QUAL-04 criterion 3 (`width`) | **2.2 discuss** | 2.3 (AY + AZ) | yes — 2.2 verify read `GainStage.cpp:147` and confirmed `p[params::width]` was not in the solve path |
| DSP-08 | 2.2 plan *(implemented 2.2, ticks 2.3)* | 2.3 (AW) | yes — AG was named supporting evidence only |

---

## Automated Checks — the stage gate set, all re-run at 2.3 verify

Against a forced full recompile of everything (`rm -rf build/plugins/O-Octagon`, 119 steps).

| Gate | Result |
|---|---|
| Clean 3-format build (VST3 + AU + Standalone) + both test targets | ✅ **zero `warning:` / `error:` / `FAILED`** |
| Both test targets | ✅ exit 0 / exit 0 — **62 probes, 0 failures** |
| `auval -v aufx OuOc OuDv` | ✅ **AU VALIDATION SUCCEEDED** |
| pluginval strictness 10, VST3 ×3 + AU ×3 | ✅ all six exit 0, zero `FAILED` |
| Marker retirement (`PHASE-2.3-*`, `PHASE-2.2-REPLACE`) | ✅ **0 / 0 / 0 / 0** in `Source/` **and** `tests/`, counted as occurrences |
| Hardcoded output channel indices outside `ChannelMap` | ✅ zero — the only output writes are `speakerToBuffer[i]` and a bounded loop var |
| `gen_dbap_reference.py --check` | ✅ exit 0, 102 cases |
| 17 parameters vs `parameter-spec.md` | ✅ **17/17 across three sides**, none hand-transcribed |
| `setLatencySamples` / `switch` on `ChannelType` / `createEditor` guard | ✅ absent / absent / present |
| `OOCTAGON_INSTRUMENT` scoping | ✅ 0 in the plugin target, 1 each in the two test targets |
| Unit-target link line has no `juce_dsp` | ✅ |

---

## Thirteen negative controls across the stage

The practice that distinguishes this stage's verifies from its executes: execute *asserts* its gates
work; verify **breaks them and measures**. Each control was reverted and the tree proved
byte-identical.

| Phase | Control | Outcome |
|---|---|---|
| 2.1 | Mutate `leftSurroundRear = 20 → 90` in a copied JUCE tree | Golden SHA moved; substituting it produced `error: static assertion failed`. **The gate fails the BUILD**, per ROADMAP:131 |
| 2.2 | NC1 — drop the `(z_i − z_s)` term | Y → 0.39153065 FAIL, Z → 0/8 FAIL. **AA still passes** → **DSP-02 cannot backstop DSP-01** |
| 2.2 | NC2 — perturb one committed fixture gain by 1e-4 | `--check` exit 1; **Y fails at exactly 1.0000e-4** |
| 2.2 | NC3 — bypass the channel map | AJ FAIL, Q′ FAIL. **AI passes bit-identically** → **AI evidences independence, not routing** |
| 2.2 | NC4 — remove the generation term from the dirty check | AQ FAIL (`THE DIRTY CHECK IS STALE`), AP venue-edit solves 1 → 0 |
| 2.3 | NC1 — remove P27's `reset(x)` seed | BB FAIL (`P27's SEED IS GONE`), **and AS + AZ FAIL** → 2.2's QUAL-04 probes now depend on a 2.3 mechanism |
| 2.3 | NC2 — remove P29's `trimDb` sanitisation | BH FAIL, `NON-FINITE`, `clamp: xinf — NOT CLAMPED`. **Only the `+inf` path kills it**; NaN and −1e30 are benign |
| 2.3 | NC3 — drop FUNC-07's multiply | BF FAIL on all four criteria |
| 2.3 | NC4 — remove the `d_hull > 0` half of D2 | BD FAIL (`FILTER RAN INSIDE THE HULL`), **and Q′ FAIL** → D2 is load-bearing for a 2.2 requirement |
| 2.3 | NC5 — restore the literal `20000` ceiling | AU FAIL (`ceilings WRONG; PAST NYQUIST`) |
| 2.3 | NC6 — revert `width` to the 2.2 literal | AY FAIL (`WIDTH IS NOT REACHING THE SHAPER`). **AZ still passes** → **QUAL-04/3 = AY + AZ, not AZ alone** |
| 2.3 | NC7 — remove P31's NaN guard | BE FAIL half 1 (`active-filter NaN LATCHED`) |
| 2.3 | NC8 — remove P31 **and** P27 together | BE half 2 `RE-ENTERED POISONED` → the two mechanisms independently close the same hole |

**Four of the thirteen corrected an evidence attribution rather than confirming one** (2.2 NC1, NC3;
2.3 NC6, NC7/NC8). That is the return on the practice: in every case a green results table read as
stronger evidence than it was, and the correction landed before the verdict rather than after.

---

## Findings worth carrying out of Stage 2

Beyond the deviations and residuals, these were learned by measurement and cost real time:

- **A `Σ v² = 1` invariant cannot backstop a distance-formula defect** — it normalises whatever
  distances it is given (2.2 NC1).
- **Two probes that look interchangeable in a results table usually are not.** Three separate
  instances: AI/AJ, AZ/AY, BE half 1 / half 2.
- **`juce::String(const char*)` is ASCII-only** and there is **no compiler warning** — build strings
  with `<<`. Matters for every Stage-3 UI string.
- **`juce::jmax` silently discards NaN** (`a < b ? b : a`), so a running max is the wrong shape for a
  NaN guard — use a last-value check where the state is sticky by construction.
- **A peak-based bound under-reads a sine by up to 13.4 %** at 6 samples/cycle. Derive the amplitude
  from RMS over an integer number of cycles.
- **A helper that returns `nx = 0.0` plus a probe that perturbs by *scaling* gives `0 == 0`** — an
  identity satisfied by nothing happening at all.
- **The §OQ4 rig is not exactly mirror-symmetric in float32** — `9.8f`/`3.2f` are inexact, so pair
  (5,6) is not bit-identical while (1,2), (3,8), (4,7) are.
- **`posix_memalign`, not `std::aligned_alloc`** — libc++ gates the latter behind a feature macro,
  and a probe that fails to build is a probe that silently stops running.

---

## Residual — open beyond Stage 2

| # | Item | Owner |
|---|---|---|
| 1 | **D5 manual Logic gate — OPEN.** ~15 min, folding in 2.2's Task 12. Corroboration for width / air / trim / lockstep / `w3 = 0`; the H2 HF-rich hull-crossing item is the **only** claim in Stage 2 that measurement cannot settle (QUAL-01/2's *audible* clause). Fresh VST3 + AU installed at verify, `auval`-clean | **Stage 3 discuss** |
| 2 | **CI gap** — no test target in this repo has ever run in CI; all 62 probes fire only under `-DOUARICON_BUILD_TESTS=ON` locally | Stage 4 |
| 3 | **`COMPAT-04`** — ticked `complete` at stage-1 against **no acceptance criteria at all**; criteria owed retroactively | Stage 4 |
| 4 | **`FUNC-06` and `UI-02..05`** — summary rows with no acceptance criteria | **Stage 3 discuss, before Stage 3 plan** |

Items 3 and 4 are the same defect this stage repaired three times (PERF-02 + QUAL-04 at the 2.2
boundary; FUNC-07 + DSP-06 + DSP-07 + DSP-08 at the 2.3 boundary). The habit that catches it — audit
`REQUIREMENTS.md` for summary rows without a `###` section at every discuss boundary — should carry
into Stage 3 unchanged.

---

## Human Verification

- [ ] **D5, ~15 min in Logic** — automate `srcX`, confirm the 8 surround-meter lanes no longer move in
      lockstep; `w3 = 0` → that lane silent, others compensate; `width` 0 → 6 audibly spreads; air
      audibly dulls **outside** the hull and is **inaudible inside** it; a per-speaker trim moves one
      lane only; **cross the hull with HF-rich material** and listen for the D2 step.
- [x] Fresh VST3 + AU installed with the AU cache cleared and both `-dev`/unsuffixed variants swept
      *(done at verify; `auval -a` lists `aufx OuOc OuDv`)*.

---

## Stage Verdict

**Status:** ✅ **VERIFIED**

**Ready for Stage 3:** **Yes.**

**Blockers:** none.

---

## Next

**Stage 3 — GUI**, three phases (3.1, 3.2, 3.3) per `ROADMAP.md`, verifying FUNC-02, FUNC-04,
FUNC-05, FUNC-06 and UI-01..05.

Carry into Stage 3 discuss: the D5 session, the `FUNC-06` / `UI-02..05` criteria debt,
`pattern_render_harness_breaks_on_webview_editor` (the `createEditor` `#if JUCE_WEB_BROWSER` guard is
present and gate 9 keeps it), `critical_juce_string_char_ctor_is_ascii_only`, and — if D5's H2 item
ticks audibly — `RESEARCH-2.3` H3's lever on `fc(d_hull = 0)`.
