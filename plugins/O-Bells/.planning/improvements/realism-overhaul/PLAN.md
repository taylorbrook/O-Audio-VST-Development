# O-Bells Realism Overhaul - Implementation Plan

---
milestone: realism-overhaul
domain: dsp
execute_agent: dsp-agent
version_bump: minor
base_version: 1.1.1
target_version: 1.2.0
created: 2026-02-02
---

## Overview

This plan implements the realism overhaul for O-Bells based on requirements from CONTEXT.md and research findings from RESEARCH.md. The milestone adds two new parameters (Bloom, Shimmer), removes one (decayShape), implements research-based material properties, and enhances stereo imaging.

**Parameter Count:** 20 → 21 (+2 new, -1 removed)

---

## Task Breakdown

### Phase 1: Decay Shape Simplification (Foundation)

**Goal:** Remove Linear and Exponential decay modes, hardcode multi-stage behavior.

#### Task 1.1: Remove decayShape Parameter from APVTS
- **File:** `Source/PluginProcessor.cpp`
- **Action:** Remove `decayShape` parameter definition from `createParameterLayout()`
- **Lines:** ~151-156
- **Verification:** Build succeeds, parameter not in tree

#### Task 1.2: Remove decayShape Cached Pointer
- **File:** `Source/PluginProcessor.h`
- **Action:** Remove `std::atomic<float>* decayShapeParam`
- **Line:** 89
- **File:** `Source/PluginProcessor.cpp`
- **Action:** Remove pointer assignment in constructor
- **Verification:** No references to decayShapeParam remain

#### Task 1.3: Remove decayShape from processBlock
- **File:** `Source/PluginProcessor.cpp`
- **Action:** Remove `decayShape` read and updateParameters call argument
- **Line:** ~357
- **Verification:** processBlock compiles without decayShape

#### Task 1.4: Update BellVoice::updateParameters Signature
- **File:** `Source/BellVoice.h`
- **Action:** Remove `int decayShape` parameter from signature (line ~38)
- **File:** `Source/BellVoice.cpp`
- **Action:** Remove `decayShape` from implementation signature
- **Action:** Remove `currentDecayShape = decayShape;` assignment
- **Verification:** updateParameters signature matches between .h and .cpp

#### Task 1.5: Hardcode Multi-Stage Decay in BellVoice
- **File:** `Source/BellVoice.cpp`
- **Action:** Remove conditional `if (currentDecayShape == 2)` checks
- **Action:** Always call `applyMultiStageDecay()` in renderNextBlock
- **Action:** Always call `calculateMultiStageCoefficients()` in startNote
- **Lines:** ~263-268, 306-312, 349-355, 729-754
- **Verification:** All voices use multi-stage decay

#### Task 1.6: Remove currentDecayShape Member Variable
- **File:** `Source/BellVoice.h`
- **Action:** Remove `int currentDecayShape` member (line ~120)
- **Verification:** No compiler errors about undeclared variable

#### Task 1.7: Remove decayShape UI Bindings
- **File:** `Source/PluginEditor.h`
- **Action:** Remove `decayShapeRelay` and `decayShapeAttachment` declarations
- **File:** `Source/PluginEditor.cpp`
- **Action:** Remove relay/attachment instantiation and options chain
- **Verification:** Editor compiles without decayShape references

#### Task 1.8: Update Factory Presets for Multi-Stage
- **File:** Factory preset JSON files in `Resources/Presets/Factory/`
- **Action:** Update presets using `decayShape: 0` or `decayShape: 1`:
  - `World/Gamelan Saron.json` - Re-voice with multi-stage settings
  - `World/Gamelan Bonang.json` - Re-voice with multi-stage settings
  - `Cinematic/Horror Stinger.json` - Re-voice with multi-stage settings
- **Verification:** All presets load without errors

**Phase 1 Dependencies:** None (foundation phase)

---

### Phase 2: Material Research Integration

**Goal:** Replace placeholder material values with research-based acoustic properties.

#### Task 2.1: Define MaterialProperties Struct
- **File:** `Source/BellVoice.h`
- **Action:** Add new struct for material properties:
  ```cpp
  struct MaterialProperties {
      float decayMultiplier;    // Relative to bronze baseline
      float brightnessOffset;   // High-frequency emphasis (-1 to +1)
      float inharmonicity;      // Partial spread factor
  };
  ```
- **Line:** After line 59 (existing material constants)
- **Verification:** Struct compiles

#### Task 2.2: Define Research-Based Material Constants
- **File:** `Source/BellVoice.h`
- **Action:** Replace MATERIAL_DECAY_* constants with MATERIALS array:
  ```cpp
  static constexpr MaterialProperties MATERIALS[5] = {
      { 1.0f, 0.0f, 0.5f },     // Bronze (baseline)
      { 0.75f, 0.15f, 0.35f },  // Brass (brighter, shorter)
      { 1.5f, 0.1f, 0.25f },    // Steel (sustained, bright)
      { 0.55f, 0.25f, 0.2f },   // Aluminum (fast, very bright)
      { 0.35f, -0.3f, 0.65f }   // Cast Iron (damped, dark)
  };
  ```
- **Action:** Remove old MATERIAL_DECAY_BRONZE/STEEL/GLASS/CRYSTAL constants
- **Verification:** Constants compile

#### Task 2.3: Update calculateMaterialDecayMultiplier
- **File:** `Source/BellVoice.cpp`
- **Action:** Rewrite to interpolate between 5 discrete materials:
  - Input: material parameter 0.0-1.0
  - Map to material index (0.0=Bronze, 0.25=Brass, 0.5=Steel, 0.75=Aluminum, 1.0=Cast Iron)
  - Apply decayMultiplier from MATERIALS array
- **Verification:** Material parameter produces expected decay characteristics

#### Task 2.4: Apply Material Brightness Offset
- **File:** `Source/BellVoice.cpp`
- **Action:** In `initializePartials()`, apply material's brightnessOffset to partial amplitudes
- **Verification:** Different materials have distinct tonal character

#### Task 2.5: Apply Material Inharmonicity
- **File:** `Source/BellVoice.cpp`
- **Action:** In `calculatePartialFrequency()`, factor in material's inharmonicity value
- **Verification:** Materials affect partial spread as expected

#### Task 2.6: Update Presets for New Material System
- **File:** Factory preset JSON files
- **Action:** Audit and adjust material values for all 25 presets
- **Verification:** Presets sound appropriate with new material mapping

**Phase 2 Dependencies:** Phase 1 (clean foundation)

---

### Phase 3: Bloom Parameter

**Goal:** Add spectral swelling effect where partials build amplitude before decay.

#### Task 3.1: Add Bloom Parameter to APVTS
- **File:** `Source/PluginProcessor.cpp`
- **Action:** Add to `createParameterLayout()`:
  ```cpp
  std::make_unique<juce::AudioParameterFloat>(
      "bloom", "Bloom",
      juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 0.0f,
      juce::AudioParameterFloatAttributes().withLabel("%"))
  ```
- **File:** `Source/PluginProcessor.h`
- **Action:** Add `std::atomic<float>* bloomParam`
- **Verification:** Parameter appears in APVTS

#### Task 3.2: Add Bloom State to ModalPartial Struct
- **File:** `Source/BellVoice.h`
- **Action:** Add to ModalPartial struct:
  ```cpp
  float bloomPhase = 0.0f;
  float bloomRate = 0.0f;
  float initialAmplitude = 0.0f;
  float peakAmplitude = 0.0f;
  ```
- **Verification:** Struct compiles

#### Task 3.3: Add currentBloom Member Variable
- **File:** `Source/BellVoice.h`
- **Action:** Add `float currentBloom = 0.0f;` to current parameters section
- **Verification:** Variable declared

#### Task 3.4: Update BellVoice::updateParameters for Bloom
- **File:** `Source/BellVoice.h` and `Source/BellVoice.cpp`
- **Action:** Add `float bloom` parameter to signature and implementation
- **Action:** Assign `currentBloom = bloom;`
- **Verification:** Bloom value propagates from processor

#### Task 3.5: Implement initializeBloom() Function
- **File:** `Source/BellVoice.cpp`
- **Action:** Add helper function per RESEARCH.md design:
  - Calculate bloom time (50-200ms based on parameter)
  - Store peak amplitude
  - Set initial amplitude (30-80% of peak based on bloom)
  - Calculate per-partial bloom rates (higher partials faster)
- **Verification:** Function compiles

#### Task 3.6: Implement applyBloom() Function
- **File:** `Source/BellVoice.cpp`
- **Action:** Add per-sample bloom processing:
  - Advance bloom phase
  - Cosine interpolation to peak amplitude
  - Skip if bloom already complete
- **Verification:** Function compiles

#### Task 3.7: Integrate Bloom into startNote and renderNextBlock
- **File:** `Source/BellVoice.cpp`
- **Action:** In `startNote()`: Call `initializeBloom(currentBloom * 0.01f)` after `initializePartials()`
- **Action:** In `renderNextBlock()`: Call `applyBloom(partial)` before decay processing
- **Verification:** Bloom effect audible at high values

#### Task 3.8: Add Bloom to Processor processBlock
- **File:** `Source/PluginProcessor.cpp`
- **Action:** Read bloomParam and pass to updateParameters
- **Verification:** Bloom parameter affects sound

#### Task 3.9: Add Bloom UI Binding
- **File:** `Source/PluginEditor.h`
- **Action:** Add `bloomRelay` and `bloomAttachment` declarations
- **File:** `Source/PluginEditor.cpp`
- **Action:** Instantiate relay/attachment, add to options chain
- **File:** `Resources/ui/index.html`
- **Action:** Add Bloom slider to Main Panel
- **Verification:** UI control changes bloom value

**Phase 3 Dependencies:** Phase 1

---

### Phase 4: Shimmer Parameter

**Goal:** Add frequency drift/beating effect that increases during decay.

#### Task 4.1: Add Shimmer Parameter to APVTS
- **File:** `Source/PluginProcessor.cpp`
- **Action:** Add to `createParameterLayout()`:
  ```cpp
  std::make_unique<juce::AudioParameterFloat>(
      "shimmer", "Shimmer",
      juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 20.0f,
      juce::AudioParameterFloatAttributes().withLabel("%"))
  ```
- **File:** `Source/PluginProcessor.h`
- **Action:** Add `std::atomic<float>* shimmerParam`
- **Verification:** Parameter appears in APVTS

#### Task 4.2: Add Shimmer State to ModalPartial Struct
- **File:** `Source/BellVoice.h`
- **Action:** Add to ModalPartial struct:
  ```cpp
  float shimmerLFOPhase = 0.0f;
  float shimmerLFORate = 0.0f;
  float shimmerDepth = 0.0f;
  ```
- **Verification:** Struct compiles

#### Task 4.3: Add currentShimmer Member Variable
- **File:** `Source/BellVoice.h`
- **Action:** Add `float currentShimmer = 0.2f;` to current parameters section
- **Verification:** Variable declared

#### Task 4.4: Update BellVoice::updateParameters for Shimmer
- **File:** `Source/BellVoice.h` and `Source/BellVoice.cpp`
- **Action:** Add `float shimmer` parameter to signature and implementation
- **Action:** Assign `currentShimmer = shimmer;`
- **Verification:** Shimmer value propagates from processor

#### Task 4.5: Implement initializeShimmer() Function
- **File:** `Source/BellVoice.cpp`
- **Action:** Add helper function per RESEARCH.md design:
  - Calculate max frequency deviation (0.1-5 cents)
  - Assign unique LFO rates per partial (prime ratios)
  - Random starting phases
  - Higher partials get more shimmer
- **Verification:** Function compiles

#### Task 4.6: Implement applyShimmer() Function
- **File:** `Source/BellVoice.cpp`
- **Action:** Add per-sample shimmer processing:
  - Advance LFO phase
  - Triangle LFO for smooth modulation
  - Calculate frequency ratio from cents deviation
  - Return modified phase increment
- **Verification:** Function compiles

#### Task 4.7: Add Decay Progress Tracking
- **File:** `Source/BellVoice.h`
- **Action:** Add `float decayProgress = 0.0f;` member
- **File:** `Source/BellVoice.cpp`
- **Action:** Calculate decay progress in renderNextBlock (time-based)
- **Verification:** Decay progress tracked

#### Task 4.8: Integrate Shimmer into startNote and renderNextBlock
- **File:** `Source/BellVoice.cpp`
- **Action:** In `startNote()`: Call `initializeShimmer(currentShimmer * 0.01f)`
- **Action:** In `renderNextBlock()`: Apply shimmer to phase increment
- **Verification:** Shimmer effect audible at high values

#### Task 4.9: Add Shimmer to Processor processBlock
- **File:** `Source/PluginProcessor.cpp`
- **Action:** Read shimmerParam and pass to updateParameters
- **Verification:** Shimmer parameter affects sound

#### Task 4.10: Add Shimmer UI Binding
- **File:** `Source/PluginEditor.h`
- **Action:** Add `shimmerRelay` and `shimmerAttachment` declarations
- **File:** `Source/PluginEditor.cpp`
- **Action:** Instantiate relay/attachment, add to options chain
- **File:** `Resources/ui/index.html`
- **Action:** Add Shimmer slider to Main Panel
- **Verification:** UI control changes shimmer value

**Phase 4 Dependencies:** Phase 1

---

### Phase 5: Mallet Enhancement

**Goal:** Add temporal spreading (attack time) based on inverse of mallet hardness.

#### Task 5.1: Add Attack Ramp State
- **File:** `Source/BellVoice.h`
- **Action:** Add member variables:
  ```cpp
  float attackRampSamples = 0.0f;
  float attackRampPosition = 0.0f;
  ```
- **Verification:** Variables declared

#### Task 5.2: Implement initializeMalletAttack() Function
- **File:** `Source/BellVoice.cpp`
- **Action:** Add helper function:
  - Calculate attack time (0-50ms based on inverse of hardness)
  - Soft mallet: extend strike noise decay, lower amplitude
- **Verification:** Function compiles

#### Task 5.3: Apply Attack Ramp in renderNextBlock
- **File:** `Source/BellVoice.cpp`
- **Action:** At start of renderNextBlock, calculate attackMultiplier
- **Action:** Apply cosine curve for natural feel
- **Action:** Apply to leftOutput and rightOutput
- **Verification:** Soft mallets have gradual attack

#### Task 5.4: Integrate into startNote
- **File:** `Source/BellVoice.cpp`
- **Action:** Call `initializeMalletAttack(currentMalletHardness)` in startNote
- **Verification:** Mallet hardness affects attack time

**Phase 5 Dependencies:** None (can run parallel to Phases 3-4)

---

### Phase 6: Stereo Enhancement

**Goal:** Implement per-partial panning, Haas effect, and stereo movement.

#### Task 6.1: Add PartialStereoState Struct
- **File:** `Source/BellVoice.h`
- **Action:** Add struct:
  ```cpp
  struct PartialStereoState {
      float panLFOPhase = 0.0f;
      float panLFORate = 0.0f;
      float basePan = 0.0f;
  };
  ```
- **Verification:** Struct compiles

#### Task 6.2: Add Partial Stereo State Array
- **File:** `Source/BellVoice.h`
- **Action:** Add `PartialStereoState partialStereo[NUM_PARTIALS];`
- **Verification:** Array declared

#### Task 6.3: Implement getPartialPan() Function
- **File:** `Source/BellVoice.cpp`
- **Action:** Add helper function:
  - Low partials center, high partials spread
  - Alternate left/right for odd/even
- **Verification:** Function compiles

#### Task 6.4: Implement initializeStereoMovement() Function
- **File:** `Source/BellVoice.cpp`
- **Action:** Add helper function:
  - Assign slow LFO rates (0.1-0.5 Hz) per partial
  - Random starting phases
  - Calculate base pan positions
- **Verification:** Function compiles

#### Task 6.5: Implement getModulatedPan() Function
- **File:** `Source/BellVoice.cpp`
- **Action:** Add per-sample pan modulation:
  - Advance LFO phase
  - Sine LFO for smooth movement
  - 30% of spread as movement range
- **Verification:** Function compiles

#### Task 6.6: Add Haas Delay State
- **File:** `Source/BellVoice.h`
- **Action:** Add members:
  ```cpp
  std::vector<float> haasDelayBuffer;
  int haasDelayWritePos = 0;
  int haasDelaySamples = 0;
  ```
- **Verification:** Variables declared

#### Task 6.7: Implement prepareHaasDelay() Function
- **File:** `Source/BellVoice.cpp`
- **Action:** Add function to initialize delay buffer in `prepare()`:
  - Max 30ms delay at any sample rate
  - Clear buffer
- **Verification:** Function compiles

#### Task 6.8: Implement setHaasDelay() Function
- **File:** `Source/BellVoice.cpp`
- **Action:** Add function to set delay samples from stereoSpread
- **Verification:** Function compiles

#### Task 6.9: Implement processHaasDelay() Function
- **File:** `Source/BellVoice.cpp`
- **Action:** Add delay processing function per RESEARCH.md
- **Verification:** Function compiles

#### Task 6.10: Integrate Stereo Enhancement into renderNextBlock
- **File:** `Source/BellVoice.cpp`
- **Action:** Replace simple unison panning with:
  - Per-partial panning via getModulatedPan()
  - Apply Haas delay to right channel
- **Action:** Call `initializeStereoMovement()` in startNote
- **Verification:** Enhanced stereo field audible

#### Task 6.11: Call prepareHaasDelay in prepare()
- **File:** `Source/BellVoice.cpp`
- **Action:** Add call in `prepare()` method
- **Verification:** Delay buffer allocated

**Phase 6 Dependencies:** None (can run parallel to Phases 3-5)

---

### Phase 7: UI Updates and Inharmonicity Rename

**Goal:** Update UI layout for new parameters, rename Inharmonicity label.

#### Task 7.1: Rename "Inharm" to "Inharmonicity"
- **File:** `Resources/ui/index.html`
- **Action:** Find slider label "Inharm" and change to "Inharmonicity"
- **Verification:** Full word appears in UI

#### Task 7.2: Add Bloom Slider to UI Layout
- **File:** `Resources/ui/index.html`
- **Action:** Add Bloom slider to Main Panel section
- **Verification:** Bloom slider visible and functional

#### Task 7.3: Add Shimmer Slider to UI Layout
- **File:** `Resources/ui/index.html`
- **Action:** Add Shimmer slider to Main Panel section
- **Verification:** Shimmer slider visible and functional

#### Task 7.4: Remove Decay Shape Dropdown from UI
- **File:** `Resources/ui/index.html`
- **Action:** Remove decayShape dropdown from Advanced section
- **Verification:** Dropdown no longer appears

#### Task 7.5: Update UI Layout for Balance
- **File:** `Resources/ui/index.html` and `Resources/ui/css/style.css`
- **Action:** Adjust layout to accommodate parameter changes
- **Verification:** UI looks balanced

**Phase 7 Dependencies:** Phases 1, 3, 4

---

### Phase 8: Preset Rework

**Goal:** Update all 25 factory presets to showcase new features.

#### Task 8.1: Audit All Factory Presets
- **File:** All files in `Resources/Presets/Factory/`
- **Action:** List each preset with required changes
- **Verification:** Audit document created

#### Task 8.2: Update Orchestral Presets (5)
- **Files:** `Resources/Presets/Factory/Orchestral/`
- **Action:** Add appropriate Bloom/Shimmer values, verify Material
- **Verification:** Each preset sounds appropriate

#### Task 8.3: Update Sacred Presets (5)
- **Files:** `Resources/Presets/Factory/Sacred/`
- **Action:** Add appropriate Bloom/Shimmer values, verify Material
- **Verification:** Each preset sounds appropriate

#### Task 8.4: Update World Presets (5)
- **Files:** `Resources/Presets/Factory/World/`
- **Action:** Add appropriate Bloom/Shimmer values, verify Material
- **Action:** Fix presets that used Linear/Exponential decay
- **Verification:** Each preset sounds appropriate

#### Task 8.5: Update Ambient Presets (5)
- **Files:** `Resources/Presets/Factory/Ambient/`
- **Action:** Add appropriate Bloom/Shimmer values, verify Material
- **Verification:** Each preset sounds appropriate

#### Task 8.6: Update Cinematic Presets (5)
- **Files:** `Resources/Presets/Factory/Cinematic/`
- **Action:** Add appropriate Bloom/Shimmer values, verify Material
- **Action:** Fix Horror Stinger (was using Linear decay)
- **Verification:** Each preset sounds appropriate

**Phase 8 Dependencies:** Phases 1-7 (all DSP changes complete)

---

### Phase 9: Code Cleanup and Magic Numbers

**Goal:** Extract magic numbers to named constants per placeholder scan.

#### Task 9.1: Extract Decay Rate Magic Numbers
- **File:** `Source/BellVoice.cpp`
- **Action:** Define named constants for:
  - Linear decay rate (0.0001f) → `LINEAR_DECAY_RATE` (if still used anywhere)
  - Noise mix level (0.3f) → `STRIKE_NOISE_MIX`
  - Partial normalization (0.4f) → `PARTIAL_NORMALIZATION`
- **Verification:** Magic numbers replaced

#### Task 9.2: Extract Multi-Stage Envelope Constants
- **File:** `Source/BellVoice.cpp`
- **Action:** Define named constants for:
  - b1 base damping (0.5f) → `DAMPING_B1_BASE`
  - b3 scaling (2e-8f) → `DAMPING_B3_SCALE`
  - Strike decay multipliers → Named constants
- **Verification:** Envelope constants documented

#### Task 9.3: Replace rand() with JUCE Random
- **File:** `Source/BellVoice.cpp`
- **Action:** Replace `rand()` calls with `juce::Random::getSystemRandom().nextFloat()`
- **Verification:** No stdlib rand() calls remain

**Phase 9 Dependencies:** Phases 1-6 (after core DSP work)

---

## Dependency Graph

```
Phase 1 (Decay Simplification) ─┬─> Phase 2 (Material)
                                │
                                ├─> Phase 3 (Bloom)
                                │
                                ├─> Phase 4 (Shimmer)
                                │
                                └─> Phase 7 (UI Updates)

Phase 5 (Mallet) ───────────────────────────────────────┐
                                                         │
Phase 6 (Stereo) ───────────────────────────────────────┤
                                                         │
                                                         v
Phases 2-7 Complete ─────────────────────────────────> Phase 8 (Presets)
                                                         │
                                                         v
Phases 1-6 Complete ─────────────────────────────────> Phase 9 (Cleanup)
```

---

## Verification Criteria

### Build Verification
- [ ] Plugin builds without warnings
- [ ] VST3 and AU formats compile
- [ ] No memory leaks in release build

### Functional Verification
- [ ] All new parameters (Bloom, Shimmer) affect sound
- [ ] Removed parameter (decayShape) causes no errors
- [ ] Material values produce distinct tonal characters
- [ ] Mallet hardness affects attack time (soft = gradual)
- [ ] Stereo spread creates spatial width
- [ ] All 25 factory presets load successfully
- [ ] Presets sound appropriate with new parameter system

### Audio Quality
- [ ] No clicks, pops, or discontinuities
- [ ] No phase cancellation from shimmer LFOs
- [ ] Haas delay transitions smoothly
- [ ] CPU usage acceptable (<15% on reference system)

### DAW Testing
- [ ] Plugin loads in Logic Pro X
- [ ] Automation works for new parameters
- [ ] Preset save/load works correctly
- [ ] MIDI input produces sound

---

## Risk Mitigation

### Backup Before Execute
Create backup of `plugins/O-Bells/` before Phase 1 begins:
```
backups/O-Bells/v1.1.1/
├── Source/
├── Resources/
├── CMakeLists.txt
└── CHANGELOG.md
```

### Incremental Testing
After each Phase:
1. Build and verify compilation
2. Load plugin in DAW
3. Test affected functionality
4. Commit working state

### Rollback Points
- After Phase 1: Decay simplification working
- After Phases 3-4: New parameters working
- After Phase 8: Presets complete

---

*Plan Phase Complete - Ready for Execute Phase*
