# O-Orbit Stage 0 Context (Ideation - Research & Planning)

> **Stage:** 0 (Ideation)
> **Date:** 2026-02-09
> **Agent:** research-planning-agent
> **Status:** ✅ Complete

---

## Phase Discussion

### Research Findings

**Complexity Tier:** 5 (Complex - score 4.2/5.0)

**Rationale for Tier 5:**
- **Parameter count:** 17 parameters (score: 2.0)
- **Algorithm complexity:** Motion engine + VBAP + distance model (score: 3.0)
- **System features:** Multi-channel bus (2-24 channels), VBAP triangulation, SAF dependency, file I/O, WebView UI (score: 3.5)
- **Total raw score:** 8.5 → capped at 5.0, adjusted to 4.2 accounting for SAF library maturity

**Research Depth:** MODERATE (15-20 minutes)

**Key Technical Decisions:**

1. **VBAP vs. Ambisonics:**
   - **Chose VBAP** (direct amplitude panning)
   - **Why:** Simpler, lower CPU, native support for custom non-equidistant layouts, no order limitations
   - **Tradeoff:** Cannot rotate scene post-processing (not needed for O-Orbit's motion engine)

2. **SAF Integration vs. Scratch Implementation:**
   - **Chose SAF** for VBAP renderer
   - **Why:** Delaunay triangulation is complex; SAF is battle-tested and ISC-licensed (commercial-friendly)
   - **Tradeoff:** Build complexity on Windows (requires BLAS/LAPACK)
   - **Mitigation:** macOS uses Accelerate (zero friction), Windows CI can use OpenBLAS

3. **Flexible Channel Count vs. Named Layouts:**
   - **Chose flexible** (accept any 2-24 channel count, user configures speaker layout)
   - **Why:** Custom speaker arrays don't map to JUCE's named layouts; flexibility is O-Orbit's differentiator
   - **Tradeoff:** More complex parameter handling, user education required

4. **Per-Block vs. Per-Sample VBAP:**
   - **Chose per-block** gain calculation with per-sample application
   - **Why:** VBAP triangle search is expensive (~10-50μs); gains change slowly
   - **Mitigation:** Linear interpolation between blocks prevents zipper noise

### Implementation Approach

**Strategy:** Staged implementation (phased DSP and GUI)

**Stage 2 (DSP) Phases:**
- **Phase 2.1:** Motion engine + basic stereo panning (no VBAP yet)
- **Phase 2.2:** VBAP 2D + multi-channel output (quad, 5.1, 7.1)
- **Phase 2.3:** VBAP 3D + custom layouts + auto-downmix

**Stage 3 (GUI) Phases:**
- **Phase 3.1:** WebView UI + parameter controls (no visualization)
- **Phase 3.2:** Orbital visualizer animation (real-time canvas)
- **Phase 3.3:** Speaker layout editor + file I/O

**Rationale for phasing:**
- VBAP triangulation is complex → build incrementally
- SAF integration requires platform-specific setup → validate macOS first
- WebView UI with animated visualizer → separate from basic controls
- Multi-channel bus testing → test per phase (stereo → 5.1 → custom)

### High-Risk Features

**1. VBAP Triangulation (HIGH RISK)**
- **Risk:** Delaunay triangulation on sphere is non-trivial; naive implementations fail for irregular layouts
- **Fallback:** Use SAF `saf_vbap` (Plan A), fall back to 2D pair-wise panning (Plan C) if 3D fails
- **Status:** Mitigated by using SAF library (production-ready)

**2. Custom Path Editor (HIGH RISK - DEFERRED)**
- **Risk:** User-drawable paths require spline editing in WebView + spline evaluation on audio thread
- **Fallback:** Omit custom path for MVP; provide Orbit/Pendulum/Linear/Drift only
- **Status:** Deferred to v1.1 (not MVP-critical)

**3. Multi-Channel Bus Negotiation (MEDIUM RISK)**
- **Risk:** DAWs have inconsistent multi-channel support (Ableton is stereo-only)
- **Fallback:** Auto-downmix to stereo always available
- **Mitigation:** Test in Logic Pro and Reaper early (best multichannel support)

**4. SAF Build Complexity on Windows (MEDIUM RISK)**
- **Risk:** Windows requires BLAS/LAPACK (Intel MKL or OpenBLAS)
- **Fallback:** macOS-only MVP, add Windows later
- **Mitigation:** Use OpenBLAS on Windows for simpler CI/CD

### Constraints & Assumptions

**Constraints:**
- JUCE 8.0.4 multi-channel bus API (no legacy `PLUGIN_CHANNEL_CONFIGURATIONS`)
- WebView UI only (no native JUCE GUI)
- Zero added latency (VBAP is non-blocking, no lookahead)
- Maximum 24 output channels (practical limit for speaker arrays)

**Assumptions:**
- SAF library integrates without major build issues
- VBAP triangulation works correctly for custom non-equidistant layouts
- WebView orbital visualizer achieves 60fps without optimization
- Multi-channel bus works in Logic Pro and Reaper without DAW-specific workarounds

**Dependencies:**
- SAF: `saf_vbap`, `saf_utilities` (ISC license)
- JUCE: `juce::juce_dsp`, `juce::juce_gui_extra`
- BLAS/LAPACK: Apple Accelerate (macOS), Intel MKL or OpenBLAS (Windows)

### Professional Plugins Researched

**VBAP Implementations:**
1. **SPARTA Panner** (Leo McCormack) - JUCE + SAF, 1-128 inputs, VBAP to arbitrary arrays
   - **Key insight:** Uses SAF `generateVBAPgainTable3D()` for pre-computed gain tables
   - **Architecture:** Per-source VBAP gain calculation, per-block processing

2. **Aalto VBAP Library** (Ville Pulkki) - Original VBAP MATLAB/C implementation
   - **Key insight:** Delaunay triangulation on unit sphere via convex hull
   - **Algorithm:** Triplet-wise panning for 3D, pair-wise for 2D

3. **IEM Plugin Suite** - Academic spatial audio plugins (Graz University)
   - **Key insight:** Multi-channel bus configuration patterns for JUCE
   - **Note:** Uses Ambisonics, not VBAP (different approach)

**Motion Panning Plugins:**
1. **Waves Brauer Motion** - Circular auto-panner (stereo only)
   - **Key insight:** Spherical panner with rhythmic energy injection
   - **Limitation:** Stereo only, no multi-channel support

2. **TAL-Filter-2** - LFO plugin with tempo sync
   - **Key insight:** Auto sync mode, MIDI/audio triggering options
   - **Pattern:** Multiple triggering methods for modulation

### JUCE Classes & Module Dependencies

**Core JUCE Classes Used:**
- `juce::AudioProcessor` - Plugin base class
- `juce::AudioProcessorValueTreeState` - Parameter management
- `juce::dsp::Oscillator<float>` - LFO for motion engine
- `juce::dsp::IIR::Filter<float>` - Air absorption LPF
- `juce::dsp::IIR::Coefficients<float>::makeLowPass()` - LPF coefficient generation
- `juce::WebBrowserComponent` - WebView UI
- `juce::WebSliderRelay` - Parameter binding
- `juce::WebSliderParameterAttachment` - Automation bridge
- `juce::ValueTree` - Speaker layout persistence
- `juce::XmlElement` - Layout file I/O
- `juce::FileChooser` - Import/export dialogs

**JUCE Module Dependencies:**
- `juce::juce_audio_processors` - Plugin framework
- `juce::juce_dsp` - IIR filters, oscillators
- `juce::juce_gui_extra` - WebBrowserComponent
- `juce::juce_gui_basics` - Base UI classes

**SAF Functions Used:**
- `generateVBAPgainTable3D()` - Pre-compute VBAP gain table
- `findLsTriplets()` - Delaunay triangulation on sphere
- `invertLsMtx3D()` - Precompute inverse matrices
- `vbap3D()` - Direct VBAP gain computation

### Research Resources Consulted

**VBAP Algorithm:**
- Ville Pulkki's VBAP Library (http://research.spa.aalto.fi/projects/vbap-lib/vbap.html)
- polarch/Vector-Base-Amplitude-Panning (GitHub)
- Andrew McWilliams - Vector-Base Amplitude Panning blog post

**SAF Integration:**
- Spatial Audio Framework repository (GitHub)
- SAF API Documentation (leomccormack.github.io)
- SPARTA plugin suite (reference implementations)

**JUCE Multi-Channel:**
- Local research: `research/juce8-multichannel-spatial-audio.md` ✅
- Local research: `research/sound-spatialization-algorithms.md` ✅
- Local research: `research/saf-juce-integration-guide.md` ✅
- JUCE Bus Layouts Tutorial (docs.juce.com)

**Distance Modeling:**
- Air absorption DSP (Code & Sound blog)
- Atmospheric absorption filter approximation (Computing and Recording blog)
- ISO 9613-1 (Attenuation of sound during propagation outdoors)

---

## Outputs Generated

1. **ARCHITECTURE.md** - Complete DSP architecture specification
   - 10 required sections: Components, Processing Chain, System Architecture, Parameters, Algorithms, Integration, Risks, Decisions, Considerations, References
   - Every feature from BRIEF.md documented
   - Every JUCE class has module dependency noted
   - Every HIGH risk feature has fallback architecture

2. **ROADMAP.md** - Implementation plan with complexity assessment
   - Complexity score: 4.2/5.0
   - Implementation strategy: Staged (3 DSP phases, 3 GUI phases)
   - Stage breakdown with test criteria per phase
   - Risk register with mitigations
   - Timeline estimate: 15-21 hours (2-3 work days)

3. **CONTEXT.md** (this file) - Phase findings and decisions

---

## Decisions Made

**1. Algorithm Selection:**
- ✅ VBAP (not Ambisonics) for direct amplitude panning
- ✅ SAF library for VBAP triangulation (not scratch implementation)
- ✅ Per-block VBAP gain calculation with per-sample interpolation

**2. Feature Scope:**
- ✅ MVP includes: 4 motion paths (Orbit/Pendulum/Linear/Drift)
- ✅ MVP includes: VBAP 2D and 3D with custom layouts
- ❌ MVP excludes: Custom path drawing (deferred to v1.1)
- ❌ MVP excludes: Doppler effect (deferred to v1.1)
- ❌ MVP excludes: Early reflections / room simulation (deferred to v1.1)

**3. Build System:**
- ✅ SAF as git submodule
- ✅ macOS: Apple Accelerate (auto-detected)
- ✅ Windows: Intel MKL (primary) or OpenBLAS (fallback)
- ✅ WebView: `NEEDS_WEB_BROWSER TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`

**4. Multi-Channel Strategy:**
- ✅ Flexible channel count (2-24), NOT named layouts
- ✅ Auto-downmix when DAW provides fewer channels
- ✅ Visual warning in UI when downmix active

---

## Next Actions

**Proceed to Stage 1 (Foundation):**
- Run `/implement O-Orbit` to invoke foundation-shell-agent
- Foundation agent will create:
  - CMakeLists.txt with SAF integration
  - All 17 parameters in APVTS
  - Project structure (Source/, Resources/ui/)
  - Stub PluginProcessor and PluginEditor
  - Initial build verification

**After Stage 1:**
- Review build output and parameter definitions
- Proceed to Stage 2 Phase 2.1 (Motion Engine + Stereo Panning)

---

## Status Summary

**Research Complete:** ✅
- Complexity assessed: 4.2/5.0 (Tier 5 - Complex)
- Implementation strategy: Staged (phased DSP + GUI)
- Technical approach validated: VBAP via SAF
- Risk mitigations documented: Fallback architectures for HIGH risks

**Planning Complete:** ✅
- ARCHITECTURE.md: 10/10 required sections
- ROADMAP.md: Complexity calculation, phase breakdown, test criteria
- CONTEXT.md: Decisions, constraints, research findings

**Ready for Implementation:** ✅
- All contracts from Ideation present (BRIEF.md, REQUIREMENTS.md)
- Architecture and plan documented
- No blocking issues identified
- Next step: Stage 1 (Foundation) via `/implement O-Orbit`

---

**End of Stage 0 Context**
