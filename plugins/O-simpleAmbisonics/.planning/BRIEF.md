# O-simpleAmbisonics - Creative Brief

## Overview

**Type:** Audio Effect (Pedagogical Ambisonic Spatializer — encoder + rotator + binaural decoder in one plugin)
**Core Concept:** A deliberately simple ambisonic spatializer built for pedagogy — one mono source, positioned on a sphere, encoded to 1st/2nd/3rd-order B-format, optionally rotated, and decoded to binaural headphones inside the plugin, so students learn how ambisonics works by hearing AND seeing the spherical-harmonic channels respond to every move.
**Status:** 💡 Ideated
**Created:** 2026-10-11

## Vision

O-simpleAmbisonics is a teaching tool first and a spatializer second. The real ambisonic toolchains (IEM Plug-in Suite, SPARTA, Envelop) are excellent but assume you already understand the method: they split encoding, rotation, and decoding across separate plugins on 16- or 64-channel buses that Logic and Ableton cannot even create. A student opening them sees channel matrices and SOFA loaders before they have heard a single source move. O-simpleAmbisonics collapses the whole chain into one stereo-out plugin that runs on any laptop with headphones: **source → encoder → rotation → decoder → L/R**, every stage visible, every stage audible.

The pedagogical payload is the tight loop between **gesture and visible consequence** (the O-simple* north star). Drag the source around the sphere and watch the B-format channel meters respond — W stays constant (omni), X/Y/Z swing with the figure-8 patterns, and the higher-order channels light up in their lobed shapes. Step the order from 1 to 2 to 3 and watch the spatial-resolution polar plot sharpen from a soft blob into a tight beam while the image audibly focuses. Turn the Yaw knob and hear the whole scene spin — without touching the source — which is THE property that makes ambisonics different from channel-based panning: the soundfield is a thing you can rotate as a unit. Move the source overhead or behind and confront front/back and elevation ambiguity with your own ears. Pull it away and hear the distance cues.

Every control is annotated: hovering any parameter surfaces a short, plain-language explanation of what it does and why ("Order 1 = 4 channels, W X Y Z; each higher order adds 2n+1 channels and a sharper image"). A live signal-flow diagram keeps the chain visible at all times so students build an accurate mental model of encode → transform → decode rather than memorizing knob positions. A concept-preset tour offers a handful of named scenes that each isolate one idea.

The design north star: a curious student should reach a genuine "oh, THAT's what the B-format channels are" moment within five minutes, with no manual and no multichannel bus.

## Architecture

**Single-source HOA encode → SH-domain rotation → binaural decode, stereo in / stereo out** (the minimal chain that still teaches the full method):

```
mono source ──► [ENCODER] ──► [ROTATE] ──► [DECODER] ──► L/R
(stereo in     az/el/dist     yaw/pitch    binaural
 summed)       order 1–3      /roll        (HRTF)
                  │              │             │
               N = (order+1)² ambisonic channels (ACN/SN3D, AmbiX)
```

- **Encoder:** multiplies the mono source by the spherical-harmonic gains for the source direction (ACN channel order, SN3D normalisation — the AmbiX convention). Order 1/2/3 → 4/9/16 channels. Distance applies gain and a gentle high-frequency roll-off before encoding.
- **Rotation:** a rotation matrix applied in the ambisonic domain. Demonstrates that the scene (not the source) can be turned — the same matrix would rotate a thousand sources at once.
- **Decoder:** ambisonic channels → binaural L/R via HRTF (MagLS or virtual-loudspeaker + HRTF; Stage 0 picks). A plain stereo decode is available for laptop speakers / projector demos.

Everything stays in the plugin; the host only ever sees a stereo effect. Students in Logic and Ableton get the full ambisonic chain on headphones.

## Parameters

*Core set defined here; Stage 0 research should surface and confirm additional standard controls (see Technical Notes). Ranges below are starting proposals to be validated in planning.*

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Azimuth | -180 to +180° | 0° | Horizontal direction of the source (0° front, +90° left per AmbiX, ±180° behind). THE primary control. |
| Elevation | -90 to +90° | 0° | Vertical direction of the source (+90° directly overhead, -90° below). |
| Distance | 0.2–20 m | 1 m | Source distance: level follows an inverse-distance law and distant sources lose a little air; near (<1 m) sources get louder and wider. |
| Order | 1 / 2 / 3 | 1 | Ambisonic order → 4 / 9 / 16 channels. Higher = sharper image. Starts at 1st so the order step-up is the lesson. |
| Yaw | -180 to +180° | 0° | Scene rotation about the vertical axis — spins the whole soundfield. The headline "rotate the room" control. |
| Pitch | -180 to +180° | 0° | Scene rotation about the left–right axis (tilt). |
| Roll | -180 to +180° | 0° | Scene rotation about the front–back axis. |
| Decoder | Binaural / Stereo | Binaural | Binaural (HRTF, headphones) or a plain stereo decode for speakers / projector demos. |
| Orbit | Off / On | Off | Automatic azimuth motion so continuous movement is audible without DAW automation. |
| Orbit Rate | 0.02–2 Hz | 0.1 Hz | Speed of the automatic orbit (one full circle per period). |
| Input Mode | Mono sum / Left only | Mono sum | Stereo input is summed to the single mono source (or L taken alone). |
| Output Level | -inf–0 dB | 0 dB | Master output gain. |

**Likely additions / confirmations from research (Stage 0):** source width / spread (point → diffuse, a classic ambisonic lesson, but non-trivial DSP), distance-model details (near-field boost, air-absorption cutoff, whether to expose a "Distance Model" on/off for before/after), decoder weighting (basic / max-rE / in-phase — another classic lesson but possibly too deep for v1.0), HRTF dataset choice and whether to offer more than one head, head-locked vs scene-locked listener for the Stereo decode, solo-a-channel audition of individual B-format channels, latency compensation for the convolution.

## UI Concept

*Captured from the sibling template; full UI design happens in the mockup phase, not here.*

**Layout:** A single clear page (no deep menus), classroom/projector-readable, laid out left-to-right as the signal path itself — Source (position, distance, orbit) | Encoder (order, B-format channel view) | Rotation (yaw/pitch/roll) | Decoder (mode, virtual-speaker/HRTF view) | Output. The live visuals occupy a prominent panel, consistent with O-simpleFM / O-simpleSubtractive.

**Visual Style:** Clean, instructional, uncluttered — consistent with the O-simple* family. Readable at a glance.

**Key Elements (pedagogical layer — first-class functional features, not decoration):**
- **Source-on-a-sphere view** — a draggable source puck in a top-down (azimuth) view with a side/elevation view, distance drawn as radius, the listener's head at the centre with a nose marking "front". Dragging sets azimuth/elevation directly. Rotation is visible as the scene's compass rose turning while the source stays where it was.
- **B-format channel view** — THE headline visual: one live meter per ambisonic channel, labelled by ACN index and name (W, Y, Z, X, V, T, R, S, U, …), each with its spherical-harmonic polar-pattern icon, so a student literally sees W stay steady while X/Y/Z swing and the higher-order lobes light up and fade as the source moves. Changing order adds or removes rows.
- **Spatial-resolution polar plot** — the virtual panning function for the current direction and order drawn as a polar beam; it visibly sharpens from order 1 to 3 (the order lesson in one picture).
- **Live signal-path diagram** — source → encoder (N channels) → rotation → decoder → L/R block diagram with the active stage highlighted and the channel count updating with order.
- **Decoder view** — the virtual-loudspeaker layout (or HRTF directions) the decoder uses, so "decoding" is not a black box.
- **On-hover pedagogical tooltips** — short plain-language explanation per control (what W/X/Y/Z are, why order adds channels, why Yaw rotates the scene and not the source, what SN3D/ACN mean, why front/back is hard on headphones).
- **Concept-preset tour** — named scenes each isolating one idea: *Point Source (FOA)*, *Order 1 → 2 → 3*, *Behind You* (front/back confusion), *Overhead*, *Rotate the Room*, *Far Away / Up Close*, *Slow Orbit*, *Speakers vs Headphones* — so students reverse-engineer each move from the minimal control set.

## Use Cases

- **Classroom demonstration** — instructor projects the plugin, drags a source around the sphere while the B-format meters move, steps the order up and shows the beam sharpening, then spins Yaw to rotate the whole scene — each concept visible and audible in real time on headphones or the room's stereo pair.
- **Self-directed student learning** — a student works the preset tour, reads tooltips, and spatializes a sound of their own in Logic or Ableton on headphones, articulating what encoder, rotation, and decoder each do and why ambisonics is not just panning.
- **Bridge to professional tools** — once the chain is understood here, the student recognises the same encoder / rotator / decoder blocks in IEM, SPARTA, and Reaper multichannel sessions.
- **Lightweight creative spatializer** — usable as a simple binaural panner with orbit motion for headphone mixes.

## Inspirations

- **O-simpleFM / O-simpleSubtractive / O-simpleAdditive** — the direct siblings and pedagogical template (irreducible control set, gesture→visible-consequence, live visuals, tooltips, concept-isolating presets).
- **IEM Plug-in Suite (StereoEncoder, SceneRotator, BinauralDecoder)** — the reference open-source chain whose three blocks this plugin fuses into one; its encoder sphere view and channel conventions (AmbiX) are the model.
- **SPARTA (Ambi ENC / Ambi BIN)** — the other reference toolchain; its MagLS binaural decoder is the quality bar.
- **Envelop for Live** — proof that an ambisonic workflow can live inside a stereo-only DAW for teaching.
- **O-GrainScatter** — in-house 3rd-order SH encoder and binaural decoder already written; **O-Orbit / O-Octagon** — in-house spatializers with the Logic surround-format constraints documented.
- **Gerzon's original Ambisonics papers / Zotter & Frank, *Ambisonics* (2019)** — the theory the tooltips distil.

## Technical Notes

- **Conventions:** AmbiX (ACN channel order, SN3D normalisation), the modern standard the reference tools use; tooltips name it so students can map to Reaper/IEM later. Encoder coefficients for orders 0–3 are fully derived in `research/ambisonics-encoding-deep-dive.md` and implemented in `plugins/O-GrainScatter/Source/dsp/SpatialEncoder.h` — reuse, don't rederive. Coefficients must be gate-checked against a reference table.
- **Rotation:** an SH-domain rotation matrix per order (recursive construction, e.g. Ivanic–Ruedenberg, or the simpler Z-Y-Z Euler route). Applied after encoding so Yaw demonstrably rotates the scene, not the source. Smooth angle changes (shortest path around ±180°, never the long way) to avoid zipper noise.
- **Binaural decoder:** HRTF-based. `research/ambisonics-binaural-decoding-deep-dive.md` covers both the virtual-loudspeaker baseline and MagLS (preferred above ~1–2 kHz at low orders). Pre-compute one pair of decoding filters per ambisonic channel from a free HRTF set (KU100 / SADIE II) **offline** and embed them as binary data; do not parse SOFA at runtime. Render with uniformly partitioned convolution (N channels × 2 ears); report the convolution latency via `setLatencySamples()` in `prepareToPlay()`. Decoder filter sets per order (1/2/3) so an order change is a filter-bank swap with a crossfade, not a click.
- **Stereo decode:** a plain amplitude decode to a ±30° virtual pair (or equivalent) for speaker/projector use; no HRTF.
- **Distance:** inverse-distance gain with a near-field clamp, plus a gentle one-pole air-absorption low-pass that opens with proximity. Keep it simple and explainable; the lesson is "distance = level + air", not a full NFC filter set.
- **Order switching:** pre-allocate for 16 channels always; lower orders zero the unused channels. Switching must be click-free (crossfade the decoder bank). Parameter smoothing on azimuth/elevation/distance/yaw/pitch/roll.
- **Real-time safety:** no allocations/locks in `processBlock`; channel meters, polar plot, and position cross to the UI via lock-free handoff. The convolution engine must be sized for the maximum host block size (see the oversampler-overrun pattern) and must not fade in from silence after `prepareToPlay()` (seed any gain ramps).
- **Bus layout:** stereo in → stereo out only (also accept mono in). No multichannel bus is declared, so the plugin behaves identically in Logic, Ableton, and Reaper. B-format passthrough is deliberately out of scope for v1.0.
- **Platform:** WebView UI (JUCE 8) for rich live visualisations + tooltips, consistent with the Ouaricon suite and siblings. Must set Windows WebView2 flags (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`) per project standards. If a second binary-data target is added (embedded HRTF filters + presets + WebView resources), give it a distinct `NAMESPACE` to avoid the BinaryData collision seen in O-simpleGrain.

## Out of Scope (v1.0)

- B-format / multichannel output and loudspeaker decoding to physical rigs — the audience is on headphones in Logic/Ableton; O-Octagon and O-Orbit already cover speaker rigs. Possible v1.x "Decoder: B-format passthrough" for Reaper.
- Multiple sources / a scene bus — one source keeps the lesson clean; multiple instances can be stacked in the DAW.
- Head tracking, custom SOFA loading, HRTF personalisation.
- Room / reverb / early reflections — keep the signal path transparent for teaching.
- Source width / spread and decoder weighting (max-rE / in-phase) — candidate v1.1 lessons once research sizes the DSP.
- Orders above 3 — the order lesson is complete by 3rd; higher orders cost CPU and add nothing visible.

## Next Steps

- [ ] Create UI mockup (`/start O-simpleAmbisonics` → option 3)
- [ ] Start planning / DSP research (`/plan O-simpleAmbisonics`)
