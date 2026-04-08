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

- O-Formant: 21 relays (20 slider + 1 toggle) works fine with constructor-body init and explicit destructor reset() -- no issues scaling beyond 10 relays
- O-Formant: For XY pad canvas in WebView, use style.width/height in px from parent clientWidth/clientHeight, then canvas.width/height = style * dpr for backing store -- avoids replaced element sizing issues
- O-Formant: Knob drag sensitivity of 200px vertical travel for 0-1 range feels natural for 55px diameter knobs

- O-Wind: TuningExporter API is `toHTML(engine, pluginName)` not `exportHTML(engine)` -- check actual method signatures before copying from O-Bells
- O-Wind: AudioParameterInt for instrumentPreset (0-7) can be bound as WebSliderRelay -- normalize as idx/7 for UI, round(norm*7) for readback
- O-Wind: When replacing std::atomic<int> currentPresetIndex with APVTS param, voice reads from getRawParameterValue() each block -- no pointer needed
- O-Wind: Lambda capturing shared_ptr<FileChooser> + complete callback requires explicit function type `std::function<void(juce::var)>` not `auto complete` for JUCE native function registration

- O-Bowed: 23 relays (21 slider + 2 combo) works fine with constructor-body init -- no issues at scale
- O-Bowed: Canvas visualizations in WebView -- use style.width/height from parent clientWidth/clientHeight, then canvas.width/height = style * dpr for DPR-aware rendering
- O-Bowed: getVisualizationState native function returning JSON string is efficient for 15Hz polling -- parse on JS side with JSON.parse()
- O-Bowed: Schelleng diagram: P_min proportional to v_B/(beta^2 * Z), P_max proportional to v_B/(beta * Z) -- simplified Helmholtz boundary model
- O-Bowed: For conditional visibility of controls (stringTuning knobs based on stringCount), use classList.add/remove('hidden') triggered by valueChangedEvent listener

- O-Reed: 35 relays (28 slider + 6 combo + 1 toggle) with XY pad (boreCharacter x doubleReed) -- use setPointerCapture on the pad element and sliderDragStarted/Ended on both axes simultaneously for smooth 2D dragging
- O-Reed: For skewed NormalisableRange params (toneHoleCutoff skew=0.3), display formatting must invert the skew: rawValue = min + pow(norm, 1/skew) * (max - min) -- otherwise displayed values won't match DAW automation readout
- O-Reed: Collapsible sections with max-height transition work well for 28+ knobs -- keeps instrument panel scannable without overwhelming the user

## Last Updated
2026-04-06 (O-Reed Stage 3)
