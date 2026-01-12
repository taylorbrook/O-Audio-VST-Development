# Stage 2 Critical Patterns - DSP Implementation

**Purpose:** Prevent repeat mistakes during Stage 2 (DSP) implementation.

**When to read:** Before implementing audio processing in processBlock.

**Patterns included:** 3 of 22 total patterns (audio processing, threading, DSP modules)

---

## 1. Bus Configuration - Effects vs Instruments

### Effects (Audio In → Audio Out)
```cpp
AudioProcessor(BusesProperties()
    .withInput("Input", juce::AudioChannelSet::stereo(), true)
    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
```

### Instruments (MIDI In → Audio Out)
```cpp
AudioProcessor(BusesProperties()
    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
```

**Common mistake:** Adding input bus to instruments causes "missing input" errors in DAWs.

**Why this matters for DSP:** Your processBlock implementation must match the bus configuration:
- Effects: Read from input buffer, write to output buffer
- Instruments: Generate audio directly into output buffer (no input to read)

---

## 2. Threading - UI ↔ Audio Thread

### ❌ WRONG (Thread violation - will crash)
```cpp
// In PluginEditor (UI thread)
button.onClick = [this] {
    audioProcessor.processBlock(...);  // ILLEGAL
};
```

### ✅ CORRECT
```cpp
// In PluginEditor (UI thread) - use parameters or atomic flags
button.onClick = [this] {
    audioProcessor.getAPVTS().getParameter("trigger")->setValueNotifyingHost(1.0f);
};
```

**Rule:** NEVER call audio processing code from UI thread. Use AudioProcessorValueTreeState (APVTS) for communication.

### Real-Time Safety in processBlock

**❌ WRONG (Allocations in audio thread)**
```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
    std::vector<float> tempBuffer(buffer.getNumSamples());  // ALLOCATION - BAD
    juce::String debugMsg = "Processing...";  // STRING ALLOCATION - BAD
}
```

**✅ CORRECT (Pre-allocated buffers)**
```cpp
// In class header
std::vector<float> tempBuffer;

void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    tempBuffer.resize(samplesPerBlock);  // Pre-allocate in prepareToPlay
}

void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
    juce::ScopedNoDenormals noDenormals;  // ALWAYS include this
    // Use pre-allocated tempBuffer - no allocation here
}
```

**Critical rules:**
- No `new`, `malloc`, `std::vector` creation in processBlock
- No string operations in processBlock
- Always use `juce::ScopedNoDenormals` at start of processBlock
- Pre-allocate all buffers in `prepareToPlay()`

---

## 3. juce::dsp::Reverb API - Modern DSP Pipeline (ALWAYS REQUIRED)

### ❌ WRONG (API mismatch - will not compile)
```cpp
// Using old juce::Reverb API with juce::dsp::Reverb class
juce::dsp::Reverb reverb;

void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    reverb.setSampleRate(sampleRate);  // No such method in juce::dsp::Reverb
    reverb.reset();
}

void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
    if (buffer.getNumChannels() == 1) {
        reverb.processMono(buffer.getWritePointer(0), buffer.getNumSamples());  // Wrong API
    } else if (buffer.getNumChannels() == 2) {
        reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), buffer.getNumSamples());  // Wrong API
    }
}
```

### ✅ CORRECT
```cpp
// Modern juce::dsp::Reverb with ProcessSpec and AudioBlock
juce::dsp::Reverb reverb;

void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    reverb.prepare(spec);
    reverb.reset();
}

void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
    juce::ScopedNoDenormals noDenormals;

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);  // Handles all channel configurations automatically
}
```

**Why:** JUCE has two different reverb classes with incompatible APIs:
- `juce::Reverb` (old, non-DSP module) - Uses `setSampleRate()`, `processMono()`, `processStereo()`
- `juce::dsp::Reverb` (modern DSP module) - Uses `prepare(spec)` and `process(context)`

**Key differences:**
- Modern DSP uses ProcessSpec (not individual setters) in `prepare()`
- Modern DSP uses AudioBlock/ProcessContext (not raw pointers) in `process()`
- Modern DSP handles mono/stereo/multi-channel automatically via AudioBlock
- Modern DSP integrates seamlessly with other juce::dsp components (DryWetMixer, etc.)

**Pattern applies to all juce::dsp components:**
- `juce::dsp::Compressor`
- `juce::dsp::Limiter`
- `juce::dsp::Chorus`
- `juce::dsp::Phaser`
- `juce::dsp::Reverb`
- `juce::dsp::DelayLine`
- `juce::dsp::IIR::Filter`
- `juce::dsp::Gain`
- `juce::dsp::Oscillator`

---

## DSP Implementation Checklist

Before completing Stage 2, verify:

- [ ] `juce::ScopedNoDenormals noDenormals;` at start of processBlock
- [ ] No allocations in processBlock (new, malloc, vector creation, strings)
- [ ] All buffers pre-allocated in prepareToPlay()
- [ ] Using modern juce::dsp API (ProcessSpec, AudioBlock, ProcessContext)
- [ ] Bus configuration matches plugin type (effect vs instrument)
- [ ] Edge cases handled:
  - [ ] Zero-length buffers (`if (buffer.getNumSamples() == 0) return;`)
  - [ ] Channel count validation
  - [ ] Parameter value clamping

---

## Quick Reference

| Pattern | What It Prevents |
|---------|-----------------|
| Bus Configuration | "Missing input" errors, silent synths |
| Threading Safety | Crashes, audio glitches, race conditions |
| Modern DSP API | Compilation errors, API confusion |

---

**Full patterns file:** `troubleshooting/patterns/juce8-critical-patterns.md`
