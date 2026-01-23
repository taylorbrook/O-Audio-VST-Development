# Requirements: O-Bass

**Defined:** 2026-01-22
**Core Value:** Make bass perceptually fuller without artifacts — enhancement that sounds natural and translates well.

## v1 Requirements

Requirements for initial release. Each maps to roadmap phases.

### DSP Core

- [ ] **DSP-01**: Plugin implements psychoacoustic harmonic generation using missing fundamental phenomenon
- [ ] **DSP-02**: Crossover filter isolates bass frequencies for processing (40-200Hz configurable)
- [ ] **DSP-03**: Bass processing occurs in mono to prevent phase cancellation
- [ ] **DSP-04**: Oversampling prevents aliasing artifacts (minimum 2x, 4x for Clean mode)
- [ ] **DSP-05**: Latency stays under 5ms for mix bus compatibility

### Enhancement Modes

- [ ] **MODE-01**: Clean mode provides transparent enhancement with high-quality oversampling
- [ ] **MODE-02**: Colored mode provides analog warmth and saturation character
- [ ] **MODE-03**: Mode switch toggles between Clean and Colored processing

### Controls

- [ ] **CTRL-01**: Frequency knob controls crossover point (40-200Hz range)
- [ ] **CTRL-02**: Enhance knob controls enhancement intensity with limiting to prevent over-processing
- [ ] **CTRL-03**: Output knob provides gain compensation
- [ ] **CTRL-04**: Mode toggle switches between Clean and Colored

### UI

- [ ] **UI-01**: WebView UI matches Ouaricon suite visual language
- [ ] **UI-02**: Interface displays 4 controls (Frequency, Enhance, Output, Mode)
- [ ] **UI-03**: Controls use existing WebSliderRelay/WebComboBoxRelay bridge patterns

### Formats & Integration

- [ ] **FMT-01**: Plugin builds as VST3 format
- [ ] **FMT-02**: Plugin builds as AU format
- [ ] **FMT-03**: Plugin builds as Standalone application
- [ ] **INT-01**: Preset system uses shared OuariconPresetManager module

## v2 Requirements

Deferred to future release. Tracked but not in current roadmap.

### Extended Features

- **EXT-01**: Dry/wet mix control for parallel processing
- **EXT-02**: Sidechain input for frequency-aware enhancement
- **EXT-03**: Spectrum visualization showing enhancement effect

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| Multi-band processing with user crossovers | Adds complexity without clear benefit for minimal UI goal |
| Mid/side processing | Keeping v1 focused on core enhancement |
| Spectrum analyzer visualization | Not needed for minimal "results over tweaking" approach |
| Linear-phase crossover option | Latency incompatible with mix bus use |
| Subharmonic synthesis | Different approach; psychoacoustic better for small speaker translation |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| DSP-01 | — | Pending |
| DSP-02 | — | Pending |
| DSP-03 | — | Pending |
| DSP-04 | — | Pending |
| DSP-05 | — | Pending |
| MODE-01 | — | Pending |
| MODE-02 | — | Pending |
| MODE-03 | — | Pending |
| CTRL-01 | — | Pending |
| CTRL-02 | — | Pending |
| CTRL-03 | — | Pending |
| CTRL-04 | — | Pending |
| UI-01 | — | Pending |
| UI-02 | — | Pending |
| UI-03 | — | Pending |
| FMT-01 | — | Pending |
| FMT-02 | — | Pending |
| FMT-03 | — | Pending |
| INT-01 | — | Pending |

**Coverage:**
- v1 requirements: 19 total
- Mapped to phases: 0
- Unmapped: 19 (pending roadmap)

---
*Requirements defined: 2026-01-22*
*Last updated: 2026-01-22 after initial definition*
