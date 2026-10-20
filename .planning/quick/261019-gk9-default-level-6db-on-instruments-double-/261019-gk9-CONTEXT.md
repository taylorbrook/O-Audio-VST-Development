# Quick 261019-gk9 — Context (locked decisions)

## Request
1. Every instrument has an output-level control at the END of the chain, default −6 dB.
2. Double-clicking a dial opens a manual value-entry box (several currently reset to default).

## Locked decisions (user, 2026-10-19)
- **Presets:** strip output level from factory presets — it becomes user-owned (matches the `feedback_presets_never_set_output_gain` rule). Preset apply must not reset it either (follow the Bells/Formant/simpleWavetable user-owned pattern: `OuariconPresetManager.h` exclusion / PP skip list).
- **Reset gesture:** double-click → value entry. Existing reset-to-default moves to **Alt/Option-click** (Prism convention, capture-phase bind).
- **Gain stages:** Bells + Wind per-voice gain moves to post-FX (true last stage, before any final safety clip/limiter? — gain goes after EQ/FX; a safety clip/NaN scrub may stay after it). Linear params (IntonationPad `masterVolume`, Prism `masterVol`, Strata `masterVol`) keep their ranges; default → 0.501187 (−6 dB). Texture gets a new dB `outputLevel` param (−60..+6 or match family, default −6, smoothed, last stage) + knob. Keep all existing param IDs/ranges (saved sessions must load unchanged).
- **Skip (uncommitted work from other sessions):** O-Bassoon, O-Freeze, O-Gain, O-Reed, O-simpleFM. Do NOT touch these paths. List as follow-ups.

## Census (Part A — instruments, output param)
| Plugin | Param | Unit/range/default | Placement | Presets set it? |
|---|---|---|---|---|
| Bells | outputGain PP.cpp:549 | dB −24..+12, 0 | per-voice BellVoice.cpp:720, BEFORE FX → move | no (user-owned already) |
| Bowed | outputLevel PP.cpp:180 | dB −60..+12, 0 | last, unsmoothed PP.cpp:467 | yes, all 10 @0 dB (PP.cpp:773+) |
| Contrabass | OUTPUT_GAIN PP.cpp:75 | dB −60..+12, 0 | last | yes, 16 presets (PP.cpp:310+) |
| Formant | outputGain PP.cpp:277 | dB −60..+12, 0 | last | no (user-owned) |
| IntonationPad | masterVolume PP.cpp:301 | linear 0..1.26, 1.0 | last | yes, PresetManager.cpp:305+ |
| Lyrica | masterVolume PP.cpp:39 | dB −60..+6, 0 | last | no — check apply resets it |
| Marimba | OUTPUT_GAIN PP.cpp:119 | dB −24..+12, 0 | last | yes, PresetManager.cpp:621 |
| MicrotonalSampler | output_gain PP.cpp:244 | dB −24..+24, 0 | last | no factory presets |
| Prism | masterVol PP.cpp:460 | linear 0..1, 0.8 | last (mod-matrix target too) | yes FactoryPresets.cpp:182, :284 |
| simpleAdditive | outputLevel PP.cpp:134 | dB −60..0, 0 | last | presets reset-to-default → must exclude |
| simpleBeatmaker | outputLevel PP.cpp:114 | dB −60..0, 0 | last | reset-to-default → exclude |
| simpleGrain | outputLevel PP.cpp:179 | dB −60..0, 0 | last | reset-to-default → exclude |
| simplePhysicalModelSynth | outputLevel PP.cpp:157 | dB, −6 already | last | yes FactoryPresets.cpp:95-145 → strip |
| simpleSampler | outputLevel PP.cpp:162 | dB −60..0, 0 | last | reset-to-default → exclude |
| simpleSubtractive | outputLevel PP.cpp:189 | dB −60..0, 0 | last | reset-to-default → exclude |
| simpleWavetable | output_level PP.cpp:164 | dB, −6 already | last | user-owned already — no change |
| Strata | masterVol PP.cpp:505 | linear 0..1, 0.8 | last | implicit (FactoryPresets.cpp:80-88) |
| Texture | NONE (MIX is last) | — | add outputLevel | no presets |
| TextureForge | OUTPUT_GAIN PP.cpp:137 | dB −60..+12, 0 | last, unsmoothed GrainScheduler.cpp:79 | no presets |
| Wind | outputLevel PP.cpp:228 | dB −60..+12, 0 | per-voice FluteSynthVoice.cpp:748, BEFORE FX → move | yes PP.cpp:882+ |
Unsmoothed last-stage gains (Bowed, Lyrica, Marimba, TextureForge): add SmoothedValue ramp while touching them.
UI knob/readout default displays and any JS `PARAMS`/`data-default` default tables must be updated to −6 to match.

## Census (Part B — dial double-click, all plugins)
No shared knob module; each plugin has its own knob code (inline index.html or js/app.js).
- **Already entry (leave, but verify reset is Alt-click):** Prism (index.html:3349, fn :3088), Strata (:3542, fn :3278), Emulator (:1081, fn :962), MultiBandCompressor (app.js:534/542), Polystutter (js/parameter-bindings.js:405, fn :51).
- **Readout-only entry (extend to the dial):** IntonationPad (app.js:1476), Freeze (SKIP).
- **Reset on dblclick → convert:** AnalogEQ :1259, AnalogSaturation :1037, Bass :1277, Bitrot :1723, Bowed :1730, Chorus :1364, Comp :1395, Contrabass :1911/:1960, GrainScatter app.js:199, MicrotonalSampler sampler-app.js:804 (+ trim sliders :3632/:3650), Octagon app.js:341, Orbit app.js:329, ReverseDelay app.js:579, simpleAmbisonics :1528, simpleBeatmaker app.js:359, simpleReverb :1186, simpleWavetable :1574/:1712, Tapestop app.js:304, Texture main.js:334/:443, TextureForge src/app.js:208 (edit src, then `npm run build` → tracked public/js/app.bundle.js). [Bassoon, Gain, Reed, simpleFM = SKIP]
- **Nothing → add:** Detune, DigiDelay, Formant, FreqPulse, simpleAdditive, simpleGrain, simplePhysicalModelSynth, simpleSampler, simpleSubtractive, Tremolo. SpectralShaper: knobs only (its curve dblclick at NodeCurve.js:49 stays).
- **Mixed → unify:** Bells (sliders/footer reset :2366; FX readouts entry :3066), Lyrica (sliders :3554, ref-pitch :4887, FX readouts app.js:649), Wind (:2332, FX :2772), Marimba (ref-pitch reset app.js:785, others nothing).
- Scope = rotary dials/knobs (and the knob-like sliders that currently reset on dblclick, e.g. Bells/Lyrica sliders, MicrotonalSampler trims). Not curves/XY pads/waveform editors.

## Reference implementation (copy the pattern, per-plugin)
**O-Prism `attachValueEntry`** — `plugins/O-Prism/Source/ui/public/index.html:3066-3170`:
inline `<input>` overlaid on the readout (readout text kept, transparent → no layout shift); Enter/blur commit, Esc cancel; untouched text never written back; per-formatter `.parse` (percent, Hz/kHz, ms/s, dB, direct); `scaledToNorm(v, state.properties)` clamps + applies skew from the live C++ start/end/skew; commit = `sliderDragStarted()` → `setNormalisedValue()` → `sliderDragEnded()`; Alt-click reset bound capture-phase (:3308), also wrapped in the gesture pair.
Families C (Bells/IntonationPad/Lyrica/Wind readout entry) use linear parseFloat and ignore skew — upgrade to Prism semantics while unifying.
Resets that skip the gesture pair (Comp, AnalogEQ, TextureForge, Bells/Lyrica sliders) get the pair when moved to Alt-click.
Readouts showing non-numeric text (choice params, "-inf") need sensible parse: `-inf`/`inf` → min; choice/stepped params: snap.

## Repo constraints
- Trunk-based on main, shared checkout with concurrent sessions: path-scoped commits only (`git commit -- plugins/<Name>`), re-check `git branch --show-current` + `git status --short` immediately before each commit. Never `git add -A`.
- `plugins/*/Source/ui/public/modules/` is gitignored generated copies — knob code isn't there anyway; don't edit them.
- Per-plugin: patch version bump (CMakeLists VERSION) + CHANGELOG `## [X.Y.Z]` entry; PLUGINS.md version column — PLUGINS.md is ALSO dirty from another session: do not commit PLUGINS.md; list version updates for follow-up.
- Build with ninja in build/ (background long builds; 600 s watchdog). Install per CLAUDE.md (AU cache clear, dual-variant sweep) — prefer `./scripts/build-and-install.sh <Name>`.
- Never tag.
