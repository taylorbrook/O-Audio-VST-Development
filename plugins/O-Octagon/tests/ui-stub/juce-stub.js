/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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
// Minimal JUCE WebView bridge stub — substitutes for js/juce/index.js so that
// Source/ui/public/index.html renders in a plain browser.
//
// CONTEXT-3.1 D4 removed the browser-iteration phase: the UI is designed inside
// 3.1 execute with no separate /ui-mockup pass. THAT MAKES THIS FILE MANDATORY
// RATHER THAN ADVISORY — without it, the first render of the page would happen
// inside a plugin, where a top-level TDZ ReferenceError kills every control
// while the HTML still looks correct (pattern_module_toplevel_init_tdz) and
// nothing in the C++ toolchain can see it.
//
// O-Octagon's stub at Phase 3.3:
//     17 slider ranges  ·  18 native fns (3 from 3.1, 10 from 3.2, 5 new)
//     0 combos  ·  0 toggles  ·  no backend.addEventListener (pull-only design)
//     no canvas stub — a real browser canvas IS the thing under test
//
// ── PHASE 3.3 ADDS A CALL COUNTER (PLAN-3.3 P81) ──────────────────────────
// UI-04 criterion 2 requires the recompute assertion to be a COUNTER, not an
// eye, and the puck half of it is a Playwright claim: N frames of drag must
// leave the getFieldGrid INVOCATION count unchanged. WRITES and GESTURES record
// parameter traffic; neither can see a native call that was never made. CALLS
// closes that gap, and layout section 27 cannot be written without it.
//
// ── STILL NO backend.addEventListener, AND THAT IS A DECISION (PLAN-3.2 P61) ─
// The ping indicator is the first thing on this surface that could plausibly
// want a push. It gets a 100 ms POLL instead, and one of the three reasons is
// this file: a push path must be modelled here before anything can be rendered
// stub-first, and D4 made stub-first mandatory. The other two are that pull was
// chosen deliberately at 3.1, and that emitEvent IS
// emitEventIfBrowserIsVisible — a dropped push never retries where a poll
// self-heals (RESEARCH-3.2 N4). The absence below is a design property.
//
// ── THE STUB MUST BE ABLE TO REJECT (PLAN-3.2 P52 / P53) ──────────────────
// setVenue() validates the label set as a permutation of the negotiated output
// set and returns { ok, reason, speaker }. A stub that always accepted would
// leave ui_layout_check.js section 15 — both rows marked, commit BLOCKED —
// with nothing to drive. mapInvalid is AUDIBLE in the shipping plugin
// (RESEARCH-3.2 N8: speaker 1 gets L and speakers 2-8 all get R at unity), so
// the rejection path is the one that matters most here.
//
// ── THE PING CYCLE IS STUB-OWNED AND STEPPED, NEVER TIMED ─────────────────
// __OCTAGON_STUB__.stepPing() advances it deterministically. Playwright must
// never race a wall clock: a 1.2 s / 0.4 s cycle asserted against real time is
// a flake generator, and the C++ side counts SAMPLES for the same reason
// (PLAN-3.2 P60 — a juce::Timer is unmeasurable in a render harness).
//
// THREE THINGS THE PRECEDENT LEARNED THE HARD WAY, all honoured below:
//   1. The ranges AND the defaults must match createParameterLayout(). The
//      precedent's own comments record its table drifting five times. Here the
//      table is not merely written carefully, it is PARSED OUT OF
//      PluginProcessor.cpp AND DIFFED by ui_frontend_check.js section 15, which
//      is the actual fix for pattern_test_fixture_mirrors_drift_silently rather
//      than another instance of it.
//   2. An UNKNOWN native-function name must REJECT, not resolve. That rejection
//      is how a bridge gap surfaces here instead of as a dead control in a DAW
//      (pattern_webview_native_fn_bridge_gap).
//   3. getVenueGeometry must be MUTABLE. UI-02 criterion 5 says the metres
//      readout is resolved against the live venue; the direct analogue in
//      stub-land is "mutate the returned geometry, with the puck stationary,
//      and watch the readout move" (PLAN-3.1 P45a). A stub returning a frozen
//      constant makes that criterion untestable.
//
// TEST FIXTURE ONLY. Never shipped, never embedded by CMake.
// ============================================================================

function listenerList() {
  const listeners = [];
  return {
    addListener: (fn) => listeners.push(fn),
    callListeners: () => listeners.forEach((fn) => fn()),
  };
}

// ── The 17 ranges ──────────────────────────────────────────────────────────
// Mirrors createParameterLayout(). ALL SEVENTEEN ARE LINEAR — there is not one
// skewed parameter in this plugin, so unlike the precedent there is no
// skewForCentre() helper here and nothing for a skew transcription to get wrong.
//
// TWO NEUTRAL-DEFAULT TRAPS, both of which section 15 of the static gate pins
// against the C++ (PLAN-3.1 P44):
//
//   * w1..w8 default to 1.0, NOT the range minimum. A stub defaulting them to
//     0.0 renders EIGHT SILENT SPEAKERS — a state the plugin never ships in —
//     and every single UI-02 criterion still passes over the top of it.
//   * blur defaults to 0.10 and airAmount to 0.35, neither of which is an
//     endpoint. This is the same trap that caught grainTilt, grainCount,
//     tukeyTaper and driftRate in the precedent, four separate times.
const RANGES = {
  srcX:       { start: 0,   end: 1,  skew: 1, interval: 0,     def: 0.5 },
  srcY:       { start: 0,   end: 1,  skew: 1, interval: 0,     def: 0.5 },
  srcZ:       { start: -2,  end: 8,  skew: 1, interval: 0,     def: 0.0 },
  width:      { start: 0,   end: 6,  skew: 1, interval: 0,     def: 0.0 },

  rolloff:    { start: 3,   end: 6,  skew: 1, interval: 0,     def: 4.0 },
  blur:       { start: 0,   end: 1,  skew: 1, interval: 0,     def: 0.10 },

  w1:         { start: 0,   end: 1,  skew: 1, interval: 0,     def: 1.0 },
  w2:         { start: 0,   end: 1,  skew: 1, interval: 0,     def: 1.0 },
  w3:         { start: 0,   end: 1,  skew: 1, interval: 0,     def: 1.0 },
  w4:         { start: 0,   end: 1,  skew: 1, interval: 0,     def: 1.0 },
  w5:         { start: 0,   end: 1,  skew: 1, interval: 0,     def: 1.0 },
  w6:         { start: 0,   end: 1,  skew: 1, interval: 0,     def: 1.0 },
  w7:         { start: 0,   end: 1,  skew: 1, interval: 0,     def: 1.0 },
  w8:         { start: 0,   end: 1,  skew: 1, interval: 0,     def: 1.0 },

  hullAtten:  { start: 0,   end: 3,  skew: 1, interval: 0,     def: 1.0 },
  airAmount:  { start: 0,   end: 1,  skew: 1, interval: 0,     def: 0.35 },

  outputGain: { start: -24, end: 12, skew: 1, interval: 0,     def: 0.0 },
};

// ── The stub venue ─────────────────────────────────────────────────────────
// The §OQ4 default coordinates, verbatim from VenueModel.cpp's
// kDefaultSpeakerXYZ, with the labels JUCE derives from kDefaultLabelTypes.
//
// The classifications are the ones ConvexHull2D::build() produces for these
// eight points and are cross-checked in C++ by render-harness probe BK: three
// collinear speakers at x = 12.50 and three at x = 0.50 mean 3 and 8 sit ON an
// edge rather than being corners of it, so the hull is a HEXAGON.
//
// trimDb rides INSIDE the speaker object rather than in a parallel trims[8]
// array, so a consumer cannot index the two out of step (PLAN-3.2 P55). With
// the two rake heights below that makes 8 x 5 + 2 = 42 venue values fully
// representable from ONE getVenueGeometry call, which is what lets the Venue
// table be a rendering job and setVenue its exact inverse.
const BASE_SPEAKERS = [
  { n: 1, x:  0.50, y:  4.50, z: 4.50, label: "L",   class: "VERTEX",  trimDb: 0.0 },
  { n: 2, x: 12.50, y:  4.50, z: 4.50, label: "R",   class: "VERTEX",  trimDb: 0.0 },
  { n: 3, x: 12.50, y:  9.85, z: 4.70, label: "C",   class: "ON_EDGE", trimDb: 0.0 },
  { n: 4, x: 12.50, y: 16.00, z: 5.10, label: "Lfe", class: "VERTEX",  trimDb: 0.0 },
  { n: 5, x:  9.80, y: 19.50, z: 5.40, label: "Lss", class: "VERTEX",  trimDb: 0.0 },
  { n: 6, x:  3.20, y: 19.50, z: 5.40, label: "Rss", class: "VERTEX",  trimDb: 0.0 },
  { n: 7, x:  0.50, y: 16.00, z: 5.10, label: "Lrs", class: "VERTEX",  trimDb: 0.0 },
  { n: 8, x:  0.50, y:  9.85, z: 4.70, label: "Rrs", class: "ON_EDGE", trimDb: 0.0 },
];

const BASE_RAKE = { front: 0.0, rear: 0.0 };

// The negotiated output set, DERIVED from the default labels rather than typed
// a second time. buildSpeakerToBuffer() resolves each label against the set the
// host negotiated; a label outside it is `labelNotInSet` and a repeat is
// `duplicateLabel`, and those are exactly the two the Venue table can produce.
const NEGOTIATED_LABELS = BASE_SPEAKERS.map((s) => s.label);

// v1.1.0 — Data/OutputOrder.h's kDeviceOrder71 as abbreviations: index k holds
// the label CoreAudio's measured Emagic_Default_7_1 device order places at
// physical output k+1. Reproduced here because a stub is the fixture that
// stands in for the plugin; the SHIPPED page carries no copy of this table —
// per-speaker `output` arrives computed in the getVenueGeometry payload (D19).
const DEVICE_ORDER_LABELS = ["L", "R", "Lrs", "Rrs", "C", "Lfe", "Lss", "Rss"];

const outputNumberForLabel = (label) => DEVICE_ORDER_LABELS.indexOf(label) + 1;

// Hull order as ConvexHull2D returns it: 6 points, 3 and 8 excluded.
const BASE_HULL_SOURCES = [1, 2, 4, 5, 6, 7];

// ARCHITECTURE section 6.2 / AD-14: the envelope is DERIVED, not stored —
// speaker bounding box + 15 % margin per axis, minimum 1.0 m. Reproduced here
// because a stub has no plugin behind it; in the shipping page this arrives
// from C++ and the page contains no copy of the rule at all
// (ui_frontend_check.js section 13 asserts the C++ side reads getVenue() and
// carries no bbox literal).
const ENVELOPE_MARGIN_FRACTION = 0.15;
const ENVELOPE_MARGIN_MIN_M = 1.0;

// oo::plane::kMinSpan — the single definition of "this axis is degenerate".
const MIN_SPAN = 1.0e-6;

let speakers = BASE_SPEAKERS.map((s) => ({ ...s }));
let rake = { ...BASE_RAKE };
let venueGen = 1;

function bboxOf(pts) {
  return {
    minX: Math.min(...pts.map((s) => s.x)),
    maxX: Math.max(...pts.map((s) => s.x)),
    minY: Math.min(...pts.map((s) => s.y)),
    maxY: Math.max(...pts.map((s) => s.y)),
  };
}

function geometryPayload() {
  const bbox = bboxOf(speakers);
  const spanX = bbox.maxX - bbox.minX;
  const spanY = bbox.maxY - bbox.minY;
  const marginX = Math.max(ENVELOPE_MARGIN_MIN_M, ENVELOPE_MARGIN_FRACTION * spanX);
  const marginY = Math.max(ENVELOPE_MARGIN_MIN_M, ENVELOPE_MARGIN_FRACTION * spanY);

  const hull = BASE_HULL_SOURCES.map((n) => {
    const s = speakers[n - 1];
    return { x: s.x, y: s.y };
  });

  return {
    envelope: {
      minX: bbox.minX - marginX,
      maxX: bbox.maxX + marginX,
      minY: bbox.minY - marginY,
      maxY: bbox.maxY + marginY,
    },
    // The SPEAKER bounding box, which is what VenueModel::normToMetres()
    // denormalises srcX / srcY against — a different box from the envelope, and
    // the reason both travel in one payload rather than the page deriving one
    // from the other (see PluginEditor.cpp's getVenueGeometry comment).
    bbox: {
      minX: bbox.minX,
      maxX: bbox.maxX,
      minY: bbox.minY,
      maxY: bbox.maxY,
      degenerateX: spanX < MIN_SPAN,
      degenerateY: spanY < MIN_SPAN,
    },
    centroid: {
      x: speakers.reduce((a, s) => a + s.x, 0) / speakers.length,
      y: speakers.reduce((a, s) => a + s.y, 0) / speakers.length,
    },
    rigScale: 7.93165,
    venueName: "Default (placeholder — NOT measured)",
    // v1.1.0: `output` rides each speaker — the 1-based physical output its
    // label reaches under DEVICE_ORDER_LABELS, 0 for a label outside the set.
    speakers: speakers.map((s) => ({ ...s, output: outputNumberForLabel(s.label) })),
    // Venue-scoped, so its own object rather than a per-speaker field.
    rake: { ...rake },
    hull,
    hullCount: hull.length,
    // Phase 3.3 (P79 / Q10). Named-scene membership is a PURE FUNCTION OF THE
    // VENUE, so it rides the payload that already refreshes on venueGen rather
    // than taking a nineteenth native function — which removes a whole
    // staleness class. The four USER slots are not a venue function and have
    // their own read (getScenes) plus their own generation on getStatus.
    scenes: namedSceneMembership(),
    generation: venueGen,
  };
}

// ── Named-scene membership (PLAN-3.3 P79 / D16) ────────────────────────────
// THE PREDICATE, evaluated here because a stub is the fixture that stands in
// for the plugin — exactly as RANGES, the envelope rule and the hull sources
// above already do. THE PAGE STILL PERFORMS NO SPEAKER ARITHMETIC: membership
// arrives whole, in the getVenueGeometry payload, and frontend section 32
// asserts that absence over Source/ui/public/js/*.js, which this file is not.
//
// D16, exactly:  classify(i) != INTERIOR  ∧  |x−cx|/hx > |y−cy|/hy
// with (cx,cy) the SPEAKER CENTROID and (hx,hy) the BBOX HALF-SPANS. On the
// §OQ4 venue that gives SIDES = {3,4,7,8}; speakers 1 and 2 miss by 6.2 %
// (1.0617 vs 1.0000), which is a property of this hall and is precisely what
// FUNC-06/3's show-before-commit makes visible instead of silent.
//
// `!= INTERIOR` and NOT `== VERTEX`: speakers 3 and 8 are ON_EDGE on the
// default venue and both belong to SIDES.
const SCENE_NAMES = ['ALL', 'FRONT', 'REAR', 'LEFT', 'RIGHT', 'SIDES'];
const USER_SLOT_COUNT = 4;

function namedSceneMembership() {
    const b = bboxOf(speakers);
    const cx = speakers.reduce((a, s) => a + s.x, 0) / speakers.length;
    const cy = speakers.reduce((a, s) => a + s.y, 0) / speakers.length;
    const hx = (b.maxX - b.minX) * 0.5;
    const hy = (b.maxY - b.minY) * 0.5;

    const sideward = (s) => {
        if (s.class === 'INTERIOR') return false;
        const nx = hx > MIN_SPAN ? Math.abs(s.x - cx) / hx : 0;
        const ny = hy > MIN_SPAN ? Math.abs(s.y - cy) / hy : 0;
        return nx > ny;
    };

    const pick = (fn) => speakers.filter(fn).map((s) => s.n);

    return SCENE_NAMES.map((id) => {
        let indices;
        switch (id) {
            case 'ALL':   indices = pick(() => true); break;
            case 'FRONT': indices = pick((s) => s.y < cy); break;
            case 'REAR':  indices = pick((s) => s.y > cy); break;
            case 'LEFT':  indices = pick((s) => s.x < cx); break;
            case 'RIGHT': indices = pick((s) => s.x > cx); break;
            default:      indices = pick(sideward); break;
        }
        return { id, indices, empty: indices.length === 0 };
    });
}

// ── The four user slots ────────────────────────────────────────────────────
// PERSISTED state in the shipping plugin (a SCENES sibling of VENUE); a plain
// array here. `scenesGen` mirrors venueGen so the page knows when to refetch —
// user slots are NOT a function of the venue and cannot ride getVenueGeometry.
let userSlots = Array.from({ length: USER_SLOT_COUNT },
                           () => ({ occupied: false, w: [0, 0, 0, 0, 0, 0, 0, 0] }));
let scenesGen = 1;

const WEIGHT_IDS_ORDER = ['w1', 'w2', 'w3', 'w4', 'w5', 'w6', 'w7', 'w8'];

function currentWeights() {
    return WEIGHT_IDS_ORDER.map((id) => {
        const st = sliderStates.get(id);
        return st === undefined ? RANGES[id].def : st.getScaledValue();
    });
}

// applyScene's stub twin. THE REFUSAL LIVES HERE TOO, not only in the disabled
// control (D20): all-zero weights are DSP-05's silence path, and reaching it by
// a mis-derived scene click mid-concert is unrecoverable. The UI disabling the
// control is the affordance; this is the guarantee.
//
// The write is EIGHT BRACKETED GESTURES, exactly as PluginEditor.cpp's
// applyScene does it — so the stub records 8 start/end pairs in GESTURES and
// layout/frontend assertions about the bracket obligation have something to
// measure (D18, the third and final gesture-bracket site).
function applyScene(name) {
    const id = String(name ?? '');
    let weights = null;

    const slotMatch = /^slot([1-4])$/.exec(id);
    if (slotMatch !== null) {
        const slot = userSlots[Number(slotMatch[1]) - 1];
        if (!slot.occupied) return { ok: false, reason: 'emptySlot', weights: [] };
        weights = slot.w.slice();
    } else {
        const named = namedSceneMembership().find((s) => s.id === id);
        if (named === undefined) return { ok: false, reason: 'unknownScene', weights: [] };
        if (named.empty) return { ok: false, reason: 'emptyScene', weights: [] };
        weights = WEIGHT_IDS_ORDER.map((_, i) => (named.indices.includes(i + 1) ? 1 : 0));
    }

    WEIGHT_IDS_ORDER.forEach((wid, i) => {
        const st = getSliderState(wid);
        st.sliderDragStarted();
        const p = st.properties;
        st.setNormalisedValue((weights[i] - p.start) / (p.end - p.start));
        st.sliderDragEnded();
    });

    return { ok: true, reason: 'none', weights };
}

function storeScene(slotNumber) {
    const n = Number(slotNumber);
    if (!Number.isFinite(n) || n < 1 || n > USER_SLOT_COUNT)
        return { ok: false, reason: 'range' };

    userSlots[n - 1] = { occupied: true, w: currentWeights() };
    scenesGen += 1;
    return { ok: true, reason: 'none' };
}

// ── The eight meters ───────────────────────────────────────────────────────
// LINEAR PEAKS, and READ-AND-ZERO, exactly as the C++ side does it: the
// −60..0 dBFS mapping and the ballistics both live in js/meters.js, so sending
// the raw measurement keeps the transform in one place, and `exchange(0)` on
// the read means a DROPPED FRAME WIDENS THE MEASUREMENT WINDOW INSTEAD OF
// LOSING THE PEAK (§4.3 amendment 2 / P77).
let meterPeaks = [0, 0, 0, 0, 0, 0, 0, 0];

function readMeters() {
    const peaks = meterPeaks.slice();
    meterPeaks = [0, 0, 0, 0, 0, 0, 0, 0];
    return { peaks };
}

// ── The DBAP field grid (P69 / P73) ────────────────────────────────────────
// 32 × 40, 8-bit, base64. The QUANTITY is 1/k = sqrt(denom) — the UN-normalised
// field the solver already computes — and NOT max_i v_i², which RESEARCH-3.3
// N10 measured as IDENTICALLY 1.0000 everywhere when exactly one weight is
// non-zero (DBAP normalises to Σv² = 1, so that expression measures
// CONCENTRATION, not level, and the picture goes blank exactly when the
// spatial situation is most extreme).
//
// minDb/maxDb are the PER-RECOMPUTE OBSERVED range, not an absolute 0..1 map:
// the field over a raked audience plane is genuinely flat (every grid point is
// at z = 0 while the speakers are 4.50–5.40 m up), so an absolute map renders a
// uniform wash WHILE LOOKING AS THOUGH IT CARRIES INFORMATION.
const FIELD_COLS = 32;
const FIELD_ROWS = 40;
let fieldComputeCount = 0;

function fieldGrid() {
    ++fieldComputeCount;

    const b = bboxOf(speakers);
    const cx = (b.minX + b.maxX) * 0.5;
    const cy = (b.minY + b.maxY) * 0.5;
    const w = currentWeights();
    const active = w.some((v) => v > 0);

    const bytes = new Uint8Array(FIELD_COLS * FIELD_ROWS);
    let bin = '';

    for (let r = 0; r < FIELD_ROWS; ++r) {
        for (let c = 0; c < FIELD_COLS; ++c) {
            const x = b.minX + ((c + 0.5) / FIELD_COLS) * (b.maxX - b.minX);
            const y = b.minY + ((r + 0.5) / FIELD_ROWS) * (b.maxY - b.minY);
            const d = Math.hypot(x - cx, y - cy);
            const span = Math.hypot(b.maxX - b.minX, b.maxY - b.minY) * 0.5;
            const v = active ? Math.max(0, 1 - d / Math.max(span, 1e-6)) : 0;
            bytes[r * FIELD_COLS + c] = Math.round(v * 255);
        }
    }

    for (let i = 0; i < bytes.length; ++i) bin += String.fromCharCode(bytes[i]);

    return {
        cols: FIELD_COLS,
        rows: FIELD_ROWS,
        minDb: active ? 1.3 : 0,
        maxDb: active ? 10.4 : 0,
        data: typeof btoa === 'function' ? btoa(bin) : '',
        computeCount: fieldComputeCount,
    };
}

// ── The stub status ────────────────────────────────────────────────────────
// mapInvalidReason / mapInvalidSpeaker are 3.2 additions (PLAN-3.2 P54).
// buildSpeakerToBuffer already separated three failure modes and threw the
// distinction away in a bool; after N8 the banner is the only thing telling an
// operator WHY seven speakers just went mono, and in a hall WHICH ROW is the
// actionable half.
let status = {
  safeMode: false,
  outputSetName: "7.1 Surround",
  numOutputChannels: 8,
  mapInvalid: false,
  mapInvalidReason: "none",
  mapInvalidSpeaker: -1,
  venueGen,
};

// ── SliderState ────────────────────────────────────────────────────────────
class StubSliderState {
  constructor(name) {
    const r = RANGES[name] || { start: 0, end: 1, skew: 1, interval: 0, def: 0 };
    this.name = name;
    this.properties = {
      start: r.start, end: r.end, skew: r.skew,
      name, label: "", numSteps: 100, interval: r.interval, parameterIndex: 0,
    };
    this.scaledValue = r.def;
    this.valueChangedEvent = listenerList();
    this.propertiesChangedEvent = listenerList();
  }

  getScaledValue() { return this.scaledValue; }

  getNormalisedValue() {
    const p = this.properties;
    return Math.pow((this.scaledValue - p.start) / (p.end - p.start), p.skew);
  }

  // Records the write AND fires the echo with the snapped value, so
  // ui_layout_check.js section 10 can prove BOTH directions of the binding from
  // one gesture: the write reached the parameter, and the parameter moved the
  // control back.
  setNormalisedValue(n) {
    const p = this.properties;
    this.scaledValue = Math.pow(n, 1 / p.skew) * (p.end - p.start) + p.start;
    WRITES[this.name] = (WRITES[this.name] || 0) + 1;
    this.valueChangedEvent.callListeners();
  }

  // The brackets JUCE does NOT supply for a WebView write:
  // WebSliderParameterAttachment::sliderValueChanged goes through
  // setValueAsPartOfGesture, i.e. setValueNotifyingHost with no
  // begin/endChangeGesture (juce_ParameterAttachments.cpp:324 -> :76). Counted
  // here so the pairing is observable from a test rather than only assertable
  // statically (RESEARCH-3.1 N1 / PLAN-3.1 P39).
  sliderDragStarted() { GESTURES.push({ id: this.name, phase: "start" }); }
  sliderDragEnded()   { GESTURES.push({ id: this.name, phase: "end" }); }
}

const WRITES = {};
const GESTURES = [];
const sliderStates = new Map();

export function getSliderState(name) {
  if (!sliderStates.has(name)) sliderStates.set(name, new StubSliderState(name));
  return sliderStates.get(name);
}

// O-Octagon has NO Choice and NO Bool parameters (RESEARCH-3.1 F1) — all 17 are
// AudioParameterFloat. The relay-type split that bit the precedent three times
// (grainShape, freeze, sourceMode) is structurally absent, so these two exist
// only to make a page that reaches for them fail LOUDLY here rather than build a
// control the backend never updates.
export function getComboBoxState(name) {
  throw new Error(`O-Octagon has no Choice parameters; getComboBoxState("${name}") is a bug`);
}

export function getToggleState(name) {
  throw new Error(`O-Octagon has no Bool parameters; getToggleState("${name}") is a bug`);
}

// ── The venue write path ───────────────────────────────────────────────────
// applyVenueEditChecked's stub twin: VALIDATE, THEN APPLY. The order is the
// whole of PLAN-3.2 P52. In the shipping plugin a label set that does not
// resolve leaves mappedOutputAvailable() false, GainStage takes its else arm
// and writes out[ch][n] = ch == 0 ? sL : sR with numWrite 8 — speaker 1 gets
// the left input and speakers 2 through 8 all get the right one, at unity. That
// is AUDIBLE, not a quiet retention, so nothing half-applies here either.

function diagnoseLabels(labels) {
  if (!Array.isArray(labels) || labels.length !== NEGOTIATED_LABELS.length)
    return { reason: "notEightChannels", speaker: -1 };

  const seen = new Map();
  for (let i = 0; i < labels.length; ++i) {
    if (!NEGOTIATED_LABELS.includes(labels[i])) return { reason: "labelNotInSet", speaker: i };
    if (seen.has(labels[i])) return { reason: "duplicateLabel", speaker: i };
    seen.set(labels[i], i);
  }

  return { reason: "none", speaker: -1 };
}

const finite = (v, fallback) => (Number.isFinite(Number(v)) ? Number(v) : fallback);

// Every INVOCATION is recorded, accepted or not. Section 15 of the layout gate
// asserts that a colliding label set produces NO CALL AT ALL — "the page did not
// ask" is a different claim from "the plugin said no", and only the first one
// keeps the audible fold off the PA for the length of a two-field swap.
const VENUE_WRITES = [];

function setVenue(payload) {
  VENUE_WRITES.push(payload === null || typeof payload !== "object"
    ? null : JSON.parse(JSON.stringify(payload)));

  if (payload === null || typeof payload !== "object")
    return { ok: false, reason: "notEightChannels", speaker: -1 };

  const rows = Array.isArray(payload.speakers) ? payload.speakers : [];
  const why = diagnoseLabels(rows.map((s) => s.label));
  if (why.reason !== "none") return { ok: false, ...why };

  // Applied only after the whole 42-value set has been accepted.
  speakers = speakers.map((s, i) => ({
    ...s,
    x: finite(rows[i].x, s.x),
    y: finite(rows[i].y, s.y),
    z: finite(rows[i].z, s.z),
    label: rows[i].label,
    trimDb: finite(rows[i].trimDb, s.trimDb),
  }));

  const r = payload.rake === null || typeof payload.rake !== "object" ? {} : payload.rake;
  rake = { front: finite(r.front, rake.front), rear: finite(r.rear, rake.rear) };

  venueGen += 1;
  return { ok: true, reason: "none", speaker: -1 };
}

// ── v1.1.0 — the speaker→output surface ────────────────────────────────────
// PluginEditor.cpp's twins, semantics matched line for line: label edits
// through the same validate-then-apply order setVenue uses. The swap and the
// repair live HERE AND IN C++ and nowhere in the page — the page sends two
// integers and renders what getVenueGeometry returns.

function assignSpeakerOutput(speakerN, outputK) {
  const n = Number(speakerN);
  const k = Number(outputK);

  if (!Number.isFinite(n) || n < 1 || n > speakers.length
      || !Number.isFinite(k) || k < 1 || k > DEVICE_ORDER_LABELS.length)
    return { ok: false, reason: "labelNotInSet", speaker: -1 };

  // Swap semantics: speaker n takes output k; the previous holder of k takes
  // n's old output. Unplaced speakers (label outside the set → 0) are repaired
  // from the pool of unclaimed outputs, so the result is a permutation by
  // construction.
  const want = speakers.map((s) => outputNumberForLabel(s.label));
  const prev = want[n - 1];

  for (let j = 0; j < want.length; ++j)
    if (j !== n - 1 && want[j] === k) want[j] = prev;

  want[n - 1] = k;

  const claimed = want.map(() => false);
  for (const w of want) if (w >= 1) claimed[w - 1] = true;

  for (let j = 0; j < want.length; ++j)
    if (want[j] < 1)
      for (let c = 0; c < claimed.length; ++c)
        if (!claimed[c]) {
          claimed[c] = true;
          want[j] = c + 1;
          break;
        }

  const labels = want.map((w) => DEVICE_ORDER_LABELS[w - 1]);
  const why = diagnoseLabels(labels);
  if (why.reason !== "none") return { ok: false, ...why };

  speakers = speakers.map((s, i) => ({ ...s, label: labels[i] }));
  venueGen += 1;
  return { ok: true, reason: "none", speaker: -1 };
}

function applyOutputOrderPreset(id) {
  const labels = id === "direct" ? DEVICE_ORDER_LABELS.slice()
    : id === "roles" ? BASE_SPEAKERS.map((s) => s.label)
      : null;

  if (labels === null) return { ok: false, reason: "labelNotInSet", speaker: -1 };

  const why = diagnoseLabels(labels);
  if (why.reason !== "none") return { ok: false, ...why };

  speakers = speakers.map((s, i) => ({ ...s, label: labels[i] }));
  venueGen += 1;
  return { ok: true, reason: "none", speaker: -1 };
}

// ── The .venue file slot ───────────────────────────────────────────────────
// The real pair opens a NATIVE MODAL through FileChooser::launchAsync, which
// Playwright cannot drive — UI-01 criterion 3 closes on three parts and the
// modal itself is Gate 13, a human launch-and-look (PLAN-3.2 P57). What the
// stub can model, and does, is the round trip: what save wrote is what load
// reads back.
let venueFileSlot = null;
const FILE_OPS = [];

function saveVenue() {
  venueFileSlot = { speakers: speakers.map((s) => ({ ...s })), rake: { ...rake } };
  FILE_OPS.push("save");
  return { ok: true, path: "/stub/venue.venue", reason: "none" };
}

function loadVenue() {
  FILE_OPS.push("load");
  if (venueFileSlot === null) return { ok: false, path: "", reason: "unreadable" };

  speakers = venueFileSlot.speakers.map((s) => ({ ...s }));
  rake = { ...venueFileSlot.rake };
  venueGen += 1;
  return { ok: true, path: "/stub/venue.venue", reason: "none" };
}

// ── The musical preset store ───────────────────────────────────────────────
// FOUR functions, not the module's ten (PLAN-3.2 P58). preset-manager.js is not
// vendored and createPresetBar is never called — it writes container.innerHTML
// and then queries its own injected markup, which is
// pattern_js_state_updater_overwrites_html_labels by construction.
const PRESET_WRITES = [];
let presetNames = ["Init"];
let currentPreset = "Init";

function savePreset(name) {
  const n = String(name ?? "").trim();
  if (n === "") return { ok: false, name: "" };
  if (!presetNames.includes(n)) presetNames = presetNames.concat([n]);
  currentPreset = n;
  PRESET_WRITES.push({ op: "save", name: n });
  return { ok: true, name: n };
}

function loadPreset(name) {
  const n = String(name ?? "");
  if (!presetNames.includes(n)) return { ok: false, name: n };
  currentPreset = n;
  PRESET_WRITES.push({ op: "load", name: n });
  return { ok: true, name: n };
}

// ── The verify ping ────────────────────────────────────────────────────────
// C++ IS THE AUTHORITY AND JS NEVER RE-DERIVES THE STEP (CONTEXT-3.2 D14). A
// drifted setInterval on the page would name speaker 5 while 6 sounds, during
// the one procedure whose entire purpose is confirming that speaker N is
// speaker N. So the cycle lives here, and the page renders getPingState().speaker
// and nothing else. ui_frontend_check.js section 26 asserts the absence of any
// JS timer that computes a speaker index.
const PING_ON_MS = 1200;
const PING_GAP_MS = 400;
const PING_LATCH_MS = 120000;

let ping = { active: false, mode: "off", speaker: 0, elapsedMs: 0 };

function pingState() {
  return {
    active: ping.active,
    mode: ping.mode,
    speaker: ping.speaker,
    elapsedMs: ping.elapsedMs,
    remainingMs: ping.active ? Math.max(0, PING_LATCH_MS - ping.elapsedMs) : 0,
  };
}

// REFUSES TO START ON AN INVALID MAP (RESEARCH-3.2 Q5). Pinging "speaker 5" on
// a stereo fold names a speaker that does not exist, inside the one diagnostic
// tool whose job is to prove the opposite — R1 reproduced in its own detector.
function startPing(target) {
  if (status.mapInvalid === true)
    return { ok: false, reason: "mapInvalid", speaker: 0, ...pingState() };

  if (target === "auto") ping = { active: true, mode: "auto", speaker: 1, elapsedMs: 0 };
  else {
    const n = Number(target);
    if (!Number.isFinite(n) || n < 1 || n > 8)
      return { ok: false, reason: "range", speaker: 0, ...pingState() };
    ping = { active: true, mode: "manual", speaker: Math.round(n), elapsedMs: 0 };
  }

  return { ok: true, reason: "none", ...pingState() };
}

function stopPing() {
  ping = { active: false, mode: "off", speaker: 0, elapsedMs: 0 };
  return { ok: true, reason: "none", ...pingState() };
}

// ── The TWENTY native functions ────────────────────────────────────────────
// Exactly the twenty PluginEditor.cpp registers. Any other name REJECTS — see
// note 2 in the header. Section 3 of the static gate diffs this whitelist
// against the C++ AS A SET, so a missing entry FAILS rather than passes; that is
// what makes it safe for this file's unregistered-name behaviour to be STRICTER
// than the plugin's, which hangs silently where this rejects.
//
// 13 -> 18 at Phase 3.3 (P81): getMeters, getScenes, applyScene, storeScene,
// getFieldGrid. UI-05 needs NO new function — getVenueGeometry already carries
// per-speaker z, rake.front/rear, bbox and the centroid, all landed by 3.2's
// P55 — and named-scene membership rides that same payload (P79).
//
// 18 -> 20 at v1.1.0: assignSpeakerOutput (the Room plan's double-click
// popover) and applyOutputOrderPreset (the Venue rail's two one-click sets).
// Per-speaker `output` itself rides getVenueGeometry — no read call was added.
const NATIVE_FNS = {
  getParameterDefaults: () =>
    Object.fromEntries(Object.entries(RANGES).map(([id, r]) => [id, r.def])),

  getVenueGeometry: () => geometryPayload(),

  getStatus: () => ({ ...status, venueGen, scenesGen }),

  setVenue: (payload) => setVenue(payload),

  // ── v1.1.0 ──
  assignSpeakerOutput: (speakerN, outputK) => assignSpeakerOutput(speakerN, outputK),

  applyOutputOrderPreset: (id) => applyOutputOrderPreset(id),

  saveVenue: () => saveVenue(),

  loadVenue: () => loadVenue(),

  savePreset: (name) => savePreset(name),

  loadPreset: (name) => loadPreset(name),

  getPresetList: () => ({ presets: presetNames.slice(), current: currentPreset }),

  getCurrentPreset: () => ({ name: currentPreset }),

  startPing: (target) => startPing(target),

  stopPing: () => stopPing(),

  getPingState: () => pingState(),

  // ── Phase 3.3 ──
  getMeters: () => readMeters(),

  getScenes: () => ({
    slots: userSlots.map((s, i) => ({ slot: i + 1, occupied: s.occupied, w: s.w.slice() })),
    generation: scenesGen,
  }),

  applyScene: (name) => applyScene(name),

  storeScene: (slot) => storeScene(slot),

  getFieldGrid: () => fieldGrid(),

  // ── v1.2.0 ──
  setTooltipsEnabled: (enabled) => { tooltipsEnabled = enabled === true; return tooltipsEnabled; },

  getTooltipsEnabled: () => tooltipsEnabled,
};

// The hover-help ("?" toggle) preference — UI state, not a parameter, mirrored
// here exactly as the processor's std::atomic<bool> holds it (default OFF).
let tooltipsEnabled = false;

// ── The invocation counter (PLAN-3.3 P81) ──────────────────────────────────
// UI-04 criterion 2's puck half is "a puck drag must not move it", and the
// counter that settles it has to count CALLS, not writes: a recompute that
// never happened leaves no trace in WRITES or GESTURES. Layout section 27 drags
// the puck across N frames and asserts CALLS.getFieldGrid is unchanged.
const CALLS = {};

export function getNativeFunction(name) {
  if (Object.prototype.hasOwnProperty.call(NATIVE_FNS, name)) {
    return (...args) => {
      CALLS[name] = (CALLS[name] || 0) + 1;
      return Promise.resolve(NATIVE_FNS[name](...args));
    };
  }

  return () => Promise.reject(new Error(`Unregistered native function: ${name}`));
}

// ── Test control surface ───────────────────────────────────────────────────
// Playwright drives the page through this. Deliberately a separate namespace
// from anything the page reads: app.js and roomplan.js must never touch it, and
// section 20 of the static gate would catch it if they did.
if (typeof window !== "undefined") {
  window.__OCTAGON_STUB__ = {
    // What the page wrote, and the gesture brackets it opened and closed.
    writes: WRITES,
    gestures: GESTURES,

    // WHAT THE PAGE ASKED FOR, per native function (P81). Distinct from
    // `writes`: a recompute that never happened leaves no parameter trace at
    // all, so UI-04/2's "a puck drag must not move the counter" is unmeasurable
    // without this.
    calls: CALLS,

    // Mutate the venue. Speakers are rescaled into the requested speaker
    // bounding box, so envelope, bbox, centroid, hull and glyphs all stay
    // mutually consistent — a torn fixture would let a test pass for the wrong
    // reason. venueGen advances, which is the same signal
    // VenueSnapshotPublisher::getGeneration() gives the real page.
    setVenueBox(minX, maxX, minY, maxY) {
      const b = bboxOf(BASE_SPEAKERS);
      const sx = (b.maxX - b.minX) > MIN_SPAN ? (maxX - minX) / (b.maxX - b.minX) : 0;
      const sy = (b.maxY - b.minY) > MIN_SPAN ? (maxY - minY) / (b.maxY - b.minY) : 0;

      speakers = BASE_SPEAKERS.map((s) => ({
        ...s,
        x: minX + (s.x - b.minX) * sx,
        y: minY + (s.y - b.minY) * sy,
      }));

      venueGen += 1;
      return venueGen;
    },

    // Move the audience plane, and NOTHING ELSE. UI-05 criterion 1's whole
    // content is that rakeRear moves the REAR of the drawn line and not the
    // front — earHeight(bbMinY) == rakeFront for any rakeRear (RESEARCH-2.2
    // H5) — so the probe needs a lever that touches the two rake values and no
    // speaker coordinate. Driving it through setVenue would move the bbox too
    // and the negative half would be measuring the wrong thing.
    setRake(front, rear) {
      rake = { front: finite(front, rake.front), rear: finite(rear, rake.rear) };
      venueGen += 1;
      return { ...rake };
    },

    resetVenue() {
      speakers = BASE_SPEAKERS.map((s) => ({ ...s }));
      rake = { ...BASE_RAKE };
      userSlots = Array.from({ length: USER_SLOT_COUNT },
                             () => ({ occupied: false, w: [0, 0, 0, 0, 0, 0, 0, 0] }));
      venueGen += 1;
      scenesGen += 1;
      return venueGen;
    },

    // Drive the SAFE banner the way a host re-negotiating a track's output
    // format drives it.
    setStatus(patch) { status = { ...status, ...patch }; },

    // Drive the mapInvalid banner, WITH its reason and row (PLAN-3.2 P54/D13).
    // The banner is frame-level and beside SAFE, so it is visible on the Room
    // screen too: R1's defining property is silence, and putting its only
    // warning behind a tab the operator is not looking at reproduces exactly
    // the failure it exists to break. Section 16 asserts BOTH screens.
    setMapInvalid(reason, speaker) {
      const bad = typeof reason === "string" && reason !== "none";
      status = {
        ...status,
        mapInvalid: bad,
        mapInvalidReason: bad ? reason : "none",
        mapInvalidSpeaker: bad ? Number(speaker ?? -1) : -1,
      };
      // A ping already running when the map goes invalid STOPS — the same
      // argument as the refusal to start, arriving from the other direction
      // (PLAN-3.2 P60, D11's fifth stop).
      if (bad && ping.active) stopPing();
      return { ...status };
    },

    // Advance the ping cycle deterministically. Never a wall clock: a 1.2 s on
    // / 0.4 s gap cycle asserted against real time is a flake generator, and it
    // is the same reason the C++ side counts SAMPLES rather than Timer ticks.
    stepPing() {
      if (!ping.active) return pingState();

      ping.speaker += 1;
      ping.elapsedMs += PING_ON_MS + PING_GAP_MS;

      // 1 -> 8 and then done. The 120 s latch below is the LATCHED ping's stop,
      // which is a different clock and is probed separately in C++ (BT).
      if (ping.speaker > 8 || ping.elapsedMs >= PING_LATCH_MS) return stopPing();
      return pingState();
    },

    // What the venue table, the preset rail and the .venue chooser asked for.
    venueWrites: VENUE_WRITES,
    presetWrites: PRESET_WRITES,
    fileOps: FILE_OPS,
    presets: () => ({ presets: presetNames.slice(), current: currentPreset }),
    ping: () => pingState(),

    // ── Phase 3.3 ──
    // Drive the eight meters the way the audio thread drives them: LINEAR
    // peaks, consumed by the next read. A test that set a dB value here would
    // be asserting against a transform that lives in js/meters.js.
    setMeters(peaks) {
      meterPeaks = Array.from({ length: 8 }, (_, i) => Number(peaks?.[i] ?? 0));
      return meterPeaks.slice();
    },

    // Light exactly ONE speaker, the way a verify ping does. UI-03 criterion 2
    // is "the speaker that lights is the speaker that sounds", and the
    // one-hot form is what makes a wrong-index meter visible rather than
    // plausible.
    pingMeter(n) {
      meterPeaks = Array.from({ length: 8 }, (_, i) => (i === Number(n) - 1 ? 0.8 : 0));
      return meterPeaks.slice();
    },

    scenes: () => namedSceneMembership(),
    slots: () => userSlots.map((s) => ({ occupied: s.occupied, w: s.w.slice() })),
    field: () => fieldGrid(),

    // Read the fixture's own numbers back, so a Playwright assertion can be
    // made against the RETURNED payload and never against a literal
    // (pattern_test_fixture_mirrors_drift_silently).
    geometry: () => geometryPayload(),
    status: () => ({ ...status, venueGen }),
    ranges: () => JSON.parse(JSON.stringify(RANGES)),
  };
}
