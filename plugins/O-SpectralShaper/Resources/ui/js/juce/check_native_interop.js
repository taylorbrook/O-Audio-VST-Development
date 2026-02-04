// JUCE native interop checker - required for WebView parameter binding
// This file must exist for JUCE 8 WebView integration

if (typeof window.__JUCE__ === 'undefined') {
    console.warn('JUCE native interop not available - running in standalone browser mode');
}
