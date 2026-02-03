# O-Bells Realism Overhaul - Requirements Context

**Milestone:** realism-overhaul
**Created:** 2026-02-02
**Version:** 1.1.1 → 1.2.0

## Overview

A comprehensive DSP and parameter overhaul to improve the physical realism of O-Bells' modal synthesis engine. Focuses on decay behavior, material authenticity, new expression parameters, and enhanced stereo imaging.

---

## 1. Decay Envelope Improvements

### Current State
- Three decay modes: Linear, Exponential, Multi-stage
- Multi-stage has Strike/Body/Hum phases with frequency-dependent damping
- Brilliance parameter controls high-frequency sustain

### Requirements
- **Remove Linear and Exponential modes** - Multi-stage covers all realistic scenarios
- **Make Brilliance more meaningful** - Currently affects b_3 coefficient; should have more audible impact
- **Parameter changes:**
  - Remove `decayShape` dropdown (hardcode to multi-stage behavior)
  - Brilliance becomes primary tonal control for decay character

### Acceptance Criteria
- [ ] decayShape parameter removed from APVTS
- [ ] All voices use multi-stage envelope by default
- [ ] Brilliance has clearly audible effect across its range
- [ ] No regression in existing multi-stage behavior

---

## 2. Bloom Parameter (NEW)

### Definition
**Spectral Swelling** - Partials gradually increase in amplitude after initial strike, then decay. Mimics the way real bells build resonance before the decay phase takes over.

### Behavior
- At 0%: Traditional instant-peak decay (current behavior)
- At 50%: Moderate swell over ~100-200ms before decay onset
- At 100%: Pronounced resonance building, partials bloom to 1.5-2x initial amplitude

### Technical Approach
- Envelope modifier per partial: attack phase before decay
- Higher partials may bloom faster than lower (frequency-dependent)
- Interacts with Strike Time in multi-stage envelope

### Parameter Spec
- **ID:** `bloom`
- **Label:** "Bloom"
- **Range:** 0.0 - 1.0 (0-100%)
- **Default:** 0.0
- **UI Location:** Main Panel, existing layout position

---

## 3. Shimmer Parameter (NEW)

### Definition
**Frequency Drift/Beating** - Subtle pitch movement and beating between partials that increases over decay time. Creates the characteristic "alive" quality of struck metal.

### Behavior
- At 0%: Stable, perfectly tuned partials (synthetic)
- At 50%: Subtle beating between partials, organic feel
- At 100%: Pronounced drift, almost chorus-like shimmer

### Technical Approach
- Add slight LFO modulation to partial frequencies
- Different rates per partial to create complex beating patterns
- Amount increases during decay (more shimmer as note fades)

### Parameter Spec
- **ID:** `shimmer`
- **Label:** "Shimmer"
- **Range:** 0.0 - 1.0 (0-100%)
- **Default:** 0.2
- **UI Location:** Main Panel, existing layout position

---

## 4. Mallet Enhancement (Temporal Spreading)

### Current State
- Mallet Hardness affects transient brightness and partial amplitudes
- Strike noise character varies with hardness

### Enhancement
- **Low mallet values (soft):** Add temporal spreading to attack
- Notes should "blossom" rather than strike sharply
- Soft mallet = longer attack envelope, gradual partial onset

### Technical Approach
- Add attack time scaling based on inverse of malletHardness
- At hardness=0: ~50ms attack ramp
- At hardness=1: Instant attack (current behavior)

### Acceptance Criteria
- [ ] Soft mallet settings produce gentle attack
- [ ] No change to hard mallet behavior
- [ ] Transient noise envelope also affected

---

## 5. Material Parameter Rework

### Current State
- Continuous morph: Bronze → Steel → Glass → Crystal
- Uses arbitrary decay multipliers (0.6, 0.8, 1.0, 1.2)

### Requirements
- Research actual acoustic properties of metals
- Implement 4-5 specific metals with measured characteristics
- Parameters to derive from research:
  - Decay time multiplier (internal loss)
  - Spectral character (partial emphasis)
  - Inharmonicity coefficient (material stiffness affects partial spread)

### Target Materials
1. **Bronze** (traditional bell metal, tin bronze)
2. **Brass** (copper-zinc, brighter than bronze)
3. **Steel** (carbon steel, sustained)
4. **Aluminum** (bright, fast decay)
5. **Cast Iron** (dark, damped)

### Parameter Spec
- **ID:** `material` (unchanged)
- **Label:** "Material" (unchanged)
- **Range:** 0.0 - 1.0, mapping to discrete material points
- **UI:** Consider showing material name tooltip on hover

---

## 6. Inharmonicity Rename

### Change
- **Old label:** "Inharm"
- **New label:** "Inharmonicity"

### Implementation
- Parameter ID stays `inharmonicity` for preset compatibility
- Only UI label changes
- Full word for clarity

---

## 7. Enhanced Stereo Imaging

### Current State
- Simple L/R balance via `stereoSpread` parameter
- Unison voices panned based on detune offset

### Requirements
Three-part enhancement:

#### 7a. True Stereo Panning
- Proper stereo field placement (not just balance)
- Individual partials can have stereo position
- Low partials center, high partials spread

#### 7b. Spatial Movement
- Partials drift in stereo field over time
- Creates organic motion during decay
- Amount controlled by existing stereoSpread or new parameter

#### 7c. Haas Effect / Width
- Subtle timing differences between L/R channels
- Creates perceived width without pure panning
- Optional: frequency-dependent delay

### Parameter Considerations
- May need new "Width" or "Motion" parameter
- Or enhance existing stereoSpread to be multi-function

---

## 8. Placeholder Scan

### Requirement
Scan all source files for:
- TODO comments
- Placeholder values (magic numbers without explanation)
- Stub implementations
- Incomplete features
- Hardcoded values that should be parameters

### Deliverable
List of findings in Research phase, fixes in Execute phase.

---

## 9. Preset Rework

### Requirements
After all parameter changes:
- Update all 25 factory presets to use new parameters
- Ensure presets showcase new features (Bloom, Shimmer, enhanced stereo)
- Re-test each preset for balance and musical utility
- Add any new showcase presets if warranted

### Preset Categories (existing)
1. Orchestral (5 presets)
2. Sacred (5 presets)
3. World (5 presets)
4. Ambient (5 presets)
5. Cinematic (5 presets)

### Update Approach
- Preserve character of existing presets
- Add subtle Bloom/Shimmer where appropriate
- Ensure Material values map correctly to new research-based system

---

## Summary of Changes

| Category | Change Type | Impact |
|----------|------------|--------|
| Decay Envelope | Remove modes | Breaking (presets using Linear/Exp) |
| Bloom | New parameter | Additive |
| Shimmer | New parameter | Additive |
| Mallet | Enhanced behavior | Non-breaking |
| Material | Research-based values | Potentially breaking |
| Inharmonicity | Label rename | Non-breaking |
| Stereo Imaging | Enhanced behavior | Non-breaking |
| Presets | Full rework | Required |

### Parameter Count Change
- **Before:** 20 parameters
- **After:** 21 parameters (add Bloom, Shimmer; remove decayShape = +2 -1 = +1)

### Version Bump
- **Type:** MINOR (new features, preset changes)
- **From:** 1.1.1
- **To:** 1.2.0

---

## Questions Resolved

| Question | Answer |
|----------|--------|
| Bloom definition | Spectral swelling (partial amplitude building) |
| Shimmer definition | Frequency drift/beating between partials |
| Temporal spreading | Integrate into Mallet parameter (soft = slow attack) |
| Material depth | 4-5 specific metals with researched properties |
| Decay modes | Remove Linear and Exponential |
| Stereo enhancement | Full: true panning + movement + Haas effect |
| Inharmonicity label | Full word "Inharmonicity" |
| New param UI location | Existing layout positions |

---

*Discuss Phase Complete - Ready for Research Phase*
