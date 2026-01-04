# [PluginName] - Implementation Plan

**Date:** [YYYY-MM-DD]
**Complexity Score:** [X.X] ([Simple/Complex])
**Strategy:** [Single-pass implementation | Phase-based implementation]

---

## Complexity Factors

[Show the calculation breakdown]

- **Parameters:** [N] parameters ([N/5] points, capped at 2.0) = [X.X]
- **Algorithms:** [N] DSP components = [N]
  - [List DSP components counted]
- **Features:** [N] points
  - [List features identified, e.g., "Feedback loops (+1)", "FFT processing (+1)"]
- **Total:** [X.X] (capped at 5.0)

---

## Stages

- Stage 0: Research ✓
- Stage 1: Planning ✓
- Stage 1: Foundation ← Next
- Stage 2: Shell
- Stage 3: DSP [phased if complex]
- Stage 3: GUI [phased if complex]
- Stage 3: Validation

---

## Simple Implementation (Score ≤ 2.0)

[Use this format for simple plugins - single-pass implementation]

### Implementation Flow

- Stage 1: Foundation - project structure
- Stage 2: Shell - APVTS parameters
- Stage 3: DSP - single pass
- Stage 3: GUI - single pass
- Stage 3: Validation - presets, pluginval, changelog

### Implementation Notes

**DSP Approach:**
[Describe straightforward implementation plan]

**GUI Approach:**
[Describe UI integration plan]

**Key Considerations:**
- [Any special notes for simple implementation]
- [Potential gotchas]

---

## Complex Implementation (Score ≥ 3.0)

[Use this format for complex plugins - phased implementation]

### Stage 3: DSP Phases

#### Phase 4.1: Core Processing

**Goal:** [Describe core audio path implementation]

**Components:**
- [List DSP components to implement in this phase]
- [Describe basic signal flow]

**Test Criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] Audio passes through (wet and dry signals audible)
- [ ] [Component 1] parameter works correctly
- [ ] [Component 2] parameter works correctly
- [ ] No artifacts or discontinuities

---

#### Phase 4.2: Parameter Modulation

**Goal:** [Describe modulation system implementation]

**Components:**
- [List modulation components (LFOs, envelopes, etc.)]
- [Describe parameter connections]

**Test Criteria:**
- [ ] Modulation sources generate correct waveforms
- [ ] Parameter modulation is smooth without clicks
- [ ] Modulation depth scales correctly
- [ ] [Specific modulation behavior test]

---

#### Phase 4.3: Advanced Features

[Only include if complex features present]

**Goal:** [Describe advanced DSP features]

**Components:**
- [List advanced components (FFT, feedback, multiband, etc.)]
- [Describe integration with core processing]

**Test Criteria:**
- [ ] [Feature 1] works as specified
- [ ] [Feature 2] integrates without artifacts
- [ ] Performance acceptable with all features active
- [ ] Edge cases handled correctly

---

### Stage 3: GUI Phases

#### Phase 5.1: Layout and Basic Controls

**Goal:** Integrate mockup HTML and bind basic parameters

**Components:**
- Copy v[N]-ui.html to Source/ui/public/index.html
- Update PluginEditor.h/cpp with WebView setup
- Configure CMakeLists.txt for WebView resources
- Bind primary parameters via relay system

**Test Criteria:**
- [ ] WebView window opens with correct size
- [ ] All basic controls visible and styled correctly
- [ ] Layout matches mockup design
- [ ] Background and styling render properly

---

#### Phase 5.2: Parameter Binding and Interaction

**Goal:** Two-way parameter communication (UI ↔ DSP)

**Components:**
- JavaScript → C++ relay calls (control changes)
- C++ → JavaScript parameter updates (host automation)
- Value formatting and display
- Real-time parameter updates during playback

**Test Criteria:**
- [ ] Control movements change DSP parameters
- [ ] Host automation updates UI controls
- [ ] Preset changes update all UI elements
- [ ] Parameter values display correctly
- [ ] No lag or visual glitches

---

#### Phase 5.3: Advanced UI Elements

[Only include if complex UI features present]

**Goal:** [Describe advanced UI features]

**Components:**
- [List advanced UI elements (VU meters, waveform displays, etc.)]
- [Describe real-time data flow]
- [Describe animation/visualization logic]

**Test Criteria:**
- [ ] [Advanced element 1] renders correctly
- [ ] [Advanced element 2] updates in real-time
- [ ] Performance acceptable (no CPU spikes)
- [ ] Visual polish and smoothness

---

### Implementation Flow

- Stage 1: Foundation - project structure
- Stage 2: Shell - APVTS parameters
- Stage 3: DSP - [N] phases
  - Phase 4.1
  - Phase 4.2
  - Phase 4.3 (if applicable)
- Stage 3: GUI - [N] phases
  - Phase 5.1
  - Phase 5.2
  - Phase 5.3 (if applicable)
- Stage 3: Validation - presets, pluginval, changelog

---

## Implementation Notes

[Add any notes that will help during implementation]

### Thread Safety
- [Note parameter access patterns]
- [Identify lock-free update mechanisms]
- [Flag any potential race conditions]

**Example:**
```markdown
- All parameter reads use atomic getRawParameterValue()->load()
- Filter coefficient updates in audio thread (no allocations)
- LFO phase state is per-channel (no shared state)
```

### Performance
- [Estimate CPU usage per component]
- [Identify optimization opportunities]
- [Note any performance-critical sections]

**Example:**
```markdown
- Reverb: ~30% CPU (most expensive)
- Delay line interpolation: ~10% CPU
- Total estimated: ~50% single core at 48kHz
```

### Latency
- [Calculate total processing latency]
- [Note host compensation requirements]

**Example:**
```markdown
- Base delay: 50ms (2400 samples at 48kHz)
- Reverb: 20-50ms internal latency
- Report via getLatencySamples() for host compensation
```

### Denormal Protection
- [Note denormal handling strategy]

**Example:**
```markdown
- Use juce::ScopedNoDenormals in processBlock()
- All JUCE DSP components handle denormals internally
```

### Known Challenges
- [List any anticipated difficulties]
- [Reference solutions from other plugins]
- [Note research or prototyping needed]

**Example:**
```markdown
- Filter state reset on type change prevents bursts (see GainKnob reference)
- Modulation delay routing requires conditional signal path (see TapeAge)
- VU meter throttling prevents UI thread starvation (60fps update rate)
```

---

## References

[Link to contract files and related documentation]

- Creative brief: `plugins/[PluginName]/.ideas/creative-brief.md`
- Parameter spec: `plugins/[PluginName]/.ideas/parameter-spec.md`
- DSP architecture: `plugins/[PluginName]/.ideas/architecture.md`
- UI mockup: `plugins/[PluginName]/.ideas/mockups/v[N]-ui.yaml`

[Link to similar plugins for reference]

- [PluginName1] - [What to reference from this plugin]
- [PluginName2] - [What to reference from this plugin]
