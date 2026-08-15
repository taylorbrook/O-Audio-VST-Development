# O-Octagon - Parameter Specification (Draft)

> ⚠️ **SUPERSEDED — historical artifact. Do not read this file for the parameter set.**
> The promoted spec is **[`parameter-spec.md`](parameter-spec.md)**. This draft still marks OQ3/OQ4/OQ5
> and the 17-vs-18 count as *open*; all four were resolved at Stage 0 (count is **17**; the venue is a
> separate `ValueTree`). Kept only to record what was known at ideation.

> Draft extracted from `BRIEF.md` §Parameters and `REQUIREMENTS.md`.
> Full specification will be generated during UI mockup finalization (two-screen: Room + Venue).

## Scope Note — Two Stores

O-Octagon splits state deliberately (BRIEF.md §Preset Strategy):

- **Musical parameters** — automatable APVTS floats, saved in musical presets, venue-portable.
- **Venue values** — measured room data, saved in a *separate* venue store. Whether these are
  APVTS parameters or a separate `ValueTree` is **Open Question 5 for Stage 0** (BRIEF.md
  §Technical Notes recommends a separate `ValueTree` outside the APVTS so 42 values do not
  pollute Logic's automation list).

Session state (`getStateInformation`) holds **both**. Only the *preset* layer is split.

---

## Musical Parameters (automatable, APVTS)

### Source Position

| Parameter | ID | Type | Range | Default | Unit | Description |
|-----------|----|----|-------|---------|------|-------------|
| Source X | `srcX` | Float | 0.0 - 1.0 | 0.5 | norm (disp. m) | Left→right. Normalised to venue bounding box for preset portability; converted to metres against the current venue at solve time. |
| Source Y | `srcY` | Float | 0.0 - 1.0 | 0.5 | norm (disp. m) | Front→rear. Normalised to venue bounding box. |
| Source Z | `srcZ` | Float | -2.0 - 8.0 | 0.0 | m | Height **above the sloped audience plane**. 0 = ear level, follows the rake (DSP-04). |
| Width | `width` | Float | 0.0 - 6.0 | 0.0 | m | Stereo spread. L/R become two sub-points straddling the puck, ⊥ to the puck's bearing from room centre. 0 = mono-summed single point. **Not venue-portable** (metres by explicit choice). |

### DBAP Solve

| Parameter | ID | Type | Range | Default | Unit | Description |
|-----------|----|----|-------|---------|------|-------------|
| Rolloff | `rolloff` | Float | 3.0 - 6.0 | 4.0 | dB/doubling | DBAP `R`. 6 = free-field inverse-distance; closed/semi-closed rooms want 3-5. |
| Blur | `blur` | Float | 0.0 - 1.0 | 0.10 | norm (capped) | Spatial blur `r_s`, normalised against covariance of speaker distances from rig centre (paper §3.1) so it is room-size independent. **Additional** to the physical floor supplied by speaker height. Cap value = Open Question 3. |

### Per-Speaker Weights — the spatial-orchestration control

| Parameter | ID | Type | Range | Default | Unit | Description |
|-----------|----|----|-------|---------|------|-------------|
| Weight 1..8 | `w1` … `w8` | Float ×8 | 0.0 - 1.0 | 1.0 | norm | Per-speaker weight `w_i`. With `Σ v_i² = 1` enforced, dropping weights **redistributes** rather than reduces level (DSP-05). All-zero must yield silence, not NaN (QUAL-02). |

### Outside-Hull Processing

| Parameter | ID | Type | Range | Default | Unit | Description |
|-----------|----|----|-------|---------|------|-------------|
| Hull Attenuation | `hullAtten` | Float | 0.0 - 3.0 | 1.0 | dB/m | Gain trim per metre of source→hull distance when outside the hull. **0 = off** (this is how DSP-07 "independently defeatable" is expressed). |
| Air Amount | `airAmount` | Float | 0.0 - 1.0 | 0.35 | norm | Air-absorption 1-pole LPF depth vs hull distance. **0 = bypassed.** Curve shape and reference distance = Open Question 3. |

### Output

| Parameter | ID | Type | Range | Default | Unit | Description |
|-----------|----|----|-------|---------|------|-------------|
| Output Gain | `outputGain` | Float | -24.0 - 12.0 | 0.0 | dB | Master output trim. Verify-ping level ceiling must be **independent** of this (FUNC-04). |

---

## Venue Values (measured room data, separate store)

Storage mechanism is **Open Question 5 for Stage 0** — separate `ValueTree` (recommended) vs APVTS.

| Data | ID | Type | Range | Default | Unit | Description |
|------|----|----|-------|---------|------|-------------|
| Speaker coords | `spk1.x/y/z` … `spk8.x/y/z` | Float ×24 | metres | traced layout, scaled | m | Measured speaker positions **including height**. Default scale for the traced normalised layout = Open Question 4. |
| Rake front | `rakeFront` | Float | metres | TBD (OQ4) | m | Front-row ear height. |
| Rake rear | `rakeRear` | Float | metres | TBD (OQ4) | m | Rear-row ear height. Together these define the sloped audience plane. |
| Speaker trim | `trim1` … `trim8` | Float ×8 | -12.0 - 6.0 | 0.0 | dB | Per-speaker calibration, applied **after** the DBAP solve (FUNC-07). |
| Label map | `map1` … `map8` | Choice ×8 | 8 × 7.1 channel labels | sane default | - | Speaker N → 7.1 channel label. Duplicate/missing assignment must be surfaced, not silently routed (FUNC-03). |

**Venue total: 24 + 2 + 8 + 8 = 42 values.** ✓ matches BRIEF.md

---

## Non-Parameters (UI actions — deliberately NOT automatable)

| Action | Behaviour |
|--------|-----------|
| Scenes: `ALL` `FRONT` `REAR` `LEFT` `RIGHT` `SIDES` + 4 user slots | A click **writes all 8 weight parameters at once**, so scenes record as ordinary automation and can be faded between (FUNC-06). The scene itself is not a parameter. |
| Verify mode | Solo-pings each speaker in turn; manual step + auto-cycle. Signal type, level ceiling, dwell, cycle rate = Open Question 2. Level bounded by a fixed conservative ceiling regardless of `outputGain`. |
| Venue save / load | Named venue file. Never written by a musical preset (FUNC-05). |

---

## Parameter Count Summary

| Group | Count |
|-------|-------|
| Source position (`srcX`, `srcY`, `srcZ`, `width`) | 4 |
| DBAP solve (`rolloff`, `blur`) | 2 |
| Weights (`w1`..`w8`) | 8 |
| Outside-hull (`hullAtten`, `airAmount`) | 2 |
| Output (`outputGain`) | 1 |
| **Total musical (automatable) parameters** | **17** |
| Venue values (separate store) | 42 |

> ⚠️ **Discrepancy flagged for Stage 0:** BRIEF.md §Parameters states "**18** musical parameters",
> but its own table enumerates **17**. No 18th parameter is invented here. Stage 0 must resolve
> whether the intended 18th is a genuine omission — candidates suggested by the requirements are
> an explicit hull-processing bypass, an explicit air bypass (currently both encoded as
> value = 0), or a plugin bypass — or whether 18 was simply an arithmetic slip and 17 is correct.
> Note the venue count of 42 **is** arithmetically correct, so only the musical figure is suspect.

---

## Parameter Type Notes for Stage 1

- All musical parameters are **`AudioParameterFloat`**. There are no Choice or Bool parameters in
  the musical set — every "defeat" is encoded as a value of 0 (see the discrepancy note above,
  which may change this).
- `map1`..`map8` are the **only** Choice-typed values in the design, and they live in the venue
  store. If Stage 0 places them in the APVTS, note the constraint: an `AudioParameterChoice`
  **cannot hold one choice** — these hold 8, so they are safe.
- `srcX`/`srcY` are stored normalised but **displayed in metres**; the metre display depends on the
  current venue's bounding box, so the value→text conversion is venue-dependent, not a static lambda.
- Gain vectors recompute on parameter change (not per sample) and the resulting 8 gains are
  smoothed (PERF-02, QUAL-04). Smoothing must run **per sample** to preserve block-size
  invariance (QUAL-03).

---

*Draft Status: Extracted from BRIEF.md + REQUIREMENTS.md on 2026-08-10*
*Full specification pending UI mockup finalization (Room + Venue screens)*
