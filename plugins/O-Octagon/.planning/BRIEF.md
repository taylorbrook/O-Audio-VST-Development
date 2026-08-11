# O-Octagon - Creative Brief

> **Status:** 💡 Ideated
> **Date:** 2026-08-10
> **Author:** Taylor Brook + Claude
> **Type:** Audio Effect (8-Channel DBAP Spatializer)
> **Based on:** `research/logic-pro-multichannel-octaphonic-dbap.md` (branch `docs/logic-multichannel-dbap-research`)
> **Primary source:** Lossius, Baltazar, de la Hogue — "DBAP – Distance-Based Amplitude Panning", ICMC 2009, **2011-04-14 revised version**

---

## Overview

**Core Concept:** A Logic Pro-native 8-channel spatializer that renders a mono/stereo source to eight discrete speaker feeds using Distance-Based Amplitude Panning, over an irregular, non-flat, user-measured speaker array.

**Target venue:** Roy Barnett Recital Hall, UBC — 255 seats, deep rectangular plan, steeply raked seating, speakers mounted high on the side walls above the audience.

## Vision

Every commercial spatial panner assumes a ring: equidistant speakers on a circle around a sweet spot. Real concert rigs are not rings. The Roy Barnett array is three pairs down the side walls of a deep rectangular hall, plus a rear pair set inboard along the back wall — non-equidistant, non-convex-friendly, and mounted well above a raked audience. VBAP degrades badly here because it discards distance entirely and only serves one listening position; DBAP was designed for exactly this case and weights every speaker by its actual distance to the virtual source.

O-Octagon exists to make that rig playable from inside Logic Pro, with the room's real measured geometry as first-class data, and with per-speaker weighting as a compositional control rather than a calibration afterthought.

The headline gesture is **spatial orchestration** in the Acousmonium sense: restrict a sound to the front pair, then the left wall, then the rear pair, and let the DBAP normalisation keep the perceived level constant while the sound changes its place in the room. Weights are automatable, so those subsets can be composed rather than performed.

## Relationship to O-Orbit

`O-Orbit` (v1.0.0, installed) is the general-purpose **VBAP** spatial *orbiter*: a motion engine driving arbitrary layouts across many surround formats. O-Octagon is the opposite specialisation — **no motion engine**, one locked 8-channel transport, and **DBAP** distance weighting for a specific irregular non-flat rig. They are complementary, not redundant. If motion is wanted on the Barnett rig, O-Orbit's path vocabulary is the model to borrow from in a future version.

---

## Locked Architecture (from prior research — not re-opened in ideation)

### 1. Transport
Logic exposes only 10 named surround formats and **no arbitrary discrete N-channel bus**. The only 8-channel containers are 7.1, 7.1 (SDDS), and 5.1.2. The plugin declares **mono/stereo in → `AudioChannelSet::create7point1()` out**.

7.1 is used purely as an **8-channel carrier**. Its L/C/R/LFE/side/rear semantics are meaningless here. All real geometry lives in the DSP.

### 2. Do not use `octagonal()` or `discreteChannels(8)`
JUCE provides both, and the AU wrapper advertises them (`juce_audio_plugin_client_AU_1.mm:2601-2617`), but Logic ignores them — its format list is fixed.

### 3. Not a multi-output design
Logic supports multi-output only for instruments (`aumu`). Effects (`aufx`) get one output bus. This is why the outside-hull reverb option, if ever built, must be **internal** — a real aux send to a reverb track is not available.

### 4. CRITICAL — channel-order trap
`AudioChannelSet` is a **bitset**. Buffer order is enum-bit order — **not** initializer-list order and **not** CoreAudio wire order. Proof in the local JUCE tree: `juce_AudioChannelSet.cpp:572` places `centre` at index 2 for `octagonal()`, while `juce_CoreAudioLayouts_mac.h:92` places it at index 4 for the same tag.

**Build the speaker→buffer map ONCE in `prepareToPlay()` via `getChannelIndexForType()`. Never hardcode indices.** A wrong map is silent: it scrambles the room and passes every build gate. Same defect class as JUCE forum thread 68674.

### 5. User-configurable label map + verify-ping
Logic's default surround I/O assignment is **not** L,R,C,LFE order (its 5.1 default is out1=L, out2=R, out3=Ls, out4=Rs, out5=C, out6=LFE). Rather than depend on Logic's custom-assignment UI, O-Octagon ships its own **8-row mapping table** (speaker 1-8 → 7.1 channel label) with a sane default, plus an in-UI **verify mode** that solo-pings each speaker in turn so physical wiring is confirmable in under a minute.

This matters more than usual: the target venue is a shared university hall the user does not control, and the patch may differ between visits.

### 6. DSP — DBAP (2011-04-14 revised equations)

```
d_i = sqrt( (x_i-x_s)² + (y_i-y_s)² + (z_i-z_s)² + r_s² )     (1, 8 — extended to 3D)
a   = R / (20 · log10(2))                                      (4)
v_i = k · w_i / d_i^a                                          (9)
k   = 1 / sqrt( Σ w_i² / d_i^(2a) )                            (10)
```

`Σ v_i² = 1` — constant intensity regardless of source position.

> ⚠️ Many online copies of the ICMC 2009 paper carry the **original** equations 3-6 and 9-10, which are wrong. The 2011-04-14 revision above is authoritative.

- **R (rolloff)** — default **4 dB/doubling**, range 3-6. R=6 is free-field inverse-distance; the paper notes "for closed or semi-closed environments R will generally be lower, in the range 3-5 dB." A recital hall wants ~4.
- **r_s (spatial blur)** — ≥ 0, prevents divide-by-zero and controls how tightly a source collapses into a single speaker. Normalised against the covariance of speaker distances from rig centre (paper §3.1) so it is independent of room size. **Capped** — excess blur lets the precedence effect drag the image toward whichever speaker is nearest each listener. In this 3D model the real speaker height already supplies a physical blur floor, so the exposed control is blur **additional** to that.
- **Convex hull (§2.3)** — load-bearing, see geometry below.
- **w_i (weights)** — per-speaker 0-1, the spatial-orchestration control.

### 7. VBAP — deferred to v1.1+
A secondary VBAP mode for A/B at the centre position was considered and **explicitly cut from v1.0**. Genuine 3D VBAP needs a triangulation of the 8 speakers, which is real Stage-2 work rather than a toggle. Math is already available in `research/spatial-audio-per-grain-spatialization.md §1` when it is picked up.

---

## Geometry

### Traced starting layout (NOT measured)

Normalised floor plan, origin at the front-left corner, x = left→right, y = front→rear. Stage is at the front between speakers 1 and 2. Numbered clockwise from front-left. Traced from a hand sketch and mirrored for symmetry — a **starting point only**.

| # | Position | x | y |
|---|----------|------|------|
| 1 | front-left | 0.19 | 0.14 |
| 2 | front-right | 0.81 | 0.14 |
| 3 | right-2nd | 0.81 | 0.40 |
| 4 | right-3rd | 0.81 | 0.70 |
| 5 | back-right | 0.67 | 0.87 |
| 6 | back-left | 0.33 | 0.87 |
| 7 | left-3rd | 0.19 | 0.70 |
| 8 | left-2nd | 0.19 | 0.40 |

```
  front (stage)
   1 ·           · 2
   8 ·           · 3
   7 ·           · 4
      6 ·     · 5
       rear wall
```

**The plugin MUST accept real measured coordinates in metres for all 8 speakers, including height, and save them as a venue preset.** The traced layout ships only as a default so the plugin is usable before a measurement session.

### Geometry fact (a) — 3D, not 2D

The hall is steeply raked and the speakers are mounted high on the side walls, above the audience. DBAP runs in **three dimensions** — eq. 1 simply gains a `(z_i - z_s)²` term, and the paper states the 2D model "can be readily extended to three dimensions." A flat floor-plan model would systematically misjudge distances over a rake.

The **audience plane is modelled as sloped**, entered once as front-row and rear-row ear heights (or an equivalent rake angle). Source Z is then a height **above that sloped plane**: `srcZ = 0` tracks ear level as the source travels front-to-back and never dives under the rake, and `srcZ = +2` rides two metres above the audience wherever it is.

### Geometry fact (b) — the hull is not a rectangle

Because speakers 5 and 6 are set **inboard** along the back wall, the hull's rear boundary runs 4→5→6→7 and cuts the corners. **The physical rear corners of the room fall outside the convex hull.**

With the traced coordinates, speakers 3 and 8 are exactly collinear with their neighbours (both walls are dead straight at x=0.81 and x=0.19), so they lie *on* hull edges rather than being strict vertices. Real measurements will push them marginally inside or outside. **This is precisely why a proper hull algorithm with explicit collinearity handling is required — never a rectangle assumption, and never an assumption that all 8 speakers are vertices.**

Strict hull vertices for the traced layout: **1, 2, 4, 5, 6, 7** (3 and 8 degenerate on-edge).

When the source is outside the hull, it is projected to the nearest point on the hull for the gain computation, and the source→hull distance drives the outside-hull processing below.

---

## Parameters

### Musical — automatable, saved in musical presets

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| `srcX` | 0-1 (normalised to venue bounding box) | 0.5 | Source position, left→right. Displayed in metres. |
| `srcY` | 0-1 (normalised to venue bounding box) | 0.5 | Source position, front→rear. Displayed in metres. |
| `srcZ` | -2 to +8 m | 0.0 | Height **above the sloped audience plane** (0 = ear level, follows the rake). |
| `width` | 0-6 m | 0.0 | Stereo spread. L/R become two sub-points straddling the puck, **perpendicular to the puck's bearing from room centre**. 0 = mono-summed single point. |
| `rolloff` | 3-6 dB/doubling | 4.0 | DBAP `R`. |
| `blur` | 0-1 (normalised, capped) | 0.10 | Spatial blur **additional** to the physical floor supplied by speaker height. |
| `w1`..`w8` | 0-1 each | 1.0 | Per-speaker weights — the spatial-orchestration control. |
| `hullAtten` | 0-3 dB/m | 1.0 | Gain trim per metre of source→hull distance when outside the hull. 0 = off. |
| `airAmount` | 0-1 | 0.35 | Air-absorption LPF depth vs hull distance. 0 = bypassed. |
| `outputGain` | -24 to +12 dB | 0.0 | Master output trim. |

**17 musical parameters.**

> *Corrected at Stage 0 (2026-08-11). This line originally read "18". The table has 10 rows, one of
> which (`w1..w8`) collapses 8 parameters, so the correct expansion is 9 + 8 = **17**. The figure 18
> arose from counting the collapsed weight row **and** its eight expansions. No 18th parameter was
> intended and none has been added — `hullAtten = 0` and `airAmount = 0` already serve as exact
> defeats, and host bypass is provided by the host. See `research/ARCHITECTURE.md` §11. The venue
> count of 42 was already correct.*

### Venue — measured room data, saved in a separate venue store

| Data | Range | Default | Description |
|------|-------|---------|-------------|
| `spkN.x/y/z` (×8) | metres | traced layout, scaled | Measured speaker coordinates including height. |
| `rakeFront` / `rakeRear` | metres | TBD | Front-row and rear-row ear heights defining the sloped audience plane. |
| `trim1`..`trim8` | -12 to +6 dB | 0.0 | Per-speaker calibration trim, applied **after** the DBAP solve. |
| `map1`..`map8` | 8 choices | sane default | Speaker N → 7.1 channel label. |

**42 venue values.**

### Scenes — UI actions, not parameters

`ALL` · `FRONT` · `REAR` · `LEFT` · `RIGHT` · `SIDES` · 4 user slots. A scene click writes all 8 weight parameters at once, so scenes are recorded as ordinary automation and can be faded between.

### Verify mode — UI action, not automated

Solo-ping each speaker in turn to confirm physical wiring. Conservative default level; pink-noise burst; manual step and auto-cycle.

---

## Preset Strategy

**Two separate stores.** This is a deliberate, load-bearing decision.

```
VENUE   RoyBarnett-2026-03.venue
        8× (x, y, z) metres + rake
        8× trim dB
        8× speaker→label map
        ── never written by a musical preset

PRESET  rear-sweep.json
        srcX/Y/Z, width, rolloff, blur
        8× weight, hullAtten, airAmount, outputGain
        ── portable between venues
```

A musical preset saved at Roy Barnett must be recallable in a different hall without dragging Roy Barnett's room along with it. Loading a musical preset writes **only** the musical subset; the venue is untouched.

Note that `srcX`/`srcY` are stored **normalised to the venue bounding box** precisely so musical presets stay portable, and are converted to metres against the current venue at solve time. `width` is in metres by explicit choice and is therefore *not* venue-portable — an accepted trade for a directly measurable control.

**Session state (`getStateInformation`) still holds everything**, so a Logic project reopens with both the room and the music intact. Only the *preset* layer is split.

---

## Signal Flow

```
Input (mono or stereo)
        │
        ├─ width = 0 ──→ mono sum ──→ 1 source point
        └─ width > 0 ──→ L / R ────→ 2 sub-points, spread ⊥ to bearing from room centre
        │
        ▼
  [Source position clamp → room bounds]
  [srcZ resolved against the sloped audience plane]
        │
        ▼
  [Convex hull test]
   inside  → use source directly
   outside → project to nearest hull point for gains
             keep d_hull for attenuation + air LPF
        │
        ▼
  [DBAP solve, 3D, per sub-point]
   d_i = √((x_i-x_s)² + (y_i-y_s)² + (z_i-z_s)² + r_s²)
   a   = R / (20·log10 2)
   k   = 1 / √( Σ w_i²/d_i^(2a) )
   v_i = k · w_i / d_i^a                       →  Σ v_i² = 1
        │
        ▼
  [Sum sub-point gain vectors]  →  8 gains
        │
        ▼
  [Outside-hull: gain trim + 1-pole air LPF, both defeatable]
        │
        ▼
  [Per-speaker trim (venue) → output gain]
        │
        ▼
  [Speaker N → buffer index, via map built ONCE in prepareToPlay()]
        │
        ▼
  8-channel output (7.1 container)
```

---

## UI Concept

**Layout:** two screens — *Room* (performance) and *Venue* (measurement).

**Room screen**
- Top-down room plan, correctly proportioned to the measured geometry
- Draggable source puck; the DBAP level-field gradient (paper figs 1-3) rendered live as a backdrop so the gain distribution is visible, not inferred
- **Convex hull drawn explicitly** — it is the boundary that changes the plugin's behaviour and the user should never have to guess where it is
- Height control alongside the plan (a side-elevation strip showing the raked audience line and the source's height above it reads better than a bare knob)
- Live per-speaker level indicators at each of the 8 speaker positions
- 8 weight controls sited *at* their speakers on the plan, plus the scene buttons
- Rolloff, blur, width, hull attenuation, air, output

**Venue screen**
- 8-row measurement entry: x / y / z in metres per speaker, plus rake front/rear
- 8-row speaker → 7.1 label mapping table
- 8 calibration trims
- Verify-ping controls
- Venue save / load

**Visual style:** technical and legible over decorative — this is an instrument read at a distance in a dark hall during a concert. High-contrast plan, unambiguous speaker numbering, no ornament that competes with the level field.

---

## Use Cases

- Diffusing a fixed-media or live electroacoustic work over the Roy Barnett 8-channel rig, with position automated in Logic against the score
- Acousmonium-style spatial orchestration — composing a sound's movement between speaker subsets (front pair → left wall → rear pair) via automated weights, at constant perceived level
- Restricting a stem to a defined region of the rig so several stems occupy distinct spatial planes simultaneously
- Confirming a shared hall's patch in under a minute on arrival, before rehearsal time is spent on it
- Preparing and revising a piece's spatialisation away from the venue, then recalling it against the measured room on site

---

## Inspirations

- Lossius / Baltazar / de la Hogue — DBAP (ICMC 2009, rev. 2011-04-14); Jamoma's DBAP implementation
- GRM Acousmonium / BEAST — spatial orchestration as compositional practice rather than mixing
- IRCAM Spat — measured-geometry-first spatialisation
- Ville Pulkki — VBAP, as the deliberate counter-example (listener-dependent, distance-blind)
- `O-Orbit` (this repo) — sibling VBAP orbiter; source of motion vocabulary for a future version

---

## Technical Notes

- **Real-time safety:** the DBAP solve is 8 distance computations, 8 `pow()` calls, one reciprocal square root — cheap, but `pow()` per block (not per sample) with smoothed gains. Hull projection is O(hull vertices) and only runs when the source is outside.
- **Recompute triggers:** gain vectors recompute when position, weights, rolloff, blur, or venue change — not per sample. Smooth the resulting 8 gains to avoid zipper noise on fast automation.
- **Denormal / NaN surface:** `d_i` can never be zero because `r_s ≥ 0` combines with the physical height offset, but the blur cap and a hard `d_i` floor should both be asserted, not assumed. `k` involves a division by a sum that goes to zero only if *all* weights are zero — an all-zero weight state must be handled explicitly (silence, not NaN).
- **Weights and normalisation interact:** with `Σ v_i² = 1` enforced, dropping weights to a subset does **not** drop level — it redistributes. This is the feature. It also means a single remaining non-zero weight puts the whole signal in one speaker at full level; the UI should make that legible.
- **Venue data placement:** the 42 venue values should most likely live in a **separate `ValueTree` outside the APVTS**, serialised alongside it, rather than as 42 automatable parameters polluting Logic's automation list. Flagged for Stage 0.
- **Channel map:** built once in `prepareToPlay()` via `getChannelIndexForType()`, per locked constraint 4. This deserves a dedicated unit test that asserts the map against known JUCE enum-bit order — the failure mode is silent.
- **Offline render parity:** gains must be block-size invariant. An offline bounce and a real-time pass must produce bit-identical output for identical automation.

---

## Open Questions for Stage 0

These do not change the architecture and can be resolved with sensible defaults during planning.

1. **Stereo-track fallback** — what `isBusesLayoutSupported()` does when the plugin lands on a stereo track: refuse instantiation, pass through, or downmix with a visible warning. Affects Stage 1 foundation.
2. **Verify-ping design** — signal type, level ceiling, dwell time, auto-cycle rate. Conservative defaults matter in a hall the user does not control.
3. **Numeric defaults** — blur cap value, hull attenuation and air-LPF curve shapes and reference distances, per-speaker meter ballistics.
4. **Default venue scale** — plausible metres for the traced normalised layout so the plugin is coherent before a measurement session (hall depth, width, speaker mounting height, rake front/rear ear heights).
5. **Venue data storage** — separate `ValueTree` vs APVTS parameters (see Technical Notes).

## Deferred to v1.1+

| Feature | Reason |
|---------|--------|
| VBAP secondary mode (A/B) | Needs a genuine 3D triangulation of the 8 speakers — Stage-2 work, not a toggle |
| Binaural / stereo fold-down | Output bus is fixed at 8 channels; fold-down must write into 2 of the 8 and mute 6. True binaural needs HRTF data |
| Quadraphonic (4ch) variant | Doubles the bus-layout and channel-map test matrix — the exact area where silent channel-order bugs live |
| Internal diffuse reverb on hull distance | Cannot be an external send (aufx = one bus); an 8-out diffuse network is a significant DSP add |
| Motion engine (tempo-synced paths, gesture record) | v1.0 is host-automation only. Borrow O-Orbit's path vocabulary if picked up |
| Multiple simultaneous sources per instance | v1.0 is one puck + stereo width |

## Next Steps

- [ ] Stage 0 planning (`/plan O-Octagon`)
- [ ] UI mockup (`/start O-Octagon` → option 3)
- [ ] Measure Roy Barnett Recital Hall — 8 × (x, y, z) in metres, plus front-row and rear-row ear heights
