---
title: "Research Coverage Audit"
created: 2026-03-07
domain: tooling
type: research
keywords:
  - coverage-audit
  - research-governance
  - gap-analysis
  - staleness-detection
  - domain-coverage
  - living-reference
---

# Research Coverage Audit

A living reference of research documentation coverage across all domains and plugin techniques. Identifies gaps where existing plugins lack corresponding research docs, and flags stale documents referencing deprecated APIs.

**Generated:** 2026-03-07
**Research docs indexed:** 53
**Plugins audited:** 23 (O-* series)

---

## 1. Domain Coverage Matrix

| Domain | Doc Count | Coverage Depth | Documents |
|--------|-----------|----------------|-----------|
| dsp | 27 | Deep | circuit-modeling-fundamentals, concatenative-synthesis-comprehensive, custom-fft-implementations, delay-effects-comprehensive-guide, dsp-click-prevention-debugging, fft-artifact-prevention, fft-processing-best-practices, generative-audio-algorithms-reference, generative-plugins-research-synthesis, granular-synthesis-state-of-the-art, microtonality-comprehensive-database, microtonality-implementation-juce, microtonality-theory-formats, modal-synthesis-bells-academic-research, multi-stage-decay-envelopes-comparison, physical-modeling-commercial-analog-modeling-ml-approaches, physical-modeling-research-agent-3-physical-modelling-optimization, reverb-comprehensive-research, spectral-sequencer-research, spectral-transient-shaper-research, stutter-effects/path-a-granular-stutter-engine, stutter-effects/path-b-beat-repeater, stutter-effects/path-c-playhead-modulator, stutter-effects/stutter-effects-research-findings, wavetable-synthesis-comprehensive, wavetable-synthesis-o-intonationpad, wavetable-synthesis-o-prism |
| market-research | 7 | Moderate | O-Detune-market-research, concatenative-synthesis-market-research, concatenative-synthesis-plugin-briefs, microtonality-commercial-performance, spatial-audio-plugins-market-research, spatial-plugin-briefs, spectral-toolbox-synopses |
| ml | 7 | Moderate | ml-ai-audio-plugins-2024-2026, ml-inference-frameworks-audio-plugins, ml-integration-juce-plugins, ml-plugin-briefs, neural-audio-synthesis-technologies, o-texture-custom-vae-architecture, umap-dimensionality-reduction-audio-plugins |
| spatial-audio | 7 | Moderate | ambisonics-binaural-decoding-deep-dive, ambisonics-encoding-deep-dive, juce8-multichannel-spatial-audio, saf-juce-integration-guide, sound-spatialization-algorithms, spatial-audio-per-grain-spatialization, spatial-granular-synthesis-research |
| ui | 3 | Thin | 2d-scatter-plot-concatenative-synthesis, cross-platform-webview-best-practices, webgl-spectrogram-patterns |
| architecture | 1 | Thin | plugin-development-without-juce |
| tooling | 1 | Thin | flucoma-core-integration-research |
| cross-platform | 0 | None | (no dedicated docs; cross-platform-webview-best-practices is categorized under ui) |

### Coverage Summary

- **Deep coverage (10+ docs):** dsp
- **Moderate coverage (5-9 docs):** market-research, ml, spatial-audio
- **Thin coverage (1-3 docs):** ui, architecture, tooling
- **No coverage:** cross-platform (as a standalone domain)

---

## 2. Plugin Technique Coverage

This section maps each plugin's core DSP technique to existing research documentation.

| Plugin | Core Technique | Research Docs | Coverage Status |
|--------|---------------|---------------|-----------------|
| O-AnalogEQ | Parametric EQ / filter design | None | GAP |
| O-AnalogSaturation | Saturation / distortion / circuit modeling | circuit-modeling-fundamentals, physical-modeling-commercial-analog-modeling-ml-approaches | Covered |
| O-Bass | Bass synthesis | None | GAP |
| O-Bells | Modal synthesis / bell physical modeling | modal-synthesis-bells-academic-research, multi-stage-decay-envelopes-comparison | Covered |
| O-Chorus | Chorus / modulation effects | None | GAP |
| O-Comp | Compression / dynamics processing | circuit-modeling-fundamentals (compressor section) | Partial (section only, no dedicated doc) |
| O-Detune | Detuning / pitch thickening | O-Detune-market-research (market only, no algorithm doc) | Partial (market research only) |
| O-DigiDelay | Delay effects | delay-effects-comprehensive-guide | Covered |
| O-Freeze | Freeze / buffer capture / spectral freeze | None | GAP |
| O-FreqPulse | Spectral processing / frequency-domain effects | fft-processing-best-practices, fft-artifact-prevention, spectral-sequencer-research, spectral-transient-shaper-research, custom-fft-implementations | Covered |
| O-GrainScatter | Granular synthesis / scattering | granular-synthesis-state-of-the-art, concatenative-synthesis-comprehensive | Covered |
| O-IntonationPad | Wavetable synthesis / microtonality | wavetable-synthesis-o-intonationpad, wavetable-synthesis-comprehensive, microtonality-* (5 docs) | Covered |
| O-Lyrica | Vocal synthesis / formant processing | None | GAP |
| O-Marimba | Physical modeling / mallet percussion | modal-synthesis-bells-academic-research (related), physical-modeling-research-agent-3-physical-modelling-optimization | Partial (bell-focused, not mallet-specific) |
| O-MultiBandCompressor | Multiband dynamics processing | circuit-modeling-fundamentals (compressor section) | Partial (no dedicated multiband dynamics doc) |
| O-Orbit | Spatial audio / orbit motion | spatial-audio (7 docs: ambisonics, spatialization, SAF) | Covered |
| O-Polystutter | Stutter effects | stutter-effects/* (4 docs) | Covered |
| O-Prism | Wavetable synthesis / microtonal | wavetable-synthesis-o-prism, wavetable-synthesis-comprehensive, microtonality-* (5 docs) | Covered |
| O-SimpleReverb | Reverb algorithms | reverb-comprehensive-research | Covered |
| O-SpectralShaper | Spectral shaping / transients | spectral-transient-shaper-research, fft-processing-best-practices | Covered |
| O-Texture | ML-driven granular / concatenative synthesis | o-texture-custom-vae-architecture, concatenative-synthesis-comprehensive, ml-* (6 docs), granular-synthesis-state-of-the-art | Covered |
| O-TextureForge | Granular corpus exploration | 2d-scatter-plot-concatenative-synthesis, concatenative-synthesis-*, umap-dimensionality-reduction-audio-plugins | Covered |
| O-Tremolo | Tremolo / amplitude modulation | None | GAP |

### Coverage Statistics

- **Fully covered:** 13 plugins (O-AnalogSaturation, O-Bells, O-DigiDelay, O-FreqPulse, O-GrainScatter, O-IntonationPad, O-Orbit, O-Polystutter, O-Prism, O-SimpleReverb, O-SpectralShaper, O-Texture, O-TextureForge)
- **Partially covered:** 4 plugins (O-Comp, O-Detune, O-Marimba, O-MultiBandCompressor)
- **No coverage (GAP):** 6 plugins (O-AnalogEQ, O-Bass, O-Chorus, O-Freeze, O-Lyrica, O-Tremolo)

---

## 3. Identified Gaps

Specific topics that need new research documents, ordered by how many existing plugins would benefit.

### High Priority (technique used by multiple plugins or core DSP topic)

| # | Gap Topic | Affected Plugins | Rationale |
|---|-----------|------------------|-----------|
| 1 | **Dynamics Processing (Compression & Limiting)** | O-Comp, O-MultiBandCompressor | Two plugins rely on dynamics processing. circuit-modeling-fundamentals has a brief compressor section but no dedicated dynamics research covering VCA/FET/optical topologies, sidechain filtering, multiband crossover design, look-ahead, or knee curves. |
| 2 | **Parametric EQ & Filter Design** | O-AnalogEQ | Core plugin with no research doc. Needs biquad cascades, analog prototype design (Butterworth/Chebyshev/Bessel), frequency warping, cramping compensation, and matched analog response techniques. |
| 3 | **Chorus & Modulation Effects** | O-Chorus, (O-Tremolo related) | No research on chorus algorithms (LFO-modulated delay, ensemble effects, dimension-style chorus, BBD emulation). Modulation effects are a common DSP building block. |

### Medium Priority (single plugin, significant technique gap)

| # | Gap Topic | Affected Plugins | Rationale |
|---|-----------|------------------|-----------|
| 4 | **Vocal & Formant Synthesis** | O-Lyrica | No research on formant synthesis, vocal modeling, singing synthesis, or phoneme-based approaches. Specialized domain with unique DSP requirements. |
| 5 | **Freeze & Spectral Freeze Effects** | O-Freeze | No research on audio freeze techniques (spectral freeze, buffer capture and loop, granular freeze, phase-vocoder freeze). Distinct from general granular or FFT processing. |
| 6 | **Tremolo & Amplitude Modulation** | O-Tremolo | No research on tremolo circuits, amplitude modulation, ring modulation, or auto-pan effects. While simple in concept, analog tremolo emulation has depth (bias tremolo, optical tremolo, harmonic tremolo). |
| 7 | **Bass Synthesis** | O-Bass | No dedicated research on bass synthesis techniques (subtractive synthesis for bass, sub-harmonic generation, bass enhancement, psychoacoustic bass). |

### Lower Priority (partial coverage exists or niche topic)

| # | Gap Topic | Affected Plugins | Rationale |
|---|-----------|------------------|-----------|
| 8 | **Detuning & Pitch Thickening Algorithms** | O-Detune | Has market research but no algorithm doc. Needs pitch shifting techniques, micro-detuning, unison voice management, stereo widening via detuning. |
| 9 | **Mallet Percussion Physical Modeling** | O-Marimba | Modal synthesis research is bell-focused. Marimba/xylophone have different modal characteristics (bar vs. bell), resonator tube modeling, and mallet interaction. |
| 10 | **Licensing & Distribution** | All plugins (tache_plugins integration) | No research on plugin licensing systems, copy protection approaches, online activation, or distribution strategies. Referenced in plan context. |

---

## 4. Stale Documents

Documents flagged with `status: stale` in their frontmatter due to deprecated API usage.

| Document | Deprecated Pattern | Details |
|----------|--------------------|---------|
| fft-artifact-prevention.md | `getLatencySamples() const override` | Non-virtual in JUCE 8; should use `setLatencySamples()` in `prepareToPlay()` instead |
| fft-processing-best-practices.md | `getLatencySamples() override` | Non-virtual in JUCE 8; should use `setLatencySamples()` in `prepareToPlay()` instead |
| spectral-sequencer-research.md | `getLatencySamples() const override` | Non-virtual in JUCE 8; should use `setLatencySamples()` in `prepareToPlay()` instead |
| delay-effects-comprehensive-guide.md | `getLatencySamples() const override` | Non-virtual in JUCE 8; should use `setLatencySamples()` in `prepareToPlay()` instead |

### Staleness Scan Summary

- **Patterns scanned:** 13 deprecated API patterns (see methodology below)
- **Docs flagged:** 4 out of 53 (7.5%)
- **Common issue:** All 4 use `getLatencySamples()` with `override` keyword, which is invalid in JUCE 8 where the method is non-virtual
- **No other deprecated patterns found** across the entire research corpus

### Methodology

Each of the 53 research documents was scanned for these deprecated patterns:
1. `getLatencySamples()` override attempts -- **4 matches**
2. `GenericAudioProcessorEditor` usage -- 0 matches
3. `AudioProcessorValueTreeState::createAndAddParameter` -- 0 matches
4. `Slider::LinearBarVertical` old slider styles -- 0 matches
5. `setLookAndFeel(nullptr)` patterns -- 0 matches
6. `MidiBuffer::Iterator` (replaced by range-based iteration) -- 0 matches
7. `AudioFormatReaderSource` without smart pointer ownership -- 0 matches
8. `juce_module_info` format references -- 0 matches
9. `ScopedPointer` (replaced by std::unique_ptr) -- 0 matches
10. `var::operator int()` implicit conversions -- 0 matches
11. `File::getSpecialLocation(File::currentApplicationFile)` for plugin paths -- 0 matches
12. Direct OpenGL rendering without `juce::OpenGLAppComponent` -- 0 matches
13. Pre-CMake build instructions (Projucer-based) -- 0 matches (1 descriptive mention in plugin-development-without-juce.md, not instructional)

---

## 5. Gap Priority Summary

Recommended priority for new research doc creation, based on number of plugins affected and technique importance.

| Priority | Gap # | Topic | Plugins Affected | Estimated Effort |
|----------|-------|-------|------------------|------------------|
| High | 1 | Dynamics Processing | 2 | Large (deep topic) |
| High | 2 | Parametric EQ & Filter Design | 1 | Large (foundational DSP) |
| High | 3 | Chorus & Modulation Effects | 1-2 | Medium |
| Medium | 4 | Vocal & Formant Synthesis | 1 | Large (specialized) |
| Medium | 5 | Freeze & Spectral Freeze Effects | 1 | Medium |
| Medium | 6 | Tremolo & Amplitude Modulation | 1 | Small-Medium |
| Medium | 7 | Bass Synthesis | 1 | Medium |
| Lower | 8 | Detuning & Pitch Thickening | 1 | Medium |
| Lower | 9 | Mallet Percussion Physical Modeling | 1 | Medium |
| Lower | 10 | Licensing & Distribution | All | Medium (non-DSP) |

---

## 6. Domain Thin-Coverage Notes

Domains with thin coverage (3 or fewer docs) that may warrant additional research:

- **architecture** (1 doc): Only covers JUCE alternatives. No docs on plugin architecture patterns, state management, preset systems, or undo/redo.
- **tooling** (1 doc): Only covers FluCoMa integration. No docs on build system configuration, CI/CD for plugins, testing frameworks, or profiling tools.
- **ui** (3 docs): Covers WebView patterns and spectrograms. No docs on native JUCE UI (CustomLookAndFeel, component layout), accessibility, or parameter-to-UI binding patterns.
- **cross-platform** (0 docs): No standalone cross-platform docs. WebView cross-platform is filed under ui. Could benefit from docs on platform-specific audio APIs, Windows vs macOS build differences, or plugin format quirks.

These are not plugin-technique gaps (Section 3) but domain-level coverage thinness worth noting for future research planning.

---

*Living document. Updated: 2026-03-07. Re-audit when plugins are added or research docs are created.*
