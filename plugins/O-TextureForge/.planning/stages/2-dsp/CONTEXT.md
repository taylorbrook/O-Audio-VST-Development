# Stage 2: DSP Implementation - Context

## Discussion Summary

**Date:** 2026-02-14
**Participants:** User, Claude

## Phase Breakdown

Stage 2 is split into 4 sub-phases:
- **Phase 2.1:** File loading & grain segmentation
- **Phase 2.2:** Descriptor extraction (19D: 13 MFCCs + spectral centroid + flatness + flux + rolloff + RMS + ZCR)
- **Phase 2.3:** KD-tree nearest-neighbor search (nanoflann)
- **Phase 2.4:** Polyphonic grain scheduler + MIDI modes (Drone first, then Pitch-Mapped + Trigger)

## Requirements Confirmed

- Resample audio to DAW sample rate on load (not during playback)
- Convert all loaded files to mono (sum stereo channels)
- 19D descriptor space: 13 MFCCs + spectral centroid + spectral flatness + spectral flux + spectral rolloff + RMS energy + zero-crossing rate (f0 replaced with spectral rolloff for reliability)
- Fixed segmentation at load time using GRAIN_SIZE value; parameter changes after load only affect playback envelope
- 50% overlap for hop size (hop = grainSize / 2)
- Grain scheduler is self-contained in O-TextureForge (no shared module extraction)
- Generative Drone MIDI mode implemented first; Pitch-Mapped and Trigger+Modulate added in second pass within Stage 2
- KD-tree macro knob search uses direct descriptor targeting (Energy->RMS, Brightness->Centroid, Texture->Flatness)

## Constraints Identified

- Background thread for all file I/O, segmentation, descriptor extraction, KD-tree construction
- Audio thread must be lock-free: read-only access to corpus buffer via atomic shared_ptr
- KD-tree queries must be allocation-free on audio thread
- Re-segment corpus if sample rate changes in prepareToPlay()
- Descriptor normalization (z-score) must store means/stddevs for consistent target mapping

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Resampling | On load | Simpler grain playback, consistent grain sizes. Re-segment if SR changes. |
| f0 Descriptor | Replaced with spectral rolloff | Autocorrelation f0 is error-prone; spectral rolloff is robust and still 19D |
| Stereo handling | Mono on load | Channel-independent descriptors, simpler pipeline, output to both channels |
| Grain segmentation | Fixed at analysis time | Avoids expensive re-analysis when GRAIN_SIZE param changes |
| MIDI mode order | Drone first | Simplest mode (no note handling), validates grain engine before adding complexity |
| Search method | Direct targeting | Intuitive: knobs set target values in descriptor dimensions, KD-tree finds nearest |
| Code reuse | Self-contained | Descriptor-based selection is fundamentally different from O-GrainScatter's approach |

## Descriptor Dimension Mapping (19D)

| Dim | Descriptor | Macro Knob | Notes |
|-----|-----------|------------|-------|
| 0-12 | MFCCs (13 coefficients) | None (indirect) | Timbral fingerprint, dominant in KD-tree clustering |
| 13 | Spectral Centroid | Brightness | Hz, normalized z-score |
| 14 | Spectral Flatness | Texture | 0=tonal, 1=noise, normalized |
| 15 | Spectral Flux | None | Rate of spectral change |
| 16 | Spectral Rolloff | None | Frequency below which 85% of energy concentrated |
| 17 | RMS Energy | Energy | Loudness in dB, normalized |
| 18 | Zero-Crossing Rate | None | Transient/noise character |

## Macro Knob → KD-tree Target Mapping

```
Energy knob (0-1) → target[17] = (energy * 2.0 - 1.0)     // Maps to RMS Energy dimension
Brightness knob (0-1) → target[13] = (brightness * 2.0 - 1.0)  // Maps to Spectral Centroid
Texture knob (0-1) → target[14] = (texture * 2.0 - 1.0)    // Maps to Spectral Flatness
```

Non-mapped dimensions use 0.0 (neutral in z-score space).

## Open Questions

- Exact nanoflann version to pin (v1.5.0 per ROADMAP, confirm latest stable)
- Spectral rolloff threshold: 85% or 95% of spectral energy? (85% is standard)
- Whether to add a simple "loading progress" callback from background thread to editor during file analysis

## Key Source Files (from Stage 1)

- `plugins/O-TextureForge/Source/PluginProcessor.h` — 12 APVTS params, VizSnapshot, atomic param pointers
- `plugins/O-TextureForge/Source/PluginProcessor.cpp` — Empty processBlock (buffer.clear()), prepareToPlay stores currentSampleRate
- `plugins/O-TextureForge/Source/PluginEditor.h/.cpp` — WebView placeholder, 30Hz timer
- `plugins/O-TextureForge/CMakeLists.txt` — Build config with IS_SYNTH, NEEDS_WEBVIEW2

## Dependencies to Add (Stage 2)

- nanoflann v1.5.0 (FetchContent, header-only, KD-tree)
- juce_audio_formats module (file loading — may already be linked)
- juce_dsp module (FFT, WindowingFunction — may already be linked)

## New Source Files Expected

- `Source/dsp/CorpusLoader.h/.cpp` — Background thread file loading + resampling
- `Source/dsp/GrainSegmenter.h/.cpp` — Fixed-size segmentation
- `Source/dsp/DescriptorExtractor.h/.cpp` — MFCC + spectral + time-domain extraction
- `Source/dsp/MFCCExtractor.h/.cpp` — FFT pipeline, mel filterbank, DCT
- `Source/dsp/KDTreeSearch.h/.cpp` — nanoflann adaptor, tree build, query interface
- `Source/dsp/GrainScheduler.h/.cpp` — Voice pool, grain triggering, crossfading
- `Source/dsp/GrainVoice.h` — Voice struct with envelope, pitch, read position

## Next Phase

Ready for: **research** phase
