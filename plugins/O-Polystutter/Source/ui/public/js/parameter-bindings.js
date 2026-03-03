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

  setupSequencerDimming();  // v1.1.4: Grey out sequencer when SEQ toggle is off
  setupTapeBypassDimming();  // v1.6.5: Grey out tape knobs when bypass is on
  setupEuclideanMode();  // v1.9.0: Euclidean rhythm toggle show/hide + pattern preview

  // v1.5.0: Lane progress bar updates (from Timer in PluginEditor)
  setupLaneProgressListener();

  console.log("[v1.9.0] All parameter bindings initialized (140 params + progress bars)");
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

  // v1.7.0: Pitch randomization knobs (min/max range -12 to +12)
  // Formatter respects quantize state for display
  bindKnob(`${prefix}_pitch_rand_min`, -12, 12, (v) => formatPitchRandValue(v, laneNum));
  bindKnob(`${prefix}_pitch_rand_max`, -12, 12, (v) => formatPitchRandValue(v, laneNum));

  // Toggles (4 buttons: enabled, pingpong, reverse, manual)
  bindToggle(`${prefix}_enabled`);
  bindToggle(`${prefix}_pingpong`);
  bindToggle(`${prefix}_reverse`);
  bindToggle(`${prefix}_manual_time_enabled`, `${prefix}_manual`); // HTML ID different

  // v1.7.0: Pitch randomization toggles
  bindToggle(`${prefix}_pitch_rand_enabled`);
  bindToggle(`${prefix}_pitch_rand_quantize`);

  // v1.9.0: Euclidean rhythm controls
  bindToggle(`${prefix}_euclidean_enabled`);
  // v1.11.0: PLS/STP changed from knobs to dropdown menus
  bindDropdown(`${prefix}_euclidean_pulses`, 1, 16);
  bindDropdown(`${prefix}_euclidean_steps`, 2, 16);

  // Subdivision combo box (Choice parameter)
  bindComboBox(`${prefix}_subdivision`);
}

/**
 * v1.7.0: Format pitch randomization min/max value
 * Shows 2 decimal places when quantize is OFF, whole numbers when ON
 */
function formatPitchRandValue(value, laneNum) {
  // Try to get quantize state for this lane
  const quantizeState = Juce.getToggleState(`lane${laneNum}_pitch_rand_quantize`);
  const isQuantized = quantizeState ? quantizeState.getValue() : true;

  if (isQuantized) {
    return Math.round(value).toString();
  } else {
    return value.toFixed(2);
  }
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

  // v1.6.5: Tape bypass toggle
  bindToggle("tape_bypass");
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
  // v1.6.5: Use AbortController to properly clean up document listeners after drag ends
  let lastY = 0;

  knobElement.addEventListener("mousedown", (e) => {
    lastY = e.clientY;
    knobElement.style.cursor = "grabbing";
    e.preventDefault(); // Prevent text selection

    // Create AbortController for this drag session
    const controller = new AbortController();

    const onMouseMove = (moveEvent) => {
      const deltaY = lastY - moveEvent.clientY; // Inverted (up = increase)
      lastY = moveEvent.clientY;

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
    };

    const onMouseUp = () => {
      controller.abort(); // Removes both mousemove and mouseup listeners
      knobElement.style.cursor = "pointer";
    };

    document.addEventListener("mousemove", onMouseMove, { signal: controller.signal });
    document.addEventListener("mouseup", onMouseUp, { signal: controller.signal });
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

// ========== DROPDOWN BINDING (v1.11.0: Select menus for integer parameters) ==========

/**
 * Bind a <select> dropdown to a JUCE slider parameter (integer range)
 * @param {string} paramId - APVTS parameter ID (e.g., "lane1_euclidean_pulses")
 * @param {number} min - Parameter minimum value
 * @param {number} max - Parameter maximum value
 */
function bindDropdown(paramId, min, max) {
  const selectElement = document.getElementById(paramId);

  if (!selectElement) {
    console.error(`[v1.11.0] Dropdown element not found: ${paramId}`);
    return;
  }

  const state = Juce.getSliderState(paramId);
  if (!state) {
    console.error(`[v1.11.0] JUCE slider state not found: ${paramId}`);
    return;
  }

  // Initialize with current JUCE value
  const range = max - min;
  const currentValue = Math.round(min + state.getNormalisedValue() * range);
  selectElement.value = currentValue;

  // User selects a new value
  selectElement.addEventListener("change", () => {
    const intValue = parseInt(selectElement.value);
    const normalized = (intValue - min) / range;
    state.setNormalisedValue(normalized);
  });

  // JUCE automation: update dropdown when parameter changes externally
  state.valueChangedEvent.addListener(() => {
    const newValue = Math.round(min + state.getNormalisedValue() * range);
    selectElement.value = newValue;
  });

  console.log(`[v1.11.0] Bound dropdown: ${paramId} (${min}-${max})`);
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

// ========== v1.6.5: TAPE BYPASS DIMMING (when bypass is on) ==========

/**
 * Setup tape knobs dimming when bypass toggle is enabled
 * Watches tape_bypass parameter and dims tape knobs row
 */
function setupTapeBypassDimming() {
  const tapeKnobsRow = document.querySelector(".tape-knobs-row");

  if (!tapeKnobsRow) {
    console.warn("[v1.6.5] Tape knobs row not found");
    return;
  }

  // Get JUCE toggle state for tape_bypass
  const state = Juce.getToggleState("tape_bypass");
  if (!state) {
    console.error("[v1.6.5] JUCE toggle state not found: tape_bypass");
    return;
  }

  // Apply initial state (bypass ON = dimmed)
  applyTapeBypassDimming(tapeKnobsRow, state.value);

  // Update on parameter change
  state.valueChangedEvent.addListener(() => {
    applyTapeBypassDimming(tapeKnobsRow, state.value);
  });

  console.log("[v1.6.5] Tape bypass dimming handler installed");
}

function applyTapeBypassDimming(tapeKnobsRow, isBypassed) {
  if (isBypassed) {
    tapeKnobsRow.classList.add("tape-bypassed");
  } else {
    tapeKnobsRow.classList.remove("tape-bypassed");
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

// ========== v1.9.0: EUCLIDEAN RHYTHM MODE ==========

/**
 * Bjorklund's algorithm for Euclidean rhythm generation (JS mirror of C++ RepeatLane implementation)
 * Distributes N pulses as evenly as possible across M steps
 * @param {number} pulses - Number of active pulses (1-16)
 * @param {number} steps - Total number of steps (2-16)
 * @returns {boolean[]} Pattern array of length 16 (padded with false)
 */
function generateEuclideanPattern(pulses, steps) {
  const result = new Array(16).fill(false);
  if (steps <= 0 || pulses <= 0) return result;
  if (pulses >= steps) {
    for (let i = 0; i < steps && i < 16; i++) result[i] = true;
    return result;
  }

  // Exact port of C++ Bjorklund's: sequence distribution approach
  // sequences[i] = sub-pattern array, seqLengths[i] = length
  const sequences = Array.from({ length: 16 }, () => new Array(16).fill(false));
  const seqLengths = new Array(16).fill(0);
  let numA = pulses;
  let numB = steps - pulses;

  // Initialize: A sequences are [true], B sequences are [false]
  for (let i = 0; i < numA; i++) { sequences[i][0] = true; seqLengths[i] = 1; }
  for (let i = 0; i < numB; i++) { sequences[numA + i][0] = false; seqLengths[numA + i] = 1; }

  let totalSeqs = numA + numB;

  while (numB > 1) {
    const pairs = Math.min(numA, numB);

    // Append each B sequence to corresponding A sequence
    for (let i = 0; i < pairs; i++) {
      const aIdx = i;
      const bIdx = numA + i;
      const aLen = seqLengths[aIdx];
      const bLen = seqLengths[bIdx];
      for (let j = 0; j < bLen; j++) sequences[aIdx][aLen + j] = sequences[bIdx][j];
      seqLengths[aIdx] = aLen + bLen;
    }

    // Move remaining (unpaired) sequences to after the paired ones
    const remaining = totalSeqs - numA - pairs;
    if (remaining > 0 && numA + pairs < totalSeqs) {
      for (let i = 0; i < remaining; i++) {
        const srcIdx = numA + pairs + i;
        const dstIdx = pairs + i;
        if (srcIdx !== dstIdx) {
          for (let j = 0; j < seqLengths[srcIdx]; j++) sequences[dstIdx][j] = sequences[srcIdx][j];
          seqLengths[dstIdx] = seqLengths[srcIdx];
        }
      }
    }

    totalSeqs = pairs + remaining;
    numA = pairs;
    numB = remaining;
  }

  // Flatten all sequences into result
  let pos = 0;
  for (let i = 0; i < totalSeqs && pos < 16; i++) {
    for (let j = 0; j < seqLengths[i] && pos < 16; j++) {
      result[pos++] = sequences[i][j];
    }
  }

  return result;
}

/**
 * Setup Euclidean mode for all lanes:
 * - Shows/hides PULSES/STEPS controls per lane
 * - Makes step grid read-only when EUC is active
 * - Updates step grid to show Euclidean pattern preview
 */
function setupEuclideanMode() {
  for (let lane = 1; lane <= 4; lane++) {
    setupEuclideanModeForLane(lane);
  }
  console.log("[v1.9.0] Euclidean mode handlers installed");
}

function setupEuclideanModeForLane(laneNum) {
  const eucEnabledState = Juce.getToggleState(`lane${laneNum}_euclidean_enabled`);
  const pulsesState = Juce.getSliderState(`lane${laneNum}_euclidean_pulses`);
  const stepsState = Juce.getSliderState(`lane${laneNum}_euclidean_steps`);
  const controlsContainer = document.getElementById(`lane${laneNum}_euc_controls`);

  if (!eucEnabledState || !pulsesState || !stepsState || !controlsContainer) {
    console.warn(`[v1.9.0] Euclidean elements missing for lane ${laneNum}`);
    return;
  }

  // Get step buttons for this lane
  const stepButtons = [];
  for (let step = 1; step <= 16; step++) {
    const btn = document.querySelector(`.step-button[data-lane="${laneNum}"][data-step="${step}"]`);
    if (btn) stepButtons.push(btn);
  }

  function updateEuclideanUI() {
    const isEnabled = eucEnabledState.getValue();

    // Show/hide PULSES/STEPS controls
    if (isEnabled) {
      controlsContainer.classList.add("visible");
    } else {
      controlsContainer.classList.remove("visible");
    }

    // Lock/unlock step buttons
    stepButtons.forEach(btn => {
      if (isEnabled) {
        btn.classList.add("euc-locked");
      } else {
        btn.classList.remove("euc-locked");
      }
    });

    // Update step grid preview when Euclidean is active, restore manual state when off
    if (isEnabled) {
      updateEuclideanPatternPreview(laneNum, pulsesState, stepsState, stepButtons);
    } else {
      // Restore step button visuals from JUCE parameter values (EUC preview overwrote them)
      stepButtons.forEach((btn, i) => {
        const stepState = Juce.getToggleState(`pattern_lane${laneNum}_step${i + 1}`);
        if (stepState) {
          updateStepButtonUI(btn, stepState.getValue());
        }
      });
    }
  }

  function updateEuclideanPatternPreview(lane, pState, sState, buttons) {
    const pulses = Math.round(1 + pState.getNormalisedValue() * 15); // 1-16
    const steps = Math.round(2 + sState.getNormalisedValue() * 14);  // 2-16
    const pattern = generateEuclideanPattern(pulses, steps);

    buttons.forEach((btn, i) => {
      if (pattern[i]) {
        btn.classList.add("active");
      } else {
        btn.classList.remove("active");
      }
    });
  }

  // Initial state
  updateEuclideanUI();

  // React to EUC toggle changes
  eucEnabledState.valueChangedEvent.addListener(updateEuclideanUI);

  // React to PULSES/STEPS changes (update pattern preview)
  pulsesState.valueChangedEvent.addListener(() => {
    if (eucEnabledState.getValue()) {
      updateEuclideanPatternPreview(laneNum, pulsesState, stepsState, stepButtons);
    }
  });

  stepsState.valueChangedEvent.addListener(() => {
    if (eucEnabledState.getValue()) {
      updateEuclideanPatternPreview(laneNum, pulsesState, stepsState, stepButtons);
    }
  });
}

// ========== v1.5.0: LANE PROGRESS BARS ==========

/**
 * Setup listener for lane progress events from PluginEditor Timer
 * Receives JSON payload: {"lane1":{"progress":0.5,"active":true},...}
 */
function setupLaneProgressListener() {
  // v1.5.1 FIX: The ID is ON the .progress-fill element, not its parent
  // So #lane1_progress IS the fill element, and its parent is the container
  const progressElements = {
    lane1: document.getElementById("lane1_progress"),
    lane2: document.getElementById("lane2_progress"),
    lane3: document.getElementById("lane3_progress"),
    lane4: document.getElementById("lane4_progress")
  };

  // Get progress bar containers (parent of the fill element) for dimming
  const progressContainers = {
    lane1: document.getElementById("lane1_progress")?.parentElement,
    lane2: document.getElementById("lane2_progress")?.parentElement,
    lane3: document.getElementById("lane3_progress")?.parentElement,
    lane4: document.getElementById("lane4_progress")?.parentElement
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
