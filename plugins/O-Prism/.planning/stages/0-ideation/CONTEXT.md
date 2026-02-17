# O-Prism Stage 0 Context: Research and Planning

**Date:** 2026-02-16
**Agent:** research-planning-agent
**Complexity:** 5.0 (Very High, capped from raw 14.0)
**Tier:** 4 (synthesizer with MIDI, oscillators, complex DSP)

---

## Key Decisions Made

### 1. Mipmap Anti-Aliasing (not runtime oversampling)

The wavetable engine will use pre-computed bandlimited mipmap tables rather than runtime oversampling. This is the industry standard (Serum, Vital, Surge) and is critical for meeting the <25% CPU budget at 16 voices with 8-voice unison. Runtime 2x oversampling would double CPU per oscillator -- unacceptable when 256 oscillator instances could be active simultaneously.

Mipmaps are generated at wavetable load time using FFT (zero harmonics above Nyquist per octave level). 10 octave levels per frame, stored as float to halve memory. Estimated ~20MB per loaded table.

### 2. JUCE Synthesiser (not custom voice allocator)

Following the proven pattern from O-Lyrica. `juce::Synthesiser` handles MIDI routing, voice stealing (oldest-note), pedal handling, and sample-accurate event timing. PrismVoice extends `juce::SynthesiserVoice` exactly as `HarpSynthVoice` does in O-Lyrica.

### 3. JUCE StateVariableTPTFilter (not custom SVF)

The JUCE 8 `StateVariableTPTFilter` implements the Zavalishin TPT design -- the same algorithm we'd implement manually. Using the built-in class reduces risk. 24dB slopes achieved by cascading two instances. Notch achieved by summing LP + HP outputs.

### 4. Per-Voice Architecture (not global oscillators)

Each voice contains its own oscillator instances, filters, and envelopes. This enables proper polyphonic behavior where each note has independent filter envelope, key tracking, and phase. The trade-off is higher CPU (per-voice processing) but this matches professional synth architecture.

### 5. Fixed Effects Chain Order for v1.0

Distortion -> Chorus -> Delay -> Reverb -> EQ. Rearrangeable chain is v2.0 scope. Fixed order reduces implementation complexity and avoids dynamic routing complexity in the DSP graph.

### 6. Shared Tuning Module (not copy-paste)

Using the `scala-tuning-engine` module v2.1.0 from `modules/tuning/`. This is the maintained version with integration checklist, snippets, and JS module. Source files are copied to the plugin's Source directory per the module's integration pattern.

---

## Constraints Identified

1. **Memory budget for wavetable mipmaps:** ~20MB per loaded table. With factory library of 100+ tables, only active tables (2-3) should be in memory. Need lazy loading with background thread.

2. **CPU budget at max settings:** 16 voices x 8 unison x 2 oscillators = 256 oscillator instances. Each with 2 filters, 2 ADSRs, sub, noise. Estimated ~50% single core worst case. May need dynamic voice limiting when high unison is active.

3. **68 APVTS parameters:** Highest count in catalog. Stage 1 foundation requires careful organization. WebView relay management needs the shared relay manager module.

4. **No modulation matrix in v1.0:** Filter envelope modulates cutoff only. All other modulation is v2.0. This simplifies DSP but limits sound design compared to Serum/Vital.

5. **Factory wavetable sourcing:** 100+ wavetables needed before v1.0 release. Can start development with procedurally generated basic waveforms (saw, square, sine, triangle morphs). Real wavetable content can be sourced/created in parallel with DSP development.

---

## Risk Assessment Summary

| Component | Risk | Fallback |
|-----------|------|----------|
| Wavetable mipmap engine | HIGH | Runtime 2x oversampling (higher CPU) |
| Dual SVF filters | LOW | Using proven JUCE class |
| Voice management / CPU | MEDIUM | Dynamic voice limiting |
| Microtonal integration | LOW | Proven code from O-Lyrica/O-Bells |
| WebView visualization | MEDIUM | Static 2D waveform (not 3D wireframe) |
| 68-parameter WebView binding | MEDIUM | Use WebViewRelayManager module |

**Highest risk:** Wavetable mipmap engine. This is novel DSP with no equivalent in existing codebase. All other components depend on it. Implementing Phase 2.1 (basic playback without mipmaps) first validates the architecture before tackling anti-aliasing.

---

## Architecture Files Created

- `plugins/O-Prism/.planning/research/ARCHITECTURE.md` -- Full DSP specification (11 sections)
- `plugins/O-Prism/.planning/ROADMAP.md` -- Implementation plan with 5 DSP phases + 3 GUI phases

---

## Next Steps

1. **Stage 1 (Foundation):** Create CMakeLists.txt, 68-parameter APVTS, processor/editor shells
2. Review ARCHITECTURE.md and ROADMAP.md before proceeding
3. Source or generate initial wavetable data (placeholder waveforms sufficient for development)
