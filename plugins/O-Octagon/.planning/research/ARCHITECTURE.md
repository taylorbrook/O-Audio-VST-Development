# DSP Architecture: O-Octagon

**Contract status:** BINDING — Stage 0 output. Stages 1-4 implement this document.
**Generated:** 2026-08-11 by research-planning-agent
**Plugin type:** Audio Effect (`aufx`) — 8-channel DBAP spatializer
**Complexity tier:** 6 (DEEP research) — see §0
**JUCE version:** 8.0.14 (local `/Users/taylorbrook/JUCE`, verified against source, not documentation)
**Primary source:** Lossius, Baltazar, de la Hogue — "DBAP – Distance-Based Amplitude Panning", ICMC 2009, **2011-04-14 revised version**
**Locked architecture:** `research/logic-pro-multichannel-octaphonic-dbap.md`

> **API verification method.** Context7-MCP's documentation-fetch tool was not available in this
> session. All JUCE API claims below were instead verified **directly against the local JUCE 8.0.14
> source tree** at `/Users/taylorbrook/JUCE`, with file and line references given inline. This is a
> strictly stronger source than either Context7 or WebSearch, and it is the same tree the plugin
> compiles against. Every line reference in this document was read, not recalled.

---

## 0. Complexity Detection

| Signal | Value |
|--------|-------|
| Musical (automatable) parameters | **17** (see §11 — the brief's "18" is an arithmetic slip, resolved) |
| Venue values (non-automatable store) | 42 |
| DSP components | 8 |
| Non-DSP features | multichannel bus negotiation, file I/O (venue store), two-store state, verify-ping generator |
| UI features | two screens, room plan + draggable puck, hull overlay, 8 live meters, DBAP level-field gradient, side-elevation rake strip, 10 scene buttons |
| Silent-failure surface | **channel map** (highest-risk defect in the plugin) |

**Tier: 6 — DEEP.** Justified by: real-time visualisation (UI-03/UI-04), file I/O (FUNC-02/FUNC-05),
non-standard multichannel bus negotiation (FUNC-01), and a computational-geometry component
(DSP-03) with explicit degeneracy requirements. Any one of these alone would put this at Tier 5+.

---

## 1. Feature Identification (meta-research)

Nine features requiring independent research were identified from BRIEF.md and REQUIREMENTS.md:

| # | Feature | Class | Requirements |
|---|---------|-------|-------------|
| F1 | 8-channel transport & bus negotiation | non-DSP | FUNC-01, COMPAT-01, COMPAT-04 |
| F2 | Speaker→buffer channel map + user label map | non-DSP (silent-failure) | FUNC-03, COMPAT-03 |
| F3 | Venue geometry model (coords, rake, bounding box) | data | FUNC-02, DSP-04 |
| F4 | Convex hull (build, classify, inside-test, project) | DSP / geometry | DSP-03 |
| F5 | DBAP solver (3D, revised equations) | DSP | DSP-01, DSP-02, DSP-05, DSP-08 |
| F6 | Source shaping — stereo sub-points, width | DSP | DSP-06 |
| F7 | Outside-hull processing — gain trim + air LPF | DSP | DSP-07 |
| F8 | Gain application, smoothing, trims, output gain | DSP / RT | PERF-01, PERF-02, QUAL-01..04 |
| F9 | Verify-ping generator | DSP / UX | FUNC-04 |

Plus two cross-cutting systems documented in §4: **two-store state persistence** (FUNC-05) and
**real-time metering** (UI-03).

---

## 2. Core Components

Eight DSP/algorithmic components. All live under `Source/DSP/` or `Source/Data/`, following the
sibling `O-Orbit` layout (`Source/{Data,DSP}/`).

### VenueModel (F3)

**Role:** Owns the 42 measured venue values, derives everything geometric that does not change per
block: bounding box, speaker centroid, rig scale, audience-plane coefficients.

**JUCE classes:**

| Need | JUCE class | Module | Verified |
|------|-----------|--------|----------|
| Venue storage tree | `juce::ValueTree` | `juce_data_structures` | `juce_ValueTree.h:442,448,470,473` — `createXml()`, `fromXml()`, `writeToStream()`, `readFromStream()` |
| Venue file save/load | `juce::File`, `juce::FileChooser` | `juce_core`, `juce_gui_basics` | standard |
| Channel label names | `juce::AudioChannelSet::getAbbreviatedChannelTypeName(ChannelType)` | `juce_audio_basics` | `juce_AudioChannelSet.h:550` |

**Derived quantities (message thread, on any venue edit):**

```
bbMinX/bbMaxX/bbMinY/bbMaxY  = min/max over the 8 speaker x,y            (normalised-coord frame)
centroid c                    = (1/8) Σ p_i                              (3D)
rigScale                      = sqrt( (1/8) Σ ||p_i − c||² )             (RMS speaker radius, metres)
earHeight(y)                  = rakeFront + (rakeRear − rakeFront) · (y − bbMinY)/(bbMaxY − bbMinY)
```

`rigScale` is the paper's §3.1 normalisation of blur "against the covariance of speaker distances
from rig centre", expressed as an RMS radius so it has units of metres and scales linearly with the
room. It makes `blur` room-size independent, satisfying DSP-08.

**Audience-plane definition (DSP-04).** The plane is linear in `y` across the **venue bounding box**
y-range, with `rakeFront` at `bbMinY` and `rakeRear` at `bbMaxY`, extrapolated linearly outside.
This was chosen over an explicit "seating front / seating rear" y-pair because it defines the plane
completely from the two values the brief already specifies, keeping the venue count at exactly 42.
Absolute source height is then `zAbs = earHeight(y_source) + srcZ`, so `srcZ = 0` rides the rake.

**Degeneracy guard:** if `bbMaxY − bbMinY < 1e-6`, `earHeight(y) ≡ rakeFront` (QUAL-02: "zero rake
span — finite output").

---

### ChannelMap (F2) — HIGHEST RISK COMPONENT

**Role:** Maps speaker 1-8 to output buffer indices. Built **once in `prepareToPlay()`**. See §3.2
for the full construction, the label-map layer, validation, and the three-layer test strategy.

**JUCE classes:**

| Need | JUCE class / method | Module | Verified |
|------|--------------------|--------|----------|
| Negotiated output set | `getBusesLayout().getMainOutputChannelSet()` | `juce_audio_processors` | `juce_AudioProcessor.h` |
| Type → buffer index | `AudioChannelSet::getChannelIndexForType(ChannelType)` | `juce_audio_basics` | `juce_AudioChannelSet.cpp:514-527` |
| Buffer index → type | `AudioChannelSet::getTypeOfChannel(int)` | `juce_audio_basics` | `juce_AudioChannelSet.h:577` |
| Type list in buffer order | `AudioChannelSet::getChannelTypes()` | `juce_audio_basics` | `juce_AudioChannelSet.cpp:529-534` |
| Human label | `AudioChannelSet::getAbbreviatedChannelTypeName()` | `juce_audio_basics` | `juce_AudioChannelSet.h:550` |

---

### ConvexHull2D (F4)

**Role:** Andrew's monotone chain hull over the **(x, y) floor projection** of the 8 speakers;
speaker classification; inside test; nearest-point-on-boundary projection; `d_hull`.

**JUCE classes:** none. Pure C++ over fixed-size `std::array`. `juce::Point<float>` is available
(`juce_graphics`) but is deliberately **not** used in the audio path — the DSP uses a local POD
`Vec2 { float x, y; }` so no graphics module is pulled into the RT path and the struct is trivially
copyable into the lock-free snapshot.

Full algorithm, collinearity handling and degeneracy matrix in §3.1.

---

### DbapSolver (F5)

**Role:** The 2011-04-14 revised equations, extended to 3D. Given a resolved 3D source point, the
8 speaker positions, `R`, `r_s`, and `w_1..w_8`, produce a gain vector `v[8]` with `Σ v_i² = 1`.

```
d_i = sqrt( (x_i−x_s)² + (y_i−y_s)² + (z_i−z_s)² + r_s² )     (1, 8 — 3D extension)
a   = R / (20 · log10 2)                                        (4)
k   = 1 / sqrt( Σ w_i² / d_i^(2a) )                             (10)
v_i = k · w_i / d_i^a                                           (9)
```

**JUCE classes:** none — `<cmath>` (`std::pow`, `std::sqrt`) only. JUCE ships **no** DBAP, no VBAP
and no distance model (`research/juce8-multichannel-spatial-audio.md` §6: "What Does NOT Exist in
JUCE"; `juce::dsp::Panner` is stereo-only, `juce_Panner.h`). This component is entirely custom.

Numeric robustness in §3.3.

---

### SourceShaper (F6)

**Role:** Resolve `srcX`/`srcY` (normalised) to metres against the current venue bounding box;
derive the two stereo sub-points perpendicular to the puck's bearing from the rig centroid; resolve
each sub-point's absolute height against the sloped audience plane.

**JUCE classes:** none. Geometry in §3.4.

---

### HullProcessor (F7)

**Role:** Outside-hull gain trim (dB per metre of `d_hull`) and the air-absorption one-pole LPF.

| Need | JUCE class | Module | Verified |
|------|-----------|--------|----------|
| One-pole LPF | `juce::dsp::FirstOrderTPTFilter<float>` | **`juce_dsp`** | `juce_FirstOrderTPTFilter.h:74` `setType()`, `:80` `setCutoffFrequency()`, `:91` `prepare()`, `:94/:97` `reset()`, `:135` `processSample(int channel, SampleType)` |
| dB↔linear | `juce::Decibels::decibelsToGain()` | `juce_audio_basics` | standard |

**Module dependency: `juce::juce_dsp` must be in `target_link_libraries`.** This is the only DSP
module dependency in the plugin.

Curve shapes and reference distances in §3.5 / OQ3.

---

### GainStage (F8)

**Role:** Per-sample application of the 16 smoothed gains (8 for the L sub-point, 8 for the R
sub-point), the venue per-speaker trims (folded into the targets), and the master output gain.
Writes through `ChannelMap`. Owns the control-rate scheduler that makes the whole plugin
block-size invariant (§3.6).

| Need | JUCE class | Module | Verified |
|------|-----------|--------|----------|
| Per-sample gain ramp ×17 | `juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>` | `juce_audio_basics` | `juce_SmoothedValue.h:265` `reset(double sampleRate, double rampLengthInSeconds)`, `:284` `setTargetValue()`, `:309` `getNextValue()`, `:75` `isSmoothing()` |
| Denormal guard | `juce::ScopedNoDenormals` | `juce_audio_basics` | standard |

---

### VerifyPing (F9)

**Role:** Solo-ping generator for physical wiring confirmation. Bypasses the entire DBAP chain and
injects directly at the `ChannelMap` stage, so it validates the map itself.

| Need | JUCE class | Module | Verified |
|------|-----------|--------|----------|
| Noise source | `juce::Random::nextFloat()` | `juce_core` | `juce_Random.h:89` |
| Band-limiting | `juce::dsp::FirstOrderTPTFilter` ×2 (HP + LP) | `juce_dsp` | as above |
| Envelope | hand-rolled raised-cosine, `<cmath>` | — | — |

> **RT-safety note:** use a **member-owned `juce::Random`**, never `juce::Random::getSystemRandom()`
> (`juce_Random.h:138`) — the system random is a shared thread-local and its use from the audio
> thread mixes streams with any other caller. Per `pattern_rng_stream_interleave_blocksize`, an
> audio-thread RNG shared across phases also breaks block-size invariance. The ping is not part of
> the bit-identity contract (it is a UI action, never automated), but the member-owned instance is
> free and removes the question.

---

## 3. Algorithm Details

### 3.1 Convex hull — full specification (DSP-03)

#### 3.1.1 Why 2D hull with 3D distances — the dimensional split

The hull is computed on the **(x, y) floor projection**; DBAP distances are fully **3D**. This is a
deliberate asymmetry and the justification is load-bearing:

1. **A 3D hull of this rig is near-degenerate.** All 8 speakers sit within roughly 1 m of each other
   in `z` across a 12 × 15 m footprint. The 3D hull is therefore a thin slab, and *any* source with
   `srcZ` more than ~0.5 m off the speaker plane would classify as outside — triggering hull
   attenuation and the air LPF constantly. That is wrong behaviour, not a subtle bias.
2. **`srcZ` is a musical control, not an error condition.** The parameter range is −2 to +8 m
   precisely so a composer can fly a sound above the audience. Penalising height with the
   outside-hull processor would fight the feature.
3. **The paper's hull treatment is a horizontal-plane argument.** §2.3 exists to solve the
   degeneracy where, as the source recedes from the array, the relative differences between `d_i`
   converge and the panning loses contrast. For a rig whose speakers span 12 × 15 m horizontally
   and ~1 m vertically, that convergence is a horizontal phenomenon.

**Consequence, stated so it is not discovered as a bug:** a source at `srcZ = +8 m` directly above
the room centre is **inside** the hull and receives no hull attenuation and no air filtering. This
is intended. `d_hull` is a floor-plane distance throughout.

#### 3.1.2 Build — Andrew's monotone chain

Runs on the **message thread only**, on any venue edit. O(n log n) with n = 8 — microseconds.

```
INPUT   p[0..7] = (x_i, y_i) floor projection of the 8 speakers
        srcIdx[k] = which speaker each retained point came from

STEP 0  Deduplicate. Two points within EPS_DEDUP = 1e-4 m are one point.
        Keep the lowest speaker index as representative; record the collapsed set.
        (A duplicate coordinate is a legitimate venue-entry error, not a crash case.)

STEP 1  Sort lexicographically by (x, then y).

STEP 2  Lower hull: for each point in order, while the chain has >= 2 points and
        cross(h[n-1] - h[n-2], p - h[n-2]) <= EPS_CROSS, pop.
        Upper hull: same over the reversed order.
        Concatenate, dropping the duplicated endpoints.

        >>> The comparison is `<= EPS_CROSS`, i.e. collinear points are POPPED.
        >>> This yields STRICT VERTICES ONLY. It is the whole reason speakers 3 and 8
        >>> of the traced layout come out correctly as on-edge rather than as vertices.

        EPS_CROSS is an AREA tolerance, not a length: EPS_CROSS = 1e-6 * (spanX * spanY),
        so it scales with the room and does not become meaningless in a 40 m hall or
        hypersensitive in a 3 m studio.

OUTPUT  hullPts[0..m-1] in counter-clockwise order (assert the signed area is >= 0;
        negate the winding once if the chain produced clockwise), m <= 8.
```

Verification against the traced layout (DSP-03 acceptance criterion 1): points 1, 2, 4, 5, 6, 7 are
strict vertices; points 3 (0.81, 0.40) and 8 (0.19, 0.40) are exactly collinear with their wall
neighbours (both walls dead straight at x = 0.81 and x = 0.19) and are popped. **Expected hull:
1, 2, 4, 5, 6, 7 — m = 6.**

#### 3.1.3 Speaker classification (diagnostic, message thread)

After the hull is built, classify all 8 original speakers against the final edge list:

| Class | Test |
|-------|------|
| `VERTEX` | index appears in `hullPts` |
| `ON_EDGE` | min perpendicular distance to any hull edge segment `< EPS_ONEDGE` (= 1e-3 m) |
| `INTERIOR` | inside and not on an edge |

Surfaced on the Venue screen. This is not decoration: it tells a user who has just typed measured
coordinates that (for example) speaker 3 has moved from on-edge to interior, which changes hull
behaviour. Real measurements will push 3 and 8 marginally inside or outside — the brief says so, and
the UI should show which.

#### 3.1.4 Inside test (audio thread, every control block)

For a CCW hull of m ≥ 3 vertices, point `p` is inside-or-on iff

```
for i in 0..m-1:  cross(hullPts[(i+1)%m] − hullPts[i], p − hullPts[i]) >= −EPS_CROSS
```

8 cross products worst case = 16 multiplies, 8 subtract-pairs. Negligible, branch-predictable, no
allocation.

#### 3.1.5 Projection when outside (audio thread, only when outside)

```
best = +inf
for each edge (a, b):
    t = clamp( dot(p − a, b − a) / max(|b − a|², EPS_LEN2), 0, 1 )
    q = a + t·(b − a)
    if |p − q| < best: best = |p − q|; proj = q
d_hull = best;  source position for the DBAP solve := proj
```

`EPS_LEN2 = 1e-12`. PERF-01 acceptance criterion 3 ("hull projection executes only when the source
is outside") is satisfied structurally — the projection block sits inside the `if (!inside)` branch.

#### 3.1.6 Degeneracy matrix (QUAL-02, DSP-03 criterion 4)

| Venue state | `m` after chain | Inside test | Projection | `d_hull` |
|---|---|---|---|---|
| Normal (8 distinct, non-collinear) | 3..8 | polygon test §3.1.4 | §3.1.5 | ≥ 0 |
| All 8 collinear (one wall) | 2 | `p` lies on segment within `EPS_ONEDGE` | nearest point on the single segment | ≥ 0 |
| All 8 coincident | 1 | `|p − h0| < EPS_ONEDGE` | `h0` | `|p − h0|` |
| Zero-area but 3+ unique points | 2 (chain degenerates) | segment test | segment projection | ≥ 0 |

`m < 3` is detected explicitly by testing the chain length, and routes to the segment/point path.
No branch produces a division by zero, and no branch produces `NaN`.

#### 3.1.7 Storage in the RT snapshot

`std::array<Vec2, 8> hullPts` + `int hullCount`. Fixed size, trivially copyable, no allocation
anywhere in the audio path.

---

### 3.2 Speaker→buffer channel map — full specification (COMPAT-03, FUNC-03)

#### 3.2.1 The verified ground truth

**`AudioChannelSet` stores channels as a bitset over `ChannelType`, and buffer order is ascending
enum-bit order.** Verified at `juce_AudioChannelSet.cpp:514-527`:

```cpp
int AudioChannelSet::getChannelIndexForType (ChannelType type) const noexcept
{
    int idx = 0;
    for (int bit = channels.findNextSetBit (0); bit >= 0; bit = channels.findNextSetBit (bit + 1))
    {
        if (static_cast<ChannelType> (bit) == type) return idx;
        idx++;
    }
    return -1;
}
```

The relevant `ChannelType` values (`juce_AudioChannelSet.h:402-432`):

| Type | Enum value |
|------|-----------|
| `left` | 1 |
| `right` | 2 |
| `centre` | 3 |
| `LFE` | 4 |
| `leftSurround` | 5 |
| `rightSurround` | 6 |
| `leftCentre` | 7 |
| `rightCentre` | 8 |
| `leftSurroundSide` | 10 |
| `rightSurroundSide` | 11 |
| `leftSurroundRear` | 20 |
| `rightSurroundRear` | 21 |

`create7point1()` (`juce_AudioChannelSet.cpp:567`) is
`{ left(1), right(2), centre(3), LFE(4), leftSurroundSide(10), rightSurroundSide(11), leftSurroundRear(20), rightSurroundRear(21) }`.

> **A trap worth naming explicitly.** For `create7point1()` the initializer-list order **happens to
> equal** the enum-bit order, because that list is already ascending. So hardcoded indices 0..7
> would appear to work for 7.1 today. That coincidence is exactly what makes the defect class
> dangerous: it validates a wrong mental model that then breaks silently the moment the negotiated
> set is anything else. It is not a guarantee, it is luck, and the rule stands unchanged.

CoreAudio wire order for the same tag is a separate ordering that JUCE remaps internally
(`juce_CoreAudioLayouts_mac.h:86`: `kAudioChannelLayoutTag_MPEG_7_1_C` is the same 8 types in the
same order here, but `:92` `kAudioChannelLayoutTag_Octagonal` puts `centre` at wire index 4 while
JUCE's `octagonal()` buffer order puts it at index 2 — the divergence is real and general).

#### 3.2.2 NEW FINDING — Logic may negotiate 7.1 (SDDS), not plain 7.1

`juce_CoreAudioLayouts_mac.h:117` contains:

```cpp
List { kAudioChannelLayoutTag_Emagic_Default_7_1,
       { left, right, leftSurround, rightSurround, centre, LFE, leftCentre, rightCentre } },
```

**Emagic** is Logic Pro's ancestor company, and this tag is Logic's own default 7.1 ordering. Its
*channel type membership* — `{left, right, centre, LFE, leftSurround, rightSurround, leftCentre,
rightCentre}` — is **not** `create7point1()`. It is **`create7point1SDDS()`**
(`juce_AudioChannelSet.cpp:568`). This is fully consistent with the prior research's independently
sourced observation that Logic's 5.1 default is `out1=L, out2=R, out3=Ls, out4=Rs, out5=C, out6=LFE`
— which is exactly `kAudioChannelLayoutTag_MPEG_5_1_B` (`:113`), the same family of ordering.

**Architectural consequence (defensive, does not re-open the locked constraint):**

- The plugin **declares `create7point1()` as its default output** in `BusesProperties` — locked
  constraint 1, unchanged.
- `isBusesLayoutSupported()` **additionally accepts** `create7point1SDDS()` and
  `create5point1point2()` — the other two 8-channel containers Logic exposes. Accepting them costs
  three lines and removes an entire class of "the plugin doesn't appear on my surround track"
  failure that we cannot reproduce without the hall.
- Because the label map is stored as **channel types** (§3.2.3), not indices, it adapts. When the
  negotiated set does not contain a stored type (7.1 and SDDS differ in 4 of 8 types), the
  unresolvable rows fall back to slot identity and the UI raises the FUNC-03 warning.

Confidence on which set Logic actually negotiates: **MEDIUM.** It is a Stage-4 verification task
(§8, COMPAT-02). The defensive acceptance means either answer works.

#### 3.2.3 Construction — once, in `prepareToPlay()`

```cpp
void OOctagonProcessor::rebuildChannelMap()   // prepareToPlay() + on label-map edit
{
    const auto outSet = getBusesLayout().getMainOutputChannelSet();

    // Enumerate the NEGOTIATED set's types in buffer order (enum-bit order).
    // getChannelTypes() is verified at juce_AudioChannelSet.cpp:529-534 to iterate
    // findNextSetBit, i.e. the identical order getChannelIndexForType counts against.
    const auto slotTypes = outSet.getChannelTypes();          // size == outSet.size()

    std::array<int, 8> next {};
    bool ok = (outSet.size() == 8);

    for (int n = 0; n < 8 && ok; ++n)
    {
        // venue.labelType[n] is an AudioChannelSet::ChannelType, stored by NAME in the
        // venue tree (see 3.2.4). NEVER a raw buffer index.
        const int idx = outSet.getChannelIndexForType (venue.labelType[n]);
        if (idx < 0) { ok = false; break; }                   // type absent from this set
        next[n] = idx;
    }

    // Validate: the map MUST be a permutation of 0..7. Anything else is silent misrouting.
    if (ok) ok = isPermutationOf0to7 (next);

    if (ok)  { publishMap (next);  mapInvalid.store (false); }
    else     { /* keep last valid map */  mapInvalid.store (true); }
}
```

Three properties this construction has, each deliberate:

1. **No hardcoded index anywhere.** COMPAT-03 acceptance criterion 1 is a grep: the string
   `speakerToBuffer` is the only thing that ever indexes an output channel, and it is only ever
   written by this function.
2. **The permutation check is the FUNC-03 duplicate/missing detector.** A duplicate label assignment
   produces a repeated target index; a missing one produces a `-1`. Both fail
   `isPermutationOf0to7`. The map is then **not applied** — the last valid map is retained and an
   atomic `mapInvalid` flag drives a persistent UI warning. It never silently routes.
3. **`getChannelIndexForType()` is a genuine lookup, not a tautology**, because the map is stored as
   a `ChannelType`. Had it been stored as a slot index (the tempting simplification) the call would
   be an identity function and the whole safety property would evaporate. Stage 2 must not "simplify"
   this.

#### 3.2.4 The user label map layer

Stored per speaker as the **abbreviated channel-type name string**
(`AudioChannelSet::getAbbreviatedChannelTypeName`, `juce_AudioChannelSet.h:550`) so a `.venue` file
is human-readable and stable across JUCE versions (enum *values* could in principle be renumbered;
the names are the public contract). Resolved to the enum at load by a small name→type table
built from `outSet.getChannelTypes()`.

**Shipped default map** — the identity map from `research/logic-pro-multichannel-octaphonic-dbap.md`
§6a, chosen so that an interleaved Logic surround bounce yields *channel N = speaker N* and drops
straight into QLab/Reaper for concert playback without a remap:

| Speaker | Type | 7.1 buffer index |
|---------|------|------------------|
| 1 | `left` | 0 |
| 2 | `right` | 1 |
| 3 | `centre` | 2 |
| 4 | `LFE` | 3 |
| 5 | `leftSurroundSide` | 4 |
| 6 | `rightSurroundSide` | 5 |
| 7 | `leftSurroundRear` | 6 |
| 8 | `rightSurroundRear` | 7 |

Note speaker 4 → `LFE` is intentional and safe: prior research §3 established that Logic applies no
automatic bass management or LFE low-pass, so the LFE slot carries a full-range feed to an ordinary
speaker. The **LFE-gain-on-bounce** question is open (MEDIUM-LOW) and is a Stage-4 test (§8).

#### 3.2.5 The test strategy — three layers (COMPAT-03)

The acceptance criterion is unusually specific: *"Test fails loudly if JUCE's enum-bit order changes
(asserted against parsed source, not a mirrored constant)."* This is a direct application of
`pattern_test_fixture_mirrors_drift_silently` — a fixture that mirrors the value it is checking
drifts with it and keeps passing forever. Three independent layers:

**Layer 1 — runtime invariant (cheap, partially circular).**
Reconstruct the expected order without using `getChannelTypes()`:

```cpp
std::vector<int> expected;
for (int bit = 0; bit < 64; ++bit)
    if (set.getChannelIndexForType (static_cast<ChannelType> (bit)) >= 0)
        expected.push_back (bit);

// assert strictly increasing, and:
for (int i = 0; i < set.size(); ++i)
    EXPECT_EQ (static_cast<int> (set.getTypeOfChannel (i)), expected[i]);
```

Catches an ordering regression. Shares the bitset implementation, so it is an invariant check, not
an independent oracle. Keep it — it is free.

**Layer 2 — source-parsed golden (the real gate).**
A build-time script `tests/tools/gen_juce_channel_order.py`:

1. reads `$JUCE_MODULES/juce_audio_basics/buffers/juce_AudioChannelSet.h`, parses the
   `enum ChannelType { ... }` block into name → integer value;
2. reads `juce_AudioChannelSet.cpp`, extracts the `create7point1()` / `create7point1SDDS()` /
   `create5point1point2()` initializer lists;
3. sorts each list's names by parsed enum value;
4. emits `JuceChannelOrderGolden.h` containing the derived orders **and a SHA-256 of the emitted
   content**.

The test compares the runtime order against the generated header, and a **committed checksum** in
the test source is compared against the generated SHA-256. A JUCE upgrade that changes the enum
values or the set membership changes the SHA and fails the build until a human re-blesses it. The
expectation is derived from JUCE's source every build; nothing is mirrored by hand.

**Layer 3 — the audible test (offline render harness).**
Follow the `plugins/O-ReverseDelay/tests/render-harness/` precedent. Render an 8-tone chord in which
speaker `i` is fed a unique frequency (e.g. 200·(i+1) Hz) by forcing `w_j = δ_ij`, then assert each
output channel's dominant FFT bin is exactly its speaker's frequency. This is the layer that catches
a wrong map at the *plugin* level rather than the JUCE level — i.e. the actual defect in JUCE forum
thread 68674 — and it is the only layer that would catch a Stage-2 refactor that reintroduces a
hardcoded index. **This test is mandatory before Stage 4.**

---

### 3.3 DBAP solver — numeric robustness (DSP-01, DSP-02, QUAL-02)

#### 3.3.1 Constants

```cpp
inline constexpr float kInvTwentyLog10Two = 1.0f / 6.020599913f;   // 1 / (20·log10 2)
inline constexpr float kMinDistance       = 0.05f;                 // metres, hard d_i floor
inline constexpr float kMaxBlurMetres     = 8.0f;                  // absolute r_s ceiling
inline constexpr float kBlurScale         = 0.5f;                  // blur=1 → r_s = 0.5·rigScale
inline constexpr float kDenomEpsilon      = 1e-20f;                // all-zero-weight detector

static_assert (kMinDistance > 0.0f, "d_i floor must be strictly positive: DBAP divides by d_i^a");
static_assert (kMaxBlurMetres > 0.0f && kBlurScale > 0.0f, "");
```

`kMinDistance` and `kMaxBlurMetres` are **asserted, not assumed** — the brief's Technical Notes call
for exactly this. `static_assert` rather than a comment, per
`pattern_ring_invariant_needs_static_assert`: an invariant that lives only in prose is an invariant
that is not enforced.

#### 3.3.2 Blur mapping (DSP-08)

```
r_s = min( blur · kBlurScale · rigScale , kMaxBlurMetres )
```

`rigScale` is the RMS speaker radius from the centroid (§2 VenueModel), which is the paper's §3.1
"covariance of speaker distances from rig centre" made dimensional. This makes `blur` room-size
independent by construction, satisfying DSP-08.

For the recommended default venue (§OQ4), `rigScale ≈ 7.95 m`:

| `blur` | `r_s` | Character |
|--------|-------|-----------|
| 0.00 | 0 m | No *additional* blur — the physical speaker height still supplies a floor |
| **0.10 (default)** | **0.40 m** | ≈ one cabinet width; the image can still collapse convincingly onto one speaker |
| 0.50 | 1.99 m | Noticeably softened |
| 1.00 (cap) | 3.97 m | Half the RMS rig radius — the paper's precedence-effect warning zone |

**The exposed 0-1 range IS the cap.** There is no separate cap parameter; `blur = 1.0` maps to
`0.5·rigScale`, and `kMaxBlurMetres = 8.0 m` is a second, absolute backstop so a mistyped 1000 m
coordinate cannot produce an `r_s` that swamps the array. Both are asserted.

Note the interaction the brief flags: because DBAP runs in 3D and the speakers are flown well above
the audience plane, the vertical offset already supplies a physical blur floor. `blur` is therefore
genuinely *additional* — this is a consequence of the 3D model, not a special case in the code.

#### 3.3.3 The distance floor

```cpp
const float dsq = dx*dx + dy*dy + dz*dz + rs*rs;
const float d   = std::max (std::sqrt (dsq), kMinDistance);
```

Applied unconditionally to every `d_i`, on every path, including the blur = 0, source-exactly-on-a-
speaker case (QUAL-02 acceptance criterion 1). With `kMinDistance = 0.05` and `a ≤ 0.9966`,
`1/d^(2a)` is bounded by ~400 per speaker, `Σ ≤ 3200`, `k ≥ 0.0177`. No overflow at any point in the
parameter space.

#### 3.3.4 All-zero weights (DSP-05, QUAL-02)

```cpp
float denom = 0.0f;
for (i) denom += w[i]*w[i] / std::pow (d[i], 2.0f*a);

if (denom < kDenomEpsilon) { for (i) v[i] = 0.0f; return; }   // SILENCE, explicitly

const float k = 1.0f / std::sqrt (denom);
for (i) v[i] = k * w[i] / std::pow (d[i], a);
```

Without the guard, `denom = 0` → `k = inf` → `v_i = inf · 0 = NaN`, which then propagates into the
`SmoothedValue` targets and latches permanently. The guard is a hard requirement, not a nicety.
DSP-05 acceptance criterion 3 tests it directly.

#### 3.3.5 `pow()` budget (PERF-01)

`std::pow` is called `2 × 8 × 2 = 32` times per **control block** (two sub-points × 8 speakers ×
{`d^a`, `d^(2a)`}), never per sample. With a 64-sample control grid at 48 kHz that is 0.5 `pow` per
sample — approximately 24 000 `pow`/second, which is under 0.1% of a core. The `d^(2a)` call can be
elided by computing `t = pow(d, -a)` once and using `t` and `t*t`, halving it to 16; do that.

PERF-01 acceptance criterion 2 (*"`pow()` calls occur on parameter change, not per sample"*) is
satisfied more strongly than written: the solve is additionally skipped entirely when the 17-float
parameter snapshot is unchanged since the last solve and the venue generation counter is unchanged.

---

### 3.4 Stereo sub-point geometry (DSP-06)

#### 3.4.1 Construction

```
1.  Resolve the puck to metres against the venue bounding box:
        px = bbMinX + srcX·(bbMaxX − bbMinX)      (guard span < 1e-6 → px = bbMinX)
        py = bbMinY + srcY·(bbMaxY − bbMinY)

2.  Bearing from the RIG CENTROID (not the bounding-box centre — the centroid is what the
    array physically "is", and it is what the level field is symmetric about):
        b = (px, py) − (c.x, c.y)

3.  Fade the spread to zero near the centroid — see 3.4.2:
        rFade  = 0.15 · rigScale
        wEff   = width · min(1, |b| / rFade)

4.  Unit bearing and its left-hand perpendicular:
        b̂ = b / max(|b|, 1e-6)          (if |b| < 1e-6, use the fallback b̂ = (0, −1),
                                          i.e. "toward the stage", so the spread axis is
                                          the room's left–right axis)
        n̂ = (−b̂.y,  b̂.x)

5.  Sub-points:
        P_L = (px, py) − (wEff/2)·n̂
        P_R = (px, py) + (wEff/2)·n̂

6.  Each sub-point resolves its OWN absolute height against the sloped audience plane at its
    OWN y — the plane slopes in y and the spread has a y component:
        z_L = earHeight (P_L.y) + srcZ
        z_R = earHeight (P_R.y) + srcZ
```

**Handedness check.** Origin at the front-left corner, `x` increases to the audience's right (the
brief's speaker 1 is "front-left" at small `x`, speaker 2 "front-right" at large `x`). With the puck
downstage of centre, `b̂ = (0, −1)`, so `n̂ = (1, 0)` = audience right. Therefore `P_R = P + (w/2)n̂`
puts R on the audience's right. Correct.

#### 3.4.2 The centre-crossing discontinuity — found and fixed at design time

As the puck sweeps through the rig centroid, `b̂` flips 180°, `n̂` flips, and **L and R swap sides
instantaneously**. At `width = 6 m` that is a 6-metre jump of both sub-points in one control block —
an audible click and a violation of QUAL-01 acceptance criterion 2 (*"rapid puck movement across the
hull boundary produces no audible discontinuity"*, and the same argument applies at the centre).

**Fix:** scale the spread to zero as the puck approaches the centroid (step 3 above). The flip then
happens only when the spread is already zero, so both sub-points are coincident with the puck at the
moment of the flip and the gain vectors are continuous. `rFade = 0.15 · rigScale` (≈ 1.19 m for the
default venue) is expressed room-relative so it is venue-portable.

Perceptual side-effect, and it is the right one: a source parked at the exact centre of the rig
collapses to a point regardless of the `width` setting. That is physically honest — there is no
meaningful "left and right" of a point at the centre of a surrounding array.

#### 3.4.3 Level convention and the `width = 0` case

The brief's flow diagram shows a branch (`width = 0 → mono sum`, `width > 0 → two sub-points`). A
literal branch creates a level and gain-vector discontinuity at `width = 0⁺`. It is unnecessary:

```
Always two sub-points, always fed at 0.5:
    s_L = 0.5 · L[n]        s_R = 0.5 · R[n]        (mono input: L = R = in)
    out_i[n] = v_L,i · s_L + v_R,i · s_R
```

At `width = 0` the two sub-points **coincide**, so `v_L ≡ v_R` bit-for-bit and the sum degenerates
to `v_i · 0.5·(L+R)` — exactly the brief's mono-sum case, reached with no branch and no
discontinuity. The `width = 0` behaviour specified in DSP-06 is preserved *as the degenerate case of
the general path*.

Level sanity:
- **Mono input** (`L = R = in`): output power = `Σv² · in²` = `in²` → **unity**. Correct.
- **Uncorrelated stereo**: `Σ(v_L² P_L + v_R² P_R)·0.25 ×` (the two solves each satisfy `Σv²=1`)
  → 0.25·(P_L + P_R) vs input P_L + P_R → −6 dB. Consistent with mono unity when P_L = P_R = P and
  the pair is coherent.
- **Correlated stereo** (`L = R = s`, input power 2P_s): output power `P_s` → −3 dB relative to the
  stereo sum. This is the *correct* behaviour for a mono-compatible pair: collapsing it must not
  produce a 3 dB gain.

**Do not add a width-dependent branch to "optimise away" the second solve.** Two solves cost 16
`pow` per control block. A branch that changes the arithmetic path risks a bit-identity divergence
between the branch taken at block boundary A and block boundary B — precisely the class of bug
QUAL-03 exists to catch.

---

### 3.5 Outside-hull processing (DSP-07)

#### 3.5.1 Gain trim

```
gain_dB = −hullAtten · d_hull        clamped to a floor of −24 dB
```

Linear in dB per metre, applied per sub-point (each sub-point has its own `d_hull`), folded into
that sub-point's gain vector **before** smoothing so it costs nothing per sample.

Why linear-in-dB rather than inverse-square: after hull projection the DBAP solve has already
renormalised to `Σv² = 1`, so this trim is a *musical* distance cue layered on top rather than a
physical propagation law. A dB-per-metre law is the one a composer can reason about and automate
predictably ("one dB per metre out"), and it is the one that reads correctly on an automation lane.
The −24 dB floor prevents a far-outside source vanishing entirely and prevents `−inf`.

`hullAtten = 0` → `gain_dB ≡ 0` → exactly defeated. This is DSP-07's "independently defeatable"
with no extra parameter. Because `d_hull = 0` whenever the source is inside, the whole stage is a
no-op inside the hull regardless of the setting.

#### 3.5.2 Air-absorption LPF

One `juce::dsp::FirstOrderTPTFilter<float>` per sub-point (2 total), lowpass, applied to the
**source signal before the gain matrix** — hull distance is a property of the source, not of a
speaker, so filtering per-speaker would be both 4× the cost and wrong.

```
fc = clamp( 20000 · 2^( −airAmount · d_hull / dRef ),  500 Hz,  20000 Hz )
with dRef = 3.0 m
```

One octave of rolloff per `dRef/airAmount` metres of hull distance:

| `airAmount` | `d_hull` | `fc` |
|---|---|---|
| 0.35 (default) | 5 m | 13.3 kHz |
| 0.35 | 15 m | 5.9 kHz |
| 1.00 | 5 m | 6.3 kHz |
| 1.00 | 15 m | 0.62 kHz |

The 500 Hz floor stops the source disappearing and keeps the TPT coefficient well away from a
degenerate corner. This is a **dramatised** cue, not a physical humidity model — real air absorption
over 15 m is far subtler. That is why the control is a 0-1 "amount" and not a physical parameter,
and the architecture should not later be "corrected" toward ISO 9613-1.

**`airAmount = 0` must SKIP the filter entirely and `reset()` its state — not merely set
`fc = 20 kHz`.** A one-pole at 20 kHz is not transparent at 44.1/48 kHz: its corner sits near
Nyquist and it imposes a measurable HF tilt in the top octave. A `fc = 20 kHz` "bypass" would mean
the plugin is never bit-transparent, which quietly breaks QUAL-01's "no level jumps" when a user
expects 0 to mean off. The skip branch is safe with respect to QUAL-03 because it is driven by a
parameter value that the harness controls at defined sample positions.

**Sticky-NaN guard.** The TPT filter is the only recursive element in the plugin. Per
`pattern_envelope_follower_state_sticky_nan`, a single non-finite input latches its state forever
and the ring never self-heals. Once per block, check the filter state (or the block output max) with
`std::isfinite` and `reset()` on failure. One branch per block; effectively free.

---

### 3.6 Real-time architecture — the control grid (PERF-01, PERF-02, QUAL-03)

#### 3.6.1 The conflict, stated plainly

PERF-02 says *recompute gain vectors per parameter change, not per sample*. The obvious
implementation is "solve once per `processBlock`". QUAL-03 says *an offline bounce is bit-identical
to a real-time pass* and *rendering identical automation at blockSize 512 and 4096 produces
bit-identical output*.

**These two are incompatible under a per-block solve.** At blockSize 512 the gain targets update
every 512 samples; at 4096, every 4096. The per-sample smoothing ramps then differ, and the outputs
diverge. This is exactly `pattern_block_rate_envelope_breaks_blocksize_invariance` — a per-block
control rate makes an offline bounce behave differently from a real-time pass, and it passes every
gate that does not explicitly compare two block sizes.

#### 3.6.2 The resolution — a fixed control grid decoupled from `blockSize`

```
kControlBlock = 64 samples          (1.33 ms at 48 kHz)

processBlock (buffer):
    ScopedNoDenormals guard;
    n = 0
    while n < numSamples:
        // absolute-sample-aligned control boundary
        samplesToNextBoundary = kControlBlock − (absoluteSampleCounter % kControlBlock)
        chunk = min (numSamples − n, samplesToNextBoundary)

        if (absoluteSampleCounter % kControlBlock == 0)
            updateControl();        // snapshot params, hull, DBAP solve, set 17 targets

        renderChunk (n, chunk);     // per-sample: 17 × getNextValue(), 8 × (2 mul + 1 add)

        n += chunk;  absoluteSampleCounter += chunk
```

Because the boundaries are keyed to an **absolute sample counter**, blockSize 512 and blockSize 4096
produce control updates at *identical absolute sample positions*. The smoothing ramps are therefore
identical, and the output is bit-identical. `SmoothedValue` is a deterministic per-sample linear
ramp with an internal countdown (`juce_SmoothedValue.h:265-309`), so given identical
`setTargetValue()` calls at identical sample positions it emits an identical sequence regardless of
how the buffer was chopped.

Cost: ≤ 16 `pow` per 64 samples worst case, and zero when the parameter snapshot is unchanged.
PERF-02 is satisfied — the solve is still gated on change; it is merely *scheduled* on a grid rather
than on the host's arbitrary block boundary.

#### 3.6.3 QUAL-03's test protocol must be specified, or it is untestable

A strict reading of *"bit-identical across block sizes"* cannot hold in general, because the host
writes automation to the APVTS atomics on the **message thread**, and when those writes land
relative to the audio thread is not deterministic and is not a property of this plugin.

**Therefore QUAL-03 is verified under a defined protocol, and Stage 2 must implement the harness to
match:**

1. The offline render harness drives parameters **programmatically**, writing the atomics between
   `processBlock` calls at **control-grid-aligned absolute sample offsets**.
2. Under that protocol, renders at blockSize 512 and 4096 must be **bit-identical** (memcmp of the
   float buffers, not a tolerance).
3. Separately: with all parameters held constant, a real-time pass and an offline bounce must be
   bit-identical for any pair of block sizes.

This is a refinement of QUAL-03's acceptance criteria, not a weakening: it turns an aspiration into
an executable test. Record it against QUAL-03 at Stage 2.

#### 3.6.4 Per-sample inner loop and its invariants

```
s_L = 0.5·L[n];   s_R = 0.5·R[n]
if (airActive) { s_L = airL.processSample (0, s_L); s_R = airR.processSample (0, s_R); }
g = outGain.getNextValue();
for i in 0..7:
    out[ speakerToBuffer[i] ][n] = ( gL[i].getNextValue() * s_L
                                   + gR[i].getNextValue() * s_R ) * g;
```

**Invariant:** every `SmoothedValue::getNextValue()` is called **exactly once per sample,
unconditionally**. A branch that skips a `getNextValue()` desynchronises that smoother's ramp
countdown from the others and produces a slow, position-dependent gain error that no single-parameter
test will find. There must be no `continue`, no early exit and no `if (w[i] == 0) skip` in this loop.

Budget: 8 channels × (2 mul + 1 add + 1 mul) + 17 `getNextValue()` ≈ 50 flops/sample ≈ 2.4 MFLOP/s
at 48 kHz. Well under 0.5% of one core. There is nothing here worth optimising.

#### 3.6.5 Smoothing time

`SmoothedValue<float, ValueSmoothingTypes::Linear>`, `reset(sampleRate, 0.005)` — a **5 ms** linear
ramp. Rationale: long enough to eliminate zipper noise on fast weight and position automation
(QUAL-04); short enough that a fast puck sweep tracks without audible lag; and comfortably longer
than the 1.33 ms control grid so the ramps overlap smoothly rather than stepping. A 20 ms ramp would
smear a scene change into a slow crossfade — which is a *musical* choice the user should make with
automation curves, not one the plugin should impose.

#### 3.6.6 Lock-free venue publication

Venue edits and hull rebuilds happen on the **message thread**. The audio thread must never touch
the `ValueTree`, never allocate, never lock.

```cpp
struct VenueSnapshot {                 // POD, trivially copyable, ~250 bytes
    std::array<Vec3,  8> spk;
    std::array<float, 8> trimLin;
    std::array<int,   8> speakerToBuffer;
    std::array<Vec2,  8> hullPts;   int hullCount;
    Vec3  centroid;   float rigScale;
    float bbMinX, bbMaxX, bbMinY, bbMaxY;
    float rakeFront, rakeRear;
};
```

Double-buffered: two `VenueSnapshot` slots plus a `std::atomic<int> activeSlot` and a
`std::atomic<uint32_t> generation`. The message thread fills the inactive slot, then
`activeSlot.store(newIdx, std::memory_order_release)`. The audio thread reads
`activeSlot.load(std::memory_order_acquire)` **once per control block** and holds that index for the
whole block. The `generation` counter also serves as part of the solver's dirty check.

No `std::shared_ptr` atomic swap: the snapshot is small and POD, and a double-buffer with a
generation counter avoids any possibility of a reference-count decrement (and therefore a `free`)
landing on the audio thread — the failure mode in `pattern_retired_map_reaper_rt_free`. With at most
one writer (message thread) and a slot that is only overwritten after at least one full
release/acquire cycle, the ABA window is not reachable at venue-edit rates (human typing).

---

## 4. System Architecture

### 4.1 State persistence — two stores (FUNC-05) — resolves OQ5

**Decision: the 42 venue values live in a separate `ValueTree`, NOT as APVTS parameters.** The brief
leaned this way; it is confirmed, and the decisive argument is repo-specific.

```
apvts.state  (root: "OOctagon")
├── PARAM × 17            ← JUCE-managed, automatable, musical presets write ONLY these
└── VENUE                 ← our child tree, message-thread only, NEVER touched by a preset
    ├── @name, @savedAt, @schemaVersion
    ├── @rakeFront, @rakeRear
    └── SPEAKER × 8  { @index, @x, @y, @z, @trimDb, @label }
```

Four reasons, in order of weight:

1. **The repo's own preset-apply pattern would destroy the venue.** `pattern_preset_apply_needs_reset_to_defaults`
   is an established, hard-won rule here: `applyPresetJson` must reset *all* parameters to defaults
   first, or partial presets inherit stale state. If the venue lived in the APVTS, that rule and
   FUNC-05 (*"loading a musical preset leaves all 42 venue values bit-identical"*) are in **direct
   contradiction**, and correctness would depend forever on every future preset writer remembering
   to exclude 42 IDs. With a separate tree the guarantee is structural: the preset code physically
   cannot reach the venue node.
2. **Automatable routing is a hazard.** `map1..map8` as `AudioParameterChoice` would make the
   speaker→channel assignment *automatable*. A stray automation lane could rewire the room
   mid-concert, and the failure would be silent and unrecoverable in performance. Venue data must be
   inert.
3. **Exact round-trip.** FUNC-02 requires *"saving then reloading a venue reproduces all 42 values
   exactly."* A host parameter is a normalised value that passes through the host's own
   representation; several hosts quantise. A `ValueTree` float attribute round-trips verbatim.
4. **Automation-list hygiene.** 42 extra parameters would bury the 17 musical ones in Logic's
   automation menu — in a plugin whose headline gesture is automating `w1..w8`, that is a direct
   usability harm.

**Session state.** `getStateInformation` serialises `apvts.copyState()` (verified
`juce_AudioProcessorValueTreeState.h:391`), which carries the VENUE child with it, so a Logic project
reopens with both the room and the music intact — exactly what the brief requires. Round-trip:

```cpp
void getStateInformation (MemoryBlock& dest) {
    auto state = apvts.copyState();          // VENUE rides along as a child
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}
void setStateInformation (const void* data, int size) {
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) {
            apvts.replaceState (ValueTree::fromXml (*xml));   // :404
            readVenueFromState();            // re-derive geometry + hull + map
        }
}
```

**Venue file** (`*.venue`): the VENUE subtree alone, written with `ValueTree::createXml()`
(`juce_ValueTree.h:442`) to a user-chosen `juce::File`. Human-readable and diffable — appropriate
for measured data a user may want to inspect or edit by hand.

**Ordering hazard.** `setStateInformation` must call `readVenueFromState()` *after*
`replaceState()`, and `rebuildChannelMap()` must run after both. If `prepareToPlay()` has not yet
run, defer the map rebuild — `getBusesLayout()` is valid but the intent is to have exactly one map
construction site. Per `pattern_asyncupdater_guard_flag_needs_cancel`, if any of this is deferred via
`AsyncUpdater`, `cancelPendingUpdate()` must be called in the restore path so a queued apply cannot
stomp restored state.

### 4.2 Bus configuration (FUNC-01) — resolves OQ1

```cpp
OOctagonProcessor()
  : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::mono(),          true)
        .withOutput ("Output", juce::AudioChannelSet::create7point1(), true))
{ }
```

**`BusesProperties` must be constructed in the member-initialiser list of the constructor, never in
`prepareToPlay()`.** (juce8-critical-patterns §4.)

```cpp
bool isBusesLayoutSupported (const BusesLayout& layouts) const override
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    if (in.isDisabled() || out.isDisabled())                 return false;
    if (in != AudioChannelSet::mono() && in != AudioChannelSet::stereo()) return false;

    // Real mode — the three 8-channel containers Logic exposes.
    if (out == AudioChannelSet::create7point1())        return true;   // primary
    if (out == AudioChannelSet::create7point1SDDS())    return true;   // Logic/Emagic default order
    if (out == AudioChannelSet::create5point1point2())  return true;   // third 8-ch container

    // SAFE mode — see OQ1. Defined, non-destructive, clearly signposted.
    if (out == AudioChannelSet::mono() || out == AudioChannelSet::stereo()) return true;

    return false;
}
```

Note `PLUGIN_CHANNEL_CONFIGURATIONS` must **not** appear in CMakeLists.txt — it understands channel
counts only, not channel types, and breaks surround layout detection
(`research/juce8-multichannel-spatial-audio.md` §7).

### 4.3 Real-time metering (UI-03)

Audio thread, after the map write, per output channel `i`:

```cpp
const float pk = buffer.getMagnitude (speakerToBuffer[i], 0, numSamples);
float prev = meterPeak[i].load (std::memory_order_relaxed);
if (pk > prev) meterPeak[i].store (pk, std::memory_order_relaxed);
```

The UI reads and zeroes at ~30 Hz. Metering the **written buffer post-map, post-trim** rather than
the computed `v_i` is deliberate: it means the meters visualise what actually leaves the plugin, so
a channel-map error shows up as the wrong speaker lighting on the plan — a second, human line of
defence on the highest-risk component.

UI ballistics per juce8-critical-patterns §20 (`requestAnimationFrame` loop, separate current/target,
`current += (target − current)·speed`): **attack 0.5, decay 0.12**, plus a 1.5 s peak-hold marker
releasing at 20 dB/s. Faster attack than a VU meter (0.4) because these are position indicators — a
composer needs to see the instantaneous spatial distribution, not an averaged loudness. Scale:
−60..0 dBFS mapped to 0..1.

### 4.4 File I/O

Only two operations, both message-thread, both user-initiated: venue save and venue load, via
`juce::FileChooser::launchAsync`. **The completion lambda must capture a
`juce::Component::SafePointer`, and on a dead pointer must `return` bare — never call `complete()`**
(`pattern_webview_launchasync_safepointer_no_complete`: calling the completion on a destroyed editor
is itself a use-after-free). No file I/O of any kind occurs in `processBlock` (PERF-01).

---

## 5. Processing Chain

```
                       ┌─────────────────────────────────────────────┐
 Input (mono/stereo)   │  MESSAGE THREAD (venue edits only)          │
        │              │   VenueModel → bbox, centroid, rigScale,    │
        │              │                audience-plane coefficients  │
        │              │   ConvexHull2D::build()  (monotone chain)   │
        │              │   rebuildChannelMap()    (prepareToPlay +   │
        │              │                           label-map edits)  │
        │              │                    │                        │
        │              │        publish VenueSnapshot (double-buffer,│
        │              │        release store, generation++)         │
        │              └────────────────────┬────────────────────────┘
        │                                   │ acquire, once per control block
        ▼                                   ▼
╔═══════════════════════ CONTROL BLOCK — every 64 absolute samples ═══════════════════════╗
║  1. Snapshot 17 parameter atomics; compare to lastSolved + venue generation.            ║
║     Unchanged → skip 2-7 entirely.                                                      ║
║                                                                                          ║
║  2. Resolve puck:   px,py  = bbox-denormalise (srcX, srcY)          [metres]            ║
║                                                                                          ║
║  3. Sub-points:     b = P − centroid_xy ;  wEff = width·min(1,|b|/rFade)                 ║
║                     n̂ = perp(b̂)  [fallback (0,−1) at |b|<1e-6]                          ║
║                     P_L = P − (wEff/2)n̂ ;  P_R = P + (wEff/2)n̂                          ║
║                     z_L = earHeight(P_L.y) + srcZ                                       ║
║                     z_R = earHeight(P_R.y) + srcZ                                       ║
║                                                                                          ║
║  4. Hull, PER SUB-POINT (2D, floor plane):                                              ║
║        inside?  → solve position := sub-point,  d_hull := 0                             ║
║        outside? → solve position := nearest point on hull,  d_hull := that distance      ║
║                                                                                          ║
║  5. DBAP solve, PER SUB-POINT (3D):                                                     ║
║        r_s = min(blur·0.5·rigScale, 8 m)                                                ║
║        d_i = max( sqrt(dx²+dy²+dz²+r_s²), 0.05 )                                        ║
║        a   = rolloff · (1/6.0206)                                                       ║
║        denom = Σ w_i²/d_i^(2a)                                                          ║
║        denom < 1e-20 ?  → v[] = 0  (SILENCE, not NaN)                                   ║
║        else k = 1/sqrt(denom) ;  v_i = k·w_i/d_i^a      →  Σ v_i² = 1                    ║
║                                                                                          ║
║  6. Outside-hull, PER SUB-POINT:                                                        ║
║        v_i *= dbToGain( max(−hullAtten·d_hull, −24) )                                   ║
║        air fc = clamp( 20k · 2^(−airAmount·d_hull/3), 500, 20k )   [set on filter]      ║
║                                                                                          ║
║  7. Fold venue trim, set 16 + 1 smoothed targets:                                       ║
║        gL[i].setTargetValue (v_L,i · trimLin_i)                                         ║
║        gR[i].setTargetValue (v_R,i · trimLin_i)                                         ║
║        outGain.setTargetValue (dbToGain (outputGain))                                   ║
╚══════════════════════════════════════════════════════════════════════════════════════════╝
                                            │
                                            ▼
        ┌──────────────── PER SAMPLE (inside the control block) ────────────────┐
        │  s_L = 0.5·L[n] ;  s_R = 0.5·R[n]      (mono in: L = R = in)          │
        │  if airActive:  s_L = airL.process(s_L) ;  s_R = airR.process(s_R)    │
        │  g = outGain.getNextValue()                                           │
        │  for i in 0..7:                                                       │
        │      out[ speakerToBuffer[i] ][n] =                                   │
        │          ( gL[i].getNextValue()·s_L + gR[i].getNextValue()·s_R ) · g  │
        │  >>> every getNextValue() called EXACTLY ONCE, unconditionally <<<    │
        └───────────────────────────────────────────────────────────────────────┘
                                            │
                                            ▼
        ┌─────────────── VERIFY-PING OVERRIDE (UI action, not automated) ───────┐
        │  if pingActive:                                                       │
        │      zero ALL 8 output channels                                       │
        │      write ping into out[ speakerToBuffer[pingSpeaker] ] ONLY         │
        │      level fixed at −20 dBFS RMS / −6 dBFS peak; outputGain NOT applied│
        │  >>> routes through the SAME map, so it validates the map itself <<<  │
        └───────────────────────────────────────────────────────────────────────┘
                                            │
                                            ▼
        ┌─────────────────────────── METERING ──────────────────────────────────┐
        │  meterPeak[i] = max(meterPeak[i], |out[speakerToBuffer[i]]|)  (relaxed)│
        └───────────────────────────────────────────────────────────────────────┘
                                            │
                                            ▼
                        8-channel output (7.1 / 7.1-SDDS / 5.1.2 container)
```

**SAFE mode (mono/stereo output, OQ1):** steps 1-7 still execute in full so the UI stays live and
correct, but the per-sample stage writes the **dry input at unity** to the 1-2 available channels and
the UI shows a persistent banner. Nothing is spatialised and the user is told.

---

## 6. Parameter Mapping

### 6.1 Musical parameters — 17, APVTS, automatable

All `juce::AudioParameterFloat`. There are **no** Choice or Bool parameters in the musical set; every
"defeat" is a value of 0. See §11 for the count finding.

| # | ID | Name | Range | Default | Skew | Unit | Consumed by |
|---|----|------|-------|---------|------|------|-------------|
| 1 | `srcX` | Source X | 0.0 – 1.0 | 0.5 | linear | norm (displayed m) | SourceShaper §3.4 |
| 2 | `srcY` | Source Y | 0.0 – 1.0 | 0.5 | linear | norm (displayed m) | SourceShaper §3.4 |
| 3 | `srcZ` | Source Z | −2.0 – +8.0 | 0.0 | linear | m above audience plane | VenueModel `earHeight()` + DbapSolver |
| 4 | `width` | Width | 0.0 – 6.0 | 0.0 | linear | m | SourceShaper §3.4 |
| 5 | `rolloff` | Rolloff | 3.0 – 6.0 | 4.0 | linear | dB/doubling | DbapSolver `a = R/6.0206` |
| 6 | `blur` | Blur | 0.0 – 1.0 | 0.10 | linear | norm (capped) | DbapSolver `r_s = blur·0.5·rigScale` |
| 7-14 | `w1`..`w8` | Weight 1..8 | 0.0 – 1.0 | 1.0 | linear | norm | DbapSolver `w_i` |
| 15 | `hullAtten` | Hull Atten | 0.0 – 3.0 | 1.0 | linear | dB/m | HullProcessor §3.5.1 |
| 16 | `airAmount` | Air | 0.0 – 1.0 | 0.35 | linear | norm | HullProcessor §3.5.2 |
| 17 | `outputGain` | Output | −24.0 – +12.0 | 0.0 | linear | dB | GainStage |

**All skews linear.** Every range here is already perceptually linear in its own units (dB, metres,
dB/doubling, normalised weight), so a skew would only obscure the automation curve. This matters
more than usual: the headline gesture is *automating* `w1..w8` and position, and a skewed lane
draws a curve that does not match what is heard.

**Display-value note for Stage 3:** `srcX`/`srcY` are stored normalised but **displayed in metres**
against the current venue bounding box. The value→text conversion is therefore **venue-dependent**
and cannot be a static lambda captured at parameter-construction time. It must read the live venue.
Per `pattern_webview_knob_readout_scaled_value`, the UI must not maintain its own min/max map — it
must ask the processor.

**Parameter-ID hazards checked:** none of `srcX srcY srcZ width rolloff blur w1..w8 hullAtten
airAmount outputGain` shadows a `juce::` free function (cf.
`critical_paramid_shadows_juce_free_function`, where `end`/`begin` collided). All 17 are safe as
both IDs and C++ identifiers.

### 6.2 Venue values — 42, separate `ValueTree`, NOT automatable

| Group | Count | Type | Range | Default | Notes |
|-------|-------|------|-------|---------|-------|
| `spk1..8 . x/y/z` | 24 | float | metres | §OQ4 | Measured positions including height |
| `rakeFront`, `rakeRear` | 2 | float | metres | 1.10 / 3.20 | Ear heights defining the sloped plane |
| `trim1..8` | 8 | float | −12 – +6 dB | 0.0 | Applied AFTER the DBAP solve (FUNC-07) |
| `map1..8` | 8 | string | channel-type name | identity map §3.2.4 | Speaker N → channel label |
| **Total** | **42** | | | | ✓ arithmetically confirmed |

The room envelope for the UI plan is **derived**, not stored: speaker bounding box + 15% margin per
axis (minimum 1.0 m). This is an explicit decision *not* to add two more venue values.

### 6.3 Non-parameters — UI actions

| Action | Behaviour |
|--------|-----------|
| Scenes `ALL` `FRONT` `REAR` `LEFT` `RIGHT` `SIDES` + 4 user slots | Writes all 8 weight parameters at once via `setValueNotifyingHost`, so scenes record as ordinary automation and fade (FUNC-06). The scene is not itself a parameter. |
| Verify mode | Solo-ping; see §OQ2. Level bounded independent of `outputGain`. |
| Venue save / load | Named `.venue` file. Never written by a musical preset (FUNC-05). |

---

## 7. Integration Points

### 7.1 Feature dependency graph

```
VenueModel ──┬──> ConvexHull2D ──┬──> HullProcessor ──┐
             │                    │                    │
             ├──> ChannelMap ─────┼────────────────────┼──> GainStage ──> output buffer
             │        ▲           │                    │        ▲
             │        │           ▼                    │        │
             │   (label map)   SourceShaper ──> DbapSolver ─────┘
             │                      ▲                           │
             │                      │                           │
             └──> rigScale, bbox, earHeight()               VerifyPing
                                                          (overrides GainStage,
                                                           reuses ChannelMap)
```

`VenueModel` is the root of everything geometric. `ChannelMap` is the only writer of output indices.
`VerifyPing` deliberately depends on `ChannelMap` and on nothing else in the chain — that is what
makes it a map test rather than a chain test.

### 7.2 Processing-order requirements (all mandatory)

| Order constraint | Why |
|---|---|
| Venue snapshot acquired **before** any geometry use, once per control block | A mid-block change would produce a torn geometry (hull from venue A, speakers from venue B) |
| `srcX/srcY` denormalised **before** sub-point construction | Sub-point offsets are in metres; the bearing must be computed in metres |
| Sub-points constructed **before** rake resolution | Each sub-point resolves `earHeight()` at its **own** y |
| Hull test **before** DBAP solve | The solve uses the *projected* position when outside |
| Hull test **per sub-point**, not per puck | The two sub-points can straddle the boundary; one in, one out is a normal state |
| `d_hull` captured **before** projection overwrites the position | Otherwise `d_hull` is always 0 |
| DBAP solve **before** hull gain trim | The trim modifies `v_i` after normalisation, deliberately breaking `Σv²=1` — which is why DSP-02 must be measured at the solver output, not at the plugin output |
| Venue trim folded **after** the solve, **before** smoothing | FUNC-07 says "after the DBAP solve"; folding into the target keeps it free per-sample |
| Channel map applied **last**, at the write | The single output-indexing site |
| Verify-ping override **after** the write, **before** metering | So the meters confirm the ping reached the intended channel |

### 7.3 Parameter interactions

| Interaction | Consequence |
|---|---|
| `w_i` × `Σv²=1` | Dropping weights **redistributes**, does not reduce level. A single non-zero weight puts the entire signal in one speaker at full level. This is the headline feature; the UI must make it legible (UI-03 meters do exactly this). |
| `blur` × `rigScale` × speaker height | The 3D model already supplies a physical blur floor from the flown speakers. `blur` is genuinely *additional*. A user who measures a flat rig will find `blur` far more sensitive — expected. |
| `rolloff` × `blur` | High `R` with low `r_s` produces the sharpest image; low `R` with high `r_s` approaches equal-level diffusion. DSP-02 must hold across the whole product of both ranges. |
| `width` × puck-at-centre | `wEff → 0` near the centroid by design (§3.4.2). A user setting `width = 6` and parking at centre will hear no spread; document it in the UI. |
| `hullAtten` / `airAmount` × inside-hull | Both are exactly no-ops inside the hull because `d_hull = 0`. No branch needed for correctness, only for the air-filter bypass. |
| venue trim × `outputGain` | Trim is per-speaker and venue-scoped; output gain is global and musical. Verify-ping bypasses **both**. |
| `srcX/srcY` × venue change | Normalised storage means a venue edit **moves the source in metres without changing the parameter**. Intended (portability), but it means a venue edit changes the sound — the UI should say so. |

### 7.4 Thread boundaries

| Thread | Owns | Never does |
|---|---|---|
| **Message** | `ValueTree` venue store, hull build, channel-map build, file I/O, preset apply, WebView | Touch `SmoothedValue` state; read the audio buffer |
| **Audio** | Control grid, DBAP solve, hull test/projection, filters, smoothers, buffer writes, meter stores | Allocate, lock, read the `ValueTree`, call `getSystemRandom()`, do file I/O |
| **Timer/UI (~30 Hz)** | Meter reads, `mapInvalid` banner, hull-classification display | Call into the DSP |

Crossing mechanisms: **message → audio** via the double-buffered `VenueSnapshot` + generation
counter (§3.6.6); **audio → UI** via `std::atomic<float>` meter array and a `std::atomic<bool>`
`mapInvalid`; **UI → parameters** via `setValueNotifyingHost` only (juce8-critical-patterns §5).

---

## 8. Implementation Risks

### R1 — Speaker→buffer channel map (HIGHEST)

**Complexity:** MEDIUM. **Risk:** **CRITICAL** — the defect is *silent*.
A wrong map does not crash, does not produce NaN, and passes `auval`, `pluginval` strictness 10 and
every unit test that does not specifically look for it. It scrambles the room and is audible only in
the hall, potentially only at the concert.

**Aggravating factor discovered in this research:** for `create7point1()` the enum-bit order happens
to equal the initializer-list order, so a hardcoded 0..7 map *appears correct today*. That is the
worst possible situation — it rewards the wrong mental model.

**Mitigations:** §3.2.3 construction (single site, `ChannelType`-keyed, permutation-validated);
§3.2.5 three-layer test including a source-parsed golden and an offline tone-per-speaker render;
verify-ping routed through the same map (§OQ2); meters read post-map so a wrong map is visible.

**Fallback if the map cannot be made reliable:** ship with the label map **locked to the identity
default and read-only**, exposing only verify-ping. Loses FUNC-03's flexibility, keeps the plugin
safe. Do not ship a writable map without Layer 3.

### R2 — Logic negotiates a different 8-channel set than expected (HIGH)

`kAudioChannelLayoutTag_Emagic_Default_7_1` (§3.2.2) shows Logic's native 7.1 ordering corresponds to
JUCE's `create7point1SDDS()` membership, not `create7point1()`. If Logic offers only SDDS, a plugin
that accepts only plain 7.1 **does not appear on the surround track at all** — and this cannot be
reproduced without Logic.

**Mitigation:** accept all three 8-channel sets (§4.2); key the label map on `ChannelType` so it
adapts; surface the negotiated set name in the Venue screen so the user can report what Logic chose.

**Fallback:** if Logic turns out to negotiate something else entirely, the `ChannelType`-keyed map
plus a "negotiated set: <name>" display makes the diagnosis a 30-second UI read rather than a
debugging session.

### R3 — Block-size invariance vs per-block solve (HIGH)

QUAL-03 and PERF-02 are incompatible under the obvious implementation (§3.6.1). Resolved by the
fixed 64-sample control grid (§3.6.2), plus a precise test protocol (§3.6.3). **Risk if forgotten:**
the plugin passes everything, and offline bounces differ subtly from what was heard while composing
— the worst possible failure for a fixed-media piece.

**Fallback:** raise the control grid to 32 samples (2× cost, still trivial) if 64 proves audible on
extreme automation; the invariance property is unaffected by the value chosen, only by the fact that
it is fixed and absolute-sample-aligned.

### R4 — Convex hull degeneracy (MEDIUM)

Real measured coordinates will push speakers 3 and 8 marginally inside or outside; a user may typo a
coordinate, producing duplicates or a collinear set. **Mitigations:** §3.1.2 dedup + area-scaled
epsilon; §3.1.6 degeneracy matrix with explicit `m < 3` paths; §3.1.3 classification surfaced in the
UI. **Fallback:** if `m < 3` after the chain, fall back to treating the source as always inside
(`d_hull = 0`) and raise a UI warning — a degenerate venue then behaves as plain DBAP with no hull
processing, which is defined and harmless.

### R5 — Centre-crossing L/R flip (MEDIUM → resolved at design time)

Found during this research (§3.4.2), not during Stage 4 listening. Fixed by the `rFade` spread
collapse. **Residual risk:** the fix changes behaviour at the centre in a way a user might report as
"width does nothing here" — a documentation task, not a bug.

### R6 — Sticky NaN in the air filter (MEDIUM)

The only recursive element. `pattern_envelope_follower_state_sticky_nan`: state reproduces a NaN
forever; unlike a ring buffer it never self-heals. **Mitigation:** per-block `std::isfinite` check +
`reset()` (§3.5.2). Also `pattern_biquad_nan_guard_sticky_silence` — a guard that resets *state* but
leaves bad *coefficients* produces sticky silence instead; the TPT filter's coefficient is derived
from a clamped `fc`, so clamp first, then reset.

### R7 — Two-screen WebView UI scale (MEDIUM)

Two screens, a canvas room plan with a live gradient backdrop, 8 in-plan weight controls, 8 meters, a
side-elevation strip, a 42-field measurement table. This is the largest UI in the repo.
**Mitigations:** `pattern_module_toplevel_init_tdz` (render against a JUCE-bridge stub before
integrating — the repo has `plugins/O-ReverseDelay/tests/ui-stub/` as precedent);
`pattern_webview_native_fn_bridge_gap` (grep-diff `getNativeFunction` against `withNativeFunction`);
`o-textureforge-cursor-bug` (canvas is a replaced element — explicit width/height + DPR backing
store, never left+right stretch). **Fallback:** ship UI-04 (gradient field) and UI-05 (elevation
strip) as v1.1 — both are `nice` priority and neither gates a concert.

### R8 — Venue measurement never happens (LOW, project risk)

The plugin is only correct for the hall once measured. **Mitigation:** ship a coherent default venue
(§OQ4) so everything is exercisable beforehand, and label it unmistakably as a placeholder in the
Venue screen.

### Overall project risk: **MEDIUM-HIGH**

Driven almost entirely by R1 and R2, both of which are *silent* and both of which live in territory
the repo has been burned by before (`critical_audiochannelset_is_a_bitset_not_an_order`,
`critical_logic_only_named_surround_formats`). The DSP itself is arithmetically simple and
well-specified; the risk is in the plumbing and the hall.

---

## 9. Architecture Decisions

| # | Decision | Alternatives rejected | Rationale |
|---|---|---|---|
| AD-1 | 2D hull, 3D distances | 3D convex hull | A 3D hull of a near-coplanar rig is a thin slab; every elevated source would read as outside. §3.1.1 |
| AD-2 | Fixed 64-sample control grid | Per-block solve; per-sample solve | Only construction satisfying PERF-01, PERF-02 and QUAL-03 simultaneously. §3.6 |
| AD-3 | Venue in a separate `ValueTree` | 42 APVTS parameters | The repo's own preset-reset pattern would wipe the venue; automatable routing is a performance hazard; exact round-trip. §4.1 |
| AD-4 | Label map keyed on `ChannelType`, stored by name | Buffer-slot index; raw enum int | A slot index makes `getChannelIndexForType()` a tautology and destroys the safety property. Names survive JUCE renumbering. §3.2.3 |
| AD-5 | Accept 7.1 + 7.1-SDDS + 5.1.2 | 7.1 only | `Emagic_Default_7_1` evidence that Logic's native 7.1 order is the SDDS membership. Three lines vs a class of unreproducible failures. §3.2.2 |
| AD-6 | Always two sub-points, `0.5` feed each | Branch at `width == 0` | Removes a level and gain-vector discontinuity; `width = 0` is the degenerate case, not a special case. §3.4.3 |
| AD-7 | `rFade` spread collapse near centroid | Hysteresis on the bearing; angular slew limit | The only fix that is continuous *and* stateless — hysteresis would make the output path-dependent and break block-size invariance. §3.4.2 |
| AD-8 | Hull attenuation linear in dB/m | Inverse-square; inverse-distance | DBAP has already renormalised; this is a musical cue and must automate predictably. §3.5.1 |
| AD-9 | Air filter pre-matrix, per sub-point | Per output channel (8 filters) | Hull distance is a source property. 4× cheaper and correct. §3.5.2 |
| AD-10 | SAFE pass-through on stereo output | Refuse the layout; silence; fold-down | Refusal kills Standalone and looks like a failed install; silence looks broken; a fold-down is deferred v1.1 territory. §OQ1 |
| AD-11 | Double-buffered POD snapshot | `std::atomic<shared_ptr>`; SpinLock | No refcount decrement can land on the audio thread (`pattern_retired_map_reaper_rt_free`). Snapshot is ~250 bytes. §3.6.6 |
| AD-12 | 17 musical parameters, no 18th added | Add hull/air bypass bools; add a custom plugin bypass | `hullAtten=0` and `airAmount=0` already defeat exactly; a duplicate bool creates a UI ambiguity. §11 |
| AD-13 | Meters read the written buffer post-map | Meter the computed `v_i` | Makes a channel-map error visible on the plan — a human line of defence on R1. §4.3 |
| AD-14 | Room envelope derived from the speaker bbox | Two more venue values | Keeps the venue count at exactly 42. §6.2 |

---

## 10. Open Questions — RESOLVED

### OQ1 — Stereo-track fallback in `isBusesLayoutSupported()`

**Resolution: accept mono/stereo output as an explicit SAFE pass-through mode — dry input at unity,
nothing spatialised, persistent UI banner "NOT SPATIALISED — 8-channel output required".**

**Rationale.** Refusing the layout kills the Standalone build on any 2-channel interface and makes
the plugin invisible in Logic before the user has switched the project to a surround format — which
reads as a failed installation. Silence reads as a broken plugin. A cheap amplitude fold-down is the
deferred v1.1 item's territory and would be mistaken for a real monitor mix. Pass-through with a
banner is the only option that cannot mislead: nothing is spatialised, and the user is told why.
FUNC-01's acceptance criterion already anticipates this ("except the Stage-0 stereo-fallback
policy"). The DBAP solve still runs in SAFE mode so the Room screen stays live — supporting the
brief's stated use case of preparing a piece away from the venue.

### OQ2 — Verify-ping design

| Aspect | Resolution | One-line rationale |
|---|---|---|
| **Signal** | Pink noise, band-limited **200 Hz – 8 kHz** (1-pole HP + 1-pole LP) | Broadband is unambiguously localisable where a sine is not; band-limiting protects an unknown small side-wall driver from full-range LF and protects the tweeter. |
| **Level ceiling** | **−20 dBFS RMS**, peak-clamped at **−6 dBFS**, fixed, **independent of `outputGain` and of all trims** | −20 dBFS RMS is the universal monitor-calibration reference level, so it is a number every engineer already recognises as safe in an uncalibrated rig. |
| **Envelope** | 20 ms raised-cosine fade in and out | A hard gate into an unknown amplifier is a thump; 20 ms is inaudible as a fade and eliminates it. |
| **Manual step** | Latched — ping runs continuously on the selected speaker until stepped or stopped, with a **120 s safety timeout** | One operator must be able to walk the hall while a speaker sounds; the timeout stops a forgotten ping running through a rehearsal. |
| **Auto-cycle** | **1.2 s on, 0.4 s gap**, order 1→8 by *speaker number* | 12.8 s per pass → four full passes inside the "under a minute" target of FUNC-04; the gap makes the boundary between speakers unmistakable. |
| **Routing** | Injected **at the channel map**, after everything else; all other channels hard-zeroed | This is the entire point — it tests the map, not the chain. Bypassing DBAP, weights, hull, trim and output gain means a ping failure has exactly one possible cause. |
| **Level control** | **None exposed in v1.0** | FUNC-04's acceptance criterion is a *fixed* conservative ceiling; a control is a way to get it wrong in a hall you do not own. |

### OQ3 — Numeric defaults

| Quantity | Value | Rationale |
|---|---|---|
| **Blur cap** | The 0-1 range *is* the cap: `r_s = blur · 0.5 · rigScale`, plus an absolute backstop `r_s ≤ 8.0 m` | At `blur=1` (half the RMS rig radius) the paper's precedence-effect warning is already in play; the absolute backstop stops a mistyped coordinate swamping the array. Both `static_assert`ed. |
| **`d_i` floor** | `kMinDistance = 0.05 m`, applied unconditionally, `static_assert`ed | Guarantees `d_i > 0` on every path including blur = 0 with the source exactly on a speaker — the QUAL-02 case. |
| **Hull attenuation** | Linear **dB per metre** of `d_hull`, floored at **−24 dB**; no reference distance needed | dB/m is directly what the parameter says, automates predictably, and the floor prevents `−inf`. |
| **Air LPF** | `fc = clamp(20000 · 2^(−airAmount·d_hull/3), 500, 20000)`; `dRef = 3.0 m` | One octave per 3 m at full amount is a dramatised cue that reads clearly at 5-15 m; the 500 Hz floor keeps the source present and the coefficient sane. `airAmount = 0` **skips the filter entirely**. |
| **Meter ballistics** | RAF loop, `current += (target−current)·speed`, **attack 0.5 / decay 0.12**; scale −60..0 dBFS → 0..1; 1.5 s peak-hold releasing at 20 dB/s | Faster attack than a VU meter (0.4) because these indicate spatial *position*, not loudness — the composer needs the instantaneous distribution. Slow decay keeps a transient legible. Per juce8-critical-patterns §20. |
| **Smoothing time** | 5 ms linear, per sample | Kills zipper without smearing a scene change into a crossfade; comfortably longer than the 1.33 ms control grid. |
| **Control grid** | 64 samples, absolute-sample-aligned | 1.33 ms at 48 kHz — below any audible control-rate artefact, and the mechanism that makes the plugin block-size invariant. |

### OQ4 — Default venue scale (metres)

**Hall envelope (for UI proportions only, derived — not stored):** 13.0 m wide × 22.0 m deep, stage
occupying y ∈ [0, 4.0].

Derivation from the one hard fact available (255 seats; published dimensions for Roy Barnett Recital
Hall could not be found): at 0.55 m seat width with two 1.1 m aisles in a 13 m width → ~19 seats per
row; 255 / 19 ≈ 13.4 rows; at 0.95 m row pitch ≈ 12.8 m of seating, plus a 2 m front cross-aisle and
1.5 m at the rear ≈ 16.3 m, plus a 4-5 m stage ≈ 21-22 m depth. A 13 × 22 m deep rectangular room
is coherent with 255 seats.

**Mapping the traced layout.** The traced normalised coordinates carry *relative* geometry (the even
side spacing, the inboard rear pair) but their absolute margins are artefacts of the sketch frame,
not wall offsets. They are therefore re-based on their own bounding box and scaled to a plausible
speaker span: 12.0 m between the side-wall lines (0.5 m in from each wall), 15.0 m front-to-rear.

| # | Position | x (m) | y (m) | z (m) |
|---|----------|-------|-------|-------|
| 1 | front-left | 0.50 | 4.50 | 4.50 |
| 2 | front-right | 12.50 | 4.50 | 4.50 |
| 3 | right-2nd | 12.50 | 9.85 | 4.70 |
| 4 | right-3rd | 12.50 | 16.00 | 5.10 |
| 5 | back-right | 9.80 | 19.50 | 5.40 |
| 6 | back-left | 3.20 | 19.50 | 5.40 |
| 7 | left-3rd | 0.50 | 16.00 | 5.10 |
| 8 | left-2nd | 0.50 | 9.85 | 4.70 |

| Rake | Value | Meaning |
|---|---|---|
| `rakeFront` | **1.10 m** | Seated ear height at the front row, floor at stage level |
| `rakeRear` | **3.20 m** | 2.1 m of rise over ~13 rows — steep for a recital hall, consistent with the brief |

Derived: `rigScale ≈ 7.95 m`, centroid ≈ (6.50, 12.46, 4.93), bbox x[0.50, 12.50] y[4.50, 19.50].

**Why the heights are graded rather than uniform.** Two reasons, and the second is an engineering
argument: (1) wall mounts are installed at a constant height above the *local* floor, so over a rake
they rise in absolute terms; (2) **a uniform default `z` would make every `(z_i − z_s)` difference
identical across speakers, which would hide a dropped `z` term in DSP-01's acceptance test.** A
graded default exercises the 3D path from the first build.

**These are plausible placeholders, not measurements.** The Venue screen must label them as such,
and the measurement session remains a prerequisite for concert use.

### OQ5 — Venue data storage

**Resolution: a separate `ValueTree`, attached as a child of `apvts.state`.** Confirmed as the brief
leaned. Full structure, round-trip code and the four-part justification are in §4.1 — the decisive
argument being that the repo's established `pattern_preset_apply_needs_reset_to_defaults` (reset all
parameters to defaults before applying a preset) is in **direct contradiction** with FUNC-05 if the
venue lives in the APVTS.

---

## 11. Parameter-Count Discrepancy — RESOLVED

**Finding: 18 was an arithmetic slip. The correct count is 17. No 18th parameter was intended, and
none should be added.**

**The slip is demonstrable.** BRIEF.md §Parameters "Musical" table has **10 rows**:

`srcX`, `srcY`, `srcZ`, `width`, `rolloff`, `blur`, `w1..w8`, `hullAtten`, `airAmount`, `outputGain`

One row (`w1..w8`) collapses 8 parameters. The correct expansion is `9 + 8 = 17`. The figure 18 is
obtained by `10 rows + 8 weights` — i.e. counting the collapsed weight row **and** its eight
expansions. That is the only arithmetic that yields exactly 18 from this table.

**Cross-check against the venue table, which is correct at 42.** The venue table's four rows are
`spkN.x/y/z` (24), `rakeFront`/`rakeRear` (2), `trim1..8` (8), `map1..8` (8) = 42. It contains no
row that is counted as *both* a row and its expansion, which is exactly why it did not slip. The two
tables' error behaviour is consistent with the double-count hypothesis and with no other.

**No 18th parameter should be added.** The three candidates raised in parameter-spec-draft.md were
each evaluated:

| Candidate | Verdict |
|---|---|
| Explicit hull bypass (bool) | **Reject.** `hullAtten = 0` defeats it exactly and is automatable. A parallel bool creates an ambiguity (bypass ON with `hullAtten = 2` — what does the UI show?) and duplicates state across the preset boundary. |
| Explicit air bypass (bool) | **Reject as a parameter, ACCEPT as an implementation requirement.** `airAmount = 0` must *skip the filter and reset its state*, not merely set `fc = 20 kHz` — a one-pole at 20 kHz is not transparent at 44.1/48 kHz. This is §3.5.2 and needs no parameter. |
| Custom plugin bypass | **Reject.** Hosts provide bypass; JUCE surfaces it via `getBypassParameter()`. A custom one duplicates host behaviour and confuses automation. |

**Action taken:** BRIEF.md line "**18 musical parameters.**" corrected to 17 with a footnote.
STATUS.md's "18 musical parameters" line corrected. parameter-spec-draft.md's flagged discrepancy is
hereby resolved in favour of 17. **The binding count for Stage 1 is 17.**

---

## 12. Special Considerations

### Thread safety
Covered in §7.4. Three rules: the audio thread never reads the `ValueTree`; the message thread never
touches `SmoothedValue`; all UI→parameter writes go through `setValueNotifyingHost`.

### Performance
~50 flops/sample in the inner loop, ≤ 16 `std::pow` per 64 samples in the control block, an O(8) hull
inside-test per control block, and an O(≤8) projection only when outside. Estimated **< 0.5% of one
core at 48 kHz**. This plugin is not CPU-bound; do not trade correctness for speed anywhere in it.

### Denormals
`juce::ScopedNoDenormals` at the top of `processBlock`. The air LPF is the only recursive element and
its 500 Hz floor keeps it well away from denormal territory on normal input, but the guard is free.

### Sample-rate handling
`prepareToPlay(sampleRate, samplesPerBlock)` must: rebuild the channel map; `prepare()` both air
filters with the new `ProcessSpec`; `reset(sampleRate, 0.005)` all 17 smoothers; reset
`absoluteSampleCounter = 0`; force the solver dirty. **The control grid is 64 *samples*, not a fixed
time** — so its period in milliseconds varies with sample rate (1.45 ms at 44.1 k, 0.67 ms at 96 k).
That is correct: block-size invariance is a *sample-domain* property, and a time-domain grid would
reintroduce rate-dependence.

### Latency
**Zero.** No lookahead, no oversampling, no FFT. Do **not** call `setLatencySamples()` at all
(`getLatencySamples()` is non-virtual in JUCE 8 — a common trap in this repo).

### Build configuration

```cmake
juce_add_plugin(OuariconOctagon
    COMPANY_NAME             "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE              OuOc            # verified unused across all 39 plugins
    FORMATS                  VST3 AU Standalone
    PRODUCT_NAME             "O-Octagon${OUARICON_DEV_SUFFIX}"
    VERSION                  1.0.0           # NOT PLUGIN_VERSION — that keyword is silently ignored
    NEEDS_WEB_BROWSER        TRUE
    NEEDS_WEBVIEW2           TRUE
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    IS_MIDI_EFFECT           FALSE
    VST3_CATEGORIES          "Spatial" "Fx"
    AU_MAIN_TYPE             "kAudioUnitType_Effect"
    # DO NOT set PLUGIN_CHANNEL_CONFIGURATIONS — counts only, not types; breaks surround detection.
)
```

Plus, per juce8-critical-patterns: `juce_generate_juce_header()` **after** `target_link_libraries`
and **before** `target_compile_definitions`; `JUCE_WEB_BROWSER=1`, `JUCE_USE_CURL=0`,
`JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`; and **`juce::juce_dsp` linked** (required by
`FirstOrderTPTFilter`).

**Target name is `OuariconOctagon`; the folder is `O-Octagon`.** These differ, as they do for 11 of
37 existing plugins. `scripts/build-and-install.sh` resolves the `juce_add_plugin` target, not the
folder name (`build_script_target_name_vs_folder`) — any packaging script added for this plugin must
do the same.

**`juce_add_binary_data` filename hazard:** hyphens are stripped from generated identifiers
(`preset-manager.js` → `presetmanager_js`). Name UI resources without hyphens, or expect the mangled
symbol.

---

## 13. Research References

### Professional plugins and prior art

| System | What was taken |
|---|---|
| **Jamoma `j.dbap`** (Lossius) | The reference DBAP implementation. Confirms the convex-hull treatment: *"if the source is located outside, then the projection of the source onto the boundary of the hull is calculated, defined as the point on the boundary with the shortest possible distance to the source. This source position is substituted for its projection in subsequent calculations."* — exactly the §3.1.5 construction. Also confirms *why*: outside the field, *"the relative difference in distance from source to each of the speakers diminishes, resulting in progressively less difference in levels between the speakers."* |
| **SpatGRIS / ControlGRIS** (GRIS, U. de Montréal) | The professional standard for this use case — VBAP + DBAP, arbitrary layouts, 256 I/O, and a **separate speaker-setup editor application** (`SpeakerView`). Validates the two-screen split: performance surface and measurement surface are different tools with different tempos. Rejected as a solution here because it requires routing audio out of Logic (BlackHole/JackTrip) plus parallel OSC, and there is a published Logic-specific octophonic incompatibility. |
| **IRCAM Spat** | Measured-geometry-first spatialisation; its "Setup" page is a dedicated speaker-editor screen. Same two-screen conclusion. |
| **GRM Acousmonium / BEAST** | The practice the weight parameters exist to serve — spatial orchestration as composition, not mixing. The DBAP paper's own worked example. |
| **Ville Pulkki, VBAP** | The deliberate counter-example. Distance-blind and listener-position-dependent; lower average error but *higher variance across listener positions* than DBAP — and variance is the metric that matters for an audience distributed through a hall. |
| **O-Orbit** (this repo, v1.0.0) | Sibling VBAP orbiter. Borrowed: the `Source/{Data,DSP}/` layout, the permissive mono/stereo-in `isBusesLayoutSupported()` shape, the queue-layout-for-the-audio-thread pattern for geometry changes. **Not** borrowed: the motion engine, VBAP, SAF. O-Octagon does not depend on SAF and must not link it. |
| **Monitor-calibration practice** | −20 dBFS pink noise is the standard reference level for monitor calibration; adopted as the verify-ping ceiling (OQ2). |

### JUCE source (verified in-tree, JUCE 8.0.14)

| Fact | File:line |
|---|---|
| Buffer order is enum-bit order | `juce_audio_basics/buffers/juce_AudioChannelSet.cpp:514-527` |
| `getChannelTypes()` uses the same bit iteration | `juce_AudioChannelSet.cpp:529-534` |
| `ChannelType` enum values | `juce_AudioChannelSet.h:402-432` |
| `create7point1()` membership | `juce_AudioChannelSet.cpp:567` |
| `create7point1SDDS()` membership | `juce_AudioChannelSet.cpp:568` |
| `octagonal()` membership | `juce_AudioChannelSet.cpp:572` |
| `getTypeOfChannel()` | `juce_AudioChannelSet.h:577` |
| `getAbbreviatedChannelTypeName()` | `juce_AudioChannelSet.h:550` |
| **`kAudioChannelLayoutTag_Emagic_Default_7_1`** | `juce_audio_basics/native/juce_CoreAudioLayouts_mac.h:117` |
| `kAudioChannelLayoutTag_MPEG_7_1_C` wire order | `juce_CoreAudioLayouts_mac.h:86` |
| `kAudioChannelLayoutTag_Octagonal` wire order | `juce_CoreAudioLayouts_mac.h:92` |
| `SmoothedValue::reset/setTargetValue/getNextValue` | `juce_audio_basics/utilities/juce_SmoothedValue.h:265, 284, 309` |
| `FirstOrderTPTFilter` API | `juce_dsp/processors/juce_FirstOrderTPTFilter.h:74, 80, 91, 94, 97, 135` |
| `APVTS::copyState/replaceState` | `juce_audio_processors/utilities/juce_AudioProcessorValueTreeState.h:391, 404` |
| `ValueTree::createXml/fromXml/writeToStream` | `juce_data_structures/values/juce_ValueTree.h:442, 448, 470, 473` |
| `Random::nextFloat` / `getSystemRandom` | `juce_core/maths/juce_Random.h:89, 138` |
| `juce::dsp::Panner` is stereo-only | `juce_dsp/processors/juce_Panner.h` |

### Local research

- `research/logic-pro-multichannel-octaphonic-dbap.md` — the locked architecture. §1 container
  constraint, §2 aufx/aumu split, §4 the bitset trap, §5 DBAP formulation, §6a bounce order and the
  identity label map, §6 open items.
- `research/juce8-multichannel-spatial-audio.md` — `AudioChannelSet` reference, bus-negotiation
  flow, the `PLUGIN_CHANNEL_CONFIGURATIONS` warning, confirmation that JUCE ships no spatial DSP.
- `research/spatial-audio-per-grain-spatialization.md` §1 — VBAP math, for the **deferred v1.1** A/B
  mode only. Not used in v1.0.
- `research/sound-spatialization-algorithms.md`, `research/spatial-audio-plugins-market-research.md`.
- `troubleshooting/patterns/juce8-critical-patterns.md` — §4 bus config in the constructor, §5
  thread rules, §9 `NEEDS_WEB_BROWSER`, §11 unique_ptr member order, §12 the three-argument
  `WebSliderParameterAttachment`, §13 `check_native_interop.js`, §20 meter ballistics, §21
  `type="module"`.

### External

- Lossius, Baltazar, de la Hogue — *DBAP – Distance-Based Amplitude Panning*, ICMC 2009,
  **2011-04-14 revised** (jamoma.org). The revision corrects equations 3-6 and 9-10; copies
  circulating with the originals are wrong.
- Jamoma API — `j.dbap.cpp` / `j.dbap.h` reference implementation.
- *Improving the Distance-based Amplitude Panning Algorithm*, arXiv 2109.08704 — speaker-placement
  agnosticism; noted for a possible v1.1 refinement, not used in v1.0.
- Andrew's monotone chain — cp-algorithms.com convex-hull article; the `<=` vs `<` cross-product
  choice is exactly the include/exclude-collinear switch §3.1.2 depends on.

---

## 14. Notes for Implementers

1. **The channel map is the whole plugin's risk budget.** If you change one thing about §3.2, change
   nothing. It has one construction site, it is keyed on `ChannelType`, and it is permutation-validated.
   All three properties are load-bearing.
2. **Never "optimise" a `getNextValue()` out of the inner loop.** §3.6.4.
3. **Never replace the control grid with a per-block solve.** §3.6.1 explains what breaks and why no
   test except the block-size comparison would catch it.
4. **Build the offline render harness in Stage 2, not Stage 4.** Precedent:
   `plugins/O-ReverseDelay/tests/render-harness/`. Guard `createEditor` with `#if JUCE_WEB_BROWSER`
   so the harness survives the Stage-3 WebView swap (`pattern_render_harness_breaks_on_webview_editor`).
5. **`hullAtten` and `airAmount` at 0 must be exact no-ops**, and for the air filter that means a
   skip-and-reset branch, not a 20 kHz cutoff.
6. **Factory presets must be authored in engineering units and converted with `convertTo0to1`**, not
   as linear fractions (`pattern_factory_preset_normalized_ignores_skew`). All ranges here are
   linear so the risk is low, but the habit prevents the next plugin's bug.
7. **MSVC:** hoist any `SafePointer(this)` init-capture to a local before use in a nested lambda,
   and mark `constexpr` locals inside lambdas `static` (C3493). Both bite on the first Windows CI run.
