# O-simpleWavetable - Creative Brief

## Overview

**Type:** Synth (Pedagogical Wavetable)
**Core Concept:** A single-oscillator wavetable synth that makes wavetable synthesis visible: a bank of single-cycle frames, a scan position that moves through it, the cycle being read, and that cycle's harmonics.
**Status:** 💡 Ideated
**Created:** 2026-10-05

## Vision

A wavetable is a bank of single-cycle frames. The synth plays a note by looping one frame at the note's pitch and changes timbre by moving the scan **Position** through the bank. Commercial wavetable synths (Serum, Vital, Pigments) bury that mechanism under warp modes, unison, effects and modulation matrices. O-simpleWavetable shows only the mechanism: **the bank, the frame being read, the cycle that comes out, and its harmonics.**

It accompanies MUSC319 **Week 7 (additive and wavetable synthesis)** and pairs with **O-simpleAdditive**. Additive builds a spectrum by stacking partials. Wavetable stores finished cycles and scans between them. O-simpleAdditive v1.0 stops at a two-frame A→B morph and explicitly excludes a multi-frame bank. This plugin is that multi-frame bank, plus the interpolation, band-limiting and import lessons that only make sense once a bank exists.

Every built-in bank isolates one concept, so each bank is a lesson:

- **Sine → Saw:** the additive build-up as frames, one more harmonic per frame (frame *k* = harmonics 1…*k* at 1/n). Frame 32 reaches harmonic 32, the top of the harmonics graph.
- **Sine → Square:** odd harmonics only.
- **Pulse width:** a narrowing pulse.
- **Formant / vowel:** frames whose spectral peaks move (vowel to vowel), tying to formants on the wk07 page.
- **Drive:** a sine pushed through increasing soft clipping, frame by frame. This bridges to wk05: a waveshaped sine *is* a frame, and scanning this bank sounds like turning up drive.
- **Imported:** a user sample sliced into consecutive 2048-sample frames. This shows where modern wavetables come from.

Like the other O-simple* plugins, it aims for a tight loop between a gesture and its visible result. Move Position and watch the highlighted frame slide through the stack while the cycle reshapes and the harmonic bars grow. Turn Interpolation off and hear the scan step. Play high with Band-limiting off and hear the aliasing, then switch it on and hear it clean up. Drop the bit depth and watch the cycle turn into a staircase.

## Signal Flow

```
  Bank (N frames × 2048 samples; built-in = 32 frames, imported ≤ 256)
        │   Band-limiting ON → per-octave band-limited copies (mip level by note pitch)
        │   Band-limiting OFF → raw full-bandwidth frames (aliases at high pitch)
        ▼
  Effective Position = clamp(Position + LFO·LFO Depth + ModEnv·Env Amount, 0, 1)
        │   (LFO global; mod env per voice)
        ▼
  Frame read:  Interpolation ON → crossfade adjacent frames
               Interpolation OFF → snap to nearest frame (audible steps)
        │   (in-cycle phase read is always interpolated; only frame-to-frame is switched)
        ▼
  Bit-depth quantize (post-interpolation, per output sample; Full = bypass)
        ▼
  × Amp ADSR × velocity   (Poly 16 voices / Mono)
        ▼
  Output Level ─► out
```

## Parameters

*Starting ranges; Stage 0 confirms tapers and exact formulations.*

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Bank | Sine→Saw / Sine→Square / Pulse Width / Formant / Drive / Imported | Sine→Saw | Which bank the oscillator reads. "Imported" is active only after an import. |
| Position | 0–100% | 0% | Scan position across the bank (frame 1 → frame N). The main control. |
| Interpolation | Off / On | On | Off snaps to the nearest frame, so scanning is heard in steps. On crossfades adjacent frames for a smooth morph. |
| Band-limiting | Off / On | On | On uses per-octave band-limited copies. Off reads raw frames, so bright frames alias at high pitches. Made for A/B listening. |
| Bit Depth | Full, 16 → 3 bits | Full | Quantizes the oscillator output after interpolation. Matches the wk07 bit-depth figure. |
| LFO Rate | 0.01–20 Hz (free) / note divisions (sync) | 0.5 Hz | Speed of the global Position LFO. |
| LFO Sync | Free / Tempo | Free | Rate in Hz, or locked to host tempo. |
| LFO Shape | Sine / Triangle / Saw / Square / S&H | Triangle | LFO waveform. |
| LFO Depth | 0–100% | 0% | How far the LFO sweeps Position (bipolar around the Position knob). |
| Mod Env Attack | 0–10 s | 0.5 s | Position-envelope attack. |
| Mod Env Decay | 0–10 s | 1.0 s | Position-envelope decay. |
| Mod Env Sustain | 0–100% | 0% | Position-envelope sustain. |
| Mod Env Release | 0–10 s | 0.5 s | Position-envelope release. |
| Env Amount | −100 – +100% | 0% | How far the envelope moves Position for each note. |
| Amp Attack | 0–5 s | 0.005 s | Amplitude attack. |
| Amp Decay | 0–5 s | 0.3 s | Amplitude decay. |
| Amp Sustain | 0–100% | 80% | Amplitude sustain. |
| Amp Release | 0–5 s | 0.2 s | Amplitude release. |
| Voice Mode | Poly / Mono | Poly | Poly = 16 voices. Mono = last-note priority. |
| Output Level | −inf – +6 dB | −6 dB | Master output. |

**Non-parameter state:** the imported bank (frame data and source filename) is saved inside the plugin state, so projects and presets reopen with the same imported table.

## UI Concept

*Synced from finalized mockup v1 (`.planning/mockups/v1-ui.yaml`, 2026-10-05). The mockup is the source of truth for layout and visuals.*

**Window:** 1120 × 780, fixed size (not resizable).

**Visual Style:** O-simple* field-guide family, matching O-simpleAdditive and O-simpleSubtractive: aged-paper ground, EB Garamond serif, seed cross-section knobs, dark walnut plot wells with sage / amber / brass inks, suite tooltip and header/footer conventions. Projector-readable (9.5 px type floor).

**Layout (top to bottom):**
1. **Header:** title, Bank selector, Import audio button + source filename + frame count, settings gear (hover-help toggle).
2. **Visual band (three panels):**
   - **Bank stack (~45% width):** oblique 3D stack of frames receding back to front (frame 1 in front), current frame inked amber. Interpolation On inks the two neighbouring frames by crossfade weight (readout e.g. "frame 12.4 / 32 · 60% f12 + 40% f13"); Off snaps to one frame. Brass diamond = Position knob value; sage ring = effective Position after LFO + Mod Env. Drag or scroll on the stack to move `position`. Empty-Imported state: dashed hollow stack, "Import audio to fill this bank", pulsing Import button, "0 frames · the oscillator is silent".
   - **Current cycle:** the interpolated, bit-quantized cycle; the quantized staircase is drawn over a faint unquantized trace. Imported banks show a loop-seam marker ("loop seam jumps … → buzz").
   - **Harmonics 1–32:** live bars. Band-limiting On: harmonics dropped by the current octave's band-limited copy show as dashed ghosts. Off: harmonics above Nyquist turn rust with "folds back — aliasing".
3. **Controls (three groups):**
   - **Oscillator:** Position (hero knob), Interpolation, Band-limiting, Bit Depth (Full, 16…3).
   - **Movement:** LFO (Sync, Shape selector, Rate ↔ Division in a shared slot, Depth) | Mod Env (A, D, S, R, Env Amount bipolar).
   - **Amp + Output:** A, D, S, R, Voice Mode (Poly/Mono), Output Level (−60…+6 dB, "-inf" at the floor).
4. **Lesson presets row:** Stepped vs Smooth, Alias Demo, Drive Sweep, Vowel Pad, 8-bit PPG.
5. **On-screen keyboard:** 2 octaves + C, computer keys A–K / W–U, Z/X or arrows for octave.

**Tooltips:** plain-language on-hover help for every control, localized per suite convention. The Band-limiting, Formant and Import tooltips name the aliasing, formant-shift and loop-buzz lessons.

## Use Cases

- **Classroom demo (wk07):** the instructor projects the plugin, steps through each bank, toggles Interpolation and Band-limiting for A/B listening, and drops the bit depth.
- **Self-directed student learning:** students reproduce the teaching outcomes on their own and save an evolving wavetable patch to their A2 palette.
- **Additive ↔ wavetable bridge:** opened next to O-simpleAdditive, the Sine→Saw bank shows that a frame is a frozen additive spectrum.
- **Saturation bridge (wk05):** the Drive bank shows that waveshaping a sine generates frames.
- **Where wavetables come from:** import a sample, see it sliced into frames, and scan through it.
- **Simple creative synth:** playable enough for evolving pads and basses.

## Teaching Outcomes

- Explain a wavetable as a stored bank of single cycles read at the note's pitch.
- Make timbre evolve over a held note by modulating Position (LFO, envelope).
- Hear the difference between stepped and interpolated scanning.
- Hear aliasing and its fix through band-limiting.
- Connect wavetable to additive (a frame is a frozen additive spectrum) and to saturation (a waveshaped sine is a frame).

## Inspirations

- **O-simpleAdditive / O-simpleFM:** sibling teaching instruments (small control set, gesture → visible result, live visuals, tooltips, presets that each isolate one concept).
- **PPG Wave / Waldorf Microwave:** the original stepped wavetable scanning and bit-depth grit.
- **Serum / Vital / Pigments:** the modern 3D frame-bank view and audio-to-wavetable import, here without the warp modes, unison and mod matrix.
- **MUSC319 wk07 page:** the frame-bank and bit-depth figures that the panels mirror.

## Technical Notes

- **Bank format:** 2048 samples per frame. Built-in banks have 32 frames, generated at load time from formulas (additive sums / waveshaping), not shipped as audio. Each frame is DC-removed and the bank normalized to one peak level, so Position doesn't jump in loudness.
- **Band-limiting:** per-octave mipmaps (~10 levels), built by FFT, zeroing harmonics above the level's Nyquist limit, then inverse FFT. Built off the audio thread whenever the bank changes. Band-limiting Off reads the raw table at any pitch, which is the intended aliasing.
- **Interpolation:** the toggle switches **frame-to-frame** behaviour only (nearest frame vs crossfade). The phase read inside the cycle always stays interpolated (linear or cubic; research decides) so "off" teaches stepping, not a second unrelated artifact. Smooth Position changes so a knob move doesn't zipper with Interpolation On. With it Off, the steps are deliberate.
- **Bit depth:** quantize each oscillator output sample after interpolation, before the amp envelope (so the envelope doesn't add steps of its own). Full = bypass.
- **Formant bank:** harmonic frames weighted by vowel formant envelopes moving across the bank (e.g. A→E→I→O→U), designed at a reference fundamental. Teaching point for the tooltip: wavetable formants are baked into the cycle, so they shift with pitch, unlike real vocal formants.
- **Sine → Square:** with 1/n odd harmonics and 32 frames, decide in research whether frame *k* adds one odd harmonic (frame 32 = h63, above the 1–32 graph) or the build-up reaches h31 by the last frame. Prefer whatever keeps the graph informative on every frame.
- **Drive bank:** frame *k* = normalized tanh(g_k·sin), with g rising across the bank (range set in research to sound like a drive knob sweep).
- **Import:** decode the file with JUCE's AudioFormatManager off the audio thread, sum to mono, and take consecutive 2048-sample slices with no resampling or pitch detection (up to 256 frames, partial tail dropped). Normalize, build mipmaps, and swap the bank in RT-safely. Arbitrary slices are not pitch-synchronous, so they buzz at the loop point; the tooltip should name this as part of the lesson. Store imported frames in plugin state (research decides the compact encoding; 256×2048 float ≈ 2 MB raw).
- **Modulation:** one global LFO (free Hz or host-tempo sync; sine/tri/saw/square/S&H) and one per-voice ADSR, both hard-wired to Position. No modulation matrix.
- **Visual handoff:** the panels show the most recently triggered voice's effective Position (and current mip level for the harmonics panel, so the bars match what is heard). Lock-free audio→UI handoff, no allocation on the audio thread.
- **Polyphony:** 16 voices (suite convention), or Mono with last-note priority. Velocity → amplitude.
- **Platform:** JUCE 8 WebView UI, consistent with the O-simple* suite. Windows WebView2 flags (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`) per project standards.
- **Factory presets:** each isolates one concept, e.g. Stepped vs Smooth, Alias Demo (high register, band-limiting off), Drive Sweep (env → Position), Vowel Pad (slow LFO on Formant), 8-bit PPG.

## Out of Scope (v1.0)

- Filters, unison/detune, warp modes, FX, a second oscillator.
- Drawing your own frames (a possible later version).
- A modulation matrix or any modulation destination other than Position.
- Pitch-synchronous / resampled import (v1.0 slicing is deliberately literal).

## Next Steps

- [ ] Create UI mockup (`/start O-simpleWavetable` → option 3)
- [ ] Start planning / DSP research (`/plan O-simpleWavetable`)
