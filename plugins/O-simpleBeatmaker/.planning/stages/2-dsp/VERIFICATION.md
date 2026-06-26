---
phase: 2-dsp
verified: 2026-06-25T00:00:00Z
status: human_needed
score: 12/12 gate must-haves verified
behavior_unverified: 2
overrides_applied: 0
behavior_unverified_items:
  - truth: "Per-voice mute/solo silences the right voices (FUNC-06)"
    test: "In a DAW/Standalone, program all 6 voices; toggle each Mute, then each Solo; confirm muted voices go silent and a solo silences the rest."
    expected: "Mute removes that voice; any Solo active mutes all non-soloed voices. (Gate is at emit: UnifiedTriggerRouter::isVoiceAudible — no audio, no viz for a gated voice.)"
    why_human: "No render-harness probe exercises the mute/solo gate; it is code-present and wired (processBlock emit gate + router.setMuteSolo) but the silencing invariant is not exercised by an automated test."
  - truth: "8- and 32-step pattern lengths wrap correctly (FUNC-07)"
    test: "Set Pattern Length to 8, then 32; play a full loop in a DAW and confirm steps wrap at the selected length with no drift, double-fire, or miss."
    expected: "patternLength choice idx 0/2 -> 8/32 steps; grid wraps at that length both host-synced and free-run."
    why_human: "All six probes run patternLen=16 only. The 8/32 wrap path (SequencerClock barLenSteps / patternLength) is implemented and symmetric in patternLength but is not exercised by any probe."
human_verification:
  - test: "Real-DAW host-transport sync (Logic AU / Reaper or Ableton VST3) + Standalone free-run"
    expected: "Sequencer locks to DAW tempo + play position; playhead tracks ppq; transport stop -> free-run at the tempo param; relocate/loop resyncs without double-fire. Pattern downbeat sits where expected (period-aligned origin, see deviation 3)."
    why_human: "The harness uses a synthetic FakePlayHead. The sync MATH is probe-verified (grid-accuracy, block-boundary), but real-host PositionInfo behavior (optional fields, ppq discontinuity on loop/relocate, tempo automation) is integration-level and belongs to /install-plugin."
  - test: "auval -v aumu OSiB Ouar after install"
    expected: "AU validates and renders audio (catches NaN/crash) once the bundle is registered in the system AU folder."
    why_human: "auval needs the AU registered in ~/Library/Audio/Plug-Ins/Components — deferred to /install-plugin. pluginval's internal AU test already passed at strictness-10."
  - test: "Per-step velocity quick-states (ghost/normal/accent) audibly change dynamics (FUNC-02)"
    expected: "setStepVelocity storage + velocity->loudness/timbre is in place; the click-again-to-accent UI quick-states land in Stage 3. Confirm audible dynamics from per-step velocity."
    why_human: "DSP half (per-step velocity honored, velocity scales loudness — probe 'velocity-scales' PASS) is verified; the accent-cycling UI is a Stage-3 deliverable, not Stage 2."
---

# Phase 2 (DSP): O-simpleBeatmaker Verification Report

**Phase Goal:** Implement the full immutable DSP spine — 6 synthesized 808/909 voices (MIDI-playable), a host-synced sample-accurate step sequencer, and the swing/humanize/quantize timing-feel engine — where the internal sequencer emits GM-mapped `MidiMessage` note-ons at sample-accurate offsets into the SAME `MidiBuffer` as host MIDI, and the viz tap reads those very messages so QUAL-02 is true by construction. First audio happens here.
**Verified:** 2026-06-25
**Status:** human_needed (Stage-2 gate fully PASS; routine DAW/install confirmations outstanding)
**Re-verification:** No — initial verification

## Goal Achievement

The Stage-2 gate, as the PLAN itself defines it ("the 6 render-harness probes + builds + pluginval — these ARE the gate"), is fully met. I re-ran the render-harness binary myself: **ALL 11 checks PASS, exit 0.** Build is clean (VST3 + AU + Standalone). pluginval VST3 strictness-10 SUCCESS (orchestrator-collected; pluginval's internal AU test passed too). The four `human_needed` items are wired-in-code facets not exercised by an automated probe, or integration smokes the PLAN explicitly routes to `/install-plugin` — none is a goal failure.

### Observable Truths (the 6 probes + the engineering invariants)

| #   | Truth (success criterion) | Status | Evidence |
| --- | ------------------------- | ------ | -------- |
| 1 | **Probe 1 — grid accuracy** (DSP-05): straight time, each ON step fires at exactly `round(k*samplesPer16th)` ±0 | ✓ VERIFIED | Re-ran harness: `grid-accuracy hits=47 viz=47 maxNominalErr=0`. `main.cpp:391-396` asserts `appliedOffsetSamples==0`, `nominalSampleInBar==round(k*samplesPer16th)`, `barRelativeOK(...,0)`, `midiHasHit`. |
| 2 | **Probe 2 — swing** (DSP-02): swing=75% → odd 16ths delayed exactly `round((1/3)*T8*fs)`, even unmoved | ✓ VERIFIED | `swing-offset expectSwing=3675 hits=33`. `main.cpp:420-421` `want = odd? expectSwing : 0`; exact equality. `TimingFeelEngine.h:70` `dSwing = (k odd)?(swing01/3)*T8:0`. |
| 3 | **Probe 3 — humanize + quantize / DSP-04 invariant**: hum spread bounded late-only +30 ms, vel ±24; q=1 → humanize→0 BUT swing survives | ✓ VERIFIED | `humanize-spread offset[137,1267] vel[79,122] maxLate=1323`; `quantize-preserves-swing q=1: swing=3675 survives, humanize->0`. See DSP-04 analysis below. |
| 4 | **Probe 4 — block-boundary independence** (DSP-05): step (±Δt) straddling a block edge fires once at the correct abs sample | ✓ VERIFIED | `block-boundary straight: 70 unique fires once=Y \| swung once=Y`. Tested at oddBlock=423; both straight + swung (carry-over) paths, `fireCount==1` per (voice,step,bar). |
| 5 | **Probe 5 — MIDI-playable + voices** (DSP-01/06, QUAL-01): each GM note → correct voice, choke, velocity scaling, aliasing budget | ✓ VERIFIED | `voices-make-sound` (all 6 >0.003 RMS), `hat-choke openTail=0.1275 chokedTail=0.0092`, `velocity-scales soft=0.2607 loud=0.5824`, `high-rate-bounded peak=1.1045`, `band-clean-tonal harm=0.5425 alias=0.0251` (alias ≪ harm). |
| 6 | **Probe 6 — viz truth** (QUAL-02 by construction): emitted `appliedSampleInBar − nominalSampleInBar` == Δt baked into the MidiMessage; FIFO agrees | ✓ VERIFIED | `viz-truth hits=134 viz=134 fifoAgrees=Y`. One emit = one push in `PluginProcessor.cpp:193-221 emitSequencerHit` (addEvent + viz.push in the same call). |
| 7 | Builds clean: VST3 + AU + Standalone | ✓ VERIFIED | Orchestrator: ninja clean. Harness links `PluginProcessor.cpp`/`PluginEditor.cpp` and runs → editor/symbols resolve. |
| 8 | pluginval VST3 strictness-10 SUCCESS | ✓ VERIFIED | Orchestrator re-run: exit 0, zero FAIL/ERROR. (auval deferred to install — human item.) |
| 9 | `processBlock` allocation-free + lock-free (PERF-01) | ✓ VERIFIED | See PERF-01 analysis below. Test-hook alloc is `#if OUARICON_BUILD_TESTS` only; LUT warmed in `prepareToPlay`. |
| 10 | Zero latency; `getLatencySamples()` never overridden | ✓ VERIFIED | `PluginProcessor.cpp:142 setLatencySamples(0)`; no `getLatencySamples` override in `PluginProcessor.h` (grep clean). |
| 11 | 42-param APVTS contract intact; grid stays custom atomics (NOT 384 params) | ✓ VERIFIED | `createParameterLayout`: 5 + 6×6(36) + 1 = **42**. Grid = `std::array<std::atomic<uint8_t>, 6*32>` (`PluginProcessor.h:116`), persisted in a "PATTERN" ValueTree child, not APVTS. |
| 12 | No `juce::`-type name collisions (suffixed `*Voice`) | ✓ VERIFIED | `KickVoice/SnareVoice/ClapVoice/HatVoice/TomVoice`; no bare `Voice` (enum kept). Builds + harness link clean. |

**Score:** 12/12 gate truths verified. 2 wired-but-unprobed behaviors (mute/solo, 8/32 wrap) + 2 integration smokes (real-DAW sync, auval) routed to human/install verification.

### DSP-04 invariant — implemented as separate terms (not coincidental)

`TimingFeelEngine::compute` (`TimingFeelEngine.h:62-86`) keeps the terms unfolded exactly as the contract demands:
- `dSwing = ((stepIndex % 2) == 1) ? (swing01/3.0)*T8 : 0.0` — deterministic, **never multiplied by q**.
- `dHumanT`, `dHumanV` sampled once per hit from pre-seeded per-voice `rng[v]`.
- `dtSec = dSwing + dHumanT*(1.0 - q)` and `finalVel = clamp(stepVel + dHumanV*(1.0 - q), 1, 127)` — `(1-q)` scales **only** the humanize terms.

At q=1 the humanize terms vanish algebraically while `dSwing` survives. Probe 3B proves it behaviorally, not coincidentally: with hum=1, swing=75%, q=1 the harness asserts `appliedOffsetSamples == (odd?3675:0)` exactly AND `velocity == 100` for every one of 33 hits — i.e. swing exact, humanize identically zero.

### PERF-01 — allocation-free / lock-free in the shipping build

- Test-hook `std::vector` push_back/reserve are `#if OUARICON_BUILD_TESTS` only (`PluginProcessor.cpp:136-138, 214-220`); the shipping target never defines it, so pluginval ran on the hook-free binary.
- LUT lazy-init removed from the audio path: `fastSine(0.0f)` warm-up in `prepareToPlay` (`PluginProcessor.cpp:120`) forces the function-local-static `__cxa_guard` + heap fill on the message thread (fixes critic advisory DSP-001).
- RNG pre-seeded in `prepareToPlay` (`TimingFeelEngine.h:51-58`), sampled by value on the audio thread, never reseeded.
- `sequencerMidi.ensureSize(4096)` preallocated; carry-over is a fixed `std::array<PendingHit,64>`; APVTS read via cached `std::atomic<float>*` once/block; audio→UI strictly `juce::AbstractFifo` + `std::atomic<float>` (`VizAnalyzer.h`). `ScopedNoDenormals` + per-voice env flush below 1e-6.

### Required Artifacts

| Artifact | Expected | Status | Details |
| -------- | -------- | ------ | ------- |
| `Source/fastSine.h` | band-clean sine, mandatory floor-mod wrap + isfinite guard, `namespace OSimpleBeatmaker` | ✓ VERIFIED | Wrap `phase -= twoPi*floor(phase/twoPi)` + `isfinite` guard present; namespace renamed from FM. |
| `Source/DrumVoiceEngine.h` | 5 voice structs + 6-voice container, choke, clap multi-burst, vel→timbre | ✓ VERIFIED | All voices present; `HatVoice.applyChoke()` swaps to ~3 ms coef; `ClapVoice` RT-safe burst counters; xorshift seeded in prepare. |
| `Source/UnifiedTriggerRouter.h` | GM reverse map, mute/solo gate, sub-slice driver (Synthesiser transcription) | ✓ VERIFIED | `noteToVoice[128]`; `isVoiceAudible`; `renderMerged` transcribes processNextBlock, skips minimumSubBlockSize coalescing. |
| `Source/SequencerClock.h` | host-synced enumeration + free-run + discontinuity safety | ✓ VERIFIED | Period-aligned origin, half-open window, neighbour-bar scan, discontinuity flag, free-run integrator. |
| `Source/TimingFeelEngine.h` | exact unfolded Δt math, Fallback A late-only, pre-seeded RNG | ✓ VERIFIED | See DSP-04 analysis. `triRand01 ∈ [0,1]` (late-only); `triangular ∈ [-1,1]` for velocity. |
| `Source/VizAnalyzer.h` | AbstractFifo + POD VizEvent ring + atomic playhead | ✓ VERIFIED | RAII `fifo.write(1)` producer; `playheadStepPhase` separate atomic; frame counter. |
| `Source/BeatmakerIDs.h` | shared roster/GM/IDs (circular-include break) | ✓ VERIFIED | Single source of truth; `kGmNotes{36,38,39,42,46,45}`; 42 IDs. (Documented refactor, behavior-neutral.) |
| `Source/PluginProcessor.{h,cpp}` | own the spine, processBlock pipeline, test hooks | ✓ VERIFIED | 7-step processBlock matches ARCHITECTURE processing order; cached pointers; carry-over; viz push. |
| `tests/render-harness/{main.cpp,CMakeLists.txt}` | FakePlayHead + 6 probes | ✓ VERIFIED | Built + run, exit 0. CMake diverges from FM template per deviation 2. |
| `CMakeLists.txt` | new headers in target_sources; OUARICON_BUILD_TESTS option + add_subdirectory | ✓ VERIFIED | Lines 29-43 (headers), 91-93 (option + add_subdirectory). |

### Key Link Verification

| From | To | Via | Status |
| ---- | -- | --- | ------ |
| SequencerClock | TimingFeelEngine | `firingColumns` → `feel.compute` per ON cell | ✓ WIRED (`PluginProcessor.cpp:306-316`) |
| TimingFeelEngine | sequencerMidi | `emitSequencerHit` → `addEvent` at finalOffset | ✓ WIRED (`cpp:193-204`) |
| emit | VizAnalyzer | `viz.push(ev)` in the SAME emit call (QUAL-02) | ✓ WIRED (`cpp:205-212`) — probe 6 confirms |
| sequencerMidi + host MIDI | DrumVoiceEngine | `addEvents(host)` merge → `router.renderMerged` sub-slice | ✓ WIRED (`cpp:350-353`) |
| PATTERN grid (atomics) | SequencerClock emit | `getStep(v,k)` lock-free load | ✓ WIRED (`cpp:311`) |
| Closed Hat trigger | Open Hat choke | `hat.applyChoke()` on ClosedHat trigger | ✓ WIRED (`DrumVoiceEngine.h:428`) — probe `hat-choke` confirms |

### Requirements Coverage

| Requirement | Status | Evidence |
| ----------- | ------ | -------- |
| FUNC-01 (16-step grid drum machine) | ✓ SATISFIED | Grid atomics + sequencer emit; probes 1/4. |
| FUNC-02 (per-step velocity, ghost/normal/accent) | ✓ DSP / ? UI | Per-step velocity stored + honored (`velocity-scales` PASS); click-again accent UI is Stage-3. |
| FUNC-03 (host-synced sequencer) | ✓ SATISFIED (synthetic) / ? real-DAW | Sync math probe-verified via FakePlayHead; real-DAW smoke → human/install. |
| FUNC-04 (MIDI-playable, GM map) | ✓ SATISFIED | Probe 5 voices-make-sound per GM note; one merged stream. |
| FUNC-06 (mute/solo) | ⚠️ wired, unprobed | `isVoiceAudible` gate present; no probe → human verify. |
| FUNC-07 (8/16/32 length) | ⚠️ wired, unprobed | Choice→8/16/32 mapped; only 16 probed → human verify. |
| DSP-01 (synthesized voices, no samples) | ✓ SATISFIED | 6 synth voices; probe 5. |
| DSP-02 (swing 0–75%) | ✓ SATISFIED | Probe 2. |
| DSP-03 (humanize timing+velocity) | ✓ SATISFIED | Probe 3A. |
| DSP-04 (quantize pulls humanize, not swing) | ✓ SATISFIED | Probe 3B + separate-term code. |
| DSP-05 (sample-accurate sub-step Δt) | ✓ SATISFIED | Probes 1/4 (±0, no block snap). |
| DSP-06 (per-voice tune/decay/tone/level) | ✓ SATISFIED | `setParams` per voice; probe 5 voices distinct. |
| PERF-01 (RT-safe, lock-free handoff) | ✓ SATISFIED | See PERF-01 analysis; pluginval s10. |
| PERF-02 (no dropouts, sample-accurate) | ✓ SATISFIED (light path) | ~6 short voices + sub-slice; pluginval s10 thrash clean. Real-load feel → DAW. |
| QUAL-01 (no clicks/zipper/aliasing) | ✓ SATISFIED | `band-clean-tonal`, `high-rate-bounded`, smoothed gain, env flush. |
| QUAL-02 (viz matches audio) | ✓ WIRED here / surfaced Stage-3 | Probe 6 by construction; UI lands Stage-3. |

### Anti-Patterns Found

None blocking. No unreferenced `TBD/FIXME/XXX` debt markers in the Stage-2 sources. Empty-return / hardcoded-empty patterns are confined to legitimate guards (out-of-range grid access, null-playhead fallback) and `#if OUARICON_BUILD_TESTS` blocks. Critic advisory DSP-001 (lazy LUT init) was fixed (warm-up in prepareToPlay); five other critic warnings remain advisory/deferred.

### Deviations (all contract-blessed or justified + documented)

1. **Fallback A — late-only timing humanize** (`Δhuman_t ∈ [0,+30ms]`): blessed by ARCHITECTURE Risk §Fallback A + RESEARCH Mechanism Gap #1. `TimingFeelEngine.h:95-98 triRand01` is one-sided; velocity humanize stays symmetric ±24. Removes the negative-offset cross-block lookahead. Symmetric timing is a documented post-gate enhancement. **Not a goal failure.**
2. **Harness CMake divergence**: no `*_UIResources` link, `JUCE_WEB_BROWSER=0` at Stage 2 (no binary-data target; editor is a GenericAudioProcessorEditor shell — verified no WebView symbols, links clean). **Justified, documented; revisit Stage 3.**
3. **Period-aligned origin** (`barStart = ppqStart − fmod(ppqStart, patternLen*0.25)`) instead of `getPpqPositionOfLastBarStart()`: last-bar-start alignment breaks 2-bar/32-step patterns; the period-aligned form (the RESEARCH fallback) is correct for 8/16/32 and is shared by synced + free-run for a seamless handoff. The harness FakePlayHead still supplies `getPpqPositionOfLastBarStart()` but the clock ignores it. **Justified, documented.** Residual: in a real DAW, downbeat alignment on relocate to arbitrary positions should be confirmed by ear (see human item 1).
4. **VizEvent fields stored raw/unwrapped** (Mechanism Gap #2): UI subtracts for Δt; bar-position modulo handled Stage-3. **Documented.**
5. **bpm read once/block**: sub-block tempo ramps out of scope for v1.0 (Mechanism Gap #4). **Accepted simplification.**

### Human Verification Required / Residual Risk for Stage 3 / install

1. **Real-DAW host-transport sync** (Logic AU / Reaper / Ableton VST3 + Standalone free-run) — synthetic playhead only in the harness; real PositionInfo / loop-relocate / tempo-automation is integration-level → `/install-plugin`.
2. **auval -v aumu OSiB Ouar** after install — needs system AU registration; pluginval's internal AU test already passed.
3. **Per-voice mute/solo audible silencing** (FUNC-06) — code-wired (`isVoiceAudible`), no probe.
4. **8/32 pattern-length wrap** (FUNC-07) — code-wired, only 16 probed.
5. **Per-step velocity accent quick-states audible** (FUNC-02 UI) — DSP verified; accent-cycling UI is a Stage-3 deliverable.

### Gaps Summary

No gaps in the Stage-2 gate. The defined gate (6 probes + clean build + pluginval s10) is fully green, re-verified independently. The DSP spine is complete, sample-accurate, RT-safe, and the QUAL-02 viz tap is truthful by construction. The four outstanding items are routine DAW/install confirmations (real-host sync, auval) and two wired-but-unprobed facets (mute/solo, 8/32 wrap) — none blocks the Stage 2→3 handoff; they are the natural smoke checks the PLAN already routes to `/install-plugin` and the next stage.

---

## Overall Stage-2 Verdict: PASS

All six render-harness probes pass (11/11 checks, exit 0, independently re-run), VST3+AU+Standalone build clean, pluginval VST3 strictness-10 SUCCESS, DSP-04 implemented as genuinely separate terms, PERF-01 (alloc-free/lock-free) and zero-latency confirmed, 42-param contract + custom-atomic grid intact. Frontmatter status is `human_needed` only because honest goal-backward verification surfaces wired-but-unprobed facets (mute/solo, 8/32 wrap) and the DAW/auval integration smokes the PLAN defers to `/install-plugin` — not because the Stage-2 gate has any gap. **Ready for the Stage 2→3 handoff.**

---

_Verified: 2026-06-25_
_Verifier: Claude (gsd-verifier)_
