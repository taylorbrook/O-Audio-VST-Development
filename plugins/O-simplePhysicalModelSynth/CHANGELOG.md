# Changelog — O-simplePhysicalModelSynth

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.1.0] — 2026-08-09

UI enhancement. No parameter/state-format changes.

### Added
- **"?" tooltip toggle button** in the header, right of the preset bar. Toggles
  the on-hover tooltips on/off (defaults to on). The choice persists across
  sessions via `localStorage` (`opms.tipsEnabled`). Same pattern as
  O-simpleGrain v1.2.0; styled as a circular button in the Naturalist palette
  with `aria-pressed` state.

## [1.0.3] — 2026-08-08

Maintenance patch. No parameter/state-format changes. First published release since
v1.0.1 — the v1.0.2 fix below ships in this release as well.

### Fixed
- **MSVC compatibility — SafePointer init-capture in nested lambdas.** Hoisted
  `SafePointer(this)` init-captures to locals so the Windows CI build compiles
  (MSVC rejects the nested-lambda init-capture form).

### Changed
- Added AGPL-3.0 notice headers to all Ouaricon-authored source files
  (repo-wide license compliance pass).

## [1.0.2] — 2026-07-16

Verify-pass residual (VR-01, residual of CR-03). No parameter/state-format changes — PATCH.

### Fixed
- **VR-01 — stale Material-macro AsyncUpdate can stomp a restored state.** A `material`
  automation event on the audio thread stashes damping/decay targets and queues
  `triggerAsyncUpdate()`; if `setStateInformation` ran before the update fired, the
  queued apply landed after `restoringState` cleared and overwrote the restored
  damping/decay with pre-restore values. Root cause: the CR-03 `restoringState` guard
  covered `parameterChanged`/`handleAsyncUpdate` re-entry during the restore, but not
  an update already queued before it. Fix: `cancelPendingUpdate()` in
  `setStateInformation` while the guard is still up — same pattern the destructor
  already uses. Testing: render-harness regression suite re-run post-fix — 22/22 PASS
  (incl. state-roundtrip).

## [1.0.1] — 2026-07-16

Code-review resolution: all Critical + Warning findings from the 2026-07-15 deep
review (CODE_REVIEW.md CR-01..CR-03, WR-01..WR-05). No parameter/state-format
changes — PATCH.

### Fixed
- **CR-01 — heap allocation on the audio thread.** The cached-param scheme was keyed
  by `juce::String` with a `const char*` lookup, so every `p()` call constructed two
  heap-allocating String temporaries — ~32 mallocs per `processBlock`, defeating the
  cache it implemented. Root cause: `std::map<juce::String, …>` + per-call `count/at`.
  Replaced with named `std::atomic<float>*` members assigned once in the constructor;
  zero lookups at render time.
- **CR-02 — FileChooser completion use-after-free.** Both `savePresetWithDialog` and
  `loadPresetFromFile` captured raw `this` and called `complete()` unconditionally; if
  the host destroyed the editor while the native dialog was open, the late completion
  dereferenced a dead editor AND a dead WebView-owned `complete`. Now guarded with
  `juce::Component::SafePointer` and a bare `return` on teardown (calling
  `complete(false)` on the null path would itself be a UAF — suite pattern from
  O-MicrotonalSampler v1.23.5 W12).
- **CR-03 — Material macro stomped explicitly saved Damping/Decay on user-preset load
  and DAW session restore.** Root cause: both persistence paths applied `material`
  after `damping`/`decay`, and the macro listener re-derived + overwrote the just-
  restored values. Two-part fix: (1) preset path — preset-manager module v1.0.5
  applies meta parameters (`isMetaParameter()`) first in both the reset and apply
  passes, so explicit saved values win while material-only factory presets still get
  the derivation; (2) state path — `setStateInformation` sets a `restoringState` flag
  that suppresses the macro (session XML always carries all 17 params explicitly).
- **WR-01 — hard voice stop didn't silence the voice.** `stopNote(…, false)` (CC120
  All Sound Off, `releaseResources`) called `clearCurrentNote()` but left the
  sustain=1 envelope active forever — the voice kept rendering its ringing tail and
  burning the full KS/modal DSP path indefinitely. Hard-stop now resets the envelope,
  string, and modal bank.
- **WR-02 — Material macro could call `setValueNotifyingHost` from the audio thread.**
  APVTS listeners fire on the thread that changed the value; VST3 host automation of
  `material` arrives on the audio thread, making the macro's host-edit callbacks a
  spec violation (deadlock/allocation hazard in some hosts). The listener now applies
  synchronously only on the message thread (preserving preset-load ordering) and
  defers via `AsyncUpdater` otherwise.
- **WR-03 — low notes clamped mistuned.** The KS delay line was sized for a 20 Hz
  floor (`fs/20+100`) but `setFrequency` accepts down to 8 Hz (MIDI 0–13 and
  `coarseTune −24` legitimately request below 20 Hz) — `setDelay` jasserted and
  pinned every such note at ~19 Hz. Delay now sized `fs/8+100` to match the clamp.
- **WR-04 — pitch wheel was dead.** `pitchWheelPos` was tracked but never read by
  `computeF0`. Bend (standard ±2 semitones) is now folded into `computeF0`; block-rate
  application via the existing per-block `setParams` → `setFrequency` path.
- **WR-05 — `getTailLengthSeconds()` under-reported the ring by an order of
  magnitude.** 5.0 s ("max amp release") ignored that the tail is resonator-dominated:
  feedback 0.999 → T60 ≈ 6904/f0 s (~16 s at A4). Now reports 30 s with the
  derivation documented; hosts honoring tail length no longer truncate bounces.

### Module
- preset-manager **1.0.4 → 1.0.5** (meta-params-first apply; no-op for plugins
  without meta parameters).

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
