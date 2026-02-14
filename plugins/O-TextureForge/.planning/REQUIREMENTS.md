# O-TextureForge Requirements

Extracted from BRIEF.md

## Functional Requirements

### FR-1: Audio File Loading
- Load single audio file via drag-and-drop onto plugin UI
- Load via file browser button
- Support WAV, AIFF, MP3, FLAC formats
- Display file name in header after loading
- Segment loaded audio into micro-grains (configurable size 10-500ms)

### FR-2: Descriptor Extraction
- Analyze each grain for timbral descriptors on background thread
- Extract: spectral centroid, spectral energy, spectral flatness, MFCC coefficients, zero-crossing rate
- Use juce::dsp::FFT (no external dependencies)
- Store descriptors in a feature matrix for KD-tree indexing

### FR-3: Dimensionality Reduction
- Compute PCA projection instantly on file load (<100ms) for initial 2D layout
- Compute UMAP on background thread (2-8s expected for 10K grains)
- Animate transition from PCA to UMAP layout when UMAP completes
- Show progress indicator during UMAP computation

### FR-4: KD-Tree Nearest-Neighbor Search
- Build nanoflann KD-tree from descriptor feature matrix
- Query nearest neighbors from audio thread (allocation-free)
- Support variable search radius (Variation parameter)
- Return N nearest grains for Grain Density parameter

### FR-5: Grain Scheduler
- Polyphonic grain playback (up to 64 simultaneous grains)
- Per-grain windowing (Hann or Tukey envelope)
- Crossfade control between overlapping grains
- Grain size control (10-500ms)
- Non-repeating grain selection with variation randomization

### FR-6: Scatter Plot Visualization
- WebGL 2D scatter plot using regl-scatterplot library
- Each grain = one point in scatter space
- Color-coded by spectral characteristics (warm earth tones)
- Pan and zoom navigation
- Click to set playback target position
- Drag to scrub through timbral space
- Active grains pulse/highlight when triggered
- Animated cursor showing current playback position
- 30Hz state push from C++ via emitEventIfBrowserIsVisible

### FR-7: Macro Controls
- Energy knob (0.0-1.0): bias grain selection quiet↔loud
- Brightness knob (0.0-1.0): bias grain selection dark↔bright
- Texture knob (0.0-1.0): bias grain selection smooth↔rough
- These parameters weight the descriptor search, reshaping which grains are selected

### FR-8: Secondary Controls
- Position slider (0.0-1.0): temporal position in source file
- Grain Density (1-64): simultaneous grain count
- Grain Size (10-500ms): individual grain length
- Variation (0.0-1.0): randomization radius around target
- Crossfade (0-100%): grain overlap amount
- Output Gain (-inf to +12dB): master level

### FR-9: MIDI Integration
- Three selectable modes via toggle button:
  - **Pitch-mapped**: chromatic repitching, C3=original, velocity=Energy, polyphonic (8 voices)
  - **Trigger + Modulate**: any note triggers at cursor, velocity=variation radius, mod wheel/aftertouch=XY
  - **Generative Drone**: continuous output, MIDI CC maps to all parameters
- Mode selection persisted in plugin state

### FR-10: Scatter Plot Interaction → C++
- Click on scatter plot sends target XY coordinates to C++ via withNativeFunction
- Scatter X/Y parameters update from plot clicks
- C++ grain selection uses scatter position as primary search target

## Non-Functional Requirements

### NFR-1: Real-Time Safety
- Audio thread: only allocation-free KD-tree queries and grain playback
- All descriptor extraction runs on background thread
- All UMAP/PCA computation runs on background thread
- Lock-free communication between analysis and audio threads

### NFR-2: Performance
- Scatter plot renders at 60fps with up to 50K points
- C++ → WebView state push at 30Hz maximum
- Grain scheduling at sample-accurate timing
- File analysis completes within 10 seconds for typical files (1-5 minutes)

### NFR-3: Binary Size
- Custom DSP approach targets <500KB overhead
- Header-only dependencies (nanoflann, umappp) minimize bloat
- No large external library dependencies

### NFR-4: Cross-Platform
- macOS: AU + VST3
- Windows: VST3 with WebView2 (NEEDS_WEBVIEW2 TRUE, static linking)
- WebView URL scheme: getResourceProviderRoot() / getBackendResourceAddress()

### NFR-5: Aesthetic
- Ouaricon Naturalist template (ouaricon-naturalist-001)
- Botanical illustration: fern
- Scatter plot integrated into aged-paper visual language
