/*
   This file is part of O-simpleFM, an Ouaricon Audio plugin.
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
// O-simpleFM — WebView UI controller
// Binds all 17 APVTS params two-way, draws the live spectrum + scope, and runs
// the pedagogical layer (routing diagram, tooltips, preset tour).
//
// NOTE: getSliderState / getToggleState / getNativeFunction live on the `Juce`
// ES-module namespace. window.__JUCE__ has the low-level backend (used here only
// for backend.addEventListener on the spectrum/scope push events).
// ============================================================================

import * as Juce from "./juce/index.js";
import { PresetManager } from "../modules/preset-manager.js";

// ════════════════════════════════════════════════════════════════════════════
// INTERFACE LANGUAGE (v1.3.0) — canon v2, verbatim from scripts/i18n-canon.js.
//
// It sits HERE, directly under the imports and above every other declaration in
// this module, because `uiLanguage` is a module-level `let`: anything above it
// that reached applyLabel() would touch it inside its temporal dead zone and
// throw out of module evaluation, taking the seventeen parameter bindings, the
// two canvases, the routing diagram and the on-screen keyboard with it
// (pattern_module_toplevel_init_tdz). This file DOES have eager top-level work
// below — KNOB_IDS, FORMAT, LESSONS — so the ordering is load-bearing here, not
// merely defensive. `node scripts/boot-all-uis.js` is the ONLY gate in the repo
// that sees this class of failure.
//
// Do NOT edit the region between the import line and the close of initI18n():
// check-i18n assertion 6 byte-compares it against scripts/i18n-canon.js.
// ════════════════════════════════════════════════════════════════════════════
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
            .then((code) => applyI18n(code === 'fr' ? 'fr' : 'en'))
            .catch((e) => console.warn('Could not read language preference:', e));
    }

    const sel = document.getElementById('lang-select');
    if (sel) sel.addEventListener('change', (e) => {
        applyI18n(e.target.value);
        if (setUiLanguageNative) setUiLanguageNative(uiLanguage).catch(() => {});
    });
}

// ── Parameter inventory (must match OSimpleFM::ParamIDs exactly) ───────────
const KNOB_IDS = [
  "ratio", "modIndex", "feedback", "modFixedHz", "modEnvToIndex", "velToIndex",
  "modAttack", "modDecay", "modSustain", "modRelease",
  "ampAttack", "ampDecay", "ampSustain", "ampRelease", "outputLevel",
];
const TOGGLE_IDS = ["ratioSnap", "modFixedMode"];

// ── Display formatters (keyed by param id) ─────────────────────────────────
// Each receives the *scaled* value (NormalisableRange::convertFrom0to1 output).
const fmtMs = (s) => (s < 1 ? `${Math.round(s * 1000)} ms` : `${s.toFixed(2)} s`);
const fmtPct = (v) => `${Math.round(v * 100)}%`;
const FORMAT = {
  ratio: (v) => `${v.toFixed(2)} : 1`,
  modIndex: (v) => v.toFixed(2),
  feedback: (v) => fmtPct(v),
  modFixedHz: (v) => (v >= 1000 ? `${(v / 1000).toFixed(2)} kHz` : `${Math.round(v)} Hz`),
  modEnvToIndex: fmtPct,
  velToIndex: fmtPct,
  modAttack: fmtMs, modDecay: fmtMs, modSustain: fmtPct, modRelease: fmtMs,
  ampAttack: fmtMs, ampDecay: fmtMs, ampSustain: fmtPct, ampRelease: fmtMs,
  outputLevel: (v) => `${v.toFixed(1)} dB`,
};

// ── Tooltip copy ────────────────────────────────────────────────────────────
// It lives in js/i18n.js now, in BOTH languages, and applyI18n writes it onto
// each anchor as data-tip-title + data-tip. Through v1.2.5 a TIPS object here
// held [title, bodyHtml] pairs keyed by the anchor's own data-tip attribute —
// which canon v2 overwrites with the tip BODY, so the key and the copy would
// have fought over one attribute.

// ── Knob geometry ──────────────────────────────────────────────────────────
const KNOB_MIN_DEG = -135;   // 0.0 normalised
const KNOB_MAX_DEG = 135;    // 1.0 normalised
const DRAG_TRAVEL_PX = 220;  // vertical px for full 0..1 sweep

// Modulation-index range + perceptual taper — mirrors the C++ single source of
// truth (FMVoice.h: OSimpleFM::kIndexMax / kIndexTaper). Keep in sync.
const INDEX_MAX = 20;
const INDEX_TAPER = 1.7;

const sliderState = {};   // id -> Juce SliderState
const toggleState = {};   // id -> Juce ToggleState

// paramID -> normalised default, fetched once from C++ (getParameterDefaults).
// Powers double-click-reset on the knobs; empty until the async fetch lands.
let paramDefaults = {};

function normToDeg(n) { return KNOB_MIN_DEG + n * (KNOB_MAX_DEG - KNOB_MIN_DEG); }

function updateKnobVisual(id) {
  const st = sliderState[id];
  if (!st) return;
  const norm = st.getNormalisedValue();
  const knob = document.getElementById(`knob-${id}`);
  if (knob) {
    const stem = knob.querySelector(".knob-stem");
    if (stem) stem.style.transform = `translate(-50%, -100%) rotate(${normToDeg(norm)}deg)`;
  }
  const valEl = document.getElementById(`val-${id}`);
  if (valEl) {
    const fmt = FORMAT[id] || ((v) => v.toFixed(2));
    valEl.textContent = fmt(st.getScaledValue());
  }
}

// One-shot fine adjust shared by the wheel + arrow-key paths (full drag gesture).
function nudge(st, delta, id) {
  const n = Math.max(0, Math.min(1, st.getNormalisedValue() + delta));
  st.sliderDragStarted();
  st.setNormalisedValue(n);
  st.sliderDragEnded();
  updateKnobVisual(id);
}

// ── Knob binding (relative vertical drag) ──────────────────────────────────
function bindKnob(id) {
  const st = Juce.getSliderState(id);
  sliderState[id] = st;

  st.valueChangedEvent.addListener(() => updateKnobVisual(id));
  st.propertiesChangedEvent.addListener(() => updateKnobVisual(id));
  updateKnobVisual(id);

  const knob = document.getElementById(`knob-${id}`);
  if (!knob) { console.error(`Missing knob element: knob-${id}`); return; }

  // Keyboard operable (accessibility): focusable + arrow-key fine adjust.
  knob.setAttribute("tabindex", "0");
  knob.setAttribute("role", "slider");
  knob.addEventListener("keydown", (e) => {
    let delta = 0;
    if (e.key === "ArrowUp" || e.key === "ArrowRight") delta = 0.02;
    else if (e.key === "ArrowDown" || e.key === "ArrowLeft") delta = -0.02;
    else return;
    nudge(st, delta, id);
    e.preventDefault();
  });

  let dragging = false;
  let startY = 0;
  let startNorm = 0;

  const onMove = (e) => {
    if (!dragging) return;
    const dy = startY - e.clientY;
    let n = startNorm + dy / DRAG_TRAVEL_PX;
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
  };

  knob.addEventListener("pointerdown", (e) => {
    dragging = true;
    startY = e.clientY;
    startNorm = st.getNormalisedValue();
    st.sliderDragStarted();
    window.addEventListener("pointermove", onMove);
    window.addEventListener("pointerup", onUp);
    e.preventDefault();
  });

  // Mouse wheel fine adjust.
  knob.addEventListener("wheel", (e) => {
    nudge(st, e.deltaY < 0 ? 0.02 : -0.02, id);
    e.preventDefault();
  }, { passive: false });

  // Double-click → reset to the parameter default (suite standard).
  knob.addEventListener("dblclick", (e) => {
    const d = paramDefaults[id];
    if (typeof d !== "number") return;
    st.sliderDragStarted();
    st.setNormalisedValue(d);
    st.sliderDragEnded();
    updateKnobVisual(id);
    e.preventDefault();
  });
}

// ── Toggle binding ─────────────────────────────────────────────────────────
function bindToggle(id) {
  const st = Juce.getToggleState(id);
  toggleState[id] = st;
  const el = document.getElementById(`toggle-${id}`);
  if (!el) { console.error(`Missing toggle element: toggle-${id}`); return; }

  const refresh = () => {
    el.classList.toggle("active", st.getValue());
    if (id === "modFixedMode") updateFixedModeVisibility(st.getValue());
  };
  st.valueChangedEvent.addListener(refresh);
  st.propertiesChangedEvent.addListener(refresh);
  refresh();

  el.addEventListener("click", () => { st.setValue(!st.getValue()); refresh(); });
}

function updateFixedModeVisibility(fixed) {
  // v1.3.0: was '.knob-cell[data-tip="modFixedHz"]'. The tip ANCHOR moved to
  // data-param when applyI18n took ownership of data-tip, and this selector
  // would have matched nothing from that moment on — no error, no warning, the
  // Fixed Hz cell just stops dimming when Fixed Mode is off.
  const cell = document.querySelector('.knob-cell[data-param="modFixedHz"]');
  if (cell) cell.style.opacity = fixed ? "1" : "0.4";
}

// ── Routing diagram (reflects ratio / modIndex / feedback) ─────────────────
function updateRouting() {
  const idx = sliderState.modIndex;
  const fb = sliderState.feedback;
  const rt = sliderState.ratio;
  if (!idx || !fb || !rt) return;

  const idxNorm = idx.getNormalisedValue();      // 0..1
  const fbNorm = fb.getNormalisedValue();        // 0..1
  const ratioVal = rt.getScaledValue();

  const line = document.getElementById("modToCar");
  const head = document.getElementById("modToCarHead");
  if (line) line.style.strokeWidth = (1.5 + idxNorm * 6).toFixed(2);
  if (head) head.style.opacity = (0.35 + idxNorm * 0.65).toFixed(2);

  const arc = document.getElementById("fbArc");
  const fbHead = document.getElementById("fbHead");
  if (arc) { arc.style.strokeWidth = (0.5 + fbNorm * 5).toFixed(2); arc.style.opacity = (0.15 + fbNorm * 0.85).toFixed(2); }
  if (fbHead) fbHead.style.opacity = (0.15 + fbNorm * 0.85).toFixed(2);

  const mod = document.getElementById("opMod");
  if (mod) mod.style.filter = `brightness(${(1 + fbNorm * 0.25).toFixed(2)})`;

  const rRead = document.getElementById("routeRatioRead");
  if (rRead) rRead.textContent = `${ratioVal.toFixed(2)} : 1`;
  const iRead = document.getElementById("routeIndexRead");
  if (iRead) iRead.textContent = idx.getScaledValue().toFixed(1);

  // Teaching meta: harmonic when the ratio is (near) a whole number; Carson's
  // rule puts ~I+1 significant sideband pairs either side of the carrier.
  //
  // v1.3.0: two literal-keyed writers behind an if/else, never one call with a
  // ternary in its argument — check-i18n assertion 13 rejects a conditional
  // anywhere inside a setLabel call, and it is right to: a reviewer cannot tell
  // a message-selection ternary from a plural one by reading it. The English
  // plural suffix went with it; the copy is authored around the inflection now
  // (contract §6), because French pluralizes zero and one alike.
  const meta = document.getElementById("routeMeta");
  if (meta) {
    const harmonic = Math.abs(ratioVal - Math.round(ratioVal)) < 0.02 && Math.round(ratioVal) >= 1;
    const nSb = Math.max(1, Math.round(idx.getScaledValue()) + 1);
    if (harmonic) setLabel(meta, "label.metaHarmonic",   { n: nSb });
    else          setLabel(meta, "label.metaInharmonic", { n: nSb });
  }

  // Carrier-null teaching badge: the carrier (Bessel J₀) collapses to zero when
  // the EFFECTIVE radian index reaches the first J₀ zero, β ≈ 2.405. The readout's
  // I is the raw control value; the DSP applies a perceptual taper
  // (FMVoice::setParams → baseIndex = 20·(I/20)^1.7), and the render-harness
  // "carrier-null@2.405" test sets I so baseIndex == 2.405. Match that here so the
  // badge lights exactly when the carrier marker sits on a nulled peak (≈ I 5.75).
  const badge = document.getElementById("carrierNullBadge");
  if (badge) {
    const effIndex = INDEX_MAX * Math.pow(idx.getScaledValue() / INDEX_MAX, INDEX_TAPER);
    badge.classList.toggle("show", Math.abs(effIndex - 2.405) <= 0.15);
  }
}

// ── Tooltips ────────────────────────────────────────────────────────────────
// The copy is no longer looked up here: applyI18n has already written it onto
// each anchor as data-tip-title + data-tip, in the current language, and it
// rewrites both on every language change. This function only positions and
// shows what the anchor carries.
function setupTooltips() {
  const tip = document.getElementById("tooltip");
  if (!tip) return;
  let active = null;

  // Built with createElement + textContent, not innerHTML. The tip text is
  // table-sourced and localized now rather than a fixed literal, and localized
  // copy must never reach a markup path. v1.2.5's tip bodies carried strong/em
  // tags; the words are unchanged and the tags are gone (check-i18n assertion 9
  // forbids an angle bracket in an i18n.js string literal).
  const show = (el, x, y) => {
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

  // DELEGATED on the document rather than attached per element. No anchor
  // carries data-tip until applyI18n has run, so the v1.2.5
  // querySelectorAll("[data-tip]") at setup time would bind NOTHING at all.
  // Delegation has no ordering to get wrong. pointerover/pointerout and
  // focusin/focusout are used because — unlike pointerenter/pointerleave and
  // focus/blur — they bubble. closest() also gets the nesting right for free:
  // the carrier-null badge sits inside the readout, which sits inside the
  // routing panel, and the innermost anchor wins without a stopPropagation.
  const anchorOf = (t) => (t && t.closest ? t.closest("[data-tip]") : null);

  document.addEventListener("pointerover", (e) => {
    const el = anchorOf(e.target);
    if (!el || el === active) return;
    active = el;
    show(el, e.clientX, e.clientY);
  });
  document.addEventListener("pointermove", (e) => {
    if (active && anchorOf(e.target) === active) position(e.clientX, e.clientY);
  });
  document.addEventListener("pointerout", (e) => {
    if (!active) return;
    // Ignore a move between two descendants of the SAME anchor: pointerout
    // fires on every child boundary and would flicker the tip off and on.
    if (anchorOf(e.relatedTarget) === active) return;
    hide();
  });
  document.addEventListener("pointerdown", hide);

  document.addEventListener("focusin", (e) => {
    const el = anchorOf(e.target);
    if (!el) return;
    active = el;
    const r = el.getBoundingClientRect();
    show(el, r.left + r.width / 2, r.bottom);
  });
  document.addEventListener("focusout", hide);

  // The routing panel has no focusable child — make it itself focusable.
  const routing = document.getElementById("routingPanel");
  if (routing && !routing.hasAttribute("tabindex")) routing.setAttribute("tabindex", "0");

  document.addEventListener("keydown", (e) => { if (e.key === "Escape") hide(); });
}

// ── Settings popover (v1.3.0) ───────────────────────────────────────────────
// The gear panel holding the language selector. All state lives in this
// closure, so nothing here can join a TDZ chain.
//
// Styled in O-simpleFM's own aged-paper field-guide vocabulary in
// css/styles.css: the panel wears the .group plate, the gear wears the
// .toggle's green border and lit state, and the selector wears the preset-bar
// button's border, radius and paper fill at panel scale. It is not a widget
// pasted in unchanged from another plugin.
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
  // click, so the panel is gone before a drag on a knob underneath it begins —
  // bindKnob calls preventDefault in its own pointerdown handler.
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

// ── Preset tour ─────────────────────────────────────────────────────────────
// The five lessons ARE the factory presets — single source of truth lives in
// FactoryPresets.cpp. The tour buttons load them by name through the preset
// manager (which pushes every param via setValueNotifyingHost, so the knobs,
// toggles, and routing diagram all sync automatically). Only the teaching
// captions live here.
// The NAME is the factory preset's name, which is also the JSON filename, so it
// stays English in both languages (D-02) — and it is why the five button faces
// are I18N_EXEMPT rather than translated: the header preset bar displays this
// same name, and a French face over an English bar entry would make the two
// disagree about what is loaded.
const LESSONS = {
  epiano:   { name: "E-Piano" },
  tubular:  { name: "Tubular Bell" },
  brass:    { name: "Brass" },
  clarinet: { name: "Clarinet" },
  clang:    { name: "Clang Bell" },
};

// The teaching captions are table entries now, written through a DISPATCH OF
// ONE-LINE WRITERS rather than one setLabel with a computed key. check-i18n
// assertion 13 rejects a computed key twice over — it cannot be checked, and a
// raw copy string in that position would ship English — and assertion 15 only
// counts a literal key as a live reference, so a lookup table of key STRINGS
// would report all five as dead translations nobody sees.
const LESSON_CAPTION = {
  epiano:   (el) => setLabel(el, "label.captionEpiano"),
  tubular:  (el) => setLabel(el, "label.captionTubular"),
  brass:    (el) => setLabel(el, "label.captionBrass"),
  clarinet: (el) => setLabel(el, "label.captionClarinet"),
  clang:    (el) => setLabel(el, "label.captionClang"),
};

async function applyLesson(key) {
  const lesson = LESSONS[key];
  if (!lesson) return;
  if (presetManager) await presetManager.loadPreset(lesson.name);
  const cap = document.getElementById("tourCaption");
  const writeCaption = LESSON_CAPTION[key];
  if (cap && writeCaption) writeCaption(cap);
  document.querySelectorAll(".tour-btn").forEach((b) =>
    b.classList.toggle("active", b.getAttribute("data-preset") === key));
}

function setupPresets() {
  document.querySelectorAll(".tour-btn").forEach((btn) => {
    btn.addEventListener("click", () => applyLesson(btn.getAttribute("data-preset")));
  });
}

// ── Preset manager (persistent factory/user JSON presets) ────────────────────
// Backed by the suite OuariconPresetManager via 10 JUCE native functions. The
// in-UI "Lesson Presets" tour above is the pedagogical layer; this bar is the
// persistent, save/load-able layer that also survives DAW session reloads.
let presetManager = null;

function closeDropdown() {
  const dd = document.getElementById("presetDropdown");
  const nameBtn = document.getElementById("presetName");
  if (dd) dd.classList.remove("show");
  if (nameBtn) nameBtn.setAttribute("aria-expanded", "false");
}

async function buildPresetDropdown() {
  const dd = document.getElementById("presetDropdown");
  if (!dd || !presetManager) return;
  // Pull a fresh list+current from C++ first, so the very first open can't race
  // the async init cache and render empty.
  await presetManager.refresh();
  const list = presetManager.getPresetList();
  const current = presetManager.getCurrentPreset();
  // Classify factory vs user (async) so the list groups cleanly.
  const flags = await Promise.all(list.map((n) => presetManager.isFactoryPreset(n)));

  dd.innerHTML = "";
  // writeLabel, not a label STRING: the heading is localized and its key has to
  // be a plain literal at the call site (assertion 13), so each caller brings
  // its own one-line writer. The preset NAMES below stay exactly as C++ reports
  // them — a preset name is its JSON filename (D-02).
  const addGroup = (names, writeLabel) => {
    if (!names.length) return;
    const hdr = document.createElement("div");
    hdr.className = "preset-group-label";
    writeLabel(hdr);
    dd.appendChild(hdr);
    names.forEach((n) => {
      const item = document.createElement("div");
      item.className = "preset-dropdown-item" + (n === current ? " active" : "");
      item.setAttribute("role", "option");
      item.tabIndex = 0;
      item.textContent = n;
      const choose = () => { presetManager.loadPreset(n); closeDropdown(); };
      item.addEventListener("click", choose);
      item.addEventListener("keydown", (e) => {
        if (e.key === "Enter" || e.key === " ") { e.preventDefault(); choose(); }
      });
      dd.appendChild(item);
    });
  };
  addGroup(list.filter((_, i) => flags[i]),  (el) => setLabel(el, "label.presetFactory"));
  addGroup(list.filter((_, i) => !flags[i]), (el) => setLabel(el, "label.presetUser"));
}

function toggleDropdown() {
  const dd = document.getElementById("presetDropdown");
  const nameBtn = document.getElementById("presetName");
  if (!dd) return;
  if (dd.classList.contains("show")) { closeDropdown(); return; }
  buildPresetDropdown();
  dd.classList.add("show");
  if (nameBtn) nameBtn.setAttribute("aria-expanded", "true");
}

// Factory presets can't be deleted — disable the button so the click isn't a silent no-op.
async function updateDeleteButtonState() {
  const delBtn = document.getElementById("presetDelete");
  if (!delBtn || !presetManager) return;
  delBtn.disabled = await presetManager.isFactoryPreset(presetManager.getCurrentPreset());
}

// Minimal in-DOM confirm dialog (window.confirm is unreliable in JUCE WebViews).
// Resolves true on Delete, false on Cancel / Escape / backdrop click.
//
// v1.3.0 takes the preset NAME rather than the finished sentence the preset
// manager composes. The sentence was English prose assembled inside a vendored
// shared module this plugin must not edit; composing it here from a {name}
// token localizes it without touching the module, and the name itself is
// substituted verbatim because a preset name is its JSON filename (D-02).
function confirmInDom(presetName) {
  return new Promise((resolve) => {
    const overlay = document.createElement("div");
    overlay.className = "confirm-overlay";
    const box = document.createElement("div");
    box.className = "confirm-box";
    box.setAttribute("role", "alertdialog");
    box.setAttribute("aria-modal", "true");

    const msg = document.createElement("div");
    msg.className = "confirm-message";
    setLabel(msg, "ui.deleteConfirm", { name: presetName });

    const row = document.createElement("div");
    row.className = "confirm-buttons";
    const cancelBtn = document.createElement("button");
    setLabel(cancelBtn, "ui.cancel");
    const okBtn = document.createElement("button");
    okBtn.className = "confirm-danger";
    setLabel(okBtn, "ui.delete");
    row.append(cancelBtn, okBtn);
    box.append(msg, row);
    overlay.appendChild(box);

    const done = (answer) => {
      document.removeEventListener("keydown", onKey, true);
      overlay.remove();
      resolve(answer);
    };
    const onKey = (e) => {
      if (e.key === "Escape") { e.stopPropagation(); done(false); }
    };
    document.addEventListener("keydown", onKey, true);
    overlay.addEventListener("pointerdown", (e) => { if (e.target === overlay) done(false); });
    cancelBtn.addEventListener("click", () => done(false));
    okBtn.addEventListener("click", () => done(true));

    document.body.appendChild(overlay);
    cancelBtn.focus();   // safe default focus for keyboard users
  });
}

function setupPresetManager() {
  const nameBtn = document.getElementById("presetName");

  presetManager = new PresetManager({
    displayElement: nameBtn,
    prevButton: document.getElementById("presetPrev"),
    nextButton: document.getElementById("presetNext"),
    saveButton: document.getElementById("presetSave"),  // → native save dialog
    getNativeFunction: Juce.getNativeFunction,           // ES-module namespace, NOT window.__JUCE__
    onPresetChanged: () => { updateRouting(); updateDeleteButtonState(); closeDropdown(); },
  });
  presetManager.initialize().then(updateDeleteButtonState);   // disable Delete on the initial (factory) preset

  if (nameBtn) nameBtn.addEventListener("click", toggleDropdown);

  // Delete current preset — destructive and irreversible, so route through the
  // module's promptDelete() with an in-DOM confirm (window.confirm is
  // unreliable inside JUCE WebViews).
  presetManager.onConfirmDelete = (name) => confirmInDom(name);
  const delBtn = document.getElementById("presetDelete");
  if (delBtn) delBtn.addEventListener("click", async () => {
    await presetManager.promptDelete();   // no-op unless confirmed
    closeDropdown();
    updateDeleteButtonState();
  });

  // Dismiss the dropdown on outside click / Escape.
  document.addEventListener("pointerdown", (e) => {
    const bar = document.getElementById("presetBar");
    if (bar && !bar.contains(e.target)) closeDropdown();
  });
  document.addEventListener("keydown", (e) => { if (e.key === "Escape") closeDropdown(); });
}

// ── Canvases (DPR-aware) ─────────────────────────────────────────────────────
function makeCanvas(id) {
  const canvas = document.getElementById(id);
  const ctx = canvas.getContext("2d");
  const resize = () => {
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.clientWidth, h = canvas.clientHeight;
    canvas.width = Math.max(1, Math.round(w * dpr));
    canvas.height = Math.max(1, Math.round(h * dpr));
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  };
  resize();   // single window 'resize' handler lives in rewireResize() (resizes + redraws)
  return { canvas, ctx, resize };
}

let spec = null, scope = null;
let lastSpectrum = null, lastScope = null;

// Spectrum x-axis is log-frequency 20 Hz → Nyquist (matches FmVizAnalyzer).
// nyquistHz comes from C++ (getSampleRate); 22.05 kHz is a safe pre-fetch default.
let nyquistHz = 22050;
const FREQ_TICKS = [100, 1000, 10000];
const fmtTickHz = (f) => (f >= 1000 ? `${f / 1000}k` : `${f}`);

// Carrier frequency of the most-recently-started note, pushed from C++ on the
// `carrierUpdate` event (0 = nothing sounding). Drives the FM sideband markers.
let carrierHz = 0;

function drawSpectrum(arr) {
  lastSpectrum = arr;
  if (!spec) return;
  const { canvas, ctx } = spec;
  const w = canvas.clientWidth, h = canvas.clientHeight;
  ctx.clearRect(0, 0, w, h);

  // subtle horizontal grid lines (dB)
  ctx.strokeStyle = "rgba(139,115,85,0.18)";
  ctx.lineWidth = 1;
  for (let g = 1; g < 4; g++) {
    const y = (h * g) / 4;
    ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
  }

  const n = arr.length;          // 256 bins, dB in ~[-100, 0]
  const bw = w / n;
  for (let i = 0; i < n; i++) {
    const db = arr[i];
    const norm = Math.max(0, Math.min(1, (db + 100) / 100)); // 0..1
    const barH = norm * (h - 2);
    const x = i * bw;
    const hue = 90 - norm * 40;   // green -> yellow-green with intensity
    ctx.fillStyle = `hsl(${hue}, 55%, ${35 + norm * 30}%)`;
    ctx.fillRect(x, h - barH, Math.max(1, bw - 0.5), barH);
  }

  // Frequency-axis ticks (log scale, matching the analyzer's 20 Hz → Nyquist map).
  const logRange = Math.log(nyquistHz / 20);
  ctx.strokeStyle = "rgba(139,115,85,0.22)";
  ctx.fillStyle = "rgba(210,190,150,0.7)";
  ctx.font = "9px Garamond, 'Times New Roman', serif";
  ctx.textAlign = "center";
  for (const f of FREQ_TICKS) {
    if (f >= nyquistHz) continue;
    const x = (Math.log(f / 20) / logRange) * w;
    ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, h - 11); ctx.stroke();
    ctx.fillText(fmtTickHz(f), x, h - 2);
  }

  drawSidebandMarkers(ctx, w, h, logRange);
}

// ── FM sideband markers (predicted f_c ± k·f_m) ────────────────────────────
// Teaching overlay: the discrete FM spectrum lands at the carrier and at
// f_c ± k·f_m. Drawing the predicted positions lets the eye confirm that the
// live peaks sit exactly where Chowning's math says. f_m mirrors the DSP
// (FMVoice): the fixed Hz in Fixed Mode, else f_c·ratio (snap-rounded). Carrier
// (k = 0) is coloured distinctly from the sidebands. Skipped when no note sounds.
function drawSidebandMarkers(ctx, w, h, logRange) {
  if (carrierHz <= 20) return;

  let ratioVal = sliderState.ratio ? sliderState.ratio.getScaledValue() : 1;
  if (toggleState.ratioSnap && toggleState.ratioSnap.getValue()) ratioVal = Math.round(ratioVal);
  ratioVal = Math.max(0.001, ratioVal);

  const fixedOn = toggleState.modFixedMode && toggleState.modFixedMode.getValue();
  const fm = fixedOn
    ? (sliderState.modFixedHz ? sliderState.modFixedHz.getScaledValue() : 0)
    : carrierHz * ratioVal;

  const freqToX = (f) => (Math.log(f / 20) / logRange) * w;
  const top = 0, bottom = h - 11;

  // Sidebands first (under the carrier): faint dashed sage ticks, k = 1..8.
  if (fm > 0) {
    ctx.strokeStyle = "rgba(158, 196, 111, 0.42)";
    ctx.lineWidth = 1;
    ctx.setLineDash([3, 3]);
    ctx.beginPath();
    for (let k = 1; k <= 8; k++) {
      for (const f of [carrierHz - k * fm, carrierHz + k * fm]) {
        if (f <= 20 || f >= nyquistHz) continue;
        const x = freqToX(f);
        ctx.moveTo(x, top); ctx.lineTo(x, bottom);
      }
    }
    ctx.stroke();
    ctx.setLineDash([]);
  }

  // Carrier (k = 0): solid amber, labelled — distinct from the sidebands.
  if (carrierHz < nyquistHz) {
    const xc = freqToX(carrierHz);
    ctx.strokeStyle = "rgba(232, 176, 74, 0.9)";
    ctx.lineWidth = 1.5;
    ctx.beginPath(); ctx.moveTo(xc, top); ctx.lineTo(xc, bottom); ctx.stroke();
    ctx.fillStyle = "rgba(236, 188, 104, 0.95)";
    ctx.font = "9px Garamond, 'Times New Roman', serif";
    ctx.textAlign = "center";
    ctx.fillText("fc", xc, 9);   // carrier (played note); sidebands sit at fc ± k·fm
  }
}

function drawScope(arr) {
  lastScope = arr;
  if (!scope) return;
  const { canvas, ctx } = scope;
  const w = canvas.clientWidth, h = canvas.clientHeight;
  ctx.clearRect(0, 0, w, h);

  // centre line
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
  // The spectrum/scope are pushed from C++ via emitEventIfBrowserIsVisible.
  // These arrive on the low-level backend (window.__JUCE__.backend), NOT Juce.*.
  if (window.__JUCE__ && window.__JUCE__.backend) {
    // carrierUpdate is emitted just BEFORE spectrumUpdate each frame, so storing
    // it here lands in time for the same frame's sideband markers.
    window.__JUCE__.backend.addEventListener("carrierUpdate", (hz) => {
      carrierHz = (typeof hz === "number" && hz > 0) ? hz : 0;
    });
    window.__JUCE__.backend.addEventListener("spectrumUpdate", (arr) => drawSpectrum(arr));
    window.__JUCE__.backend.addEventListener("scopeUpdate", (arr) => drawScope(arr));
    // C++ pushes the rate whenever it changes (host rate switch, first
    // prepareToPlay after the editor opened) — keeps the log-frequency axis
    // and sideband markers on the live Nyquist.
    window.__JUCE__.backend.addEventListener("sampleRateUpdate", (sr) => {
      if (typeof sr === "number" && sr > 0 && sr / 2 !== nyquistHz) {
        nyquistHz = sr / 2;
        if (lastSpectrum) drawSpectrum(lastSpectrum);
      }
    });
  } else {
    console.error("window.__JUCE__.backend unavailable — viz events will not arrive.");
  }
}

// Single resize handler: re-fit both canvas backing stores, then redraw the
// last frame (preserves the visible image across an editor resize).
function rewireResize() {
  window.addEventListener("resize", () => {
    if (spec) spec.resize();
    if (scope) scope.resize();
    if (lastSpectrum) drawSpectrum(lastSpectrum);
    if (lastScope) drawScope(lastScope);
  });
}

// ── On-screen keyboard (play without external MIDI) ──────────────────────────
// Notes are injected into the synth via the C++ `uiMidi` native function (queued
// through a MidiMessageCollector, merged into processBlock). Mouse is monophonic
// with glide; the computer keyboard is polyphonic over one octave (C4–C5).
const KB_LOW = 48, KB_HIGH = 72;              // C3 … C5 inclusive
const BLACK_OFFSETS = new Set([1, 3, 6, 8, 10]);
const QWERTY = { a: 60, w: 61, s: 62, e: 63, d: 64, f: 65, t: 66, g: 67, y: 68, h: 69, u: 70, j: 71, k: 72 };

let uiMidiFn = null;
const keyEls = {};                  // midi note -> key element
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
    else kb.appendChild(el);          // white keys flow in the flex row
  }
  blacks.forEach((el) => kb.appendChild(el));   // black keys overlay (absolute)

  // Position each black key straddling the boundary of the white key below it.
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

  // Mouse / touch — monophonic with glide while the button is held.
  let pointerNote = null;
  const noteAt = (target) => {
    const k = target.closest ? target.closest(".key") : null;
    return k ? +k.dataset.note : null;
  };
  const releasePointerNote = () => {
    if (pointerNote != null) { noteOff(pointerNote); pointerNote = null; }
  };

  // Pointer capture guarantees pointerup/pointercancel reach us even when the
  // mouse is released outside the plugin window (no capture = stuck note).
  // Captured events all target `kb`, so glide tracks via elementFromPoint
  // instead of pointerover.
  kb.addEventListener("pointerdown", (e) => {
    const n = noteAt(e.target);
    if (n == null) return;
    kb.setPointerCapture(e.pointerId);
    pointerNote = n;
    noteOn(n);
    e.preventDefault();
  });
  kb.addEventListener("pointermove", (e) => {
    if (pointerNote == null) return;
    const under = document.elementFromPoint(e.clientX, e.clientY);
    const n = under ? noteAt(under) : null;
    if (n != null && n !== pointerNote) { noteOff(pointerNote); noteOn(n); pointerNote = n; }
  });
  kb.addEventListener("pointerup", releasePointerNote);
  kb.addEventListener("pointercancel", releasePointerNote);

  // A held QWERTY note's keyup is lost if the WebView loses focus mid-hold
  // (click into the DAW, Cmd-Tab) — sweep everything on blur.
  const allNotesOff = () => {
    [...heldNotes].forEach(noteOff);
    pointerNote = null;
  };
  window.addEventListener("blur", allNotesOff);

  // Computer keyboard — polyphonic, auto-repeat suppressed. Ignored while
  // focus sits on an interactive control (preset bar/dropdown, any input):
  // typing there must not trigger notes.
  window.addEventListener("keydown", (e) => {
    if (e.repeat || e.metaKey || e.ctrlKey || e.altKey) return;
    const t = e.target;
    if (t && t.closest && t.closest("input, textarea, [contenteditable], .preset-bar, .preset-dropdown")) return;
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

// Pull the host sample rate for the spectrum frequency-axis labels.
async function fetchSampleRate() {
  try {
    const sr = await Juce.getNativeFunction("getSampleRate")();
    if (sr > 0) nyquistHz = sr / 2;
    if (lastSpectrum) drawSpectrum(lastSpectrum);   // relabel once we know the real rate
  } catch (e) { /* keep the default */ }
}

// Pull { paramID: normalisedDefault } for double-click-reset on the knobs.
async function fetchParameterDefaults() {
  try {
    const d = await Juce.getNativeFunction("getParameterDefaults")();
    if (d && typeof d === "object") paramDefaults = d;
  } catch (e) { /* dblclick-reset stays inert */ }
}

// ── Boot ────────────────────────────────────────────────────────────────────
function boot() {
  spec = makeCanvas("spectrumCanvas");
  scope = makeCanvas("scopeCanvas");

  KNOB_IDS.forEach(bindKnob);
  TOGGLE_IDS.forEach(bindToggle);

  // Routing diagram tracks the three relevant params.
  ["ratio", "modIndex", "feedback"].forEach((id) => {
    const st = sliderState[id];
    if (st) { st.valueChangedEvent.addListener(updateRouting); st.propertiesChangedEvent.addListener(updateRouting); }
  });
  // updateRouting() writes the routing meta line through setLabel, so it must
  // run before initI18n() rather than after: the sweep then re-renders it with
  // the data-i18n-vars the call stored, and nothing paints twice.
  updateRouting();

  // The popover and the language sweep go here, EACH IN ITS OWN try/catch.
  // applyI18n is what writes data-tip onto every anchor and what names the
  // fifteen knobs; setupTooltips below reads that attribute, but delegation
  // means the order between the two is free. A translation-table typo must not
  // be allowed to take the seventeen parameter bindings, the two canvases and
  // the keyboard down with it — that is the v1.4.0 TDZ failure this repo has
  // already paid for once.
  try { setupSettingsPopover(); } catch (e) { console.error("settings popover init failed:", e); }
  try { initI18n(); }             catch (e) { console.error("i18n init failed:", e); }

  setupTooltips();
  setupPresets();
  setupPresetManager();
  setupKeyboard();
  setupVizEvents();
  rewireResize();
  fetchSampleRate();
  fetchParameterDefaults();

  // initial empty frames so the canvases aren't black
  drawSpectrum(new Array(256).fill(-100));
  drawScope(new Array(128).fill(0));
}

if (document.readyState === "loading")
  document.addEventListener("DOMContentLoaded", boot);
else
  boot();
