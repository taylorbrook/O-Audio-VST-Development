/*
  ==============================================================================

    OuariconPolystutter - Parameter Bindings (Phase 3.2)
    JUCE WebView ↔ APVTS synchronization

    CRITICAL: Parameter IDs must match PluginProcessor.cpp APVTS exactly

    v1.4.0: Added double-click text input for knob values

  ==============================================================================
*/

import * as Juce from "./juce/index.js";

// ========== v1.4.0: DOUBLE-CLICK TEXT INPUT FOR KNOBS ==========

/**
 * Active text input state (only one input can be active at a time)
 */
let activeTextInput = null;

/**
 * Start text input mode for a knob
 * @param {HTMLElement} valueElement - The knob-value element to edit
 * @param {object} state - JUCE slider state
 * @param {number} min - Parameter minimum value
 * @param {number} max - Parameter maximum value
 * @param {function} formatter - Value display formatter
 * @param {HTMLElement} knobElement - The knob element (for UI update)
 */
function startKnobTextInput(valueElement, state, min, max, formatter, knobElement) {
  // Cancel any existing input
  if (activeTextInput) {
    cancelTextInput();
  }

  // Get current actual value (not normalized)
  const currentNormalized = state.getNormalisedValue();
  const currentValue = min + currentNormalized * (max - min);

  // Extract raw number from displayed value (remove suffix like %, st)
  let displayNumber;
  if (formatter === null) {
    // Integer or raw number - just round
    displayNumber = Math.round(currentValue);
  } else {
    // Use the actual value, formatted appropriately for input
    const formatted = formatter(currentValue);
    // Convert to string first (formatter may return number or string)
    const formattedStr = String(formatted);
    // Extract number from formatted string (e.g., "50%" -> 50, "0st" -> 0, "4" -> 4)
    const match = formattedStr.match(/-?\d+\.?\d*/);
    displayNumber = match ? match[0] : Math.round(currentValue);
  }

  // Store original value for cancel
  const originalNormalized = currentNormalized;

  // Create input element
  const input = document.createElement("input");
  input.type = "text";
  input.className = "knob-value-input";
  input.value = displayNumber;
  input.setAttribute("data-original", originalNormalized);

  // Store context for handlers
  activeTextInput = {
    input,
    valueElement,
    knobElement,
    state,
    min,
    max,
    formatter,
    originalNormalized
  };

  // Hide value text and show input
  valueElement.style.display = "none";
  valueElement.parentNode.insertBefore(input, valueElement.nextSibling);

  // Focus and select all
  input.focus();
  input.select();

  // Handle Enter key (confirm)
  input.addEventListener("keydown", (e) => {
    if (e.key === "Enter") {
      e.preventDefault();
      confirmTextInput();
    } else if (e.key === "Escape") {
      e.preventDefault();
      cancelTextInput();
    }
  });

  // Handle blur (click away = confirm)
  input.addEventListener("blur", () => {
    // Small delay to allow click events to process first
    setTimeout(() => {
      if (activeTextInput && activeTextInput.input === input) {
        confirmTextInput();
      }
    }, 10);
  });
}

/**
 * Confirm text input and apply value
 */
function confirmTextInput() {
  if (!activeTextInput) return;

  const { input, valueElement, knobElement, state, min, max, formatter } = activeTextInput;

  // Parse input value
  const inputText = input.value.trim();
  let rawValue = parseFloat(inputText);

  if (isNaN(rawValue)) {
    // Invalid input - cancel instead
    cancelTextInput();
    return;
  }

  // Determine if this is a percentage parameter (0-1 normalized displayed as 0-100%)
  const isPercentage = (min === 0 && max === 1) || (min === 0 && max === 100);

  // For percentage parameters displayed as 0-100%, user types 0-100
  if (min === 0 && max === 1) {
    // User types 50 to mean 50% = 0.5 normalized
    rawValue = rawValue / 100;
  }

  // Clamp to valid range
  const clampedValue = Math.max(min, Math.min(max, rawValue));

  // Convert to normalized (0-1)
  const normalized = (clampedValue - min) / (max - min);

  // Apply to JUCE
  state.setNormalisedValue(normalized);

  // Update UI
  updateKnobUI(knobElement, valueElement, normalized, min, max, formatter);

  // Clean up
  cleanupTextInput();
}

/**
 * Cancel text input and revert to original value
 */
function cancelTextInput() {
  if (!activeTextInput) return;

  const { valueElement, knobElement, state, min, max, formatter, originalNormalized } = activeTextInput;

  // Restore original value display
  updateKnobUI(knobElement, valueElement, originalNormalized, min, max, formatter);

  // Clean up
  cleanupTextInput();
}

/**
 * Clean up text input state
 */
function cleanupTextInput() {
  if (!activeTextInput) return;

  const { input, valueElement } = activeTextInput;

  // Remove input element
  if (input.parentNode) {
    input.parentNode.removeChild(input);
  }

  // Show value text again
  valueElement.style.display = "";

  activeTextInput = null;
}

// ========== INITIALIZATION ==========

document.addEventListener("DOMContentLoaded", () => {
  console.log("[Phase 3.2] Initializing parameter bindings...");
  console.log("[Phase 3.2] JUCE backend:", typeof window.__JUCE__ !== "undefined");

  // Bind all parameters (64 main + 64 pattern steps + 2 mix = 130 total) - v1.3.0: removed 2 (ENV/SC)
  bindAllLaneParameters();
  bindTapeParameters();
  bindMixParameters();  // v1.1.0: Wet/dry mix knobs
  bindGlobalToggles();

  // Phase 3.3: Pattern sequencer step bindings
  bindPatternSteps();

  // Phase 3.3: Visual state updates
  setupLaneDimming();
  setupFreezeIndicators();
  setupSequencerDimming();  // v1.1.4: Grey out sequencer when SEQ toggle is off

  // v1.5.0: Lane progress bar updates (from Timer in PluginEditor)
  setupLaneProgressListener();

  console.log("[v1.5.0] All parameter bindings initialized (128 params + progress bars)");
});

// ========== LANE PARAMETERS (4 lanes × 13 params = 52, excluding subdivision) ==========

function bindAllLaneParameters() {
  for (let lane = 1; lane <= 4; lane++) {
    bindLaneParameters(lane);
  }
}

function bindLaneParameters(laneNum) {
  const prefix = `lane${laneNum}`;

  // Knobs (8 sliders)
  bindKnob(`${prefix}_repeats`, 1, 16, (v) => Math.round(v)); // Int
  bindKnob(`${prefix}_decay`, 0, 1, (v) => `${Math.round(v * 100)}%`); // Float 0-100%
  bindKnob(`${prefix}_pitch`, -12, 12, (v) => `${v.toFixed(1)}st`); // Float ±12 semitones
  bindKnob(`${prefix}_filter`, -100, 100, (v) => Math.round(v)); // Float ±100
  bindKnob(`${prefix}_probability`, 0, 1, (v) => `${Math.round(v * 100)}%`); // Float 0-100%
  bindKnob(`${prefix}_volume`, 0, 1, (v) => `${Math.round(v * 100)}%`); // Float 0-100%
  bindKnob(`${prefix}_pan`, -100, 100, (v) => Math.round(v)); // Float ±100
  bindKnob(`${prefix}_swing`, 0, 1, (v) => `${Math.round(v * 100)}%`); // Float 0-100%

  // Toggles (5 buttons: enabled, pingpong, reverse, manual, freeze)
  bindToggle(`${prefix}_enabled`);
  bindToggle(`${prefix}_pingpong`);
  bindToggle(`${prefix}_reverse`);
  bindToggle(`${prefix}_manual_time_enabled`, `${prefix}_manual`); // HTML ID different
  bindToggle(`${prefix}_freeze`);

  // Subdivision combo box (Choice parameter)
  bindComboBox(`${prefix}_subdivision`);
}

// ========== TAPE DEGRADATION PARAMETERS (6 knobs) ==========

function bindTapeParameters() {
  // CRITICAL: HTML IDs use short names (saturation), but APVTS uses "tape_saturation"
  bindKnob("tape_saturation", 0, 1, (v) => `${Math.round(v * 100)}%`, "saturation");
  bindKnob("tape_wow", 0, 1, (v) => `${Math.round(v * 100)}%`, "wow");
  bindKnob("tape_flutter", 0, 1, (v) => `${Math.round(v * 100)}%`, "flutter");
  bindKnob("tape_hiss", 0, 1, (v) => `${Math.round(v * 100)}%`, "hiss");
  bindKnob("tape_rolloff", 0, 1, (v) => `${Math.round(v * 100)}%`, "rolloff");
  bindKnob("tape_dropout", 0, 1, (v) => `${Math.round(v * 100)}%`, "dropout");
}

// ========== v1.1.0: WET/DRY MIX PARAMETERS (2 knobs) ==========

function bindMixParameters() {
  // Dry: 0-100% controls original signal level
  // Wet: 0-100% controls stutter effect level
  bindKnob("mix_dry", 0, 100, (v) => `${Math.round(v)}%`);
  bindKnob("mix_wet", 0, 100, (v) => `${Math.round(v)}%`);
  console.log("[v1.1.0] Bound wet/dry mix knobs");
}

// ========== GLOBAL TOGGLES (3 footer buttons - v1.3.0: Removed ENV and SC) ==========

function bindGlobalToggles() {
  bindToggle("sequencer_enabled", "seq_toggle");  // v1.0.2: Sequencer enable toggle
  bindToggle("midi_enabled", "midi_toggle");
  bindToggle("manual_trigger", "trig_toggle");
}

// ========== KNOB BINDING (Rotary sliders with relative drag) ==========

/**
 * Bind rotary knob to JUCE slider parameter
 * @param {string} paramId - APVTS parameter ID (e.g., "lane1_repeats")
 * @param {number} min - Parameter minimum value
 * @param {number} max - Parameter maximum value
 * @param {function} formatter - Value display formatter (normalized → string)
 * @param {string} htmlId - HTML element ID (defaults to paramId if different)
 */
function bindKnob(paramId, min, max, formatter, htmlId = null) {
  const elementId = htmlId || paramId;
  const knobElement = document.getElementById(elementId);
  const valueElement = document.getElementById(`${elementId}_value`);

  if (!knobElement) {
    console.error(`[Phase 3.2] Knob element not found: ${elementId}`);
    return;
  }

  // Get JUCE slider state for this parameter
  const state = Juce.getSliderState(paramId);
  if (!state) {
    console.error(`[Phase 3.2] JUCE slider state not found: ${paramId}`);
    return;
  }

  // Initialize UI with current value
  updateKnobUI(knobElement, valueElement, state.getNormalisedValue(), min, max, formatter);

  // Pattern #11: Relative drag interaction (frame delta, not absolute)
  let isDragging = false;
  let lastY = 0;

  knobElement.addEventListener("mousedown", (e) => {
    isDragging = true;
    lastY = e.clientY;
    knobElement.style.cursor = "grabbing";
    e.preventDefault(); // Prevent text selection
  });

  document.addEventListener("mousemove", (e) => {
    if (!isDragging) return;

    const deltaY = lastY - e.clientY; // Inverted (up = increase)
    lastY = e.clientY;

    // Sensitivity: 1px drag = 0.005 normalized change (200px for full range)
    const sensitivity = 0.005;
    const normalizedDelta = deltaY * sensitivity;

    // Update normalized value (clamped 0-1)
    // CRITICAL: Use getNormalisedValue() and setNormalisedValue() - direct property access doesn't emit events
    const currentValue = state.getNormalisedValue();
    const newValue = Math.max(0, Math.min(1, currentValue + normalizedDelta));
    state.setNormalisedValue(newValue); // Method call triggers JUCE backend update

    // Update UI immediately for smooth feedback
    updateKnobUI(knobElement, valueElement, newValue, min, max, formatter);
  });

  document.addEventListener("mouseup", () => {
    if (isDragging) {
      isDragging = false;
      knobElement.style.cursor = "pointer";
    }
  });

  // Pattern #10: valueChangedEvent callback receives NO parameters
  // Use getNormalisedValue() inside callback
  state.valueChangedEvent.addListener(() => {
    const newValue = state.getNormalisedValue(); // Use method to get normalized value
    updateKnobUI(knobElement, valueElement, newValue, min, max, formatter);
  });

  // v1.4.0: Double-click on knob or value to enter text input mode
  const handleDoubleClick = (e) => {
    e.preventDefault();
    e.stopPropagation();
    startKnobTextInput(valueElement, state, min, max, formatter, knobElement);
  };

  knobElement.addEventListener("dblclick", handleDoubleClick);
  if (valueElement) {
    valueElement.addEventListener("dblclick", handleDoubleClick);
  }

  console.log(`[Phase 3.2] Bound knob: ${paramId} (HTML: ${elementId})`);
}

/**
 * Update knob visual rotation and value display
 */
function updateKnobUI(knobElement, valueElement, normalized, min, max, formatter) {
  // Rotation: 0% = -135°, 100% = +135° (270° total range)
  const degrees = -135 + normalized * 270;
  knobElement.style.transform = `rotate(${degrees}deg)`;

  // Display value using formatter
  if (valueElement) {
    const actualValue = min + normalized * (max - min);
    valueElement.textContent = formatter(actualValue);
  }
}

// ========== TOGGLE BINDING (Boolean buttons) ==========

/**
 * Bind toggle button to JUCE boolean parameter
 * @param {string} paramId - APVTS parameter ID (e.g., "lane1_enabled")
 * @param {string} htmlId - HTML element ID (defaults to paramId if different)
 */
function bindToggle(paramId, htmlId = null) {
  const elementId = htmlId || paramId;
  const toggleElement = document.getElementById(elementId);

  if (!toggleElement) {
    console.error(`[Phase 3.2] Toggle element not found: ${elementId}`);
    return;
  }

  // Get JUCE toggle state for this parameter
  const state = Juce.getToggleState(paramId);
  if (!state) {
    console.error(`[Phase 3.2] JUCE toggle state not found: ${paramId}`);
    return;
  }

  // Initialize UI with current value
  updateToggleUI(toggleElement, state.value);

  // User interaction: click toggles state
  toggleElement.addEventListener("click", () => {
    // CRITICAL: Use setValue() to emit event to JUCE backend - direct assignment doesn't work
    state.setValue(!state.getValue());
    updateToggleUI(toggleElement, state.getValue());
  });

  // JUCE automation: update UI when parameter changes
  state.valueChangedEvent.addListener(() => {
    updateToggleUI(toggleElement, state.value);
  });

  console.log(`[Phase 3.2] Bound toggle: ${paramId} (HTML: ${elementId})`);
}

/**
 * Update toggle button visual state
 */
function updateToggleUI(toggleElement, isActive) {
  if (isActive) {
    toggleElement.classList.add("active");
  } else {
    toggleElement.classList.remove("active");
  }
}

// ========== COMBOBOX BINDING (Choice parameters) ==========

/**
 * Bind combo box (dropdown) to JUCE choice parameter
 * @param {string} paramId - APVTS parameter ID (e.g., "lane1_subdivision")
 */
function bindComboBox(paramId) {
  const comboElement = document.getElementById(paramId);

  if (!comboElement) {
    console.error(`[Phase 3.2] ComboBox element not found: ${paramId}`);
    return;
  }

  // Get JUCE combo box state for this parameter
  const state = Juce.getComboBoxState(paramId);
  if (!state) {
    console.error(`[Phase 3.2] JUCE combo state not found: ${paramId}`);
    return;
  }

  // Subdivision choices (from parameter-spec.md)
  const choices = ["1/4", "1/8", "1/16", "1/32", "1/8T", "1/16T"];

  // Initialize UI with current value
  // CRITICAL: Use getChoiceIndex() instead of non-existent selectedId property
  updateComboBoxUI(comboElement, state.getChoiceIndex(), choices);

  // User interaction: click cycles through choices
  comboElement.addEventListener("click", () => {
    // CRITICAL: Use getChoiceIndex() and setChoiceIndex() to properly communicate with JUCE
    const nextId = (state.getChoiceIndex() + 1) % choices.length;
    state.setChoiceIndex(nextId); // Method call triggers JUCE backend update
    updateComboBoxUI(comboElement, nextId, choices);
  });

  // JUCE automation: update UI when parameter changes
  state.valueChangedEvent.addListener(() => {
    updateComboBoxUI(comboElement, state.getChoiceIndex(), choices);
  });

  console.log(`[Phase 3.2] Bound combo box: ${paramId}`);
}

/**
 * Update combo box display text
 */
function updateComboBoxUI(comboElement, selectedId, choices) {
  comboElement.textContent = choices[selectedId] || "—";
}

// ========== PATTERN SEQUENCER STEPS (Phase 3.3) ==========

/**
 * Bind pattern sequencer step buttons (64 steps: 4 lanes × 16 steps)
 * Parameters: pattern_lane[N]_step[M] where N=1-4, M=1-16
 */
function bindPatternSteps() {
  for (let lane = 1; lane <= 4; lane++) {
    for (let step = 1; step <= 16; step++) {
      bindPatternStep(lane, step);
    }
  }
  console.log("[Phase 3.3] Bound 64 pattern step buttons");
}

/**
 * Bind individual pattern step button
 */
function bindPatternStep(laneNum, stepNum) {
  const paramId = `pattern_lane${laneNum}_step${stepNum}`;

  // Find step button by data attributes
  const stepButton = document.querySelector(
    `.step-button[data-lane="${laneNum}"][data-step="${stepNum}"]`
  );

  if (!stepButton) {
    console.error(`[Phase 3.3] Step button not found: lane ${laneNum} step ${stepNum}`);
    return;
  }

  // Get JUCE toggle state for this parameter
  const state = Juce.getToggleState(paramId);
  if (!state) {
    console.error(`[Phase 3.3] JUCE toggle state not found: ${paramId}`);
    return;
  }

  // Initialize UI with current value
  updateStepButtonUI(stepButton, state.value);

  // User interaction: click toggles step on/off
  stepButton.addEventListener("click", () => {
    // CRITICAL: Use setValue() to emit event to JUCE backend
    state.setValue(!state.getValue());
    updateStepButtonUI(stepButton, state.getValue());
  });

  // JUCE automation: update UI when parameter changes
  state.valueChangedEvent.addListener(() => {
    updateStepButtonUI(stepButton, state.value);
  });
}

/**
 * Update step button visual state
 */
function updateStepButtonUI(stepButton, isActive) {
  if (isActive) {
    stepButton.classList.add("active");
  } else {
    stepButton.classList.remove("active");
  }
}

// ========== LANE DIMMING (Phase 3.3) ==========

/**
 * Setup lane dimming when lane is disabled
 * Watches lane[N]_enabled parameters and dims entire lane section
 */
function setupLaneDimming() {
  for (let lane = 1; lane <= 4; lane++) {
    setupLaneDimmingForLane(lane);
  }
  console.log("[Phase 3.3] Lane dimming handlers installed");
}

function setupLaneDimmingForLane(laneNum) {
  const paramId = `lane${laneNum}_enabled`;
  const laneContainer = document.querySelector(`.lane${laneNum}`);

  if (!laneContainer) {
    console.warn(`[Phase 3.3] Lane container not found: lane${laneNum}`);
    return;
  }

  // Get JUCE toggle state
  const state = Juce.getToggleState(paramId);
  if (!state) {
    console.error(`[Phase 3.3] JUCE toggle state not found: ${paramId}`);
    return;
  }

  // Apply initial state
  applyLaneDimming(laneContainer, state.value);

  // Update on parameter change
  state.valueChangedEvent.addListener(() => {
    applyLaneDimming(laneContainer, state.value);
  });
}

function applyLaneDimming(laneContainer, isEnabled) {
  if (isEnabled) {
    laneContainer.classList.remove("lane-disabled");
  } else {
    laneContainer.classList.add("lane-disabled");
  }
}

// ========== FREEZE INDICATORS (Phase 3.3) ==========

/**
 * Setup freeze visual indicators
 * Watches lane[N]_freeze parameters and shows visual feedback
 */
function setupFreezeIndicators() {
  for (let lane = 1; lane <= 4; lane++) {
    setupFreezeIndicatorForLane(lane);
  }
  console.log("[Phase 3.3] Freeze indicator handlers installed");
}

function setupFreezeIndicatorForLane(laneNum) {
  const paramId = `lane${laneNum}_freeze`;
  const freezeButton = document.getElementById(paramId);

  if (!freezeButton) {
    console.warn(`[Phase 3.3] Freeze button not found: ${paramId}`);
    return;
  }

  // Get JUCE toggle state
  const state = Juce.getToggleState(paramId);
  if (!state) {
    console.error(`[Phase 3.3] JUCE toggle state not found: ${paramId}`);
    return;
  }

  // Apply initial state
  applyFreezeIndicator(freezeButton, state.value);

  // Update on parameter change
  state.valueChangedEvent.addListener(() => {
    applyFreezeIndicator(freezeButton, state.value);
  });
}

function applyFreezeIndicator(freezeButton, isFrozen) {
  if (isFrozen) {
    freezeButton.classList.add("frozen");
  } else {
    freezeButton.classList.remove("frozen");
  }
}

// ========== v1.1.4: SEQUENCER DIMMING (when SEQ toggle is off) ==========

/**
 * Setup sequencer section dimming when SEQ toggle is disabled
 * Watches sequencer_enabled parameter and dims pattern grid
 */
function setupSequencerDimming() {
  const sequencerSection = document.querySelector(".sequencer-section");

  if (!sequencerSection) {
    console.warn("[v1.1.4] Sequencer section not found");
    return;
  }

  // Get JUCE toggle state for sequencer_enabled
  const state = Juce.getToggleState("sequencer_enabled");
  if (!state) {
    console.error("[v1.1.4] JUCE toggle state not found: sequencer_enabled");
    return;
  }

  // Apply initial state
  applySequencerDimming(sequencerSection, state.value);

  // Update on parameter change
  state.valueChangedEvent.addListener(() => {
    applySequencerDimming(sequencerSection, state.value);
  });

  console.log("[v1.1.4] Sequencer dimming handler installed");
}

function applySequencerDimming(sequencerSection, isEnabled) {
  if (isEnabled) {
    sequencerSection.classList.remove("seq-disabled");
  } else {
    sequencerSection.classList.add("seq-disabled");
  }
}

// ========== DEBUG MONITOR (test HTML only, not for production) ==========

// Update debug monitor on any parameter change (if element exists)
window.addEventListener("load", () => {
  const debugParam = document.getElementById("debugParam");
  const debugValue = document.getElementById("debugValue");
  const debugNormalized = document.getElementById("debugNormalized");

  if (!debugParam) return; // No debug monitor in production build

  // Example: monitor lane1_repeats for testing
  const testParamId = "lane1_repeats";
  const state = Juce.getSliderState(testParamId);

  if (state) {
    state.valueChangedEvent.addListener(() => {
      debugParam.textContent = testParamId;
      debugValue.textContent = `Value: ${Math.round(state.normalisedValue * 15 + 1)}`; // 1-16
      debugNormalized.textContent = `Norm: ${state.normalisedValue.toFixed(3)}`;
    });
  }
});

// ========== v1.5.0: LANE PROGRESS BARS ==========

/**
 * Setup listener for lane progress events from PluginEditor Timer
 * Receives JSON payload: {"lane1":{"progress":0.5,"active":true},...}
 */
function setupLaneProgressListener() {
  // Get progress bar fill elements
  const progressElements = {
    lane1: document.querySelector("#lane1_progress .progress-fill"),
    lane2: document.querySelector("#lane2_progress .progress-fill"),
    lane3: document.querySelector("#lane3_progress .progress-fill"),
    lane4: document.querySelector("#lane4_progress .progress-fill")
  };

  // Get progress bar containers for dimming
  const progressContainers = {
    lane1: document.getElementById("lane1_progress"),
    lane2: document.getElementById("lane2_progress"),
    lane3: document.getElementById("lane3_progress"),
    lane4: document.getElementById("lane4_progress")
  };

  // Check if elements exist
  const missingElements = Object.entries(progressElements)
    .filter(([, el]) => !el)
    .map(([name]) => name);

  if (missingElements.length > 0) {
    console.warn(`[v1.5.0] Progress bar fill elements not found: ${missingElements.join(", ")}`);
  }

  // Listen for custom event from JUCE
  // JUCE emitEventIfBrowserIsVisible("laneProgress", ...) calls backend.emitByBackend()
  // which passes the parsed JSON object directly to listeners (not as event.detail)
  if (window.__JUCE__ && window.__JUCE__.backend) {
    window.__JUCE__.backend.addEventListener("laneProgress", (payload) => {
      try {
        // payload is the JSON string from C++, parse it
        const data = typeof payload === "string" ? JSON.parse(payload) : payload;

        // Update each lane's progress bar
        for (let i = 1; i <= 4; i++) {
          const laneKey = `lane${i}`;
          const laneData = data[laneKey];
          const fillElement = progressElements[laneKey];
          const container = progressContainers[laneKey];

          if (fillElement && laneData) {
            // Set width based on progress (0-100%)
            const widthPercent = Math.round(laneData.progress * 100);
            fillElement.style.width = `${widthPercent}%`;

            // Apply dimmed state when lane is not actively repeating
            if (container) {
              if (laneData.active) {
                container.classList.remove("progress-inactive");
                container.classList.add("progress-active");
              } else {
                container.classList.remove("progress-active");
                container.classList.add("progress-inactive");
              }
            }
          }
        }
      } catch (e) {
        console.error("[v1.5.0] Error parsing lane progress data:", e);
      }
    });

    console.log("[v1.5.0] Lane progress listener installed");
  } else {
    console.warn("[v1.5.0] JUCE backend not available - progress bars will not animate");
  }
}
