# Stage 2 — DSP · Phase 2.2 (DBAP Solve and Gain Application) — Execute Summary

**Plugin:** O-Octagon
**Stage:** 2 of 4 — DSP
**Phase:** 2.2 of 3 — DBAP Solve and Gain Application
**GSD phase:** execute
**Date:** 2026-08-11
**Branch:** `feat/o-octagon` @ `a47cef88`
**Plan:** `PLAN-2.2.md` — 13 tasks

---

## Entry / exit check — contract checksums

Re-computed at **both** boundaries of this phase, not read out of `PLAN-2.2.md` (C6,
`pattern_promotion_checksum_pins_replaced_file`). All four byte-exact against `STATUS.md` frontmatter
on arrival **and** on departure — no contract was edited during execute.

| Contract | SHA-256 | Entry | Exit |
|---|---|---|---|
| `BRIEF.md` | `697a4f32…f6b9fbd6` | ✅ | ✅ |
| `parameter-spec.md` | `b45f88dc…cbb9e02f` | ✅ | ✅ |
| `research/ARCHITECTURE.md` | `cd881a10…4b10861b` | ✅ (D2 re-pin) | ✅ |
| `ROADMAP.md` | `aec7d0ce…0137ee29` | ✅ | ✅ |

---

## Outcome

**Audio spatialises.** 12 of 13 tasks complete. Task 12 (the manual Logic gate, D4) is **HUMAN and
open** — it is corroboration, not the gate for FUNC-03/3, and does not block verify from starting.

**46 probes across two test targets, 0 failures.** Every gate was re-run from scratch here; none was
read out of a prior document.

| | Phase 2.1 | Phase 2.2 | Total |
|---|---|---|---|
| Unit target (`O-Octagon-geometry-test`) | A–P (16) | **V–AH (13)** | **29**, exit 0 |
| Render harness (`O-Octagon-render-test`) | Q′, R–U (5) | **AI–AT (12)** | **17**, exit 0 |

All of A–U still pass alongside the new work. Probe **Q was re-specified before it ran**, exactly as
the plan predicted — see *Predicted and confirmed* below.

---

## Gate results — actual output, all re-run at execute

| # | Gate | Result |
|---|---|---|
| 1 | Clean 3-format build (VST3 + AU + Standalone) + both test targets, forced TU recompile | ✅ **0 warning/error matches in the entire log**; 114 O-Octagon compile/link lines |
| 2 | Hardcoded output channel indices outside `ChannelMap` | ✅ **zero** — see the audit below |
| 3 | `PHASE-2.3-WIDTH` / `-AIR` / `-TRIM` each exactly once in `Source/` | ✅ **1 / 1 / 1**; `PHASE-2.2-REPLACE` still **0** |
| 4 | `auval -a` then `auval -v aufx OuOc OuDv` | ✅ **AU VALIDATION SUCCEEDED** |
| 5 | pluginval strictness 10, VST3 ×3 and AU ×3 | ✅ **SUCCESS on all six**, exit 0 each, **zero `FAILED`** |
| 6 | Both test targets with `-DOUARICON_BUILD_TESTS=ON` | ✅ exit 0 / exit 0 — 29 + 17 probes, 0 failures |
| 7 | `gen_dbap_reference.py --check` | ✅ exit 0 — 102 cases, committed fixture matches its generator |
| 8 | 17 parameters vs `parameter-spec.md` | ✅ **17/17** on name, range and default, compared programmatically; **2.2 adds none** |
| 9 | `setLatencySamples` / `switch` on `ChannelType` / `createEditor` guard | ✅ absent / absent / `#if JUCE_WEB_BROWSER` present |
| 10 | `OOCTAGON_INSTRUMENT` never defined by the plugin target | ✅ **0** in the plugin `CMakeLists.txt`; defined only by the two test targets |

### Gate 2, stated precisely

`grep` returns two literal-index hits, and **both are INPUT reads**:

```
GainStage.cpp:210   const float* const in0 = numIn > 0 ? buffer.getReadPointer (0) : nullptr;
GainStage.cpp:214   const float* const in1 = numIn > 1 ? buffer.getReadPointer (1) : in0;
```

§3.4.3 requires exactly this (`s_L = 0.5·L`, `s_R = 0.5·R`), and both are bounded by `numIn`. There
are exactly **two output writes in the whole plugin**: one through
`snapshot.speakerToBuffer[i]`, one through a loop variable bounded by `jmin (numOut, 8)`. A loop
variable is not a hardcoded index. Every remaining bare `8` is an array dimension, a named constant,
or doc prose.

### Gate 5 — one benign warning, already on record

pluginval AU emits `!!! WARNING: Current program is -1... Is this correct?` on every run. This is JUCE
AU-wrapper reporting, present across the repo, recorded as benign at Stage 1 and again at 2.1 verify.
The runs still return `SUCCESS` with exit 0 and **no line containing `FAILED`**.

---

## PERF-01 criterion 1 — the method, not just the verdict (H8)

**This must not be summarised as "RT-safety harness pass ✓". It would overstate what ran.**

| Sub-claim | How it was established |
|---|---|
| No **allocation** in `processBlock` | **MEASURED.** Probe AO replaces the entire global `operator new` family — plain, `[]`, both `std::align_val_t` overloads, and every matching `operator delete` — with counting `malloc`/`posix_memalign` wrappers. **0 allocations across 66 `processBlock` calls**, including an 8192-sample over-size block and a mid-stream venue edit. Warmed up with one un-counted `processBlock` first, so libc++/JUCE first-touch initialisation is not attributed to the audio path. |
| No **lock**, no **file I/O** | **GREP + INSPECTION ONLY.** Not measured. |
| `-fsanitize=realtime` | **UNAVAILABLE** — unsupported by Apple clang 17.0.0, verified at research by running it. |

An un-replaced aligned-new would be silently uncounted, and a probe that counts nothing passes — which
is why every variant is replaced rather than just the common two.

---

## Key measurements

| Probe | Measurement | Gate |
|---|---|---|
| **Y** — DBAP vs the Python oracle | **max \|impl − oracle\| = 1.0236e-7** over 102 cases (worst: `random-22`) | 1e-5 hard gate — **and DSP-01's 1e-6 IS MET** |
| **AA** — `Σ v_i² = 1` | max \|Σv²−1\| = **3.2590e-7** over **7686 solves** (rolloff 3–6 × blur 0–1, 61 inside / 122 outside per pair) | 1e-6 |
| **AE** — pow budget | **16** per solve pair, asserted `== 16` not merely `≤ 32` | PERF-02/3 |
| **AP** — solve/projection counts | inside proj **0**; idle solves **0** (pow **0**); param-change solves **1** (pow **16**); outside proj **2**; venue-edit solves **1** | PERF-01/3, PERF-02/1,2 |
| **AO** — allocation | **0** across 66 blocks | PERF-01/1 |
| **AS** — zipper | bound 0.0041677; per-sample **position 0.0008846**, **weights 0.0016529**; **negative control 0.0564730 / 0.1057403 (both OVER)** | QUAL-04/1,2 |
| **Q′** — unity through the addressed lane | max \|out − in\| = **6.0e-8**, loudest other lane **0.000000000** | 1e-6 relative |
| **AI** — independence | **0 duplicate pairs of 28**, closest lanes differ by 0.00398982 | FUNC-01/3 |
| **AK** — DSP-04/3 | rear source delta **0.00522166** (moved); front source delta **0.0000000000** (unchanged) | both halves |
| **AQ** — H1 regression | gain vector moved by **0.03991491** after a venue edit published between two `processBlock` calls | P16 |
| **AL / AM / AN** | **bit-identical by `memcmp`**, never a tolerance | QUAL-03 |

**Probe AS's negative control is the point.** A probe never observed to fail is a probe whose failure
path is untested. Sampling the same rendered series at 64-sample control boundaries — which is what
the output would look like with the vector held per control block instead of ramped — exceeds the
bound by 13× and 25×. The sweep is genuinely fast enough to be a test.

---

## Deviations from PLAN-2.2 and from the contracts

Three, all deliberate. The first two were planned; the third was not and is recorded here rather than
left to be discovered.

### 1. `VenueSnapshot::generation` — beyond ARCHITECTURE §3.6.6's field list *(planned, P16)*

The generation counter moved **inside** the snapshot payload and is stamped by `publish()` before the
release store. The separate `std::atomic<uint32_t>` is gone; `getGeneration()` survives as a
message-thread diagnostic that the audio thread must not read.

Same class of deviation as 2.1's `hullEpsCross`, and for the same reason: two acquires would let a
`publish()` landing between them hand the control block the **new** geometry with the **old**
generation, which is then stored as `lastSolvedGeneration` so every later block compares equal —
**the venue edit is present in the snapshot and the solve never runs against it, permanently.**
Probe AQ measures it.

### 2. `SourceShaper` created at 2.2 — beyond ROADMAP's placement *(planned, P15)*

ROADMAP assigns `Source/DSP/SourceShaper.{h,cpp}` to 2.3, but 2.2 cannot solve without §5 step 2 and
the per-sub-point rake resolution. §3.4.1 is implemented **in full**, steps 1–6, including `rFade` and
the `(0,−1)` fallback bearing; the degeneracy is entirely in the caller, which passes a literal
`0.0f` at the `PHASE-2.3-WIDTH` marker.

### 3. `OOctagonProcessor::getAPVTS()` added *(NOT in the plan)*

The render harness needs the synchronous `getParameter(id)->setValueNotifyingHost(...)` write path
that RESEARCH-2.2 Q1 mandates, and `apvts` was private.

Recorded because it widens the public API. It is not a test-only hook: `getAPVTS()` is the accessor
name used by **12+ sibling plugins in this repo**, and the Stage 3.1 WebView editor's parameter
relays will need it regardless. No behaviour changed.

---

## Findings worth carrying

### F1 — `juce::String (const char*)` is ASCII-only; concatenation is UTF-8

`juce_String.h:88-94` states it outright, and `String::operator+= (const char*)` appends through
`CharPointer_UTF8` (`juce_String.cpp:773`). So

```cpp
detail = "… — not NaN …";     // MANGLED: renders as "â"
detail << "… — not NaN …";    // correct
```

Found by reading the probe output, not by a compiler warning — there is none. Every non-ASCII detail
string in both test mains is now built by concatenation. Worth knowing before Stage 3 puts µ, ×, Σ or
° into UI strings.

### F2 — the §OQ4 rig is NOT exactly mirror-symmetric once it is float32

The rig mirrors about x = 6.5 m on paper. In float32, `0.5f` and `12.5f` are exact and sit exactly
±6.0 either side, so pairs (1,2), (3,8) and (4,7) are bit-identical. But `9.8f` and `3.2f` land at
9.80000019 and 3.20000005 — i.e. 3.30000019 and 3.29999995 from the axis — so **pair (5,6) is not**.
The double-precision oracle shows the asymmetry plainly (the fixture's `inside-centre` case differs
between speakers 5 and 6 in the 9th significant figure); in float32 it currently sits *below one ULP*
and both round to the same value.

Probe AF therefore asserts **bit-identity only for the three genuinely exact pairs** and holds (5,6)
to a tolerance. Tightening it to bit-identity because today's build happens to satisfy that would
pin a rounding accident.

### F3 — the puck can never leave the speaker bounding box, and (0,0) is a hull VERTEX

`srcX`/`srcY` are normalised into the **speaker bbox**, which is also the hull's bounding box, so the
only outside-hull region reachable by the parameters is near the bbox corners. Worse, the obvious
"outside" guess `srcX = 0, srcY = 0` denormalises to exactly speaker 1 — and `hull::isInside` is an
inside-**or-on** test, so it reports inside and **no projection ever fires**.

That is correct behaviour and a silently vacuous probe. AP now **searches** for a normalised position
the plugin's own hull calls outside, and asserts one was found before relying on it — the same
discipline probe S applies to the F3 hazard.

### F4 — a helper that leaves only the last block in its destination must return the offset

`renderSteady()` overwrites its destination each chunk, so it holds the **tail** of the render. Probe
Q′ compared it against `noiseAt(0..511)` — the **head** — and failed at 0.96 while routing was
provably perfect (`loudest other lane 0.000000000`). Against a position-deterministic excitation this
kind of window mismatch reads exactly like a gain bug. The helper now returns the absolute start index
of the block it leaves behind.

### F5 — `posix_memalign`, not `std::aligned_alloc`

libc++ only exposes `std::aligned_alloc` behind `_LIBCPP_HAS_ALIGNED_ALLOC`. A probe that fails to
**build** on one toolchain is a probe that silently stops running, so the aligned `operator new`
replacement uses `posix_memalign` (`free()` is the correct deallocator for both).

---

## Predicted and confirmed

Nothing below was discovered at execute; each was named in `PLAN-2.2.md` before any code was written.

- **Probe Q failed as written**, at `max |out − in| = 0.344283581`, on the first run after the solver
  went live. The plan predicted this and specified the replacement: with `w = δ_ij` the denominator is
  `w_j²·d_j^(−2a)`, so `k = d_j^a` and `v_j = 1` **analytically**. Q′ asserts lane
  `speakerToBuffer[j]` reproduces the input at **1e-6 relative, not bit-exact** — the `k = 1/√(t²)`
  round trip is not guaranteed exact in single precision and a bit-exact gate would be flaky rather
  than strict. Measured: **6.0e-8**.
- **`powCalls == 16` exactly**, not merely under the 32 budget.
- **The QUAL-04 probe carries a working negative control**, and it fires.
- **Three `PHASE-2.3-*` markers, each exactly once.** Two rounds were needed: the first pass had each
  marker appearing **twice** because prose comments elsewhere quoted the literal token. A prose
  mention would survive the real marker's retirement and keep 2.3's "zero occurrences" gate non-zero —
  grandfathering exactly what the mechanism exists to retire. The prose now says "the Phase 2.3 width
  marker" rather than quoting the token.
- **`QUAL-04` closes ⚠️ PARTIAL**, criterion 3 (`width`) staged to 2.3 — declared at *discuss*.

---

## Requirement outcomes

`REQUIREMENTS.md` is updated at **verify**, not here.

| Req | Expected at verify | Evidence |
|---|---|---|
| **FUNC-01** | ✅ **complete** | AI (independence, off-centre, non-identity map) + AJ |
| **FUNC-03** | ✅ **complete** — criterion 3 closes | AJ (Layer 3, mandatory) |
| **DSP-01** | ✅ complete | X (α), Z (the z term), **Y (1.0236e-7, meets 1e-6)** |
| **DSP-02** | ✅ complete | AA — 7686 solves, 3.26e-7 |
| **DSP-04** | ✅ **complete** — criterion 3 closes | AK, **both halves** |
| **DSP-05** | ✅ complete | AB, AC |
| **PERF-01** | ✅ complete | AO (allocation, measured), AP — **read the H8 note above** |
| **PERF-02** | ✅ complete | AE, AP, AT (`sampleAdvances == totalSamples`) |
| **QUAL-02** | ✅ complete | AC, AD, AR (**parameter** NaN, not only input NaN) |
| **QUAL-03** | ✅ complete | AL (mandated), AM (ragged — the real gate), AN |
| **QUAL-04** | ⚠️ **PARTIAL** — 2/3 | AS closes 1 and 2. **Criterion 3 (`width`) → 2.3**, declared at discuss |
| **DSP-08** | **implemented, NOT ticked** | AG is supporting evidence only; the traceability table assigns DSP-08 to 2.3 |

---

## Files

**New (10)**

- `Source/Data/VenueGeometry.h` — `oo::plane::earHeight` / `normToMetres` + the shared `kMinSpan`
- `Source/DSP/DbapSolver.{h,cpp}` — §3.3 verbatim, raw inputs, four instrumentation counters
- `Source/DSP/SourceShaper.{h,cpp}` — §3.4.1 steps 1–6 in full
- `Source/DSP/GainStage.{h,cpp}` — the control grid, 17 smoothers, §3.6.4 inner loop
- `tests/tools/gen_dbap_reference.py` — the independent oracle + `--check`
- `tests/fixtures/DbapReferenceFixture.h` — **committed**, 102 cases
- `.planning/stages/2-dsp/SUMMARY-2.2.md`

**Modified (10)**

- `Source/Data/VenueModel.{h,cpp}` — `kMinSpan` aliases `oo::plane::kMinSpan`; three one-line delegates
- `Source/Data/VenueSnapshot.h` — the generation stamp (deviation 1)
- `Source/PluginProcessor.{h,cpp}` — indexed parameter array, derived defaults, `GainStage`,
  `processBlock` rewrite, `getAPVTS()` (deviation 3)
- `CMakeLists.txt`, `tests/unit/CMakeLists.txt`, `tests/render-harness/CMakeLists.txt`
- `tests/unit/main.cpp` (+13 probes), `tests/render-harness/main.cpp` (Q′ + 12 probes)

`.github/workflows/build-and-release.yml` **untouched**, per the standing rule.

---

## Open at the close of 2.2

1. **Task 12 — the manual Logic gate (D4). HUMAN, ~10 minutes, before 2.2 verify closes.**
   Not a blocker for starting verify, and *not* the gate for FUNC-03/3 (that is probe AJ, which
   passed). Two observations to record verbatim, positive or negative:
   - automate `srcX` across the room and confirm the 8 surround-meter lanes **no longer move in
     lockstep** — the direct contrast with 2.1, where identical signal on all 8 was correct by design;
   - set `w3 = 0` and confirm that lane goes **silent** while the others compensate.
2. **QUAL-04 criterion 3** (`width`) → 2.3, as declared at discuss.
3. **CI gap** — unchanged from 2.1. No test target in this repo has ever run in CI, so every gate
   above only fires for whoever configures with `-DOUARICON_BUILD_TESTS=ON`. Logged at
   `.planning/todos/pending/2026-08-11-secrets-free-test-ci-workflow.md`; belongs to Stage 4.
4. **Owed at 2.3 discuss** — FUNC-07, DSP-06, DSP-07 and DSP-08 have summary rows in
   `REQUIREMENTS.md` and **no acceptance criteria**. They must be written **before 2.3 plan**, or 2.3
   verifies against nothing.

---

## Next phase

**Ready for:** verify — Phase 2.2. `VERIFICATION-2.2.md`. The stage-level `VERIFICATION.md` is
written only at the close of 2.3.
