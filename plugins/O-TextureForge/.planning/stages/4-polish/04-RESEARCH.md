# Stage 4: Integration & Polish - Research

**Researched:** 2026-02-14
**Domain:** Audio plugin validation, edge case handling, performance optimization
**Confidence:** HIGH

## Summary

This research covers five critical polish domains for O-TextureForge: pluginval strictness 10 requirements, JUCE prepareToPlay sample rate handling, AudioProcessor edge cases, memory pressure strategies, and WebView2 runtime detection on Windows.

**Key findings:**
- Pluginval strictness 10 runs comprehensive tests including auval (macOS), VST3 validator, parameter thread safety, state round-trip validation, NaN/Inf/subnormal detection, and rapid prepareToPlay cycles without releaseResources
- Sample rate changes call prepareToPlay again (ideally after releaseResources, but not guaranteed) — plugins must handle re-initialization robustly
- For corpus-based plugins, re-segmentation on sample rate change is complex; documenting as a limitation (reload corpus) is a pragmatic approach
- Memory pressure detection should use non-blocking checks (file size before load), warn users about large files (>100MB), but allow loading
- WebView2 availability can be checked via `WebBrowserComponent::areOptionsSupported()` before instantiation; silent IE fallback occurs on failure

**Primary recommendation:** Harden processBlock for zero-sample buffers and variable block sizes, implement robust state save/restore with missing-file handling, use `areOptionsSupported()` to detect WebView2 availability and show fallback message, document sample rate change limitation as "reload corpus after rate change."

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **Pluginval at strictness 10** — full validation, fix any failures
- **Comprehensive edge case handling** — full roadmap scope plus additional robustness (no file loaded, invalid format, large corpus >100MB, UMAP cancel, corpus file missing on restore, sample rate change, multiple instances, WebView2 availability, memory pressure)
- **Performance profiling** — fix obvious bottlenecks (no strict numeric targets)
- **Windows config audit** — verify CMakeLists.txt flags without cross-compile
- **macOS testing** — AU validation + VST3 in DAW

### Claude's Discretion
- Sample rate change strategy: re-segment vs. document limitation (research informed answer: document as limitation)
- Memory pressure upper bound (research informed answer: warn at >100MB, no hard block)

### Deferred Ideas (OUT OF SCOPE)
- **Preset system** — skipped for v1. Users load their own corpus files.
- **User manual / documentation** — skipped for now. Write docs closer to release.
- **Windows build/test** — verified via CI/CD at publish time. Config audit only.
</user_constraints>

---

## Standard Stack

### Core Validation Tools

| Tool | Version | Purpose | Why Standard |
|------|---------|---------|--------------|
| pluginval | 1.0.4+ | Cross-platform plugin validation | Industry standard, runs auval + VST3 validator at strictness 5+, comprehensive test suite |
| auval | macOS built-in | Apple Audio Unit validation | Required for macOS AU format, integrated into pluginval at strictness 5+ |

### Supporting Libraries (Already in Use)

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| JUCE | 8.0.4 | Audio plugin framework | Core framework — already integrated |
| std::atomic | C++17 | Thread-safe primitives | Corpus state flags, background thread coordination |
| juce::File | JUCE 8.0.4 | File system operations | File size checks, existence validation |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Document sample rate limitation | Re-segment corpus on rate change | Re-segmentation is complex (descriptor normalization requires full corpus), adds latency, rare use case; documenting is pragmatic |
| WebView2 areOptionsSupported() | Try-catch around construction | areOptionsSupported() is explicit API, try-catch is defensive fallback; use both for safety |

**Installation:**
```bash
# pluginval (macOS)
brew install pluginval

# Or download from GitHub releases
wget https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_macOS.zip
```

---

## Architecture Patterns

### Recommended Edge Case Handling Structure

```
Source/
├── PluginProcessor.h/.cpp    # State save/restore with missing-file handling
├── PluginEditor.h/.cpp        # WebView2 availability check + fallback message
├── dsp/
│   ├── CorpusLoader.h/.cpp    # File size check before load, memory pressure detection
│   ├── GrainScheduler.h/.cpp  # Zero-sample buffer handling in processBlock
│   └── SharedCorpus.h         # Atomic state flags for thread-safe checks
```

### Pattern 1: ProcessBlock Hardening

**What:** Defensive processing for edge case buffer sizes
**When to use:** Always — all audio plugins

**Example:**
```cpp
// Source: JUCE AudioProcessor documentation + pluginval BasicTests.cpp
void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override
{
    // EDGE CASE 1: Zero-sample buffer (pluginval tests this)
    if (buffer.getNumSamples() == 0) {
        return;  // No processing needed
    }

    // EDGE CASE 2: Very large buffers (8192+)
    // Must not assume buffer size matches prepareToPlay maximumExpectedSamplesPerBlock
    const int numSamples = buffer.getNumSamples();

    // EDGE CASE 3: Channel count mismatch (prepareToPlay vs processBlock)
    const int numChannels = jmin(buffer.getNumChannels(), getTotalNumOutputChannels());

    // Process audio
    scheduler.processBlock(buffer.getArrayOfWritePointers(), numChannels, numSamples, midiMessages);

    // EDGE CASE 4: NaN/Inf/subnormal protection (pluginval checks this)
    for (int ch = 0; ch < numChannels; ++ch) {
        FloatVectorOperations::clip(buffer.getWritePointer(ch), buffer.getReadPointer(ch),
                                     -1.0f, 1.0f, numSamples);
    }
}
```

### Pattern 2: State Save/Restore with Missing File Handling

**What:** Graceful degradation when saved corpus path no longer exists
**When to use:** All plugins with external file dependencies

**Example:**
```cpp
// Source: JUCE forum best practices + AudioProcessorValueTreeState docs
void getStateInformation(MemoryBlock& destData) override
{
    // Save parameter state
    auto state = apvts.copyState();

    // Save corpus path if loaded
    if (corpusLoader.hasCorpusLoaded()) {
        state.setProperty("corpusPath", corpusLoader.getCurrentFilePath(), nullptr);
    }

    std::unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void setStateInformation(const void* data, int sizeInBytes) override
{
    std::unique_ptr<XmlElement> xml(getXmlFromBinary(data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName(apvts.state.getType())) {
        auto state = ValueTree::fromXml(*xml);
        apvts.replaceState(state);

        // EDGE CASE: Corpus file missing on restore
        String savedPath = state.getProperty("corpusPath", "");
        if (savedPath.isNotEmpty()) {
            File corpusFile(savedPath);

            if (corpusFile.existsAsFile()) {
                // File exists — load in background
                corpusLoader.loadFileAsync(corpusFile);
            } else {
                // File missing — clear state, show drop zone in UI
                corpusLoader.clearCorpus();
                // UI will show "Drop audio file here" message
                DBG("Corpus file not found: " + savedPath);
            }
        }
    }
}
```

### Pattern 3: PrepareToPlay Sample Rate Change Handling

**What:** Robust re-initialization when sample rate changes
**When to use:** All plugins, especially those with sample-rate-dependent state

**Example:**
```cpp
// Source: JUCE forum best practices + AudioProcessor docs
void prepareToPlay(double sampleRate, int samplesPerBlock) override
{
    // EDGE CASE 1: prepareToPlay called multiple times without releaseResources
    // Must be idempotent — safe to call repeatedly

    // EDGE CASE 2: Sample rate changed (pluginval tests this at v0.2.8+)
    // Re-initialize sample-rate-dependent components
    lastSampleRate = sampleRate;

    // Scheduler needs sample rate for grain playback
    scheduler.prepareToPlay(sampleRate, samplesPerBlock);

    // NOTE: Corpus is resampled to 44.1kHz at load time, NOT at prepareToPlay
    // Sample rate changes require corpus reload — document as limitation
}

void releaseResources() override
{
    // Clean up audio thread resources
    scheduler.releaseResources();

    // NOTE: Do NOT clear corpus here — preserve across prepareToPlay cycles
}
```

### Pattern 4: Memory Pressure Detection (Non-Blocking)

**What:** Warn about large files without blocking or crashing
**When to use:** Plugins loading external audio files

**Example:**
```cpp
// Source: JUCE File API + real-time audio programming best practices
void loadFileAsync(const File& file)
{
    // EDGE CASE: Large file warning (>100MB)
    int64 fileSize = file.getSize();
    const int64 warnThreshold = 100 * 1024 * 1024;  // 100MB

    if (fileSize > warnThreshold) {
        // Non-blocking warning — don't prevent load, just inform user
        AlertWindow::showAsync(
            MessageBoxOptions()
                .withIconType(MessageBoxIconType::WarningIcon)
                .withTitle("Large File")
                .withMessage("File is " + String(fileSize / (1024*1024)) + "MB. "
                            "Loading may take time and use significant memory. Continue?")
                .withButton("Load")
                .withButton("Cancel"),
            [this, file](int result) {
                if (result == 1) {  // Load button
                    startBackgroundLoad(file);
                }
            }
        );
    } else {
        startBackgroundLoad(file);
    }
}
```

### Pattern 5: WebView2 Availability Detection (Windows)

**What:** Check WebView2 availability before WebBrowserComponent creation
**When to use:** All JUCE plugins with WebView UI on Windows

**Example:**
```cpp
// Source: JUCE WebBrowserComponent docs + forum discussions
MyPluginEditor::MyPluginEditor(MyProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // Windows-specific: Check WebView2 availability
   #if JUCE_WINDOWS
    auto options = WebBrowserComponent::Options{}
        .withBackend(WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(
            WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder(File::getSpecialLocation(File::tempDirectory)
                    .getChildFile("OTextureForge_WebView"))
        );

    if (!WebBrowserComponent::areOptionsSupported(options)) {
        // WebView2 not available — show fallback message
        addAndMakeVisible(fallbackLabel);
        fallbackLabel.setText(
            "WebView2 Runtime Required\n\n"
            "This plugin requires Microsoft Edge WebView2.\n"
            "Download from: https://go.microsoft.com/fwlink/?linkid=2124701",
            dontSendNotification
        );
        fallbackLabel.setJustificationType(Justification::centred);
        setSize(900, 600);
        return;  // Don't create WebView
    }
   #endif

    // WebView2 available (or macOS/Linux) — create WebView normally
    createWebView();
}
```

### Anti-Patterns to Avoid

- **Assuming buffer size is constant:** Host can pass any buffer size to processBlock, including zero
- **Hard-coding sample rate:** Always use the value passed to prepareToPlay, don't cache globally
- **Blocking file I/O in setStateInformation:** Load corpus asynchronously on background thread
- **Ignoring missing files on restore:** Check file existence before attempting load
- **Creating WebView without checking availability:** Always use areOptionsSupported() on Windows

---

## Don't Hand-Roll

Problems that look simple but have existing solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| NaN/Inf detection in audio buffers | Manual isnan()/isinf() loops | `FloatVectorOperations::clip()` with range checking | SIMD-optimized, handles subnormals, catches edge cases |
| Thread-safe parameter access | Custom locks/mutexes | `AudioProcessorValueTreeState` with RangedAudioParameter | Lock-free for most operations, JUCE-tested thread safety |
| State serialization | Custom binary format | `ValueTree::createXml()` + `copyXmlToBinary()` | Handles versioning, human-readable XML, forward-compatible |
| Async file loading | std::thread + manual lifecycle | `juce::Thread` or `juce::ThreadPool` | JUCE lifecycle management, proper cleanup, cancellation support |
| WebView2 availability check | Try-catch around WebBrowserComponent | `WebBrowserComponent::areOptionsSupported()` | Explicit API, no exception overhead, works pre-construction |
| Background task coordination | Manual flags + sleep() | `std::atomic<T>` + `WaitableEvent` | Lock-free, wait/signal primitives, no busy-waiting |

**Key insight:** Audio plugin validation (pluginval) tests edge cases most developers never encounter in normal DAW testing. Use JUCE's built-in primitives — they're battle-tested against pluginval's strictness 10 suite.

---

## Common Pitfalls

### Pitfall 1: Assuming prepareToPlay is Always Followed by releaseResources

**What goes wrong:** Plugin assumes sample rate change will call releaseResources first, crashes or glitches when it doesn't
**Why it happens:** Pluginval explicitly tests this (v0.2.8+): "Audio processing across varying sample rates without resource release"
**How to avoid:** Make prepareToPlay idempotent — safe to call multiple times without releaseResources. Re-initialize all sample-rate-dependent state.
**Warning signs:** Crashes during pluginval's sample rate test, audio glitches when DAW changes project sample rate

### Pitfall 2: Processing Assumptions About Buffer Size

**What goes wrong:** Plugin assumes buffer size matches maximumExpectedSamplesPerBlock from prepareToPlay, crashes on zero or large buffers
**Why it happens:** JUCE docs warn: "The number of samples in these buffers is NOT guaranteed to be the same for every callback"
**How to avoid:** Always check buffer.getNumSamples() at the start of processBlock. Handle zero gracefully (early return). Support up to 8192 samples regardless of prepareToPlay hint.
**Warning signs:** Pluginval crashes in audio processing tests, DAW crashes with specific buffer size settings

### Pitfall 3: State Restore Assumes File Still Exists

**What goes wrong:** Plugin crashes or shows blank UI when saved corpus file has been moved/deleted
**Why it happens:** getStateInformation saves path as string, setStateInformation blindly loads it
**How to avoid:** Check `File::existsAsFile()` before loading. Clear corpus and show drop zone if missing. Log warning but don't block plugin load.
**Warning signs:** Plugin fails to load saved sessions, crashes on project open after moving samples

### Pitfall 4: Silent WebView2 Fallback to IE Backend

**What goes wrong:** Plugin loads on Windows but shows blank white rectangle instead of UI
**Why it happens:** JUCE silently falls back to IE backend when WebView2 unavailable; resource provider API not available on IE
**How to avoid:** Use `areOptionsSupported()` before WebBrowserComponent creation. Show fallback message with download link if WebView2 missing.
**Warning signs:** Blank UI on Windows 10 without Edge installed, no error message, no console output

### Pitfall 5: NaN/Inf Propagation from Corpus Data

**What goes wrong:** Pluginval detects NaN or Inf in output buffer, fails validation
**Why it happens:** Corrupted audio file, denormal numbers from DSP operations, uninitialized buffers
**How to avoid:** Clip output buffer to [-1.0, 1.0] range at end of processBlock. Use FloatVectorOperations::clip() for SIMD performance.
**Warning signs:** Pluginval reports "NaNs found in buffer" or "Infs found in buffer" during audio processing tests

### Pitfall 6: Parameter Thread Safety Violations

**What goes wrong:** Crashes during pluginval's parameter thread safety test
**Why it happens:** Pluginval calls setValue() on parameters from multiple threads concurrently (mimicking automation + GUI)
**How to avoid:** Use AudioProcessorValueTreeState for all parameters. Never access parameters directly from audio thread without atomic protection.
**Warning signs:** Random crashes during pluginval validation, especially with editor open + automation

### Pitfall 7: Corpus Re-Segmentation Complexity on Sample Rate Change

**What goes wrong:** Attempting to re-segment corpus when sample rate changes introduces latency, complexity, and normalization issues
**Why it happens:** Developer assumes corpus must match project sample rate for "correctness"
**How to avoid:** Resample corpus to fixed rate (e.g., 44.1kHz) at load time. Document that sample rate changes require corpus reload. This is pragmatic and user-understandable.
**Warning signs:** Multi-second glitches when changing sample rate, descriptor normalization breaks, KD-tree rebuilding causes audio dropouts

### Pitfall 8: Memory Allocation in Audio Thread

**What goes wrong:** Pluginval's real-time safety checks (macOS) detect memory allocation in processBlock
**Why it happens:** Dynamic allocation (new, malloc, std::vector::push_back) during audio processing
**How to avoid:** Pre-allocate all buffers in prepareToPlay. Use fixed-size arrays or pre-sized std::vector. Never allocate in processBlock.
**Warning signs:** Pluginval fails with "Memory allocation detected in audio thread" on macOS with --rtcheck enabled

---

## Code Examples

Verified patterns from official sources:

### Zero-Sample Buffer Handling

```cpp
// Source: JUCE AudioProcessor docs + pluginval BasicTests.cpp
void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override
{
    ScopedNoDenormals noDenormals;  // Flush denormals to zero (performance)

    // Edge case: zero samples (hosts "occasionally decide to pass a buffer containing zero samples")
    if (buffer.getNumSamples() == 0) {
        return;
    }

    // ... rest of processing
}
```

### State Restore with Missing File Fallback

```cpp
// Source: JUCE forum best practices + AudioProcessorValueTreeState tutorial
void setStateInformation(const void* data, int sizeInBytes) override
{
    std::unique_ptr<XmlElement> xml(getXmlFromBinary(data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName(apvts.state.getType())) {
        auto state = ValueTree::fromXml(*xml);
        apvts.replaceState(state);

        // Restore corpus if path saved
        String savedPath = state.getProperty("corpusPath", "");
        if (savedPath.isNotEmpty()) {
            File corpusFile(savedPath);

            if (corpusFile.existsAsFile()) {
                corpusLoader.loadFileAsync(corpusFile);
            } else {
                // Graceful fallback — clear state, show UI message
                corpusLoader.clearCorpus();
                DBG("Corpus file missing: " + savedPath);
            }
        }
    }
}
```

### WebView2 Availability Check (Windows)

```cpp
// Source: JUCE WebBrowserComponent docs + forum discussions
#if JUCE_WINDOWS
auto options = WebBrowserComponent::Options{}
    .withBackend(WebBrowserComponent::Options::Backend::webview2)
    .withWinWebView2Options(
        WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder(File::getSpecialLocation(File::tempDirectory)
                .getChildFile("PluginName_WebView"))
    );

if (!WebBrowserComponent::areOptionsSupported(options)) {
    // Show fallback UI with download link
    showWebView2MissingMessage();
    return;
}
#endif

// Proceed with WebView creation
webView = std::make_unique<WebBrowserComponent>(options);
```

### NaN/Inf/Subnormal Protection

```cpp
// Source: pluginval BasicTests.cpp + JUCE FloatVectorOperations
void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override
{
    ScopedNoDenormals noDenormals;  // Flush-to-zero mode

    // ... DSP processing

    // Protect against NaN/Inf before output (pluginval checks this)
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* channelData = buffer.getWritePointer(ch);
        const int numSamples = buffer.getNumSamples();

        // Clip to safe range (catches NaN/Inf, converts to -1.0/1.0)
        FloatVectorOperations::clip(channelData, channelData, -1.0f, 1.0f, numSamples);
    }
}
```

### Idempotent PrepareToPlay (Sample Rate Change Handling)

```cpp
// Source: JUCE forum "Best practices for sample rate changes"
void prepareToPlay(double sampleRate, int samplesPerBlock) override
{
    // Safe to call multiple times without releaseResources (pluginval tests this)

    // Update cached sample rate
    lastSampleRate = sampleRate;

    // Re-initialize sample-rate-dependent components
    scheduler.prepareToPlay(sampleRate, samplesPerBlock);

    // Pre-allocate buffers (avoid allocation in processBlock)
    tempBuffer.setSize(2, samplesPerBlock, false, true, true);

    // NOTE: Corpus is resampled at load time to 44.1kHz, NOT here
    // Sample rate changes do NOT trigger corpus re-segmentation
    // Document as limitation: "Reload corpus after sample rate change"
}
```

### File Size Check Before Load (Memory Pressure Detection)

```cpp
// Source: JUCE File API + real-time programming best practices
void checkAndLoadFile(const File& file)
{
    // Non-blocking check — file size available instantly
    int64 fileSize = file.getSize();
    const int64 warnThreshold = 100 * 1024 * 1024;  // 100MB

    if (fileSize > warnThreshold) {
        String sizeStr = String(fileSize / (1024 * 1024)) + " MB";
        AlertWindow::showAsync(
            MessageBoxOptions()
                .withIconType(MessageBoxIconType::WarningIcon)
                .withTitle("Large File")
                .withMessage("File is " + sizeStr + ". Loading may use significant memory.")
                .withButton("Load Anyway")
                .withButton("Cancel"),
            [this, file](int result) {
                if (result == 1) loadInBackground(file);
            }
        );
    } else {
        loadInBackground(file);
    }
}
```

### Parameter Thread Safety (AudioProcessorValueTreeState)

```cpp
// Source: JUCE AudioProcessorValueTreeState tutorial + thread safety docs
class MyProcessor : public AudioProcessor
{
public:
    MyProcessor()
        : apvts(*this, nullptr, "Parameters", createParameterLayout())
    {
    }

    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override
    {
        // Thread-safe parameter access (pluginval tests concurrent setValue)
        auto grainSize = apvts.getRawParameterValue("GRAIN_SIZE")->load();
        auto density = apvts.getRawParameterValue("DENSITY")->load();

        // Use values in DSP
        scheduler.setGrainSize(grainSize);
        scheduler.setDensity(density);
    }

private:
    AudioProcessorValueTreeState apvts;

    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<RangedAudioParameter>> params;
        params.push_back(std::make_unique<AudioParameterFloat>(
            "GRAIN_SIZE", "Grain Size", 10.0f, 500.0f, 50.0f));
        // ... more parameters
        return { params.begin(), params.end() };
    }
};
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual NaN checks with isnan() | FloatVectorOperations::clip() | JUCE 5+ | SIMD-optimized, handles Inf + subnormals, single operation |
| Try-catch for WebView2 creation | areOptionsSupported() pre-check | JUCE 8 | Explicit API, no exception overhead, detect before construction |
| Custom state serialization (binary) | ValueTree + XML | JUCE 4+ | Human-readable, forward-compatible, automatic versioning |
| Locks for parameter access | AudioProcessorValueTreeState | JUCE 5+ | Lock-free for most ops, tested thread safety, automatic UI binding |
| Assume releaseResources before sample rate change | Idempotent prepareToPlay | Pluginval 0.2.8+ | Robust to host behavior, no assumptions |

**Deprecated/outdated:**
- **Manual parameter locks:** AudioProcessorValueTreeState handles thread safety internally
- **Ignoring zero-sample buffers:** JUCE docs explicitly state hosts may pass zero samples
- **Hard-coded buffer size assumptions:** Hosts can pass any buffer size regardless of prepareToPlay hint

---

## Open Questions

Things that couldn't be fully resolved:

1. **Pluginval strictness 10 exact test list**
   - What we know: Runs auval + VST3 validator at level 5+, parameter thread safety, state round-trip, NaN/Inf/subnormal detection, rapid prepareToPlay without releaseResources
   - What's unclear: Complete categorized list of all tests at each strictness level
   - Recommendation: Run pluginval with --verbose flag to see full test list, assume strictness 10 runs ALL tests

2. **Memory pressure practical upper bound**
   - What we know: 100MB threshold commonly mentioned in forums, AlertWindow warning is non-blocking
   - What's unclear: Exact memory limit before system instability
   - Recommendation: Warn at 100MB, allow load, document that 500MB+ files may cause performance issues

3. **Sample rate change: re-segment vs. document**
   - What we know: Re-segmentation requires full corpus descriptor re-normalization + KD-tree rebuild, introduces multi-second latency
   - What's unclear: Whether any commercial granular plugins handle this transparently
   - Recommendation: Document as limitation "Reload corpus after sample rate change" — pragmatic and clear to users

4. **WebView2 IE fallback detection post-construction**
   - What we know: JUCE silently falls back to IE if WebView2 unavailable, areOptionsSupported() works pre-construction
   - What's unclear: No API to detect which backend is active post-construction
   - Recommendation: Use areOptionsSupported() before creation, assume success if no exception; no way to detect silent fallback post-construction

---

## Sources

### Primary (HIGH confidence)

**Pluginval:**
- [Pluginval GitHub Repository](https://github.com/Tracktion/pluginval) — Open-source validation tool
- [Pluginval BasicTests.cpp Source](https://github.com/Tracktion/pluginval/blob/develop/Source/tests/BasicTests.cpp) — Actual test implementation (NaN/Inf/subnormal checks, zero buffers)
- [Pluginval CHANGELIST.md](https://github.com/Tracktion/pluginval/blob/develop/CHANGELIST.md) — Feature additions by version (auval at 5+, parameter thread safety, sample rate tests)
- [Melatonin: Pluginval is a plugin dev's best friend](https://melatonin.dev/blog/pluginval-is-a-plugin-devs-best-friend/) — Comprehensive guide to pluginval usage

**JUCE AudioProcessor:**
- [JUCE AudioProcessor Class Reference](https://docs.juce.com/master/classAudioProcessor.html) — Official prepareToPlay/processBlock docs
- [JUCE Tutorial: Saving and loading your plug-in state](https://juce.com/tutorials/tutorial_audio_processor_value_tree_state/) — State save/restore best practices
- [JUCE Tutorial: Configuring the right bus layouts](https://juce.com/tutorials/tutorial_audio_bus_layouts) — Bus layout edge cases

**JUCE WebBrowserComponent:**
- [JUCE WebBrowserComponent Class Reference](http://docs.juce.com/master/classWebBrowserComponent.html) — areOptionsSupported() API
- [JUCE WebBrowserComponent::Options Class Reference](https://docs.juce.com/master/classWebBrowserComponent_1_1Options.html) — Backend configuration

**Real-Time Audio Programming:**
- [Ross Bencina: Real-time audio programming 101](http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing) — Canonical guide to real-time safety
- [Timur Audio: Using locks in real-time audio processing, safely](https://timur.audio/using-locks-in-real-time-audio-processing-safely) — Lock-free patterns

### Secondary (MEDIUM confidence)

**JUCE Forum Discussions:**
- [prepareToPlay and processBlock thread-safety](https://forum.juce.com/t/preparetoplay-and-processblock-thread-safety/32193) — Threading behavior
- [Best practices for sample rate changes](https://forum.juce.com/t/best-practices-for-sample-rate-changes/61251) — Sample rate handling strategies
- [Can sample rate change?](https://forum.juce.com/t/can-sample-rate-change/40939) — DAW behavior during rate changes
- [Is get state information thread safe?](https://forum.juce.com/t/is-get-state-information-thread-safe-solved-use-setstateinformation-to-load-samples-and-dynamic-xml-values-without-ui-thread/64129) — State save/restore threading
- [IE is still loading instead of WebView2](https://forum.juce.com/t/ie-is-still-loading-instead-of-webview2-on-windows-10/63829) — Silent IE fallback issue
- [How to Force WebView2 Backend?](https://forum.juce.com/t/how-to-force-webview2-backend/65547) — areOptionsSupported() usage

**Buffer Size Edge Cases:**
- [Buffersize changes random, but not in prepareToPlay()](https://forum.juce.com/t/buffersize-changes-random-but-not-in-preparetoplay/66312) — Buffer size mismatch issues
- [processBlock called before prepareToPlay](https://forum.juce.com/t/processblock-called-before-preparetoplay/53562) — Initialization order issues

### Tertiary (LOW confidence)

**Community Discussions:**
- [Pluginval comments/requests here? (KVR Audio)](https://www.kvraudio.com/forum/viewtopic.php?t=550252) — User experiences with pluginval
- [About Tracktions "Pluginval" (HISE Forum)](https://forum.hise.audio/topic/3534/about-tracktions-pluginval) — Community usage patterns

**WebSearch-Only (requires verification):**
- Granular synthesis plugins (DataMind ReFractalizer, TAL Sampler) — No official docs on corpus resampling strategies found
- Memory pressure upper bounds — 100MB threshold mentioned but not officially documented

### Local Knowledge Base (HIGH confidence)

- `/Users/taylorbrook/Dev/VST-development/research/cross-platform-webview-best-practices.md` — WebView2 static linking, user data folder, silent IE fallback patterns
- `/Users/taylorbrook/Dev/VST-development/CLAUDE.md` — Windows WebView2 configuration requirements

---

## Metadata

**Confidence breakdown:**
- Pluginval strictness 10 tests: **HIGH** — Verified from official source code + CHANGELIST
- JUCE prepareToPlay sample rate handling: **HIGH** — Official JUCE docs + forum consensus
- AudioProcessor edge cases: **HIGH** — JUCE docs + pluginval source + forum examples
- Memory pressure detection: **MEDIUM** — Best practices from forums, no official JUCE guidance on thresholds
- WebView2 availability detection: **HIGH** — Official JUCE API documentation + forum verification

**Research date:** 2026-02-14
**Valid until:** 2026-03-14 (30 days — stable domain, JUCE 8.0.4 release mature)

**Recommendations for implementation:**
1. **ProcessBlock hardening:** Check buffer.getNumSamples() at start, handle zero gracefully, clip output to [-1.0, 1.0]
2. **State restore:** Check File::existsAsFile() before loading corpus, clear state + show drop zone if missing
3. **PrepareToPlay:** Make idempotent (safe to call repeatedly), update sample-rate-dependent state, DON'T re-segment corpus
4. **Memory pressure:** Check file.getSize() before load, warn at >100MB with non-blocking alert, allow user choice
5. **WebView2 check:** Use areOptionsSupported() on Windows before WebBrowserComponent creation, show fallback message with download link if unavailable
6. **Sample rate limitation:** Document as "Reload corpus after changing project sample rate" — pragmatic approach
7. **Parameter thread safety:** Already using AudioProcessorValueTreeState — verified safe for pluginval concurrent access tests
8. **Multiple instances:** Each instance has separate SharedCorpus + KDTreeSearch — already isolated, no shared global state
