# Stage 2 (DSP) — SUMMARY

**Plugin:** O-simpleBeatmaker · **Stage:** 2 DSP · **Mode:** express (staged) · **Date:** 2026-06-25
**Result:** All 6 render-harness probes green · VST3+AU+Standalone build clean · pluginval strictness-10 SUCCESS.

---

## What was implemented (by sub-phase)

### Phase 2.1 — DrumVoiceEngine (MIDI-playable voices)
- 6 synthesized 808/909-lineage voices as suffixed structs (`KickVoice`, `SnareVoice`, `ClapVoice`,
  `HatVoice`, `TomVoice`) — never bare `Voice`, never a `juce::` type name. `HatVoice` serves BOTH
  closed (42) and open (46) from a shared high-passed noise source with separate envelopes +
  `applyChoke()` (closed fires -> open tail fast-fades ~3 ms).
  - Kick: fastSine body + fast exp pitch env (~+1-2 oct) + amp env + click. Tom: pitched body + mild
    env. Snare: two detuned sines + band-passed noise (tone = body<->noise). Clap: 3 retriggered noise
    bursts (RT-safe counters) -> BP + tail. Hats: shared HP noise, short/long decay + choke.
  - Exponential envelopes flushed to 0 below 1e-6 (denormal-safe). Per-voice decay-ms mapping lives in
    the voices, not the param range. Velocity -> loudness + timbre, composed with per-voice tone.
  - Filtered-noise voices use dsp::StateVariableTPTFilter::processSample; noise = inline xorshift32
    seeded in prepareToPlay (never on the audio thread).
- UnifiedTriggerRouter (MIDI half): noteToVoice[128] GM map, mute/solo gating, sub-slice render driver
  transcribed from Synthesiser::processNextBlock (skips minimumSubBlockSize coalescing).
- Mixer/master: SmoothedValue output gain (per-sample ramp); -60 dB => silence.
- Processor: ScopedNoDenormals, 42 params cached + read once/block, setLatencySamples(0) (never overrides
  the non-virtual getLatencySamples), block-level NaN scrub.

### Phase 2.2 — SequencerClock (host-synced grid)
- getPlayHead()->getPosition() once/block with 3 guards; free-run fallback (integrated phaseInSteps at
  tempo) when absent/stopped.
- Pattern alignment PERIOD-aligned to the global ppq origin (barStart = ppqStart - fmod(ppqStart,
  patternLen*0.25)) — robust for 8/16/32, shared by synced + free-run for a seamless handoff. Stateless
  per-block enumeration over a half-open window + neighbour bars => each step fires exactly once at block
  edges. playheadStepPhase atomic stored each block.
- Sequencer emits noteOn into member sequencerMidi (pre-ensureSize'd), addEvents(host) merges sorted,
  router sub-slices the merged stream.

### Phase 2.3 — TimingFeelEngine + VizAnalyzer (the lesson)
- Terms kept separate (DSP-04): dSwing = (k odd) ? (swing01/3)*T8 : 0 (NOT scaled by q); humanize sampled
  once per hit from pre-seeded per-voice juce::Random; dtSec = dSwing + dHumanT*(1-q);
  finalVel = clamp(stepVel + dHumanV*(1-q),1,127).
- Fallback A adopted: timing humanize late-only [0,+30ms]; velocity humanize symmetric +-24.
- Carry-over queue (fixed std::array<PendingHit,64>, alloc-free) drains late hits crossing the block end;
  cleared on transport discontinuity.
- VizAnalyzer = AbstractFifo + POD VizEvent ring + atomic playheadStepPhase. The viz push happens in the
  SAME emitSequencerHit call as sequencerMidi.addEvent (one emit = one push) => QUAL-02 by construction.
  Host-MIDI note-ons also push (source=1, dt=0).

---

## Files created / modified

Created (Source/): BeatmakerIDs.h (shared roster/GM map/APVTS IDs, extracted to break a circular
include), fastSine.h, DrumVoiceEngine.h, UnifiedTriggerRouter.h, SequencerClock.h, TimingFeelEngine.h,
VizAnalyzer.h.
Created (tests/): tests/render-harness/CMakeLists.txt, tests/render-harness/main.cpp.
Modified: Source/PluginProcessor.h (owns the spine + cached param pointers + carry-over queue + viz
accessor + #if OUARICON_BUILD_TESTS hooks; ID block moved to BeatmakerIDs.h), Source/PluginProcessor.cpp
(prepareToPlay, full processBlock, cacheParamPointers, patternLengthSteps, emitSequencerHit,
drainCarryOver), CMakeLists.txt (new headers in target_sources; un-commented OUARICON_BUILD_TESTS option
+ add_subdirectory).

---

## Final probe results (harness PASS output)

    O-simpleBeatmaker render-harness — fs=44100, block=512
      [PASS] voices-make-sound          Kick=0.5454 Snare=0.4241 Clap=0.1142 Closed Hat=0.1258 Open Hat=0.2672 Tom=0.5043
      [PASS] hat-choke                  openTail=0.1275 chokedTail=0.0092
      [PASS] velocity-scales            soft=0.2607 loud=0.5824
      [PASS] high-rate-bounded          peak=1.1045
      [PASS] band-clean-tonal           harm=0.5425 alias=0.0251
      [PASS] grid-accuracy              hits=47 viz=47 maxNominalErr=0
      [PASS] swing-offset               expectSwing=3675 hits=33
      [PASS] humanize-spread            offset[137,1267] vel[79,122] maxLate=1323
      [PASS] quantize-preserves-swing   q=1: swing=3675 survives, humanize->0, hits=33
      [PASS] block-boundary             straight: 70 unique fires once=Y | swung once=Y
      [PASS] viz-truth                  hits=134 viz=134 fifoAgrees=Y

    ALL PASS — 0 failure(s)

Probe -> ROADMAP: 1 grid-accuracy; 2 swing-offset; 3 humanize-spread + quantize-preserves-swing (DSP-04);
4 block-boundary; 5 the five voice checks; 6 viz-truth.

pluginval: --strictness-level 10 -> SUCCESS (exit 0, zero FAIL/ERROR/exception). Internal auval test
inside pluginval passed; standalone auval -v aumu OSiB deferred to /install-plugin (needs the bundle
registered in the system AU folder).

---

## Deviations / notes

- Fallback A (late-only humanize) — blessed by ARCHITECTURE Risk. Symmetric (early) timing humanize is a
  post-gate enhancement; velocity humanize stays symmetric +-24.
- Harness CMake diverges from the FM template: NO *_UIResources link, JUCE_WEB_BROWSER=0 (Stage-2 editor
  is a GenericAudioProcessorEditor shell — verified no WebView symbols, links clean).
- SequencerClock pattern alignment uses the period-aligned origin (ppqStart - fmod(...)) rather than the
  literal getPpqPositionOfLastBarStart() — last-bar-start alignment breaks 2-bar/32-step patterns; the
  period-aligned form (the RESEARCH fallback formula) is correct for all of 8/16/32. The harness
  FakePlayHead still supplies getPpqPositionOfLastBarStart().
- BeatmakerIDs.h extraction (refactor, not behavior): shared roster/GM/ID block moved out of
  PluginProcessor.h so the DSP spine headers can include it without a circular dependency.
- VizEvent sample fields stored raw/unwrapped; UI subtracts for dt (Mechanism Gap #2).
- bpm read once/block — sub-block tempo ramps out of scope for v1.0.

### Gotcha hit & resolved (carry forward)
Half-sample grid-boundary rounding split (caused Probes 1/2/4 to fail on the first build). Odd 16ths land
on a fractional sample (e.g. 5512.5). The bar-relative nominalSampleInBar (llround(k*samplesPer16th)=5513)
and the naive in-block offset (llround((stepPpq-ppqStart)*samplesPerPpq)) rounded to different neighbours
— the latter lost the .5 to floating-point subtraction error and produced 5512. Fix: anchor the in-block
offset to the bar-relative grid value — off = llround((barRepStartPpq-ppqStart)*samplesPerPpq) +
nominalSampleInBar — so the emitted MIDI offset and the viz value are identical by construction (applied
to synced + free-run). This is the one place where audio<->viz consistency is load-bearing for QUAL-02.

---

## RT-safety / quality bars (verified)
- processBlock allocation-free + lock-free in the shipping build: the only push_back (test hook) and
  reserve are #if OUARICON_BUILD_TESTS (the plugin target never defines it -> pluginval ran on the real,
  hook-free binary). RNG pre-seeded in prepareToPlay; sequencerMidi.ensureSize(4096) + fixed std::array
  carry-over preallocated; APVTS via cached pointers; audio->UI strictly AbstractFifo + atomic.
- ScopedNoDenormals + per-voice env flush below 1e-6. Zero added latency. 42-param APVTS contract intact;
  grid stays the lock-free atomic array. No juce:: type-name collisions.
