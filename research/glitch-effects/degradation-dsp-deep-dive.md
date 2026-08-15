---
title: "Digital Audio Degradation Effects — Deep Technical Dive"
created: 2026-08-14
last_verified: 2026-08-15
juce_version: "8.0.14"
summary: "Bitcrush, sample-rate reduction, codec/telecom artifact emulation, and broken-media effects: formulas, anti-zipper techniques, and open-source reference implementations for JUCE 8. Primary DSP source for O-Bitrot."
domain: dsp
type: algorithm
keywords:
  - bitcrusher
  - sample-rate-reduction
  - decimator
  - mu-law
  - gsm
  - packet-loss
  - cd-skip
  - vinyl
  - bitrot
  - lo-fi
  - dither
stages: [0, 2]
agents: [dsp, research]
---

# Digital Audio Degradation Effects — Deep Technical Dive

Level 3 research deliverable (2026-08-14). All formulas assume float samples in [-1, 1], `fs` = host sample rate. Primary target: an "O-Bitrot" degradation plugin and degradation modules for O-Glitch.

## 1. Bit-Depth Reduction

### 1.1 Core quantization

For `b` bits, step `Δ = 2^(1-b)` (full-scale ±1):

```cpp
// Mid-tread: level AT zero — silence stays silent. (Real converters.)
float midTread(float x, float delta) { return delta * std::floor(x / delta + 0.5f); }
// Mid-riser: zero BETWEEN levels — silence becomes a ±Δ/2 buzzing LSB floor. Nastier.
float midRiser(float x, float delta) { return delta * (std::floor(x / delta) + 0.5f); }
```

D16 Decimort 2 exposes both as a user toggle — good precedent. Fractional bit depths (`delta = 2.0f * exp2(-bits)` with float `bits`) sweep continuously — essential for automatable crush. Airwindows DeRez2 smooths the *step size itself* per-sample (`incrementB = ((incrementB*999.0)+targetB)/1000.0`) so the knob never zippers.

Variants: musicdsp #124 truncates via integer cast (asymmetric flavor); RSBrokenMedia quantizes by remainder subtraction with `powf(2.0f, bitDepth)` supporting fractional depths.

### 1.2 Dither

TPDF (Lipshitz/Wannamaker/Vanderkooy): dither of 2 LSB peak-to-peak decouples error moments from signal.

```cpp
float tpdf = (rand01() - rand01()) * delta;
float out  = midTread(x + tpdf, delta);
// Cheaper HP-shaped TPDF (one RNG/sample, Earlevel via KVR t=408629):
float r = rand01() * delta;  out = midTread(x + r - prevR, delta);  prevR = r;
// 1st-order noise shaping (error feedback):
float in = x + errState;  out = midTread(in, delta);  errState = in - out;
```

Design fork to expose: dithered = clean raised noise floor ("8-bit sampler hiss"); undithered = correlated harmonic grunge + gating. Default dither OFF for a crusher; offer Dither 0→2 LSB as a "grit ↔ hiss" morph. Shaping only matters at 8–12 bit emulation; below ~6 bits it becomes audible modulation.

### 1.3 Dynamic bitcrushing (Digitalis-style)

Envelope follower drives effective fractional bit depth ("bitcrusher with integrated compressor", down to 1 bit):

```cpp
// per-SAMPLE one-pole (block-rate followers break offline-bounce invariance)
float rect = std::abs(x);
env += (rect > env ? attackCoef : releaseCoef) * (rect - env);
float envDb   = juce::Decibels::gainToDecibels(env, -60.0f);
float t       = juce::jmap(envDb, -60.0f, 0.0f, 0.0f, 1.0f);
float bitsNow = juce::jmap(polarity ? t : 1.0f - t, bitsMin, bitsMax); // fractional
float delta   = 2.0f * std::exp2(-bitsNow);   // continuous → zipper-free by construction
```

Two polarities: "duck" (loud clean, quiet crushed — crush blooms in tails, the classic Digitalis move) and "pump" (transients splatter). A threshold+ratio UI reproduces Digitalis' compressor framing.

### 1.4 μ-law / A-law companding as a crush flavor

```cpp
float muComp(float x)   { return std::copysign(std::log1p(255.0f*std::abs(x)) / std::log1p(255.0f), x); }
float muExpand(float y) { return std::copysign((std::pow(256.0f, std::abs(y)) - 1.0f) / 255.0f, y); }
```

Compress → quantize 8-bit → expand = level-dependent quantization noise that breathes with the signal — the actual telephone/answering-machine sound, unreachable with linear crushing. RSBrokenMedia uses the real segmented G.711 table codec plus Butterworth pre/post filters at `(fs/downsampling) * 0.4`. DeRez2 wraps its crusher in μ-law for an "analog" curve.

## 2. Sample-Rate Reduction

### 2.1 Fractional hold (the key quality trick)

```cpp
phase += rate;                    // rate = newFs / fs — supports non-integer factors
if (phase >= 1.0f) { phase -= 1.0f; held = x; }   // naive latch
out = held;
```

Naive latching snaps to the host-rate grid → up to 1 sample of hold-period jitter → warble at low rates. **Airwindows DeRez fix**: interpolate the held value at the exact fractional crossing:

```cpp
heldSample = (lastSample * position) + (inputSample * (1.0 - position));
// plus 'soften': out = out*(1-soften) + held*soften  — rounds the stair edges
```

Kills periodicity error; makes rate fully sweepable ("the only ANALOG bitcrusher").

### 2.2 Anti-aliasing as a feature

Proper decimation = filter then downsample. For degradation, aliasing is the point — make AA a knob:

- **Pre-filter** tracking `~0.4 × newFs` (4–8 pole Butterworth; `juce::dsp::IIR::ArrayCoefficients`, RT-safe per repo pattern). Recompute at control rate; crossfade two instances over ~10 ms for big jumps.
- **Post "images" filter** at `newFs/2` suppressing ZOH stair images — Decimort 2's exact switchable architecture (it simulates the whole AD/DA path: input filter → S&H **with jitter** → quantizer → output filter). Decimort's Jitter = `phase += rate * (1 + jitterAmt * noise())` → inharmonic sideband hash, cheap and distinctive.
- **"Analog" option**: resonant 2–4 pole pre-filter with Q — the resonant bump at tracking cutoff is much of vintage-sampler tone.

### 2.3 ZOH sinc droop

`|H(f)| = |sinc(f/newFs)|` → −3.92 dB at the new Nyquist, first null at newFs. Authentic "sampler darkness" — don't compensate for vintage; offer inverse-sinc tilt as a "modern/bright" mode.

### 2.4 First-order hold (linear-interp decimation)

Ramp between latch points instead of holding: `sinc²` response, 12 dB/oct image rolloff vs ZOH's 6 — "cheap-converter smooth" vs "harsh digital." Still aliases (KVR t=548568); it's a flavor, not a cure. A "clean" mode wants polyphase windowed-sinc / Farrow (KVR t=583897, or chowdsp resamplers).

### 2.5 Zipper-free sweeping — the rules

1. Smooth the **rate/step target**, never the output (DeRez2's per-sample one-pole toward target).
2. Fractional-crossing interpolation (§2.1).
3. **Never reset the phase accumulator** on parameter change — that's the classic zipper source.
4. Verify with a liveness-gated zipper probe (memory: a no-zipper sweep probe is vacuous if the param is wired to nothing).

## 3. Lossy Codec / Telecom Artifacts

### 3.1 MP3 artifact anatomy

- **Spectral holes**: bit-starved encoders zero whole scale-factor bands.
- **Birdies**: bins near the keep/zero boundary toggle frame-to-frame → metallic chirps.
- **Pre-echo**: transient late in a long window smears quantization noise ~10–25 ms backward.
- **HF swirl**: phase incoherence + intensity-stereo above ~10 kHz.

### 3.2 STFT emulation recipe

```text
STFT (1024, 50–75% overlap; frame size = "codec era" macro: 2048–4096 smeary, 512 tight)
per frame: 1. keep top-N bins by energy (N ∝ bitrate knob) → holes ("loud parts steal all bits")
           2. coarse-quantize surviving magnitudes → graininess
           3. bins within ±ε of threshold: random keep/zero toggle per frame → birdies
           4. optional: quantize phase to few levels above ~8 kHz → swirl
Pre-echo comes free from zeroing in long windows.
```

Cheaper + bit-exact alternative: run a real codec in-line — libgsm (below) is the easy one; LAME/minimp3 round-trip is possible but license/RT complexity favors the STFT fake.

### 3.3 Phone/GSM chain

```text
mono sum → HPF 300 Hz + LPF 3400 Hz (4-pole each) → downsample to 8 kHz (crude filters of §2)
→ μ-law 8-bit round trip (landline) OR real GSM 06.10 (cellphone) → fast ~4:1 comp (AGC)
→ optional crackle/hum bed → packet loss (§3.4)
```

GSM 06.10 via vendored libgsm (tiny, MIT-style; RSBrokenMedia's `GSMProcessor` shows the pattern): 8 kHz, 160-sample/20 ms frames, scale float → `(gsm_signal)(x * 4096) << 3`, encode+decode round trip per frame.

### 3.4 Packet loss — Gilbert–Elliott bursts

```text
Packetize at 20 ms. 2-state Markov: Good (P(loss)≈1%) / Bad (≈50%);
P(G→B) = burstiness, P(B→G) = burst length — losses come in BURSTS like real networks.
Concealment modes (each a distinct sound):
  1. silence (hard dropout)   2. repeat previous packet (robotic "VoIP")
  3. repeat with ~3 dB decay per repetition (real PLC: fades out over ~60 ms)
  4. waveform substitution: continue last pitch period (buzzy frozen formant)
Crossfade 1–5 ms at packet boundaries; expose a "hard edges" toggle for deliberate clicks.
```

### 3.5 Bitrot / corrupted media

- **Bit flips**: reinterpret as 16-bit PCM, XOR random bits (low bits = crackle, sign/high = impulses); rate-limit, post-clip.
- **Wrong-decode stretches**: byte-offset-by-one (white-noise burst at correct envelope — the "corrupt WAV" sound), swapped L/R bytes (metallic), wrong-endian.
- **Sticky sample**: hold one value tens of ms (decoder hang).
- **Buffer shuffling**: random repeat/swap/reverse/drop of 10–200 ms blocks per clock tick (RSBrokenMedia's whole architecture, §4.2).

## 4. Vinyl/CD Skip and Broken Media

### 4.1 Why they sound different

**Vinyl** (physical): pop at the jump, then playback continues exactly ±1 revolution away (1.8 s @33⅓, 1.33 s @45); locked groove = precise one-revolution loop with a pop per pass; pitch never changes. Emulate: revolution-quantized read-pointer jumps + synthesized pop (few-ms LPF'd impulse).

**CD** (informational — CIRC failure escalation ladder, each stage emulatable):
1. full correction (inaudible) → 2. interpolation concealment (momentary dullness: brief LPF dip) → 3. mute (~ms hard silence + residual tick) → 4. buffer loop: the machine-gun stutter — short segment (~1/75 s sector-related up to few hundred ms) repeating at exact intervals, hard-edged, chirpy tick at each restart, then a jump forward on recovery.

Expose both as separate modes.

### 4.2 RSBrokenMedia architecture (the portable pattern)

github.com/reillypascal/RSBrokenMedia (GPL-3.0, JUCE). One continuously-written circular buffer (66150 samples) + per-channel variable-rate read heads. A **clock** fires `receiveClockedPulse()` which **probabilistically re-rolls the machine state** — three macro knobs (Analog/Digital/Distortion FX) are probability-intensity weights on dice rolls per tick.

- Tape speed: ramped line generator × direction; bends from musical-interval table `{1.0, 0.67, 1.5, 0.5, 2.0}`; all transitions through ~150 ms linear ramps (the ramp IS the glide sound and the click-safety). Tape-stop = ramped multiplier to 0.
- CD skip: buffer divided into 8 segments (~100 ms); repeater resets read pos to segment start; chirp synthesized in each segment's first frames.
- Distortion dice: `powf(prob, 3)` curve (subtle until pushed) → bit depth / downsampling / tanh.
- Codec dice: μ-law tables or GSM round trip.

**Lessons**: (1) one shared circular buffer + many cheap read-head behaviors beats separate chains; (2) probability-per-clock-tick as the user parameter makes it feel like failing hardware, not an LFO; (3) ramp every rate change, crossfade every jump, synthesize artifacts (pop/chirp) explicitly; (4) quantize glitch decisions to a clock so chaos stays rhythmic.

## 5. Reference implementations

| Project | License | Value |
|---|---|---|
| RSBrokenMedia (github.com/reillypascal/RSBrokenMedia) | GPL-3.0 | Complete broken-media reference: clocked stochastic architecture, real GSM + μ-law, CD-skip machinery. JUCE. (GPL: read for patterns, don't copy code into AGPL suite without checking compatibility — AGPL-3.0 can incorporate GPL-3.0 one-way; verify.) |
| musicdsp.org #124 Decimator | public | Canonical 10-line fractional-rate S&H + bit reduce |
| Airwindows DeRez / DeRez2 / DeRez4, BitGlitter, CrunchCoat (github.com/airwindows/airwindows) | MIT | Interpolated hold, per-sample smoothed targets, μ-law wrap, soften blend — MIT, safe to adapt |
| Airwindows CrunchyGrooveWear | MIT | Vinyl groove-wear via cascaded slew/derivative-domain averaging — vinyl degradation that isn't noise |
| chowdsp_utils | BSD | Variable-rate resamplers (integer + non-integer) for a clean mode |
| D16 Decimort 2 (docs only) | — | The architecture spec: full AD/DA-path sim — pre-filter, S&H + jitter, mid-tread/riser toggle, dither, images post-filter |

## 6. Suggested module decomposition (JUCE 8)

```
CrushStage   — fractional-hold SRR (interp latch, smoothed rate, jitter) + tracking pre/post AA (ArrayCoefficients, bypassable, resonance)
QuantStage   — mid-tread/riser, fractional bits, TPDF dither knob, 1st/2nd-order shaping, envelope-driven dynamic depth, μ-law/A-law wrap
CodecStage   — 8k downsample + 300–3400 BP + μ-law table or libgsm round trip; STFT hole/birdie processor for MP3 flavor (FFT latency → setLatencySamples)
MediaPlayer  — circular buffer + clocked stochastic read heads: tape bends/stops/reverse (ramped), CD-skip segments + chirp, Gilbert–Elliott packet loss with 4 concealment modes, vinyl mode (revolution-quantized jumps + pops)
```

All rate/depth params continuous, per-sample-smoothed at the target; all position jumps crossfaded 1–5 ms with a "hard edges" bypass.

## Key sources

musicdsp.org/en/latest/Effects/124-decimator.html · github.com/reillypascal/RSBrokenMedia · airwindows.com/derez2 (+ DeRez2Proc.cpp in the airwindows repo) · d16.pl/decimort2 · KVR t=408629 (bandlimited bitcrusher), t=583897 (arbitrary downsampling), t=455406 (S&H vs downsampling), t=548568 (linear-interp aliasing) · dspguru.com decimation FAQ · dspillustrations.com quantization · Lipshitz/Wannamaker/Vanderkooy nonsubtractive dither · gearnews.com + kvraudio.com on Aberrant DSP Digitalis · Wikipedia "Skip (audio playback)", "Zero-order hold" · soundcy.com phone-call recipe · thimeo.com Delossifier docs (artifact taxonomy) · github.com/Chowdhury-DSP/chowdsp_utils
