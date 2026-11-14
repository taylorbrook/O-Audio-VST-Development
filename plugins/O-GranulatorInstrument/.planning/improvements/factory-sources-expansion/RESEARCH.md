# Research: Factory sources expansion

**Plugin:** O-GranulatorInstrument
**Milestone:** factory-sources-expansion
**Created:** 2026-11-13
**Phase:** Research
**Version:** 1.1.0 → 1.2.0 (MINOR)

Everything marked "measured" was run on 2026-11-13 from the originals with the scripts in `research/` (prototype outputs live in a scratch folder, nothing was written to `Resources/` or `Source/`).

---

## Calls for Taylor (before the plan is approved)

| # | Question | Recommendation |
|---|---|---|
| C1 | **Crystal octave.** The strongest partial is 559.7 Hz (Db5 +17 ct). The 280.4 Hz partial (Db4 +20 ct, the octave the file name gives) is 20 dB below it. CONTEXT says Db4 (61) +18. | Root **Db5 (73) +17 ct**: with 61 the note is heard an octave above the key. |
| C2 | **Guitar Overtones cents.** CONTEXT gives it the Drift root (D2 −10). Its own partials say otherwise: the 7th harmonic sits at 513.9 Hz (D2 +0 ct), the 5th at 368.6 Hz (D2 +7 ct). | Root **D2 (38) +3 ct**. |
| C3 | **Contrabassoon cents.** Re-measured: G#0 −19 ct (autocorrelation median 25.671 Hz over 28 frames; the six strongest harmonics give −16 to −23; the pitch rises about 5 ct over the note). Nothing supports −31. | Root **G#0 (20) −19 ct**, as in CONTEXT. |
| C4 | **Level of the two one-shot collections.** Stick Snaps and Scissors are sparse clicks (crest 34 dB). Capped at −1 dBFS peak they land at −36.0 / −34.5 dBFS RMS; the existing 8 span −18.0 to −28.2. | Accept, and define the level row on peak plus the loud-frame level with a floor 2 dB below `dust` (see Level rule). The alternative is shorter gaps between hits. |
| C5 | **`vinyl` duplicates `dust`.** The existing `dust` recipe is already sparse crackle over band noise. | Swap `vinyl` for **`breath`** (air noise through a resonance at C3, flute-like). This is the one swap CONTEXT allows. |
| C6 | **Picker layout.** 41 rows in one 20 px list is 820 px, taller than the 800 px frame, so a single column must scroll. The Scale picker (`#scale-list`) already solves this with headed columns. | A **headed multi-column panel** (7 groups, 4 columns, about 250 px tall, no scrolling at 41), with `max-height` + scroll kept as the guard. Type-ahead and arrows as specified. |

Guitar Drift stays D2 (38) −10 ct: the dominant partial is the octave (145.5 Hz, D3 −16 ct), the third harmonic sits at A3 −1 ct, the autocorrelation period drifts from −3 to −12 ct; −10 is a fair centre.

---

## Context Summary

- **Core requirement:** 41 embedded factory sources (8 existing untouched, 14 samples, 19 synthetic), each with a category and a root that the Factory picker writes to the stream.
- **Scope:** sources, table generator, ingest script, picker, harness rows, i18n. No preset, parameter or state-format change.
- **Success criteria:** FSP1 over 41 with the 8 old pins unchanged; total ≤ 24 MB, each ≤ 3 MB; every pick writes the root; the seven long textures wrap cleanly; auval and pluginval 10 pass.

---

## Findings

### 1. Size budget (measured)

Samples, cut per `BRIEF.md`, seam applied to the seven textures, 48 kHz:

| id | ch | length | gain | peak | RMS | 16-bit FLAC | 24-bit FLAC |
|---|---|---|---|---|---|---|---|
| guitar_overtones | 1 | 9.90 s | +9.7 dB | −7.2 | −24.2 | 241 233 | 701 262 |
| guitar_drift | 1 | 9.70 s | −2.5 | −11.1 | −22.6 | 174 869 | 527 070 |
| stream | 2 | 10.30 s | +25.9 | −2.6 | −19.6 | 1 364 922 | 2 354 000 |
| park | 2 | 9.90 s | +23.1 | −1.0 | −22.8 | 663 995 | 1 420 096 |
| waves | 2 | 10.40 s | +4.6 | −3.6 | −24.6 | 703 874 | 1 332 769 |
| woods | 2 | 10.20 s | +20.7 | −7.0 | −22.5 | 793 895 | 1 492 789 |
| garden | 2 | 9.80 s | +25.4 | −4.5 | −20.3 | 1 193 074 | 2 134 570 |
| crystal | 2 | 11.88 s | +10.5 | −7.0 | −24.4 | 488 355 | 1 273 877 |
| fire | 1 | 11.50 s | +2.6 | −1.0 | −25.9 | 650 253 | 1 202 873 |
| bubbles | 1 | 8.94 s | +24.6 | −1.0 | −27.3 | 487 169 | 916 826 |
| metal_grate | 1 | 2.87 s | +27.3 | −1.0 | −23.9 | 171 623 | 309 594 |
| stick_snaps | 1 | 3.08 s | +1.2 | −1.0 | −36.0 | 90 119 | 178 858 |
| scissors | 1 | 1.52 s | +4.8 | −1.0 | −34.5 | 35 996 | 60 993 |
| contrabassoon | 2 | 8.93 s | −5.8 | −6.0 | −19.4 | 366 151 | 1 114 447 |
| **total** | | | | | | **7 425 528** | **15 020 024** |

- Existing 8: 3 455 333 B. Synthetic 19: estimated 10.7 MB from the existing byte rates (mono tonal 21–57 kB/s at 24-bit, stereo noise on a 16-bit grid 130–145 kB/s).
- **16-bit samples: about 21.6 MB, inside 24 MB. 24-bit samples: about 29 MB, over.** So all 14 samples ship 16-bit (TPDF dither, fixed seed), not only the noisy ones. Two of the originals are 16-bit already (guitars, contrabassoon).
- Largest single source: stream, 1.36 MB (cap 3 MB).
- The margin is about 2.4 MB and rests on an estimate. The plan should cap the synthetic set at 11 MB and keep stereo noise sources at 8 s or less.
- A 16-bit FLAC decodes exactly onto the 24-bit hash grid (code × 256), so `contentHash` and the full-PCM pin work unchanged.

### 2. Level rule (measured)

Existing 8: tonal sources peak at −12.0 dBFS, RMS −20.3 to −27.1; noise sources RMS −18.0 / −28.2 with peaks to −1.0.

Rule used for the table above: gain so that the 90th-percentile 400 ms frame sits at −20 dBFS, capped so the peak stays at or below −1 dBFS. Twelve samples land at −19.4 to −27.3 dBFS RMS, inside the existing spread. The two one-shot collections do not (C4).

Level row proposal: peak ≤ −1.0 dBFS and loud-frame level in [−33, −18] dBFS for all 41. Existing range of that measure is −30.8 (`dust`) to −18.9 (`vowel`).

Five samples take +23 to +27 dB of gain (stream, park, garden, bubbles, metal_grate). The audition cuts were normalised to −3 dBFS peak, so Taylor heard them at about this gain already.

### 3. Seam (measured)

Fold: drop the last 0.5 s and mix it into the first 0.5 s with sin / cos gains. The file gets 0.5 s shorter and its last sample runs into its first. No end fades on these seven.

Why it matters in this engine: a grain that crosses the top of the loop region wraps mid-grain (`GiRenderer.h:231`). With plain 10 ms fades the wrap passes through silence (a dip); with unfaded ends it would be a step inside a grain (a click). The fold removes both.

| source | centre of the fold, re mean power of the two segments | linear-fade control | step at the wrap / 99.9th pct sample step |
|---|---|---|---|
| stream | +0.5 dB | −2.5 dB | 0.27 |
| park | −1.1 | −4.1 | 0.26 |
| waves | +0.1 | −2.8 | 0.07 |
| woods | +0.7 | −2.3 | 0.07 |
| garden | +0.8 | −2.1 | 0.44 |
| fire | +0.4 | −2.6 | 0.01 |
| bubbles | −0.3 | −3.4 | 0.26 |

Seam row proposal, on the decoded file read circularly: centre 100 ms of the fold within ±1.5 dB of the mean power of the two mixed segments, and the wrap step ≤ the 99.9th-percentile sample step of the body. Negative control: the same cut with a linear fade (worst case −2.1 dB, fails). The control margin on garden is 0.6 dB; if it proves fragile, tighten to ±1.2.

A level comparison against the body median is the wrong metric: bubbles reads +12.9 dB at the wrap because the content itself has a loud event there.

`sfx_fire_crackling_loop1.wav` is authored as a loop; trimming breaks that, so it is folded like the rest.

### 4. Root write

- **Where:** `GranulatorInstrumentProcessor::loadFactorySource`, after `requestFactoryLoad` returns true. Same call pattern as the `rec_arm` auto-clear (`PluginProcessor.cpp:270`): `beginChangeGesture` / `setValueNotifyingHost (convertTo0to1 (v))` / `endChangeGesture`, per parameter. No new native function, so the 36-name pin in `ui_frontend_check.js` holds.
- **Why native rather than from the page:** the acceptance row drives the picker path from the C++ harness, and refusals are decided in the same function.
- **Refusals** (`recording`, `unknown`, `badSlot`): no write. The root changes only when the load is queued.
- **Order:** the write is synchronous, the tape swap follows on the worker. The picker is reachable from an empty lane only, and an empty lane's stream is silent, so nothing is heard at the old tape with the new root. (`loadFactorySource` on a loaded lane, harness only, would shift the old tape for the length of the decode.)
- **Skip when equal:** write only when the value differs. An empty gesture leaves a touch in hosts that are recording automation; the readback is the same either way.
- **Not the swallowed-write trap:** that one is a write to another parameter from inside `parameterChanged`. This is a message-thread call from a native function.
- **Readback:** `root_note` has interval 1 and reads back exactly. `root_fine` has interval 0 over ±100, so −19 returns as a float near −19; the row compares within 0.001 ct.
- **Undo:** two gestures, so two steps in hosts that record parameter undo (Reaper). The slot load itself is not undoable by the host, as in 1.1.0.
- **Page:** the Root fields already follow the parameters through their relays; `renderPlaybackCaps` already reruns on `root_note` (`index.html:3045`).
- **Range:** G#0 = 20 is inside 0…127. Contrabassoon played at C3 is +28.2 st, inside the ±48 clamp.

### 5. Table and names

- `FactoryEntry` gains `name`, `category`, `rootNote`, `rootFineCt`.
- **`factoryDisplayName` cannot name the new set:** `fm_bell` becomes "Fm Bell". The table carries an explicit name; for the old 8 it must equal `factoryDisplayName (id)` (the spelling the 64 presets carry), checked in FSP1. `SlotLoader::requestFactoryLoad` takes the name from the entry.
- The stubs stay last, after the 41 (`kCuratedFactoryCount` 41, stubs at 41 / 42). `ui_gates_31.h` and `ui_gates_32.h` address them by id, not index; FSP1 addresses index 8 and must move.
- One manifest (`Resources/factory/manifest.json`: id, name, category, root, channels, frames, kind, pins) generates three files: the table include for `GiFactory.h`, the CMake source list, `tests/golden/factory-sources.sha256`. A `--check` mode fails on drift.
- **Pins need two builds.** No Python `xxhash` here, and the canonical hash lives in `GiHash.h`. A harness mode (`--pin-factory <dir>`) decodes each FLAC and prints `kHash` / `kFull`; the manifest takes them; regenerate; rebuild. The 8 existing pins printed by the same mode are the proof that it is right.
- `--gen-factory --force` rewrites all 8 old files today. The new generator must write only ids that are missing, and the old 8 are checked against the golden sha256 after every run.
- `tests/golden/factory-sources.sha256` is checked by nothing automatic today. Add it to the generator's `--check`.

### 6. Binary data at this size (measured)

A 41-file stand-in (19.9 MB) through the repo's `juceaide binarydata`: one `.cpp` per resource (largest 4.9 MB of source), 8 s in total to compile at −O3, peak 297 MB of compiler memory. JUCE emits numeric arrays, not string literals, so MSVC's string-length limit does not apply. The Windows arm still only runs at `/publish`; nothing here predicts a failure.

### 7. Categories (proposal)

| Category | Sources |
|---|---|
| Tonal (8) | harmonic, bowed, reed, organ, saw_stack, chord_minor, chord_sus, crystal |
| Plucked / Struck (9) | pluck, bell, fm_bell, glass, kalimba, marimba_bar, metal_plate, guitar_overtones, guitar_drift |
| Voice (2) | vowel, choir |
| Texture (9) | noise_bed, dust, wind, rain_synth, breath (C5), shimmer, fire, bubbles, metal_grate |
| Drone (3) | drone, sub, contrabassoon |
| Rhythmic (5) | pulse, clicks, pulse_triplet, stick_snaps, scissors |
| Field (5) | stream, park, waves, woods, garden |

Source names stay unlocalized (they are lane names, like file names); type-ahead matches on them. Seven category keys in en / fr / zh.

### 8. Synthetic set

Three decaying inharmonic sources sit close together (`bell`, `fm_bell`, `glass`). They stay distinct only if the recipes are held apart: `bell` as it is (6 s, hum + partials); `fm_bell` a two-operator FM tine with a decaying index (bright attack, near-sine tail); `glass` a short struck glass (2 s, sparse high partials 2.32 / 4.25 / 6.63, no hum). `kalimba` (thin, one strong overtone near 5.4×) and `marimba_bar` (4× and 10×, fast decay) differ from `pluck` (harmonic string) by partial series. If two still sound alike after rendering, execute reports it rather than swapping silently.

### 9. Pitch row

"Sounds at the key's pitch" needs a stated estimator per source, since the four have different strongest partials: contrabassoon by autocorrelation period, crystal by its strongest partial, guitar_drift by its 2nd harmonic / 2, guitar_overtones by its 7th harmonic / 7. The manifest carries the reference partial. The row picks the source through `loadFactorySource`, plays C3 with long grains and no spray, and requires the reference partial within ±10 ct of k × 130.81 Hz. Control: the same render with the root left at C3 fails. `engine_gates_21.h:1189–1244` is the model.

---

## Affected Components

| File | Change | Complexity |
|---|---|---|
| `Source/dsp/GiFactory.h` | entry fields, generated table include, count 41 | Medium |
| `Source/dsp/SlotLoader.cpp` | name from the entry | Low |
| `Source/PluginProcessor.cpp` | root write in `loadFactorySource` | Low |
| `Source/PluginEditor.cpp` | `getFactorySources` adds category | Low |
| `Source/ui/public/index.html` | grouped panel, type-ahead, column keys | Medium |
| `Source/ui/public/js/i18n.js` | 7 category keys × 3 languages | Low |
| `CMakeLists.txt` | generated source list, budget comment | Low |
| `tests/render-harness/factory_gen.cpp` | 19 recipes, write-missing-only, `--pin-factory` | High |
| `tests/render-harness/preset_gates_4.h` | FSP1 over 41, budget, level, seam, root, pitch rows | Medium |
| `tests/ui_frontend_check.js`, `tests/i18n-states.json` | picker pins, states | Low |
| `CHANGELOG.md`, `PLUGINS.md` | 1.2.0; note that a 1.2.0 session with a new source shows that lane as missing in 1.1.0 / 1.0.0 | Low |

New: `Resources/factory/manifest.json`, 33 FLAC files, `tools/factory_ingest.py` (from `research/ingest_proto.py`), `tools/factory_table.py`, the generated table include.

No JUCE API beyond what 1.1.0 uses.

---

## Complexity and risk

**Overall: Medium-high**, by volume (19 recipes, 33 assets, 6 new rows), not by novelty. No DSP change in the engine, no parameter change, no state-format change.

1. **Budget margin of 2.4 MB rests on an estimate.** Mitigation: the budget row from the first content task on; an 11 MB cap for the synthetic set.
2. **The old 8 change by accident** (generator rerun, table reorder). Mitigation: write-missing-only, the sha256 check, FSP1 with the old pins as literals.
3. **Samples are not reproducible from the repo** (originals live under `/Users/taylorbrook/Projects`). The committed FLACs are the source of truth; the ingest script and the manifest record the windows.
4. **Picker keyboard path in columns.** Mitigation: flat order for Up / Down and type-ahead, Left / Right by column, gated in `ui_frontend_check.js`.

---

## Domain Detection

DSP (generators, harness rows) and GUI (picker, i18n) both score. **Mixed → general-purpose** execute agent. Version bump: **MINOR**; parameter ids, ranges and preset format unchanged.

---

## Next Phase

Plan: task breakdown in three waves (pipeline and table; content and pins; picker and rows), with C1–C6 settled first.

---

*Generated by improve-milestone research phase*
