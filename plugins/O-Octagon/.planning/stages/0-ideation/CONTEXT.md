# O-Octagon — Stage 0 Context (Discuss / Research / Plan)

**Date:** 2026-08-11
**Stage:** 0 — Ideation → Research & Planning
**Status:** complete
**Complexity tier:** 6 (DEEP research)
**complexity_score:** 5.0 (capped; raw 13.0)

---

## What Stage 0 Produced

| Artifact | Path |
|---|---|
| DSP architecture (binding contract) | `plugins/O-Octagon/.planning/research/ARCHITECTURE.md` |
| Implementation roadmap | `plugins/O-Octagon/.planning/ROADMAP.md` |
| This context file | `plugins/O-Octagon/.planning/stages/0-ideation/CONTEXT.md` |

---

## Discuss-Phase Findings

### The locked architecture held — nothing was re-litigated

All five locked constraints from `research/logic-pro-multichannel-octaphonic-dbap.md` survived
Stage 0 intact and are carried into ARCHITECTURE.md as hard requirements:

1. mono/stereo in → `AudioChannelSet::create7point1()` out; 7.1 as an 8-channel carrier only
2. never `octagonal()` or `discreteChannels(8)`
3. not multi-output (`aufx` gets one bus)
4. `AudioChannelSet` is a bitset; map built once in `prepareToPlay()` via `getChannelIndexForType()`
5. DBAP per the 2011-04-14 revised equations

Stage 0's job was **HOW**, not **WHETHER**, and it stayed inside that boundary.

### Two findings that change implementation without changing architecture

**1. `kAudioChannelLayoutTag_Emagic_Default_7_1` exists in JUCE, and Logic is Emagic's descendant.**

`juce_CoreAudioLayouts_mac.h:117` maps that tag to
`{ left, right, leftSurround, rightSurround, centre, LFE, leftCentre, rightCentre }` — whose *type
membership* is JUCE's `create7point1SDDS()`, **not** `create7point1()`. This is consistent with the
prior research's independently sourced note that Logic's 5.1 default is `L, R, Ls, Rs, C, LFE` (the
`MPEG_5_1_B` family ordering).

Consequence: `isBusesLayoutSupported()` accepts all three 8-channel containers Logic exposes (7.1,
7.1-SDDS, 5.1.2), and the user label map is keyed on `ChannelType` rather than on a buffer slot so it
adapts to whichever Logic negotiates. The declared default output remains `create7point1()`. Three
lines of defence against an entire class of failure that cannot be reproduced without the hall.
Confidence MEDIUM; settled at Stage 4.

**2. The 7.1 enum-bit order coincidentally equals its initializer-list order.**

Verified from source: `create7point1()` is `{left(1), right(2), centre(3), LFE(4),
leftSurroundSide(10), rightSurroundSide(11), leftSurroundRear(20), rightSurroundRear(21)}` — already
ascending. So a **hardcoded 0..7 map would appear correct today**. This makes the locked constraint
*more* important, not less: the trap now actively rewards the wrong mental model. Recorded
prominently in ARCHITECTURE §3.2.1 so no future implementer mistakes the coincidence for a guarantee.

### One design defect found and fixed before any code exists

**The centre-crossing L/R flip.** As the puck sweeps through the rig centroid, the bearing vector
reverses, the perpendicular reverses, and the two stereo sub-points swap sides instantaneously — a
6-metre jump of both sub-points in one control block at `width = 6`. Audible click; violates QUAL-01.

Fixed by collapsing the effective spread to zero as the puck approaches the centroid
(`wEff = width · min(1, |b|/rFade)`, `rFade = 0.15·rigScale`). The flip then occurs when the spread
is already zero, so the gain vectors are continuous. Stateless, so it does not compromise block-size
invariance — a hysteresis-based fix would have.

### One requirement conflict found and resolved

**PERF-02 and QUAL-03 are incompatible under the obvious implementation.** PERF-02 asks for a
per-parameter-change (i.e. per-block) solve; QUAL-03 demands bit-identical output at blockSize 512
and 4096. A per-block solve makes the control rate a function of blockSize, so the smoothing ramps
differ and the outputs diverge — `pattern_block_rate_envelope_breaks_blocksize_invariance`.

Resolved with a **fixed 64-sample control grid keyed to an absolute sample counter**. Control updates
land at identical absolute sample positions regardless of how the host chops the buffer. Satisfies
PERF-01, PERF-02, QUAL-03 and QUAL-04 simultaneously. Cost is ≤ 16 `pow` per 64 samples.

QUAL-03's acceptance criteria were additionally given an explicit **test protocol** (harness drives
parameters at control-grid-aligned absolute offsets), because a strict reading of "bit-identical
across block sizes" is otherwise untestable — when the host's message-thread automation writes land
is not a property of this plugin.

---

## Open Questions — All Five Resolved

| # | Question | Resolution | Rationale (one line) |
|---|---|---|---|
| 1 | Stereo-track fallback | **Accept mono/stereo out as SAFE pass-through** — dry input at unity, nothing spatialised, persistent UI banner | Refusal kills Standalone and reads as a failed install; silence reads as broken; a fold-down is deferred v1.1 territory. |
| 2 | Verify-ping design | Pink noise **200 Hz–8 kHz**; **−20 dBFS RMS / −6 dBFS peak**, fixed, independent of `outputGain` and trims; 20 ms raised-cosine envelope; latched manual step with a **120 s** safety timeout; auto-cycle **1.2 s on / 0.4 s gap**, 1→8; injected **at the channel map** with all other channels hard-zeroed; **no level control exposed** | −20 dBFS is the universal monitor-calibration reference, so it is a number every engineer already reads as safe in a rig they do not own; injecting at the map means a ping failure has exactly one possible cause. |
| 3 | Numeric defaults | Blur: the 0-1 range **is** the cap, `r_s = blur·0.5·rigScale`, absolute backstop 8 m; `d_i` floor **0.05 m**, `static_assert`ed; hull atten **linear dB/m floored at −24 dB**; air `fc = clamp(20000·2^(−airAmount·d_hull/3), 500, 20000)`, `dRef = 3 m`, **`airAmount = 0` skips the filter**; meters attack **0.5** / decay **0.12**, −60..0 dBFS, 1.5 s peak-hold; smoothing **5 ms** per sample | Every value is either derived from the paper's own normalisation (blur), asserted rather than assumed (`d_i` floor, blur cap), or chosen so the control automates predictably in the units it is labelled with. |
| 4 | Default venue scale | Hall **13.0 × 22.0 m**, stage y ∈ [0, 4]; speaker span 12.0 m across × 15.0 m deep; **graded** heights 4.50 → 5.40 m; `rakeFront` **1.10 m**, `rakeRear` **3.20 m`; derived `rigScale ≈ 7.95 m` | Derived from the one hard fact available (255 seats → ~19 seats/row × ~13 rows in a 13 m width ≈ 22 m depth); published Barnett dimensions do not exist online. Heights are graded, not uniform, **specifically so a dropped `z` term cannot pass DSP-01 vacuously.** |
| 5 | Venue storage | **Separate `ValueTree`**, attached as a child of `apvts.state` | Decisive argument is repo-specific: `pattern_preset_apply_needs_reset_to_defaults` (reset all params to defaults before applying a preset) is in **direct contradiction** with FUNC-05 if the venue lives in the APVTS. Also: automatable routing is a performance hazard, and FUNC-02 needs exact float round-trip. |

Full reasoning for each is recorded in `ARCHITECTURE.md` §10.

---

## Parameter-Count Discrepancy — Resolved

**Finding: 18 was an arithmetic slip. 17 is correct. No 18th parameter was intended, and none is
being added.**

The BRIEF.md musical-parameter table has **10 rows**, one of which (`w1..w8`) collapses 8
parameters. Correct expansion: `9 + 8 = 17`. The figure 18 is obtained by `10 rows + 8 weights` —
counting the collapsed weight row **and** its eight expansions. That is the only arithmetic that
yields exactly 18 from that table.

Cross-check: the venue table's 42 is correct precisely because none of its four rows is counted as
both itself and its expansion. The two tables' error behaviour is consistent with the double-count
hypothesis and with nothing else.

All three candidate 18th parameters were evaluated and rejected:
- **hull bypass bool** — `hullAtten = 0` defeats it exactly and is automatable; a parallel bool
  creates a UI ambiguity and duplicates state across the preset boundary
- **air bypass bool** — rejected as a parameter but **accepted as an implementation requirement**:
  `airAmount = 0` must *skip the filter and reset its state*, not merely set `fc = 20 kHz`, because a
  one-pole at 20 kHz is not transparent at 44.1/48 kHz
- **custom plugin bypass** — hosts provide this; JUCE surfaces it via `getBypassParameter()`

**Action taken:** BRIEF.md's "18 musical parameters" corrected to 17 with a footnote; STATUS.md
corrected. **The binding count for Stage 1 is 17.**

---

## Key Decisions Carried Forward

| Decision | Where |
|---|---|
| 2D convex hull, 3D DBAP distances — a 3D hull of a near-coplanar rig is a thin slab and every elevated source would read as outside | ARCHITECTURE §3.1.1, AD-1 |
| Fixed 64-sample absolute-sample-aligned control grid | §3.6, AD-2 |
| Venue in a separate `ValueTree` child of `apvts.state` | §4.1, AD-3 |
| Label map keyed on `ChannelType`, stored by name — a slot index would make `getChannelIndexForType()` a tautology and destroy the safety property | §3.2.3, AD-4 |
| Accept 7.1 + 7.1-SDDS + 5.1.2 | §3.2.2, AD-5 |
| Always two stereo sub-points at `0.5` feed each; `width = 0` is the degenerate case, not a branch | §3.4.3, AD-6 |
| `rFade` spread collapse near the centroid | §3.4.2, AD-7 |
| Hull attenuation linear in dB/m, floored at −24 dB | §3.5.1, AD-8 |
| Air filter pre-matrix, per sub-point (2 instances, not 8) | §3.5.2, AD-9 |
| SAFE pass-through on a stereo output bus | §OQ1, AD-10 |
| Double-buffered POD `VenueSnapshot`, not `atomic<shared_ptr>` | §3.6.6, AD-11 |
| 17 musical parameters, no 18th | §11, AD-12 |
| Meters read the **written buffer post-map** so a map error is visible on the plan | §4.3, AD-13 |
| Room envelope derived from the speaker bbox — keeps the venue count at exactly 42 | §6.2, AD-14 |

---

## Risk Register (carried into Stage 1)

| ID | Risk | Severity | Mitigation |
|---|---|---|---|
| R1 | **Speaker→buffer channel map** — silent failure, passes `auval`, `pluginval` 10 and every test that does not look for it; audible only in the hall | **CRITICAL** | Single construction site, `ChannelType`-keyed, permutation-validated; three-layer test (runtime invariant → source-parsed golden with committed SHA → offline tone-per-speaker render); verify-ping through the same map; meters post-map |
| R2 | Logic negotiates a different 8-channel set than expected | HIGH | Accept all three containers; `ChannelType`-keyed map; surface the negotiated set name in the UI. Settled at Stage 4 |
| R3 | Block-size invariance vs per-block solve | HIGH | 64-sample absolute-aligned control grid + explicit QUAL-03 test protocol |
| R4 | Convex hull degeneracy from real/typo'd measurements | MEDIUM | Dedup pre-pass, area-scaled epsilon, explicit `m < 3` paths, classification surfaced in the UI |
| R5 | Centre-crossing L/R flip | MEDIUM → resolved | `rFade` spread collapse (found at design time) |
| R6 | Sticky NaN in the air filter (only recursive element) | MEDIUM | Per-block `std::isfinite` check + `reset()`; clamp `fc` before reset so bad coefficients cannot cause sticky silence |
| R7 | Two-screen WebView UI is the largest in the repo | MEDIUM | Stub-render first; grep-diff the native-fn bridge; explicit canvas sizing + DPR backing store. Descope UI-04/UI-05 to v1.1 if needed |
| R8 | Venue never measured | LOW (project) | Coherent default venue; label it unmistakably as a placeholder |

**Overall: MEDIUM-HIGH**, driven almost entirely by R1 and R2 — both silent, both in territory this
repo has been burned by before.

---

## Constraints for Stage 1

- Target name **`OuariconOctagon`**, folder `plugins/O-Octagon`, `PRODUCT_NAME "O-Octagon${OUARICON_DEV_SUFFIX}"`,
  `PLUGIN_CODE OuOc` (verified unused across all 39 existing plugins)
- **`VERSION 1.0.0`**, never `PLUGIN_VERSION` — the latter is silently ignored and ships 1.0.0
- **No `PLUGIN_CHANNEL_CONFIGURATIONS`** — counts only, not types; breaks surround detection
- `juce::juce_dsp` must be linked (`FirstOrderTPTFilter`)
- `BusesProperties` in the constructor member-initialiser list, never `prepareToPlay()`
- **Must not link SAF** — O-Octagon has no VBAP and no SAF dependency, unlike its sibling O-Orbit
- 17 `AudioParameterFloat`; zero Choice, zero Bool in the musical set
- No parameter ID shadows a `juce::` free function (checked)

---

## Next Steps

1. **UI mockup** — two screens, Room + Venue. Due before Stage 3.1; not a blocker for Stage 1 or 2.
2. **Stage 1: Foundation** — `/plugin-discuss O-Octagon` then the Stage-1 phase commands, or
   `/implement O-Octagon` for orchestrated mode.
3. **Measure Roy Barnett Recital Hall** — 8 × (x, y, z) in metres plus front-row and rear-row ear
   heights. A project dependency, not a code dependency: the default venue makes everything
   testable beforehand.
4. **Stage 4 feedback loop** — record which 8-channel container Logic actually negotiated, and the
   bounce-order and LFE-gain test results, back into
   `research/logic-pro-multichannel-octaphonic-dbap.md` §6/§6a, which currently mark those MEDIUM
   confidence.
