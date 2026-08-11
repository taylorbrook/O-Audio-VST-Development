# Stage 2 — DSP · Phase 2.1 (Geometry Core) — Execute Summary

**Plugin:** O-Octagon
**Stage:** 2 of 4 — DSP
**Phase:** 2.1 of 3 — Geometry Core
**GSD phase:** execute
**Date:** 2026-08-11
**Branch:** `feat/o-octagon`
**Plan:** `PLAN-2.1.md` (14 tasks)

---

## Entry check — contract checksums

Re-verified at this boundary, not read out of a prior document
(`pattern_promotion_checksum_pins_replaced_file`). All four byte-exact against `STATUS.md`:

| Contract | SHA-256 | Result |
|---|---|---|
| `BRIEF.md` | `697a4f32…9fbd6` | ✅ |
| `parameter-spec.md` | `b45f88dc…b9e02f` | ✅ |
| `research/ARCHITECTURE.md` | `bff8a83b…06cfe` | ✅ |
| `ROADMAP.md` | `aec7d0ce…7ee29` | ✅ |

No drift.

---

## Outcome

**13 of 14 tasks complete. Task 1 (the manual Logic check) was reported done by the developer;
its four observations are still to be recorded — see Outstanding.**

The plugin now owns the room. It still writes the same mono sum to every output, but it writes it
**through the channel map**, the Stage-1 placeholder marker is gone from the source entirely, and
R1 — the highest-risk component in the plugin — has a three-layer test suite standing over it with
no gain math in the picture to confuse a diagnosis.

**21 probes across two test targets, 0 failures.**

---

## Files

### New — plugin source

| File | Contents |
|---|---|
| `Source/DSP/Vec.h` | POD `Vec2`/`Vec3` + `cross`/`dot`/`sub`/`len2`/`floorOf`. Header-only, `static_assert`ed trivially copyable, deliberately not `juce::Point` |
| `Source/Data/VenueModel.{h,cpp}` | The 42 values, the `VENUE` tree schema, per-attribute defaulting, and every derived geometric quantity |
| `Source/DSP/ConvexHull2D.{h,cpp}` | Andrew's monotone chain, classification, inside test, boundary projection, full degeneracy matrix |
| `Source/DSP/ChannelMap.{h,cpp}` | Namespace `ochan`, free functions only, no processor reference |
| `Source/Data/VenueSnapshot.h` | The POD snapshot + the lock-free double-buffer publisher |

### New — tests and tooling

| File | Contents |
|---|---|
| `tests/tools/gen_juce_channel_order.py` | Layer-2 generator: parses JUCE source, emits the golden + SHA-256 |
| `tests/unit/{CMakeLists.txt,main.cpp}` | `O-Octagon-geometry-test` — 16 probes (A–P) |
| `tests/render-harness/{CMakeLists.txt,main.cpp}` | `O-Octagon-render-test` — 5 probes (Q–U) |
| `.planning/todos/pending/2026-08-11-secrets-free-test-ci-workflow.md` | The CI gap, logged as a repo-level todo (P13) |

### Modified

- `Source/PluginProcessor.{h,cpp}` — venue store in the Stage-1-reserved slot, `rebuildChannelMap()`,
  `readVenueFromState()`, `publishSnapshot()`, `applyVenueEdit()`, `mappedOutputAvailable()`,
  the `processBlock` rewrite, the `setStateInformation` ordering, the `createEditor` guard
- `CMakeLists.txt` — three new TUs in `target_sources`; `OUARICON_BUILD_TESTS` gating both test dirs

---

## Gate results (Task 13) — every one re-run, none read out of a prior document

| # | Gate | Result |
|---|---|---|
| 1 | Clean 3-format build (VST3 + AU + Standalone) + both test targets, forced TU recompile | ✅ **0 warnings, 0 errors** in the entire log |
| 2 | `grep` for hardcoded output channel indices outside `ChannelMap` | ✅ **zero** — the four remaining hits on `8` are doc-comment prose (`"8-channel"`), no code |
| 3 | `grep -c "PHASE-2.2-REPLACE"` | ✅ **0** — retired, not grandfathered |
| 4 | `auval -a` → `aufx OuOc OuDv`; `auval -v` | ✅ **AU VALIDATION SUCCEEDED**, 17 × `parameter PASS` |
| 5 | pluginval strictness 10, VST3 ×3 and AU ×3 | ✅ **SUCCESS** on all six runs |
| 6 | Both test targets with `-DOUARICON_BUILD_TESTS=ON` | ✅ `geometry-test` exit 0, `render-test` exit 0 |
| 7 | 17 parameters vs `parameter-spec.md` | ✅ **17/17** on name, range, default **and group**, verified programmatically from the auval dump |
| 8 | `setLatencySamples` | ✅ absent (one comment saying never to call it) |

Benign, unchanged from Stage 1: pluginval AU emits `WARNING: Current program is -1` — a JUCE
AU-wrapper report present across the repo; the run still returns SUCCESS.

---

## Probe results

### `O-Octagon-geometry-test` — 16 probes, 0 failures

```
[PASS] A layer1-enum-bit-order      all 3 accepted 8-channel sets, scan bound 256 + size assertion
[PASS] B layer2-golden-vs-runtime   7.1 {1,2,3,4,10,11,20,21} | SDDS {1,2,3,4,5,6,7,8} | 5.1.2 {1,2,3,4,5,6,28,29}
[PASS] C layer2-sha256              5cd774cb228888a28f1526d127ce4ee1d076386b26a98ef8315b35a4abdd7b00
[PASS] D non-identity-map-same-set  built {1,2,3,4,5,6,7,0}, expected {1,2,3,4,5,6,7,0}
[PASS] E cross-container-missing-label  rejected, out {9,9,9,9,9,9,9,9}  (completely untouched)
[PASS] F duplicate-label-rejected   rejected, last valid map retained: {0,1,2,3,4,5,6,7}
[PASS] G numeric-label-...-rejected "7" -> type 134 (a discrete channel, NOT unknown) — map rejected
[PASS] H hull-default-venue         m=6 vertices {1,2,4,5,6,7}, spk3 ON_EDGE, spk8 ON_EDGE
[PASS] I near-collinear-epsilon     eps=0.00010000, d=1e-6 popped, d=1e-3 kept
[PASS] J rear-corner-outside        outside, d_hull=4.0608 m
[PASS] K projection-vs-oracle-200pt max |impl - oracle| = 0.000000867 m over 200 pts, 147 outside
[PASS] L degeneracy-matrix          collinear m=2; coincident m=1; zero-rake-span -> rakeFront; degenerate-bbox finite
[PASS] M ear-height-linearity       front 1.100 mid 3.550 rear 6.000; rakeRear moves the rear, front unchanged
[PASS] N srcZ0-rides-the-rake       rear height 8.0000 m >= rakeRear 8.0000 m, worst deficit 0.000000
[PASS] O rig-scale-and-invariant    rigScale 7.9317 m; doubled -> 15.8633 m (invariant holds)
[PASS] P venue-missing-and-partial  missing-node -> §OQ4 defaults; partial-node -> per-attribute defaults
```

### `O-Octagon-render-test` — 5 probes, 0 failures

```
[PASS] Q unity-gain-8-outputs       7.1 negotiated, max |out - in| = 0.000000000 across all 8 lanes
[PASS] R bus-layouts-1-2-8          mono(1ch) ok; stereo(2ch) ok; 7.1(8ch) ok
[PASS] S f3-narrow-buffer-hazard    3..7 ch buffers under a 7.1 layout: no OOB, finite, hazard state confirmed
[PASS] T venue-session-round-trip   42 values, 0 speaker mismatch(es), map valid, venue is non-default
[PASS] U stage1-session-defaults    VENUE stripped, restored to §OQ4 defaults, map valid
```

**Two Stage-1 carry-overs closed a phase early**, exactly as D3 predicted:

- **Issue 3 — unity gain was confirmed by inspection only.** Probe Q measures it: `max |out − in| =
  0.000000000` across all 8 lanes. Bit-exact, not "close enough".
- **Issue 4 — the F3 3–7-channel hazard was reasoned, not measured.** Probe S constructs those
  layouts programmatically, with no hardware, and asserts *both* halves: that the hazardous state is
  genuinely reproduced (`getTotalNumOutputChannels() == 8` while the buffer is narrower) and that
  nothing writes out of bounds. A probe that only asserted "no crash" could have passed against a
  state that never occurred.

---

## Findings

### F1 — `ARCHITECTURE §OQ4`'s stated `rigScale ≈ 7.95 m` is off; the true value is **7.9317 m**

Probe O measures 7.9317 m from the §OQ4 coordinate table. Recomputing by hand from the same table
gives 7.93165 m, so this is not an implementation error — **§OQ4's "≈ 7.95" is a hand-calculation
that does not follow from the coordinates listed immediately above it.** Everything else derived in
that section (centroid `(6.50, 12.4625, 4.925)`, bbox `x[0.50, 12.50] y[4.50, 19.50]`) matches
exactly.

Impact is nil today — `rigScale` is only consumed by the blur mapping at Phase 2.2, and the
discrepancy is 0.23%. It is recorded because a future reader comparing the two would otherwise
reasonably conclude the implementation was wrong.

**This is precisely why probe O asserts the scaling invariant rather than the constant.** A bare
`≈ 7.95` assertion would have been "fixed" by editing the code until it produced 7.95, encoding a
wrong number permanently (`pattern_test_fixture_mirrors_drift_silently`). The invariant — doubling
every coordinate doubles the RMS radius — is venue-independent and cannot be satisfied by tuning.
The value is additionally checked against a wide 7.0–9.0 band, which catches a gross error without
pinning a hand-calculated digit.

### F2 — probe K meets DSP-03 criterion 3's 1e-6, but the asserted bound is deliberately looser

Measured worst deviation from the independent oracle: **8.67e-7 m over 200 points**, i.e. the
criterion's 1e-6 m *is* met. The hard assertion is nonetheless set at 1e-4 m: the implementation is
single-precision and the sampling box reaches ~27 m, where one float ULP is already ~2e-6 m, so
1e-6 sits close enough to the arithmetic's own resolution that a different optimiser could cross it
with nothing actually wrong. The measured number is printed on every run and is the real signal — a
genuine regression moves it by orders of magnitude, not by 15%.

**The oracle is genuinely independent.** It finds the nearest point on each edge by *ternary search
on a convex 1-D function in double precision*, not by the closed-form dot-product projection the
implementation uses. An oracle that reran the implementation's own formula would reproduce its
errors and pass forever.

### F3 — probe H does not test what it appears to test, and probe I is why

The §OQ4 side walls are dead straight, so speakers 3 and 8 have **exactly zero** cross product with
their wall neighbours and are popped for *any* non-negative epsilon. **Probe H would pass with
`EPS_CROSS = 0`.** Probe I supplies the missing coverage with a near-collinear case in both
directions: a 1 µm displacement is popped (treated collinear), a 1 mm displacement survives as a
vertex, against a measured `EPS_CROSS` of 1.0e-4.

### F4 — G4's numeric-label trap confirmed empirically

A label of `"7"` resolves through `getChannelTypeFromAbbreviation` to type **134** —
`discreteChannel0 + 6`, a plausible-looking discrete channel, **not** `unknown`. Probe G asserts all
three properties: it is a discrete type, it is not `unknown`, and the map is rejected anyway — by
the permutation check, not by a parse error. The distinction matters because a reader who assumes
"bad label → unknown → obvious failure" would be wrong about the mechanism keeping this safe.

### F5 — the generator's failure paths are tested, not asserted in prose

Verified directly rather than claimed:

| Case | Result |
|---|---|
| `--juce-modules` pointing at a non-existent directory | exit **1**, no header written |
| A JUCE tree whose `enum ChannelType` block parses to zero entries | exit **1**, **no header written** |
| Two consecutive runs against the same JUCE tree | **byte-identical output** |

The second case is the one that matters: a vacuous golden reports green, which is worse than no
golden at all.

### F6 — `String(double)` round-trips exactly; FUNC-02's exactness is not at risk

`juce::var`'s double serialisation goes through `serialiseDouble()`, which uses 15–20 decimal places
with length reduction, **not** `std::ostream`'s 6-significant-digit default. Float → double → XML →
double → float is therefore exact. Probe T asserts all 42 values bit-exactly (via `memcmp`, so no
`-Wfloat-equal` suppression is involved) and passes with 0 mismatches.

---

## Plan decisions as implemented

| Decision | Implementation |
|---|---|
| **P5** — target naming | `O-Octagon-geometry-test`, `O-Octagon-render-test`; both reference the **plugin target `OuariconOctagon`**, never the folder name |
| **P6** — SAFE/REAL as a named helper | `mappedOutputAvailable(int)`. `processBlock` is its only caller today; 2.2's `GainStage` inherits it |
| **P7** — `ChannelMap` free functions | Namespace `ochan`, no processor reference. `rebuildChannelMap()` remains the only caller, the only writer of `mapInvalid`, and the only publisher |
| **P8** — Layer 1, one function two callers | Debug `jassert` in `rebuildChannelMap()` + probe A. Scan bound **256** and the `expected.size() == set.size()` assertion, both present |
| **P9** — 200-point fixture generated in-test | Seed `0x0C7A9042`, oracle in the same TU, no committed fixture file |
| **P10** — checksum as a `static_assert` | In `tests/unit/main.cpp`. SHA covers the **parsed data**, verified deterministic across runs |
| **P11** — hull classification first-class | `ConvexHull2D::classify()` returning `VERTEX`/`ON_EDGE`/`INTERIOR` |
| **P12** — two supporting headers | `Vec.h`, `VenueSnapshot.h`, both header-only, both absent from `target_sources` |
| **P13** — CI gap as a repo-level todo | Written; `build-and-release.yml` untouched |

### Deviations from the plan

Two, both minor and both deliberate:

1. **`VenueSnapshot` carries one field beyond ARCHITECTURE §3.6.6's list** — `hullEpsCross`.
   `hull::isInside()` needs the room-scaled area tolerance, and recomputing it on the audio thread
   from the bbox would be a second derivation of a message-thread quantity, which is the exact drift
   the snapshot exists to prevent.
2. **`ConvexHull2D::classify()` also returns `VERTEX` by coordinate, not only by index.**
   §3.1.3 defines `VERTEX` as "index appears in `hullPts`", which leaves the STEP-0 dedup case
   ambiguous: a speaker collapsed onto a point that *is* a corner would report `ON_EDGE`. A duplicate
   sitting exactly on a corner is a corner, and reporting otherwise would mislead a user who has just
   mistyped two identical coordinates. Documented at the call site.

---

## Requirement outcomes (as planned at plan, not discovered at verify)

| Req | Status | Note |
|---|---|---|
| **COMPAT-03** | ✅ complete | All 3 criteria. Grep clean; Layer 1 + Layer 2 both standing; the golden is parsed source with a build-failing checksum |
| **DSP-03** | ✅ complete | All 4 criteria. Probes H, I, J, K, L |
| **DSP-04** | ⚠️ **partial** | Criteria 1–2 complete (probes M, N). Criterion 3 — *changing `rakeRear` alone changes the **gain vector*** — needs `DbapSolver` → **2.2** |
| **FUNC-03** | ⚠️ **partial** | Criteria 1–2 complete (probes D, E, F, G). Criterion 3 — *changing a row moves audio to that output* — is unobservable while all 8 lanes carry identical signal → **2.2 Layer 3** |

`REQUIREMENTS.md` is updated at **verify**, not here.

---

## Outstanding

1. **Task 1 (Logic 8-channel negotiation) — reported done, observations not yet recorded.** The four
   data points the plan asks for are still needed before verify closes:
   - did all 8 surround-meter lanes move?
   - **which container did Logic negotiate** (7.1 / 7.1-SDDS / 5.1.2)? R2 predicts SDDS. Observation,
     not a gate — all three are accepted — and it feeds COMPAT-02 at Stage 4
   - did `outputGain` survive save / close / reopen?
   - did the automation menu list 17 parameters under 5 groups?

   **Do not over-read the meter check.** All 8 lanes carry identical signal at this phase, so it
   proves negotiation and writability, not independence. Independence is FUNC-01 at 2.2.

2. **Residual CI gap** — no test target in this repo has ever run in CI, so the Layer-2
   build-failing checksum only fires for whoever configures with `OUARICON_BUILD_TESTS=ON`. Logged
   at `.planning/todos/pending/2026-08-11-secrets-free-test-ci-workflow.md`. Not closed at 2.1.

3. **`ARCHITECTURE §OQ4`'s `rigScale ≈ 7.95`** should be corrected to 7.93 at the next contract
   revision. Not corrected here — ARCHITECTURE is a checksummed contract and editing it mid-phase
   would invalidate the pins that Phase 2.2 will re-verify against.

---

## Non-goals — confirmed absent

`DbapSolver`, `GainStage`, the 64-sample control grid, `SmoothedValue`, `SourceShaper`,
`HullProcessor`, `VerifyPing`, any WebView editor, any `juce_add_binary_data` target, any change to
`build-and-release.yml`, any FUNC-07 trim *application* (`trimLin` is carried in the snapshot and
applied nowhere), any new APVTS parameter (17, unchanged).

---

## Next

**verify phase** — `/plugin-verify O-Octagon 2-dsp`.

Writes `stages/2-dsp/VERIFICATION-2.1.md`. The stage-level `VERIFICATION.md` is written only at the
close of 2.3.
