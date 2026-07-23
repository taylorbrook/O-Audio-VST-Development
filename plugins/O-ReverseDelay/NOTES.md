# O-ReverseDelay Notes

## Status
- **Current Status:** 🚧 Stage 0
- **Version:** N/A
- **Type:** Audio Effect (Granular Reverse Delay)

## Overview

Ambient granular reverse delay: the wet signal is assembled from overlapping Hann-windowed reversed grains over a circular capture buffer — a continuous reverse smear rather than chunked backwards blocks. Tempo-synced (dotted/triplet divisions) or free-time, with damped feedback (in-loop low-cut/high-cut) that stays loop-stable at 100% via tanh soft-clip.

**Complexity:** 5.0 (capped) — phased implementation (DSP 3 phases, GUI 2 phases)

## Lifecycle Timeline

- **2026-07-23:** Ideated — creative brief, requirements (14), draft parameters (10)
- **2026-07-23 (Stage 0):** Research & Planning complete — ARCHITECTURE.md and ROADMAP.md documented (Complexity 5.0, staged implementation)

## Key Architecture Points

- Reverse read law: grain read offset from write head grows +2 samples/output sample (`D + 2n`); collision-free by construction; 3.5 s capture buffer
- Feedback closes through the shared capture buffer → alternating-direction regenerations (intended, classic hardware character)
- Damping: 2nd-order Butterworth HP/LP via ArrayCoefficients in-place updates (RT-safe)
- Click-free via per-grain parameter latching (D/G/pan/gain snapshotted at spawn)
- Zero reported latency; estimated < 5% single core

## Planning Files

- `plugins/O-ReverseDelay/.planning/research/ARCHITECTURE.md`
- `plugins/O-ReverseDelay/.planning/ROADMAP.md`
- `plugins/O-ReverseDelay/.planning/REQUIREMENTS.md`
- UI mockup: not yet created (required before Stage 3)

**Last Updated:** 2026-07-23
