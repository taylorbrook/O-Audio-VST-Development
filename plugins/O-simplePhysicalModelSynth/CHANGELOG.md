# Changelog — O-simplePhysicalModelSynth

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.0.0] — 2026-06-27

First release. A teaching physical-modeling synth that makes excitation→resonator
synthesis legible: pick how a string/bar is *driven* and what *resonates*, and watch
the energy-recirculation loop, spectrum, and scope respond in lockstep with the sound.

### Added
- **Excitation→resonator architecture** — 3 exciters (**Pluck**, **Strike**, **Bow**)
  drive 2 resonators (**String**, Karplus–Strong / **Modal**). All six exciter×resonator
  combinations are live and swappable mid-note; the live "swap the resonator" gesture is
  the core demo.
- **Material macro** — a single message-thread macro that co-moves **Damping** + **Decay**
  along the steel↔nylon axis, so the most musical control over a struck/plucked string is
  one knob; the underlying sliders visibly track.
- **6 concept-isolating factory presets** (FUNC-07), each isolating one timbral idea and
  together covering all 3 exciters + both resonators:
  - **Bright Steel** — Pluck → String, low Material (bright, long ring)
  - **Muted Nylon** — Pluck → String, high Material (dark, short decay)
  - **Koto Harp** — Pluck → String, mid Material (plucky, medium decay)
  - **Struck Bar** — Strike → Modal, low Inharmonicity (near-harmonic bar)
  - **Bell** — Strike → Modal, high Inharmonicity (inharmonic, long ring)
  - **Bowed String** — Bow → String, mid bow Force (sustained)

  Presets author raw/real-unit values converted to normalized via `convertTo0to1` at
  build time. String presets set `Material` only; Modal presets set `Damping`/`Decay`
  only — never both — so the macro listener and explicit values never fight on load.
- **WebView UI** — animated energy-loop diagram (the recirculation pulse dims in lockstep
  with the audible decay; the Modal skin shows live partial stems), a real-time spectrum
  (harmonic comb for String vs inharmonic for Modal), an output scope, a preset bar
  (navigate / save / load / delete; factory presets non-deletable), and an on-screen +
  QWERTY keyboard. Controls grey out contextually as the exciter/resonator changes.
- **17 parameters** across Excitation (type, position, color, bow force), Resonator
  (type, string model, inharmonicity, mode brightness), Body (damping, decay, material),
  Tuning (coarse/fine), and Amp (attack, release, vel→brightness, output level).

### Validation
- Render-harness (offline DSP, `JUCE_WEB_BROWSER=0`): **ALL PASS** — makes-sound,
  finite/no-blowup, tuning across C1–C7, bow sustain, strike, modal pluck/strike/bow,
  inharmonicity stretch, decay tracking, no-DC, state round-trip.
- pluginval strictness-10: **SUCCESS** (VST3 **and** AU).
- `auval`: **SUCCEEDED**. Native-fn parity 12↔12; param-ID parity 17/17; `node --check` OK.

### Known limitations / deferred
- **Waveguide string model (DSP-06)** deferred to **v1.1** — v1.0 ships the Karplus–Strong
  string model only (`stringModel` defaults to KS).
- Cross-driving demonstrator presets (struck string / plucked bell / bowed bar) were
  intentionally left out of the factory set — the live resonator swap is the demo move.
