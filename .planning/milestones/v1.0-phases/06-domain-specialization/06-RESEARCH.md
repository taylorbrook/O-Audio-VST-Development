# Phase 6: Domain Specialization - Research

**Researched:** 2026-01-31
**Domain:** DSP real-time safety, GUI thread safety, JUCE 8 patterns, professional audio quality, music theory
**Confidence:** HIGH

## Summary

Phase 6 encodes professional domain expertise into agents so they catch domain-specific quality issues automatically. The research confirms three distinct specialization areas: (1) DSP agents need zero-tolerance real-time safety rules based on established audio programming principles, (2) GUI agents need APVTS atomic patterns and JUCE 8-specific lifecycle rules especially for WebView, and (3) professional quality standards can be quantified through established audio metrics (THD, SNR) and DAW compatibility requirements.

The existing infrastructure from Phase 4-5 (critic templates, scoring schemas, gate integration) provides the foundation. Phase 6 extends this by making the domain rules exhaustive, adding JUCE 8-specific patterns, defining professional quality thresholds, and creating the music theory agent prototype. The context decisions are well-specified: zero tolerance for processBlock violations, strict member declaration order enforcement, and music theory covering both tuning/temperament and harmonic analysis.

The key implementation approach: (1) enhance dsp-agent.md and gui-agent.md with comprehensive rule sets, (2) update critic templates with expanded checklists and new categories, (3) define professional quality standards as measurable criteria, (4) create music-theory-agent as a working prototype (not just spec), and (5) create aesthetics-agent spec for future implementation.

**Primary recommendation:** Update existing agents/critics with exhaustive domain rules rather than creating new artifacts. The critic templates already have the scoring infrastructure; they need expanded rule coverage based on the decisions in CONTEXT.md.

## Standard Stack

The established patterns for domain specialization:

### Core (Existing Infrastructure)
| Component | Location | Purpose | Enhancement Needed |
|-----------|----------|---------|-------------------|
| dsp-agent.md | .claude/agents/ | DSP implementation | Exhaustive real-time rules |
| gui-agent.md | .claude/agents/ | GUI implementation | APVTS patterns, WebView lifecycle |
| critic-dsp.md | .claude/critics/ | DSP validation | Lambda/std::function rules |
| critic-ui.md | .claude/critics/ | UI validation | Thread safety scoring (7 threshold) |

### New Agents (Phase 6 Deliverables)
| Agent | Priority | Purpose | Deliverable |
|-------|----------|---------|-------------|
| music-theory-agent | HIGH | Tuning, intervals, harmonic analysis | Working prototype |
| aesthetics-agent | MEDIUM | UI design guidance | Specification only |

### Supporting References
| Resource | Purpose | Status |
|----------|---------|--------|
| stage-2-patterns.md | DSP pattern reference | Exists, may need expansion |
| stage-3-patterns.md | GUI pattern reference | Exists, comprehensive |
| juce8-critical-patterns.md | Full pattern library | Exists (22 patterns) |

**No new tools required:** Builds on existing agent/critic infrastructure. Enhancement is content, not architecture.

## Architecture Patterns

### Pattern 1: Real-Time Safety Rule Encoding (DSP Agent)

**What:** Comprehensive ruleset for processBlock safety with zero-tolerance enforcement
**When to use:** Every DSP implementation and Stage 2 validation
**Decision from CONTEXT.md:** Zero tolerance for allocations, locks, syscalls in processBlock

**Complete Rule Checklist:**
```markdown
## Real-Time Safety - REJECT on any violation

### Memory Allocation (REJECT)
- `new`, `delete`
- `malloc`, `calloc`, `realloc`, `free`
- `std::make_unique`, `std::make_shared`
- `std::vector::push_back`, `std::vector::emplace_back`, `std::vector::resize`
- `std::string` operations (construction, concatenation, append)
- `juce::String` operations in processBlock
- `juce::Array` dynamic operations

### std::function (REJECT entirely)
- Any `std::function` usage in processBlock path
- Type erasure may allocate on construction
- Even small captures can exceed small object optimization threshold
- No exceptions - reject all std::function in audio path

### Lambda Rules (Conditional)
- Capture-less lambdas `[]` - ALLOWED (compile to function pointers)
- Lambdas with captures - REJECT unless:
  - Not passed through std::function
  - Capture is proven non-allocating (e.g., `[this]` or small primitives)
  - Lambda is created at compile time, not runtime

### Locks and Synchronization (REJECT)
- `std::mutex`, `std::recursive_mutex`
- `std::lock_guard`, `std::unique_lock`, `std::scoped_lock`
- `juce::ScopedLock`, `juce::CriticalSection`
- `pthread_mutex_lock` or any platform lock primitives
- Any blocking wait on condition variables

### System Calls and I/O (REJECT)
- `printf`, `fprintf`, `std::cout`, `std::cerr`
- `DBG()` in release builds (allowed in debug only)
- `fopen`, `fread`, `fwrite`, `fclose`
- `std::ifstream`, `std::ofstream`
- `juce::File` operations
- Network operations
- Registry/plist access

### Exception Handling (REJECT)
- `throw` statements
- `try/catch` blocks in processBlock
- Any code that may throw (unchecked)

### Unbounded Operations (REJECT)
- `while` loops with external condition
- Recursion without proven bounds
- Algorithm calls with unbounded time complexity

### Pre-allocation Patterns (REQUIRED)
- All buffers allocated in prepareToPlay()
- juce::ScopedNoDenormals at start of processBlock
- Channel count validation before iteration
- Zero-length buffer early exit
```

### Pattern 2: Thread-Safety Patterns (GUI Agent)

**What:** APVTS atomic patterns and WebView relay lifecycle
**When to use:** Every GUI implementation and Stage 3 validation
**Decision from CONTEXT.md:** Member order strictly enforced, relays before WebView before attachments

**APVTS Thread-Safety Rules:**
```markdown
## APVTS Pattern Enforcement

### Atomic Reads (REQUIRED)
- Use getRawParameterValue()->load() for audio thread reads
- Never call getValue() from audio thread (may lock)
- Never call setValue() from audio thread (message queue)

### Attachment Classes (REQUIRED)
- Use SliderAttachment, ButtonAttachment, ComboBoxAttachment
- Attachments handle thread-safe synchronization automatically
- Never manually sync parameters between threads

### Message Thread Restrictions
- Never call processBlock from UI thread
- Never access audio buffers from UI thread
- Use APVTS for all UI->Audio communication
```

**WebView Relay Lifecycle (STRICT):**
```markdown
## Member Declaration Order - CRITICAL

### Declaration Order (PluginEditor.h)
1. Relays FIRST (no dependencies)
2. WebView SECOND (depends on relays via withOptionsFrom)
3. Attachments LAST (depend on both relays and WebView)

### Destruction Order (Reverse of Declaration)
1. Attachments destroyed first -> stop using relays/webView
2. WebView destroyed second -> safe (attachments gone)
3. Relays destroyed last -> safe (nothing using them)

### Wrong Order Consequences
- Release build crashes on plugin reload
- Attachments call evaluateJavascript() on destroyed WebView
- Random crashes that don't reproduce in debug builds

### Constructor Initialization Pattern
```cpp
, gainRelay("gain")
, cutoffRelay("cutoff")
, webView(juce::WebBrowserComponent::Options{}
    .withNativeIntegrationEnabled()
    .withOptionsFrom(gainRelay)
    .withOptionsFrom(cutoffRelay)
)
, gainAttachment(*params.getParameter("gain"), gainRelay, nullptr)
, cutoffAttachment(*params.getParameter("cutoff"), cutoffRelay, nullptr)
```

### Timer Patterns
- stopTimer() MUST be called in destructor
- Timer callbacks must check component validity
- Never assume component exists when timer fires
```

### Pattern 3: MessageManager Communication (DSP->GUI)

**What:** Safe patterns for audio thread to GUI communication
**When to use:** VU meters, waveform displays, any audio-thread data to GUI
**Decision from CONTEXT.md:** Claude determines rejection/suggestion based on what's being communicated

**Safe Patterns:**
```markdown
## Audio Thread -> GUI Communication

### Atomic + Timer Polling (SAFEST)
```cpp
// Processor
std::atomic<float> vuLevel{0.0f};

void processBlock(...) {
    vuLevel.store(calculateLevel(), std::memory_order_release);
}

// Editor (Timer callback, ~30ms)
void timerCallback() {
    float level = processorRef.vuLevel.load(std::memory_order_acquire);
    updateMeter(level);
}
```

### Lock-Free Queue (SAFE for complex data)
- juce::AbstractFifo pattern
- Audio thread writes, GUI thread reads
- No blocking on either end

### AsyncUpdater (CAUTION)
- triggerAsyncUpdate() may block on some OSes
- Use only for infrequent updates
- Not suitable for per-buffer updates

### MessageManager::callAsync (REJECT from audio thread)
- Posts message to queue - may block
- Causes priority inversion risk
- Use atomic + polling instead

### Decision Matrix
| Data Type | Frequency | Recommended Pattern |
|-----------|-----------|---------------------|
| Level meter (float) | Per buffer | Atomic + Timer |
| Waveform (buffer copy) | Per N buffers | Lock-free FIFO |
| State change (event) | Rare | AsyncUpdater OK |
| UI rebuild (complex) | Very rare | callAsync OK (not from audio) |
```

### Pattern 4: Professional Quality Standards

**What:** Measurable quality criteria for commercial-grade plugins
**When to use:** Final validation, quality assurance, release readiness
**Decision from CONTEXT.md:** Core mandatory, recommended thresholds, DAW compatibility required

**DSP Quality Metrics:**
```markdown
## DSP Quality Standards

### Core Mandatory (MUST PASS)
| Metric | Requirement | Test |
|--------|-------------|------|
| DC Offset | < 0.001 (at output) | Process silence, measure offset |
| Digital Clipping | None at 0dBFS input | Process full-scale sine, check output |
| Null Test | Passes for bypass mode | Compare input/output in bypass |
| Silence | Produces silence for silent input | Feed zeros, measure output |

### Recommended Thresholds (TARGET)
| Metric | Professional Target | Acceptable |
|--------|--------------------|-----------|
| THD+N | < 0.005% | < 0.01% |
| SNR | > 100dB | > 90dB |
| Dynamic Range | > 110dB | > 100dB |
| Frequency Response | +/- 0.1dB 20Hz-20kHz | +/- 0.5dB |

### Reference Comparison
- Output quality should match commercial plugins (FabFilter, Soundtoys level)
- A/B testing against reference implementations
- CPU usage competitive with similar commercial plugins
```

**UI Quality Metrics:**
```markdown
## UI Quality Standards

### Visual Consistency (REQUIRED)
| Element | Requirement |
|---------|-------------|
| Spacing | Consistent margins/padding throughout |
| Alignment | Elements on consistent grid |
| Font Hierarchy | Clear size progression (title > label > value) |
| Color Palette | Systematic, limited palette |

### Interaction Quality (RECOMMENDED)
| Element | Requirement |
|---------|-------------|
| Animations | Smooth (60fps) or instant, never stuttery |
| Responsive Controls | < 16ms response time |
| Hover/Focus States | All interactive elements |
| Keyboard Navigation | Tab order logical |

### DAW Compatibility (REQUIRED)
| DAW | Format | Minimum Version |
|-----|--------|-----------------|
| Logic Pro | AU | 10.7+ |
| Ableton Live | VST3/AU | 11+ |
| Pro Tools | AAX | 2022+ |
| Cubase | VST3 | 12+ |

### Validation Tools
- pluginval strictness level 10 (REQUIRED)
- auval (macOS AU validation)
- VST3 SDK validator
```

### Pattern 5: Music Theory Agent Design

**What:** Agent for tuning systems, harmonic analysis, and pitch calculations
**When to use:** Pitch detection plugins, tuning utilities, harmonic analyzers
**Decision from CONTEXT.md:** Working prototype, not just spec; covers tuning AND harmonic analysis

**Query Types:**
```markdown
## Music Theory Agent Capabilities

### Tuning/Temperament Calculations
| Query Type | Input | Output |
|------------|-------|--------|
| interval_ratio | interval name, temperament | frequency ratio, cents |
| temperament_frequencies | root freq, temperament | 12-note frequency table |
| tuning_table | root freq, scale, temperament | custom scale frequencies |
| cents_conversion | ratio or frequency pair | cents value |

### Harmonic Analysis
| Query Type | Input | Output |
|------------|-------|--------|
| harmonic_series | fundamental freq, limit | harmonic frequencies |
| chord_voicing | chord type, root, voicing | frequencies, ratios |
| scale_degrees | scale name, root | degree names, intervals |
| pitch_class | frequency | note name, octave, cents deviation |

### Supported Temperaments
- Equal (12-TET)
- Just Intonation (5-limit, 7-limit)
- Pythagorean
- Meantone (various comma fractions)
- Werckmeister III
- Well-tempered (various historical)
- Custom (user-defined ratios)

### Key Formulas
```
Equal Temperament: f_n = f_0 * 2^(n/12)
Just Intonation: ratios are rational (e.g., 3/2 for P5, 5/4 for M3)
Cents: c = 1200 * log2(f2/f1)
Frequency from cents: f = f_0 * 2^(c/1200)
```

### Integration Points
- Consulted by dsp-agent for tuning algorithm implementation
- Provides C++ code snippets for pitch calculations
- Validates musical correctness of tuning implementations
```

## Don't Hand-Roll

Problems that look simple but have established solutions:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Real-time safety check | Manual code review | Structured critic checklist | Comprehensive, reproducible |
| Thread safety validation | Ad-hoc inspection | Member order pattern | Catches 90% of release crashes |
| Parameter binding | Custom sync code | APVTS + Attachments | JUCE handles thread safety |
| Tuning calculations | Ad-hoc math | Music theory agent | Centralizes expertise, reusable |
| UI/audio communication | Custom locks | Atomic + Timer polling | Lock-free, real-time safe |
| Professional quality check | Subjective assessment | Measurable metrics | Objective, reproducible |

**Key insight:** Domain expertise should be encoded once in agent rules and reused across all plugins. Don't re-evaluate the same safety rules for each project.

## Common Pitfalls

### Pitfall 1: std::function in processBlock

**What goes wrong:** Runtime allocation during audio processing causes glitches
**Why it happens:** std::function uses type erasure which may allocate for large captures
**How to avoid:** Reject all std::function in processBlock path; use direct function pointers or templates
**Warning signs:** Occasional audio dropouts, especially with new parameter values

### Pitfall 2: WebView Member Order

**What goes wrong:** Crash on plugin reload in release builds
**Why it happens:** Attachments destroyed after WebView, call methods on dead object
**How to avoid:** Strict declaration order: Relays -> WebView -> Attachments
**Warning signs:** Crash never happens in debug, always in release; crash on editor close

### Pitfall 3: MessageManager from Audio Thread

**What goes wrong:** Priority inversion, potential deadlock
**Why it happens:** MessageManager::callAsync posts to message queue which may block
**How to avoid:** Use atomic variables + Timer polling for audio->GUI data
**Warning signs:** Occasional hangs when DAW stops playback

### Pitfall 4: Incomplete Parameter Binding

**What goes wrong:** Parameters don't affect sound or automation doesn't work
**Why it happens:** Parameter declared but not read in processBlock, or wrong atomic pattern
**How to avoid:** Critic verifies every spec parameter is accessed with atomic read
**Warning signs:** Some knobs do nothing, automation curves ignored

### Pitfall 5: Release Build Different from Debug

**What goes wrong:** Works in debug, crashes or misbehaves in release
**Why it happens:** Different memory patterns, uninitialized variables, timing differences
**How to avoid:** Always test release builds; use pluginval at strictness 10
**Warning signs:** "Works on my machine" syndrome, user crash reports

### Pitfall 6: Capture-by-Reference in Lambdas

**What goes wrong:** Dangling reference causes crash or undefined behavior
**Why it happens:** Captured reference outlives the referenced object
**How to avoid:** Audit lambda captures; prefer capture-by-value for safety
**Warning signs:** Intermittent crashes, corruption that depends on timing

## Code Examples

### Real-Time Safety Check (DSP Critic Enhancement)

```markdown
## Real-Time Safety Violations Checklist

### Check 1: Allocation Patterns
Search for these patterns in processBlock and called functions:
- `new\\s+\\w+` (new keyword)
- `std::make_unique|std::make_shared`
- `malloc|calloc|realloc`
- `push_back|emplace_back|resize` on vectors
- `juce::String\\s+\\w+\\s*=` (string construction)

### Check 2: std::function Detection
- `std::function` anywhere in processBlock path
- Even if defined outside processBlock, called from within = violation

### Check 3: Lambda Capture Analysis
```cpp
// ALLOWED: Capture-less
auto fn = []() { return 42; };

// ALLOWED: This pointer only (small, no allocation)
auto fn = [this]() { return memberVar; };

// REJECTED: Passed through std::function
std::function<void()> fn = [this]() { ... };

// SUSPICIOUS: Large capture (may allocate)
auto fn = [buffer, param1, param2, param3]() { ... };
```

### Check 4: Lock Detection
Search for mutex/lock patterns:
- `std::mutex|std::recursive_mutex`
- `std::lock_guard|std::unique_lock|std::scoped_lock`
- `juce::ScopedLock|CriticalSection`
```

### Thread Safety Validation (UI Critic Enhancement)

```markdown
## Thread Safety Category (Threshold: 7/10)

### Check 1: Member Declaration Order
Parse PluginEditor.h private section:
1. Find all WebSliderRelay, WebToggleButtonRelay, WebComboBoxRelay declarations
2. Find WebBrowserComponent declaration
3. Find all Attachment declarations
4. Verify order: ALL relays before WebView, WebView before ALL attachments

### Check 2: APVTS Access Patterns
In PluginProcessor.cpp processBlock():
- Parameter reads MUST use: getRawParameterValue("id")->load()
- REJECT: getParameter("id")->getValue() (may lock)
- REJECT: setValue from audio thread

### Check 3: Timer Destructor Pattern
In PluginEditor.cpp destructor:
- Check for stopTimer() call
- Must be called before any member destruction

### Check 4: Relay Registration
In WebBrowserComponent construction:
- Count relays declared
- Count .withOptionsFrom() calls
- All relays must be registered
```

### Music Theory Agent Implementation Skeleton

```cpp
// Query: interval_ratio
// Input: { "interval": "perfect_fifth", "temperament": "just" }
// Output: { "ratio": "3/2", "cents": 702, "decimal": 1.5 }

struct IntervalRatio {
    std::string ratio;
    double cents;
    double decimal;
};

IntervalRatio getJustInterval(const std::string& interval) {
    static const std::map<std::string, std::pair<int, int>> JUST_RATIOS = {
        {"unison", {1, 1}},
        {"minor_second", {16, 15}},
        {"major_second", {9, 8}},
        {"minor_third", {6, 5}},
        {"major_third", {5, 4}},
        {"perfect_fourth", {4, 3}},
        {"tritone", {45, 32}},
        {"perfect_fifth", {3, 2}},
        {"minor_sixth", {8, 5}},
        {"major_sixth", {5, 3}},
        {"minor_seventh", {9, 5}},
        {"major_seventh", {15, 8}},
        {"octave", {2, 1}}
    };

    auto it = JUST_RATIOS.find(interval);
    if (it == JUST_RATIOS.end()) {
        throw std::invalid_argument("Unknown interval: " + interval);
    }

    auto [num, den] = it->second;
    double decimal = static_cast<double>(num) / den;
    double cents = 1200.0 * std::log2(decimal);

    return {
        std::to_string(num) + "/" + std::to_string(den),
        cents,
        decimal
    };
}

// Query: temperament_frequencies
// Input: { "root_frequency": 440.0, "temperament": "equal" }
// Output: { "frequencies": [440.0, 466.16, 493.88, ...] }

std::vector<double> getEqualTemperamentFrequencies(double rootFreq) {
    std::vector<double> frequencies(12);
    for (int i = 0; i < 12; ++i) {
        frequencies[i] = rootFreq * std::pow(2.0, i / 12.0);
    }
    return frequencies;
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual safety review | Encoded agent rules | Phase 6 | Consistent enforcement |
| Ad-hoc thread patterns | APVTS strict patterns | JUCE 8 (2024) | Fewer thread bugs |
| Subjective quality | Measurable metrics | Industry standard | Objective quality gates |
| One-off tuning code | Reusable music theory agent | Phase 6 | Expertise centralization |
| Silent std::function issues | Zero-tolerance rejection | Phase 6 | Catches allocation bugs |

**Current project status:**
- DSP/GUI agents exist with basic rules
- Critics exist with scoring infrastructure
- Missing: Exhaustive real-time rules, std::function rejection, WebView lifecycle enforcement
- Missing: Professional quality standards as formal criteria
- Missing: Music theory agent (prototype needed per CONTEXT.md)
- Missing: Aesthetics agent specification

## Open Questions

1. **std::function small object optimization threshold**
   - What we know: std::function may not allocate for small captures
   - What's unclear: Exact threshold varies by implementation
   - Recommendation: Reject all std::function in processBlock to be safe; no exceptions

2. **Professional quality metric thresholds**
   - What we know: Industry targets (THD < 0.005%, SNR > 100dB)
   - What's unclear: Appropriate thresholds for this project's plugins
   - Recommendation: Start with industry targets, adjust based on plugin category

3. **Music theory agent scope boundaries**
   - What we know: Tuning + harmonic analysis, not instrument-specific
   - What's unclear: How deep into composition/arrangement assistance
   - Recommendation: Focus on calculable theory (frequencies, ratios, intervals); defer subjective advice

4. **Aesthetics agent implementation depth**
   - What we know: Spec only for Phase 6, per CONTEXT.md
   - What's unclear: Whether spec should be detailed enough for immediate Phase 7 implementation
   - Recommendation: Make spec implementation-ready but defer working prototype

## Sources

### Primary (HIGH confidence)
- [Ross Bencina: Real-time Audio Programming 101](http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing) - Authoritative real-time rules
- [JUCE AudioProcessorValueTreeState Documentation](https://docs.juce.com/master/classAudioProcessorValueTreeState.html) - Thread safety patterns
- [JUCE Forum: APVTS Thread Safety](https://forum.juce.com/t/audioprocessorvaluetreestate-raw-value-pointer-thread-safety/29957) - Atomic access patterns
- [JUCE 8 Release Notes](https://juce.com/releases/whats-new/) - WebView, animation, new features
- Existing project files: dsp-agent.md, gui-agent.md, critic-dsp.md, critic-ui.md, stage-2-patterns.md, stage-3-patterns.md

### Secondary (MEDIUM confidence)
- [Timur Doumler: Using Locks in Real-Time Audio](https://timur.audio/using-locks-in-real-time-audio-processing-safely) - Lock-free patterns
- [JUCE Forum: AsyncUpdater vs callAsync](https://forum.juce.com/t/asyncupdater-vs-messagemanager-callasync/23459) - Audio->GUI communication
- [Audio Precision Specifications Guide](https://sengpielaudio.com/Mathew-AudioPrecisionSpecifications.pdf) - THD, SNR measurement standards
- [Wikipedia: Equal Temperament](https://en.wikipedia.org/wiki/Equal_temperament) - Tuning system math
- [Wikipedia: Just Intonation](https://en.wikipedia.org/wiki/Just_intonation) - Ratio-based tuning

### Tertiary (LOW confidence)
- WebSearch results on std::function allocation behavior - Implementation varies
- WebSearch results on FabFilter/Soundtoys quality - Subjective comparisons
- WebSearch results on DAW compatibility - Version requirements change

## Metadata

**Confidence breakdown:**
- Real-time safety rules: HIGH - Based on established audio programming principles (Bencina, Doumler)
- Thread-safety patterns: HIGH - JUCE documentation and forum discussions authoritative
- JUCE 8 specifics: HIGH - Official documentation, project's existing pattern files
- Professional quality standards: MEDIUM - Industry standards clear, project-specific thresholds TBD
- Music theory formulas: HIGH - Mathematical, well-defined
- Aesthetics agent scope: LOW - Subjective domain, spec only in Phase 6

**Research date:** 2026-01-31
**Valid until:** 2026-04-30 (90 days - domain rules stable, JUCE updates may affect patterns)
