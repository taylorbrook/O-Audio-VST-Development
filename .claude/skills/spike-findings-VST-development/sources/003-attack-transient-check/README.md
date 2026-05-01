---
spike: 003
name: attack-transient-check
validates: "Given a tuned note with NE at sampleOffset 0, when voice starts, then first output sample is at tuned frequency (no pitch glide/zipper)"
verdict: VALIDATED
related: [001-patch-build-load, 002-quarter-sharp-end-to-end]
tags: [dsp, transients, refinement]
---

# Spike 003: attack-transient-check

## What This Validates

**Given** a Dorico-driven tuned note with a Note Expression event at `sampleOffset == noteOn.sampleOffset` (confirmed in Spike 002 trace),
**when** the voice allocator starts the voice,
**then** the very first output sample is rendered at the tuned frequency — no audible pitch glide, click, or zipper at onset.

## Why This Matters

The research (`Part 3 → Gotchas`) warned:
> Dorico typically emits the NE value event at the same `sampleOffset` as `noteOn`. The voice allocator must consume the pending tuning BEFORE synthesizing the first sample, or you get a pitch glide/zipper at note attack.

## How It's Handled

By design: in `OLyricaAudioProcessor::processBlock`, the side-channel queue is drained *before* `synthesiser.renderNextBlock`. This populates `pendingTuningSemis[midi]` for every NE event in the current block. When `renderNextBlock` iterates MIDI events and dispatches `noteOn` to a voice, `HarpSynthVoice::startNote` reads `pendingTuningSemis[midi]` (via `exchange(0.0)`), applies it to `currentFrequency`, and *then* calls `stringModel.trigger(currentFrequency, …)` on line 243. The stringModel's delay-line length is sized from this final frequency — the first sample is at the tuned pitch.

Trace evidence from Spike 002:

```
16:19:52.255 [PROC] applied pitch=61 semitones=-0.5000
16:19:52.255 [VOICE] startNote midi=61 semis=-0.5000 freqBefore=277.183 freqAfter=269.292
```

Both entries share the same timestamp — populated and consumed within the same block, ordered `[PROC]` → `[VOICE]`.

## How to Verify

Aural: play a quarter-sharp note in Dorico and compare the attack transient to a plain note's attack. If the tuned note's attack has a pitch glide (frequency sweeping into the target), zipper (sample-level discontinuity), or distinctive onset click not present in the plain note, the spike fails.

## Results

User listened to the attack directly in Dorico (2026-04-23) and reported a clean, audible result: "it works I can hear it clearly" — no glide, no click, no zipper.

## Verdict

**VALIDATED.** The drain-before-render ordering is correct by construction, and confirmed aurally. No further work needed on the transient path for the Dorico use case (same-block NE events).

### Edge case left open (not blocking — noted for real build)

If an NE event arrives in a *later* block than its `noteOn`, our spike code skips it (`"NE noteId=X has no matching NoteOn this block — skip"`). Dorico always emits them in the same block per the research, so this is fine for the Dorico case. Real build should keep a persistent `noteId → voice` map that survives across blocks, in case a host we haven't tested yet emits NE mid-note.
