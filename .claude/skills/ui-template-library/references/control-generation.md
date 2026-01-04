# Control Generation Logic

This document describes how to generate styled controls when applying an aesthetic to a plugin.

## Core Principle

**Aesthetic defines control styling, parameters define which controls to create.**

When applying an aesthetic, each parameter gets a control styled according to the aesthetic's control definitions, with sizing adapted to layout density.

## Control Type Mapping

Map parameter types to control types:

```javascript
function getControlType(parameter) {
    switch (parameter.type) {
        case 'FLOAT':
            if (isProminentParameter(parameter)) {
                return 'vertical_slider';
            }
            return 'rotary_knob';

        case 'BOOL':
            return 'button';  // toggle button

        case 'CHOICE':
            return 'dropdown';  // combo box

        default:
            return 'rotary_knob';
    }
}
```

## Rotary Knob Generation

### HTML Structure

```html
<div class="control-container">
    <div class="rotary-knob" data-param-id="{{PARAM_ID}}">
        <div class="knob-body">
            <div class="knob-pointer"></div>
            <!-- Tick marks if style includes them -->
            {{#if hasTicks}}
            <div class="tick-marks">
                {{#each ticks}}
                <div class="tick" style="transform: rotate({{angle}}deg)"></div>
                {{/each}}
            </div>
            {{/if}}
            <!-- Center dot if style includes it -->
            {{#if centerDot}}
            <div class="center-dot"></div>
            {{/if}}
        </div>
        <label class="control-label">{{PARAM_NAME}}</label>
        <div class="value-display">{{value}}</div>
    </div>
</div>
```

### CSS Generation

Apply aesthetic styling to knob:

```javascript
function generateKnobCSS(aesthetic, layoutDensity) {
    const knobDef = aesthetic.controls.rotary_knob;
    const size = adaptControlSize(knobDef.diameter, layoutDensity);

    return `
        .rotary-knob {
            position: relative;
            width: ${size}px;
            height: ${size}px;
        }

        .knob-body {
            width: 100%;
            height: 100%;
            border-radius: 50%;
            background: ${knobDef.background};
            border: ${knobDef.border};
            box-shadow: ${knobDef.shadow};
            cursor: pointer;
            user-select: none;
            transition: transform 0.05s ease-out;
        }

        .knob-body:hover {
            filter: brightness(1.1);
        }

        .knob-body:active {
            transform: scale(0.98);
        }

        .knob-pointer {
            position: absolute;
            top: 10%;
            left: 50%;
            transform: translateX(-50%);
            width: 3px;
            height: 30%;
            background: ${knobDef.pointerColor || aesthetic.colors.primary};
            border-radius: 2px;
        }

        ${knobDef.center_dot ? `
        .center-dot {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            width: 8px;
            height: 8px;
            border-radius: 50%;
            background: ${knobDef.pointerColor || aesthetic.colors.primary};
        }
        ` : ''}

        ${knobDef.tick_count ? generateTickMarkCSS(knobDef, size) : ''}

        .control-label {
            display: block;
            text-align: center;
            margin-top: ${aesthetic.spacing.control_label_gap}px;
            font-family: ${aesthetic.typography.body.family};
            font-size: ${aesthetic.typography.body.size};
            color: ${aesthetic.colors.text_primary};
            user-select: none;
        }

        .value-display {
            text-align: center;
            margin-top: 4px;
            font-family: ${aesthetic.typography.mono.family};
            font-size: ${aesthetic.typography.mono.size};
            color: ${aesthetic.colors.text_secondary};
        }
    `;
}
```

### Tick Mark Generation

```javascript
function generateTickMarkCSS(knobDef, size) {
    const tickCount = knobDef.tick_count;
    const angleRange = 270;  // -135° to +135°
    const angleStep = angleRange / (tickCount - 1);
    const startAngle = -135;

    let tickCSS = '.tick-marks { position: relative; width: 100%; height: 100%; }\n';

    for (let i = 0; i < tickCount; i++) {
        const angle = startAngle + (i * angleStep);
        tickCSS += `
        .tick:nth-child(${i + 1}) {
            position: absolute;
            top: 50%;
            left: 50%;
            width: 2px;
            height: ${size * 0.1}px;
            background: ${knobDef.tickColor || aesthetic.colors.text_secondary};
            transform: translate(-50%, -${size * 0.45}px) rotate(${angle}deg);
            transform-origin: center ${size * 0.45}px;
        }
        `;
    }

    return tickCSS;
}
```

### Interaction Logic

```javascript
function setupKnobInteraction(knobElement, paramId, binding) {
    let isDragging = false;
    let startY = 0;
    let startValue = 0;

    knobElement.addEventListener('mousedown', (e) => {
        isDragging = true;
        startY = e.clientY;
        startValue = binding.getParameterValue(paramId);
        e.preventDefault();
    });

    document.addEventListener('mousemove', (e) => {
        if (!isDragging) return;

        const deltaY = startY - e.clientY;
        const sensitivity = 0.005;
        const newValue = Math.max(0, Math.min(1, startValue + deltaY * sensitivity));

        binding.setParameterValue(paramId, newValue);
        updateKnobRotation(knobElement, newValue);
        updateValueDisplay(knobElement, newValue);
    });

    document.addEventListener('mouseup', () => {
        isDragging = false;
    });
}

function updateKnobRotation(knobElement, normalizedValue) {
    const angleRange = 270;
    const startAngle = -135;
    const angle = startAngle + (normalizedValue * angleRange);

    const knobBody = knobElement.querySelector('.knob-body');
    knobBody.style.transform = `rotate(${angle}deg)`;
}
```

## Vertical Slider Generation

### HTML Structure

```html
<div class="control-container">
    <div class="vertical-slider" data-param-id="{{PARAM_ID}}">
        <label class="control-label">{{PARAM_NAME}}</label>
        <div class="slider-track">
            <div class="slider-fill" style="height: {{value}}%"></div>
            <div class="slider-thumb" style="bottom: {{value}}%"></div>
        </div>
        <div class="value-display">{{value}}</div>
    </div>
</div>
```

### CSS Generation

```javascript
function generateSliderCSS(aesthetic, layoutDensity) {
    const sliderDef = aesthetic.controls.vertical_slider;
    const width = adaptControlSize(sliderDef.width, layoutDensity);
    const height = adaptControlSize(sliderDef.height, layoutDensity);

    return `
        .vertical-slider {
            display: flex;
            flex-direction: column;
            align-items: center;
            gap: ${aesthetic.spacing.control_label_gap}px;
        }

        .slider-track {
            position: relative;
            width: ${width}px;
            height: ${height}px;
            background: ${sliderDef.track_color};
            border: ${sliderDef.border};
            border-radius: ${width / 2}px;
            cursor: pointer;
            user-select: none;
        }

        .slider-fill {
            position: absolute;
            bottom: 0;
            left: 0;
            width: 100%;
            background: ${sliderDef.thumb_color};
            border-radius: ${width / 2}px;
            pointer-events: none;
            transition: height 0.05s ease-out;
        }

        .slider-thumb {
            position: absolute;
            left: 50%;
            transform: translate(-50%, 50%);
            width: ${width * 1.5}px;
            height: ${width * 1.5}px;
            background: ${sliderDef.thumb_color};
            border: 2px solid ${aesthetic.colors.background};
            border-radius: 50%;
            box-shadow: ${aesthetic.effects.shadows.level !== 'none' ?
                `0 2px 8px ${aesthetic.effects.shadows.color}` : 'none'};
            cursor: grab;
            transition: transform 0.05s ease-out;
        }

        .slider-thumb:hover {
            transform: translate(-50%, 50%) scale(1.1);
        }

        .slider-thumb:active {
            cursor: grabbing;
            transform: translate(-50%, 50%) scale(1.05);
        }

        .control-label {
            font-family: ${aesthetic.typography.body.family};
            font-size: ${aesthetic.typography.body.size};
            color: ${aesthetic.colors.text_primary};
            user-select: none;
        }

        .value-display {
            font-family: ${aesthetic.typography.mono.family};
            font-size: ${aesthetic.typography.mono.size};
            color: ${aesthetic.colors.text_secondary};
        }
    `;
}
```

### Interaction Logic

```javascript
function setupSliderInteraction(sliderElement, paramId, binding) {
    const track = sliderElement.querySelector('.slider-track');
    let isDragging = false;

    track.addEventListener('mousedown', (e) => {
        isDragging = true;
        updateSliderFromEvent(e, track, paramId, binding);
        e.preventDefault();
    });

    document.addEventListener('mousemove', (e) => {
        if (!isDragging) return;
        updateSliderFromEvent(e, track, paramId, binding);
    });

    document.addEventListener('mouseup', () => {
        isDragging = false;
    });
}

function updateSliderFromEvent(event, track, paramId, binding) {
    const rect = track.getBoundingClientRect();
    const y = event.clientY - rect.top;
    const height = rect.height;

    // Inverted: top of slider = 1.0, bottom = 0.0
    const normalizedValue = Math.max(0, Math.min(1, 1 - (y / height)));

    binding.setParameterValue(paramId, normalizedValue);
    updateSliderVisuals(track, normalizedValue);
}

function updateSliderVisuals(track, normalizedValue) {
    const fill = track.querySelector('.slider-fill');
    const thumb = track.querySelector('.slider-thumb');

    fill.style.height = `${normalizedValue * 100}%`;
    thumb.style.bottom = `${normalizedValue * 100}%`;
}
```

## Button/Toggle Generation

### HTML Structure

```html
<div class="control-container">
    <button class="toggle-button" data-param-id="{{PARAM_ID}}" data-state="{{state}}">
        <span class="button-label">{{PARAM_NAME}}</span>
    </button>
</div>
```

### CSS Generation

```javascript
function generateButtonCSS(aesthetic, layoutDensity) {
    const buttonDef = aesthetic.controls.button;
    const height = adaptControlSize(buttonDef.height, layoutDensity);

    return `
        .toggle-button {
            height: ${height}px;
            padding: ${buttonDef.padding};
            background: ${buttonDef.offColor || aesthetic.colors.surface};
            color: ${aesthetic.colors.text_primary};
            border: ${aesthetic.effects.borders.width}px ${aesthetic.effects.borders.style} ${aesthetic.effects.borders.color};
            border-radius: ${buttonDef.border_radius}px;
            font-family: ${aesthetic.typography.body.family};
            font-size: ${aesthetic.typography.body.size};
            font-weight: ${aesthetic.typography.font_weights.bold};
            cursor: pointer;
            user-select: none;
            transition: all 0.1s ease-out;
        }

        .toggle-button:hover {
            filter: brightness(${buttonDef.hover_brightness});
        }

        .toggle-button:active {
            transform: scale(0.97);
        }

        .toggle-button[data-state="on"] {
            background: ${buttonDef.onColor || aesthetic.colors.primary};
            color: ${buttonDef.text_color || aesthetic.colors.background};
        }

        .button-label {
            display: block;
            text-transform: uppercase;
            letter-spacing: 0.05em;
        }
    `;
}
```

### Interaction Logic

```javascript
function setupButtonInteraction(buttonElement, paramId, binding) {
    buttonElement.addEventListener('click', () => {
        const currentValue = binding.getParameterValue(paramId);
        const newValue = currentValue < 0.5 ? 1.0 : 0.0;

        binding.setParameterValue(paramId, newValue);
        updateButtonState(buttonElement, newValue);
    });
}

function updateButtonState(buttonElement, normalizedValue) {
    const state = normalizedValue >= 0.5 ? 'on' : 'off';
    buttonElement.setAttribute('data-state', state);
}
```

## Control Size Adaptation

Adapt control sizes based on layout density:

```javascript
function adaptControlSize(baseSize, layoutDensity) {
    const multipliers = {
        'spacious': 1.3,   // 1-3 params
        'comfortable': 1.1, // 4-6 params
        'standard': 1.0,    // 7-10 params
        'compact': 0.85,    // 11+ params
    };

    return Math.round(baseSize * (multipliers[layoutDensity] || 1.0));
}

function getLayoutDensity(paramCount) {
    if (paramCount <= 3) return 'spacious';
    if (paramCount <= 6) return 'comfortable';
    if (paramCount <= 10) return 'standard';
    return 'compact';
}
```

## Complete Control Generation

```javascript
function generateControl(parameter, aesthetic, layoutDensity) {
    const controlType = getControlType(parameter);

    switch (controlType) {
        case 'rotary_knob':
            return {
                html: generateKnobHTML(parameter, aesthetic, layoutDensity),
                css: generateKnobCSS(aesthetic, layoutDensity),
                js: generateKnobJS(parameter.id),
            };

        case 'vertical_slider':
            return {
                html: generateSliderHTML(parameter, aesthetic, layoutDensity),
                css: generateSliderCSS(aesthetic, layoutDensity),
                js: generateSliderJS(parameter.id),
            };

        case 'button':
            return {
                html: generateButtonHTML(parameter, aesthetic, layoutDensity),
                css: generateButtonCSS(aesthetic, layoutDensity),
                js: generateButtonJS(parameter.id),
            };

        default:
            throw new Error(`Unknown control type: ${controlType}`);
    }
}
```

## Summary

**Key principles:**
1. Aesthetic defines visual styling (colors, fonts, shadows, borders)
2. Parameter type determines control type (knob, slider, button)
3. Layout density adapts control sizes
4. Interaction logic is standard (binding to APVTS)
5. Visual styling applied consistently across all controls
6. Hover/active states enhance usability
7. Accessibility maintained (keyboard support, ARIA labels)
