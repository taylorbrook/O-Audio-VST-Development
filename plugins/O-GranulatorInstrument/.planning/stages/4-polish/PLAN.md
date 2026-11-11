---
title: "O-GranulatorInstrument Stage 4 (Polish / Validation) — Execution Plan"
created: 2026-11-11
juce_version: "8.0.15"
phase: "4-polish"
summary: "Six waves: (W0) pre-flight + baselines on the unmodified tree, PLUGINS.md → 🚧 Stage 4; (W1) audio-neutral foundations: harness temp-HOME re-exec, snap-only ranges on 6 params (param-dump defaultText re-pin, overrides regen, spec addendum), AU1 drift-set row, D55c detached WAV writer + USV3, commit C1; (W2) D55a / D55b page + `toast.wavFailed` + lane-embed copy fix, 8 generated factory sources (fp contract off, integer RNG, full-PCM pins, FA3) with the stubs moved to a harness-only target, commit C2; (W3) 64-preset bank (FactoryPresets.h, bank-hash stamp, factory-dir save guard), VERSION 1.0.0, PR0–PR6 + PN1, `--cpu4` per-preset CPU verdict (BG) + content retune loop, commit C3; (W4) build / install / validators, Windows WebView2 grep gate, UI proof (47 states, toast-chain probe rows, five gates), commit C4; (W5) final regression, CHANGELOG / PLUGINS.md / FORK-MANIFEST / REQUIREMENTS / registry, HANDS-ON-SESSION-4, SUMMARY / STATUS, commit C5; then a Taylor-confirmed push + validate-only Windows dispatch (T19). No tag. No forked-leaf edit. No module edit. Renders never re-pinned."
stages: [4]
agents: [polish-agent, build-gate-runner]
signoffs: "2026-11-11 Taylor: D55 scope = all 6 params; Windows proof = push + dispatch checkpoint; auval = document + AU1; stubs = harness-only fixtures"
---

# O-GranulatorInstrument — Stage 4 Execution Plan (Polish / Validation, 1.0.0)

## Goal

1.0.0 becomes release-ready and stays untagged:
- **FUNC-08:** 8 generated factory sources and 64 factory presets that load out of the box, sources only (PD1 / D53).
- **PERF-02:** a per-preset CPU verdict on the real bank.
- **D55:** the four carried defects are fixed.
- **D54:** the Windows build is proven in CI after a push you confirm.
- **D51:** one combined human session sheet (HANDS-ON-SESSION-4) is ready for verify.

**Stage 2 / 3 audio does not move.** Renders, legacy renders and parity stay byte-identical throughout. The param-dump moves only in the 6 `defaultText` cells.

**Out of scope (CONTEXT § Out of scope):**
- Dorico / `.doricolib`, MPE hardware, Reaper.
- Moving the fork pin.
- Real Windows hands-on.
- Formant-preserving scan.
- Running the human session, which is verify's job (D51).
- Tagging, which only `/publish` does.

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/4-polish/CONTEXT.md` | D51–D59. **Binding, not reopened.** |
| `stages/4-polish/RESEARCH.md` | Summary 1–12 (cited **RSx**). Part A (**A§Qx**), Part B (**B§…**), Part C (**C§…**), sign-offs (resolved in the frontmatter). |
| `stages/3-gui/PLAN-3.3.md`, `SUMMARY-3.3.md`, `VERIFICATION-3.3.md` | Carried gate order (PD26), state-file rules (PD18), row naming discipline (PD21), checkpoint procedure. |
| `stages/3-gui/HANDS-ON-*.md`, `stages/2-dsp/HANDS-ON-*.md`, `HANDS-ON-SESSION-3.3.md` | Inputs to SESSION-4 (T17). |
| This plugin | `CMakeLists.txt`, `Source/{GiParamIds.h, PluginProcessor.{h,cpp}, PluginEditor.cpp, GiUiJson.h}`, `Source/dsp/{GiFactory.h, SlotLoader.{h,cpp}, GiHash.h}`, `Source/ui/public/{index.html, js/i18n.js}`, `.planning/mockups/{v1-ui.html, v1-i18n.js}`, `tests/render-harness/{main.cpp, engine_gates_24.h, engine_gates_25b.h, engine_gates_26a.h, engine_gates_26b.h, ui_gates_3x.h}`, `tests/{ui_frontend_check.js, ui_binding_probe.js, i18n-states.json, fork-drift-check.sh}`, `tests/golden/*`, `Source/dsp/forked/FORK-MANIFEST.md`, `.planning/{parameter-spec.md, REQUIREMENTS.md, params.tsv}`. |
| Parent (read, never edit) | `plugins/O-Granulator/Source/FactoryPresets.h` (struct, `validateSpec :847-861`, `makeFactoryPresets :876-891`), `tests/render-harness/main.cpp:877-905` (CFFIXED_USER_HOME re-exec), `engine_gates_4b.h:386-483` (PR4 / PR5 precedent), `tests/preset_name_fit.js`. |
| Repo (run, never edit) | `scripts/{check-i18n, i18n-fr-lint, i18n-zh-lint, check-ui-labels, boot-all-uis, serve-ui}.js`, `scripts/build-and-install.sh`, `scripts/resolve-target.sh`, the FR / zh glossaries. `modules/persistence/preset-manager/cpp/OuariconPresetManager.h` (read only). |

**Plan-time facts** (2026-11-11, HEAD `02f23889`). Last code commit under `Source/ tests/ CMakeLists.txt` = **`34f8e83e`**.

| File | Facts |
|---|---|
| `GiParamIds.h` (500 lines) | `output_gain :231`, `rec_gain :318` (both `floatDef`, interval 0); comment block `:39-44` |
| `PluginProcessor.cpp` (2075) | preset manager setup `:114-117` (excluded `output_gain`, `rec_arm`; validated apply); `saveWorker->stopThread (-1) :143`. **No `initializeFactoryPresets` call exists today** (grep 0); see correction 1. |
| `PluginEditor.cpp` (1228) | `savePresetToFile` branch `:1075-1079`; `isFactoryPreset` native `:1052` |
| `GiUiJson.h` (480) | `slotInfoToVar :129` |
| `GiFactory.h` (105) | `factoryCount :67`, table `:81-82` |
| `CMakeLists.txt` | `VERSION 0.1.0 :47`; factory binary-data target `:173-178`; harness target `:215-223` (`OGI_TEST_HOOKS=1`) |
| `index.html` (5097) | `showToast :3115`, `noteSlotErrors :3736` |
| Tests | `i18n-states.json`: 46 states. `main.cpp` (1723): `--gen-factory :1555`, `--cpu26 :1644`. `ui_binding_probe.js` 2047, `ui_frontend_check.js` 986. Harness **431 / 0** at 3.3 verify. |
| Elsewhere | `FORK-MANIFEST.md` lives at `Source/dsp/forked/FORK-MANIFEST.md`. `CHANGELOG.md` does not exist. `PLUGINS.md:76` = `🚧 Stage 1 | 0.1.0`. |

**Line numbers drift: re-grep before editing.**

---

## Research corrections (checked at plan time)

1. **The factory-bank write is not live yet.** RS7 and A pitfall 1 say the ctor writes `~/Library/O-GranulatorInstrument/Presets/Factory`. Today nothing calls `initializeFactoryPresets` (grep: 0 hits in `Source/`). The write starts when T11 registers the bank.
   - So the temp-HOME re-exec (T2) must land and be proven **before** T11. Once T11 lands, any harness run without the re-exec writes the real bank.
2. **The FORK-MANIFEST path** is `Source/dsp/forked/FORK-MANIFEST.md`, not the plugin root (C§Q8 left the path implicit).

Everything else checked matches RESEARCH: snap-candidate rows, the toast and slot-error sites, `stopThread(-1)`, the save-guard hole, the version line, the PLUGINS.md row and the state count of 46.

---

## Plan-phase decisions

### Bank and sources

**PD1 Bank (A§Q1, RS1).**
- 8 categories × 8: Drones, Expressive, Keys, Live, Motion, Pads, Rhythmic, Textures.
- Names are `"<Cat> · <Name>"` (U+00B7), ≤ 24 chars, no `/ \ :`, not localized, in a flat menu (no page change, D40 / D48).
- Feature coverage per the A§Q1 table is a PR0 assertion, not a guideline.
- Realism-titled Keys presets ("Bell", "Celesta", …) get `pitch_rand` 0 and no pitch-LFO route.

**PD2 Authoring.**
- `Source/FactoryPresets.h` ports the parent spec struct and adds `slots[]` = `{index 0–3, source "factory:<id>", name}` with **no `contentHash`** (A pitfall 4).
- Values are in engineering units, converted once through the parameter's own `convertTo0to1`; choices are by INDEX, dests through `modDest("id")`.
- `validateSpec` rejects:
  - `output_gain` or `rec_arm`;
  - an unknown id;
  - an out-of-range value;
  - a missing seed;
  - an unknown `factory:` id;
  - an enabled stream without a listed slot (Live presets exempt).
- `customState` = `{seed: "<int64 string>", custom?, masks?, slots}`.
- Live presets list **no** `slots` key (D45). "Live · Init" resets every parameter and the scale and keeps the lanes.

**PD3 Bank registration and stamp.**
- `initializeFactoryPresets (makeFactoryPresets())` runs in the processor ctor, after `setExcludedParameters` / `setValidatedApply`.
- **Before** it runs, a bank-hash stamp is checked: XXH64 over the canonical serialisation of the 64 defs, stored in `Factory/.bank-hash`. On a mismatch or a missing file, delete `Factory/*.json`, `.factory-version` and `.bank-hash`, init, then write the new stamp.
- The delete is restricted to `*.json` directly inside `getFactoryPresetsDirectory()`. The stamp write is the last step, so an interrupted init re-runs.
- This covers fix-wave edits at an unchanged 1.0.0 (C pitfall P3) and renamed-preset orphans.

**PD4 Factory-dir save guard.**
- In `savePresetWithDialog` (`PluginEditor.cpp:1075-1079`), a target `isAChildOf (getFactoryPresetsDirectory())`, or equal to it, is refused with the existing `toast.saveFailed` path. No new key.
- Row **PG1** plus a control: the guard disabled under a test flag → the factory file is overwritten.

**PD5 Sources (A§Q4, RS2).** The 8 recipes from the A§Q4 table, all 48 kHz / 24-bit:

| Id | Content |
|---|---|
| `harmonic` | additive partials |
| `pluck` | KS-like decaying partials |
| `bell` | inharmonic partials |
| `vowel` | formant pulse train |
| `noise_bed` | stereo noise, 16-bit resolution |
| `dust` | stereo crackle + band noise, 16-bit resolution |
| `drone` | stereo |
| `pulse` | 120 bpm |

- Every pitched source has its fundamental (the bell: its prime) at **C3 = 130.8128 Hz**.
- Levels: tonal −12 dBFS peak; noise −18 dBFS RMS.
- Generator: `--gen-factory` grows a recipe table and a size print-out. It lives in its own `tests/render-harness/factory_gen.cpp` compiled with `-ffp-contract=off` (plus `#pragma clang fp contract(off)` as a belt). It uses integer RNG only (splitmix64 / `eg21::hashNoise`), double phase accumulators and no `std::*_distribution`.
- It refuses to overwrite without `--force` (carried).

**PD6 Pins.** Per source in `GiFactory.h`:
- `kHash<Id>`: the existing 131 072-frame `contentHash`.
- **`kFull<Id>`**: XXH64 over the full decoded PCM, all platforms.
- File sha256 goes in `tests/golden/factory-sources.sha256` (mac arm64 regeneration only, the 2.6 precedent).
- **FA3** checks:
  - Σ FLAC bytes ≤ 8 388 608;
  - each FLAC ≤ 3 145 728 (MSVC envelope, C§Q7);
  - decode + channels / rate / frames == recipe;
  - both hashes == pin;
  - every id resolves;
  - `factoryCount() == 8` in the shipping build.
- **FA3n:** one pin perturbed → FAIL.

**PD7 Stubs become harness-only fixtures** (sign-off 4).
- `stub_tone.flac` / `stub_noise.flac` move to `tests/fixtures/factory/` in a second binary-data target, `OuariconGranulatorInstrument_TestFactory`, linked **only** to the render harness.
- `GiFactory.h` appends them after the 8 curated entries under `#if OGI_TEST_HOOKS`, so the harness sees 10.
- FA1 changes from `== 2` to "the two stub ids resolve with their carried pins". This is a planned amendment and is recorded; the row name is unchanged.
- The 21 carried stub references and `generic-overrides.json`'s stub peaks stay byte-identical.
- X2 gains `stub_` in its symbol scan: 0 in shipping binaries.

### `output_gain` and auval

**PD8 Snap-only ranges (D55 scope = 6 params, sign-off 1).**
- Applies to `output_gain`, `rec_gain` and `s1`–`s4_level`.
- Build them as `NormalisableRange<float>(lo, hi, nullptr, nullptr, snapClampZero)`:
  - `snapClampZero` = clamp to `[start, end]`, then `|v| < 1e-5f → 0.0f`;
  - no interval; skew 1 is kept.
- One helper in `GiParamIds.h`, and the `:39-44` comment is updated.
- Write a `parameter-spec.md` **addendum** under the `:36` convention, plus one for `:355` (D53, presets carry sources, no blobs). Both are appended notes; LOCKED text is not edited.
- Re-pin:
  - `.planning/params.tsv`: `defaultText` on rows 13 / 30 / 68 / 106 / 144 / 185 → `0.0000000`; **any other cell moving = STOP**;
  - STATUS `parameter_spec` checksum re-hashed (the addendum is appended); `parameter_spec_body` must stay unchanged, and if it moves → STOP;
  - STATUS records the reason;
  - `generic-overrides.json` is regenerated twice, byte-identical, and the diff is limited to those 6 defaults.
- Document the APVTS raw atomic being seeded unsnapped (A§Q5) as a code comment.

**PD9 AU1 (sign-off 3).**
- Harness row: for all 261 real `AudioParameterFloat` / choice / bool params, compute `to01(from01(to01(def))) != to01(def)` and assert the set == `{lfo1_rate…lfo4_rate}`.
- **AU1n:** a temporary range with skew 0.3 at a drifting default joins the set.
- AU1 runs after PD8 and proves the snap ranges add no 5th member.
- CHANGELOG "Known validator notes" gets one line.

### D55 carried fixes

**PD10 D55a (B§D55a).**
- `slotInfoToVar` emits `wavFailed`.
- The page's `noteSlotErrors` edge-tracks `wavFailed` the same way it tracks `error`: the first pull is not news; a false → true transition toasts `toast.wavFailed {n}`.
- No recorder, telemetry or RT change.
- Rows:
  - **UWF1:** `hooks.failWav` → commit → `wavFailed == true` and `slotRev` moved; the next good take → `false`.
  - **UWF1n:** the field is omitted under a test flag → the probe stays silent.

**PD11 D55b.**
- A batch-scoped ascending chain inside `noteSlotErrors`: collect the lanes with a new error or `wavFailed`, show the first immediately, and schedule the rest at +1600 ms steps. Store the timers and clear them at the next batch.
- Other toasts keep replace-now behaviour.
- **No global FIFO** (ST1 timing).

**PD12 D55c.**
- The WAV write moves to a detached writer thread that owns `shared_ptr<WritePayload>` + `shared_ptr<SaveShared>{abandon, state, error}`.
- `writeSaveWav` becomes member-free, checks `abandon` per chunk and deletes `.part` on abandon.
- SaveWorker waits on `state` with the existing exit-polled 2 ms loop; on exit it sets `abandon` and delivers `cancelled` once.
- `stopThread(-1)` stays; it is now bounded. **Never `stopThread(timeout)`.**
- Test hooks: `testHoldWrite` (a non-polling WaitableEvent before chunk 0), `testInlineWrite` (the control) and a static `writerExits`.
- Row **USV3:**
  - dtor ≤ 500 ms while the writer is held;
  - after release: `writerExits == 1` within 2 s, `.part` gone, completion delivered once (`cancelled`).
- **USV3n:** with `testInlineWrite`, the dtor takes ≥ the 3 s hold.
- USV1 / USV2 must stay green.
- Residual risks are recorded, not fixed: a detached thread across a bundle unload, and `~SlotLoader`'s 10 s pool join.

**PD13 Strings (D59).**
- One new key, **`toast.wavFailed`**:
  - en "Slot {n}: the take plays, but its WAV file could not be written"
  - fr "Emplacement {n} : la prise joue, mais son fichier WAV n’a pas pu être écrit" (`reviewed: false`)
  - zh-Hans "插槽 {n}：录音可播放，但 WAV 文件写入失败" (`reviewed: 'mt'`)
- One copy fix: the `lane-embed` help drops "and presets" in EN / FR / zh, and its `reviewed` flags are reset per suite rules.
- New state `toast-wav-failed` (46 → **47**), appended at the end of the state file (PD18 of 3.3: new states go last).
- Every page or i18n edit lands in **both** pair files (`index.html` / `v1-ui.html`, `js/i18n.js` / `v1-i18n.js`).
- fr `vstate.held` "tenue" is a SESSION-4 row, not an edit.

### Proof rows and CPU

**PD14 Preset rows** (A§Q3, new header `tests/render-harness/preset_gates_4.h`, flag `--pg4`, also in the full set):

| Row | Check |
|---|---|
| **PR0** | The PD2 table rules plus coverage per category. PR0 also gates the source table for `output_gain`. |
| **PR1** | Apply from fresh: engineering read-back == table (interval / snap tolerance), absent == default (exact for the PD8 six), `output_gain` −7 dB survives, listed slots `loaded` / `resolvedBy == 4` / hash == pin, `fsTouches == 0`. **PR1n:** linear-fraction `s1_size` fails. **PR1n2:** `file:` slot → `fsTouches > 0`. |
| **PR2** | 20 s per preset on a fixed note script: finite, bounded, RMS > −60 dBFS, bank loudness spread ≤ 9 dB. Live presets preload factory sources into lanes 1–4. Balance by voicing, never `output_gain`. |
| **PR3** | Rhythmic presets lock to tempo at 120 and 93 bpm through the TS playhead fixture. |
| **PR4** | Load after dirty: on one instance, randomise 259 params + custom scale + 3 masks + seed + foreign `file:` lanes, then apply B. A 10 s render must be bitwise-equal to fresh-apply-B, over 320 ordered pairs. Non-vacuity: 64 fresh renders all distinct. |
| **PR4b** | Load after play: param + customState snapshot == fresh B; listed slots `dirty == false` with the generation bumped. |
| **PR5** | Real path under the temp HOME: the ctor writes 64 files + `.factory-version` + `.bank-hash`; read-back == table. Save as user → reload on a fresh instance: equal, and the render is bitwise-equal. Stamp arm: edit one stored file + change the stamp → the next ctor rewrites it; an orphan `Old · Name.json` is deleted. |
| **PR6** | No click on a switch while playing: CK6 metric, ~8 pairs (A§Q3). Max \|d2\| over [0.9, 1.3] s ≤ 2.0 × the larger reference. **NC:** `swapKillHard`. |
| **PG1** | Save guard (PD4) + control. |
| **FA3** | PD6. |
| **AU1** | PD9. |
| **UWF1** | PD10. |
| **USV3** | PD12. |

- Every new name is grepped against `tests/render-harness/*` and must have 0 prior hits (T1).
- Every `n` row must FAIL its predicate. NOT FIRED = 0.

**PD15 Temp HOME** (A pitfall 1, B pitfall 3).
- At `main()` entry the harness captures the **real** home (for GD1 and the overrides hygiene check), then re-execs once with `CFFIXED_USER_HOME=<mkdtemp>` (the parent `main.cpp:877-905` pattern).
- It verifies `File::getSpecialLocation(userHomeDirectory)` == the temp dir before any processor is built, and builds one processor before the render pools.
- **HM1:** after a full run, `~/Library/O-GranulatorInstrument/Presets/Factory` mtime / listing is unchanged, and no new file appears under the real `~/Music/Ouaricon/O-GranulatorInstrument/`.
- **HM1n:** `--no-home-redirect` → the probe sees the temp dir unused.

**PD16 `--cpu4`** (B§Q2, RS4). Run alone, in the background.
- It first runs the `--cpu26` rows (C25m ≤ 40, C25tm ≤ 12) and prints the session factor C25tm / 6.91.
- Per preset:
  1. `make()` → `shipping()` → `loadPresetFromFile(<in-repo JSON>)` → `waitForIdle(20000)` + 2048-sample render.
  2. Assert `fsTouches == 0` and the listed slots `loaded`.
  3. Run **T** (6-note chord {48,52,55,59,62,64}) and **W** (16 held notes, 40–55) through `measure25` unchanged.
- Sweep at 5 × 6 s + 3 s warm-up. Borderline (T > 10.5 or W > 36) or NOVERDICT → full 7 × 10 s; that median decides. Two invalid runs → FAIL.
- **Gate per preset:** T ≤ 12 %, W ≤ 40 %.
- INFO lines: bank median / p90 / max T, max W, top-5, effective voices and live grains.
- Special cases:
  - Live presets load a factory stand-in into the rec target (flagged "stand-in").
  - Rhythmic presets run at 120 bpm through the TS fixture.
  - MPE presets get member-channel input as INFO.
- In-repo JSON comes from `--dump-factory-presets <dir>` (the same `makeFactoryPresets()` → JSON as the manager writes). It never calls `initializeFactoryPresets`.
- **Overrun:**
  1. Retune that preset's content (density / size / streams / `grain_limit` / FX), then re-run PR2 / PR4 for it.
  2. Then the D21 levers 1 → 3, each with renders proven byte-identical.
  3. A 256 → 128 default = **stop and ask**.

### Release

**PD17 Release metadata (D56, C§D54/D56).**
- `CMakeLists.txt:47` `VERSION 1.0.0` lands in T11, the same wave as the bank, so the first install writes the bank under 1.0.0. The comment is kept.
- `CHANGELOG.md`:
  - `# O-GranulatorInstrument Changelog`, then **exactly one** `## [1.0.0] - 2026-11-xx`;
  - sections Added / Known issues / Known validator notes;
  - no other `## [`.
- Known issues: COMPAT-02 (MPE / Dorico untested in a DAW), Windows UI untested by hand, Reaper untested, `~SlotLoader` 10 s join on a hung disk, the detached-writer bundle-unload residual.
- `PLUGINS.md:76`: `🚧 Stage 4 | 1.0.0` in this stage. `✅ Working` is set at verify-complete, not here.
- `REQUIREMENTS.md`: COMPAT-02 `pending` → `partial` (2.5b harness evidence). FUNC-08 and PERF-02 → `partial` (automated arm green; the human arm runs at verify). Nothing else moves; the rest move at verify.
- `Source/dsp/forked/FORK-MANIFEST.md` take / skip log gets the line: `e77d29c8 — UI only (index.html value entry), no leaf; taken at 3.1 D41; skip; pin unchanged`.
- `modules/registry.yaml`: 7 × `used_by O-GranulatorInstrument version 0.1.0 → 1.0.0`, **only through own-hunk staging** with a temp index (memory `index_git_shared_checkout`). If the foreign hunks make that unsafe → defer, and say so in STATUS.

**PD18 Windows.**
- `tests/windows-webview2-gate.sh` checks: `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, and `withBackend (…webview2)` + `withUserDataFolder (` inside the `#if JUCE_WINDOWS` block. It also checks `withResourceProvider` present and SafePointer init-capture count 0.
- **NCs:** each required line deleted on a scratch copy → exit 1 (6 runs).
- CI proof = T19 (sign-off 2).

**PD19 HANDS-ON-SESSION-4 (D51, C§Q9).**
- One sheet, with row IDs back-referenced (`2.1-4`, `3.2-13`).
- Passes, in order:
  - **0 Setup:** includes wiping `Presets/Factory` + `.factory-version` + `.bank-hash`, and quitting Logic / Live before install.
  - **1 Standalone.**
  - **2 Preset audition** (64 rows plus the proof rows: load after dirty, OUTPUT untouched, switch while holding a chord, lanes named, save / reload, realism titles don't wobble).
  - **3 Logic AU** (SESSION-3.3 L1–L6 + the C§Q9 list + CPU meter on the 3 heaviest presets at 48 k / 128).
  - **4 Live VST3** (2.6 r3 Live arm + LV1–LV5).
  - **5 Deferred** (MPE, Dorico, Reaper, W1 / W2).
- New D55 rows: read-only Recordings → `toast.wavFailed`; two bad drops → two toasts; OUTPUT reads exactly 0.0 dB; fr "tenue".
- Stale rows (C pitfall P6) are rewritten. The "old patches" rows are marked N/A, citing the harness row.
- The old sheets get a one-line "superseded by HANDS-ON-SESSION-4" header.
- The ❌ triage section routes each ❌ to blocker → gap wave `PLAN-4.F`, known issue → CHANGELOG, or stale → rewrite. VERSION stays 1.0.0.
- Requirement-per-pass table from C§Q9.

**PD20 Audio-neutral checkpoint** (after T3, T5, T7 and T11, one per task).
- Build the harness (BG).
- `--dump-render-hashes` == `tests/golden/renders-2.4.sha256`; legacy == `renders-2.2.sha256`; parity 168 / 0 + GV 2 / 0.
- param-dump == `params.tsv` (the T3 re-pinned version from T3 on).
- A move → STOP and attribute it to that task. Renders are **never** re-pinned.

---

## Tasks

**Roles.**
- **[agent]** = polish-agent writes files.
- **[run]** = builds, gates, validators, commits.

**Conventions.**
- **BG** = `run_in_background`, logging to `$S/<name>.log`.
- Anything over ~5 min runs BG (600 s watchdog); `--cpu4` (~12 min) and the full harness are always BG.
- `$H` = the render-harness binary under `build/plugins/O-GranulatorInstrument/O-GranulatorInstrument-render-harness_artefacts/Release/`.
- Commits are path-scoped: `git commit -- plugins/O-GranulatorInstrument PLUGINS.md` (plus `modules/registry.yaml` only in C5 via the temp index). Re-check `git branch --show-current` and `git status --short` immediately before each commit.

### Wave 0 — pre-flight

1. **T1 — pre-flight + baselines** [run]. Output: `$S/preflight.txt`.
   - Contracts re-hashed against the STATUS frontmatter.
   - `git log 34f8e83e..HEAD -- plugins/O-GranulatorInstrument/{Source,tests,CMakeLists.txt}` is empty.
   - Branch is `main`.
   - Both pairs `cmp` clean.
   - Every Plan-time fact symbol re-grepped; drift recorded.
   - Baselines on the unmodified tree:
     - harness **431 / 0** (BG; carried extract **B**)
     - parity 168 / 0 + GV 2 / 0
     - render hashes, param-dump
     - drift 0 / 13
     - overrides diff-clean
     - closure, probe (261 × 2 + VE 14 + lanes 12 + mod 14)
     - five UI gates green (`default + 46`)
     - auval: exactly the 4 LFO-rate warnings recorded
   - The real `~/Library/O-GranulatorInstrument/Presets/` listing is snapshotted (expected: absent or no Factory dir).
   - PD14 row names grepped → 0 hits each.
   - PLUGINS.md:76 → `🚧 Stage 4 | 0.1.0` (the version moves in T11). Committed with C1.
   - Any mismatch → STOP and ask.
   - Depends on: none

### Wave 1 — audio-neutral foundations

2. **T2 — harness temp HOME** (PD15) [agent + run]. Rows HM1 / HM1n. GD1 and the overrides hygiene check read the captured real home, and both stay non-vacuous (assert the stimulus).
   - Files: `tests/render-harness/main.cpp`
   - Depends on: T1
3. **T3 — snap-only ranges + re-pin + AU1** (PD8, PD9) [agent + run].
   - Includes the spec addenda (`:36`, `:355`).
   - `params.tsv` re-pin: exactly 6 cells.
   - Overrides regen ×2; the diff is the 6 defaults only.
   - AU1 / AU1n.
   - Checkpoint (PD20).
   - Files: `Source/GiParamIds.h`, `.planning/params.tsv`, `.planning/parameter-spec.md` (addendum only), `tests/render-harness/preset_gates_4.h` (AU1), `tests/render-harness/main.cpp`, `tests/ui-stub/generic-overrides.json`
   - Depends on: T2
4. **T4 — D55c detached writer** (PD12) [agent]. USV3 / USV3n; USV1 / USV2 green.
   - Files: `Source/PluginProcessor.{h,cpp}`, `tests/render-harness/ui_gates_32.h` or `preset_gates_4.h` (wherever USV1 / USV2 live: re-grep)
   - Depends on: T1 (independent of T2 / T3; written beside them)
5. **T5 — neutral proof + commit C1** [run].
   - Full harness (BG): carried extract diff vs **B** is **empty**; HM1, AU1, USV3 and their NCs behave.
   - Checkpoint (PD20).
   - Drift 0 / 13.
   - Commit C1.
   - Depends on: T3, T4

### Wave 2 — page fixes, factory sources

6. **T6 — D55a / D55b + strings** (PD10, PD11, PD13) [agent].
   - `slotInfoToVar` `wavFailed`; the chain in `noteSlotErrors` (both HTML files); `toast.wavFailed` + the `lane-embed` copy fix (both i18n files); state `toast-wav-failed` appended (47).
   - UWF1 / UWF1n harness rows.
   - Files: `Source/GiUiJson.h`, `Source/ui/public/index.html`, `.planning/mockups/v1-ui.html`, `Source/ui/public/js/i18n.js`, `.planning/mockups/v1-i18n.js`, `tests/i18n-states.json`, `tests/render-harness/preset_gates_4.h`
   - Depends on: T5
7. **T7 — 8 factory sources + stubs to fixtures** (PD5, PD6, PD7) [agent + run].
   - Run `factory_gen.cpp` + `--gen-factory` recipe table; generate twice and compare bytes (**byte-identical** regeneration on mac arm64).
   - Size print-out vs the PD6 caps.
   - Stub FLACs `git mv` to `tests/fixtures/factory/`; the second binary-data target; the `#if OGI_TEST_HOOKS` tail.
   - FA1 amended (planned); FA3 / FA3n; X2 `stub_` scan.
   - Checkpoint (PD20). The stub-based rows and renders must be unchanged.
   - Files: `Resources/factory/*.flac` (8 new), `tests/fixtures/factory/stub_{tone,noise}.flac`, `CMakeLists.txt`, `Source/dsp/GiFactory.h`, `tests/render-harness/{factory_gen.cpp, main.cpp, engine_gates_26b.h}`, `tests/golden/factory-sources.sha256` (+ `factory-stubs.sha256` path note)
   - Depends on: T5
8. **T8 — commit C2** [run].
   - Full harness (BG): **B** empty except FA1 (planned amendment, same name, PASS).
   - Overrides diff-clean (the stubs still feed them).
   - Five UI gates on 47 states (a quick pass here; the full sequence runs at T15).
   - Commit C2.
   - Depends on: T6, T7

### Wave 3 — the bank

9. **T9 — `FactoryPresets.h` (64)** (PD1, PD2) [agent].
   - Category by category, with seeds fixed as int64 literals.
   - `--dump-factory-presets <dir>` mode.
   - Files: `Source/FactoryPresets.h`, `tests/render-harness/main.cpp`
   - Depends on: T8
10. **T10 — preset rows** (PD14) [agent + run].
    - PR0–PR6, PG1 skeleton, against `makeFactoryPresets()` with no disk except PR5.
    - Iterate on preset content until PR0 / PR1 / PR2 / PR4 / PR6 pass. Fixes go into the table, never into the gates.
    - Files: `tests/render-harness/preset_gates_4.h`, `main.cpp`
    - Depends on: T9
11. **T11 — registration, stamp, save guard, VERSION** (PD3, PD4, PD17 VERSION) [agent + run].
    - Pre-check: T2's HM1 is green on this tree **before** the first harness run that constructs a processor.
    - PR5 / PG1 live.
    - Checkpoint (PD20).
    - Files: `Source/PluginProcessor.{h,cpp}`, `Source/PluginEditor.cpp`, `CMakeLists.txt`
    - Depends on: T10
12. **T12 — `--cpu4`** (PD16) [agent + run, **BG**].
    - Mode in `tests/render-harness/preset_gates_4.h` + `main.cpp`.
    - Run alone with the machine idle. Retune loop per PD16; each retune re-runs PR0 / PR1 / PR2 / PR4 / PR6 for that preset.
    - Record session factor, bank median / p90 / max, top-5.
    - A 256 → 128 or engine lever need = STOP and ask.
    - Depends on: T11
13. **T13 — commit C3** [run].
    - Full harness (BG): **B** empty except FA1. All new rows PASS; every `n` row FAILS; NOT FIRED 0.
    - Parity, renders, param-dump (T3 pin), drift 0 / 13.
    - HM1 green.
    - `--cpu4` verdict PASS (log path recorded).
    - Commit C3.
    - Depends on: T12

### Wave 4 — build, validators, UI proof

14. **T14 — build / install / validate** [run].
    - Build VST3 + AU + Standalone + consoles, **0 warnings in plugin sources**.
    - `./scripts/build-and-install.sh O-GranulatorInstrument` (cache clear, dual-variant sweep).
    - auval: PASS with **exactly** the 4 LFO-rate warnings.
    - pluginval 10 VST3 + AU, sidechain on, then off (`OGI_GATE_SC_DEFAULT_OFF`); shipping build restored + reinstalled.
    - X2 0 (incl. `stub_`).
    - First-run check: the plugin instantiated once via auval wrote `~/Library/O-GranulatorInstrument/Presets/Factory` with 64 files + `.bank-hash`; listing recorded.
    - `tests/windows-webview2-gate.sh` + 6 NCs (PD18).
    - Standalone smoke (window capture only): open the preset menu (64 entries, names unclipped), load one preset per category, read each shot.
    - Files: `tests/windows-webview2-gate.sh`
    - Depends on: T13
15. **T15 — UI proof** (PD11, PD13) [agent + run].
    - Probe rows: **TC1** (two errors in one pull → `{n:2}` at +150 ms, `{n:3}` at +1750 ms), **TC1n** (chain disabled → `{n:3}` never), **TW1** (`wavFailed` edge → toast once; the first pull is silent).
    - PN1 / PN1n preset-name fit (port of the parent `preset_name_fit.js` as `tests/preset_name_fit.js`; EN / FR / zh-Hans; a 40-char name overflows).
    - Closure updated for the `wavFailed` slot field.
    - Full PD26 gate order: overrides → `ui_frontend_check.js` → `ui_binding_probe.js` → check-i18n → fr-lint → zh-lint → check-ui-labels (`states: default + 47`; the fr toast fits `#toast`) → boot-all-uis `--strict-tips`.
    - A new collision or fit failure is a D48 defect: fix it in both HTML files and re-run the sequence.
    - Files: `tests/ui_binding_probe.js`, `tests/ui_frontend_check.js`, `tests/preset_name_fit.js`
    - Depends on: T14
16. **T16 — commit C4** [run]. Both pairs `cmp` clean. Commit.
    - Depends on: T15

### Wave 5 — release-ready + docs

17. **T17 — docs and metadata** (PD17, PD19) [agent].
    - `CHANGELOG.md`; PLUGINS.md row `🚧 Stage 4 | 1.0.0`; REQUIREMENTS (COMPAT-02 / FUNC-08 / PERF-02 → partial); FORK-MANIFEST line.
    - `HANDS-ON-SESSION-4.md` + superseded headers on the old sheets + stale-row rewrites.
    - `SUMMARY.md` (complex template): rows, counts, NC results, `--cpu4` table, the params.tsv re-pin reason, deviations, residual risks, open items.
    - Files: `CHANGELOG.md`, `../../PLUGINS.md`, `.planning/REQUIREMENTS.md`, `Source/dsp/forked/FORK-MANIFEST.md`, `.planning/stages/4-polish/{HANDS-ON-SESSION-4.md, SUMMARY.md}`, `.planning/stages/{2-dsp,3-gui}/HANDS-ON-*.md` (header lines + the stale rows only)
    - Depends on: T16
18. **T18 — final regression + commit C5** [run].
    - Full harness (BG) vs **B** + new rows; parity; renders + legacy; param-dump (T3 pin); drift 0 / 13; X2 0.
    - `--cpu26` once (BG). auval; pluginval 10 VST3 + AU SC on / off.
    - Final install; `auval -a | grep -i granulatorinstrument`; `cmp` both pairs.
    - CHANGELOG heading check: `grep -c '^## \[' CHANGELOG.md` == 1, and the CI awk extract over the file prints a non-empty body.
    - STATUS → `stage_4_execute_complete`, with the requirements checksum re-hashed and the param-dump re-pin note. Next: `/plugin-verify O-GranulatorInstrument 4-polish`.
    - registry.yaml via the temp index (PD17) or deferral recorded.
    - Commit C5.
    - Depends on: T17
19. **T19 — Windows CI checkpoint** (PD18, sign-off 2) [run, **Taylor confirms first**].
    - Present: the commit count ahead of origin, the sessions it includes, and the dispatch command. Wait for an explicit go-ahead.
    - On go: `git push origin main`, then `gh workflow run build-and-release.yml -f plugin_name=O-GranulatorInstrument -f validate_only=true`. Record the run id; wait (external, poll at ~8 min).
    - Record macOS universal build + notarise, Windows VST3 build + pluginval 10 + Inno, and that `create-release` was skipped.
    - A failure → fix on the tree (path-scoped), re-run T18's affected gates, re-dispatch. Never tag.
    - If declined: SUMMARY / STATUS record "D54 CI arm pending `/publish`".
    - Depends on: T18

---

## Dependency graph

```
T1 ─┬─ T2 ── T3 ─┐
    └─ T4 ───────┴─ T5 ─┬─ T6 ─┐
                        └─ T7 ─┴─ T8 ── T9 ── T10 ── T11 ── T12 ── T13 ── T14 ── T15 ── T16 ── T17 ── T18 ── T19
```

Checkpoint order: T3 → T5 → T7 → T11 (shared build dir; a render move is attributed to one task).

## Commit split

| Commit | Content | Tasks |
|---|---|---|
| C1 | temp HOME, snap-only ranges + params.tsv re-pin + spec addenda + overrides, AU1, detached writer, PLUGINS.md 🚧 Stage 4 | T1–T5 |
| C2 | `wavFailed` + toast chain + `toast.wavFailed` + lane-embed copy, 8 factory sources + pins, stubs → harness fixtures | T6–T8 |
| C3 | 64-preset bank, stamp, save guard, VERSION 1.0.0, PR rows, `--cpu4` | T9–T13 |
| C4 | Windows gate script, probe / closure rows, PN1, any D48 fix | T14–T16 |
| C5 | CHANGELOG, PLUGINS.md 1.0.0, REQUIREMENTS, FORK-MANIFEST, HANDS-ON-SESSION-4, SUMMARY, STATUS (+ registry via temp index) | T17–T18 |

---

## Gate criteria (Stage 4 execute)

| Gate | Pass |
|---|---|
| Harness | carried extract diff vs **B** empty except FA1 (planned amendment); new rows PASS (HM1, AU1, USV3, UWF1, FA3, PR0–PR6, PG1); every `n` row FAILS; 0 NOT FIRED |
| Parity | 168 / 0 + GV 2 / 0 |
| Renders | renders-2.4 + legacy byte-identical at every PD20 checkpoint and on the final tree |
| param-dump | == the T3 pin. The diff vs the 3.3 pin is the 6 `defaultText` cells, nothing else. |
| Fork drift | 0 / 13; FORK-MANIFEST line present |
| Factory | 8 sources, Σ ≤ 8 388 608 B, each ≤ 3 MB, both hashes pinned, regeneration byte-identical (arm64) |
| Presets | 64, PR0–PR6 PASS, stamp + guard proven, `fsTouches 0` |
| CPU | `--cpu4`: every preset T ≤ 12 %, W ≤ 40 %; `--cpu26` C25m ≤ 40, C25tm ≤ 12 |
| Home hygiene | HM1: no harness write under the real `~/Library/O-GranulatorInstrument` or `~/Music/Ouaricon/O-GranulatorInstrument` |
| Build | VST3 + AU + Standalone + consoles, 0 warnings in plugin sources; VERSION 1.0.0 |
| auval | PASS, warnings == exactly the 4 LFO rates |
| pluginval 10 | VST3 + AU SUCCESS, SC on and off |
| X2 | 0 test-hook symbols, 0 `stub_` in shipping |
| Windows gate | script exit 0; 6 NCs exit 1 |
| UI | closure 0 fail; probe 261 × 2, VE 14, lanes 12, mod 14, TC1 / TW1 PASS, NCs FAIL; PN1 PASS / PN1n FAIL; check-i18n · fr · zh 0 · 0 · 0; check-ui-labels 0 with `default + 47`; boot-all-uis 0 DEAD |
| `cmp` | both pairs clean |
| Release | CHANGELOG single `## [1.0.0]`; PLUGINS.md `🚧 Stage 4 \| 1.0.0`; no tag created |

Exit 77 anywhere = not run = not a pass.

## Risks / watch items

- **The real bank write** begins at T11. A harness run without the re-exec, or a pluginval / auval run, writes `~/Library/...`. That write is expected for the installed plugin and **must not** happen from the harness (HM1).
- **Cross-arch FMA** in the generator. Only checked-in FLACs ship. Regeneration identity is claimed on arm64 only; x86_64 drift is not a defect.
- **Preset content vs gates.** The 64 presets will be iterated against PR2 (loudness), PR4 (bitwise reload) and `--cpu4`. Fix the table, never the threshold.
- **PR4 vacuity.** Needs an enabled stream with a listed slot (PD2), and all 64 fresh renders distinct.
- **Mid-apply tearing** in a real host is invisible to the harness (A pitfall 7). It is a SESSION-4 Logic / Live row.
- **Stamp delete scope.** Only `*.json` directly in the factory dir, plus the three sentinels. PR5 asserts that a user file in `User/` survives.
- **params.tsv re-pin** is the only contract move. Any 7th cell = STOP.
- **T19 is outward-facing and repo-wide.** No push without an explicit go-ahead in that moment. Approval to plan is not approval to push.

## Deviations protocol

Record every departure in SUMMARY.md (what, why, evidence, which gate). PD7's FA1 amendment and PD8's re-pin are planned and are recorded as such.

Stop and ask only for:
- an audio-moving change;
- a LOCKED-contract edit beyond PD8;
- a module or `scripts/` change;
- a forked-leaf edit;
- a page design change beyond a defect (D48);
- a carried-row change other than FA1;
- a `grain_limit` 256 → 128 default or a D21 engine lever;
- any push / tag.

## Carried (not Stage 4)

- COMPAT-02 DAW pass (MPE, Dorico) → 1.0.x.
- Reaper pass.
- Windows hands-on (W1 / W2).
- `~SlotLoader` 10 s join on a hung disk (known issue).
- Factory-source browser UI.
- Blobs in presets.
- Float WAV for hot takes.
- README row (left to `/publish`).

## Success criteria (gate for verify)

- [ ] 8 generated factory sources ship within budget with full-PCM pins; stubs are harness-only (FA1, FA3, X2)
- [ ] 64 factory presets load from fresh, after dirty state and after play, round-trip through save, never touch `output_gain`, resolve lanes without disk, switch without clicks (PR0–PR6)
- [ ] Factory bank refreshes on any bank change at a fixed version; factory files cannot be overwritten from the editor (PR5 stamp arm, PG1)
- [ ] Every factory preset meets T ≤ 12 % and W ≤ 40 % (`--cpu4`); `--cpu26` regression rows hold
- [ ] `output_gain`, `rec_gain`, `s1`–`s4_level` default to exactly 0 dB; param-dump moved only in those 6 `defaultText` cells; renders unchanged
- [ ] auval warnings frozen at the 4 LFO rates (AU1) and documented
- [ ] D55a–c: `wavFailed` toast, one toast per failing lane, bounded close on a hung disk (UWF1, TC1, TW1, USV3)
- [ ] Harness never writes the real home (HM1)
- [ ] All five UI gates green with 47 states; PN1 fit in EN / FR / zh-Hans; both pairs `cmp` clean
- [ ] Windows WebView2 grep gate green; Windows CI validate run green, or recorded as pending `/publish` (T19)
- [ ] VERSION 1.0.0, single-heading CHANGELOG, PLUGINS.md / REQUIREMENTS / FORK-MANIFEST updated, no tag
- [ ] HANDS-ON-SESSION-4.md ready for the one combined human session at verify (D51)
