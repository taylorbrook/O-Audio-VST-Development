# Stage 2 — DSP · Phase 2.1 (Geometry Core) — Context

**Plugin:** O-Octagon
**Stage:** 2 of 4 — DSP
**Phase:** 2.1 of 3 — Geometry Core
**GSD phase:** discuss
**Date:** 2026-08-11
**Branch:** `feat/o-octagon` @ `f135b3da`
**Participants:** Taylor Brook, Claude

---

## Entry Check — carried obligations from Stage 1

Stage 1 verify carried one process obligation into every stage boundary: *"Re-verify all four
checksums at every stage boundary — a checksum that silently points at the wrong file is worse than
no checksum, because it reports green."* (`pattern_promotion_checksum_pins_replaced_file`)

**Re-run at this boundary. All four byte-exact against STATUS.md frontmatter:**

| Contract | SHA-256 | Result |
|---|---|---|
| `BRIEF.md` | `697a4f32…9fbd6` | ✅ matches |
| `parameter-spec.md` | `b45f88dc…b9e02f` | ✅ matches *(the value corrected at `f135b3da`)* |
| `research/ARCHITECTURE.md` | `bff8a83b…06cfe` | ✅ matches |
| `ROADMAP.md` | `aec7d0ce…7ee29` | ✅ matches |

No contract drift. Phase 2.1 plans against these exact documents.

---

## Discussion Summary

Stage 2's architecture is unusually closed. ARCHITECTURE.md resolves all five open questions
(OQ1–OQ5), specifies the hull algorithm to the epsilon, specifies the channel-map construction to
the line, fixes every numeric default, and fixes two design defects (the centre-crossing L/R flip,
the PERF-02 ↔ QUAL-03 incompatibility) before code existed. Discuss therefore did **not** re-open
algorithm choice. It settled four things the architecture deliberately left to execution.

---

## Requirements Confirmed

**Phase 2.1 verifies:** COMPAT-03, DSP-03, DSP-04, FUNC-03 *(per ROADMAP Stage 2 → Phase 2.1)*

- **COMPAT-03** — speaker→buffer map built once in `prepareToPlay()` via `getChannelIndexForType()`;
  zero hardcoded channel indices anywhere; test asserted against **parsed JUCE source**, not a
  mirrored constant
- **DSP-03** — convex hull by a proper algorithm with explicit collinearity handling; traced layout
  yields vertices **1, 2, 4, 5, 6, 7** with speakers 3 and 8 classified `ON_EDGE`; outside sources
  projected to the nearest hull point; degenerate venues finite and non-crashing
- **DSP-04** — sloped audience plane; `srcZ = 0` tracks ear level from `rakeFront` to `rakeRear`
- **FUNC-03** — 8-row label map with a sane shipped default; duplicate/missing assignment detected
  and surfaced, never silently routed

**Not verified in this phase** (2.2 / 2.3): FUNC-01, FUNC-07, DSP-01, DSP-02, DSP-05–08, PERF-01,
PERF-02, QUAL-01–04.

---

## Approach Decisions

| # | Decision | Choice | Rationale |
|---|---|---|---|
| D1 | Stage 2 GSD cycle granularity | **One full cycle per phase** — 2.1, then 2.2, then 2.3 | The roadmap's phase design exists so each phase's test gate closes before the next depends on it. The channel map is R1 (CRITICAL, silent failure, audible only in the hall) and gets its own verify before any gain math exists to confuse a diagnosis. |
| D2 | Task 13 (Logic 8-ch negotiation) | **Run before Phase 2.1 execute** — not before discuss/research/plan | If Logic fails to negotiate 8 channels the fault is in the bus predicate; unpicking that after geometry code exists costs materially more. Nothing in discuss/research/plan depends on the result. |
| D3 | Phase 2.1 test vehicle | **Stand up BOTH `tests/` unit target AND `tests/render-harness/` now** | The harness costs little once the target exists, and 2.2's block-size and channel-map-Layer-3 gates then have somewhere to land the moment the solver compiles. It also closes Stage 1 issues 3 and 4 (see *Carried in*, below) a phase earlier than planned. |
| D4 | DBAP reference implementation (for 2.2's DSP-01 gate) | **Python script in `tests/tools/`**, emitting a committed fixture of *(position → 8 gains)* that the C++ test loads | A separate language forces an independent transcription of the 2011-04-14 equations from the paper. A C++ reference written beside the implementation mirrors the same misreading and passes — `pattern_test_fixture_mirrors_drift_silently`. Scaffolded at 2.1 alongside `gen_juce_channel_order.py`; **consumed** at 2.2. |

### D1 consequence — artifact naming

Because Stage 2 runs three cycles inside one stage directory, artifacts are **phase-suffixed** so
the stage-level commands still resolve `2-dsp`:

```
stages/2-dsp/
  CONTEXT-2.1.md   RESEARCH-2.1.md   PLAN-2.1.md   SUMMARY-2.1.md   VERIFICATION-2.1.md
  CONTEXT-2.2.md   …
  CONTEXT-2.3.md   …
  VERIFICATION.md          ← stage-level, written only at the close of 2.3
```

`STATUS.md` carries the active phase so each command knows which suffix to write.

---

## Constraints Identified

### Locked — do NOT re-litigate

Inherited from prior research and Stage 0/1. ARCHITECTURE §14 states it as *"If you change one thing
about §3.2, change nothing."*

- `AudioChannelSet` is a **bitset**; buffer order is **ascending enum-bit order** — not
  initializer-list order, not CoreAudio wire order (`critical_audiochannelset_is_a_bitset_not_an_order`)
- The map is stored as a **`ChannelType`** (by abbreviated name string in the venue tree), never as a
  slot index. Storing an index makes `getChannelIndexForType()` an identity function and the entire
  safety property evaporates. **Stage 2 must not "simplify" this.**
- **One construction site** — `rebuildChannelMap()`, called from `prepareToPlay()` and on label-map
  edit. `speakerToBuffer` is the only thing that ever indexes an output channel.
- **Permutation-validated** — the map must be a permutation of 0..7. A duplicate label repeats a
  target index; a missing label yields `-1`. Both fail; the map is **not applied**, the last valid
  map is retained, and `mapInvalid` (atomic) drives a persistent UI warning.
- Hull is 2D on the **(x, y) floor projection** while DBAP distances are fully 3D — a deliberate
  asymmetry (ARCHITECTURE §3.1.1). A source at `srcZ = +8 m` above room centre is **inside** the
  hull and receives no hull attenuation and no air filtering. Intended, not a bug.
- Monotone chain pops collinear points (`<= EPS_CROSS`) → strict vertices only. `EPS_CROSS` is an
  **area** tolerance, `1e-6 · spanX · spanY`, so it scales with the room.
- Venue = separate `ValueTree` child of `apvts.state` (root `OOctagon`, never renamed), **not** APVTS
  parameters. Message-thread only, never touched by a musical preset.
- Audio thread reads the venue through a **double-buffered POD `VenueSnapshot`** with a
  release/acquire generation counter — never a `ValueTree`, never a lock, and deliberately **not**
  `std::atomic<shared_ptr>` (`pattern_retired_map_reaper_rt_free`).
- Default venue = ARCHITECTURE §OQ4. The **graded** speaker heights (4.50 → 5.40 m) are load-bearing:
  a uniform default `z` would make every `(z_i − z_s)` difference identical and hide a dropped `z`
  term in DSP-01's test. Do not flatten them.

### Repo constraints that bite this phase

- **`-Wswitch-enum` bans switching on `AudioChannelSet::ChannelType`** (~60 enumerators; warns even
  with a `default:`). The label map must be a table or an `if`-chain.
- Stage 1 sessions carry **no** `VENUE` child, so `readVenueFromState()` must treat a missing or
  partial node as *"use defaults"* — not as an error, not as zeros.
- Zero-span guards are required on both the bbox denormalisation and `earHeight(y)`; a venue with
  `rakeFront == rakeRear` or a degenerate bbox must produce finite results.
- No `setLatencySamples()` anywhere — `getLatencySamples()` is non-virtual in JUCE 8.

---

## Carried in from Stage 1 verify — obligations this phase inherits

| # | Obligation | Where it lands in 2.1 |
|---|---|---|
| C1 | **A container-only channel-map test is VACUOUS.** F1 established that all three accepted 8-channel containers have initializer order == enum-bit order, so a hardcoded 0..7 map is byte-identical to a correct one under every accepted layout. Only a **non-identity `map1..map8` label assignment** permutes the buffer. | Every channel-map test must drive non-identity label maps. Layer 3 (2.2) inherits this too. |
| C2 | **Retire the `PHASE-2.2-REPLACE` block** at `PluginProcessor.cpp:188-223` — the single hardcoded output index in the plugin. "Retire it, do not grandfather it." | 2.1's *"zero hardcoded output indices outside `ChannelMap`"* grep gate is expected to fail against it until it is removed. See *Open Questions* Q1. |
| C3 | **Bound every output loop by `buffer.getNumChannels()`** — never `8`, never `getTotalNumOutputChannels()`, which is the accessor that lies under F3 (`pattern_standalone_canonical_channelset_oob`). | Preserved through the 2.1 rewrite; exercised by the harness (see C5). |
| C4 | **Stage 1 issue 3** — unity gain through the placeholder is confirmed by *inspection only*. No offline measurement exists. | Closed by the D3 harness: a unity-gain-through-all-outputs case. |
| C5 | **Stage 1 issue 4** — the F3 hazard (3–7 output channels) is *reasoned, not measured*; no such interface was available. | Closed by the D3 harness: construct 1-, 2- and 8-output bus layouts programmatically; no hardware needed. |
| C6 | **Contract checksums** — re-verify all four at every stage boundary. | Done above; repeat at 2.1 verify. |

---

## Components in Scope (ROADMAP Phase 2.1)

- `Source/Data/VenueModel.{h,cpp}` — 42-value `ValueTree` store, bounding box, centroid, `rigScale`,
  `earHeight(y)` with the zero-span guard, default venue per §OQ4
- `Source/DSP/ConvexHull2D.{h,cpp}` — monotone chain, dedup pre-pass (`EPS_DEDUP = 1e-4 m`),
  `<= EPS_CROSS` collinear pop, area-scaled epsilon, CCW winding assert, classification
  (`VERTEX` / `ON_EDGE` / `INTERIOR`), inside test, nearest-point projection, full §3.1.6 degeneracy
  matrix
- `Source/DSP/ChannelMap.{h,cpp}` — `rebuildChannelMap()` per §3.2.3, permutation-validated,
  `mapInvalid` atomic
- `VenueSnapshot` double-buffer + generation counter (§3.6.6)
- `VENUE` child attached to `apvts.state`; session round-trip extended
- `tests/` unit target + `tests/render-harness/` + `tests/tools/gen_juce_channel_order.py`
  *(D3; harness pulled forward from 2.2)*

**Explicit non-goals for 2.1** — these belong to 2.2/2.3 and must not appear:
`DbapSolver`, `GainStage`, the 64-sample control grid, `SmoothedValue` targets, `SourceShaper`,
`HullProcessor`, `VerifyPing`, any WebView editor.

---

## Open Questions — for the research phase

**Q1 — What replaces the `PHASE-2.2-REPLACE` block at 2.1, given no solver exists yet?**
C2 says retire it, but 2.1 produces no gain vector. The natural answer is to route the mono sum
through `ChannelMap` — all 8 lanes still carry identical signal, but the hardcoded index is gone and
the map is exercised by real audio a phase early. Needs confirming against the grep gate's exact
wording and against C3's loop bound. *Recommended, not decided.*

**Q2 — How does `gen_juce_channel_order.py` locate the JUCE modules portably?**
The root `CMakeLists.txt:36-41` resolves JUCE via `$ENV{JUCE_DIR}`, else `C:/JUCE`, else
`/Users/taylorbrook/JUCE`. CI does not use the local tree
(`critical_juce_vendor_overrides_for_ci` — the patch script cannot patch upstream zips). The
generator must resolve the module path from CMake at configure time rather than assume a path, or
Layer 2 becomes a local-only gate that silently no-ops in CI.

**Q3 — Does the unit target run under CI, or locally only?**
Layer 2's whole value is *"fails the build until a human re-blesses it"* on a JUCE upgrade. That is
only true if the test is wired into the build CI actually runs. Scope decision for research: add to
`.github/workflows/build-and-release.yml` now, or land it local-only at 2.1 and wire CI at Stage 4.

**Q4 — Where does hull classification surface before the Venue screen exists?**
`VERTEX`/`ON_EDGE`/`INTERIOR` is specified as a Venue-screen readout (Stage 3.2). At 2.1 it needs a
non-UI sink for the DSP-03 gate — a test-only accessor is the obvious answer, but it must not become
dead code the UI later duplicates.

**Q5 — Which unit-test framework?**
No repo-wide convention is evident across the 12 plugins carrying `tests/`. Research should pick the
one already used by the closest analog rather than introduce a new dependency.

---

## Risks Active in This Phase

| Risk | Severity | Status in 2.1 |
|---|---|---|
| **R1 — speaker→buffer channel map** | CRITICAL | **Front-loaded here by design.** Silent failure, passes every automated gate, audible only in the hall. Layers 1 and 2 land in this phase; Layer 3 at 2.2. Mitigation is C1: non-identity label maps, always. |
| R4 — convex hull degeneracy | MEDIUM | Fully in scope; the §3.1.6 matrix is the gate. |
| R8 — venue measurement never happens | LOW (project) | Unblocked: §OQ4 defaults make everything testable without the hall. Roy Barnett measurement remains outstanding and is **not** a code dependency. |

---

## Next Phase

**Ready for:** research phase — Phase 2.1 (Geometry Core)

Research should answer Q1–Q5 and produce `RESEARCH-2.1.md`. It should **not** re-derive the hull
algorithm, the channel-map construction, the venue schema or the numeric defaults — all four are
fully specified in ARCHITECTURE §3.1, §3.2, §4.1 and §OQ3/OQ4 and are contract-checksummed above.
