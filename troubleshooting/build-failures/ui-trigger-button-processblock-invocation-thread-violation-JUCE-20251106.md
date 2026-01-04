---
plugin: JUCE
date: 2025-11-06
problem_type: build_error
component: juce_processor
symptoms:
  - "non-const lvalue reference to type 'AudioBuffer' cannot bind to a temporary"
  - Compilation error when calling processBlock from UI thread
  - Button compiles but trigger functionality fails to build
root_cause: thread_violation
juce_version: 8.0.9
resolution_type: code_fix
severity: moderate
tags: [thread-safety, processblock, ui-thread, audio-thread, midi, references]
---

# Troubleshooting: UI Trigger Button Cannot Call processBlock Directly

## Problem
Attempting to trigger audio from a UI button by calling `processBlock()` directly results in compilation error about non-const lvalue references binding to temporaries.

## Environment
- JUCE Version: 8.0.9
- Affected: PluginEditor.cpp, UI thread attempting to invoke audio processing
- Date: 2025-11-06

## Symptoms
- Compilation error:
  ```
  error: non-const lvalue reference to type 'AudioBuffer<...>' cannot bind to a temporary of type 'AudioBuffer<...>'
  ```
- Error occurs when trying to call `audioProcessor.processBlock(juce::AudioBuffer<float>(2, 512), juce::MidiBuffer())`
- Button compiles fine, but trigger functionality fails to build

## What Didn't Work

**Attempted Solution 1:** Calling `processBlock()` with temporary AudioBuffer and MidiBuffer objects
- **Why it failed:** The `processBlock()` method signature requires non-const lvalue references (`AudioBuffer<float>&` and `MidiBuffer&`). You cannot pass temporary objects (rvalues) to functions expecting non-const references. Additionally, even if this compiled, calling `processBlock()` from the UI thread is fundamentally unsafe - it violates JUCE's thread safety model.

## Solution

**For a test trigger button:** Remove the audio triggering functionality entirely and only trigger visual feedback (LED).

The proper approach for MIDI-triggered plugins is to let MIDI input trigger the sound through the processor's normal audio processing path. The UI button should only provide visual feedback.

**Code changes:**
```cpp
// Before (broken):
void ClapPlugin808Editor::triggerTestSound()
{
    // Trigger the LED
    ledIndicator.trigger();

    // Send MIDI note to processor
    auto midiMessage = juce::MidiMessage::noteOn(1, 60, static_cast<juce::uint8>(100));
    audioProcessor.processBlock(juce::AudioBuffer<float>(2, 512), juce::MidiBuffer());

    // Note: Proper implementation would inject MIDI into the processor's MIDI buffer
    // This is a simplified version for UI testing
}

// After (fixed):
void ClapPlugin808Editor::triggerTestSound()
{
    // Trigger the LED for visual feedback
    // Note: This button is for LED testing only
    // Actual sound triggering happens via MIDI input to the processor
    ledIndicator.trigger();
}
```

## Why This Works

**C++ Technical Reason:**
The `processBlock()` method signature is:
```cpp
void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&);
```

The `&` means non-const lvalue reference. These can only bind to:
- Named variables (lvalues)
- NOT temporary objects (rvalues)

When you write `juce::AudioBuffer<float>(2, 512)`, that creates a temporary object that will be destroyed at the end of the statement. C++ prevents binding non-const references to temporaries because the function could modify the object, but the modifications would be lost when the temporary is destroyed.

**JUCE Architecture Reason:**
Even if the compilation issue was fixed by creating named variables, calling `processBlock()` from the UI thread is architecturally wrong:

1. **Thread Safety:** `processBlock()` runs on the audio thread (high priority, real-time). The UI runs on the message thread (lower priority, non-real-time). Directly calling between threads without synchronization causes race conditions.

2. **Audio Context:** `processBlock()` expects to be called by the host with proper audio setup (sample rate initialized, buffers prepared, etc.). Calling it manually bypasses this context.

3. **Proper Pattern:** JUCE plugins should use thread-safe message queues or parameter changes to communicate from UI to audio thread.

**Simple Solution:**
For a MIDI-triggered instrument plugin, there's no need for a test trigger button that actually produces sound. The button triggers visual feedback (LED), and users trigger sound via MIDI input through their DAW.

**Alternative (Complex):**
If you genuinely need UI-triggered sound, implement thread-safe MIDI injection:
1. Add a lock-free FIFO queue to the processor
2. UI button pushes MIDI messages to the queue
3. `processBlock()` reads from queue and processes messages
4. This is overkill for most plugins

## Prevention

**When adding UI trigger buttons:**
1. **Never call `processBlock()` from UI thread** - This violates JUCE threading model
2. **For visual feedback only:** Just trigger UI elements (LEDs, animations)
3. **For actual audio triggering:** Implement proper thread-safe communication:
   - Use `AudioProcessorValueTreeState` for parameters
   - Use lock-free FIFOs for MIDI/event injection
   - Use atomic flags for simple state changes
4. **Question necessity:** Does a MIDI-triggered plugin really need a test button?

**Thread safety rules:**
- UI thread (MessageManager): Read parameters, update UI
- Audio thread (processBlock): Process audio, read parameters (thread-safe)
- Never share mutable state without synchronization
- Never call audio thread functions from UI thread directly

## Related Issues
No related issues documented yet.
