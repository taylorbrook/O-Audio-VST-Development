---
title: "Logic Pro 8-Channel Spatialization: Container Constraints, JUCE Channel Order, and DBAP for Irregular Arrays"
created: 2026-08-10
domain: spatial-audio
type: research
keywords:
  - logic-pro
  - multichannel
  - octaphonic
  - dbap
  - vbap
  - surround
  - audiochannelset
  - au
  - spatial-audio
---
# Logic Pro 8-Channel Spatialization

**Researched:** 2026-08-10
**JUCE version:** 8.0.14 (local `/Users/taylorbrook/JUCE`)
**Driving use case:** octaphonic / irregular concert speaker array driven from Logic Pro, Roy Barnett Recital Hall (UBC)
**Confidence:** HIGH on the Logic container constraint, the effect/instrument multi-out split, and the JUCE channel-order trap (verified against local JUCE source). MEDIUM on Logic's per-channel custom I/O assignment (see §6).

---

## 1. The Governing Constraint

**Logic Pro exposes exactly 10 named surround formats and no arbitrary discrete N-channel bus.**

| Format | Channels |
|--------|----------|
| Quadraphonic | 4 |
| LCRS | 4 |
| 5.1 (ITU 775) | 6 |
| 6.1 (ES/EX) | 7 |
| **7.1** | **8** |
| **7.1 (SDDS)** | **8** |
| **5.1.2** | **8** |
| 5.1.4 | 10 |
| 7.1.2 | 10 |
| 7.1.4 | 12 |

There is no "8 discrete channels" option. If you want 8 speaker feeds out of Logic, the container **must** be 7.1, 7.1 (SDDS), or 5.1.2.

This is a Logic limitation, not a JUCE or AU one. Both of the layouts you would actually want exist in JUCE and are advertised to the host by the AU wrapper — Logic simply ignores them:

- `AudioChannelSet::octagonal()` — 8 channels, maps to `kAudioChannelLayoutTag_Octagonal`
  (`juce_AudioChannelSet.h:367`, `juce_AudioChannelSet.cpp:572`)
- `AudioChannelSet::discreteChannels(8)` — advertised as `kAudioChannelLayoutTag_DiscreteInOrder | 8`
  (`juce_audio_plugin_client_AU_1.mm:2601-2617`)

Do not design around either for a Logic target.

---

## 2. Effects Cannot Multi-Out; Instruments Can

Logic supports multiple output busses **only for instruments** (`aumu`). Audio effect plugins (`aufx`) get a single output bus — extra output busses are not exposed.

Consequence: the intuitive design "an AU effect that writes to aux/outputs 1–8" **cannot be built**. The multi-output aux route is available only if the plugin is an instrument, in which case Logic surfaces outputs 3/4, 5/6, 7/8 as aux channel strips (the first pair always stays on the instrument's own strip).

For an effect, the surround-container route in §3 is the only path.

---

## 3. Recommended Architecture: 7.1 as a Transport Container

7.1 is a *geometry* (L/C/R/LFE + sides + rears). An octagon or an irregular hall array is not. The two are reconciled by treating 7.1 as **nothing but an 8-channel carrier** — the channel names are routing labels, and the real geometry lives entirely in the plugin's DSP.

```
Plugin:  mono/stereo in  →  AudioChannelSet::create7point1() out
Logic:   Project Settings → Audio → General → Surround Format = 7.1
         Preferences → Audio → I/O Assignments → Output → 7.1 → physical outs 1–8
```

Two verified facts make this legitimate rather than a hack:

1. **Logic's Surround Panner is transparent in surround→surround mode.** It becomes a *Surround Balancer*; per Apple's documentation, source channels "are passed on to their respective output channels without any cross-panning or mixing" — it changes level only. Left centered, it does not disturb the plugin's computed gains.

2. **Logic applies no automatic LFE bass management or low-pass.** Filtering the LFE is something the user does deliberately (typically a multi-mono EQ on the surround master). So the LFE slot carries a full-range feed to an ordinary speaker without special handling.

This matches established community practice for 8-channel work in Logic: use 7.1 and treat the sub channel as the 8th speaker.

---

## 4. CRITICAL: `AudioChannelSet` Is a Bitset, Not an Ordering

**The single highest-risk implementation detail.** `AudioChannelSet` stores channels as a bitmask over the `ChannelType` enum. Buffer channel order is therefore **enum-bit order** — *not* the order of the initializer list in the JUCE source, and *not* the CoreAudio wire order.

Proof, both files on disk:

```
juce_AudioChannelSet.cpp:572
  octagonal() = { left, right, centre, leftSurround, rightSurround,
                  centreSurround, wideLeft, wideRight }
                            ^ centre at index 2

juce_CoreAudioLayouts_mac.h:92
  kAudioChannelLayoutTag_Octagonal = { left, right, leftSurround, rightSurround,
                                       centre, centreSurround, wideLeft, wideRight }
                                                  ^ centre at index 4
```

Same 8 channel types, two different orderings, one tag. JUCE performs the remap internally; the wrapper's job is exactly this translation.

**Rule: never hardcode buffer indices for a multichannel layout.** Build the speaker→buffer map once in `prepareToPlay()`:

```cpp
// speakerToBuffer[n] = buffer index carrying the label assigned to speaker n
const auto set = getBusesLayout().getMainOutputChannelSet();
for (int n = 0; n < 8; ++n)
    speakerToBuffer[n] = set.getChannelIndexForType (labelForSpeaker[n]);
```

**Why this matters more than usual:** a wrong map is *silent*. It does not crash, does not produce NaN, does not fail `auval` or `pluginval`. It simply rotates or scrambles which physical speaker each feed reaches — a failure only audible in the room. It passes every automated gate.

This is the same defect as JUCE forum thread 68674 ("C and LSR channels are flipped" in Logic AU); the JUCE team's first diagnostic question there was whether the developer used `getTypeOfChannel()`.

Reference for the 7.1 set:
```
create7point1() = { left, right, centre, LFE,
                    leftSurroundSide, rightSurroundSide,
                    leftSurroundRear, rightSurroundRear }
```
(`juce_AudioChannelSet.cpp:567` — again, a *set*, not an order.)

---

## 5. VBAP Is Wrong for Irregular Arrays — Use DBAP

**VBAP assumes all speakers are equidistant from a listener at the centre.** It normalizes speaker positions to unit direction vectors and discards distance entirely. The DBAP paper's own framing of the problem: matrix techniques like VBAP "require that the listeners are restricted to a relatively small listening area surrounded by loudspeakers arranged in a circle or sphere."

Applying VBAP to a non-equidistant array requires explicit amplitude *and* time-delay compensation per speaker.

**DBAP (Distance-Based Amplitude Panning)** — Lossius, Baltazar, de la Hogue, ICMC 2009 — was built for precisely this case: "no assumptions are made concerning the layout of the speaker array nor the position of the listener," targeting "concerts, stage productions, installations."

Comparative literature: DBAP scores slightly worse than VBAP on *average* localization accuracy, but shows **lower variance across listener positions**. For an audience distributed through a hall — where there is no sweet spot to optimize for — lower variance is the metric that matters.

### 5.1 Formulation

Use the **2011-04-14 revised version** of the paper. It explicitly corrects errors in equations 3–6 and 9–10; many copies circulating online still carry the erroneous originals.

```
d_i = sqrt( (x_i-x_s)² + (y_i-y_s)² + r_s² )       (1, 8)   2D + spatial blur
d_i = sqrt( (x_i-x_s)² + (y_i-y_s)² + (z_i-z_s)² + r_s² )   3D extension

a   = R / (20 · log10 2)                           (4)

v_i = k · w_i / d_i^a                              (9)
k   = 1 / sqrt( Σ_{i=1..N} w_i² / d_i^(2a) )       (10)
```

Without speaker weights this reduces to eqs. (3) and (5): `v_i = k / d_i^a`, `k = 1/sqrt(Σ 1/d_i^(2a))`.

| Symbol | Meaning |
|--------|---------|
| `N` | number of speakers |
| `(x_i, y_i, z_i)` | position of speaker *i* |
| `(x_s, y_s, z_s)` | virtual source position |
| `d_i` | source→speaker *i* distance |
| `v_i` | gain for speaker *i* |
| `R` | rolloff, dB per doubling of distance |
| `a` | rolloff coefficient derived from `R` |
| `r_s` | spatial blur, ≥ 0 |
| `w_i` | per-speaker weight |

The construction guarantees `I = Σ v_i² = 1` — constant intensity regardless of source position (eq. 2).

### 5.2 Parameters in Practice

- **Rolloff `R`** — `R = 6` dB is the free-field inverse-distance law. The paper: "for closed or semi-closed environments R will generally be lower, in the range 3-5 dB." A recital hall wants ≈ 4. Expose 3–6.

- **Spatial blur `r_s`** — prevents division by zero when a source sits exactly on a speaker (eq. 7 shows the un-blurred limit collapses all output into that one speaker), and controls how tightly a source can converge. The paper reads it geometrically: "In two dimensions blur can be understood as a vertical displacement between source and speakers." Normalize it against the covariance of speaker distances from rig centre so it is independent of rig size (§3.1). **Cap it** — too much blur invites the precedence effect, dragging the perceived image toward whichever speaker is nearest each listener.

  *Corollary for real rooms:* if the speakers are flown above a raked audience, that height difference already supplies a physical blur floor. In a 3D model, expose `r_s` as blur *additional* to the real geometry.

- **Convex hull (§2.3)** — if the source lies outside the speaker field, compute the hull, project the source onto the nearest point on its boundary, and use that projected position for the gain computation. Return the source→hull distance; it is the natural driver for optional gain attenuation, air-absorption filtering, Doppler, or a reverb send. This is not an edge case in irregular rooms — inboard rear speakers put the actual room corners outside the hull.

- **Speaker weights `w_i`** — restrict a source to a subset of the array. The paper's own worked example is the **Acousmonium**, where weights "restrict diffusion of sources to specific groups of speakers," letting "a spatial orchestration be prepared or composed prior to performance, and then adjusted to the specific room." For concert diffusion work this is a headline feature, not a refinement.

### 5.3 Visualization

The paper's figures 1–3 describe a directly reusable UI: render a 2D matrix where each pixel holds the squared DBAP amplitude for a speaker if the source were at that position, producing a gradient "zone of predominance" map. Combining per-speaker maps and taking the max yields a segmented map of the whole array. Explicitly intended as an interactive backdrop for source positioning.

---

## 6. Open Items / Lower Confidence

- **Per-channel custom I/O assignment (MEDIUM).** Apple's I/O Assignments documentation page returned only its table of contents on fetch. Search results describe Default / ITU / WG-4 presets plus manual assignment, and note Logic's 5.1 default is `out1=L, out2=R, out3=Ls, out4=Rs, out5=C, out6=LFE` — i.e. **not** L,R,C,LFE order. Unverified whether all 8 channels of 7.1 can be freely reassigned.

  **Mitigation (recommended regardless):** give the plugin its own speaker→label mapping table in its UI, so it adapts to whatever Logic imposes rather than depending on Logic's assignment UI. Pair it with a "verify" mode that solo-pings each speaker in turn.

- **Logic Pro 11 legacy surround (MEDIUM).** Confirmed via secondary sources that the 10 named formats persist alongside Atmos; not confirmed against an Apple Logic 11 document.

- **Interleaved bounce channel order (MEDIUM — TEST BEFORE RELYING ON IT).** See §6a. The canonical-order claim rests on user reports about 5.1, not Apple documentation, and the 7.1 canonical order is unverified.

- **LFE gain on bounce (MEDIUM-LOW).** No evidence found that Logic applies the +10 dB LFE offset on bounce — that convention belongs to the decoder/monitor, not the encoder. This is absence of evidence, not proof. If it did fire, whichever speaker occupies the LFE slot would bounce 10 dB hot. Test it (§6a).

- **Down Mixer behaviour (UNTESTED).** Logic's surround Down Mixer was not investigated.

---

## 6a. Bouncing Multichannel

**Logic bounces surround in two modes:**

| Mode | Output |
|------|--------|
| Interleaved | one N-channel file |
| Split | N mono files, one per channel, each with a configurable extension |

Split extensions (`.L`, `.R`, `.C`, `.LFE`, `.Ls`, `.Rs`, …) are editable in the
**Bounce Extensions** preference pane; they double as channel identifiers on
re-import.

- **Formats: WAV, AIFF, CAF.** Compressed surround is refused — selecting AAC/MP3
  raises a dialog stating a compressed surround (or split stereo) bounce is not
  possible.
- Bounces are **unencoded PCM**. Dolby Digital / DTS encoding is a downstream
  Compressor (or third-party) step.
- Route: File → Bounce → Project or Section, tick **Surround Bounce**. All surround
  outputs bounce simultaneously regardless of which channel strip's Bounce button
  was clicked.
- DVD-A export tops out at 6 channels (5.1) @ 24-bit/48 kHz.

### The useful part: bounce order appears decoupled from I/O assignment

User reports indicate interleaved surround bounces emerge in **canonical channel
order** (`L R C LFE Ls Rs` for 5.1) *regardless* of I/O assignment settings, and
that changing the output assignment does not reorder split files either.

If this holds for 7.1, the bounce path is **cleaner than the live path** — fully
independent of physical routing, leaving the plugin's speaker→label map as the sole
determinant of bounced channel order. Choose the default map so the mapping is the
identity:

| Speaker | Label | Canonical ch |
|---------|-------|--------------|
| 1 | L | 1 |
| 2 | R | 2 |
| 3 | C | 3 |
| 4 | LFE | 4 |
| 5 | Ls | 5 |
| 6 | Rs | 6 |
| 7 | Lsr | 7 |
| 8 | Rsr | 8 |

An interleaved bounce is then an 8-channel file where **channel N = speaker N** —
which drops straight into QLab / Reaper / any multichannel player for concert
playback without a remap. **Ship this as the default label map.**

### Two tests to run before relying on any of this

1. **Order:** bounce a 7.1 project with a distinct tone (or a spoken channel
   number) in each of the 8 slots, interleaved. Open the file and read off the
   order.
2. **LFE gain:** bounce identical −20 dBFS tone into the LFE slot and one other
   slot. Compare levels. Any delta is Logic touching the LFE path.

---

## 7. Alternatives Considered

| Approach | Verdict |
|----------|---------|
| **7.1 container + DBAP** | **Recommended.** Only simple in-Logic path to 8 discrete feeds. |
| Multi-output instrument (aumu), 4 stereo busses → aux 3/4, 5/6, 7/8 | Viable *only* if the plugin is an instrument. Awkward for processing existing audio. |
| **SpatGRIS / ControlGRIS** (GRIS, Université de Montréal) | Free, open source, VBAP + DBAP, any layout, 256 I/O. The professional standard for this exact use case. But requires routing audio out of Logic (BlackHole/JackTrip) plus parallel OSC, and there is a published paper on a Logic-specific incompatibility for an octophonic ring where displayed ≠ heard spatialization. Powerful, not simple. |
| **SPARTA Panner** (McCormack, SAF) | Free, ships AU, frequency-dependent VBAP to arbitrary arrays up to 128ch. Hits the same Logic 8-channel container wall. |
| Ambisonics + decoder | Does not help. 2nd order = 9ch, 3rd = 16ch — neither fits a Logic bus, and the same 8-channel container is still required for output. Only wins for order-free rotation or frequently changing layouts. |

---

## 8. Sources

- [Logic Pro: overview of surround formats](https://support.apple.com/guide/logicpro/surround-formats-overview-lgcp31f97343/mac)
- [Logic Pro: surround I/O assignments](https://support.apple.com/guide/logicpro/logic-pro-surround-settings-lgcp31fcdfce/mac)
- [Logic Pro: Surround Panner parameters](https://logicpro.skydocu.com/en/surround-in-logic-pro-x/logic-pro-x-surround-features/surround-panner/surround-panner-parameters/)
- [Logic Pro: multi-output instruments](https://support.apple.com/guide/logicpro/use-multi-output-instruments-lgcp4ff5d47e/mac)
- [JUCE forum 68674: surround panning with Logic and AU](https://forum.juce.com/t/juce-surround-panning-with-logic-and-au/68674)
- [JUCE: configuring bus layouts](https://docs.juce.com/master/tutorial_audio_bus_layouts.html)
- [Lossius, Baltazar, de la Hogue — DBAP, ICMC 2009 (2011-04-14 revised)](https://jamoma.org/publications/attachments/icmc2009-dbap-rev1.pdf)
- [DBAP notes, UCSB MAT 240D](https://w2.mat.ucsb.edu/240/D/notes/DBAP.html)
- [Speaker Placement Agnosticism: Improving DBAP (arXiv 2109.08704)](https://arxiv.org/abs/2109.08704)
- [SpatGRIS (GRIS-UdeM)](https://github.com/GRIS-UdeM/SpatGRIS)
- [SpatGRIS ↔ Logic Pro X incompatibility (Journal of Student Research)](https://www.jsr.org/index.php/path/article/view/699)
- [SPARTA (McCormack)](https://github.com/leomccormack/SPARTA)
- [8-channel/octophonic spatialization in Logic (community)](https://www.logicprohelp.com/forums/topic/73214-8-channel-mixingoctophonic-spatialization/)

## 9. Related Local Research

- `research/spatial-audio-per-grain-spatialization.md` §1 — VBAP math and C++ implementation
- `research/saf-juce-integration-guide.md` §5 — SAF `saf_vbap` API, MDAP spreading
- `research/juce8-multichannel-spatial-audio.md` — `AudioChannelSet` reference table, `isBusesLayoutSupported()` examples, DAW compatibility matrix
