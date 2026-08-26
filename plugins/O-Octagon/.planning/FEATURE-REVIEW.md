---
plugin: O-Octagon
version_reviewed: 1.3.0
reviewed: 2026-08-25
scope: features-and-capabilities
method: capability inventory from BRIEF/REQUIREMENTS/CHANGELOG/index.html + web research of the spatializer field
gaps:
  high: 3
  medium: 4
  low: 3
status: propose_only
---

# O-Octagon v1.3.0 — Feature & Capability Review

**Reviewed:** 2026-08-25 · **Scope:** what it does today, how it sits against the field, and what would
most improve it. No code was changed by this review.

## Verdict

O-Octagon v1.3.0 occupies a genuinely empty market slot: no other shipping VST3/AU plugin renders measured-geometry 3D DBAP in-DAW for irregular concert arrays — SPAT Revolution has DBAP but is a $1,790 external application, SpatGRIS's CUBE mode is free but is a realtime server that cannot bounce offline, and every free ambisonics suite (IEM, SPARTA, Envelop) assumes the regular sweet-spot array that irregular halls break. Its venue-first tooling (sloped audience plane, .venue/preset separation, verify-ping, speaker-output assignment, per-speaker trims) is unmatched anywhere. The three gaps that matter most for its composer/venue niche are: binaural/stereo monitoring fold-down (the away-from-hall workflow is currently inaudible — every competitor has this), a motion engine borrowing O-Orbit's path vocabulary (trajectories are table stakes from ControlGRIS to L-ISA), and per-speaker delay compensation (a small add that completes the venue-calibration story). OSC input, snapshot/cue morphing, and a mono decorrelator behind the width control are the strongest medium-priority follow-ons; higher channel counts for Reaper would widen the niche but multiply the channel-map risk surface deliberately locked down for Logic.

## What O-Octagon does today

- 3D DBAP panning (Lossius 2011 revised equations) over 8 measured speaker positions: srcX/srcY/srcZ puck, rolloff 3-12 dB/doubling, rig-scaled spatial blur, stereo width via perpendicular sub-points — constant intensity (sum v_i^2 = 1) everywhere, verified against an independent oracle to 1e-7
- Measured venue model as first-class data: 42 values (8x x/y/z in metres, sloped audience plane via front/rear rake ear heights, 8 calibration trims, 8 output labels), editable in a dedicated Venue screen and saved to standalone .venue files — deliberately separate from musical presets so a patch is portable between halls
- Convex hull computed with collinearity handling; outside-hull sources project to the hull with a dB/metre gain trim (floored -24 dB) and a distance-driven air-absorption LPF, both independently defeatable; srcZ proximity level cue (+/-6 dB from the un-normalised 1/k field, v1.3.0)
- Spatial orchestration via 8 per-speaker weights (w1-w8), all automatable; 6 named scenes (ALL/FRONT/REAR/LEFT/RIGHT/SIDES) derived from geometry not indices, plus 4 user slots — scene clicks write ordinary automation so scenes can be faded between
- Speaker->output assignment (v1.1.0): double-click popover per glyph with swap semantics, per-glyph '->k' badges, one-click 'Direct 1-8' / 'Roles' order presets — solving the CoreAudio Emagic_Default_7_1 device-order permutation
- Verify-ping: level-bounded pink-noise solo ping per speaker, manual step + auto-cycle, wiring confirmable in under a minute in a shared hall
- Room screen: proportioned plan with DBAP level-field gradient backdrop, explicit hull overlay, draggable puck, in-plan weight controls, 8 post-map/post-trim meters with peak-hold, side-elevation strip showing the raked audience line and source height
- SAFE mode (defined fold + banner on non-8-channel layouts), MAP banner on invalid label maps, hover-help tooltips on 49 controls (v1.2.0), preset migration hooks (v1.3.0)
- Six factory presets (room character only — a preset never moves the source or scene, via loadPreserving), session state carries venue + music + scenes together
- Transport: mono/stereo in -> 7.1 (also 7.1-SDDS, 5.1.2) as an 8-channel carrier; channel map derived at prepareToPlay, never hardcoded; block-size invariant so offline bounce == realtime; pluginval strictness 10, auval, Logic Pro 12.3 host-validated including measured bounce and device channel orders; per-commit CI incl. Windows MSVC

## Where it already wins

- Only in-DAW plugin found that renders measured-geometry 3D DBAP for irregular arrays directly on the track: SPAT Revolution has DBAP but costs $1,790 and lives outside the DAW; SpatGRIS's CUBE mode is free but is a separate spatialization server needing loopback routing and cannot do an offline bounce — O-Octagon bounces bit-identically in Logic
- Venue-as-data discipline nobody else matches at this scale: sloped audience plane, per-speaker trims, .venue files separated from musical presets so pieces are portable between halls — L-ISA/SPAT have speaker configs but not the rake model or the preset/venue store split
- Practical shared-hall tooling absent from the whole field: verify-ping wiring check in under a minute, speaker->output assignment with swap semantics and device-order presets, MAP/SAFE banners — competitors assume you own and control the rig
- Constant-intensity weight scenes are Acousmonium-style spatial orchestration as recordable automation — GRM Spaces and diffusion consoles do this live but not as portable, geometry-derived, host-automatable scenes
- Rigour: 95+ offline probes, oracle-verified DBAP, measured (not assumed) Logic bounce/device channel orders, block-size invariance — a correctness story none of the small/free competitors document
- Free (AGPL) against SPAT Revolution $1,790, L-ISA subscription, dearVR PRO 2 $199 — and unlike the free ambisonics suites (IEM, SPARTA, Envelop) it does not force the ambisonic encode/decode detour that smears irregular, non-spherical arrays

# Competitive landscape

## Direct competitors (arbitrary/irregular speaker layouts)

**Flux/IRCAM SPAT Revolution** — the reference commercial product, **$1,790**. Unlimited sources, channel-based/HOA/binaural/transaural streams, and crucially offers **DBAP 2D & 3D alongside VBAP/VBIP/KNN** ([Flux panning types](https://desk.flux.audio/hc/en-us/articles/360008231154-Panning-types), [Flux product page](https://www.flux.audio/project/spat-revolution/), [JRRshop pricing](https://www.jrrshop.com/ircam-tools-spat-revolution.html)). Full room/reverb engine, up to 384 kHz, and an advanced OSC engine (8 simultaneous connections, ADM-OSC, SpaceMap Go grammars). It is an external application fed from the DAW, not an in-track renderer.

**SpatGRIS 3/4 + ControlGRIS (GRIS, Université de Montréal)** — free, open source, the closest philosophical neighbour. Up to 256 ins/outs; five modes: DOME (VBAP), **CUBE (MBAP — a DBAP-family metric panner for non-dome layouts)**, HYBRID, BINAURAL (HRTF monitoring), STEREO ([GitHub](https://github.com/GRIS-UdeM/SpatGRIS), [SoundingFuture overview](https://soundingfuture.com/tools/spatgris)). ControlGRIS is the in-DAW AAX/AU/VST3 companion that **draws 2D/3D trajectories** and streams them by OSC to the SpatGRIS server. Weaknesses O-Octagon exploits: server architecture (no offline bounce, loopback routing), no venue/preset separation, no wiring-verification tooling, no sloped-audience model.

**L-Acoustics L-ISA Studio** — object-based scene authoring, speaker layouts to **57 speakers**, binaural monitoring, **snapshot engine with trajectories** triggered by MIDI PC/MTC/OSC, head-tracker support ([L-Acoustics](https://www.l-acoustics.com/products/l-isa-controller/), [Mix review](https://www.mixonline.com/technology/l-acoustics-l-isa-studio-review)). Subscription-licensed and oriented toward L-Acoustics hardware shows; same family as **d&b Soundscape/En-Scene** (hardware processor, En-Space room engine) — feature references, not realistic alternatives for a university hall rig.

## Ambisonics suites (free, layout via decoding)

**IEM Plug-in Suite** — 20+ free open-source plugins to 7th order; AllRADecoder can target arbitrary layouts, plus RoomEncoder, granular encoder, multiband compressor ([plugins.iem.at](https://plugins.iem.at/), [overview](https://www.axisambisoniclab.com/free-ambisonic-plugins/)). **SPARTA/COMPASS** (Aalto) — to 10th order, parametric soundfield analysis/resynthesis, binaural, panners incl. VBAP ([SPARTA site](https://leomccormack.github.io/sparta-site/)). Both discard distance and assume a reasonably regular array around a sweet spot — exactly the assumption the Barnett rig breaks. **Envelop for Live** — free Max-for-Live 3rd-order ambisonics for Ableton only ([GitHub](https://github.com/EnvelopSound/EnvelopForLive)); **ICST Ambisonics** — Max externals with algorithmic source-motion modules ([ambisonics.ch](https://ambisonics.ch/)).

## Creative multichannel / other

**GRM Tools Spaces** — Spaces 3D moves a 1-64 ch source in a 1-64 ch space; SpaceFilter (4-band spatialization), SpaceGrain (100 spatialized grains) ([INA GRM](https://inagrm.com/en/store/product/15/spaces)). Motion-and-texture oriented, no measured geometry. **dearVR PRO 2** — $199, 35 output formats, binaural + 3rd-order ambisonics, distance simulation, 46 acoustics presets ([Sennheiser newsroom](https://newsroom.sennheiser.com/dear-reality-releases-dearvr-pro-2-spatializer-plugin-xhdtie)) — binaural/Atmos-first, no irregular-array support. **Sound Particles Space Controller/Density** — phone-as-tracker panning up to 7.1.2 and multichannel voice-cloud generation, ~$99-599 ([Sound On Sound](https://www.soundonsound.com/reviews/sound-particles-space-controller), [bundle](https://www.soundonsound.com/news/sound-particles-introduce-spatial-music-bundle)). **Reaper's ReaSurroundPan** — free-with-DAW octophonic panner with parameter-modulation random trajectories ([whyreaper tutorial](https://whyreaper.com/tutorial/13-create-surround-sound-random-panning-trajectories-in-2d-and-3d-using-reasurroundpan-two-minute-or-so-tutorials-for-reaper/)) — no distance model.

## Open-source DBAP specifically

No shipping DBAP **VST/AU plugin** was found besides SPAT Revolution's mode and SpatGRIS's CUBE. DBAP lives in externals: Jamoma/Max (the original authors' implementation), Pd ([kronihias/dbap](https://github.com/kronihias/dbap)), SuperCollider ([woolgathering/dbap](https://github.com/woolgathering/dbap)). O-Octagon occupies a genuinely empty slot: in-DAW, measured-venue, 3D DBAP with offline-bounce parity.

## Gap analysis

Ordered by priority, then by effort within each band. "Effort" is a rough implementation-size estimate
(small = days, medium = a stage-sized cycle, large = a milestone).

### Per-speaker delay (distance/time-alignment) compensation

> **✅ SHIPPED in v1.4.0 (2026-08-26).** Built as scoped: 0–50 ms per speaker beside the trims, applied post-solve, ms/metres toggle, `Derive` filling all eight from align-to-farthest against the audience-plane centroid. `.venue` schema 1 → 2, additive; pre-v1.4.0 rooms render bit-identically. See the v1.4.0 CHANGELOG entry.

`priority: high` · `effort: small`

A venue-scoped delay per speaker (0-50 ms or entered as metres), sitting beside the existing trims, applied post-solve. Auto-derive a suggestion from the measured distances to a chosen reference point.

**Why it matters:** The venue model measures positions but the DSP only compensates level; on a deep rectangular hall with 3 speaker pairs down the walls, arrival-time skew is audible and every PA-world tool (L-ISA, Soundscape, even basic system processors) has it. Completes the venue-calibration story cheaply — 8 delay lines and 8 venue values.

### Binaural / stereo monitoring fold-down

`priority: high` · `effort: medium`

A monitor mode that folds the 8 solved feeds to headphones (position-derived stereo pan+delay+distance gain at minimum; HRTF convolution as a stretch) written into 2 of the 8 carrier channels with the rest muted, clearly bannered as MONITOR so it can never leak into a bounce. Already listed as deferred in the brief.

**Why it matters:** The brief's own use case — 'preparing and revising a piece away from the venue' — is currently impossible to hear. SpatGRIS (BINAURAL mode), L-ISA Studio, SPAT Revolution and dearVR all treat headphone monitoring as table stakes; it is the single most-used feature for a composer who visits the hall twice a year.

### Motion engine: orbits, paths, tempo-synced LFO on the puck

`priority: high` · `effort: large`

Generative motion for srcX/srcY/srcZ — circular/elliptical orbits, figure-8s, line sweeps, random walks, tempo-synced rate, depth, and phase — written as parameter modulation inside the plugin (not just host automation). O-Orbit in this repo already owns the path vocabulary to borrow, per the brief.

**Why it matters:** ControlGRIS's trajectory designer, L-ISA's snapshot trajectories, and even free ReaSurroundPan's parameter-modulation randomness all make motion cheap; hand-drawing Logic automation for a 10-minute rotation is the most tedious task in the current workflow. Biggest creative-capability gap.

### Mono decorrelator behind the width control

`priority: medium` · `effort: small`

An optional all-pass/velvet-noise decorrelation of the two sub-point feeds so width is audible on mono material. Flagged in the v1.3.0 changelog as a known limitation and deliberate future improvement.

**Why it matters:** Most fixed-media stems are effectively mono; today width 'separates the L and R feeds in space' but two identical feeds a few metres apart mostly comb. A small DSP add that makes an existing headline control deliver on mono sources.

### OSC input for live diffusion control

`priority: medium` · `effort: medium`

Receive OSC (juce_osc) for puck position, weights, and scene triggers — a phone/tablet (TouchOSC/Lemur) or fader box becomes a diffusion console; optionally echo state out (ADM-OSC-ish grammar) for lighting/show-control sync.

**Why it matters:** Live acousmatic diffusion is performed, not just automated; SPAT Revolution (8 OSC connections), L-ISA, and SpatGRIS are all OSC-native. Turns O-Octagon from a fixed-media renderer into a performance instrument in the hall. Needs care re Logic's plugin threading but JUCE's OSC classes make the transport trivial.

### Snapshot morphing / cue list beyond weight scenes

`priority: medium` · `effort: medium`

Extend the scene system to full musical-state snapshots (position + width + blur + weights) with a morph time and a next/previous cue trigger — still written through parameters so the host records the interpolation.

**Why it matters:** Concert diffusion runs on cues; L-ISA's snapshot engine (MIDI PC/MTC/OSC triggered) is the model. The scene infrastructure (geometry-derived sets, gesture-bracketed parameter writes, fadeability already proven by probe CJ) is 70% of the machinery.

### Higher channel counts / discrete-N builds for Reaper

`priority: medium` · `effort: large`

A build (or bus-policy extension) accepting discrete 8/12/16-channel layouts in hosts that allow arbitrary channel counts (Reaper, Nuendo), with the venue table growing to N speakers. The Logic 7.1-carrier constraint stays for AU; VST3 in Reaper has no such ceiling.

**Why it matters:** Many concert rigs are 12-24 speakers; SpatGRIS does 256, SPAT unlimited. Reaper is the DAW of choice for much of the electroacoustic community, and the current 8-speaker cap plus the 7.1 carrier is the main reason a SpatGRIS user would not switch. Expands the addressable niche substantially but multiplies the channel-map test matrix — the exact area where the silent bugs live.

### Venue interchange import (SpatGRIS / L-ISA / ADM speaker configs)

`priority: low` · `effort: small`

Import speaker positions from SpatGRIS speaker-setup XML or L-ISA layout files into the .venue store.

**Why it matters:** Halls that already have a SpatGRIS or L-ISA config measured (common in Canadian/European electroacoustic studios) become one-click venues; cheap goodwill and adoption lever, low urgency.

### Multiple simultaneous sources per instance

`priority: low` · `effort: large`

2-4 pucks per instance, each with its own weights/width/blur, mixed into one solve. Workaround today is one instance per stem, which is fine for mixing but blocks inter-source gestures (swaps, mirrors, chase).

**Why it matters:** SPAT/L-ISA are multi-object by design, but the per-instance model matches Logic's track paradigm and per-stem automation stays cleaner; value is real but lower than motion/monitoring. Doubles the parameter surface and the UI complexity.

### VBAP A/B mode and internal hull-distance reverb

`priority: low` · `effort: large`

The two remaining brief deferrals: a triangulated 3D VBAP mode for centre-position A/B, and an 8-out diffuse reverb driven by hull distance (must be internal — aufx gets one bus).

**Why it matters:** VBAP A/B is academically interesting but contradicts the plugin's thesis (DBAP exists because VBAP fails on this rig); the reverb adds depth cues SPAT/dearVR provide via room engines but is a significant DSP project competing with simply using a send in the DAW before O-Octagon.

## Recommended sequencing

If the goal is maximum user-visible improvement per unit of work, the order is:

1. ~~**Per-speaker delay compensation** (high value, small)~~ — **DONE, v1.4.0.** 8 delay lines plus 8 venue
   values; the venue-calibration story the plugin half-told is now complete. Landed inside a single
   `/improve` cycle, as scoped.
2. **Binaural / stereo monitoring fold-down** (high value, medium) — unblocks the brief's own stated use case
   (working on a piece away from the hall), which is currently inaudible. Every competitor treats this as
   table stakes.
3. **Mono decorrelator behind width** (medium value, small) — the v1.3.0 changelog already names this as the
   known limitation of an existing headline control.
4. **Motion engine** (high value, large) — the biggest creative gap, but a milestone rather than an improvement.
   O-Orbit in this repo already owns the path vocabulary to borrow.
5. **OSC input** and **snapshot/cue morphing** (both medium) — these turn a fixed-media renderer into a
   performance instrument; the scene infrastructure is reportedly ~70% of the cue machinery already.

Higher channel counts (Reaper/Nuendo discrete-N) is the one item to approach with caution: it widens the
addressable niche most, but it multiplies the channel-map test matrix — which is precisely where the silent
bugs in this plugin have historically lived.
