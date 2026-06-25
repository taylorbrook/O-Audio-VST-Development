---
plugin: O-simpleBeatmaker
stage: 0
status: complete
last_updated: 2026-06-25
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: invoke_foundation_shell_agent
next_stage: 1
ready_for_implementation: true
contract_checksums:
  brief: sha256:d4c7b23b26982ad7f06c6fff0d7feb960f0877a987097ec7cfcf29941931baf7
  parameter_spec: sha256:4a25f36678e6a5954633010bdb3bb0a2fbd7f039b91173ffa74f53ec7ca9bccf
  architecture: sha256:3c8279e6dd99618539438ae6c6e7f7b85c57087b28d96fb95da958163c07de36
  roadmap: sha256:ac3e6a9844c938d4fbc4a7268019ce3958fc1166e059464a482cbeded1e6864f
---

# O-simpleBeatmaker Status

## Current Position

Stage: 0 of 4 (Ideation) — complete
Status: Research & Planning complete. ARCHITECTURE.md + ROADMAP.md produced. Ready for Stage 1 (Foundation).
Progress: [##..................] 10%

## Completed So Far

**Ideation:** ✓ Complete
- Core concept: pedagogical TR-808/909 step-sequencer drum machine for MUSC319 wk09 (MIDI sequencing & timing feel)
- Four architectural forks resolved at ideation; 24 requirements extracted (must 15 / should 7 / nice 2)

**Stage 0:** ✓ Complete
- Plugin type: Synth (step-sequencer drum machine), MIDI-in → stereo-out, host-synced + MIDI-playable, WebView UI
- Tier 4 (synth+MIDI) escalated toward tier 6 by first-class real-time viz + brand-new host-transport sync + sample-accurate scheduler → research depth DEEP
- **Defining decision:** the internal sequencer emits GM-mapped MIDI note-ons at sample-accurate offsets into the SAME MidiBuffer as host MIDI → one merged stream feeds voices AND the viz tap ("step grid & piano roll = two views of one MIDI stream", literally). Sub-step Δt = message samplePosition; timing-lane shows the applied Δt → QUAL-02 by construction.
- **Timing-feel math RESOLVED:** 16th swing `s = 0.5 + (swing/75)/3` (MPC-canonical, off-beat 16ths, NOT removed by quantize); humanize ±30 ms timing / ±24 velocity from pre-seeded RNG; composition `Δt = Δswing + Δhuman·(1−q)` (quantize=100 → dead tight but swing remains).
- **Voices RESOLVED:** Kick(808)/Snare(808-909 hybrid)/Clap(808 multi-burst)/Closed+Open Hat(band-passed noise + choke)/Tom(808). GM map 36/38/39/42/46/45. Synthesized, no samples.
- **State RESOLVED:** 6×32 grid + velocities = custom `std::atomic<uint8_t>` array persisted in a `ValueTree "PATTERN"` child — NOT 384 APVTS params. ~42 knob params are APVTS.
- JUCE 8 APIs verified against local source (8.0.9): `AudioPlayHead::PositionInfo`, `MidiBuffer::addEvent` (sample-accurate), `AbstractFifo`, `Random`, `dsp::StateVariableTPTFilter`/`IIR`, `LookupTableTransform`, APVTS+ValueTree. (No sibling queries the playhead — SequencerClock is genuinely new.)
- Professional research: TR-808/909 voice synthesis, MPC/Roger-Linn swing curve, Ableton/Logic quantize-strength + groove.
- Complexity score: **5.0** (capped; raw 10.0 = params 2.0 + 6 algorithms + 2 features)
- Strategy: **staged** (Stage 2 DSP × 3 phases, Stage 3 GUI × 3 phases)
- ARCHITECTURE.md + ROADMAP.md + Stage-0 CONTEXT.md documented

## Next Steps

1. **Stage 1: Foundation** — CMake (IS_SYNTH + MIDI + WebView2 flags), ~42-param APVTS + custom PATTERN ValueTree state + persistence (silent shell). Run `/clear` then `/implement O-simpleBeatmaker`.
2. Highest risk lives in Stage 2: host-transport sync + sample-accurate sub-step Δt — build straight-time grid FIRST (Phase 2.2), add swing/humanize/quantize SECOND (Phase 2.3), gated by the offline render-harness.
3. Pause here (handoff at Stage 0→1 boundary).

## Context to Preserve

**Key decisions:**
- Sequencer-emits-MIDI-into-shared-buffer spine (one MIDI stream; voices sub-slice on sample offsets; viz reads applied Δt → QUAL-02)
- Timing-feel: 16th swing `s=0.5+(swing/75)/3` (deterministic, quantize-immune); humanize pre-seeded RNG ±30 ms / ±24 vel; `Δt = Δswing + Δhuman·(1−q)`
- 6 synthesized voices (808/909 flavor per voice); GM map 36/38/39/42/46/45; closed-hat chokes open-hat
- Grid = custom `std::atomic<uint8_t>[6×32]` + `ValueTree "PATTERN"`; NOT APVTS params
- Standalone free-run clock at `tempo` when host not playing
- Highest risk: sample-accurate sub-step Δt — straight-time grid first, feel math second; render-harness is the gate; Fallback A = late-only humanize

**Strategy:** complexity 5.0, staged implementation (3 DSP phases + 3 GUI phases + polish).

**Files created:**
- plugins/O-simpleBeatmaker/.planning/research/ARCHITECTURE.md
- plugins/O-simpleBeatmaker/.planning/ROADMAP.md
- plugins/O-simpleBeatmaker/.planning/stages/0-ideation/CONTEXT.md
- plugins/O-simpleBeatmaker/.planning/STATUS.md (this file)

**Sibling references:** O-simpleFM (primary template — voice/viz/CMake/harness), O-simpleSubtractive & O-simpleAdditive (WebView + QUAL-02 discipline), O-simpleGrain (BinaryData namespace + harness gate).
