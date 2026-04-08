---
title: "O-Reed: Acoustic Properties of Reed Wind Instruments"
created: 2026-04-04
juce_version: "8.0.4"
summary: "Comprehensive acoustic reference for reed wind instrument physical modeling. Covers single reed, double reed, free reed, and non-western instruments with bore dimensions, reed parameters, harmonic behavior, extended techniques, and material effects."
domain: dsp
type: research
keywords:
  - physical-modeling
  - reed-instrument
  - waveguide
  - clarinet
  - saxophone
  - oboe
  - bassoon
  - duduk
  - zurna
  - hichiriki
  - bore-geometry
  - double-reed
  - single-reed
  - free-reed
  - conical-bore
  - cylindrical-bore
  - extended-techniques
stages: [0]
agents: [research, dsp]
---

# O-Reed: Acoustic Properties of Reed Wind Instruments

## Table of Contents
1. [Single Reed — Cylindrical Bore](#1-single-reed--cylindrical-bore)
2. [Single Reed — Conical Bore](#2-single-reed--conical-bore)
3. [Double Reed — Conical Bore](#3-double-reed--conical-bore)
4. [Free Reed Instruments](#4-free-reed-instruments)
5. [Non-Western Reed Instruments](#5-non-western-reed-instruments)
6. [Key Acoustic Parameters — Comparison Table](#6-key-acoustic-parameters--comparison-table)
7. [What Makes Each Family Distinctive](#7-what-makes-each-family-distinctive)
8. [Extended Techniques for Modeling](#8-extended-techniques-for-modeling)
9. [Material Effects on Timbre](#9-material-effects-on-timbre)
10. [Implications for O-Reed Engine Design](#10-implications-for-o-reed-engine-design)

---

## 1. Single Reed — Cylindrical Bore

### 1.1 Clarinet Family

The clarinet behaves acoustically as a **closed-open pipe**: the mouthpiece end (with reed) is a pressure maximum (closed), the first open tone hole or bell is a pressure minimum (open). This boundary condition is the single most important factor defining the clarinet sound.

#### Why a clarinet sounds like a clarinet

1. **Odd harmonics dominate.** A closed cylindrical pipe supports only odd-numbered resonance modes (1st, 3rd, 5th, 7th...). The even harmonics are weak, particularly in the low (chalumeau) register. This gives the clarinet its characteristic "hollow" or "woody" quality.
2. **Overblowing at the twelfth.** The register key encourages the 3rd harmonic (3:1 ratio = octave + fifth = twelfth), not the 2nd. This is unique among common woodwinds and defines the clarinet's register structure.
3. **Register behavior.** The chalumeau register (low) has the strongest odd-harmonic character. The clarion register (middle, overblown) has a brighter, more complete harmonic series because the higher modes are no longer purely odd multiples of a shortened effective length. The altissimo register uses cross-fingerings and has a complex spectrum.
4. **Mouthpiece correction.** The mouthpiece cavity adds an equivalent volume that slightly modifies the effective length, compensating for the fact that the reed end is not perfectly "closed."

#### Bore Dimensions

| Parameter | Bb Clarinet | A Clarinet | Bass Clarinet | Eb Soprano |
|-----------|-------------|------------|---------------|------------|
| Bore diameter (mm) | 14.65-14.85 | 14.6-14.8 | 23-24 | ~13.5 |
| Effective bore length (cm) | ~66 | ~71 | ~130 | ~47 |
| Barrel length (mm) | 60-66 (typ. 65) | 64-67 | N/A (neck) | ~52 |
| Bell opening diameter (mm) | ~60-70 | ~60-70 | ~100 | ~55 |
| Bell flare | Gentle to pronounced | Gentle | Wide | Moderate |
| Pitch range | D3-Bb6 | C#3-A6 | Bb1-Bb5 | G3-E7 |

#### Reed Properties

- **Material:** Arundo donax cane (standard), synthetic polymers available
- **Reed natural frequency:** ~2000-3000 Hz (adjustable by embouchure; well above playing frequency)
- **Young's modulus (longitudinal):** ~5 GPa
- **Young's modulus (transverse):** ~0.5 GPa (highly anisotropic — 10:1 ratio)
- **Stiffness grades:** 1-5 scale (1 = softest, 5 = hardest); typical orchestral player uses 3-3.5
- **Reed function:** Pressure-controlled valve — the reed vibrates at the frequency of the bore resonance, not its own natural frequency. The bore drives the reed.

#### Mouthpiece Geometry

- Mouthpiece bore is slightly conical (tapering toward the top), rarely cylindrical
- Tip opening: 0.95-1.25 mm (distance from reed tip to mouthpiece tip)
- Facing length: 15-22 mm
- Internal volume: provides the "equivalent volume" correction to the effective closed end

#### Blowing Pressure

- Typical range: 2-5 kPa
- Slight decrease with increasing fundamental frequency (unlike double reeds)
- Piano to forte increase roughly doubles the pressure

---

## 2. Single Reed — Conical Bore

### 2.1 Saxophone Family

The saxophone uses a nearly conical bore with a single reed mouthpiece. Despite being single-reed, the conical bore gives it acoustic behavior closer to the oboe or bassoon than to the clarinet.

#### Why a saxophone sounds like a saxophone

1. **All harmonics present.** A conical bore (open at both ends in acoustic terms) supports both odd and even harmonics. The harmonic series is complete, giving a fuller, brighter sound than the clarinet.
2. **Overblowing at the octave.** The octave key encourages the 2nd harmonic (2:1 ratio), just like oboe and bassoon — not the twelfth like clarinet.
3. **Bright attack transients.** The large mouthpiece cavity combined with the conical bore produces fast, bright attack transients. The reed-mouthpiece assembly has significant volume, which replaces the "missing cone tip" of the truncated conical bore.
4. **Subtone capability.** At low dynamics, the reed does not close fully against the mouthpiece facing. This controlled air leak produces the characteristic airy, breathy "subtone" quality unique to saxophones. The Reynolds number drops, changing the flow regime.
5. **Large bore angle.** The saxophone has a much wider cone angle than oboe or bassoon, meaning it radiates sound more efficiently from the bell, especially at high frequencies.

#### Bore Dimensions

| Parameter | Soprano (Bb) | Alto (Eb) | Tenor (Bb) | Baritone (Eb) |
|-----------|-------------|-----------|------------|---------------|
| Bore length uncoiled (cm) | ~58 | ~74 | ~100-110 | ~132 |
| Bore diameter at neck (mm) | ~9-11.5 | ~12-13 | ~16.5-17.4 | ~19-20 |
| Cone half-angle (deg) | 1.74 | ~1.6 (est.) | 1.52 | ~1.4 (est.) |
| Bell opening diameter (mm) | ~50-65 | ~87 (interior) | ~133-171 | ~200+ |
| Bell flare | Moderate | Pronounced | Large | Very large |
| Pitch range (written) | Bb3-F#6 | Bb3-F#6 | Bb3-F#6 | A2-F#6 |

**Note:** Adolphe Sax's original design specified a 3-degree total taper (1.5-degree half-angle). Modern instruments vary, but the range is approximately 1.4-1.75 degrees half-angle across the family.

#### How bore taper affects the harmonic series

- **Ideal taper:** Produces a 2nd harmonic at exactly twice the fundamental frequency (perfect octave). All higher harmonics are exact integer multiples.
- **Less taper (narrower cone):** Upper register goes sharp relative to lower register. Brighter, more focused sound. Less radiation from bell (more reflections at open end).
- **More taper (wider cone):** Upper register goes flat relative to lower register. Warmer, darker sound. More radiation from bell.
- **Deviations from true cone:** Real saxophone bores deviate from perfect cones, especially near the mouthpiece (where the truncated cone is replaced by the mouthpiece cavity) and near the bell (where flare accelerates). These deviations are used by designers to tune intonation and timbre.

#### Reed and Mouthpiece

- Single cane reed, similar to clarinet but wider and slightly thicker
- Mouthpiece bore diameter: ~15.9 mm (alto), ~17.0 mm (tenor)
- Mouthpiece tip opening: 1.5-2.8 mm (much wider range than clarinet)
- Chamber size significantly affects upper partials and response

#### Blowing Pressure

- Typical: ~2 kPa (soft) to 8 kPa (loud, harsh tone in middle register)
- Pressure tends to increase with pitch in first octave
- Second octave follows clarinet-like pattern (slight decrease with frequency)

---

## 3. Double Reed — Conical Bore

### 3.1 What Makes Double Reeds Different from Single Reeds

The fundamental difference is in the **excitation mechanism**:

1. **Narrower aperture.** Double reeds have a much smaller opening than single reed mouthpieces. Two cane blades beat against each other rather than one blade against a rigid mouthpiece.
2. **High flow resistance.** The long, narrow passage through the double reed introduces significant flow resistance and Bernoulli forces. This creates a more complex pressure-flow relationship.
3. **Reed coupling.** Both blades vibrate, creating a symmetric oscillation mode. The coupling between the two blades adds a characteristic "edge" or "nasal" quality.
4. **Higher blowing pressure.** The narrow reed channel and curved blade geometry require higher blowing pressures, especially for the oboe.
5. **Staple/bocal.** The conical metal tube connecting reed to bore (staple for oboe, bocal for bassoon) provides critical impedance matching. Its dimensions affect intonation by ~5-6 cents per mm of length change.

### 3.2 Oboe

#### Acoustic Character

The oboe's narrow conical bore and tiny double reed produce a characteristically bright, penetrating, and slightly nasal sound. Rich in upper harmonics. The sound cuts through orchestral texture.

#### Bore Dimensions

| Parameter | Value |
|-----------|-------|
| Bore type | Narrow conical |
| Overall length | ~65 cm |
| Bore diameter at top | ~4 mm |
| Bore diameter at bell | ~35-40 mm |
| Cone half-angle | 0.82 deg (main cone), effective 0.71 deg |
| Missing cone length | 82.4 mm |
| Bell shape | Small, gently flared |
| Pitch range | Bb3-A6 |

#### Reed Properties

- Width: ~7 mm
- Length (vibrating): ~10-12 mm
- Staple length: 46-47 mm
- Staple tip opening: 2.4-2.6 mm wide, ~1.9 mm high
- Staple bottom diameter: 4.6-5.0 mm
- Reed closing time: 0.4-0.5 ms

#### Blowing Pressure

- Systematically increases with fundamental frequency
- Higher than clarinet: approximately 3-7 kPa
- The small aperture requires precise pressure control

### 3.3 Bassoon

#### Acoustic Character

The bassoon's long folded conical bore produces a warm, reedy, slightly buzzy sound. The bore doubles back on itself (the "boot" joint), creating a compact instrument from a very long air column. More mellow than the oboe, with a rich low register.

#### Bore Dimensions

| Parameter | Value |
|-----------|-------|
| Bore type | Long narrow conical, folded |
| Overall instrument length | ~135 cm |
| Unfolded bore length | ~250-260 cm |
| Bore diameter at bocal tip | ~4 mm |
| Bore diameter at bell | ~40 mm |
| Cone half-angle | 0.41 deg |
| Bell opening | ~45-70 mm |
| Bell shape | Open, minimal flare |
| Pitch range | Bb1-Eb5 |

#### Reed Properties

- Width: 13.5-15.9 mm (much wider than oboe)
- Length: ~27-30 mm (vibrating portion)
- Bocal: curved metal tube, ~25 cm long, highly critical for intonation
- The wide reed and narrow bore combine to produce a distinctive "buzzy" quality in the low register

### 3.4 English Horn (Cor Anglais)

#### Acoustic Character

More mellow and plaintive than the oboe. Essentially an alto oboe in F, pitched a fifth lower. The pear-shaped bell is the defining feature — it creates a more enclosed, filtered sound compared to the oboe's open bell.

#### Bore Dimensions

| Parameter | Value |
|-----------|-------|
| Bore type | Conical (wider than oboe) |
| Overall length | ~90-95 cm |
| Bore diameter | Wider than oboe throughout |
| Cone half-angle | ~0.65-0.75 deg (est., more gradual than oboe) |
| Bell shape | **Pear-shaped (bulb bell)** — distinctive |
| Pitch range | E3-C6 (written F3-C#6) |

The pear-shaped bell acts as a low-pass filter compared to a conventional flared bell, contributing to the instrument's darker timbre. The lowest tones are produced much more easily than on the oboe.

---

## 4. Free Reed Instruments

### 4.1 Excitation Mechanism

Free reeds operate fundamentally differently from beating reeds (single/double):

- **Beating reed (clarinet/oboe):** Reed beats against a fixed surface (mouthpiece lay or other blade). Air flow is periodically interrupted by the reed closing against the lay. The bore resonance controls the vibration frequency.
- **Free reed (harmonica/accordion):** Reed vibrates freely through a slot in a frame, alternately blocking and unblocking airflow. Sound production is like a siren — the airstream is "chopped" into pulses by the oscillating reed.

### 4.2 Key Differences for Modeling

| Property | Beating Reed | Free Reed |
|----------|-------------|-----------|
| Pitch determination | Bore resonance (primarily) | Reed's own natural frequency |
| Reed-bore coupling | Strong — bore drives reed | Weak (Western) to moderate (Asian) |
| Player control | Embouchure + air pressure | Air pressure + tongue blocking |
| Harmonic content | Rich, bore-dependent | Dominated by fundamental transverse beam mode |
| Higher modes present | Yes, controlled by bore | First torsional mode + higher transverse modes at low amplitude |
| Dynamic range | Wide | Moderate |

### 4.3 Western Free Reeds

**Harmonica, Accordion, Harmonium, Reed Organ**

- The Western free reed can sound on its own without a resonating pipe
- Motion dominated by fundamental transverse beam mode
- Higher transverse and first torsional modes present even at low amplitude during steady oscillation
- Pitch determined primarily by reed dimensions and mass (not air column)
- Air column in harmonica cells provides some resonance enhancement but does not control pitch

### 4.4 Asian Free Reeds

**Sheng (Chinese), Khaen (Thai/Lao), Sho (Japanese)**

- The Asian free reed **requires a resonating pipe** to sound
- Reed-pipe coupling is essential — the pipe resonance interacts with the reed
- This makes them acoustically intermediate between Western free reeds and beating-reed instruments
- The sheng is one of the oldest known instruments using free reeds (3000+ years)

### 4.5 Modeling Implications

For O-Reed, free reeds represent the opposite end of the reed-coupling spectrum from double reeds. The key parameter is **reed-bore coupling strength**:
- Double reed (oboe): very strong coupling, bore dominates
- Single reed (clarinet): strong coupling, bore dominates
- Asian free reed (sheng): moderate coupling, both contribute
- Western free reed (harmonica): weak coupling, reed dominates

---

## 5. Non-Western Reed Instruments

### 5.1 Duduk (Armenian)

| Property | Value |
|----------|-------|
| Reed type | Large double reed (ghamish), made from single piece of bamboo/cane |
| Bore shape | **Cylindrical** (unusual for double reed) |
| Bore material | Apricot wood (Prunus armeniaca) |
| Body length | 28-40 cm (standard in A: ~36 cm; piccolo in F: 26 cm; bass in A: 64 cm) |
| Reed length | 8-10.5 cm (varies by key) |
| Reed width | 19-32 mm |
| Reed playing gap | ~2 mm when moistened |
| Finger holes | 10 (8 front, 2 back) |
| Pitch range | ~1 octave + a fourth |
| Cultural context | Armenian folk/classical music, UNESCO intangible heritage |

**What makes it distinctive:**
- The combination of cylindrical bore + large double reed is acoustically unusual. Most double reeds use conical bores.
- Apricot wood provides optimal density and resonance properties, contributing warmth.
- The large reed relative to body size gives exceptional dynamic control and pitch flexibility.
- Below the beating-reed threshold (low dynamics), spectrum contains mainly 1st and 2nd harmonics. Above the threshold, timbre changes dramatically with more harmonics (per Acta Acustica 2024 study by Maugeais & Dalmont).
- Sound is often described as "warm," "mellow," "human-voice-like" — closer to English horn than oboe.

### 5.2 Zurna / Surnai (Turkish/Persian)

| Property | Value |
|----------|-------|
| Reed type | Double reed (kalem), very tight/short, cane or wheat straw |
| Bore shape | Conical |
| Bore material | Hardwood (plum, apricot, or walnut) |
| Body length | 25-35 cm |
| Bell | Flared wooden bell, sometimes with metal rim |
| Finger holes | 7 front + 1 thumb + small tuning holes near bell (filled with beeswax) |
| Pitch range | ~2 octaves |
| Cultural context | Turkish/Persian/Central Asian outdoor ceremonies, weddings, military music |

**What makes it distinctive:**
- Extremely loud and piercing — designed for outdoor use
- Reed is inserted fully into the player's mouth (free-blowing technique)
- Requires high blowing pressure to produce any tone at all
- Almost constantly loud — very limited soft dynamics
- Wide conical bore angle enhances projection
- Tuning holes near bell filled with beeswax for fine-tuning
- Variants: Persian Sorna, Turkish Zurna, Armenian Surna (slightly different tunings and bore profiles)

### 5.3 Shehnai (Indian)

| Property | Value |
|----------|-------|
| Reed type | Quadruple reed (one set of 4 reeds, not double as sometimes described) |
| Bore shape | Conical (gradually broadening) |
| Bore material | Wood |
| Body length | 30-50 cm |
| Bell | Metal or wooden flared bell |
| Finger holes | 6-8 (keyless) |
| Pitch range | A3-A5 (~2 octaves) |
| Cultural context | Indian classical music, wedding ceremonies (Bismillah Khan tradition) |

**What makes it distinctive:**
- Powerful, nasal quality with strong projection
- The quadruple reed (4 layers) creates a uniquely complex excitation
- Similar to zurna in overall form but with different tone hole geometry producing a more refined, less harsh sound
- Integral to North Indian (Hindustani) classical music — capable of melodic subtlety despite loud nature

### 5.4 Suona (Chinese)

| Property | Value |
|----------|-------|
| Reed type | Double reed (small, on copper tube/staple) |
| Bore shape | Conical wooden body |
| Bore material | Hardwood (rosewood, mahogany) with copper tube overlay |
| Body length | 30-45 cm (varies by size/key) |
| Bell | **Metal (brass) flared bell** — distinctive |
| Finger holes | 8 (7 front + 1 thumb) |
| Pitch range | ~2 octaves |
| Cultural context | Chinese folk, ceremonial, military, opera music. Most common Chinese double-reed instrument. |

**What makes it distinctive:**
- Extremely loud and shrill — can be heard over large ensembles
- Metal bell amplifies and brightens the sound significantly
- Frequent use of tonguing in performance
- Conical wooden body + metal bell creates a hybrid acoustic character
- Wide dynamic range from soft to powerful blasts
- Originally from Arabia, adopted in China since 16th century

### 5.5 Hichiriki (Japanese)

| Property | Value |
|----------|-------|
| Reed type | Double reed (flat), large relative to body |
| Bore shape | **Cylindrical with reverse taper** (narrows toward lower end) — highly unusual |
| Bore material | Bamboo, lacquered |
| Body length | 18 cm |
| Reed length | ~5-6 cm |
| Finger holes | 9 (7 front + 2 back) |
| Pitch range | F4-A5 (narrow, approximately one octave) |
| Cultural context | Japanese gagaku (court music), one of two main melodic instruments alongside ryuteki |

**What makes it distinctive:**
- The reverse conical bore (wider at top, narrowing at bottom) is essentially unique among reed instruments
- Despite being a double reed instrument, the cylindrical/reverse-conical bore gives it tonal similarity to a clarinet
- **Embai** ("salted plum seasoning"): characteristic pitch-gliding ornament controlled entirely by embouchure
- Pitch and ornamentation controlled largely by embouchure, not fingering
- Loose but controlled embouchure produces characteristic sound
- Extremely expressive pitch bending — can bend individual notes over a wide range
- Loud, piercing, reedy tone that cuts through the gagaku ensemble

### 5.6 Arghul (Arabic)

| Property | Value |
|----------|-------|
| Reed type | Single reed (idioglot, cut from the pipe itself) |
| Bore shape | Cylindrical (both pipes) |
| Bore material | Bamboo/cane |
| Construction | **Two pipes**: melody pipe (5-7 holes) + longer drone pipe (no holes) |
| Variants | Small arghul (~40 cm), medium, large arghul (~130 cm drone pipe) |
| Pitch range | ~1.5 octaves on melody pipe |
| Cultural context | Egyptian/Levantine folk music, belly dance, ancient origins |

**What makes it distinctive:**
- Drone + melody combination from a single instrument
- Tone similar to clarinet (cylindrical bore + single reed) but more "buzzy"
- Requires circular breathing for continuous sound
- Deep, meditative drone creates otherworldly resonance
- The drone pipe can be very long (over 1 meter in the large arghul), producing a deep bass drone

### 5.7 Mijwiz (Arabic)

| Property | Value |
|----------|-------|
| Reed type | Single reed (idioglot) |
| Bore shape | Cylindrical |
| Bore material | Bamboo/cane |
| Construction | Two pipes of **equal length** (unlike arghul), 5-6 holes each |
| Body length | ~30-35 cm per pipe |
| Cultural context | Levantine folk music, dabke dance |

**What makes it distinctive:**
- Both pipes play melody simultaneously (parallel unison or close intervals)
- Bright, buzzing, lively tone — much more energetic than arghul
- Rapid, rhythmic melodies characteristic of dabke music
- Requires circular breathing
- Two pipes slightly detuned create natural beating/chorusing effect

### 5.8 Piri (Korean)

| Property | Value |
|----------|-------|
| Reed type | Large double reed |
| Bore shape | Cylindrical |
| Bore material | Bamboo |
| Body length | 25-30 cm |
| Bore diameter | ~6 mm |
| Wall thickness | ~2 mm |
| Reed length | ~7 cm (hyang-piri) |
| Finger holes | 8 (7 front + 1 back) |
| Pitch range | Ab3-C5 (expandable upward with reed pressure manipulation) |
| Cultural context | Korean court and folk music |

**What makes it distinctive:**
- Three variants: hyang-piri (court, widest bore), se-piri (chamber, thinner), dae-piri (folk, largest)
- Cylindrical bore + double reed (similar to duduk acoustically)
- Significant pitch bending through reed pressure manipulation
- Can extend range upward by approximately a fourth through embouchure alone
- No bell — straight cylindrical tube

### 5.9 Pungi / Been (Indian Snake Charmer)

| Property | Value |
|----------|-------|
| Reed type | Single or double reed (varies), traditionally palm leaf |
| Bore shape | Cylindrical pipes in gourd resonator |
| Bore material | Bamboo pipes, dried gourd (bottle gourd or coconut shell) air chamber |
| Construction | Gourd resonator + two bamboo pipes (jivala): one melody (5-9 holes), one drone |
| Body length | ~40-50 cm total |
| Pitch range | ~1.5 octaves |
| Cultural context | Indian snake charmer tradition (Sapera community), Sindh/Rajasthan |

**What makes it distinctive:**
- Gourd acts as wind chamber/pressure reservoir, not a resonator in the traditional sense
- Two simultaneous voices: continuous drone + melody
- High, thin, nasal tone
- Requires circular breathing
- The gourd creates a unique coupling between the two reed pipes
- Reedy, buzzy character rich in overtones

### 5.10 Alboka (Basque)

| Property | Value |
|----------|-------|
| Reed type | Single reed (idioglot, cut from cane) |
| Bore shape | Cylindrical (two small-diameter melody pipes) |
| Bore material | Cane/bamboo melody pipes |
| Construction | Two melody pipes with finger holes, set in animal horn mouthpiece at one end, animal horn bell at other |
| Pitch range | ~1 octave |
| Cultural context | Basque folk music tradition |

**What makes it distinctive:**
- Animal horn at both ends: mouthpiece horn allows circular breathing without cheek puffing, bell horn amplifies
- Two parallel pipes create natural harmonies and beating effects
- Single-reed idioglot construction (reed cut from the pipe material)
- Compact, portable instrument
- The horn mouthpiece is a windcap — the player does not touch the reed directly

### 5.11 Launeddas (Sardinian)

| Property | Value |
|----------|-------|
| Reed type | Single reed (idioglot, 3 reeds — one per pipe) |
| Bore shape | Cylindrical (all three pipes) |
| Bore material | Arundo donax (tumbu) and Arundo pliniana (chanters) |
| Construction | **Three pipes**: tumbu (drone, no holes) + mancosa (melody, 5 holes) + mancosedda (counter-melody, 5 holes) |
| Pitch range | Varies by "cuntzertus" (tuning system) — many traditional configurations |
| Cultural context | Sardinian folk music, one of oldest known polyphonic instruments (~3000 years) |

**What makes it distinctive:**
- **Triple-pipe polyphony**: drone + two independent melodies playing in thirds and sixths
- Requires circular breathing (essential, not optional — pieces can last 10+ minutes without pause)
- Reeds weighted with calibrated beeswax for tuning
- Extremely ancient — Bronze Age Sardinian statuettes show the instrument
- The three-voice texture from a single player is unique among wind instruments
- Rectangular finger holes (not round)
- Multiple traditional tuning systems (cuntzertus) for different musical contexts

### 5.12 Other Notable Reed Instruments

**Ney with Reed Mouthpiece (some Middle Eastern traditions):**
While the ney is primarily an air-jet (flute-family) instrument, some regional variants use a reed insert or a head joint modification that introduces reed-like behavior. The Turkish ney uses the player's lips as an "air reed" against the rim. The Persian ney sometimes uses a brass or horn mouthpiece. These are acoustically different from true reed instruments.

**Guan / Bili (Chinese):**
Cylindrical bore double reed, similar in concept to the hichiriki. The guanzi has a short cylindrical bamboo body (~20 cm) with a large double reed. It is the ancestor of the hichiriki.

**Rhaita (Moroccan):**
Double reed, conical bore, similar to the zurna family. Used in Moroccan Gnawa and ceremonial music. Features a large wooden bell.

**Mey/Balaban (Turkish/Azerbaijani):**
Cylindrical bore double reed, very similar to the duduk. Often considered the same instrument family. Typically made of plum or mulberry wood rather than apricot.

---

## 6. Key Acoustic Parameters — Comparison Table

### 6.1 Western Instruments

| Parameter | Bb Clarinet | Alto Sax | Tenor Sax | Oboe | Bassoon | English Horn |
|-----------|-------------|----------|-----------|------|---------|--------------|
| Bore type | Cylindrical | Conical | Conical | Narrow conical | Narrow conical (folded) | Conical (wider than oboe) |
| Bore length (cm) | ~66 | ~74 | ~100-110 | ~65 | ~250-260 (unfolded) | ~90-95 |
| Bore diameter narrow end (mm) | ~14.7 | ~12 | ~17 | ~4 | ~4 (bocal tip) | ~5 |
| Bore diameter wide end (mm) | ~60 (bell) | ~87 (bell) | ~150 (bell) | ~35-40 | ~40 (bell) | ~50-60 (bulb bell) |
| Cone half-angle (deg) | N/A (cyl.) | ~1.6 | ~1.52 | 0.82 | 0.41 | ~0.65-0.75 |
| Reed type | Single | Single | Single | Double | Double | Double |
| Reed natural freq. (Hz) | 2000-3000 | ~2000-3000 | ~1500-2500 | >3000 | ~1500-2500 | ~2500-3500 |
| Reed stiffness (grade) | 2.5-4 | 2-3.5 | 2-3.5 | N/A (hand-made) | N/A (hand-made) | N/A (hand-made) |
| Blowing pressure (kPa) | 2-5 | 2-8 | 2-8 | 3-7 | 3-6 | 3-6 |
| Pitch range (concert) | D3-Bb6 | Db3-Ab5 | Ab2-E5 | Bb3-A6 | Bb1-Eb5 | E3-C6 |
| Characteristic harmonics | **Odd** (low reg.) | All | All | All | All | All |
| Overblowing interval | 12th | Octave | Octave | Octave | Octave | Octave |
| Bell flare | Gentle-moderate | Pronounced | Large | Small | Minimal | **Pear-shaped (bulb)** |

### 6.2 Non-Western Instruments

| Parameter | Duduk | Zurna | Shehnai | Suona | Hichiriki | Piri | Arghul |
|-----------|-------|-------|---------|-------|-----------|------|--------|
| Bore type | Cylindrical | Conical | Conical | Conical | Reverse conical | Cylindrical | Cylindrical |
| Bore length (cm) | 28-40 | 25-35 | 30-50 | 30-45 | 18 | 25-30 | 40-130 (with drone) |
| Bore diameter (mm) | ~12-15 (est.) | ~8-15 (est.) | ~10-20 (est.) | ~10-20 (est.) | ~9-13 (est.) | ~6 | ~8-10 |
| Cone half-angle (deg) | N/A | ~2-3 (est.) | ~1.5-2.5 (est.) | ~2-3 (est.) | Negative (reverse) | N/A | N/A |
| Reed type | Large double | Small double | Quadruple | Double | Double | Double | Single (idioglot) |
| Reed length (cm) | 8-10.5 | ~2-3 | ~3-5 | ~2-3 | ~5-6 | ~7 | Cut from pipe |
| Pitch range | 1 oct + 4th | ~2 oct | ~2 oct | ~2 oct | ~1 oct | ~1.5 oct | ~1.5 oct (melody) |
| Typical volume | Soft-moderate | **Very loud** | **Loud** | **Very loud** | Moderate-loud | Moderate | Moderate |
| Bell | None (open end) | Flared wood | Flared metal/wood | **Flared metal** | None | None | None |
| Drone capability | No | No | No | No | No | No | **Yes (drone pipe)** |
| Circular breathing | Not required | Sometimes | Sometimes | Sometimes | No | No | **Required** |

---

## 7. What Makes Each Family Distinctive

### 7.1 Perceptual Qualities and Their Acoustic Causes

| Instrument | Perceptual Quality | Primary Acoustic Cause |
|------------|-------------------|----------------------|
| Clarinet | Hollow, woody, dark (low reg.) | Odd harmonics from cylindrical closed-pipe behavior |
| Clarinet | Bright, singing (high reg.) | More complete harmonic series in overblown registers |
| Saxophone | Full, bright, punchy | Complete harmonic series from conical bore + wide angle |
| Saxophone | Airy, breathy (subtone) | Incomplete reed closure at low dynamics |
| Oboe | Bright, nasal, penetrating | Narrow conical bore + small double reed, rich upper harmonics |
| Bassoon | Warm, buzzy, reedy | Very long narrow conical bore, wide double reed |
| English horn | Mellow, plaintive, dark | Wider bore than oboe + pear-shaped bell (low-pass filtering) |
| Duduk | Warm, human-voice-like | Cylindrical bore + huge double reed, limited harmonics at low dynamics |
| Zurna | Piercing, harsh, powerful | Wide conical bore + high blowing pressure, maximum radiation |
| Hichiriki | Crying, bending, ethereal | Reverse conical bore + expressive embouchure control |

### 7.2 Parameters for Morphing Between Instruments

These parameters can be **smoothly interpolated** to morph between instrument characters:

1. **Cone angle** (0 = cylindrical clarinet-like, increasing = more conical saxophone/oboe-like)
2. **Bore length** (pitch and register behavior)
3. **Reed stiffness / compliance** (softer = more responsive, harder = brighter attack)
4. **Reed opening area** (larger = more air, louder, breathier)
5. **Reed damping (Q-factor)** — range from ~1.25 to ~5; higher Q = less damped = easier high register access
6. **Blowing pressure** (dynamics and timbre)
7. **Bell flare rate** (more flare = more high-frequency radiation = brighter)
8. **Bore losses / wall damping** (more loss = darker, less sustain)
9. **Reed-bore coupling strength** (stronger = bore dominates more, weaker = reed-like)

### 7.3 Parameters That Create Discontinuities

These parameters **cannot be smoothly morphed** — they represent qualitative regime changes:

1. **Single vs. double reed excitation.** The reed-mouthpiece coupling topology is fundamentally different. A single reed against a rigid lay has different nonlinear characteristics than two flexible blades beating against each other. **Workaround:** Use a parameterized nonlinear reed model where "reed symmetry" crossfades between asymmetric (single) and symmetric (double) oscillation modes.

2. **Overblowing interval (12th vs octave).** The transition from cylindrical (odd harmonics, overblow at 12th) to conical (all harmonics, overblow at octave) is gradual in theory but produces a perceptible "jump" in register behavior at intermediate bore shapes. **Workaround:** This naturally emerges from the bore simulation — a slightly conical bore already shifts the overblowing interval. The key is accurate bore impedance calculation.

3. **Beating reed vs. free reed.** The transition from a reed that beats against a surface to one that oscillates freely through a slot is a topological change in the mechanical model. **Workaround:** Use the reed-bore coupling parameter. At zero coupling, the reed is effectively "free."

4. **Drone pipe presence.** Instruments like arghul and launeddas have parallel air columns that interact. This cannot be modeled by a single bore — it requires a second parallel waveguide. **Workaround:** O-Reed could offer a "drone" mode that activates a second, simplified bore model.

---

## 8. Extended Techniques for Modeling

### 8.1 Multiphonics

**Mechanism:** Non-standard fingerings create bore impedance peaks at two or more non-harmonically-related frequencies simultaneously. The nonlinear reed excitation couples energy into both resonances.

**Modeling approach:**
- Requires accurate tonehole lattice model (not just effective tube length)
- The reed nonlinearity is what generates the combination tones
- Works better with vibrant reeds, more reed in mouth, looser embouchure
- Louder and more stable on double reeds than single reeds
- Key parameters: fingering (which sets impedance peaks), reed damping, blowing pressure

### 8.2 Flutter Tonguing

**Mechanism:** The player's tongue rapidly interrupts the airstream (like rolling an R). This creates amplitude modulation of the sound at the tongue flutter rate (~20-30 Hz).

**Modeling approach:**
- Modulate the blowing pressure with a low-frequency (~25 Hz) oscillation
- Alternatively, modulate the reed opening periodically
- In middle clarinet register, higher partials disappear compared to normal tone
- Works across all wind instruments
- Two methods: lingual flutter (tongue behind reed) or uvular flutter (throat growl)

### 8.3 Circular Breathing

**Mechanism:** Player inhales through nose while pushing stored cheek air through the instrument, maintaining continuous sound.

**Modeling approach:**
- Momentary pressure dip during the transition from lung air to cheek air
- Slight pitch dip at reconnection point (typically a few cents)
- Embouchure tightens momentarily to compensate
- Model as a brief (~50-100 ms) pressure envelope dip
- Essential for: arghul, mijwiz, launeddas, pungi, alboka (these instruments are designed for it)
- Optional/advanced for: clarinet, saxophone, oboe, duduk

### 8.4 Growling (Vocal Fold Coupling)

**Mechanism:** Player hums or sings into the instrument while playing. The vocal folds produce a second fundamental frequency that interacts with the reed/bore system.

**Modeling approach:**
- Add a second oscillator (vocal fold model) coupled to the input of the bore
- The two frequencies produce sum and difference tones (intermodulation)
- Creates characteristic "dirty" or "rough" quality
- When sung note is within harmonic series of played note: reinforcement
- When sung note is at non-harmonic interval: beating, roughness, multiphonic-like textures
- The vocal tract acts as a second resonator upstream of the reed

### 8.5 Slap Tongue

**Mechanism:** The tongue is pressed against the reed and rapidly pulled away, creating a suction release that produces a percussive "pop" followed by a brief pitched resonance.

**Modeling approach:**
- Model as an impulse excitation of the bore (like plucking a string)
- The bore rings at its natural resonance frequencies, decaying quickly
- Most effective and loudest in the low register
- No sustained airflow — purely transient excitation
- Can be combined with normal blowing for "slap + sustain" articulation

### 8.6 Key Clicks / Pad Sounds

**Mechanism:** Mechanical noise from keys striking tone holes or pad cups striking the bore. Also, the brief pressure transient when a key opens or closes affects the standing wave.

**Modeling approach:**
- Short broadband noise burst, filtered by the bore resonance
- Duration: ~5-20 ms
- Can be modeled as a small perturbation to one or more tone holes in the waveguide model
- Important for realism in fast passages — the build-up of different harmonics is preceded by inharmonic noise transients that are very important for perceived quality

### 8.7 Subtone (Saxophone)

**Mechanism:** At low dynamics, the reed does not close fully against the mouthpiece facing. Air leaks around the reed during what would normally be the "closed" phase of oscillation.

**Modeling approach:**
- Modify the reed nonlinearity: instead of the reed fully closing, leave a minimum gap
- This reduces higher harmonics and adds turbulence noise (the "airy" quality)
- Easier with larger tip openings and slightly too-hard reeds
- The flow regime changes (lower Reynolds number)
- Key parameter: minimum reed opening (0 = normal, increasing = more subtone)

### 8.8 Pitch Bending

**Mechanism:** Embouchure changes (lip pressure, jaw position) alter the effective reed stiffness and the acoustic termination at the reed end. Some instruments (especially hichiriki, duduk) are designed for extensive pitch bending.

**Modeling approach:**
- Vary reed stiffness and/or reed opening in real-time
- Hichiriki embai: continuous pitch slide over a semitone or more, controlled by lip pressure
- Duduk: similar wide-range bending, aided by the large reed
- Clarinet/sax: lip bending typically limited to ~quarter tone
- Cross-fingerings can also produce intermediate pitches (partial hole covering)

### 8.9 Overblowing and Cross-Fingerings

**Mechanism:** Register keys or cross-fingerings weaken the fundamental and promote higher harmonics. Cross-fingerings partially open holes at specific positions along the bore to disrupt certain modes.

**Modeling approach:**
- Requires accurate tonehole lattice model
- Register key = small vent hole that disrupts fundamental node
- Cross-fingering = closing holes below an open hole, creating a second resonating section
- Clarinet: overblows at 12th (3rd harmonic), requires different fingerings for second register
- Conical instruments: overblow at octave (2nd harmonic), register key works similarly to octave key
- Quarter tones: achieved by partial hole covering or special cross-fingerings

### 8.10 Summary Table: Extended Techniques as Model Parameters

| Technique | Model Parameter | Control Rate | Applicable Instruments |
|-----------|----------------|-------------|----------------------|
| Multiphonics | Tonehole configuration | Note-rate | All (esp. clarinet, oboe) |
| Flutter tongue | Pressure modulation ~25Hz | LFO-rate | All |
| Circular breathing | Pressure envelope dip | Phrase-rate | All (esp. drone instruments) |
| Growling | Second oscillator coupling | Audio-rate | All |
| Slap tongue | Impulse excitation | Note-rate | Clarinet, saxophone |
| Key clicks | Noise burst + tonehole perturbation | Note-rate | All keyed instruments |
| Subtone | Minimum reed opening | Continuous | Saxophone (primarily) |
| Pitch bending | Reed stiffness modulation | Continuous | All (esp. hichiriki, duduk) |
| Overblowing | Register vent + fingering | Note-rate | All |

---

## 9. Material Effects on Timbre

### 9.1 The Academic Consensus

**After careful scientific study, the material of the bore has no direct influence on timbre.** Acoustics is first a question of bore geometry. Blindfold tests show no discernible difference in timbre among instrument materials (wood vs. metal vs. plastic) when bore geometry is held constant.

Key findings:
- Wood, metal, and plastic clarinets with identical bore geometry produce the same sound to listeners
- The player perceives a difference because the material vibrates (sympathetically) and this vibration is felt through the fingers, teeth, and facial bones
- This tactile feedback affects how the player plays, which in turn affects the sound
- But the material itself does not directly change the radiated sound

### 9.2 What Material Actually Affects

| Factor | Does It Matter? | Why |
|--------|----------------|-----|
| Bore geometry (shape, taper) | **YES — dominant factor** | Determines resonance frequencies, harmonic structure, radiation |
| Tone hole positions | **YES** | Determines tuning, register behavior, cross-fingering options |
| Bell shape and flare | **YES** | Determines radiation efficiency vs. frequency, affects lowest notes most |
| Reed properties | **YES** | Stiffness, damping, mass determine excitation characteristics |
| Surface finish / porosity | **Minor** | Affects wall losses and turbulence near surface; measurable but small |
| Bore material (wood/metal/plastic) | **No direct effect** | Bore walls are acoustically rigid at relevant frequencies |
| Manufacturing precision | **YES (indirect)** | Different materials lead to different achievable tolerances — a well-made wood clarinet has tighter tolerances than a cheap plastic one |

### 9.3 Reed Material: Cane vs. Synthetic

| Property | Arundo donax (Cane) | Synthetic (Polymer) |
|----------|-------------------|-------------------|
| Young's modulus (longitudinal) | ~5 GPa | Designed to match |
| Anisotropy ratio (Ex/Ey) | ~10:1 | ~2-4:1 (less anisotropic) |
| Moisture sensitivity | High (must be wet to play) | None |
| Consistency | Variable (each reed different) | Very consistent |
| Lifespan | Days to weeks | Months to years |
| Player preference | ~90%+ of professionals | Growing adoption |
| Stiffness matching | Similar at same grade | Similar at same grade |
| Damping characteristics | Complex, moisture-dependent | More uniform |

**For physical modeling:** The key measurable difference is the anisotropy ratio. Cane reeds are highly anisotropic (10:1 longitudinal:transverse stiffness), while synthetics are less so (2-4:1). This affects higher vibrational modes of the reed but has minimal effect on the fundamental oscillation mode that dominates sound production. The model should parameterize **reed stiffness** and **reed damping** independently rather than trying to model specific materials.

### 9.4 Implications for O-Reed

**Material modeling is unnecessary for O-Reed.** The engine should focus on:

1. **Bore geometry** (shape, length, taper, tone holes, bell flare)
2. **Reed mechanics** (stiffness, damping, opening, mass)
3. **Excitation nonlinearity** (single vs. double, reed-bore coupling)
4. **Radiation** (bell impedance matching, directivity)

These four domains fully determine the instrument's character. "Material" is a user-facing metaphor that could be mapped to bore loss/damping coefficients if desired, but it should not be modeled as a distinct physical parameter.

---

## 10. Implications for O-Reed Engine Design

### 10.1 Core Model Requirements

Based on this research, the O-Reed engine needs:

1. **Parameterized bore model** — must handle cylindrical, conical, and arbitrary profiles including:
   - Straight cylindrical (clarinet, duduk, piri)
   - Conical (saxophone, oboe, zurna, suona)
   - Folded conical (bassoon)
   - Reverse conical (hichiriki)
   - Compound profiles (cylindrical + bell flare)

2. **Parameterized reed model** — single degree-of-freedom oscillator with:
   - Stiffness (compliance)
   - Damping (Q-factor, range 1.25-5)
   - Opening area at rest
   - Mass per unit area
   - Symmetry parameter (single vs. double reed behavior)
   - Reed-bore coupling strength

3. **Nonlinear excitation** — the reed-mouthpiece flow characteristic:
   - Pressure-flow curve with tunable parameters
   - Must support both beating-reed (closes against lay) and free-reed (oscillates through slot) regimes
   - Subtone mode (minimum opening parameter)

4. **Tonehole lattice** — for accurate multiphonics, cross-fingerings, and register behavior

5. **Bell radiation model** — variable flare profile affecting high-frequency radiation

6. **Optional second bore** — for drone instruments (arghul, pungi, launeddas)

### 10.2 The Morphing Axes

The continuous parameter space that defines "instrument identity" can be organized along these primary axes:

| Axis | Low End | High End | What It Controls |
|------|---------|----------|-----------------|
| Bore conicity | Cylindrical (clarinet, duduk) | Wide conical (saxophone, zurna) | Harmonic series completeness, overblowing behavior |
| Reed coupling | Free reed (harmonica) | Tight double reed (oboe) | How strongly bore controls reed frequency |
| Reed size | Small (oboe) | Large (duduk, bassoon) | Dynamic response, pitch flexibility |
| Bore length | Short (hichiriki: 18cm) | Long (bassoon: 260cm) | Pitch range, register density |
| Bell flare | None (piri, duduk) | Large metal (suona, saxophone) | Brightness, projection, radiation efficiency |
| Blowing pressure | Low (duduk soft playing) | High (zurna, suona) | Dynamic range, harmonic content |
| Bore losses | Low (metal sax) | High (soft wood, bamboo) | Warmth vs. brightness, sustain |

### 10.3 Key Physical Model Parameters (Summary)

For the reed model:
- Reed angular resonance frequency: omega_r
- Reed quality factor: Q_r (range 1.25-5)
- Reed opening height at rest: H_0
- Effective reed mass per unit area: mu
- Reed closure completeness (0 = subtone, 1 = full beating)
- Reed symmetry (0 = single reed, 1 = double reed)

For the bore model:
- Bore profile function: r(x) where x is position along bore
- Tonehole positions and effective radii
- Bell flare profile
- Wall loss coefficient
- Temperature (affects speed of sound)

For the excitation:
- Blowing pressure: P_m
- Lip force / embouchure parameter
- Vocal fold coupling strength (for growling)
- Tongue position / modulation (for flutter tongue)

---

## Sources

### Academic / Research
- [Euphonics: Reed Instruments (Ch. 11.3)](https://euphonics.org/11-3-reed-instruments/)
- [UNSW: Bore Angles of Woodwind Instruments (Joe Wolfe)](https://newt.phys.unsw.edu.au/jw/bore-angle.html)
- [UNSW: Double Reed Acoustics](https://newt.phys.unsw.edu.au/jw/double-reed-acoustics.html)
- [UNSW: Clarinet Acoustics Introduction](https://newt.phys.unsw.edu.au/jw/clarinetacoustics.html)
- [UNSW: Saxophone Acoustics Introduction](https://newt.phys.unsw.edu.au/jw/saxacoustics.html)
- [Wolfe, "The Acoustics of Woodwind Musical Instruments" (Acoustics Today 2018)](https://acousticstoday.org/wp-content/uploads/2018/03/The-Acoustics-of-Woodwind-Musical-Instruments.pdf)
- [Maugeais & Dalmont, "What makes the duduk special" (Acta Acustica 2024)](https://acta-acustica.edpsciences.org/articles/aacus/full_html/2024/01/aacus240044/aacus240044.html)
- [Braasch & Cottingham, "Free Reeds: An Intertwined Tale" (Acoustics Today 2023)](https://acousticstoday.org/wp-content/uploads/2023/11/Free-Reeds-An-Intertwined-Tale-of-Asian-and-Western-Musical-Instruments-Jonas-Braasch-and-James-P.-Cottingham-3.pdf)
- [Pfiester, "Sound Production Analysis of the Oboe" (UIUC)](https://courses.physics.illinois.edu/phys406/sp2017/NSF_REU_Reports/2008_reu/Nicole_Pfiester_Oboe_Analysis/Sound_Production_Analysis_of_a_Double_Reed_Instrument.pdf)
- [Acta Acustica: Single Cane Reeds Review (2024)](https://acta-acustica.edpsciences.org/articles/aacus/full_html/2024/01/aacus240040/aacus240040.html)
- [Dalmont et al., "Toward a Simple Physical Model of Double-Reed Instruments"](https://www.researchgate.net/publication/233654283_Toward_a_Simple_Physical_Model_of_Double-Reed_Musical_Instruments)
- [Acta Acustica: Tonehole Lattice Cutoff Frequency of Conical Resonators](https://acta-acustica.edpsciences.org/articles/aacus/full_html/2020/04/aacus200005/aacus200005.html)
- [Vergez et al., "Blowing Pressures in Bassoon, Clarinet, Oboe and Saxophone"](https://www.researchgate.net/publication/233620195_Blowing_Pressures_in_Bassoon_Clarinet_Oboe_and_Saxophone)
- [Physical Study of Double-Reed Instruments for Sound Synthesis (HAL)](https://hal.science/hal-01161426/document)

### Instrument References
- [Yamaha: Saxophone Structure](https://www.yamaha.com/en/musical_instrument_guide/saxophone/mechanism/mechanism003.html)
- [Yamaha: Oboe Structure](https://www.yamaha.com/en/musical_instrument_guide/oboe/mechanism/)
- [Yamaha: Bassoon Structure](https://www.yamaha.com/en/musical_instrument_guide/bassoon/mechanism/)
- [Sax Gourmet: Saxophone Bores Explained](https://www.saxgourmet.com/i-dont-want-to-bore-you-saxophone-bores-explained/)
- [ClarinetPerfection: Bore](https://www.woodwindforum.com/clarinetperfection/bore/)
- [Basic Clarinet Acoustics (S. Fox)](http://www.sfoxclarinets.com/baclac_art.htm)

### Non-Western Instruments
- [Duduk - Wikipedia](https://en.wikipedia.org/wiki/Duduk)
- [Zurna - Wikipedia](https://en.wikipedia.org/wiki/Zurna)
- [Hichiriki - Wikipedia](https://en.wikipedia.org/wiki/Hichiriki)
- [Shehnai - Wikipedia](https://en.wikipedia.org/wiki/Shehnai)
- [Suona - Wikipedia](https://en.wikipedia.org/wiki/Suona)
- [Piri - Wikipedia](https://en.wikipedia.org/wiki/Piri_(instrument))
- [Arghul - Wikipedia](https://en.wikipedia.org/wiki/Arghul)
- [Mijwiz - Wikipedia](https://en.wikipedia.org/wiki/Mijwiz)
- [Alboka - Wikipedia](https://en.wikipedia.org/wiki/Alboka)
- [Launeddas - Wikipedia](https://en.wikipedia.org/wiki/Launeddas)
- [Pungi - Wikipedia](https://en.wikipedia.org/wiki/Pungi)
- [Cor anglais - Wikipedia](https://en.wikipedia.org/wiki/Cor_anglais)
- [Organology: Zurna](https://organology.net/instrument/zurna/)
- [Organology: Sorna](https://organology.net/instrument/sorna/)
- [Organology: Shehnai](https://organology.net/instrument/shehnai/)
- [Organology: Launeddas](https://organology.net/instrument/launeddas/)
- [Organology: Piri](https://organology.net/instrument/piri/)
- [Hichiriki (Stanford Gagaku)](https://gagaku.stanford.edu/en/woodwinds/hichiriki/)
- [Arghul and Mijwiz (Arab Instruments)](https://www.arabinstruments.com/blogs/arabinstruments-blog/arghul-and-mijwiz)

### Material Debate
- [Bret Pimentel: Does Material Affect Tone Quality in Woodwind Instruments?](https://bretpimentel.com/does-material-affect-tone-quality-in-woodwind-instruments-why-scientists-and-musicians-just-cant-seem-to-agree/)
- [SYOS: Influence of Material on Sound](https://syos.co/en-us/blogs/news/the-influence-of-the-material-on-the-sound)
- [Saxophone Material and Finish (Taming the Saxophone)](https://tamingthesaxophone.com/saxophone/technical/saxophone-material)

### Extended Techniques
- [Multiphonic - Wikipedia](https://en.wikipedia.org/wiki/Multiphonic)
- [Oboe Extended Techniques (OboeHelp)](https://oboehelp.com/extended-techniques/)
- [Clarinet Extended Techniques (HelloMusicTheory)](https://hellomusictheory.com/learn/clarinet-extended-techniques/)
- [Vocal Tract Resonances in Playing Musical Instruments (PMC)](https://pmc.ncbi.nlm.nih.gov/articles/PMC2689615/)
- [Saxophone Subtone Discussion (CafeSaxophone)](https://cafesaxophone.com/threads/what-exactly-is-subtone.36277/)
