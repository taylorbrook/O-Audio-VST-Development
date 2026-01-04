# HTML Generation Rules

## Key Principles

1. **Production-ready**: HTML IS the final plugin UI, not a prototype
2. **Parameter bindings**: Use `data-param="paramID"` for C++ communication
3. **No external dependencies**: Pure HTML/CSS/JS only
4. **WebView API**: Use `window.juce.postMessage()` for updates
5. **Responsive**: Adapt to window size (if resizable)

## HTML Structure

```html
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>[PluginName]</title>
    <style>
        /* Inline CSS - no external stylesheets */
        body {
            margin: 0;
            padding: 0;
            font-family: Inter, system-ui, sans-serif;
            background: #2b2b2b;
            color: #ffffff;
            overflow: hidden;
        }

        /* Layout styles */
        /* Control styles */
        /* Animation styles */
    </style>
</head>
<body>
    <!-- UI structure matching YAML layout -->
    <div id="plugin-container">
        <div class="control" data-param="threshold">
            <div class="knob"></div>
            <div class="label">Threshold</div>
            <div class="value">-20.0 dB</div>
        </div>
        <!-- ... more controls ... -->
    </div>

    <script>
        // Parameter state
        const params = {
            threshold: -20.0,
            // ... all parameters with defaults from YAML
        };

        // Send parameter change to C++
        function setParameter(paramID, value) {
            params[paramID] = value;
            if (window.juce) {
                window.juce.postMessage({
                    type: 'parameterChange',
                    id: paramID,
                    value: value
                });
            }
            updateDisplay(paramID, value);
        }

        // Receive parameter updates from C++ (automation, presets)
        window.receiveParameterUpdate = function(paramID, value) {
            params[paramID] = value;
            updateDisplay(paramID, value);
        };

        // Update UI display
        function updateDisplay(paramID, value) {
            const control = document.querySelector(`[data-param="${paramID}"]`);
            const valueDisplay = control.querySelector('.value');
            valueDisplay.textContent = formatValue(paramID, value);
            // Update knob/slider position
        }

        // Event listeners for controls
        document.querySelectorAll('.control').forEach(control => {
            const paramID = control.dataset.param;
            // Add mouse/touch handlers
        });
    </script>
</body>
</html>
```

## Parameter Binding Conventions

**Use data attributes:**

```html
<div class="control" data-param="threshold" data-min="-60" data-max="0" data-unit="dB">
```

**Benefits:**

- Clear parameter identification
- Easy C++ parsing
- Self-documenting HTML
- Enables dynamic behavior

## WebView Communication

**C++ → JavaScript (parameter updates):**

```javascript
// C++ calls: webView->evaluateJavascript("receiveParameterUpdate('threshold', -30.5);");
window.receiveParameterUpdate = function(paramID, value) {
    params[paramID] = value;
    updateDisplay(paramID, value);
};
```

**JavaScript → C++ (user interactions):**

```javascript
function setParameter(paramID, value) {
    if (window.juce) {
        window.juce.postMessage({
            type: 'parameterChange',
            id: paramID,
            value: value
        });
    }
}
```

## Value Formatting

**Format values based on type:**

```javascript
function formatValue(paramID, value) {
    const control = document.querySelector(`[data-param="${paramID}"]`);
    const unit = control.dataset.unit || '';

    // Float parameters
    if (unit === 'dB' || unit === 'Hz' || unit === 'ms') {
        return value.toFixed(1) + ' ' + unit;
    }

    // Percentage
    if (unit === '%') {
        return (value * 100).toFixed(0) + '%';
    }

    // Bool parameters
    if (control.dataset.type === 'bool') {
        return value > 0.5 ? 'ON' : 'OFF';
    }

    // Choice parameters
    if (control.dataset.choices) {
        const choices = control.dataset.choices.split(',');
        return choices[Math.round(value * (choices.length - 1))];
    }

    return value.toFixed(2);
}
```

## Control Implementations

### Rotary Knob

```html
<div class="control knob-control" data-param="threshold" data-min="-60" data-max="0">
    <svg class="knob" width="60" height="60">
        <circle cx="30" cy="30" r="25" fill="#333" stroke="#666" stroke-width="2"/>
        <line x1="30" y1="30" x2="30" y2="10" stroke="#4a9eff" stroke-width="3" class="knob-pointer"/>
    </svg>
    <div class="label">Threshold</div>
    <div class="value">-20.0 dB</div>
</div>
```

### Linear Slider

```html
<div class="control slider-control" data-param="mix" data-min="0" data-max="100">
    <div class="label">Mix</div>
    <div class="slider-track">
        <div class="slider-fill" style="width: 50%"></div>
        <div class="slider-handle" style="left: 50%"></div>
    </div>
    <div class="value">50%</div>
</div>
```

### Button/Toggle

```html
<div class="control button-control" data-param="bypass" data-type="bool">
    <button class="toggle-button">
        <span class="label">Bypass</span>
        <span class="state">OFF</span>
    </button>
</div>
```

## Styling Guidelines

**Use CSS custom properties for theming:**

```css
:root {
    --bg-color: #2b2b2b;
    --primary-color: #4a9eff;
    --text-color: #ffffff;
    --control-bg: #333333;
    --control-hover: #444444;
}

body {
    background: var(--bg-color);
    color: var(--text-color);
}

.control {
    background: var(--control-bg);
}

.control:hover {
    background: var(--control-hover);
}
```

**Ensure readability:**

- Minimum font size: 12px
- Sufficient contrast (WCAG AA)
- Clear visual hierarchy

## Responsive Design

**If window resizable:**

```css
@media (max-width: 500px) {
    .control {
        font-size: 0.9em;
    }
}

@media (min-width: 800px) {
    .control {
        font-size: 1.1em;
    }
}
```

## Performance

**Optimize for 60fps:**

- Use CSS transforms (GPU-accelerated)
- Throttle mouse events (requestAnimationFrame)
- Avoid layout thrashing
- Minimize DOM queries

```javascript
let animationId;
function updateKnob(paramID, value) {
    if (animationId) return;
    animationId = requestAnimationFrame(() => {
        // Update knob rotation
        animationId = null;
    });
}
```
