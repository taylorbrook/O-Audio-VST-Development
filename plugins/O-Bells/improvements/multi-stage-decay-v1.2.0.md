# Multi-Stage Decay Implementation Plan

**Plugin:** O-Bells
**Version:** 1.1.1 → 1.2.0 (MINOR)
**Date:** 2026-02-02

---

## Summary

Implement a physically-accurate multi-stage decay envelope for bells, replacing the current placeholder that just uses exponential decay. Users will have full control over three decay stages with 4 new parameters in a dedicated UI section.

---

## Design Decisions (User-Confirmed)

| Decision | Choice |
|----------|--------|
| Design Goal | Realistic Bell Physics |
| Controllability | Full Control (4 parameters) |
| Damping Interaction | Affects Hum stage only |
| UI Placement | New 'Envelope' section |
| Parameter Visibility | Multi-stage decay shape only |
| Version Bump | MINOR (1.2.0) |

---

## New Parameters

### Parameter Definitions (Research-Informed)

| Parameter ID | Display Name | Range | Default | Unit | Industry Precedent |
|--------------|--------------|-------|---------|------|-------------------|
| `strikeTime` | Strike Time | 5-100 | 30 | ms | Collision "Attack", Pianoteq "Attack" |
| `brilliance` | Brilliance | 0-100 | 50 | % | Pigments "Brilliance", Pianoteq "Cutoff" |
| `bodyTime` | Body Time | 100-5000 | 1500 | ms | Chromaphone "Decay" |
| `humSustain` | Hum Sustain | 0-100 | 50 | % | Pianoteq "Impedance" |

### Parameter Behavior

**Strike Time (5-100ms)**
- Controls how long the initial bright, metallic transient lasts
- During this phase, high-frequency partials (index 5+) decay rapidly
- Shorter = snappier attack, longer = more "bloom"
- *Research basis: Sound On Sound confirms strike phase is 0-50ms*

**Brilliance (0-100%)** *(renamed from "Strike Decay")*
- Controls how much longer high-frequency partials sustain relative to low
- 0% = high partials decay much faster (warm, woody sound)
- 100% = high partials sustain equally (bright, glassy sound)
- *Research basis: Pigments uses "Brilliance" for same concept; based on `R_k = b₁ + b₃×f_k²` formula*

**Body Time (100-5000ms)**
- Duration of the main tonal decay phase
- Mid-frequency partials (index 2-4) decay primarily here
- The musical note is most audible during this stage
- *Research basis: Strike Note Phase confirmed at 50ms-3s by Fletcher & Rossing*

**Hum Sustain (0-100%)**
- Extends the lowest partials (index 0-1) beyond normal decay
- 0% = hum decays with body
- 100% = hum sustains 3x longer than body
- Interacts with existing Damping parameter (Damping affects hum stage)
- *Research basis: Hum Tone Phase can last 5-60+ seconds in real bells*

---

## Technical Implementation

### 1. APVTS Parameter Registration

Add to `PluginProcessor.cpp` `createParameterLayout()`:

```cpp
// Multi-stage Envelope Parameters (only active when decayShape == 2)
// Parameter IDs match industry naming conventions from research

params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "strikeTime", "Strike Time",
    juce::NormalisableRange<float>(5.0f, 100.0f, 0.1f), 30.0f,
    juce::AudioParameterFloatAttributes().withLabel("ms")));

params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "brilliance", "Brilliance",
    juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
    juce::AudioParameterFloatAttributes().withLabel("%")));

params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "bodyTime", "Body Time",
    juce::NormalisableRange<float>(100.0f, 5000.0f, 1.0f), 1500.0f,
    juce::AudioParameterFloatAttributes().withLabel("ms")));

params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "humSustain", "Hum Sustain",
    juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
    juce::AudioParameterFloatAttributes().withLabel("%")));
```

### 2. BellVoice Multi-Stage Algorithm (Research-Based)

Replace the placeholder in `BellVoice.cpp` using formulas from academic research:

```cpp
// === BellVoice.h additions ===

// Multi-stage envelope state machine (JUCE best practice)
enum class EnvelopeStage { Strike, Body, Hum };

// New member variables
float currentStrikeTime = 30.0f;      // ms
float currentBrilliance = 50.0f;      // % (was "Stage1Decay")
float currentBodyTime = 1500.0f;      // ms
float currentHumSustain = 50.0f;      // %
int samplesSinceNoteOn = 0;           // Track time since note start

// Pre-calculated coefficients (computed once in startNote, not per-sample)
float strikeDecayCoeffs[NUM_PARTIALS];  // Per-partial strike phase coefficients
float bodyDecayCoeffs[NUM_PARTIALS];    // Per-partial body phase coefficients
float humDecayCoeffs[NUM_PARTIALS];     // Per-partial hum phase coefficients

// === BellVoice.cpp - Coefficient calculation in startNote() ===

void BellVoice::calculateMultiStageCoefficients(float fundamental)
{
    // Based on academic formula: R_k = b_1 + b_3 * f_k^2
    // Brilliance controls b_3 (frequency-dependent damping)

    float b1 = 0.5f;  // Base damping (frequency-independent)
    // Brilliance 0% = b3 high (high freqs decay fast = warm)
    // Brilliance 100% = b3 low (high freqs sustain = bright)
    float b3 = (100.0f - currentBrilliance) / 100.0f * 2e-8f;

    float strikeTimeSec = currentStrikeTime / 1000.0f;
    float bodyTimeSec = currentBodyTime / 1000.0f;
    float humExtension = 1.0f + (currentHumSustain / 100.0f) * 2.0f;

    for (int p = 0; p < NUM_PARTIALS; ++p)
    {
        float freq = calculatePartialFrequency(p, fundamental, currentInharmonicity);

        // Frequency-dependent loss factor (academic formula)
        float R_k = b1 + b3 * freq * freq;
        float baseDecayTime = 1.0f / R_k;

        // Strike phase: fast decay for high partials (50ms target)
        float strikeDecayTime = strikeTimeSec * (p < 2 ? 2.0f : (p < 5 ? 1.0f : 0.3f));
        strikeDecayCoeffs[p] = std::exp(-1.0f / (strikeDecayTime * currentSampleRate));

        // Body phase: frequency-dependent decay (Risset duration ratios)
        float bodyDecayTime = bodyTimeSec * DECAY_MULTIPLIERS[p];
        bodyDecayCoeffs[p] = std::exp(-1.0f / (bodyDecayTime * currentSampleRate));

        // Hum phase: extended low partials, damping affects here only
        float humDecayTime = baseDecayTime * humExtension;
        if (p < 2) {
            // Apply damping only to hum partials (user requirement)
            float dampingFactor = 1.0f - (currentDamping * 0.8f);
            humDecayTime *= dampingFactor;
        }
        humDecayCoeffs[p] = std::exp(-1.0f / (humDecayTime * currentSampleRate));
    }
}

// === BellVoice.cpp - Per-sample envelope (state machine approach) ===

void BellVoice::applyMultiStageDecay(ModalPartial& partial, int partialIndex)
{
    float timeSeconds = samplesSinceNoteOn / currentSampleRate;
    float strikeEndTime = currentStrikeTime / 1000.0f;
    float bodyEndTime = strikeEndTime + (currentBodyTime / 1000.0f);

    // State machine (JUCE best practice from research)
    if (timeSeconds < strikeEndTime) {
        // STRIKE PHASE: High partials decay fast, low partials hold
        partial.amplitude *= strikeDecayCoeffs[partialIndex];
    }
    else if (timeSeconds < bodyEndTime) {
        // BODY PHASE: Main tonal decay, mid partials prominent
        partial.amplitude *= bodyDecayCoeffs[partialIndex];
    }
    else {
        // HUM PHASE: Only low partials remain, affected by Damping
        partial.amplitude *= humDecayCoeffs[partialIndex];
    }
}
```

### 3. Parameter Flow

```
PluginProcessor.processBlock()
    │
    ├─► Read APVTS values (stage1Time, stage1Decay, stage2Time, humSustain)
    │
    └─► bellVoice.updateParameters(..., stage1Time, stage1Decay, stage2Time, humSustain)
            │
            └─► BellVoice::renderNextBlock()
                    │
                    └─► if (decayShape == 2) applyMultiStageDecay()
```

### 4. WebView UI Changes

Add new "Envelope" section to `Resources/ui/index.html`:

```html
<!-- Envelope Section (visible only when Multi-stage selected) -->
<div id="envelope-section" class="section hidden">
    <h3>Multi-Stage Envelope</h3>
    <p class="section-hint">Controls how different frequencies decay over time</p>
    <div class="param-grid">
        <div class="param" title="Duration of the bright, metallic transient (0-50ms typical)">
            <label>Strike Time</label>
            <input type="range" id="strikeTime" min="5" max="100" value="30" step="1">
            <span class="value">30 ms</span>
        </div>
        <div class="param" title="High-frequency sustain. Low = warm/woody, High = bright/glassy">
            <label>Brilliance</label>
            <input type="range" id="brilliance" min="0" max="100" value="50" step="1">
            <span class="value">50%</span>
        </div>
        <div class="param" title="Duration of the main tonal decay phase">
            <label>Body Time</label>
            <input type="range" id="bodyTime" min="100" max="5000" value="1500" step="10">
            <span class="value">1.5 s</span>
        </div>
        <div class="param" title="How long the low hum tone sustains (affected by Damping)">
            <label>Hum Sustain</label>
            <input type="range" id="humSustain" min="0" max="100" value="50" step="1">
            <span class="value">50%</span>
        </div>
    </div>
</div>
```

JavaScript to show/hide based on decay shape:
```javascript
// Called when decayShape parameter changes
function updateEnvelopeVisibility() {
    const decayShape = parseInt(document.getElementById('decayShape').value);
    const envelopeSection = document.getElementById('envelope-section');

    // 0 = Linear, 1 = Exponential, 2 = Multi-stage
    const isMultiStage = (decayShape === 2);
    envelopeSection.classList.toggle('hidden', !isMultiStage);

    // Disable/enable the inputs to prevent accidental changes
    const inputs = envelopeSection.querySelectorAll('input');
    inputs.forEach(input => input.disabled = !isMultiStage);
}

// Register change listener for decayShape
document.getElementById('decayShape').addEventListener('change', updateEnvelopeVisibility);
window.addEventListener('load', updateEnvelopeVisibility);
```

### 5. Relay/Attachment Setup

Add to `PluginEditor.h`:
```cpp
// Multi-stage envelope relays (WebView binding)
ouaricon::WebSliderRelay strikeTimeRelay, brillianceRelay, bodyTimeRelay, humSustainRelay;

// APVTS attachments
std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
    strikeTimeAttachment, brillianceAttachment, bodyTimeAttachment, humSustainAttachment;
```

Add to `PluginEditor.cpp` constructor:
```cpp
// Initialize relays with parameter IDs matching APVTS
strikeTimeRelay.setParameterId("strikeTime");
brillianceRelay.setParameterId("brilliance");
bodyTimeRelay.setParameterId("bodyTime");
humSustainRelay.setParameterId("humSustain");

// Create attachments
strikeTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
    audioProcessor.apvts, "strikeTime", strikeTimeRelay);
// ... repeat for other parameters
```

---

## Files to Modify

| File | Changes |
|------|---------|
| `PluginProcessor.h` | Add atomic parameters for multi-stage |
| `PluginProcessor.cpp` | Add 4 params to APVTS, pass to voice |
| `BellVoice.h` | Add stage params, samplesSinceNoteOn |
| `BellVoice.cpp` | Implement `applyMultiStageDecay()` |
| `PluginEditor.h` | Add 4 relays and attachments |
| `PluginEditor.cpp` | Initialize relays, create attachments |
| `Resources/ui/index.html` | Add Envelope section, show/hide logic |

---

## Validation Plan

1. **Build:** VST3 + AU with ninja
2. **pluginval:** Strictness level 5
3. **auval:** Verify AU registration
4. **DAW Testing:**
   - Test all 3 decay shapes still work
   - Verify Multi-stage parameters only affect Multi-stage mode
   - Confirm Damping only affects hum in Multi-stage
   - Check preset compatibility (existing presets should load)
5. **Audio Quality:**
   - Strike transient sounds metallic/bright
   - Body has clear musical tone
   - Hum sustains when Hum Sustain is high
   - No clicks/pops at stage transitions

---

## Preset Updates

Update factory presets to use new Multi-stage where appropriate:

| Preset | Strike Time | Brilliance | Body Time | Hum Sustain | Rationale |
|--------|-------------|------------|-----------|-------------|-----------|
| Tubular Bells | 25ms | 60% | 2000ms | 40% | Bright attack, moderate sustain |
| Church Bell | 40ms | 45% | 3000ms | 80% | Warm, long hum (real church bells) |
| Meditation Bowl | 15ms | 70% | 4000ms | 100% | Crystal-like, very long sustain |
| Glockenspiel | 10ms | 85% | 800ms | 20% | Bright, short, pitched percussion |
| Temple Gong | 50ms | 30% | 3500ms | 90% | Dark, massive, long decay |
| Steel Pan | 20ms | 75% | 1200ms | 30% | Bright metallic, moderate decay |

## Research Sources

This implementation is informed by comprehensive research across three domains:

### Academic/DSP Literature
- CCRMA Stanford - Julius Smith's Physical Audio Signal Processing
- IRCAM Modalys - Modal synthesis system documentation
- Chaigne & Doutaut (1997-1998) - "Numerical simulations of xylophones" (JASA)
- Fletcher & Rossing - "The Physics of Musical Instruments" (bell partials)
- Nathan Ho - "Exploring Modal Synthesis" (frequency-dependent damping formula)
- DAFx 2017 - Carillon modal analysis paper

### Professional Synth Implementations
- Ableton Collision - Material slider for freq-dependent decay
- AAS Chromaphone 3 - Material parameter, mode density
- Arturia Pigments 6 - Brilliance control for high-freq decay
- Modartt Pianoteq - Impedance/Cutoff/Slope system

### JUCE Best Practices
- Nigel Redmon (EarLevel Engineering) - Iterative exponential envelope formula
- Shane Dunne - juce-MultiStepEnvelopeGenerator
- JUCE Forum - Modal synthesis performance optimization

Full research documents saved to:
- `research/modal-synthesis-bells-academic-research.md`
- `research/multi-stage-decay-envelopes-comparison.md`

---

## Rollback Plan

Backup location: `backups/O-Bells/v1.1.1/`

If issues occur:
1. Restore from backup
2. Rebuild with `ninja O-Bells_VST3 O-Bells_AU`
3. Reinstall to system folders

---

## Approval Checklist

- [ ] Plan reviewed and understood
- [ ] Parameter names/ranges acceptable
- [ ] UI placement confirmed
- [ ] Ready to proceed with implementation
