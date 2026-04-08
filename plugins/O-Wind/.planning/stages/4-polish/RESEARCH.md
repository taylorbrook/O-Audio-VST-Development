# Stage 4: Polish - Research

## Research Date

2026-04-05

## Scope (from CONTEXT.md)

1. **pluginval level 10** (VST3 + AU) — upgrade from level 5
2. **CPU profiling** — verify <2.5% per voice at 44.1kHz (PERF-02)
3. **Latency verification** — confirm oversampling latency reported correctly (PERF-03)
4. **CC2 breath mapping verification** — standard MIDI, no physical hardware (COMPAT-03)
5. **UI placeholders** — reserved DOM elements for UI-06, UI-07, UI-08 (no implementation)
6. **Bug fixes** surfaced by pluginval level 10

---

## 1. Pluginval Level 10: Risk Analysis

### Level 5 vs Level 10 Differences

| Test Area | Level 5 | Level 10 |
|-----------|---------|----------|
| Parameter fuzz | Basic coverage | 500x random iterations |
| State save/restore | Single cycle | Multiple cycles, binary-exact matching |
| Sample rate changes | Standard test | Rapid switching (44.1k -> 96k -> 44.1k) |
| Buffer sizes | Normal | Variable sizes incl. zero-length |
| Threading stress | Basic | Simultaneous message + audio thread |
| Denormal detection | Warning | **Error** (enforced at level 6+) |

### Waveguide-Specific Risks at Level 10

| Risk | Component | Severity | Current Mitigation | Action Needed |
|------|-----------|----------|--------------------|---------------|
| Runaway oscillation from extreme parameter fuzz | JetExciter + BoreWaveguide feedback loop | HIGH | Hard clip at +/-2.0f in renderNextBlock, tanh saturation in JetNonlinearity, input clamp [-3,3] | Likely sufficient — verify with actual level 10 run |
| Denormal accumulation in filter states | DCBlocker yPrev, BoreWaveguide IIR filters, JetExciter noise filter | MEDIUM | `ScopedNoDenormals` in processBlock | ScopedNoDenormals covers FTZ/DAZ for the audio thread — filter states will flush to zero. **No fix needed.** |
| Stale DSP state on voice reuse | FluteSynthVoice silent-counter cleanup path | HIGH | `clearCurrentNote()` called but **no DSP reset** | **FIX REQUIRED** — add DSP component reset when voice clears via silent counter |
| State save/restore binary mismatch | OuariconPresetManager XML serialization | LOW | Uses APVTS replaceState/copyState | Verify with level 10; OuariconPresetManager proven on other plugins |
| Sample rate transition crash | prepareToPlay recreates voices, reinits oversampling | LOW | Full tear-down and rebuild in prepareToPlay | Safe — same pattern as O-Bowed (passes level 10) |
| Zero-length buffer | processBlock + renderNextBlock | NONE | Early returns at both levels (PluginProcessor.cpp:276, FluteSynthVoice.cpp:167) | Already handled |

### Critical Fix: Voice Cleanup Missing DSP Reset

**Location:** `FluteSynthVoice.cpp:174-181`

When a voice goes silent via the `silentSampleCount >= silentThreshold` path, only `clearCurrentNote()` is called. The DSP components (jetExciter, jetNonlinearity, dcBlocker, boreWaveguide, jetDelay) are NOT reset. Next time this voice is allocated to a new note, stale filter states from the previous note could produce artifacts or transient pops.

Compare with the `stopNote(velocity, false)` path (line 83-91) which correctly resets all components.

**Fix:** Add DSP reset before `clearCurrentNote()` in the silent-counter path:
```cpp
if (silentSampleCount >= silentThreshold)
{
    jetExciter.reset();
    jetNonlinearity.reset();
    dcBlocker.reset();
    boreWaveguide.reset();
    jetDelay.reset();
    clearCurrentNote();
    return;
}
```

### Instrument Preset Comparison Fragility

**Location:** `FluteSynthVoice.cpp:319`

Preset change detection compares only `jetGain`:
```cpp
if (std::abs (preset.jetGain - currentPreset.jetGain) > 0.001f)
```

All 8 presets currently have unique jetGain values (2.0, 1.5, 1.8, 1.3, 2.5, 1.0, 2.2, 1.6), so this works. But it's fragile. A more robust approach would compare the preset index directly:

```cpp
static int lastPresetIdx = -1;
if (presetIdx != lastPresetIdx)
{
    lastPresetIdx = presetIdx;
    currentPreset = preset;
    applyPresetCoefficients();
}
```

**Risk for level 10:** LOW — parameter fuzz sends integer values 0-7, and the current approach works. But worth fixing as a code quality improvement.

---

## 2. CPU Profiling (PERF-02)

### Expected CPU Budget

- **Model complexity:** Simple jet-drive + bore waveguide (no iterative solver like O-Bowed/O-Formant)
- **Per-sample operations:** JetExciter (Bernoulli + noise + vibrato LFO), Lagrange3rd jet delay, tanh saturation, DCBlocker, BoreWaveguide (2x Thiran delay + 3 IIR filters)
- **Oversampling:** 2x polyphase IIR (process loop runs at 2x native rate)
- **Target:** <2.5% per voice at 44.1kHz (nice-tier requirement)

### Measurement Approach

Use JUCE's `juce::Time::getHighResolutionTicks()` around the voice render loop, or simply measure total processBlock time with all 8 voices active vs idle. The simpler approach:

```bash
# Build Standalone with Release optimizations, play 8 simultaneous notes
# Monitor Activity Monitor CPU% or use Instruments.app
```

Alternatively, add a temporary `DBG` with timing in processBlock for a quick measurement, then remove.

### Expected Result

O-Wind's model is significantly simpler than O-Bowed (no Newton-Raphson solver, no body resonator). O-Bowed achieves ~2% per voice with a more complex model. O-Wind should comfortably be under 2.5%.

---

## 3. Latency Verification (PERF-03)

### Current Implementation

`PluginProcessor.cpp:253-254`:
```cpp
if (auto* firstVoice = dynamic_cast<FluteSynthVoice*>(synthesiser.getVoice(0)))
    setLatencySamples(static_cast<int>(std::ceil(firstVoice->getOversamplingLatency())));
```

`FluteSynthVoice.cpp:159-162`:
```cpp
float FluteSynthVoice::getOversamplingLatency() const
{
    return oversampling.getLatencyInSamples();
}
```

### Expected Value

2x oversampling with `filterHalfBandPolyphaseIIR` = ~8 samples latency at native rate (JUCE's standard polyphase IIR filter group delay).

### Verification

1. Check `getLatencySamples()` returns non-zero after `prepareToPlay`
2. Verify the value is consistent across sample rates (should be ~8 regardless of sample rate)
3. pluginval level 10 tests latency reporting as part of its validation suite

### Note: getLatencySamples() is Non-Virtual

Per JUCE 8 (documented in memory), `getLatencySamples()` is **not virtual** — you can't override it. The `setLatencySamples(N)` call in `prepareToPlay` is the correct approach. Already implemented correctly.

---

## 4. CC2 Breath Mapping (COMPAT-03)

### Current Implementation

`FluteSynthVoice.cpp:107-125`:
```cpp
void FluteSynthVoice::controllerMoved(int controllerNumber, int newValue)
{
    float normalized = static_cast<float>(newValue) / 127.0f;
    switch (controllerNumber)
    {
        case 2:   ccBreathPressure = normalized; break;  // CC2 -> breath
        case 74:  ccEmbouchure = normalized; break;      // CC74 -> embouchure
        case 1:   ccVibratoDepth = normalized; break;    // CC1 -> vibrato
    }
}
```

`FluteSynthVoice.cpp:276-278`:
```cpp
if (ccBreathPressure > 0.0f) breathPressure = ccBreathPressure;
if (ccEmbouchure > 0.0f) embouchure = ccEmbouchure;
if (ccVibratoDepth > 0.0f) vibratoDepth = ccVibratoDepth;
```

### Verification Approach

Without a physical wind controller, verify via:
1. **MIDI CC2 in DAW** — send CC2 automation and confirm breath pressure responds
2. **Standalone + MIDI keyboard** — route CC2 from any controller knob
3. **Code path audit** — verify `controllerMoved` is called by JUCE Synthesiser for all MIDI CC messages (it is — `Synthesiser::handleController` dispatches to all voices)

### Potential Issue: CC Override Logic

The `if (ccBreathPressure > 0.0f)` check means CC2 value 0 reverts to APVTS control. This is intentional and correct — allows APVTS breathPressure to work when no CC2 is active. However, if a wind controller sends CC2=0 to indicate "no breath," the APVTS value takes over instead of producing silence. This is acceptable for v1.0 — the APVTS value acts as a "minimum breath" floor.

---

## 5. UI Placeholder Elements

### Requirements

Reserve layout space in index.html for:
- **UI-06:** Breath/jet real-time visualization (Sound tab)
- **UI-07:** Register indicator (Sound tab)
- **UI-08:** Visual polish and animations (all tabs)

### Implementation Approach

Add hidden `<div>` elements with class names and CSS `display: none` or `visibility: hidden`. This preserves the DOM structure for future implementation without affecting current layout or functionality.

```html
<!-- Placeholder: Breath/Jet Visualization (UI-06) -->
<div id="breath-viz-placeholder" class="future-feature" style="display:none;"></div>

<!-- Placeholder: Register Indicator (UI-07) -->
<div id="register-indicator-placeholder" class="future-feature" style="display:none;"></div>
```

### Risk

NONE — hidden elements have zero visual/functional impact. pluginval doesn't inspect DOM content.

---

## 6. WebView Plugin Validation at Level 10

### Known JUCE WebView Issue

From O-Texture research: JUCE WebView plugins can crash during high-iteration editor automation tests (1000+ open/close cycles at level 6+). This is a JUCE framework limitation, not a plugin bug.

### Mitigation Strategy

**Phase 1:** Run pluginval with `--skip-gui-tests` to validate DSP in isolation:
```bash
pluginval --strictness-level 10 --skip-gui-tests --validate <VST3>
```

**Phase 2:** Run full pluginval (with GUI tests). If GUI tests fail but DSP passes, document as JUCE WebView limitation.

### O-Wind-Specific Concerns

The destructor properly resets attachments before webView (PluginEditor.cpp:521-539). The resource provider uses bare path comparison (correct pattern). WinWebView2 user data folder is set. These patterns have been validated on multiple other plugins.

---

## 7. Existing Quality Baseline

| Check | Status | Last Verified |
|-------|--------|---------------|
| Build (VST3 + AU + Standalone) | PASS | Stage 3 |
| pluginval level 5 (VST3) | PASS | Stage 3 |
| auval (AU) | PASS | Stage 3 |
| Zero build warnings | PASS | Stage 3 |
| Relay/Attachment destruction order | Correct | Stage 3 |
| WebView2 static linking | Enabled | Stage 1 |
| ScopedNoDenormals | Present | Stage 2 |
| Hard clip safety | Present (+/-2.0f) | Stage 2 |

---

## 8. Execution Plan Recommendations

### Phase 1: Pre-Validation Fixes
1. Fix voice cleanup missing DSP reset (critical for level 10)
2. Improve instrument preset comparison (robustness)
3. Add UI placeholder elements

### Phase 2: Pluginval Level 10
1. Build Release (VST3 + AU)
2. Run `--skip-gui-tests` first (DSP isolation)
3. Run full pluginval level 10
4. Fix any failures

### Phase 3: Verification
1. CPU profiling (PERF-02)
2. Latency reporting check (PERF-03)
3. CC2 code path verification (COMPAT-03)
4. Update REQUIREMENTS.md with verification results

### Estimated Scope

- **Code changes:** Small (voice cleanup fix, preset comparison, UI placeholders)
- **Risk level:** Low — O-Wind has a simpler DSP model than O-Bowed/O-Formant
- **Expected iterations:** 1-2 pluginval runs (fix-and-rerun pattern from O-Formant/O-Texture)
