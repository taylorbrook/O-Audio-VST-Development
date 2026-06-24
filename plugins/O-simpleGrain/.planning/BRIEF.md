# O-simpleGrain - Creative Brief

## Overview

**Type:** Synth (Pedagogical Granular Synthesizer)
**Core Concept:** A deliberately simple granular synthesizer — shatter a short sound into a cloud of windowed grains, freeze and stretch a single moment, and make grain size, density, overlap-add, window shape, spray, and the synchronous→asynchronous continuum directly hearable and visible — built for classroom teaching and self-directed learning.
**Status:** 💡 Ideated
**Created:** 2026-06-24

## Vision

O-simpleGrain is the third sibling in the pedagogical trilogy after **O-simpleFM** and **O-simpleAdditive**: a teaching instrument first and a synth second. Where O-simpleFM strips FM down to carrier/modulator/ratio/index and O-simpleAdditive strips additive down to a bank of harmonic drawbars, O-simpleGrain strips granular synthesis down to its irreducible core — **a short source sound, a windowed grain read from it, and a scheduler that sprays many overlapping grains into a continuous cloud** — and makes the central insight tangible: *a sound is raw material, not a fixed object; reopen it from the inside, grain by grain, and you can stretch one instant into an endless evolving texture.*

It is built to run alongside the **MUSC319 wk08 granular & spectral** session. The class teaches that a grain is a few-to-fifty-millisecond fragment shaped by an amplitude window; that grain size sets the basic character (a few ms reads as a pitched buzz, tens of ms as recognizable fragments); that grains spaced closer than their length overlap and sum via **overlap-add** into one unbroken sound; and that the regularity of the grain period sets where the result sits between pitch and noise — **synchronous** (constant period, pitched) through **asynchronous** (randomized properties, dense noisy clouds). O-simpleGrain lets a student reproduce every one of those moves in a real instrument.

The pedagogical payload is the tight loop between **gesture and visible consequence** (the sibling north star). Sweep grain size from 5 ms to 60 ms and hear it cross from buzz to fragments while the grain-cloud scatter thickens. Raise density and watch separate grains fuse into a continuous overlap-add texture. Switch the window from Hann to rectangular and hear a click appear on every grain — the single most audible low-level control in granular, made obvious. Hit **Freeze** to pin the read head on one instant and sustain it into a pad; add pitch and position spray and watch the texture shimmer instead of buzzing on one repeated grain. Crank the scatter (period randomness) and watch the output spectrum smear from discrete sidebands toward noise — the synchronous→asynchronous axis in one knob. Load the **fire** preset and rebuild the class's worked example: short grains, asynchronous, position spray across a two-second recording → an endless non-looping crackle.

Every control is annotated with a short, plain-language tooltip (what overlap-add is, why a rectangular window clicks, what freeze does, why density × grain size × polyphony is the CPU cost the class warns about). A live **grain-count / CPU readout** ties the class's CPU lesson directly to the controls. The design north star, like its siblings: a curious student should reach a genuine "oh, *that's* how granular works" moment within five minutes, with no manual — and leave able to freeze and stretch a short sound into an evolving texture and save it to their A3 palette (the in-class activity).

## Architecture

**A single granular voice engine: source buffer → windowed grain scheduler → overlap-add → amp envelope → out.** The grain is the atom; everything else schedules and shapes it.

```
  built-in samples ─┐
                    ├─► SOURCE BUFFER ──► read head (position + scan/stretch + freeze)
  load-your-own ────┘                          │
                                               ▼
                        GRAIN SCHEDULER  (grain size · density/overlap · scatter)
                                               │
                   per grain: read position (+ position spray)
                              transpose       (+ pitch spray, + MIDI key)
                              window shape     (rect / tri / Welch / Gauss / Hann)
                                               │
                                          OVERLAP-ADD  (windowed grains sum)
                                               │
                              per-voice amp ADSR  ×  output level ──► out   (poly)
```

- **Source buffer:** a short sound with internal movement. Curated built-ins (fire, voice, water, piano hit — matching the class's worked examples) ship embedded for a frictionless projector demo; **load-your-own** (drag-drop or file picker) covers the in-class "load a sound, freeze and stretch" activity.
- **Read head:** where in the source grains are read from. **Position** sets the point; **Scan / Time-Stretch** moves it through the source faster/slower than realtime (the time-stretch lesson); **Freeze** pins it on one instant and holds (the freeze lesson).
- **Grain scheduler:** fires grains at a grain period. **Grain Size** sets each grain's length; **Density** sets how many fire per second (and thus the overlap depth); **Scatter** randomizes the grain period, moving the result from synchronous (pitched) to asynchronous (noisy).
- **Per-grain shaping:** each grain is read from `position (± position spray)`, transposed by `grain pitch (± pitch spray, + MIDI key)`, and multiplied by the selected **window shape** — the five envelopes from the class figure (rectangular, triangular, Welch, Gaussian, Hann).
- **Overlap-add:** overlapping windowed grains sum into one continuous sound — the engine's beating heart and the headline visual.
- **MIDI + freeze:** each held note transposes the grain cloud to that pitch (polyphonic, synth-sibling consistent); Freeze lets a held note sustain a single frozen moment as a pad.
- **Amp ADSR + polyphony:** standard per-voice amplitude envelope mirroring the siblings; modest polyphony (granular is heavier — see CPU note).

## Parameters

*Core set defined here; Stage 0 research should confirm ranges, tapers, and the exact density/overlap and scatter formulations. Ranges below are starting proposals.*

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Source Sample | curated list + Load… | "fire" | Which built-in short sound is granulated; **Load…** drags/picks a user file. THE raw material. |
| Grain Size | 2–200 ms | 30 ms | Length of each grain. THE buzz↔fragments control: a few ms reads as a pitched buzz; tens of ms as recognizable fragments. Defining granular control. |
| Density | 1–200 grains/s | 40 grains/s | How many grains fire per second. With grain size, sets the overlap depth; raising it fuses separate grains into a continuous cloud. (Research: confirm whether to expose as grains/sec, grain period, or overlap factor — show an **overlap readout** either way.) |
| Position | 0–100% of source | 50% | Read position in the source — the playhead / freeze point. |
| Scan / Time-Stretch | -200% – +200% | 0% | Speed the read head moves through the source. 0% = held; <100% = stretched/slowed; negative = reverse. The time-stretch lesson; pairs with Freeze. |
| Freeze | on / off | off | Pins the read head on the current instant and sustains it indefinitely — the headline class move (one moment → a held pad). |
| Window Shape | rectangular / triangular / Welch / Gaussian / Hann | Hann | The grain amplitude envelope — the single most audible low-level control. Rectangular adds a broadband click per grain; smooth windows stay clean. The five shapes from the class figure. |
| Pitch Spray | 0 – ±12 st | 0 | Random per-grain transposition — makes a frozen texture shimmer instead of buzzing. |
| Position Spray | 0–100% | 0 | Random per-grain read position — scatters reads across the source so no two grains are identical. |
| Scatter (period randomness) | 0–100% | 0 | Randomizes the grain period. 0% = synchronous (constant, pitched, discrete sidebands); high = asynchronous (noisy cloud). The sync↔async axis in one knob. |
| Grain Pitch | -24 – +24 st | 0 | Global transposition of grains (independent of MIDI). (Research: confirm MIDI-key→grain coupling — key-tracked resample vs gate-only.) |
| Amp Attack | 0–5 s | 0.01 s | Per-voice amplitude attack. |
| Amp Decay | 0–5 s | 0.3 s | Per-voice amplitude decay. |
| Amp Sustain | 0–100% | 80% | Per-voice amplitude sustain. |
| Amp Release | 0–5 s | 0.4 s | Per-voice amplitude release. |
| Output Level | -inf – 0 dB | 0 dB | Master output gain. |

**Likely additions / confirmations from research (Stage 0):** density-vs-overlap exposure and the live overlap readout formula; MIDI-key → grain-pitch coupling (key-tracked transposition vs gate-only); fractional-read interpolation + anti-aliasing on upward transposition; per-grain stereo pan spray (keep minimal/optional); a **grain-count / CPU meter** (pedagogical — the class explicitly teaches CPU cost); max simultaneous grain cap and graceful voice-stealing; polyphony confirmation (proposing 8 — granular is heavier than FM/additive); source-length cap for built-ins and loaded files; velocity routing (to amp and/or density).

## UI Concept

*Captured from user-volunteered direction; full UI design happens in the mockup phase, not here.*

**Layout:** A single clear page (no deep menus), classroom/projector-readable, consistent with O-simpleFM and O-simpleAdditive. Grouped as: **Source** (sample selector + Load…) | **Grain** (size, density, position, scan/stretch, freeze) | **Window Shape** | **Spray & Scatter** (pitch spray, position spray, scatter) | **Amp Envelope** | **Output**. The four live visualizations occupy a prominent panel above the controls.

**Visual Style:** Clean, instructional, uncluttered — readable at a glance, suited to a projector.

**Key Elements (pedagogical layer — first-class functional features, not decoration; all four selected for v1.0):**
- **Grain cloud scatter** — accumulating dots over the source waveform (read-position × time), the embedded class demo's headline "watch the cloud form" visual. The signature granular picture.
- **Source waveform + playheads** — the loaded/selected sound with live grain read-positions, the freeze point, and the position-spray range shaded — makes position, scan, and spray tangible.
- **Grain-envelope inset** — a live plot of the selected window shape on one grain, redrawn as the shape changes — ties the "window shape is the most audible control" lesson directly to what's heard.
- **Output scope / spectrum** — time-domain scope and/or FFT of the resulting texture, showing synchronous (discrete sidebands, pitched) vs asynchronous (smeared, noisy) regimes.
- **Grain-count / CPU readout** — live count of active grains, connecting density × grain size × polyphony to the CPU cost the class warns about (and why producers freeze-and-bounce).
- **Concept-isolating preset tour** — each preset isolates one idea (see Use Cases): single grain, pitched buzz, fragments, smooth cloud, frozen pad, asynchronous cloud, granular fire.
- **On-hover pedagogical tooltips** — short plain-language explanation per control (overlap-add, why rectangular clicks, what freeze/scan do, sync vs async, the CPU cost).

## Use Cases

- **Classroom demonstration** — instructor projects the plugin, sweeps grain size from buzz to cloud, raises density to fuse the overlap-add texture, hits Freeze, and adds spray — immediate audio + visual feedback for every key term.
- **The wk08 in-class activity** — a student loads a short sound with internal movement, freezes one moment, stretches it, and sweeps grain size/density into an evolving texture to evaluate as atmosphere — the raw material for A3.
- **Self-directed learning** — a student works the preset tour, reads tooltips, rebuilds the granular-fire worked example, and saves a frozen/stretched texture to their A3 palette.
- **Lightweight creative granular instrument** — playable and musical enough to double as a simple texture/pad/atmosphere instrument, MIDI-played and polyphonic.

## Inspirations

- **O-simpleFM / O-simpleAdditive** — the direct siblings and pedagogical template (irreducible control set, gesture→visible-consequence, live visuals, tooltips, concept-isolating presets, WebView UI).
- **Barry Truax / *Riverrun* (1986)** — the granular touchstone the class cites; a whole piece accumulated from thousands of tiny grains into moving clouds.
- **Curtis Roads, *Microsound* (2001); Dennis Gabor, *Acoustical Quanta* (1947)** — the theory the grain-as-quantum idea rests on.
- **Granulator III (Robert Henke), Ableton Grain Delay, Output Portal, Mutable Instruments Clouds/Beads, Tasty Chips GR-1** — modern granular engines; O-simpleGrain is the deliberately minimal teaching counterpart.
- **O-Freeze / O-GrainScatter / O-TextureForge** — existing Ouaricon granular engines; internal references for a real-time-safe grain scheduler and overlap-add.
- **MUSC319 wk08 embedded granular demo** — builds exactly this behavior (grain size, density, position, pitch/position spray accumulating into a visible cloud); this plugin lets students reproduce that move in a real instrument.

## Technical Notes

- **DSP:** A grain scheduler firing windowed grains read from a source buffer, summed by **overlap-add**. Preallocated grain pool; **no allocation, no locks in `processBlock`**. Each grain carries its own read position, transposition, window, and amplitude.
- **Window shapes:** the five class envelopes (rectangular, triangular, Welch, Gaussian, Hann) precomputed as lookup tables; the rectangular click is a real, audible teaching artifact (not a bug).
- **Source loading:** built-in samples embedded as binary data; **load-your-own** off the audio thread. On macOS WebView use the established content-streaming drag-drop pattern (base64 stream through a JUCE `NativeFunction` into a session temp dir, then load — and **`juce::Base64::convertFromBase64`, NOT `MemoryBlock::fromBase64Encoding`**, per the documented O-MicrotonalSampler v1.0.4 gotcha) plus a standard file picker fallback.
- **Transposition:** fractional read increment with interpolation; band-limit / anti-alias on upward transposition so high pitch-spray grains stay clean (a teaching tool must not buzz).
- **Spray & scatter:** per-grain RNG (audio-thread-safe, lock-free) for position spray, pitch spray, and grain-period scatter; scatter moves the spectrum from discrete sidebands (synchronous) to noise (asynchronous).
- **Scan / Freeze:** the read head advances by Scan/Time-Stretch each block; Freeze pins it. Smooth (no zipper) so freeze→unfreeze and stretch are artifact-free.
- **Visualizations:** lock-free FIFO handoff — grain events (position, size, pitch, time) pushed to a UI ring buffer for the cloud scatter and waveform playheads; output samples for the scope; FFT for the spectrum. Audio thread stays allocation-free; UI draws from the handoff.
- **CPU meter:** derived from active grain count — first-class pedagogy (the class teaches that density × grain size × polyphony is the cost, and why producers freeze-and-bounce).
- **Polyphony:** modest (proposing 8); confirm in research against the grain budget.
- **Platform:** WebView UI (JUCE 8) for the four live visualizations + tooltips, consistent with the Ouaricon suite. Must set Windows WebView2 flags (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`) per project standards.

## Out of Scope (v1.0)

- **Spectral processing (STFT freeze / blur / filter)** — the spectral half of the wk08 session. Deliberately deferred to a future sibling (**O-simpleSpectral**) to preserve the strict one-concept discipline of the trilogy; the title is "Grain."
- **Phase-vocoder / tempo-locked time-stretch** — granular Scan + Freeze covers the "time-stretch/freeze" key term; true tempo-synced stretch is deferred.
- **Recording live input into the buffer** — built-ins + load-your-own suffice; live-input granulation (effect mode) is a separate idea.
- **Effects (reverb / delay / chorus)** — keep the signal path transparent for teaching.
- **Deep modulation matrix / LFO networks** — spray and scatter provide the randomness the class teaches; an auto-scan LFO is a possible later addition.
- **Multi-sample / multi-layer sources** — one source buffer at a time in v1.0.

## Next Steps

- [ ] Create UI mockup (`/start O-simpleGrain` → option 3)
- [ ] Start planning / DSP research (`/plan O-simpleGrain`)
