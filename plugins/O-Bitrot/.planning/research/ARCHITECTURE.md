# DSP Architecture: O-Bitrot

**Contract status:** BINDING — Stage 0 output. Stages 1-4 implement this document.
**Generated:** 2026-08-15 by research-planning-agent
**Plugin type:** Audio Effect (`aufx`) — broken-media degradation box, stereo in/out
**Complexity tier:** 3 (complex DSP) — research depth MODERATE, backed by pre-existing Level-3 research
**JUCE version:** 8.0.14 (local `/Users/taylorbrook/JUCE`, verified against source, not documentation)
**Primary sources:** `research/glitch-effects/degradation-dsp-deep-dive.md` (Level-3, 2026-08-14) §1–§6; `research/glitch-effects/multi-effect-sequencer-reuse-audit.md` (suite reuse map)
**Requirements covered:** FUNC-01…06, DSP-01…08, PERF-01, QUAL-01/02 (`.planning/REQUIREMENTS.md`)

> **API verification method.** Context7-MCP's documentation-fetch tool was not available in this
> session. All JUCE API claims below were verified **directly against the local JUCE 8.0.14 source
> tree** at `/Users/taylorbrook/JUCE` (the tree the plugin compiles against), plus repo-proven usage
> in shipped plugins (O-Polystutter, O-ReverseDelay). Verified this session:
> `juce::dsp::DryWetMixer` (`juce_dsp/processors/juce_DryWetMixer.h`),
> `juce::dsp::IIR::ArrayCoefficients` incl. `makeLowPass/makeHighPass/makeBandPass` with Q
> (`juce_dsp/processors/juce_IIRFilter.h:45–72`),
> `juce::dsp::FirstOrderTPTFilter`, `juce::dsp::StateVariableTPTFilter`,
> `juce::SmoothedValue` (`juce_audio_basics/utilities/juce_SmoothedValue.h`),
> `juce::Random` with `setSeed(int64)` (`juce_core/maths/juce_Random.h:46,112`),
> `juce::AudioPlayHead::PositionInfo` accessors `getBpm()/getPpqPosition()/getIsPlaying()`
> (`juce_audio_basics/audio_play_head/juce_AudioPlayHead.h:313+`).

---

## 0. Complexity Detection

| Signal | Finding |
|---|---|
| Parameter count | 31 (+1 UI-only reseed trigger) |
| DSP complexity | HIGH — six degradation families over one shared engine, mostly custom DSP |
| Non-DSP features | Vendored third-party codec (libgsm); no file I/O, no multi-out, no MIDI |
| UI complexity | Moderate — six panels + global strip, one action button (dice) |
| State | APVTS only (seed is a persisted Int param); no custom ValueTree needed |

**Tier: 3** (complex DSP algorithms). Effective research depth is DEEP because the repo already
contains a Level-3 deep dive written specifically for this plugin
(`research/glitch-effects/degradation-dsp-deep-dive.md`); this document builds on it per Stage 0
instruction rather than re-researching.

## 0.1 Feature Inventory (meta-research)

1. **MediaClock** — tempo-synced / free-running re-roll clock (FUNC-01, FUNC-03)
2. **Seeded RNG bank** — one stream per stochastic subsystem, deterministic renders (FUNC-04)
3. **CaptureRing + read-head engine** — shared circular buffer, per-channel variable-rate read heads, jump crossfades (FUNC-01, QUAL-01)
4. **Tape family** — interval-table speed bends, tape-stop, ramped transitions (DSP-01)
5. **CD-skip family** — CIRC severity ladder: LPF dip → mute+tick → segment loop + chirp (DSP-02)
6. **Vinyl family** — revolution-quantized jumps, synthesized pops, locked groove (DSP-03)
7. **Packet-loss family** — Gilbert–Elliott Markov over 20 ms packets, 4 concealment modes (DSP-04)
8. **Codec stage** — phone chain: BP → 8 kHz → μ-law round trip; GSM 06.10 via vendored libgsm (DSP-05)
9. **Crush stage** — fractional-hold SRR + jitter + fractional-bit quantizer + TPDF dither + envelope-driven depth (DSP-06/07/08)
10. **Global plumbing** — dry/wet with latency compensation, hardEdges, enable fades, state persistence (FUNC-05/06)

---

## Core Components

### MediaClock
- **JUCE Class:** Custom (uses `juce::AudioPlayHead::PositionInfo` for sync mode)
- **Purpose:** Emits the "re-roll" tick that quantizes all transport-failure decisions (FUNC-01/03)
- **Parameters Affected:** CLOCK_MODE, CLOCK_SYNC_DIV, CLOCK_FREE_RATE
- **Configuration:**
  - Sync mode: subdivision-boundary edge detection via `int(ppq / subdivPPQ)` crossing — port the proven `updateBeatSync` pattern from `plugins/O-Polystutter/Source/PluginProcessor.cpp:1691–1824` including BPM fallback/clamp and offline/no-playhead fallback (falls back to free-rate behavior at 120 BPM equivalent).
  - Free mode: sample-counting phase accumulator at CLOCK_FREE_RATE Hz.
  - **Tick resolution is sample-accurate within the block** — the tick's sample offset is computed and the state transition applied mid-block, otherwise QUAL-02 (512-vs-4096 invariance) fails. This is the exact trap in memory pattern "delay/grain read latched before the block's capture write breaks at blockSize ≥ D".
  - Divisions: 1/16, 1/8T, 1/8, 1/4T, 1/4, 1/2, 1 bar (7 choices).

### RngBank (seeded determinism)
- **JUCE Class:** `juce::Random` (verified: `setSeed(int64)`, `juce_core/maths/juce_Random.h`)
- **Purpose:** Deterministic per-subsystem randomness (FUNC-04, QUAL-02)
- **Parameters Affected:** SEED
- **Configuration:**
  - **One stream per stochastic subsystem** (repo pattern: a shared audio-thread RNG breaks block-size invariance — `pattern_rng_stream_interleave_blocksize`). Streams: `arbitration`, `tape`, `cd`, `vinyl`, `packet`, `jitter`, `dither`, `artifactSynth` (pops/chirps/tick noise).
  - Stream k seeded as `splitmix64(SEED * 0x9E3779B97F4A7C15 + k)` — decorrelated derived seeds from one user seed.
  - All streams re-seeded in `prepareToPlay()` and whenever SEED changes → two offline renders with identical seed/input/params are bit-identical (FUNC-04 acceptance). Mid-session realtime playback is not required to match a bounce started from zero — documented behavior.
  - Reseed dice = UI writes a new random value to the SEED Int parameter (message thread, via APVTS — never touches the audio thread directly).

### CaptureRing + ReadHead engine (MediaPlayer core)
- **JUCE Class:** Custom (modeled on `plugins/O-ReverseDelay/Source/dsp/CaptureBuffer.h` absolute-index ring + `plugins/O-Polystutter/Source/DSP/RepeatLane.cpp:183–217` varispeed read loop)
- **Purpose:** One continuously-written circular buffer; per-channel variable-rate read heads whose *behavior* is the current failure state (FUNC-01)
- **Parameters Affected:** (indirect — all transport families), HARD_EDGES
- **Configuration:**
  - Span constant: `kRingSeconds = 2.5` → `static_assert(kRingSeconds >= 1.8 + 0.5 + 0.1)` (max vinyl revolution + max tape ramp + safety margin — FUNC-01 acceptance). Sized in samples in `prepareToPlay()` (`kRingSeconds * sampleRate`); ~480 k samples/ch at 192 kHz ≈ 1.9 MB/ch, allocated once.
  - Absolute-index write head (no wrap ambiguity, O-ReverseDelay pattern). Read heads hold fractional absolute positions; linear interpolation read (matches RepeatLane; upgrade path: `juce::Interpolators` Lagrange if audible).
  - **Write-then-read order per sample** within processBlock so the read head may sit arbitrarily close behind the write head (blockSize-invariance trap, `pattern_grain_read_before_capture_write_blocksize`).
  - Read head clamped to `[writeAbs − ringSpan + margin, writeAbs − minLag]`; jump targets clamped before applying.
  - **All position jumps crossfaded 1–5 ms** (equal-gain linear, RepeatLane retrigger-crossfade generalization `RepeatLane.cpp:284–297`) **unless HARD_EDGES** — QUAL-01.
  - Bypass/passthrough state: read head tracks `writeAbs − compLatency` at rate 1.0 (see Latency).

### TapeTransport
- **JUCE Class:** Custom (varispeed read + ramped rate)
- **Purpose:** Speed bends from interval table, tape-stops, ramped transitions (DSP-01)
- **Parameters Affected:** TAPE_ENABLE, TAPE_PROB, TAPE_STOP_PROB, TAPE_RAMP
- **Configuration:**
  - Rate targets from musical-interval table `{1.0, 0.67, 1.5, 0.5, 2.0}` (deep-dive §4.2); tape-stop = ramp rate target to 0.0, hold for the state duration, ramp back on recovery.
  - Rate transitions through a linear ramp of TAPE_RAMP ms — **the ramp IS the glide sound and the click safety** (no crossfade needed for rate changes; only position jumps crossfade).
  - Rate applied as read-head increment; per-sample smoothed **target** (anti-zipper rule 1, §2.5). Phase/position accumulator is never reset on parameter change (rule 3).

### CDSkip
- **JUCE Class:** Custom (read-head behavior + `juce::dsp::FirstOrderTPTFilter` for concealment dulling)
- **Purpose:** CIRC failure ladder (DSP-02)
- **Parameters Affected:** CD_ENABLE, CD_PROB, CD_SEVERITY, CD_SEGMENT
- **Configuration:**
  - CD_SEVERITY (0–1) positions a weighting across the ladder; an event picks its rung by biased roll (cd RNG stream):
    - **Rung 1 — interpolation concealment** (severity ≈ 0–0.33 weighted): 30–80 ms LPF dip — FirstOrderTPTFilter cutoff swept 20 kHz → ~2 kHz → back.
    - **Rung 2 — mute** (≈ 0.33–0.66): 2–20 ms hard mute with a residual synthesized tick (2–3 ms filtered impulse from artifactSynth stream).
    - **Rung 3 — buffer loop** (≈ 0.66–1.0): read head loops a CD_SEGMENT-ms window at exact intervals (hard-edged repeats are the aesthetic; boundary crossfade still applied unless HARD_EDGES, at the minimum 1 ms), synthesized chirp (fast decaying sine sweep ~3 kHz→8 kHz, few ms) mixed at each segment restart; on recovery the read head **jumps forward toward the write head** (skip-ahead), crossfaded.
  - Loop repeat count derived from state duration (one or more clock ticks; re-rolled per tick).

### VinylTransport
- **JUCE Class:** Custom (read-head behavior)
- **Purpose:** Revolution-quantized jumps + pops; locked groove (DSP-03)
- **Parameters Affected:** VINYL_ENABLE, VINYL_PROB, VINYL_RPM, VINYL_POP
- **Configuration:**
  - Revolution quantum: 60/33.333 = **1.8 s** @ 33⅓, 60/45 = **1.333 s** @ 45; jump distance = ±1 revolution in samples. Backward jumps always available; "forward" jump implemented as jump toward the write head clamped to `writeAbs − minLag` (buffer has no future).
  - **Rate stays 1.0 across jumps — pitch never changes** (DSP-03 acceptance: autocorrelation pitch probe).
  - Locked groove: repeated backward jump of exactly one revolution each time the read head returns to the jump point — precise one-revolution loop with a pop per pass.
  - Pop synthesis: few-ms LPF'd impulse (artifactSynth stream, `FirstOrderTPTFilter` ~1–3 kHz), level VINYL_POP, mixed at each jump instant.

### PacketLossStage
- **JUCE Class:** Custom
- **Purpose:** Gilbert–Elliott bursty loss over 20 ms packets, 4 concealment modes (DSP-04)
- **Parameters Affected:** PACKET_ENABLE, PACKET_LOSS, PACKET_BURST, PACKET_CONCEAL
- **Configuration:**
  - Operates on MediaPlayer **output** at a fixed 20 ms packet grid (grid counter in samples, advanced per-sample — not tied to MediaClock; see Architecture Decisions).
  - 2-state Markov advanced once per packet (packet RNG stream): Good (P(loss) ≈ 1%) / Bad (P(loss) ≈ 50%). PACKET_LOSS maps to stationary Bad-state occupancy π_B; PACKET_BURST maps to expected burst length E[B] = 1/P(B→G) ∈ [1, 8] packets; solve `P(G→B) = π_B · P(B→G) / (1 − π_B)` clamped to [0, 1].
  - Concealment (deep-dive §3.4): 1 **Silence** (hard dropout), 2 **Repeat** (previous packet, robotic), 3 **Decay** (repeat with −3 dB per repetition, fades over ~60 ms), 4 **Substitute** (continue last pitch period: period from zero-crossing/AMDF estimate over the last good packet; fallback rung = Decay if estimate fails).
  - 1–5 ms crossfade at every packet boundary state change unless HARD_EDGES. Packet history buffer: 2 × 20 ms per channel, preallocated at max fs.

### CodecStage
- **JUCE Class:** `juce::dsp::IIR::Filter` with `ArrayCoefficients` (verified `juce_IIRFilter.h:45–72`) + custom μ-law + vendored libgsm
- **Purpose:** Landline / cellphone chain (DSP-05)
- **Parameters Affected:** CODEC_ENABLE, CODEC_MODE, CODEC_MIX
- **Configuration:**
  - Chain (deep-dive §3.3): mono sum → HPF 300 Hz (2× cascaded 2-pole = 4-pole) → LPF 3400 Hz (4-pole) → **crude** downsample to 8 kHz grid (fractional-hold latch, shared code with CrushStage — the degraded resampling IS the aesthetic; zero-latency by design) → codec round trip → hold-upsample back to fs → post LPF ~3400 Hz → equal-power blend CODEC_MIX with pre-codec stereo signal.
  - Filters: `ArrayCoefficients<float>::makeHighPass/makeLowPass` with Q — **RT-safe array form, never `Coefficients::makeXXX`** (repo pattern `pattern_arraycoefficients_rt_safe_iir`: 5-normalised vs 6-raw → Inf/NaN). Coefficients computed in `prepareToPlay` (cutoffs are fixed).
  - **μ-law mode:** continuous formula round trip + 8-bit quantize (deep-dive §1.4): `y = sign(x)·log1p(255|x|)/log1p(255)` → mid-tread 8-bit → `sign(y)·(256^|y| − 1)/255`. Zero latency.
  - **GSM mode:** vendored **libgsm** (GSM 06.10 full-rate; permissive TU-Berlin license, MIT-style — verify license file at vendoring time). `gsm_create()` in `prepareToPlay` (allocates once), `gsm_encode`/`gsm_decode` per 160-sample 8 kHz frame on the audio thread (allocation-free after create; RSBrokenMedia's `GSMProcessor` proves the pattern). Scale float → `(gsm_signal)(x * 4096) << 3` per deep-dive §3.3. Frame accumulation at the 8 kHz grid → **20 ms framing latency** (see Latency).
  - Mode switch μ-law ↔ GSM crossfaded ~10 ms to avoid clicks; both sub-paths delay-aligned (see Latency).

### CrushStage (SRR + jitter)
- **JUCE Class:** Custom (fractional-hold latch)
- **Purpose:** DeRez-style sweepable sample-rate reduction (DSP-06, DSP-08)
- **Parameters Affected:** CRUSH_ENABLE, CRUSH_RATE, CRUSH_JITTER
- **Configuration:**
  - Fractional-crossing interpolated hold (deep-dive §2.1, Airwindows DeRez fix): `phase += rate` with `rate = targetFs/fs`; at crossing, `held = lastSample·pos + input·(1−pos)` — kills the hold-period jitter warble, makes rate fully sweepable.
  - CRUSH_RATE smoothed at the **target** per-sample (one-pole, DeRez2 style `((v*999)+t)/1000` equivalent via `juce::SmoothedValue` on the target); **phase accumulator never reset** (anti-zipper rules §2.5; QUAL-01/DSP-06 acceptance).
  - CRUSH_JITTER: `phase += rate * (1 + jitterAmt * noise())` (jitter RNG stream) — Decimort-style inharmonic sideband hash.
  - Skew: CRUSH_RATE exponential (500 Hz–20 kHz UI range, clamped to fs/2 at runtime); no AA filters in v1.0 (aliasing is the point; tracking pre/post filters are a documented v1.1 extension per deep-dive §2.2).

### QuantStage (bits + dither + dynamic depth)
- **JUCE Class:** Custom + per-sample envelope follower
- **Purpose:** Fractional-bit quantization, TPDF dither, envelope-driven depth (DSP-06/07/08)
- **Parameters Affected:** CRUSH_BITS, CRUSH_DITHER, CRUSH_ENV_AMT
- **Configuration:**
  - Mid-tread quantize with fractional bits: `delta = 2·exp2(−bits)`, `out = delta·floor(x/delta + 0.5)` (deep-dive §1.1); bits target per-sample smoothed → zipper-free by construction.
  - TPDF dither: `(r1 − r2) · delta · (CRUSH_DITHER/2)` added pre-quantize (dither RNG stream), 0–2 LSB "grit ↔ hiss" morph.
  - Dynamic depth (deep-dive §1.3, Digitalis framing): **per-sample** one-pole follower (attack ~5 ms, release ~120 ms; block-rate followers break offline-bounce invariance — `pattern_block_rate_envelope_breaks_blocksize_invariance`), env → dB → normalized t; `bitsNow = jmap(polarity, …)` between CRUSH_BITS and a floor of 1.0, scaled by |CRUSH_ENV_AMT|; sign = duck (−, tails crush) / pump (+, transients splatter).
  - Follower NaN hygiene: input to follower sanitized (`std::isfinite` guard resets to 0) — sticky-NaN state trap (`pattern_envelope_follower_state_sticky_nan`, QUAL-01).

### Global Mix / Enables
- **JUCE Class:** `juce::dsp::DryWetMixer` (verified `juce_dsp/processors/juce_DryWetMixer.h`)
- **Purpose:** Latency-compensated global dry/wet (FUNC-06)
- **Parameters Affected:** MIX, all *_ENABLE
- **Configuration:**
  - `DryWetMixer::setWetLatency(compLatencySamples)` aligns dry with the constant-latency wet path.
  - Every *_ENABLE toggles through a ~10 ms gain fade (no hard engage clicks); transport families additionally only take effect at the next clock tick (state machine consults enables at roll time).
  - All-families-off ⇒ engine passes through the ring at rate 1.0 with unity stages ⇒ bit-transparent minus reported latency (FUNC-02 acceptance).

---

## Processing Chain

```
                          ┌────────────────────────────────────────────────┐
 Input (stereo) ──┬──────▶│ DryWetMixer.pushDrySamples (dry, latency-comp) │◀── MIX
                  │       └────────────────────────────────────────────────┘
                  ▼
        CaptureRing write (per-channel, every sample, rate 1.0)
                  ▼
   ┌──────────────────────────────────────────────────────────────┐
   │ MediaPlayer: per-channel read heads                          │
   │   state ∈ {NORMAL, TAPE_BEND, TAPE_STOP, CD_CONCEAL,         │◀── MediaClock tick
   │            CD_MUTE, CD_LOOP, VINYL_JUMP, VINYL_LOCKED}       │◀── TAPE_*, CD_*, VINYL_*
   │   re-rolled per tick by arbitration (enabled families only); │◀── HARD_EDGES
   │   all jumps crossfaded 1–5 ms unless hardEdges               │
   └──────────────────────────────────────────────────────────────┘
                  ▼
        PacketLossStage (20 ms grid, GE Markov, concealment) ◀── PACKET_*
                  ▼
        CodecStage (mono→BP→8k→μ-law|GSM→up→blend) ◀── CODEC_*   [both sub-paths delay-aligned]
                  ▼
        CrushStage (fractional-hold SRR + jitter) ◀── CRUSH_RATE, CRUSH_JITTER
                  ▼
        QuantStage (env follower → fractional bits + TPDF) ◀── CRUSH_BITS, CRUSH_ENV_AMT, CRUSH_DITHER
                  ▼
        Compensation delay trim (equalize path latency to kCompLatency)
                  ▼
        DryWetMixer.mixWetSamples ◀── MIX
                  ▼
 Output (stereo)
```

**Routing notes:**
- Transport failures (tape/CD/vinyl) are **read-head behaviors** on the shared ring — mutually
  exclusive states, arbitrated per tick. Packet/Codec/Crush are **serial post stages**, continuous
  while enabled. This is the RSBrokenMedia lesson (§4.2): one buffer + many cheap read-head
  behaviors beats parallel chains.
- No feedback loops anywhere. No sidechain.
- Stereo: both channels share one state machine (glitches hit both channels together — broken
  *player*, not broken channels); read heads are per-channel only to keep independent interpolation
  state.

---

## System Architecture

### File I/O System
Not applicable — no file I/O.

### Multi-Output Routing
Not applicable — standard stereo in/out:
```cpp
BusesProperties()
    .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
    .withOutput("Output", juce::AudioChannelSet::stereo(), true)
```
(Constructor, not prepareToPlay — juce8-critical-patterns §4.) Mono-in/mono-out also accepted via
`isBusesLayoutSupported` (matched in/out, mono or stereo).

### MIDI Routing
Not applicable — no MIDI input.

### Third-Party Library Vendoring (libgsm)
- **What:** GSM 06.10 full-rate codec, `third_party/libgsm/` inside the plugin folder (add via
  `add_subdirectory` or direct source-list compile — ~20 small C files, no dependencies).
- **License:** TU-Berlin permissive ("as-is", MIT-style). **Gate:** copy the license file verbatim
  into `third_party/libgsm/COPYRIGHT` and record it in the plugin NOTES before first commit of
  vendored code. Compatible with repo AGPL-3.0 (permissive → AGPL is fine).
- **RT safety:** `gsm_create()` allocates in `prepareToPlay`; `gsm_encode`/`gsm_decode` are
  allocation-free, fixed-work per 160-sample frame. `gsm_destroy()` in destructor/`releaseResources`.
- **GPL firewall:** RSBrokenMedia (GPL-3.0) is *patterns-only* reference. libgsm itself is
  independently vendored from its upstream, not copied out of RSBrokenMedia's tree.

### State Persistence
- **What is saved:** all 31 APVTS parameters (SEED is an `AudioParameterInt` 0–9999, so
  seed round-trips through save/restore automatically — FUNC-04 acceptance).
- **No custom ValueTree state needed** for v1.0. Reseed dice is a UI action writing SEED; not
  itself a parameter.
- **JUCE classes:** `juce::AudioProcessorValueTreeState`; standard
  `getStateInformation`/`setStateInformation` XML round trip.
- **Versioned ParameterIDs** (`juce::ParameterID{"TAPE_PROB", 1}`) per suite convention.

---

## Parameter Mapping

| Parameter ID | Type | Range | Default | DSP Component | Usage |
|---|---|---|---|---|---|
| CLOCK_MODE | Choice | Sync/Free | Sync | MediaClock | Selects tick source |
| CLOCK_SYNC_DIV | Choice | 1/16…1 bar (7) | 1/8 | MediaClock | Subdivision PPQ quantum |
| CLOCK_FREE_RATE | Float | 0.1–20 Hz (exp skew) | 2.0 | MediaClock | Free-run tick rate |
| SEED | Int | 0–9999 | 0 | RngBank | Derives all stream seeds; reseeds on change |
| HARD_EDGES | Bool | Off/On | Off | ReadHead/Packet | Bypass 1–5 ms jump/packet crossfades |
| MIX | Float | 0–100 % | 100 | DryWetMixer | Wet proportion 0–1 |
| TAPE_ENABLE | Bool | Off/On | On | TapeTransport | Family in arbitration + fade |
| TAPE_PROB | Float | 0–100 % | 25 | Arbitration | P(tape event) per tick |
| TAPE_STOP_PROB | Float | 0–100 % | 10 | TapeTransport | Share of tape events that are stops |
| TAPE_RAMP | Float | 20–500 ms | 150 | TapeTransport | Rate ramp time |
| CD_ENABLE | Bool | Off/On | On | CDSkip | Family in arbitration + fade |
| CD_PROB | Float | 0–100 % | 25 | Arbitration | P(cd event) per tick |
| CD_SEVERITY | Float | 0–1 | 0.5 | CDSkip | Ladder rung weighting (conceal→mute→loop) |
| CD_SEGMENT | Float | 10–400 ms | 100 | CDSkip | Loop segment length |
| VINYL_ENABLE | Bool | Off/On | On | VinylTransport | Family in arbitration + fade |
| VINYL_PROB | Float | 0–100 % | 25 | Arbitration | P(vinyl event) per tick |
| VINYL_RPM | Choice | 33⅓/45 | 33⅓ | VinylTransport | Revolution quantum 1.8 s / 1.333 s |
| VINYL_POP | Float | 0–100 % | 50 | VinylTransport | Pop synth level |
| PACKET_ENABLE | Bool | Off/On | Off | PacketLossStage | Stage engage + fade |
| PACKET_LOSS | Float | 0–100 % | 20 | PacketLossStage | Stationary loss occupancy π_B |
| PACKET_BURST | Float | 0–100 % | 30 | PacketLossStage | E[burst len] 1–8 packets → P(B→G) |
| PACKET_CONCEAL | Choice | Silence/Repeat/Decay/Substitute | Decay | PacketLossStage | Concealment algorithm |
| CODEC_ENABLE | Bool | Off/On | Off | CodecStage | Stage engage + fade |
| CODEC_MODE | Choice | μ-law/GSM | μ-law | CodecStage | Landline vs cellphone path |
| CODEC_MIX | Float | 0–100 % | 100 | CodecStage | Codec chain blend (pre-codec vs post) |
| CRUSH_ENABLE | Bool | Off/On | Off | Crush/QuantStage | Stage engage + fade |
| CRUSH_BITS | Float | 1–16 bits | 16 | QuantStage | Fractional bit depth (target-smoothed) |
| CRUSH_RATE | Float | 500 Hz–20 kHz (exp) | 20 kHz | CrushStage | SRR target (clamped fs/2; max = clean) |
| CRUSH_JITTER | Float | 0–100 % | 0 | CrushStage | S&H clock jitter depth |
| CRUSH_ENV_AMT | Float | −100…+100 % | 0 | QuantStage | Env→bits amount; sign = duck/pump |
| CRUSH_DITHER | Float | 0–2 LSB | 0 | QuantStage | TPDF dither amplitude |

Notes: draft said "CRUSH_RATE 500 Hz–fs" — parameter ranges must be fs-independent for automation
portability, so the spec fixes 500 Hz–20 kHz with runtime clamp to fs/2; at max the latch runs every
sample = transparent. CLOCK_FREE_RATE and CRUSH_RATE use exponential skew (wide ranges); all other
floats linear.

---

## Algorithm Details

### Clock arbitration (per tick)
1. Read enables + probabilities (atomic APVTS reads).
2. Roll each enabled transport family against its probability using **its own stream** in fixed
   order (tape, cd, vinyl) — order fixed so identical seeds give identical rolls.
3. If several fire, the `arbitration` stream picks uniformly among the firers.
4. Winner installs its state (duration = until next tick, re-rolled each tick; a family already
   mid-event may extend or release). No firer ⇒ ramp back toward NORMAL (tape rate ramps to 1.0,
   loops release at segment end, etc.).
5. Tick boundaries land at exact sample offsets inside the block (split-block processing).

### Tape (DSP-01)
- Bend: pick interval from `{1.0, 0.67, 1.5, 0.5, 2.0}` (tape stream), ramp read rate to it over
  TAPE_RAMP ms (linear line generator, RSBrokenMedia pattern), optionally back next tick.
- Stop: with probability TAPE_STOP_PROB per tape event, ramp rate → 0 and hold; recovery ramps back
  to 1.0. Instantaneous-frequency probe must show continuous ramps (acceptance).
- The read head naturally falls behind/ahead of the write head during bends; position drift is
  bounded by the ring margin logic (clamp + gentle re-approach in NORMAL state at ≤ ±2 % rate trim,
  itself ramped — never a hidden jump).

### CD skip (DSP-02)
- Ladder per component description. Chirp synth: `sin(2π·f(t)·t)·exp(−t/τ)` with f sweeping
  ~3→8 kHz over ~4 ms, τ ≈ 1.5 ms; tick: single-sample impulse through 4 kHz BPF, both scaled to
  ~−18 dBFS pre-mix. Loop repeats at exact CD_SEGMENT intervals (sector-flavored machine-gun);
  recovery jump forward toward `writeAbs − minLag`.

### Vinyl (DSP-03)
- Jump: `readAbs −= revolutionSamples` (or toward write head, clamped) at tick; rate stays 1.0.
- Locked groove: state persists across ticks while re-rolled; each wrap re-jumps exactly one
  revolution and fires a pop.
- Pop synth: ±impulse pair (few samples) through ~1–3 kHz LPF, amplitude from VINYL_POP with ±3 dB
  random variation (artifactSynth stream).

### Packet loss (DSP-04)
- GE parameter mapping (given π_B from PACKET_LOSS scaled 0–0.6, E[B] from PACKET_BURST 1–8):
  `p_BG = 1/E[B]`, `p_GB = clamp(π_B·p_BG/(1−π_B), 0, 1)`. Loss decision per packet: in Bad state
  lose with 0.5, in Good with 0.01 (packet stream). Burst-length distribution vs geometric
  expectation is the acceptance probe.
- Substitute concealment: estimate period T from last good packet (min-AMDF over 2–15 ms lags);
  replay last T samples cyclically with slight −1 dB per cycle decay; fall back to Decay mode when
  no periodicity found (AMDF minimum not < 0.5× mean).

### Codec (DSP-05)
- See CodecStage component. μ-law noise floor breathes with level (level-dependent quantization —
  acceptance); bandwidth probe ~300–3400 Hz.
- 8 kHz grid: one fractional-hold latch at fixed `rate = 8000/fs` (shares CrushStage latch code);
  GSM frames assembled from consecutive latch outputs (exactly 160 per frame at the 8 kHz grid).

### Crush (DSP-06/07/08)
- Per component descriptions; formulas from deep-dive §1.1/§1.3/§2.1. Full-range sweeps must pass a
  **liveness-gated** zipper probe (verify the param moves the DSP before asserting no zipper —
  `pattern_zipper_sweep_probe_needs_liveness_gate`).

---

## Integration Points

### Feature Dependencies
- MediaClock → arbitration → transport families: transport events exist only at ticks (FUNC-01).
- RngBank → everything stochastic: all streams derive from SEED; seed change resets streams.
- CaptureRing → all transport families: shared ring; families are read-head *behaviors*, not chains.
- CrushStage latch ↔ CodecStage 8 kHz grid: same fractional-hold primitive, two instances with
  independent state (never shared state — different rates).
- DryWetMixer ↔ latency scheme: dry path must be delayed by the same constant as the wet path.
- Enable params → arbitration set: a disabled family can never win a roll; disabling mid-event
  releases the event gracefully (ramp/loop-end), not instantly.

### Parameter Interactions
- TAPE_PROB + CD_PROB + VINYL_PROB: not normalized — each rolls independently; more enabled
  families with high probabilities ⇒ more arbitration collisions ⇒ uniform pick. Documented so the
  UI/manual can explain "everything at 100 % ≠ everything at once".
- CD_SEVERITY × CD_SEGMENT: segment length only matters on rung 3 (loop); UI may gray it below
  severity ≈ 0.5 (Stage 3 decision).
- PACKET_LOSS × PACKET_BURST: jointly determine (p_GB, p_BG); extreme corner (loss 100 %, burst 0 %)
  clamps to valid Markov probabilities.
- CRUSH_ENV_AMT × CRUSH_BITS: env modulates *downward* from CRUSH_BITS toward 1 bit; at
  CRUSH_BITS = 16 and amt = 0 the quantizer is transparent.
- HARD_EDGES affects ReadHead jumps and packet boundaries; it does **not** affect tape ramps
  (ramps are the sound, not a safety) or enable fades.
- MIX at 0 % must be bit-transparent dry (DryWetMixer handles latency alignment).

### Processing Order Requirements
1. Push dry into DryWetMixer (before any mutation).
2. Write input into CaptureRing (before reads — blockSize invariance).
3. Advance MediaClock; split block at tick offsets; arbitrate at ticks.
4. Read heads render transport output (with jump crossfades).
5. PacketLossStage (operates on rendered transport audio — packets conceal *what you hear*).
6. CodecStage (phone chain colors the possibly-glitched signal — a broken CD down a phone line).
7. CrushStage → QuantStage last ("output converter" position, Decimort AD/DA framing).
8. Latency trim + mixWetSamples.

Order rationale: MediaPlayer must precede packet/codec/crush because those emulate the *transmission
and conversion* stages after the failing player; swapping would re-glitch codec artifacts, which is
not the physical model and makes concealment inaudible.

### Thread Boundaries
- **Audio thread:** everything DSP, including libgsm encode/decode (fixed-work, allocation-free) and
  all artifact synthesis. Parameter reads via cached `std::atomic<float>*` raw pointers
  (`getRawParameterValue`, suite convention). No allocations, locks, logging (PERF-01).
- **Message thread:** UI, reseed dice (writes SEED via `setValueNotifyingHost`), preset load.
- **No background thread.**
- Seed change detection on audio thread: compare cached last-seed each block; on change, reseed all
  streams at block start (deterministic, allocation-free).

---

## Implementation Risks

### GSM codec vendoring (libgsm)
**Complexity:** MEDIUM · **Risk:** MEDIUM-HIGH
- Risk factors: CMake integration of C89 sources into the plugin target (warnings-as-errors
  interactions); license file verification; frame-buffer bookkeeping across block boundaries;
  Windows build (MSVC C quirks).
- Alternatives: (1) μ-law only at 6-bit for a "cell" flavor fake; (2) STFT codec fake (rejected —
  spectral territory assigned to O-Lossy, out of scope).
- **Fallback architecture:** ship CODEC_MODE with μ-law only (choice param keeps both entries but
  GSM maps to a μ-law+extra-decimation approximation) and move real GSM to v1.1.
  ⚠ `AudioParameterChoice` must always keep ≥ 2 choices (`critical_choice_param_needs_two_choices`).
- Mitigation: vendor early in Stage 2 phase for codec; compile libgsm as its own static lib target
  with relaxed warnings; harness-render a GSM frame round trip before integrating.

### Constant-latency compensation scheme
**Complexity:** MEDIUM · **Risk:** MEDIUM
- Plan: report **one constant latency** = one codec frame (20 ms → `ceil(0.020·fs)` samples) via
  `setLatencySamples()` in `prepareToPlay()` (non-virtual getter in JUCE 8 — memory pattern), and
  delay-align all non-GSM paths through the ReadHead's NORMAL-state lag / a trim delay so every
  path has identical latency. Benefits: no dynamic latency renegotiation on CODEC_MODE flips, FUNC-02
  "bit-transparent minus reported latency" holds for every mode.
- Risk factors: subtle off-by-frames between the 8 kHz grid and host-fs sample counts; passthrough
  transparency probe must be exact.
- Alternative: dynamic `setLatencySamples` on mode change + `updateHostDisplay()` (hosts vary in
  handling); zero-latency with un-compensated GSM smear (breaks FUNC-02 probe).
- **Fallback:** if constant 20 ms is judged too expensive for the all-off case, report 0 and accept
  that GSM mode is internally delayed (document in NOTES; FUNC-02 probe then excludes GSM).

### Block-size invariance of the clocked state machine (QUAL-02)
**Complexity:** MEDIUM · **Risk:** MEDIUM
- Risk factors: ticks not applied at exact sample offsets; per-block parameter reads that gate
  events; RNG consumption order varying with block size.
- Mitigation: split-block processing at tick offsets; consume RNG only at ticks/packets (never
  per-block); per-subsystem streams; 512-vs-4096 bit-identity harness probe from day one
  (`pattern_grain_read_before_capture_write_blocksize`, `pattern_rng_stream_interleave_blocksize`).
- Fallback: none needed — this is a discipline, not an algorithm.

### Waveform-substitution concealment
**Complexity:** MEDIUM · **Risk:** MEDIUM
- Period estimation can fail on noise/polyphony → buzzy garbage.
- Fallback (built-in): auto-degrade to Decay mode when AMDF confidence low. Worst case: ship 3
  concealment modes + Substitute aliased to Decay (DSP-04 "4 audibly distinct modes" then needs
  re-scoping — flag at verify).

### Read-head / write-head collision management
**Complexity:** MEDIUM · **Risk:** MEDIUM
- Tape bends at 2.0× rate and vinyl back-jumps move the read head toward ring limits; clamping must
  never teleport without a crossfade.
- Mitigation: single `clampAndScheduleJump()` choke point for all position changes; margin
  constants asserted against ring span; NORMAL-state gentle re-approach trim.

### CD severity continuous ladder
**Complexity:** LOW · **Risk:** LOW — weighting map is tunable; worst case it becomes a 3-zone
discrete mapping (still satisfies DSP-02).

### Tape/vinyl/CD artifact synthesis (pops/chirps/ticks)
**Complexity:** LOW · **Risk:** LOW — simple enveloped impulses/sines through TPT filters; levels
tuned in harness renders.

### Overall Project Risk
**Overall complexity:** HIGH (many MEDIUM pieces, no single algorithmic monster — no FFT, no
pitch-shifting, no feedback).
**Highest risk:** libgsm vendoring + the latency scheme (they interact) — ~40 % of project risk.
**Recommended approach:** build MediaPlayer core + tape first (validates ring/clock/crossfade/RNG
infrastructure everything else rides on), transport families next, then crush (pure math, easy
wins), codec last (isolates the vendoring risk behind a proven engine). Phasing in ROADMAP.md.

---

## Architecture Decisions

### One shared circular buffer + read-head behaviors (vs parallel effect chains)
**Decision:** All transport failures are behaviors of read heads over one ring (RSBrokenMedia §4.2).
**Rationale:** Failures are mutually exclusive states of one "playback machine" — that's the
physical model and the sound; parallel chains would double memory, blur the metaphor, and require a
mixer that has no physical meaning.
**Alternatives:** per-family chains (rejected: cost, incoherent metaphor); granular engine à la
O-ReverseDelay (rejected: overkill — no overlapping grains needed; varispeed read loop suffices).
**Tradeoff:** families can't fire simultaneously — accepted; arbitration collisions documented.

### Per-module explicit controls (vs RSBrokenMedia macro dice)
**Decision:** From ideation (FUNC-05). Each family: enable + params; no Analog/Digital/Distortion
macro knobs. Locked.

### Packet loss on its own 20 ms grid (not the MediaClock)
**Decision:** GE Markov advances per packet, independent of the re-roll clock.
**Rationale:** Burstiness statistics (DSP-04 acceptance: geometric burst lengths) only hold on the
packet grid; quantizing loss to a musical clock would destroy the network-realism that defines the
family. PACKET_ENABLE still fades in/out and the family is not part of transport arbitration.
**Tradeoff:** packet events aren't rhythmic — intentional; that contrast is the aesthetic.

### Constant reported latency (one GSM frame) with all paths aligned
**Decision:** See Implementation Risks. Chosen over dynamic latency because CODEC_MODE is
automatable and hosts handle mid-play latency changes inconsistently.

### Crush last in chain (Decimort AD/DA framing)
**Decision:** MediaPlayer → Packet → Codec → Crush.
**Rationale:** Crush emulates the output converter; codec emulates transmission; packet conceals
received audio; the player fails first. Physical-chain order = the reference products' order.

### Custom DSP with JUCE utility classes (vs juce::dsp processor chain)
**Decision:** Read heads, latches, quantizers, GE Markov custom; JUCE supplies `DryWetMixer`,
`IIR::Filter` + `ArrayCoefficients`, `FirstOrderTPTFilter`, `SmoothedValue`, `Random`.
**Rationale:** No JUCE class exists for any of the core behaviors (verified against local source
tree — nothing in `juce_dsp` covers variable-rate ring readers, companders, or Markov loss);
utilities cover the commodity pieces. This matches every shipped suite plugin.

### Seed as an automatable Int parameter
**Decision:** SEED is `AudioParameterInt` 0–9999; reseed dice writes it from the UI.
**Rationale:** Free state persistence + host-recallable renders; deterministic bounce is the
selling point (FUNC-04). Reseed itself is not a parameter (no trigger params in APVTS).

---

## Special Considerations

### Thread Safety
- Cached `std::atomic<float>*` for all 31 params (suite convention, O-Polystutter
  `PluginProcessor.cpp:734–792` pattern).
- No shared mutable state across threads beyond APVTS; no locks anywhere.

### Performance
- Estimated: ReadHead render ~5 %, packet stage ~1 %, codec (GSM) ~3–5 %, crush+quant ~3 %,
  filters ~2 % → **≈ 10–15 % single core @ 48 kHz worst case** (all families on, GSM). Cheap by
  design — no FFT, no oversampling.
- Memory: ring 2 × 2.5 s + packet history + GSM state ≈ < 5 MB at 192 kHz.

### Denormal Protection
- `juce::ScopedNoDenormals` in `processBlock()`.
- Envelope follower floor at −60 dB equivalent; tape-stop holds exact 0.0 rate (held sample decays
  to a constant — no denormal generation); IIR filters flushed by ScopedNoDenormals.
- NaN hygiene: sanitize follower input; IIR coefficient updates only in prepareToPlay (fixed
  cutoffs) except the CD-conceal TPT sweep (TPT structure is unconditionally stable for cutoff
  sweeps).

### Sample Rate Handling
- Ring, packet buffers, crossfade lengths, ramp coefficients, revolution quanta, comp latency all
  recomputed in `prepareToPlay()`.
- GSM/μ-law grid is **fixed 8 kHz regardless of fs** (`rate = 8000/fs` latch); GSM frames always
  160 grid samples.
- `static_assert(kRingSeconds >= kMaxRevolutionSeconds + kMaxRampSeconds + kSafetySeconds)` on the
  compile-time constants (FUNC-01 acceptance).

### Latency
- Reported once: `setLatencySamples((int)std::ceil(0.020 * sampleRate))` in `prepareToPlay()`
  (JUCE 8: getter non-virtual — always report via setter; memory pattern).
- Wet path aligned to exactly that figure in every mode; `DryWetMixer::setWetLatency` aligns dry.

---

## Research References

### Professional Plugins / Products
1. **XLN Audio RC-20 Retro Color** — six-module degradation suite (noise/wobble/distort/digital/
   space/magnetic); validates the per-module-panels product shape O-Bitrot uses. ([xlnaudio.com](https://www.xlnaudio.com/products/addictive_fx/effect/rc-20_retro_color))
2. **iZotope Vinyl** — mechanical/electrical noise, wear, warp, spin-down; reference for vinyl
   artifact taxonomy. ([izotope via loopcloud roundup](https://www.loopcloud.com/cloud/blog/4437-The-best-11-lofi-effects-plugins-vinyl-tape-VHS-and-more-in-your-DAW))
3. **D16 Decimort 2** — full AD/DA-path SRR architecture: pre-filter, S&H + jitter,
   mid-tread/riser, dither, images post-filter (deep-dive §2.2; crush-stage spec source).
4. **Aberrant DSP Digitalis** — envelope-driven dynamic bitcrush, duck/pump framing (deep-dive §1.3).
5. **Airwindows DeRez/DeRez2** (MIT) — fractional-crossing interpolated hold, per-sample smoothed
   targets, μ-law wrap; MIT ⇒ safe to adapt code.
6. **RSBrokenMedia** (reillypascal, GPL-3.0) — clocked stochastic architecture, CD-skip machinery,
   GSM chain. **Patterns only; no code copying** (GPL→AGPL one-way needs verification; firewall
   documented above).

### JUCE Documentation (verified against local 8.0.14 source)
- `juce::dsp::DryWetMixer` — latency-compensated dry/wet (`setWetLatency`).
- `juce::dsp::IIR::Filter` + `ArrayCoefficients::makeHighPass/LowPass(sampleRate, freq, Q)` —
  RT-safe 6-raw-coefficient form (`juce_IIRFilter.h:45–72`).
- `juce::dsp::FirstOrderTPTFilter` — sweep-stable concealment/pop LPFs.
- `juce::SmoothedValue` — target smoothing for rate/bits.
- `juce::Random::setSeed(int64)` — per-stream deterministic RNG.
- `juce::AudioPlayHead::PositionInfo` — `getBpm()/getPpqPosition()` Optionals with fallbacks.
- Modules required: `juce_audio_processors`, `juce_audio_basics`, `juce_dsp` (link in CMake),
  `juce_gui_extra` at Stage 3 (WebView).

### Repo / Technical Resources
- `research/glitch-effects/degradation-dsp-deep-dive.md` — all formulas (§1 quantize/dither/dynamic,
  §2 SRR/anti-zipper, §3 codec/GE packets, §4 vinyl/CD/RSBrokenMedia, §6 module decomposition).
- `research/glitch-effects/multi-effect-sequencer-reuse-audit.md` — reuse map: O-ReverseDelay
  `CaptureBuffer.h` (absolute-index ring), O-Polystutter `RepeatLane.cpp` (varispeed read, anti-click
  crossfade stack), `PluginProcessor.cpp:1691` (updateBeatSync).
- musicdsp.org #124 decimator; Lipshitz/Wannamaker/Vanderkooy nonsubtractive dither; Gilbert (1960)/
  Elliott (1963) burst-loss channel model.

---

## Notes
- Draft→spec changes made here: CRUSH_RATE range fixed to 500 Hz–20 kHz (fs-independent);
  CLOCK_SYNC_DIV enumerated (7 divisions); GE probability mapping formalized. Carry these into
  `parameter-spec.md` at mockup finalization (mockup remains source of truth for UI).
- No UI mockup exists yet — no design-sync conflicts to record. The six-panel + global-strip layout
  in the BRIEF matches this architecture 1:1 (one panel per family, enable + params per panel).
- Out of scope confirmations (REQUIREMENTS): no STFT/MP3 codec mode (O-Lossy territory), no
  bit-flip PCM corruption, no freeze/latch, no macro dice.
