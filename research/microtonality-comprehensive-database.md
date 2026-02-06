---
title: "Comprehensive Microtonality Database for VST Development"
created: 2026-01-09
last_verified: 2026-02-06
juce_version: "8.0.4"
summary: "Consolidated 4,683-line database covering tuning system theory, file formats (Scala, KBM, TUN, MTS-ESP), JUCE implementation patterns for synthesizers/samplers/effects, performance optimization, UI/UX design, and testing/validation approaches."
domain: dsp
type: reference
keywords:
  - microtonality
  - tuning-systems
  - scala
  - mts-esp
  - just-intonation
  - xenharmonic
  - juce-dsp
  - temperament
stages: [0, 2]
agents: [dsp, research]
---

# Comprehensive Microtonality Database for VST Development

**Version:** 1.0
**Compiled:** January 2026
**Source Documents:** Theory/Formats, JUCE Implementation, Commercial/Performance Research
**Total Scope:** 4,683 lines of consolidated research

---

## Master Table of Contents

### Part I: Foundations
1. [Executive Summary](#executive-summary)
2. [Tuning System Theory](#part-i-tuning-system-theory)
   - [12-TET Standard](#12-tet-twelve-tone-equal-temperament)
   - [Just Intonation](#just-intonation)
   - [N-TET Equal Divisions](#n-tet-equal-divisions)
   - [Historical Temperaments](#historical-temperaments)
   - [Xenharmonic Scales](#xenharmonic-scales)
   - [Gamelan Tunings](#gamelan-tunings)
3. [Mathematical Foundations](#mathematical-foundations)
   - [Frequency/Cents Conversion](#frequency-and-cents-conversion)
   - [Harmonic Lattices](#harmonic-lattices-and-tonnetz)
   - [Temperament Optimization](#temperament-optimization)
4. [Scale Classification](#scale-classification)
   - [MOS Scales](#mos-moment-of-symmetry-scales)
   - [Rothenberg Propriety](#rothenberg-propriety)

### Part II: File Formats & Standards
5. [Scala Format (.scl)](#scala-format-scl)
6. [Keyboard Mapping (.kbm)](#keyboard-mapping-kbm)
7. [AnaMark TUN Format](#anamark-tun-format-tun)
8. [MTS-ESP Protocol](#mts-esp-protocol)
9. [MIDI Tuning Standard](#midi-tuning-standard-mts)
10. [MIDI 2.0 Per-Note Pitch](#midi-20-per-note-pitch)

### Part III: Commercial Ecosystem
11. [Product Comparison Matrix](#commercial-product-analysis)
12. [Detailed Product Analysis](#detailed-product-analysis)
13. [MTS-ESP Masters Available](#available-masters)

### Part IV: JUCE Implementation
14. [Implementation Approaches](#implementation-approaches)
15. [Synthesizer Implementation](#synthesizer-implementation)
16. [Sampler Implementation](#sampler-implementation)
17. [Physical Modeling](#physical-modeling)
18. [Effects Processing](#effects-processing)
19. [JUCE-Specific Patterns](#juce-implementation-patterns)

### Part V: Performance & UI/UX
20. [Performance Optimization](#performance-optimization)
21. [UI/UX Design Patterns](#uiux-design-patterns)
22. [Preset Management](#preset-management)

### Part VI: Modern Approaches
23. [MPE Integration](#mpe-midi-polyphonic-expression)
24. [MIDI 2.0 Adoption](#midi-20-per-note-controllers)
25. [Web Audio & ML](#web-audio-api-microtonality)

### Part VII: Testing & Validation
26. [Accuracy Verification](#tuning-accuracy-verification)
27. [Automated Testing](#automated-testing)
28. [Testing Checklist](#testing-checklist)

### Appendices
- [A: Quick Reference Tables](#appendix-a-quick-reference-tables)
- [B: Complete Code Examples](#appendix-b-complete-code-examples)
- [C: Integration Libraries](#appendix-c-integration-libraries)
- [D: Common Pitfalls](#appendix-d-common-pitfalls-and-solutions)
- [E: Consolidated References](#appendix-e-consolidated-references)

---

# Executive Summary

## The Microtonality Landscape (2020-2026)

The microtonality ecosystem in VST plugins has evolved significantly, driven by three key developments:

1. **MTS-ESP Standardization**: ODDSound's MTS-ESP protocol has become the de facto standard for cross-plugin microtuning, adopted by major manufacturers including Arturia, u-he, and Native Instruments.

2. **MPE Maturation**: MIDI Polyphonic Expression provides per-note pitch control, enabling real-time microtonal expression without dedicated tuning tables.

3. **MIDI 2.0 Emergence**: Native per-note controllers and 16-bit resolution promise to simplify microtonal implementations, though DAW adoption remains in progress.

## Implementation Decision Matrix

| Approach | Complexity | Best For | Integration Time |
|----------|------------|----------|------------------|
| **MTS-ESP Client** | LOW | Universal support, DAW-wide tuning | 15 minutes |
| **Surge Tuning Library** | MEDIUM | Scala/KBM file support | 1-2 hours |
| **Custom Tuning Tables** | MEDIUM | Full control, specialized needs | 2-4 hours |
| **MPE Integration** | MEDIUM-HIGH | Per-note expression, controllers | 4-8 hours |

## Quick Start Recommendation

For new synthesizers, implement **MTS-ESP** as the primary method with **Scala file support** via the Surge tuning library as a fallback. This provides compatibility with both DAW-wide tuning and standalone file-based tuning.

```cpp
// Minimal MTS-ESP Integration (15-minute implementation)
#include "libMTSClient.h"

class MicrotonalSynth {
    MTSClient* mtsClient = nullptr;

    void initialize() {
        mtsClient = MTS_RegisterClient();
    }

    ~MicrotonalSynth() {
        if (mtsClient) MTS_DeregisterClient(mtsClient);
    }

    double getFrequency(int midiNote, int midiChannel = -1) {
        if (mtsClient && MTS_HasMaster(mtsClient)) {
            return MTS_NoteToFrequency(mtsClient, midiNote, midiChannel);
        }
        // Fallback to 12-TET
        return 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
    }
};
```

---

# Part I: Tuning System Theory

## 12-TET (Twelve-Tone Equal Temperament)

12-TET divides the octave into 12 equal parts on a logarithmic scale. Each semitone has a frequency ratio equal to the 12th root of 2.

### Mathematical Basis

```
Semitone ratio = 2^(1/12) = 1.059463...
```

**Key formulas:**
- Frequency of note n semitones from reference: `f = f_ref * 2^(n/12)`
- Each semitone = 100 cents
- Octave = 1200 cents

### Interval Accuracy in 12-TET

| Interval | Just Ratio | Just Cents | 12-TET Cents | Error |
|----------|------------|------------|--------------|-------|
| Perfect Fifth | 3/2 | 701.955 | 700 | -1.955 |
| Perfect Fourth | 4/3 | 498.045 | 500 | +1.955 |
| Major Third | 5/4 | 386.314 | 400 | +13.686 |
| Minor Third | 6/5 | 315.641 | 300 | -15.641 |
| Major Sixth | 5/3 | 884.359 | 900 | +15.641 |
| Minor Sixth | 8/5 | 813.686 | 800 | -13.686 |

### Historical Development

12-TET was independently calculated by:
- **Zhu Zaiyu** (China, 1584) - using successive division by the 12th root of 2
- **Simon Stevin** (Netherlands, 1585) - first Western mathematician to develop the system

12-TET became standard because it is the smallest EDO (equal division of the octave) that can reasonably represent 5-limit harmony while enabling unlimited key modulation.

---

## Just Intonation

Just intonation uses frequency ratios of whole numbers, producing pure intervals based on the harmonic series.

### Prime Limit Systems

**3-Limit (Pythagorean)**
- Uses only powers of 2 and 3
- Primary intervals: Perfect fifth (3:2), Perfect fourth (4:3)
- Generates melodically useful scales via chains of fifths

**5-Limit**
- Adds factor of 5 to create consonant thirds
- Primary intervals:
  - Major third: 5:4 (386.31 cents)
  - Minor third: 6:5 (315.64 cents)
  - Major sixth: 5:3 (884.36 cents)
  - Minor sixth: 8:5 (813.69 cents)
- Closest to conventional Western harmony

**7-Limit**
- Adds septimal intervals outside conventional Western theory
- Key intervals:
  - Subminor third: 7:6 (266.87 cents)
  - Septimal tritone: 7:5 (582.51 cents)
  - Harmonic seventh: 7:4 (968.83 cents)

**7-Limit Tonality Diamond:**
```
1/1, 8/7, 7/6, 6/5, 5/4, 4/3, 7/5, 10/7, 3/2, 8/5, 5/3, 12/7, 7/4, 2/1
```

**11-Limit and Beyond**
- Introduces quartertone-like intervals (~50 cents)
- Neutral thirds and sevenths (between major and minor)
- Primary intervals for first 8 primes: 2/1, 3/2, 5/4, 7/4, 11/8, 13/8, 17/16, 19/16

### Common Just Intervals Table

| Interval Name | Ratio | Cents | Limit |
|--------------|-------|-------|-------|
| Unison | 1:1 | 0.000 | 1 |
| Minor second | 16:15 | 111.73 | 5 |
| Major second | 9:8 | 203.91 | 3 |
| Minor third | 6:5 | 315.64 | 5 |
| Major third | 5:4 | 386.31 | 5 |
| Perfect fourth | 4:3 | 498.04 | 3 |
| Tritone | 45:32 | 590.22 | 5 |
| Perfect fifth | 3:2 | 701.96 | 3 |
| Minor sixth | 8:5 | 813.69 | 5 |
| Major sixth | 5:3 | 884.36 | 5 |
| Harmonic seventh | 7:4 | 968.83 | 7 |
| Minor seventh | 16:9 | 996.09 | 3 |
| Major seventh | 15:8 | 1088.27 | 5 |
| Octave | 2:1 | 1200.00 | 2 |

---

## N-TET Equal Divisions

Different numbers of equal divisions offer varying approximations to just intervals.

### 19-TET
- Step size: 63.16 cents
- **Excellent minor third/major sixth** (less than 0.2 cents from just)
- Fifth: 695 cents (slightly flat)
- Good for 5-limit harmony with different character than 12-TET

### 31-TET
- Step size: 38.71 cents
- **Very close approximation to quarter-comma meantone**
- Fifth: 696.77 cents
- Excellent major third (386.3 cents, nearly just)
- Good approximations to 7th and 11th harmonics
- Most manageable size for extended xenharmonic work

**Key intervals in 31-TET:**
| Interval | Steps | Cents |
|----------|-------|-------|
| Minor second | 2-3 | 77-116 |
| Major second | 5 | 193.5 |
| Minor third | 8 | 309.7 |
| Major third | 10 | 387.1 |
| Perfect fourth | 13 | 503.2 |
| Perfect fifth | 18 | 696.8 |

### 53-TET
- Step size: 22.64 cents (Holdrian comma)
- **Best approximation of 5-limit in smaller EDOs**
- Fifth: 701.89 cents (within 0.07 cents of just)
- Major third: 384.91 cents (within 1.4 cents of just)
- Used in Turkish music theory
- Essentially interchangeable with extended Pythagorean tuning

**Historical note:** Chinese theorist Jing Fang (78-37 BCE) observed that 53 just fifths very nearly equals 31 octaves.

### Comparison of EDO Approximations

| EDO | Fifth (cents) | Fifth Error | Major Third (cents) | M3 Error |
|-----|---------------|-------------|---------------------|----------|
| 12 | 700.00 | -1.96 | 400.00 | +13.69 |
| 19 | 694.74 | -7.22 | 378.95 | -7.36 |
| 31 | 696.77 | -5.18 | 387.10 | +0.78 |
| 41 | 702.44 | +0.48 | 390.24 | +3.93 |
| 53 | 701.89 | -0.07 | 384.91 | -1.41 |
| 72 | 700.00 | -1.96 | 383.33 | -2.98 |

---

## Historical Temperaments

### Meantone Temperament

Meantone tuning (late 15th - early 18th century) features pure major thirds (5:4) with slightly compromised fifths.

**Quarter-comma meantone:**
- Fifths tempered by 1/4 syntonic comma (~5.38 cents flat)
- Fifth size: 696.58 cents
- Pure major thirds: 386.31 cents
- **Wolf fifth:** One interval (typically G#-Eb) is extremely sharp (~737 cents)

**Meantone fifths chain:**
```
Eb - Bb - F - C - G - D - A - E - B - F# - C# - G#
    ^                                              ^
    └──────────── Wolf fifth (unusable) ──────────┘
```

### Well-Temperaments

Well-temperaments (late 17th century onwards) eliminate the wolf fifth while preserving key color differences.

**Werckmeister III (1691)**

The most popular well-temperament, designed for easy conversion from meantone.

| Note | Cents from C |
|------|-------------|
| C | 0.00 |
| C# | 90.22 |
| D | 192.18 |
| Eb | 294.13 |
| E | 390.22 |
| F | 498.04 |
| F# | 588.27 |
| G | 696.09 |
| G# | 792.18 |
| A | 888.27 |
| Bb | 996.09 |
| B | 1092.18 |

Four fifths (C-G, G-D, D-A, B-F#) are tempered by 1/4 Pythagorean comma; all others are pure.

**Key characteristics of well-temperaments:**
- All keys playable (no wolf)
- Different keys have different "colors"
- Keys near C have purer thirds; remote keys have wider thirds
- Expressive possibilities through key choice

---

## Xenharmonic Scales

### Bohlen-Pierce Scale

A non-octave scale designed around odd harmonics, developed independently by Heinz Bohlen and John R. Pierce in the 1970s.

**Structure:**
- **Tritave** (3:1, perfect twelfth) replaces octave as period
- 13 equal divisions of the tritave (13-EDT)
- Step size: 146.3 cents
- Emphasizes odd-number ratios: 3:5:7:9 tetrads

**Just intonation version:**
```
1/1, 27/25, 25/21, 9/7, 7/5, 75/49, 5/3, 9/5, 49/25, 15/7, 7/3, 63/25, 25/9, 3/1
```

**Natural affinity with clarinet:** The clarinet's spectrum contains primarily odd harmonics, and it overblows at the tritave rather than octave.

### 833-Cents Golden Scale (Bohlen)

Based on the golden ratio (phi = 1.618...).

**Structure:**
- Period: 833.09 cents (the golden ratio interval)
- 7 unequal steps
- Octaves occur incidentally, not as a structural feature
- Based on combination tones and Fibonacci sequence

---

## Gamelan Tunings

Indonesian gamelan uses two primary tuning systems that vary between ensembles.

### Slendro
- **Pentatonic** (5 notes per octave)
- Approximately equal spacing within the octave
- Notation: 1, 2, 3, 5, 6 (ji, ro, lu, mo, nem)
- Associated with light, cheerful atmosphere

**Approximate slendro intervals (vary by ensemble):**
```
Note 1: 0 cents
Note 2: ~240 cents
Note 3: ~480 cents
Note 5: ~720 cents
Note 6: ~960 cents
Octave: 1200 cents
```

### Pelog
- **Heptatonic** (7 notes per octave)
- Uneven intervals
- Often played using 5-note subsets
- Notation: 1, 2, 3, 4, 5, 6, 7 (ji, ro, lu, pat, mo, nem, tu)
- Associated with regal, sacred atmosphere

### Ensemble-Specific Tuning
- Each gamelan ensemble is tuned uniquely
- Instruments within a set are tuned to each other
- No fixed external pitch standard (unlike A=440)
- Creates distinctive timbre and resonance per ensemble

**Balinese ombak (beating):** Paired instruments are tuned slightly apart to create shimmering interference patterns.

---

## Mathematical Foundations

### Frequency and Cents Conversion

#### Fundamental Formulas

**Ratio to cents:**
```
cents = 1200 * log2(ratio)
cents = 1200 * log10(ratio) / log10(2)
cents = 1200 * ln(ratio) / ln(2)
```

**Cents to ratio:**
```
ratio = 2^(cents / 1200)
```

**Frequency to MIDI note (12-TET):**
```
midi_note = 69 + 12 * log2(frequency / 440)
```

**MIDI note to frequency (12-TET):**
```
frequency = 440 * 2^((midi_note - 69) / 12)
```

#### Code Implementation

```cpp
#include <cmath>

class TuningMath {
public:
    static double ratioToCents(double ratio) {
        return 1200.0 * std::log2(ratio);
    }

    static double centsToRatio(double cents) {
        return std::pow(2.0, cents / 1200.0);
    }

    static double getFrequency(int midiNote, double centsOffset = 0.0,
                               double a4Freq = 440.0) {
        double semitonesFromA4 = midiNote - 69.0 + centsOffset / 100.0;
        return a4Freq * std::pow(2.0, semitonesFromA4 / 12.0);
    }
};
```

### Harmonic Lattices and Tonnetz

The Tonnetz is a lattice diagram representing pitch relationships:
- **Horizontal axis:** Perfect fifths (3:2)
- **Diagonal axis:** Major thirds (5:4)
- **Other diagonal:** Minor thirds (6:5)

```
      ...  F#   C#   G#   D#   A#  ...
         /  \  /  \  /  \  /  \  /
      ...  D    A    E    B    F#  ...
         /  \  /  \  /  \  /  \  /
      ...  Bb   F    C    G    D   ...
         /  \  /  \  /  \  /  \  /
      ...  Gb   Db   Ab   Eb   Bb  ...
```

#### Prime Limit Dimensions

| Limit | Dimensions | Vectors |
|-------|------------|---------|
| 3-limit | 1D | Fifth (3:2) |
| 5-limit | 2D | Fifth (3:2), Major third (5:4) |
| 7-limit | 3D | + Harmonic seventh (7:4) |
| 11-limit | 4D | + Undecimal tritone (11:8) |

#### Monzo Notation

A monzo represents a JI ratio as prime factor exponents:
```
Ratio = 2^a * 3^b * 5^c * 7^d * ...
Monzo = |a b c d ...>

Examples:
3/2 = 2^(-1) * 3^1 = |-1 1>
5/4 = 2^(-2) * 5^1 = |-2 0 1>
7/4 = 2^(-2) * 7^1 = |-2 0 0 1>
```

---

## Scale Classification

### MOS (Moment of Symmetry) Scales

Invented by Erv Wilson in 1975, MOS scales have exactly two step sizes.

#### Definition

A MOS scale is generated by:
1. Repeatedly stacking a **generator** interval
2. Reducing by a **period** (usually octave)
3. Result has exactly 2 distinct step sizes: Large (L) and small (s)

#### Examples

**Diatonic scale (5L+2s):**
- Generator: ~702 cents (fifth)
- Period: 1200 cents (octave)
- 7 notes: L L s L L L s (in C major: C D E F G A B)

**Pentatonic scale (2L+3s):**
- Same generator and period
- 5 notes: L s L L s

### Rothenberg Propriety

David Rothenberg's 1978 classification system for scales.

#### Definitions

- **Strictly proper:** Every N-step interval is smaller than every (N+1)-step interval
- **Proper:** Allows ambiguities (equal sizes across classes) but no contradictions
- **Improper:** Contains contradictions (some N-step > some (N+1)-step)

**Diatonic scale (proper, not strictly proper):**
- Has one ambiguity: augmented fourth = diminished fifth (both 600 cents in 12-TET)

**Pentatonic scale (strictly proper):**
- All 1-steps < all 2-steps < all 3-steps < all 4-steps

---

# Part II: File Formats & Standards

## Scala Format (.scl)

The Scala scale file format is the de facto standard for scale exchange, used by the Scala program and supported by hundreds of synthesizers and tools.

### Complete Specification

**File characteristics:**
- Extension: `.scl`
- Encoding: ASCII or 8-bit text (ISO 8859-1 preferred)
- One scale per file
- Human-readable text format

**File structure:**
```
! [comment line - starts with exclamation mark]
[Description - first non-comment line, single line, may be empty]
[Note count - number of pitch values to follow]
[Pitch value 1]
[Pitch value 2]
...
[Pitch value N]
```

**Pitch value formats:**

1. **Ratio format:** `numerator/denominator`
   - Example: `3/2`, `81/64`, `5/4`
   - Integer alone treated as ratio: `2` means `2/1`
   - Maximum value: 2^31 - 1 (2,147,483,647)
   - Negative ratios invalid

2. **Cents format:** Must contain a decimal point
   - Examples: `700.0`, `386.314`, `100.`, `-5.0`
   - Text after value is ignored

**Important notes:**
- The starting pitch 1/1 (or 0.0 cents) is **implicit** and not listed
- The note count refers to listed pitches only
- Whitespace before values is ignored

### Example Files

**12-TET:**
```
! 12tet.scl
!
12 tone equal temperament
12
!
100.0
200.0
300.0
400.0
500.0
600.0
700.0
800.0
900.0
1000.0
1100.0
2/1
```

**5-Limit Just Intonation:**
```
! ji_5limit.scl
!
5-limit just intonation major scale
7
!
9/8
5/4
4/3
3/2
5/3
15/8
2/1
```

**Bohlen-Pierce (13-EDT):**
```
! bp_13edt.scl
!
Bohlen-Pierce scale, 13 equal divisions of 3/1
13
!
146.304
292.608
438.913
585.217
731.521
877.826
1024.130
1170.434
1316.739
1463.043
1609.347
1755.652
3/1
```

---

## Keyboard Mapping (.kbm)

The keyboard mapping file defines how scale degrees map to MIDI note numbers.

### Complete Specification

**Required parameters (in order):**

```
! [optional comments]
[Size of map] - Pattern repeats every N keys (0 for linear)
[First MIDI note to retune] - Usually 0
[Last MIDI note to retune] - Usually 127
[Middle note] - MIDI note for scale degree 0
[Reference note] - MIDI note for reference frequency
[Reference frequency] - Hz (e.g., 440.0)
[Octave degree] - Scale degree considered formal octave
[Mapping entries...] - One per line, or 'x' for unmapped
```

### Parameter Details

| Parameter | Description | Default |
|-----------|-------------|---------|
| Size of map | Number of entries in mapping pattern | 0 (linear) |
| First MIDI note | Start of retuning range | 0 |
| Last MIDI note | End of retuning range | 127 |
| Middle note | MIDI note for degree 0 | 60 |
| Reference note | MIDI note for reference frequency | 69 |
| Reference frequency | Frequency in Hz | 440.0 |
| Octave degree | Scale degree = formal octave | (scale size) |

### Example: 7-note scale on white keys

```
! 7-note scale on white keys only
12
0
127
60
69
440.0
7
0
x
1
x
2
3
x
4
x
5
x
6
```

---

## AnaMark TUN Format (.tun)

The AnaMark TUN format is widely supported by VST instruments.

### Version 2.0 Structure

**File characteristics:**
- Extension: `.tun`
- Encoding: ASCII text
- Comments: Lines starting with `;`
- Sections: Denoted by `[SectionName]`

**Basic structure:**
```
; Comment line
[Tuning]
note 0 = xxx.xxxxxx cents
note 1 = xxx.xxxxxx cents
...
note 127 = xxx.xxxxxx cents

[Scale Begin]
[Info]
Name = "Scale Name"
ID = "unique.identifier"

[Exact Tuning]
note 0 = xxx.xxxxxx cents
...

[Scale End]
```

---

## MTS-ESP Protocol

MTS-ESP (MIDI Tuning Standard - Extended Specification Protocol) enables real-time centralized microtuning control developed by ODDSound.

### Architecture

```
┌─────────────────┐         ┌─────────────────┐
│  MTS-ESP MASTER │         │  MTS-ESP CLIENT │
│                 │         │                 │
│ - Defines tuning│────────>│ - Follows tuning│
│ - One per       │  IPC    │ - Unlimited     │
│   session       │         │   instances     │
│ - Can filter    │         │ - Can query     │
│   notes         │         │   continuously  │
│                 │         │   or at note-on │
└─────────────────┘         └─────────────────┘
```

### API Functions (C/C++)

```c
#include "libMTSClient.h"

// Registration
MTSClient* MTS_RegisterClient();
void MTS_DeregisterClient(MTSClient* client);

// Connection status
bool MTS_HasMaster(MTSClient* client);

// Note filtering (for unmapped keys)
bool MTS_ShouldFilterNote(MTSClient* client, char midiNote, signed char midiChannel);

// Frequency queries
double MTS_NoteToFrequency(MTSClient* client, char midiNote, signed char midiChannel);
double MTS_RetuningInSemitones(MTSClient* client, char midiNote, signed char midiChannel);
double MTS_RetuningAsRatio(MTSClient* client, char midiNote, signed char midiChannel);

// Reverse lookup
char MTS_FrequencyToNote(MTSClient* client, double freq, signed char midiChannel);

// Scale information
const char* MTS_GetScaleName(MTSClient* client);
```

### Features

- **17 tuning tables:** One per MIDI channel (1-16) plus general table
- **Note filtering:** Master can mark notes as unmapped
- **Fallback:** Defaults to 12-TET when no master connected
- **Latency:** Zero (direct memory access)
- **CPU:** Negligible

---

## MIDI Tuning Standard (MTS)

The original MIDI Tuning Standard allows SysEx-based microtuning.

### Resolution

- **14-bit fraction:** 16,384 divisions per semitone
- **Total resolution:** 196,608 parts per octave
- **Precision:** ~0.0061 cents

### Frequency Data Format (3 bytes per note)

```
Byte 1: [0sssssss] - Semitone (MIDI note number, 0-127)
Byte 2: [0fffffff] - Fraction MSB (bits 13-7)
Byte 3: [0fffffff] - Fraction LSB (bits 6-0)

Fraction = (Byte2 << 7) | Byte3
Cents offset = Fraction / 16384 * 100
```

---

## MIDI 2.0 Per-Note Pitch

MIDI 2.0 introduces high-resolution per-note pitch control.

### Note-On Pitch Attribute

- **Resolution:** 7.9 format (7 bits semitone, 9 bits fraction)
- **Precision:** 1/512 of a semitone
- Embedded directly in Note-On message

### Registered Per-Note Controller #3 (Pitch)

- **Resolution:** 7.25 format
  - 7 bits: Semitones
  - 25 bits: Fraction (1/33,554,432 semitone)
- Real-time control over note lifetime

### Key Advantages over MIDI 1.0

| Feature | MIDI 1.0 | MIDI 2.0 |
|---------|----------|----------|
| Pitch bend resolution | 14-bit global | 32-bit per-note |
| Per-note tuning | Requires SysEx | Native support |
| Real-time changes | Limited | Full support |
| Controller resolution | 7-14 bit | 32-bit |

---

# Part III: Commercial Ecosystem

## Commercial Product Analysis

### Feature Comparison Table

| Product | Scala SCL | TUN Files | MTS-ESP | MPE | Per-Note Pitch | Notes/Octave | Price |
|---------|-----------|-----------|---------|-----|----------------|--------------|-------|
| **Surge XT** | Yes | Yes | Master + Client | Yes | Yes | Unlimited | Free/OSS |
| **Pianoteq** | Yes + KBM | No | Incoming MTS | No | No | Unlimited | $149-599 |
| **u-he Zebra** | No | Yes | Client | No | No | 128 | $199 |
| **u-he Diva** | No | Yes | Client | No | No | 128 | $189 |
| **Omnisphere** | No | Yes | No | No | No | 48/octave | $499 |
| **Kontakt** | Via Script | Via Script | No | Limited | Via KSP | Varies | $399-649 |
| **Arturia V Collection** | Yes (Pigments) | Yes | Client | Yes (v9+) | Yes | 128 | $599 |
| **Bitwig (DAW)** | Via Micro-pitch | Via Micro-pitch | No | Yes | Yes | Unlimited | $99-399 |
| **Madrona Aalto** | Yes | No | No | Yes | Yes | 128 | $99 |
| **NI Reaktor** | Manual | Manual | No | Yes | Yes | User-defined | $199-599 |

---

## Detailed Product Analysis

### Surge XT (Open Source Reference Implementation)

**Architecture**: Dual-scene hybrid synthesizer built on JUCE framework.

**Tuning Implementation**:
- Full Scala SCL/KBM format support
- Integrated Tuning Editor with analysis tools
- Can act as both MTS-ESP master AND client (since v1.2)
- Non-monotonic intonation system support
- Two tuning application modes:
  - "Apply tuning at MIDI input" (default)
  - "Apply tuning after modulation"

**Source Code**: https://github.com/surge-synthesizer/surge

**Key Files**:
- `src/common/` - Engine code including tuning logic
- `src/surge-xt/gui/` - Tuning Editor UI
- SSE2 hand-coded DSP for performance

### Pianoteq (Physical Modeling)

**Implementation**:
- One of the most complete Scala implementations
- Direct SCL + KBM file loading from UI
- Built-in temperaments: Pythagorean, Zarlino, Meantone, Werckmeister III, Equal, Flat
- Real-time MTS message support
- Tuning modeling options: "Full rebuild" vs "String tension"

**Unique Features**:
- Physical modeling responds to tuning changes naturally
- Octave stretching options
- Master pitch range: 420-460 Hz

### u-he Products (Zebra, Diva, Repro, ACE, Bazille)

**Implementation**:
- Native TUN file format support
- Files stored in `~/u-he/Tunefiles/`
- MTS-ESP client support added (all products)
- No native Scala support (requires conversion)

**Limitations**:
- 128-note tuning table limit
- No direct SCL loading

**Best Practice**: Use MTS-ESP for dynamic tuning or convert SCL to TUN using Scale Workshop.

### Kontakt (Native Instruments)

**Implementation**: Entirely via KSP (Kontakt Script Processor)

**Key Function**: `change_tune($EVENT_ID, millicents, 0)` - tunes in millicents (1/1000 of a cent)

**Challenges**:
- Many commercial libraries are "12-locked"
- Requires scripting knowledge
- Some libraries ignore tuning scripts

---

## Available Masters

| Master | Features | Price |
|--------|----------|-------|
| MTS-ESP Mini | SCL/KBM/TUN/MTS SysEx loading | Free |
| MTS-ESP Master | Full editing, automation, multi-channel | $79 |
| Surge XT | Built-in tuning editor, can act as master | Free |
| Wilsonic MTS-ESP | Erv Wilson scale designs | Open source |
| Infinitone DMT | Advanced tuning design | Commercial |

---

# Part IV: JUCE Implementation

## Implementation Approaches

| Approach | Complexity | Best For | Integration Time |
|----------|------------|----------|------------------|
| **MTS-ESP Client** | LOW | Universal support | 15 minutes |
| **Surge Tuning Library** | MEDIUM | Scala/KBM support | 1-2 hours |
| **Custom Tuning Tables** | MEDIUM | Full control | 2-4 hours |
| **MPE Integration** | MEDIUM-HIGH | Per-note expression | 4-8 hours |

---

## Synthesizer Implementation

### Oscillator Frequency Calculation

```cpp
class MicrotonalFrequency {
public:
    // Method 1: Direct frequency lookup (MTS-ESP style)
    double getFrequency(int midiNote) {
        return tuningTable[midiNote];
    }

    // Method 2: Semitone offset from 12-TET
    double getFrequencyFromOffset(int midiNote, double centsOffset) {
        double baseFreq = 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
        return baseFreq * std::pow(2.0, centsOffset / 1200.0);
    }

    // Method 3: Ratio-based (for just intonation)
    double getFrequencyFromRatio(double baseFreq, int numerator, int denominator) {
        return baseFreq * (static_cast<double>(numerator) / denominator);
    }

    // Method 4: Floating-point MIDI note
    double getFrequencyFromFloat(double floatMidiNote) {
        return 440.0 * std::pow(2.0, (floatMidiNote - 69.0) / 12.0);
    }
};
```

### Voice Management with Per-Note Tuning

```cpp
class MicrotonalVoice : public juce::SynthesiserVoice {
    double currentFrequency = 440.0;
    double pitchBendSemitones = 0.0;
    MTSClient* mtsClient = nullptr;
    int currentMidiChannel = 1;

public:
    void setMTSClient(MTSClient* client) { mtsClient = client; }

    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound*, int currentPitchWheelPosition) override {
        if (mtsClient && MTS_HasMaster(mtsClient)) {
            currentFrequency = MTS_NoteToFrequency(mtsClient, midiNoteNumber, currentMidiChannel);
        } else {
            currentFrequency = getTuningTableFrequency(midiNoteNumber);
        }
        updateOscillatorFrequency();
    }

    void pitchWheelMoved(int newPitchWheelValue) override {
        double pitchBendRange = 2.0;
        pitchBendSemitones = ((newPitchWheelValue - 8192) / 8192.0) * pitchBendRange;
        updateOscillatorFrequency();
    }

private:
    void updateOscillatorFrequency() {
        double finalFreq = currentFrequency * std::pow(2.0, pitchBendSemitones / 12.0);
        oscillator.setFrequency(finalFreq);
    }
};
```

### FM Synthesis Considerations

**Tuning Design Decision:**
- **Harmonic FM:** Keep modulator as a ratio of carrier - preserves harmonic relationships
- **Inharmonic FM:** Allow absolute modulator frequencies - creates non-harmonic timbres
- **Microtonal FM:** Consider whether ratios should follow the tuning scale

### Filter Keyboard Tracking

```cpp
void updateFilterForNote(int midiNote, double microtonalFrequency) {
    // Calculate semitone offset from middle C (MIDI 60)
    double semitoneOffset = 12.0 * std::log2(microtonalFrequency /
        juce::MidiMessage::getMidiNoteInHertz(60));

    // Apply keyboard tracking
    double trackingMultiplier = std::pow(2.0,
        (semitoneOffset * keyboardTrackingAmount) / 12.0);

    double finalCutoff = baseCutoff * trackingMultiplier;
    finalCutoff = juce::jlimit(20.0, 20000.0, finalCutoff);

    filter.setCutoff(finalCutoff);
}
```

---

## Sampler Implementation

### Pitch Shifting Algorithms Comparison

| Algorithm | Quality | CPU Cost | Best For |
|-----------|---------|----------|----------|
| **Linear Interpolation** | Low | Very Low | Simple playback |
| **Cubic/Lagrange** | Medium | Low | General use |
| **Sinc Interpolation** | High | Medium | High quality |
| **PSOLA** | High (speech) | Medium | Vocal samples |
| **Phase Vocoder** | High | High | Polyphonic |
| **Granular** | Variable | Medium | Creative effects |

### Resampling for Arbitrary Tuning

```cpp
class MicrotonalSampler {
    const float* sampleData;
    double originalPitch;
    double readPosition = 0.0;
    double playbackRatio = 1.0;

public:
    void setPitch(double targetPitch) {
        playbackRatio = targetPitch / originalPitch;
    }

    // Cubic interpolation (better quality)
    float processCubic() {
        int i0 = static_cast<int>(readPosition);
        float frac = static_cast<float>(readPosition - i0);

        // Catmull-Rom spline interpolation
        float a = sampleData[std::max(0, i0 - 1)];
        float b = sampleData[i0];
        float c = sampleData[std::min(sampleLength - 1, i0 + 1)];
        float d = sampleData[std::min(sampleLength - 1, i0 + 2)];

        float t = frac;
        float sample = 0.5f * ((2*b) + (-a + c) * t +
                               (2*a - 5*b + 4*c - d) * t*t +
                               (-a + 3*b - 3*c + d) * t*t*t);

        readPosition += playbackRatio;
        return sample;
    }
};
```

---

## Physical Modeling

### Karplus-Strong with Microtonal Pitch

```cpp
class MicrotonalKarplusStrong {
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>
        delayLine { 88200 };
    double sampleRate = 44100.0;

public:
    void trigger(double frequencyHz, float velocity) {
        // Fractional samples supported by Lagrange interpolation
        double delaySamples = sampleRate / frequencyHz;
        delaySamples = juce::jlimit(2.0, 88200.0, delaySamples);

        delayLine.reset();

        // Fill with noise burst
        for (int i = 0; i < static_cast<int>(delaySamples); ++i) {
            float noise = (random.nextFloat() * 2.0f - 1.0f) * velocity;
            delayLine.pushSample(0, noise);
        }
    }

    float process(double frequencyHz, float damping) {
        double delaySamples = sampleRate / frequencyHz;
        float output = delayLine.popSample(0, static_cast<float>(delaySamples));
        float filtered = loopFilter.processSample(output);
        delayLine.pushSample(0, filtered * damping);
        return filtered;
    }
};
```

### Fractional Delay for Precise Tuning

```cpp
class FractionalDelayAllpass {
    float y1 = 0.0f, x1 = 0.0f, a1 = 0.0f;

public:
    void setFractionalDelay(double fractionalSamples) {
        // Thiran allpass coefficient (valid for delays 0.1-1.9)
        double d = fractionalSamples;
        a1 = static_cast<float>((1.0 - d) / (1.0 + d));
    }

    float process(float input) {
        float output = a1 * (input - y1) + x1;
        x1 = input;
        y1 = output;
        return output;
    }
};
```

---

## Effects Processing

### Pitch-Aware Delay

```cpp
class PitchAwareDelay {
public:
    // Set delay as multiple of the period
    void setDelayForPitch(double fundamentalHz, int periods) {
        double periodSamples = sampleRate / fundamentalHz;
        delayLine.setDelay(static_cast<float>(periodSamples * periods));
    }

    // Avoid comb filtering
    void setDelayToAvoidComb(double fundamentalHz) {
        double periodSamples = sampleRate / fundamentalHz;
        delayLine.setDelay(static_cast<float>(periodSamples * 0.5));
    }
};
```

### Harmonic-Aware Distortion

```cpp
class MicrotonalDistortion {
    // Soft clipping preserves pitch content
    float softClip(float input, float drive) {
        return std::tanh(input * drive);
    }

    // Low-order waveshaping (fewer added harmonics)
    float gentleDistortion(float input) {
        float x = juce::jlimit(-1.0f, 1.0f, input);
        return x - (x * x * x) / 3.0f;
    }
};
```

---

## JUCE Implementation Patterns

### MPE Integration

```cpp
class MicrotonalMPEProcessor : public juce::AudioProcessor {
    juce::MPEInstrument mpeInstrument;
    juce::MPESynthesiser mpeSynth;

public:
    MicrotonalMPEProcessor() {
        auto layout = juce::MPEZoneLayout();
        layout.setLowerZone(juce::MPEZone(juce::MPEZone::Type::lower,
                                          15,    // 15 member channels
                                          48));  // 48 semitone pitch bend range
        mpeInstrument.setZoneLayout(layout);

        for (int i = 0; i < 16; ++i) {
            mpeSynth.addVoice(new MicrotonalMPEVoice());
        }
    }
};

class MicrotonalMPEVoice : public juce::MPESynthesiserVoice {
    void updateFrequency() {
        auto note = getCurrentlyPlayingNote();

        // getFrequencyInHertz() includes per-note + master pitchbend
        double frequency = note.getFrequencyInHertz();

        // Apply additional MTS-ESP offset
        if (mtsClient && MTS_HasMaster(mtsClient)) {
            double offset = MTS_RetuningInSemitones(mtsClient,
                note.initialNote, note.midiChannel);
            frequency *= std::pow(2.0, offset / 12.0);
        }

        oscillator.setFrequency(frequency);
    }
};
```

### Real-Time Tuning Table Updates

```cpp
class RealTimeTuningManager {
    std::atomic<bool> tuningChanged { false };
    TuningTable currentTuning;
    TuningTable pendingTuning;
    juce::SpinLock tuningLock;

public:
    // Called from UI thread
    void loadNewTuning(const Tunings::Scale& scale,
                       const Tunings::KeyboardMapping& mapping) {
        TuningTable newTuning;
        newTuning.loadScala(scale, mapping);

        {
            const juce::SpinLock::ScopedLockType lock(tuningLock);
            pendingTuning = newTuning;
        }
        tuningChanged.store(true);
    }

    // Called from audio thread
    void updateIfNeeded() {
        if (tuningChanged.load()) {
            const juce::SpinLock::ScopedTryLockType lock(tuningLock);
            if (lock.isLocked()) {
                currentTuning = pendingTuning;
                tuningChanged.store(false);
            }
        }
    }
};
```

### Parameter Design for Tuning Controls

```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createTuningParameters() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Reference pitch (A4 = 440 Hz default)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "TUNING_REF", "Reference Pitch",
        juce::NormalisableRange<float>(400.0f, 480.0f, 0.1f),
        440.0f, "Hz"));

    // Pitch bend range
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "BEND_RANGE", "Bend Range",
        juce::NormalisableRange<float>(0.0f, 48.0f, 1.0f),
        2.0f, "st"));

    // Tuning mode
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "TUNING_MODE", "Tuning Mode",
        juce::StringArray { "12-TET", "Scala File", "MTS-ESP", "Just Intonation" },
        0));

    return layout;
}
```

---

# Part V: Performance & UI/UX

## Performance Optimization

### Real-Time Pitch Calculation Strategies

#### 1. Lookup Tables (LUT)

**Advantages:**
- O(1) access time
- Predictable performance

**Disadvantages:**
- Cache pressure with large tables
- "Very large lookup tables mess up the cache. RAM is really slow these days."

**Optimal Size:** 128-4096 entries with linear interpolation

#### 2. Hybrid Approach (Recommended)

```cpp
class TuningCache {
    std::array<float, 128> baseTable;      // Pre-computed at scale load
    std::array<float, 128> modulatedTable; // Updated on parameter change
    bool tableValid = false;

    float getFrequency(int note) {
        if (!tableValid) rebuildTable();
        return modulatedTable[note];
    }

    void onParameterChange() {
        tableValid = false;  // Lazy rebuild
    }
};
```

### Voice Management Efficiency

**Per-Voice Tuning State:**
- Store tuned frequency at note-on (for note-on-only mode)
- OR store reference to tuning source for continuous updates
- Minimize per-sample calculations

**Memory Considerations:**
- Voice base memory: ~10-20 KB typical
- Per-instance overhead: ~500 KB

**CPU Guidelines:**
- Stay below 85% total CPU
- Profile with worst-case polyphony (16+ voices, complex tuning)

### DSP Optimization Tips

**Polynomial Approximation** (alternative to LUT for sine):
- "Polynomial approximation is the best way to do sine"
- Straightforward to vectorize with SIMD
- Better cache behavior

**SIMD Considerations:**
- Align data on 16-byte (SSE2) or 32-byte (AVX) boundaries
- Process voices in groups of 4 or 8
- Surge XT uses "hand-coded SSE2 implementations"

---

## UI/UX Design Patterns

### Scala File Loading Interface

**Best Practices:**
1. **Dual File Support**: Allow loading SCL and KBM separately
2. **Preset Browser**: Organize by category (historical, cultural, equal temperaments)
3. **Recent Files**: Quick access to recently used tunings
4. **Preview**: Play test notes while browsing
5. **Validation**: Show warnings for parsing issues

### Visual Tuning Editors

**Essential Visualizations:**
1. **Keyboard View**: Show pitch deviation per key
2. **Interval Matrix**: Display relationships between degrees
3. **Frequency Spectrum**: Show harmonic relationships
4. **Cent Ruler**: Visual scale structure

### Live Tuning Controls

**Essential Parameters:**
1. **Reference Pitch**: A4 frequency (default 440 Hz, range 415-466 Hz)
2. **Root Note**: Starting note for scale mapping
3. **Octave Stretch**: For piano/physical modeling
4. **Real-time Modulation**: Tuning amount as automatable parameter

---

## Preset Management

### Organization Structure

```
Tunings/
├── Equal Temperaments/
│   ├── 12-TET (Standard)
│   ├── 19-TET
│   ├── 31-TET
│   └── ...
├── Historical/
│   ├── Werckmeister III
│   ├── Kirnberger III
│   └── Meantone (1/4 comma)
├── Just Intonation/
│   ├── 5-limit JI
│   ├── 7-limit JI
│   └── Harry Partch 43-tone
├── Cultural/
│   ├── Arabic Maqam
│   ├── Gamelan Slendro
│   └── Gamelan Pelog
└── User/
    └── [Custom scales]
```

---

# Part VI: Modern Approaches

## MPE (MIDI Polyphonic Expression)

### How It Works
- Each note assigned to separate MIDI channel
- Channel 1 (or 16): Global messages
- Channels 2-16: Individual note data
- Maximum 15-note polyphony with full expression

### Compatible Controllers
- Haken Continuum Fingerboard
- ROLI Seaboard
- Roger Linn Design Linnstrument
- Sensel Morph
- Eigenharp

### JUCE Implementation

```cpp
// Key MPE classes
MPESynthesiser
MPEInstrument
MPENote
MPEValue
SmoothedValue  // For glide smoothing
```

---

## MIDI 2.0 Per-Note Controllers

### Standardization Progress
- November 2017: MPE specification released
- January 2018: MPE officially adopted by MMA
- June 2020: USB transport completed
- May 2024: DAW Working Group summit
- November 2024: Ethernet Network transport completed

### Key Improvements
- **16-bit velocity** (vs 7-bit in MIDI 1.0)
- **Per-note pitch bend** native
- **Per-note controllers** for vibrato, modulation
- **Bidirectional communication**

### Current Adoption
- DAWs: Cubase (partial), MultitrackStudio
- Controllers: KORG Keystage, NI S-Series (hardware ready)
- Plugins: Limited native support, MPE bridge common

---

## Web Audio API Microtonality

### Scale Workshop
- Design microtonal scales in browser
- Export to VST formats (SCL, KBM, TUN, MTS SysEx)
- Built-in isomorphic keyboard
- Repository: https://github.com/SeanArchibald/scale-workshop

### Tone.js Framework
- DAW-like features in browser
- Note specification as frequency or pitch-octave notation
- High-performance building blocks

---

# Part VII: Testing & Validation

## Tuning Accuracy Verification

### Frequency Measurement Tools

1. **DDMF PluginDoctor**: Double-precision FFT, harmonic analysis
2. **MathAudio THD Meter**: Harmonics h1-h5 measurement
3. **Melda Analyser**: Peak frequency pitch tracking
4. **Voxengo Span** (Free): Real-time FFT spectrum analyzer

### Testing Methodology

```
1. Generate test signal: sine wave at -18dBFS
2. Use 997Hz or 1kHz (997Hz shows aliasing better)
3. Measure fundamental frequency accuracy
4. Check for harmonic distortion
5. Verify against expected frequency table
```

### Accuracy Standards
- **Target:** +/- 0.1 cents
- **Acceptable:** +/- 1 cent
- **Perceptible difference:** ~5 cents

---

## Automated Testing

### Unit Tests
- Frequency calculation accuracy per note
- Scale parsing correctness
- Keyboard mapping validity
- Edge cases (note 0, note 127, extreme pitch bend)

### Integration Tests
- MTS-ESP master/client communication
- Tuning file loading/saving round-trip
- Parameter automation
- State recall after preset save/load

### Performance Tests
- CPU usage per voice with tuning active
- Latency measurement
- Memory allocation during scale changes

---

## Testing Checklist

- [ ] All 128 MIDI notes produce correct frequency
- [ ] Pitch bend applies correctly over tuning
- [ ] Scale files load without errors
- [ ] State saves and restores correctly
- [ ] MTS-ESP connection status visible
- [ ] CPU usage acceptable at full polyphony
- [ ] No audio glitches on tuning change

---

# Appendix A: Quick Reference Tables

## Common Just Intervals by Limit

| Limit | Interval | Ratio | Cents |
|-------|----------|-------|-------|
| 3 | Pythagorean minor third | 32/27 | 294.13 |
| 3 | Pythagorean major third | 81/64 | 407.82 |
| 5 | Just minor third | 6/5 | 315.64 |
| 5 | Just major third | 5/4 | 386.31 |
| 5 | Syntonic comma | 81/80 | 21.51 |
| 7 | Septimal minor third | 7/6 | 266.87 |
| 7 | Septimal tritone | 7/5 | 582.51 |
| 7 | Harmonic seventh | 7/4 | 968.83 |
| 11 | Undecimal tritone | 11/8 | 551.32 |
| 11 | Neutral third | 11/9 | 347.41 |

## EDO Comparison Chart

| EDO | Step (cents) | Fifth | Fifth Error | M3 | M3 Error |
|-----|--------------|-------|-------------|-----|----------|
| 12 | 100.00 | 700.0 | -1.96 | 400.0 | +13.69 |
| 17 | 70.59 | 705.9 | +3.93 | 352.9 | -33.40 |
| 19 | 63.16 | 694.7 | -7.22 | 378.9 | -7.36 |
| 22 | 54.55 | 709.1 | +7.14 | 381.8 | -4.51 |
| 24 | 50.00 | 700.0 | -1.96 | 400.0 | +13.69 |
| 31 | 38.71 | 696.8 | -5.18 | 387.1 | +0.78 |
| 41 | 29.27 | 702.4 | +0.48 | 390.2 | +3.93 |
| 53 | 22.64 | 701.9 | -0.07 | 384.9 | -1.41 |

## Frequency Conversions

```cpp
// MIDI to Hz (12-TET)
hz = 440 * pow(2, (midi - 69) / 12.0);

// Hz to MIDI
midi = 69 + 12 * log2(hz / 440);

// Cents to ratio
ratio = pow(2, cents / 1200.0);

// Ratio to cents
cents = 1200 * log2(ratio);

// Pitch bend to semitones (14-bit, +/- range)
semitones = ((bend - 8192) / 8192.0) * range;
```

## Scala File Format Quick Reference

```
! comment
Description line
[note count]
[pitch 1 - ratio or cents]
[pitch 2]
...
[pitch N - usually 2/1 or 1200.0]
```

## KBM File Format Quick Reference

```
! comment
[map size: 0=linear, N=repeating pattern]
[first MIDI note to retune]
[last MIDI note to retune]
[middle note for degree 0]
[reference note for frequency]
[reference frequency in Hz]
[formal octave degree]
[mapping entries: integers or 'x' for unmapped]
```

---

# Appendix B: Complete Code Examples

## Complete Tuning Table Class

```cpp
class MicrotonalTuningTable {
public:
    static constexpr int NUM_NOTES = 128;

    MicrotonalTuningTable() {
        initializeToEqual12TET();
    }

    void initializeToEqual12TET(double referenceHz = 440.0, int referenceNote = 69) {
        for (int note = 0; note < NUM_NOTES; ++note) {
            frequencies[note] = referenceHz *
                std::pow(2.0, (note - referenceNote) / 12.0);
            centsFromEqual[note] = 0.0;
        }
    }

    void initializeToEqualTemperament(int divisions, double periodRatio = 2.0,
                                       double referenceHz = 440.0, int referenceNote = 69) {
        for (int note = 0; note < NUM_NOTES; ++note) {
            int steps = note - referenceNote;
            frequencies[note] = referenceHz *
                std::pow(periodRatio, static_cast<double>(steps) / divisions);

            double equal12Freq = referenceHz * std::pow(2.0, (note - referenceNote) / 12.0);
            centsFromEqual[note] = 1200.0 * std::log2(frequencies[note] / equal12Freq);
        }
    }

    double getFrequency(int midiNote) const {
        return frequencies[juce::jlimit(0, 127, midiNote)];
    }

    double getCentsOffset(int midiNote) const {
        return centsFromEqual[juce::jlimit(0, 127, midiNote)];
    }

    double getFrequencyWithOffset(int midiNote, double semitoneOffset) const {
        return getFrequency(midiNote) * std::pow(2.0, semitoneOffset / 12.0);
    }

    double getInterpolatedFrequency(int fromNote, int toNote, double position) const {
        double fromLog = std::log2(getFrequency(fromNote));
        double toLog = std::log2(getFrequency(toNote));
        return std::pow(2.0, fromLog + (toLog - fromLog) * position);
    }

private:
    std::array<double, NUM_NOTES> frequencies;
    std::array<double, NUM_NOTES> centsFromEqual;
};
```

## Complete Scala File Parser

```cpp
class ScalaParser {
public:
    struct ParseResult {
        std::string description;
        std::vector<double> cents;
        bool success = false;
        std::string errorMessage;
    };

    static ParseResult parse(const juce::String& sclContent) {
        ParseResult result;
        auto lines = juce::StringArray::fromLines(sclContent);

        int lineIndex = 0;

        // Skip comments
        while (lineIndex < lines.size() && lines[lineIndex].trimStart().startsWith("!")) {
            ++lineIndex;
        }

        // Description
        if (lineIndex >= lines.size()) {
            result.errorMessage = "Missing description";
            return result;
        }
        result.description = lines[lineIndex++].toStdString();

        // Skip comments
        while (lineIndex < lines.size() && lines[lineIndex].trimStart().startsWith("!")) {
            ++lineIndex;
        }

        // Note count
        if (lineIndex >= lines.size()) {
            result.errorMessage = "Missing note count";
            return result;
        }
        int noteCount = lines[lineIndex++].getIntValue();

        // Parse intervals
        while (result.cents.size() < noteCount && lineIndex < lines.size()) {
            auto line = lines[lineIndex++].trim();
            if (line.startsWith("!") || line.isEmpty()) continue;

            if (line.contains("/")) {
                auto parts = juce::StringArray::fromTokens(line, "/", "");
                if (parts.size() >= 2) {
                    double num = parts[0].getDoubleValue();
                    double den = parts[1].getDoubleValue();
                    if (den > 0) {
                        result.cents.push_back(1200.0 * std::log2(num / den));
                    }
                }
            } else if (line.contains(".")) {
                result.cents.push_back(line.getDoubleValue());
            } else {
                double value = line.getDoubleValue();
                if (value > 0) {
                    result.cents.push_back(1200.0 * std::log2(value));
                }
            }
        }

        result.success = (result.cents.size() == noteCount);
        return result;
    }
};
```

## MTS-ESP Client Integration

```cpp
class MTSESPTuning {
    MTSClient* client = nullptr;
    bool connected = false;

public:
    MTSESPTuning() {
        client = MTS_RegisterClient();
        connected = (client != nullptr);
    }

    ~MTSESPTuning() {
        if (client) MTS_DeregisterClient(client);
    }

    bool isConnected() const {
        return connected && MTS_HasMaster(client);
    }

    double getFrequency(int midiNote, int midiChannel = -1) const {
        if (!client) {
            return 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
        }
        return MTS_NoteToFrequency(client, midiNote, midiChannel);
    }

    bool shouldFilterNote(int midiNote, int midiChannel = -1) const {
        if (!client) return false;
        return MTS_ShouldFilterNote(client, midiNote, midiChannel);
    }

    juce::String getScaleName() const {
        if (client && MTS_HasMaster(client)) {
            return juce::String(MTS_GetScaleName(client));
        }
        return "Not connected";
    }
};
```

---

# Appendix C: Integration Libraries

## MTS-ESP (ODDSound)

**Repository:** https://github.com/ODDSound/MTS-ESP

**Integration Steps:**
1. Add `libMTSClient.h` and `libMTSClient.cpp` to project
2. Register client in constructor, deregister in destructor
3. Query frequencies using client API

**Complete API:**
```cpp
MTSClient* MTS_RegisterClient();
void MTS_DeregisterClient(MTSClient* client);
bool MTS_HasMaster(MTSClient* client);
bool MTS_ShouldFilterNote(MTSClient* client, char midiNote, signed char midiChannel);
double MTS_NoteToFrequency(MTSClient* client, char midiNote, signed char midiChannel);
double MTS_RetuningInSemitones(MTSClient* client, char midiNote, signed char midiChannel);
const char* MTS_GetScaleName(MTSClient* client);
```

## Surge Tuning Library

**Repository:** https://github.com/surge-synthesizer/tuning-library

**Integration:** Header-only (`Tunings.h`, `TuningsImpl.h`)

```cpp
#include "Tunings.h"

auto scale = Tunings::readSCLFile("path/to/scale.scl");
auto mapping = Tunings::readKBMFile("path/to/mapping.kbm");
Tunings::Tuning tuning(scale, mapping);

double freq = tuning.frequencyForMidiNote(midiNote);
double cents = tuning.retuningFromEqualInCentsForMidiNote(midiNote);
bool mapped = tuning.isMidiNoteMapped(midiNote);
```

---

# Appendix D: Common Pitfalls and Solutions

## Pitch Accuracy Issues

```cpp
// PITFALL 1: Integer delay (physical modeling)
// BAD:
int delaySamples = static_cast<int>(sampleRate / frequency);

// GOOD: Use fractional delay
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine;
float delaySamples = static_cast<float>(sampleRate / frequency);

// PITFALL 2: Sample rate mismatch
// BAD: Hardcoded sample rate
phase += freq / 44100.0;

// GOOD: Use actual sample rate
phase += freq / sampleRate;

// PITFALL 3: Pitch bend order
// BAD: Apply bend to 12-TET then add offset
double freq = midiToHz(note) * pitchBendMultiplier * microtonalRatio;

// GOOD: Apply bend to microtonal frequency
double freq = tuningTable.getFrequency(note) * pitchBendMultiplier;
```

## Parameter Smoothing

```cpp
class SmoothTuningVoice {
    juce::SmoothedValue<float> smoothedFrequency;

    void prepareToPlay(double sampleRate, int samplesPerBlock) {
        smoothedFrequency.reset(sampleRate, 0.01);  // 10ms smoothing
    }

    void setFrequency(double targetFreq) {
        smoothedFrequency.setTargetValue(static_cast<float>(targetFreq));
    }
};
```

## MPE + Microtonality Conflicts

```cpp
enum class TuningMode {
    MPE_PITCH_BEND,        // Use MPE pitch bend only
    MICROTONAL_TABLE,      // Use tuning table only
    COMBINED               // MPE bend relative to microtonal base
};

void updateFrequency(TuningMode mode) {
    auto note = getCurrentlyPlayingNote();

    switch (mode) {
        case TuningMode::COMBINED:
            double baseFreq = tuningTable->getFrequency(note.initialNote);
            double bendRatio = note.getFrequencyInHertz() /
                juce::MidiMessage::getMidiNoteInHertz(note.initialNote);
            frequency = baseFreq * bendRatio;
            break;
    }
}
```

---

# Appendix E: Consolidated References

## Official Documentation
- [Scala SCL Format](https://www.huygens-fokker.org/scala/scl_format.html)
- [MTS-ESP GitHub](https://github.com/ODDSound/MTS-ESP)
- [Surge Tuning Library](https://github.com/surge-synthesizer/tuning-library)
- [JUCE MPE Documentation](https://docs.juce.com/master/classMPEInstrument.html)
- [JUCE MPE Tutorial](https://juce.com/tutorials/tutorial_mpe_introduction/)

## Standards
- [MIDI.org Microtuning](https://www.midi.org/midi-articles/microtuning-and-alternative-intonation-systems)
- [MPE Specification](https://d30pueezughrda.cloudfront.net/campaigns/mpe/mpespec.pdf)
- [MIDI 2.0 Resources](https://midi.org/category/https-midi-org-information-for-midi-2-0-developers)

## Open Source Implementations
- [Surge XT](https://surge-synthesizer.github.io/)
- [Vital](https://vital.audio/)
- [Scale Workshop](https://github.com/SeanArchibald/scale-workshop)

## Academic Resources
- [J.O. Smith - Physical Audio Signal Processing](https://ccrma.stanford.edu/~jos/pasp/)
- [Xenharmonic Wiki](https://en.xen.wiki/)
- [Kyle Gann - Just Intonation Explained](https://www.kylegann.com/tuning.html)

## Community
- [JUCE Forum - Microtonality](https://forum.juce.com/t/microtonality/46130)
- [KVR Audio Forums](https://www.kvraudio.com/forum/)
- [Sevish Microtonal Blog](https://sevish.com/category/microtonal-music/)

## Testing Tools
- [DDMF PluginDoctor](https://ddmf.eu/plugindoctor/)
- [Voxengo Span](https://www.voxengo.com/product/span/) (Free)
- [REW Room Acoustics](https://www.roomeqwizard.com/)

---

*Comprehensive Microtonality Database for VST Development*
*Compiled: January 2026*
*Plugin Freedom System Research Archive*
