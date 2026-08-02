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

// ── Tooltip copy (UI-04) — plain-language, projector-friendly hover copy keyed
// by data-tip. Shape: key → [title, bodyHtml] (the body is dropped into innerHTML
// after the title span; the title-attr fallback strips the HTML). Covers all 20
// controls + the source drop/load affordances + the four viz cells + the seven
// concept-preset lessons. Teaching tone: what it does, in something you can hear.
const TIPS = {
  // ── Source ──────────────────────────────────────────────────────────────────
  loadSource: ["Load your own",
    "The plugin starts on its built-in recording — everything else on this panel shapes <em>that</em> sound. Press this to open a file picker and sample any <code>.wav</code>&nbsp;/&nbsp;<code>.aif</code>&nbsp;/&nbsp;<code>.flac</code> instead. Anything longer than 30&nbsp;s is trimmed (you'll see a notice). The same controls then play your sound."],
  dropZone: ["Drop a sound here",
    "Drag an audio file straight from your desktop onto this panel to sample it. Try a spoken word, a drum hit, or a field recording — a sampler can play <strong>any</strong> sound across the keyboard. This and the Load&#8230; button are the two ways to change the source."],

  // ── Region ──────────────────────────────────────────────────────────────────
  start: ["Start",
    "Where playback begins in the source. Pull it in to skip silence or a soft front edge so a key press lands right on the sound. Drag the gold marker on the waveform too."],
  end: ["End",
    "Where playback stops. Pull it in to drop a noisy tail or dead air at the end. Start and End together isolate just the useful part of the recording."],
  loopStart: ["Loop Start",
    "The front edge of the repeating section, measured <em>inside</em> the trimmed region. Only matters when Loop&nbsp;Mode is on — it sets where each repeat begins."],
  loopEnd: ["Loop End",
    "The back edge of the repeating section. While you hold a key the sound cycles Loop&nbsp;Start&nbsp;&rarr;&nbsp;Loop&nbsp;End forever, so a short sample can sustain indefinitely."],
  loopCrossfade: ["Loop Crossfade",
    "Blends the loop's end back into its start so the seam doesn't click. 0&nbsp;ms is a hard splice; longer fades smooth a rough loop into a seamless sustain."],
  loopMode: ["Loop Mode",
    "<strong>Off</strong> plays once and stops. <strong>Forward</strong> repeats the loop start&rarr;end. <strong>Ping-Pong</strong> runs it forward then backward — smoother for held pads and textures."],
  reverse: ["Reverse",
    "Plays the sample backwards. Pair it with a slow attack for a rising swell, or use it for whooshes and that distinctive 'sucking-in' tail."],

  // ── Pitch ───────────────────────────────────────────────────────────────────
  rootKey: ["Root Key",
    "The key where the sample plays at its original recorded pitch. Notes above it play higher, notes below play lower — this is what turns one recording into a whole instrument."],
  pitchMode: ["Pitch Mode",
    "The headline A/B. <strong>Repitch</strong> changes speed to change pitch (like speeding up a record — higher is faster). <strong>Stretch</strong> holds the timing and moves pitch on its own."],
  tune: ["Tune",
    "Coarse pitch in whole semitones, &plusmn;24. Use it to drop the whole sample into the key of your song without re-loading anything."],
  fine: ["Fine",
    "Tiny pitch trim in cents (1/100 of a semitone). Tune a sample exactly in, or detune it a hair to thicken a layered sound."],

  // ── Vintage ─────────────────────────────────────────────────────────────────
  vintage: ["Vintage",
    "Old-sampler grit: lowers the sample rate and bit depth to throw away resolution. At&nbsp;0 it's clean; turn it up for crunchy, lo-fi SP-1200 character."],

  // ── Filter ──────────────────────────────────────────────────────────────────
  filterCutoff: ["Filter Cutoff",
    "The brightness control. Wide open lets everything through; lower it to roll off the highs and darken the sound. The curve above shows exactly what gets through."],
  filterResonance: ["Filter Resonance",
    "Boosts the frequencies right at the cutoff, adding a vocal, whistling peak. Push it for a sharper, more synth-like sweep as you move the cutoff."],

  // ── Amplitude envelope ───────────────────────────────────────────────────────
  ampAttack: ["Attack",
    "How long the note takes to fade in after a key press. Short&nbsp;= a sharp hit; long&nbsp;= a slow swell that eases in."],
  ampDecay: ["Decay",
    "After the attack peak, how fast the level falls to the sustain level. This shapes the initial 'thump' before the held part of the note."],
  ampSustain: ["Sustain",
    "The level the note holds at while you keep the key down. 100% holds full volume; lower it so the sound settles back after its attack."],
  ampRelease: ["Release",
    "How long the note takes to fade out after you let go. Short&nbsp;= an abrupt stop; long&nbsp;= a lingering tail that rings on."],
  velToAmp: ["Velocity &rarr; Amp",
    "How much your playing strength (velocity) changes loudness. At&nbsp;0 every note is equal; higher makes soft and hard playing far more expressive."],

  // ── Output ──────────────────────────────────────────────────────────────────
  outputLevel: ["Output Level",
    "The master volume of the plugin, in decibels. Use it to balance against your other tracks; the bottom of the range (&minus;inf) is silent."],

  // ── Viz cells ────────────────────────────────────────────────────────────────
  vizWaveform: ["Waveform Editor",
    "A picture of the loaded sound over time. Drag the gold and red edges to trim the region, the green handles to set the loop, and watch the white playhead track where the sample is being read."],
  vizFilter: ["Filter Response",
    "The filter's actual frequency shape. It shows what passes through — falling away past the cutoff, with a peak when resonance is up. This curve <em>is</em> what you hear."],
  vizAmp: ["Envelope Display",
    "The Attack&ndash;Decay&ndash;Sustain&ndash;Release volume shape drawn from the four knobs. The moving dot shows where a held note sits on that curve right now."],
  vizScope: ["Output Scope",
    "A live oscilloscope of the sound leaving the plugin. Watch the waveform react as you play and as you turn the filter, vintage, and envelope controls."],

  // ── Concept presets (each tells you the lesson it isolates) ───────────────────
  lessonRawOneShot: ["Raw One-Shot",
    "Press a key, hear the whole sample once, no loop. The simplest thing a sampler does and the place to start."],
  lessonTunedKeyboard: ["Tuned Across the Keyboard",
    "How one recording becomes a playable instrument: set the Root&nbsp;Key and every key plays the sample at its own pitch."],
  lessonLoopedPad: ["Looped Pad",
    "A loop with a crossfade turns a short sound into an endless one you can hold — no click at the seam."],
  lessonReversedSwell: ["Reversed Swell",
    "Reverse plus a slow attack makes a backwards swell that rises into the downbeat — a classic intro and transition effect."],
  lessonRepitchStretch: ["Repitch vs Stretch",
    "The headline A/B. Play the same note in each mode: Repitch changes speed with pitch (tape-style); Stretch keeps the timing and moves pitch on its own."],
  lessonSp1200: ["SP-1200 Crunch",
    "Lean on Vintage to hear how an old sampler's low sample rate and bit depth add the gritty, lo-fi character beloved in hip-hop."],
  lessonFilteredEnv: ["Filtered & Enveloped",
    "The low-pass filter and the amp envelope together — the two main shaping tools — sculpt a raw sample into a finished, musical note."],
};

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

// ── Toast ────────────────────────────────────────────────────────────────────
let toastTimer = null;
function showToast(msg) {
  const t = document.getElementById("toast");
  if (!t) return;
  t.textContent = msg;
  t.classList.add("show");
  if (toastTimer) clearTimeout(toastTimer);
  toastTimer = setTimeout(() => t.classList.remove("show"), 2600);
}

function setSourceStatus(text, truncated) {
  const el = document.getElementById("sourceStatus");
  if (!el) return;
  el.textContent = text || "";
  el.classList.toggle("truncated", !!truncated);
}

// ── After a load completes, surface the 30 s truncation notice ──────────────
async function reportTruncationIfAny(label) {
  try {
    const wasTrunc = await Juce.getNativeFunction("wasLastLoadTruncated")();
    if (wasTrunc) setSourceStatus(`${label} — truncated to 30 s`, true);
    else setSourceStatus(`${label} loaded`, false);
  } catch (e) {
    setSourceStatus(`${label} loaded`, false);
  }
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
  showToast(`Loading ${fileEntry.name}…`);
  try {
    const startOk = await Juce.getNativeFunction("dropSampleStart")(sessionId, fileEntry.name);
    if (!startOk) { showToast("Drop session start failed"); return; }

    const base64 = await readFileEntryAsBase64(fileEntry);

    const chunkOk = await Juce.getNativeFunction("dropSampleChunk")(sessionId, fileEntry.name, base64);
    if (!chunkOk) { showToast("File transfer failed"); return; }

    const commitOk = await Juce.getNativeFunction("dropSampleCommit")(sessionId, fileEntry.name, "");
    if (!commitOk) { showToast("File load failed at commit"); return; }

    await reportTruncationIfAny(fileEntry.name);
    fetchSourceThumbnail();   // new source published → refresh the waveform editor
  } catch (e) {
    console.error("[O-simpleSampler] drop failed:", e);
    showToast(`Drop failed: ${e && e.message ? e.message : e}`);
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

    if (entry.isDirectory) { showToast("Drop a single audio file, not a folder"); return; }
    if (!audioRe.test(entry.name)) { showToast("Drop a .wav / .aif / .flac file"); return; }
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
      setTimeout(() => { reportTruncationIfAny("Source"); fetchSourceThumbnail(); }, 1200);
    } catch (e) {
      console.error("[O-simpleSampler] Load… failed:", e);
      showToast("Load failed");
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
const PRESET_LESSONS = {
  "Raw One-Shot":
    "Raw One-Shot — press a key and hear the whole sample once, no loop. The simplest thing a sampler does, and the place to start.",
  "Tuned Across the Keyboard":
    "Tuned Across the Keyboard — one recording becomes a playable instrument. Set the Root Key and every key plays the sample at its own pitch.",
  "Looped Pad":
    "Looped Pad — a loop with a crossfade turns a short sound into an endless one you can hold, with no click at the seam.",
  "Reversed Swell":
    "Reversed Swell — Reverse plus a slow attack makes a backwards swell that rises into the downbeat. A classic intro / transition.",
  "Repitch vs Stretch A/B":
    "Repitch vs Stretch — the headline A/B. Repitch changes speed with pitch (tape-style); Stretch keeps the timing and moves pitch independently.",
  "SP-1200 Crunch":
    "SP-1200 Crunch — Vintage drops the sample rate and bit depth to add the gritty, lo-fi character of classic hip-hop samplers.",
  "Filtered & Enveloped":
    "Filtered & Enveloped — the low-pass filter and the amp envelope together sculpt a raw sample into a finished, musical note.",
};

let applyPresetFn = null;

async function applyPreset(name) {
  if (applyPresetFn) {
    try { await applyPresetFn(name); }
    catch (e) { console.error("[O-simpleSampler] applyFactoryPreset failed:", e); }
  }
  const cap = document.getElementById("tourCaption");
  if (cap) cap.textContent = PRESET_LESSONS[name] || name;
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
function setupTooltips() {
  const tip = document.getElementById("tooltip");
  if (!tip) return;
  let active = null;

  const show = (key, x, y) => {
    const entry = TIPS[key];
    if (!entry) return;   // 3.1: TIPS is empty (3.3 authors the copy)
    tip.innerHTML = `<span class="tip-title">${entry[0]}</span>${entry[1]}`;
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

  const plain = (html) => {
    const d = document.createElement("div");
    d.innerHTML = html;
    return (d.textContent || d.innerText || "").replace(/\s+/g, " ").trim();
  };

  document.querySelectorAll("[data-tip]").forEach((el) => {
    const key = el.getAttribute("data-tip");
    const entry = TIPS[key];
    if (entry && !el.hasAttribute("title"))
      el.setAttribute("title", `${entry[0]} — ${plain(entry[1])}`);
    el.addEventListener("pointerenter", (e) => { active = key; show(key, e.clientX, e.clientY); });
    el.addEventListener("pointermove", (e) => { if (active === key) position(e.clientX, e.clientY); });
    el.addEventListener("pointerleave", hide);
    el.addEventListener("pointerdown", hide);
  });
  document.addEventListener("keydown", (e) => { if (e.key === "Escape") hide(); });
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
const PITCH_MODE_TEXT = [
  "Repitch — pitch & time linked",
  "Stretch — time held, pitch independent",
];
function setupPitchModeReadout() {
  const el = document.getElementById("pitchModeReadout");
  const st = comboState["pitchMode"];
  if (!el || !st) return;
  const refresh = () => { el.textContent = PITCH_MODE_TEXT[st.getChoiceIndex()] || ""; };
  st.valueChangedEvent.addListener(refresh);
  st.propertiesChangedEvent.addListener(refresh);
  refresh();
}

// ── Boot ────────────────────────────────────────────────────────────────────
function boot() {
  KNOB_IDS.forEach(bindKnob);
  COMBO_IDS.forEach(bindCombo);
  TOGGLE_IDS.forEach(bindToggle);

  bindSourceDrop();
  bindLoadButton();
  setupPresets();
  setupTooltips();
  setupKeyboard();

  // Phase 3.2 — interactive waveform editor + viz layer.
  setupCanvases();
  setupPitchModeReadout();
  setupVizEvents();
  fetchSourceThumbnail();

  // Seed the source status line. Without a selector in the Source group, nothing
  // else tells the user what is playing until a load completes — setSourceStatus
  // only ever fires from a picker/drop. A fresh instance therefore has to read as
  // "loaded" rather than blank. Any subsequent load overwrites this text.
  setSourceStatus(`${BUILT_IN_SOURCE_NAME} — built-in source`, false);
}

if (document.readyState === "loading")
  document.addEventListener("DOMContentLoaded", boot);
else
  boot();
