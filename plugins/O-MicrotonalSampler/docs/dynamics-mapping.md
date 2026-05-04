# O-MicrotonalSampler — Dynamics Mapping

**Version:** 1.15.0
**Audience:** Composers using O-MicrotonalSampler in a DAW or in Dorico,
plus anyone setting up an expression-map for orchestral mockups.

This document explains how the plugin handles dynamics — both how a
note's volume is determined at note-on and how it can be modulated
during sustain. Get this wrong and you'll either pick the wrong sample
layer or lose all dynamic shaping in long notes; get it right and the
plugin behaves the way Sibelius / Dorico / orchestral-library users
expect.

---

## TL;DR

| Path | What it controls | When it acts |
|------|------------------|--------------|
| **Note-on velocity** | Which velocity *layer* plays (sample selection). | Only at note-on. Locked for the lifetime of the note. |
| **CC 11 ("Expression")** | Continuous post-mix gain. Wired to the `Expression` APVTS parameter (squared curve, 10 ms smoothing). | Continuously — modulates volume during sustain. |
| **Output Gain** | Master output trim, post-expression. | Continuously, but typically a static knob. |

**Recommended Dorico exp-map setting: `<volumeType><type>kCC11</type></volumeType>` ("CC11 Expression").**
Velocity then handles layer selection at note-on, and CC11 handles
sustained dynamic shaping — the two-axis split orchestral libraries
have used for decades.

---

## How velocity selects a sample layer

1. Note-on arrives with a 0–127 velocity byte.
2. `MicrotonalSamplerVoice::startNote` clamps it to 1..127.
3. The voice reads the loaded `SampleMap`'s `numVelocityLayers` (1, 2,
   4, or 8 — set automatically by the loader, or by the user via the
   folder-load options modal).
4. The 128-wide velocity space is split into `numVelocityLayers`
   consecutive bands of width `128 / numVelocityLayers`.
5. The voice picks the band that contains `(velocity - 1)`:

```
layer_index = jlimit(0, N - 1, (vel - 1) / (128 / N))
```

Examples (the unit test `dynamics_layer_check` pins these boundaries):

| numLayers | Band 0 | Band 1 | Band 2 | Band 3 |
|-----------|--------|--------|--------|--------|
| 1 | 1..127 | — | — | — |
| 2 | 1..64 | 65..127 | — | — |
| 4 | 1..32 | 33..64 | 65..96 | 97..127 |
| 8 | 1..16 | 17..32 | 33..48 | 49..64 (… 49..64 thru 113..127) |

**Key consequence:** velocity is a *layer-pick* signal, not a gain
modulator. Once the voice has decided which layer to play, the
velocity byte does **not** continue to influence loudness during
sustain. That dynamic shaping is what CC 11 is for.

A subtle corner case: `velocity_crossfade` is a separate APVTS
parameter (0..1) that controls how much the **adjacent** layer is
blended underneath the primary layer at note-on. This crossfade ratio
is also captured at note-on and frozen — same contract as the layer
selection itself.

---

## How CC 11 ("Expression") modulates sustain

The plugin's `Expression` APVTS parameter is bound to MIDI CC 11. The
audio thread:

1. Reads the latest CC 11 byte off the host's MIDI buffer in
   `processBlock`.
2. Forwards it to the host (via `handleAsyncUpdate`) so automation
   recording and parameter listeners see the change.
3. Applies a **squared curve** (`gain = expression²`) — this is the
   industry-standard expression-pedal response; equal-step CC values
   feel equal-perceptual at the listener.
4. Runs the squared target through a **10 ms linear smoother** before
   applying it to the post-mix buffer with `applyGainRamp`.

The result is continuous, click-free volume control during a sustained
note, independent of the velocity layer that is currently playing.

---

## Recommended Dorico exp-map setting

Dorico's expression-map XML has a `<volumeType>` field that controls
which signal Dorico uses to drive a "primary volume curve" through the
notation's dynamic markings.

The relevant choices:

| `<type>` value | Meaning | Match to O-MicrotonalSampler |
|----------------|---------|------------------------------|
| `kCC11` | Continuous CC 11 modulation. **Recommended.** | Smooth swells, hairpins, and dynamic markings shape **sustained** volume. Layer crossfade is fixed at note-on (the player picked one of the four layers; Dorico can't second-guess that mid-note). |
| `kNoteVelocity` | Note-on velocity drives volume directly. | Layer selection follows the dynamic marking, but a sustained `< … >` hairpin will NOT swell — velocity is locked at note-on. Useful for short, articulated passages where dynamic-marking-to-layer-selection matters more than swell control. |
| `kCC1` | Modulation wheel as primary volume. | Not idiomatic for a sampler — reserve CC 1 for vibrato or another orchestral-library convention. |
| `kCC7` | Channel volume as primary. | Works, but conventional orchestral-library practice reserves CC 7 for global track volume. Prefer CC 11. |
| `kAftertouch` | Channel pressure. | Not exposed by the plugin. |

**Default recommendation: ship `<volumeType><type>kCC11</type></volumeType>`** in the bundled `.doricolib` (v1.16.0). Users who prefer
velocity-driven dynamics for short / articulated passages can switch
the volumeType to `kNoteVelocity` in Dorico's UI without rebuilding
the exp-map.

---

## Secondary volume control (Dorico 3+)

Dorico 3+ added a `<secondaryVolumeType>` slot in the expression-map
schema. Today the secondary type **mirrors** the primary type — it
exists for forward compatibility. Future Dorico versions are expected
to allow independent curves: e.g. primary = velocity (selects the
right layer at note-on) + secondary = CC 11 (smooths volume within
the layer) — the dual-axis dynamics model that orchestral mockups
have always wanted.

When that future version ships, the sampler is ready: layer selection
is already velocity-driven (note-on only), and the post-mix path is
already CC 11-driven (continuous). Updating the bundled `.doricolib`
to use both axes will be a docs-only change.

---

## What the plugin does NOT do

- **Velocity does not continuously modulate volume.** A sustained
  note ignores velocity changes that arrive after note-on — same as
  every keymapped sampler since the early '90s.
- **CC 11 does not change which sample layer is playing.** A swell
  from `pp` to `ff` driven by CC 11 will be a smooth gain ramp on
  whichever layer was picked at note-on; it will not crossfade to a
  brighter velocity layer mid-note. Layer crossfade is a
  note-on-time decision.
- **No "humanize" / random-velocity offset.** What you send is what
  the layer-picker sees. Round-robin variance happens at the
  *variant* axis, not at the velocity-layer axis.

---

## Quick sanity test in your DAW

1. Load a multi-layer library (4 layers recommended).
2. Hold a long note at a fixed velocity.
3. Send a CC 11 ramp from 0 to 127 over 5 seconds.
4. You should hear a smooth post-mix swell. The timbre should NOT
   change — same layer, same sample, just louder.
5. Now release, then hold the same note at a higher velocity.
   The timbre should noticeably change (you've crossed a layer
   boundary). Confirm by sending the same CC 11 ramp — same swell
   shape, different starting timbre.

If the swell is stepped or zippered, your DAW is sending CC 11 at
< 30 Hz — increase the controller resolution. The plugin's 10 ms
smoother handles the rest.

---

*See also: `tests/dynamics_layer_check.cpp` (pins the velocity-bucketing
boundaries) and `Source/PluginProcessor.cpp` `processBlock` (the
authoritative implementation of the CC 11 path).*
