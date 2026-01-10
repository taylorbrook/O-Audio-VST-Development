# Comprehensive Microtonality Theory and File Formats for VST Development

## Table of Contents
1. [Tuning System Theory](#1-tuning-system-theory)
   - [12-TET (Twelve-Tone Equal Temperament)](#12-tet-twelve-tone-equal-temperament)
   - [Just Intonation](#just-intonation)
   - [N-TET Equal Divisions](#n-tet-equal-divisions)
   - [Historical Temperaments](#historical-temperaments)
   - [Xenharmonic Scales](#xenharmonic-scales)
   - [Gamelan Tunings](#gamelan-tunings)
2. [File Formats and Standards](#2-file-formats-and-standards)
   - [Scala Format (.scl)](#scala-format-scl)
   - [Keyboard Mapping (.kbm)](#keyboard-mapping-kbm)
   - [AnaMark TUN Format (.tun)](#anamark-tun-format-tun)
   - [MTS-ESP Protocol](#mts-esp-protocol)
   - [MIDI Tuning Standard (MTS)](#midi-tuning-standard-mts)
   - [MIDI 2.0 Per-Note Pitch](#midi-20-per-note-pitch)
3. [Mathematical Foundations](#3-mathematical-foundations)
   - [Frequency and Cents Conversion](#frequency-and-cents-conversion)
   - [Harmonic Lattices and Tonnetz](#harmonic-lattices-and-tonnetz)
   - [Temperament Optimization](#temperament-optimization)
4. [Scale Classification](#4-scale-classification)
   - [MOS (Moment of Symmetry) Scales](#mos-moment-of-symmetry-scales)
   - [Rothenberg Propriety](#rothenberg-propriety)
5. [Implementation Code Examples](#5-implementation-code-examples)
6. [References and Resources](#6-references-and-resources)

---

## 1. Tuning System Theory

### 12-TET (Twelve-Tone Equal Temperament)

12-TET divides the octave into 12 equal parts on a logarithmic scale. Each semitone has a frequency ratio equal to the 12th root of 2.

#### Mathematical Basis

```
Semitone ratio = 2^(1/12) = 1.059463...
```

**Key formulas:**
- Frequency of note n semitones from reference: `f = f_ref * 2^(n/12)`
- Each semitone = 100 cents
- Octave = 1200 cents

#### Interval Accuracy in 12-TET

| Interval | Just Ratio | Just Cents | 12-TET Cents | Error |
|----------|------------|------------|--------------|-------|
| Perfect Fifth | 3/2 | 701.955 | 700 | -1.955 |
| Perfect Fourth | 4/3 | 498.045 | 500 | +1.955 |
| Major Third | 5/4 | 386.314 | 400 | +13.686 |
| Minor Third | 6/5 | 315.641 | 300 | -15.641 |
| Major Sixth | 5/3 | 884.359 | 900 | +15.641 |
| Minor Sixth | 8/5 | 813.686 | 800 | -13.686 |

#### Historical Development

12-TET was independently calculated by:
- **Zhu Zaiyu** (China, 1584) - using successive division by the 12th root of 2
- **Simon Stevin** (Netherlands, 1585) - first Western mathematician to develop the system

12-TET became standard because it is the smallest EDO (equal division of the octave) that can reasonably represent 5-limit harmony while enabling unlimited key modulation.

---

### Just Intonation

Just intonation uses frequency ratios of whole numbers, producing pure intervals based on the harmonic series.

#### Prime Limit Systems

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

#### Common Just Intervals Table

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

### N-TET Equal Divisions

Different numbers of equal divisions offer varying approximations to just intervals.

#### 19-TET
- Step size: 63.16 cents
- **Excellent minor third/major sixth** (less than 0.2 cents from just)
- Fifth: 695 cents (slightly flat)
- Good for 5-limit harmony with different character than 12-TET

**Scala representation:**
```
! 19tet.scl
19 tone equal temperament
19
63.15789
126.31579
189.47368
252.63158
315.78947
378.94737
442.10526
505.26316
568.42105
631.57895
694.73684
757.89474
821.05263
884.21053
947.36842
1010.52632
1073.68421
1136.84211
2/1
```

#### 31-TET
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

#### 53-TET
- Step size: 22.64 cents (Holdrian comma)
- **Best approximation of 5-limit in smaller EDOs**
- Fifth: 701.89 cents (within 0.07 cents of just)
- Major third: 384.91 cents (within 1.4 cents of just)
- Used in Turkish music theory
- Essentially interchangeable with extended Pythagorean tuning

**Historical note:** Chinese theorist Jing Fang (78-37 BCE) observed that 53 just fifths very nearly equals 31 octaves.

#### Comparison of EDO Approximations

| EDO | Fifth (cents) | Fifth Error | Major Third (cents) | M3 Error |
|-----|---------------|-------------|---------------------|----------|
| 12 | 700.00 | -1.96 | 400.00 | +13.69 |
| 19 | 694.74 | -7.22 | 378.95 | -7.36 |
| 31 | 696.77 | -5.18 | 387.10 | +0.78 |
| 41 | 702.44 | +0.48 | 390.24 | +3.93 |
| 53 | 701.89 | -0.07 | 384.91 | -1.41 |
| 72 | 700.00 | -1.96 | 383.33 | -2.98 |

---

### Historical Temperaments

#### Meantone Temperament

Meantone tuning (late 15th - early 18th century) features pure major thirds (5:4) with slightly compromised fifths.

**Quarter-comma meantone:**
- Fifths tempered by 1/4 syntonic comma (~5.38 cents flat)
- Fifth size: 696.58 cents
- Pure major thirds: 386.31 cents
- **Wolf fifth:** One interval (typically G#-Eb) is extremely sharp (~737 cents)

**Meantone fifths chain:**
```
Eb - Bb - F - C - G - D - A - E - B - F# - C# - G#
    ↑                                              ↑
    └──────────── Wolf fifth (unusable) ──────────┘
```

#### Well-Temperaments

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

**Kirnberger III**

Johann Philipp Kirnberger, a student of J.S. Bach, developed this temperament:
- Based on meantone but adjusted for wider key use
- Most fifths are pure (3:2)
- Syntonic comma distributed among a few fifths

**Key characteristics of well-temperaments:**
- All keys playable (no wolf)
- Different keys have different "colors"
- Keys near C have purer thirds; remote keys have wider thirds
- Expressive possibilities through key choice

---

### Xenharmonic Scales

#### Bohlen-Pierce Scale

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

#### 833-Cents Golden Scale (Bohlen)

Based on the golden ratio (phi = 1.618...).

**Structure:**
- Period: 833.09 cents (the golden ratio interval)
- 7 unequal steps
- Octaves occur incidentally, not as a structural feature
- Based on combination tones and Fibonacci sequence

**Scale degrees:**
```
0, 91.5, 196.2, 287.7, 392.4, 545.4, 637.0, 833.1
```

---

### Gamelan Tunings

Indonesian gamelan uses two primary tuning systems that vary between ensembles.

#### Slendro

- **Pentatonic** (5 notes per octave)
- Approximately equal spacing within the octave
- Notation: 1, 2, 3, 5, 6 (ji, ro, lu, mo, nem)
- Associated with light, cheerful atmosphere
- Used for war scenes and marching in wayang performances

**Approximate slendro intervals (vary by ensemble):**
```
Note 1: 0 cents
Note 2: ~240 cents
Note 3: ~480 cents
Note 5: ~720 cents
Note 6: ~960 cents
Octave: 1200 cents
```

#### Pelog

- **Heptatonic** (7 notes per octave)
- Uneven intervals
- Often played using 5-note subsets
- Notation: 1, 2, 3, 4, 5, 6, 7 (ji, ro, lu, pat, mo, nem, tu)
- Associated with regal, sacred atmosphere

**Approximate pelog intervals (vary significantly):**
```
Note 1: 0 cents
Note 2: ~120 cents
Note 3: ~270 cents
Note 4: ~425 cents
Note 5: ~550 cents
Note 6: ~670 cents
Note 7: ~800 cents
```

#### Tumbuk (Shared Tone)

When both pelog and slendro are used together, they share one pitch called the **tumbuk**. Common tumbuk positions are 5 and 6.

#### Ensemble-Specific Tuning

Each gamelan ensemble is tuned uniquely:
- Instruments within a set are tuned to each other
- No fixed external pitch standard (unlike A=440)
- Creates distinctive timbre and resonance per ensemble

**Balinese ombak (beating):** Paired instruments are tuned slightly apart to create shimmering interference patterns at consistent speeds across all registers.

---

## 2. File Formats and Standards

### Scala Format (.scl)

The Scala scale file format is the de facto standard for scale exchange, used by the Scala program and supported by hundreds of synthesizers and tools.

#### Complete Specification

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
- Text after valid pitch values is ignored (can be used for comments)

#### Example Files

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

**Quarter-comma meantone:**
```
! meanquar.scl
!
1/4-comma meantone scale. Pietro Aaron's temperament (1523)
12
!
76.04900
193.15686
310.26471
5/4
503.42157
579.47057
696.57843
25/16
889.73529
1006.84314
1082.89214
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

### Keyboard Mapping (.kbm)

The keyboard mapping file defines how scale degrees map to MIDI note numbers.

#### Complete Specification

**File characteristics:**
- Extension: `.kbm`
- Encoding: Pure ASCII
- Comments: Lines starting with `!`

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

#### Parameter Details

| Parameter | Description | Default |
|-----------|-------------|---------|
| Size of map | Number of entries in mapping pattern | 0 (linear) |
| First MIDI note | Start of retuning range | 0 |
| Last MIDI note | End of retuning range | 127 |
| Middle note | MIDI note for degree 0 | 60 |
| Reference note | MIDI note for reference frequency | 69 |
| Reference frequency | Frequency in Hz | 440.0 |
| Octave degree | Scale degree = formal octave | (scale size) |

#### Example Files

**Standard 12-TET mapping (linear):**
```
! Standard keyboard mapping
! Size of map (0 = linear)
0
! First MIDI note to retune
0
! Last MIDI note to retune
127
! Middle note (MIDI note for 1/1)
60
! Reference note
69
! Reference frequency (Hz)
440.0
! Scale degree for formal octave
12
! Mapping (not needed for linear)
```

**7-note scale mapped to white keys:**
```
! 7-note scale on white keys only
! Size of map
12
! First MIDI note
0
! Last MIDI note
127
! Middle note (C = degree 0)
60
! Reference note
69
! Reference frequency
440.0
! Formal octave (7 degrees)
7
! Mapping: C D E F G A B, black keys unmapped
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

**19-TET starting on A:**
```
! 19-TET starting on A=440
0
0
127
69
69
440.0
19
```

---

### AnaMark TUN Format (.tun)

The AnaMark TUN format is widely supported by VST instruments.

#### Version 2.0 Structure

**File characteristics:**
- Extension: `.tun`
- Encoding: ASCII text
- Comments: Lines starting with `;`
- Sections: Denoted by `[SectionName]`
- Case-insensitive section names

**Basic structure:**
```
; Comment line
[Tuning]
; Legacy format: 128 cent values for MIDI notes 0-127
note 0 = xxx.xxxxxx cents
note 1 = xxx.xxxxxx cents
...
note 127 = xxx.xxxxxx cents

[Scale Begin]
; Version 2.0 content

[Info]
Name = "Scale Name"
ID = "unique.identifier"

[Exact Tuning]
note 0 = xxx.xxxxxx cents
...

[Scale End]
```

**Version 2.0 features:**
- Scale description metadata
- Algorithmic scale construction
- Multiple scales per file
- Keyboard mapping support
- Embeddable in other files

#### Example TUN File

```
; 12-TET tuning file
[Tuning]
note 0 = -1200.000000 cents
note 1 = -1100.000000 cents
; ... (continues for all 128 notes)
note 60 = 0.000000 cents
note 61 = 100.000000 cents
note 62 = 200.000000 cents
; ...
note 69 = 900.000000 cents
; ...
note 127 = 6700.000000 cents

[Scale Begin]

[Info]
Name = "12-TET Standard"
ID = "12tet.standard"

[Exact Tuning]
note 0 = -1200.000000 cents
; ... (all 128 notes)

[Scale End]
```

**Compatibility:** Version 2.0 files are backward-compatible with software supporting only Version 1.0.

---

### MTS-ESP Protocol

MTS-ESP (MIDI Tuning Standard - Extended Specification Protocol) enables real-time centralized microtuning control developed by ODDSound.

#### Architecture

**Master plugin:**
- Defines the tuning
- Broadcasts frequency for each MIDI note
- Only one master per session

**Client plugins:**
- Follow the tuning from master
- Query frequencies in real-time
- Update pitch continuously during note playback

#### API Functions (C/C++)

**Client initialization:**
```c
#include "libMTSClient.h"

MTSClient* client = MTS_RegisterClient();
```

**Getting retuned frequency:**
```c
// Get frequency for MIDI note
double freq = MTS_NoteToFrequency(client, note, channel);

// Check if note should be filtered
bool filtered = MTS_ShouldFilterNote(client, note, channel);
```

**Cleanup:**
```c
MTS_DeregisterClient(client);
```

#### Features

- **17 tuning tables:** One per MIDI channel (1-16) plus general table
- **Note filtering:** Master can mark notes as unmapped
- **Fallback:** Defaults to 12-TET when no master connected
- **MTS SysEx parsing:** Client API parses standard MTS messages

#### Resources

- GitHub: https://github.com/ODDSound/MTS-ESP
- Official site: https://oddsound.com

---

### MIDI Tuning Standard (MTS)

The original MIDI Tuning Standard allows SysEx-based microtuning.

#### Resolution

- **14-bit fraction:** 16,384 divisions per semitone
- **Total resolution:** 196,608 parts per octave
- **Precision:** ~0.0061 cents

#### Bulk Tuning Dump Request

```
F0 7E    ; Universal non-realtime SysEx header
[id]     ; Device ID (00-7F, or 7F for all)
08       ; Sub-ID #1: MIDI tuning standard
00       ; Sub-ID #2: Bulk dump request
[tt]     ; Tuning program number (00-7F)
F7       ; End of SysEx
```

#### Bulk Tuning Dump Reply

```
F0 7E          ; Universal non-realtime SysEx header
[id]           ; Device ID
08             ; Sub-ID #1: MIDI tuning standard
01             ; Sub-ID #2: Bulk dump reply
[tt]           ; Tuning program number
[name]         ; 16 ASCII characters (padded with spaces)
[data]         ; 128 x 3 bytes (384 bytes) of frequency data
[checksum]     ; XOR of all bytes except F0, F7, checksum
F7             ; End of SysEx
```

#### Frequency Data Format (3 bytes per note)

```
Byte 1: [0sssssss] - Semitone (MIDI note number, 0-127)
Byte 2: [0fffffff] - Fraction MSB (bits 13-7)
Byte 3: [0fffffff] - Fraction LSB (bits 6-0)

Fraction = (Byte2 << 7) | Byte3
Cents offset = Fraction / 16384 * 100
```

**Encoding example for 440 Hz (A4, note 69):**
```
Byte 1: 0x45 (69 = A4)
Byte 2: 0x00 (no fraction)
Byte 3: 0x00
```

**Encoding for 449.33 Hz (A4 + 36 cents):**
```
Cents offset = 36
Fraction = 36 / 100 * 16384 = 5898
Byte 1: 0x45 (69)
Byte 2: 0x2E (5898 >> 7 = 46)
Byte 3: 0x0A (5898 & 0x7F = 10)
```

#### Real-Time Single Note Tuning

```
F0 7F          ; Universal realtime SysEx header
[id]           ; Device ID
08             ; Sub-ID #1: MIDI tuning standard
02             ; Sub-ID #2: Single note tuning change
[tt]           ; Tuning program number
01             ; Number of notes to change
[note]         ; MIDI note number
[xx yy zz]     ; 3-byte frequency data
F7             ; End of SysEx
```

#### Checksum Calculation

```c
uint8_t calculateChecksum(uint8_t* data, int length) {
    uint8_t checksum = 0;
    for (int i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum & 0x7F;
}
```

---

### MIDI 2.0 Per-Note Pitch

MIDI 2.0 introduces high-resolution per-note pitch control.

#### Note-On Pitch Attribute

- **Resolution:** 7.9 format (7 bits semitone, 9 bits fraction)
- **Precision:** 1/512 of a semitone
- Embedded directly in Note-On message

#### Registered Per-Note Controller #3 (Pitch)

- **Resolution:** 7.25 format
  - 7 bits: Semitones
  - 25 bits: Fraction (1/33,554,432 semitone)
- Real-time control over note lifetime
- Per-note modulation capability

#### Key Advantages over MIDI 1.0

| Feature | MIDI 1.0 | MIDI 2.0 |
|---------|----------|----------|
| Pitch bend resolution | 14-bit global | 32-bit per-note |
| Per-note tuning | Requires SysEx | Native support |
| Real-time changes | Limited | Full support |
| Controller resolution | 7-14 bit | 32-bit |

---

## 3. Mathematical Foundations

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
    // Convert frequency ratio to cents
    static double ratioToCents(double ratio) {
        return 1200.0 * std::log2(ratio);
    }

    // Convert cents to frequency ratio
    static double centsToRatio(double cents) {
        return std::pow(2.0, cents / 1200.0);
    }

    // Convert ratio string "n/d" to cents
    static double parseRatioToCents(const std::string& ratio) {
        size_t slashPos = ratio.find('/');
        if (slashPos != std::string::npos) {
            double num = std::stod(ratio.substr(0, slashPos));
            double den = std::stod(ratio.substr(slashPos + 1));
            return ratioToCents(num / den);
        }
        // Integer ratio (e.g., "2" = "2/1")
        return ratioToCents(std::stod(ratio));
    }

    // Get frequency for a given MIDI note and cents offset
    static double getFrequency(int midiNote, double centsOffset = 0.0,
                               double a4Freq = 440.0) {
        double semitonesFromA4 = midiNote - 69.0 + centsOffset / 100.0;
        return a4Freq * std::pow(2.0, semitonesFromA4 / 12.0);
    }

    // Apply tuning table to get frequency
    static double getTunedFrequency(int midiNote,
                                    const double* tuningTable,
                                    double baseFreq = 261.6255653) {
        // tuningTable[i] = cents offset from 12-TET for note i
        double cents12TET = (midiNote - 60) * 100.0;
        double tunedCents = cents12TET + tuningTable[midiNote];
        return baseFreq * std::pow(2.0, tunedCents / 1200.0);
    }
};
```

#### Extended Precision for MTS

```cpp
// Encode frequency to MTS 3-byte format
void frequencyToMTS(double frequency, uint8_t mtsBytes[3]) {
    double midiNote = 69.0 + 12.0 * std::log2(frequency / 440.0);

    int semitone = static_cast<int>(std::floor(midiNote));
    semitone = std::clamp(semitone, 0, 127);

    double fraction = (midiNote - semitone) * 16384.0;
    int frac14bit = static_cast<int>(std::round(fraction));
    frac14bit = std::clamp(frac14bit, 0, 16383);

    mtsBytes[0] = static_cast<uint8_t>(semitone);
    mtsBytes[1] = static_cast<uint8_t>((frac14bit >> 7) & 0x7F);
    mtsBytes[2] = static_cast<uint8_t>(frac14bit & 0x7F);
}

// Decode MTS 3-byte format to frequency
double mtsToFrequency(const uint8_t mtsBytes[3]) {
    int semitone = mtsBytes[0];
    int fraction = (mtsBytes[1] << 7) | mtsBytes[2];

    double midiNote = semitone + fraction / 16384.0;
    return 440.0 * std::pow(2.0, (midiNote - 69.0) / 12.0);
}
```

---

### Harmonic Lattices and Tonnetz

#### Tonnetz Structure

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
81/64 = 2^(-6) * 3^4 = |-6 4>
```

---

### Temperament Optimization

#### Least Squares Tuning

Minimizes the sum of squared errors from target intervals:

```
minimize: Σ (tuned_interval - target_interval)²
```

**Implementation:**
```cpp
// Find optimal generator for a rank-2 temperament
double optimizeGenerator(const std::vector<double>& targetCents,
                         const std::vector<int>& generatorCounts,
                         double period = 1200.0) {
    // Solve: minimize Σ (g*count[i] - target[i])²
    double sumXY = 0, sumXX = 0;
    for (size_t i = 0; i < targetCents.size(); i++) {
        sumXY += generatorCounts[i] * targetCents[i];
        sumXX += generatorCounts[i] * generatorCounts[i];
    }
    return sumXY / sumXX;  // Optimal generator in cents
}
```

#### Minimax Tuning

Minimizes the maximum error ("weakest link" approach):

```
minimize: max(|tuned_interval - target_interval|)
```

This can be solved as a linear programming problem.

#### Common Optimization Targets

**5-limit tonality diamond:**
```
1/1, 6/5, 5/4, 4/3, 3/2, 8/5, 5/3
```

**7-limit tonality diamond:**
```
1/1, 8/7, 7/6, 6/5, 5/4, 4/3, 7/5, 10/7, 3/2, 8/5, 5/3, 12/7, 7/4
```

---

## 4. Scale Classification

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

#### MOS Properties

```
For generator g and period P:
- Scale size N is valid if N is denominator of a
  convergent or semiconvergent of g/P

Diatonic (fifth = 700¢):
g/P = 700/1200 = 7/12
Convergents: 1/2, 3/5, 7/12
Valid sizes: 2, 5, 7, 12
```

#### Code for MOS Generation

```cpp
#include <vector>
#include <algorithm>

std::vector<double> generateMOS(double generator, double period, int size) {
    std::vector<double> scale;

    for (int i = 0; i < size; i++) {
        double pitch = std::fmod(generator * i, period);
        if (pitch < 0) pitch += period;
        scale.push_back(pitch);
    }

    std::sort(scale.begin(), scale.end());
    return scale;
}

// Get step sizes
std::pair<double, double> getStepSizes(const std::vector<double>& scale,
                                        double period) {
    std::vector<double> steps;
    for (size_t i = 1; i < scale.size(); i++) {
        steps.push_back(scale[i] - scale[i-1]);
    }
    steps.push_back(period - scale.back() + scale[0]);

    double minStep = *std::min_element(steps.begin(), steps.end());
    double maxStep = *std::max_element(steps.begin(), steps.end());

    return {minStep, maxStep};  // s and L
}
```

---

### Rothenberg Propriety

David Rothenberg's 1978 classification system for scales.

#### Definitions

- **Strictly proper:** Every N-step interval is smaller than every (N+1)-step interval
- **Proper:** Allows ambiguities (equal sizes across classes) but no contradictions
- **Improper:** Contains contradictions (some N-step > some (N+1)-step)

#### Examples

**Diatonic scale (proper, not strictly proper):**
- Has one ambiguity: augmented fourth = diminished fifth (both 600¢ in 12-TET)

**Pentatonic scale (strictly proper):**
- All 1-steps < all 2-steps < all 3-steps < all 4-steps

**Hirajoshi scale (improper):**
- Steps: 2, 1, 4, 1, 4 semitones
- Some 1-step intervals (4 semitones) > some 2-step intervals (3 semitones)

#### Rothenberg Efficiency

Measures how quickly a scale's position can be determined:

```
Efficiency = 1 - Redundancy

Redundancy = (information needed) / (maximum possible information)
```

Higher efficiency = easier to orient within the scale.

---

## 5. Implementation Code Examples

### Complete Scala File Parser

```cpp
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>
#include <stdexcept>

struct ScalaScale {
    std::string description;
    std::vector<double> cents;  // Including implicit 0.0 at start

    double getPitch(int degree) const {
        if (degree < 0) {
            int octaves = (-degree) / (cents.size() - 1) + 1;
            degree += octaves * (cents.size() - 1);
        }
        int octave = degree / (cents.size() - 1);
        int idx = degree % (cents.size() - 1);
        return cents[idx] + octave * cents.back();
    }
};

class ScalaParser {
public:
    static ScalaScale parse(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }

        ScalaScale scale;
        scale.cents.push_back(0.0);  // Implicit unison

        std::string line;
        int noteCount = -1;
        bool foundDescription = false;

        while (std::getline(file, line)) {
            // Skip comments
            if (line.empty() || line[0] == '!') continue;

            // Trim whitespace
            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos) continue;
            line = line.substr(start);

            if (!foundDescription) {
                scale.description = line;
                foundDescription = true;
            } else if (noteCount < 0) {
                noteCount = std::stoi(line);
            } else {
                double cents = parsePitch(line);
                scale.cents.push_back(cents);
                if (scale.cents.size() > static_cast<size_t>(noteCount + 1)) {
                    break;
                }
            }
        }

        return scale;
    }

private:
    static double parsePitch(const std::string& line) {
        // Extract first token
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        // Check for ratio (contains /)
        size_t slashPos = token.find('/');
        if (slashPos != std::string::npos) {
            double num = std::stod(token.substr(0, slashPos));
            double den = std::stod(token.substr(slashPos + 1));
            return 1200.0 * std::log2(num / den);
        }

        // Check for cents (contains .)
        if (token.find('.') != std::string::npos) {
            return std::stod(token);
        }

        // Integer ratio (e.g., "2" = "2/1")
        return 1200.0 * std::log2(std::stod(token));
    }
};
```

### KBM Parser

```cpp
struct KeyboardMapping {
    int mapSize = 0;          // 0 = linear mapping
    int firstMIDI = 0;
    int lastMIDI = 127;
    int middleNote = 60;
    int referenceNote = 69;
    double referenceFreq = 440.0;
    int formalOctave = 12;
    std::vector<int> mapping;  // -1 = unmapped

    int mapNote(int midiNote) const {
        if (midiNote < firstMIDI || midiNote > lastMIDI) {
            return -1;  // Outside range
        }

        if (mapSize == 0) {
            // Linear mapping
            return midiNote - middleNote;
        }

        int offset = midiNote - middleNote;
        int octaves = offset >= 0 ? offset / mapSize : (offset - mapSize + 1) / mapSize;
        int idx = ((offset % mapSize) + mapSize) % mapSize;

        if (idx >= static_cast<int>(mapping.size()) || mapping[idx] < 0) {
            return -1;  // Unmapped
        }

        return mapping[idx] + octaves * formalOctave;
    }
};

class KBMParser {
public:
    static KeyboardMapping parse(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }

        KeyboardMapping kbm;
        std::string line;
        int paramIndex = 0;

        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '!') continue;

            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos) continue;
            line = line.substr(start);

            switch (paramIndex++) {
                case 0: kbm.mapSize = std::stoi(line); break;
                case 1: kbm.firstMIDI = std::stoi(line); break;
                case 2: kbm.lastMIDI = std::stoi(line); break;
                case 3: kbm.middleNote = std::stoi(line); break;
                case 4: kbm.referenceNote = std::stoi(line); break;
                case 5: kbm.referenceFreq = std::stod(line); break;
                case 6: kbm.formalOctave = std::stoi(line); break;
                default:
                    // Mapping entries
                    if (line[0] == 'x' || line[0] == 'X') {
                        kbm.mapping.push_back(-1);
                    } else {
                        kbm.mapping.push_back(std::stoi(line));
                    }
                    break;
            }
        }

        return kbm;
    }
};
```

### Complete Tuning Engine

```cpp
class TuningEngine {
private:
    ScalaScale scale;
    KeyboardMapping mapping;
    double tuningTable[128];

public:
    void loadScale(const std::string& sclFile) {
        scale = ScalaParser::parse(sclFile);
        rebuildTable();
    }

    void loadMapping(const std::string& kbmFile) {
        mapping = KBMParser::parse(kbmFile);
        rebuildTable();
    }

    void setReferenceFrequency(double freq) {
        mapping.referenceFreq = freq;
        rebuildTable();
    }

    double getFrequency(int midiNote) const {
        if (midiNote < 0 || midiNote > 127) return 0.0;
        return tuningTable[midiNote];
    }

    double getCentsOffset(int midiNote) const {
        double tunedFreq = getFrequency(midiNote);
        double standardFreq = 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
        return 1200.0 * std::log2(tunedFreq / standardFreq);
    }

private:
    void rebuildTable() {
        for (int note = 0; note < 128; note++) {
            int degree = mapping.mapNote(note);

            if (degree == -1) {
                // Unmapped - use standard 12-TET
                tuningTable[note] = 440.0 * std::pow(2.0, (note - 69) / 12.0);
            } else {
                // Get cents from scale
                double cents = scale.getPitch(degree);

                // Calculate reference point
                int refDegree = mapping.mapNote(mapping.referenceNote);
                double refCents = scale.getPitch(refDegree);

                // Frequency = refFreq * 2^((cents - refCents) / 1200)
                tuningTable[note] = mapping.referenceFreq *
                    std::pow(2.0, (cents - refCents) / 1200.0);
            }
        }
    }
};
```

### MTS-ESP Client Integration

```cpp
#include "libMTSClient.h"

class MTSESPTuning {
private:
    MTSClient* client = nullptr;
    bool connected = false;

public:
    MTSESPTuning() {
        client = MTS_RegisterClient();
        connected = (client != nullptr);
    }

    ~MTSESPTuning() {
        if (client) {
            MTS_DeregisterClient(client);
        }
    }

    bool isConnected() const {
        return connected && MTS_HasMaster(client);
    }

    double getFrequency(int midiNote, int midiChannel = -1) const {
        if (!client) {
            // Fallback to 12-TET
            return 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
        }
        return MTS_NoteToFrequency(client, midiNote, midiChannel);
    }

    bool shouldFilterNote(int midiNote, int midiChannel = -1) const {
        if (!client) return false;
        return MTS_ShouldFilterNote(client, midiNote, midiChannel);
    }

    double getRetuning(int midiNote, int midiChannel = -1) const {
        if (!client) return 0.0;
        return MTS_RetuningInSemitones(client, midiNote, midiChannel);
    }
};
```

---

## 6. References and Resources

### Academic and Theoretical

- [Equal Temperament - Wikipedia](https://en.wikipedia.org/wiki/Equal_temperament)
- [12 Equal Temperament - Wikipedia](https://en.wikipedia.org/wiki/12_equal_temperament)
- [Just Intonation - Wikipedia](https://en.wikipedia.org/wiki/Just_intonation)
- [Tonnetz - Wikipedia](https://en.wikipedia.org/wiki/Tonnetz)
- [Rothenberg Propriety - Wikipedia](https://en.wikipedia.org/wiki/Rothenberg_propriety)
- [MIDI Tuning Standard - Wikipedia](https://en.wikipedia.org/wiki/MIDI_tuning_standard)
- [Bohlen-Pierce Scale - Wikipedia](https://en.wikipedia.org/wiki/Bohlen–Pierce_scale)
- [Werckmeister Temperament - Wikipedia](https://en.wikipedia.org/wiki/Werckmeister_temperament)
- [Kirnberger Temperament - Wikipedia](https://en.wikipedia.org/wiki/Kirnberger_temperament)

### Xenharmonic Resources

- [Xenharmonic Wiki - Main](https://en.xen.wiki/)
- [Xenharmonic Wiki - 12edo](https://en.xen.wiki/w/12edo)
- [Xenharmonic Wiki - Just Intonation](https://en.xen.wiki/w/Just_intonation)
- [Xenharmonic Wiki - MOS Scales](https://en.xen.wiki/w/MOS_scale)
- [Xenharmonic Wiki - 7-limit](https://en.xen.wiki/w/7-limit)
- [Xenharmonic Wiki - Bohlen-Pierce](https://en.xen.wiki/w/Bohlen-Pierce)
- [Xenharmonic Wiki - Pelog](https://en.xen.wiki/w/Pelog)
- [Xenharmonic Wiki - AnaMark Format](https://en.xen.wiki/w/Anamark_tuning_file_format)

### File Format Specifications

- [Scala SCL Format - Huygens-Fokker](https://www.huygens-fokker.org/scala/scl_format.html)
- [Scala Home Page](https://www.huygens-fokker.org/scala/)
- [MTS-ESP GitHub Repository](https://github.com/ODDSound/MTS-ESP)
- [ODDSound MTS-ESP Suite](https://oddsound.com/mtsespsuite.php)
- [AnaMark Tuning Library - GitHub](https://github.com/zardini123/AnaMark-Tuning-Library)
- [Hpi Instruments Tuning Files](https://hpi.zentral.zone/filetypes)

### Tutorials and Guides

- [Kyle Gann - Just Intonation Explained](https://www.kylegann.com/tuning.html)
- [Kyle Gann - Historical Tunings](https://www.kylegann.com/histune.html)
- [Sevish - Mapping Scales in Scala](https://sevish.com/2017/mapping-microtonal-scales-keyboard-scala/)
- [Sevish - Convert TUN to SCL](https://sevish.com/2019/how-to-convert-tun-scl-files/)
- [Erv Wilson - MOS Introduction](https://www.anaphoria.com/wilsonintroMOS.html)
- [Microtonal Synthesis - MTS Tutorial](http://microtonal-synthesis.com/MIDItuning.html)

### Software Tools

- [Scale Workshop - GitHub](https://github.com/SeanArchibald/scale-workshop)
- [Cycling '74 RNBO Scala Reference](https://rnbo.cycling74.com/learn/scala-and-custom-tuning-reference)
- [libscala-file C++ Library](https://github.com/MarkCWirt/libscala-file)

### Mathematical References

- [Tonalsoft Encyclopedia - Lattices](http://tonalsoft.com/monzo/lattices/lattices.aspx)
- [Tonalsoft - Cents Calculator](https://sengpielaudio.com/calculator-centsratio.htm)
- [Mathematics of MOS - Xenharmonic Wiki](https://en.xen.wiki/w/Mathematics_of_MOS)
- [Target Tuning Optimization - Xenharmonic Wiki](https://en.xen.wiki/w/Target_tuning)

### Historical and Cultural

- [Indonesian Gamelan Article](https://www.drumsforschools.com/wp-content/uploads/2019/10/Indonesian-Gamelan-Article-004-Tuning.pdf)
- [Slendro - Wikipedia](https://en.wikipedia.org/wiki/Slendro)
- [Pelog - Wikipedia](https://en.wikipedia.org/wiki/Pelog)
- [Ableton Sundanese Gamelan Guide](https://tuning.ableton.com/sundanese-gamelan/intro-to-sundanese-gamelan/)

---

## Appendix A: Quick Reference Tables

### Common Just Intervals by Limit

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

### EDO Comparison Chart

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

### Scala File Format Quick Reference

```
! comment
Description line
[note count]
[pitch 1 - ratio or cents]
[pitch 2]
...
[pitch N - usually 2/1 or 1200.0]
```

### KBM File Format Quick Reference

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

*Document compiled: January 2026*
*For VST development research - Plugin Freedom System*
