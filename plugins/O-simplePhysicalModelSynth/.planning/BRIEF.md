# O-simplePhysicalModelSynth - Creative Brief

## Overview

**Type:** Synth (Pedagogical Physical Modeling)
**Core Concept:** A deliberately simple physical-modeling synth that makes the class's one mental model — **excitation → resonator → material/damping** — playable: you choose how energy is injected (pluck / strike / bow), what carries it (a Karplus-Strong/waveguide **string** or a **modal** body), and how it loses that energy (low-pass cutoff + feedback), and you see the model react in real time. Built for MUSC319 wk08 (physical modeling) and for making music.
**Status:** 💡 Ideated
**Created:** 2026-06-26

## Vision

O-simplePhysicalModelSynth is the physical-modeling sibling to **O-simpleFM** and **O-simpleAdditive**: a teaching instrument first and a synth second. Where O-simpleFM strips FM to a carrier, a modulator, a ratio, and an index, and O-simpleAdditive strips additive to a bank of partials, this plugin strips physical modeling down to the single structure the class says unifies the whole family: **an excitation that drives a resonator whose material/damping colors and sustains the energy.** A pluck, strike, or bow injects energy; a string or a struck body carries and shapes it; and the way that body loses energy over time sets the decay.

The class teaches three techniques under that one structure — **Karplus-Strong** (a noise burst into a delay line with feedback and a one-pole low-pass), the **digital waveguide** that generalizes it to two traveling waves, and **modal synthesis** (a sum of decaying sinusoids for bells and bars). This plugin makes that unity tangible by exposing **one resonator with two settings**: a *String* (Karplus-Strong / waveguide) and a *Modal* body. The same three exciters drive either one, so a student can hold the exciter fixed and swap the resonator — hearing why a struck bar is not a struck string — or hold the resonator fixed and swap the exciter, exactly the two comparisons the demo asks for.

The pedagogical payload is the tight loop between **gesture and visible consequence** — the north star inherited from O-simpleFM. The headline visual is the class's own block diagram **brought to life**: the excitation enters, the energy circulates the delay line (or reflects up and down the waveguide, or rings as modal stems), and you *watch it dampen on every pass*. Turn the **Damping** (loop low-pass) down and the **Decay** (feedback) up and watch the same model travel from a bright steel string to a muted nylon one — the demo's central move — while the waveform, the spectrogram, and the circulating-energy diagram all respond. Switch the resonator to *Modal*, raise **Inharmonicity**, and watch the harmonic comb break into the inharmonic stems that make a bell sound like a bell rather than a string. Play harder and hear the model brighten the way a real instrument does — the thing the class says sample-and-subtract methods struggle with.

Every control is annotated with a short, plain-language tooltip (what the delay length does to pitch, why feedback near one sustains, why higher modes decay faster). The design north star, like its siblings: a curious student reaches a genuine "oh, *that's* how physical modeling works" moment within five minutes, with no manual — and leaves able to save a plucked-string patch and a struck-bar/bell patch to their A2 patch palette.

**In-house lineage:** the Ouaricon suite already ships production-grade physical models — **O-Lyrica** (Karplus-Strong harp), **O-Bells** (modal), **O-Bowed**/**O-Wind**/**O-Reed** (waveguides). This plugin is the *teaching distillation* of those instruments — the smallest control set that still explains the family — exactly as O-simpleFM relates to a full DX7. Their validated DSP is a direct reference for Stage 0.

## Architecture

**One excitation → resonator → material chain, with a two-way resonator switch.** The instrument always follows the same path; only the resonator's internal model changes.

```
        EXCITATION                    RESONATOR                      MATERIAL / DAMPING
   ┌───────────────────┐     ┌──────────────────────────┐
   │ Pluck (noise burst)│    │  STRING  (Karplus-Strong  │      loop low-pass  (Damping)
   │ Strike (impulse)   │──► │          / waveguide)     │ ───► +  feedback    (Decay)   ──► amp ──► out
   │ Bow (friction)     │    │     ── or ──              │      = steel ◄────► nylon
   │  position · color  │    │  MODAL   (decaying sines) │           (Material macro)
   └───────────────────┘     └──────────────────────────┘
                                   pitch from delay length /            16-voice poly
                                   mode set;  velocity → brightness     viz follows loudest voice
```

- **Excitation:** how energy enters. *Pluck* = short broadband noise burst (the canonical KS exciter). *Strike* = a short filtered impulse / mallet hit (struck strings, and the natural exciter for the modal body). *Bow* = a continuous friction (stick-slip) drive that sustains the note instead of letting it decay. **Position** (where along the string) and **Color/Hardness** (how bright the exciter is) shape the attack.
- **Resonator — String:** a Karplus-Strong loop (single delay + one-pole low-pass + feedback), with a digital-waveguide option (two traveling-wave delays reflecting at each end) that additionally makes **excitation position** meaningful. Pitch comes from the delay length (fundamental = sample rate ÷ N), tuned accurately with a fractional-delay all-pass so high notes stay in tune.
- **Resonator — Modal:** a small bank of decaying sinusoids (each a mode with its own frequency, amplitude, and decay rate). **Inharmonicity** stretches the mode spacing from harmonic (bar-like) toward inharmonic (bell-like); higher modes are quieter and decay faster, so a struck body starts bright and settles onto its low hum.
- **Material / Damping:** the two numbers the class says control the result. **Damping** is the loop low-pass cutoff (darkens the tone a little on every pass — bright steel ↔ muted nylon). **Decay** is the feedback gain / ring time (near one = long sustain; lower = a damped pluck). A single **Material** macro co-moves both for the demo's one-gesture steel↔nylon sweep.
- **Amp + polyphony:** a per-voice amplitude envelope (mainly attack/release shaping — the body's natural decay is intrinsic to the model, and matters most for the sustained *Bow* exciter), 16-voice polyphony to match the suite, and **velocity → excitation strength/brightness** so the model responds to how hard you play.

## Parameters

*Core set defined here; Stage 0 research should confirm ranges, tapers, mode count, the Material-macro curve, and the fractional-delay tuning. Ranges below are starting proposals.*

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| **Excitation Type** | Pluck / Strike / Bow | Pluck | How energy is injected. Pluck = noise burst; Strike = impulse/mallet; Bow = sustained friction. The demo's "swap the exciter" control. |
| Excitation Position | 0–100% | 25% | Where along the string the energy enters (comb-filter / waveguide pickup point). Mid = rounder; near the end = thinner/brighter. Most meaningful on the waveguide string. |
| Excitation Color | 0–100% | 60% | Brightness/hardness of the exciter (noise-burst or impulse low-pass; soft mallet ↔ hard). |
| Bow Force | 0–100% | 50% | (Bow only) friction pressure driving the stick-slip — affects attack noise and harmonic richness. |
| **Resonator Type** | String / Modal | String | THE engine switch. String = Karplus-Strong/waveguide (harmonic); Modal = decaying sinusoids (inharmonic bars/bells). |
| String Model | Karplus-Strong / Waveguide | Karplus-Strong | (String only) single-delay KS vs dual-delay waveguide. Waveguide enables Excitation Position and the traveling-wave view. Confirm scope in research. |
| Inharmonicity | 0–100% | 0% | (Modal only) stretches mode spacing from harmonic (bar) toward inharmonic (bell). The defining modal control. |
| Mode Brightness | 0–100% | 50% | (Modal only) balance/decay of upper modes — how bright and metallic the struck body is. |
| **Damping** (loop low-pass) | dark ↔ bright (cutoff) | mid-bright | Loop low-pass cutoff — removes a little high-frequency energy each pass; the tone darkens as it decays. Bright steel ↔ muted nylon. |
| **Decay** (feedback) | short ↔ long (≈ feedback 0.80–0.999) | medium-long | Feedback gain / ring time. Near one = long sustain; lower = a damped, muted pluck. |
| **Material** (macro) | steel ↔ nylon | steel-ish | One-knob convenience that co-moves Damping + Decay for the demo's single-gesture material sweep. |
| Coarse Tune | -24 to +24 st | 0 | Transpose. |
| Fine Tune | -100 to +100 cents | 0 | Fine pitch. |
| Amp Attack | 0–2 s | 0.001 s | Output amplitude attack (mostly for Bow / shaping). |
| Amp Release | 0–5 s | 0.2 s | Output amplitude release / note-off damping. |
| Velocity → Brightness | 0–100% | 60% | How much harder playing brightens/strengthens the excitation (the model's dynamic response). |
| Output Level | -inf–0 dB | -6 dB | Master output gain. |

**Likely additions / confirmations from research (Stage 0):** exact modal mode count and default mode sets (bar vs bell), fractional-delay tuning method (all-pass interpolation) for accurate KS/waveguide pitch, anti-aliased / DC-safe excitation design, whether Bow gets a second control (speed vs force), per-voice vs global visualization tap, polyphony confirmation (proposing 16), optional sustain/sostenuto behavior, and a future **Blow/tube** exciter+resonator (winds/brass) — deliberately deferred (see Out of Scope).

## UI Concept

*Captured from user-volunteered direction; full UI design happens in the mockup phase, not here.*

**Layout:** A single clear page (no deep menus), classroom/projector-readable, laid out left-to-right as the signal flows: **Excitation** (type + position + color/bow force) → **Resonator** (String/Modal switch + the engine-specific control: String Model or Inharmonicity/Brightness) → **Material/Damping** (Damping, Decay, Material macro) → **Amp / Output**. The live visualization panel is prominent and central — the model's behavior is the headline, mirroring O-simpleFM's spectrum analyzer.

**Visual Style:** Clean, instructional, uncluttered — consistent with O-simpleFM / O-simpleAdditive. Readable at a glance.

**Key Elements (pedagogical layer — first-class functional features, not decoration). All four selected:**
- **Animated loop / flow diagram** — the class's own figure brought to life: excitation enters, energy circulates the delay line (KS), reflects up/down the two delays (waveguide), or rings as stems (modal), and *visibly dampens on every pass*. The headline "mental model" visual; it re-skins itself to the selected resonator.
- **Waveform + decay scope** — live time-domain output showing the pluck attack and its decay envelope (the class's decay figure, in real time).
- **Live spectrum / spectrogram** — shows the harmonics fading top-down as the loop low-pass darkens the tone each pass (and the inharmonic spacing in Modal mode), tying back to the additive/FM engines.
- **Modal stem display** — in Modal mode, the inharmonic mode frequencies drawn as stems with their decay rates — the right-hand panel of the class's modal figure, showing why a bell isn't a string.
- **On-hover pedagogical tooltips** — short plain-language explanation per control (pitch = SR ÷ delay length; feedback near one sustains; higher modes decay faster; what inharmonicity does).

## Use Cases

- **Classroom demonstration** — instructor projects the plugin and walks the demo steps live: build a KS plucked string from a noise burst, set pitch from the delay length, sweep Damping/Decay to turn steel into nylon, swap the exciter (pluck → strike → bow), then switch the resonator to Modal for a bell — with immediate audio + visual feedback at every step.
- **Self-directed student learning** — a student works the preset tour, reads tooltips, and saves a plucked-string patch and a struck-bar/bell patch to their A2 patch palette (the in-class activity: build one physical-modeling patch to complete the comparison).
- **Engine comparison** — the unifying capstone of the synthesis unit: hold the exciter fixed and swap String↔Modal, or hold the resonator fixed and swap exciters, to feel *why* physical modeling reaches acoustic timbres the other engines force.
- **Lightweight creative instrument** — playable and musical enough to double as a real instrument: plucked strings, kotos/harps, struck bars and bells, and sustained bowed tones, 16-voice polyphonic.

## Inspirations

- **Karplus & Strong (1983), *Digital Synthesis of Plucked-String and Drum Timbres*** — the canonical algorithm the plugin makes tangible.
- **CCRMA / Julius O. Smith, *Physical Audio Signal Processing*** — the waveguide and modal theory behind the resonator switch.
- **Yamaha VL1** — the first commercial waveguide instrument; the historical anchor the class pictures.
- **Logic Sculpture / Ableton Tension + Collision + Corpus / AAS Chromaphone + String Studio / Madrona Aalto + Kaivo** — the modern physical-modeling instruments the class names; O-simplePhysicalModelSynth is the deliberately minimal teaching counterpart.
- **O-Lyrica (KS harp), O-Bells (modal), O-Bowed / O-Wind / O-Reed (waveguides)** — the in-house production siblings whose validated DSP this plugin distills.
- **O-simpleFM / O-simpleAdditive** — the direct pedagogical templates (irreducible control set, gesture→visible-consequence, live visuals, tooltips, concept-isolating presets).

## Technical Notes

- **String DSP:** Karplus-Strong loop = excitation written once into an N-sample delay, then each sample outputs the oldest value and writes back feedback × ½(current + previous) — the two-tap average being the one-pole low-pass the class derives. Waveguide option = two delay lines (right- and left-going) with reflect-and-invert terminations; this is the case that makes Excitation Position physically meaningful.
- **Tuning (critical gotcha):** integer delay length quantizes pitch (fundamental = SR ÷ N), which goes audibly out of tune at higher notes. Tune with a **fractional-delay all-pass interpolator** (the standard KS-extension) so notes are in tune across the keyboard. Confirm method in research; O-Lyrica is the in-house reference.
- **Modal DSP:** a small bank of decaying sinusoids (resonant filters / state-variable modes), each with frequency, amplitude, and decay rate. Inharmonicity stretches the mode frequencies; higher modes get lower amplitude and faster decay. O-Bells is the in-house reference for stable real-time modal banks.
- **Excitation design:** keep the noise burst / impulse DC-safe and band-limited so the model doesn't buzz; Bow needs a stable stick-slip / continuous-noise drive that sustains without blowing up (clamp feedback). A teaching tool must not click or alias.
- **Material mapping:** Damping = loop low-pass cutoff; Decay = feedback gain (clamped below 1 for stability). The Material macro maps a single steel↔nylon axis onto a co-moving (cutoff, feedback) curve — define the curve in research so the one-knob sweep is audibly and visibly the demo's move.
- **Velocity / dynamics:** velocity scales excitation strength and brightness so the model responds to playing dynamics — the central physical-modeling selling point the class highlights.
- **Visualizations:** the loop/flow diagram, scope, spectrum, and modal stems all draw from the audio thread via a lock-free FIFO; keep audio-thread work allocation-free. The diagram must reflect the *actual* circulating energy (tap the loop state), not a canned animation, so what students see is what they hear.
- **Polyphony:** proposing 16 voices (matches O-simpleFM / O-simpleAdditive); the visualization follows the most recent / loudest voice. Confirm in research.
- **Platform:** WebView UI (JUCE 8) for the rich live diagram + tooltips, consistent with the Ouaricon suite. Must set Windows WebView2 flags (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`) per project standards.

## Out of Scope (v1.0)

- **Blow / tube resonator (winds, brass).** Bow was chosen over blow for v1; a blown excitation paired with an open/closed-end tube waveguide is the natural next extension, not part of v1.0. (The in-house O-Wind / O-Reed already cover production winds.)
- **Sculpture-level component modeling** — two movable pickups, multiple simultaneous strings, material morphing. Out of scope; keep the chain legible.
- **Sympathetic / coupled strings and body resonance convolution.**
- **Effects (reverb/delay/chorus)** — keep the signal path transparent for teaching (a student can add reverb downstream).
- **Deep modulation matrix / LFO networks** beyond the velocity→brightness response.
- **Large modal preset libraries** — v1.0 ships a small concept-isolating set (bright steel, muted nylon, koto/harp, struck bar, bell, bowed string), not an instrument bank.

## Next Steps

- [ ] Create UI mockup (`/start O-simplePhysicalModelSynth` → option 3)
- [ ] Start planning / DSP research (`/plan O-simplePhysicalModelSynth`)
