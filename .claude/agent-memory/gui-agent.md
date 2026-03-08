# GUI Agent Memory

## Learned Patterns
- General: WebView2 on Windows requires JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 (static linking via WebView2LoaderStatic.lib) -- using JUCE_USE_WIN_WEBVIEW2=1 alone tries dynamic loading of WebView2Loader.dll which is not distributed with the plugin
- General: NEEDS_WEBVIEW2 TRUE in juce_add_plugin() links WebView2LoaderStatic.lib via CMake; JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 auto-defines JUCE_USE_WIN_WEBVIEW2=1 (juce_gui_extra.h:97)
- General: Cross-platform URL schemes differ -- macOS/iOS/Linux use juce://juce.backend/, Windows/Android use https://juce.backend/. Never hard-code; use getResourceProviderRoot() in C++ and getBackendResourceAddress() in JS
- General: Resource provider withResourceProvider() callback receives bare PATHS (e.g., "/", "/index.html", "/js/app.js") NOT full URLs -- use direct equality checks against path strings, never strip scheme/host (fromFirstOccurrenceOf causes empty string and "Frame load interrupted")
- General: WebView2 user data folder must be explicitly set with withUserDataFolder() to a temp directory -- default location may be access-denied in DAW plugin hosts. Pattern: File::getSpecialLocation(File::tempDirectory).getChildFile("PluginName_WebView")
- General: If WebView2 construction fails on Windows, JUCE silently falls back to IE backend which does NOT support resource providers -- results in blank page with no error or warning
- General: Plugin audit (2026-02-06) found 34/35 plugins missing NEEDS_WEBVIEW2 TRUE + JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 -- only O-AnalogEQ had correct cross-platform config
- O-TextureForge: Canvas is a CSS replaced element -- position: absolute with left+right or top+bottom does NOT stretch it (stays at default 300x150). Fix: use explicit width: calc(100% - Npx); height: calc(100% - Npx) instead
- O-TextureForge: DPR-aware canvas rendering requires setting backing store dimensions (canvas.width = clientWidth * dpr) plus ctx.setTransform(dpr, 0, 0, dpr, 0, 0) for crisp Retina display
- General: O-Bells provides the proven resource provider pattern -- direct equality checks against path strings ("/", "/index.html", etc.) with MIME type detection from file extension

## Common Issues
- Blank WebView on Windows: first check static vs dynamic linking flags, then check user data folder permissions, then check if IE fallback occurred
- WebView resource loading fails silently -- if withResourceProvider returns empty Optional for a path, the page shows "Frame load interrupted" with no console error
- JUCE WebBrowserComponent options must be set before construction -- cannot change WebView2 options after the component is created

- O-Gain: For plugins with many atomic metering values (17+), juce::String::formatted() with %f works reliably for passing data from C++ timer to JS updateMeters() -- no need for JSON serialization
- O-Gain: SVG arc knobs via describeArc(cx,cy,r,startAngle,endAngle) with 135-405 degree sweep (270 deg total) render cleanly in WebView; use stroke-dashoffset animation or direct path d= updates
- O-Gain: When a plugin has both APVTS parameters (gain_offset) and transient UI state (learnActive atomic bool), use native functions (toggleLearn) for non-parameter interactions and timer polling for state readback
- O-Gain: ComboBox properties (choices array) may not be available immediately on page load -- listen to propertiesChangedEvent in addition to valueChangedEvent for reliable initial state

## Common Issues
- Blank WebView on Windows: first check static vs dynamic linking flags, then check user data folder permissions, then check if IE fallback occurred
- WebView resource loading fails silently -- if withResourceProvider returns empty Optional for a path, the page shows "Frame load interrupted" with no console error
- JUCE WebBrowserComponent options must be set before construction -- cannot change WebView2 options after the component is created

## Last Updated
2026-03-07 (O-Gain Stage 3)
