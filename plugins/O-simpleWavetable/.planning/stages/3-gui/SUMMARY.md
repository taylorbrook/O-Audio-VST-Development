# Stage 3 (GUI) — SUMMARY

**Plugin:** O-simpleWavetable · **Stage:** 3 of 4 (GUI) · **Phases:** 3.1 Layout & Bindings · 3.2 Visualization Panels · 3.3 Import UX, Tooltips, i18n · **Executed:** 2026-10-06 (Part 1 `f53925c4`, Part 2 this commit)
**Plan:** `stages/3-gui/PLAN.md` (20 tasks, 2 parts). VERSION stays `0.1.0` (D-O).

---

## Outcome

The Stage 1 generic editor is replaced by the finalized single-page WebView "Wavetable Field Guide" (mockup v1, byte-identical):
- 21 params bound two-way (13 slider / 6 combo / 2 toggle relays), all 8 natives live.
- Bank stack + highlight + marker, quantized current cycle, harmonics 1–32 with band-limit ceiling, pushed at 30 Hz from the message thread with 0 audio-thread cost and 0 idle traffic.
- Five lesson buttons apply their recipes live, never touching `output_level`.
- Import from the button and by drop, with filename, frame count, empty-Imported state and localized errors.
- Tooltips + en / fr / zh-Hans; all five UI gates green on the real tree.
- Stage 2 DSP output unchanged (R-GOLD clean at Tasks 5, 10, 16).

## Files

**Created:** `Source/CycleView.{h,cpp}`, `Source/VizPayload.h`, `Source/ui/public/index.html`, `Source/ui/public/js/i18n.js`, `Source/ui/public/js/juce/{index.js, check_native_interop.js}`, `Source/ui/public/img/insects.png`, `tests/i18n-states.json`, `tests/viz-check/main.cpp`, `.planning/stages/3-gui/SUMMARY.md`.
**Modified:** `CMakeLists.txt` (UIResources binary data, `viz-check` target, new sources), `Source/PluginEditor.{h,cpp}` (rewrite from template + viz wiring), `Source/PluginProcessor.{h,cpp}` (`applyFactoryPreset`, `importFromBase64`, display state, `vizRenderer`, `buildCycleView`, `getBankThumbnails`, `getSelectedBankIndex`), `Source/WtVoice.h` (+5 / −0: `lastNote`, `getLastNote()`), `.planning/research/ARCHITECTURE.md` (Stage 3 Amendments 14–18), `.planning/STATUS.md`, `PLUGINS.md` (this row).
Part 2 touched no `Source/` file: Task 14 extended `tests/viz-check/main.cpp` only; Task 15 needed no fixes (no gui-agent dispatch D).

## Decisions as applied

| ID | Applied |
|----|---------|
| D-P | Quantized-payload FNV-1a hash; `lfo` gated on depth; var built only on emit; non-finite → 0 |
| D-Q | 8/8 natives registered in 3.1 |
| D-R | `frame = −1` with Interp On; silent = knob pos, level 0, note −1, f0 = nyquistH = 0 |
| D-S | `bankUpdate` filename from `cachedBlob.filename` under `bankStateLock` |
| D-T | Gen **or** bank-index watch + content dedupe; order `bankUpdate` → `importStatus` → `cycleUpdate` |
| D-U | **Kept** at bins 1..1023 (Taylor did not flip; `kRefMaxBin = 1023`) |
| D-V | Alias Demo = Sine→Saw, Pos 1.0, Band-limit Off |
| D-W | `CycleView` / `BankThumbs` / `CycleRenderer` in `Source/CycleView.*`; `VizPayload.h` header-only; `vizRenderer` before `importPool` |
| D-X | `dispNote` / `dispHz` / `displayFs` + `WtVoice::lastNote` |
| D-Y | Single pass over `ParamIDs::all`, `output_level` skipped |
| D-Z | `importFromBase64` in the processor; cap before decode; standard Base64 only |

## Measured values

**viz-check (Debug, out-of-repo, Task 16 run):** ALL PASS
- **G-LESSON:** 5 ids + reapply (0 changed) + unknown `"nope"` / `""` (no change); `output_level` −17.3 dB untouched in every case; Imported unchanged. NC (naive reset loop) FAILS as designed (output → −6.00 dB).
- **G-VIZ-EXACT:** 90/90 cases; worst cycle |err| 3.82e-05 (tol 2e-4); worst bar 0.0050 dB vs direct DFT (tol 0.02). N4-LIVE 90/90.
- **G-VIZ-CEIL:** On level 7 / kmax 8 / bars above −60 dB / nyquistH 11.4668; Off level 0 / kmax 1023, worst 0.005 dB.
- **Negative controls:** N1 ×347.8, N2 ×59.8, N3 ×1886.1, N4 ×2490.9, SILENT-NC ×2666.7 — all FAIL as designed.
- **G-VIZ-SILENT:** idle-quiet (cycleHash equal 100 blocks apart). **NOTE** poly / mono legato correct. **IMPORTED** import / P8 / empty pass.
- **ALLOC:** (a) 0 audio-thread allocations under concurrent viz; (b) 0 allocations over 1000 idle ticks (bank 0 and a 4-frame Imported).
- **TIME (Debug, log only):** `cycleUpdate` 817.9 µs/tick, 5161 B; `bankUpdate` N = 256 39.0 ms, 238 155 B.
- **G-DROP (Task 14):** (a) 12 392 WAV bytes → 16 524 standard base64 chars → bank 5, "drop me.wav", 3 frames, 11/11 mip levels memcmp-equal to `importFromMemory`; (b) cap + 1 (134 217 733 chars) → false / `tooLarge`, snapshot unchanged; (c) `"@@@@"` → false / `unreadable`; (d) `"../../x\n.wav"` → `"x.wav"` on status, thumbnails and wire. **G-DROP-N1** `MemoryBlock::toBase64Encoding` payload FAILS as designed (`unreadable`, snapshot unchanged).
- **G-IMPORT-ERR (Task 14):** 2047 samples → `tooShort` / "short.wav"; bank 5 and "keep.wav" / 2 frames unchanged; `importToVar` keys exactly {error, filename, frames, state}; emitted codes ⊆ {tooLarge, tooShort, unreadable, unsupported} (11 literal sites + `errorCode`).

**R-GOLD (Stage 2 goldens):** bank / dsp / dsp-alloc / mod / import / state PASS-FAIL lines identical to the baseline except timing / soak counts (`G-TIME` buildMillis 26.40 → 26.05 ms; `G-REAP-SOAK` blocks 2764 → 2729 and its `G-FINITE` sample count). G-PITCH 0.0009 c, G-Q2-C8 −114.3 dB, G-Q2-SWEEP −74.9 dB, G-Q2-PULSE −67.0 dB, G-VEL −11.905 dB, G-STEAL 1.076×, G-SWITCH-TAIL 0.656×, G-RETRIG-VEL 0.589×, G-CLICK 1.414 / 1.144, `--alloc-check` 0. Debug build: 0 plugin warnings; 0 JUCE assertions in any log.

**UI gates (Task 15, real tree):**
- `check-i18n`: exit 0, 0 FAIL ("ALL CHECKS PASS").
- `i18n-fr-lint`: CLEAN, exit 0.
- `i18n-zh-lint`: O-simpleWavetable 142 entries, 0 findings, exit 0.
- `check-ui-labels`: 8 states incl. [7] geometry pins, "ALL CHECKS PASSED", every resource served.
- `boot-all-uis --strict-tips`: O-simpleWavetable 1120×780 clean 1/1; 0 DEAD, 0 late, 0 warn.
- Static: `100vh/vw` 0; `user-select: none` 2; MIME types correct (html, javascript, css, png, woff2).

**Host (Task 16, Release):** build-and-install OK; Release + Debug Standalone built; `strings | grep cycleUpdate` 4 / 4 (VST3 / Standalone); Standalone mtime ≥ the built VST3 artefact; `nm -gU | grep ForTesting` 0. auval `AU VALIDATION SUCCEEDED` (2 known benign skew warnings). pluginval strictness 10 VST3 + AU SUCCESS, 0 FAILED, editor tests on.

## Human checkpoints

- **Task 12 (visual, Release Standalone):** signed off by Taylor 2026-10-06, no notes.
- **Task 17 (Standalone hands-on):** all pass (Taylor, 2026-10-06).
- **Task 18 (Logic AU pass):** all pass (Taylor, 2026-10-06) — closes the Stage 2 deferred item "save/reopen with Imported" and the Stage 1 Task 14 "AU under Instruments" leftover.

## Deviations

- **Part 1** (see the Part 1 commit body): S9 grep excludes the generated `modules/` dir; N4 split into N4-LIVE + dead-payload NC; Formant liveness anchored at its strongest bar (D-U consequence).
- **Task 16 freshness:** the installed VST3's mtime is the copy time, newer than the Standalone; freshness was checked against the built VST3 artefact instead (Standalone ≥ artefact). No `Source/` or `CMakeLists.txt` change since Part 1, so both binaries are from the same source.
- **G-IMPORT-ERR[codes]** enumerates emitted codes by scanning `PluginProcessor.cpp` at run time (via `__FILE__`); it fails loudly if the path does not resolve.
- No Task 15 findings, so no gui-agent dispatch D.

## Deferred (unchanged from PLAN §Out of scope)

FUNC-08 preset bank / §A9 recipes, PERF-02, COMPAT-02 Windows, QUAL-04, VERSION 1.0.0 / CHANGELOG / CODE_REVIEW, Stage 2 W3 / W4 and notes 3–8 → Stage 4. REQUIREMENTS flips for UI-01..06 / PERF-03 belong to `/plugin-verify`.
