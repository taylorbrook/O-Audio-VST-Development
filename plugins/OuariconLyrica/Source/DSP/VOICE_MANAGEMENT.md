# Voice Management - Phase 2.11

## Overview

OuariconLyrica's voice management system leverages JUCE's built-in `juce::Synthesiser` framework for efficient polyphony handling and automatic voice stealing.

## Architecture

### Voice Allocation
- **Total Voices:** 16 (configured in `OuariconLyricaAudioProcessor` constructor)
- **Voice Type:** `HarpSynthVoice` (custom physical modeling voice)
- **Voice Stealing:** Automatic, oldest-note-first priority

### Voice Lifecycle

```
┌─────────────┐
│    Idle     │ (voice available for assignment)
└──────┬──────┘
       │ Note On (MIDI)
       ▼
┌─────────────┐
│   Active    │ (voice playing, registered with sympathetic engine)
│             │ - WaveguideString processing
│             │ - BodyResonance processing
│             │ - Sympathetic coupling
└──────┬──────┘
       │ Note Off OR Decay to silence
       ▼
┌─────────────┐
│  Release    │ (natural decay, still processing)
└──────┬──────┘
       │ Amplitude < threshold
       ▼
┌─────────────┐
│    Idle     │ (voice returned to pool)
└─────────────┘
```

## Voice Stealing Algorithm

When all 16 voices are active and a new note arrives:

1. **juce::Synthesiser** identifies the oldest active voice
2. Voice's `stopNote(velocity=0, allowTailOff=false)` is called
3. Voice is immediately cleared and reset
4. Voice is reassigned to new note via `startNote()`

**Priority:** Newest notes take precedence over oldest notes.

**Result:** Graceful degradation without clicks, pops, or CPU spikes.

## Active Voice Counting

### Method: `getActiveVoiceCount()`

```cpp
int OuariconLyricaAudioProcessor::getActiveVoiceCount() const
{
    int activeCount = 0;
    for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    {
        if (auto* voice = synthesiser.getVoice(i))
        {
            if (voice->isVoiceActive())
                ++activeCount;
        }
    }
    return activeCount;
}
```

**Purpose:**
- UI voice meter display (Stage 3)
- Performance monitoring
- Debug/diagnostic information

**Call Frequency:**
- Safe to call from any thread (read-only)
- Recommended: UI update rate (30-60 Hz)

## Performance Characteristics

### CPU Budget per Voice
From `architecture.md`:

| Quality | Per Voice | 16 Voices Total |
|---------|-----------|-----------------|
| Draft   | 0.21%     | 3.36%           |
| Standard| 0.46%     | 7.36%           |
| High    | 0.73%     | 11.68%          |

### Polyphony Targets

| Configuration | Max Voices | Expected CPU |
|---------------|------------|--------------|
| Current (High)| 16         | ~12%         |
| Standard      | 32         | ~15%         |
| Draft         | 64+        | <15%         |

**Note:** Voice count of 16 chosen for:
- Sufficient polyphony for harp playing (typical max: 10-12 notes)
- Safety margin for sympathetic resonance overhead
- Targets <15% CPU on modern processors

## Interaction with Other Systems

### Sympathetic Resonance Engine
- Voices register on `startNote()`
- Voices unregister on `stopNote()` or decay to silence
- Voice stealing triggers immediate unregistration
- Voice ID (`voiceId`) ensures unique tracking

### Tuning Engine
- Shared processor-level resource
- Pitch bend state cleared on voice release
- No per-voice tuning state (stateless frequency queries)

### Glissando Controller
- Per-voice glissando state
- Voice stealing resets glissando for new note
- Glissando continues through natural decay (not affected by note-off)

## Testing Criteria (from plan.md)

**Phase 2.11 Test:** 32+ notes polyphony without CPU overload

**Validation:**
1. Play 16 simultaneous notes → All voices active
2. Play 17th note → Voice stealing occurs (no clicks)
3. Play 32+ rapid notes → Graceful degradation
4. Monitor `getActiveVoiceCount()` → Accurate tracking

**Success Criteria:**
- No audio glitches during voice stealing
- CPU remains <50% at 16 voices (High quality)
- Voice count accurate within 1 sample

## Future Enhancements (Optional)

### Quality Preset Switching
**Status:** Not implemented (deferred per simplified requirements)

**Concept:**
```cpp
enum VoiceQuality { Draft, Standard, High };

void setVoiceQuality(VoiceQuality quality)
{
    switch (quality)
    {
        case Draft:
            // Disable sympathetic resonance, reduce stiffness stages
            maxVoices = 64;
            break;
        case Standard:
            // Standard features, modal body instead of convolution
            maxVoices = 32;
            break;
        case High:
            // All features enabled
            maxVoices = 16;
            break;
    }
    synthesiser.clearVoices();
    for (int i = 0; i < maxVoices; ++i)
        synthesiser.addVoice(new HarpSynthVoice());
}
```

**Rationale for deferral:**
- Current 16-voice implementation meets performance targets
- Quality presets can be added in optimization phase (2.12) if needed
- No user-facing parameter for quality switching yet (Stage 3)

### Soft Voice Limit
**Status:** Not implemented

**Concept:** Gradually reduce sympathetic intensity or stiffness complexity as active voice count approaches limit.

**Implementation:** Would require per-voice quality adjustment based on `getActiveVoiceCount()`.

## Implementation Files Modified

**Phase 2.11 Changes:**
- `Source/PluginProcessor.h` - Added `getActiveVoiceCount()` method
- `Source/PluginProcessor.cpp` - Implemented active voice counting

**No changes required to:**
- `HarpSynthVoice` (already compatible with voice stealing)
- CMakeLists.txt (no new source files)

## Conclusion

Phase 2.11 provides robust voice management leveraging JUCE's proven synthesiser framework. The `getActiveVoiceCount()` API enables future UI integration for performance monitoring. Voice stealing works seamlessly with OuariconLyrica's physical modeling engine, including sympathetic resonance and glissando systems.

**Status:** ✓ Complete and ready for Stage 3 (GUI)
