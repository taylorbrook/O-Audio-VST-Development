# Ouaricon Saturation Modeling - Creative Brief

## Vision

A sophisticated tape/analog saturation plugin that achieves the rare balance of extreme simplicity and genuine sonic depth. Four distinct physical modeling algorithms—each representing a fundamentally different approach to analog saturation—are accessible through a single intensity knob and model selection buttons. The plugin proves that "easy to use" and "high quality" are not mutually exclusive.

## Sonic Goals

**Core Effect**: Harmonic saturation through four physically-modeled analog processes, each with its own character and frequency response.

### The Four Models

#### 1. MAGNETIC (Jiles-Atherton Tape Hysteresis)
**Physical Basis**: Ferromagnetic hysteresis modeling based on the Jiles-Atherton equations, simulating actual magnetic tape recording behavior.

**Character**:
- Smooth, warm compression with subtle harmonic generation
- Natural "memory" effect from hysteresis—sound depends slightly on what came before
- Gentle high-frequency saturation that softens transients
- Classic tape "glue" that makes mixes cohesive

**Frequency Response**:
- Head bump around 80-100Hz (+2-3dB gentle boost)
- Subtle high-frequency rolloff above 12kHz (tape head loss)
- Mid-presence enhancement from transformer coloration

**Intensity Mapping**: Controls drive into the magnetic medium
- 0%: Subtle warmth, gentle hysteresis coloring
- 50%: Classic "pushed tape" sound, noticeable compression
- 100%: Heavy saturation, significant harmonic content, "tape crunch"

#### 2. TUBE (Triode Waveshaping via Koren Equations)
**Physical Basis**: Mathematical model of triode vacuum tube behavior, including plate current characteristics and grid voltage response.

**Character**:
- Rich even and odd harmonic generation
- Asymmetric clipping (different behavior for positive/negative swings)
- Musical compression that "breathes"
- Presence and clarity enhancement from harmonic structure

**Frequency Response**:
- Slight low-mid warmth from coupling capacitor simulation
- Upper-mid presence boost (~3kHz)
- Natural high-frequency air from harmonic series

**Intensity Mapping**: Controls tube bias point and drive
- 0%: Clean warmth, subtle second harmonic enhancement
- 50%: Classic "pushed" tube sound, noticeable coloration
- 100%: Heavy saturation, aggressive but musical clipping

#### 3. TRANSFORMER (Core Saturation + Resonance)
**Physical Basis**: Iron core transformer saturation modeling, including magnetizing inductance, core nonlinearity, and frequency-dependent behavior.

**Character**:
- Low-frequency thickening and "weight"
- Subtle resonant coloration at low-mid frequencies
- Soft limiting that controls peaks naturally
- "Console" character—glue and punch

**Frequency Response**:
- Low-frequency bump at 60-80Hz (transformer resonance)
- Gentle high-shelf boost around 8kHz ("sheen")
- Natural bandwidth limiting at frequency extremes

**Intensity Mapping**: Controls input level into transformer core
- 0%: Subtle coloration, frequency shaping only
- 50%: Noticeable saturation, enhanced low-end weight
- 100%: Heavy core saturation, significant harmonic distortion, punchy compression

#### 4. DIODE (Soft Clipping via Newton-Raphson)
**Physical Basis**: Accurate diode junction modeling using the Shockley equation, solved iteratively for precise soft-clipping behavior.

**Character**:
- Edgier, more aggressive saturation
- Symmetric or asymmetric clipping (modeled on classic circuits)
- Clear harmonic structure with defined "edge"
- Cuts through mixes while adding density

**Frequency Response**:
- Relatively flat frequency response (circuit-dependent)
- Slight presence boost from harmonic generation
- Can emphasize transients rather than smoothing them

**Intensity Mapping**: Controls gain into clipping circuit
- 0%: Subtle edge, gentle harmonic enhancement
- 50%: Clear clipping character, "driven" sound
- 100%: Heavy distortion, aggressive but controlled clipping

---

## Quality Tiers

Three CPU budget options accessible via secondary button group:

### LOW Quality
- No oversampling
- Lookup table approximations for expensive functions (tanh, exp)
- Simplified algorithms (e.g., polynomial approximation of Jiles-Atherton)
- Target: < 1% CPU single instance
- Use case: Live performance, low-powered systems, tracking

### MID Quality
- 2x oversampling with polyphase filters
- Standard algorithm implementations
- Newton-Raphson with 4 iterations
- Target: ~2-3% CPU single instance
- Use case: General mixing, default setting

### HIGH Quality
- 4x oversampling with high-quality anti-aliasing
- Full physical models with no approximations
- Newton-Raphson with 8 iterations for maximum accuracy
- ADAA (Antiderivative Anti-Aliasing) where applicable
- Target: ~5-8% CPU single instance
- Use case: Mastering, final renders, critical listening

---

## Control Scheme

### Primary Controls

**INTENSITY Knob** (Large, Central)
- Range: 0-100%
- Function: Model-specific saturation intensity (see mappings above)
- Behavior: Smooth, click-free parameter changes with appropriate smoothing time
- Visual: Large, prominent—the main interaction point

**MODEL Buttons** (4 buttons, exclusive selection)
- MAGNETIC | TUBE | TRANSFORMER | DIODE
- Behavior: Click to select, only one active at a time
- Visual: Clear active/inactive states, possibly with model-specific iconography

### Secondary Controls

**QUALITY Buttons** (3 buttons, exclusive selection)
- LOW | MID | HIGH
- Default: MID
- Behavior: Click to select CPU/quality tradeoff
- Note: Switching quality may cause brief audio glitch (acceptable)

**AUTO Button** (Toggle)
- Function: Auto-gain compensation
- When ON: Output level automatically adjusted to match input level
- Implementation: RMS-based level matching with slow time constant (~100ms)
- Visual: Clear on/off state

---

## UX Principles

**Aesthetic**: Continue Ouaricon botanical theme (consistent with Ouaricon Tremolo)
- Vintage paper texture background
- Botanical illustration elements
- Earthy, organic color palette
- Baskerville or similar period typeface
- Hand-crafted quality meets precision

**Layout Philosophy**:
- Intensity knob as clear focal point (large, central)
- Model selection buttons prominently displayed
- Quality and Auto controls in secondary position (smaller, less prominent)
- Minimal visual noise—every element serves a purpose

**Interaction Model**:
- Vertical drag for knob (standard plugin convention)
- Click for button toggles
- Hover states for discoverability
- Value display on knob interaction

**Visual Feedback**:
- Active model clearly indicated
- Quality level visible at a glance
- Auto-gain state obvious
- Optional: Subtle saturation visualization (waveform or meter)

---

## Technical Requirements

**Plugin Formats**: VST3, AU, Standalone
**UI Technology**: WebView (HTML/CSS/JS) for consistency with Ouaricon line
**Sample Rate Support**: 44.1kHz - 192kHz
**Processing**: Real-time, latency depends on quality setting
**Latency**:
- LOW: Zero latency
- MID: ~10-20 samples (2x oversampling filter)
- HIGH: ~20-40 samples (4x oversampling filter)
**Preset Management**: DAW-standard preset system

---

## Implementation Complexity Assessment

| Component | Complexity | Notes |
|-----------|------------|-------|
| Jiles-Atherton (Magnetic) | High | Iterative hysteresis calculation, state management |
| Triode Model (Tube) | Medium-High | Koren equations, Newton-Raphson optional |
| Transformer Saturation | Medium | Core saturation + biquad filters |
| Diode Clipper | Medium | Newton-Raphson solver, 4-8 iterations |
| Oversampling System | Medium | Polyphase filters, buffer management |
| Auto-Gain | Low | RMS envelope follower |
| WebView UI | Medium | Parameter binding, visual updates |

**Overall Assessment**: Medium-High complexity
- Multiple distinct DSP algorithms
- Quality tier switching requires careful state management
- Oversampling adds latency reporting complexity
- But: Simple parameter count, straightforward UI

---

## Success Criteria

1. **Sonic Distinction**: Each of the four models sounds genuinely different—not just "more" or "less" of the same thing
2. **Intensity Consistency**: The knob feels consistent across models—0% is always subtle, 100% is always heavy
3. **Quality Transparency**: LOW quality should sound good enough for most uses; HIGH should be audibly superior on critical material
4. **CPU Honesty**: Quality settings deliver on their CPU promises
5. **Auto-Gain Accuracy**: Auto mode maintains perceived loudness across intensity range
6. **Visual Coherence**: Fits seamlessly into Ouaricon product line aesthetically
7. **Professional Workflow**: Works reliably in all major DAWs, no crashes, proper preset handling

---

## Research References

Physical modeling approaches documented in:
- `research/circuit-modeling-fundamentals.md` - Diode, tube, transformer models
- `troubleshooting/dsp-issues/physical-modelling-synthesis-complete-guide.md` - JUCE implementation patterns
- `research-agent-3-physical-modelling-optimization.md` - Performance optimization strategies

Key algorithms:
- Jiles-Atherton: Section 5.1 of circuit-modeling-fundamentals.md
- Triode/Koren: Section 4.3 of circuit-modeling-fundamentals.md
- Transformer: Section 6 of circuit-modeling-fundamentals.md
- Diode Newton-Raphson: Section 4.1 of circuit-modeling-fundamentals.md
- Oversampling: Section 2.1 of physical-modelling-synthesis-complete-guide.md
