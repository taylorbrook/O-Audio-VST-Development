# External Integrations

**Analysis Date:** 2026-01-29

## APIs & External Services

**Not Applicable**
- This is a self-contained audio plugin framework with no external API integrations
- All functionality is built directly into JUCE framework
- No third-party cloud services, SaaS, or external APIs consumed
- Network functionality explicitly disabled: `JUCE_USE_CURL=0` in all plugin configurations

## Data Storage

**Databases:**
- Not used - Plugins maintain state locally only
- No database server connections required
- State persisted locally via preset system (see Persistence below)

**File Storage:**
- Local filesystem only - No cloud storage integrations
- Preset files stored in OS-standard locations:
  - macOS: `~/Library/Audio/Presets/[PluginName]/`
  - Accessible via OuariconPresetManager (`plugins/O-Bass/Source/OuariconPresetManager.h`)
- Plugin state saved/restored via JUCE's `AudioProcessorValueTreeState` serialization

**Caching:**
- No distributed caching system
- Local AU cache cleared during installation: `~/Library/Caches/AudioUnitCache/`
- VST3 scan cache cleared: `~/Library/Caches/com.apple.audiounits.cache/`
- Build artifacts cached in: `build/plugins/[PluginName]/[PluginName]_artefacts/`

## Authentication & Identity

**Auth Provider:**
- Not applicable - No user authentication system
- Plugins are standalone audio processors with no user accounts
- No plugin licensing or DRM system implemented
- Company metadata: "Ouaricon Development" (manufacturer code: OuDv)

## Monitoring & Observability

**Error Tracking:**
- Not integrated - No error reporting service
- Errors handled locally within plugin (see DSP error handling in source code)
- Runtime validation via pluginval tool: validates VST3/AU compliance post-build

**Logs:**
- Local file logging only
- Build logs: `logs/[PluginName]/build_YYYYMMDD_HHMMSS.log`
- No remote logging or telemetry
- Bash scripts use color-coded console output for status reporting

## CI/CD & Deployment

**Hosting:**
- GitHub repositories (source code only)
- Plugins built locally on developer machine: `/Users/taylorbrook/Dev/VST-development/build/`

**CI Pipeline:**
- GitHub Actions workflow: `.github/workflows/build-and-release.yml`
- Trigger: Git tags matching pattern `*-v*` (e.g., `O-Bass-v1.3.1`)
- Builds: macOS (universal binary), Windows, Linux
- Artifacts: GitHub Release with pre-compiled plugin binaries
- JUCE downloaded fresh per build: version 8.0.4 (may differ from development 8.0.9)

**Installation Pipeline:**
- Manual installation via `/install-plugin [Name]` command
- Script: `scripts/build-and-install.sh`
- Destination: System plugin directories
  - VST3: `~/Library/Audio/Plug-Ins/VST3/`
  - AU: `~/Library/Audio/Plug-Ins/Components/`
- Includes cache clearing and verification steps

## Environment Configuration

**Required env vars:**
- None - System is self-contained
- Build uses system default compilers and standard library
- Installation paths hardcoded for macOS standard locations

**Build-time configuration:**
- CMakeLists.txt defines:
  - JUCE location: `/Users/taylorbrook/JUCE/` (hardcoded absolute path)
  - Deployment target: macOS 10.13
  - Plugin name, manufacturer code, plugin code
  - JUCE modules to link
  - WebView resource files

**Secrets location:**
- Not applicable - No secrets management system
- No API keys, credentials, or sensitive configuration required

## Webhooks & Callbacks

**Incoming:**
- Not applicable - Plugins are passive audio processors
- No HTTP endpoints or server functionality

**Outgoing:**
- Not applicable - No external service calls
- Plugins do not initiate network requests
- All audio processing contained within plugin binary

## Module System & Code Reuse

**Modules (Reusable Components):**
- Location: `modules/` directory
- Registry: `modules/registry.yaml` (master index of available modules)
- Categories: core, persistence, metering, tuning, effects, synthesis, ui

**Key Modules Available:**

| Module | Path | Purpose |
|--------|------|---------|
| webview-relay-manager | `modules/core/webview-relay-manager/` | Parameter binding between C++ and WebView UI |
| resource-provider | `modules/core/resource-provider/` | Embedded resource serving to WebView |
| OuariconPresetManager | `plugins/O-Bass/Source/` | Preset file I/O (reused across plugins) |

**Module Integration:**
- CMake-based import pattern (not NPM or package manager)
- Plugins include modules directly in CMakeLists.txt
- No version locking or dependency resolver - modules updated in-place
- Backward compatibility maintained within major versions

## Build Artifact Management

**Compiled Artifacts:**
- Location: `build/plugins/[PluginName]/[PluginName]_artefacts/Release/`
- Formats:
  - `VST3/[PluginName].vst3/` - VST3 plugin (universal binary)
  - `AU/[PluginName].component/` - AudioUnit component (universal binary)
  - `Standalone/[PluginName].app/` - Standalone application (for testing)

**Distribution:**
- GitHub Actions creates signed PKG installers
- Releases available via GitHub Release API
- No software update mechanism (manual re-installation required)

## Development Tooling Integration

**Validation Tool:**
- pluginval (external binary not in repo)
- Validates VST3 plugin compliance with spec
- Validates AU plugin registration and compatibility
- Runs after build completion (blocking validation)

**Version Control:**
- Git repository at `/Users/taylorbrook/Dev/VST-development/`
- Commits tracked in `logs/` directory
- Auto-commits on checkpoint completion

**Testing:**
- No automated test framework integrated
- Manual testing via DAW (Ableton, Logic Pro, Reaper, etc.)
- Standalone mode for headless validation
- Runtime validation via pluginval against VST3/AU specs

---

*Integration audit: 2026-01-29*
