# O-GranulatorInstrument — Stage 4 (Polish / Validation) — RESEARCH

**Date:** 2026-11-11 · **HEAD:** `804872fb` · **Input:** `stages/4-polish/CONTEXT.md` (D51–D59, open questions 1–9)
**Method:** three read-only parallel slices: A presets / factory sources / `output_gain` (Q1, Q3, Q4, Q5); B CPU / auval / D55 carried fixes (Q2, Q6, D55a–c); C CI / fork drift / human session / release (Q7, Q8, Q9, D54, D56). Nothing in the repo was built or edited; auval was run on the installed dev component. Tags: **V** / VERIFIED = file:line, SHA or probe this session; **A** / ASSUMED otherwise. Paths are relative to `plugins/O-GranulatorInstrument/` unless prefixed.

## Summary (what the plan must absorb)

1. **Bank = 8 × 8, O-Granulator precedent** (`plugins/O-Granulator/Source/FactoryPresets.h`). Names are `"<Cat> · <Name>"`, ≤ 24 chars, not localized, in a flat menu. Categories: Keys, Pads, Textures, Drones, Rhythmic, Motion, Expressive, Live. Authoring: a C++ table in engineering units with a `convertTo0to1` pass, a fixed int64 seed per preset, and slots as `{index, source, name}` with **no `contentHash`**. With a hash, a load onto an identical source is a no-op that keeps a feedback-dirtied tape. Live presets list no slots (D45).
2. **8 generated sources:** 48 kHz / 24-bit, every pitched source at **C3** (slots have no root metadata; the default `sN_root_note` is 48), noise beds at 16-bit resolution, about 3.9 MB total (gate ≤ 8 388 608 B; any one source ≤ 3 MB for the MSVC binary-data envelope).
   - The generator needs `fp contract(off)` and integer-only RNG: arm64 FMA contraction differs from x86_64.
   - Pins gain a **full-PCM XXH64**, because `contentHash` covers only the first 131 072 frames.
   - The two stubs stay as **harness-only fixtures** (`OGI_TEST_HOOKS` table tail + a second binary-data target), so the 21 carried rows and the UI-stub golden stay unchanged.
3. **`output_gain` 7.15e-07 is arm64 FMA inside `NormalisableRange::convertFrom0to1`, not a stepped-range snap** (the interval is already 0). The same defect hits `rec_gain` and `s1`–`s4_level` (1.8e-06).
   - Audio is already exact (`ControlGrid::snapDb`), so renders cannot move.
   - Fix: a snap-only range (clamp, then |v| < 1e-5 → 0) that keeps start / end / skew visible to the WebView.
   - Param-dump delta = `defaultText` cells only. No session migration (restore passes through the snap). No preset interaction.
   - **Sign-off needed:** apply it to all 6 rows (recommended) or only `output_gain` (D55 as written).
4. **CPU over presets = a new `--cpu4` background mode** reusing `measure25` unchanged.
   - It runs the `--cpu26` rows first (regression + session factor).
   - Then, per preset: T = the 6-note C25tm chord ≤ 12 % and W = 16 held notes ≤ 40 %, **gated per preset**. Bank median / p90 / max are INFO.
   - Sweep at 5 × 6 s. A borderline result or NOVERDICT re-runs 7 × 10 s.
   - Every preset session must call `shipping()` and wait for the loader to go idle before timing.
5. **auval: 4 warnings = LFO 1–4 Rate**, a 1-ulp normalised round-trip drift at 0.5 Hz with skew 0.3. Every fix moves the LOCKED spec. → **Document**, plus harness row AU1 freezing the drift set to exactly {lfo1..4_rate} (with a control). AU1 must run after the Q5 range change, to prove the snap ranges add no 5th member.
6. **D55 carried fixes:**
   - (a) Emit `wavFailed` in `slotInfoToVar`; the page edge-toasts `toast.wavFailed` on the slotRev pull. This avoids the audio thread, so it works while the host idles.
   - (b) A batch-scoped ascending toast chain at 1600 ms steps inside `noteSlotErrors`. Not a global FIFO, which would break ST1 timing.
   - (c) The WAV write moves to a detached writer with a shared payload and an abandon flag, so `stopThread(-1)` is bounded at about 100 ms. `stopThread(timeout)` kills the thread and can leave `saveLock` held. Row USV3 with hook `testHoldWrite`.
   - One new i18n key, `toast.wavFailed` (states 46 → 47).
7. **Harness must stop writing the real home.** `initializeFactoryPresets` writes `~/Library/O-GranulatorInstrument/Presets/Factory`. Port O-Granulator's `CFFIXED_USER_HOME` re-exec, and capture the real home first for GD1 and the overrides hygiene check.
8. **Factory bank goes stale at a fixed version.** `.factory-version` is keyed on the version string, and init never deletes. → A plugin-side bank-hash stamp that clears `Factory/*.json` on mismatch, plus a hands-on setup step that wipes the dir.
   - **Factory-dir overwrite hole:** `savePresetWithDialog` (`PluginEditor.cpp:1075-1079`) needs an `isAChildOf(factoryDir)` refusal.
9. **CI:** the release workflow configures this plugin cleanly (no FetchContent; WebView2 static link + `withBackend` + `withUserDataFolder` present). However:
   - The Standalone is never shipped.
   - CI is the first x86_64 and MSVC compile.
   - **None of this plugin is on `origin/main`** (185 commits ahead).
   - `ci-tests.yml` only accepts O-Octagon / O-Strata.
   - → Pre-tag Windows proof = push main + `workflow_dispatch build-and-release.yml validate_only=true`. **Sign-off needed** (repo-wide push).
10. **Fork drift:** one parent commit since the pin, `e77d29c8` (1.2.0 value entry, UI only, already ported at 3.1 D41). Zero module drift. → One record-only line in FORK-MANIFEST.
11. **`/publish` always bumps.** The handoff must say "publish 1.0.0 as-is, no bump". The CHANGELOG has exactly one `## [1.0.0] - date` heading. `PLUGINS.md:76` is stale (`🚧 Stage 1 | 0.1.0`). `modules/registry.yaml` (7 × 0.1.0) has another session's uncommitted hunks: bump via own-hunk staging or defer.
12. **HANDS-ON-SESSION-4:** 146 carried rows plus new rows, ordered Standalone → 64-preset audition → Logic AU → Live VST3 → Deferred. Stale DevLoader-era / "old patches" rows in 2.1 / 2.2 / 2.4 / 2.6 must be rewritten or marked N/A first. Every ❌ is routed to a Stage 4 gap wave (blocker / known issue / stale row). The version stays 1.0.0.

## Cross-slice reconciliation

| Topic | Slices | Resolution |
|---|---|---|
| Harness HOME | A pitfall 1, B pitfall 3 | One temp-HOME re-exec serves PR5 (real factory write path) and `--cpu4`. `--cpu4` additionally loads in-repo JSON via `loadPresetFromFile` and never calls `initializeFactoryPresets` itself. |
| Stale factory bank | A pitfall 2, C P3 | Both: bank-hash stamp in the plugin (A decision 3) + the hands-on setup wipe (C §Q9-0). The stamp also covers fix-wave preset edits at an unchanged 1.0.0. |
| Source size | A Q4, C binary data | ≤ 8 388 608 B total, any single source ≤ 3 MB (C), recipe estimates max ~1.0 MB (A). FA3 gates both. |
| Version bump timing | C decision 2, A stamp | VERSION 1.0.0 in the wave that lands the bank; with the stamp, later bank edits still refresh. |
| `output_gain` range vs AU1 / WebView | A Q5, B Q6 | The snap-only range uses `nullptr` convert functions, so it is not the banned lambda-mapped range. AU1 runs on the final tree and must still read {lfo1..4_rate}. |
| Preset switch click | A PR6, C Logic / Live rows | PR6 (CK6 metric, NC `swapKillHard`) is the automated arm; switching while holding a chord in Logic + Live is the human arm (mid-apply tearing risk, A pitfall 7). |
| New strings | A D59, B D59 | Total new keys = 1 (`toast.wavFailed`); one copy fix (`lane-embed` help: "session and presets" → session only, EN / FR / zh); PN1 preset-name fit gate (port of O-Granulator `preset_name_fit.js`). |

## Sign-offs for the plan (user decisions)

1. **`output_gain` fix scope:** all 6 same-cause params (recommended), or `output_gain` only.
2. **Windows CI proof:** push main (publishes ~185 commits from all sessions) + validate-only dispatch, at a checkpoint after the release-ready commit. Fallback: "Windows CI pending `/publish`".
3. **auval warnings:** document + AU1 (recommended), rather than a LOCKED-spec default move.
4. **Stubs:** keep as harness-only fixtures (recommended), rather than repointing 21 rows.

---

# Part A — Presets, factory sources, output_gain (Q1, Q3, Q4, Q5)


Scope: CONTEXT open questions Q1, Q3, Q4, Q5, plus D59 strings for this slice. Read-only pass on `main`. Paths are relative to `plugins/O-GranulatorInstrument/` unless they start with `modules/`, `scripts/` or `~/JUCE`.

### A · Q1 Preset bank design (64)

**Suite precedent: O-Granulator 1.1.0.** VERIFIED in `plugins/O-Granulator/Source/FactoryPresets.h:30-45`:
- 64 presets, 8 categories of 8 each.
- Names are `"<Category> · <Name>"`, at most 24 characters. The module has no categories, so the case-insensitive sort groups them.
- Values are in engineering units and pass once through `convertTo0to1`.
- Every preset has a fixed seed. `output_gain` is never set. Descriptions are documentation only.
- Validator `validateSpec` (`:847-861`); defs built by `makeFactoryPresets` (`:876-891`), written by `initializeFactoryPresets` in the ctor (`O-Granulator/Source/PluginProcessor.cpp:193`).

**Names are not localized.**
- O-Granulator: VERIFIED, `tests/preset_name_fit.js:27` ("the names are not localised").
- This plugin: VERIFIED. `Source/ui/public/js/i18n.js:37-38` says "preset names, file / recording names … are not localized". The menu sets `dataset.userText` and `textContent = name` (`index.html:4706-4708`).

**The module is flat.** VERIFIED in `modules/persistence/preset-manager/cpp/OuariconPresetManager.h`:
- `getPresetList` lists `Factory/*.json` then `User/*.json` and sorts (`:580-607`).
- `FactoryPresetDef` = `{ name, map<id, normalized float>, customState }` (`:240-245`).

**The page is flat.** The menu is a 300 px list of `.pm-item` buttons (`index.html:141-147, 4700-4712`). There are no category headers. v1 is locked to defects only (D40/D48), so do not add headers.

**Preset bar geometry is identical to O-Granulator.** VERIFIED: `#preset` 300 px, `.pname b` 14 px nowrap + ellipsis, `.save` 52 px (`index.html:129-140` vs `O-Granulator/.../index.html:128-140`). So the ≤ 24-character rule carries over.

**Engine features to exercise** (spec, VERIFIED `parameter-spec.md:54-198`):
- 4 streams (stream N reads slot N, `Source/dsp/GiConstants.h:39`).
- Scan rate and scan modes Free-run / Retrigger / Ping-pong, plus the loop region.
- Key track / root note.
- Per-grain filter, drive, crush, ring.
- Feedback (convex write) and freeze; reverb.
- 4 LFOs (global / per-voice, sync); 2 mod envelopes; 16-slot matrix with 18 sources (velocity, key, note random, mod wheel, pressure, slide, bend, macros 1–4).
- MPE; Poly / Mono / Legato + glide.
- Tempo `sync_mode`; 32 scales + custom scale + degree masks.
- Record-in (`rec_target / trigger / length / length_sync / threshold / gain`).
- `grain_limit`.

7-Limit Just (idx 25) is safe here. The fork insertion-sorts the active array (`Source/dsp/forked/ScaleQuantiser.h:43-45`), VERIFIED.

**Recommended table (8 × 8 = 64).** Names are `"<Cat> · <Name>"`, ≤ 24 chars, middle dot U+00B7. Sort order: Drones, Expressive, Keys, Live, Motion, Pads, Rhythmic, Textures.

| Category | n | Sources | Features each category must cover (PR0 checks coverage) |
|---|---|---|---|
| Keys | 8 | pluck, bell, harmonic | Poly, key track 100 %, short amp env, `vel_to_amp`, percussive window, small spray. Realism-titled presets ("Bell", "Celesta") get `pitch_rand` 0 and no pitch LFO. |
| Pads | 8 | harmonic, vowel, drone | 2–4 streams with octave / fifth `sN_pitch`, long attack / release, LFO→position, reverb 30–60 %. Scale-quantised `pitch_rand` (Pentatonic, 5-Limit Just); 2 presets with custom scale + masks via `custom` / `masks`. |
| Textures | 8 | noise_bed, dust, vowel | High density, spray 100, BP filter + `filter_rand`, `gain_rand`, `reverse_prob`, crush + `crush_prob`, drive. |
| Drones | 8 | drone, harmonic, noise_bed | `key_track` 0–50 %, slow `scan_rate`, Ping-pong, feedback 30–80 % + `fb_diffusion`, 1 with freeze, long release, S&H LFO on filter. |
| Rhythmic | 8 | pulse, pluck | `sync_mode` Tempo, `density_sync` / `size_sync`, jitter 0, Retrigger scan, synced LFOs. PR3 checks tempo lock at 120 and 93 bpm. |
| Motion | 8 | mixed | Free-run shared playhead, negative scan, narrow loop, ≥ 6 matrix routes, per-voice LFOs, ring + crush. Macros 1–4 routed. |
| Expressive | 8 | harmonic, vowel, bell | 4 × MPE (`mpe_enable`; Pressure→level / filter, Slide→position / cutoff, Bend), 2 × Mono / Legato + glide, 2 × velocity / key / note random / mod wheel. |
| Live | 8 | **no `slots` list** (D45 keeps the user's lanes) | Record-in templates: Manual / Threshold / Next Note, `rec_length` vs `rec_length_sync` (Tempo), `rec_target` 1–4, `rec_gain`, feedback, freeze. "Live · Init" = full parameter + scale reset that keeps the lanes. |

### A · Q3 Preset proof rows

**What apply already does.** VERIFIED:
- The manager resets every non-excluded parameter to default, meta parameters first, then applies the values with validated gestures (`OuariconPresetManager.h:403-432`).
- Excluded: `output_gain`, `rec_arm` (`PluginProcessor.cpp:116`). `setValidatedApply(true)` (`:117`).
- `customLoad` runs on every apply (`OuariconPresetManager.h:441-442`).
- `applyPresetCustomState` resets non-parameter state first (`PluginProcessor.cpp:975` → `resetNonParameterState`, `:780`), except:
  - the **seed**, which is kept unless the preset has one (`:774-787`);
  - the **slots**, where only listed slots are restored (`clearUnlisted = false`, `:977-983`).

**Trap: identical-source no-op.** VERIFIED `SlotLoader.cpp:1385-1404`. When a listed slot has the same source and a matching `contentHash`, restore is a no-op: the working copy (dirtied by feedback) and the old lane name stay. Without `contentHash` in the entry, it re-resolves: new generation, pristine decode, swap (`:1406-1430`).
- → Author factory slot entries as `{index, source, name}` only, with **no hash**, so every preset load starts from pristine tapes.
- Side effect: a user copy saved from a factory preset carries the hash (`PluginProcessor.cpp:950-951`), so it behaves differently on identical sources. That is acceptable, but PR5 must start from a fresh instance.

**`fsTouches`.** VERIFIED. A test-hook counter (`SlotLoader.h:159`), bumped at every disk-touching resolve site (`SlotLoader.cpp:938, 1111, 1195, 1257`). Factory resolve = rung 4, decode from memory (`:1328-1349`). Existing row FA2 / FA2n (`tests/render-harness/engine_gates_26b.h:649-667`). The preset JSON read is not counted.

**Proposed rows** (prefix `PR`, unused here; FA3+ extends `FA`):
- **PR0 table.** 64 presets, 8 per prefix, unique names and seeds, ≤ 24 chars, no `/ \ :`, no `output_gain` / `rec_arm` key, every value in range, every preset seeded, every `factory:` id exists in `GiFactory`. Feature coverage per the Q1 column.
- **PR1 apply from fresh** (`applyPresetData`, no disk).
  - Engineering read-back == table, using the interval/snap tolerance; absent == default.
  - `output_gain` set to −7 dB before the load is kept.
  - Listed slots: `loaded`, `resolvedBy == 4`, hash == pin. `fsTouches == 0`.
  - **PR1n:** `s1_size` (skew 0.3) authored as a linear fraction fails read-back.
  - **PR1n2:** a `file:` slot gives `fsTouches > 0`.
- **PR2 audible / bounded** (O-Granulator PR2 precedent).
  - 20 s per preset on a fixed note script: finite, bounded, RMS > −60 dBFS.
  - Loudness spread ≤ 9 dB. Balance by voicing, never `output_gain`.
  - Live presets: the harness preloads a factory source into lanes 1–4 first.
- **PR4 load after dirty, bitwise.** Before any audio, on instance 1:
  - randomise all 259 non-excluded parameters;
  - set a custom scale and 3 masks; set seed X;
  - load foreign `file:` sources into unlisted lanes;
  - then apply B.

  Pass: 10 s render == fresh-apply-B render, bitwise. Use 320 ordered pairs (O-Granulator `engine_gates_4b.h:386-427`) and require non-vacuity (64 fresh renders all distinct). Bitwise only holds if a preset whose lanes are unlisted keeps those streams disabled. **PR0 must check: every enabled stream has a listed slot** (Live is exempt; it uses a fixed preload).
- **PR4b load after play.** Play A for 5 s with feedback 80 %, all notes off, apply B, `waitForIdle`. Pass:
  - parameter + customState snapshot == fresh B;
  - every listed slot `dirty == false` and generation bumped.

  Not bitwise: reverb tail and global LFO phase carry over by design.
- **PR5 real path.** Uses a temp HOME (O-Granulator `engine_gates_4b.h:429-483`).
  - The ctor writes 64 files + `.factory-version`; `loadPreset` read-back == table.
  - Save as user → reload on a fresh instance: parameters and customState equal, 10 s render bitwise == factory apply.
- **PR6 no click on switch while playing.** Reuse the CK6 metric (`engine_gates_26b.h:845-915`):
  - Subject: A with 2 held notes; apply B at 1.0 s inside `s.at(T, …)` (+ `waitForIdle`, like CK6 mode 3). References: A only and B only.
  - Pass: max |d2| over [0.9, 1.3] s ≤ 2.0 × the larger reference; local κ is INFO (S6 decision).
  - NC: `swapKillHard`.
  - About 8 pairs: same source with different parameters; source change; Poly→Mono; MPE on→off; reverb 60 %→0; Tempo→Free; feedback 80→0; `grain_limit` 256→64.
- **FA3 budget and pins.** Σ FLAC bytes ≤ 8 388 608. Each source decodes; channels / rate / frames match the recipe. **The full-PCM hash** matches its pin (see Q4: `contentHash` covers only the first 131 072 frames). Ids resolve.

**How presets are authored.** VERIFIED:
- Hand-written C++ table `Source/FactoryPresets.h`, a port of the O-Granulator struct plus a `slots` list.
- Engineering units: choice INDEX, bool 0/1, Hz, ms, dB, %, `modDest("id")` helper. Converted once via `rp->convertTo0to1`.
- The module can only store normalised values (`OuariconPresetManager.h:425-429`), so "store denormalised" means the source table, not the JSON.
- `customState` = `{seed: "<int64 string>", custom?, masks?, slots: [...]}`. The seed must be a string (`PluginProcessor.cpp:962-971`).
- Slot `index` is 0–3 (`:861-874`).
- Skew-0.3 parameters (`glide`, `sN_size`, `sN_density`, `lfoN_rate`) and setSkewForCentre ones (cutoff, ring, envelopes, `fb_*`) are why linear fractions are banned (`pattern_factory_preset_normalized_ignores_skew`).

### A · Q4 Source synthesis

**Current stubs.** VERIFIED (`GiFactory.h:47-50`; `metaflac`: 48000 Hz / 24-bit / vendor `reference libFLAC 1.4.3 20230623`, no tags):

| Stub | Format | Bytes |
|---|---|---|
| `stub_tone` | mono, 48 kHz, 3 s, 220 Hz | 124 716 |
| `stub_noise` | stereo, 48 kHz, 3 s | 777 546 |

- Generator: `tests/render-harness/main.cpp:1170-1262` (`encodeFlac24` quality 5, own quantiser, refuses to overwrite without `--force`).
- Table: `GiFactory.h:63-89`, explicit symbols, `factoryCount()` hard-coded to 2. FA1 hard-codes `== 2` (`engine_gates_26b.h:634`).

**Measured rates** (flac 1.5.0 `-5` on numpy-synthesised PCM, scratch probe; the stub-like tone is within 2.4 % of JUCE's 124 716 B):

| Material | Rate |
|---|---|
| Mono tonal 24-bit | 26–45 KB/s |
| Stereo LP noise 24-bit at −18 dBFS RMS | 255 KB/s |
| Same at −30 dBFS | 232 KB/s |
| Same at **16-bit resolution in the 24-bit container** | **162 KB/s** (FLAC wasted-bits) |

Measured with CLI flac 1.5.0. JUCE's vendored libFLAC 1.4.3 is ASSUMED to behave the same.

**Root note.** VERIFIED. A slot has no root metadata: `rootNoteHint` is reserved and never written in v1 (`parameter-spec.md:325`). Pitch is per stream: `sN_root_note` interval 1, default **48 (C3)** (`GiParamIds.h:277`; spec `:104`), plus `sN_root_fine`. Mono sources become L = R at tape build (`SlotLoader.cpp:481`).
- → Synthesise every pitched source with its fundamental (bell: its prime) at **C3 = 130.8128 Hz**. Then a fresh lane plays in tune at the default root.
- Presets still set `sN_root_note` explicitly.
- `stub_tone` (220 Hz = A3) is 9 st off at the default root.

**Recommended 8 recipes** (48 kHz, 24-bit; ≈ 3.9 MB total, half the budget):

| Id | Content | Channels / length | Est. size |
|---|---|---|---|
| `harmonic` | 24 additive partials, slow per-partial amplitude LFOs | mono, 8 s | ~360 KB |
| `pluck` | KS-like decaying partials | mono, 3 s | ~80 KB |
| `bell` | inharmonic partials 1 / 2 / 2.76 / 5.40 / 8.93, decay | mono, 6 s | ~200 KB |
| `vowel` | pulse train through 3 formant BPs morphing a→o→i | mono, 8 s | ~400 KB |
| `noise_bed` | pink-ish noise with a slow spectral sweep | stereo, 6 s, **16-bit resolution** | ~1.0 MB |
| `dust` | sparse crackle + band noise | stereo, 6 s, 16-bit resolution | ~0.6 MB |
| `drone` | C2 / G2 / C3, decorrelated L/R detune | stereo, 12 s | ~1.0 MB |
| `pulse` | 120 bpm, 2 bars, kick-sine drop + noise hat | mono, 4 s | ~150 KB |

- Level targets: tonal −12 dBFS peak; noise −18 dBFS RMS.
- Lengths stay under the 30 s slot max.
- Budget wording conflict: "≤ 8 MB total" (CONTEXT; `CMakeLists.txt:164`) vs "each ≤ 8 MB" (`parameter-spec.md:333`). Gate the total.

**Determinism**
- Shipping determinism is guaranteed by checking in the FLACs; FLAC decode is exact. VERIFIED (RESEARCH-2.6 §3).
- Regeneration byte-identity is claimed **on mac arm64 only** (2.6 precedent, `tests/golden/factory-stubs.sha256`).
- **Pitfall, VERIFIED by probe:** clang arm64 contracts `a + b*c` into FMA by default, x86_64 does not. The same expression gave 7.15e-07 vs 0.
  - → Put `#pragma clang fp contract(off)` at the top of the generator's namespace (or move it to its own `.cpp` with `-ffp-contract=off`).
  - → Use only integer RNG (splitmix64 / the existing `eg21::hashNoise`), never `std::*_distribution` (implementation-defined across stdlibs, ASSUMED).
  - → Use phase accumulators in double; no `std::pow` in inner loops.
  - libm `sin` / `exp` are not correctly rounded across platforms (ASSUMED). After 24-bit quantise a flip is rare, but that is another reason to keep it mac-only.
- **Pin gap, VERIFIED:** `contentHash` hashes only the first `min(frames, 131072)` frames (`GiHash.h:44-46, 60, 283-288`). The FA1 pin therefore covers ≤ 2.73 s. The stubs (3 s) are already partly unpinned. → Add a full-length XXH64 pin per source (`kFull…`) and check it in FA1 / FA3.

**Test fixtures.** 21 harness references use `stub_tone` / `stub_noise`: RL5, FA1 / FA2, CK, UPM6, `ui_gates_31/32`, and `dumpUiOverrides`, whose overview peaks feed `tests/ui-stub/generic-overrides.json:5174`. → Recommend keeping the two stubs as **harness-only fixtures**:
- move them to a second binary-data target linked only to `O-GranulatorInstrument-render-harness`;
- append them in `GiFactory.h` under `#if OGI_TEST_HOOKS`.

Carried rows then stay byte-identical, and the shipping table holds only the 8 curated ids. Repointing the rows instead moves content-dependent rows and the UI-stub golden.

### A · Q5 output_gain exact 0 dB

- **Code** (VERIFIED `GiParamIds.h:231`): `floatDef("output_gain", "Output Gain", -24, 12, 0)`, so skew 1, centre 0, **interval 0** (`:88-89`). That matches spec `:62` / `:36`.
- The memory pattern "stepped range + negative start" does **not** apply. The interval is already 0.
- **Real cause** (VERIFIED, probe + JUCE source):
  - `getDefaultValue()` = 0.6666667f.
  - `NormalisableRange::convertFrom0to1` returns `start + (end - start) * proportion` (`~/JUCE/modules/juce_core/maths/juce_NormalisableRange.h:172`). Clang arm64 fuses that into an FMA, which gives **7.15255737e-07**.
  - x86_64 (Rosetta) gives 0. `-ffp-contract=off` gives 0.
  - An interval makes it worse: 0.01 → −5.36e-07, 0.1 → 3.58e-07.
- **Same defect class on 6 rows**, from `.planning/params.tsv` (VERIFIED):

| Row | Parameter | `defaultText` |
|---|---|---|
| 13 | `output_gain` | `0.0000007` |
| 185 | `rec_gain` | `0.0000007` |
| 30 / 68 / 106 / 144 | `s1`–`s4_level` | `0.0000018` |

  UI-stub defaults show the same: `output_gain` 7.152557373046875e-7, `s1_level` 1.788139343261719e-6.
- **Audio is already exact.** `ControlGrid::snapDb` maps \|dB\| < 1e-3 to 0 (`Source/dsp/forked/ControlGrid.h:139-145`). This feeds output gain (`Engine.cpp:130, 1046`), levels and `rec_gain` (`GiRecorder.h:50, 209`). → **Renders cannot move.** Re-run them to prove it.
- **Fix: snap-only range.**
  - Build these 6 with `juce::NormalisableRange<float>(lo, hi, nullptr, nullptr, snap)`, where `snap = clamp to [s, e], then |v| < 1e-5 → 0`. The clamp is required: a custom snap replaces JUCE's clamp (`juce_NormalisableRange.h:188-197`).
  - Every setter goes through `RangedAudioParameter::convertFrom0to1` → snap (`juce_RangedAudioParameter.cpp:54-58`; `AudioParameterFloat::setValue` / `getText` at `.cpp:98, 101`). That covers host reset, UI double-click, session restore, preset reset pass and `getParameterDefaults` (`GiUiJson.h:427`).
  - The WebView sees the same start / end / skew / interval (`juce_ParameterAttachments.cpp:297-311`), so this is not the banned lambda-*mapped* range. It still needs a spec addendum (convention `:36`) and an update to the `GiParamIds.h:39-44` comment.
  - Remaining wrinkle: the APVTS raw atomic is seeded **unsnapped** at construction ("should not be snapped", `juce_AudioProcessorValueTreeState.cpp:79-81`). It reads 7.15e-07 until the first set. `get()` is exactly 0 (ctor `value(def)`, `juce_AudioParameterFloat.cpp:45`). Harmless (the DSP snaps); document it.
- **Param-dump delta** (ASSUMED until run): only the `defaultText` column changes, to `0.0000000`, on row 13 (D55 scope) or on all 6 rows (class fix). `defaultNorm` 0.666667 / 0.909091, `textAtMin` / `textAtMax` and `numSteps` 2147483647 stay unchanged. Also regenerate `generic-overrides.json` (defaults go to 0) via `--dump-ui-overrides`. No harness row asserts the old values (grep: 0 hits).
- **Migration: none needed.** VERIFIED:
  - Restore calls `setDenormalisedValue` → `setValueNotifyingHost(normalise(v))` → snap (`juce_AudioProcessorValueTreeState.cpp:101-105, 166-177, 413`).
  - Old sessions holding 7.15e-07 (or 0, which today reloads as 7.15e-07) land on exact 0. The normalised mapping is unchanged, so automation does not shift.
- **Presets.** `output_gain` is never saved or applied (`OuariconPresetManager.h:342, 409, 418`), so there is no interaction. With the class fix, the reset pass gives `sN_level` / `rec_gain` exact 0 instead of 1.8e-06 / 7.15e-07, which is what PR1's "absent == default" needs.

### A · D59 strings

- **This slice adds 0 new i18n keys** if the preset menu stays flat. Preset names and factory source / lane names are user text and never localized (`i18n.js:37-38`; spec `:320`).
- **One copy defect to fix:** `lane-embed` help says audio is embedded "in the session and presets" (`i18n.js:399`; fr `:401`; zh `:403`). Presets carry no blobs (PD1; `PluginProcessor.cpp:940`). Edit EN, FR and zh-Hans; reset the `reviewed` flags per suite rules.
- **Gates:**
  - `scripts/check-i18n.js`, `i18n-fr-lint.js`, `i18n-zh-lint.js`, `check-ui-labels.js`, `boot-all-uis.js` (the five UI gates), plus `tests/i18n-states.json`.
  - Plus a port of O-Granulator `tests/preset_name_fit.js`, named **PN1 / PN1n** (`PF` is taken in this harness). It checks that every name fits `#preset-name-text` and the 300 px `.pm-item` in EN / FR / zh-Hans, and that a 40-char name overflows. Nothing currently measures runtime user text.

### A · Pitfalls

1. **The harness writes the real bank.** `initializeFactoryPresets` in the ctor writes `~/Library/O-GranulatorInstrument/Presets/Factory` (`OuariconPresetManager.h:307-319, 702-755`). This harness has no HOME redirect.
   - → Port the `CFFIXED_USER_HOME` re-exec (O-Granulator `main.cpp:877-905`).
   - Capture the **real** home first: GD1 (`engine_gates_26b.h:1072-1076`) and the `dumpUiOverrides` hygiene check (`main.cpp:1526-1532`) read `userMusicDirectory` / `userHomeDirectory` and would go vacuous.
   - Construct one processor before the render pools (avoids the WR-04 write race).
2. **Stale factory files.** The sentinel is the version string (`:713-716`), and init never deletes files. At a fixed 1.0.0 an edited bank never reaches `~/Library`, and renamed presets leave undeletable orphans (`isFactoryPreset` returns true).
   - → Plugin-side: keep a bank-hash stamp; on mismatch, delete `Factory/*.json` + `.factory-version` before init.
   - Hands-on step: wipe the Factory dir.
3. **Overwrite hole in the editor.** `savePresetWithDialog` sends any non-User directory to `savePresetToFile` (`PluginEditor.cpp:1075-1079`). Navigating into `Presets/Factory` overwrites a factory JSON. That becomes reachable once the bank exists (`pattern_savepresettofile_has_no_factory_guard`). → Refuse `isAChildOf(getFactoryPresetsDirectory())`.
4. **Identical-source no-op** keeps dirtied tapes and old names when the preset slot carries `contentHash` (Q3). → No hash in factory entries.
5. **Seed** is kept when absent (`PluginProcessor.cpp:782-787`). → Every factory preset needs a seed.
6. **Unlisted lanes survive** (D45). → An enabled stream must have a listed slot (PR0), or the preset sounds different per user.
7. **Mid-apply tearing in a real host** (ASSUMED risk). 259 × 2 `setValueNotifyingHost` calls on the message thread can interleave with an audio block that sees reset defaults. The harness applies between blocks, so it is blind to this; there are no APVTS listeners in the plugin (grep 0). → Add a HANDS-ON row: switch presets while holding a chord in Logic and Live.
8. **Cross-arch FMA.** It affects both the generator (Q4) and any harness row comparing `getRawParameterValue` to a literal on a fresh instance.
9. **`contentHash` pins only 131 072 frames.** → Add full-length pins.
10. **Memory files that do not apply.** `critical_preset_manager_stale_customstate_child`: the plugin never calls `getStateAsXml` / `setStateFromXml` (grep; `PluginProcessor.cpp:829-830`). `project_preset_manager_v102_rollout`: the plugin uses the canonical module 1.1.1 header (`PluginProcessor.h:32`), not a vendored copy.
11. **Memory files that do apply.** `pattern_preset_apply_needs_reset_to_defaults` is built in (module v1.1.1). `feedback_presets_never_set_output_gain` is enforced by exclusion; PR0 should also gate the source table. `feedback_realism_titled_presets_must_not_wobble` applies to Keys ("Bell" etc.).
12. **Sources only reach users through presets.** There is no factory-source browser in the UI. Your own dev sessions and presets that reference `stub_*` ids will show the slot as missing once the shipping table drops them. No alias is needed for a first public 1.0.0.

### A · Decisions for the plan

1. **Bank:** 8 categories × 8 per the Q1 table; `"<Cat> · <Name>"` ≤ 24 chars; flat menu, no headers; names not localized (precedent).
2. **Authoring:** C++ `Source/FactoryPresets.h`, a port of O-Granulator's spec struct.
   - Values in engineering units, converted once via `convertTo0to1`; `validateSpec`.
   - Fixed int64 seed string per preset.
   - Slots as `{index, source, name}` with no hash. Live presets list no slots.
   - Registered with `initializeFactoryPresets` in the ctor.
3. **Bank stamp:** plugin-side bank-hash stamp that clears stale `Factory/*.json` on mismatch (pitfall 2).
4. **Factory-dir save guard:** add it in `savePresetWithDialog` (pitfall 3).
5. **Sources:** the 8 recipes, 48 kHz / 24-bit, pitched at C3, noise beds at 16-bit resolution, total ≤ 8 388 608 B gated by FA3.
   - Extend `--gen-factory` with a recipe table, size printout, `fp contract(off)` and integer RNG.
   - Pins: decoded 131 072-frame `contentHash` + a new **full-PCM XXH64** on all platforms; file sha256 on mac arm64 only.
6. **Stubs** become harness-only fixtures (separate binary-data target + `OGI_TEST_HOOKS` table tail). Carried rows and the UI-stub golden stay unchanged.
7. **Harness:** temp-HOME re-exec with the real home captured for GD1 and hygiene. Rows PR0, PR1 / PR1n / PR1n2, PR2, PR4, PR4b, PR5, PR6 (CK6 metric, NC `swapKillHard`), FA3, plus the JS gate PN1 / PN1n.
8. **output_gain:** snap-only `NormalisableRange` (clamp + \|v\| < 1e-5 → 0), no interval. **Recommend the class fix on all 6 rows** (`output_gain`, `rec_gain`, `s1`–`s4_level`): same root cause, and PR1 needs exact defaults. This widens D55, so confirm with you.
   - Re-pin `.planning/params.tsv` (`defaultText` cells only), record the reason, update the checksum in STATUS.
   - Regenerate `generic-overrides.json`.
   - Spec addendum (convention `:36`).
   - Renders / parity must stay byte-identical (audio already snaps).
9. **D59:** fix the `lane-embed` help copy in EN / FR / zh; no new keys from this slice; the five UI gates + PN1.
10. **HANDS-ON-SESSION-4 preset pass:** switch while holding a chord (Logic AU + Live VST3), audition all 64, check realism titles for wobble, confirm `output_gain` is untouched across loads, and confirm a stale `~/Library` bank was cleared.

---

# Part B — CPU over presets, auval, D55 carried fixes (Q2, Q6, D55a–c)


Paths are relative to `plugins/O-GranulatorInstrument/` unless stated otherwise. Tags: **V** = VERIFIED this session (file:line read, or a probe run). **A** = ASSUMED.

### B · Q2: CPU verdict over the 64 factory presets

**How `--cpu26` measures today (V)**
- Entry: `main.cpp:1644` calls `eg26b::cpu26()` (`engine_gates_26b.h:1119-1140`). That runs `eg25b::cpu25a()` (`engine_gates_25b.h:911-983`) and then a C26rec INFO row. The return code is the FAIL count of C25m and C25tm.
- Timer: `eg24::measure25` (`engine_gates_24.h:1202-1271`).
  - Notes go on in block 0 and are held with no note-off. 5 s untimed warm-up, then `reps = 7` runs of 10 s.
  - `CLOCK_THREAD_CPUTIME_ID` brackets each `processBlock` (`:1232-1234`). pct = cpu / 10 s of audio. The verdict uses the **median**; min and max are reported (`:1264-1265`).
  - Duty witness: thread / wall ≥ 0.9, else `contended` (`:1238-1239`, `:1267`).
  - Speed witness: `calibNs()` runs before and after (`cpu_calib.h:36-60`, a fixed 2·10⁸ FMA workload, median of 5). `slowed` if calB > 1.15 × ref_ns or calA / calB > 1.10 (`:1268`). ref_ns is 40523708 on an M4 Max, frozen in `tests/golden/cpu-calib-2.3.txt`.
  - Settings: fs `kFs` 48 k, block 128 (`25b:913`).
- **C25w** = `c25Session` (`24:1273-1284`):
  - C22 recipe: 4 streams, density 200, size 1000, `grain_limit` 3 (= 256), polyphony 16 (`23:1144-1155`).
  - Plus `worstFx()`: BP res 100, drive, crush and ring all at 100 on 4 streams (`23:1157-1165`).
  - Plus `c25Extra()`: feedback 100, diffusion 100, sat 20, reverb mix 100, size 100.
  - 16 held notes, 40–55.
- **C25m** = C25w + `c25mRoutes()`: 16 per-voice routes, 4 PV LFOs + 2 Mod Envs (`25b:891-909`).
  - Gate: ≤ 40 %, valid only with live ≥ 240, ≥ 16 PV dests per tick, quota recomputes ≥ 90 %, 4 pair-mode streams (`:940-945`).
- **C25tm** = typical proxy + 4 routes (`25b:948-977`):
  - 2 streams at density 40 / size 150, polyphony 8, s1 LP 1.2 kHz res 40 + drive 30, 6-note chord {48,52,55,59,62,64}.
  - Gate: ≤ 12 %, valid only with live in [60, 90].
  - C25t (no routes) is INFO.
- Measurement precondition (V): `Session` defaults to `coherenceOff = true` (`engine_gates_21.h:453`). Every CPU session calls `shipping(*s)` (`engine_gates_22.h:117`). **A preset session must call it too, or it measures a non-shipping path.**
- Cost (V arithmetic): wall time ≈ 75 audio-seconds × pct.
  - Typical: about 5 s per config. Worst: about 22 s.
  - 64 presets × (chord + worst) ≈ 30 min with the full recipe. That exceeds the 600 s watchdog, so it must run in the background (memory `pattern_executor_watchdog_stall`).

**Proposed `--cpu4` (new mode, run alone; keeps `--cpu26` unchanged as the regression row)**
1. **Session per preset.**
   - `make()` → `shipping(*s)` → `p->getPresetManager().loadPresetFromFile(tmpCopy)`. The apply is synchronous JSON (`modules/persistence/preset-manager/cpp/OuariconPresetManager.h:531-551`) (V).
   - Then `getLoader().waitForIdle(20000)` + render 2048 samples, the same pattern as `settle()` (`engine_gates_26b.h:143-147`), so factory sources resolve before timing. Assert `fsTouches == 0` and every referenced slot `loaded`.
   - Read preset JSON from the in-repo generated set. Never call `initializeFactoryPresets` in the harness: it writes `~/…/Presets/Factory`. Memory `pattern_juce_mac_home_ignores_HOME`: if any path touches home, re-exec with `CFFIXED_USER_HOME`.
2. **Script T (typical):** the C25tm chord, 6 notes {48,52,55,59,62,64} vel 100, held, through `measure25` unchanged. Comparable with the frozen proxy, and no new timing code. Single note 48 = INFO.
3. **Script W (worst):** 16 held notes 40–55, the same `measure25` call. The preset's own `polyphony` caps voices; report effective voices + live grains.
4. **Validity for presets:** `!slowed && !contended` only. Drop the proxy-specific live windows (≥ 240, [60, 90]); report live instead.
5. **Thresholds (per preset, hard gate):** T ≤ 12 %, W ≤ 40 %.
   - Bank lines (INFO): median T, p90 T, max T, max W, sorted top-5.
   - Recommend a per-preset gate over a bank median. ARCHITECTURE:466 says "Typical factory preset ≤ 12 %", which reads as each preset, and a median would hide a 20 % pad.
6. **Recipe and noise.**
   - Sweep: `reps 5 × 6 s` + 3 s warm-up. That is about 33 audio-seconds, or about 12 min for the bank (background).
   - **Borderline re-run:** any preset with T > 10.5 % or W > 36 % (within about 12 % of the gate), or NOVERDICT, re-runs the full D19 recipe (7 × 10 s). That median is the verdict.
   - Statistic = **median**, the same as D19 / C25m. Min-of-N is reported but never gates, because it is optimistic for a budget.
   - Allow a NOVERDICT re-run up to 2×; still invalid → FAIL (parent 2.6 finding, D19).
7. **Session drift row.** `--cpu4` first runs the `--cpu26` rows in the same session (C25m ≤ 40, C25tm ≤ 12 kept as regression rows). It prints a session factor = C25tm_now / 6.91 (the 3.3 value).
   - 3.3 already saw +7 % machine drift: C25w 26.38 → 28.20 (VERIFICATION-3.3:82).
8. **Special presets.**
   - Record-in templates: their slots are empty, which under-reads. Load a factory source into the rec target with `loadSlotSync` and flag the row "stand-in".
   - Tempo-synced presets: they need host tempo in the harness. Use the `TS` row playhead fixture, 120 BPM (A: fixture API not read).
   - MPE presets: feed `C25mpe`-style member channels (`engine_gates_25d.h:922-987`) as INFO (A on reuse).
9. **If a preset exceeds:**
   - First the content lever: that preset's density, size, enabled streams, `grain_limit`, or FX. All are presets' own params, not a spec change.
   - Only if a typical-shaped preset still fails: the D21 engine levers in order (CONTEXT-2.3:44-49):
     1. Hermite at rate ≤ 1, mip 0, timbre-gated.
     2. 4-lane SIMD with goldens byte-identical.
     3. FX-skip.
   - The 256 → 128 default fallback is locked: stop and ask.

### B · Q6: auval LFO-rate default warnings

- **Exact text (V, run this session):** `auval -v aumu OuGI OuDv` on the installed `O-GranulatorInstrument-dev.component` → rc 0, `AU VALIDATION SUCCEEDED`. There are 4 lines identical to:
  `WARNING: retrievedValue = 0.328712 (was 0.328712), Parameter did not retain default value when set`
  - Each follows `Values: Minimum = 0.0100000, Default = 0.5000002, Maximum = 20.0000000`, which is LFO 1–4 · Rate. Log: scratchpad `auval-ogi.txt` lines 2151 / 2319 / 2473 / 2534.
- **Cause (V):**
  - The param is `floatDef("rate", 0.01, 20, def 0.5, skew 0.3)` (`Source/GiParamIds.h:352`). It becomes `NormalisableRange(lo, hi, interval 0, skew)` + `AudioParameterFloat` (`Source/PluginProcessor.cpp:198-203`).
  - The JUCE 8.0.15 AU wrapper reports default = `getDefaultValue()`, which is **normalised** (`juce_audio_plugin_client_AU_1.mm:1108-1110`). Get / Set pass normalised values (`:1152-1186`).
  - `AudioParameterFloat::setValue` stores `convertFrom0to1(n)` and `getValue` returns `convertTo0to1(value)` (`juce_AudioParameterFloat.cpp:97-99`). The NormalisableRange pow / exp-log math is at `juce_NormalisableRange.h:136-172`.
  - A float simulation of that math (scratchpad `rt/rt.cpp`) reproduces the warning exactly: 0.5 → n 0.328711569 → 0.500000179 Hz (auval's "Default = 0.5000002") → n 0.328711599. That is 1 ulp of drift on the normalised value.
  - **Every other skewed / centred param is round-trip stable** (glide, envs, size, density, cutoff, ring, rec_length, fb cuts, level, output_gain at its current range). So the warning set is exactly the 4 LFO rates.
- **Impact:** none audible. 1.8e-7 Hz, auval PASS, "retain value across reset and initialization" PASS.
- **Fixes all move the contract.**
  - Default → 0.5000003 (the round-trip fixed point) changes params.tsv / param-dump and `parameter-spec.md` (LOCKED).
  - Skew change is LOCKED.
  - A custom range with lambdas hits memory `critical_lambda_normalisable_range_invisible_to_webview_slider_frontend`.
  - An `AudioParameterFloat` subclass snapping `setValue` cannot reach the private `value`.
- **Recommendation: DOCUMENT, do not fix.** Record it in VERIFICATION-4 / a CHANGELOG "known validator notes" line. Add a harness row **AU1**: compute `to01(from01(to01(def)))` for all 261 params (via the real `AudioParameterFloat`) and assert the drift set == {lfo1..4_rate}. That freezes the warning count, catches any new param or the D55 `output_gain` range change adding a 5th, and has a control (a temp range with skew 0.3 on another param must enter the set). No param-dump or preset impact.

### B · D55 carried fixes

**(a) `WavFailed` surfaced after a take: currently silent (V)**
- Path:
  1. Loader worker `runCommit` (`Source/dsp/SlotLoader.cpp:1084-1141`) writes the take WAV and sets `ns.wavFailed = !wavOk` (`:1131`).
  2. It publishes, then `onCommitted(slot, wavFailed)` (`:1139-1140`).
  3. The processor lambda calls `rec->noteWavFailed()` (`PluginProcessor.cpp:99-103`), an atomic flag consumed **only** in the audio thread's `beginBlock` (`GiRecorder.h:221-222`, `:423`).
  4. `GiRecRunHold` deliberately gives WavFailed no hold (`PluginProcessor.h:86-91`).
  5. The page reads only `rec.run`, never `reason` (grep `index.html`: 0 hits).
- Also: `SlotInfo.wavFailed` exists (`SlotLoader.h:123`, set `SlotLoader.cpp:730`) but `slotInfoToVar` does not emit it (`Source/GiUiJson.h:129-151`).
- **Minimal fix:**
  1. Emit `o->setProperty("wavFailed", info.wavFailed)` in `slotInfoToVar`.
  2. Page `noteSlotErrors()` (`index.html:3736-3744`) also tracks `wavFailed`, using the same edge rule: the first pull is not news, and a false→true transition toasts `toast.wavFailed {n}`.
  - The commit already moves `slotRev` → `pullSlotState` (`index.html:3898-3901`).
  - This bypasses the audio thread entirely. It works while the host idles the callback, where the recorder flag would wait for the next `beginBlock` (memory `pattern_generation_reaper_freezes_when_host_idles_audio_thread`).
  - It is per-lane, where the recorder reason is global.
  - No telemetry or recorder change, so the RT surface is untouched.
- **Proof:**
  - Harness row: `hooks.failWav` (`SlotLoader.h:151`; used at `engine_gates_26a.h:909`, `ui_gates_33.h:1241`) → commit → `slotInfoToVar(...)["wavFailed"] == true` and slotRev moved; the next good take → false.
  - Control: a test flag omitting the field must leave the page probe silent.
  - i18n state `toast-wav-failed` (below).

**(b) One toast per failing lane: currently the last write wins (V)**
- `showToast` writes one `#toast` element and re-arms a single 1600 ms timer (`index.html:3115-3121`). `noteSlotErrors` loops n = 4 → 1 calling it per lane (`:3739-3741`), so with several failing lanes only **slot 1's** text survives.
- **Minimal fix: a batch-scoped chain, not a global queue.**
  - `noteSlotErrors` collects the lanes with a new error or wavFailed in ascending order. It shows the first immediately and schedules the rest at +1600 ms steps, with timers stored and cleared at the next batch.
  - Other toasts keep their current replace-immediately behaviour.
  - A global FIFO would delay later i18n-states keys past ST1's +150 / +450 ms checks (PLAN-3.3:156).
- **Proof:**
  - Probe row: set `slots[1].error` and `slots[2].error` in one stub pull + `slotRev += 1` → `#toast` shows `toast.loadFailed {n:2}` at +150 ms and `{n:3}` at +1750 ms.
  - Control: chain disabled → `{n:3}` never appears.

**(c) Bounded save-worker join on a hung disk: currently unbounded (V)**
- The destructor calls `saveWorker->stopThread(-1)` (`PluginProcessor.cpp:141-144`). Its comment claims "bounded by one WAV chunk write".
- The snapshot-phase waits are exit-polled (`:1823-1867`). The write phase is not: `writeSaveWav` blocks in `FileOutputStream` writes / `flush` / `getSize` / `moveFileTo` (`:1885-1931`). On a hung volume (a network disk, an ejected USB), plugin close hangs the host's message thread forever.
- `stopThread(timeout)` is not a fix: JUCE **kills** the thread on timeout (`juce_Thread.cpp:264-275`, `killThread`, "very bad karma"), which can leave `saveLock` held → deadlock in the destructor.
- **Minimal fix: isolate the I/O on a detached writer.**
  1. At `finishDone` (`:1783-1807`), move `saveBufL/R` (HeapBlock swap, under `saveLock`, worker thread) into a `shared_ptr<WritePayload>{bufL, bufR, frames, rate, file}`.
  2. A detached writer thread owns the payload plus a `shared_ptr<SaveShared>{atomic<bool> abandon, atomic<int> state, String error}`. It runs a member-free `writeSaveWav`, checking `abandon` per chunk (it deletes `.part` when abandoned).
  3. `SaveWorker` waits on `state` with the existing exit-polled 2 ms loop. On exit it sets `abandon` and returns `cancelled`.
  - With every SaveWorker wait now exit-polled, `stopThread(-1)` stays bounded at ≤ 100 ms + 2 ms. No forced kill, no leaked `this`, and the completion still runs exactly once (the destructor delivers `cancelled`, `:160`).
  - No audio-thread change. The snapshot is already Idle before the write (`:1787`).
- **Test hook (`OGI_TEST_HOOKS`):** `testHoldWrite`, a `WaitableEvent` the writer blocks on before chunk 0 **without** polling exit (this models a syscall). A static `writerExits` counter.
- **Row USV3:**
  1. Take → `saveRecordingTo` (async worker) → wait until the writer has entered → destroy the processor, timed.
  2. Assert dtor ≤ 500 ms. Release the hook → `writerExits == 1` within 2 s, `.part` gone, completion delivered once (`cancelled`).
  3. Control `testInlineWrite` (the old in-worker write): dtor ≥ the 3 s hold, released by a helper thread.
  - USV1 / USV2 must stay green.
- Residual risk (A): a detached thread still running if the host **unloads the bundle** would crash. macOS hosts rarely unload AU / VST3 code, and it is strictly better than a hang. Document it.
- Out of D55 scope but the same class (V): `~SlotLoader` uses `pool.removeAllJobs(true, 10000)` (`SlotLoader.cpp:261`), so a take-WAV write on a hung disk can stall close for 10 s, then ThreadPool may kill. Note it in VERIFICATION, do not fix (A on the ThreadPool kill path).

### B · D59 strings

- **New key: `toast.wavFailed` only.** (b) reuses `toast.loadFailed` / `toast.wavFailed`. (c) has no UI: the editor is gone at close, and a `timeout` save already maps to `toast.saveFailed` (`index.html:3795`).
  - Proposed text, following the house pattern at `js/i18n.js:2659-2688` (fr `reviewed: false`, zh `reviewed: 'mt'`):
    - en `"Slot {n}: the take plays, but its WAV file could not be written"`
    - fr `"Emplacement {n} : la prise joue, mais son fichier WAV n’a pas pu être écrit"` (glossary slot → emplacement, `scripts/i18n-fr-glossary.js:283`; "prise" as in `toast.saveFailed`)
    - zh-Hans `"插槽 {n}：录音可播放，但 WAV 文件写入失败"` (glossary `i18n-zh-glossary.js:151`)
  - Avoid "kept in the session": embed is refused above 16 MB FLAC.
- **Files:**
  - `Source/ui/public/js/i18n.js` **and** `.planning/mockups/v1-i18n.js` (pair `cmp` clean today, V).
  - `index.html` **and** `mockups/v1-ui.html` (pair `cmp` clean, V) for the `noteSlotErrors` / slotInfo edit.
  - `tests/i18n-states.json` (not paired; it differs from `v1-i18n-states.json` today, V).
- **New i18n state (needed):** `toast-wav-failed`, modelled on `toast-load-failed` (`i18n-states.json:35`): `c.rec(); c.nat.getSlotState.slots[2].wavFailed = true; c.tel.slotRev += 1;`. That makes **46 → 47** states, and the check-ui-labels banner must equal the file count (PLAN-3.3:105).
  - The batch chain needs no extra state, since its first item is immediate. Its sequence is a probe row.
- **Gates, in PD26 order (PLAN-3.3:159):** overrides → `ui_frontend_check.js` → `ui_binding_probe.js` → `scripts/check-i18n.js` ([1] every key in en/fr/zh-Hans, [4] fr ≠ en, [5] reviewed flags) → `i18n-fr-lint.js` → `i18n-zh-lint.js` → `check-ui-labels.js` (fit at the widest language: the fr string is the longest, check `#toast` width) → `boot-all-uis --strict-tips`. 3.3 baseline: 0 · 0 · 0, labels 1977 / 0.

### B · Pitfalls

1. A preset CPU session without `shipping()` measures the `coherenceOff` path (`engine_gates_21.h:453`).
2. Timing before the loader is idle: the worker contends a core → duty < 0.9 → NOVERDICT = FAIL. Always use `waitForIdle` + a render.
3. Calling `initializeFactoryPresets` / the factory dir in the harness writes to the real home. Use in-repo JSON + `loadPresetFromFile`.
4. Reusing the proxy validity windows (live ≥ 240, [60, 90]) on presets invalidates honest presets. Report live, gate only on duty / calib.
5. Bank median as the gate hides a heavy preset. Use a per-preset gate, with the median as INFO.
6. Fixing Q6 by moving the default is a LOCKED contract move with zero user benefit.
7. `stopThread(timeout)` kills the thread → `saveLock` can stay held. Never use it as the "bound".
8. A global toast FIFO breaks ST1 / i18n-state timing (+150 / +450 ms).
9. Routing WavFailed through the recorder reason / `rec.run` needs the audio thread to run, so it stays silent while the host idles. It is also global, not per-lane.
10. Every page or i18n edit must land in both pair files, or the `cmp` gates fail.

### B · Decisions for the plan

1. **Q2 mode:** add `--cpu4`, run alone in the background. It reuses `measure25` unchanged, runs the `--cpu26` rows first (regression + session factor), then per preset runs script T (6-note chord, ≤ 12 %) and W (16 held, ≤ 40 %). Gate per preset; bank median / p90 / max are INFO.
2. **Q2 noise:** sweep at 5 × 6 s. Borderline (T > 10.5, W > 36) or NOVERDICT re-runs the full 7 × 10 s recipe, whose median decides. Two invalid runs = FAIL. Min is reported only.
3. **Q2 overrun:** retune the preset first (density / size / streams / `grain_limit` / FX). Then the D21 levers 1 → 3. The 256 → 128 fallback is stop-and-ask.
4. **Q6:** document. Add harness row AU1 freezing the round-trip drift set at {lfo1..4_rate}, with a control. No param-dump or preset change.
5. **D55a:** emit `wavFailed` in `slotInfoToVar`. Page edge-toast `toast.wavFailed` via the slotRev pull. Harness row (failWav) + control.
6. **D55b:** a batch-scoped ascending toast chain in `noteSlotErrors` at 1600 ms steps. Other toasts unchanged. Probe row + control.
7. **D55c:** move the WAV write to a detached writer with a shared payload and abandon flag. SaveWorker waits stay exit-polled, so `stopThread(-1)` is bounded at about 100 ms. Hook `testHoldWrite`, row USV3 (dtor ≤ 500 ms, writer exits once, `.part` removed, completion once) + inline-write control. Record the SlotLoader 10 s join as a known item.
8. **D59:** one new key, `toast.wavFailed` (en / fr `reviewed:false` / zh `mt`), in both i18n pair files. Add state `toast-wav-failed` (46 → 47). Run the full PD26 gate order. Confirm the fr toast fits `#toast` at check-ui-labels.

---

# Part C — CI / release, fork drift, human session (Q7, Q8, Q9, D54, D56)


Researched 2026-11-11, read-only, at HEAD `804872fb`. Tags: **V** = VERIFIED (file:line, SHA or command output this session); **A** = ASSUMED.

### C · Q7: CI

**Release workflow** (`.github/workflows/build-and-release.yml`)
- **Triggers.** It fires on tag `*-v*` (:38-40) and on `workflow_dispatch` with inputs `plugin_name` (free string), `version` (default `0.0.0-validate`) and `validate_only` (default true) (:44-59). **V**
- **Tag parsing.** `${TAG%-v*}` and `${TAG##*-v}` (:122,124). `O-GranulatorInstrument-v1.0.0` parses as plugin `O-GranulatorInstrument`, version `1.0.0`. **V**
- **Configures only the tagged plugin.** Both jobs build `SKIP_PLUGINS` from every other `plugins/*/` (macOS :195-208, Windows :584-600).
  - This plugin's CMake has no FetchContent. It reads only `${CMAKE_SOURCE_DIR}/modules/...` (CMakeLists.txt:39, :150-155) and its own `Resources/factory/*.flac` (:177-178). **V**
  - The stub FLACs are tracked in git, and new `.flac` files are not gitignored (`git check-ignore` rc=1). **V**
  - The six modules used have no FetchContent or find_package (grep came back empty). **V**
  - The test consoles stay off because `OUARICON_BUILD_TESTS` defaults to OFF (CMakeLists.txt:205). **V**
- **Target and product name.** `resolve-target.sh` returns `OuariconGranulatorInstrument` and `O-GranulatorInstrument` (ran it). The only uncommented call is CMakeLists.txt:41. The header comment avoids the resolver trap (:9-11). **V**
- **Formats built.** macOS builds VST3 + AU only (:221). Windows builds VST3 only (:614). **The Standalone is never built, signed or shipped by CI.** **V**
- **macOS architectures.** CI builds a universal binary (`arm64;x86_64`, :206). The local build is arm64 only (`CMAKE_OSX_ARCHITECTURES` empty in the cache; `lipo` shows arm64). So the first x86_64 compile happens in CI. **V**
  - Source has no NEON, SSE or `__builtin` code (grep came back empty). **V**
- **Release branding.** Release builds use `-DOUARICON_RELEASE=ON` (:207,:600), giving manufacturer `OuAu` and no `-dev` suffix (root CMakeLists.txt:23-31). **V**
  - So hands-on sessions saved with the `-dev` AU or VST3 will not reopen against the release plugin. **A**: different AU/VST3 identity; consistent with the CLAUDE.md "same AU triple" note, which applies only to the suffix.
- **Signing (macOS).** Eight secrets (:19-22). The certificate is imported into a temporary keychain with Apple's intermediates (:223-284). Then:
  - `codesign --deep --force --options runtime --timestamp` on each `.vst3` and `.component`, with no entitlements file (:286-321).
  - `notarytool submit --wait` on a zip of both, then `stapler staple` on each (:323-369).
  - A PKG built with pkgbuild/productbuild, signed with `productsign` (Developer ID Installer), then notarised and stapled (:371-522). Bundle id is `io.oaudio.o-granulatorinstrument` (:382). **V**
- **Windows.**
  - No Authenticode signing step in the file. **V** (whole file read)
  - WebView2 NuGet 1.0.1901.177 (:576-579).
  - pluginval 1.0.3 at strictness 10 with a 600 s timeout. It opens the editor (:616-633).
  - Inno Setup with a WebView2 bootstrapper whose signature is checked (:667-743). **V**
- **Release creation.** Needs both build jobs (:756). Release notes come from `awk "/^## \[${VERSION}\]/{flag=1; next} /^## \[/{flag=0} flag"` (:783), falling back to `Release <P> v<V>` (:786-789). **V**
- **`plugins/O-GranulatorInstrument/CHANGELOG.md` does not exist** (`ls` failed). **V** Stage 4 creates it.
  - House style: `# <Name> Changelog` then `## [X.Y.Z] - date` (O-Granulator CHANGELOG.md:1-3; O-Prism and O-Strata use an em dash). None of the 47 CHANGELOGs carries an "adheres to Semantic Versioning" line. **V**

**Windows WebView2 (D54 inputs): all present today.**
- CMakeLists.txt: `NEEDS_WEB_BROWSER TRUE` (:52), `NEEDS_WEBVIEW2 TRUE` (:53), `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (:189).
- PluginEditor.cpp, inside `#if JUCE_WINDOWS` (:1104-1131):
  - `.withBackend (juce::WebBrowserComponent::Options::Backend::webview2)` (:1120)
  - `.withUserDataFolder (webView2DataFolder)` (:1123), with the folder at `userApplicationDataDirectory/Ouaricon/WebView2/O-GranulatorInstrument` and created first (:1113-1117)
  - `areOptionsSupported` probe (:1131); on failure the editor shows a native notice. **V**
- **That `#if JUCE_WINDOWS` block is the plugin's only platform-conditional code, and it has never been compiled** (local builds are clang/macOS). **V** (grep)
  - It mirrors O-simpleSubtractive PluginEditor.cpp:211-223, which built green in Windows release CI (run 2026-10-20, tag `O-simpleSubtractive-v1.7.4`, `gh run list`). **V**
  - The APIs exist in JUCE 8.0.15: `juce_WebBrowserComponent.h:195,200,205,461`; StandardHeader is 8/0/15 and matches `.github/juce-version.txt` = 8.0.15. **V**

**MSVC trap scan** (memories c3615 / c3493 / SafePointer):
- SafePointer: six sites, all in the hoisted-local form `auto safeThis = juce::Component::SafePointer<...> (this);` (PluginEditor.cpp:560,606,814,849,1059,1085). **V**
- Non-static `constexpr` locals: 22 found. The ones checked (PluginProcessor.cpp:1230,1903; SlotLoader.cpp:33-37 are namespace scope, :398) are not inside lambdas. **V**
  - The forked leaves (OutputLaw.h, WindowMorphBank.h) come from O-Granulator, **which has never built on Windows** (`git tag -l 'O-Granulator*'` is empty). **V** C3615/C3493 in those leaves is unproven. **A**
- `/bigobj`: JUCE adds it on MSVC as an INTERFACE option of juce_core, juce_gui_basics, juce_audio_processors and juce_graphics (`JUCEModuleSupport.cmake:632-635`). The plugin links those, so Engine.cpp (4,173 lines) inherits it. **V**

**Binary-data size:**
- juceaide writes one `BinaryDataN.cpp` per resource as a decimal byte array, about 3.6× the FLAC size: `stub_noise.flac` 777,546 B became a 2,782,629 B `.cpp`. **V**
- Precedent: O-AnalogEQ has an 11.2 MB generated `BinaryData6.cpp` and has released through Windows CI (tag `O-AnalogEQ-v1.5.4`). **V** for the file and the tag.
- At the 8 MB cap over 6-8 sources (1-1.3 MB each), each `.cpp` is about 4-5 MB, under that precedent.
- MSVC has no array-initialiser limit at this size. **A** The C1091 limit applies to string literals, which juceaide does not emit.
- **Keep any single source ≤ ~3 MB FLAC** to stay inside the proven envelope. **A**

**Pre-tag Windows proof: none runs automatically.**
- `ci-tests.yml` is dispatch-only (:71-81). Its `plugin` input is a **choice limited to `O-Octagon` and `O-Strata`** (:76-80), so this plugin cannot be dispatched there without editing that repo-wide workflow.
  - The parent's RESEARCH-4.md:119 recommended ci-tests and missed this. **V**
  - That GitHub rejects an out-of-list choice value is **A**.
- `ui-static-gates.yml` runs on push to main, but it only runs i18n lint (:25-68). **V**
- The working path is `build-and-release.yml` dispatched with `validate_only=true`:
  - It builds and notarises on macOS and uploads a `0.0.0-validate` PKG artifact.
  - It builds the Windows VST3, runs pluginval 10 and builds the Inno installer.
  - `create-release` is skipped (:760).
  - Precedent: the `workflow_dispatch` on main of 2026-10-15 succeeded (`gh run list`). **V**
- **Blocker: none of this plugin exists on origin.**
  - `git ls-tree origin/main plugins/O-GranulatorInstrument` is empty.
  - `main` is 185 commits ahead of origin, 75 of them touching this plugin. **V**
  - A dispatch builds the remote ref, so it needs a push first. The push is repo-wide (it also publishes about 110 commits from other sessions) and outward-facing, so it must be a Taylor-confirmed checkpoint.
  - `/publish` pushes `HEAD` anyway (plugin-publishing SKILL.md:224). **V**

**Where the version lives (CMakeLists.txt:47):**
- `VERSION 0.1.0`. **V** No other copy in `Source/` or `tests/`; grep for `0.1.0` and `VersionString` came back empty. **V**
- Runtime carrier: preset-manager stamps `JucePlugin_VersionString` into presets (OuariconPresetManager.h:357,744). It also writes a `.factory-version` sentinel and **skips rewriting the factory files when the sentinel equals the version** (:713-716,752). **V** (pitfall P3)
- The param-dump compiles the version (ParamDump.cmake:208) but does not print it (no `version` in main.cpp). **V** The bump is expected to be dump-neutral.
- `PLUGINS.md:76` is stale: `| O-GranulatorInstrument | 🚧 Stage 1 | 0.1.0 | Synth (Sample-Based Polyphonic Granular) | 2026-11-07 |`. **V**
  - The parent's row after its Stage 4 is `| O-Granulator | ✅ Working | 1.2.0 | ... |` (:75). **V**
- `/publish` blocks unless the status is 📦 or ✅ (publish.md preconditions). **V**
- `modules/registry.yaml` has `used_by: O-GranulatorInstrument version: 0.1.0` in **7 entries** (:114,194,407,451,519,657,724). **V**
  - `used_by.version` tracks the current plugin version (O-GrainScatter 3.6.0 and O-FreqPulse 1.20.2 equal their PLUGINS.md rows). **V**
  - **The file has another session's uncommitted hunks** (O-Granulator 0.1.0 → 1.0.0, 5 hunks, `git diff`). **V**
- README.md has no row for O-Granulator or O-GranulatorInstrument (grep). **V** The published set comes from the oaudio.io sitemap (memory). Out of path scope; leave for `/publish` or a separate commit.
- No `NOTES.md` exists for this plugin or its parent; 34 other plugins have one. **V** Optional.

### C · Q8: Fork drift (D58)

- **Since the pin, the parent has one commit:** `git log 82406fce..HEAD -- plugins/O-Granulator` returns only `e77d29c8 improve: O-Granulator v1.2.0 - click a knob label to type a value`. **V**
  - It touches `PLUGINS.md`, the parent's `STATUS.md`, `CHANGELOG.md`, `CMakeLists.txt` (version) and `Source/ui/public/index.html` (+162/−7). **V**
  - **It touches no forked leaf:** `git diff --quiet 82406fce HEAD -- plugins/O-Granulator/Source/dsp` returns 0. **V**
  - **Classification: UI feature, already ported.** D41 ported "parent 1.2.0 value entry" into `index.html` and `v1-ui.html` at 3.1 (STATUS.md:77). **V**
  - **Disposition: record only.** No port, no parity re-run, pin unchanged.
- **Shared modules: zero drift.** Since `82406fce`, grain-engine, pitch-collections, reverb-unit, note-expression, preset-manager, webview-drop-streaming and eb-garamond each have 0 commits, and so does `scripts/param-dump`. **V**
  - `tests/parity/run.sh:36-39` refuses to run if grain-engine, pitch-collections, reverb-unit or param-dump differ from the pin. That guard is clean today (`git diff --quiet` returns 0). **V**
- **What "drift 0 / 13" measures** (fork-drift-check.sh:9-20, :117-197): per leaf, it checks both directions.
  - **Check 1 (upstream):** the manifest's pin-blob sha equals `git show PIN:path`, **and** the parent at `HEAD`, **and** the parent working-tree file.
  - **Check 2 (local fidelity):** VERBATIM leaves equal the renamed pin blob byte for byte; ADAPTED leaves match the `sha256(local)` column.
  - So 0 / 13 means **parent HEAD == pin for all 13 leaves AND no unrecorded local edit**. Re-run today: `13 leaves checked / 0 drift`. **V**
  - It does **not** cover non-leaf parent files (`index.html`, `PluginProcessor.cpp`, presets). Those need the `git log` above.
- **FORK-MANIFEST.md entry for D58:**
  - Add one line to "Take / skip log" (FORK-MANIFEST.md:37-39, currently "(empty — zero drift at pin)"): `e77d29c8 — UI only (index.html value entry), no leaf; taken at 3.1 D41; skip; pin unchanged`.
  - Note that 82406fce itself added the parent's 64 factory presets (8 × 8, commit subject). **V** Useful as bank precedent for slice A.

### C · Q9: Human session layout (D51)

**Inventory (row counts from table-row grep; V):**

| Sheet | Rows | Stale wording to fix before the session |
|---|---|---|
| 2.1 | 13 | Header: DevLoaderEditor + "voice gain −6 dB" (2.1:3,13; DevLoader removed at D38). r1 "Load slot 1…4". r13 "slots come back empty". |
| 2.2 | 10 | r10 "empty until 2.6". |
| 2.3 | 9 | none |
| 2.4 | 11 | r5 "(Stage 3 UI.)". r11 "old patches". |
| 2.5a | 12 | r12 "old patches". |
| 2.5b | 11 | r11 "old patches". |
| 2.6 | 13 | r7 "factory not reachable until Stage 3". r12 "old patches". |
| 3.1 | 16 | none |
| 3.2 | 27 | none |
| 3.3 | 18 | none |
| SESSION-3.3 | Logic L1-L6 (6) + 11 tally rows | ❌ routing "reopens 2-dsp / 3-gui" (:63) is obsolete at Stage 4. |

Total carried: 79 + 61 + 6 = 146 (CONTEXT D51 agrees, 79 + 61). The "old patches" rows are vacuous because no pre-Stage-4 sessions exist (hands-on never ran). Mark them N/A, citing the harness (TB7 etc.).

**Proposed `HANDS-ON-SESSION-4.md` order.** Use one sheet with row IDs that point back to the source rows (`2.1-4`, `3.2-13`). Rewrite only the stale rows; the old sheets get a one-line "superseded by SESSION-4" header.

**0. Setup.**
- Install from the 1.0.0 dev build via `build-and-install.sh` and build the Standalone.
- Check `auval -a | grep -i granulatorinstrument`.
- Quit Logic and Live before installing.
- **Delete the factory preset dir and the `.factory-version` sentinel** (P3).
- Materials:
  - sine, voice, drum loop, left-only signal
  - a 44.1 k file, a file > 30 s, a fake `.wav`
  - a MIDI keyboard with sustain, mod wheel and aftertouch

**1. Standalone pass (engine + UI, no host).**
- 2.1 r1-11
- 2.2 r1-5, r7-9
- 2.3 r1-7
- 2.4 r1-7
- 2.5a r1-5, r7, r8, r11
- 2.5b r1-5; r10 for CK5b and CK5c
- 3.1 r1, r2, r4, r5, r7-16
- 3.2 r1-12, r16, r18, r19, r21-27, including r26, the fr "tenue" wording (D59)
- 3.3 r1-14, r16-18
- New D55 rows:
  - a take whose WAV write fails shows a toast (set Recordings read-only)
  - two bad drops give one toast per lane
  - OUTPUT reads exactly 0.0 dB at default

**2. Preset audition pass (Standalone).**
- 64 presets: load, play the script, ✅/❌ each.
- Plus the preset proof rows from slice A: load after a dirty state, OUTPUT untouched, switch while playing with no click, factory lanes resolve and are named, save/reload round-trip.
- This runs before the DAW passes so they can use real presets.

**3. Logic AU pass.**
- SESSION-3.3 L1-L6
- 2.1 r12 (UAF), r13
- 2.2 r6 (vs O-Granulator at mix 100 %), r10
- 2.3 r8, r9
- 2.4 r8-10
- 2.5a r6, r9, r10
- 2.5b r6 (transport stop)
- 2.6 r1, r2, r4-11, r13
- 3.1 r3, r6
- 3.2 r3, r4, r13-15, r17, r20
- 3.3 r15
- New: CPU meter on the three heaviest factory presets at 48 k / 128.

**4. Live VST3 pass.**
- 2.6 r3, Live arm only: capture plus bypass-while-routed gives silence (the PD20 fix, PluginProcessor.cpp:386-400).
- New rows:
  - LV1: aux into Record-in, all three triggers
  - LV2: no sidechain routed → "No signal", nothing captured (QUAL-02)
  - LV3: save, close and reopen the Set; slots, takes and preset come back
  - LV4: automation plus Tempo sync in a looped clip (2.5a r9 in Live)
  - LV5: factory preset browse and load
- That Live offers a sidechain selector for a VST3 instrument's kAux bus is **A**. If it does not, LV1 is ❌-by-host and gets recorded, not fixed.

**5. Deferred (marked DEFERRED, not run).**
- 2.5b r7, r8 (MPE controller)
- 2.5b r9 (Dorico NE)
- the CK5f arm of 2.5b r10 (MPE member bend)
- 2.6 r3, Reaper arm
- W1: Windows drop and browse
- W2: Windows DAW load with the WebView rendering
- Owner: 1.0.x / hardware.

**Requirements each pass closes:**

| Pass | Requirements |
|---|---|
| Standalone | FUNC-01, FUNC-03, FUNC-04, FUNC-05, FUNC-09, DSP-02, DSP-03, UI-02; the macOS arm of FUNC-02 and UI-01; QUAL-01 (with the 2.6 r13 Logic row) |
| Preset | FUNC-08, plus the human arm of PERF-02 (the automated verdict is slice B) |
| Logic | FUNC-06, FUNC-07; QUAL-02 (AU unrouted: 2.6 r2, L3, L5; memory `critical_au_hands_unrouted_sidechain_silence`; the design decides on signal presence, Stage 0 D6); DSP-07 (with 2.1 r10); PERF-03 (2.1 r8 / r12, 2.6 r11) |
| Live | QUAL-02 (VST3 arm), FUNC-07 (VST3) |

Stays partial:
- UI-01, for its Windows arm (D54).
- COMPAT-02, for MPE and Dorico (D57). Note that REQUIREMENTS.md:64 still says `pending`, while CONTEXT:17 says it "stays partial". **V** Set it to partial with the 2.5b harness evidence.

**Defects to the fix wave (before 1.0.0):**
1. Triage each ❌ as one of:
   - (a) a 1.0.0 blocker, which goes to the fix wave
   - (b) a known issue, which goes into the CHANGELOG `[1.0.0]` "Known issues" section
   - (c) a stale expectation, which gets the row rewritten
2. Verify returns PARTIAL with the list.
3. Run `/plugin-execute O-GranulatorInstrument 4-polish` as a gap wave (e.g. `PLAN-4.F`). Its fixes are path-scoped commits.
4. Re-run the full automated bar: harness, parity, drift, param-dump, renders, auval, pluginval SC on/off, UI gates, `--cpu26`.
5. Retest the ❌ rows plus the adjacent rows in the same pass.
6. VERSION stays 1.0.0 because nothing has been published. Amend only the CHANGELOG body and date.

### C · D54 / D56 release checklist

1. **D54 grep gate**, as plugin-local `tests/windows-webview2-gate.sh`. Exit 1 on any miss.
   - CMakeLists.txt contains each of these:
     - `NEEDS_WEB_BROWSER TRUE`
     - `NEEDS_WEBVIEW2 TRUE`
     - `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`
   - In PluginEditor.cpp, between `#if JUCE_WINDOWS` and the matching `#else` / `#endif`:
     - `withBackend (juce::WebBrowserComponent::Options::Backend::webview2)`
     - `withUserDataFolder (`
   - `withResourceProvider` is present (census per memory `critical_webview2_runtime_gotchas_windows`).
   - SafePointer init-capture count is 0: `= juce::Component::SafePointer` inside a `[`.
   - Negative controls: run the gate on a scratch copy with each line deleted; each run must fail.
2. **Windows CI proof** (Taylor checkpoint):
   - `git push origin main` (repo-wide, 185+ commits).
   - `gh workflow run build-and-release.yml -f plugin_name=O-GranulatorInstrument -f validate_only=true`.
   - Record the run id and the pluginval log. If declined, D54's CI arm is "pending `/publish`" (parent precedent `658161fd`).
3. `CMakeLists.txt:47`: `VERSION 0.1.0` → `VERSION 1.0.0`. Keep the comment. Then re-run the bar.
4. Create `CHANGELOG.md`:
   - `# O-GranulatorInstrument Changelog`
   - **exactly one** version heading, `## [1.0.0] - <date>`
   - sections: Added, Known issues (COMPAT-02 MPE/Dorico untested in a DAW, Windows UI untested by hand, Reaper untested)
   - Nothing else may use `## [`.
5. `PLUGINS.md:76`:
   - Set `🚧 Stage 4` while the stage runs.
   - At verify-complete: `| O-GranulatorInstrument | ✅ Working | 1.0.0 | Synth (Sample-Based Polyphonic Granular) | <date> |`.
   - `/install-plugin` then moves it to 📦 (install-plugin.md:38).
6. `STATUS.md`:
   - stage-4 fields
   - `requirements` checksum (:30) and the param-dump re-pin note (D55)
   - next action `/install-plugin` → `/publish O-GranulatorInstrument`, "publish 1.0.0 as-is"
7. `REQUIREMENTS.md` status rows, per the pass table above.
8. `FORK-MANIFEST.md` take/skip line (Q8).
9. `modules/registry.yaml`: 7 × `0.1.0` → `1.0.0`. Only if the foreign hunks are committed by then; otherwise stage only our own hunks through a temp index (memory `index_git_shared_checkout`), or defer.
10. **No tag, no push of a tag.** `/publish` alone does that.

### C · Pitfalls

- **P1. `/publish` has no "publish the current version" path.** It always computes `NEW_VERSION` from a patch, minor or major bump (SKILL.md:89-104), seds VERSION, prepends a CHANGELOG entry and edits PLUGINS.md (:147-165). **V**
  - With Stage 4 already at 1.0.0, a naive run produces 1.0.1, 1.1.0 or 2.0.0, plus a duplicate entry.
  - Also, its `grep "VERSION" | head -1` (:84) would first hit `cmake_minimum_required(VERSION 3.22)` (CMakeLists.txt:37). **V**
  - No repo tag `*-v1.0.0` exists, so there is no precedent. **V**
  - The handoff must say: publish 1.0.0 as-is; skip steps 2-4; tag `O-GranulatorInstrument-v1.0.0`.
- **P2. A tag pushed before Windows has been proven** fails `create-release` (it needs both builds, :756). A rerun uses the tag's workflow, so a fix means delete and retag (memory). Prove Windows with the validate dispatch first.
- **P3. The factory sentinel is keyed on the version.** Any preset or source edit at an unchanged 1.0.0 (for example in the fix wave) is **not** rewritten on a machine that already ran 1.0.0. Stale factory `.json` files from earlier builds are never deleted; `initializeFactoryPresets` only writes (OuariconPresetManager.h:713-753). **V** The hands-on setup must delete the factory dir and sentinel after every preset change.
- **P4. Path scope.** `PLUGINS.md` and `registry.yaml` are shared. `registry.yaml` is dirty with another session's hunks right now. Commit with `-- plugins/O-GranulatorInstrument PLUGINS.md` only, and re-check `git status --short` right before each commit.
- **P5.** The first MSVC and x86_64 compile of the forked leaves and the `#if JUCE_WINDOWS` block happens in CI. Run the grep scans beforehand, but only a CI run proves it.
- **P6.** The hands-on sheets for 2.1, 2.2, 2.4 and 2.6 describe the DevLoader-era UI and the pre-2.6 "empty slots" behaviour. If they are run unrewritten, ❌s will be false.
- **P7.** Sessions saved with the dev AU/VST3 do not reopen under the release identity (`OuAu`, no `-dev`). Do not expect Logic or Live projects from the session to load the published build. **A**
- **P8.** A release body made from a CHANGELOG with only one bracket heading runs to end of file, so the whole file is the release notes. Keep the file to that single section.

### C · Decisions for the plan

1. **Windows proof.** Recommend a Taylor-confirmed checkpoint after the release-ready commit: push main, then dispatch `build-and-release.yml` with `validate_only=true`. Do not edit `ci-tests.yml` (repo-wide). Fallback if declined: "Windows CI pending /publish".
2. **Version bump timing.** Recommend setting VERSION 1.0.0 in the execute wave that lands the 64 presets, or earlier. The first hands-on install then writes the factory bank under 1.0.0. Re-run the full bar after the bump.
3. **CHANGELOG content.** One `## [1.0.0] - <date>` section, with Added plus Known issues (COMPAT-02, Windows UI, Reaper). Written at the final execute wave and amended by any fix wave.
4. **PLUGINS.md.** `🚧 Stage 4` now; `✅ Working 1.0.0` at verify-complete; 📦 via `/install-plugin`.
5. **registry.yaml.** Recommend bumping the 7 rows to 1.0.0 only through own-hunk staging. Otherwise defer, and note the deferral in STATUS.
6. **Fork drift.** Record-only line for `e77d29c8`. No port, pin unchanged, no parity re-run beyond the normal bar.
7. **SESSION-4 format.** One sheet with back-referenced row IDs and passes ordered Standalone → Presets → Logic → Live → Deferred. Rewrite the stale rows from P6; mark "old patches" rows N/A (harness-covered).
8. **❌ routing.** Every ❌ goes to a Stage 4 gap wave (not to 2-dsp or 3-gui), triaged as blocker, known issue or stale row. The retest covers the ❌ rows plus adjacent rows, with the full automated bar.
9. **COMPAT-02.** Correct REQUIREMENTS.md:64 from `pending` to `partial` (harness 2.5b; DAW pass in 1.0.x).
10. **Handoff text.** The Stage 4 completion handoff names `/install-plugin O-GranulatorInstrument`, then `/publish O-GranulatorInstrument` with the instruction "publish 1.0.0 as-is, no bump" (P1).
11. **Binary data.** Recommend capping any single factory source at ≤ 3 MB FLAC (precedent envelope), with the total ≤ 8 MB (slice A owns the recipes).
