# Stage 0 (Ideation / Research & Planning) — CONTEXT

**Plugin:** O-simpleBeatmaker
**Date:** 2026-06-25
**Outcome:** Stage 0 complete — ARCHITECTURE.md + ROADMAP.md produced. Ready for Stage 1 (Foundation).

---

## What this plugin is

The rhythm sibling of O-simpleFM / O-simpleAdditive / O-simpleGrain / O-simpleSubtractive — same pedagogical DNA (gesture → visible consequence, tooltips, concept-isolating presets, one projector-readable page), but the subject is **MIDI sequencing & timing feel**, not a synthesis engine. A deliberately simple TR-808/909-lineage step-sequencer drum machine: program a beat on a 6×16 grid, then watch and hear velocity, swing, humanize, and quantize-strength reshape it in real time. North star: a curious student reaches an "oh, THAT'S what swing/quantize/velocity do" moment within 5 minutes, no manual. Built for MUSC319 wk09 (MIDI = control data, not audio; a beat is programmed on a grid where velocity/quantize/swing/humanize separate a stiff pattern from a living one).

---

## The defining architectural decision

**The internal sequencer emits its hits as GM-mapped `juce::MidiMessage` note-ons, inserted at sample-accurate offsets into the SAME `MidiBuffer` that host MIDI arrives on.** One merged, sorted MIDI stream feeds both the drum voices and the visualization tap. This makes the brief's teaching claim ("step grid and piano roll are two views of one MIDI stream") literally true in code, and it makes three otherwise-hard things fall out for free:
1. Sub-step Δt is just the message's `samplePosition` (no block-boundary snapping).
2. MIDI-playable voices need no separate path (a host note-on and a sequencer note-on are identical to a voice).
3. The timing-lane shows the **applied** Δt (read from the same message) → QUAL-02 by construction, not by recomputation.

Everything else in the architecture serves this spine.

---

## Resolved open questions (from BRIEF / parameter-spec-draft)

1. **808-vs-909 flavor per voice — RESOLVED:** Kick 808 (long sub boom — best "decay" lesson), Snare 808/909 hybrid (body↔noise blend via Tone), Clap 808 (3–4 retriggered noise bursts + diffuse tail), Closed/Open Hat band-passed-noise (chosen for see-inside clarity; the 6-oscillator metallic source is an optional Stage-4 enhancement), Tom 808 (pitched body). 6 voices: **Kick, Snare, Clap, Closed Hat, Open Hat, Tom.** Rimshot/Cowbell out of v1.0.

2. **Exact swing curve + 8th vs 16th — RESOLVED:** 16th-note swing (natural for a 16-step grid). MPC-canonical ratio `s = 0.5 + (swing/75)/3` so the 0–75% param maps to ratio 0.5→0.75 (50%-display → triplet swing; 75% → MPC max). Off-beat (odd-index) 16ths delayed by `Δswing = (s−0.5)·T8`. **Swing is deterministic and is never removed by quantize.** 8th/16th toggle deferred to v1.1.

3. **Humanize distribution + timing/velocity split + composition with quantize — RESOLVED:** per hit, sampled once from a pre-seeded `juce::Random`: timing `±humanize·30 ms`, velocity `±humanize·24` (triangular distribution, centre-weighted). Composition: `Δt = Δswing + Δhuman·(1−q)`, `vel = clamp(stepVel + Δhuman_v·(1−q), 1, 127)` where `q = quantizeStrength`. At q=100%: humanize fully removed (dead tight) but swing remains — the entire quantize-vs-feel tradeoff in one line.

4. **Sample-accurate sub-step Δt scheduling — RESOLVED (and is the highest-risk item):** the sequencer emits timed note-ons into the shared MidiBuffer; voices sub-slice their render on event sample positions (the same mechanism `juce::Synthesiser` uses for host MIDI). A one-block lookahead keeps symmetric (±) humanize representable. **Fallback A** if the lookahead is fiddly: late-only humanize. **Fallback B:** per-sample trigger array. The offline render-harness is the gate (assert exact sample offsets before building the UI).

5. **GM drum note map — RESOLVED:** 36 Kick, 38 Snare, 39 Clap, 42 Closed Hat, 46 Open Hat, 45 Tom. Closed hat (42) chokes open hat (46) — classic and pedagogically visible (included).

6. **Per-voice polyphony / tail — RESOLVED:** each voice is monophonic-retrigger with its own decaying tail; no note-off handling for one-shots (except the HH choke). ~6 voices mixed.

7. **Internal free-run tempo for standalone — RESOLVED:** host-synced whenever the host is playing; internal free-run clock at the `tempo` param (default 120 BPM) when there is no playhead or transport is stopped.

8. **Pattern/velocity matrix state representation — RESOLVED (important):** the 6×32 grid + per-step velocity is **custom state, NOT APVTS parameters** (384 automatable params would be absurd and un-pedagogical). Stored as a flat array of `std::atomic<uint8_t>` (0=off, 1–127=on@velocity) for lock-free audio-thread reads; persisted in a child `ValueTree "PATTERN"` inside the APVTS state. Only the ~42 "knobs" are APVTS.

9. **Per-voice pan — DEFERRED to v1.1** (keeps the v1.0 mixer trivial; brief lists it as nice-to-have).

10. **Factory pattern presets — Stage 4** (FUNC-05): concept-isolating — Straight Beat, Triplet Swing, Humanized (loose), Quantize-Strength demo, Ghost-Note Groove. These are the lesson plan.

---

## Complexity & strategy

- **Tier:** synth + MIDI (tier 4) **escalated toward tier 6** by first-class real-time visualization (timing lane + MIDI readout) AND the brand-new host-transport-sync + sample-accurate scheduler → research depth **DEEP**.
- **Complexity score: 5.0** (capped; raw 10.0 = params 2.0 + 6 algorithms + 2 features). Caps like every sibling, but the difficulty is **timing infrastructure**, not DSP math.
- **Strategy: phased** — Stage 2 DSP in 3 phases, Stage 3 GUI in 3 phases.
- **Highest risk (~60%):** host-transport sync + sample-accurate sub-step Δt. De-risk by building the straight-time grid first (Phase 2.2), adding the feel math second (Phase 2.3), with the render-harness as the gate.

---

## Constraints honored

- JUCE 8.0.9; CMake+Ninja; macOS VST3+AU + Windows VST3.
- `IS_SYNTH TRUE` + `NEEDS_MIDI_INPUT TRUE`; `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`.
- If a 2nd `juce_add_binary_data` target is ever added, give it a **distinct `NAMESPACE`** (O-simpleGrain Stage 3.1 collision).
- RT-safe `processBlock`: no alloc/lock/file-I/O; humanize RNG pre-seeded; PATTERN grid via atomics; lock-free viz handoff via `AbstractFifo`.
- The timing lane carries the **applied** sample offset (QUAL-02), never a UI-side recomputation.
- `setLatencySamples(0)` (zero added latency, no oversampling); `getLatencySamples()` is non-virtual in JUCE 8.

## JUCE 8 APIs verified against local source (8.0.9)

- `juce::AudioPlayHead::PositionInfo` — `getBpm/getPpqPosition/getPpqPositionOfLastBarStart/getIsPlaying/getTimeInSamples/getTimeSignature` (all `Optional`); via `getPlayHead()->getPosition()`. **No sibling uses this — genuinely new.**
- `juce::MidiBuffer::addEvent(msg, sampleNumber)` — sorted insertion → sample-accurate scheduling primitive.
- `juce::AbstractFifo`, `juce::Random` (pre-seed), `dsp::StateVariableTPTFilter`/`dsp::IIR`, `dsp::LookupTableTransform`, `AudioProcessorValueTreeState` + `ValueTree`.

## Stage-2 correctness gate

Port the O-simpleFM offline render-harness with a synthetic `AudioPlayHead` + a test hook exposing the sequencer's emitted MidiBuffer. Probes: grid accuracy (±0 samples), swing offset = expected, humanize spread bounded, **quantize=100 collapses humanize but preserves swing**, block-boundary independence, MIDI-playable + HH choke, viz-Δt = applied-Δt.

## Sibling references (read in-repo)

- **O-simpleFM** — voice skeleton, lock-free `VizRing`/analyzer, WebView editor, CMake, render-harness. Primary template.
- **O-simpleSubtractive / O-simpleAdditive** — WebView pedagogical template + QUAL-02 truth-by-construction discipline + single-page projector layout.
- **O-simpleGrain** — BinaryData `NAMESPACE` collision lesson + render-harness gate precedent.

## Files produced this stage

- `plugins/O-simpleBeatmaker/.planning/research/ARCHITECTURE.md`
- `plugins/O-simpleBeatmaker/.planning/ROADMAP.md`
- `plugins/O-simpleBeatmaker/.planning/stages/0-ideation/CONTEXT.md` (this file)
- `plugins/O-simpleBeatmaker/.planning/STATUS.md` (updated → stage 0 complete)
