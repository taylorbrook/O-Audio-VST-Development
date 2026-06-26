# Stage 2 (DSP) — CONTEXT

> **Source:** Auto-Generated from existing contracts (express mode, no interactive session).
> Compiled from `research/ARCHITECTURE.md`, `parameter-spec.md`, `ROADMAP.md` (Stage-2 phases + render-harness gate),
> and `stages/1-foundation/VERIFICATION.md`.
> **Date:** 2026-06-25

## Goal

Implement the full DSP for the pedagogical TR-808/909 step-sequencer drum machine: 6 synthesized
voices, a host-synced sample-accurate sequencer, and the swing/humanize/quantize timing-feel engine,
plus the lock-free viz tap that carries the **applied** Δt. First audio happens here. UI is Stage 3.

## What already exists (Stage 1 Foundation — complete)

- `CMakeLists.txt` — target `OSiB`, `IS_SYNTH TRUE` + MIDI-in + WebView2 flags, silent shell.
- `Source/PluginProcessor.{h,cpp}` — 42-param APVTS (IDs/ranges locked in `parameter-spec.md`)
  + custom `std::array<std::atomic<uint8_t>, 6*32> grid` + `ValueTree "PATTERN"` persistence.
  - Grid API present: `setStep / setStepVelocity / toggleStep / getStep / clearGrid` (lock-free).
  - `ParamIDs` namespace + `kGmNotes {36,38,39,42,46,45}` + `kVoicePrefix` + `voiceParamID()` helper.
  - `processBlock` is currently silent (clears buffer). `acceptsMidi()==true`, `getTailLengthSeconds()==3.0`.
- `Source/PluginEditor.{h,cpp}` — `GenericAudioProcessorEditor` shell (replaced in Stage 3).
- Verified: builds VST3 + AU + Standalone; pluginval VST3 @ strictness 8 SUCCESS.

## The architectural spine (immutable — ARCHITECTURE.md)

**The internal sequencer emits GM-mapped `MidiMessage` note-ons at sample-accurate offsets into the
SAME `MidiBuffer` as host MIDI.** One merged, sorted stream feeds the voices AND the viz tap. Therefore:
- Sub-step Δt = the `samplePosition` of the emitted message (no block snapping).
- Viz Δt = `appliedSampleInBar − nominalSampleInBar` read from the same message → QUAL-02 by construction.
- MIDI-playable voices need no separate path: host note 36 ≡ sequencer note 36 to the voice layer.

## Staged execution (de-risk timing FIRST) — 3 sub-phases

This stage's execute phase is internally staged per ROADMAP. Each sub-phase is a checkpoint commit; the
**offline render-harness is the gate** for the hardest item (sample-accurate sub-step Δt).

### Phase 2.1 — DrumVoiceEngine (MIDI-playable, NO sequencer yet)
Conventional, low-risk half. 6 voices triggered by **incoming MIDI only** (GM map). Per-voice structs
(Kick/Snare/Clap/Hat[closed+open shared+choke]/Tom): exp pitch/amp envelopes, `LookupTableTransform`
sine (reuse O-simpleFM `fastSine`), `juce::Random`/xorshift noise, `dsp::StateVariableTPTFilter`/`IIR`
shaping. 808/909 flavor per ARCHITECTURE §3 table. `UnifiedTriggerRouter` (MIDI half): GM note→voice,
velocity→loudness+timbre, mute/solo at mixer. `Mixer`/master with `SmoothedValue` output gain. Processor
reads APVTS once/block → `setParams`; `ScopedNoDenormals`; `setLatencySamples(0)` (NEVER override the
non-virtual `getLatencySamples()`).

### Phase 2.2 — SequencerClock + UnifiedTriggerRouter (host-synced grid, swing/humanize OFF)
Prove the **sample-accurate** straight-time grid BEFORE feel math. `SequencerClock`: read
`getPlayHead()->getPosition()` once/block (every field `Optional`); enumerate firing steps in
`[ppqStart, ppqStart+block)` (+1-block lookahead bookkeeping); commit each ON step exactly once
(discontinuity-safe). Standalone/stopped → integrate `phaseInSteps` at `tempo` BPM. `patternLength`
(8/16/32) wraps; bar align via `getPpqPositionOfLastBarStart()`. Emit `MidiMessage::noteOn` at
`nominalOffsetInBlock` into the sequencer buffer; **merge** (sorted) with host buffer; voices sub-slice
render on event offsets. `playheadStepPhase` atomic updated each block.

### Phase 2.3 — TimingFeelEngine + VizAnalyzer (swing/humanize/quantize Δt + truthful viz tap — THE lesson)
`TimingFeelEngine` (exact math is the contract):
- Swing `s = 0.5 + swing01/3` on off-beat 16ths (odd indices); `Δswing = (swing01/3)*(60/bpm/2)` sec.
  **Deterministic, NOT scaled by quantize.**
- Humanize per hit (sampled ONCE at commit) from **pre-seeded** `juce::Random`: timing
  `Δhuman_t = rand[−1,1]*humanize01*30ms`, velocity `Δhuman_v = rand[−1,1]*humanize01*24` (triangular
  distribution recommended).
- Composition: `Δt = Δswing + Δhuman_t*(1−q)`, `vel = clamp(stepVel + Δhuman_v*(1−q), 1, 127)`, `q=quantize01`.
  **q=1 → humanize collapses to ~0 BUT swing remains** (DSP-04 invariant).
- `appliedOffsetSamples = round((Δswing + Δhuman_t*(1−q)) * fs)`; bake into the emitted note-on `samplePosition`.

`VizAnalyzer`: `AbstractFifo` ring of POD `VizEvent { uint8 voice; int16 step; int32 nominalSampleInBar;
int32 appliedSampleInBar; uint8 velocity; uint8 source; }` + `std::atomic<float> playheadStepPhase`.
Lookahead handles symmetric (±) humanize; **Fallback A** = late-only humanize if lookahead proves fiddly.

## Stage-2 Correctness Gate — offline DSP render-harness (REQUIRED)

Port O-simpleFM `tests/render-harness/` (console app, `-DOUARICON_BUILD_TESTS=ON`, no DAW). Beatmaker
additions vs the FM template:
- A small `juce::AudioPlayHead` subclass returning a known bpm/ppq/isPlaying (synthetic transport).
- A test hook exposing the sequencer's emitted `MidiBuffer` so offsets are asserted **directly**
  (not only via audio transients).

Probes (all must pass to clear the gate):
1. **Grid accuracy** — straight time: each ON step fires at exactly `stepIndex * samplesPer16th` within the bar (±0 samples).
2. **Swing** — swing=75%: off-beat 16ths delayed by exactly `(swing01/3)*T8` samples; on-beat 16ths unmoved.
3. **Humanize + quantize** — humanize=100/q=0: spread >0, bounded ±30 ms timing / ±24 vel. q=100 → spread→~0 **but swing remains**.
4. **Block-boundary independence** — a step + Δt straddling a block edge still fires at the correct absolute sample (no snapping). Hardest case.
5. **MIDI-playable + voices** — host note-on per GM note → correct voice; closed-hat chokes open-hat; velocity scales output; aliasing budget on high-rate hits (DSP-06/QUAL-01).
6. **Viz truth** — emitted `appliedSampleInBar − nominalSampleInBar` equals the offset applied to audio (QUAL-02).

## Constraints & locked decisions

- **Params:** stored ranges/IDs are LOCKED (parameter-spec.md). swing/humanize/quantize/tone/decay stored
  0–1; tune ±12 st; levels −60..0 dB; `quantizeStrength` default **1.0**; `patternLength` default 16.
- **Per-voice decay ms mapping** lives in `DrumVoiceEngine` (kick boom vs hat tick differ by order of magnitude), NOT in the param range.
- **Grid is NOT APVTS** — read lock-free via the existing atomic array; never add 384 params.
- **Thread safety:** all knobs via cached `getRawParameterValue()` once/block; RNG pre-seeded in `prepareToPlay`,
  never reseeded on audio thread; audio→UI strictly `AbstractFifo` + atomic playhead. `processBlock`
  allocation-free + lock-free (PERF-01).
- **Latency:** zero. `setLatencySamples(0)` in `prepareToPlay`.
- **Quantize must NOT touch swing** — compute `Δswing` and `Δhuman` as separate terms; q scales only `Δhuman`. Single most important invariant.
- **Denormals:** `ScopedNoDenormals` + flush decaying tails (resonant hat filters most at risk).

## Requirements covered (ROADMAP traceability)

FUNC-01/02/03/04/06/07, DSP-01/02/03/04/05, DSP-06, PERF-01/02, QUAL-01.

## Sibling references

- **O-simpleFM** — voice/envelope skeleton, `fastSine`, lock-free viz analyzer, render-harness (PRIMARY template).
- **O-simpleSubtractive / O-simpleAdditive** — QUAL-02 truth-by-construction discipline.
- **O-simpleGrain** — render-harness gate precedent; BinaryData `NAMESPACE` collision lesson (matters Stage 3, not here).
- **No sibling queries the host playhead** — `SequencerClock` is genuinely new; verify `AudioPlayHead::PositionInfo` against local JUCE 8.0.9 source.

## Open decisions for this stage

None blocking. Defaults from contracts apply. Express mode: proceed through 2.1→2.2→2.3 with the
render-harness as the automated gate between sub-phases; stop at the Stage-2→3 boundary for handoff.
