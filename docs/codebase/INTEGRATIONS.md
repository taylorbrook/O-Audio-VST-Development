# External Integrations

**Analysis Date:** 2026-01-22

## APIs & External Services

**No external APIs detected.**

The codebase contains no integrations with third-party HTTP services, cloud APIs, or online platforms. All functionality is self-contained within the plugin.

Evidence:
- Compile definition: `JUCE_USE_CURL=0` in `plugins/OuariconLyrica/CMakeLists.txt` line 90
- No curl/network includes found in any source files
- No references to API keys, tokens, or URL endpoints in codebase

## Data Storage

**File Storage:**

Local filesystem only - no cloud storage integration.

**Preset Storage:**
- Location: `~/Library/{pluginName}/Presets/`
- Structure:
  - `Factory/` - Read-only factory presets (shipped with plugin via binary data)
  - `User/` - User-created presets
- Format: JSON text files
- Implementation: `modules/persistence/preset-manager/cpp/OuariconPresetManager.h`

**Session State:**
- Format: XML serialization (JUCE XmlElement)
- Storage: Handled by DAW's session file system
- Callback methods: `PluginProcessor::getStateInformation()` / `setStateInformation()`
- File path: `plugins/OuariconLyrica/Source/PluginProcessor.cpp`

**No Databases:**
- SQLite: Not used
- Remote databases: Not used
- In-memory caching of presets only during plugin runtime

**No Cloud Storage:**
- No AWS S3, Google Drive, Dropbox, or other cloud service integration
- All data persisted locally on user's machine

## Authentication & Identity

**No authentication system.**

Plugin operates without user login, accounts, or identity management. All functionality available to all users.

- No auth providers (not using Auth0, Firebase, OAuth, etc.)
- No licensing checks or product activation
- No telemetry or user tracking

## Monitoring & Observability

**Error Tracking:**
- Not detected - No Sentry, Rollbar, or equivalent service

**Logging:**
- JUCE Logger framework only
- Logs written to console (plugin log output within DAW)
- Example: `juce::Logger::writeToLog("[PresetManager] Preset saved: " + presetName);`
- File: `modules/persistence/preset-manager/cpp/OuariconPresetManager.h` lines 301, 316

**No Remote Monitoring:**
- No analytics tracking
- No crash reporting
- No performance metrics collection

## CI/CD & Deployment

**Hosting:**
- Not applicable - VST3/AU plugins are installed locally on user machines
- Installation: System audio plugin folders (handled by `scripts/build-and-install.sh`)

**CI Pipeline:**
- Not detected - No GitHub Actions, GitLab CI, Jenkins, etc. configured
- Build system: Local CMake via `scripts/build-and-install.sh`

**Distribution:**
- Manual installation or via build script
- No automated deployment pipeline

## Environment Configuration

**Required env vars:**

None detected. Plugin uses:
- Hardcoded paths for presets: `~/Library/{pluginName}/Presets/`
- Plugin name passed as string to preset manager constructor
- No external configuration files needed

**Compile-time configuration:**
- CMake variables in `CMakeLists.txt` (deployment target, plugin format selection)
- JUCE module selection in target_link_libraries()

**No Secrets Management:**
- No API keys
- No authentication tokens
- No configuration files with sensitive data
- .env files: Not present (confirmed by search)

## Webhooks & Callbacks

**Incoming Webhooks:**
- None - Plugin receives no external HTTP requests

**Outgoing Webhooks:**
- None - Plugin makes no HTTP requests

**Native Callbacks (Internal):**

These are JUCE plugin callbacks, not external integrations:
- `AudioProcessor::processBlock()` - Audio processing callback
- `AudioProcessor::getStateInformation()` / `setStateInformation()` - DAW session persistence
- `Timer::timerCallback()` - UI update timers for MIDI visualization (line 27 in `PluginEditor.h`)
- Parameter callbacks via APVTS (AudioProcessorValueTreeState)

## Inter-Plugin Communication

**WebView ↔ Plugin Communication:**

Used for UI parameter binding only (not external communication):
- JUCE WebBrowserComponent provides bidirectional JavaScript bridge
- JavaScript calls native C++ functions via `window.__JUCE__.backend`
- Example: `PluginEditor.h` uses WebSliderRelay and WebSliderParameterAttachment
- Purpose: Bind HTML UI sliders to APVTS parameters

**MIDI Input/Output:**

- MIDI Input: Yes (plugin accepts MIDI notes from DAW)
  - Configuration: `NEEDS_MIDI_INPUT TRUE` in `CMakeLists.txt` line 11
  - Implementation: `processBlock()` receives juce::MidiBuffer
  - Used for triggering synth voices and glissando control

- MIDI Output: No
  - Configuration: Plugin does not produce MIDI (`producesMidi() return false` in PluginProcessor.h line 89)

## Summary

**Zero external integrations.** This is a standalone audio synthesis plugin with:
- No cloud connectivity
- No remote data sources
- No third-party service dependencies
- No authentication or licensing checks
- All state stored locally on user's machine
- All DSP computation performed within plugin process

Perfect for offline use and privacy-conscious audio production.

---

*Integration audit: 2026-01-22*
