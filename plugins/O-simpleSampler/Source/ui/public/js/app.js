/*
   This file is part of O-simpleSampler, an Ouaricon Audio plugin.
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
// O-simpleSampler — WebView UI controller (Phase 3.1)
// Binds all 20 APVTS params two-way (17 sliders + 2 combos + 1 toggle), wires the
// source drag-drop + "Load…" picker + the >30 s truncation notice, and the
// on-screen keyboard. The four canvases (waveform editor / filter curve /
// amp-ADSR / scope) are RESERVED in the DOM — their rendering + the viz push-event
// subscriptions land in Phase 3.2 (this file leaves them blank).
//
// NOTE: getSliderState / getComboBoxState / getToggleState / getNativeFunction
// live on the `Juce` ES-module namespace — NOT the low-level postMessage handler.
// Pass `Juce` to anything that calls getNativeFunction (silent-TypeError trap).
// String param IDs are the contract: the C++ identifiers regionStart/regionEnd
// carry the string IDs "start"/"end" (the juce::end shadow fix), so the DOM /
// relays / JS all use the STRING ids ("start","end").
// ============================================================================

import * as Juce from "./juce/index.js";
import { readFileEntryAsBase64 } from "./modules/webview-drop-streaming.js";

// ═══════════════════════════════════════════════════════════════════════════
// INTERFACE LANGUAGE (v1.4.0) — canon v2, verbatim from scripts/i18n-canon.js.
//
// It sits HERE, directly under the imports and above every other declaration in
// this module, because `uiLanguage` is a module-level `let`: anything above it
// that reached applyLabel() would touch it inside its temporal dead zone and
// throw out of module evaluation, taking the 20 parameter bindings, the four
// canvases and the keyboard with it (pattern_module_toplevel_init_tdz). This
// file DOES have eager top-level work below — KNOB_IDS, FORMAT, the formatters
// — so the ordering is load-bearing here, not merely defensive.
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
            .then((code) => applyI18n(code === 'fr' ? 'fr' : 'en'))
            .catch((e) => console.warn('Could not read language preference:', e));
    }

    const sel = document.getElementById('lang-select');
    if (sel) sel.addEventListener('change', (e) => {
        applyI18n(e.target.value);
        if (setUiLanguageNative) setUiLanguageNative(uiLanguage).catch(() => {});
    });
}

// ── Parameter inventory (must match OSimpleSampler::ParamIDs string IDs) ─────
const KNOB_IDS = [
  "start", "end", "loopStart", "loopEnd", "loopCrossfade",
  "rootKey", "tune", "fine",
  "vintage",
  "filterCutoff", "filterResonance",
  "ampAttack", "ampDecay", "ampSustain", "ampRelease", "velToAmp",
  "outputLevel",
];
const COMBO_IDS = ["loopMode", "pitchMode"];

// The single embedded source the plugin starts on. Mirrors kBuiltInNames[0] in
// PluginProcessor.h — the UI has no native fn to query the live source identity,
// so this is the one place the name is written on the JS side.
const BUILT_IN_SOURCE_NAME = "piano";
const TOGGLE_IDS = ["reverse"];

// ── Display formatters (keyed by string id) — receive the *scaled* value ────
const fmtSec = (s) => (s < 1 ? `${Math.round(s * 1000)} ms` : `${s.toFixed(2)} s`);
const fmtPct = (v) => `${Math.round(v)}%`;            // value already 0..100
const fmtPct01 = (v) => `${Math.round(v * 100)}%`;    // value 0..1

// MIDI note number → note name. This plugin labels middle C (60) as C3
// (kRootNote = 60 = C3), so octave = floor(n/12) − 2.
const NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
const fmtNote = (n) => {
  const m = Math.round(n);
  return `${NOTE_NAMES[((m % 12) + 12) % 12]}${Math.floor(m / 12) - 2}`;
};
const fmtSt = (v) => `${v >= 0 ? "+" : ""}${Math.round(v)} st`;
const fmtCents = (v) => `${v >= 0 ? "+" : ""}${Math.round(v)} c`;
const fmtHz = (v) => (v >= 1000 ? `${(v / 1000).toFixed(1)} kHz` : `${Math.round(v)} Hz`);
const fmtDb = (v) => (v <= -59.5 ? "-inf" : `${v.toFixed(1)} dB`);

const FORMAT = {
  start: fmtPct, end: fmtPct, loopStart: fmtPct, loopEnd: fmtPct,
  loopCrossfade: (v) => `${Math.round(v)} ms`,
  rootKey: fmtNote, tune: fmtSt, fine: fmtCents,
  vintage: fmtPct,
  filterCutoff: fmtHz, filterResonance: fmtPct,
  ampAttack: fmtSec, ampDecay: fmtSec, ampRelease: fmtSec,
  ampSustain: fmtPct01,
  velToAmp: fmtPct,
  outputLevel: fmtDb,
};

// ── Tooltip copy ────────────────────────────────────────────────────────────
// MOVED to js/i18n.js at v1.4.0. Through v1.3.1 the copy lived in a `TIPS`
// object here and each anchor carried the KEY in its own data-tip attribute.
// applyI18n now WRITES data-tip (the body) and data-tip-title on every anchor
// named by TIP_BINDINGS, so the renderer below reads the attributes rather than
// a table — one code path, and no way for a tip to be stranded in the previous
// language after the selector fires.
//
// The bodies used to carry `<em>` / `<strong>` / `<code>` because the renderer
// dropped them into innerHTML. The renderer builds the tip with createElement +
// textContent now, so the tags are gone and the words are unchanged; that
// tag-free form is the string v1.3.1 already installed as the native `title=`
// fallback, so nothing a user has read has changed.

// ── Knob geometry ────────────────────────────────────────────────────────────
const KNOB_MIN_DEG = -135;   // 0.0 normalised
const KNOB_MAX_DEG = 135;    // 1.0 normalised
const DRAG_TRAVEL_PX = 220;  // vertical px for a full 0..1 sweep

const sliderState = {};   // id -> Juce SliderState
const comboState  = {};   // id -> Juce ComboBoxState
const toggleState = {};   // id -> Juce ToggleState

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

// One-shot fine adjust shared by wheel + arrow-key (a full bracketed gesture).
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
}

// ── Combo box binding (loopMode, pitchMode) ─────────────────────────────────
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
  // ComboBox choices may not be present on first load — listen for both.
  st.propertiesChangedEvent.addListener(refresh);
  st.valueChangedEvent.addListener(refresh);
  refresh();

  sel.addEventListener("change", () => st.setChoiceIndex(sel.selectedIndex));
}

// ── Toggle binding (reverse) ─────────────────────────────────────────────────
function bindToggle(id) {
  const st = Juce.getToggleState(id);
  toggleState[id] = st;
  const el = document.getElementById(`toggle-${id}`);
  if (!el) { console.error(`Missing toggle element: toggle-${id}`); return; }

  const refresh = () => el.classList.toggle("active", st.getValue());
  st.valueChangedEvent.addListener(refresh);
  st.propertiesChangedEvent.addListener(refresh);
  refresh();

  el.addEventListener("click", () => { st.setValue(!st.getValue()); refresh(); });
}

// ── Toast + source status ────────────────────────────────────────────────────
// Both were `el.textContent = "some English sentence"` through v1.3.1. They are
// setLabel() writers now, so each element becomes a [data-i18n] element the
// moment it is written and the language sweep owns it from then on. A raw
// string would be stranded in whichever language the drop happened in — and a
// drop is exactly the kind of event that outlives a language switch, because
// the toast and the status line both persist on screen.
//
// The key is a PLAIN STRING LITERAL at every call site. A `showToast(key)`
// forwarding wrapper would put a variable in setLabel's key position, which
// check-i18n assertion 13 rejects: a computed key cannot be checked against the
// table. So the element lookup and the flash are the two helpers, and the copy
// is named where it is chosen.
let toastTimer = null;
const toastEl  = () => document.getElementById("toast");
const statusEl = () => document.getElementById("sourceStatus");

function flashToast() {
  const t = toastEl();
  if (!t) return;
  t.classList.add("show");
  if (toastTimer) clearTimeout(toastTimer);
  toastTimer = setTimeout(() => t.classList.remove("show"), 2600);
}

function markStatusTruncated(truncated) {
  const el = statusEl();
  if (el) el.classList.toggle("truncated", !!truncated);
}

// ── After a load completes, surface the 30 s truncation notice ──────────────
// TWO reporters, not one with a name argument. The drop path knows the filename;
// the picker path never learns it, because the C++ FileChooser is async and
// nothing reports the chosen file back to the page. v1.3.1 papered over that by
// passing the English word "Source" as the name, which would have shipped an
// English word inside a French sentence — and routing that word through the
// table as a var VALUE instead makes its key invisible to check-i18n
// assertion 15, which counts a key as referenced only where it is a literal in a
// setLabel call. Two literal-keyed writers, no indirection.
async function isLastLoadTruncated() {
  try { return !!(await Juce.getNativeFunction("wasLastLoadTruncated")()); }
  catch (e) { return false; }
}

async function reportLoadedFile(name) {
  const trunc = await isLastLoadTruncated();
  if (trunc) setLabel(statusEl(), "label.sourceTruncated", { name: name });
  else       setLabel(statusEl(), "label.sourceLoaded",    { name: name });
  markStatusTruncated(trunc);
}

async function reportPickedSource() {
  const trunc = await isLastLoadTruncated();
  if (trunc) setLabel(statusEl(), "label.sourceTruncatedGeneric");
  else       setLabel(statusEl(), "label.sourceLoadedGeneric");
  markStatusTruncated(trunc);
}

// ── Single-source drag-drop (content-streaming, macOS WKWebView-safe) ───────
// Uses webkitGetAsEntry() + readFileEntryAsBase64 (shared module) and drives the
// sampler's three drop native fns directly. All getNativeFunction calls go through
// the `Juce` ES-module namespace. The base64 is standard btoa() output; the C++
// side decodes it with juce::Base64::convertFromBase64.
function newSessionId() {
  return `s${Date.now()}-${Math.random().toString(36).slice(2, 8)}`;
}

async function commitDroppedFile(fileEntry) {
  const sessionId = newSessionId();
  setLabel(toastEl(), "toast.loading", { name: fileEntry.name });
  flashToast();
  try {
    const startOk = await Juce.getNativeFunction("dropSampleStart")(sessionId, fileEntry.name);
    if (!startOk) { setLabel(toastEl(), "toast.dropStartFailed"); flashToast(); return; }

    const base64 = await readFileEntryAsBase64(fileEntry);

    const chunkOk = await Juce.getNativeFunction("dropSampleChunk")(sessionId, fileEntry.name, base64);
    if (!chunkOk) { setLabel(toastEl(), "toast.transferFailed"); flashToast(); return; }

    const commitOk = await Juce.getNativeFunction("dropSampleCommit")(sessionId, fileEntry.name, "");
    if (!commitOk) { setLabel(toastEl(), "toast.commitFailed"); flashToast(); return; }

    await reportLoadedFile(fileEntry.name);
    fetchSourceThumbnail();   // new source published → refresh the waveform editor
  } catch (e) {
    console.error("[O-simpleSampler] drop failed:", e);
    // The message is picked BEFORE the call. check-i18n assertion 13 rejects a
    // conditional anywhere inside a setLabel call, not only in the key position,
    // and it is right to: a ternary in a localized argument is where inflection
    // logic creeps back in, and a reviewer cannot tell a message-selection
    // ternary from a plural one by reading the call.
    const detail = (e && e.message) ? e.message : String(e);
    setLabel(toastEl(), "toast.dropFailed", { error: detail });
    flashToast();
  }
}

function bindSourceDrop() {
  const zone = document.getElementById("source-drop-zone");
  const audioRe = /\.(wav|aif|aiff|flac)$/i;

  const hover = (on) => { if (zone) zone.classList.toggle("drop-hover", on); };

  document.addEventListener("dragenter", (e) => { e.preventDefault(); hover(true); });
  document.addEventListener("dragover", (e) => {
    e.preventDefault();
    try { e.dataTransfer.dropEffect = "copy"; } catch (_) { /* read-only in some hosts */ }
  });
  document.addEventListener("dragleave", (e) => { if (e.relatedTarget === null) hover(false); });

  document.addEventListener("drop", async (e) => {
    e.preventDefault();
    hover(false);
    if (!window.__JUCE__) return;

    const items = Array.from(e.dataTransfer?.items || []);
    const entry = items.length > 0 && typeof items[0].webkitGetAsEntry === "function"
      ? items[0].webkitGetAsEntry()
      : null;
    if (!entry) return;

    if (entry.isDirectory) { setLabel(toastEl(), "toast.dropFolder"); flashToast(); return; }
    if (!audioRe.test(entry.name)) { setLabel(toastEl(), "toast.dropFileType"); flashToast(); return; }
    await commitDroppedFile(entry);
  });
}

// ── "Load…" picker (always works where drag-drop doesn't) ───────────────────
function bindLoadButton() {
  const btn = document.getElementById("btnLoad");
  if (!btn) return;
  btn.addEventListener("click", async () => {
    try {
      await Juce.getNativeFunction("loadSourceFromFileChooser")();
      // The picker is async on the C++ side; report truncation + refresh the
      // waveform after a beat so the decode has a chance to finish. Best-effort.
      setTimeout(() => { reportPickedSource(); fetchSourceThumbnail(); }, 1200);
    } catch (e) {
      console.error("[O-simpleSampler] Load… failed:", e);
      setLabel(toastEl(), "toast.loadFailed");
      flashToast();
    }
  });
}

// ── Concept-preset tour (FUNC-07) ────────────────────────────────────────────
// Each button forwards its data-preset label to the C++ applyFactoryPreset native
// fn (a full-APVTS snapshot); the relays/attachments then sync every knob / combo
// / toggle back to the page automatically — no DOM poking. We only set the caption
// + the active-button highlight here. data-preset MUST match the C++ preset names.
// NB: the parameter VALUES inside each preset are authored in Stage 4 (the hook is
// live now: clicking resets to defaults + resyncs the UI, proving the round-trip).
// Use the `Juce` ES-module namespace for getNativeFunction (window.__JUCE__ has no
// such method — a silent TypeError otherwise eats the call and the button is dead).
// Through v1.3.1 a PRESET_LESSONS table here held each caption as one English
// string. Both the caption and the button tooltip are authored table entries in
// js/i18n.js now: a caption chosen by a click and written as a raw string is
// stranded in the language it was picked in the instant the selector fires, and
// it is the one string on this page a user changes deliberately.
//
// What is left is a dispatch from the C++ preset name — which is also the
// button's data-preset, and is never localized — to a WRITER that names its
// caption key as a plain string literal.
//
// `setLabel(cap, KEYS[name] || "label.tourCaption")` reads more naturally and is
// what this was first written as. check-i18n assertion 13 rejects it twice over,
// correctly: a computed key cannot be checked against the table, and the `||` is
// the conditional-inside-a-localized-string shape contract §6 forbids. Seven
// call sites, seven literals, and assertion 15 can see that all seven keys are
// live.
const LESSON_CAPTION_WRITERS = {
  "Raw One-Shot":              (el) => setLabel(el, "label.captionRawOneShot"),
  "Tuned Across the Keyboard": (el) => setLabel(el, "label.captionTuned"),
  "Looped Pad":                (el) => setLabel(el, "label.captionLoopedPad"),
  "Reversed Swell":            (el) => setLabel(el, "label.captionReversed"),
  "Repitch vs Stretch A/B":    (el) => setLabel(el, "label.captionRepitchStretch"),
  "SP-1200 Crunch":            (el) => setLabel(el, "label.captionSp1200"),
  "Filtered & Enveloped":      (el) => setLabel(el, "label.captionFiltered"),
};

let applyPresetFn = null;

async function applyPreset(name) {
  if (applyPresetFn) {
    try { await applyPresetFn(name); }
    catch (e) { console.error("[O-simpleSampler] applyFactoryPreset failed:", e); }
  }
  // setLabel, not textContent: the caption becomes a [data-i18n] element from
  // this moment on, so the language sweep owns it. An unknown name falls back to
  // the RESTING caption rather than to the raw preset name, so a C++/JS name
  // drift shows as the default instruction instead of an untranslated word.
  const cap = document.getElementById("tourCaption");
  if (cap) {
    const write = LESSON_CAPTION_WRITERS[name];
    if (write) write(cap);
    else setLabel(cap, "label.tourCaption");
  }
  document.querySelectorAll(".tour-btn").forEach((b) =>
    b.classList.toggle("active", b.getAttribute("data-preset") === name));
}

function setupPresets() {
  try { applyPresetFn = Juce.getNativeFunction("applyFactoryPreset"); }
  catch (e) { applyPresetFn = null; }
  document.querySelectorAll(".tour-btn").forEach((btn) => {
    btn.addEventListener("click", () => applyPreset(btn.getAttribute("data-preset")));
  });
}

// ── Tooltips ─────────────────────────────────────────────────────────────────
// ONE hover surface, not two. Through v1.3.1 this layer ALSO installed a native
// `title=` on every [data-tip] element as a plain-text fallback, and the "?"
// toggle had to strip those titles as well or the OS tooltip kept popping up
// with the explanations switched off. Contract §4 deletes the native title
// outright: on an element that already has a rich tip it is a second,
// untranslated OS tooltip competing with the measure-then-pin renderer.
//
// Deleting it also deletes the save/restore path the toggle needed. That path
// parked the authored English in data-tip-title and put it back verbatim when
// tips came on again — and applyI18n WRITES data-tip-title on every sweep, so a
// restore of the parked copy would have resurrected English text after a French
// switch. The bug never shipped because the layer it belonged to is gone.
let tipsEnabled = true;                 // shipped default; the session state wins at boot
let hideTooltip = () => {};             // published by setupTooltips (used by the toggle)

function applyTipsEnabled(on) {
  tipsEnabled = !!on;
  if (!tipsEnabled) hideTooltip();

  const btn = document.getElementById("help-toggle");
  if (!btn) return;
  btn.setAttribute("aria-pressed", tipsEnabled ? "true" : "false");
  // Two calls behind an if/else, never a ternary in the setLabel argument
  // (check-i18n assertion 13). The face is a [data-i18n] element from here on,
  // so the language sweep owns whichever of the two is showing.
  if (tipsEnabled) setLabel(btn, "ui.on");
  else             setLabel(btn, "ui.off");
}

function setupTooltips() {
  const tip = document.getElementById("tooltip");
  if (!tip) return;
  let active = null;

  // The copy is read from the anchor's OWN attributes, which applyI18n rewrote
  // in the current language. v1.3.1 looked it up in a TIPS object keyed by the
  // anchor's data-tip; that table is gone, and with it the second code path
  // that would have gone stale on a language switch.
  //
  // Built with createElement + textContent, not innerHTML. The tip text is now
  // table-sourced rather than a fixed literal, and localized copy must never
  // reach a markup path.
  const show = (el, x, y) => {
    if (!tipsEnabled) return;
    const title = el.getAttribute("data-tip-title");
    const body  = el.getAttribute("data-tip");
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
  // carries data-tip until applyI18n has run, so a querySelectorAll at setup
  // time would bind nothing at all — the anchors hold data-param / an id / a
  // data-preset now, and the copy arrives on the first sweep. Delegation has no
  // ordering to get wrong. pointerover/pointerout and focusin/focusout are used
  // because — unlike pointerenter/pointerleave and focus/blur — they bubble.
  const anchorOf = (t) => (t && t.closest ? t.closest("[data-tip]") : null);

  // FOCUS LATCH (v1.4.3, Stage O item 58 — the Stage M lastInputWasPointer
  // latch, O-Comp v1.7.0 setupTooltips). A pointer click on a focusable anchor
  // (#toggle-reverse, #btnLoad, the two combos, the knobs are tabindex=0 too)
  // used to fire pointerdown -> hide, then focus -> focusin -> show: the tip
  // came straight back under the pointer. Keyboard focus is the accessibility
  // half and keeps its tip. :focus-visible is deliberately NOT the
  // discriminator — Chromium reports it false for a programmatic .focus() that
  // follows a click, so a gate driving focus directly would measure "no tip"
  // and record the false pass. The latch is drivable with real events:
  // pointerdown latches, ANY keydown releases, focusin opens only while
  // released. The page's one programmatic .focus() — gearBtn.focus() in
  // setupSettingsPopover's Escape handler, registered BEFORE this listener —
  // is covered by construction: a popover opened by pointer closes on Escape
  // while still latched (no tip), one opened by keyboard gets its tip hidden
  // by this listener's own Escape branch a tick later.
  let lastInputWasPointer = false;

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
  document.addEventListener("pointerdown", () => { lastInputWasPointer = true; hide(); });

  document.addEventListener("focusin", (e) => {
    if (lastInputWasPointer) return;
    const el = anchorOf(e.target);
    if (!el) return;
    active = el;
    const r = el.getBoundingClientRect();
    show(el, r.left + r.width / 2, r.bottom);
  });
  document.addEventListener("focusout", hide);

  // One keydown listener, two jobs: any key means the keyboard is driving
  // again, which releases the latch; Escape additionally hides.
  document.addEventListener("keydown", (e) => {
    lastInputWasPointer = false;
    if (e.key === "Escape") hide();
  });

  hideTooltip = hide;   // the toggle needs to retract a tip that is already open
}

// ── Hover-help toggle (inside the settings popover) ──────────────────────────
// The flag is NOT an APVTS parameter — it is a UI preference living in the
// custom <UI tipsEnabled="…"/> child of the saved state tree, alongside
// <SOURCE>. That keeps the automatable parameter count at 20 and, just as
// importantly, keeps the seven concept presets from resetting the user's choice
// every time one is clicked (applyFactoryPreset writes parameters only).
//
// The C++ bridge is unchanged from v1.3.0: getTipsEnabled / setTipsEnabled and
// the tipsEnabledChanged push. Only the button moved — from a lone "?" chip in
// the header into the gear panel beside the language selector.
function setupHelpToggle() {
  const btn = document.getElementById("help-toggle");
  if (!btn) { console.error("Missing help-toggle element"); return; }

  let setTipsFn = null;
  try { setTipsFn = Juce.getNativeFunction("setTipsEnabled"); }
  catch (e) { setTipsFn = null; }

  btn.addEventListener("click", () => {
    applyTipsEnabled(!tipsEnabled);
    if (setTipsFn) setTipsFn(tipsEnabled);
  });

  // Adopt the persisted value: a restored session must not come back with the
  // explanations switched on again.
  (async () => {
    try {
      const v = await Juce.getNativeFunction("getTipsEnabled")();
      applyTipsEnabled(Array.isArray(v) ? !!v[0] : !!v);
    } catch (e) {
      applyTipsEnabled(true);
    }
  })();

  // A host can restore state UNDER an already-open editor; the C++ Timer pushes
  // this on a real edge only (not at 30 Hz).
  const be = window.__JUCE__ && window.__JUCE__.backend;
  if (be) be.addEventListener("tipsEnabledChanged",
                              (v) => applyTipsEnabled(Array.isArray(v) ? !!v[0] : !!v));
}

// ── Settings popover (v1.4.0) ────────────────────────────────────────────────
// The gear panel holding the language selector and the hover-help toggle. All
// state lives in this closure, so nothing here can join a TDZ chain.
//
// Styled in O-simpleSampler's own aged-paper vocabulary in css/styles.css: the
// panel wears the .group plate the seven rack groups wear, the selector is the
// .combo the two drop-downs already wear at panel scale, and the lit face is
// the same --green-mid that .tour-btn.active and the old "?" chip used. It is
// not a widget pasted in unchanged from another plugin.
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
  // the knobs call preventDefault in their own pointerdown handlers.
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

// ── On-screen keyboard (play without external MIDI) ──────────────────────────
const KB_LOW = 48, KB_HIGH = 72;              // C2 … C4 inclusive (Yamaha labelling)
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
  // uiMidi bridges keys → the processor's MidiMessageCollector (merged into
  // processBlock). The try/catch keeps the highlight working even if the binding
  // is ever absent. (External host MIDI works regardless.)
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

// ════════════════════════════════════════════════════════════════════════════
// PHASE 3.2 — Interactive waveform editor + viz layer
// ----------------------------------------------------------------------------
// Headline interactive waveform editor (draggable start/end + loop handles +
// shaded loop region + root-key indicator + live playhead) plus three small
// viz cells (filter-response curve, amp-ADSR animation, output scope). All four
// canvases are DPR-aware (crisp on Retina). The C++ editor Timer pushes
// playheadUpdate / filterCurveUpdate / scopeUpdate at 30 Hz via
// emitEventIfBrowserIsVisible. The waveform handles drive the EXISTING
// start/end/loopStart/loopEnd slider relays (no new params — the 21-param APVTS
// contract is frozen). The amp-ADSR shape is reconstructed JS-side from the 4 amp
// params (no per-frame push); its dot is animated off the playhead heartbeat.
// ════════════════════════════════════════════════════════════════════════════

// ── DPR-aware canvas helper (replaced-element-safe; CSS sizes the .canvas-wrap) ─
function makeCanvas(id) {
  const canvas = document.getElementById(id);
  if (!canvas) { console.error(`Missing canvas: ${id}`); return null; }
  const ctx = canvas.getContext("2d");
  const resize = () => {
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.clientWidth, h = canvas.clientHeight;
    canvas.width = Math.max(1, Math.round(w * dpr));
    canvas.height = Math.max(1, Math.round(h * dpr));
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);   // draw in CSS px on a DPR backing store
  };
  resize();
  return { canvas, ctx, resize };
}

let canvases = {};
let sourceThumb = null;     // flat [min,max,…] envelope of the loaded source (in [-1,1])
let nyquistHz = 22050;      // updated from filterCurveUpdate.sr
let lastScope = null;       // 128-pt scope array
let lastFilter = null;      // { cutoffHz, k, sr }

// Playhead (UI-01/02) — published normalized [0,1] over the live [start,end]
// region. The value is 0 (= region start) when nothing sounds, so we only DRAW
// it while a voice is sounding, detected via a movement heartbeat (a moving read
// head = a sounding voice; Repitch advances it pitch-coupled, Stretch ~1×).
let playheadNorm = 0;
let playheadActive = false;
let playheadStill = 0;
let lastPlayheadRaw = -1;

function setupCanvases() {
  canvases = {
    wave:   makeCanvas("waveformCanvas"),
    filter: makeCanvas("filterCanvas"),
    amp:    makeCanvas("ampCanvas"),
    scope:  makeCanvas("scopeCanvas"),
  };

  // Re-fit every backing store on resize, then redraw the current image (no flash).
  window.addEventListener("resize", () => {
    Object.values(canvases).forEach((c) => c && c.resize());
    drawWaveformEditor();
    if (lastFilter) drawFilterCurve(lastFilter);
    drawAmpEnv();
    if (lastScope) drawScope(lastScope);
  });

  // Repaint the waveform when any region/loop/root param OR the loop mode changes
  // (so dragging a knob moves the handle, and host automation moves both).
  const repaintWave = () => drawWaveformEditor();
  ["start", "end", "loopStart", "loopEnd", "rootKey"].forEach((id) => {
    if (sliderState[id]) sliderState[id].valueChangedEvent.addListener(repaintWave);
  });
  if (comboState["loopMode"]) comboState["loopMode"].valueChangedEvent.addListener(repaintWave);

  // Redraw the amp-ADSR shape when any amp param changes (no per-frame push).
  ["ampAttack", "ampDecay", "ampSustain", "ampRelease"].forEach((id) => {
    if (sliderState[id]) sliderState[id].valueChangedEvent.addListener(() => drawAmpEnv());
  });

  bindWaveformEditor();
  drawWaveformEditor();
  drawAmpEnv();
  startAmpAnim();
}

// Fetch the source min/max envelope (on load + at boot, NOT per frame) + repaint.
async function fetchSourceThumbnail() {
  try {
    const env = await Juce.getNativeFunction("getSourceThumbnail")(512);
    sourceThumb = Array.isArray(env) ? env : (env && env.length ? Array.from(env) : null);
  } catch (e) {
    sourceThumb = null;
  }
  drawWaveformEditor();
}

// ── Waveform-editor geometry (normalized helpers) ────────────────────────────
const MARKER_HIT_PX = 8;
const MIN_GAP_NORM = 0.005;   // 0.5 % minimum gap between paired markers

function regionBounds() {
  const s = (sliderState["start"]?.getScaledValue() ?? 0) / 100;     // 0..1 of source
  const e = (sliderState["end"]?.getScaledValue() ?? 100) / 100;
  return { s: Math.max(0, Math.min(1, s)), e: Math.max(0, Math.min(1, e)) };
}
function loopFracs() {
  const ls = (sliderState["loopStart"]?.getScaledValue() ?? 0) / 100;  // 0..1 of region
  const le = (sliderState["loopEnd"]?.getScaledValue() ?? 100) / 100;
  return { ls, le };
}
function loopActive() { return (comboState["loopMode"]?.getChoiceIndex() ?? 0) > 0; }
// Region-fraction (0..1 within [start,end]) → absolute source x.
function regionFracToX(frac, w) {
  const { s, e } = regionBounds();
  return (s + frac * (e - s)) * w;
}

function drawMarker(ctx, x, h, colour, kind) {
  if (!Number.isFinite(x)) return;
  ctx.strokeStyle = colour;
  ctx.fillStyle = colour;
  ctx.lineWidth = kind === "loop" ? 1.5 : 2;
  ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, h); ctx.stroke();
  const s = kind === "loop" ? 4 : 6;            // top handle triangle
  ctx.beginPath();
  ctx.moveTo(x - s, 0); ctx.lineTo(x + s, 0); ctx.lineTo(x, s);
  ctx.closePath(); ctx.fill();
}

function drawWaveformEditor() {
  const c = canvases.wave;
  if (!c) return;
  const { canvas, ctx } = c;
  const w = canvas.clientWidth, h = canvas.clientHeight;
  const mid = h / 2;
  ctx.clearRect(0, 0, w, h);

  const { s, e } = regionBounds();

  // 1. Static source thumbnail (min/max → filled waveform).
  if (sourceThumb && sourceThumb.length >= 2) {
    const pairs = sourceThumb.length / 2;
    ctx.fillStyle = "rgba(210,180,140,0.55)";
    ctx.beginPath();
    for (let p = 0; p < pairs; p++) {
      const x = (p / (pairs - 1)) * w;
      const y = mid - sourceThumb[p * 2 + 1] * (mid - 2);
      if (p === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    }
    for (let p = pairs - 1; p >= 0; p--) {
      const x = (p / (pairs - 1)) * w;
      const y = mid - sourceThumb[p * 2] * (mid - 2);
      ctx.lineTo(x, y);
    }
    ctx.closePath();
    ctx.fill();
  } else {
    ctx.fillStyle = "rgba(210,190,150,0.4)";
    ctx.font = "12px Garamond, 'Times New Roman', serif";
    ctx.textAlign = "center"; ctx.textBaseline = "middle";
    ctx.fillText("drop or load a source to see its waveform", w / 2, mid);
  }

  // centerline
  ctx.strokeStyle = "rgba(201,162,123,0.25)";
  ctx.lineWidth = 1;
  ctx.beginPath(); ctx.moveTo(0, mid); ctx.lineTo(w, mid); ctx.stroke();

  // 2. Dim the excluded head/tail (outside [start,end]) — drawn OVER the wave so
  //    the active region reads brighter than the trimmed parts.
  ctx.fillStyle = "rgba(20,15,10,0.5)";
  if (s > 0) ctx.fillRect(0, 0, s * w, h);
  if (e < 1) ctx.fillRect(e * w, 0, (1 - e) * w, h);

  // 3. Shaded loop region (only when loopMode ≠ Off) + green loop markers.
  if (loopActive() && (e - s) > 1e-4) {
    const { ls, le } = loopFracs();
    const lx0 = regionFracToX(Math.min(ls, le), w);
    const lx1 = regionFracToX(Math.max(ls, le), w);
    ctx.fillStyle = "rgba(107,142,78,0.22)";
    ctx.fillRect(lx0, 0, Math.max(1, lx1 - lx0), h);
    drawMarker(ctx, regionFracToX(ls, w), h, "rgba(60,92,26,0.92)", "loop");
    drawMarker(ctx, regionFracToX(le, w), h, "rgba(60,92,26,0.92)", "loop");
  }

  // 4. Start / end markers (region edges) — gold / red.
  drawMarker(ctx, s * w, h, "rgba(184,134,11,0.95)", "start");
  drawMarker(ctx, e * w, h, "rgba(192,57,43,0.95)", "end");

  // 5. Root-key indicator (Task 15 — static label; click-to-set deferred).
  const rk = sliderState["rootKey"]?.getScaledValue();
  if (rk != null) {
    ctx.fillStyle = "rgba(245,230,211,0.7)";
    ctx.font = "10px Garamond, 'Times New Roman', serif";
    ctx.textAlign = "left"; ctx.textBaseline = "top";
    ctx.fillText(`root ${fmtNote(rk)}`, 6, 5);
  }

  // 6. Live playhead (only while sounding) — map region-norm → source-norm.
  if (playheadActive) {
    const phx = (s + playheadNorm * (e - s)) * w;
    ctx.strokeStyle = "rgba(245,230,211,0.92)";
    ctx.lineWidth = 1.6;
    ctx.beginPath(); ctx.moveTo(phx, 0); ctx.lineTo(phx, h); ctx.stroke();
  }
}

// ── Draggable handles → relays (Task 14) ─────────────────────────────────────
let waveDrag = null;     // 'start' | 'end' | 'loopStart' | 'loopEnd'
let waveDragId = -1;

function waveHitTest(x, w) {
  const { s, e } = regionBounds();
  const cands = [{ id: "start", x: s * w }, { id: "end", x: e * w }];
  if (loopActive() && (e - s) > 1e-4) {
    const { ls, le } = loopFracs();
    cands.push({ id: "loopStart", x: regionFracToX(ls, w) });
    cands.push({ id: "loopEnd",   x: regionFracToX(le, w) });
  }
  let best = null, bestD = MARKER_HIT_PX + 1;
  for (const c of cands) {
    const d = Math.abs(x - c.x);
    if (d <= MARKER_HIT_PX && d < bestD) { best = c.id; bestD = d; }
  }
  return best;
}

function applyWaveDrag(x, w) {
  const sourceFrac = Math.max(0, Math.min(1, x / w));
  const { s, e } = regionBounds();
  const st = sliderState[waveDrag];
  if (!st) return;

  // start/end map 0..1 of source (range 0–100, skew 1 → normalised = scaled/100 =
  // sourceFrac). loopStart/loopEnd map 0..1 of the [start,end] region.
  if (waveDrag === "start") {
    st.setNormalisedValue(Math.max(0, Math.min(sourceFrac, e - MIN_GAP_NORM)));
  } else if (waveDrag === "end") {
    st.setNormalisedValue(Math.min(1, Math.max(sourceFrac, s + MIN_GAP_NORM)));
  } else {
    const span = e - s;
    let frac = span > 1e-4 ? (sourceFrac - s) / span : 0;
    frac = Math.max(0, Math.min(1, frac));
    const { ls, le } = loopFracs();
    if (waveDrag === "loopStart") frac = Math.min(frac, le - MIN_GAP_NORM);
    else                          frac = Math.max(frac, ls + MIN_GAP_NORM);
    st.setNormalisedValue(Math.max(0, Math.min(1, frac)));
  }
  drawWaveformEditor();   // setNormalisedValue updates the local cache synchronously
}

function bindWaveformEditor() {
  const c = canvases.wave;
  if (!c) return;
  const canvas = c.canvas;
  const localX = (e) => {
    const r = canvas.getBoundingClientRect();
    return { x: e.clientX - r.left, w: r.width };
  };

  canvas.addEventListener("pointerdown", (e) => {
    const { x, w } = localX(e);
    const target = waveHitTest(x, w);
    if (!target || !sliderState[target]) return;
    waveDrag = target;
    waveDragId = e.pointerId;
    sliderState[target].sliderDragStarted();
    try { canvas.setPointerCapture(e.pointerId); } catch (_) { /* ignore */ }
    applyWaveDrag(x, w);
    e.preventDefault();
  });
  canvas.addEventListener("pointermove", (e) => {
    if (!waveDrag || e.pointerId !== waveDragId) return;
    const { x, w } = localX(e);
    applyWaveDrag(x, w);
    e.preventDefault();
  });
  const endDrag = (e) => {
    if (e.pointerId !== waveDragId) return;
    if (canvas.hasPointerCapture && canvas.hasPointerCapture(e.pointerId)) {
      try { canvas.releasePointerCapture(e.pointerId); } catch (_) { /* ignore */ }
    }
    if (waveDrag && sliderState[waveDrag]) sliderState[waveDrag].sliderDragEnded();
    waveDrag = null;
    waveDragId = -1;
  };
  canvas.addEventListener("pointerup", endDrag);
  canvas.addEventListener("pointercancel", endDrag);
}

// ── Filter response curve (QUAL-02 by construction) ──────────────────────────
// Closed-form 12 dB/oct LP magnitude — mirrors SubFilterCurve::magnitudeDb
// (type=0 LP, slope=1): g = tan(π·fc/fs), W = tan(π·f/fs)/g,
// |H| = 1/sqrt((1−W²)² + (kW)²). Same g/k the audio thread feeds the running
// StateVariableTPTFilter, so the drawn curve matches what is heard.
function lpMagnitudeDb(f, fc, k, fs) {
  f = Math.max(1, Math.min(0.5 * fs - 1, f));
  fc = Math.max(20, Math.min(0.45 * fs, fc));
  const g = Math.tan(Math.PI * fc / fs);
  const W = Math.tan(Math.PI * f / fs) / g;
  const dd = Math.sqrt((1 - W * W) * (1 - W * W) + (k * W) * (k * W));
  return 20 * Math.log10(1 / dd + 1e-9);
}

const FILT_DB_TOP = 18, FILT_DB_BOT = -48;   // curve display window
function drawFilterCurve(f) {
  lastFilter = f;
  if (f && f.sr > 0) nyquistHz = f.sr / 2;
  const c = canvases.filter;
  if (!c || !f) return;
  const { canvas, ctx } = c;
  const w = canvas.clientWidth, h = canvas.clientHeight;
  ctx.clearRect(0, 0, w, h);

  const fs = f.sr > 0 ? f.sr : 44100;
  const minHz = 20, maxHz = fs * 0.5;
  const logR = Math.log(maxHz / minHz);
  const yFor = (db) => h - ((db - FILT_DB_BOT) / (FILT_DB_TOP - FILT_DB_BOT)) * h;

  // 0 dB grid line.
  ctx.strokeStyle = "rgba(139,115,85,0.18)";
  ctx.lineWidth = 1;
  ctx.beginPath(); ctx.moveTo(0, yFor(0)); ctx.lineTo(w, yFor(0)); ctx.stroke();

  // Cutoff marker.
  const fcClamped = Math.max(minHz, Math.min(maxHz, f.cutoffHz));
  const fcx = (Math.log(fcClamped / minHz) / logR) * w;
  ctx.strokeStyle = "rgba(184,134,11,0.5)";
  ctx.beginPath(); ctx.moveTo(fcx, 0); ctx.lineTo(fcx, h); ctx.stroke();

  // Magnitude curve.
  ctx.strokeStyle = "#9ec46f";
  ctx.lineWidth = 1.8;
  ctx.lineJoin = "round";
  ctx.beginPath();
  const N = 160;
  for (let i = 0; i < N; i++) {
    const frac = i / (N - 1);
    const hz = minHz * Math.pow(maxHz / minHz, frac);
    const db = lpMagnitudeDb(hz, f.cutoffHz, f.k, fs);
    const x = frac * w;
    const y = Math.max(-2, Math.min(h + 2, yFor(db)));
    if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  }
  ctx.stroke();
}

// ── Amp-ADSR shape + animated dot (UI-03) ────────────────────────────────────
// Reconstructed from the 4 amp params (no per-frame push). The dot advances
// A→D→S while a note is active (playhead heartbeat / on-screen keys) and runs the
// release segment when it goes idle. Segment display widths scale with their
// times (sqrt-compressed so 5 s still fits); sustain is a fixed plateau.
const ENV_PAD = 4;
function envLayout() {
  const a = sliderState["ampAttack"]?.getScaledValue() ?? 0;
  const d = sliderState["ampDecay"]?.getScaledValue() ?? 0;
  const s = sliderState["ampSustain"]?.getScaledValue() ?? 1;   // 0..1
  const r = sliderState["ampRelease"]?.getScaledValue() ?? 0;
  const wA = 0.5 + Math.sqrt(a), wD = 0.5 + Math.sqrt(d), wR = 0.5 + Math.sqrt(r), wS = 1.6;
  const tot = wA + wD + wS + wR;
  return { a, d, s, r, fA: wA / tot, fD: wD / tot, fS: wS / tot, fR: wR / tot };
}

let ampAnim = { phase: "idle", t: 0, level: 0, seg: "idle", segFrac: 0, relFrom: 0 };
let ampRAF = 0;
let ampLastTs = 0;

function noteActiveNow() { return heldNotes.size > 0 || playheadActive; }

function advanceAmp(dt) {
  const L = envLayout();
  const active = noteActiveNow();
  const p = ampAnim;

  if (active && (p.phase === "idle" || p.phase === "release")) { p.phase = "attack"; p.t = 0; }
  if (!active && (p.phase === "attack" || p.phase === "decay" || p.phase === "sustain")) {
    p.relFrom = p.level; p.phase = "release"; p.t = 0;
  }

  if (p.phase === "attack") {
    p.t += dt;
    const f = Math.min(1, p.t / Math.max(0.005, L.a));
    p.level = f; p.seg = "attack"; p.segFrac = f;
    if (f >= 1) { p.phase = "decay"; p.t = 0; }
  } else if (p.phase === "decay") {
    p.t += dt;
    const f = Math.min(1, p.t / Math.max(0.005, L.d));
    p.level = 1 - f * (1 - L.s); p.seg = "decay"; p.segFrac = f;
    if (f >= 1) { p.phase = "sustain"; p.t = 0; }
  } else if (p.phase === "sustain") {
    p.level = L.s; p.seg = "sustain"; p.segFrac = 1;
  } else if (p.phase === "release") {
    p.t += dt;
    const f = Math.min(1, p.t / Math.max(0.005, L.r));
    p.level = (p.relFrom || L.s) * (1 - f); p.seg = "release"; p.segFrac = f;
    if (f >= 1) { p.phase = "idle"; p.level = 0; }
  } else {
    p.level = 0; p.seg = "idle";
  }
}

function startAmpAnim() {
  const step = (ts) => {
    const dt = ampLastTs ? Math.min(0.1, (ts - ampLastTs) / 1000) : 0;
    ampLastTs = ts;
    const wasIdle = ampAnim.phase === "idle";
    advanceAmp(dt);
    const isIdle = ampAnim.phase === "idle";
    if (!isIdle || !wasIdle) drawAmpEnv();   // redraw while active + one final clear frame
    ampRAF = requestAnimationFrame(step);
  };
  ampRAF = requestAnimationFrame(step);
}

function drawAmpEnv() {
  const c = canvases.amp;
  if (!c) return;
  const { canvas, ctx } = c;
  const w = canvas.clientWidth, h = canvas.clientHeight;
  ctx.clearRect(0, 0, w, h);

  const L = envLayout();
  const x0 = ENV_PAD, yTop = ENV_PAD, yBot = h - ENV_PAD;
  const cw = (w - ENV_PAD) - x0;
  const yFor = (lvl) => yBot - lvl * (yBot - yTop);
  const xA = x0 + L.fA * cw, xD = xA + L.fD * cw, xS = xD + L.fS * cw, xR = xS + L.fR * cw;

  // Filled envelope + outline.
  const trace = () => {
    ctx.beginPath();
    ctx.moveTo(x0, yBot);
    ctx.lineTo(xA, yFor(1));
    ctx.lineTo(xD, yFor(L.s));
    ctx.lineTo(xS, yFor(L.s));
    ctx.lineTo(xR, yBot);
  };
  ctx.fillStyle = "rgba(107,142,78,0.18)";
  trace(); ctx.closePath(); ctx.fill();
  ctx.strokeStyle = "#6b8e4e";
  ctx.lineWidth = 1.6; ctx.lineJoin = "round";
  trace(); ctx.stroke();

  // Animated dot.
  const p = ampAnim;
  if (p.phase !== "idle") {
    let dx;
    if (p.seg === "attack") dx = x0 + p.segFrac * (xA - x0);
    else if (p.seg === "decay") dx = xA + p.segFrac * (xD - xA);
    else if (p.seg === "sustain") dx = xS;
    else dx = xS + p.segFrac * (xR - xS);
    ctx.fillStyle = "rgba(245,230,211,0.95)";
    ctx.beginPath(); ctx.arc(dx, yFor(p.level), 3, 0, Math.PI * 2); ctx.fill();
  }
}

// ── Output scope (oscilloscope; 128 pts in [-1,1]) ───────────────────────────
function drawScope(arr) {
  lastScope = arr;
  const c = canvases.scope;
  if (!c || !arr) return;
  const { canvas, ctx } = c;
  const w = canvas.clientWidth, h = canvas.clientHeight;
  ctx.clearRect(0, 0, w, h);

  ctx.strokeStyle = "rgba(139,115,85,0.25)";
  ctx.lineWidth = 1;
  ctx.beginPath(); ctx.moveTo(0, h / 2); ctx.lineTo(w, h / 2); ctx.stroke();

  const n = arr.length;
  ctx.strokeStyle = "#9ec46f";
  ctx.lineWidth = 1.8;
  ctx.lineJoin = "round";
  ctx.beginPath();
  for (let i = 0; i < n; i++) {
    const x = (i / (n - 1)) * w;
    const y = h / 2 - arr[i] * (h / 2 - 2);
    if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  }
  ctx.stroke();
}

// ── Live viz event subscriptions (low-level backend, NOT Juce.*) ─────────────
// The C++ editor Timer emits these via emitEventIfBrowserIsVisible; they arrive
// on window.__JUCE__.backend. Param state still flows through the Juce namespace.
function setupVizEvents() {
  const be = window.__JUCE__ && window.__JUCE__.backend;
  if (!be) { console.error("window.__JUCE__.backend unavailable — viz events will not arrive."); return; }

  be.addEventListener("playheadUpdate", (v) => {
    const raw = (typeof v === "number") ? v : (Array.isArray(v) ? v[0] : Number(v));
    if (!Number.isFinite(raw)) return;
    // Movement heartbeat → "sounding"; the value is 0 (= region start) when idle.
    if (lastPlayheadRaw < 0 || Math.abs(raw - lastPlayheadRaw) > 1e-5) {
      playheadActive = true; playheadStill = 0;
    } else if (++playheadStill > 15) {           // ~0.5 s of stillness → idle
      playheadActive = false;
    }
    lastPlayheadRaw = raw;
    playheadNorm = Math.max(0, Math.min(1, raw));
    drawWaveformEditor();
  });

  be.addEventListener("filterCurveUpdate", (f) => drawFilterCurve(f));
  be.addEventListener("scopeUpdate", (a) => drawScope(a));
}

// ── Repitch-vs-Stretch readout (UI-02) ───────────────────────────────────────
// The two faces were a PITCH_MODE_TEXT array indexed by the choice index. They
// are two table entries written through setLabel() now, each named by a plain
// string literal behind an if/else — a `setLabel(el, TEXT[i])` would be a
// computed key (check-i18n assertion 13) and a ternary in the argument would be
// the inflection shape contract §6 forbids. The default face is also authored
// in index.html so the row is never blank without JS.
function setupPitchModeReadout() {
  const el = document.getElementById("pitchModeReadout");
  const st = comboState["pitchMode"];
  if (!el || !st) return;
  const refresh = () => {
    if (st.getChoiceIndex() === 1) setLabel(el, "label.pitchStretch");
    else                           setLabel(el, "label.pitchRepitch");
  };
  st.valueChangedEvent.addListener(refresh);
  st.propertiesChangedEvent.addListener(refresh);
  refresh();
}

// ── Boot ────────────────────────────────────────────────────────────────────
function boot() {
  // The popover and the language sweep go FIRST, EACH IN ITS OWN try/catch. A
  // translation-table typo must not be allowed to take the 20 parameter
  // bindings, the four canvases and the keyboard down with it — that is the
  // v1.4.0 TDZ failure this repo has already paid for once. Everything the
  // sweep touches is authored in index.html, so there is nothing to build
  // first: this page generates no labelled cells at runtime.
  try { setupSettingsPopover(); } catch (e) { console.error("settings popover init failed:", e); }
  try { initI18n(); }             catch (e) { console.error("i18n init failed:", e); }

  KNOB_IDS.forEach(bindKnob);
  COMBO_IDS.forEach(bindCombo);
  TOGGLE_IDS.forEach(bindToggle);

  bindSourceDrop();
  bindLoadButton();
  setupPresets();
  setupTooltips();
  setupHelpToggle();   // MUST follow setupTooltips — applyTipsEnabled retracts an open tip
  setupKeyboard();

  // Phase 3.2 — interactive waveform editor + viz layer.
  setupCanvases();
  setupPitchModeReadout();
  setupVizEvents();
  fetchSourceThumbnail();

  // Seed the source status line. Without a selector in the Source group, nothing
  // else tells the user what is playing until a load completes — the status line
  // is only written from a picker/drop otherwise. A fresh instance therefore has
  // to read as "loaded" rather than blank. Any subsequent load overwrites it.
  // "piano" is the embedded WAV's filename and is exempt (it is also the saved
  // <SOURCE identity>), so it goes in as a var value, not as a key.
  setLabel(statusEl(), "label.sourceBuiltIn", { name: BUILT_IN_SOURCE_NAME });
  markStatusTruncated(false);
}

if (document.readyState === "loading")
  document.addEventListener("DOMContentLoaded", boot);
else
  boot();
