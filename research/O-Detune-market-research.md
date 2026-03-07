---
title: "O-Detune Market Research"
created: 2026-02-01
last_verified: 2026-02-06
juce_version: "8.0.4"
summary: "Competitive landscape analysis of detuning and pitch thickening plugins, covering Soundtoys MicroShift, Eventide MicroPitch, and identifying market gaps for analog-style wobble with unison detuning."
domain: market-research
type: market-research
keywords:
  - market-research
  - detuning
  - pitch-thickening
  - competitive-analysis
  - product-planning
  - stereo-widening
  - chorus
stages: [0]
agents: [research]
---

# O-Detune Market Research

**Project:** O-Detune - Analog-Style Detuning/Pitch Thickening Plugin
**Researched:** 2026-02-01
**Confidence Level:** HIGH (based on multiple authoritative sources, official product pages, and professional reviews)

---

## Executive Summary

The detuning/pitch thickening plugin market is well-established but fragmented across several use-case categories: stereo widening, tape emulation, vocal processing, and lo-fi effects. Major players like Soundtoys MicroShift and Eventide MicroPitch dominate the "classic micropitch" space, while newer entrants like XLN Audio RC-20 and Goodhertz Wow Control address the lo-fi/tape aesthetic. A significant gap exists for a plugin that combines **analog-style wobble**, **unison detuning**, and **colorful character** in a single, focused tool with excellent mono compatibility.

---

## 1. Popular Detuning Plugins - Competitive Landscape

### 1.1 Soundtoys MicroShift

**Category:** Stereo Widening / Micropitch
**Price:** $99 (frequently on sale for $29-39)
**Format:** VST, VST3, AU, AAX (64-bit)

**Core Features:**
- Three distinct "Styles" emulating classic hardware:
  - Style 1 & 2: Based on Eventide H3000 algorithms (notably preset 519)
  - Style 3: AMS DMX 15-80s emulation
- Key parameters: Mix, Focus, Detune, Delay
- Focus control: Frequency-selective widening to prevent muddiness
- Includes Little MicroShift (simplified version)

**Algorithm Approach:** Delay-based pitch shifting with phase inversion, high-pass filtering, and pitch modulation. For stereo inputs, creates doubling via time-varying delay and detuning. Mono inputs get basic chorus-style processing.

**Strengths:**
- Industry standard for vocal/synth widening
- Hardware emulation captures analog saturation characteristics
- Simple yet powerful interface
- Very low latency (32 samples)

**Weaknesses:**
- No tape wobble/flutter aesthetic
- Limited to micropitch (no large pitch shifts)
- No modulation rate control
- Stereo-only focus (mono input handling less sophisticated)

**Sources:** [Soundtoys Official](https://www.soundtoys.com/product/microshift/), [Sound On Sound Review](https://www.soundonsound.com/reviews/soundtoys-microshift)

---

### 1.2 Eventide MicroPitch

**Category:** Pitch Shifting / Delay
**Price:** $99 (frequently on sale for $29)
**Format:** VST, VST3, AU, AAX; iOS version $9.99

**Core Features:**
- Two voices of pitch shifting (-50 to +50 cents each)
- Up to 2 seconds of delay per voice
- DAW tempo sync with subdivision options
- Modulation (depth and rate controls)
- Feedback control (recirculates delays through pitch shifter)
- RIBBON control for morphing between two complete settings

**Algorithm Approach:** Based on classic H910/H3000 Harmonizer algorithms. Uses fine-resolution pitch shifting with delay, creating stereo spread by panning upshifted/downshifted signals left/right.

**Strengths:**
- Heritage from the original Eventide hardware
- More parameters than MicroShift (modulation depth/rate)
- RIBBON for performance/automation
- Can go from subtle widening to aggressive metallic resonances
- Also available on iOS

**Weaknesses:**
- More complex than necessary for simple widening
- No tape/analog character options
- No saturation/color
- UI can be overwhelming for beginners

**Sources:** [Eventide Official](https://www.eventideaudio.com/plug-ins/micropitch/), [Sound On Sound Review](https://www.soundonsound.com/reviews/eventide-micropitch)

---

### 1.3 Valhalla Space Modulator

**Category:** Modulation / Flanger / Detuner
**Price:** FREE
**Format:** VST, VST3, AU, AAX (32/64-bit)

**Core Features:**
- 11 different modulation algorithms
- Through-zero flanging, barberpole flanging, detuning, doubling
- Parameters: Rate, Depth, Feedback, Mix, Pre-delay
- 70+ presets
- Resizable UI

**Algorithm Approach:** Primarily flanger-based with extensions into detuning territory. Uses various modulation algorithms to create pitch-related effects including Shepard's tone (infinite ascending/descending).

**Strengths:**
- Completely free
- Incredibly versatile (flanging to detuning to reverb-like effects)
- Simple 5-knob interface
- "Thick and deep sounding" per user reviews
- "Gooey modulation" quality

**Weaknesses:**
- Not specifically designed for micropitch/widening
- Can get weird quickly (not always musical)
- No stereo spread-specific controls
- No tape/analog character

**Sources:** [Valhalla DSP Official](https://valhalladsp.com/shop/modulation/valhalla-space-modulator/), [Tape Op Review](https://tapeop.com/reviews/gear/121/spacemodulator-flanger-plug-in/)

---

### 1.4 Infected Mushroom / Polyverse Wider

**Category:** Stereo Widening
**Price:** FREE
**Format:** VST, VST3, AU, AAX

**Core Features:**
- Single-parameter stereo widening (0-200%)
- Mono-to-stereo conversion
- 100% mono-compatible (cancels out when summed to mono)
- Low-end bypass option (Wider 2.0)

**Algorithm Approach:** All-pass and comb filtering algorithm that creates stereo image illusion without phase issues. Completely cancels when summed to mono, leaving original signal intact.

**Strengths:**
- Free and high quality
- Perfect mono compatibility (unique selling point)
- Dead simple interface
- "I would pay for WIDER. It's that good." - user reviews
- No artifacts or distortion

**Weaknesses:**
- No pitch variation/wobble
- No character/color
- Single dimension of control
- Not a "creative" effect - purely utilitarian widening

**Sources:** [Polyverse Official](https://polyversemusic.com/products/wider/), [MusicRadar](https://www.musicradar.com/news/infected-mushrooms-free-wider-plugin-can-increase-stereo-width-by-up-to-200)

---

### 1.5 Goodhertz Wow Control

**Category:** Tape Emulation / Wow & Flutter
**Price:** $129
**Format:** VST, VST3, AU, AAX

**Core Features:**
- Three tape machine modes:
  - 15 IPS: 1960s Ampex 351 character
  - 7.5 IPS: 1970s Teac A-2300S character
  - Cassette: 1980s Aiwa CSD-K330 character
- Wow and Flutter controls with shape customization
- Beat sync option
- Stereo phase control
- Random section for non-uniform modulations
- Analog tape section with Color and Saturation (up to 200%)

**Algorithm Approach:** Models specific tape machines including their noise characteristics (not just added white noise - noise is modulated by signal). Creates "double/triple" sound effects through stereo phase and randomized wow speed.

**Strengths:**
- Most comprehensive tape wow/flutter plugin
- Three distinct era-specific characters
- Random modulation creates non-repeating, authentic wobble
- Can create "two different drummers" stereo effect
- Beautiful Goodhertz UI

**Weaknesses:**
- High price point ($129)
- Focused exclusively on tape - no unison/micropitch
- Can be CPU intensive with all features
- Learning curve for advanced parameters

**Sources:** [Goodhertz Official](https://goodhertz.com/wow-ctrl/), [Tape Op Review](https://tapeop.com/reviews/gear/137/wow-control-plug-in/)

---

### 1.6 Goodhertz Lossy

**Category:** Lo-Fi / Digital Degradation
**Price:** $69
**Format:** VST, VST3, AU, AAX

**Core Features:**
- Bitrate reduction and compression artifacts
- "Dial-Up Disaster" and similar presets
- Jitter and "internet noise"
- 1980s-style digital reverb (pre/post processing)
- Bandpass/bandreject filter

**Algorithm Approach:** Simulates lossy digital audio compression (MP3, streaming) artifacts rather than analog tape. Creates "the year 2001" sound of low bitrate digital audio.

**Strengths:**
- Unique "bad digital" aesthetic (vs. analog tape)
- Used on Grammy-nominated recordings (Phoebe Bridgers)
- Beautiful UI
- Distinct from tape plugins

**Weaknesses:**
- Not pitch-related - purely degradation
- Niche use case
- No stereo widening
- Digital aesthetic may not suit all genres

**Sources:** [Goodhertz Official](https://goodhertz.com/lossy/), [Chase Bliss Pedal](https://www.chasebliss.com/lossy)

---

### 1.7 Soundtoys Little AlterBoy

**Category:** Vocal Processing / Pitch & Formant Shifting
**Price:** $99
**Format:** VST, VST3, AU, AAX

**Core Features:**
- Pitch shifting (+/- 1 octave, semitone or fine)
- Independent formant shifting
- Three modes: Transpose, Quantize (auto-tune style), Robot (monotone)
- Drive control (Decapitator-based saturation)
- MIDI control for "playing" the plugin

**Algorithm Approach:** Real-time pitch and formant shifting based on Soundtoys' heritage from Eventide H3000 and their own PurePitch TDM (first real-time pitch/formant shifter).

**Strengths:**
- Independent pitch and formant = gender swapping, character changes
- Decapitator saturation built-in
- MIDI playable in Robot mode
- Heritage from H3000 developers
- "More natural than any other pitch shift plugin" per reviews

**Weaknesses:**
- Monophonic only (single-voice sources)
- Doesn't work well with stereo
- Not designed for subtle thickening - more dramatic effects
- No stereo widening

**Sources:** [Soundtoys Official](https://www.soundtoys.com/product/little-alterboy/), [Produce Like A Pro Review](https://producelikeapro.com/blog/little-alterboy-review/)

---

### 1.8 XLN Audio RC-20 Retro Color

**Category:** Lo-Fi / Vintage Effects Suite
**Price:** $59 (originally $99)
**Format:** VST, VST3, AU, AAX (64-bit)

**Core Features:**
- Six effect modules:
  - **Noise**: Vinyl crackle, tape hiss
  - **Wobble**: Wow and flutter (slow/fast pitch modulation)
  - **Distort**: Saturation to fuzz
  - **Digital**: Bit reduction, sample rate reduction
  - **Space**: Reverb/ambience
  - **Magnetic**: Tape saturation and frequency loss
- **Flux Engine**: Pseudo-random variations across all modules
- Extensive preset library

**Algorithm Approach:** Multi-effect processor with individual modules. Wobble module creates wow (slow) and flutter (fast) pitch modulation. Flux Engine adds non-repeating randomization for organic feel.

**Strengths:**
- All-in-one lo-fi solution
- Flux Engine creates authentic non-repetitive modulation
- Beautiful, inspiring UI
- Very affordable
- "Wobbly, wonky, fizzy, soul-warming" per Plugin Boutique

**Weaknesses:**
- Jack of all trades, master of none
- Wobble module is one part of larger plugin
- No stereo widening/unison focus
- CPU heavier than single-purpose plugins
- Can be overwhelming (too many options)

**Sources:** [XLN Audio Official](https://www.xlnaudio.com/products/addictive_fx/effect/rc-20_retro_color), [Produce Like A Pro Review](https://producelikeapro.com/blog/rc-20-retro-color-review/)

---

### 1.9 Baby Audio TAIP

**Category:** Tape Emulation / Saturation
**Price:** ~$49 (affordable)
**Format:** VST, VST3, AU, AAX

**Core Features:**
- AI-trained tape emulation (neural network approach)
- Drive: Subtle warmth to heavy distortion
- Glue: Tape compression character
- Wear: Introduces wow/flutter and frequency loss
- Single/Dual mode (one or two tape machines in series)
- Presence: Control high-end rolloff

**Algorithm Approach:** Neural network trained on dry vs. tape-recorded audio comparisons. Creates believable tape saturation through AI rather than traditional DSP component modeling.

**Strengths:**
- Novel AI approach creates convincing tape sound
- Wear + Mix < 100% = tape flanging effect
- Very affordable
- Clean, modern UI
- Can emulate many different tape machines

**Weaknesses:**
- Higher CPU than traditional DSP
- Wear parameter is the only pitch modulation
- No dedicated stereo widening
- Not specifically designed for detuning

**Sources:** [Baby Audio Official](https://babyaud.io/taip-plugin), [Sound On Sound Review](https://www.soundonsound.com/reviews/baby-audio-taip)

---

### 1.10 Denise Audio Bad Tape 2

**Category:** Tape Harm / Creative Destruction
**Price:** ~$49 (often on sale)
**Format:** VST, VST3, AU, AAX

**Core Features:**
- **Detune**: Press-and-hold tape slowdown effect
- **Wobble & Shake**: Random pitch modulation (flutter)
- **Freeze**: Sustain pitch-shift amount when releasing detune
- Additional effects: Phaser, Doubler, Resonator, Magnet

**Algorithm Approach:** Simulates physically manipulating tape (pressing finger on reel). Detune effect is applied while button held, with optional "rewind" on release.

**Strengths:**
- Interactive/performative tape manipulation
- Unique "finger on tape" concept
- Low CPU
- Aggressive lo-fi aesthetic

**Weaknesses:**
- More "sound design" than mixing tool
- Less useful for subtle thickening
- Quirky interface may not suit all workflows
- Limited traditional controls

**Sources:** [Denise Audio Official](https://www.deniseaudio.com/plugins/bad-tape-2), [MusicTech Review](https://musictech.com/reviews/plug-ins/denise-audio-bad-tape/)

---

### 1.11 Other Notable Plugins

| Plugin | Category | Price | Key Differentiator |
|--------|----------|-------|-------------------|
| iZotope Vocal Doubler | Doubling | FREE | XY pad for separation/variation |
| Waves Doubler | Doubling | $29-79 | 4 voice replication, per-voice controls |
| Sonnox VoxDoubler | Vocal Processing | ~$99 | Two plugins: Widen and Thicken |
| JST Sidewidener II | Stereo Width | ~$49 | Mono-focused spatial processor |
| Boz Digital ProVocative | Stereo Width | ~$49 | Frequency-selective pitch widening |
| UAD Brigade Chorus | Chorus | $149+ | BOSS CE-1 emulation |
| Audio Damage Discord4 | Pitch Shifting | $59 | Vintage/Clean/Granular modes |

---

## 2. Feature Analysis Matrix

### Core Algorithms

| Plugin | Delay-Based | Granular | Tape Simulation | Unison | Mono-Compatible |
|--------|-------------|----------|-----------------|--------|-----------------|
| MicroShift | Yes | No | No | No | Partial |
| MicroPitch | Yes | No | No | No | Partial |
| Space Modulator | Yes | No | No | No | Yes |
| Wider | No | No | No | No | **100%** |
| Wow Control | No | No | **Yes** | No | Partial |
| RC-20 | No | No | Yes | No | Unknown |
| Little AlterBoy | No | Yes | No | No | Mono-only |
| TAIP | No | No | Yes | No | Partial |
| Bad Tape 2 | No | No | Yes | No | Unknown |

### Key Parameters Exposed

| Plugin | Detune Amount | Delay | Modulation Rate | Modulation Depth | Saturation | Stereo Width | Focus/Filter |
|--------|---------------|-------|-----------------|------------------|------------|--------------|--------------|
| MicroShift | Yes | Yes | No | No | Built-in | Implicit | Yes |
| MicroPitch | Yes | Yes | Yes | Yes | No | Implicit | No |
| Space Modulator | Yes | Yes | Yes | Yes | No | No | No |
| Wider | N/A | N/A | N/A | N/A | No | Yes | Low-pass |
| Wow Control | N/A | Yes | Yes | Yes | Yes | Yes | No |
| RC-20 | Limited | No | Yes | Yes | Yes | No | No |
| Little AlterBoy | Yes (1 oct) | No | No | No | Yes | No | No |
| TAIP | Via Wear | No | Limited | Limited | Yes | No | No |

### Stereo Handling

| Plugin | Input | Output | Mono Sum Behavior |
|--------|-------|--------|-------------------|
| MicroShift | Mono/Stereo | Stereo | Phase issues at extreme settings |
| MicroPitch | Mono/Stereo | Stereo | Phase issues possible |
| Space Modulator | Mono/Stereo | Stereo | Generally safe |
| Wider | Mono | Stereo | **Perfect cancellation** |
| Wow Control | Mono/Stereo | Stereo | Depends on settings |
| RC-20 | Mono/Stereo | Stereo | Unknown |
| Little AlterBoy | Mono | Mono | N/A |

### Price Positioning

| Plugin | List Price | Sale Price | Value Proposition |
|--------|------------|------------|-------------------|
| MicroShift | $99 | $29-39 | Industry standard micropitch |
| MicroPitch | $99 | $29 | Eventide heritage + RIBBON |
| Space Modulator | **FREE** | N/A | Unbeatable value |
| Wider | **FREE** | N/A | Best free mono-compatible widener |
| Wow Control | $129 | ~$80 | Premium tape simulation |
| RC-20 | $59 | ~$30 | All-in-one lo-fi suite |
| Little AlterBoy | $99 | ~$44 | Vocal character transformation |
| TAIP | $49 | ~$25 | AI tape saturation |

---

## 3. Market Gaps and Opportunities

### 3.1 Identified Gaps

**Gap 1: Combined Tape Wobble + Unison Detuning**
No single plugin offers both authentic tape-style wow/flutter AND clean unison/micropitch detuning. Users must:
- Use MicroShift for widening, THEN add RC-20 or Wow Control for wobble
- Chain multiple plugins, increasing CPU and complexity

**Gap 2: Mono-Compatible Creative Detuning**
Polyverse Wider proves mono-compatible widening is possible and desirable. However:
- Wider has no pitch modulation/wobble
- MicroShift/MicroPitch can have phase issues when summed to mono
- No plugin offers both creative detuning AND guaranteed mono compatibility

**Gap 3: Colorful Character Without Complexity**
- RC-20 and Wow Control have character but overwhelming options
- MicroShift is simple but limited in character
- No plugin offers "colorful" detuning with simple controls

**Gap 4: Visual/Aesthetic Differentiation**
- Most detuning plugins have utilitarian UIs (MicroShift, MicroPitch)
- Lo-fi plugins (RC-20) have distinctive visuals but are multi-effect
- Opportunity for a visually distinctive, single-purpose detuner

**Gap 5: Unison-Style Effect for Non-Synth Sources**
- Synth unison is well-understood (Serum, Vital, Sylenth1)
- Applying unison-style detuning to recorded audio (vocals, guitars, drums) is underserved
- Could replicate the "7-voice supersaw" thickness on any source

### 3.2 Underserved Use Cases

1. **Lo-fi vocals with character**: Users want warm, wobbly vocals without full tape emulation chain
2. **Pad/synth movement without chorus artifacts**: Subtle pitch drift for evolving textures
3. **Mono-safe widening with personality**: Widening that translates to all playback systems while having character
4. **Creative destruction with precision**: Controlled tape "harm" for modern production
5. **Quick "vibe" without preset diving**: Simple controls that sound good immediately

### 3.3 What Would Make O-Detune Unique

**Proposed Unique Value Proposition:**

> "The missing link between MicroShift's widening and RC-20's wobble - a colorful detuning plugin that combines analog tape drift, unison thickness, and guaranteed mono compatibility in a single, beautiful interface."

**Key Differentiators:**

1. **Dual-Mode Architecture**:
   - Mode A: "Wobble" - Tape-style wow/flutter with era selection
   - Mode B: "Unison" - Supersaw-style detuned voices (2-7 voices)
   - Blend between modes for hybrid effects

2. **100% Mono-Compatible**:
   - Use Wider-style phase-safe algorithms
   - Mono check button in UI
   - Guaranteed to work on phone speakers, clubs, etc.

3. **Saturated Character**:
   - Built-in analog-modeled saturation (subtle to aggressive)
   - Color knob affecting tone/warmth
   - Not just widening - adds personality

4. **Colorful Lo-Fi Aesthetic**:
   - Distinctive, artistic UI (not utilitarian grey)
   - Visual feedback of modulation/wobble
   - Inspiring to use, not just functional

5. **Simplicity**:
   - Core controls: Mode, Amount, Character, Width, Mix
   - Advanced panel for power users
   - Sounds great with minimal tweaking

---

## 4. Technical Approaches - DSP Methods

### 4.1 Delay-Based Micro-Pitch Shifting

**How It Works:**
The algorithm uses cross-fading between two channels with time-varying delays and gains. Takes advantage of the Doppler effect - pitch shifts as delay increases/decreases.

**Classic Implementation (H3000-style):**
- Left channel: -9 cents detune, 15ms delay
- Right channel: +11 cents detune, 25ms delay
- Creates stereo spread without obvious modulation

**Parameters:**
- Delay time (5-50ms typical)
- Detune amount (+/- 50 cents typical)
- Crossfade algorithm (affects "glitching")

**Pros:**
- Low latency
- Clean, predictable results
- Industry-proven approach

**Cons:**
- Can sound "digital" or sterile
- Limited character
- Phase issues when summed to mono

**Sources:** [MATLAB Delay-Based Pitch Shifter](https://www.mathworks.com/help/audio/ug/delay-based-pitch-shifter.html), [Valhalla DSP Blog](https://valhalladsp.com/2010/05/06/digital-pitch-shifting-early-work/)

---

### 4.2 Granular Pitch Shifting

**How It Works:**
Audio is divided into small "grains" (1-50ms). Each grain can be played back at different rates to achieve pitch shift without time stretch. Multiple overlapping grains create smooth transitions.

**Implementation Details:**
- Buffer with two read positions, half-buffer apart
- Each read moves at desired speed ratio
- Crossfade at grain boundaries to prevent clicks
- Grain size ~2000 samples to preserve low frequencies

**Parameters:**
- Grain size (affects frequency response and artifacts)
- Overlap amount
- Grain shape (windowing function)
- Randomization (for natural variation)

**Pros:**
- Can achieve large pitch shifts
- More natural than simple delay-based
- Good for formant-independent pitch shifting

**Cons:**
- Higher latency
- Can introduce "granular artifacts" (bubbly sound)
- More CPU intensive
- Not ideal for subtle micropitch

**Quality Considerations:**
- 5 semitones typical limit for transparent results
- Single instruments can go further than complex mixes
- TDHS (Time Domain Harmonic Scaling) variant uses fundamental-length grains

**Sources:** [Sound On Sound - Granular Synthesis](https://www.soundonsound.com/techniques/granular-synthesis), [Zynaptiq Blog](http://blogs.zynaptiq.com/bernsee/time-pitch-overview/)

---

### 4.3 Tape Wow/Flutter Simulation

**How It Works:**
Modulates delay time (and thus pitch) using LFOs with specific characteristics:
- **Wow**: Slow modulation (0.5-2 Hz) from motor/reel inconsistency
- **Flutter**: Fast modulation (4-15 Hz) from tape-to-head friction
- **Scrape Flutter**: Very fast, from tape physical properties

**Key to Authenticity:**
- NOT simple sine wave LFOs
- Multiple overlapping modulation sources
- Random/noise components for non-repeating patterns
- Speed tied to tape speed (15 IPS vs 7.5 IPS vs cassette)

**Advanced Techniques:**
- Triangle wave for wow (more linear than sine)
- Noise-modulated rate for organic feel
- Smoothing filters to prevent discontinuities
- Era-specific frequency response curves

**Parameters:**
- Wow rate and depth
- Flutter rate and depth
- Noise/randomization amount
- Tape speed/era selection

**Goodhertz Approach:**
"Random wow makes more sense on a tape delay which has a bunch of loose tape loop in a cartridge" vs. periodic wow from a reel.

**Sources:** [Baby Audio Blog](https://babyaud.io/blog/wow-and-flutter), [HISE Forum - Faust Implementation](https://forum.hise.audio/topic/11185/retro-80s-tape-wow-flutter-with-faust/20)

---

### 4.4 Unison Detuning Algorithms

**How It Works:**
Creates multiple copies ("voices") of the input, each detuned by small amounts, then summed and spread across stereo field.

**Classic Supersaw (JP-8000 Style):**
- 7 detuned sawtooth oscillators
- Central voice at nominal pitch
- Others spread +/- from center
- Stereo spread via panning

**For Audio Processing (not synthesis):**
- Create N copies of input signal
- Apply pitch shift to each (linear or non-linear distribution)
- Pan across stereo field
- Sum with original

**Detuning Distribution:**
- Linear: Equal spacing between voices
- Non-linear: More voices near center, less at extremes
- Random: Each voice randomly detuned within range

**Best Practices:**
- 4-8 voices typical (more can sound "bad")
- Detune < 50% to avoid dissonance
- Central voice at unity for tonal anchor
- Fine mode: max +/- 99 cents for classic unison sound

**Mono Compatibility Challenge:**
- Multiple pitch-shifted signals can cause phase cancellation
- Solution: Use different processing per voice that cancels on mono sum
- Or: Accept that unison effect will reduce on mono systems

**Sources:** [Spectrasonics Omnisphere Manual](https://support.spectrasonics.net/manual/Omnisphere2/25/en/topic/layer-page-oscillator-page22), [FaderPro - Supersaw](https://blog.faderpro.com/techniques/supersaw-how-make-iconic-sound/)

---

### 4.5 Mono-Compatible Stereo Widening

**The Problem:**
Most stereo widening techniques cause phase cancellation when summed to mono, resulting in thin or hollow sound.

**Polyverse Wider's Approach:**
- All-pass and comb filtering creates stereo illusion
- When summed to mono, effect cancels out completely
- Original signal passes through unaffected
- "Mono-compatible" means effect disappears, not distorts

**Alternative Approaches:**
- Mid-Side processing (affect only Side channel)
- Frequency-dependent widening (widen highs, leave lows mono)
- Haas effect with compensation

**Design Consideration for O-Detune:**
A plugin could offer:
- "Safe" mode: 100% mono-compatible (effect cancels on mono)
- "Character" mode: Some effect preserved on mono, some phase issues accepted
- User chooses based on target playback systems

**Sources:** [Polyverse Wider](https://polyversemusic.com/products/wider/), [Sound On Sound Forum](https://www.soundonsound.com/forum/viewtopic.php?t=87345)

---

## 5. Recommendations for O-Detune

### 5.1 Core Feature Set

**Essential (Must Have):**

1. **Dual-Mode Engine**
   - **Wobble Mode**: Tape-style wow/flutter
     - Rate: 0.1 - 10 Hz (covers wow and flutter)
     - Depth: 0 - 100 cents
     - Shape: Sine/Triangle/Random
     - Era: 60s/70s/80s presets for tape character

   - **Unison Mode**: Multi-voice detuning
     - Voices: 2, 3, 4, 5, 7
     - Detune: 0 - 50 cents spread
     - Distribution: Linear/Exponential/Random
     - Spread: Stereo width of voices

2. **Blend Control**: Crossfade between Wobble and Unison (0-100%)

3. **Character Section**
   - Drive: Subtle saturation to tube-style warmth
   - Color: Tone shaping (dark to bright)
   - Age: Combined degradation (hiss + filtering + drift)

4. **Width Control**: Overall stereo width (mono to extra-wide)

5. **Mix**: Wet/Dry blend with solo option

6. **Mono-Safe Toggle**
   - ON: Guarantees mono compatibility (effect may reduce)
   - OFF: Full effect, phase issues possible

**Important (Should Have):**

7. **Focus/Filter**: Frequency range for effect (like MicroShift's Focus)
8. **Delay**: Pre-delay for spatial depth (0-50ms)
9. **Feedback**: Recirculate for intensifying effect
10. **Tempo Sync**: Lock wobble rate to DAW tempo

**Nice to Have:**

11. **Modulation routing**: LFO to various parameters
12. **Randomization**: Per-voice random variation
13. **Presets**: Genre-specific starting points
14. **MIDI learn**: For hardware control

### 5.2 Target Use Cases

| Use Case | Primary Mode | Key Parameters |
|----------|--------------|----------------|
| Vocal thickening | Unison | 3 voices, 10 cents, high Drive |
| Synth widening | Unison | 5-7 voices, 25 cents, low Drive |
| Pad movement | Wobble | Low rate, medium depth, high blend |
| Lo-fi vocals | Wobble + Unison | 70s era, Age up, medium of both |
| Creative destruction | Wobble | High rate, high depth, Feedback |
| Bass fattening | Unison (low-passed) | 2 voices, 5 cents, Focus on low-mids |
| Drum room | Unison | 3 voices, 15 cents, Delay + Space |

### 5.3 UI/UX Recommendations

**Visual Design:**
- Colorful, lo-fi aesthetic (think vintage synth meets cassette culture)
- Animated visualization of pitch wobble/voices
- Warm color palette (oranges, teals, creams)
- Not skeuomorphic hardware - modern but characterful

**Layout:**
- Central Mode selector (Wobble | Blend | Unison)
- Main parameters in arc around center
- Advanced parameters in expandable panel
- Preset browser sidebar

**Key Interactions:**
- Single knob for "instant vibe" (macro combining multiple parameters)
- A/B comparison
- Bypass per section
- Visual feedback of mono compatibility status

### 5.4 Competitive Positioning

**Price Point:** $49-69
- Below Goodhertz Wow Control ($129)
- At or below Soundtoys MicroShift ($99)
- Above commodity plugins ($29)
- Premium feel, indie price

**Target Audience:**
- Lo-fi/bedroom producers
- Indie/alternative mixers
- Sound designers
- Electronic music producers
- Podcast/voiceover (subtle thickening)

**Marketing Angle:**
"The detuning plugin that combines analog warmth with digital precision. MicroShift meets RC-20 - finally, tape wobble and unison thickness in one colorful, mono-safe package."

### 5.5 Technical Specifications

**Performance Targets:**
- Latency: < 256 samples (5.8ms at 44.1kHz)
- CPU: < 3% on modern systems
- Memory: < 50MB
- Zero-latency mode option (higher CPU)

**Format Support:**
- VST3, AU, AAX (64-bit)
- Windows 10+ and macOS 11+
- Consider iOS version for mobile market growth

**Sample Rates:**
- 44.1kHz - 192kHz support

---

## 6. Appendix: Sources and References

### Official Product Pages
- [Soundtoys MicroShift](https://www.soundtoys.com/product/microshift/)
- [Eventide MicroPitch](https://www.eventideaudio.com/plug-ins/micropitch/)
- [Valhalla Space Modulator](https://valhalladsp.com/shop/modulation/valhalla-space-modulator/)
- [Polyverse Wider](https://polyversemusic.com/products/wider/)
- [Goodhertz Wow Control](https://goodhertz.com/wow-ctrl/)
- [Goodhertz Lossy](https://goodhertz.com/lossy/)
- [Soundtoys Little AlterBoy](https://www.soundtoys.com/product/little-alterboy/)
- [XLN Audio RC-20 Retro Color](https://www.xlnaudio.com/products/addictive_fx/effect/rc-20_retro_color)
- [Baby Audio TAIP](https://babyaud.io/taip-plugin)
- [Denise Audio Bad Tape](https://www.deniseaudio.com/plugins/bad-tape)

### Reviews and Analysis
- [Sound On Sound - MicroShift Review](https://www.soundonsound.com/reviews/soundtoys-microshift)
- [Sound On Sound - MicroPitch Review](https://www.soundonsound.com/reviews/eventide-micropitch)
- [Tape Op - Wow Control Review](https://tapeop.com/reviews/gear/137/wow-control-plug-in/)
- [MusicRadar - TAIP Review](https://www.musicradar.com/reviews/baby-audio-taip)
- [Produce Like A Pro - RC-20 Review](https://producelikeapro.com/blog/rc-20-retro-color-review/)

### Technical Resources
- [MATLAB - Delay-Based Pitch Shifter](https://www.mathworks.com/help/audio/ug/delay-based-pitch-shifter.html)
- [Valhalla DSP Blog - Digital Pitch Shifting](https://valhalladsp.com/2010/05/06/digital-pitch-shifting-early-work/)
- [Stanford CCRMA - Effect Design Part 2](https://ccrma.stanford.edu/~dattorro/EffectDesignPart2.pdf)
- [Zynaptiq - Time Stretching Overview](http://blogs.zynaptiq.com/bernsee/time-pitch-overview/)
- [Baby Audio - Wow and Flutter](https://babyaud.io/blog/wow-and-flutter)

### Market Research
- [Integraudio - Best Detune Plugins 2025](https://integraudio.com/best-detune-plugins/)
- [Production Expert - Stereo Widening Plugins](https://www.production-expert.com/production-expert-1/7-stereo-widening-plugins-you-should-check-out-in-2021)
- [KVR Audio Forums - Microshift vs Doublers](https://www.kvraudio.com/forum/viewtopic.php?t=457797)

---

**Research Confidence Assessment:**

| Category | Confidence | Notes |
|----------|------------|-------|
| Competitor Features | HIGH | Based on official product pages and professional reviews |
| Pricing | HIGH | Current prices verified from multiple retailers |
| DSP Algorithms | MEDIUM | Based on technical articles and documentation; implementation details vary |
| Market Gaps | MEDIUM | Synthesized from feature analysis; validation through user feedback recommended |
| Technical Specs | HIGH | Based on industry standards and comparable plugins |

---

*Document prepared for O-Detune creative brief development.*
