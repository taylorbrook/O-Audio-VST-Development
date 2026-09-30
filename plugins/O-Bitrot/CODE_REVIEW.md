---
phase: O-Bitrot-code-review
reviewed: 2026-09-30T00:00:00Z
version_reviewed: 1.17.0
depth: deep
files_reviewed: 26
files_reviewed_list:
  - plugins/O-Bitrot/Source/PluginProcessor.h
  - plugins/O-Bitrot/Source/PluginProcessor.cpp
  - plugins/O-Bitrot/Source/PluginEditor.h
  - plugins/O-Bitrot/Source/PluginEditor.cpp
  - plugins/O-Bitrot/CMakeLists.txt
  - plugins/O-Bitrot/Source/dsp/*.h (20 headers)
  - plugins/O-Bitrot/Source/ui/public/index.html
  - plugins/O-Bitrot/Source/ui/public/js/i18n.js
findings:
  critical: 0
  warning: 12
  info: 14
  total: 26
status: issues_found
---

# O-Bitrot: Code Review Report

**Reviewed:** 2026-09-30 (v1.17.0)
**Depth:** deep. There were four parallel passes: core DSP, media DSP,
processor/editor and WebView UI. Every Warning below was re-read against the
current source before it was filed.
**Status:** issues_found. There are no Critical findings.

## Summary

The plugin is structurally sound:

- **Audio thread:**
  - No allocation, locks or strings.
  - Every IIR gets its coefficients before `reset()`.
  - Non-finite input is scrubbed.
  - Rot output is clipped after the flip.
- **Parameters:**
  - All 45 IDs, ranges, defaults and skews match `parameter-spec.md`.
  - The relays, the JS `data-param`s and the 15 native functions all match on
    both sides.
  - Member order is relays → WebView → attachments.
- **Determinism:** RNG streams are seeded only from SEED, never from the clock
  or `this`.
- **Block size and sample rate:** all DSP is per-sample, so output does not
  depend on block size, and time constants are derived from fs.

The defects fall into three groups:

1. **Host integration.**
   - Host bypass drops the 20 ms latency compensation (WR-01).
   - `reset()` is never overridden, so stop/locate/bounce carry the capture
     ring and running events over. This breaks FUNC-04 bounce determinism
     (WR-02).
   - The preset name is not saved with the session (WR-09).
2. **Discontinuities (clicks/steps).** These are found at five DSP seams:
   WR-03 through WR-07.
3. **Behaviour drifting from design.**
   - The Rot bit field ignores its own lower bound (WR-08).
   - Pop and tick levels collapse at 96/192 kHz (WR-12).
   - There are two UI interaction bugs (WR-10, WR-11).

## Warnings

### WR-01: Host bypass loses the 20 ms latency compensation

**File:** `Source/PluginProcessor.h:61-73` (no `processBlockBypassed` and no
`getBypassParameter`); latency is set at `PluginProcessor.cpp:1268-1270`.

**Issue:**
- The plugin reports `ceil(0.020·fs)` samples of latency. JUCE's default
  `processBlockBypassed` passes input straight through and asserts
  `getLatencySamples() == 0`.
- When the host bypasses the plugin, it still compensates 20 ms, so the
  bypassed track plays 20 ms early. The timing jumps on every toggle.
- On un-bypass, the codec delay line and the `DryWetMixer` dry delay replay
  20 ms of stale audio.
- In a mono→stereo layout, bypass leaves R silent.

**Fix:** override `processBlockBypassed` with a per-channel integer delay of
`compLatencySamples`, allocated in `prepareToPlay`, and copy L to R for
mono→stereo. Flag a DSP reset for the first block after bypass ends.
O-Octagon is the only plugin in the suite that already overrides it; use it
as the reference.

### WR-02: `AudioProcessor::reset()` is not overridden, so stop/locate/bounce keep all DSP state

**File:** `Source/PluginProcessor.h`: there is no `reset()`. All reset logic
lives in `prepareToPlay` (`PluginProcessor.cpp:1263-1311`).

**Issue:**
- AU `Reset()` and VST3 `setProcessing(false)` call `reset()`, not
  `prepareToPlay`. Logic resets on stop, locate and bounce start.
- The following all survive:
  - the 10 s capture ring
  - running tape-stop, CD-loop and locked-groove events
  - the read-head lag
  - the RNG stream positions
- Two consecutive bounces of the same region with the same SEED therefore
  differ, which violates FUNC-04 ("same seed + input + params = identical
  bounce").
- After a locate, a lagging head or a CD loop replays audio from the previous
  song position.

**Fix:** add an allocation-free `void reset() override` that does the following:
- Clear the capture ring.
- Call `reset()` on every stage: read head, media clock, tape\*, wow/flutter,
  vinyl warp, beds, CD skip, vinyl transport, artifact synth, rot, packet,
  codec, crush and quant.
- Call `dryWetMixer.reset()`.
- Reseed the RNG from SEED, and reset `lastSeed` and `lastAppliedRate`.
- `CodecStage` needs a delay/GSM-state reset that does not call `gsm_create`
  again.

This also closes the I-6 race below (preset load with an unchanged SEED).

### WR-03: The outgoing crossfade head repeats a sample on every jump from live

**File:** `Source/dsp/ReadHead.h:296-298`

```cpp
oldPos += oldRate;
if (oldPos > hi)
    oldPos = hi;            // main head pins at hi + 1.0 (line 316)
```

**Issue:**
- At lag 0 (the steady state), a jump sets `oldPos = pos = hi`. After one fade
  sample it is clamped back to `hi` while the write advances, so the outgoing
  head reads x[n] twice.
- The error is about (1−t)·(x[n+1]−x[n]), which is up to ~0.65 FS on a 5 kHz
  full-scale sine at 48 kHz, weighted at 0.92–0.99.
- It hits most CD-loop entries and vinyl jumps that start from live.

**Fix:** pin it the same way as the main head:
`if (oldPos > hi + 1.0) oldPos = hi + 1.0;`

### WR-04: The CD conceal rung switches its filter in with no blend, a ~20 % step at onset

**File:** `Source/dsp/CDSkip.h:258-262` (install), `:304-315` (render)

**Issue:**
- The one-pole is reset to zero state and switched straight into the signal at
  fMax = min(20 kHz, 0.45·fs), where it is not transparent. Its first sample is
  G·x ≈ 0.79·x at 48 kHz, followed by overshoot.
- It snaps back to dry at the exit.
- `TapeDropout.h:39-43, 139-149` measured and fixed this exact shape, and even
  cites CDSkip as its source, but CDSkip never received the fix.

**Fix:** blend by `tri`, as TapeDropout does:

```cpp
const float wet = (float) tri, dry = 1.0f - wet;
const float fl = concealFilter.processSample (0, left);
const float fr = concealFilter.processSample (1, right);
left = left * dry + fl * wet;  right = right * dry + fr * wet;
```

### WR-05: A tape stop installed during a down-bend steps the gain by −3 to −6.4 dB in one sample (PLAUSIBLE)

**File:** `Source/dsp/TapeStopGain.h:156-186`; reached from `Arbitration.h:320`
when two tape ticks arrive back to back (`installBend(0.5|0.67)`, then
`installStop`).

**Issue:**
- The stop arms with `appliedRate` already below the 0.9 threshold, so the law
  engages mid-curve: x = 0.556, g = 0.625. `speedFilter.reset()` also enters at
  G ≈ 0.47.
- The first sample is ≈ 0.48·x from the 0.5 bend, or ≈ 0.71·x from the 0.67 bend.
- The "identity at threshold" argument only holds when the rate crosses 0.9
  from above.
- This was traced by the reviewer but not re-derived independently.
  **Confirm with a harness render before fixing.**

**Fix:** when engaging below the threshold, latch
`xRef = appliedRate / kRateThreshold` and use
`x = min(1, (rate / kRateThreshold) / xRef)`, then clear it on disarm. The
filter needs the same treatment: prime its state with the current sample
rather than zero.

### WR-06: Changing VINYL_RPM with warp depth > 0 steps the read offset (click and pitch glitch)

**File:** `Source/dsp/VinylWarp.h:150-163`; `setRpm` runs every block from
`PluginProcessor.cpp:~1495`.

**Issue:** `lagAmp = kMaxDeviation · revSamples / π` changes instantly. At depth
1 and 48 kHz, going from 33⅓ to 78 rpm at the LFO peak moves the offset from
~165 to ~70 samples, a 95-sample read jump with no crossfade. The header's
"no step on a parameter touch" only covers phase.

**Fix:** slew `lagAmp` to its target over ~1–3 s, in the same way as the depth
ramp. `phaseInc` can keep switching immediately.

### WR-07: Re-engaging GSM leaves a gap of up to 10 ms, then a +10 dB AGC overshoot

**File:** `Source/dsp/CodecStage.h:375` (the `g > 0 && w > 0` gate) and `:465-471`.

**Issue:** encode/decode only run while the stage is audible.

- **CODEC_ENABLE turned on in GSM mode:** the enable fade reaches unity in
  10 ms, but the first decoded frame is 20–40 ms away, so the output is silent
  for up to ~10 ms.
  - While the stage was off, `agcEnv` decayed toward 0, so the first frame
    meets the full +10 dB makeup. That is the same ~+3.7 dBFS overshoot the
    `prepare()` comment at `:285-293` says was fixed; the fix only covers a
    cold start.
- **Mode switch from mu-law to GSM:** this dips in the same way.
- **Stale state:** the LTP state is stale from the last time GSM ran.

**Fix (recommended):** drop the `g > 0 && w > 0` gate. Run the codec every frame
whenever the handles exist; that is 50 frames/s, trivial CPU, and still
deterministic. Bypass stays bit-exact because the `g == 0` rail outputs dry.
Minimal alternative: re-prime `agcEnv = kAgcUnityEnv` when the gate reopens.
Either way, re-anchor the digests of any render that toggles GSM. The header
claim at `:110-113` ("≤ 20 ms silence covered by the 10 ms fades") is wrong for
the same reason; correct it.

### WR-08: The Rot flip bit field ignores `kFlipBitMin`

**File:** `Source/dsp/RotStage.h:241` vs `:121-122`, `:202-205`

```cpp
const int bit = rotStream.nextInt (flipBits + 1);   // draws [0, flipBits]
```

**Issue:**
- `kFlipBitMin = 3` and the comment ("one flip in thirteen" is the sign bit,
  i.e. bits 3–15) describe a field of 3..flipBits. The code draws from
  0..flipBits instead.
- At DEPTH 100, the sign bit comes up 1 time in 16, and 3/16 of flips land on
  bits 0–2 (≤ −78 dBFS, inaudible).
- At DEPTH 0, 3/4 of flips are wasted.
- The flip density is thinner than designed everywhere.

**Fix:** `const int bit = kFlipBitMin + rotStream.nextInt (flipBits - kFlipBitMin + 1);`.
It is still one draw, so stream alignment holds, but Rot renders and presets
using Flip will sound denser. Audition this, or decide the current density is
the intended sound and fix the comment instead.

### WR-09: The current preset name is not saved with the session

**File:** `Source/PluginProcessor.cpp:1751-1769` and `1771-1814`

**Issue:**
- State uses `apvts.copyState()`/`replaceState()` directly, and never writes the
  `currentPreset` property.
- 26 other plugins in the suite write it, via `presetManager.getStateAsXml()`
  or by hand.
- Load "Worn Cassette", save, reopen: the band shows "Default", and ◀ ▶ starts
  from the top of the list.

**Fix:**
- In `getStateInformation`, add
  `state.setProperty("currentPreset", presetManager.getCurrentPresetName(), nullptr)`.
- In `setStateInformation`, read it back with an `isVoid()` guard and call
  `setCurrentPresetName`.

### WR-10: Pressing or releasing Shift mid-drag makes the knob jump

**File:** `Source/ui/public/index.html:1702-1706`

```js
const fine = e.shiftKey ? 0.001 : 0.005;
state.setNormalisedValue(clamp01(dragNorm + (dragY - e.clientY) * fine));
```

**Issue:**
- The whole drag distance is re-scaled by the current modifier. After a 100 px
  drag, pressing Shift jumps the value back by 0.4 normalised, and releasing
  Shift jumps it forward again.
- It affects all 30 float knobs.

**Fix:** accumulate per move:

```js
dragNorm = clamp01(dragNorm + (lastY - e.clientY) * fine);
lastY = e.clientY;
state.setNormalisedValue(dragNorm);
```

Seed `lastY` in `pointerdown`.

### WR-11: With hover help on, the Preset tooltip covers the open preset menu

**File:** `Source/ui/public/index.html:285-298` (`.preset-menu { z-index:1200 }`)
and `:2407-2414` (mouseover).

**Issue:**
- `.inner` (`position:absolute; z-index:2`) is a stacking context, so the
  menu's 1200 only ranks inside it. The root-level `.tooltip` (z 1000) paints
  above it, which makes the CSS comment wrong.
- After the click, crossing the inner `#preset-name`/caret spans re-fires
  mouseover. After 350 ms the "Preset" tip opens over the first category header
  and rows.
- Measured by a headless probe: tip rect [212,62 → 442,151], menu rect
  [257,60 → 629,595].

**Fix:**
- In the mouseover handler and in `showTip`, skip targets with
  `aria-expanded="true"`.
- Call `hideTip()` in `openMenu()`.

### WR-12: Pop and tick levels drop 4–10 dB at 96/192 kHz (the ×20 cap)

**File:** `Source/dsp/ArtifactSynth.h:155`, `:176`

**Issue:**
- The level compensation `1/g` (pop) and `(1+g)/g` (tick) is capped at 20.
- At 192 kHz, a 900–1800 Hz pop needs 34–68, so pops come out 4.6–10.6 dB
  quieter than at 48 kHz. At 96 kHz they are up to 4.6 dB quieter.
- Ticks below ~3.2 kHz lose up to ~4 dB at 192 kHz.
- Vinyl crackle balance therefore depends on the session rate.

**Fix:** the cap only guards g → 0, and the cutoffs are always ≥ 900 Hz, so
raise it to ~1000.

## Info

- **IN-01: `DryWetMixer` fades in from 100 % wet at first start**
  (`PluginProcessor.cpp:1301-1304`). `prepare()` snaps the ramp to the default
  mix (1.0) before `setWetMixProportion` sets the saved MIX, so the first 50 ms
  is mostly wet. Fix: call `setWetMixProportion` before `prepare()`.
- **IN-02: Dry path misaligned above ~409.6 kHz** (`PluginProcessor.h:235`).
  `ceil(0.020·fs)` exceeds `kMaxWetLatencySamples = 8192` at 705.6/768 kHz, and
  the Release build clamps silently. Fix: raise the constant to 16384.
- **IN-03: Oversized blocks lose the dry tail.** The `DryWetMixer` FIFO holds
  `nextPowerOfTwo(samplesPerBlock)`, so a block larger than the prepared size
  gets no dry signal at its end (MIX < 100 only).
- **IN-04: `getTailLengthSeconds()` returns 0** (`PluginProcessor.h:73`). Tape
  stops leave the head seconds behind, and CD loops keep playing, so
  bounce-to-silence can truncate. Report ~1–2 s.
- **IN-05: Changing the tooltip or language preference doesn't dirty the
  project** (`PluginEditor.cpp:145-197`). Optionally call
  `updateHostDisplay(ChangeDetails().withNonParameterStateChanged(true))`.
- **IN-06: A preset load with an unchanged SEED may or may not reseed**
  (`PluginProcessor.cpp:1390-1395`). The WR-01 reset-to-default in the preset
  module briefly sets SEED to 0, and whether an audio block lands between that
  and the preset value is a race. WR-02 covers this.
- **IN-07: The Rot comment says a sign-bit flip is a "polarity inversion"**
  (`RotStage.h:202-203` vs `:336-338`). The XOR happens in the int32 domain, so
  the result is a same-sign full-scale spike. Correct the comment.
- **IN-08: Substitute's `subGain *= kMinus1dB` has no floor**
  (`PacketLossStage.h:449`). Long bursts decay into denormal range. Floor it to
  0 below ~1e-5.
- **IN-09: Aperiodic Decay restarts at `gL[0]` every packet with no seam blend**
  (`PacketLossStage.h:470-474`). This steps at each 20 ms boundary for up to 3
  repeats. Reuse the ~1 ms OLA, or document it as intended.
- **IN-10: Latent sticky NaN.** Rot `envSq`, the codec front-end IIR and
  `agcEnv`, and the crush latch `lastL/lastR` have no finite guard. They are
  unreachable today because the input is scrubbed, but a NaN there would kill
  the codec path permanently.
- **IN-11: Media timing edge cases:**
  - A `MediaClock.h:157-164` boundary exactly on `ppqEnd` fires one sample
    early. Use a half-open interval.
  - The `VinylTransport.h:113-118` `pendingLag` can be stale after
    `cd.release()`, which costs a zero-distance jump that still pops.
  - A tape recovery jump during a CD loop (`PluginProcessor.cpp:1579` vs
    `CDSkip.h:341`) relocates the loop. Skip the recovery while
    `isLooping() || isLocked()`.
- **IN-12: Minor artifact/bed issues:**
  - The stop-arm latch survives `installBend`, so a mid-stop 0.5/0.67 bend
    stays at −4 dB and filtered.
  - A scratch retrigger resets `thumpPhase` mid-ring.
  - `ComfortNoise::setLevel` steps per block with no smoothing.
- **IN-13: UI minor issues** (all in `index.html`):
  - A `-0 %` readout at CRUSH_ENV_AMT ≈ 0 (`:1677`).
  - No `lostpointercapture` handler or left-button check on the knob drag
    (`:1697-1715`), so a stolen capture leaves the knob following the hover.
  - The bipolar centre tick touches the "ENV" caption (`:545-552`).
  - Preset-menu category headers stay English in fr/zh-Hans (`:2088`).
- **IN-14: Stale comments and copy:**
  - `PluginEditor.h:43-53` says "TWELVE" native functions; there are 15.
  - `PluginEditor.h:62` says 38 relays; there are 45 (31/8/6).
  - `index.html:1215` claims the page is served with a charset.
  - `index.html:2245, 2429` claim a pointerdown `preventDefault` that isn't
    there.
  - The i18n tooltip at `i18n.js:411-412` promises that saving under a factory
    name "writes a user copy". That copy can't be recalled or deleted: this is
    a shared `OuariconPresetManager` issue (`savePresetToFile` has no factory
    guard).

## Suggested batching

| Batch | Findings | Render impact |
|---|---|---|
| A: host integration | WR-01, WR-02, WR-09, IN-01, IN-04 | none on steady-state renders |
| B: click fixes | WR-03, WR-04, WR-05, WR-06 | event-seam samples change; re-anchor affected goldens |
| C: design drift | WR-07, WR-08, WR-12 | audible; audition, then re-anchor |
| D: UI | WR-10, WR-11, IN-13 | none |
