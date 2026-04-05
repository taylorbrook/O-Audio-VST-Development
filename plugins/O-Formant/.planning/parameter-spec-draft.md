# O-Formant Parameter Specification (Draft)

**Plugin Type:** Synthesizer (Physical Model Vocal - Source-Filter)
**Total Parameters:** 21
**APVTS Required:** Yes

---

## Vowel Morph Parameters (3)

### VOWEL_X
- **ID:** `vowelX`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.5
- **Label:** "Vowel X"
- **Unit:** none
- **DSP Component:** Formant Interpolation Engine
- **Behavior:** XY pad horizontal axis (front/back vowel space). Maps to 5-formant parallel bandpass filter bank frequencies via Shepard interpolation in log-frequency domain.

### VOWEL_Y
- **ID:** `vowelY`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.5
- **Label:** "Vowel Y"
- **Unit:** none
- **DSP Component:** Formant Interpolation Engine
- **Behavior:** XY pad vertical axis (open/close vowel space). Combined with Vowel X to interpolate between 5 cardinal vowels at acoustic positions.

### VOWEL_FOCUS
- **ID:** `vowelFocus`
- **Type:** Float
- **Range:** 1.0 - 6.0
- **Default:** 2.5
- **Label:** "Focus"
- **Unit:** none
- **DSP Component:** Formant Interpolation Engine
- **Behavior:** Shepard interpolation power parameter. Low values = washy ambient blends between vowels. High values = snapping vowel articulation.

---

## Glottal Source Parameters (5)

### GLOTTAL_RD
- **ID:** `glottalRd`
- **Type:** Float
- **Range:** 0.3 - 2.7
- **Default:** 1.0
- **Label:** "Voice Quality"
- **Unit:** none
- **DSP Component:** LF Glottal Pulse Model
- **Behavior:** Liljencrants-Fant Rd parameter controlling voice quality. 0.3 = pressed (near-saw wave), 1.0 = modal (natural), 2.7 = breathy (approaching noise). Controls open quotient, speed quotient, and return phase of glottal cycle.

### BREATHINESS
- **ID:** `breathiness`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.1
- **Label:** "Breathiness"
- **Unit:** "%"
- **DSP Component:** Aspiration Noise Mixer
- **Behavior:** Mix level of aspiration noise added to glottal source. MPE pressure target.

### VIBRATO_RATE
- **ID:** `vibratoRate`
- **Type:** Float
- **Range:** 0.5 - 12.0
- **Default:** 5.5
- **Label:** "Vib Rate"
- **Unit:** "Hz"
- **DSP Component:** Vibrato LFO
- **Behavior:** Speed of pitch vibrato LFO applied to F0.

### VIBRATO_DEPTH
- **ID:** `vibratoDepth`
- **Type:** Float
- **Range:** 0.0 - 100.0
- **Default:** 15.0
- **Label:** "Vib Depth"
- **Unit:** "cents"
- **DSP Component:** Vibrato LFO
- **Behavior:** Vibrato LFO modulation depth in cents.

### VIBRATO_DELAY
- **ID:** `vibratoDelay`
- **Type:** Float
- **Range:** 0.0 - 2000.0
- **Default:** 300.0
- **Label:** "Vib Delay"
- **Unit:** "ms"
- **DSP Component:** Vibrato LFO
- **Behavior:** Delay after note-on before vibrato engages. Natural vocal behavior.

---

## Consonant / Noise Parameters (4)

### CONSONANT_LEVEL
- **ID:** `consonantLevel`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.3
- **Label:** "Consonant"
- **Unit:** "%"
- **DSP Component:** Consonant Noise Branch
- **Behavior:** Overall consonant noise mix level. Controls parallel noise branch output.

### CONSONANT_TONE
- **ID:** `consonantTone`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.5
- **Label:** "Tone"
- **Unit:** none
- **DSP Component:** Consonant Noise Shaping Filter
- **Behavior:** Dark (/f/) to bright (/s/) noise filter. Controls cutoff of noise shaping bandpass.

### SIBILANCE
- **ID:** `sibilance`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.0
- **Label:** "Sibilance"
- **Unit:** "%"
- **DSP Component:** Consonant Noise Branch
- **Behavior:** High-frequency /s/ and /sh/ emphasis. Adds peaked resonance around 4-8kHz.

### AUTO_CONSONANT
- **ID:** `autoConsonant`
- **Type:** Bool
- **Range:** off / on
- **Default:** off
- **Label:** "Auto-Consonant"
- **Unit:** none
- **DSP Component:** Plosive Burst Generator
- **Behavior:** When enabled, fires 10-25ms plosive burst on note-on, crossfading into vowel ADSR.

---

## Envelope Parameters (4)

### ATTACK
- **ID:** `attack`
- **Type:** Float
- **Range:** 0.001 - 5.0
- **Default:** 0.01
- **Label:** "Attack"
- **Unit:** "s"
- **DSP Component:** ADSR Envelope
- **Behavior:** Attack time with fast-skewed range.

### DECAY
- **ID:** `decay`
- **Type:** Float
- **Range:** 0.001 - 5.0
- **Default:** 0.3
- **Label:** "Decay"
- **Unit:** "s"
- **DSP Component:** ADSR Envelope
- **Behavior:** Decay time.

### SUSTAIN
- **ID:** `sustain`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.8
- **Label:** "Sustain"
- **Unit:** "%"
- **DSP Component:** ADSR Envelope
- **Behavior:** Sustain level.

### RELEASE
- **ID:** `release`
- **Type:** Float
- **Range:** 0.001 - 10.0
- **Default:** 0.5
- **Label:** "Release"
- **Unit:** "s"
- **DSP Component:** ADSR Envelope
- **Behavior:** Release time.

---

## Voice Character Parameters (3)

### FORMANT_SHIFT
- **ID:** `formantShift`
- **Type:** Float
- **Range:** -24.0 - 24.0
- **Default:** 0.0
- **Label:** "Formant Shift"
- **Unit:** "st"
- **DSP Component:** Formant Filter Bank
- **Behavior:** Gender knob — shifts all formant center frequencies up/down in semitones. Negative = larger vocal tract (male), Positive = smaller (female/child).

### FORMANT_SPREAD
- **ID:** `formantSpread`
- **Type:** Float
- **Range:** 0.5 - 2.0
- **Default:** 1.0
- **Label:** "Spread"
- **Unit:** "x"
- **DSP Component:** Formant Filter Bank
- **Behavior:** Formant spacing multiplier. Expands or contracts spacing between formant frequencies from their center.

### PITCH_GLIDE
- **ID:** `pitchGlide`
- **Type:** Float
- **Range:** 0.0 - 1000.0
- **Default:** 0.0
- **Label:** "Glide"
- **Unit:** "ms"
- **DSP Component:** Voice Pitch Handler
- **Behavior:** Portamento/glide time between notes.

---

## Output Parameters (2)

### OUTPUT_GAIN
- **ID:** `outputGain`
- **Type:** Float
- **Range:** -60.0 - 12.0
- **Default:** 0.0
- **Label:** "Output"
- **Unit:** "dB"
- **DSP Component:** Output Stage
- **Behavior:** Master output level.

### STEREO_WIDTH
- **ID:** `stereoWidth`
- **Type:** Float
- **Range:** 0.0 - 1.0
- **Default:** 0.5
- **Label:** "Width"
- **Unit:** "%"
- **DSP Component:** Stereo Spread
- **Behavior:** Per-voice pan spread by pitch. 0 = mono, 1 = full stereo spread.
