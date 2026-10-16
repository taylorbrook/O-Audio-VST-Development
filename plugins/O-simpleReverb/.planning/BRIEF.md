# O-simpleReverb - Creative Brief

## Overview

**Type:** Audio Effect (Pedagogical Algorithmic Reverb)
**Core Concept:** A lightweight reverb for instrument chains that also teaches how reverberation works: six spaces on three real engines, and four live wells that show what the processor is doing — the tail decaying against its RT60, the early-reflection taps, the tone of the tail and the output.
**Status:** 📦 Installed (v2.0.0) · v3.0.0 family retrofit in progress (milestone `family-retrofit`)
**Created:** 2026-01-13 (as Ouaricon Simple Reverb) · this brief: 2026-10-15

History: Ouaricon Simple Reverb (v1.0.0) → O-SimpleReverb (v1.5.0) → **O-simpleReverb** (v3.0.0, case-only rename into the O-simple family). The v1 planning documents (`creative-brief.md`, `architecture.md`, `plan.md`) describe the original Freeverb-based design and are kept as history.

## Vision

Still a "set it and forget it" reverb for adding room to dry instruments — but one that answers *why* it sounds the way it does. A student who moves SIZE sees the walls move and the tail length stay; moves DECAY and watches the slope tilt; flips Low Cut and sees the low end leave the tail, not the dry signal. The O-simple family layout (field-guide paper, walnut wells, lessons along the bottom) makes it a sibling of O-simpleAmbisonics, O-simpleFM and the rest.

## Architecture

```
input ─┬────────────────────────────────────────────── DRY gain ─┐
       └─ pre-delay ─ early reflections ─┬─ engine ─ tone ─ WET ──┴─ out
                     (FDN types only)    │  FDN (Booth, Room, Hall, Ambient)
                                         │  Dattorro plate (Plate)
                                         │  3 dispersive springs (Spring)
                                         └─ tone = type EQ + Character (dark SVF / bright shelf) + Low Cut
```

- `Source/dsp/`: `FdnEngine.h` (16-line Hadamard FDN), `PlateEngine.h`, `SpringEngine.h`, `EarlyReflections.h`, `ReverbPrimitives.h` — frozen since v2.0.0.
- A TYPE change lets the old tail ring out in a second slot.
- `MeterFrame` (inDb, wetDb, outPeakL/R) published once per block through a `TripleBuffer`; the editor emits it at 30 Hz.

## Parameters

8 parameters, unchanged since v1.14.0 (IDs, ranges, skews, defaults, state format):

| ID | Name | Range | Default |
|----|------|-------|---------|
| TYPE | Type | Booth / Room / Hall / Spring / Plate / Ambient | Room |
| CHARACTER | Character | −100 … +100 (dark ↔ bright) | 0 |
| WET | Wet | 0 … 100 % | 25 |
| DRY | Dry | 0 … 100 % | 100 |
| DECAY | Decay | 0.50 … 2.00 × the type's base RT60 (skew 0.6309) | 1.00 |
| SIZE | Size | 0 … 100 % | 50 |
| LPFREQ | Low Cut Freq | 20 … 400 Hz | 200 |
| LPON | Low Cut On | off / on | off |

Base RT60: Booth 0.40 s, Room 1.1 s, Hall 3.0 s, Spring 2.5 s, Plate 2.5 s, Ambient 7.0 s.

## UI Concept (v3.0.0)

1180 × 780, resizable 0.6–1.5× at a fixed aspect (editorScale saved with the session). Top to bottom:

- **Header:** wordmark, the Tail chip (type · RT60 · pre-delay), preset bar (◀ name ▶ Save Delete), gear (language, hover help, Load file…, version).
- **Wells:** Live Decay (inDb amber, wetDb sage, brass RT60 slope from `getReverbModel`) · Early Reflections (`getEarlyReflectionTaps`) · Tone of the Tail (`getToneCurve`).
- **Signal path:** Input → Early → Engine → Tone → Mix → Output stations, lit by the control that touched them.
- **Controls:** Engine (TYPE row, SIZE, DECAY, type note) · Tone (Character, Low Cut + IN/OUT pill) · Mix (Wet, Dry) · Output meter.
- **Lessons:** Spaces (6 types) and Ideas (Init, Size ≠ Decay, Character: dark ↔ bright, Low Cut, Insert vs Send).
- en / fr / zh-Hans throughout; type names stay English (M2).

## Use Cases

- Room tone on dry synths, samplers and virtual instruments (insert, Dry 100).
- A send reverb (Wet 100, Dry 0; each type has a Send preset).
- Teaching: what SIZE vs DECAY do, why a plate has no early reflections, what a spring's dispersion looks like, insert vs send gain staging.

## Out of Scope

- New DSP or parameters (v3.0.0 is a UI / identity release; the DSP nulls bit-exact against v2.0.0).
- Lesson files on disk (lessons are applied from `PresetRecipes.h`).
- Linux distribution.
