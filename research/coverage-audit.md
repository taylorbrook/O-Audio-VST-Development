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
**Last updated:** 2026-03-07 (Plan 20-03: all 10 gaps filled)
**Research docs indexed:** 63 (53 original + 10 gap-fill)
**Plugins audited:** 23 (O-* series)

---

## 1. Domain Coverage Matrix

| Domain | Doc Count | Coverage Depth | Documents |
|--------|-----------|----------------|-----------|
| dsp | 35 | Deep | circuit-modeling-fundamentals, concatenative-synthesis-comprehensive, custom-fft-implementations, delay-effects-comprehensive-guide, dsp-click-prevention-debugging, fft-artifact-prevention, fft-processing-best-practices, generative-audio-algorithms-reference, generative-plugins-research-synthesis, granular-synthesis-state-of-the-art, microtonality-comprehensive-database, microtonality-implementation-juce, microtonality-theory-formats, modal-synthesis-bells-academic-research, multi-stage-decay-envelopes-comparison, physical-modeling-commercial-analog-modeling-ml-approaches, physical-modeling-research-agent-3-physical-modelling-optimization, reverb-comprehensive-research, spectral-sequencer-research, spectral-transient-shaper-research, stutter-effects/path-a-granular-stutter-engine, stutter-effects/path-b-beat-repeater, stutter-effects/path-c-playhead-modulator, stutter-effects/stutter-effects-research-findings, wavetable-synthesis-comprehensive, wavetable-synthesis-o-intonationpad, wavetable-synthesis-o-prism, **dynamics-processing-compression-limiting**, **parametric-eq-filter-design**, **chorus-modulation-effects**, **vocal-formant-synthesis**, **freeze-spectral-freeze-effects**, **tremolo-amplitude-modulation**, **bass-synthesis**, **detuning-pitch-thickening**, **mallet-percussion-physical-modeling** |
| market-research | 7 | Moderate | O-Detune-market-research, concatenative-synthesis-market-research, concatenative-synthesis-plugin-briefs, microtonality-commercial-performance, spatial-audio-plugins-market-research, spatial-plugin-briefs, spectral-toolbox-synopses |
| ml | 7 | Moderate | ml-ai-audio-plugins-2024-2026, ml-inference-frameworks-audio-plugins, ml-integration-juce-plugins, ml-plugin-briefs, neural-audio-synthesis-technologies, o-texture-custom-vae-architecture, umap-dimensionality-reduction-audio-plugins |
| spatial-audio | 7 | Moderate | ambisonics-binaural-decoding-deep-dive, ambisonics-encoding-deep-dive, juce8-multichannel-spatial-audio, saf-juce-integration-guide, sound-spatialization-algorithms, spatial-audio-per-grain-spatialization, spatial-granular-synthesis-research |
| ui | 3 | Thin | 2d-scatter-plot-concatenative-synthesis, cross-platform-webview-best-practices, webgl-spectrogram-patterns |
| architecture | 2 | Thin | plugin-development-without-juce, **licensing-distribution** |
| tooling | 2 | Thin | flucoma-core-integration-research, coverage-audit |
| cross-platform | 0 | None | (no dedicated docs; cross-platform-webview-best-practices is categorized under ui) |

### Coverage Summary

- **Deep coverage (10+ docs):** dsp (35 docs)
- **Moderate coverage (5-9 docs):** market-research, ml, spatial-audio
- **Thin coverage (1-3 docs):** ui, architecture, tooling
- **No coverage:** cross-platform (as a standalone domain)

---

## 2. Plugin Technique Coverage

This section maps each plugin's core DSP technique to existing research documentation.

| Plugin | Core Technique | Research Docs | Coverage Status |
|--------|---------------|---------------|-----------------|
| O-AnalogEQ | Parametric EQ / filter design | **parametric-eq-filter-design** | Covered (gap filled) |
| O-AnalogSaturation | Saturation / distortion / circuit modeling | circuit-modeling-fundamentals, physical-modeling-commercial-analog-modeling-ml-approaches | Covered |
| O-Bass | Bass synthesis | **bass-synthesis** | Covered (gap filled) |
| O-Bells | Modal synthesis / bell physical modeling | modal-synthesis-bells-academic-research, multi-stage-decay-envelopes-comparison | Covered |
| O-Chorus | Chorus / modulation effects | **chorus-modulation-effects** | Covered (gap filled) |
| O-Comp | Compression / dynamics processing | circuit-modeling-fundamentals, **dynamics-processing-compression-limiting** | Covered (gap filled) |
| O-Detune | Detuning / pitch thickening | O-Detune-market-research, **detuning-pitch-thickening** | Covered (gap filled) |
| O-DigiDelay | Delay effects | delay-effects-comprehensive-guide | Covered |
| O-Freeze | Freeze / buffer capture / spectral freeze | **freeze-spectral-freeze-effects** | Covered (gap filled) |
| O-FreqPulse | Spectral processing / frequency-domain effects | fft-processing-best-practices, fft-artifact-prevention, spectral-sequencer-research, spectral-transient-shaper-research, custom-fft-implementations | Covered |
| O-GrainScatter | Granular synthesis / scattering | granular-synthesis-state-of-the-art, concatenative-synthesis-comprehensive | Covered |
| O-IntonationPad | Wavetable synthesis / microtonality | wavetable-synthesis-o-intonationpad, wavetable-synthesis-comprehensive, microtonality-* (5 docs) | Covered |
| O-Lyrica | Vocal synthesis / formant processing | **vocal-formant-synthesis** | Covered (gap filled) |
| O-Marimba | Physical modeling / mallet percussion | modal-synthesis-bells-academic-research, physical-modeling-research-agent-3-physical-modelling-optimization, **mallet-percussion-physical-modeling** | Covered (gap filled) |
| O-MultiBandCompressor | Multiband dynamics processing | circuit-modeling-fundamentals, **dynamics-processing-compression-limiting** | Covered (gap filled) |
| O-Orbit | Spatial audio / orbit motion | spatial-audio (7 docs: ambisonics, spatialization, SAF) | Covered |
| O-Polystutter | Stutter effects | stutter-effects/* (4 docs) | Covered |
| O-Prism | Wavetable synthesis / microtonal | wavetable-synthesis-o-prism, wavetable-synthesis-comprehensive, microtonality-* (5 docs) | Covered |
| O-SimpleReverb | Reverb algorithms | reverb-comprehensive-research | Covered |
| O-SpectralShaper | Spectral shaping / transients | spectral-transient-shaper-research, fft-processing-best-practices | Covered |
| O-Texture | ML-driven granular / concatenative synthesis | o-texture-custom-vae-architecture, concatenative-synthesis-comprehensive, ml-* (6 docs), granular-synthesis-state-of-the-art | Covered |
| O-TextureForge | Granular corpus exploration | 2d-scatter-plot-concatenative-synthesis, concatenative-synthesis-*, umap-dimensionality-reduction-audio-plugins | Covered |
| O-Tremolo | Tremolo / amplitude modulation | **tremolo-amplitude-modulation** | Covered (gap filled) |

### Coverage Statistics

- **Fully covered:** 23 plugins (all O-* series)
- **Partially covered:** 0 plugins
- **No coverage (GAP):** 0 plugins

---

## 3. Identified Gaps (ALL FILLED)

All 10 identified gaps have been filled with dedicated research documents as of Plan 20-03 (2026-03-07).

### High Priority (FILLED)

| # | Gap Topic | Affected Plugins | Research Doc Created | Status |
|---|-----------|------------------|---------------------|--------|
| 1 | **Dynamics Processing (Compression & Limiting)** | O-Comp, O-MultiBandCompressor | dynamics-processing-compression-limiting.md | FILLED |
| 2 | **Parametric EQ & Filter Design** | O-AnalogEQ | parametric-eq-filter-design.md | FILLED |
| 3 | **Chorus & Modulation Effects** | O-Chorus, (O-Tremolo related) | chorus-modulation-effects.md | FILLED |

### Medium Priority (FILLED)

| # | Gap Topic | Affected Plugins | Research Doc Created | Status |
|---|-----------|------------------|---------------------|--------|
| 4 | **Vocal & Formant Synthesis** | O-Lyrica | vocal-formant-synthesis.md | FILLED |
| 5 | **Freeze & Spectral Freeze Effects** | O-Freeze | freeze-spectral-freeze-effects.md | FILLED |
| 6 | **Tremolo & Amplitude Modulation** | O-Tremolo | tremolo-amplitude-modulation.md | FILLED |
| 7 | **Bass Synthesis** | O-Bass | bass-synthesis.md | FILLED |

### Lower Priority (FILLED)

| # | Gap Topic | Affected Plugins | Research Doc Created | Status |
|---|-----------|------------------|---------------------|--------|
| 8 | **Detuning & Pitch Thickening Algorithms** | O-Detune | detuning-pitch-thickening.md | FILLED |
| 9 | **Mallet Percussion Physical Modeling** | O-Marimba | mallet-percussion-physical-modeling.md | FILLED |
| 10 | **Licensing & Distribution** | All plugins | licensing-distribution.md | FILLED |

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

## 5. Gap Priority Summary (ALL FILLED)

All 10 gaps have been filled. This section retained for historical reference.

| Priority | Gap # | Topic | Plugins Affected | Status | Doc Created |
|----------|-------|-------|------------------|--------|-------------|
| High | 1 | Dynamics Processing | 2 | FILLED | dynamics-processing-compression-limiting.md |
| High | 2 | Parametric EQ & Filter Design | 1 | FILLED | parametric-eq-filter-design.md |
| High | 3 | Chorus & Modulation Effects | 1-2 | FILLED | chorus-modulation-effects.md |
| Medium | 4 | Vocal & Formant Synthesis | 1 | FILLED | vocal-formant-synthesis.md |
| Medium | 5 | Freeze & Spectral Freeze Effects | 1 | FILLED | freeze-spectral-freeze-effects.md |
| Medium | 6 | Tremolo & Amplitude Modulation | 1 | FILLED | tremolo-amplitude-modulation.md |
| Medium | 7 | Bass Synthesis | 1 | FILLED | bass-synthesis.md |
| Lower | 8 | Detuning & Pitch Thickening | 1 | FILLED | detuning-pitch-thickening.md |
| Lower | 9 | Mallet Percussion Physical Modeling | 1 | FILLED | mallet-percussion-physical-modeling.md |
| Lower | 10 | Licensing & Distribution | All | FILLED | licensing-distribution.md |

---

## 6. Domain Thin-Coverage Notes

Domains with thin coverage (3 or fewer docs) that may warrant additional research:

- **architecture** (1 doc): Only covers JUCE alternatives. No docs on plugin architecture patterns, state management, preset systems, or undo/redo.
- **tooling** (1 doc): Only covers FluCoMa integration. No docs on build system configuration, CI/CD for plugins, testing frameworks, or profiling tools.
- **ui** (3 docs): Covers WebView patterns and spectrograms. No docs on native JUCE UI (CustomLookAndFeel, component layout), accessibility, or parameter-to-UI binding patterns.
- **cross-platform** (0 docs): No standalone cross-platform docs. WebView cross-platform is filed under ui. Could benefit from docs on platform-specific audio APIs, Windows vs macOS build differences, or plugin format quirks.

These are not plugin-technique gaps (Section 3) but domain-level coverage thinness worth noting for future research planning.

---

---

## 7. Gap-Fill Approval

**Decision date:** 2026-03-07
**Decision:** approve-all
**Approved by:** User (via checkpoint:decision gate in Plan 20-02)

All 10 identified gaps are approved for research doc creation in Plan 20-03:

1. Dynamics Processing (Compression & Limiting)
2. Parametric EQ & Filter Design
3. Chorus & Modulation Effects
4. Vocal & Formant Synthesis
5. Freeze & Spectral Freeze Effects
6. Tremolo & Amplitude Modulation
7. Bass Synthesis
8. Detuning & Pitch Thickening Algorithms
9. Mallet Percussion Physical Modeling
10. Licensing & Distribution

**Status:** All 10 gaps filled in Plan 20-03 (2026-03-07). See Section 3 for the created docs.

---

## 8. Final Verification

**Verified by:** User (via checkpoint:human-verify gate in Plan 20-03, Task 2)
**Date:** 2026-03-07
**Result:** Approved -- all 10 gap-fill research documents verified and accepted
**Resource index total:** 64 documents (53 original + 10 gap-fill + 1 coverage-audit)

---

*Living document. Updated: 2026-03-07 (Plan 20-03 complete -- all gaps filled, user verified). Re-audit when plugins are added or research docs are created.*
