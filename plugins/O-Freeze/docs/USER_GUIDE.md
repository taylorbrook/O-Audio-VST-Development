# O-Freeze User Guide

Complete documentation for the O-Freeze granular freeze effect.

## Getting Started

O-Freeze is a granular freeze effect that captures and sustains audio. Insert it on any track where you want to create sustained textures from transient sounds.

### Quick Start

1. Insert O-Freeze on a track (instrument, vocal, drum bus, etc.)
2. Play audio through the plugin
3. Click the central FREEZE button to capture the sound
4. Adjust DRIFT to add movement to the frozen texture
5. Click FREEZE again to release

## Modes

### Manual Mode

In Manual mode, you control when the freeze engages and releases using the FREEZE button.

**Use cases:**
- Performance effects (freeze on dramatic moments)
- Sound design (capture and manipulate specific sounds)
- Creative transitions (sustain into a new section)

**Workflow:**
1. Ensure MODE is set to "Manual"
2. The THRESHOLD knob is disabled in this mode
3. Click FREEZE to engage/disengage

### Threshold Mode

In Threshold mode, the freeze automatically engages when input level drops below the threshold.

**Use cases:**
- Sustaining reverb tails
- Filling gaps in sparse arrangements
- Creating ambient textures from decaying sounds

**Workflow:**
1. Set MODE to "Threshold"
2. Adjust THRESHOLD knob to the desired level
3. The FREEZE button is disabled in this mode
4. Freeze engages when signal drops below threshold
5. Freeze releases when signal rises above threshold + 3dB (hysteresis prevents flutter)

## Drift Control

The DRIFT parameter adds randomization to grain playback positions, creating movement in the frozen texture.

| Setting | Character | Best For |
|---------|-----------|----------|
| 0% | Static, perfect loop | Sustained notes, clean holds |
| 10-25% | Subtle shimmer | Natural sustain |
| 25-50% | Gentle movement | Pads, ambient textures |
| 50-75% | Noticeable variation | Sound design |
| 75-100% | Granular texture | Experimental effects |

**Tip:** Start at 25% (default) and adjust based on your source material. Tonal sources often benefit from lower drift, while noisy sources can handle more.

## Spectral Engine (v2.9.0)

The ENGINE toggle beside MODE chooses how the freeze is made. **Granular** (the
default, and the only engine before 2.9.0) loops overlapping grains of the captured
audio. **Spectral** freezes the sound's spectrum: an 8192-point STFT with 75 %
overlap analyses the audio continuously, and on freeze the newest AVG_FRAMES frames are
averaged into one magnitude spectrum that is resynthesised for as long as the freeze
is held.

In Spectral the Size knob becomes **Diffusion**, the Grains knob becomes **Avg**,
Reverse dims and the LFO group reads **Diffusion LFO**.

| Control | In Spectral |
|---------|-------------|
| Diffusion (0-100 %) | Random phase step per frame. 0 % = locked, tonal hold (phase-locked vocoder); 100 % = smooth, diffuse wash |
| Avg (1-16 frames) | Frames averaged into the frozen spectrum. 1 = a single snapshot (can ring metallically on noise); 8 (default) or more = smoother. Applies at the next freeze |
| Drift | Each frequency's level wanders slowly and independently, up to about +-3 dB |
| Detune | Per-frequency spread: low partials detune gently, high ones shimmer. Not a clean pitch shift |
| LFO (Rate / Depth / Shape) | Modulates Diffusion |
| Mix, Freeze, Threshold, Mode | As in Granular |

**Tips**

- Diffusion 0 % on a held chord or vowel gives the most "photographic" freeze;
  20-40 % loosens it; 70-100 % turns any source into an airy pad.
- On noisy sources (breath, cymbals, room tone) keep Avg at 8 or above.
- Switching ENGINE while frozen crossfades the two engines over 50 ms. Switching to
  Spectral while frozen analyses the frozen audio on the spot.
- At 96/192 kHz the FFT size stays 8192 points, so the analysis window is shorter
  (43 ms at 192 kHz) and very low notes (below about 60 Hz) smear more.
- Spectral output is not latency-compensated: the frozen sound is a capture of the
  last fraction of a second before the freeze, as in Granular.

## Mix Control

The MIX parameter blends between dry (original) and wet (frozen) signals.

| Setting | Behavior |
|---------|----------|
| 100% (default) | Full wet - only frozen signal when engaged, full dry when released |
| 50% | Parallel blend - frozen signal layered with dry |
| 0% | Bypass - dry signal only |

**Tip:** Try 50-70% mix for parallel freeze effects where you want the original transients to punch through while the freeze adds sustain.

## Technical Details

### Granular Engine

O-Freeze uses 8 overlapping grains with 87.5% overlap for smooth, artifact-free sustain:

- **Grain size:** 200ms
- **Overlap:** 87.5% (grains trigger every 25ms)
- **Window:** Asymmetric Blackman-Harris (60% attack, 40% release)
- **Buffer:** 2 seconds at up to 192kHz

### Crossfade System

Transitions are smoothed to prevent clicks:

- **Fade-in:** 50ms when freeze engages
- **Fade-out:** 100ms when freeze releases

### Threshold Gate

The threshold detection uses RMS level measurement:

- **Window:** 20ms rolling average
- **Hysteresis:** 3dB (prevents flutter near threshold)

## Troubleshooting

### Freeze sounds clicky or glitchy

- Ensure you're not engaging freeze at the exact moment of a transient
- Try increasing DRIFT slightly for smoother texture
- Check that your DAW buffer size is at least 128 samples

### Threshold mode keeps triggering

- Lower the THRESHOLD value (more negative dB)
- The 3dB hysteresis should prevent rapid on/off, but very dynamic material may need a lower threshold

### Audio sounds different when frozen

This is expected behavior. The granular engine creates a looped texture that may differ from the original transient. Use DRIFT at 0% for the most static reproduction.

### High CPU usage

- Increase your DAW buffer size
- O-Freeze is optimized but runs 8 simultaneous grains
- Typical CPU usage is 15-25% at 256-sample buffers

### Plugin not appearing in DAW

1. Ensure plugins are in correct folders:
   - VST3: `~/Library/Audio/Plug-Ins/VST3/`
   - AU: `~/Library/Audio/Plug-Ins/Components/`
2. Clear AU cache: `rm -rf ~/Library/Caches/AudioUnitCache/`
3. Restart DAW and rescan plugins

## Version History

See [CHANGELOG.md](CHANGELOG.md) for version history.

---

*Ouaricon Development - Taylor Brook*
