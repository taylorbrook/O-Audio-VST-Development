# O-Tapestop — code audit queue (2026-08-16, from v1.3.0)

Findings from a full read of the processor, all six DSP headers, the editor and
the UI JS. Work through **in order** — `/clear` between each. Each command is
self-contained (assume no memory of this session).

Ordering rationale: bugs before cleanup, because S1/S2 delete fields in the
same file B1/B3 edit. B2's DSP change is last among the bugs because it needs a
harness measurement first. The preset refactor is last — largest churn, zero
behaviour change.

---

## 1. B1 — Scratch LUT double-buffer race  *(PATCH → 1.3.1)* — ✅ DONE 2026-08-17

**Resolved in v1.3.1.** Fixed as prescribed: `TapestopTransport` owns a
`std::array<float, ScratchLutSize>` and `engageScratch()` memcpys the published
LUT into it before pointing `scratchLut` at it. The `ScratchEnvelope.h` thread
contract now states where the guarantee actually comes from. Acceptance met by
a new harness probe `scratch-envelope-edit-midpass-inert` — two mid-pass
`commitScratchEnvelopeJson()` calls leave the in-flight pass bit-identical.
Confirmed a real discriminator: it FAILED against the pre-fix build at sample
52096, the exact sample of the second commit. Harness 66/66.


`ScratchEnvelope::bakeAndPublish()` (Source/dsp/ScratchEnvelope.h:255-268) picks
its write target as "whichever of bufA/bufB is not currently published". The
audio thread latches a **raw `const float*`** into one of those buffers at the
engage edge (PluginProcessor.cpp:840-842 → TapestopTransport.h:248) and reads it
for the entire pass — up to 8 s (ENV_FREE_MS max).

Failure sequence:

```
published = A ; audio thread latches A and starts a pass
edit 1 → bake writes B, publishes B      (safe — audio still on A)
edit 2 → bake writes A                   (UNSAFE — audio is reading A)
```

Reachable in normal use: `envelope_editor.js:384-388` commits 50 ms after every
pointer-up, so any two curve edits while ENGAGE is latched trigger it. Result is
a torn envelope mid-pass (an `r` discontinuity — audible speed jerk) and a
formal data race. The ScratchEnvelope.h header claim that "edits mid-pass affect
the NEXT pass only" is not structurally true — double buffering only survives
ONE outstanding reader generation.

Fix: have the audio thread take its own copy at the engage edge. Add
`std::array<float, ScratchLutSize> scratchLutCopy;` to TapestopTransport, and in
`engageScratch()` memcpy the 2048 floats into it before pointing `scratchLut` at
it. 8 KB, no allocation, no lock — RT-safe, and it makes the latch contract
provable rather than probabilistic.

Acceptance: two `commitScratchEnvelopeJson()` calls during a live pass leave the
in-flight pass bit-identical to the pre-edit envelope; harness suite still green.

---

## 2. B3 — trim never lands on exactly 0 at the resync→Bypassed handoff  *(PATCH → 1.3.2)* — ✅ DONE 2026-08-17

**Resolved in v1.3.2, with one correction and one addition to the prescription
below.**

*Correction:* latching `trimTarget` "from the state as it was on entry to
`tick()`" does **not** satisfy this item's own acceptance criterion. The resync
fade is exactly `xfLenSamples` ticks, and the FIRST of them is the tick where
the switch calls `enterResync()` — so on entry that tick still reads `Catchup`
(or `ScratchPass`) and an entry latch loses a decrement, landing trim on
`1/xfLen` rather than 0. The target must be latched *between* the state machine
and the crossfade-completion block: the only point that reads `ResyncXfade` on
all `xfLenSamples` ticks.

*Addition:* the ordering fix alone still could not land on exactly 0, because
the ramp was a **float accumulator** — `trim ± trimStep` clamped at the rails
only reaches a rail if accumulated rounding overshoots it, which is a property
of the sample rate. Measured: exact at 44.1/48/192 kHz, but leaving 4.07e-5 /
1.88e-5 / 6.73e-5 at 88.2/96/176.4 kHz. The ramp is now an integer position over
`[0, xfLenSamples]` with both rails snapped.

Acceptance met by a new transport-level harness probe
`B3-trim-exact-rails-6-rates` — transport-level because the final fade tick
renders dry (`PluginProcessor.cpp` re-checks `isBypassed()` after `tick()`), so
no rendered waveform can discriminate the value; rate-swept because a 48 kHz
probe passes over the accumulator defect entirely. Confirmed a real
discriminator for both defects independently: pre-fix v1.3.1 fails at all 6
rates, and the correct latch with the accumulator restored fails at exactly the
3 predicted rates. Output byte-identical at the 0 dB OUTPUT_GAIN default
(hash `77815f94d22f1c13` matches v1.3.1); worst case 0.0025 dB difference during
the 50 ms fade at +12 dB. Harness 67/67; pluginval s10 SUCCESS; auval SUCCEEDED.

---

In `TapestopTransport::tick()` the state flips to `Bypassed` **inside** the
crossfade-completion block (TapestopTransport.h:526-530). The trim update below
it (line 546) then reads `state == Bypassed`, so `trimTarget` becomes 1.0 and
`trim` ticks back UP on the final fade sample instead of reaching 0.

`trim` ends at ~2/xfLen (≈8.3e-4 @ 48 kHz) instead of 0, and the last sample
that actually consumes `trimAmount` sees ~1/xfLen. Worst case that is a 0.011 dB
step at OUTPUT_GAIN = +12 dB — inaudible. This is a correctness/comment defect,
not an audio one, but the file header claims trim "lands on EXACTLY 1.0 as the
fade ends" and `reset()` documents the Bypassed baseline as 0. Neither holds.

Fix: latch `trimTarget` from the state as it was on entry to `tick()`, before
the crossfade block can flip it.

Acceptance: trim is exactly 0.0f once the resync fade completes; post-resync
bitwise-dry probes stay green.

---

## 3. B2a — correct the splice-law documentation  *(PATCH, comments only)*  — ✅ DONE 2026-08-17

**Resolved in v1.3.3.** Both comment blocks rewritten to describe the
implemented raised-cosine equal-gain law, including the correlated-vs-
decorrelated table (flat 0 dB correlated; power sum `(1+cos^2(pi*phi))/2` → 0.5,
a 3.01 dB dip, decorrelated) and the note that the square roots would invert the
trade. The `Linear` law's own 3.01 dB decorrelated dip is now documented too,
since it is also amplitude-sum-1. Enum name deliberately untouched, per this
item's instruction to leave it for step 4.

Comments-only was **verified, not assumed**: both `WindowLut.h` and
`TapestopTransport.h` hash byte-identical from `#pragma once` onward against the
v1.3.2 backup, so no DSP, parameter or enum change is possible in this commit.
Harness 67/67.

⚠️ **Item 4's step 1 is already done** — see the table under item 4. The harness
run that verified this change produced the dip measurement, and it changed what
the comments say: the dip is ~6 dB, not the 3.01 dB the analytic law predicts.
The comments were written to the measured figure, not the analytic one.



```
fadeOut = hann(0.5 + phi/2) = cos^2(pi*phi/2)
fadeIn  = hann(phi/2)       = sin^2(pi*phi/2)
fadeOut + fadeIn = 1        <- amplitude sum, not power sum
```

True equal-power needs the square root of those. Two comment blocks state the
opposite of what the code does:

- `Source/dsp/WindowLut.h:36-38` — "Equal-power by construction (sin^2 + cos^2 = 1)"
- `Source/dsp/TapestopTransport.h:64-66` — "(sin^2 + cos^2 = 1 — but over-sums
  CORRELATED material by up to +3 dB)". An amplitude-sum-of-1 law sums correlated
  material to exactly 0 dB; it is the *true* equal-power law that over-sums by
  +3 dB. The sentence describes a law that isn't implemented.

Fix comments only in this step — describe the implemented raised-cosine
equal-gain law and note that it dips ~3 dB on decorrelated material. Leave the
enum name alone until step 4 decides whether the DSP changes.

---

## 4. B2b — measure and (maybe) fix the mid-resync power dip  *(MINOR — changes the sound)*

Consequence of B2a: every resync crossfade splices the live-head rider against a
fading voice seconds behind it — decorrelated material. An amplitude-sum-of-1
law gives a ~3 dB power dip in the middle of that 50 ms fade, i.e. a dip at the
moment the tape returns to speed.

The harness already has the measurement rig: `tests/render-harness/main.cpp`
lines ~1045-1115 (`AB-splice-equal-power` / `AB-splice-linear`) computes bump and
dip in dB, but the assertion only gates on `|bump| < 4.0`, so a dip of any size
passes. That is why this shipped.

**Step 1 is DONE (2026-08-17, during v1.3.3).** Measured:

| law | bump | dip |
|---|---|---|
| `AB-splice-equal-power` (raised cosine, shipped) | −0.48 dB | **−6.21 dB** |
| `AB-splice-linear` | −0.58 dB | **−6.99 dB** |

The dip is **real and roughly twice the analytic 3.01 dB**, so step 2 is live.
Two caveats for whoever picks this up:

- The 3.01 dB figure is the law's floor for two *equal-power* decorrelated
  sources. The extra ~3 dB is not the gain law — it is the fading voice being a
  varispeed read of different material at a different level. **A sqrt law will
  therefore not recover the full 6 dB**; it removes the law's 3 dB share only.
  Budget the expected improvement accordingly before judging the A/B.
- The probe's window is the 50 ms fade padded by ±480 samples, and `dip` is the
  min 480-sample RMS in it against a post-resync reference — so it also catches
  anything in the catchup tail, not just the crossfade.

Remaining:

1. ~~Run the harness and read the reported `dip=` for both laws.~~ ✅ above.
2. If the dip is real (≈3 dB), add a true equal-power option — `sqrt` of the
   current gains, or a `readAtSqrt()` on WindowLut — and A/B it.
3. Tighten the harness check to gate on dip as well as bump, so a regression
   here cannot ship silently again.

This is the only item in the queue that changes how the plugin sounds. Skip it
if the current return swell is wanted as a character.

---

## 5. Dead state removal  *(PATCH)*

All confirmed by grep across `Source/` and `tests/` — no reads anywhere.

| Symbol | Location | Status |
|---|---|---|
| `VarispeedVoice::gain` | VarispeedVoice.h:74 | never read *or* written |
| `VarispeedVoice::active` | set/cleared 8× in TapestopTransport.h | write-only |
| `ContinuousMotion::eventTargetR` | ContinuousMotion.h:148, 674 | assigned in reset() only |
| `ContinuousMotion::eventForcedSnap` | ContinuousMotion.h:145,196,519,522 | write-only |
| `ContinuousMotion::slotCount` | ContinuousMotion.h:509-510, 669 | member, used only inside `recomputeSlots()` → make it local |

Also: `ContinuousMotion::reset()` (lines 126-158) skips `samplesSinceJump`,
`lastXfLen`, `snapEmitted` and `shuffleEmitted`, while `engage()` resets some of
them. Harmless today — member initialisers cover construction, and `prepare()`
calls `reset()` before any engage can land — but it is an easy trap for the next
edit. Make `reset()` cover the full state set.

Acceptance: harness suite green and byte-identical output (this is pure deletion
— any waveform change means something was load-bearing after all).

---

## 6. Structural de-duplication  *(PATCH)*

- **Dead switch case.** `TapestopTransport::release()` lists
  `case State::ScratchPass:` at line 327, but the early
  `if (state == State::ScratchPass)` at line 296 already returned. Unreachable.
- **The double-ended debt clamp is spelled out four times verbatim** —
  TapestopTransport.h:475-483 (carrier), 511-519 (fading voice), 582-586
  (`spliceCarrierTo`), and again in `VarispeedVoice::read()` at
  VarispeedVoice.h:97-102. Extract one helper
  (`clampToRing(pos, ring) -> double`) and call it from all four. Four copies of
  a safety invariant is four places for it to drift.
- **`chans`** (PluginProcessor.cpp:889) exists only to compute `chR` on the next
  line. Inline it.

Acceptance: byte-identical harness output.

---

## 7. Factory-preset table refactor  *(PATCH, optional)*

`PluginProcessor.cpp:236-523` — 28 presets × 19 params ≈ 290 lines, where 13 of
the 19 entries per preset are identical boilerplate carried only to satisfy the
"every preset lists all 19 IDs" defence-in-depth rule. A `basePreset()` +
per-preset override map collapses this to ~90 lines while still emitting all 19
IDs per preset.

Largest churn in the queue and zero behaviour change, so it goes last. Gate it
hard: dump the generated preset JSON before and after and diff — it must be
byte-identical, including the CR-02 `convertTo0to1` round-trip on the three
skew-0.35 FREE_MS ranges.
