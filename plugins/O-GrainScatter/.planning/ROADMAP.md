# O-GrainScatter Implementation Roadmap

## Complexity Assessment

| Factor | Value | Weight | Score |
|--------|-------|--------|-------|
| Parameter Count | 17 | x1 | 17 |
| DSP Components | 7 (DelayBuffer, GrainPool, Scheduler, Tempo, Freeze, ScaleQuantizer, Euclidean) | x3 | 21 |
| Beat Sync (PPQ) | Yes | x5 | 5 |
| Voice Polyphony | 64 voices | x3 | 3 |
| UI Complexity | WebView + grain viz + Euclidean viz | x2 | 4 |
| Heritage Code | Scatter reference (conceptual, not copy) | x-2 | -2 |

**Complexity Score: 48 / 60 — High**

This is a complex plugin with multiple interacting DSP subsystems, beat synchronization, and a polyphonic voice engine. The extensive research document de-risks the DSP design significantly.

### Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| PPQ drift across DAWs | Medium | High | Test in Logic + Ableton early; use subdivision crossing (floor-based) not accumulator |
| Click artifacts on grain boundaries | Medium | Medium | Hann window + fade-in/out on voice steal |
| CPU overload at max density + fast subdivisions | Low | Medium | Voice limit (64) acts as natural cap; measure with profiler |
| Freeze engage/release clicks | Medium | Medium | Crossfade frozen↔live over ~5ms |
| Euclidean pattern thread safety | Low | Low | Use fixed-size array + atomic length |

---

## Stage Breakdown

### Stage 1: Foundation + Shell
**Goal:** Buildable plugin with all parameters defined, WebView shell loading, audio passthrough.

| Task | Description | Est |
|------|-------------|-----|
| 1.1 | CMakeLists.txt with NEEDS_WEBVIEW2, static linking flags | S |
| 1.2 | PluginProcessor scaffolding with APVTS (all 17 params) | M |
| 1.3 | PluginEditor with WebBrowserComponent + Options setup | M |
| 1.4 | Minimal index.html (loads, shows plugin name) | S |
| 1.5 | Audio passthrough in processBlock | S |
| 1.6 | Build verification (VST3 + AU) | S |

**Exit Criteria:** Plugin loads in DAW, shows WebView, passes audio through unchanged, all 17 parameters visible in DAW's generic editor.

### Stage 2: DSP Implementation
**Goal:** Complete audio engine — all grain processing, beat sync, freeze, pitch modes, Euclidean patterns.

| Task | Description | Est | Depends |
|------|-------------|-----|---------|
| 2.1 | DelayBuffer (circular buffer + Lagrange3rd read) | M | — |
| 2.2 | GrainVoice struct + GrainPool (spawn, process, Hann window) | L | 2.1 |
| 2.3 | GrainScheduler — Free mode (density-based timer) | M | 2.2 |
| 2.4 | Core processBlock integration (delay→scheduler→pool→mix) | L | 2.1-2.3 |
| 2.5 | TempoTracker (PPQ reading, manual fallback) | M | — |
| 2.6 | GrainScheduler — Sync mode (subdivision crossing) | M | 2.3, 2.5 |
| 2.7 | ScaleQuantizer (5 scales, root note, pitch→rate) | M | — |
| 2.8 | PitchLadder modes (Random, Up, Down, Pendulum) | M | 2.7 |
| 2.9 | Texture morph (position spread + voice count modulation) | S | 2.4 |
| 2.10 | EuclideanGenerator + scheduler integration | M | 2.6 |
| 2.11 | FreezeManager (capture, read, engage/release) | M | 2.1 |
| 2.12 | Feedback path (grain output → delay input) | S | 2.4 |
| 2.13 | Pan randomization + reverse grain playback | S | 2.2 |
| 2.14 | Dry/wet mixing with smoothed crossfade | S | 2.4 |
| 2.15 | Repeat count logic (return to live after N repeats) | S | 2.6 |

**Size estimates:** S = small (~30 min), M = medium (~1-2 hrs), L = large (~2-4 hrs)

**Exit Criteria:** All DSP features functional. Beat sync accurate in Logic Pro. Freeze engages/releases cleanly. Euclidean patterns audibly correct. No clicks on parameter changes. Passes pluginval level 5.

### Stage 3: GUI Implementation
**Goal:** WebView UI with all controls, grain visualization, Euclidean circle visualizer.

| Task | Description | Est |
|------|-------------|-----|
| 3.1 | UI mockup design (YAML + test HTML) | M |
| 3.2 | WebView ↔ C++ parameter relay system | M |
| 3.3 | Core controls — knobs for granular params | M |
| 3.4 | Sync section — dropdown + probability + repeats | M |
| 3.5 | Texture slider (stutter↔cloud label morph) | S |
| 3.6 | Pitch mode dropdown + Freeze toggle button | S |
| 3.7 | Euclidean controls (pulses/steps) + circle visualizer | L |
| 3.8 | Grain activity visualization (optional — real-time dots) | L |
| 3.9 | Styling, layout polish, responsive sizing | M |

**Exit Criteria:** All parameters controllable from WebView. Euclidean visualizer updates on parameter change. Plugin looks professional and is usable. UI ↔ DSP fully bidirectional.

---

## Implementation Order

```
Stage 1 (Foundation)
    │
    ▼
Stage 2 (DSP) ──── Build in layers:
    │                 2.1-2.4: Core grain engine (delay + pool + free scheduler + mix)
    │                 2.5-2.6: Beat sync layer
    │                 2.7-2.8: Pitch/scale layer
    │                 2.9-2.15: Feature layer (texture, euclidean, freeze, feedback, etc.)
    │
    ▼
Stage 3 (GUI) ──── Can start UI mockup in parallel with late Stage 2
```

---

## Dependencies

- **JUCE 8.0.4** at `/Users/taylorbrook/JUCE`
- **CMake + Ninja** build system
- **WebView2** static linking for Windows cross-platform support
- No external DSP libraries required — all algorithms implemented from scratch

---

## Success Criteria

1. Free mode produces density-controlled grain clouds identical in character to Scatter heritage
2. Beat sync triggers grains at musically correct PPQ positions for all 6 subdivisions
3. Triplet timing (1/8T, 1/16T) is audibly accurate
4. Freeze captures and loops without clicks or artifacts
5. Texture slider provides smooth, audible morph from tight stutter to scattered cloud
6. All 4 pitch modes produce musically correct, scale-quantized sequences
7. Euclidean patterns match expected Bjorklund output (e.g., E(3,8) = [x..x..x.])
8. Plugin passes pluginval at strictness level 5
9. No audio clicks on parameter changes, mode switches, or freeze engage/release
10. Standalone mode works with manual tempo tracking (no DAW required)
