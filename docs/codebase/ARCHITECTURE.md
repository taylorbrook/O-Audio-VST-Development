# Architecture

**Analysis Date:** 2026-01-22

## Pattern Overview

**Overall:** JUCE-based plugin architecture with modular DSP layers and shared component system

**Key Characteristics:**
- Modular plugin suite: Each plugin is self-contained with independent CMake build configs
- Shared module system: Common components (`modules/`) used across plugins
- WebView-based UI: HTML5/JavaScript UI embedded via JUCE WebBrowserComponent
- Real-time audio DSP: Physical modeling synthesis using waveguide and modal techniques
- Thread-safe parameter passing: APVTS (AudioProcessorValueTreeState) for audio/UI sync
- Lock-free communication: Atomic queues for MIDI/visualization data between audio and UI threads

## Layers

**Audio Processing Layer:**
- Purpose: Real-time audio synthesis and processing on audio thread
- Location: `plugins/[PluginName]/Source/PluginProcessor.cpp|h`
- Contains: AudioProcessor implementation, JUCE Synthesiser, voice management, DSP parameter updates
- Depends on: JUCE framework, DSP components, APVTS
- Used by: Plugin framework, editor for state management

**Voice/Instrument Layer:**
- Purpose: Per-note DSP rendering (one voice per MIDI note)
- Location: `plugins/[PluginName]/Source/[VoiceName].cpp|h` (e.g., `HarpSynthVoice.cpp`)
- Contains: Individual voice rendering, note triggering, pitch control, per-voice DSP state
- Depends on: DSP components, material models, parameter references
- Used by: Synthesiser (voice manager)

**DSP Component Layer:**
- Purpose: Specialized signal processing modules (waveguide, filters, resonators)
- Location: `plugins/OuariconLyrica/Source/DSP/` (varies by plugin)
- Contains: Physical modeling (WaveguideString, BodyResonance), tuning (TuningEngine), sympathetic coupling, glissando
- Depends on: JUCE DSP utilities, mathematical models
- Used by: Voices and processor

**Shared Module Layer:**
- Purpose: Plugin-agnostic utilities and managers
- Location: `modules/[category]/[module]/cpp/`
- Contains: Preset manager, WebView relay manager, effects units, VU meter bridges
- Depends on: JUCE framework
- Used by: Multiple plugins via include paths and linking

**UI/Editor Layer:**
- Purpose: Visual interface and parameter control
- Location: `plugins/[PluginName]/Source/PluginEditor.cpp|h` and `Resources/ui/`
- Contains: JUCE editor, WebBrowserComponent hosting, relay setup, web assets
- Depends on: PluginProcessor, WebView relays, BinaryData resources
- Used by: Plugin framework

## Data Flow

**MIDI Input → Audio Output:**

1. **MIDI Event Reception:** JUCE plugin framework receives MIDI note-on/off from DAW
2. **Voice Allocation:** Synthesiser allocates voice from pool based on availability
3. **Note Start:** Voice.startNote() calculates frequency via TuningEngine, reads parameters from APVTS
4. **DSP Initialization:** Voice initializes string model (WaveguideString), body resonance, glissando state
5. **Parameter Modulation:** Each render block, voice reads updated parameters from APVTS (brightness, damping, decay, etc.)
6. **Audio Synthesis:** WaveguideString.processSample() executed per sample, generates waveform
7. **Body Filtering:** BodyResonance applies modal filtering to string output
8. **Sympathetic Coupling:** (OuariconLyrica only) SympatheticResonanceEngine adds coupling from other active voices
9. **Output Buffering:** Rendered samples accumulated into AudioBuffer, sent to DAW

**Parameter Change Flow:**

1. **UI Slider Move:** User adjusts slider in WebView
2. **JavaScript Event:** WebView JavaScript sends parameter change to relay
3. **Relay Processing:** WebSliderRelay updates APVTS parameter value
4. **Audio Thread Read:** Next render block, voice reads updated value via `getRawParameterValue()`
5. **DSP Update:** Voice applies parameter to affected DSP component (e.g., WaveguideString.setDamping())

**MIDI Event Visualization (UI Feedback):**

1. **Audio Thread:** processBlock() creates MidiNoteEvent for each note-on/off
2. **Queue Push:** Event pushed to lock-free MidiEventQueue (try-lock pattern)
3. **UI Timer:** Editor timer callback pops events from queue
4. **WebView Update:** evaluateJavascript() called to highlight pressed keys on keyboard visualization

**State Management:**

- **Preset Save:** APVTS state serialized to JSON via preset manager; custom state (tuning files) handled via callbacks
- **Preset Load:** JSON deserialized back to APVTS; custom callbacks restore tuning/DSP state
- **Plugin State:** getStateInformation/setStateInformation use APVTS XML + custom state callbacks

## Key Abstractions

**AudioProcessor (PluginProcessor):**
- Purpose: Core plugin interface; manages voices, parameters, and DSP engine state
- Examples: `plugins/OuariconLyrica/Source/PluginProcessor.h`, `plugins/OuariconMarimba/Source/PluginProcessor.h`
- Pattern: Inherits from juce::AudioProcessor; owns Synthesiser, TuningEngine, SympatheticResonanceEngine; creates/manages APVTS

**SynthesiserVoice (HarpSynthVoice, MarimbaVoice):**
- Purpose: Single note rendering; delegates to DSP components
- Examples: `plugins/OuariconLyrica/Source/HarpSynthVoice.h`, `plugins/OuariconMarimba/Source/MarimbaVoice.h`
- Pattern: Inherits from juce::SynthesiserVoice; owns string model, body resonance, glissando controller; renders one note per block

**TuningEngine:**
- Purpose: MIDI-to-frequency conversion with support for custom tunings (12-TET, Scala files, preset temperaments)
- Examples: `plugins/OuariconLyrica/Source/DSP/TuningEngine.h`, `plugins/OuariconMarimba/Source/TuningEngine.h`
- Pattern: Processor-level component shared by all voices; thread-safe frequency lookups via double-buffered tables; supports per-note pitch bend

**WaveguideString:**
- Purpose: Physical model of plucked string using bidirectional digital waveguide
- Examples: `plugins/OuariconLyrica/Source/DSP/WaveguideString.h`
- Pattern: Two delay lines (upper/lower rails), bridge/nut filters, loop damping, stiffness filter for inharmonicity; processSample() called per sample

**BodyResonance:**
- Purpose: Modal synthesis of resonant body (convolution or peak filter-based)
- Examples: `plugins/OuariconLyrica/Source/DSP/BodyResonance.h`, `plugins/OuariconMarimba/Source/BodyResonance.h`
- Pattern: Processor-level setup, per-voice application; filters string output through body modes

**SympatheticResonanceEngine:**
- Purpose: Tracks active voices and applies acoustic coupling between harmonically related strings
- Examples: `plugins/OuariconLyrica/Source/DSP/SympatheticResonance.h`
- Pattern: Processor-level (shared); double-buffered coupling matrix; voices register/unregister with frequency info; lock-free audio thread access

**OuariconPresetManager:**
- Purpose: Generic preset save/load with JSON serialization and custom state callbacks
- Examples: `modules/persistence/preset-manager/cpp/OuariconPresetManager.h`
- Pattern: Owned by processor; supports factory presets (read-only) and user presets; callbacks for plugin-specific state (e.g., tuning files)

**AudioProcessorEditor + WebBrowserComponent:**
- Purpose: Native JUCE editor containing embedded WebView for HTML5 UI
- Examples: `plugins/OuariconLyrica/Source/PluginEditor.h`
- Pattern: Creates relays (WebSliderRelay, WebComboBoxRelay) first, WebBrowserComponent second, parameter attachments last (destruction order critical for safety)

## Entry Points

**PluginProcessor Constructor:**
- Location: `plugins/[PluginName]/Source/PluginProcessor.cpp`
- Triggers: Plugin instantiation by DAW
- Responsibilities: Initialize parameter layout (APVTS), create DSP engines (TuningEngine, SympatheticResonanceEngine), initialize preset manager

**prepareToPlay():**
- Location: `plugins/[PluginName]/Source/PluginProcessor.cpp`
- Triggers: Audio session starts or sample rate changes
- Responsibilities: Set sample rate for all DSP components, allocate/prepare voice pool, initialize filters/buffers

**processBlock():**
- Location: `plugins/[PluginName]/Source/PluginProcessor.cpp`
- Triggers: Audio buffer request from DAW (once per audio quantum ~5-1000 samples)
- Responsibilities: Process MIDI buffer, render active voices, apply processor-level effects, update visualization queues

**createEditor():**
- Location: `plugins/[PluginName]/Source/PluginProcessor.cpp`
- Triggers: User opens plugin window in DAW
- Responsibilities: Instantiate PluginEditor which owns WebView and parameter attachments

**paint() and resized():**
- Location: `plugins/[PluginName]/Source/PluginEditor.cpp`
- Triggers: Editor window drawn or resized
- Responsibilities: Render background, position WebView component

## Error Handling

**Strategy:** Silent degradation with fallback behavior; atomic flags for state safety

**Patterns:**

- **Parameter Access:** APVTS guaranteed to have all registered parameters; safe to call getRawParameterValue() without null checks
- **Frequency Calculation:** If TuningEngine unavailable (null), fall back to standard juce::MidiMessage::getMidiNoteInHertz()
- **Thread-Safe Queues:** Lock-free FIFO (MidiEventQueue, WaveformFifo) uses try-lock for push; drops events under contention (acceptable for visualization)
- **Sympathetic Coupling:** Fixed-size voice array; voices beyond max capacity silently ignored (no allocation on audio thread)
- **File I/O:** Preset save/load wrapped in error handling; invalid files logged but don't crash audio engine

## Cross-Cutting Concerns

**Logging:** Console output via JUCE logging (Debug builds only); no runtime logging in production audio path

**Validation:** Parameters validated at creation time (JUCE NormalisableRange); DSP components clip/saturate silently rather than asserting

**Authentication:** None (audio plugins are sandboxed by DAW)

**Concurrency Model:**
- **Audio Thread:** Real-time, lock-free access to DSP state and parameter values
- **UI Thread:** Updates APVTS parameters, triggers file dialogs, updates relays
- **Thread Safety:** APVTS provides atomic parameter access; lock-free queues (MidiEventQueue) bridge threads; no explicit locks in audio path

**Performance Considerations:**
- **Per-Sample DSP:** WaveguideString.processSample() unrolled and inlined; no allocations
- **Per-Block Setup:** TuningEngine frequency table pre-computed; sympathetic coupling matrix rebuilt at block boundaries (not per-voice)
- **Memory:** Fixed-size arrays (voice pool, event queues) prevent audio-thread allocation

---

*Architecture analysis: 2026-01-22*
