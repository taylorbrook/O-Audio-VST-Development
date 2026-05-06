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

- Error resolved: {"parentUuid":"941c4386-d08b-44c7-9980-9bb4a5899a27","isSidechain":true,"agentId":"aa721dd207d88eca1

- Error resolved: {"parentUuid":"e7e32580-3173-495c-b879-e8b075b8d4b8","isSidechain":true,"promptId":"1af0c0a2-87c0-44
- Error resolved: {"parentUuid":"7851605e-5263-4bcc-af03-133e6b72b98f","isSidechain":true,"promptId":"1af0c0a2-87c0-44
- Error resolved: {"parentUuid":"4909afa1-6ddb-4376-8541-e816243156e9","isSidechain":true,"promptId":"1af0c0a2-87c0-44
- Error resolved: {"parentUuid":"2d788c8d-125f-43d9-b18c-fdccc3f306e6","isSidechain":true,"agentId":"a2ac2d05aeb447215
- Error resolved: {"parentUuid":"2d7369f6-6eea-43c8-be39-bb7b7972a543","isSidechain":true,"agentId":"a2ac2d05aeb447215

- Error resolved: {"parentUuid":"5ef4bd8b-50ea-459d-ba93-75e41191a297","isSidechain":true,"promptId":"1af0c0a2-87c0-44
- Error resolved: {"parentUuid":"c62cbb62-662a-49ec-aba8-d3299ea0caca","isSidechain":true,"promptId":"1af0c0a2-87c0-44
- Error resolved: {"parentUuid":"9be1b1fc-7a7d-459c-9264-04e4b11a759e","isSidechain":true,"agentId":"ad78bc7e2f6001c74

- Error resolved: {"parentUuid":"e8eed1b2-e346-4b2a-a9d0-4df087cc0bb4","isSidechain":true,"promptId":"1af0c0a2-87c0-44

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

- O-MicrotonalSampler: For per-cell sample replace UX (sampler-style plugins), make SampleSlot::audio a std::shared_ptr<juce::AudioBuffer<float>> at the SampleMap layer -- map deep-copy on cell replace becomes vector-of-pointers (cheap) instead of vector-of-buffers (700MB on full orchestral library). Voices keep currentMap snapshot pinned for active-note duration; transitive ref keeps the buffer alive even if the map gets replaced mid-note.
- O-MicrotonalSampler: When carrying tuning-panel.{js,css} verbatim from O-Bells in display-only mode, the right approach is (1) selectively register only read-side native functions (the panel's setter calls fail-silently in its own try/catch) + (2) overlay tuning-panel-readonly.css to hide write affordances + (3) JS shim that walks .interval-input nodes after init() and inserts <span class="interval-display"> next to each. Don't fork tuning-panel.js -- the in-flight generalize-microtones extraction relies on verbatim copies.
- O-MicrotonalSampler: JUCE binary data symbol naming for hyphenated filenames -- "sampler-shell.css" -> samplershell_css (hyphens stripped), "tuning-panel.js" -> tuningpanel_js, "tuning-panel-readonly.css" -> tuningpanelreadonly_css. Verify against the auto-generated BinaryData.h after first build before referencing in PluginEditor.cpp.
- O-MicrotonalSampler: Stage 2/3 hand-off pattern when a Stage 3 invariant addition modifies Stage 2 source files -- run a Task-4-style regression gate (build + cache-clear + install + pluginval --strictness 5 + auval) on the pre-editor snapshot to isolate any Stage 2 audio regression to the buffer-ownership change. Only then proceed to editor work. The render-harness identity test from the plan can be substituted by pluginval+auval if no harness exists.
- O-MicrotonalSampler: For sampleMapUpdated push events, the cleanest plumbing is processor::setSampleMapChangedCallback(std::function<void()>) -- the editor's lambda captures `this` and emits webView->emitEventIfBrowserIsVisible("sampleMapUpdated", snapshotJson()). Editor MUST clear the callback in its destructor (processorRef.setSampleMapChangedCallback(nullptr)) to prevent post-destruction calls.

- O-Bassoon: When mockup pass is skipped per user authority, lift O-Wind index.html structure verbatim (palette, knob web-component, tab pattern, lazy tuning mount) -- adapting only the section markup + parameter IDs + push-channel JS receivers takes ~1 file write and produces a working UI on first build. No CSS surprises at 900x600.
- O-Bassoon: Static-check #17 (no fromFirstOccurrenceOf) is grep-literal -- comment text containing "fromFirstOccurrenceOf" trips the gate. Reword regression-warning comments to avoid the literal substring (e.g. "never strip a scheme/host" instead of "never use fromFirstOccurrenceOf"). Same applies to #21 (window.__JUCE__ in any comment trips the inverse-form check) -- describe the wrong pattern abstractly instead of literally.
- O-Bassoon: Effective-breath snapshot for UI feedback can come from BassoonVoice::breathSmoother.getCurrentValue() directly (no separate processor-level CC2 atomic needed) -- breathSmoother already composes ui_breath x cc2_normalised in setExpression()/controllerMoved(), so first-active-voice sampling gives the exact value the audio thread is rendering. Add a 1-line const accessor on BassoonVoice and read first active voice in processBlock prologue.
- O-Bassoon: Vibrato member is named `onset` (SmoothedValue), NOT `onsetEnvelope` -- always check actual source rather than skeleton naming when adding header-inline accessors. Vibrato::getEnvelope() const noexcept { return onset.getCurrentValue(); }
- O-Bassoon: TuningExporter::toHTML(engine, "PluginName") is the 2-arg signature in O-Wind precedent -- not toHTML(engine) or exportHTML(engine). Lift the lambda body verbatim from O-Wind PluginEditor.cpp:461-481 with only the plugin-name string swapped.
- O-Bassoon: Ouaricon dev-suffix plugins do not enumerate via `auval -a` reliably (custom registrar quirk on macOS) -- but `auval -v aumu OBsn OuDv` direct validation works and is the load-bearing test. Use OuDv (dev-suffix manufacturer code), NOT Ouar, for any plugin built with OUARICON_DEV_SUFFIX active.

## Last Updated
2026-05-01 (auto write-back)