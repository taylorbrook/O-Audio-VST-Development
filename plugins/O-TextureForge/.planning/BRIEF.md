# O-TextureForge

## One-Line Pitch
Infinite evolving textures from a single sound -- drop in any audio, sculpt non-repeating variations through an interactive grain scatter plot.

## Plugin Type
Instrument (VST3/AU)

## Core Concept
Load a single audio file -- a field recording, synth pad, drum loop, anything. O-TextureForge segments it into hundreds of micro-grains, analyzes each grain's timbral properties (spectral centroid, energy, roughness), and plots them on a 2D scatter plot that IS the instrument. The scatter plot is front-and-center: click to play grains, drag to scrub through timbral space, and watch active grains pulse as they fire. Three macro knobs (Energy, Brightness, Texture) weight the descriptor search to bias grain selection. A Position slider controls where in the source file grains are drawn from. Under the hood it's concatenative synthesis, but the user experience is "visual texture sculpting."

## Interaction Model
**XY Pad + Scatter Plot hybrid.** The scatter plot (WebGL via regl-scatterplot) is the primary interface -- a large, interactive 2D visualization of all grains in the loaded file. Users click/drag directly on the plot to set a target position. The three macro knobs (Energy, Brightness, Texture) act as descriptor weights that reshape the scatter layout and bias nearest-neighbor grain selection. Grain density control sets how many grains fire simultaneously. The result is infinite non-repeating output that responds to both spatial (plot position) and parametric (knob) control.

## MIDI Integration
Three complementary modes, selectable via toggle:

1. **Pitch-mapped** -- MIDI notes repitch grains chromatically. C3 = original pitch, other keys transpose up/down. Velocity maps to Energy. Polyphonic (up to 8 voices).
2. **Trigger + Modulate** -- Any MIDI note triggers grain playback at the current scatter plot cursor position. Velocity controls variation radius (how far from cursor to search). Mod wheel and aftertouch modulate XY position on the scatter plot.
3. **Generative Drone** -- Continuous output with internal clock, no MIDI note triggering needed. MIDI CC messages map to all macro controls for hardware knob control. Focus is on evolving, breathing textures.

## Scatter Plot Visualization (Central Feature)
The 2D WebGL scatter plot is the primary differentiator and dominant UI element:
- Built with regl-scatterplot (handles 50K+ points at 60fps)
- Each grain = one point, color-coded by spectral characteristics
- Pan/zoom for navigating large grain clouds
- Click to set playback position, drag to scrub
- Active grains pulse/highlight when triggered
- Animated cursor shows current playback position
- PCA for instant layout on file load, UMAP computed on background thread for final arrangement
- 30Hz state push from C++ to WebView via emitEventIfBrowserIsVisible

## Audio Engine
**Custom descriptor extraction** using juce::dsp::FFT (no external dependencies):
- Spectral centroid (brightness)
- Spectral energy (loudness/energy)
- Spectral flatness (noisiness/texture)
- MFCC coefficients (timbral fingerprint)
- Zero-crossing rate (roughness proxy)
- ~500 lines of DSP code, <500KB binary overhead

**Search:** nanoflann KD-tree for fast nearest-neighbor grain lookup (single header, BSD, allocation-free queries safe for audio thread)

**Dimensionality reduction:**
- PCA (Eigen) for instant 2D projection on load (<100ms)
- umappp (header-only, BSD-2) on background thread for final UMAP layout (2-8s with progress indicator)

## Key Parameters
| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Energy | 0.0-1.0 | 0.5 | Bias grain selection toward quiet (0) or loud (1) grains |
| Brightness | 0.0-1.0 | 0.5 | Bias toward dark (0) or bright (1) spectral content |
| Texture | 0.0-1.0 | 0.5 | Bias toward smooth/tonal (0) or rough/noisy (1) grains |
| Position | 0.0-1.0 | 0.0 | Temporal position in source file (which region to draw grains from) |
| Grain Density | 1-64 | 8 | Number of simultaneous grains |
| Grain Size | 10-500ms | 50ms | Length of each grain |
| Scatter X | 0.0-1.0 | 0.5 | Cursor X position on scatter plot |
| Scatter Y | 0.0-1.0 | 0.5 | Cursor Y position on scatter plot |
| Variation | 0.0-1.0 | 0.2 | Randomization radius around target position |
| Crossfade | 0-100% | 50% | Grain overlap/crossfade amount |
| Output Gain | -inf-+12dB | 0dB | Master output level |
| MIDI Mode | Pitch/Trigger/Drone | Drone | MIDI interaction mode |

## Aesthetic
**Ouaricon Naturalist** (ouaricon-naturalist-001) -- the official Ouaricon Audio brand aesthetic.
- Aged paper background, warm earth tones, serif typography (Garamond)
- Botanical seed cross-section knobs
- **Botanical illustration:** Fern (`fern_naturalistsmisc1Geor_0089.png`) -- fractal, self-similar, evolving patterns mirror the texture generation concept
- The scatter plot sits on the aged-paper background, with grain points rendered in warm earth tones (amber/brown dots, green highlights for active grains)
- Knobs arranged around the scatter plot, not competing with it

## UI Layout Concept
```
┌─────────────────────────────────────────────────────────┐
│  O-TEXTUREFORGE            [file name]    [MIDI: Drone] │
│                                                         │
│  ┌─────────────────────────────────────┐  ╭─╮ ╭─╮ ╭─╮ │
│  │                                     │  │E│ │B│ │T│ │
│  │        SCATTER PLOT (WebGL)         │  │n│ │r│ │e│ │
│  │    click/drag to set position       │  │e│ │i│ │x│ │
│  │    grains pulse when active         │  │r│ │g│ │t│ │
│  │    PCA → UMAP animated transition   │  │g│ │h│ │u│ │
│  │                                     │  │y│ │t│ │r│ │
│  │                                     │  ╰─╯ ╰─╯ ╰─╯ │
│  └─────────────────────────────────────┘                │
│                                                         │
│  [Position ●───────]  [Density ●──]  [Size ●──]  [Var] │
│  [Crossfade ●──────]  [Gain ●─────]         🌿 (fern)  │
└─────────────────────────────────────────────────────────┘
```
- Scatter plot dominates ~60% of interface area
- Three macro knobs (Energy, Brightness, Texture) vertically stacked to the right
- Secondary controls in a horizontal strip below
- Botanical fern overlay at low opacity in bottom-right corner, behind controls
- File drop zone / load button at top

## Target User
- Ambient/drone producers creating evolving soundscapes
- Game audio designers generating environmental textures
- Film/TV composers needing non-repeating atmospheric beds
- Sound designers exploring timbral space
- Lo-fi / experimental electronic producers

## Differentiators
- **First VST/AU with interactive grain scatter plot** -- no existing plugin offers this
- vs AudioTexture: visual scatter plot, three MIDI modes, UMAP-based grain arrangement
- vs CataRT/FluCoMa: works in any DAW (no Max/MSP), polished commercial UX
- vs Granulator II: visual feedback, descriptor-based selection (not random), timbral navigation
- Single-file simplicity -- no corpus management, just drop a file and sculpt

## Scope (v1 -- Engine + Scatter Plot)
**In scope:**
- Concatenative DSP engine (descriptor extraction, KD-tree search, grain scheduler)
- WebGL scatter plot with click/drag interaction and animated active grains
- PCA instant layout + UMAP background computation
- Three macro knobs + secondary parameters
- All three MIDI modes
- Drag-and-drop file loading
- Ouaricon Naturalist aesthetic

**Out of scope for v1 (future versions):**
- Freeze/capture feature
- Built-in reverb tail
- Path recorder (draw cursor paths for automated playback)
- LFO modulation of XY position
- Factory presets per source type
- Multiple file layering

## Complexity Assessment
**HIGH** -- This is a technically ambitious plugin combining:
1. Custom DSP descriptor extraction pipeline
2. KD-tree nearest-neighbor search (real-time safe)
3. Polyphonic grain scheduler with crossfading
4. UMAP dimensionality reduction (background thread)
5. WebGL scatter plot with real-time animation (regl-scatterplot)
6. Bidirectional C++ ↔ WebView communication at 30Hz
7. Three MIDI modes with pitch tracking

Mitigating factors: proven architecture patterns from O-GrainScatter, well-researched algorithm choices, header-only dependencies (nanoflann, umappp).

## Price Point
$79-129 (premium instrument category)

## Research Foundation
- `research/concatenative-synthesis-comprehensive.md`
- `research/concatenative-synthesis-market-research.md`
- `research/2d-scatter-plot-concatenative-synthesis.md`
- `research/flucoma-core-integration-research.md`
- `research/umap-dimensionality-reduction-audio-plugins.md`
