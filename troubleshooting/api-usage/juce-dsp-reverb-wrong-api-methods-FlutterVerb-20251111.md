---
plugin: FlutterVerb
date: 2025-11-11
problem_type: api_misuse
component: juce_dsp
symptoms:
  - "error: no member named 'setSampleRate' in 'juce::dsp::Reverb'"
  - "error: no member named 'processMono' in 'juce::dsp::Reverb'"
  - "error: no member named 'processStereo' in 'juce::dsp::Reverb'"
root_cause: wrong_api
juce_version: 8.0.0
resolution_type: api_migration
severity: critical
tags: [dsp, reverb, api-migration, juce-8, processblock]
---

# Troubleshooting: juce::dsp::Reverb API Mismatch (JUCE 8)

## Problem
The dsp-agent subagent generated code using the old `juce::Reverb` API (`setSampleRate()`, `processMono()`, `processStereo()`) instead of the modern `juce::dsp::Reverb` API, causing build failures.

## Environment
- Plugin: FlutterVerb
- JUCE Version: 8.0.0
- Affected: PluginProcessor.cpp (prepareToPlay and processBlock), Stage 4.1
- Date: 2025-11-11

## Symptoms
- Build fails with compiler errors:
  ```
  error: no member named 'setSampleRate' in 'juce::dsp::Reverb'
  error: no member named 'processMono' in 'juce::dsp::Reverb'
  error: no member named 'processStereo' in 'juce::dsp::Reverb'
  ```
- Code compiles if `juce::Reverb` (non-DSP) is used instead
- Modern JUCE DSP pipeline expects `prepare()` and `process()` API

## What Didn't Work

**Direct solution:** The problem was identified immediately from build errors showing API mismatch.

## Solution

Replace old `juce::Reverb` API calls with modern `juce::dsp::Reverb` API:

**Code changes:**

```cpp
// prepareToPlay() - Before (broken):
reverb.setSampleRate(sampleRate);
reverb.reset();

// prepareToPlay() - After (fixed):
reverb.prepare(spec);  // spec contains sampleRate, blockSize, channels
reverb.reset();
```

```cpp
// processBlock() - Before (broken):
if (buffer.getNumChannels() == 1) {
    reverb.processMono(buffer.getWritePointer(0), buffer.getNumSamples());
} else if (buffer.getNumChannels() == 2) {
    reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), buffer.getNumSamples());
}

// processBlock() - After (fixed):
juce::dsp::AudioBlock<float> block(buffer);
juce::dsp::ProcessContextReplacing<float> context(block);
reverb.process(context);
```

## Why This Works

**Root cause:** API confusion between two different reverb classes:
1. **`juce::Reverb`** (old, non-DSP module) - Uses `setSampleRate()`, `processMono()`, `processStereo()`
2. **`juce::dsp::Reverb`** (modern DSP module) - Uses `prepare(spec)` and `process(context)`

The subagent mixed APIs: declared `juce::dsp::Reverb` but called old `juce::Reverb` methods.

**Why the solution works:**
- `juce::dsp::Reverb` follows modern JUCE DSP architecture:
  - `prepare(spec)` receives sample rate, block size, and channel count as ProcessSpec
  - `process(context)` uses AudioBlock and ProcessContext for type-safe processing
  - Handles mono/stereo/multi-channel automatically via AudioBlock
- Eliminates manual channel counting and pointer management
- Integrates with DryWetMixer and other juce::dsp components seamlessly

**Modern DSP pipeline pattern:**
```cpp
// 1. Create AudioBlock from buffer
juce::dsp::AudioBlock<float> block(buffer);

// 2. Wrap in ProcessContext (Replacing = in-place processing)
juce::dsp::ProcessContextReplacing<float> context(block);

// 3. Process (handles all channel configurations)
reverb.process(context);
```

## Prevention

To avoid this in future DSP implementations:

1. **Check header includes:** If using `<juce_dsp/juce_dsp.h>`, use `juce::dsp::` classes with modern API
2. **ProcessSpec pattern:** All juce::dsp components use `prepare(spec)` not individual setters
3. **AudioBlock/ProcessContext:** Modern DSP uses these wrappers, not raw pointers
4. **Reference existing code:** Check architecture.md - it specifies `juce::dsp::Reverb`, not `juce::Reverb`
5. **JUCE docs verification:** Cross-check API against JUCE 8 docs (https://docs.juce.com/master/classdsp_1_1Reverb.html)

**Pattern to follow:**
```cpp
// Declaration
juce::dsp::Reverb reverb;

// prepareToPlay()
juce::dsp::ProcessSpec spec;
spec.sampleRate = sampleRate;
spec.maximumBlockSize = samplesPerBlock;
spec.numChannels = getTotalNumOutputChannels();
reverb.prepare(spec);

// processBlock()
juce::dsp::AudioBlock<float> block(buffer);
juce::dsp::ProcessContextReplacing<float> context(block);
reverb.process(context);
```

## Related Issues

No related issues documented yet.

## References

- JUCE 8 dsp::Reverb docs: https://docs.juce.com/master/classdsp_1_1Reverb.html
- JUCE DSP introduction: https://docs.juce.com/master/tutorial_dsp_introduction.html
- ProcessSpec: https://docs.juce.com/master/structdsp_1_1ProcessSpec.html
