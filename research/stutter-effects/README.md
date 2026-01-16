# Stutter Audio Effects Research

**Generated:** 2026-01-14
**Research Level:** Deep (Level 3 - Parallel Investigation)

---

## Overview

This folder contains comprehensive implementation guides for three distinct approaches to building stutter/glitch audio effects in JUCE. Each path offers unique features and targets different use cases.

---

## Implementation Paths

### [Path A: Granular Stutter Engine](path-a-granular-stutter-engine.md)

**Extend existing Scatter plugin with beat-synchronized triggering**

| Aspect | Details |
|--------|---------|
| Development Time | 1-2 weeks |
| Complexity | Medium |
| Starting Point | 80% complete (Scatter plugin) |
| Unique Feature | Harmonic scale-quantized stutters |

**Key Features:**
- Beat-sync grain triggering (1/4, 1/8, 1/16, triplets)
- Freeze mode (capture and hold)
- Pitch ladder mode (arpeggiated repeats)
- Euclidean rhythm patterns
- Density morphing (stutter → granular cloud)

**Best For:** Textural effects, melodic stutters, ambient processing

---

### [Path B: Multi-Lane Beat Repeater](path-b-beat-repeater.md)

**Simple buffer capture with polyrhythmic capabilities**

| Aspect | Details |
|--------|---------|
| Development Time | 2-3 weeks |
| Complexity | Medium |
| Starting Point | New plugin |
| Unique Feature | 4 independent polyrhythmic lanes |

**Key Features:**
- 4 simultaneous repeat lanes with different subdivisions
- Per-lane decay, pitch shift, filter sweep
- Tape degradation simulation (saturation, wow/flutter, hiss)
- Envelope follower triggering
- Sidechain input for external triggering

**Best For:** DJ performance, EDM builds, rhythmic glitch

---

### [Path C: Playhead Modulator](path-c-playhead-modulator.md)

**Drawable envelope controls playback position (TimeShaper style)**

| Aspect | Details |
|--------|---------|
| Development Time | 4-6 weeks |
| Complexity | High |
| Starting Point | New plugin |
| Unique Feature | Harmonic locking during speed changes |

**Key Features:**
- Drawable bezier envelope editor
- Multiband time manipulation (independent per band)
- Harmonic locking (pitch snaps to scale during speed change)
- Physics-based envelope behavior (gravity, bounce)
- Motion recording (perform scratches, save as preset)

**Best For:** Tape stop effects, scratching, creative time manipulation

---

## Comparison Matrix

| Feature | Path A | Path B | Path C |
|---------|--------|--------|--------|
| Existing Code Base | 80% | 0% | 0% |
| Development Effort | Low | Medium | High |
| CPU Usage | High | Low | Medium |
| Latency | Low | Low-Med | Zero |
| UI Complexity | Low | Medium | High |
| Beat Sync | ✓ | ✓ | ✓ |
| Multiband | ✗ | ✗ | ✓ |
| Pitch Shifting | ✓ | ✓ | ✓ (via rate) |
| Scale Quantization | ✓ | ✗ | ✓ |
| Granular Engine | ✓ | ✗ | ✗ |
| Drawable Curves | ✗ | ✗ | ✓ |
| Physics Simulation | ✗ | ✗ | ✓ |

---

## Recommended Approach

**Start with Path A** because:
1. 80% of code already exists in Scatter plugin
2. Lowest risk - adding features vs. building from scratch
3. Highest differentiation - no plugin combines granular + scales + beat-sync
4. Fastest to market - functional in days, polished in weeks

**Then consider Path B** as a companion "lite" plugin for users who want simple rhythmic stutters without granular complexity.

**Path C is the moonshot** - highest effort but most innovative. The harmonic locking feature doesn't exist in any commercial plugin.

---

## Research Sources

### Academic/Technical
- Ross Bencina: "Implementing Real-Time Granular Synthesis"
- Curtis Roads: "Microsound" (granular theory)
- Will Pirkle: "Designing Audio Effect Plugins in C++"

### Commercial Plugin Analysis
- Sugar Bytes Effectrix 2 (step sequencer architecture)
- iZotope Stutter Edit 2 (gesture triggering)
- Image-Line Gross Beat (spline envelope system)
- Cableguys TimeShaper (playhead modulation)
- Output Portal (granular + scale quantization)
- Native Instruments Replika XT (diffusion algorithms)

### Open Source References
- [Argotlunar](https://github.com/mourednik/argotlunar) - Granular delay (GPL)
- [TIME-12](https://github.com/tiagolr/time12) - Envelope-based stutter
- [RSBrokenMedia](https://github.com/reillypascal/RSBrokenMedia) - Buffer subdivision glitch
- [Delayyyyyy](https://github.com/ejaaskel/Delayyyyyy) - Beat repeater

### JUCE Documentation
- [dsp::DelayLine](https://docs.juce.com/master/classdsp_1_1DelayLine.html)
- [AbstractFifo](http://docs.juce.com/master/classAbstractFifo.html)
- [AudioPlayHead::PositionInfo](https://docs.juce.com/master/classAudioPlayHead_1_1PositionInfo.html)
- [SmoothedValue](https://docs.juce.com/master/classSmoothedValue.html)

---

## Files in This Folder

| File | Description |
|------|-------------|
| `README.md` | This summary document |
| `stutter-effects-research-findings.md` | Raw research findings from Level 1-3 investigation |
| `path-a-granular-stutter-engine.md` | Full implementation guide for Path A |
| `path-b-beat-repeater.md` | Full implementation guide for Path B |
| `path-c-playhead-modulator.md` | Full implementation guide for Path C |

---

## Next Steps

1. **Choose a path** based on your goals and timeline
2. **Review the detailed implementation guide** for your chosen path
3. **Start with Phase 1** of the implementation
4. **Use `/implement`** to begin building when ready

For questions or to begin implementation, run:
```
/implement [PluginName]
```
