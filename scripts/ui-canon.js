/*
   This file is part of the Ouaricon Audio plugin suite.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    ui-canon.js — the UI component canon, held as DATA.

    Every plugin hand-copies its UI JS. The i18n runtime is hand-copied too, but
    it has a canon and a gate (i18n-canon.js + check-i18n.js), and the review
    measured the difference: 1 variant across 44 copies WITH a gate, 13–22
    variants per component WITHOUT one (260924-nho-UI-DESIGN-REVIEW.md, R3).

    This file is the canon half of R3 for the ungated components: the tooltip
    renderer, the hover-help switch, knob binding and the knob visual.
    scripts/check-ui-canon.js reads it and reports, per plugin, whether each
    component matches.

    ── REPORT-ONLY ─────────────────────────────────────────────────────────
    Nothing fails on a mismatch. Converging a plugin onto this canon changes its
    behaviour, so it is done one plugin at a time through /improve with a
    version bump — never as a sweep across the suite. The report is the
    burn-down list for that work, not a gate.

    ── Why DATA and not a live read of O-ReverseDelay ──────────────────────
    The canon is machine-copied from one recorded commit of O-ReverseDelay's
    app.js (UI_CANON_SOURCE below). Reading the live file instead would move
    the target every time O-ReverseDelay itself is improved, and silently
    redefine "canon" for the other 43 plugins with no diff anyone reviews.
    Held here, a re-anchor is a deliberate, reviewed change to this file, and
    O-ReverseDelay is measured against it like every other plugin.

    ── Why //| line comments and not one block comment ─────────────────────
    i18n-canon.js stores its canon inside a block comment. That cannot work
    here: the canon functions themselves contain the block-comment terminator
    (bindKnob has an "already released" note and initTipsToggle a "private
    mode" note, both written as block comments). Either would close an
    enclosing block comment mid-canon. Every canon line is instead prefixed
    with "//|", which makes backticks, ${...} and comment terminators inert
    with zero escaping — the canon stays verbatim.

    ── Re-anchoring (deliberate, reviewed) ──────────────────────────────────
      1. node scripts/check-ui-canon.js --emit-canon <rev>  > /tmp/canon.txt
         (stderr prints the full SHA and that revision's CMake VERSION)
      2. Replace this file's block — from the line holding the BEGIN sentinel
         through the line holding the END sentinel, inclusive — with the
         stdout of step 1. Never hand-edit canon text.
      3. Update UI_CANON_SOURCE.commit and .version to what step 1 printed.
      4. node scripts/check-ui-canon.js --self-test   (N4 compares the block
         against a fresh emit of UI_CANON_SOURCE.commit)

    The sentinel strings are assembled at runtime and never spelled whole in
    any comment: readCanonText() takes the FIRST line containing each one, so
    a literal sentinel anywhere above the block would hijack the extraction.

  ==============================================================================
*/

'use strict';

const fs = require('fs');

// Provenance of the canon block below. `commit` is the last commit that touched
// `file` when the canon was anchored; `version` is that revision's CMake VERSION.
const UI_CANON_SOURCE = Object.freeze({
    plugin:  'O-ReverseDelay',
    file:    'plugins/O-ReverseDelay/Source/ui/public/js/app.js',
    commit:  'ed1fc465e67973c3df2e365a7a1f70341b3aecac',
    version: '1.20.0',
});

// One entry per component; array order is the report's column order. `canon`
// names are O-ReverseDelay's functions, in the order the report joins them.
//
// Why aliases: O-ReverseDelay does not use the names that drift. It splits the
// tooltip renderer across five functions, where 29 plugins fold it into one
// setupTooltips (8 more into initializeTooltips). Its hover-help switch is
// applyTipsEnabled + initTipsToggle, where 23 plugins use initializeTipsToggle
// and 8 use initializeHelpToggle + setTooltipsEnabled. Aliases exist so the
// report NAMES what a plugin actually defines — "setupTooltips diverges" is
// then literally visible. An alias hit is always a variant, never canon.
const COMPONENTS = Object.freeze([
    { id: 'tooltip-renderer', label: 'tooltip renderer',
      canon:   ['handleTooltipOver', 'handleTooltipOut', 'showTooltip', 'hideTooltip', 'initTooltips'],
      aliases: ['setupTooltips', 'initializeTooltips'] },
    { id: 'hover-help-init', label: 'hover-help init',
      canon:   ['applyTipsEnabled', 'initTipsToggle'],
      aliases: ['initializeTipsToggle', 'initializeHelpToggle', 'setTooltipsEnabled'] },
    { id: 'knob-binding', label: 'knob binding',
      canon:   ['bindKnob'],
      aliases: ['setupKnob'] },
    { id: 'knob-visual', label: 'knob visual',
      canon:   ['updateKnobVisual'],
      aliases: [] },
]);

const CANON_BEGIN = '<<<UI_CANON_' + 'BEGIN>>>';
const CANON_END   = '<<<UI_CANON_' + 'END>>>';

// ─────────────────────────────────────────────────────────────── canon block
// <<<UI_CANON_BEGIN>>>
//| function handleTooltipOver(e) {
//|   const target = e.target.closest ? e.target.closest("[data-tip]") : null;
//|   if (!target || target === tooltipTarget) return;
//|
//|   tooltipTarget = target;
//|   clearTimeout(tooltipTimer);
//|
//|   if (tooltipSuppressed) return;
//|   tooltipTimer = setTimeout(() => showTooltip(target), TOOLTIP_DELAY_MS);
//| }
//|
//| function handleTooltipOut(e) {
//|   const target = e.target.closest ? e.target.closest("[data-tip]") : null;
//|   if (!target) return;
//|
//|   // Moving between children of the same control is not a real exit.
//|   if (e.relatedTarget && target.contains(e.relatedTarget)) return;
//|
//|   hideTooltip();
//| }
//|
//| function showTooltip(target) {
//|   // The pointer may have moved on, or gone down, during the delay.
//|   if (!tooltipEl || tooltipSuppressed || target !== tooltipTarget) return;
//|
//|   // v1.11.0 THE SWITCH, and it is a SHOW gate rather than a bind gate: this
//|   // renderer is delegated on document, so there is nothing to unbind.
//|   // data-tip-always survives it — see the markup comment on #tips-toggle in
//|   // index.html for which two controls carry it and why.
//|   if (!tipsEnabled && !target.hasAttribute("data-tip-always")) return;
//|
//|   const title = target.getAttribute("data-tip-title");
//|   const body  = target.getAttribute("data-tip");
//|
//|   // textContent, not innerHTML — the copy stays inert.
//|   tooltipEl.textContent = "";
//|
//|   if (title) {
//|     const titleEl = document.createElement("div");
//|     titleEl.className = "tooltip-title";
//|     titleEl.textContent = title;
//|     tooltipEl.appendChild(titleEl);
//|   }
//|
//|   const bodyEl = document.createElement("div");
//|   bodyEl.className = "tooltip-body";
//|   bodyEl.textContent = body;
//|   tooltipEl.appendChild(bodyEl);
//|
//|   const anchor = target.getBoundingClientRect();
//|
//|   // MEASURE-THEN-PIN. A fixed-position box with `left` set and `width:auto`
//|   // shrinks to fit whatever space remains to its right, so measuring at the
//|   // PREVIOUS offset under-reports the width, and applying a near-edge `left`
//|   // afterwards re-wraps a 230 px tip into a ~70 px ribbon. Release the width,
//|   // measure from the left edge, pin the result in px, and only then place.
//|   // Here the exposed control is `mix` — right-most, OUTPUT panel. Invisible to
//|   // build, auval and pluginval (pattern_fixed_tooltip_shrink_to_fit_edge).
//|   tooltipEl.style.width = "";
//|   tooltipEl.style.left  = "0px";
//|   tooltipEl.style.top   = "0px";
//|
//|   const width = tooltipEl.getBoundingClientRect().width;
//|   tooltipEl.style.width = `${width}px`;
//|
//|   // Height is only stable once the width is definite.
//|   const height = tooltipEl.getBoundingClientRect().height;
//|
//|   // Prefer above; flip below only when there is no room at the top.
//|   let top = anchor.top - height - TOOLTIP_MARGIN;
//|   let placement = "above";
//|
//|   if (top < TOOLTIP_MARGIN) {
//|     top = anchor.bottom + TOOLTIP_MARGIN;
//|     placement = "below";
//|   }
//|
//|   const anchorCentreX = anchor.left + anchor.width / 2;
//|   const maxLeft = window.innerWidth - width - TOOLTIP_MARGIN;
//|   const left = Math.max(TOOLTIP_MARGIN, Math.min(maxLeft, anchorCentreX - width / 2));
//|
//|   tooltipEl.style.left = `${left}px`;
//|   tooltipEl.style.top  = `${top}px`;
//|   tooltipEl.dataset.placement = placement;
//|
//|   // The tip is clamped to the viewport, but the arrow still points at the
//|   // control — held clear of the rounded corners.
//|   const arrowX = Math.max(10, Math.min(width - 10, anchorCentreX - left));
//|   tooltipEl.style.setProperty("--arrow-x", `${arrowX}px`);
//|
//|   tooltipEl.classList.add("visible");
//|   tooltipEl.setAttribute("aria-hidden", "false");
//| }
//|
//| function hideTooltip() {
//|   clearTimeout(tooltipTimer);
//|   tooltipTarget = null;
//|
//|   if (!tooltipEl) return;
//|   tooltipEl.classList.remove("visible");
//|   tooltipEl.setAttribute("aria-hidden", "true");
//| }
//|
//| function initTooltips() {
//|   tooltipEl = document.getElementById("tooltip");
//|   if (!tooltipEl) { console.warn("Tooltip element not found — tooltips disabled"); return; }
//|
//|   document.addEventListener("mouseover", handleTooltipOver);
//|   document.addEventListener("mouseout", handleTooltipOut);
//|
//|   // Any press begins a click or a drag: get the tip out of the way and keep it
//|   // away until release, so it cannot hang over a knob mid-drag. Capture phase,
//|   // because the knobs call preventDefault in their own pointerdown handlers.
//|   document.addEventListener("pointerdown", () => {
//|     tooltipSuppressed = true;
//|     hideTooltip();
//|   }, true);
//|
//|   document.addEventListener("pointerup", () => { tooltipSuppressed = false; }, true);
//| }
//|
//| function applyTipsEnabled(on) {
//|   tipsEnabled = !!on;
//|   if (!tipsEnabled) hideTooltip();
//|   const btn = document.getElementById("tips-toggle");
//|   if (!btn) return;
//|   btn.setAttribute("aria-pressed", tipsEnabled ? "true" : "false");
//|   // TWO CALLS IN TWO BRANCHES, never one call with a ternary — the same rule
//|   // the delete/confirm pair above already follows, and the one check-i18n
//|   // assertion [13] enforces: an inflection decided in JS is a string no
//|   // translator can see.
//|   if (tipsEnabled) setLabel(btn, "ui.on");
//|   else             setLabel(btn, "ui.off");
//| }
//|
//| function initTipsToggle() {
//|   const btn = document.getElementById("tips-toggle");
//|   if (!btn) { console.error("Missing tips-toggle element"); return; }
//|   let stored = null;
//|   try { stored = localStorage.getItem("ord.tipsEnabled"); } catch (e) { stored = null; }
//|   // Anything that is not the literal string "false" — including a first run with
//|   // nothing stored, and a private-mode throw — resolves to ON.
//|   applyTipsEnabled(stored !== "false");
//|   btn.addEventListener("click", () => {
//|     applyTipsEnabled(!tipsEnabled);
//|     try { localStorage.setItem("ord.tipsEnabled", String(tipsEnabled)); }
//|     catch (e) { /* private mode: the switch still works, it just forgets */ }
//|   });
//| }
//|
//| function bindKnob(juce, id) {
//|   const st = juce.getSliderState(id);
//|   sliderState[id] = st;
//|
//|   st.valueChangedEvent.addListener(() => updateKnobVisual(id));
//|   st.propertiesChangedEvent.addListener(() => updateKnobVisual(id));
//|   updateKnobVisual(id);
//|
//|   const knob = document.getElementById(`knob-${id}`);
//|   if (!knob) { console.error(`Missing knob element: knob-${id}`); return; }
//|
//|   // Accessibility: focusable + arrow-key fine adjust.
//|   knob.setAttribute("tabindex", "0");
//|   knob.setAttribute("role", "slider");
//|   knob.addEventListener("keydown", (e) => {
//|     let dir = 0;
//|     if (e.key === "ArrowUp" || e.key === "ArrowRight") dir = 1;
//|     else if (e.key === "ArrowDown" || e.key === "ArrowLeft") dir = -1;
//|     else return;
//|     endWheelGesture();
//|     nudge(st, dir, id);
//|     e.preventDefault();
//|   });
//|
//|   let dragging  = false;
//|   let startY    = 0;
//|   let startNorm = 0;
//|   let lastY     = 0;       // v1.14.0: previous move's Y, the re-base point
//|   let fine      = false;   // v1.14.0: Shift held for the current drag segment
//|
//|   // v1.12.3: the open wheel gesture, if any. Every other interaction on this
//|   // knob closes it first, so a key, drag or double-click never nests its own
//|   // begin/end inside it.
//|   let wheelTimer = null;
//|   const endWheelGesture = () => {
//|     if (wheelTimer === null) return;
//|     clearTimeout(wheelTimer);
//|     wheelTimer = null;
//|     st.sliderDragEnded();
//|   };
//|
//|   // v1.14.0: Shift drags at FINE_DRAG_RATE. The drag is absolute from an origin
//|   // (startY, startNorm), so a bare rate switch would jump the knob by the whole
//|   // distance travelled so far times the rate change. Instead the origin is
//|   // re-based to where the knob IS whenever the modifier flips — at the previous
//|   // move's Y, so this event's motion is applied at the new rate, not dropped.
//|   const onMove = (e) => {
//|     if (!dragging) return;
//|     if (e.shiftKey !== fine) {
//|       fine      = e.shiftKey;
//|       startY    = lastY;
//|       startNorm = st.getNormalisedValue();
//|     }
//|     lastY = e.clientY;
//|     const travel = fine ? DRAG_TRAVEL_PX / FINE_DRAG_RATE : DRAG_TRAVEL_PX;
//|     const dy = startY - e.clientY;
//|     const n = Math.min(1, Math.max(0, startNorm + dy / travel));
//|     st.setNormalisedValue(n);
//|     updateKnobVisual(id);
//|     e.preventDefault();
//|   };
//|
//|   // v1.7.2 (WR-05): terminates on cancel and lost-capture as well as on up, and
//|   // is idempotent (the !dragging early-return), because all four paths can fire.
//|   const onUp = (e) => {
//|     if (!dragging) return;
//|     dragging = false;
//|     st.sliderDragEnded();
//|     knob.removeEventListener("pointermove", onMove);
//|     knob.removeEventListener("pointerup", onUp);
//|     knob.removeEventListener("pointercancel", onUp);
//|     knob.removeEventListener("lostpointercapture", onUp);
//|     if (e && e.pointerId !== undefined) {
//|       try { knob.releasePointerCapture(e.pointerId); } catch (_) { /* already released */ }
//|     }
//|   };
//|
//|   knob.addEventListener("pointerdown", (e) => {
//|     endWheelGesture();
//|     dragging  = true;
//|     startY    = e.clientY;
//|     lastY     = e.clientY;
//|     fine      = e.shiftKey;
//|     startNorm = st.getNormalisedValue();
//|     st.sliderDragStarted();
//|     // v1.7.2 (WR-05): capture on the KNOB rather than listening on window.
//|     //
//|     // With window listeners and only a `pointerup` to end the drag, any path that
//|     // does not deliver that event leaves `dragging` true and both listeners
//|     // attached: drag out of the plugin window and release over the DAW, let the
//|     // host take a modal grab, or let the WebView lose focus mid-drag and the OS
//|     // synthesise a pointercancel instead. Two silent consequences followed —
//|     // every later mouse move over the page kept writing setNormalisedValue()
//|     // with no button held (the knob follows the cursor until the next click), and
//|     // sliderDragStarted() was left unmatched, so the host's parameter gesture
//|     // stayed open, which latches automation write in Logic and Live.
//|     //
//|     // Capturing routes every subsequent pointer event for this pointerId to the
//|     // knob even outside the WebView, and guarantees a terminating
//|     // pointerup/pointercancel/lostpointercapture. try/catch covers older
//|     // backends; the window path is not kept as a fallback because a partial
//|     // failure there is what produced the bug.
//|     try { knob.setPointerCapture(e.pointerId); } catch (_) { /* older backends */ }
//|     knob.addEventListener("pointermove", onMove);
//|     knob.addEventListener("pointerup", onUp);
//|     knob.addEventListener("pointercancel", onUp);
//|     knob.addEventListener("lostpointercapture", onUp);
//|     e.preventDefault();
//|   });
//|
//|   // v1.12.3: horizontal-dominant events are not ours — up to v1.12.2 a sideways
//|   // trackpad swipe (deltaY 0) read as "down" and walked the knob to its floor.
//|   // Not preventDefault()ed, so they reach whatever else wants them. Ignored
//|   // mid-drag too: the drag already holds the gesture, and a tick there used to
//|   // close it early.
//|   knob.addEventListener("wheel", (e) => {
//|     if (dragging) { e.preventDefault(); return; }
//|     if (Math.abs(e.deltaX) >= Math.abs(e.deltaY)) return;
//|     e.preventDefault();
//|
//|     const px = Math.abs(e.deltaY) *
//|       (e.deltaMode === 1 ? WHEEL_LINE_PX : e.deltaMode === 2 ? WHEEL_PAGE_PX : 1);
//|     const nudges = Math.min(WHEEL_MAX_NUDGES, Math.max(1, px / WHEEL_PX_PER_NUDGE));
//|
//|     if (wheelTimer === null) st.sliderDragStarted();
//|     else clearTimeout(wheelTimer);
//|     wheelTimer = setTimeout(endWheelGesture, WHEEL_GESTURE_MS);
//|
//|     stepBy(st, e.deltaY < 0 ? nudges : -nudges, id);
//|   }, { passive: false });
//|
//|   // Dblclick-reset uses the engineering default fetched from C++ — the
//|   // properties payload carries no default field, so a JS default table would
//|   // be the only alternative (and would drift).
//|   knob.addEventListener("dblclick", (e) => {
//|     endWheelGesture();
//|     resetToDefault(st, id);
//|     e.preventDefault();
//|   });
//| }
//|
//| function updateKnobVisual(id) {
//|   const st = sliderState[id];
//|   if (!st) return;
//|
//|   const text = FORMAT[id](st.getScaledValue());   // scaled value — never a JS range map
//|
//|   const knob = document.getElementById(`knob-${id}`);
//|   if (knob) {
//|     const stem = knob.querySelector(".knob-stem");
//|     if (stem) {
//|       stem.style.transform =
//|         `translate(-50%, -100%) rotate(${normToDeg(st.getNormalisedValue())}deg)`;
//|     }
//|     knob.setAttribute("aria-valuetext", text);
//|   }
//|
//|   const valEl = document.getElementById(`val-${id}`);
//|   if (valEl) valEl.textContent = text;
//| }
// <<<UI_CANON_END>>>
// ───────────────────────────────────────────────────────────────────────────

// Lazy on purpose: the file must load while the block is empty, because
// --emit-canon (which produces the block) requires this module.
function readCanonText() {
    const lines = fs.readFileSync(__filename, 'utf8').replace(/\r\n?/g, '\n').split('\n');

    const beginAt = lines.findIndex((l) => l.includes(CANON_BEGIN));
    const endAt   = lines.findIndex((l) => l.includes(CANON_END));

    if (beginAt < 0 || endAt < 0)
        throw new Error('ui-canon.js: canon sentinel missing — the file has been edited in a way that destroys the canon.');
    if (endAt <= beginAt)
        throw new Error('ui-canon.js: canon sentinels out of order.');

    const out = [];
    for (let i = beginAt + 1; i < endAt; ++i) {
        const l = lines[i];
        if (!l.startsWith('//|'))
            throw new Error(`ui-canon.js: line ${i + 1} inside the canon block lacks the //| prefix.`);
        out.push(l.startsWith('//| ') ? l.slice(4) : l.slice(3));
    }

    const text = out.join('\n');
    if (text.trim().length === 0)
        throw new Error('ui-canon.js: the canon block is empty — an empty canon makes every comparison vacuous.');

    return text;
}

module.exports = {
    UI_CANON_SOURCE,
    COMPONENTS,
    CANON_BEGIN,
    CANON_END,
    readCanonText,
};
