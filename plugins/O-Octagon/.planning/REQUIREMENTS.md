# O-Octagon - Requirements

---
version: 1.0.0
plugin: O-Octagon
created: 2026-08-10
lastUpdated: 2026-08-11
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 30
**Coverage:** must: 18 | should: 10 | nice: 2

---

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Accepts mono or stereo input and renders 8 discrete speaker feeds through an `AudioChannelSet::create7point1()` output bus used purely as an 8-channel carrier | must | partial | stage-2 *(re-mapped from stage-1)* |
| FUNC-02 | User can type real measured coordinates in metres (x, y, z) for all 8 speakers plus front/rear rake heights, and save/load them as a venue store | must | pending | stage-3 |
| FUNC-03 | User-configurable 8-row mapping table assigning speaker 1-8 to a 7.1 channel label, with a sane shipped default | must | pending | stage-2 |
| FUNC-04 | Verify mode solo-pings each speaker in turn (manual step and auto-cycle) so physical wiring is confirmable in under a minute | must | pending | stage-3 |
| FUNC-05 | Venue data and musical state are separate preset stores; loading a musical preset never writes venue geometry, trims, or the label map | must | pending | stage-3 |
| FUNC-06 | Weight scenes (ALL / FRONT / REAR / LEFT / RIGHT / SIDES + 4 user slots) write all 8 weight parameters at once so they record as ordinary automation | should | pending | stage-3 |
| FUNC-07 | Per-speaker calibration trim (-12 to +6 dB), venue-scoped, applied after the DBAP solve | should | pending | stage-2 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | DBAP implemented per the 2011-04-14 revised equations, in three dimensions (`d_i` includes the `(z_i-z_s)²` term) | must | pending | stage-2 |
| DSP-02 | Constant intensity: `Σ v_i² = 1` holds within tolerance for every source position, inside and outside the hull, at every rolloff and blur setting | must | pending | stage-2 |
| DSP-03 | Convex hull computed by a proper algorithm with explicit collinearity handling — no rectangle assumption, no assumption that all 8 speakers are vertices; sources outside are projected to the nearest hull point for gain computation | must | pending | stage-2 |
| DSP-04 | Audience plane modelled as sloped from front-row to rear-row ear height; `srcZ` is height above that plane, so `srcZ = 0` tracks ear level front-to-back | must | pending | stage-2 |
| DSP-05 | Per-speaker weights `w_1..w_8` (0-1) restrict a source to a subset of speakers and redistribute rather than reduce level | must | pending | stage-2 |
| DSP-06 | Stereo input with `width > 0` renders as two sub-points straddling the puck, spread perpendicular to the puck's bearing from room centre; `width = 0` collapses to a mono-summed single point | should | pending | stage-2 |
| DSP-07 | Source→hull distance drives a gain trim (0-3 dB/m) and a 1-pole air-absorption LPF, each independently defeatable | should | pending | stage-2 |
| DSP-08 | Spatial blur `r_s` is normalised against the covariance of speaker distances from rig centre (paper §3.1) so it is room-size independent, and is capped; it is additional to the physical floor supplied by speaker height | should | pending | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Venue measurement screen: 8-row x/y/z entry in metres, rake front/rear, 8-row label map, 8 trims, verify-ping controls, venue save/load | must | pending | stage-3 |
| UI-02 | Room screen: top-down plan proportioned to the measured geometry, draggable source puck, convex hull drawn explicitly | should | pending | stage-3 |
| UI-03 | Live per-speaker level indicators at each of the 8 speaker positions on the plan | should | pending | stage-3 |
| UI-04 | DBAP level-field gradient rendered as the plan backdrop (paper figs 1-3) | nice | pending | stage-3 |
| UI-05 | Height control presented as a side-elevation strip showing the raked audience line and the source's height above it | nice | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing — no allocations, locks, or file I/O in `processBlock` | must | pending | stage-2 |
| PERF-02 | Gain vectors recomputed only when position, weights, rolloff, blur, or venue change — not per sample — and the resulting 8 gains are smoothed | should | pending | stage-2 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU), including strictness level 10 | must | complete | stage-1 |
| COMPAT-02 | Instantiates in Logic Pro on a surround track and outputs 8 discrete channels | must | pending | stage-4 |
| COMPAT-03 | Speaker→buffer index map is built once in `prepareToPlay()` via `getChannelIndexForType()`; no hardcoded channel indices anywhere in the codebase | must | pending | stage-2 |
| COMPAT-04 | Defined, non-crashing behaviour when instantiated on a stereo track (exact policy resolved at Stage 0) | should | complete | stage-1 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts across the full parameter ranges | must | pending | stage-2 |
| QUAL-02 | No NaN or Inf for any source position, any weight state including all-zero, any blur value, or any degenerate venue geometry | must | pending | stage-2 |
| QUAL-03 | Block-size invariant — an offline bounce is bit-identical to a real-time pass for identical automation | must | pending | stage-2 |
| QUAL-04 | No zipper noise on fast position, width, or weight automation | should | pending | stage-2 |

---

## Acceptance Criteria Details

### FUNC-01: 8-channel transport

- [x] `isBusesLayoutSupported()` accepts mono-in/7.1-out and stereo-in/7.1-out — *met at stage-1;
      `auval` reports `[1, 8]` and `[2, 8]` in the derived AU config set*
- [x] Rejects layouts whose output channel count is not 8 (except the Stage-0 stereo-fallback policy)
      — *met at stage-1; the derived AU set is exactly six configs and admits nothing else*
- [ ] All 8 output channels carry independent, non-duplicated signal for an off-centre source
      — **stage-2.** Not satisfiable by the Stage 1 shell: the `PHASE-2.2-REPLACE` placeholder writes
      the same mono sum to all 8 by design. Independence requires the DBAP solve (DSP-01/DSP-05)

> **Re-mapped stage-1 → stage-2 at Stage 1 verify (2026-08-11).** Two of three criteria were met by
> the foundation shell; the third cannot be met until Stage 2 exists. See
> `stages/1-foundation/VERIFICATION.md` §Issues 1.

### FUNC-02: Measured venue entry

- [ ] All 24 coordinate fields and both rake heights accept typed metre values and reject non-numeric input
- [ ] Saving then reloading a venue reproduces all 42 values exactly
- [ ] The DBAP solve uses the entered values, verified by a changed gain vector after a coordinate edit

### FUNC-03: Label map

- [ ] Each of the 8 rows can be assigned any of the 8 7.1 channel labels
- [ ] A duplicate or missing assignment is detected and surfaced rather than silently routed
- [ ] Changing a row moves audio to the corresponding physical output, confirmed by verify-ping

### FUNC-04: Verify-ping

- [ ] Ping plays from exactly one speaker at a time, with all others silent
- [ ] Manual step advances 1→8 and auto-cycle completes all 8 unattended
- [ ] Ping level is bounded by a fixed conservative ceiling regardless of `outputGain`

### FUNC-05: Preset separation

- [ ] Loading a musical preset leaves all 42 venue values bit-identical
- [ ] A musical preset saved under venue A recalls correctly under venue B, with position resolved against venue B's bounding box
- [ ] Session state (`getStateInformation` / `setStateInformation`) round-trips both stores together

### DSP-01: DBAP, 3D, revised equations

- [ ] `a = R / (20·log10 2)` — verified against hand-computed values at R = 3, 4, 6
- [ ] `d_i` includes the `(z_i - z_s)²` term; changing only `srcZ` changes the gain vector
- [ ] Gain vector matches an independent reference implementation of eqs 9-10 to within 1e-6 for a fixture set of source positions

### DSP-02: Constant intensity

- [ ] `Σ v_i² = 1 ± 1e-6` across a dense sweep of source positions inside the hull
- [ ] Holds for sources outside the hull, at hull vertices, and at speaker positions
- [ ] Holds across the full rolloff (3-6) and blur (0-1) ranges

### DSP-03: Convex hull

- [ ] Hull of the traced layout yields vertices 1, 2, 4, 5, 6, 7, correctly classifying collinear speakers 3 and 8 as on-edge rather than interior or vertex
- [ ] A source at a physical rear corner of the room is correctly classified as outside the hull
- [ ] Hull projection returns the nearest point on the boundary, verified against brute-force nearest-point-on-segment for a fixture set
- [ ] Degenerate venues (all speakers collinear, duplicate coordinates) do not crash or produce NaN

### DSP-04: Sloped audience plane

- [ ] With `srcZ = 0`, the resolved absolute source height varies linearly from `rakeFront` at the front of the room to `rakeRear` at the rear
- [ ] A source at `srcZ = 0` at the rear of a steeply raked room is never below the rear-row ear height
- [ ] Changing `rakeRear` alone changes the gain vector for a rear-positioned source

### DSP-05: Speaker weights

- [ ] Setting `w_i = 0` produces exactly zero gain at speaker `i`
- [ ] Restricting to a 2-speaker subset preserves `Σ v_i² = 1` — measured output level is unchanged
- [ ] All-zero weights produce silence, not NaN or full-scale output

### UI-01: Venue measurement screen

- [ ] All 42 venue values are viewable and editable without leaving the screen
- [ ] Verify-ping is reachable from this screen and its active speaker is visually indicated
- [ ] Venue save/load round-trips through a named file

### PERF-01: Real-time safety

- [ ] No heap allocation, lock, or file I/O in `processBlock` (verified by inspection and an RT-safety harness pass)
- [ ] `pow()` calls occur on parameter change, not per sample
- [ ] Hull projection executes only when the source is outside the hull

### COMPAT-01: pluginval

- [x] VST3 passes pluginval strictness 10 — *SUCCESS ×3, re-run at Stage 1 verify*
- [x] AU passes pluginval strictness 10 and `auval` — *SUCCESS ×3; `AU VALIDATION SUCCEEDED`*
- [x] Run 2-3 times locally before any publish (per known latent-NaN pattern) — *3 runs per format*

### COMPAT-02: Logic Pro

- [ ] Instantiates on a surround track with 7.1 output
- [ ] Verify-ping confirms all 8 outputs reach distinct physical channels
- [ ] Automation of `srcX`/`srcY`/`srcZ` and `w1..w8` is visible and writable in Logic's automation lanes

### COMPAT-03: Channel map

- [ ] Grep confirms zero hardcoded channel indices in the output path
- [ ] Unit test asserts the map built by `getChannelIndexForType()` against known JUCE enum-bit order for 7.1
- [ ] Test fails loudly if JUCE's enum-bit order changes (asserted against parsed source, not a mirrored constant)

### QUAL-01: No artifacts

- [ ] Full-range sweeps of every parameter produce no clicks, discontinuities, or level jumps
- [ ] Rapid puck movement across the hull boundary produces no audible discontinuity

### QUAL-02: No NaN or Inf

- [ ] Source at each exact speaker coordinate — finite output at blur = 0
- [ ] All-zero weights, single non-zero weight, rolloff at both range ends — finite output
- [ ] Degenerate venue geometry (coincident speakers, zero rake span) — finite output
- [ ] Pathological input (silence, DC, full-scale, denormals) produces no sticky NaN state

### QUAL-03: Block-size invariance

- [ ] Rendering identical automation at blockSize 512 and 4096 produces bit-identical output
- [ ] An offline bounce is bit-identical to a real-time pass
- [ ] Any envelope or smoothing runs per sample, not per block

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 ✅, COMPAT-04 ✅ *(FUNC-01 partial — re-mapped to stage-2)* |
| stage-2 | **FUNC-01**, FUNC-03, FUNC-07, DSP-01..08, PERF-01, PERF-02, COMPAT-03, QUAL-01..04 |
| stage-3 | FUNC-02, FUNC-04, FUNC-05, FUNC-06, UI-01..05 |
| stage-4 | COMPAT-02, all remaining |

---

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| VBAP secondary mode (A/B) | Needs a genuine 3D triangulation of the 8 speakers — Stage-2 work, not a toggle | v1.1+ |
| Binaural / stereo fold-down | Output bus is fixed at 8 channels; fold-down must write into 2 of 8 and mute 6. True binaural needs HRTF data | v1.1+ |
| Quadraphonic (4ch) container variant | Doubles the bus-layout and channel-map test matrix — the exact area where silent channel-order bugs live | v1.1+ |
| Internal diffuse reverb driven by hull distance | Cannot be an external send (`aufx` gets one bus); an 8-out diffuse network is a significant DSP add | v1.1+ |
| Motion engine (tempo-synced paths, gesture record/playback) | v1.0 is host-automation only, deliberately | v1.1+ |
| Multiple simultaneous sources per instance | v1.0 is one puck plus stereo width | v1.1+ |

---
*Generated from BRIEF.md on 2026-08-10*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
