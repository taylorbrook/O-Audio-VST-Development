// Parameter Binding Pattern for JUCE WebView Communication

// Example: Knob control bound to JUCE parameter

// 1. HTML knob element
<input type="range"
       id="size-knob"
       class="knob-control"
       min="0"
       max="100"
       value="50">

// 2. JavaScript binding (in v[N]-ui-test.html)
const sizeKnob = document.getElementById('size-knob');

// Listen for user interaction
sizeKnob.addEventListener('input', (e) => {
    const normalizedValue = e.target.value / 100; // Normalize to 0.0-1.0

    // Send to JUCE (mock for test.html, real for production)
    if (window.webkit && window.webkit.messageHandlers.juce) {
        // Production: Send to JUCE
        window.webkit.messageHandlers.juce.postMessage({
            action: 'parameterChange',
            parameterId: 'size',
            value: normalizedValue
        });
    } else {
        // Test mode: Console log
        console.log(`Parameter 'size' changed to ${normalizedValue}`);
    }
});

// Listen for parameter updates from JUCE (automation, preset changes)
window.updateParameter = function(parameterId, value) {
    if (parameterId === 'size') {
        sizeKnob.value = value * 100; // Denormalize from 0.0-1.0
    }
};

// 3. JUCE C++ side (PluginEditor.cpp)
// Parameter listener callback
void parameterChanged(const juce::String& parameterID, float newValue) override
{
    if (parameterID == "size") {
        // Execute JavaScript to update knob
        webView->evaluateJavascript(
            "window.updateParameter('size', " + juce::String(newValue) + ");"
        );
    }
}
