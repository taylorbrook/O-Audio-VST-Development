# O-simpleSampler - Creative Brief

## Overview

**Type:** Synth (Pedagogical Sampler Instrument)
**Core Concept:** A deliberately simple keyboard sampler — load one found sound, isolate it with start/end, tune it to the keyboard with a root key, loop it to sustain, and shape it with a filter and amp envelope — built to make the common sampler parameters tangible for classroom teaching and self-directed learning.
**Status:** 💡 Ideated
**Created:** 2026-06-25

## Vision

O-simpleSampler is the sampler sibling to **O-simpleFM**, **O-simpleAdditive**, **O-simpleGrain**, and **O-simpleSubtractive**: a teaching instrument first and a sampler second. Where the others strip a *synthesis* method down to its irreducible core, O-simpleSampler strips the *software sampler* down to its spine — **a recording, a region of it, a root key, a loop, and an envelope** — and makes the central insight of the instrument tangible: *a sampler makes a real recording playable from a keyboard; one static found sound becomes a melodic, tunable instrument.*

It is built to run alongside the **MUSC319 wk05-wed sampling** session. That session traces the sampler from musique concrète tape splicing through the Mellotron, Fairlight CMI, Akai MPC, and E-mu SP-1200 into the modern software sampler, and teaches its common parameters directly: **start/end** to isolate the useful region, a **root key** so the sample tracks the keyboard in tune, **loop points** so a short sound sustains, **pitch/time independence** as the defining move, and a **filter and amp ADSR** to give a raw recording a playable contour. O-simpleSampler lets a student perform every one of those moves in a real instrument and *see each one happen* on the waveform.

The pedagogical payload is the tight loop between **gesture and visible consequence** (the sibling north star). Drag the start and end handles inward and watch the played region shrink on the waveform while the silence and noise fall away. Set the root key and hear the same recording snap into tune across the keys. Drop in a loop and hear a half-second sound sustain forever as a held pad, with the loop region shaded on the waveform. Flip **Reverse** and hear the swell-and-cymbal-suck of a sound played backwards. Toggle **Repitch ↔ Stretch** and hear the sampler's defining trick: in Repitch a low note slows and deepens the sound (the honest varispeed behaviour of the tape and early-digital machines), then in Stretch the *pitch* moves while the *duration* holds — pitch/time independence, made audible by A/B. Open the **Vintage** knob and hear the recording crunch into the gritty low-rate, low-bit character the SP-1200 made an aesthetic. Then shape it with a filter and an amp envelope so a percussive hit can be softened to a slow swell, or a sustained tone snapped to a pluck.

Every control is annotated with a short, plain-language tooltip (what start/end do, why the root key tunes the sample, what a loop point is, the difference between repitch and stretch, what bit/rate reduction does). The design north star, like its siblings: a curious student should reach a genuine "oh, *that's* how a sampler works" moment within five minutes, with no manual — and leave able to **turn one non-musical found sound into a playable instrument and perform a short melodic phrase** (the wk05 in-class activity), saving the result to their patch palette.

## Architecture

**A single sampler voice: source recording → playback region (start/end + loop) → pitch engine (repitch/stretch) → filter → amp VCA → out.** The recording is the raw material; everything else isolates, tunes, sustains, and shapes it.

```
  curated found-sounds ─┐
                        ├─► SOURCE RECORDING ──► PLAYBACK REGION
  load-your-own ────────┘                        (start · end · loop on/fwd/pingpong + crossfade · reverse)
                                                         │
                                            PITCH ENGINE  (root key + MIDI key)
                                            Repitch (varispeed: pitch+time coupled)
                                              ↕ toggle
                                            Stretch (pitch/time independent)
                                                         │
                                              VINTAGE  (rate decimation + bit crush)
                                                         │
                                            FILTER (LP, cutoff · resonance)
                                                         │
                                            per-voice AMP ADSR  ×  output level ──► out   (poly)
```

- **Source recording:** a short found sound. Curated built-ins (a spoken-word/vocal fragment, a single instrument hit, a found texture, a percussive object — chosen so a non-musical sound can be made melodic) ship embedded for a frictionless projector demo; **load-your-own** (drag-drop or file picker) covers the in-class "load one found sound" activity.
- **Playback region:** **Start/End** isolate the useful part of the recording; **Loop** (off / forward / ping-pong) with a **loop crossfade** lets a short sound sustain as a held note; **Reverse** plays the region backwards.
- **Pitch engine:** the sample tracks the keyboard relative to a **Root Key**. **Repitch** (default) changes playback speed to change pitch — the honest varispeed behaviour of the Mellotron/Fairlight/early samplers (low notes slow and deepen the sound). **Stretch** decouples pitch from duration so the held note keeps its length while pitch tracks the key — the class's "defining move," made an A/B toggle.
- **Vintage:** one macro combining sample-rate decimation and bit-depth reduction for the SP-1200 lo-fi character (clean at zero).
- **Filter:** a single resonant low-pass (cutoff + resonance) to give the raw recording a playable tonal contour.
- **Amp ADSR + polyphony:** standard per-voice amplitude envelope mirroring the siblings — the attack/decay/sustain/release that turns a static hit into a responsive, held, velocity-sensitive note. Polyphonic so the found sound plays as chords and melodies.

## Parameters

*Core set defined here; Stage 0 research should confirm ranges, tapers, the stretch algorithm, and the loop-crossfade / vintage formulations. Ranges below are starting proposals.*

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Source Sample | curated list + Load… | (first built-in) | Which recording is played; **Load…** drags/picks a user file. THE raw material. |
| Start | 0–100% of source | 0% | Start of the played region — isolates the useful part of the recording. |
| End | 0–100% of source | 100% | End of the played region. Start/End together are the "isolate the useful region" lesson. |
| Root Key | C-1–G9 (MIDI note) | C3 | The key at which the sample plays back at its original pitch; makes the recording track the keyboard in tune. |
| Loop Mode | off / forward / ping-pong | off | Whether and how the region loops so a short sound sustains as a held note. |
| Loop Start | 0–100% of region | 0% | Start of the loop within the region. |
| Loop End | 0–100% of region | 100% | End of the loop within the region. |
| Loop Crossfade | 0–500 ms | 10 ms | Crossfade across the loop seam to remove the click on each repeat. |
| Reverse | on / off | off | Plays the region backwards (swells, reverse-cymbal transitions). |
| Pitch Mode | Repitch / Stretch | Repitch | Repitch = varispeed (pitch+time coupled, the classic-sampler behaviour); Stretch = pitch/time independent. THE pitch/time-independence lesson, as an A/B toggle. |
| Tune | -24–+24 st | 0 st | Coarse transposition of the sample independent of the keyboard. |
| Fine | -100–+100 cents | 0 | Fine detune. |
| Vintage | 0–100% | 0% | One macro: sample-rate decimation + bit-depth reduction for the SP-1200 lo-fi grit. 0 = clean. |
| Filter Cutoff | 20 Hz–20 kHz (log) | 20 kHz (open) | Low-pass cutoff — gives the raw recording a tonal contour by removing highs. |
| Filter Resonance | 0–100% | 0% | Emphasis at the cutoff. |
| Amp Attack | 0–5 s | 0.005 s | Amplitude attack — sharp = the recording's own transient; slow = softens a percussive hit into a swell. |
| Amp Decay | 0–5 s | 0.3 s | Amplitude decay toward the sustain level. |
| Amp Sustain | 0–100% | 100% | Amplitude held while the key is down. |
| Amp Release | 0–5 s | 0.2 s | Amplitude release after key-up — lets a short sound sustain past the key. |
| Velocity → Amp | 0–100% | ~50% | How much note velocity scales loudness (the responsiveness the class describes). |
| Output Level | -inf–0 dB | 0 dB | Master output gain. |

**Likely additions / confirmations from research (Stage 0):** stretch algorithm (granular / PSOLA — reuse of the O-simpleGrain scheduler vs a phase vocoder) and its artifact profile; fractional-read interpolation + anti-aliasing on upward repitch so high keys stay clean (a teaching tool must not buzz); loop-crossfade equal-power formulation and zero-crossing snap; whether Start/End and loop markers snap to zero-crossings to avoid clicks; key-tracking behaviour in Stretch mode; polyphony confirmation (proposing 16, sibling-consistent) and voice-stealing; velocity → filter routing (optional); built-in sample set and per-sample default root key; source-length cap for built-ins and loaded files; whether Tune/Fine fold into a single transpose control.

## UI Concept

*Captured from the sibling template and the wk05 class figures; full UI design happens in the mockup phase, not here.*

**Layout:** A single clear page (no deep menus), classroom/projector-readable, consistent with the simple* siblings. The **waveform editor is the dominant element** (the sampler's defining interface — the recording made visible), with the parameter groups beneath it laid out roughly as the signal path: **Source** (sample selector + Load…) | **Region** (start/end, loop, reverse) | **Pitch** (root key, repitch/stretch, tune/fine) | **Vintage** | **Filter** | **Amp Envelope** | **Output**.

**Visual Style:** Clean, instructional, uncluttered — readable at a glance, suited to a projector. Consistent with O-simpleFM/Additive/Grain/Subtractive.

**Key Elements (pedagogical layer — first-class functional features, not decoration):**
- **Waveform editor with draggable markers** — THE headline interface: the loaded recording drawn full-width, with draggable **start/end** handles, a shaded **loop region** with its own handles, a **playhead** showing the live read position, and the **root key** indicated. The class's "set start/end, add a loop point" figure made live and hands-on. This is where the sampler becomes legible.
- **Repitch-vs-Stretch indicator** — a small live readout (or the playhead behaviour itself) that makes the pitch/time difference visible: in Repitch the playhead races/slows with pitch; in Stretch its duration holds while pitch changes.
- **Filter + envelope visuals** — a live filter-response curve and an animated amp-ADSR drawn over a note, sibling-consistent, so the "shape the raw recording into a playable contour" lesson is visible.
- **On-hover pedagogical tooltips** — short plain-language explanation per control (what start/end do, why the root key tunes, what a loop point is, repitch vs stretch, what rate/bit reduction does).
- **Concept-isolating preset tour** — named patches each isolating one idea: *Raw One-Shot*, *Tuned Across the Keyboard*, *Looped Pad (short sound sustained)*, *Reversed Swell*, *Repitch vs Stretch (A/B)*, *SP-1200 Crunch*, *Filtered & Enveloped Instrument* — so students reverse-engineer each move from the minimal control set, then load their own found sound and build the in-class instrument.
- **Drag-drop target** — the waveform area accepts a dropped audio file (the in-class "load one found sound" move).

## Use Cases

- **Classroom demonstration** — instructor projects the plugin, drops in a found sound, drags start/end to isolate it, sets the root key, adds a loop to sustain it, A/Bs repitch vs stretch, opens the Vintage knob, and shapes it with the filter and amp envelope — immediate audio + visual feedback for every common sampler parameter the session names.
- **The wk05 in-class activity** — a student loads one found sound, maps and tunes it across the keyboard, sets start/end and a loop, shapes it with the envelope, and **plays a short melodic phrase from material that was never melodic** — experiencing the step from raw recording to instrument, and saves it to their patch palette.
- **Self-directed learning** — a student works the preset tour, reads tooltips, and rebuilds each sampler move (isolate, tune, loop, reverse, repitch/stretch, lo-fi) on the built-in sounds with no manual.
- **Lightweight creative sampler** — playable and musical enough to double as a simple one-shot/loop instrument for found-sound melodies, pads, and textures, MIDI-played and polyphonic.

## Inspirations

- **O-simpleFM / O-simpleAdditive / O-simpleGrain / O-simpleSubtractive** — the direct siblings and pedagogical template (irreducible control set, gesture→visible-consequence, live visuals, tooltips, concept-isolating presets, WebView UI).
- **Mellotron (1963)** — the tape-replay keyboard ancestor; a real recording made playable from a key, the varispeed/repitch behaviour O-simpleSampler's default mode honours.
- **Fairlight CMI (1979) / E-mu Emulator (1981)** — the first digital samplers; recording-into-memory played back across the keyboard with on-screen waveform editing (the waveform editor lineage).
- **E-mu SP-1200 (1987)** — the gritty low-rate/low-bit character the Vintage knob makes a hands-on control rather than a limitation.
- **Ableton Simpler / Logic Quick Sampler & Sampler (EXS24)** — the modern software-sampler workflow (load, set start/end + root key, loop, envelope) O-simpleSampler distills to its teaching core.
- **MUSC319 wk05-wed sampling session** — traces the sampler from musique concrète to software and names its common parameters; this plugin lets students reproduce every demo move in a real instrument.

## Technical Notes

- **DSP:** A single sampler voice — buffered source recording, region playback (start/end + loop with crossfade + reverse), a pitch engine, a resonant low-pass filter, and a per-voice amp ADSR. 16-voice polyphony (sibling-consistent). **No allocation, no locks in `processBlock`**; loaded samples buffered off the audio thread.
- **Pitch engine:** **Repitch** = fractional-read varispeed (pitch tracks key by changing read increment; pitch and time coupled). **Stretch** = pitch/time independent; confirm in research whether to reuse the O-simpleGrain granular scheduler or a phase vocoder, and characterise its artifacts (a teaching tool's stretch should be clean enough to read as "same length, different pitch"). The Repitch↔Stretch difference must be audibly and visibly obvious — it is the headline lesson.
- **Interpolation / anti-aliasing:** fractional read with interpolation; band-limit / anti-alias on upward transposition so high keys stay clean (a teaching tool must not buzz).
- **Looping:** forward and ping-pong loop with an equal-power crossfade across the seam to remove the per-loop click; consider zero-crossing snap for Start/End and loop markers.
- **Vintage:** sample-rate decimation (sample-and-hold downsampling) + bit-depth quantization, as one macro; mirrors O-simpleAdditive's bit-depth lesson; clean at zero.
- **Source loading:** built-in samples embedded as binary data; **load-your-own** off the audio thread. On macOS WebView use the established content-streaming drag-drop pattern (base64 stream through a JUCE `NativeFunction` into a session temp dir, then load — and **`juce::Base64::convertFromBase64`, NOT `MemoryBlock::fromBase64Encoding`**, per the documented O-MicrotonalSampler v1.0.4 gotcha) plus a standard file picker fallback.
- **Visualizations:** the waveform editor draws the source with live start/end/loop markers and a playhead fed by a lock-free FIFO from the audio thread; the filter-response curve and amp-ADSR draw sibling-consistent. Audio thread stays allocation-free; UI draws from the handoff.
- **Polyphony:** proposing 16 voices; confirm in research.
- **Platform:** WebView UI (JUCE 8) for the waveform editor + live visuals + tooltips, consistent with the Ouaricon suite and the siblings. Must set Windows WebView2 flags (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`) per project standards. If a second binary-data target is added (embedded samples + WebView resources), give it a distinct `NAMESPACE` to avoid the BinaryData collision seen in O-simpleGrain.

## Out of Scope (v1.0)

- **Slicing a break into hits / chop-to-pads** (the MPC/Drum Rack "other face" of the sampler) — deliberately excluded to keep the one-irreducible-idea sibling discipline; the wk05 lesson addressed here is the melodic keyboard sampler. Possible future "O-simpleSlicer" sibling.
- **Internal step sequencer / pad grid / beat-making** — O-simpleSampler is a pure instrument played by the DAW's MIDI; no built-in sequencing.
- **Multi-zone / velocity-layered key mapping** (multiple samples mapped across the keyboard) — v1.0 is one recording tuned across the keys; multisampling is a future expansion.
- **Effects** (reverb/delay/chorus) — keep the signal path transparent for teaching (Vintage and the filter are the only colour).
- **Tempo-sync / warp-to-grid** — the Stretch mode teaches pitch/time independence; locking loops to project tempo is out of scope for v1.0.

## Next Steps

- [ ] Create UI mockup (`/start O-simpleSampler` → option 3)
- [ ] Start planning / DSP research (`/plan O-simpleSampler`)
