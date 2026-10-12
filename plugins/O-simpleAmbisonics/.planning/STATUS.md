---
plugin: O-simpleAmbisonics
stage: ideation
status: creative_brief_complete
last_updated: 2026-10-11 00:00:00
---

# Resume Point

## Current State: Creative Brief Complete

Creative brief has been finalized for O-simpleAmbisonics. Ready to proceed to Stage 0 planning (DSP research + architecture) or a UI mockup.

## Completed So Far

**Ideation:** ✓ Complete
- Core concept defined (pedagogical ambisonic spatializer: mono source → SH encoder → SH rotation → binaural decoder → L/R, all in one stereo-out plugin)
- Architecture decided via 4 ideation questions
- Parameters specified
- UI vision captured (mirrors O-simpleFM / O-simpleSubtractive)
- Use cases identified
- Requirements extracted with acceptance criteria

## Key Architecture Decisions (from ideation)

- **Output:** encode + decode in ONE plugin, stereo in / stereo out. No multichannel bus — students are on headphones in Logic Pro and Ableton Live, neither of which offers an ambisonic bus.
- **Order:** 1 / 2 / 3 selectable (4 / 9 / 16 channels), default 1 so the order step-up is the lesson. Nothing above 3rd.
- **Source:** one mono source positioned by azimuth / elevation / distance; stereo input summed.
- **Rotation:** yaw (must), pitch/roll (should) applied in the SH domain — the "rotate the scene, not the source" lesson.
- **Decoder:** Binaural (HRTF, embedded precomputed filters, partitioned convolution) default; plain Stereo decode for speakers/projector.
- **Conventions:** AmbiX (ACN / SN3D). Reuse the 3rd-order encoder in `plugins/O-GrainScatter/Source/dsp/SpatialEncoder.h` and the two research docs `research/ambisonics-encoding-deep-dive.md`, `research/ambisonics-binaural-decoding-deep-dive.md`.
- **Headline visual:** live B-format channel meters with SH pattern icons; spatial-resolution polar plot sharpening with order.

## Next Steps

1. Start Stage 0 planning / DSP research (recommended): `/plan O-simpleAmbisonics`
2. Create UI mockup to visualize design: `/start O-simpleAmbisonics` → option 3

## Context to Preserve

**Key Decisions:**
- Plugin type: Audio Effect (Pedagogical Ambisonic Spatializer)
- Core concept: collapse encoder → rotator → decoder into one stereo plugin; gesture → visible consequence; teaching tool first

**Open research questions (Stage 0):**
- Binaural decoder method: MagLS vs virtual-loudspeaker + HRTF; which free HRTF set (KU100 / SADIE II); offline filter precomputation pipeline and embedding as binary data
- SH rotation matrix construction for orders 2–3 (recursive vs Euler Z-Y-Z) and its cost
- Distance model details (near-field clamp, air-absorption cutoff); whether to expose source width / spread in v1.0
- Click-free order/decoder switching strategy (crossfaded filter banks) and latency equality between Binaural and Stereo modes
- Convolution engine sizing for max host block (oversampler-overrun pattern) and CPU budget at order 3 / 64-sample buffers

**Sibling references:** O-simpleFM (1.6.0), O-simpleSubtractive (1.7.0), O-simpleWavetable (1.0.1) — same pedagogical template. O-GrainScatter (in-house SH encoder + binaural decoder), O-Octagon / O-Orbit (Logic surround constraints).

**Files Created:**
- plugins/O-simpleAmbisonics/.planning/BRIEF.md
- plugins/O-simpleAmbisonics/.planning/REQUIREMENTS.md
