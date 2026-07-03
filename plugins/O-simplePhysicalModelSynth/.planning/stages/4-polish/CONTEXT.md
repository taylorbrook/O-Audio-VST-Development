# Stage 4: Polish - Context

## Discussion Summary

**Date:** 2026-06-27
**Participants:** User, Claude
**Plugin:** O-simplePhysicalModelSynth
**Stage:** 4 of 4 (Polish) — final stage before ship

Entering Stage 4 with Stage 3 (GUI) ✓ VERIFIED. Only **one** functional requirement
remains open: **FUNC-07** (concept-isolating factory presets). The preset-manager
shell is already wired (D4, Stage 3) — `OuariconPresetManager` member, 10 preset
native fns, state-I/O swap, preset bar UI — but `initializeFactoryPresets()` is an
empty stub (the bar currently runs **Default-only**). Stage 4 seeds the factory set,
re-runs the full validation gate, writes the CHANGELOG, installs, and then publishes
cross-platform.

---

## Requirements Confirmed

- **FUNC-07** (should) — ship the **6 concept-isolating factory presets** named in the
  brief, seeded via `OuariconPresetManager::initializeFactoryPresets()`:

  | Preset | Excitation | Resonator | Character intent |
  |--------|-----------|-----------|------------------|
  | Bright Steel | Pluck | String (KS) | bright Damping + long Decay; low Material (steel) |
  | Muted Nylon | Pluck | String (KS) | dark Damping + short Decay; high Material (nylon) |
  | Koto / Harp | Pluck | String (KS) | plucky, mid Position/Color, medium Decay |
  | Struck Bar | Strike | Modal | low Inharmonicity (near-harmonic bar) |
  | Bell | Strike | Modal | high Inharmonicity (inharmonic bell), long ring |
  | Bowed String | Bow | String (KS) | sustained; bow Force mid; long Decay |

  These exactly satisfy the locked FUNC-07 spec and cover all 3 exciters + both
  resonators. Cross-driving demonstrators (struck string / plucked bell / bowed bar)
  were considered and **declined** for v1.0 — the live "swap the resonator" gesture is
  the demo move; presets stay minimal/concept-isolating.

- **COMPAT-01 / COMPAT-02** (must) — re-confirm pluginval (VST3 + AU) and the Windows
  WebView2 flags survive into the shipping build. **Now exercised on Windows for real**
  for the first time via the cross-platform publish (see Distribution).

- **All remaining** must/should reqs roll up to a green final verification.

---

## Constraints Identified

- **Re-run render-harness at the START of Stage 4** (ROADMAP + O-simpleBeatmaker lesson):
  the editor is now a WebView; the harness compiles `PluginProcessor.cpp` under
  `JUCE_WEB_BROWSER=0` with `PluginEditor.cpp` dropped and `createEditor`/`#include
  "PluginEditor.h"` guarded by `#if JUCE_WEB_BROWSER`. Stage 3 verify already confirmed
  this green as a bonus, but the gate is mandatory at Stage-4 start before any new code.

- **Factory preset value format** — `FactoryPresetDef` carries
  `std::map<juce::String,float>` of param-ID → value. Research/plan must confirm
  whether the manager applies these as **raw** values (O-Bells seeds raw, e.g.
  `{"airAbsorptionTime", 4.0f}`) or **normalized** [0,1] (the struct comment says
  "normalized") and author the 6 presets accordingly. All 14 sliders here are simple
  ranges (% 0–100, dB, seconds, semitones, cents); the 3 choices are
  `excitationType` (0=Pluck,1=Strike,2=Bow), `resonatorType` (0=String,1=Modal),
  `stringModel` (0=KS,1=Waveguide). v1.0 ships KS only — keep `stringModel`=0.

- **Material macro interaction** — `material` is a macro that co-moves `damping` +
  `decay` via a message-thread APVTS listener. A preset that sets `material` AND
  `damping`/`decay` independently could fight the listener on load. Plan must decide:
  set `material` to a representative value and let it drive, OR set damping/decay
  directly and leave material neutral. (Flag for plan — preset-load ordering.)

- **Windows build is unproven** — the publish path compiles MSVC for the first time.
  Known suite gotchas to pre-empt: MSVC C3493 on non-static `constexpr` inside lambdas
  (add `static`); two `juce_add_binary_data` namespace collision (this plugin has a
  single binary-data target — verify); WebView2 static-linking flag already set
  (COMPAT-02). Scan before/with the first CI run.

- **No new DSP / no scope creep** — Stage 4 is presets + validation + ship only.
  DSP-06 (waveguide) stays deferred to v1.1. Do not modify working Stage-2/3 behavior.

---

## Approach Decisions

| # | Decision | Choice | Rationale |
|---|----------|--------|-----------|
| D1 | Factory preset set | **The 6 from the brief** (Bright Steel, Muted Nylon, Koto/Harp, Struck Bar, Bell, Bowed String) | Matches locked FUNC-07; concept-isolating; covers all exciters + both resonators. Cross-driving combos declined — the live swap is the demo. |
| D2 | Preset mechanism | `OuariconPresetManager::initializeFactoryPresets()` (already wired) | Shell built in Stage 3 (D4); only the seed data is missing. No new infra. |
| D3 | Behavioral/audio verification | **Automated gate in-stage + user play-tests after install** (I provide a focused DAW checklist) | Audio timbre/lockstep can't be meaningfully automated; user auditions. Closes the owed Stage-3 human play-through. |
| D4 | Distribution scope | **Full cross-platform publish** (GitHub Actions: mac VST3/AU + Windows VST3) | Classroom (MUSC319) needs student-installable builds on both platforms. First real Windows/MSVC exercise of this plugin. |
| D5 | Render-harness re-run | Mandatory at Stage-4 START before new code | ROADMAP + O-simpleBeatmaker lesson; WebView editor must not have broken the `JUCE_WEB_BROWSER=0` seam. |

---

## Automated Gate (I run, in-stage)

- Render-harness builds @ `JUCE_WEB_BROWSER=0` **and** ALL PASS (DSP regression guard).
- pluginval strictness-10 (VST3) + (AU).
- `auval -v aumu OsPM OuDv` SUCCEEDS.
- Native-fn parity (JS ↔ C++), param-ID parity (APVTS ↔ JS), tooltip coverage, `node --check`.
- Factory presets: each of the 6 loads without error; round-trips state; sets the expected
  excitation/resonator combo.
- Standalone renders (not blank) — screenshot.
- build-and-install.sh (dual-variant sweep).

## Manual Checklist (user runs, after install — closes owed play-through)

- Audition all 6 presets — each sounds like its name (steel vs nylon, bar vs bell, bowed sustains).
- Dragging **Material** visibly co-moves Damping + Decay.
- Grey-out tracks `resonatorType`/`excitationType` live as you switch (selectors never trapped).
- Keyboard plays (mouse + QWERTY); preset bar navigates/saves/loads/deletes; factory presets non-deletable.
- **UI-02** loop pulse dims in lockstep with audible decay; Modal skin shows live stems.
- **UI-03** scope decays after note-off. **UI-04** spectrum: harmonic comb (String) vs inharmonic (Modal).
- No console errors; no clicks/buzz across the keyboard and parameter ranges.

---

## Open Questions

- **(research)** Factory preset values — raw vs normalized in `initializeFactoryPresets`?
  Confirm against the module's `applyPresetJson` and a sibling that seeds simple-range
  params. Derive the exact 17-value map per preset from the Stage-2 DSP behavior.
- **(research/plan)** Material-vs-damping/decay load ordering — does setting all three in
  a preset fight the macro listener? Pick the safe authoring convention.
- **(plan/execute)** Windows/MSVC first-build risks — pre-scan for C3493 constexpr-in-lambda
  and binary-data namespace before the publish CI run; confirm `OUARICON_DEV_SUFFIX`/
  branding produces release (unsuffixed) bundles in CI.
- **(execute/publish)** Version/tag for the GitHub release (v1.0.0) and CHANGELOG content.

---

## Next Phase

Ready for: **research** phase (investigate factory-preset value format + Material load
ordering + Windows publish pre-flight), then plan → execute → verify → publish.
