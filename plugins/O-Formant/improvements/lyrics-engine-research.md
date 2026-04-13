# O-Formant Lyrics Engine - Research Brief

## Concept
Add a Lyrics tab where users type words (English or IPA) and the synth performs them -- one syllable per MIDI note, stepping through the sequence. The engine drives vowelX/Y, consonant place/manner, and nasal parameters to articulate each phoneme.

---

## Architecture Overview

```
User Types Text (JS)
       |
  [CMUdict lookup + G2P fallback]
       |
  [Syllabifier (MOP algorithm)]
       |
  Phoneme Schedule per Syllable
       |
  [Send to C++ via nativeFunction]
       |
  LyricsEngine (C++) -- audio thread
       |
  On noteStarted() -> read next syllable
       |
  Per-sample parameter automation:
    vowelX/Y, consonantTone, sibilance,
    consonantLevel, nasalCoupling, nasalPlace
```

### Split: JS handles text processing, C++ handles real-time parameter automation

**JS (WebView):**
- CMU Pronouncing Dictionary (~134K words, ~1.2MB gzipped as JSON)
- NRL letter-to-sound rules for unknown words (~500 lines, based on SAM reciter)
- Syllabification via Maximum Onset Principle
- Phoneme-to-parameter mapping tables
- Lyrics tab UI: text input, phoneme display, syllable highlighting

**C++ (Audio Thread):**
- `LyricsEngine` class: stores syllable queue, advances on note-on
- Per-syllable phoneme schedule with sample-accurate timing
- Smooth parameter interpolation (SmoothedValue for vowelX/Y transitions)
- Pre-utterance consonant timing (consonants start before pitch onset)

---

## Phoneme-to-Parameter Mapping

### Vowels -> VowelX/Y Coordinates

Existing cardinals in VowelData.h:

| IPA | ARPABET | vowelX | vowelY | Status |
|-----|---------|--------|--------|--------|
| i   | IY      | 0.00   | 1.00   | exists |
| e   | EY*     | 0.31   | 0.43   | exists |
| a   | AA      | 0.83   | 0.00   | exists |
| o   | OW*     | 1.00   | 0.35   | exists |
| u   | UW      | 0.98   | 0.93   | exists |
| r   | R       | 0.12   | 0.72   | exists |
| l   | L       | 0.55   | 0.85   | exists |

New targets needed for English:

| IPA | ARPABET | vowelX | vowelY | Example |
|-----|---------|--------|--------|---------|
| ɪ   | IH      | 0.10   | 0.80   | bit     |
| ɛ   | EH      | 0.25   | 0.30   | bet     |
| ae  | AE      | 0.50   | 0.05   | bat     |
| ʌ   | AH1     | 0.60   | 0.15   | but     |
| ʊ   | UH      | 0.85   | 0.78   | book    |
| ə   | AH0     | 0.50   | 0.45   | comma   |
| ɔ   | AO      | 0.92   | 0.20   | caught  |
| ɝ   | ER      | 0.55   | 0.50   | bird    |

These don't need new entries in VowelData.h -- the IDW morpher already interpolates continuously. We just target these XY coordinates.

### Diphthong Trajectories

| ARPABET | Start X,Y         | End X,Y           | Example |
|---------|--------------------|--------------------|---------|
| EY      | (0.25, 0.35)      | (0.05, 0.85)      | bait    |
| AY      | (0.65, 0.10)      | (0.08, 0.72)      | bite    |
| AW      | (0.55, 0.08)      | (0.85, 0.25)      | bout    |
| OW      | (0.55, 0.22)      | (0.55, 0.65)      | boat    |
| OY      | (0.90, 0.28)      | (0.05, 0.80)      | boy     |

Transition: interpolate from start to end over 60% of vowel duration (starting at 30%, ending at 90%).

### Consonants -> Place/Manner/Nasal

| ARPABET | IPA | Place | Manner | Voiced | Special |
|---------|-----|-------|--------|--------|---------|
| P       | p   | 0.00  | 0.00   | no     | plosive burst |
| B       | b   | 0.00  | 0.00   | yes    | voiced plosive |
| M       | m   | 0.00  | -      | yes    | nasal: coupling=1.0, nasalPlace=0.0 |
| F       | f   | 0.08  | 1.00   | no     | fricative |
| V       | v   | 0.08  | 1.00   | yes    | voiced fricative |
| W       | w   | 0.05  | -      | yes    | approximant: vowelXY glide |
| TH      | th  | 0.15  | 1.00   | no     | fricative |
| DH      | dh  | 0.15  | 1.00   | yes    | voiced fricative |
| T       | t   | 0.33  | 0.00   | no     | plosive burst |
| D       | d   | 0.33  | 0.00   | yes    | voiced plosive |
| N       | n   | 0.33  | -      | yes    | nasal: coupling=1.0, nasalPlace=0.5 |
| S       | s   | 0.33  | 1.00   | no     | fricative |
| Z       | z   | 0.33  | 1.00   | yes    | voiced fricative |
| L       | l   | 0.33  | -      | yes    | approximant: use existing L vowel |
| R       | ɹ   | 0.40  | -      | yes    | approximant: use existing R vowel |
| CH      | tʃ  | 0.55  | 0.15   | no     | affricate |
| JH      | dʒ  | 0.55  | 0.15   | yes    | voiced affricate |
| SH      | ʃ   | 0.55  | 1.00   | no     | fricative |
| ZH      | ʒ   | 0.55  | 1.00   | yes    | voiced fricative |
| Y       | j   | 0.67  | -      | yes    | approximant: vowelXY glide |
| K       | k   | 1.00  | 0.00   | no     | plosive burst |
| G       | ɡ   | 1.00  | 0.00   | yes    | voiced plosive |
| NG      | ŋ   | 1.00  | -      | yes    | nasal: coupling=1.0, nasalPlace=1.0 |
| HH      | h   | 0.50  | 0.85   | no     | aspiration noise |

**Key insight:** Nasals (M/N/NG) and approximants (W/R/L/Y) don't map cleanly to the noise-based consonant engine. They need different handling:
- **Nasals**: Engage NasalPoleZero (already exists) with appropriate nasalPlace, suppress consonant noise
- **Approximants**: Model as rapid vowelXY transitions rather than noise events. W = glide from U-region, Y = glide from I-region, R and L already have entries in VowelData.h

---

## Intra-Syllable Timing Model

```
Syllable: "string" = S T R IH NG

Timeline relative to note pitch onset (t=0):

PRE-UTTERANCE (before t=0):
  t = -120ms : S fricative (place=0.33, manner=1.0, level=0.5)
  t = -70ms  : T plosive closure (suppress source)
  t = -40ms  : T burst release (place=0.33, manner=0.0)
  t = -30ms  : R approximant (vowelXY glides toward IH)

NOTE BODY (after t=0):
  t = 0ms    : IH vowel target (vowelX=0.10, vowelY=0.80)
  t = sustain: hold IH formants (+ vibrato/expression as normal)

RELEASE / CODA:
  t = release-30ms : NG nasal coda (nasalCoupling=1.0, nasalPlace=1.0)
```

Default consonant durations (adjustable):
- Plosive closure: 40-80ms
- Plosive burst: 10-20ms
- Fricative: 60-120ms
- Affricate: 80-120ms
- Nasal: 60-100ms
- Approximant: 30-50ms
- Aspiration (HH): 50-80ms

**Pre-utterance**: Onset consonants borrow time from BEFORE the note's pitch. This is how VOCALOID/SynthV achieve intelligible consonants -- if you start the consonant at note-on, the vowel arrives late and sounds sluggish.

---

## Syllabification Algorithm

**Maximum Onset Principle** on ARPABET phoneme sequences:

1. Identify vowel nuclei (AA, AE, AH, AO, AW, AY, EH, ER, EY, IH, IY, OW, OY, UH, UW)
2. Each vowel = one syllable
3. For consonant clusters between vowels, assign maximum consonants to onset of next syllable
4. Constrained by English onset legality:
   - Legal 2-consonant onsets: PL, PR, BL, BR, TR, DR, KL, KR, GL, GR, FL, FR, TH+R, SH+R, SK, SP, ST, SM, SN, SL, SW
   - Legal 3-consonant onsets: SPL, SPR, STR, SKR, SKW
   - NG never in onset
   - Single-consonant onsets: all except NG and ZH

Example: "INTERESTING" = IH N T R AH S T IH NG
- Syllables: [IH N] [T R AH S] [T IH NG]
- Wait -- that's wrong. Let me re-apply MOP:
  - Vowels at positions: IH(0), AH(4), IH(7)
  - Between IH and AH: N T R -> max onset = T R (legal), so N goes to coda -> [IH N] [T R AH]
  - Between AH and IH: S T -> max onset = S T (legal), so -> [S T IH NG]
  - Result: IHN / TRAH / STIHNG = in-ter-sting

---

## English Text Input

Two modes in the UI:
1. **English mode**: User types "Hello World", JS converts to phonemes and displays IPA
2. **Phonetic mode**: User types IPA directly (or edits the auto-generated IPA)

For English mode, the pipeline is:
1. Tokenize input into words (split on spaces/punctuation)
2. For each word, look up in CMUdict JSON
3. If not found, apply NRL G2P rules
4. Display result as IPA with syllable boundaries marked
5. User can click to edit individual phonemes

---

## UI Design (Lyrics Tab)

```
+------------------------------------------------------------------+
|  SYNTH    LYRICS    EFFECTS    TUNING                            |
+------------------------------------------------------------------+
| [English] [Phonetic]     Mode toggle                             |
+------------------------------------------------------------------+
|                                                                    |
|  Text Input:                                                       |
|  +--------------------------------------------------------------+  |
|  | Hello world how are you today                                 |  |
|  +--------------------------------------------------------------+  |
|                                                                    |
|  Phonemes:                                                         |
|  +--------------------------------------------------------------+  |
|  | [hɛ] [loʊ]  [wɜːld]  [haʊ]  [ɑːr]  [juː]  [tə] [deɪ]     |  |
|  +--------------------------------------------------------------+  |
|     ^current                                                       |
|                                                                    |
|  Syllable: 1 / 8                  [Loop] [Reset]                  |
|                                                                    |
|  Timing:                                                           |
|  Pre-utterance: [--====--] 80ms                                   |
|  Transition:    [--====--] 40ms                                   |
|                                                                    |
+------------------------------------------------------------------+
```

---

## Open Questions for Discussion

### 1. Voiced vs. Unvoiced Consonants
The current ConsonantEngine produces shaped noise -- it doesn't distinguish voiced (B,D,G,V,Z) from unvoiced (P,T,K,F,S). Voiced consonants need the glottal source active during the consonant phase. Options:
- **A)** Add a `voiced` flag that suppresses glottal onset suppression for voiced consonants
- **B)** Treat voiced/unvoiced as a mix between noise and glottal source during consonant phase

### 2. Pre-utterance Timing
VOCALOID starts consonants BEFORE the MIDI note boundary. In our architecture:
- **A)** Implement actual pre-utterance by delaying the pitch onset (adds latency complexity)
- **B)** Accept that consonants start at note-on and the vowel arrives slightly late (simpler, less natural)
- **C)** Use the existing pitchGlide mechanism -- consonant phase IS the beginning of the note, vowel emerges naturally as consonant envelope decays

### 3. Polyphony / Global vs Per-Voice Stepping
When notes overlap (legato, chords):
- **A)** Global syllable counter: each note-on advances to next syllable regardless of voice
- **B)** Per-voice syllable tracking: each voice maintains its own position (but how to assign syllables?)
- Recommendation: **A** -- global counter, sequential stepping. This matches VOCALOID/SynthV behavior.

### 4. Loop Behavior
When the sequence reaches the end:
- **A)** Loop back to start
- **B)** Hold on last syllable
- **C)** User toggle for loop vs. hold

### 5. State Persistence
The lyrics text needs to survive DAW save/load:
- Store as a string in the ValueTree state (getStateInformation/setStateInformation)
- The phoneme schedule can be regenerated from text on load

### 6. Approximant Handling Complexity
W, Y, R, L are currently handled differently:
- R and L already have VowelData entries (they're modeled as vowel-like formant positions)
- W and Y need to be modeled as rapid glides from U-space / I-space to the following vowel
- This is arguably the most natural-sounding approach since these ARE vowel-like sounds

### 7. Scope: MVP vs Full Feature
**MVP (recommended first pass):**
- Phonetic input only (skip G2P engine initially)
- Simple ARPABET entry with auto-syllabification
- Basic consonant/vowel timing (no pre-utterance)
- Global stepping, loop at end

**Full feature (second pass):**
- English text input with CMUdict + G2P fallback
- Pre-utterance consonant timing
- Diphthong trajectories
- Per-syllable timing adjustment UI
- Visual syllable-to-note mapping
