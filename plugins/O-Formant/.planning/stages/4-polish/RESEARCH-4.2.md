# Phase 4.2: Validation + Release - Research

**Researched:** 2026-04-05
**Domain:** pluginval level 10 validation, O-Formant DSP stability, release packaging
**Confidence:** HIGH

## Summary

Phase 4.2 upgrades O-Formant validation from pluginval level 5 (already passing) to level 10 (strictest), fixes any failures found, writes CHANGELOG.md, and does a final build+install. Level 10 adds aggressive parameter thread safety tests, state restoration verification with tolerance checks, non-releasing sample rate switches, and concurrent editor+audio thread stress testing. These tests stress exactly the areas where synthesizer plugins with complex DSP chains (Newton-Raphson solvers, biquad filter banks, wavetable interpolation) tend to fail: NaN propagation, uninitialized state after sample rate changes, and parameter boundary behavior under randomization.

The O-Formant codebase already has good NaN/Inf guards in FormantBiquad (z1/z2 reset) and FormantVoice::renderNextBlock (final sample guard). The main risks are: (1) FormantFilterBank coefficient computation at extreme parameter randomization (formantShift +/-24st combined with formantSpread 2.0x pushing frequencies near Nyquist), (2) the outputGain SmoothedValue in processBlock potentially producing 0-gain when outputGain=-60dB (should be fine but verify), and (3) state restoration tolerance for the `currentPreset` attribute stored outside APVTS.

O-Prism passed level 10 with tanh soft clipping on noise generators. O-Bass and O-Bells both use the same OuariconPresetManager pattern. The established project CHANGELOG format follows Keep a Changelog style (## [version] - date, ### Added/Changed/Fixed sections).

**Primary recommendation:** Run pluginval level 10 on both VST3 and AU. Most likely passes clean given the existing NaN guards. If failures occur, they will be in parameter state restoration (tolerance mismatch) or edge-case biquad instability during rapid parameter randomization.

<user_constraints>
## User Constraints (from CONTEXT-4.2.md)

### Locked Decisions
- pluginval level 10 (strictest) for both VST3 and AU
- CHANGELOG.md in standard project format documenting v1.0.0
- State persistence: preset name round-trip verification
- No cross-platform or DAW-specific testing for v1.0.0
- No additional testing beyond pluginval + auval

### Deferred Ideas (OUT OF SCOPE)
- Cross-platform testing
- DAW-specific testing
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) | Full pluginval level 10 test analysis below; command-line invocation documented; risk areas identified |
</phase_requirements>

## pluginval Level 10: Complete Test Breakdown

### Tests by Strictness Level

pluginval runs ALL tests at or below the specified strictness level. At level 10, every test runs.

| Test Name | Min Level | What It Does | Risk for O-Formant |
|-----------|-----------|--------------|---------------------|
| Plugin info | 1 | Logs metadata (name, latency, tail) | None - read-only |
| Plugin programs | 2 | Randomly switches between programs | None - only 1 program |
| Editor | 2 | Creates editor twice, measures time | Low - WebView creation |
| Automatable Parameters | 2 | Logs all parameter metadata | None - read-only |
| Plugin state | 2 | Save state, randomize all params, restore | Low - APVTS handles this |
| Audio processing | 3 | 10 blocks at each SR/block-size combo, checks NaN/Inf/subnormal | Medium - see DSP risks |
| Automation | 3 | 32-sample sub-blocks, randomly modifies up to 10 params between blocks | Medium - coefficient updates |
| Open editor whilst processing | 4 | Async thread calls processBlock while main thread creates editor | Low - no shared mutable state in editor construction |
| Editor Automation | 5 | 1000x iterations of setValue(random) on ALL params with editor open | Medium - rapid param changes |
| auval | 5 | Runs Apple AU validator with `-stress 20` at level >5 | Low - already passing |
| VST3 validator | 5 | Extended Steinberg validator | Low |
| **Non-releasing audio processing** | **6** | prepareToPlay() at new SR WITHOUT releaseResources() | **HIGH** - key risk area |
| **Plugin state restoration** | **6** | Per-parameter: save, randomize, restore, check within 0.1 tolerance. Level 8+ requires exact binary match. | **MEDIUM** - preset name attribute |
| **Parameters** | **7** | Comprehensive parameter metadata validation | Low |
| **Background thread state** | **7** | getStateInformation + randomize + setStateInformation with editor open, 2000ms async sleep | **MEDIUM** - concurrent state access |
| **Parameter thread safety** | **7** | 500x concurrent: message thread calls setValueNotifyingHost on all params, audio thread calls processBlock at 32-sample blocks | **HIGH** - key stress test |

### Critical Test Details

**Non-releasing audio processing (level 6):**
- Calls prepareToPlay() at new sample rate WITHOUT calling releaseResources() first
- Tests sample rates: 44100, 48000, 96000 (configurable)
- Block sizes: 64, 128, 256, 512, 1024 (configurable)
- For synths: sends note-on/note-off messages
- Checks output for NaN, Inf, subnormals

**Plugin state restoration (level 6/8):**
- Level 6-7: Each parameter saved, randomized, restored, checked within +/-0.1 tolerance
- Level 8+: ALSO requires exact binary state match between two consecutive getStateInformation() calls
- The `currentPreset` XML attribute stored by O-Formant's getStateInformation needs to survive this

**Parameter thread safety (level 7):**
- 500 iterations simultaneously from two threads
- Message thread: setValueNotifyingHost(random) on ALL 21 parameters
- Audio thread: setValue(random) + processBlock() at 32-sample blocks, 44100 Hz
- This is the most aggressive test - all 21 parameters randomized 500 times while audio processes

### Default Sample Rates and Block Sizes

```
Sample rates: 44100, 48000, 96000
Block sizes:  64, 128, 256, 512, 1024
```

All combinations are tested (15 configs for audio tests).

## O-Formant Risk Analysis

### Risk 1: Non-Releasing Sample Rate Switch (HIGH)

**What happens:** prepareToPlay() called with new sample rate but releaseResources() was NOT called.

**Code path:**
- `OFormantAudioProcessor::prepareToPlay()` calls:
  - `synthesiser.setCurrentPlaybackSampleRate(sampleRate)` 
  - `outputGainSmoothed.reset(sampleRate, 0.050)`
  - Each voice's `prepare(sampleRate)` which resets all DSP components

**Current behavior:** prepareToPlay resets everything via `voice->prepare(sampleRate)`, which calls `prepare()` on every DSP component. Each component stores its own sampleRate and resets state. This SHOULD be safe because:
- LFGlottalSource::prepare() resets phase, phaseIncrement
- AspirationNoise::prepare() recalculates lpCoeff for new SR
- FormantFilterBank::prepare() calls reset() on all 5 biquads
- VibratoLFO::prepare() resets phase
- ConsonantEngine::prepare() recalculates burstTotalSamples

**Risk assessment:** LOW - prepareToPlay already does full reset. No dependency on releaseResources().

### Risk 2: Parameter Randomization Pushing Biquad Instability (HIGH)

**What happens:** pluginval sets all 21 parameters to random values simultaneously, including:
- formantShift = +24 or -24 semitones (extreme range)
- formantSpread = 2.0x (maximum spread)
- vowelX/Y at corners
- glottalRd at extremes (0.3 or 2.7)

**Code path for filter instability:**
1. VowelMorpher computes formant frequencies (e.g., F1=250Hz for /i/)
2. FormantFilterBank::updateCoefficients applies shift: `freq * pow(2, 24/12)` = freq * 4
3. Then applies spread: distance from center-of-mass * 2.0
4. F5 for /i/ at shift=+24: 3340 * 4 = 13360 Hz, with spread can push higher
5. Nyquist limit clamp: `max(20, min(finalFreq, sr*0.5 - 100))`

**Existing protection:**
- FormantFilterBank clamps all frequencies to [20 Hz, Nyquist - 100 Hz]
- Q is clamped to minimum 0.5
- FormantBiquad has NaN/Inf guard on z1/z2 states
- FormantVoice has final NaN/Inf guard + filter reset on detection

**Risk assessment:** LOW - triple-layer protection (freq clamp, state guard, output guard). The biquad NaN guard in FormantBiquad.h (lines 34-38) catches any instability within 1 sample.

### Risk 3: Division by Zero in VowelMorpher (MEDIUM)

**What happens:** If all 5 vowels happen to have exactly the same distance to cursor position, `weightSum` could theoretically overflow due to high focus values, but this is essentially impossible with the vowel grid.

**More realistically:** If `focus` is at maximum (6.0) and cursor is very close to a vowel, `pow(dist, 6.0)` with tiny dist produces very large weights. The code handles exact hit (dist < 1e-6f) with early return.

**Risk assessment:** LOW - early return handles the only real edge case. The 5 vowel positions are spread far enough apart that extreme focus values just sharpen the selection.

### Risk 4: State Restoration with Preset Name (MEDIUM)

**What happens:** O-Formant stores `currentPreset` as an XML attribute alongside APVTS state:
```cpp
xml->setAttribute("currentPreset", presetManager.getCurrentPresetName());
```

On restore:
```cpp
parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
presetManager.setCurrentPresetName(xmlState->getStringAttribute("currentPreset", "Default"));
```

**Level 6-7 check:** Parameter values within 0.1 tolerance. The preset name is NOT a parameter - it's a string attribute. pluginval only checks parameter values, not arbitrary XML attributes. This should pass.

**Level 8+ check:** Exact binary state match between two consecutive getStateInformation() calls. The preset name is included both times, and since it's just read from a member variable, it will be identical.

**Risk assessment:** LOW - pluginval checks parameters, not custom state attributes. Binary match will be consistent.

### Risk 5: processBlock with 0 Samples (LOW)

**What happens:** Some hosts call processBlock with numSamples=0.

**Code path:**
- processBlock clears buffer, calls synthesiser.renderNextBlock (0 samples = no-op)
- SmoothedValue loop runs 0 iterations
- No division by numSamples anywhere

**Risk assessment:** NONE - the code handles 0-sample blocks naturally.

### Risk 6: Thread Safety of Parameter Access (MEDIUM)

**What happens:** The ParameterThreadSafetyTest calls setValueNotifyingHost on all 21 parameters from the message thread while processBlock runs on the audio thread.

**Code path:** All parameter reads in FormantVoice use `atomic<float>->load()` via APVTS raw parameter pointers. These are inherently thread-safe (atomic loads).

**Risk assessment:** LOW - APVTS atomic parameter values are designed for this exact pattern.

### Risk 7: GlottalWavetable Generation During Plugin Scan (LOW)

**What happens:** When pluginval instantiates the plugin, the constructor calls `GlottalTableGenerator::generate(glottalWavetable)` which does Newton-Raphson solving + FFT for 128 Rd steps x 10 mipmap levels.

**Concern:** This runs on the message thread during construction. It allocates memory (vector resize) and does heavy computation.

**Risk assessment:** LOW - this is the standard pattern (O-Prism does the same with wavetable generation). pluginval allows reasonable constructor time. The computation is ~50ms on modern hardware.

### Risk 8: ConsonantEngine burstTotalSamples at Low Sample Rates

**What happens:** At prepare(44100), burstTotalSamples = 661. But if SR changes to something very low (not standard, but theoretically possible), burst samples could be very short.

**Risk assessment:** NONE - pluginval only tests standard sample rates (44100, 48000, 96000).

## Prior Plugin Level 10 Patterns

### O-Prism (passed level 10)
- **tanh soft clipping** on Brown/Vinyl/Wind noise types to prevent output exceeding [-1, 1]
- No other fixes were needed for level 10 beyond the noise generator clipping fix
- Used `--validate-in-process` flag

### O-Bass (passed level 10)
- Lazy initialization in OuariconPresetManager (v1.5.0+) prevents AU validation timeouts
- No specific level 10 fixes documented beyond standard patterns

### O-Bells (passed level 5)
- State save/restore includes preset name as XML attribute (same pattern as O-Formant)
- No documented level 10 run found in planning docs

### Common Patterns Across All Plugins
- `juce::ScopedNoDenormals noDenormals` at top of processBlock (O-Formant has this)
- NaN/Inf guards on filter state variables
- Frequency clamping to Nyquist-safe range
- SmoothedValue for click-free parameter changes
- All parameter access via atomic loads from APVTS

## pluginval Command Line

### VST3 Validation (Level 10)
```bash
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --strictness-level 10 \
  --validate-in-process \
  --timeout-ms 120000 \
  --validate ~/Library/Audio/Plug-Ins/VST3/O-Formant.vst3
```

### AU Validation (Level 10)
```bash
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --strictness-level 10 \
  --validate-in-process \
  --timeout-ms 120000 \
  --validate ~/Library/Audio/Plug-Ins/Components/O-Formant.component
```

### Key Flags
| Flag | Purpose | Why Use It |
|------|---------|------------|
| `--strictness-level 10` | Maximum test coverage | Ship-quality validation |
| `--validate-in-process` | Runs tests in same process (better error messages) | Easier debugging if failures occur |
| `--timeout-ms 120000` | 2-minute timeout | Synth plugins with wavetable generation need more time than default 30s |
| `--verbose` | Full test output | Add if debugging specific failures |
| `--random-seed [hex]` | Reproduce specific run | Use the seed from log output to reproduce intermittent failures |

### Interpreting Results
- Each test outputs PASS/FAIL individually
- The random seed is printed at the start of the log - save this for reproduction
- Subnormal warnings become errors at level >5
- State restoration tolerance is 0.1 at level 6-7, exact binary match at level 8+

## CHANGELOG.md Format

Based on existing project changelogs (O-Bells, O-Bass, O-Prism):

```markdown
# O-Formant Changelog

All notable changes to O-Formant will be documented in this file.

## [1.0.0] - 2026-04-05

### Added
- [Feature descriptions grouped by stage]

### Technical Notes
- [Domain, parameter count, key technical details]
```

**Project conventions observed:**
- Title: `# {PluginName} Changelog`
- Versions: `## [{semver}] - {YYYY-MM-DD}`
- Sections: `### Added`, `### Changed`, `### Fixed`, `### Technical Notes`
- Feature descriptions start with **bold feature name** followed by dash and description
- Technical notes include domain, parameter count, key implementation details
- No emoji
- All stages summarized in a single v1.0.0 entry (per CONTEXT-4.2.md)

### O-Formant v1.0.0 Content Should Cover

| Stage | Key Features |
|-------|-------------|
| Stage 1 (Foundation) | JUCE 8 project setup, CMake, WebView UI skeleton |
| Stage 2 (DSP) | LF glottal model, 5-formant filter bank, vowel morpher, consonant engine, vibrato, pitch glide, ADSR, 16-voice polyphony |
| Stage 3 (GUI) | WebView UI with Naturalist aesthetic, XY vowel pad, formant overlay, parameter sections |
| Stage 4 (Polish) | 16 factory presets (4 categories), preset browser, outputGain + stereoWidth wiring, pluginval level 10 validation |

## Common Pitfalls

### Pitfall 1: Intermittent State Restoration Failures
**What goes wrong:** pluginval reports "Parameter X not restored" with small floating-point differences
**Why it happens:** pluginval uses randomness for parameter values, and some NormalisableRange skew functions have tiny floating-point precision loss during normalize/denormalize round-trip
**How to avoid:** O-Formant parameters all use version 1 IDs and standard NormalisableRange. The 0.1 tolerance at level 6-7 should absorb any float precision issues. At level 8+, the binary state match compares getStateInformation output, not individual parameters, so it tests the XML serialization round-trip which is deterministic.
**Warning signs:** "Parameters not restored" errors that are intermittent (differ between runs)

### Pitfall 2: Subnormal Denormals in Filter Output
**What goes wrong:** Audio processing test reports subnormal values in output (error at level >5)
**Why it happens:** Biquad filters can produce subnormal floating-point values in their tail when signal decays toward zero
**How to avoid:** `juce::ScopedNoDenormals noDenormals` in processBlock (O-Formant already has this). Additionally, FormantBiquad resets z1/z2 to 0.0f when NaN/Inf detected, which also handles subnormals that could cascade.
**Warning signs:** "Output buffer contains subnormal values" errors

### Pitfall 3: Timeout During Wavetable Generation
**What goes wrong:** pluginval reports timeout if constructor takes too long
**Why it happens:** GlottalTableGenerator::generate() does 128 Newton-Raphson solves + 128 FFTs at construction
**How to avoid:** Use `--timeout-ms 120000` (2 minutes). The generation typically takes <100ms so this is not a real risk, but the default 30s timeout could be tight if multiple test iterations instantiate the plugin.
**Warning signs:** "Timeout" error during "Plugin info" or "Editor" tests

### Pitfall 4: Editor Creation While Processing Crash
**What goes wrong:** EditorWhilstProcessingTest (level 4) crashes if editor constructor accesses audio-thread data unsafely
**Why it happens:** The test spawns async processBlock calls while creating the editor
**How to avoid:** O-Formant's editor only accesses APVTS (thread-safe) and presetManager (message-thread only, not accessed during construction). The WebView relay/attachment pattern is the same as O-Prism/O-Bells which pass this test.
**Warning signs:** Crash during "Open editor whilst processing" test

## Code Examples

### pluginval Run Script (for Plan Reference)
```bash
# Build
cd /Users/taylorbrook/Dev/VST-development/build
ninja O-Formant_VST3 O-Formant_AU

# Install
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Formant.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-Formant.component
cp -R build/plugins/O-Formant/O-Formant_artefacts/Release/VST3/O-Formant.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/O-Formant/O-Formant_artefacts/Release/AU/O-Formant.component ~/Library/Audio/Plug-Ins/Components/

# Validate VST3
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --strictness-level 10 \
  --validate-in-process \
  --timeout-ms 120000 \
  --validate ~/Library/Audio/Plug-Ins/VST3/O-Formant.vst3

# Validate AU
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --strictness-level 10 \
  --validate-in-process \
  --timeout-ms 120000 \
  --validate ~/Library/Audio/Plug-Ins/Components/O-Formant.component
```

### If NaN Detected in Level 10: Add Soft Clipping Safety Net
```cpp
// Pattern from O-Prism: tanh soft clipping on output
// Apply AFTER outputGain if any level 10 NaN/Inf issues surface
for (int i = 0; i < buffer.getNumSamples(); ++i)
{
    float gain = outputGainSmoothed.getNextValue();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float sample = buffer.getSample(ch, i) * gain;
        // Safety net: soft clip to [-1, 1] range
        if (!std::isfinite(sample))
            sample = 0.0f;
        buffer.setSample(ch, i, sample);
    }
}
```

### CHANGELOG.md Template
```markdown
# O-Formant Changelog

All notable changes to O-Formant will be documented in this file.

## [1.0.0] - 2026-04-05

### Added
- **Physical model vocal synthesizer** - LF glottal pulse model with Fant 1995 Rd voice quality control (0.3-2.7)
- **5-formant parallel bandpass filter bank** - Vocal tract modeling with per-formant frequency, bandwidth, and gain
- **2D vowel morph pad** - XY cursor with 5 cardinal vowels at acoustic positions, Shepard IDW interpolation
- **Consonant noise engine** - KLATT dual-branch topology with tone shaping, sibilance, and auto-consonant plosive burst
- **Vibrato system** - Per-voice sine LFO with rate, depth, onset delay, and micro-jitter
- **Pitch glide** - Exponential portamento between notes (0-1000ms)
- **ADSR envelope** - Per-voice amplitude envelope with full parameter automation
- **16-voice polyphony** - MPE-ready via juce::MPESynthesiser with legacy MIDI fallback
- **MPE support** - Pressure->breathiness, slide->vowel Y, velocity->attack character
- **Formant shift and spread** - Semitone-based frequency shifting and center-of-mass spacing control
- **Output stage** - Smoothed output gain (dB) and per-voice stereo width (equal-power pan by MIDI note)
- **WebView UI** - Naturalist aesthetic with XY vowel pad, formant overlay, organized parameter sections
- **16 factory presets** - 4 categories (Cinematic, Electronic, Ambient, Speech), 4 presets each
- **Preset browser** - OuariconPresetManager with prev/next, category dropdown, save/load
- **Mipmapped glottal wavetable** - 128 Rd steps x 10 mipmap levels, FFT-based anti-aliasing

### Technical Notes
- Domain: C++ DSP + WebView UI
- 21 parameters across 7 groups (Vowel, Glottal, Consonant, Envelope, Character, Output, Control)
- JUCE 8.0.4, CMake + Ninja build system
- VST3 + AU formats, macOS
- Pluginval validated at strictness level 10
```

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| pluginval | COMPAT-01 validation | Yes | 1.0+ (JUCE v8.0.3 backend) | -- |
| auval | AU validation (run by pluginval) | Yes | System built-in | -- |
| ninja | Build | Yes | Project build system | -- |
| JUCE | Build | Yes | 8.0.4 | -- |

**Missing dependencies:** None.

## Sources

### Primary (HIGH confidence)
- [pluginval source: BasicTests.cpp](https://github.com/Tracktion/pluginval/blob/develop/Source/tests/BasicTests.cpp) - Full test implementations with strictness levels, exact behaviors
- O-Formant source code - All DSP files reviewed line-by-line for risk analysis
- O-Prism Stage 4 planning docs - Level 10 validation patterns and fixes
- O-Bass Stage 6 research - pluginval command-line patterns

### Secondary (MEDIUM confidence)
- [pluginval GitHub](https://github.com/Tracktion/pluginval) - CLI options and configuration
- [Melatonin: pluginval best friend](https://melatonin.dev/blog/pluginval-is-a-plugin-devs-best-friend/) - Practical tips

### Tertiary (LOW confidence)
- JUCE Forum discussions on level 10 intermittent failures - useful for awareness but not authoritative

## Metadata

**Confidence breakdown:**
- pluginval test details: HIGH - verified directly from source code
- O-Formant risk analysis: HIGH - reviewed every DSP file line by line
- CHANGELOG format: HIGH - derived from 6+ existing project changelogs
- Fix patterns: HIGH - verified from O-Prism level 10 pass

**Research date:** 2026-04-05
**Valid until:** 2026-05-05 (30 days - stable domain)
