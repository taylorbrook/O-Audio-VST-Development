/*
   This file is part of O-simpleAdditive, an Ouaricon Audio plugin.
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
// ============================================================================
// O-simpleAdditive — WebView UI controller
// Binds all 33 APVTS params two-way (16 drawbars + 15 knobs + 2 combo boxes),
// draws the oscilloscope, lights the drawbars as the live spectrum, and runs the
// pedagogical layer (tooltips, lesson preset tour, on-screen keyboard).
//
// NOTE: getSliderState / getComboBoxState / getNativeFunction live on the `Juce`
// ES-module namespace. window.__JUCE__ has the low-level backend (used here only
// for backend.addEventListener on the scope / drawbar-spectrum push events).
// ============================================================================

import * as Juce from "./juce/index.js";

// ═══════════════════════════════════════════════════════════════════════════
// INTERFACE LANGUAGE (v1.1.0) — canon v2, verbatim from scripts/i18n-canon.js.
//
// It sits HERE, directly under the import and above every other declaration in
// this module, because `uiLanguage` is a module-level `let`: anything above it
// that reached applyLabel() would touch it inside its temporal dead zone and
// throw out of module evaluation, taking the drawbars, the knobs, the scope and
// the keyboard with it (pattern_module_toplevel_init_tdz). This file DOES have
// eager top-level work below — DRAWBAR_IDS, FORMAT, the LESSON tables — so the
// ordering is load-bearing here, not merely defensive.
// `node scripts/boot-all-uis.js` is the ONLY gate in the repo that sees this
// class of failure.
//
// Do NOT edit the region between the import line and the close of initI18n():
// check-i18n assertion 6 byte-compares it against scripts/i18n-canon.js.
// ═══════════════════════════════════════════════════════════════════════════
import { LANGUAGES, I18N, LABELS, TIP_BINDINGS, tr } from './i18n.js';

let uiLanguage = 'en';
let getUiLanguageNative = null;
let setUiLanguageNative = null;

// LABELS first, I18N as the fallback: a control whose tooltip title already IS
// its label carries one key, not two copies of the same string.
function trLabel(key, lang, vars) {
    const entry = (typeof LABELS === 'object' && LABELS && LABELS[key]) || I18N[key];
    if (!entry) { console.warn(`i18n: missing label key ${key}`); return key; }
    const s = entry[lang] || entry.en;
    const resolve = (v) => {
        const nested = (typeof LABELS === 'object' && LABELS && LABELS[v]) || I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };
    return vars
        ? String(s.t).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(s.t);
}

function applyLabel(el) {
    const key = el.dataset.i18n;
    if (!key) return;
    let vars = null;
    try { vars = el.dataset.i18nVars ? JSON.parse(el.dataset.i18nVars) : null; }
    catch (e) { console.warn(`i18n: bad vars on ${key}`); }
    const s = trLabel(key, uiLanguage, vars);
    el.dataset.label = s;
    el.textContent   = s;
}

function applyI18nAttributes(el) {
    const pairs = [['i18nAria', 'aria-label'], ['i18nPlaceholder', 'placeholder'], ['i18nAlt', 'alt']];
    for (const [prop, attr] of pairs) {
        const key = el.dataset[prop];
        if (key) el.setAttribute(attr, trLabel(key, uiLanguage, null));
    }
}

function setLabel(el, key, vars) {
    if (!el) return;
    el.dataset.i18n = key;
    if (vars) el.dataset.i18nVars = JSON.stringify(vars); else delete el.dataset.i18nVars;
    applyLabel(el);
}

function applyI18n(lang) {
    uiLanguage = LANGUAGES.includes(lang) ? lang : 'en';
    // <html lang> follows the selector: screen readers pick the French voice,
    // and CSS hyphens:auto / quotes resolve in the page's actual language.
    document.documentElement.lang = uiLanguage;
    for (const [selector, key, wrapper, vars] of TIP_BINDINGS) {
        const el = document.querySelector(selector);
        if (!el) { console.warn(`i18n: tip target not found: ${selector}`); continue; }
        const target = wrapper ? (el.closest(wrapper) || el) : el;
        const s = tr(key, uiLanguage, vars);
        target.setAttribute('data-tip-title', s.t);
        target.setAttribute('data-tip', s.b);
    }
    for (const el of document.querySelectorAll('[data-i18n]')) applyLabel(el);
    for (const el of document.querySelectorAll('[data-i18n-aria],[data-i18n-placeholder],[data-i18n-alt]'))
        applyI18nAttributes(el);
    const sel = document.getElementById('lang-select');
    if (sel && sel.value !== uiLanguage) sel.value = uiLanguage;
}

// Exposed so a clamp gate can drive the language without teaching the ui-stub a
// promise contract: page.evaluate((l) => window.__setLanguage(l), 'fr').
window.__setLanguage = applyI18n;
// Exposed for the same reason, and so a sibling module can write a localized
// label without app.js having to export anything — O-Bitrot's controller is an
// inline <script type="module">, where an export declaration has nowhere to go.
window.__setLabel = setLabel;
window.__reapplyI18n = () => applyI18n(uiLanguage);

function initI18n() {
    try {
        getUiLanguageNative = Juce.getNativeFunction('getUiLanguage');
        setUiLanguageNative = Juce.getNativeFunction('setUiLanguage');
    } catch (e) {
        console.warn('Language preference not available, session-only:', e);
    }

    // Paint the default SYNCHRONOUSLY first. Never blank, never a flash.
    try { applyI18n('en'); } catch (e) { console.error('i18n init failed:', e); }

    if (getUiLanguageNative) {
        getUiLanguageNative()
            .then((code) => applyI18n(code))
            .catch((e) => console.warn('Could not read language preference:', e));
    }

    const sel = document.getElementById('lang-select');
    if (sel) sel.addEventListener('change', (e) => {
        applyI18n(e.target.value);
        if (setUiLanguageNative) setUiLanguageNative(uiLanguage).catch(() => {});
    });
}


// ── Parameter inventory (must match OSimpleAdditive::ParamIDs exactly) ───────
const NUM_PARTIALS = 16;
const DRAWBAR_IDS = Array.from({ length: NUM_PARTIALS }, (_, i) => `partial${i + 1}`);
const KNOB_IDS = [
  "scanPosition", "scanLfoRate", "scanLfoDepth", "scanEnvAmount",
  "spectralDecay", "velToDecay",
  "ampAttack", "ampDecay", "ampSustain", "ampRelease",
  "modAttack", "modDecay", "modSustain", "modRelease",
  "outputLevel",
];
const COMBO_IDS = ["frameBSource", "bitDepth"];

// ── Display formatters (keyed by param id) ──────────────────────────────────
// Each receives the *scaled* value (NormalisableRange::convertFrom0to1 output).
const fmtMs = (s) => (s < 1 ? `${Math.round(s * 1000)} ms` : `${s.toFixed(2)} s`);
const fmtPct = (v) => `${Math.round(v * 100)}%`;
const fmtPctSigned = (v) => `${v >= 0 ? "+" : ""}${Math.round(v * 100)}%`;
const FORMAT = {
  scanPosition: fmtPct,
  scanLfoRate: (v) => `${v.toFixed(2)} Hz`,
  scanLfoDepth: fmtPct,
  scanEnvAmount: fmtPctSigned,
  spectralDecay: fmtPct,
  velToDecay: fmtPct,
  ampAttack: fmtMs, ampDecay: fmtMs, ampSustain: fmtPct, ampRelease: fmtMs,
  modAttack: fmtMs, modDecay: fmtMs, modSustain: fmtPct, modRelease: fmtMs,
  outputLevel: (v) => `${v.toFixed(1)} dB`,
};

// ── Tooltip copy ────────────────────────────────────────────────────────────
// MOVED to js/i18n.js at v1.1.0. Through v1.0.7 the copy lived in a `TIPS`
// object here and each anchor carried the KEY in its own data-tip attribute.
// applyI18n now WRITES data-tip (the body) and data-tip-title on every anchor
// named by TIP_BINDINGS, so the renderer below reads the attributes rather than
// a table — one code path, and no way for a tip to be stranded in the previous
// language after the selector fires.
//
// The sixteen per-partial tips were GENERATED here by a harmonicTip(k) helper.
// They are sixteen written-out entries in js/i18n.js now: a generated string is
// invisible to the translator reviewing that file. The generator also built its
// ordinal as `${k}th` with no special-casing and so shipped "The 2th harmonic"
// and "The 3th harmonic"; the table reads "2nd" and "3rd".

// ── Knob geometry ────────────────────────────────────────────────────────────
const KNOB_MIN_DEG = -135;   // 0.0 normalised
const KNOB_MAX_DEG = 135;    // 1.0 normalised
const DRAG_TRAVEL_PX = 220;  // vertical px for full 0..1 sweep

const sliderState = {};   // id -> Juce SliderState (knobs + drawbars)
const comboState = {};    // id -> Juce ComboBoxState
const drawbarCells = {};  // id -> .drawbar-cell element

function normToDeg(n) { return KNOB_MIN_DEG + n * (KNOB_MAX_DEG - KNOB_MIN_DEG); }

// One-shot fine adjust shared by wheel + arrow keys (a full drag gesture).
function nudge(st, delta, after) {
  const n = Math.max(0, Math.min(1, st.getNormalisedValue() + delta));
  st.sliderDragStarted();
  st.setNormalisedValue(n);
  st.sliderDragEnded();
  if (after) after();
}

// ── Knob binding (relative vertical drag) ───────────────────────────────────
function updateKnobVisual(id) {
  const st = sliderState[id];
  if (!st) return;
  const norm = st.getNormalisedValue();
  const fmt = FORMAT[id] || ((v) => v.toFixed(2));
  const scaled = st.getScaledValue();
  const knob = document.getElementById(`knob-${id}`);
  if (knob) {
    const stem = knob.querySelector(".knob-stem");
    if (stem) stem.style.transform = `translate(-50%, -100%) rotate(${normToDeg(norm)}deg)`;
    // ARIA: role="slider" needs value attributes or screen readers announce a
    // valueless slider. Range comes from the pushed C++ properties (skew-safe,
    // re-applied on propertiesChangedEvent) — never a hardcoded JS min/max.
    const props = st.properties || {};
    knob.setAttribute("aria-valuemin", props.start != null ? props.start : 0);
    knob.setAttribute("aria-valuemax", props.end != null ? props.end : 1);
    knob.setAttribute("aria-valuenow", scaled);
    knob.setAttribute("aria-valuetext", fmt(scaled));
  }
  const valEl = document.getElementById(`val-${id}`);
  if (valEl) valEl.textContent = fmt(scaled);
}

function bindKnob(id) {
  const st = Juce.getSliderState(id);
  sliderState[id] = st;

  st.valueChangedEvent.addListener(() => updateKnobVisual(id));
  st.propertiesChangedEvent.addListener(() => updateKnobVisual(id));
  updateKnobVisual(id);

  const knob = document.getElementById(`knob-${id}`);
  if (!knob) { console.error(`Missing knob element: knob-${id}`); return; }

  knob.setAttribute("tabindex", "0");
  knob.setAttribute("role", "slider");
  // The accessible name is NOT set here any more. Each knob carries
  // data-i18n-aria="<paramId>" in the markup and applyI18n's attribute sweep
  // resolves it through trLabel, which falls back from LABELS to I18N and so
  // returns the control's tooltip TITLE. A setAttribute here would write the
  // English once and be stranded in it the moment the selector fires.
  knob.addEventListener("keydown", (e) => {
    let delta = 0;
    if (e.key === "ArrowUp" || e.key === "ArrowRight") delta = 0.02;
    else if (e.key === "ArrowDown" || e.key === "ArrowLeft") delta = -0.02;
    else return;
    nudge(st, delta, () => updateKnobVisual(id));
    e.preventDefault();
  });

  let dragging = false, startY = 0, startNorm = 0;
  const onMove = (e) => {
    if (!dragging) return;
    let n = startNorm + (startY - e.clientY) / DRAG_TRAVEL_PX;
    n = Math.max(0, Math.min(1, n));
    st.setNormalisedValue(n);
    updateKnobVisual(id);
    e.preventDefault();
  };
  const onUp = () => {
    if (!dragging) return;
    dragging = false;
    st.sliderDragEnded();
    window.removeEventListener("pointermove", onMove);
    window.removeEventListener("pointerup", onUp);
    window.removeEventListener("pointercancel", onUp);
  };
  knob.addEventListener("pointerdown", (e) => {
    dragging = true;
    startY = e.clientY;
    startNorm = st.getNormalisedValue();
    st.sliderDragStarted();
    // Capture the pointer so pointerup/pointercancel are delivered even when the
    // button is released outside the plugin window — otherwise the drag sticks
    // and the host automation gesture stays open.
    try { knob.setPointerCapture(e.pointerId); } catch (_) {}
    window.addEventListener("pointermove", onMove);
    window.addEventListener("pointerup", onUp);
    window.addEventListener("pointercancel", onUp);
    e.preventDefault();
  });
  knob.addEventListener("wheel", (e) => {
    nudge(st, e.deltaY < 0 ? 0.02 : -0.02, () => updateKnobVisual(id));
    e.preventDefault();
  }, { passive: false });
}

// ── Drawbar bay (16 vertical faders that double as the live spectrum) ───────
function buildDrawbars() {
  const bay = document.getElementById("drawbarBay");
  if (!bay) return;
  for (let k = 1; k <= NUM_PARTIALS; k++) {
    const id = `partial${k}`;
    const cell = document.createElement("div");
    cell.className = "drawbar-cell " + (k === 1 ? "db-fundamental" : (k % 2 === 1 ? "db-odd" : "db-even"));
    // data-param, not data-tip: applyI18n writes data-tip as the tip BODY, so a
    // key parked there would be overwritten by the copy meant for it.
    cell.setAttribute("data-param", id);

    // Built node by node rather than by innerHTML. The accessible name was
    // "Partial 3 level", interpolated into that markup string; it is now
    // data-i18n-aria, resolved by the attribute sweep through I18N.partial3 to
    // the same title the hover help shows. Interpolating a localized name into
    // a markup string would put prose back on an innerHTML path.
    const track = document.createElement("div");
    track.className = "drawbar";
    track.id = `drawbar-${id}`;
    track.setAttribute("role", "slider");
    track.setAttribute("tabindex", "0");
    track.dataset.i18nAria = id;

    const fill = document.createElement("div");
    fill.className = "drawbar-fill";
    fill.id = `fill-${id}`;

    const live = document.createElement("div");
    live.className = "drawbar-live";
    live.id = `live-${id}`;

    track.append(fill, live);

    const num = document.createElement("div");
    num.className = "drawbar-num";
    num.textContent = String(k);              // the harmonic number — a readout (D-03)

    const val = document.createElement("div");
    val.className = "drawbar-val";
    val.id = `val-${id}`;
    val.textContent = "0";                    // a readout (D-03)

    cell.append(track, num, val);
    bay.appendChild(cell);
    drawbarCells[id] = cell;
  }
}

function bindDrawbar(id) {
  const st = Juce.getSliderState(id);
  sliderState[id] = st;
  const track = document.getElementById(`drawbar-${id}`);
  const fill = document.getElementById(`fill-${id}`);
  const valEl = document.getElementById(`val-${id}`);
  if (!track) { console.error(`Missing drawbar element: drawbar-${id}`); return; }

  const update = () => {
    const n = st.getNormalisedValue();
    const pct = Math.round(st.getScaledValue() * 100);
    if (fill) fill.style.height = `${n * 100}%`;
    if (valEl) valEl.textContent = `${pct}`;
    // ARIA values in display units (0–100 %), matching the on-screen readout.
    track.setAttribute("aria-valuemin", 0);
    track.setAttribute("aria-valuemax", 100);
    track.setAttribute("aria-valuenow", pct);
    track.setAttribute("aria-valuetext", `${pct}%`);
  };
  st.valueChangedEvent.addListener(update);
  st.propertiesChangedEvent.addListener(update);
  update();

  // Absolute fader drag: the bar follows the pointer (natural drawbar feel).
  const setFromY = (clientY) => {
    const r = track.getBoundingClientRect();
    let n = (r.bottom - clientY) / r.height;
    n = Math.max(0, Math.min(1, n));
    st.setNormalisedValue(n);
    update();
  };
  let dragging = false;
  const onMove = (e) => { if (dragging) { setFromY(e.clientY); e.preventDefault(); } };
  const onUp = () => {
    if (!dragging) return;
    dragging = false;
    st.sliderDragEnded();
    window.removeEventListener("pointermove", onMove);
    window.removeEventListener("pointerup", onUp);
    window.removeEventListener("pointercancel", onUp);
  };
  track.addEventListener("pointerdown", (e) => {
    dragging = true;
    st.sliderDragStarted();
    setFromY(e.clientY);
    // Same stuck-drag guard as the knobs: capture the pointer and treat cancel as up.
    try { track.setPointerCapture(e.pointerId); } catch (_) {}
    window.addEventListener("pointermove", onMove);
    window.addEventListener("pointerup", onUp);
    window.addEventListener("pointercancel", onUp);
    e.preventDefault();
  });
  track.addEventListener("wheel", (e) => {
    nudge(st, e.deltaY < 0 ? 0.04 : -0.04, update);
    e.preventDefault();
  }, { passive: false });
  track.addEventListener("keydown", (e) => {
    let delta = 0;
    if (e.key === "ArrowUp" || e.key === "ArrowRight") delta = 0.04;
    else if (e.key === "ArrowDown" || e.key === "ArrowLeft") delta = -0.04;
    else return;
    nudge(st, delta, update);
    e.preventDefault();
  });
}

// ── Combo box binding (Frame B source, Bit depth) ───────────────────────────
function bindCombo(id) {
  const st = Juce.getComboBoxState(id);
  comboState[id] = st;
  const sel = document.getElementById(`combo-${id}`);
  if (!sel) { console.error(`Missing combo element: combo-${id}`); return; }

  const buildOptions = () => {
    const choices = (st.properties && st.properties.choices) || [];
    if (choices.length === 0) return false;
    if (sel.options.length === choices.length) return true;   // already built
    sel.innerHTML = "";
    choices.forEach((c, i) => {
      const opt = document.createElement("option");
      opt.value = String(i);
      opt.textContent = c;
      sel.appendChild(opt);
    });
    return true;
  };
  const refresh = () => {
    buildOptions();
    const idx = st.getChoiceIndex();
    if (idx >= 0 && idx < sel.options.length) sel.selectedIndex = idx;
  };
  st.propertiesChangedEvent.addListener(refresh);
  st.valueChangedEvent.addListener(refresh);
  refresh();

  sel.addEventListener("change", () => st.setChoiceIndex(sel.selectedIndex));
}

// ── Live drawbar spectrum (morphed + decayed levels of the sounding voice) ──
function updateDrawbarSpectrum(sounding, levels) {
  for (let k = 0; k < NUM_PARTIALS; k++) {
    const id = `partial${k + 1}`;
    const cell = drawbarCells[id];
    const live = document.getElementById(`live-${id}`);
    if (!cell || !live) continue;
    if (sounding && levels) {
      cell.classList.add("sounding");
      const lvl = Math.max(0, Math.min(1, levels[k]));
      live.style.height = `${lvl * 100}%`;
    } else {
      cell.classList.remove("sounding");   // idle → only the brass (set level) shows
    }
  }
}

// ── Oscilloscope (DPR-aware canvas) ─────────────────────────────────────────
function makeCanvas(id) {
  const canvas = document.getElementById(id);
  const ctx = canvas.getContext("2d");
  // Returns true only when the backing store actually changed — assigning
  // canvas.width/height clears the canvas even when the value is identical, so a
  // no-op resize must not repaint.
  const resize = () => {
    const dpr = window.devicePixelRatio || 1;
    const w = Math.max(1, Math.round(canvas.clientWidth * dpr));
    const h = Math.max(1, Math.round(canvas.clientHeight * dpr));
    if (canvas.width === w && canvas.height === h) return false;
    canvas.width = w;
    canvas.height = h;
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    return true;
  };
  resize();

  // Track the ELEMENT, not the window. The canvas box resizes whenever the layout
  // settles or the frame reflows, and neither fires a window `resize`. Sizing the
  // backing store only at boot pinned it to a transient height; the browser then
  // squashed that store into the settled box and the trace drifted off centre
  // (measured: drawn at h/2, landed at 24% of the box). The window listener in
  // rewireResize() stays for the case this cannot see — a devicePixelRatio change,
  // which alters the store without changing the element's CSS size.
  if (typeof ResizeObserver !== "undefined") {
    new ResizeObserver(() => { if (resize() && lastScope) drawScope(lastScope); }).observe(canvas);
  }
  return { canvas, ctx, resize };
}

let scope = null, lastScope = null;

function drawScope(arr) {
  lastScope = arr;
  if (!scope) return;
  const { canvas, ctx } = scope;
  const w = canvas.clientWidth, h = canvas.clientHeight;
  ctx.clearRect(0, 0, w, h);

  ctx.strokeStyle = "rgba(139,115,85,0.25)";
  ctx.lineWidth = 1;
  ctx.beginPath(); ctx.moveTo(0, h / 2); ctx.lineTo(w, h / 2); ctx.stroke();

  const n = arr.length;          // 128 pts in [-1, 1]
  ctx.strokeStyle = "#9ec46f";
  ctx.lineWidth = 2;
  ctx.lineJoin = "round";
  ctx.beginPath();
  for (let i = 0; i < n; i++) {
    const x = (i / (n - 1)) * w;
    const y = h / 2 - arr[i] * (h / 2 - 3);
    if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  }
  ctx.stroke();
}

function setupVizEvents() {
  if (window.__JUCE__ && window.__JUCE__.backend) {
    window.__JUCE__.backend.addEventListener("drawbarSpectrumUpdate", (d) => {
      if (d) updateDrawbarSpectrum(!!d.sounding, d.levels);
    });
    window.__JUCE__.backend.addEventListener("scopeUpdate", (arr) => drawScope(arr));
  } else {
    console.error("window.__JUCE__.backend unavailable — viz events will not arrive.");
  }
}

function rewireResize() {
  window.addEventListener("resize", () => {
    if (scope) scope.resize();
    if (lastScope) drawScope(lastScope);
  });
}

// ── Hover-help switch (v1.2.0) ──────────────────────────────────────────────
// The toggle O-simpleGrain carries, ported. It is a BROWSER-side preference
// under localStorage "osadd.tipsEnabled", not session state: this plugin has no
// tooltips bridge and never had one, and C++ state for it would be a new
// persistence surface rather than a settings-panel change. The tooltip engine
// below reads tipsEnabled at show time, so the switch takes effect on the next
// hover with nothing to rebind.
let tipsEnabled = true;                 // shipped default; localStorage wins at boot
let hideTooltip = () => {};             // published by the tooltip setup (used by the toggle)

function applyTipsEnabled(on) {
  tipsEnabled = !!on;
  if (!tipsEnabled) hideTooltip();

  const btn = document.getElementById("help-toggle");
  if (!btn) return;
  btn.setAttribute("aria-pressed", tipsEnabled ? "true" : "false");
  // Two calls behind an if/else, never a ternary in the setLabel argument:
  // check-i18n assertion 13 rejects a conditional anywhere inside the call, and
  // it is right to — a reviewer cannot tell a message-selection ternary from a
  // hand-inflection one.
  if (tipsEnabled) setLabel(btn, "ui.on");
  else             setLabel(btn, "ui.off");
}

function setupTipsToggle() {
  const btn = document.getElementById("help-toggle");
  if (!btn) { console.error("Missing help-toggle element"); return; }

  let stored = null;
  try { stored = localStorage.getItem("osadd.tipsEnabled"); } catch (e) { stored = null; }
  applyTipsEnabled(stored !== "false");

  btn.addEventListener("click", () => {
    applyTipsEnabled(!tipsEnabled);
    try { localStorage.setItem("osadd.tipsEnabled", String(tipsEnabled)); } catch (e) { /* private mode */ }
  });
}

// ── Tooltips ────────────────────────────────────────────────────────────────
function setupTooltips() {
  const tip = document.getElementById("tooltip");
  let active = null;

  // The copy is read from the anchor's OWN attributes, which applyI18n rewrote
  // in the current language. v1.0.7 looked it up in a TIPS object keyed by the
  // anchor's data-tip; that table is gone, and with it the second code path
  // that would have gone stale on a language switch.
  //
  // Built with createElement + textContent, not innerHTML. The tip text is now
  // table-sourced rather than a fixed literal, and localized copy must never
  // reach a markup path.
  const show = (el, x, y) => {
    if (!tipsEnabled) return;
    const title = el.getAttribute("data-tip-title");
    const body = el.getAttribute("data-tip");
    if (!title && !body) return;
    tip.textContent = "";
    if (title) {
      const t = document.createElement("span");
      t.className = "tip-title";
      t.textContent = title;
      tip.appendChild(t);
    }
    if (body) tip.appendChild(document.createTextNode(body));
    tip.classList.add("show");
    tip.setAttribute("aria-hidden", "false");
    position(x, y);
  };
  const position = (x, y) => {
    const r = tip.getBoundingClientRect();
    let nx = x + 14, ny = y + 16;
    if (nx + r.width > window.innerWidth - 8) nx = x - r.width - 14;
    if (ny + r.height > window.innerHeight - 8) ny = y - r.height - 12;
    tip.style.left = `${Math.max(8, nx)}px`;
    tip.style.top = `${Math.max(8, ny)}px`;
  };
  const hide = () => { tip.classList.remove("show"); tip.setAttribute("aria-hidden", "true"); active = null; };
  hideTooltip = hide;

  // DELEGATED on the document rather than attached per element. The anchors do
  // not carry data-tip until applyI18n has run and the sixteen drawbar cells do
  // not exist until buildDrawbars has run, so a querySelectorAll at setup time
  // would bind whatever happened to exist at that instant. Delegation has no
  // ordering to get wrong. pointerover/pointerout and focusin/focusout are used
  // because — unlike pointerenter/pointerleave and focus/blur — they bubble.
  const anchorOf = (t) => (t && t.closest ? t.closest("[data-tip]") : null);

  // v1.1.2 — FOCUS LATCH (Stage O item 58, the O-Comp v1.7.0 shape). Through
  // v1.1.1 the focusin handler below opened the tip for ANY focus, so a pointer
  // click on the gear, the language selector, either combo or a lesson button
  // (ten anchors the browser focuses on click) showed hover help the user had
  // just dismissed with the very pointerdown that hides it. Keyboard focus is
  // the accessibility half of this feature and must still open the tip, so the
  // discriminator is the last input device: pointerdown latches, any keydown
  // releases, focusin opens only while released.
  //
  // :focus-visible is deliberately NOT the discriminator. Chromium reports it
  // false for a programmatic .focus() following a click, so a gate driving
  // focus directly would measure "no tip" and record that as correct — a false
  // pass built into the fix. An explicit latch on the last input device is the
  // same rule and is drivable with real events.
  //
  // The one programmatic .focus() on this page is setupSettingsPopover's
  // Escape → gearBtn.focus(). Its keydown listener is registered before this
  // one (boot order), so the gear's focusin runs while the latch still holds
  // whatever opened the popover: a click leaves it latched (no tip), a key had
  // released it (tip opens) — and this listener's Escape then hides either way.
  //
  // The knobs never reached focusin from a click at all: their pointerdown
  // calls preventDefault, which suppresses the focus change.
  let lastInputWasPointer = false;

  // SECOND MECHANISM, found by counting (the sixteen drawbars). A press on a
  // drawbar hides the tip, then setFromY grows #fill-partialN under the pointer,
  // and the child boundary fires pointerout/pointerover with active === null —
  // so the hover path re-opened the tip on the anchor the pointer had just
  // pressed. Remember the pressed anchor and refuse to re-open on it until the
  // pointer has LEFT it; a knob already behaved this way by accident (its
  // stem is pointer-events: none, so nothing changes under the pointer).
  let pressedAnchor = null;

  document.addEventListener("pointerover", (e) => {
    const el = anchorOf(e.target);
    if (!el || el === active || el === pressedAnchor) return;
    active = el;
    show(el, e.clientX, e.clientY);
  });
  document.addEventListener("pointermove", (e) => {
    if (active && anchorOf(e.target) === active) position(e.clientX, e.clientY);
  });
  document.addEventListener("pointerout", (e) => {
    const to = anchorOf(e.relatedTarget);
    if (pressedAnchor && to !== pressedAnchor) pressedAnchor = null;
    if (!active) return;
    // Ignore a move between two descendants of the SAME anchor: pointerout
    // fires on every child boundary and would flicker the tip off and on.
    if (to === active) return;
    hide();
  });
  document.addEventListener("pointerdown", (e) => {
    lastInputWasPointer = true;
    pressedAnchor = anchorOf(e.target);
    hide();
  });

  document.addEventListener("focusin", (e) => {
    if (lastInputWasPointer) return;
    const el = anchorOf(e.target);
    if (!el) return;
    active = el;
    const r = el.getBoundingClientRect();
    show(el, r.left + r.width / 2, r.bottom);
  });
  document.addEventListener("focusout", hide);

  // One keydown listener, two jobs: any key at all means the keyboard is
  // driving again, which releases the latch above; Escape additionally hides.
  // Registered separately from setupSettingsPopover's own Escape handler,
  // which closes the popover — both run, and they are independent.
  document.addEventListener("keydown", (e) => {
    lastInputWasPointer = false;
    if (e.key === "Escape") hide();
  });
}

// ── Lesson preset tour ──────────────────────────────────────────────────────
// The lessons are applied in C++ (applyFactoryPreset native fn) as full APVTS
// snapshots; the relays/attachments sync every knob, drawbar, and combo here.
//
// Through v1.0.7 a LESSONS table here held each caption as one "Name — body"
// string and the six button tooltips were DERIVED from it by splitting on the
// first em-dash. Both halves are authored table entries in js/i18n.js now:
// a derived string is invisible to the translator reviewing that file, and once
// the page has two languages the table is the single source of truth rather
// than the caption.
//
// What is left here is a dispatch from the C++ preset name (which is also the
// button's data-preset, and is never localized) to a WRITER that names its
// caption key as a plain string literal.
//
// A map of name -> key with `setLabel(cap, KEYS[name] || "label.captionDefault")`
// reads more naturally and is what this was first written as. check-i18n
// assertion 13 rejects it twice over, correctly: a computed key cannot be
// checked against the table, and the `||` is the conditional-inside-a-localized
// -string shape contract §6 forbids. Seven call sites, seven literals, and
// assertion 15 can see that all seven keys are live.
const LESSON_CAPTION_WRITERS = {
  "Pure Sine":   (el) => setLabel(el, "label.captionSine"),
  "Sawtooth":    (el) => setLabel(el, "label.captionSaw"),
  "Square":      (el) => setLabel(el, "label.captionSquare"),
  "Organ":       (el) => setLabel(el, "label.captionOrgan"),
  "Morph Pad":   (el) => setLabel(el, "label.captionMorph"),
  "Lo-Fi Bells": (el) => setLabel(el, "label.captionLofi"),
};

let applyPresetFn = null;

async function applyLesson(name) {
  if (applyPresetFn) { try { await applyPresetFn(name); } catch (e) { /* ignore */ } }
  const cap = document.getElementById("tourCaption");
  // setLabel, not textContent: the caption becomes a [data-i18n] element from
  // this moment on, so the language sweep owns it. Written as a raw string it
  // would be stranded in the language it was picked in the instant the selector
  // fires — and it is the one string on this page that is chosen by a click.
  // An unknown name falls back to the resting caption rather than to "", so a
  // C++/JS name drift shows as the default text instead of a blank row.
  if (cap) {
    const write = LESSON_CAPTION_WRITERS[name];
    if (write) write(cap);
    else setLabel(cap, "label.captionDefault");
  }
  document.querySelectorAll(".tour-btn").forEach((b) =>
    b.classList.toggle("active", b.getAttribute("data-preset") === name));
}

function setupPresets() {
  try { applyPresetFn = Juce.getNativeFunction("applyFactoryPreset"); } catch (e) { applyPresetFn = null; }
  document.querySelectorAll(".tour-btn").forEach((btn) => {
    btn.addEventListener("click", () => applyLesson(btn.getAttribute("data-preset")));
  });
}

// ── On-screen keyboard (play without external MIDI) ──────────────────────────
const KB_LOW = 48, KB_HIGH = 72;              // C3 … C5 inclusive
const BLACK_OFFSETS = new Set([1, 3, 6, 8, 10]);
const QWERTY = { a: 60, w: 61, s: 62, e: 63, d: 64, f: 65, t: 66, g: 67, y: 68, h: 69, u: 70, j: 71, k: 72 };

let uiMidiFn = null;
const keyEls = {};
const heldNotes = new Set();

function noteOn(note, vel = 0.8) {
  if (note < 0 || note > 127 || heldNotes.has(note)) return;
  heldNotes.add(note);
  if (uiMidiFn) uiMidiFn(note, true, vel);
  const el = keyEls[note];
  if (el) el.classList.add("active");
}
function noteOff(note) {
  if (!heldNotes.has(note)) return;
  heldNotes.delete(note);
  if (uiMidiFn) uiMidiFn(note, false, 0);
  const el = keyEls[note];
  if (el) el.classList.remove("active");
}

function setupKeyboard() {
  const kb = document.getElementById("keyboard");
  if (!kb) return;
  try { uiMidiFn = Juce.getNativeFunction("uiMidi"); } catch (e) { uiMidiFn = null; }

  const qFor = (n) => Object.keys(QWERTY).find((k) => QWERTY[k] === n);
  const blacks = [];
  for (let n = KB_LOW; n <= KB_HIGH; n++) {
    const isBlack = BLACK_OFFSETS.has(((n % 12) + 12) % 12);
    const el = document.createElement("div");
    el.className = "key " + (isBlack ? "key-black" : "key-white");
    el.dataset.note = String(n);
    const q = qFor(n);
    if (q) {
      const lab = document.createElement("span");
      lab.className = "key-label";
      lab.textContent = q.toUpperCase();
      el.appendChild(lab);
    }
    keyEls[n] = el;
    if (isBlack) blacks.push(el);
    else kb.appendChild(el);
  }
  blacks.forEach((el) => kb.appendChild(el));

  const positionBlacks = () => {
    for (const el of blacks) {
      const n = +el.dataset.note;
      const leftWhite = keyEls[n - 1];
      if (!leftWhite) continue;
      el.style.left = (leftWhite.offsetLeft + leftWhite.offsetWidth - el.offsetWidth / 2) + "px";
    }
  };
  requestAnimationFrame(positionBlacks);
  window.addEventListener("resize", positionBlacks);

  let pointerNote = null;
  const noteAt = (target) => {
    const k = target.closest ? target.closest(".key") : null;
    return k ? +k.dataset.note : null;
  };
  kb.addEventListener("pointerdown", (e) => {
    const n = noteAt(e.target);
    if (n == null) return;
    pointerNote = n;
    noteOn(n);
    e.preventDefault();
  });
  kb.addEventListener("pointerover", (e) => {
    if (pointerNote == null) return;
    const n = noteAt(e.target);
    if (n != null && n !== pointerNote) { noteOff(pointerNote); noteOn(n); pointerNote = n; }
  });
  window.addEventListener("pointerup", () => {
    if (pointerNote != null) { noteOff(pointerNote); pointerNote = null; }
  });

  // Stuck-note panic path: pointerup released outside the plugin window never
  // arrives, a pointercancel fires neither up nor cleanup, and a QWERTY keyup
  // goes to the DAW when focus leaves mid-press. Release everything on any of
  // those escapes so no note can drone indefinitely.
  const releaseAll = () => {
    [...heldNotes].forEach(noteOff);
    pointerNote = null;
  };
  window.addEventListener("blur", releaseAll);
  document.addEventListener("visibilitychange", () => { if (document.hidden) releaseAll(); });
  window.addEventListener("pointercancel", () => {
    if (pointerNote != null) { noteOff(pointerNote); pointerNote = null; }
  });

  window.addEventListener("keydown", (e) => {
    if (e.repeat || e.metaKey || e.ctrlKey || e.altKey) return;
    const n = QWERTY[e.key.toLowerCase()];
    if (n == null) return;
    noteOn(n);
    e.preventDefault();
  });
  window.addEventListener("keyup", (e) => {
    const n = QWERTY[e.key.toLowerCase()];
    if (n != null) noteOff(n);
  });
}

// ── Settings popover (v1.1.0) ───────────────────────────────────────────────
// The gear panel holding the language selector and, since v1.2.0, the
// hover-help switch (wired by setupTipsToggle above). All state lives in this
// closure, so nothing here can join a TDZ chain.
//
// The panel holds the selector ALONE: this plugin has no tooltips bridge and
// never had a hover-help toggle — its help layer is always on — so a toggle row
// would be a control for a preference that does not exist.
//
// Styled in O-simpleAdditive's own aged-paper vocabulary in css/styles.css: the
// same .combo border, radius and inset shadow the two drop-downs already wear,
// with the green reserved for the lit state exactly as .tour-btn.active uses
// it. It is not a widget pasted in unchanged from another plugin.
function setupSettingsPopover() {
  const gearBtn = document.getElementById("gear-btn");
  const popover = document.getElementById("settings-popover");

  if (gearBtn === null || popover === null) {
    console.warn("Settings popover missing — language selector unavailable");
    return;
  }

  const setOpen = (open) => {
    popover.hidden = !open;
    gearBtn.setAttribute("aria-expanded", open ? "true" : "false");
  };

  gearBtn.addEventListener("click", (e) => {
    e.stopPropagation();
    setOpen(popover.hidden);
  });

  // Dismiss on a press anywhere else, and on Escape. pointerdown rather than
  // click, so the panel is gone before a drag on a knob or drawbar underneath
  // it begins — both call preventDefault in their own pointerdown handlers.
  document.addEventListener("pointerdown", (e) => {
    if (popover.hidden) return;
    if (popover.contains(e.target) || gearBtn.contains(e.target)) return;
    setOpen(false);
  });

  document.addEventListener("keydown", (e) => {
    if (e.key === "Escape" && !popover.hidden) {
      setOpen(false);
      gearBtn.focus();
    }
  });
}

// ── Boot ────────────────────────────────────────────────────────────────────
function boot() {
  scope = makeCanvas("scopeCanvas");

  // buildDrawbars FIRST: the sixteen cells it creates carry data-param and
  // data-i18n-aria, and initI18n's sweep can only reach elements that exist.
  // A cell built after the sweep would have no tip and no accessible name.
  buildDrawbars();

  // The popover and the language sweep go next, EACH IN ITS OWN try/catch. A
  // translation-table typo must not be allowed to take the 33 parameter
  // bindings, the scope and the keyboard down with it — that is the v1.4.0 TDZ
  // failure this repo has already paid for once.
  try { setupSettingsPopover(); } catch (e) { console.error("settings popover init failed:", e); }
  try { initI18n(); }             catch (e) { console.error("i18n init failed:", e); }
  try { setupTipsToggle(); }      catch (e) { console.error("tips toggle init failed:", e); }

  DRAWBAR_IDS.forEach(bindDrawbar);
  KNOB_IDS.forEach(bindKnob);
  COMBO_IDS.forEach(bindCombo);

  setupTooltips();
  setupPresets();
  setupKeyboard();
  setupVizEvents();
  rewireResize();

  // initial empty scope so the canvas isn't black
  drawScope(new Array(128).fill(0));
}

if (document.readyState === "loading")
  document.addEventListener("DOMContentLoaded", boot);
else
  boot();
