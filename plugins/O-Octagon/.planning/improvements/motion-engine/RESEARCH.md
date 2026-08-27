# Research: Motion Engine — generative trajectories for the source puck

**Plugin:** O-Octagon
**Milestone:** motion-engine
**Created:** 2026-08-27
**Phase:** Research
**Base version:** 1.7.0 → target 1.8.0 (MINOR, confirmed)

---

## Context Summary

From CONTEXT.md:
- **Core requirement:** six generative paths (Orbit, Figure-8, Sweep, Drift, Pendulum, Spiral) produce an
  (x, y, z) offset in **venue metres** added downstream of `srcX/srcY/srcZ`; the parameters stay the
  anchor (D1/D2); tempo-synced and PPQ-locked (D7/R4); trace + ghost anchor + live puck on the map (D6).
- **Scope:** the seven decisions D1–D7 are closed. This phase settles the seven open implementation
  questions and maps integration points.
- **Hard gates:** motion OFF is bit-identical to v1.7.0 (R3, digest probe); two bounces identical
  including Drift (R4); block-size invariant (R5).

---

## The seven open questions — settled from the codebase

### Q1 — Control rate: ride the existing 64-sample absolute grid; NEVER integrate per call

`GainStage::process()` (`GainStage.cpp:297-340`) walks an **absolute-sample-aligned 64-sample control
grid**: `phase = absoluteSampleCounter & 63`, and `updateControl()` runs at every `phase == 0`, with the
render chunked to grid boundaries regardless of the host buffer. That grid is the block-size-invariant
control rate this plugin already proved (probes AL/AM/AN/BI/CT/CX).

**O-Orbit's trap, precisely:** `MotionEngine::advance(numSamples)` reads state once per host block and
integrates `phaseAccumulator += 2π·f·numSamples/sr` — so both the sample point *and* the increment depend
on the buffer size (`pattern_block_rate_envelope_breaks_blocksize_invariance`). Its PPQ lock hides this
only while synced and rolling.

**Decision:** phase is a **pure function of absolute position**, evaluated at each grid boundary:

| Mode | Phase source | Why it is invariant |
|---|---|---|
| Free (Hz) | `t = absoluteSampleCounter / sr`; `cycles = rateHz · t + phaseBase` | `absoluteSampleCounter` is grid-aligned and host-independent |
| Synced, rolling | `cycles = (ppqBlockStart + gridOffsetSamples/sr · bpm/60) · mult` | The grid boundary's PPQ is extrapolated from the block-start PPQ, so a 1024-sample block reaches the same boundaries as sixteen 64-sample blocks |
| Synced, stopped / no PPQ | **hold the last synced phase** (Free mode keeps free-running) | See Q7 |

Perlin `noiseTime = cycles` the same way. **No accumulator survives across calls** except the
continuity re-base below.

**Rate change continuity (Free mode only):** a position-derived phase jumps when `rateHz` changes. At the
grid boundary where the new rate is first observed, re-base: `phaseBase += (oldRate − newRate) · t`. The
observation point is a grid boundary — absolute — so it is deterministic and block-size invariant. In
Sync mode a division change may jump; that is the downbeat lock working, not a defect.

**Zipper:** no new smoothing needed. Positions already step every 64 samples with the 17 gain smoothers
ramping 5 ms (`kControlBlock` comment, `GainStage.h:130`), and the existing sweep probes (e.g. the
sinusoidal srcX/srcY/srcZ sweep at `main.cpp:2125-2127`) certify moving sources at exactly this cadence.
At the worst case (4 Hz, 12 m radius) a source moves ≈0.4 m per grid — well inside what the ramps
already handle for hand automation.

**RNG:** `PerlinNoise` is stateless after `seed()` (a 512-entry perm table; `noise(x)` is pure), so
`pattern_rng_stream_interleave_blocksize` does not apply — there is no stream to interleave.

### Q2 — Injection point: a metres entry into SourceShaper, and a BRANCH at motion-off

`shape()` (`SourceShaper.cpp:29-35`) does `p = plane::normToMetres(bbox, srcXNorm, srcYNorm)` as step 1
and everything after is in metres. Split at that seam:

```cpp
SubPoints shape   (v, srcXNorm, srcYNorm, srcZ, widthMetres);      // unchanged signature, old path
SubPoints shapeAt (v, Vec2 metres,        srcZ, widthMetres);      // NEW: steps 2-6 from a metres pair
```

`shape()` becomes `return shapeAt (v, normToMetres(...), srcZ, width);`. Same TU, same arithmetic,
inlined — but **the CU-style digest probe is what decides whether the refactor is bit-transparent**, not
this argument. If the v1.7.0 digest moves, fall back to leaving `shape()` byte-identical and adding
`shapeAt` beside it (accepting the six-step duplication — at that point the digest is the anti-drift gate).

In `updateControl()` (`GainStage.cpp:777-830`):

```cpp
if (motionOn)  { p_m = normToMetres(...) + offset.xy;  z = p[srcZ] + offset.z;  sub = shapeAt (...); }
else           { sub = shape (snapshot, p[srcX], p[srcY], p[srcZ], width); }      // v1.7.0 path, verbatim
```

**A branch, not `+ 0.0f`.** Adding a zero offset is numerically transparent (only −0 vs +0 differs, and
nothing downstream is sign-of-zero sensitive), but the branch makes R3 *structural* — the v1.7.0 arithmetic
is not touched at all when motion is off, which is the same argument v1.7.0 made for `MonitorFold`
("nothing is clocked") and v1.5.0 for `decorr = 0`.

**Two places take Z, not one:** `shape()` gets `srcZ` for the sub-point heights, and `solveSubPoint()`
takes `p[params::srcZ]` **separately** for the proximity level cue (`GainStage.cpp:828-829`). Both must
receive the effective Z. Missing the second one would move the source without moving its level cue —
an audible defect no digest catches.

**The dirty check** (`GainStage.cpp:794`) memcmps the parameter snapshot and skips the solve when nothing
changed. With motion on the offset changes every grid, so the skip must be bypassed: `if (haveSolved &&
!motionOn && generation == last && memcmp == 0) return;`. At motion-off the predicate is the v1.7.0
predicate. PERF-02's skip-when-unchanged survives for every session that does not use motion.

### Q3 — SAFE mode: motion runs; it is inaudible there by construction, and that is fine

The unmapped branch of `renderChunk()` (`GainStage.cpp:630+`) never reads a position — SAFE mode is a
defined mono/stereo fold of the input. Motion therefore has **no audible effect** in SAFE mode without
any refusal logic. Let the engine run: the map still shows the trace and the live puck, which is the
correct UX (the composer is designing a gesture the host cannot currently render), and there is nothing
to contaminate. No `isSafeMode()` gate, no banner change.

### Q4 — Width sub-points and decorr: rigid translation, gate untouched

`shapeAt` receives the moved puck and derives the bearing from the **rig centroid** (step 2), so the
pair translates rigidly *and* its spread axis re-orients as the puck orbits — the same behaviour a
hand-automated puck has today. The rFade collapse (step 3, `kFadeFraction = 0.05`) fires when a path
crosses the centroid, exactly as it does for automation, so the 180° axis flip stays continuous.
`decorrWanted = decorr > 0 && wEff > 0` (`GainStage.cpp:492`) reads `subPoints.wEff` and is unaffected.
Probe CW (decorr inert at `wEff == 0`) should be re-run with motion on as a regression check, nothing more.

### Q5 — The D5 safety limit: bound by parameter RANGES, not a runtime clamp

`hullTrimGain()` (`HullProcessor.h:113-118`) floors at `kTrimFloorDb = −24 dB`. At the default
`hullAtten = 1 dB/m` the floor is reached 24 m outside the hull; at the max 3 dB/m, at 8 m. Beyond the
floor distance only the DBAP rolloff and the air LPF (also distance-driven, Nyquist-safe by H4) still
change — nothing goes non-finite. So there is no physical reason for a clamp; the bound is the ranges:

| Parameter | Range | Rationale |
|---|---|---|
| `motionSize` (path extent, m) | 0 – 24 m, default 6 m | 24 m ≈ the default envelope's long side (15.6 × 19.5 m); radius 12 m from an edge anchor reaches ~20 m outside — past the −24 dB floor at any `hullAtten` ≥ 1.2 |
| `motionHeight` (m, amplitude) | 0 – 8 m, default 0 | Matches `srcZ`'s +8 m ceiling; `srcZ + height·sin(t)` spans −10..16 m worst case, already inside what the solver handles for automation |

The only guard that stays is the existing ingestion sanitiser pattern (`snapshotParameters`, P17): the
offset is built from sanitised floats and `sin/cos/fbm` of finite inputs, so it cannot be non-finite.

### Q6 — The trace comes from the SAME C++ generator, via a native function; the live puck rides getMeters

**Trace:** a new native function `getMotionTrace` evaluates the C++ path generator on the message thread
for N points over one cycle (N = 128; Drift excluded — see below) and returns **anchor-relative metres**.
JS projects with the existing `metresToPx()` (`roomplan.js:74`) — the one projection function section 19
of the static gate already polices — after adding the anchor's metres from the same `normToMetres()` the
puck uses. Translation is the only JS arithmetic, so there is no second implementation of a path
(`pattern_test_fixture_mirrors_drift_silently`). The generator must be header-only and JUCE-free
(`Source/DSP/MotionPath.h`, like `VenueGeometry.h`) so the editor, the audio thread **and the fast unit
target** all compile the identical function.

Re-fetch on `valueChangedEvent` of the shape parameters only (path, size, ratio, angle, height, phase);
the anchor and rate do not change the trace's shape. This is the "real-time parameter response" the
CONTEXT asks for.

**Live puck:** the audio thread publishes the current offset (x, y, z metres) as three `std::atomic<float>`
(same idiom as the meters). It rides the **getMeters** poll (~30 Hz, `app.js:59`) as three extra floats,
NOT getStatus (2 Hz — a puck at 2 Hz stutters). No new native function for the read. The ghost anchor
is the existing `#puck` element restyled hollow when motion is on; the live puck is a new element on the
same `#plan-controls` layer. The elevation strip (`elevation.js:283-290`) should draw the live height the
same way from the polled Z.

**Drift on the map:** Perlin drift has no closed cycle to trace and its future depends on PPQ. Show a
**trailing tail** of the last ~48 polled positions instead of a trace (JS ring of received points, drawn
as a fading polyline). Honest, and it needs no second generator.

### Q7 — Stopped transport and no-PPQ fallback

| Situation | Behaviour |
|---|---|
| Free mode (Sync = Free) | Always runs from `absoluteSampleCounter` — Standalone, stopped or rolling |
| Synced, host rolling with PPQ | PPQ-derived (Q1) |
| Synced, transport stopped | **Hold** the last synced phase. The puck rests where playback will resume, which is what a composer scrubbing a cue wants; O-Orbit free-runs here and the ring drifts away from the bar it will snap back to |
| Synced, host supplies no PPQ (Standalone) | Free-run from the absolute counter at `bpm/60 · mult` using `hostBpm` default 120 |

`absoluteSampleCounter` resets in `prepare()` — a re-prepare restarts Free-mode motion from phase 0,
which is deterministic and matches every other piece of DSP state (RESEARCH-2.2 H9).

**Plumbing:** `processBlock()` never calls `getPlayHead()` today (grep: zero hits). Read it once per block
exactly as O-Orbit does (`O-Orbit/PluginProcessor.cpp:561-574`: `getPosition()` → `getBpm()`,
`getPpqPosition()`, `getIsPlaying()`), pack a `HostClock { bpm, ppq, ppqValid }` and pass it into
`gainStage.process()` beside `monitorOn`. GainStage keeps its rule of being handed facts (P24).

---

## Implementation Approach

### Recommended Approach

**Strategy:** "Grid-evaluated metric offset" — a header-only, JUCE-free path generator evaluated at each
64-sample control boundary from absolute position, injected into `SourceShaper` through a metres entry,
with the identical generator serving the map trace.

Three new source files, all header-only or JUCE-free where the unit target needs them:

- `Source/DSP/PerlinNoise.h` — copied from O-Orbit (there is no shared module; every plugin hand-copies,
  `pattern_no_shared_knob_module_two_families` is the precedent). Add a `constexpr` note naming the source.
- `Source/DSP/MotionPath.h` — `struct MotionParams`, `Vec3 evaluate (const MotionParams&, double cycles,
  const PerlinNoise&) noexcept`, and the six path equations (below). No JUCE, no allocation.
- `Source/DSP/MotionClock.h` — the phase derivation of Q1 (`cyclesAt (absoluteSample, sr, HostClock,
  MotionParams)`) plus the Free-mode continuity re-base. Kept separate from the equations so the unit
  target can test "same cycles at 64/256/1024" without a render.

**Path equations (Cartesian, anchor-relative, R = size/2, t = 2π·frac(cycles) + phaseDeg, rotated by
`angle`):**

| Path | x(t), y(t) | Notes |
|---|---|---|
| Orbit | `R cos t`, `R·ratio·sin t` | O-Orbit case 0 re-derived; ratio 1 = circle |
| Figure-8 | `R sin t`, `R·ratio·sin 2t` | Lissajous 1:2 — closes at t = 2π (R1) |
| Sweep | `R·(2·fold − 1)`, `0` with `fold = u<0.5 ? 2u : 2−2u` | O-Orbit cases 2+4 merged; the fold removes the saw wrap (R1) |
| Drift | `R·fbm(n)`, `R·ratio·fbm(n+1000)`, n = cycles | O-Orbit case 3; `perlin.seed(seed)` |
| Pendulum | `R sin t`, `0` | O-Orbit case 1 |
| Spiral | `R·s·cos t`, `R·ratio·s·sin t`, `s = fold(u)` | Winds in over the first half-cycle, out over the second — continuous at both ends |
| all | `z = height · sin t` | D4; Drift uses `height·fbm(n+2000)` |

`angle` rotates (x, y) about the anchor so Sweep and Pendulum are not pinned to the room's x axis —
without it two of the six paths are near-useless in a portrait room. It was not in the CONTEXT's
control list and the plan should add it (one float, 0–360°).

**Parameter group `motion` (10 parameters, kCount 18 → 28):**

| id | type | range / choices | default |
|---|---|---|---|
| `motionOn` | Bool | — | off |
| `motionPath` | Choice | Orbit, Figure-8, Sweep, Drift, Pendulum, Spiral | Orbit |
| `motionSync` | Choice | Free, 1/16 … 4 Bars (O-Orbit's 14 divisions) | Free |
| `motionRate` | Float | 0.01 – 4 Hz, log skew | 0.1 Hz |
| `motionSize` | Float | 0 – 24 m | 6 m |
| `motionRatio` | Float | 0 – 1 | 1.0 |
| `motionAngle` | Float | 0 – 360 ° | 0 |
| `motionHeight` | Float | 0 – 8 m | 0 |
| `motionPhase` | Float | 0 – 360 ° | 0 |
| `motionSeed` | Float, step 1 | 1 – 64 | 1 |

`ParamSnapshot` is a float array read through `getRawParameterValue`; Bool and Choice raw values are
floats (0/1 and the index), so the snapshot, the memcmp dirty check and `snapshotParameters()`'s
sanitiser all work unchanged. The `params::Index` enum and `id()` table extend together (static_assert
guards the pairing); `kCount == 18` static_asserts at `GainStage.h:73`, `ui_frontend_check.js:651/662/707`
and the `PresetPolicy.h` partition all move to 28.

**Pros**
- R3 is structural (branch), R5 is inherited from the proven grid, R4 is a pure function of PPQ.
- One generator, three consumers (audio, trace, unit test) — no JS reimplementation.
- No new smoothing, no new allocation, no change to `DbapSolver`, `HullProcessor`, `MonitorFold`.

**Cons**
- Two new relay TYPES in the editor (see Pattern Analysis) — the first break in the all-float discipline.
- The Room screen's controls column is a fixed 1100 × 720 budget already policed to the pixel; a
  ten-control group does not fit as a plain `.group` (see Risk 1).

### Alternative Approaches Considered

**Alternative 1: port `MotionEngine::advance(numSamples)` per host block**
- Why not: integrates per call; fails R5 by construction (the CONTEXT already flagged this).

**Alternative 2: sub-divide inside the plugin at a fixed control rate with its own smoothing**
- Why not: the 64-sample absolute grid *is* that — a second grid would be a second thing to keep
  aligned, and the existing gain ramps already smooth the 64-sample position steps.

**Alternative 3: `setValueNotifyingHost()` on srcX/srcY/srcZ**
- Rejected at D1; noted only because it would have avoided the shaper change.

**Alternative 4: all-float motion parameters to keep the single relay list**
- Why not: `motionPath` in a host automation lane must read "Figure-8", not 0.2. The cost (one
  `WebComboBoxRelay` + one `WebToggleButtonRelay`, each with its attachment) is small and the O-ReverseDelay
  relay-type bug is avoided by the gate extension in Pattern Analysis.

---

## Affected Components

### Files to Modify

| File | Changes | Complexity |
|---|---|---|
| `Source/DSP/GainStage.h` | `params::Index` + `id()` +10; `kCount` 18→28; `HostClock` struct; `process()` signature; `MotionClock`/`PerlinNoise` members; three live-offset atomics | Medium |
| `Source/DSP/GainStage.cpp` | `updateControl()` branch + dirty-check bypass; effective Z into both `shape`/`solveSubPoint` sites; clock evaluation at `phase == 0`; publish offset | Medium |
| `Source/DSP/SourceShaper.h/.cpp` | `shapeAt (v, Vec2, z, width)`; `shape()` delegates | Low (gated by digest) |
| `Source/PluginProcessor.h/.cpp` | `motion` parameter group; `getPlayHead()` read per block → `HostClock`; `motionLivePosition()` accessor; `makeChoice`/`makeBool` helpers | Medium |
| `Source/Data/PresetPolicy.h` | 10 ids into `kPreserved` (a preset never starts or stops motion — same argument as `srcX`) | Low |
| `Source/PluginEditor.h/.cpp` | `WebComboBoxRelay` ×2 + `WebToggleButtonRelay` ×1 and attachments in declaration order; `getMotionTrace` native fn; `getMeters` +3 floats; native count literal 26→27 | Medium |
| `Source/ui/public/index.html` | `#group-motion` — **before** `#group-elevation` (gate §40: elevation is last) | Medium (layout) |
| `Source/ui/public/js/app.js` | bind toggle/combo relays (`Juce.getToggleState`/`getComboBoxState`); `FORMAT` entries; trace fetch on shape-param change; forward meters' motion floats | Medium |
| `Source/ui/public/js/roomplan.js` | trace `<path>` on `#plan-geometry`; ghost anchor class; live puck element; Drift tail ring | Medium |
| `Source/ui/public/js/elevation.js` | live source height from polled Z | Low |
| `Source/ui/public/js/i18n.js` | EN + FR hover help for 10 controls; French geometry re-measured (v1.6.0 gate sweeps both languages) | Low |
| `Source/ui/public/css/styles.css` | motion group, trace stroke, ghost/live puck styles | Low |
| `tests/ui-stub/juce-stub.js` | `RANGES` +10; `NATIVE_FNS.getMotionTrace`; meters payload | Low |
| `tests/ui_frontend_check.js` | literals 18→28, 26→27; relay-order check extended to the three vectors | Low |
| `tests/ui_layout_check.js` | new section: motion group fits; §21/22 re-measured | Medium |
| `tests/render-harness/main.cpp` | `HarnessPlayHead : juce::AudioPlayHead`; `renderInto` advances PPQ; probes DC–DJ (below) | High |
| `tests/unit/main.cpp` | path analytics + clock invariance (see Probes) | Medium |
| `tests/unit/CMakeLists.txt` | nothing to add if the three new files are header-only | — |
| `CMakeLists.txt` | `VERSION 1.8.0` | Trivial |
| `.planning/parameter-spec.md`, `CHANGELOG.md`, `NOTES.md` | 28-parameter table; v1.8.0 entry | Low |

### New Files

| File | Purpose |
|---|---|
| `Source/DSP/PerlinNoise.h` | Byte-copy of O-Orbit's (header-only, no JUCE) |
| `Source/DSP/MotionPath.h` | `MotionParams` + the six equations; the single generator |
| `Source/DSP/MotionClock.h` | Absolute-position phase derivation, PPQ extrapolation, Free-mode re-base |

### Dependencies

**JUCE:** `juce::AudioPlayHead::PositionInfo` (juce_audio_processors) — new use; `WebComboBoxRelay`,
`WebToggleButtonRelay`, `WebComboBoxParameterAttachment`, `WebToggleButtonParameterAttachment`
(juce_gui_extra) — new use. No new modules linked; no SAF (CMake header already forbids it).

**External:** none.

---

## Pattern Analysis

### Patterns to follow (in-repo precedent)

1. **v1.5.0 `decorr` / v1.7.0 monitor — the "off is structural" shape.** A default that selects the
   old code path, a digest probe against the previous binary (CU), and an instrumentation counter that
   proves the new code never ran (`oo::instr::decorrSamples`). Add `oo::instr::motionSolves`.
2. **`VenueGeometry.h` — one header, three threads.** `MotionPath.h` follows it exactly; that is what
   makes the trace non-drifting and the unit test cheap.
3. **`getMeters` as the fast poll, `getStatus` as the slow one** (Q4/P77 split) — live puck on the fast one.
4. **v1.3.0 preset migration hook** — NOT needed: no range moved. Pre-1.8.0 presets omit the ten keys;
   WR-01 resets them to defaults (`motionOn = 0`) and `loadPreserving` holds them across a load anyway.
   Sessions: APVTS fills a missing parameter with its default. R7 is satisfied without a version gate;
   the plan should still add a probe that loads a v1.7.0-shaped preset and asserts motion-off.
5. **Relay derivation from `params::id(i)`** (`PluginEditor.cpp:274`, gate §16) — keep the float loop
   derived; the three non-float relays are the exception and the gate must be taught the split
   explicitly: assert `sliderRelays` covers exactly `kCount − 3` and name the three.

### Anti-patterns to avoid

- **Per-block integration** (O-Orbit `advance`) — Q1.
- **A JS path generator** — Q6.
- **A trace that draws from `srcX/srcY` after motion** — the anchor never moves; the trace translates
  with the anchor, the live puck rides the poll.
- **Adding `z` only to `shape()`** — the proximity cue takes Z separately (Q2).
- **The hyphen** — `figure-8` must not be a filename; `MotionPath.h` is hyphen-free
  (`critical_binary_data_strips_hyphens` for any UI asset too).
- **A one-shot push of motion state at init** — every motion read is a pull or a poll
  (`pattern_webview_one_shot_state_push_stale_on_preset_load`).

---

## Risk Assessment

| # | Risk | Severity | Mitigation |
|---|---|---|---|
| 1 | **The controls column cannot take ten more controls.** Frame is fixed 1100 × 720; `.group-body` is a 2-col grid; the elevation strip is the flex:1 last child and layout §21/22 measure it to the pixel (699 vs 592 fired once already). A plain group of 5 rows (~150 px) will overflow. | High | Plan must choose a layout before coding: **recommended** — a compact group: row 1 `On` toggle + `Path` select; row 2 `Sync` select + `Rate`; rows 3–4 the six knobs as a 3-col mini-grid; `Seed` shown only when Path = Drift. Alternatively a Motion *tab* inside the Position group. Measure with `ui_layout_check` on the stub FIRST (the v1.5.0 precedent: one extra cell was a deliberate, measured cost). |
| 2 | `shape()` refactor moves the v1.7.0 digest | Medium | Capture the v1.7.0 digest BEFORE editing (build the harness at tag `v1.7.0-O-Octagon`, run the CU scenario + one motion-scenario-shaped run, transcribe). If the refactor moves it, keep `shape()` verbatim and add `shapeAt` beside it. Never re-record from the new build (`pattern_reanchor_cross_version_digest_probe`). |
| 3 | PPQ extrapolation across a tempo change mid-block | Low | Tempo ramps are rare and re-sync at the next block start; document as accepted. |
| 4 | First break in the all-float relay list re-opens the O-ReverseDelay relay-type bug | Medium | Gate §11/§16 extended to the three new vectors; browser-stub exercise of both the combo and the toggle; the `ui-stub` must stub `getComboBoxState`/`getToggleState` (`pattern_juce_webview_backend_stub_direction`). |
| 5 | French copy for ten new tooltips at the frame edge | Low | The v1.6.0 clamp gate sweeps both languages; budget the French pass explicitly. |
| 6 | `kPreserved` growth changes `loadPreserving`'s restore loop size | Low | Array sized from the constexpr list; probe CP re-runs at 22 preserved. |
| 7 | Harness `renderInto` gains a playhead; every existing probe must render identically with `ppqValid = false` | Medium | Run the full 61-probe suite BEFORE adding any motion probe, with the playhead installed and motion parameters at default. |

---

## Probe plan (for PLAN.md to pin down)

Render harness (extends 61 → ~70):

| Probe | Claim | Type |
|---|---|---|
| DC | motion-off digest == v1.7.0 binary, `motionSolves == 0`, live | R3 anchor (CU shape) |
| DD | `srcX/srcY/srcZ` read back bit-unchanged across a full cycle with motion on | R2 |
| DE | 64 / 256 / 1024 / ragged block renders memcmp-identical, Free mode, each path | R5 |
| DF | same as DE, Sync 1/4 @ 120 BPM with the harness playhead rolling | R5 + R4 |
| DG | two renders of one session identical, Drift, seed 7 | R4/D7 |
| DH | render starting at PPQ 37.5 equals the tail of a render from 0 (mid-timeline start) | R4 |
| DI | host `srcX` step moves the anchor and the whole path (offset invariant under anchor change) | R2/D1 |
| DJ | Sync + stopped holds phase; Free + stopped keeps moving | Q7 |
| NC | delete the `motionOn` branch → DC fails, DE still passes (targets the branch: `pattern_probe_must_target_the_branch_the_fix_changed`) | negative control |

Unit target (geometry-test, seconds not minutes):

- Figure-8 closes: `evaluate(t=0) == evaluate(t=1)` bit-exact; Sweep fold has no discontinuity
  (max step over 4096 samples < 2·R/2048); Orbit at ratio 1 has `|p| == R` to 1e-6; Spiral continuous at
  the half-cycle; `angle = 90°` maps Sweep's x onto y.
- `MotionClock::cyclesAt` at absolute sample N is identical whether reached by 1×N or by 64-sample steps.

---

## Domain and version

- **Domain:** mixed — DSP (control grid, shaper, clock) and GUI (three new relay types, trace, live
  puck, layout budget). Per the detection rule both scores are non-zero → **general-purpose** executes.
- **Version bump:** MINOR — 1.7.0 → **1.8.0**. Ten new parameters, a new APVTS group, a new native
  function; no range moves, no schema change (venue stays at 2).

---

## Open items for PLAN

1. Layout decision for Risk 1 — measure on the stub before any C++.
2. Confirm `motionAngle` joins the control list (research recommends yes; it was not in CONTEXT D3).
3. `motionSize` semantics: full extent (diameter) vs radius — research assumes **extent** so "6 m" reads
   as the ring's width on the map.
4. Drift on the map as a tail rather than a trace (Q6) — confirm.

---

*Generated by improve-milestone research phase*
