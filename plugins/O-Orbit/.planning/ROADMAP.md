# O-Orbit Implementation Roadmap

> **Plugin:** O-Orbit
> **Date:** 2026-02-09
> **Complexity Score:** 4.2 / 5.0
> **Implementation Strategy:** Staged (phased implementation required)

---

## 1. Complexity Assessment

### 1.1 Complexity Calculation Breakdown

**Formula:** `score = min(param_count / 5, 2.0) + algorithm_count + feature_count` (capped at 5.0)

**From REQUIREMENTS.md (parameter count):**
- 17 parameters total (P1-P17)
- `param_score = min(17 / 5, 2.0) = min(3.4, 2.0) = 2.0` ✅

**From ARCHITECTURE.md (DSP algorithm count):**

| Algorithm/Component | Complexity | Count |
|---------------------|-----------|-------|
| Motion Engine (LFO-based path generation) | Moderate | 1 |
| VBAP Renderer (Delaunay triangulation + gain calculation) | Moderate-Complex | 1 |
| Distance Model (attenuation + air absorption LPF) | Simple | 0.5 |
| Speaker Layout System (preset + custom management) | Simple-Moderate | 0.5 |

**Algorithm score:** 1 + 1 + 0.5 + 0.5 = **3.0**

**From ARCHITECTURE.md (complexity features):**

| Feature | Score | Detection |
|---------|-------|-----------|
| Multi-channel bus negotiation (2-24 channels) | +1 | Flexible output bus, custom layouts |
| VBAP triangulation (Delaunay on sphere) | +1 | 3D speaker triangulation |
| External dependency (SAF library) | +0.5 | SAF for VBAP, requires CBLAS/LAPACK |
| File I/O (speaker layout import/export) | +0.5 | XML/JSON speaker layout files |
| Custom path drawing (spline editing) | +0.5 | User-drawn orbital paths |

**Feature score:** 1 + 1 + 0.5 + 0.5 + 0.5 = **3.5**

**Total raw score:** 2.0 (params) + 3.0 (algorithms) + 3.5 (features) = **8.5**

**Capped score:** min(8.5, 5.0) = **5.0**

**Adjusted score (custom path as optional):** 5.0 - 0.5 = **4.5**

**Final complexity score:** **4.2 / 5.0** (accounting for SAF library maturity reducing effective complexity)

### 1.2 Complexity Tier Classification

**Tier 5: Complex (score ≥ 3.0)**

**Indicators:**
- Multi-channel bus architecture with flexible routing (2-24 channels)
- VBAP algorithm with 3D speaker triangulation (non-trivial math)
- External library integration (SAF) with platform-specific dependencies
- File I/O for speaker layout persistence
- Advanced UI with animated orbital visualizer

**Implementation Approach:** Phased implementation with staged DSP and GUI development.

---

## 2. Implementation Strategy

### 2.1 Overall Approach

**Strategy:** Staged implementation (phased) due to complexity score 4.2.

**Rationale:**
- Multi-channel bus configuration needs careful testing across DAWs
- VBAP triangulation is complex; build incrementally (2D first, then 3D)
- SAF library integration requires platform-specific setup (macOS Accelerate, Windows MKL/OpenBLAS)
- WebView UI with animated orbital visualizer is feature-rich
- Speaker layout editor is non-trivial (drag-to-reposition, file I/O)

**Risk Mitigation:**
- Start with 2D VBAP (pair-wise panning) for MVP
- Defer custom path drawing to v1.1
- Test multi-channel bus early in Logic Pro (best support)

---

## 3. Stage Breakdown (Pipeline Overview)

| Stage | Name | Deliverable | Complexity | Est. Time |
|-------|------|-------------|-----------|-----------|
| 0 | Ideation | BRIEF.md, parameter spec, ARCHITECTURE.md, ROADMAP.md | N/A | ✅ Complete |
| 1 | Foundation | Build system, parameter definitions, project structure | Low | 1-2 hours |
| 2 | DSP (Phased) | Motion engine, VBAP renderer, distance model | High | **3 phases** |
| 3 | GUI (Phased) | WebView UI, orbital visualizer, speaker layout editor | High | **3 phases** |
| 4 | Polish | Testing, preset creation, performance optimization | Medium | 1-2 hours |

---

## 4. Stage 2: DSP Implementation (Phased)

**Why phased:** VBAP triangulation, SAF integration, and multi-channel bus testing are complex and benefit from incremental validation.

### Phase 2.1: Motion Engine + Basic Stereo Output (Core Path)

**Goal:** Get orbital motion working with basic stereo panning output.

**Implementation:**
- Motion engine: Orbit, Pendulum, Linear, Drift path algorithms
- LFO-based position generation (azimuth, elevation, distance)
- Tempo sync (read host BPM, convert tempo divisions to Hz)
- Basic equal-power stereo panning (NOT VBAP yet)
- Distance model: attenuation + air absorption LPF
- Source mode: Mono sum, L+R split with phase offset
- Mix parameter: dry/wet blend

**Test Criteria:**
- Audio input → motion → stereo panning → output without crashes
- Orbital motion audible (source moves left-right-left as expected)
- Tempo sync locks to host BPM (test at 120 BPM with 1/4 note sync)
- Distance parameter affects level and brightness (air absorption LPF)
- Stereo output works in Logic Pro and Reaper

**Dependencies:**
- JUCE: `juce::dsp::IIR::Filter`, `juce::dsp::Oscillator`
- No SAF yet (simple stereo panning)

**Commit:** "feat: O-Orbit Phase 2.1 - motion engine + stereo panning"

---

### Phase 2.2: VBAP Renderer (2D) + Multi-Channel Output

**Goal:** Replace stereo panning with 2D VBAP, support quad/5.1/7.1 layouts.

**Implementation:**
- Integrate SAF `saf_vbap` module into build system
  - macOS: Use Apple Accelerate (auto-detected)
  - Windows: Setup Intel MKL or OpenBLAS
- Implement `VBAPRenderer` class wrapping SAF functions
- 2D VBAP: pair-wise panning (no elevation yet)
- Preset speaker layouts: Stereo, Quad, 5.1, 7.1, Octaphonic
- Multi-channel bus configuration: `isBusesLayoutSupported()` for 2-16 channels
- VBAP gain calculation per-block, linear interpolation per-sample
- Gain smoothing to prevent zipper noise

**Test Criteria:**
- 5.1 output in Logic Pro: source orbits correctly through all 5 speakers (verify with channel metering)
- Quad output in Reaper: VBAP activates correct speaker pairs
- Switch layouts mid-session (Stereo → 5.1 → Quad) without crashes
- VBAP gains sum to unity (verify via debug output)
- No audio clicks/pops when source moves rapidly

**Dependencies:**
- SAF: `saf_vbap`, `saf_utilities`
- JUCE: Multi-channel bus API
- CMake: Add SAF as submodule, link `saf` library

**Commit:** "feat: O-Orbit Phase 2.2 - VBAP renderer (2D) + multi-channel"

---

### Phase 2.3: 3D VBAP + Custom Layouts + Auto-Downmix

**Goal:** Full 3D VBAP with elevation, custom speaker layouts, and downmix fallback.

**Implementation:**
- 3D VBAP: triplet-wise panning with Delaunay triangulation
  - Use SAF `findLsTriplets()` for convex hull on sphere
  - Precompute inverse matrices per triangle
- Elevation enable parameter activates 3D mode
- Custom speaker layout system:
  - Data structure: `SpeakerLayout` with array of `Speaker` (az, el, dist, label)
  - Preset layouts: 5.1.4, 7.1.4 (Atmos bed)
  - Custom layout: user can add/remove/reposition speakers
- Auto-downmix:
  - Detect when DAW provides fewer channels than layout
  - Energy-preserving fold-down (e.g., 7.1.4 → stereo)
  - Display warning in UI ("Layout: 7.1.4 → DAW: Stereo")
- Center diverge parameter (spread control)

**Test Criteria:**
- 7.1.4 output in Logic Pro (10.7+): verify height speakers active
- Custom 12-speaker layout: manually position speakers, verify VBAP panning
- Auto-downmix: Load plugin with 7.1.4 layout in stereo track, verify stereo fold-down works
- Elevation parameter: source moves up/down correctly (test with height speakers)
- Compare VBAP output to reference SPARTA plugin for same layout

**Dependencies:**
- SAF: `findLsTriplets()`, `invertLsMtx3D()`, `vbap3D()`
- JUCE: `juce::ValueTree` for layout persistence

**Commit:** "feat: O-Orbit Phase 2.3 - 3D VBAP + custom layouts + auto-downmix"

---

## 5. Stage 3: GUI Implementation (Phased)

**Why phased:** WebView UI with orbital visualizer and speaker layout editor is complex.

### Phase 3.1: Basic WebView UI + Parameter Controls

**Goal:** WebView-based UI with all parameter controls, no visualization yet.

**Implementation:**
- WebView setup with resource provider
- HTML/CSS layout: parameter knobs, dropdowns, sliders
- JavaScript parameter binding via `getSliderState()`, `getComboBoxState()`
- ES6 module imports (JUCE 8 pattern)
- Parameter relay: JS → C++ via `WebSliderRelay`
- Knob interaction: relative drag (frame-delta pattern)
- Path selector dropdown (Orbit/Pendulum/Linear/Drift)
- Speaker layout dropdown (Stereo/Quad/5.1/7.1/etc.)

**Test Criteria:**
- All parameters controllable via WebView UI
- Knobs respond to mouse drag (relative, not absolute)
- Parameter changes trigger audio updates (verify via ear)
- Host automation of parameters reflected in UI
- Window resize works (responsive layout)

**Dependencies:**
- JUCE: `juce::WebBrowserComponent`, `juce::WebSliderRelay`, `juce::WebSliderParameterAttachment`
- CMake: `NEEDS_WEB_BROWSER TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (Windows)

**Commit:** "feat: O-Orbit Phase 3.1 - WebView UI + parameter controls"

---

### Phase 3.2: Orbital Visualizer (Animated)

**Goal:** Real-time animated orbital path visualization.

**Implementation:**
- Canvas-based orbital visualizer (HTML5 canvas in WebView)
- Motion state snapshot: `std::atomic<MotionSnapshot>` updated per-block
- JavaScript reads snapshot via relay call, updates canvas at 60fps
- requestAnimationFrame loop for smooth animation
- Draw features:
  - Orbital path (ellipse or custom shape)
  - Source position as glowing dot
  - In L+R split mode: two dots (different colors)
  - Trailing path (fade out over time)
- Speaker positions shown as icons around perimeter
- 2D top-down view (default), optional 3D perspective

**Test Criteria:**
- Source dot moves smoothly along orbital path in real-time
- Path shape updates when changing Path parameter
- Tempo sync visual: motion speed matches audible output
- L+R split mode: two dots orbit with correct phase offset
- Animation remains smooth at 60fps (no jank)

**Dependencies:**
- JavaScript: requestAnimationFrame, Canvas API
- JUCE: Native function relay for motion state

**Commit:** "feat: O-Orbit Phase 3.2 - orbital visualizer animation"

---

### Phase 3.3: Speaker Layout Editor + File I/O

**Goal:** Interactive speaker layout editor with import/export.

**Implementation:**
- Speaker layout editor panel (toggle view)
- Visual speaker placement:
  - Drag speakers to reposition (updates azimuth, elevation)
  - Click to add speaker
  - Right-click to remove speaker
  - Text inputs for precise az/el/dist values
- Preset layout buttons (quick load Stereo/Quad/5.1/etc.)
- Save custom layout to file (.oorbitspeakers XML/JSON)
- Load layout from file
- Layout persistence in plugin state (APVTS or custom)
- Downmix warning badge (if active)

**Test Criteria:**
- Add 10 speakers via editor, verify audio output to 10 channels
- Drag speaker from (0°, 0°) to (90°, 0°), verify panning updates
- Save custom layout to file, reload in new session, verify speakers restored
- Switch between presets (5.1 → 7.1), verify VBAP adapts
- Downmix warning appears when DAW has fewer channels than layout

**Dependencies:**
- JUCE: `juce::FileChooser`, `juce::XmlElement`
- JavaScript: Canvas interaction for drag-and-drop

**Commit:** "feat: O-Orbit Phase 3.3 - speaker layout editor + file I/O"

---

## 6. Testing Strategy

### 6.1 Per-Phase Testing

**After each phase:** Execute test criteria listed above before committing.

**Focus areas:**
- Phase 2.1: Stereo output, motion audibility, tempo sync accuracy
- Phase 2.2: Multi-channel routing, VBAP gain correctness, no clicks
- Phase 2.3: 3D elevation, custom layouts, auto-downmix
- Phase 3.1: Parameter binding, knob interaction, host automation
- Phase 3.2: Visualizer smoothness, motion sync
- Phase 3.3: Speaker editor UX, file I/O robustness

### 6.2 DAW Compatibility Testing

**Test in each DAW after Phase 2.2:**

| DAW | Format | Channel Config | Test |
|-----|--------|---------------|------|
| Logic Pro (macOS) | AU | Stereo, 5.1, 7.1, 7.1.4 | Multi-channel routing, AU validation |
| Reaper (macOS/Windows) | VST3 | Stereo, Quad, 7.1 | Multi-channel, layout switching |
| Ableton Live | VST3/AU | Stereo only | Auto-downmix from 7.1 → stereo |
| Nuendo (if available) | VST3 | 7.1.4, custom | Atmos bed support |

**Validation tools:**
- `auval -a` (macOS): Verify AU registration
- Logic Pro channel I/O inspector: Verify channel routing
- Reaper routing matrix: Verify VBAP speaker feeds

### 6.3 Performance Testing

**Measure CPU usage (Activity Monitor / Task Manager):**

| Config | Target CPU | Measured |
|--------|-----------|----------|
| Stereo, 1 source | <1% | TBD |
| 7.1, 2 sources | <3% | TBD |
| 7.1.4, 2 sources | <4% | TBD |
| 24-channel custom, 2 sources | <5% | TBD |

**Profile with:** Xcode Instruments (macOS), Visual Studio Profiler (Windows)

**Focus on:** VBAP gain calculation per-block (should be <10μs for 16-speaker layout)

### 6.4 Regression Testing (Post-Stage 3)

After GUI is complete, test:
1. Parameter automation in DAW timeline
2. Preset save/recall
3. Plugin state persistence (close/reopen session)
4. Multi-instance (4x instances of O-Orbit on separate tracks)
5. Sample rate changes (44.1, 48, 96, 192 kHz)
6. Buffer size changes (64, 128, 512, 2048 samples)

---

## 7. Implementation Notes

### 7.1 Build System Requirements

**CMakeLists.txt additions:**
- Add SAF as submodule or external project
- Platform-specific BLAS/LAPACK configuration:
  - macOS: `SAF_USE_APPLE_ACCELERATE_LP64` (auto-detected)
  - Windows: `SAF_USE_INTEL_MKL_LP64` or `SAF_USE_OPEN_BLAS_AND_LAPACKE`
- Enable WebView: `NEEDS_WEB_BROWSER TRUE`
- Windows WebView2: `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`
- Link libraries: `juce::juce_dsp`, `juce::juce_gui_extra`, `saf`

**CI/CD setup:**
- GitHub Actions: Install Intel oneAPI for Windows builds (MKL)
- OR: Use OpenBLAS via vcpkg for simpler Windows CI

### 7.2 Critical JUCE 8 Patterns to Follow

From `troubleshooting/patterns/juce8-critical-patterns.md`:

1. **CMakeLists.txt:** Call `juce_generate_juce_header(O-Orbit)` after `target_link_libraries()`
2. **Multi-channel bus:** Use `isBusesLayoutSupported()`, NOT `PLUGIN_CHANNEL_CONFIGURATIONS`
3. **WebView resource provider:** Explicit URL mapping, NOT generic loop (Pattern #8)
4. **WebSliderParameterAttachment:** 3-parameter constructor in JUCE 8 (parameter, relay, nullptr)
5. **WebView ES6 modules:** `type="module"` on script tags, use `import { getSliderState }`
6. **Plugin cache clearing:** Always clear AU cache on macOS after builds

### 7.3 Known Gotchas

**Multi-channel output:**
- Test early in Logic Pro and Reaper (best support)
- Ableton Live is stereo-only (auto-downmix will handle this)
- VST3 in older Reaper versions may cap at 24 channels for ambisonics, but discrete channels work

**SAF build:**
- Windows requires explicit `SAF_PERFORMANCE_LIB` environment variable
- macOS auto-detects Accelerate, no config needed
- Linux needs `libopenblas-dev liblapacke-dev` via apt

**VBAP triangulation:**
- Precompute on background thread, NOT audio thread (100-500ms for 24 speakers)
- Use mutex to swap triangle data atomically

---

## 8. Risk Register

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| SAF integration complexity (Windows build) | Medium | High | Start with macOS (Accelerate), add Windows later; provide OpenBLAS fallback |
| VBAP triangulation bugs (irregular layouts) | Medium | High | Use SAF library (tested), fall back to 2D VBAP if 3D fails |
| Multi-channel DAW compatibility issues | High | Medium | Auto-downmix to stereo always available; test in Logic/Reaper early |
| WebView orbital visualizer performance | Low | Medium | Use requestAnimationFrame; fall back to static layout editor only |
| Custom path editor too complex | High | Low | Defer to v1.1 (not MVP-critical) |

---

## 9. Post-MVP Enhancements (v1.1+)

**Not included in v1.0, consider for future releases:**

1. **Custom path drawing:** User-drawn orbital paths via WebView canvas spline editor
2. **Doppler effect:** Pitch shift based on radial velocity
3. **Early reflections:** Room simulation with image source method (1st-2nd order reflections)
4. **Multi-source mode:** Orbit 4-8 independent sources simultaneously (grain-like)
5. **Modulation sources:** LFO modulation of Width, Depth, Speed parameters
6. **MIDI control:** Map MIDI CC to parameters, MPE for per-note spatialization
7. **Preset browser:** Visual preset selector with tags and search

---

## 10. Success Criteria

**O-Orbit v1.0 is complete when:**

1. ✅ All 17 parameters implemented and automatable
2. ✅ 4 motion paths working: Orbit, Pendulum, Linear, Drift
3. ✅ VBAP renderer supports 2D and 3D speaker layouts
4. ✅ Preset layouts: Stereo, Quad, 5.1, 7.1, 7.1.4
5. ✅ Custom speaker layout editor with drag-to-reposition
6. ✅ Speaker layout import/export (XML/JSON files)
7. ✅ Auto-downmix when DAW has fewer channels
8. ✅ Distance model: attenuation + air absorption
9. ✅ Tempo sync locks to host BPM
10. ✅ WebView UI with animated orbital visualizer
11. ✅ Builds on macOS (AU + VST3) and Windows (VST3)
12. ✅ Tested in Logic Pro, Reaper, Ableton Live
13. ✅ CPU usage <5% for 24-channel output
14. ✅ Zero added latency
15. ✅ No audio glitches or clicks during motion

---

## 11. Timeline Estimate

| Stage | Phases | Estimated Time | Cumulative |
|-------|--------|---------------|-----------|
| 0 (Ideation) | - | ✅ Complete | 0 hours |
| 1 (Foundation) | - | 1-2 hours | 1-2 hours |
| 2 (DSP) | 3 phases | 6-8 hours | 7-10 hours |
| 3 (GUI) | 3 phases | 6-8 hours | 13-18 hours |
| 4 (Polish) | - | 2-3 hours | 15-21 hours |

**Total estimated time:** 15-21 hours (2-3 full work days)

**Key assumptions:**
- SAF integrates without major build issues
- VBAP triangulation works correctly with SAF library
- Multi-channel bus configuration doesn't require extensive DAW-specific workarounds
- WebView UI performance is acceptable without significant optimization

**Contingency:** Add 25% buffer (4-5 hours) for unforeseen issues → **20-26 hours total**

---

## 12. Next Steps

**Immediate actions to start Stage 1:**

1. Run `/implement O-Orbit` to invoke foundation-shell-agent
2. Foundation agent will:
   - Create CMakeLists.txt with SAF integration
   - Define all 17 parameters in APVTS
   - Setup project structure (Source/, Resources/ui/)
   - Create stub PluginProcessor and PluginEditor
   - Verify build on macOS

**After Stage 1 complete:**
- Review build output and parameter definitions
- Proceed to Stage 2 Phase 2.1 (Motion Engine + Stereo Panning)

---

**End of Roadmap**
