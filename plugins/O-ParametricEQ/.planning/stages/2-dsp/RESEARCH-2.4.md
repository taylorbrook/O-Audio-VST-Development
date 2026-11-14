# O-ParametricEQ Stage 2 (DSP), Phase 2.4: Research (structural crossfades and bypass)

**Researched:** 2026-11-14
**Scope:** ROADMAP Phase 2.4: the second slot per band, the 20 ms raised-cosine crossfade and its latched request, neutral sleep and wake, the global soft bypass and `processBlockBypassed`. Gates G3 (step rows), G2 (live part), G4 (d), (e), (f), G5 with structural parameters randomised, and the row "a parameter changed while bypassed is in effect after un-bypass". No output stage or auto gain (2.5), no listen path (2.6).
**Method:** a scratch bench over the real `Source/dsp` headers, `stages/2-dsp/research-2.4/fade_bench.cpp`: a prototype of the two-slot band built from two unmodified `eq::Band` objects and `eq::Router`, with the fade, the latch, the sleep and a global bypass around them, fed by `eq::Designer::design`. Every number marked [BENCH] is from that bench on this machine (Apple M4 Max, clang -O3, scalar, double engine). Nothing in `Source/`, the harnesses, the model or the goldens was changed.
**Read this time:** the JUCE 8.0.15 plugin wrappers (`juce_audio_plugin_client_VST3.cpp`, `_AU_1.mm`) for who calls `processBlockBypassed`. Marked [JUCE].
**Not in the bench:** the JUCE processor, the float output, the render harness. Statements about those are from reading `Source/PluginProcessor.cpp` and are marked [ARGUED].

## Summary

1. **The two-slot crossfade as specified is click-free.** On 484 step readings per tone (every type pair, every slope pair of the five slope types, every placement pair, enable, global bypass; blocks 32 and 512), the largest second difference of the output is at most 1.53x that of the louder held render at 50 Hz and 1.02x at 1 kHz. With the fades off (the Phase 2.3 behaviour) the same reading is 274x to 171 000x at 50 Hz and 18x to 373x at 1 kHz.
2. **Gate G3's step threshold cannot be met as written, by any crossfade.** "Local κ ≤ 2.0x the larger of the two held references" fails 202 of 484 readings on the 50 Hz tone (worst 20.5x). A reference that crossfades two filters which have both been running since the start of the render, with the same weights, fails 121 of them itself. A 20 ms fade between two levels, or between two phases, bends a 20 ms period inside one cycle: the same finding as the sweep rows in 2.3. At 1 kHz the written gate nearly holds (8 readings at 2.05x to 2.20x, identical in the warm reference). The rows need a verdict that a level change cannot inflate (call C1, Taylor's).
3. **The incoming slot starts from rest, and on steep low filters that is audible as a dip, not a click.** A 96 dB/oct Low Cut at 30 Hz enabled under a 50 Hz tone falls 6 dB below what a warm crossfade gives, for about 10 ms longer; the same cut placed at the tone, 13 dB; a 96 dB/oct High Cut at the tone, 35 dB for 26 ms. Everything at 1 kHz and every bell and shelf row equals the warm crossfade. Letting the incoming slot run for 40 ms before its fade starts removes most of it and delays every structural change by 40 ms (call C2, Taylor's; recommendation: as specified, no pre-roll).
4. **A gain type at exactly 0 dB is the identity by design.** The mix values are exactly `(1, k, 1)` and `(1, 1)` in 100 000 designs at five rates, and noise through such a band from rest comes out within 6.7e-16 of the input (-291 dB). So the sleep and wake fades change nothing audible; what the sleep buys is the `memcmp`. Dropping a sleeping band without a fade reads the same and is 18 ms earlier.
5. **G4 (d), (e), (f) hold with room.** Bit-equal output 86 to 89 ms after the gains reach 0 (limit 200 ms), 20 ms after a bypass toggle (limit 100 ms), at five rates and six block sizes. A band enabled at exactly 0 dB starts asleep and never differs.
6. **A bypass change during the bypass fade must be latched, not reversed.** Reversing the weight at its current position puts a corner in the envelope: 39x to 48x on the second difference at 50 Hz. Finishing the fade first reads 1.07x.
7. **Stress with structure is clean.** 24 bands, 8 rates, 10 minutes each, structural parameters and the global bypass randomised (in a shorter run up to 990 fades and 750 latched requests per second): 0 non-finite samples; with all bands then switched off the output equals the input bit for bit within 40 ms.
8. **CPU.** A fade costs the second chain and nothing else: 24 cuts x 96 dB/oct fading without pause 5.2 % of a core (settled 2.65 %), 24 shelves x 96 dB/oct 9.9 % (settled 4.9 %). 24 sleeping bands: 0.04 %.
9. **`processBlockBypassed` is never called by the VST3 or AU wrapper for this plugin**, because it returns a bypass parameter [JUCE]. The override matters for hosts built on `AudioProcessorGraph` and for the harness.

Two calls are Taylor's (C1, C2). Ten are the plan's.

---

## Q1. Does the specified crossfade pass a click gate? (G3 step rows; DSP-04, FUNC-01, FUNC-04)

`fade_bench g3`. 48 kHz, stereo, one band; L carries the tone at -6 dBFS and R the same tone at -10.5 dBFS and 1.1 rad later, so every placement does something different. Timeline: 0.5 s at A, then B / A alternating every 100 ms, 8 steps. Local κ as G3 defines it, worst of the two channels, first 0.3 s excluded. Deep settings: +18 dB (second set -18 dB) at or beside the tone; cuts at 24 dB/oct with the corner at the tone.

Four renders per row and block size:
- **engine**: two slots, 20 ms raised-cosine weights, the incoming slot designed from the target and started from rest.
- **held A, held B**: the two configurations, each held for the whole render.
- **warm**: the two held renders mixed with the same weights at the same samples. Both filters have been running since sample 0; no slot, no state clear. It is what a crossfade could do at best.
- **hard**: fades off, the Phase 2.3 behaviour (design, state cleared).

Two readings: local κ (the gate's measure), and the **largest second difference** of the output relative to that of the louder held render ("abs d2"). The second one has no frame RMS in the denominator, so a dip in level cannot inflate it.

| Group | Readings | κ: engine / held > 2.0 (worst) | κ: warm / held > 2.0 | κ: engine / warm > 2.0 (worst) | abs d2: engine worst (warm) | abs d2: hard, lowest at block 512 |
|---|---|---|---|---|---|---|
| **50 Hz tone** | | | | | | |
| Type pairs, +18 and -18 | 100 | 75 (20.5) | 64 | 11 (4.6) | 1.51 (1.41) | 16 777 |
| Slope pairs, 5 types | 280 | 84 (16.1) | 44 | 43 (12.9) | 1.53 (1.41) | 13 433 |
| Placement pairs, Bell +18 and Low Cut 96 | 40 | 21 (6.1) | 0 | 18 (5.2) | 1.48 (1.23) | 4796 |
| Enable, 15 bands | 30 | 10 (16.4) | 5 | 4 (15.2) | 1.31 (1.26) | 287 |
| Global bypass, 15 bands + 24 bells | 32 | 10 (17.1) | 6 | 4 (15.9) | 1.33 (1.26) | 274 |
| Enable, 24 bells at once | 2 | 2 (2.7) | 2 | 0 (0.8) | 0.81 (1.11) | 15 269 |
| **all** | **484** | **202** | **121** | **80** | **1.53** | **274** |
| **1 kHz tone** | | | | | | |
| Type pairs | 100 | 8 (2.20) | 8 | 0 (1.01) | 1.001 | 41.6 |
| Slope pairs | 280 | 0 (1.92) | 0 | 0 (1.02) | 1.002 | 37.1 |
| Placement pairs | 40 | 0 (1.48) | 0 | 0 (1.00) | 1.001 | 18.3 |
| Enable | 30 | 0 (1.49) | 0 | 0 (1.02) | 1.021 | 17.9 (one row void) |
| Global bypass | 32 | 0 (1.65) | 0 | 0 (1.03) | 1.021 | 17.9 (one row void) |
| Enable, 24 bells | 2 | 0 (1.56) | 0 | 0 (0.95) | 1.000 | 50.5 |
| **all** | **484** | **8** | **8** | **0** | **1.02** | **17.9** |

What the table says:

- **Against the held references at 2.0x the gate is unpassable at 50 Hz.** 202 of 484 readings exceed it, and so do 121 readings of the warm reference, which has no engine in it. The largest (Low Shelf ↔ High Cut 20.5x, warm 12.0x; Bell ↔ Notch 20.1x, warm 14.1x) are frames in which the tone is passing through a low level during the fade: the confound ARCHITECTURE names under "Click-free modulation and structural crossfades".
- **At 1 kHz the written gate nearly holds.** Eight readings sit at 2.05x to 2.20x (Notch ↔ Tilt, High Shelf ↔ Notch, Bell ↔ Low Cut, Bell ↔ High Cut), and the warm reference reads the same value to two decimals on every one. They are the depth of the level change, not the engine.
- **On the absolute reading the engine is clean at both tones.** Worst 1.53x at 50 Hz (four readings above 1.5x, all slope or type changes on a 96 dB/oct cut at block 512), 1.02x at 1 kHz. The warm reference's own worst is 1.41x: a raised-cosine fade between two tones in opposite phase.
- **The hard control fires everywhere it can.** Lowest 274x at 50 Hz and 17.9x at 1 kHz at block 512. One subject is void at 1 kHz by construction and must be listed as such: the 10 Hz, Q 40 bell does nothing to a 1 kHz tone (hard reads 1.00). At block 32 the lowest non-void hard reading is 22x at 50 Hz and 17.9x at 1 kHz, so this control, unlike the sweep rows' control, also works at block 32.
- **Engine against warm is not a usable verdict.** 80 readings above 2.0x at 50 Hz (worst 15.9x), all on slot starts of steep cuts. That is Q2.

The fade shape and length (`fade_bench g3var`, κ against held, worst per group at 50 Hz / 1 kHz):

| Variant | Type | Slope | Placement | Enable and bypass |
|---|---|---|---|---|
| raised cosine 20 ms (specified) | 20.5 / 2.20 | 16.1 / 1.92 | 6.1 / 1.48 | 17.1 / 1.65 |
| raised cosine 10 ms | 22.9 / 2.62 | 288 / 1.82 | 5.6 / 1.78 | 944 / 2.04 |
| raised cosine 5 ms | 74.6 / 3.29 | 1268 / 1.48 | 18.7 / 2.24 | 978 / 2.66 |
| raised cosine 40 ms | 10.1 / 2.03 | 7.3 / 1.94 | 2.9 / 1.25 | 4.7 / 1.36 |
| linear 20 ms | 892 / 2.26 | 963 / 1.95 | 190 / 1.50 | 845 / 1.65 |
| equal power 20 ms | 1136 / 2.91 | 1417 / 1.94 | 312 / 1.69 | 10 244 / 7.86 |

Linear and equal-power weights have corners at the ends and read two orders worse at 50 Hz; equal power is also wrong for two correlated signals. 10 ms is not enough for a 50 Hz tone. 40 ms halves the count of readings above 2.0x and still does not pass the written gate (27 of 56 type readings). The raised cosine at 20 ms stays.

**Recommended (call C1):** the step rows get two verdicts and a control.
- (a) Both tones: the largest second difference of the render ≤ **2.0x** that of the louder of the two held renders (measured worst 1.53x).
- (b) 1 kHz tone: local κ ≤ **2.5x** the larger held reference (measured worst 2.20x). At 50 Hz the local κ reading is printed and not gated.
- (c) Hard control: with the fades off, reading (a) ≥ 10x at block 512, or the row is void. Void rows are listed by name in the plan (measured: the 10 Hz bell under the 1 kHz tone).

The 2.3 precedent (a same-motion reference written in the harness) also applies and is the plan's to place (C11): an independent render of the specified fade, two static filters with the incoming one started from rest at the fade's first sample, mixed with the raised-cosine weights, against which the engine should read about 1.0x. It checks that the engine does what is specified; (a) and (b) check that what is specified does not click.

## Q2. What the start from rest costs

`fade_bench dip`: lowest level of the left channel in 5 ms frames during the 100 ms after each step, relative to the quieter of the two held levels, and the time spent more than 3 dB below it. On a 50 Hz tone a 5 ms frame reads -4.3 dB at the zero crossing with no change at all, so -4.3 dB is this measure's floor there.

| Row, 50 Hz tone | Engine | Warm crossfade | Engine, 20 ms pre-roll | Engine, 40 ms pre-roll |
|---|---|---|---|---|
| Enable Bell ±18, Bell Q 40, shelves 12 and 96, Tilt, Notch, Band Pass, High Cut 6 | -4.2 to -6.5 dB | the same within 0.3 dB | the same | the same |
| Enable Low Cut 96 dB/oct at 30 Hz | -10.2 dB, 27 ms | -4.6 dB, 18 ms | -5.3 dB, 19 ms | -4.8 dB, 18 ms |
| Enable Low Cut 96 dB/oct at the tone | -17.7 dB, 32 ms | -4.3 dB, 8 ms | -13.8 dB, 25 ms | -7.6 dB, 16 ms |
| Enable High Cut 96 dB/oct at the tone | -39.7 dB, 26 ms | -4.4 dB, 9 ms | -18.7 dB, 20 ms | -7.0 dB, 14 ms |
| Enable Low Cut 96 dB/oct, Q 4, corner at 0.8x the tone | -11.5 dB, 29 ms | -8.1 dB, 20 ms | -11.3 dB, 24 ms | -14.3 dB, 21 ms |
| Placement, Low Cut 96: deepest pair | -17.7 dB, 32 ms | -4.3 dB, 8 ms | -16.5 dB | -8.7 dB |
| Slope pairs: deepest | -45.1 dB (High Cut 18 ↔ 96) | -31.6 dB (Low Cut 6 ↔ 36, where the engine reads -19.4) | | |
| Slope pairs dipping more than 6 dB | 53 of 140 | 50 of 140 | 54 | 53 |

At 1 kHz the deepest rows of every group equal the warm crossfade (largest difference seen 0.2 dB).

- **The cause is the incoming filter's own rise time.** A 16th-order filter with its corner at 50 Hz takes about 50 ms to respond; the fade hands the signal to it in 20. While it is still filling, the outgoing slot has already been faded down. The second difference stays at 1.1x to 1.5x of a held tone throughout: there is no click in it, there is a hole.
- **Most slope and type dips are not this.** 50 of 140 slope rows dip more than 6 dB in the warm reference as well: two filters whose phases at the tone differ by about half a cycle cancel in the middle of any amplitude-complementary fade. No slot arrangement removes that; only a different fade law would, and equal power is worse on every other row.
- **Pre-roll** (the incoming slot runs at weight 0 for a while, then fades): 40 ms brings the enable and placement rows to the warm crossfade's κ within 1.22x (from 15.2x) and the slope rows within 3.1x (from 12.9x). It is not uniform: the resonant cut in the table reads 3 dB deeper with it. It costs 40 ms of response on every structural change, a second chain for 60 ms instead of 20, and it does nothing at 1 kHz because nothing is wrong there. In the prototype it is a change to the weight table and nothing else (leading zeros).
- **Un-bypass has the same start from rest** (the engine is idle while bypassed, per ARCHITECTURE): the global bypass rows read the same figures as the enable rows.

**Recommended (call C2):** as specified, no pre-roll. The affected case is a cut of 48 dB/oct or steeper with its corner within about an octave of sustained low content, at the moment it is switched in; the realistic one (a 30 Hz, 96 dB/oct Low Cut under 50 Hz content) is 6 dB for about 10 ms beyond the warm crossfade. Verify gets a hands-on item for it. The weight table should be built so that a pre-roll is one constant if the listening says otherwise. Alternative: 40 ms pre-roll on band fades and on un-bypass now.

## Q3. Neutral sleep and wake (G4 (d), (e); QUAL-01)

**The 0 dB design is the identity** [BENCH `ident`]. 20 000 random Bell, Low Shelf, High Shelf and Tilt settings at exactly 0 dB per rate (all slopes, frequency and Q including the range ends), at 22.05, 44.1, 48, 96 and 192 kHz: every SVF half has `mL = 1`, `mB = k`, `mH = 1` and every one-pole half `cL = cH = 1`, exactly, in all 100 000; the closed-form response is 0.00 dB. Noise at 0.25 peak through 2000 such bands per rate, started from rest: largest `|y − x|` 4.4e-16 to 6.7e-16 (-291 dB re the input peak). The band is transparent to rounding whatever its state holds, and RESEARCH-2.2's remark stands: it is not bit-transparent, so the band has to leave the list.

**Click reading** [BENCH `sleep`]: gain 0 ↔ ±18 dB every 150 ms (the band falls asleep 70 ms after each return to 0), seven band shapes, both tones, blocks 32 and 512; κ as a ratio to an engine that never sleeps.

| | Sleep and wake with 20 ms fades (specified) | No fades (drop on sleep, start on wake) |
|---|---|---|
| 56 readings, range | 0.10 to 2.10 | 0.10 to 2.03 |
| readings above 1.25 | 2 | 2 |

The two readings above 1.25 (High Shelf 96 dB/oct at -18 dB on 50 Hz, block 512: 2.10 / 2.03; Bell Q 40 at -18 dB on 1 kHz, block 512: 1.91 / 1.85) are the same with and without the fades. The fades do nothing measurable, in either direction.

**What sleep does change:** a band that slept wakes with its state at rest, where a band that never slept has state that already follows the signal. After a wake, against the never-slept band (re its output peak): Bell Q 1 at the tone -21.5 dB at 50 Hz and -19.0 dB at 1 kHz, 96 dB/oct Low Shelf -20.7 / -18.4 dB, Bell Q 40 at a 50 Hz tone -4.6 dB over the 150 ms window (the resonance has to ring up: about `Q / (π f)` = 250 ms). It is the behaviour of a band that was just enabled, and it is reached only by a jump from exactly 0 dB to a large gain on a narrow low band. A drag away from 0 dB wakes the band at the first movement, long before the gain is large.

**Time to bit transparency** [BENCH `g4`], five rates x block sizes 1, 16, 64, 512, 1024, 4096, 24 bands, noise:

| Event | Output equals the input bit for bit after | Limit |
|---|---|---|
| (d) 24 bells enabled at exactly 0 dB | 0 ms (the bands start asleep) | 200 ms |
| (d) 24 bells, gain 6 dB → 0 | 85.7 to 88.3 ms | 200 ms |
| (e) 24 gain-bearing bands at ±12 dB, gain scale → 0 % | 87.0 to 89.1 ms (69.7 to 70.0 ms without the sleep fade) | 200 ms |
| (f) global bypass on, 24 bands at ±12 dB | 19.95 to 20.00 ms | 100 ms |

20 ms ramp + 50 ms at 0 + 20 ms fade, counted on the tick grid and so independent of the host block: a band falls asleep inside a 4096-sample block and is skipped for the rest of it.

One thing the processor must then do [ARGUED]: a lane is converted to double when the block starts with a band that touches it. If the last such band falls asleep mid-block, the lane is still written back for that block. For normal float samples the round trip is exact; for denormal samples under flush-to-zero it is not (RESEARCH-2.2). G4 (d) and (e) read "after 200 ms", by when no band is on the list at the block start, so the gate is not affected; the plan should still say which it is (C7).

## Q4. Changes that meet other changes

`fade_bench mix`, one band, both tones, blocks 32 and 512. The absolute reading as in Q1.

| Case | abs d2, 50 Hz | abs d2, 1 kHz | Hard control, lowest |
|---|---|---|---|
| Type toggled at every block for 0.7 s (the latch turns it into 35 to 37 back-to-back fades) | 1.17 | 1.00 | 16 125 / 38.5 |
| Type + frequency + gain in one block (a preset load) | 1.06 | 1.00 | 12 084 / 30.5 |
| Enable + gain 0 → +18 dB in one block | 1.11 | 1.00 | 11 828 / 45.1 |
| Type change 5 ms into a type fade (latched) | 1.21 | 1.00 | 14 811 / 46.3 |
| Bypass 5 ms into a type fade | 1.25 | 1.00 | 5811 / 23.5 |
| Un-bypass 5 ms into the bypass fade, **latched** | 1.07 | 1.00 | 10 912 / 27.9 |
| the same, the bypass fade **reverses** where it stands | **48.5** | 1.00 | |
| Bypass on for one block | 1.11 | 1.00 | 11 828 / 45.1 |
| Type change 5 ms into a gain ramp | 27.7 (the gain moves alone: 19.7) | 1.00 (1.00) | |
| Gain change 5 ms into a type fade | 59.0 (the gain moves alone: 78.6) | 3.98 (3.98) | |
| Gain 0 → +18 → 0 within 5 ms (wake, then back) | 6.4 (with sleep off: 14.4) | 0.28 (0.37) | |

- **Latching works.** A request that arrives during a fade waits for the fade to end and then starts its own; block-rate toggling of the type is a chain of clean fades. Latest wins: only the newest request is kept.
- **A preset load needs no ramp on the incoming slot.** The slot is designed from the full target (type, slope, placement and the new frequency, gain and Q) and fades in; the outgoing slot keeps whatever it was doing.
- **The bypass fade must not reverse.** The raised cosine is smooth at its ends, not in its middle: turning round at 5 ms flips the slope of the envelope in one sample. Latched, the engine finishes the 20 ms to bypass, goes idle, and fades back in.
- **The three rows with large readings are the gain ramp's own corners,** not the fade: the same gain moves with the structural change taken out read the same or higher. This is VERIFICATION-2.3 Issue 6 (a ramp linear in dB starts and stops with a change of slope) seen again, and it is not this phase's to change. A continuous change that arrives during a fade goes to the incoming slot as an ordinary `setTarget`; the outgoing slot keeps its last target.

## Q5. Global bypass and `processBlockBypassed`

**Who calls what** [JUCE]. `juce_audio_plugin_client_VST3.cpp` line 3776: `processBlockBypassed` runs only `if (pluginInstance->getBypassParameter() == nullptr && …)`; the comment above it says so. `juce_audio_plugin_client_AU_1.mm` line 2194: `else if (bypassParam == nullptr && isBypassed)`. This plugin returns its `bypass` parameter from `getBypassParameter()`, so in both formats the host's bypass arrives as a parameter value and `processBlock` keeps being called. `processBlockBypassed` is reached from `AudioProcessorGraph` (a bypassed node) and from any host or harness that calls it directly.

**So the engine has one bypass input per block:** bypass wanted = `bypass` parameter ≥ 0.5, or this block arrived through `processBlockBypassed`. Everything else is one path [ARGUED from the prototype]:

- Position `p` from 0 (processed) to `N` (bypassed), `N = round (0.020 fs)`; dry weight `0.5 − 0.5 cos (π p / N)`; one step per sample. While `0 < p < N` the block's input is kept (a dry copy of the touched lanes, at most the prepared block) and the output is `processed + w (dry − processed)` in double, before the conversion to float.
- At `p = N`: every band's state cleared, fades in flight ended on their target, ramps snapped; from the next sample the buffer is not touched. The tick clock keeps running.
- Un-bypass from idle: every band is brought to its **current** parameters from rest, settled, with no band fade and no ramp (the bypass fade covers it), then `p` runs back to 0. This is what makes "a parameter changed while bypassed is in effect after un-bypass" true, and it replaces SUMMARY-2.3 deviation 4 (a continuous edit made during bypass ramping on un-bypass).
- A bypass change while `0 < p < N` is latched (Q4).
- A lane no active band touches is never converted, so there is nothing to fade on it; with no active band at all a bypass toggle is a flag and nothing else.
- Parameters keep being read during the fade-out; while idle they are not applied, only read at un-bypass.

Measured through that prototype: Q1's 32 global-bypass readings per tone (abs d2 ≤ 1.33x, hard ≥ 274x / 17.9x), (f) 20 ms, and the three bypass rows of Q4.

## Q6. Block-size identity with structural events

`fade_bench blk`: 6 bands, stereo noise, at every event each band redraws type and slope (p 0.5), placement (0.3), enable (0.3), frequency, gain (0 dB exactly with p 0.25) and Q, and the global bypass toggles (0.1); 160 events per render; 8690 fades, 1264 latched requests and 194 wakes over the 28 renders.

| Events on multiples of 2048 | 44.1 k | 48 k | 96 k | 192 k |
|---|---|---|---|---|
| blocks 1, 32, 64, 512, 2048 | bit-identical | bit-identical | bit-identical | bit-identical |
| blocks 7 and 4096 | differ | differ | differ | differ |

It holds because every position is a sample count: a fade starts at the block start that sees the request (or at the sample the previous fade ends, for a latched one), runs `N` samples by a table indexed with the fade position, and the sleep is counted in ticks. Blocks 7 and 4096 see the event later, as in 2.3.

## Q7. Stress with structure (G5 re-run)

`fade_bench g5`. As RESEARCH-2.3's stress, plus: per band, type / slope / placement / enable redrawn with probability 1/50 per block (second run: every block), global bypass toggled with probability 1/200 per block. 24 bands, stereo, double.

| fs | Non-finite | Peak re input | Peak − running stack | Peak − stack of the last 2 s | Fades / s | Latched / s | All off → bit-equal after |
|---|---|---|---|---|---|---|---|
| 22.05 k | 0 | +157 dB | -297 dB | -202 dB | 388 | 217 | 9.2 ms |
| 32 k | 0 | +120 | -293 | -148 | 441 | 261 | 39.4 |
| 44.1 k | 0 | +215 | -259 | -97 | 496 | 292 | 11.1 |
| 48 k | 0 | +132 | -304 | -252 | 468 | 276 | 10.4 |
| 88.2 k | 0 | +156 | -332 | -129 | 582 | 325 | 26.0 |
| 96 k | 0 | +170 | -283 | -164 | 567 | 317 | 35.8 |
| 176.4 k | 0 | +168 | -285 | -224 | 667 | 376 | 14.7 |
| 192 k | 0 | +138 | -469 | -217 | 676 | 389 | 26.8 |

20 s per rate, readings after the first second. With the structure redrawn at **every** block (10 s per rate): 0 non-finite at all 8 rates, 592 to 989 fades and 403 to 751 latched requests per second, bit-equal 0.05 to 39.1 ms after all bands go off. The contract's 10 minutes per rate (1/50 draw): 0 non-finite at all 8 rates, peak +183 to +241 dB re input, at least 232 dB under the running stack and at least 49.6 dB under the stack of the last 2 s, 382 to 666 fades per second, bit-equal 0.8 to 39.9 ms after all bands go off; 285 s of machine time for the eight rates.

- Finite everywhere. A fade output is a convex mix of two finite slot outputs, so it adds no gain of its own.
- **"Afterwards all bands off → `memcmp` equal within 100 ms"** holds with a worst case of 40 ms: the fade in flight ends, the latched "off" fades, 20 ms each.
- **The peak clause** (VERIFICATION-2.3 Issue 5): against the largest stack of the trailing 2 s the worst reading is -97 dB in the 20 s runs and -49.6 dB over 10 minutes, so even the tighter form catches nothing that finiteness does not. With structural fades the "stack" also has to take, per fading band, the larger of its two slots. Recommended (C10): keep the clause as amended in 2.3, computed with the larger slot, and do not spend rows on tightening it.

## Q8. CPU

`fade_bench cpu`, 48 kHz stereo, 512-sample blocks, 20 s of noise, 24 bands. "Always fading": a new structural request the moment each fade ends.

| Case | Settled | Always fading | One fade per second | 24 bands asleep |
|---|---|---|---|---|
| 24 cuts x 96 dB/oct (Low Cut ↔ High Cut) | 2.65 % | 5.17 % | 2.71 % | (cuts do not sleep) |
| 24 shelves x 96 dB/oct (High ↔ Low) | 4.93 % | 9.88 % | 5.06 % | 0.04 % |
| 24 bells ↔ 12 dB/oct shelves | 0.67 % | 1.74 % | 0.87 % | 0.04 % |
| 24 shelves, 96 ↔ 72 dB/oct | 4.97 % | 8.39 % | 4.26 % | 0.04 % |

A fade is the second chain for 20 ms plus one design per slot change (4.7 µs at most, RESEARCH-2.2). A preset load that changes all 24 types doubles the engine cost for 20 ms. The mix itself (two copies and one multiply per lane per sample) is not visible. PERF-02 has no fading case and needs none.

## Q9. What the band and the processor have to do [ARGUED, from the bench prototype]

**A slot is an `eq::Band`.** The prototype uses two unmodified `eq::Band` objects per band, each with its own placement and an "off" flag, and adds only what is new: the fade position, the latched request, the sleep counter. `Band.h`, `SectionEngine.h`, `ControlRamp.h` and `Router.h` (verified in 2.2 and 2.3) are used as they are. The new code is one std-only header around them, drivable in double from the designer harness.

**Requests** (at a host block start, from `classifyBandChange` as now):
- identical: nothing.
- continuous, band awake: `setTarget` on the current slot (on the incoming slot during a fade).
- continuous, band asleep: gain still exactly 0 → stored only. Gain no longer 0 → wake: a slot is designed at the new frequency and Q **at 0 dB**, state at rest, then given the target, so it ramps like any gain move.
- structural, no fade in flight: the idle slot is designed from the full target, state cleared, its placement set; or marked off when the target is inactive or a gain type at exactly 0 dB. Off → off is a stored value and no fade.
- structural, fade in flight: latched. Only the newest request is kept; it is compared with what the incoming slot realises when the fade ends, and if they are equal by then nothing starts.

**Fade.** `N = round (0.020 fs)` samples (882 at 44.1 kHz, 3840 at 192 kHz), incoming weight `w[n] = 0.5 − 0.5 cos (π (n + 1) / (N + 1))` from a table built in prepare, so no sample of the fade has weight exactly 0 or 1 and the sample after the fade is the incoming slot alone. Both slots run on a copy of the band's input lanes; the output is `a + w (b − a)`, which is exact (`a`) on any lane neither placement touches. An off slot's copy is the dry input. The fade position lives in the band and a fade may end in the middle of a segment.

**After a fade:** the retired slot's state is cleared and its ramp snapped (`reset`, `snapToTarget`), and it is marked off.

**Control tick:** both slots tick while a fade is in flight. Sleep test at the tick, when no fade is in flight: active, gain type, target gain exactly 0 dB, `! isMoving()`; `round (0.050 fs / K)` consecutive ticks start the fade to off. Any other tick resets the count. (A ramp that only passes through 0 dB never satisfies it: the test is on the target and on the ramp having arrived.)

**Active list:** a band is on it while a fade is in flight or its current slot is not off. A band asleep, disabled or bypassed with no fade in flight is not on it and costs one comparison per block.

**Global bypass:** Q5.

**Non-finite input** (the 2.3 end-of-chunk check): clear the state of both slots of every band; fades in flight carry on.

**G2 live** needs nothing of its own: after a slope change the band is the incoming slot, which is a static design of the new settings started from rest at the fade's first sample. The row measures the corner gain and stop-band slope on a render taken after the fade, against the same tolerances as the static rows.

## Q10. Harness shape

| Level | Rows |
|---|---|
| Engine in double (`tests/designer-harness`) | G3 step rows per C1 with the hard control, both tones, blocks 32 and 512: type pairs, slope pairs, placement pairs, enable, band bypass, sleep / wake; the independent fade reference (C11); the Q4 cases (latch, preset load, change during a fade); G4 (d), (e) timing at five rates and six block sizes; block identity with structural events on 2048; G5 with structure at 8 rates (`--long` for the 10 minutes) with "all off → bit-equal within 100 ms"; a fade leaves untouched lanes bit-equal (Left ↔ Right: no sample of the far lane changes outside the fade) |
| Plugin, float (`tests/render-harness`) | G3 step rows through `processBlock`, including global bypass through the parameter **and** through `processBlockBypassed`; G4 (d), (e), (f) as `memcmp` on the float output at block sizes 1 to 4096; G2 live (slope changed on a running band, all 8 slopes of both cuts); "changed while bypassed is in effect after un-bypass", against a fresh instance; G6 (`malloc_logger`) around a structural stress; fading CPU as INFO |

Rows carried from VERIFICATION-2.3, with where they now belong:

| Issue | Row |
|---|---|
| 1 Mid / Side ramping path | A three-parameter move on Mid with L = R and on Side with L = −R, equal to the Stereo render (designer harness, RM-st family) |
| 2 One-pole on the ramping path | One G3 sweep motion on a 6 or 18 dB/oct cut |
| 3 `snapToTarget` on bypass during a move | Replaced: bypass during a move, then un-bypass → the blocks after the fade equal a fresh instance on the same parameters, bit for bit |
| 3 Tick phase across chunks | A host block of 4096 on a prepared size of 512, bit-identical to prepared 4096 |
| 3 Clock reset in `prepareToPlay` | Re-prepare, then two moves in blocks of 500, against a fresh instance |
| 4 State rule of a structural change | Replaced: the incoming slot starts from rest. Row: after a type change on a ringing band (the RM-keep stimulus), the output from the end of the fade equals a fresh band started at the fade's first sample, bit for bit |
| 5 G5 peak clause | Kept as amended, larger slot per fading band (Q7) |

- The step rows' two held renders double as the references of verdicts (a) and (b); no extra render.
- Placement rows need a stimulus with L ≠ R in level and phase, or Stereo ↔ Mid and anything ↔ Side change nothing.
- Deep settings beside the tone, not on it: a Notch at the tone or a Band Pass centred on it leaves nothing above -80 dBFS, or changes nothing.
- A negative control for the sleep: with the sleep disabled G4 (d) and (e) must fail (the 0 dB band is within 7e-16 of the input and not equal to it).

## Pitfalls carried

- The bypass fade reverses cleanly only at its ends (Q4). Latch it.
- The weight table must not contain 0 or 1, or the first or last fade sample duplicates the neighbour outside the fade and a `memcmp` row can pass or fail on rounding of `a + 1 · (b − a)`.
- `a + w (b − a)` keeps an untouched lane exact; `(1 − w) a + w b` does not.
- "Off" must be a flag, not a band designed at unity: G4 (a) and (b) compare bits.
- The sleep condition tests the gain for exact 0 and nothing else (CONTEXT: the skewed frequency and Q defaults are inexact).
- A latched request compared at the fade's end with the incoming slot's input, not with the request that started the fade, or A → B → A inside one fade starts a needless second fade to B.
- The hard control at 1 kHz is void for a band that does nothing at 1 kHz; list such rows instead of lowering the 10x.
- No `juce::SmoothedValue`, no `juce::dsp` filter class, no audio golden across platforms (as 2.3).
- Stage 1's 36 rows, the designer's 88 and the render harness's 83 stay green; the parameter golden is untouched. Rows that asserted the 2.3 hard behaviour (a structural change clears the state and clicks; bypass is a hard switch; SUMMARY-2.3 deviation 4) are amended by name in the plan.

## Calls for the plan

| # | Call | Recommendation | Whose |
|---|---|---|---|
| C1 | G3 step-row verdict | (a) largest second difference ≤ 2.0x the louder held render's, both tones (measured 1.53x); (b) local κ ≤ 2.5x the larger held reference at 1 kHz (measured 2.20x), printed and not gated at 50 Hz; (c) hard control ≥ 10x on (a) at block 512 or the row is void, void rows listed. Amends ARCHITECTURE G3 and the ROADMAP 2.4 criterion. Alternative: keep "κ ≤ 2.0x held" and accept that 202 of 484 readings at 50 Hz fail for any crossfade (not recommended). | Taylor (contract) |
| C2 | Start from rest of the incoming slot | As specified, no pre-roll; a hands-on item in verify for a steep low cut switched in under bass. The weight table takes a pre-roll length (0 in v1). Alternative: 40 ms pre-roll on band fades and un-bypass (dips to the warm crossfade's level within 3 dB; every structural change 40 ms late). | Taylor (behaviour) |
| C3 | Slot type | Two `eq::Band` per band, unmodified, plus one new std-only header for the fade, the latch and the sleep. | Plan |
| C4 | Fade | `round (0.020 fs)` samples, weights from a table, `(n + 1) / (N + 1)`, output `a + w (b − a)`, starts at the block start that sees the request. | Plan |
| C5 | Latch | Newest request only; compared with the incoming slot at the fade's end. Continuous changes during a fade go to the incoming slot. | Plan |
| C6 | Sleep and wake | Sleep after `round (0.050 fs / K)` ticks at target gain exactly 0 with the ramp arrived; fade to off as specified (the fade is inaudible either way; without it transparency comes 18 ms earlier). Wake: slot at 0 dB from rest, then the target; a band enabled at exactly 0 dB starts asleep. | Plan |
| C7 | A lane whose last band falls asleep mid-block | Written back for that block (exact for normal samples), not converted from the next block on. State it in the plan and keep G4's denormal tail on (a), (b) only. | Plan |
| C8 | Global bypass | One path for the parameter and `processBlockBypassed`; 20 ms raised cosine against a dry copy held only during the fade; latched, never reversed; idle clears state and ends fades; un-bypass brings every band to its current parameters from rest with no band fade. | Plan |
| C9 | Non-finite input during a fade | Clear both slots of every band; fades carry on. | Plan |
| C10 | G5 | Structure and bypass in the draw (1/50 and 1/200 per block, plus a short every-block run); peak clause as amended in 2.3 with the larger slot per fading band; "all off → bit-equal within 100 ms". Default 60 s, `--long` 10 minutes. | Plan |
| C11 | Row placement | Q10's two tables, including the independent fade reference and the six rows from VERIFICATION-2.3. | Plan |
| C12 | Version | 0.4.0 with build, install, pluginval 10 and `auval`, as 0.2.0 and 0.3.0. | Plan |

## Not answered here

- The float output's own step readings (the bench is double) and the fades through the real processor.
- Whether the dip of Q2 is audible on real material, and whether 20 ms feels immediate for enable and bypass: hands-on items for verify.
- The listen path's crossfade (2.6) and the output-stage ramps (2.5); the global bypass here is specified without them and sits last in the chain when they arrive.
- A control ramp with smooth ends (VERIFICATION-2.3 Issue 6). Q4 shows its corners again; it is outside this phase.
- A fade law that avoids the mid-fade cancellation between two filters in opposite phase. None was found that is not worse elsewhere.
