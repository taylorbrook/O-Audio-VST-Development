# Building Audio Plugins Without JUCE: Benefits, Drawbacks, and Alternatives

## Executive Summary

JUCE dominates the audio plugin development landscape, but it is not the only path. This document analyzes the trade-offs of going framework-free or using lighter alternatives, grounded in real-world data from this project's own plugin builds and the broader developer community.

**Key finding:** Your current JUCE-built plugins range from 4.3 MB (O-FreqPulse, a simple effect) to 15 MB (O-AnalogEQ, complex with WebView). A minimal VST3 built with raw SDK or single-header approaches can be as small as 14.5 KB -- roughly 300x smaller. However, the binary size savings come at a steep cost in development time, cross-platform support, and ongoing maintenance.

---

## 1. Benefits of Going Framework-Free or Using Lighter Alternatives

### 1.1 Binary Size Reduction

**Your current JUCE plugin sizes (macOS, Release builds):**

| Plugin | VST3 Size | Complexity |
|--------|-----------|------------|
| O-FreqPulse | 4.3 MB | Simple effect, WebView GUI |
| O-SpectralShaper | 4.3 MB | Spectral processing |
| O-Detune | 4.6 MB | Simple effect |
| O-Freeze | 4.8 MB | Granular effect |
| O-Polystutter | 5.2 MB | Sequenced effect |
| O-SimpleReverb | 5.2 MB | Basic reverb |
| O-Bells | 5.3 MB | Synthesizer |
| O-Lyrica | 5.7 MB | Synthesizer |
| O-Bass | 6.8 MB | Synthesizer |
| O-Tremolo | 6.8 MB | Effect |
| O-AnalogEQ | 15 MB | Complex EQ with WebView |

**Framework-free comparison points:**
- **vst3_plugin.h** (single-header, no Steinberg SDK): ~14.5 KB for a gain plugin (.so on Linux)
- **CPLUG** (C99 wrapper): Designed for "tiny binaries," typically under 100 KB for minimal plugins
- **Raw Steinberg VST3 SDK**: ~200-500 KB for a simple effect (no GUI)
- **nih-plug** (Rust framework): Typically 1-2 MB for simple plugins

The baseline JUCE overhead (even for a trivial plugin with no real DSP) is roughly 3-5 MB on macOS. This includes the JUCE module system, GUI framework, event loop, parameter management, and format wrappers compiled into every binary.

Historical note: JUCE 4.2 introduced a notable binary size regression -- the demo plugin grew from 3.8 MB to 15.4 MB as a universal binary, which was [extensively discussed on the JUCE forum](https://forum.juce.com/t/plugin-binary-size-4-2-bloat/17325).

### 1.2 Build Time Improvements

JUCE compile times are a well-documented pain point:
- Even minor changes (tweaking a single number) can trigger compile times of tens of seconds or minutes in complex projects
- The monolithic module system means touching one header can cascade across many compilation units
- GUI development is the largest contributor to iteration time, as C++ recompilation is needed for every visual change

Framework-free or lighter approaches offer:
- **CPLUG**: ~5,300 total lines of code across all format wrappers -- compiles in seconds
- **Raw VST3 SDK**: Only compile what you use, no module overhead
- **WebView-only GUI** (without JUCE GUI modules): Eliminates the largest compile-time contributor since HTML/CSS/JS changes require zero C++ recompilation
- **nih-plug** (Rust): Incremental compilation is generally faster than JUCE's C++ module system

### 1.3 Licensing Cost Savings

**Current JUCE 8 pricing (as of early 2026):**

| Tier | Monthly | Perpetual | Revenue Limit |
|------|---------|-----------|---------------|
| Starter (Free) | $0 | $0 | Unrestricted (AGPLv3 terms) |
| Educational | $0 | $800 | Up to $20,000 |
| Indie | $40/user | $800 | Up to $300,000 |
| Pro | $175/user | $3,500 | Unrestricted |

**Important caveats:**
- The "free" Starter tier requires AGPLv3 compliance, meaning your plugin source code must be made available if distributed. This is unacceptable for most commercial plugins.
- Indie at $40/month = $480/year per developer. For a solo developer, this is $480/year ongoing cost.
- Pro at $175/month = $2,100/year per developer. For a team of 3, that is $6,300/year.
- Perpetual licenses are one-time but do not include future major version upgrades (30% upgrade discount available).

**Framework-free alternatives are all free:**
- Raw VST3 SDK: Free (proprietary Steinberg license, but no fees)
- CLAP: MIT license, completely free
- Audio Units: Apple's SDK, free (macOS only)
- iPlug2: Free, open source (multiple licenses)
- CPLUG: Free, permissive license
- DPF: ISC license, free
- nih-plug: Free, open source

### 1.4 More Control Over DSP Pipeline and Audio Threading

JUCE's `AudioProcessor::processBlock()` is a convenient abstraction, but it imposes:
- A fixed processing model (block-based, single callback)
- Thread management decisions you cannot override without hacking JUCE internals
- Parameter smoothing and automation handled through JUCE's system, which may not match your needs

Going direct to the VST3 or CLAP API gives you:
- **Sample-accurate automation** by default in CLAP (JUCE requires extra work for this)
- **Custom threading models** -- CLAP explicitly supports collaborative multicore between host and plugin via thread pools
- **Direct control** over buffer sizes, internal oversampling, and processing order
- **Zero-copy parameter updates** without JUCE's atomic parameter infrastructure overhead

### 1.5 Avoiding JUCE Abstractions and Overhead

JUCE wraps everything -- sometimes adding unnecessary indirection:
- `juce::AudioBuffer` vs. raw float pointers (minor overhead but adds copying in some paths)
- `juce::MidiBuffer` vs. direct MIDI event handling
- The `AudioProcessorValueTreeState` layer for parameters adds XML serialization overhead
- JUCE's message thread / audio thread separation enforces a specific concurrency model

For performance-critical DSP (real-time convolution, spectral processing, neural network inference), every layer of abstraction adds latency risk and makes profiling harder.

### 1.6 Custom GUI Flexibility

Without JUCE's `juce::Component` system, you can use:

**GPU-accelerated rendering:**
- **Visage** (MIT, by Matt Tytel / Vital Audio): GPU-accelerated, cross-platform (Direct3D 11, Metal, Vulkan, WebGL). Built specifically for audio plugin UIs. Supports shader cross-compilation, partial dirty-region rendering, blur/bloom effects.
- **Dear ImGui**: Immediate-mode GUI, very fast iteration. Some developers have created CLAP + ImGui plugin templates.
- **NanoVG**: Vector graphics on OpenGL, commonly used with CPLUG.
- **Sokol**: Thin cross-platform windowing + graphics abstraction.

**Web technologies (without JUCE's WebView module):**
- Direct WebView2 (Windows) / WKWebView (macOS) integration
- Full React/Vue/Svelte UIs communicating with C++ backend
- Hot-reloading during development (no C++ recompilation for GUI changes)

**Native platform APIs:**
- Direct Metal/DirectX rendering for visualizers and spectrum analyzers
- Platform-native controls for maximum performance

### 1.7 Reduced Dependency Footprint

JUCE 8 pulls in ~200,000+ lines of C++ across its modules. Even if you only use `juce_audio_processors`, it has transitive dependencies on `juce_audio_basics`, `juce_core`, `juce_events`, `juce_data_structures`, and more. This means:
- Larger attack surface for security vulnerabilities
- More code to audit for real-time safety
- Dependency on JUCE's release cycle for bug fixes
- Risk of JUCE licensing changes affecting your business (has happened multiple times: JUCE 5, 6, 7, 8 all changed terms)

### 1.8 Deep Understanding of Plugin Architecture

Building directly against the VST3 or CLAP API teaches you:
- How hosts discover and instantiate plugins
- The COM-like interface system in VST3 (IPluginFactory, IComponent, IEditController)
- How parameter automation actually flows between host and plugin
- Platform-specific plugin loading (`.vst3` bundles on macOS, `.component` bundles for AU)
- State serialization at the format level (not JUCE's abstraction over it)

This knowledge is invaluable for debugging DAW compatibility issues, which JUCE's abstraction layer can obscure.

---

## 2. Drawbacks and Challenges of Going Without JUCE

### 2.1 Cross-Platform Complexity

Each target platform has significant differences:

| Concern | macOS | Windows | Linux |
|---------|-------|---------|-------|
| Plugin discovery | `/Library/Audio/Plug-Ins/` | Registry + VST3 paths | `~/.vst3/` |
| GUI integration | NSView embedding | HWND embedding | X11/XCB |
| Audio threading | CoreAudio model | WASAPI/ASIO model | ALSA/JACK/PipeWire |
| Code signing | Notarization required | Authenticode signing | None required |
| Default graphics API | Metal | Direct3D | Vulkan/OpenGL |

JUCE abstracts all of this. Without it, you must handle:
- Different compiler toolchains (Xcode/clang, MSVC, GCC)
- Different build systems per platform (or invest heavily in CMake)
- Platform-specific window management and embedding
- File system differences (paths, bundle structures, plugin locations)

**Estimated effort:** 2-4 weeks of dedicated work per platform just for basic plugin loading and GUI embedding, assuming familiarity with each OS.

### 2.2 Plugin Format Wrappers

Each format has a fundamentally different API:

**VST3 (Steinberg):**
- COM-based architecture with IUnknown-style reference counting
- Split processor/controller model
- Complex parameter normalization (0-1 range required)
- ~2,700 lines of wrapper code in CPLUG

**Audio Units v2 (Apple):**
- Objective-C/C++ API, macOS/iOS only
- Component Manager registration
- Property-based parameter system
- Different state save/restore model
- ~1,700 lines of wrapper code in CPLUG

**CLAP:**
- Clean C API, simplest of the three
- Single plugin struct with function pointers
- ~900 lines of wrapper code in CPLUG
- Growing adoption but not yet universal (not supported by Logic Pro, Ableton Live, or Pro Tools as of early 2026)

**AAX (Avid):**
- Requires Avid developer agreement
- Requires iLok for code signing
- Most restrictive API with strict threading requirements
- JUCE is one of very few frameworks that supports it

Without JUCE, supporting all four formats means maintaining four separate wrappers or using a lighter framework like CPLUG (which does not support AAX).

### 2.3 GUI Development Burden

JUCE provides out-of-the-box:
- `juce::Slider`, `juce::ComboBox`, `juce::TextButton` -- battle-tested audio controls
- `juce::LookAndFeel` for consistent theming
- `juce::Graphics` with anti-aliased drawing, path operations, gradients
- `juce::Timer` and `juce::AnimatedPosition` for smooth animations
- Accessible parameter attachment (`SliderAttachment`, etc.)
- Tooltips, popup menus, file choosers
- Retina/HiDPI support

Building all of this from scratch is a massive undertaking. Even with libraries like Dear ImGui or NanoVG, you need to build:
- Knob/slider widgets with proper mouse capture and modifier key support
- Combobox dropdowns that work correctly when embedded in a DAW window
- Preset management UI
- Resizable plugin windows
- Keyboard focus handling within a DAW host window

### 2.4 Parameter Management and State Persistence

JUCE's `AudioProcessorValueTreeState` handles:
- Parameter normalization and denormalization
- Thread-safe parameter access (audio thread vs. GUI thread)
- Undo/redo support
- Automatic XML serialization for state save/restore
- Host parameter automation mapping

Without JUCE, you must implement:
- Thread-safe parameter storage (lock-free atomics or ring buffers)
- Format-specific state serialization (VST3's `IBStream`, AU's `CFPropertyList`, CLAP's `clap_ostream`)
- Version migration for saved states when parameters change between plugin versions
- Parameter value mapping (linear, logarithmic, discrete, string lists)

The VST3 state management is particularly tricky -- when states are restored, the host passes the processor state to both the processor and the controller, and you must handle the ordering correctly or parameters get set three times.

### 2.5 MIDI Handling Differences

- **VST3**: MIDI is unified with parameter automation in an event list. Note events use note IDs for per-note expression.
- **AU**: MIDI uses `MusicDeviceMIDIEvent` and `AUMIDIEvent`. MIDI 2.0 support varies.
- **CLAP**: Has first-class MIDI 2.0 and per-note expression support via `clap_event_note` and `clap_event_note_expression`.

Each format has different timing models for MIDI events within a processing block. JUCE normalizes all of this into `juce::MidiBuffer`, which is a significant convenience.

### 2.6 Testing Infrastructure

JUCE provides:
- `juce::UnitTest` framework integrated with the audio module system
- The `AudioPluginHost` application for testing plugins
- `auval` integration for AU validation (though this is Apple's tool)
- Projucer/CMake integration for building test targets

Without JUCE, you need:
- Your own test harness for offline audio processing
- A way to instantiate and test your plugin outside of a DAW
- Mock host implementations for each plugin format
- Automated DAW testing (no good solutions exist for this regardless of framework)

### 2.7 Time to Market

Rough estimates for building a production-quality audio plugin:

| Task | With JUCE | Without JUCE |
|------|-----------|--------------|
| Project setup + build system | 1 day | 3-5 days |
| Plugin format wrapper (1 format) | 0 (included) | 1-2 weeks |
| Plugin format wrapper (VST3 + AU + CLAP) | 0 (included) | 3-6 weeks |
| Basic GUI (knobs, meters, layout) | 1-2 weeks | 4-8 weeks |
| Parameter management + state | 1-2 days | 1-2 weeks |
| Cross-platform builds (Mac + Win) | 1-2 days | 1-2 weeks |
| DAW compatibility testing | 1 week | 2-3 weeks (more edge cases) |

A plugin that takes 1-2 months with JUCE could take 4-6 months without it, if starting from scratch.

### 2.8 DAW Compatibility

JUCE has been tested against dozens of DAWs over 20+ years. Known quirks and workarounds are baked in:
- Logic Pro's strict AU validation requirements
- Ableton Live's unique parameter handling
- Pro Tools AAX threading constraints
- FL Studio's unusual plugin scanning
- Reaper's liberal but idiosyncratic hosting behavior

Building your own wrappers means discovering and fixing these compatibility issues yourself, one DAW at a time.

---

## 3. Real-World Examples

### 3.1 Commercial Plugins Built Without JUCE

**FabFilter** (Pro-Q, Pro-L, Pro-C, etc.)
- Uses a completely proprietary in-house framework
- Supports VST2, VST3, AU, CLAP, AAX, AudioSuite
- Known for exceptionally smooth, responsive GUIs (custom GPU rendering)
- One of the most successful plugin companies, proving that JUCE is not required for commercial success
- [FabFilter website](https://www.fabfilter.com/)

**u-he** (Diva, Zebra, Repro, Hive)
- Custom C++ engine developed by founder Urs Heckmann
- Co-created the CLAP plugin format with Bitwig
- Heckmann built an internal "engine" where adding a parameter takes a single line of code
- Background in industrial design informed the plugin UI architecture
- [u-he website](https://u-he.com/)
- [MusicRadar interview with Urs Heckmann](https://www.musicradar.com/news/tech/u-hes-urs-heckmann-talks-synths-programming-and-modular-mayhem-640045)

**Airwindows** (400+ plugins by Chris Johnson)
- Deliberately uses no framework and no GUI (generic host controls only)
- Minimal DSP-only approach: "impossibly light and efficient compared to how plugins usually are"
- Demonstrates that eliminating the GUI entirely is viable for certain markets
- Patreon-supported, fully open source
- Originally Mac AU only, later added VST2/VST3 for Windows/Linux
- [Airwindows website](https://www.airwindows.com/)
- [Chris Johnson's Patreon](https://www.patreon.com/airwindows/about)

### 3.2 Open-Source Plugins Built Without JUCE (or Partially)

**Vital Synthesizer** (Matt Tytel)
- Uses JUCE for plugin wrapping but replaced JUCE's GUI entirely
- Created **Visage**, a custom GPU-accelerated graphics library (MIT license)
- Visage supports Direct3D 11, Metal, Vulkan, and WebGL
- Shader cross-compilation: write once, transpile for all backends
- Partial dirty-region rendering for performance
- [Visage on GitHub](https://github.com/VitalAudio/visage)
- [Vital website](https://vital.audio/)

**Surge XT**
- Originally built with a custom framework (versions 1.0-1.9)
- Migrated TO JUCE for Surge XT, citing "far more reliable plugins"
- This is a cautionary tale: maintaining a custom framework became unsustainable for an open-source project
- [Surge Synthesizer on GitHub](https://github.com/surge-synthesizer/surge)

**nih-plug ecosystem** (Robbert van der Helm)
- Rust-based framework for VST3 and CLAP plugins
- Several production plugins including Diopser (spectral processing)
- "Behaves mostly like JUCE, just without all of the boilerplate"
- Getting a basic plugin running takes ~20 minutes
- [nih-plug on GitHub](https://github.com/robbert-vdh/nih-plug)

### 3.3 Notable Frameworks and Their Trade-offs

| Framework | Language | Formats | License | GUI Included | Binary Size | Maturity |
|-----------|----------|---------|---------|--------------|-------------|----------|
| JUCE | C++ | VST2/3, AU, AAX, LV2 | AGPLv3 / Commercial | Full GUI framework | 4-15 MB | 20+ years |
| iPlug2 | C++ | VST2/3, AU, AAX, WAM | Multiple (mostly free) | Basic GUI + web | 2-5 MB | 10+ years |
| CPLUG | C99 | VST3, AUv2, CLAP | Permissive | None (bring your own) | <100 KB | 2-3 years |
| DPF | C++ | VST2/3, CLAP, LV2, LADSPA | ISC | Basic (Cairo/OpenGL) | 1-3 MB | 8+ years |
| nih-plug | Rust | VST3, CLAP | Multiple | Optional (iced, vizia, egui) | 1-3 MB | 3+ years |
| Raw VST3 SDK | C++ | VST3 (+ wrappers for AU, AAX) | Steinberg | VSTGUI optional | 200 KB-2 MB | 15+ years |
| CLAP SDK | C | CLAP only | MIT | None | <50 KB | 3+ years |

---

## 4. Hybrid Approaches

### 4.1 JUCE for Wrapping + Custom GUI (What You're Already Doing)

This is the most common hybrid approach and is essentially what your project does with WebView-based UIs:
- JUCE handles VST3/AU wrapping, parameter management, state persistence
- WebView (HTML/CSS/JS) handles the entire GUI
- C++ DSP code runs in JUCE's `processBlock()`
- Communication via `juce::WebBrowserComponent` and JavaScript interop

**Advantages:**
- Zero C++ recompilation for GUI changes
- Full web framework ecosystem (React, Vue, Svelte, Tailwind CSS)
- Hot-reloading during development
- Easy to hire web developers for GUI work

**Disadvantages:**
- WebView adds ~1-3 MB to binary size (WebKit on macOS, WebView2 on Windows)
- Memory overhead of hosting a browser engine (~30-80 MB RAM)
- Communication latency between JS and C++ (not suitable for real-time visualization at 60fps)
- Platform-specific WebView quirks (your project already documents Windows WebView2 issues extensively)

### 4.2 JUCE for Audio + Visage/ImGui for GUI

Possible but less common:
- Keep `juce::AudioProcessor` for the audio backend
- Replace `juce::Component` with Visage or Dear ImGui for rendering
- Requires custom window embedding code per platform
- Loses JUCE's parameter attachment convenience

### 4.3 CLAP-First with Format Wrappers

An emerging pattern:
- Build your plugin as CLAP (simplest API, MIT licensed)
- Use [clap-wrapper](https://github.com/free-audio/clap-wrapper) to expose as VST3 and AU
- Write one codebase, distribute three formats
- Limitation: CLAP is not yet supported by Logic Pro, Ableton Live, or Pro Tools

### 4.4 CPLUG + Choose-Your-Own-GUI

The "Unix philosophy" approach:
- CPLUG for plugin format wrapping (~5,300 lines total)
- Pugl for cross-platform windowing
- NanoVG or Visage for rendering
- dr_libs for audio file loading
- pffft or KFR for FFT
- Custom or sds for string handling

This produces the smallest possible binaries but requires assembling and maintaining many small libraries.

### 4.5 nih-plug (Rust)

For teams comfortable with Rust:
- Memory safety guarantees are valuable for real-time audio code
- No null pointer dereferences, no data races
- Growing ecosystem of audio DSP crates
- Limitations: smaller community, fewer DAW compatibility reports, Rust's learning curve

---

## 5. Recommendations for This Project

### Stay With JUCE For Now -- Here's Why

Given this project's characteristics (18 plugins, solo/small team, WebView GUIs, targeting Mac + Windows), JUCE remains the pragmatic choice:

1. **Your WebView hybrid approach already mitigates the biggest JUCE pain point** (slow GUI iteration). You get web dev speed for UI while keeping JUCE's format wrappers.

2. **35 plugins needing cross-platform support** means the format wrapper work alone would take months to replicate.

3. **The licensing cost** ($40/month Indie = $480/year) is modest compared to the engineering time saved.

4. **DAW compatibility** is well-tested in JUCE. Reimplementing this is high-risk, low-reward.

### When Going Without JUCE Makes Sense

Consider alternatives if:
- You are building a single, flagship plugin and want absolute control over every aspect
- Binary size is critical (e.g., distributing via web download in bandwidth-constrained markets)
- You need CLAP-only distribution (JUCE does not support CLAP natively)
- You want to avoid AGPL restrictions without paying for a license
- You are building a performance-critical plugin where JUCE's abstractions measurably impact latency
- You are starting a new project from scratch and want to invest in long-term framework independence

### Potential Incremental Steps

1. **Evaluate CPLUG** for a new simple effect plugin to understand the raw development experience
2. **Experiment with Visage** for GPU-accelerated visualizations within your existing JUCE plugins
3. **Consider CLAP support** via the clap-juce-extensions project to future-proof your plugins
4. **Profile your DSP code** to determine if JUCE's abstractions are actually bottlenecks before optimizing

---

## Sources

- [JUCE Pricing Page](https://juce.com/get-juce/)
- [JUCE Plugin Binary Size Discussion](https://forum.juce.com/t/plugin-binary-size-4-2-bloat/17325)
- [JUCE 8 EULA](https://juce.com/legal/juce-8-licence/)
- [JUCE Compilation Times Survey](https://forum.juce.com/t/compilation-times-a-survey/58329)
- [JUCE WebView UIs Feature Overview](https://juce.com/blog/juce-8-feature-overview-webview-uis/)
- [CPLUG on GitHub](https://github.com/Tremus/CPLUG)
- [Awesome Audio Plugin Framework](https://github.com/Tremus/awesome-audio-plugin-framework)
- [CLAP Plugin Format](https://github.com/free-audio/clap)
- [CLAP Overview by u-he](https://u-he.com/community/clap/)
- [CLAP on Wikipedia](https://en.wikipedia.org/wiki/CLever_Audio_Plug-in)
- [Visage Graphics Library](https://github.com/VitalAudio/visage)
- [nih-plug Framework](https://github.com/robbert-vdh/nih-plug)
- [iPlug2 vs JUCE Discussion (KVR)](https://www.kvraudio.com/forum/viewtopic.php?t=565161)
- [DPF (DISTRHO Plugin Framework)](https://github.com/DISTRHO/DPF)
- [Airwindows](https://www.airwindows.com/)
- [FabFilter](https://www.fabfilter.com/)
- [u-he Interview (MusicRadar)](https://www.musicradar.com/news/tech/u-hes-urs-heckmann-talks-synths-programming-and-modular-mayhem-640045)
- [Vital Synthesizer](https://vital.audio/)
- [Surge XT Migration to JUCE](https://synthanatomy.com/2022/01/surge-xt-1-0-open-source-synth-now-with-juce-framework-and-tons-of-new-features.html)
- [KVR: Is JUCE the Best Framework?](https://www.kvraudio.com/forum/viewtopic.php?t=569868)
- [KVR: Open Source Plugin Development](https://www.kvraudio.com/forum/viewtopic.php?t=566455)
- [KVR: GUI Library for Plugins](https://www.kvraudio.com/forum/viewtopic.php?t=618016)
- [Rust for Audio Programming in 2025](https://andrewodendaal.com/rust-audio-programming-ecosystem/)
- [Steinberg VST3 Hello World Example](https://github.com/steinbergmedia/vst3_example_plugin_hello_world)
- [JUCE Compilation Speed Discussion](https://forum.juce.com/t/juce-modules-compilation-speed-whats-the-trick/50811)
