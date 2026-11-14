# Factory sources expansion — improvement brief (target v1.2.0, MINOR)

Created 2026-11-13 via `/improve`. Decisions below are Taylor's unless marked "proposed".

## Goal
Grow the factory sources from 8 to about 40: the 8 existing, 13 samples from Taylor's own recordings, about 19 new synthetic. Sources only; the factory presets are reworked in a later improve.

## Decided
- Embedded FLAC (`factory:<id>` resolution unchanged). Raise the embedded budget from 8 MB to about 24 MB; per-source cap 3 MB stays. 3–12 s each (short one-shot collections may be under 3 s).
- The existing 8 ids, bytes and hash pins stay untouched (64 presets stay bitwise). No parameter change.
- Allowed sample folders under `/Users/taylorbrook/Projects`: `gameAudio/SP - Sound Effects`, `TensedComputerImproviser`, `electric guitar and improvisor`, `Soundscaper/git/soundscaper/audio`, Nox crystal tones. Nothing else from that tree (third-party or other performers). The `Jeff*` corpora were excluded by Claude's assumption.

## Sample picks (Taylor, audition of 100 candidates)
Windows are seconds in the original file; re-cut from the originals at ingest (`audition_cut.py` reproduces the audition cuts: 10 ms fades, ends snapped to the quietest 50 ms frame within ±0.75 s).

| audition id | length | ch | source (under /Users/taylorbrook/Projects) | window |
|---|---|---|---|---|
| gtr_overtones_a | 9.8 s | mono | TensedComputerImproviser/corpus/Dovertones.wav | 6.3–16.2 s |
| gtr_corpus_b | 9.8 s | mono | TensedComputerImproviser/corpus/DguitarCorpus.wav | 316.9–326.6 s |
| field_stream_a | 10.8 s | stereo | Soundscaper/git/soundscaper/audio/stream.wav | 11.1–21.9 s |
| field_park_a | 10.4 s | stereo | Soundscaper/git/soundscaper/audio/park.wav | 4.7–15.1 s |
| field_waves_a | 10.8 s | stereo | Soundscaper/git/soundscaper/audio/waves.wav | 27.1–38.0 s |
| field_woods_b | 10.7 s | stereo | Soundscaper/git/soundscaper/audio/woods.wav | 187.1–197.8 s |
| amb_garden_a | 10.3 s | stereo | gameAudio/SP - Sound Effects/ambiances/ambiance_garden.wav | 4.0–14.3 s |
| nox_crystadb4p16 | 11.9 s | stereo | Nox/NOX - Performance Package/max/max/06_crystaDb4+16.wav | first 12 s, trimmed |
| fire_crackle | 12.0 s | mono | gameAudio/SP - Sound Effects/fire/sfx_fire_crackling_loop1.wav | whole |
| bubbles_airy | 9.4 s | mono | gameAudio/SP - Sound Effects/water/water_bubbles_airytexture_1.wav | whole |
| metal_grate | 2.9 s | mono | gameAudio/SP - Sound Effects/Metals/granular/MetalFiles_grate_2.wav | whole |
| stick_snaps | 3.1 s | mono | gameAudio/SP - Sound Effects/sticks/Sticks_snap_*.wav | 6 files end to end, 0.18 s gaps |
| scissors | 1.5 s | mono | gameAudio/SP - Sound Effects/Scissors/Scissors_snip*.wav | 5 files end to end, 0.18 s gaps |

Measured pitch of the pitched picks (autocorrelation, 2026-11-13):
- gtr_corpus_b: D2 −10 c, steady (octave D3 in the second half).
- gtr_overtones_a: harmonics of D2 (7th at C5 −31 c, then the 5th near F#4).
- nox_crystadb4p16: C#/Db +18 c, steady (file name says Db4 +16).
- metal_grate: unpitched.

## Open: root note (proposed, not yet decided)
Proposed: no per-source root field. Tune at ingest by resampling to the nearest C: the two guitar sources −190 c (D → C), the crystal −118 c (→ C). Pitch class is then right at C3; the octave is not (guitar sounds an octave below the key, crystal above), as with the existing `drone`. Alternative: a root field in the factory table (engine pitch mapping change) or in the slot (state-format change).

## Synthetic additions (proposed list, about 19; in `tests/render-harness/factory_gen.cpp`, pitched at C3)
Tonal: fm_bell, glass, bowed, reed, organ, choir, sub, saw_stack, chord_minor, chord_sus. Plucked / struck: kalimba, marimba_bar, metal_plate. Texture: wind, rain_synth, vinyl, shimmer. Rhythmic: clicks (irregular), pulse_triplet.

## Work (proposed three phases, `/improve-milestone`)
1. Pipeline and table: one manifest generates the `GiFactory.h` table, CMake file list and golden sha256; ingest script for samples (48 kHz, level-matched to the existing 8, 16-bit for noisy material); FSP1 over every source, size-budget row, level-spread row.
2. Content: ingest the 13 picks, write the synthetic generators, pin hashes.
3. Picker UI: categories (Tonal, Plucked / Struck, Voice, Texture, Drone, Rhythmic, Field), scroll, type-ahead, wider list; i18n keys for category names (fr / zh); `ui_frontend_check`, check-i18n, lint, three-language fit.
Then backup v1.1.0, build, install, auval, pluginval, CHANGELOG.
