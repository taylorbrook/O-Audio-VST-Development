# Context: Motion Engine — generative trajectories for the source puck

**Plugin:** O-Octagon
**Milestone:** motion-engine
**Created:** 2026-08-26
**Phase:** Discuss
**Base version:** 1.7.0 → target 1.8.0 (MINOR, provisional — confirmed at plan)

---

## Original Request

> O-Octagon Motion engine: generative trajectories for srcX/srcY/srcZ — circular and elliptical
> orbits, figure-8s, line sweeps, random walks, tempo-synced rate/depth/phase — written as
> parameter modulation inside the plugin rather than hand-drawn host automation. O-Orbit in this
> repo already owns the path vocabulary to borrow.

This is item 4 of `.planning/FEATURE-REVIEW.md` — `priority: high` · `effort: large`, and the
review's own words: *"the biggest creative gap, but a milestone rather than an improvement."*
Its stated justification: hand-drawing Logic automation for a 10-minute rotation is the most
tedious task in the current workflow, and every comparable tool (ControlGRIS's trajectory
designer, L-ISA's snapshot trajectories, even free ReaSurroundPan's parameter-modulation
randomness) makes motion cheap.

---

## The seven decisions taken in this phase

Each was a real fork with a live alternative. Recorded here because the research and plan phases
must not silently re-open them.

### D1 — Motion is an internal OFFSET, not a parameter write

`srcX` / `srcY` / `srcZ` remain the **anchor**. The motion engine produces a displacement applied
downstream of the parameters; the host lanes stay flat.

```
srcX/srcY/srcZ  ──anchor──┐
                          ├─► + motion(t) ─► SourceShaper ─► DBAP
MotionEngine ──offset─────┘
```

**Rejected:** `setValueNotifyingHost()` on the three position parameters. It would make motion
recordable as ordinary automation, but it collides with any automation already on those lanes and
floods the host every block.

**Consequences the plan must honour:**
- Nothing in the position lanes changes, so **existing sessions and presets are unaffected** and
  hand-drawn automation continues to work — it moves the anchor, and the path travels with it.
- An offline bounce is deterministic without the host having to replay anything.
- The anchor is no longer the audible position, so the UI must show both (see D6).

### D2 — Paths are drawn in VENUE METRES

The path is centred on the anchor and sized in metres: `Size = 6 m` is a 6 m orbit in any hall.
This matches the rest of O-Octagon's metric surface (`rolloff` dB/2x, `hullAtten` dB/m, `srcZ` in m,
`width` in m).

**Rejected:** normalised 0–1 units (silently rescales when a `.venue` is swapped) and
hull-inscribed paths (venue-aware for free, but the shape stops being dialable).

**Note for research:** `srcX`/`srcY` are normalised against the **speaker bounding box** —
`SourceShaper.cpp:35` calls `plane::normToMetres (v.bbMinX, v.bbMaxX, v.bbMinY, v.bbMaxY, …)`.
A metric offset therefore wants to be added *after* that denormalisation, not before it.

### D3 — Six paths ship in v1.8.0

| # | Path | Source |
|---|------|--------|
| 1 | Orbit — circle/ellipse, `Ratio` 0–1 | port of O-Orbit case 0, re-derived in Cartesian |
| 2 | Figure-8 — Lissajous 1:2 | **new** — O-Orbit has no Lissajous |
| 3 | Sweep — line with ping-pong fold | O-Orbit cases 2 + 4, merged |
| 4 | Drift — Perlin fBm random walk | port of O-Orbit case 3 |
| 5 | Pendulum — single-axis swing | straight port of O-Orbit case 1 |
| 6 | Spiral — winds in/out over the cycle | **new** |

The brief named four (orbit, figure-8, sweep, random walk); Pendulum and Spiral were added because
Pendulum is nearly free (O-Orbit already has it) and Spiral is two lines on top of Orbit.

**Rejected for this milestone:** a user-drawn polyline path traversed by arc length. Closest to
ControlGRIS and the most expressive option, but it needs path storage in preset state, an editing
UI, and arc-length parameterisation — materially larger. Deferred, not dropped.

### D4 — Height (`srcZ`) is COUPLED to the path phase

One phase accumulator drives all three axes:

```
phase t ─┬─► X = f(t)
         ├─► Y = g(t)
         └─► Z = Height · sin(t)
```

An orbit becomes a tilted ring; a figure-8 becomes a 3D lobe. `Height` is its own range control
(metres, matching `srcZ`'s −2..8 m).

**Rejected:** an independent vertical rate (more expressive, non-repeating 3D figures — but a
second accumulator to keep PPQ-locked) and plan-view-only (no vertical gesture at all).

### D5 — The path MAY leave the speaker bounding box

The offset is added in metres downstream of `normToMetres`, so a large orbit genuinely flies out
over the audience and back. `hullAtten` and `rolloff` already model out-of-hull sources — this is
the physically honest behaviour in a DBAP venue model, **and it is the one gesture no host
automation can draw**, because `srcX`/`srcY` cannot exceed 0–1. A generous safety limit bounds it.

**Rejected:** clamping to the bounding box (safe, but an oversized orbit silently flattens into a
rounded rectangle and the gesture is lost) and auto-fitting `Size` to the venue (re-introduces
exactly the swap-time silent rescale that D2 chose metres to avoid).

### D6 — The venue map draws the TRACE and a GHOST anchor

Motion panel with the controls, plus the map renders the full path as a faint trace, the anchor as
a hollow ghost, and the live puck travelling it.

```
┌───── venue map ───────┐  ┌─ MOTION ─────┐
│ ●       ●       ●     │  │ On    [●  ]  │
│   ·············       │  │ Path  Orbit ▾│
│  ·    ◌      ·        │  │ Rate  ● 1/2  │
│  ·  anchor  ●←live    │  │ Size  ● 6.0m │
│   ·············       │  │ Ratio ● 0.60 │
│ ●       ●       ●     │  │ Phase ● 0°   │
└───────────────────────┘  └──────────────┘
```

Seeing the trajectory before hearing it is what makes generative motion dialable instead of
guesswork.

**Rejected:** panel-only with a moving puck (smallest surface, but `Size`/`Ratio` are dialled
blind) and drag-handles on the trace itself (most fluid, largest UI job — hit-testing, handles,
gesture-to-parameter writes). Direct manipulation is deferred with the custom path (D3).

### D7 — Drift is DETERMINISTIC, with a `Seed` parameter

Perlin time derives from host PPQ — O-Orbit's C1 lock, `MotionEngine.cpp:26-33` — so the same bar
always gives the same wander and two bounces of a session are identical. A `Seed` parameter lets
the composer shop for a shape and keeps it in the preset. Free-run only when the transport is
stopped or supplies no PPQ (Standalone).

**Rejected:** free-running drift (a living quality some composers want, but no render is ever
reproducible or A/B-able) and a fixed seed (one wander shape, take it or leave it).

---

## Requirements

### R1 — Six trajectories, correct in venue metres

- **Description:** Orbit, Figure-8, Sweep, Drift, Pendulum, Spiral generate an (x, y, z) offset in
  metres from a single phase accumulator, per D3/D4.
- **Acceptance:** each path traced offline at a known rate reproduces its analytic shape; Figure-8
  closes (returns to origin at cycle end); Sweep's ping-pong fold has no saw-wrap discontinuity
  (the reason O-Orbit has a separate case 4).

### R2 — Anchor semantics preserved exactly

- **Description:** `srcX`/`srcY`/`srcZ` are never written by the engine (D1).
- **Acceptance:** with motion running, the three parameters read back unchanged across a full
  cycle; automating `srcX` from the host moves the anchor and the whole path follows.

### R3 — Motion OFF is bit-identical to v1.7.0

- **Description:** with the engine disarmed the signal path must be untouched — no offset, no
  extra smoothing, no re-ordered arithmetic.
- **Acceptance:** a render digest at `motionOn = 0` matches the v1.7.0 binary's own digest for the
  same session, **bit-for-bit**. This is the v1.5.0 `decorr = 0` precedent (probe CU) and it is a
  hard gate, not a nice-to-have.

### R4 — Tempo sync and PPQ determinism

- **Description:** rate is settable free (Hz) or as a tempo division; when synced and the transport
  rolls, phase derives from `ppqPosition` rather than free-running (O-Orbit's C1 lock).
- **Acceptance:** two offline bounces of the same session produce identical output for every path
  including Drift (D7); motion is downbeat-aligned; a bounce started mid-timeline lands at the
  correct phase.

### R5 — Block-size invariance

- **Description:** the rendered result must not depend on the host's buffer size.
- **Acceptance:** the same session rendered at 64 / 256 / 1024 samples produces the same output
  within tolerance when synced, and identical phase at block boundaries.
- **Why this is called out:** O-Orbit's engine advances **once per block** and reads its state once
  per block. That is a known trap in this repo — see `pattern_block_rate_envelope_breaks_blocksize_invariance`
  and `pattern_rng_stream_interleave_blocksize`. The research phase owns the decision between
  per-block-with-PPQ-derivation and a smoothed control-rate sub-division; it is **not** a free port.

### R6 — Path trace, ghost anchor, live puck on the venue map

- **Description:** per D6.
- **Acceptance:** the trace matches the audible path (same generator, not a second JS
  reimplementation that can drift — see `pattern_test_fixture_mirrors_drift_silently`); the ghost
  anchor sits at the parameter position; the live puck is on the trace at all times.

### R7 — Full preset and automation coverage

- **Description:** every motion control is an APVTS parameter, so presets carry motion and every
  control is host-automatable (rate, depth/size, phase, path, seed).
- **Acceptance:** save/load round-trips motion state; a preset written before v1.8.0 loads with
  motion off and the position untouched (R3).

### Scope boundaries

**In scope**
- The six paths of D3, in venue metres, offset-applied (D1/D2/D5)
- Phase-coupled height (D4)
- Free/tempo-synced rate with the PPQ lock, plus `Seed` (D7)
- Motion parameter group in the APVTS, preset-carried and automatable (R7)
- Venue-map trace + ghost anchor + live puck (D6)
- Bit-identity gate at motion-off (R3)

**Out of scope — deferred, not dropped**
- User-drawn polyline paths (D3 rejected branch)
- Drag-handles on the trace for direct Size/Ratio/rotate manipulation (D6 rejected branch)
- Baking the traced path into host automation lanes (D1 rejected branch — noted as a future
  companion to the offset model, which is what makes a bake tractable at all)
- Multiple independent motion layers / per-axis LFOs
- OSC-driven motion (separate FEATURE-REVIEW item)
- Snapshot/cue morphing (separate FEATURE-REVIEW item)

---

## User preferences

### Interaction style
- [x] Real-time parameter response — the trace must update as `Size`/`Ratio`/`Path` are dialled
- [x] Preset-based switching — motion state travels in presets (R7)
- [x] Automation-friendly — every motion control automatable; position lanes deliberately left free

### Quality priorities
1. **Determinism and render reproducibility** — an offline bounce must be repeatable (R4/D7); this
   plugin's whole use case is preparing a fixed-media piece away from the venue
2. **Non-contamination** — motion off is bit-identical to v1.7.0 (R3)
3. **Dialability** — you can see what you are about to hear (D6)
4. CPU efficiency last: this is a control-rate generator, not a per-sample DSP block

### Breaking changes
- [ ] OK to break existing presets
- [x] **Must maintain preset compatibility** — pre-v1.8.0 presets load with motion off
- [x] **Must maintain automation compatibility** — this is the load-bearing reason for D1

---

## Plugin context

### Current state
- **Version:** 1.7.0 (2026-08-26)
- **Stage:** 4 — `stage_4_complete`, all four stages complete
- **Last change:** v1.7.0 binaural/stereo monitoring fold-down — closed the *other* HIGH-priority
  gap in FEATURE-REVIEW.md. Motion is the remaining one.

### Relevant existing features
- `srcX` (0–1), `srcY` (0–1), `srcZ` (−2..8 m) — the modulation anchors (`PluginProcessor.cpp:82-84`)
- `SourceShaper::shape()` — takes `srcXNorm`/`srcYNorm` and denormalises via
  `plane::normToMetres` against the speaker bounding box (`SourceShaper.cpp:29-35`). **The
  integration point.**
- `width` (0–12 m) + `decorr` — splits the source into two sub-points; motion moves the pair
- `DbapSolver`, `ConvexHull2D`, `hullAtten` (dB/m), `rolloff` (dB/2x) — already model out-of-hull
  sources, which is what makes D5 defensible
- `MonitorFold` (v1.7.0) and `VerifyPing` — the two other things that move the audible image;
  motion must compose with both
- SAFE mode (`isSafeMode()`, `PluginProcessor.h:143`) — mono/stereo host negotiation. Whether
  motion runs in SAFE mode is a research-phase call; the fold refuses, but motion is a creative
  parameter, not a monitoring aid, so refusing is *not* the obvious answer.
- 8 per-speaker weights + 6 named scenes — orthogonal to motion, but both write the audible image

### Known limitations and constraints carried in
- **O-Orbit's engine is polar, O-Octagon is Cartesian.** `MotionState { azimuth, elevation,
  distance }` around a listener vs. `(x, y, z)` in a measured venue. The path *vocabulary* ports;
  the *code* is a re-derivation, not a copy. Do not plan this as a file copy.
- **O-Orbit has no figure-8 and no spiral** — two of the six are new (D3).
- **O-Orbit advances per block** — see R5. Carried in as a known trap, not as a pattern to follow.
- `AudioParameterChoice` cannot hold one choice (`critical_choice_param_needs_two_choices`) —
  irrelevant at six paths, but the Path parameter is a choice and the constraint is on record.
- Preset migration: a new parameter group needs its own version gate
  (`pattern_preset_migration_per_param_version_gate`) so pre-1.8.0 presets land motion-off.
- WebView member declaration order — relays before attachments — for the new motion controls.

---

## Questions answered

**Q1 — How should the motion engine reach srcX/srcY/srcZ?**
> Internal offset. Parameters stay the anchor; motion adds a displacement downstream. → D1

**Q2 — What space are the trajectories drawn in, and how are they sized?**
> Venue metres, centred on the anchor. → D2

**Q3 — Which trajectories should ship in v1.8.0?**
> Six — the brief's four plus Pendulum and Spiral. → D3

**Q4 — How should height (srcZ) behave under motion?**
> Coupled to the path phase, with its own Height range. → D4

**Q5 — A metric path can carry the source outside the speaker bounding box. What should happen?**
> Let it leave. hullAtten and rolloff already model it. → D5

**Q6 — How much should the venue map show of the motion?**
> Trace + ghost anchor + live puck. → D6

**Q7 — Must an offline bounce of the random-walk path reproduce bit-for-bit?**
> Yes — deterministic, PPQ-derived, plus a Seed parameter. → D7

---

## Success criteria

1. [ ] All six paths (D3) generate correct metric offsets and are audibly distinct (R1)
2. [ ] `srcX`/`srcY`/`srcZ` read back unchanged across a full cycle with motion running (R2)
3. [ ] Render digest at `motionOn = 0` is **bit-identical** to the v1.7.0 binary (R3)
4. [ ] Two offline bounces of the same session are identical, Drift included (R4/D7)
5. [ ] Same session at 64 / 256 / 1024 samples renders the same (R5)
6. [ ] Venue map shows trace + ghost anchor + live puck, matching the audible path (R6)
7. [ ] Motion state round-trips in presets; pre-1.8.0 presets load motion-off (R7)
8. [ ] Build succeeds without warnings
9. [ ] Pluginval passes (Level 5+, and strictness 10 on CI Windows —
       `pattern_ci_pluginval10_catches_latent_nan`)
10. [ ] `auval -a | grep -i octagon` passes after install

---

## Open questions carried to Research

These were deliberately **not** put to the user — they are implementation calls the research phase
should settle from the codebase:

1. **Control rate.** Per-block advance (O-Orbit) vs. a fixed control-rate subdivision with
   smoothing. R5 is the constraint. Related: does the existing position path already smooth, and
   does a moving source zipper through `DbapSolver`?
2. **Where the offset is injected.** Inside `SourceShaper::shape()` after `normToMetres`, or as a
   metric pre-stage that hands `shape()` a metres pair directly. The second keeps `SourceShaper`
   ignorant of motion but changes its signature.
3. **SAFE mode.** Does motion run when the host negotiated mono/stereo? (The fold refuses; motion
   is a different kind of thing.)
4. **Interaction with `width` sub-points and `decorr`** — the pair translates rigidly, but confirm
   the `wEff == 0` decorr gate is unaffected.
5. **The safety limit for D5** — how far outside the bounding box is "generous"? Should follow from
   `hullAtten`'s dB/m and the rolloff curve, not from a round number.
6. **Trace generation for the UI** — how to drive the JS trace from the same generator as the audio
   path without a second implementation that can drift.
7. **Standalone / no-PPQ fallback** and what happens to phase when the transport stops mid-cycle.

---

## Next phase

This context feeds **Research**, which will:
- Settle the seven open questions above from the codebase
- Map `SourceShaper` / `DbapSolver` / `PluginEditor` integration points precisely
- Re-derive the six path equations in Cartesian venue metres
- Assess the block-size-invariance approach (R5) — the highest-risk item
- Identify affected files and the domain for the execute agent

---

*Generated by improve-milestone discuss phase*
