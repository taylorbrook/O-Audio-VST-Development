# O-GrainScatter Requirements

Auto-extracted from BRIEF.md on 2026-02-06

## Functional Requirements

### FR-1: Core Granular Engine
- 64-voice grain pool with pre-allocated voices
- 2-second delay buffer with Lagrange3rd interpolation
- Hann window envelope per grain
- Grain size control (10-500ms)
- Density-based grain scheduling (Free mode)
- Dry/wet mixing with feedback path

### FR-2: Scale-Quantized Pitch
- 5 musical scales: Chromatic, Major, Minor, Pentatonic, Whole Tone
- Root note selection (C through B)
- Pitch randomization with scale quantization
- Playback rate derived from semitone intervals

### FR-3: Beat Synchronization
- DAW tempo sync via AudioPlayHead (PPQ position tracking)
- Subdivision options: 1/4, 1/8, 1/16, 1/32, 1/8T, 1/16T
- PPQ boundary crossing detection for sample-accurate triggering
- Manual tempo fallback for standalone mode
- Probability gate (0-100%) per beat division

### FR-4: Freeze Mode
- Capture current delay buffer contents on activation
- Loop frozen audio indefinitely
- Grains read from frozen buffer instead of live delay
- Smooth transition on engage/release
- Freeze length = 4x current grain size

### FR-5: Texture Morph (Stutter to Cloud)
- Continuous 0-100% control
- At 0%: All grains spawn at same position (unison stutter)
- At 100%: Grains spawn at random positions (scattered cloud)
- Modulates both position spread and active grain count
- Smooth interpolation between states

### FR-6: Pitch Sequencing Modes
- Random: Existing scale-quantized random pitch (default)
- Ladder Up: Each successive grain steps up one scale degree
- Ladder Down: Each successive grain steps down one scale degree
- Pendulum: Up then down, bouncing at scale boundaries

### FR-7: Euclidean Rhythm Patterns
- Bjorklund's algorithm for pattern generation
- Configurable pulses (1-16) and steps (2-16)
- Pattern gates grain spawning in sync mode
- Step counter advances on each beat subdivision

### FR-8: Spatial Processing
- Per-grain pan randomization (0-100%)
- Per-grain reverse probability (0-100%)
- Stereo output from mono or stereo input

### FR-9: Feedback Path
- Grain output mixed back into delay buffer input
- Feedback amount (0-100%)
- Must not cause runaway gain at high settings

## Non-Functional Requirements

### NFR-1: Performance
- Real-time safe processBlock (no allocations)
- 64 simultaneous grains without CPU overload
- Fast subdivisions (1/32 at 180 BPM) handled gracefully

### NFR-2: DAW Compatibility
- Correct PPQ position reading across DAWs (Logic, Ableton, FL, Reaper)
- Offline rendering produces same output as real-time
- Tempo changes mid-playback handled without clicks/glitches

### NFR-3: State Persistence
- All parameters saved/restored via getStateInformation/setStateInformation
- Preset compatibility (APVTS-based)

### NFR-4: UI
- WebView-based interface
- Grain position visualization (real-time)
- Euclidean pattern circle visualizer
- Freeze toggle button with visual feedback
- Responsive controls for all parameters

### NFR-5: Formats
- VST3 and AU builds
- Cross-platform WebView support (NEEDS_WEBVIEW2 + static linking for Windows)

## Acceptance Criteria

1. Free mode (density-based) works identically to Scatter heritage engine
2. Beat sync triggers grains at correct PPQ positions for all subdivisions
3. Triplet timing is musically accurate
4. Freeze captures and loops without clicks or artifacts
5. Texture slider smoothly morphs between stutter and cloud
6. All 4 pitch modes produce musically correct sequences
7. Euclidean patterns match Bjorklund's algorithm output
8. Plugin passes pluginval at strictness level 5
9. No audio clicks on parameter changes or mode switches
10. Standalone mode works with manual tempo tracking
