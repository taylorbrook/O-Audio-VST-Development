# Troubleshoot Agent Memory

## Learned Patterns
- General: macOS AU cache must be cleared before installing updated plugins -- killall -9 AudioComponentRegistrar, rm -rf ~/Library/Caches/AudioUnitCache/ and ~/Library/Caches/com.apple.audiounits.cache, then remove old binaries from ~/Library/Audio/Plug-Ins/ before copying fresh builds
- General: Stale plugin behavior in DAW after rebuild -- close DAW completely and restart; some DAWs cache plugin state in memory even after rescan
- General: WebView2 blank page on Windows has three common causes: (1) missing JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 causing dynamic DLL load failure, (2) user data folder access denied in plugin host context, (3) silent fallback to IE backend which lacks resource provider support
- O-Texture: ANIRA dylib distribution requires: embed libanira.dylib + libonnxruntime.dylib in Contents/Frameworks/, set rpath @loader_path/../Frameworks, create versioned symlinks (libanira.2.dylib -> libanira.2.0.3.dylib), fix anira's onnxruntime reference with install_name_tool -change
- General: auval -a | grep -i [pluginname] verifies AU component registration on macOS -- if plugin does not appear, check AU cache clearing and binary installation paths
- General: Windows plugin verification uses DAW plugin scanner (Ableton, FL Studio, Reaper) -- no equivalent of auval for VST3 on Windows
- General: Build targets differ by platform -- macOS uses ninja [PluginName]_VST3 [PluginName]_AU, Windows uses cmake --build build --config Release --target [PluginName]_VST3 --parallel. AU is macOS-only
- General: JUCE 8 getLatencySamples() is NOT virtual -- code that overrides it will compile silently but the override is never called. Use setLatencySamples(N) in prepareToPlay() instead. Four research docs had stale examples of this pattern
- General: Clear Ableton plugin cache on Windows with Remove-Item "$env:APPDATA\Ableton\*\PluginScanDb.txt" when plugin changes are not detected after rebuild
- General: Resource provider path matching -- if WebView shows "Frame load interrupted", check that the provider callback matches bare paths ("/", "/index.html") not full URLs. The fromFirstOccurrenceOf("://") pattern on a bare path returns empty string, failing all lookups

## Common Issues
- Plugin builds successfully but sounds wrong or shows stale UI: always check cache clearing + binary installation sequence first before investigating code
- ANIRA/ONNX Runtime crashes on load: verify dylib symlinks and install_name_tool rpath fixes in Contents/Frameworks/ -- missing symlinks cause dlopen failure at runtime
- FFT-based plugins report wrong latency: check for getLatencySamples() override (no-op in JUCE 8) -- must use setLatencySamples() in prepareToPlay()

## Last Updated
2026-03-07 (seed patterns from Phase 21)
