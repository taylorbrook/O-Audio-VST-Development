# Stage 3 — GUI — SUMMARY (Part 1: 3.1 + 3.2)

**Plugin:** O-simpleAmbisonics · **Executor:** gui-agent · **Date:** 2026-10-13 · **Commits:** `5d9d351b` (3.1), `bc1645ef` (3.2) · **Part 2 (3.3, T10–T13) NOT started — visual checkpoint pending.**

## Tasks and files

| Task | Files | Result |
|---|---|---|
| T1 | `Source/MeterFramePayload.h` | JUCE-free `ui::` quantise (non-finite → 0, ±1e6 clamp, `!live` zeroes peaks), FNV-1a hash incl. `live`, `to_chars` bounded format, `LiveTracker` (`kStaleTicksIdle` 3), `sampleFrame` (hasNewData before read) |
| T2 | `Source/PluginProcessor.{h,cpp}` | Guarded include + `createEditor`; `uiLanguage` (code string, `isVoid()` restore); `editorScale` atomic, written as `"0.850"` when > 0 and removed from the copy otherwise, restored only when finite and in 0.6–1.5; `applyFactoryPreset` (8 lessons, one pass, never `outputLevel`, skips within 1e-6, begin/set/end). No changes to prepareToPlay / processBlock / fillParams / createParameterLayout |
| T3 | `Source/PluginEditor.{h,cpp}`, CMake, `ui/public/{index.html,i18n.js,juce/*.js,insects.png}`, `.gitkeep` removed | Template + C1 fixes 1–4 + C7. 12 relays → webView → 12 attachments, 5 natives, `setSize (1180, 780)` last. Assets copied verbatim (md5 01ff05e3 / 3c4db27f / 6d824230) |
| T4 | Harness `main.cpp` (+ST1–6, PL1–4), `tests/ui-stub/generic-overrides.json`, `tests/pedalboard/verify_3_state.py` | Real ranges, choices and defaults (skew 0.2890648); own JUCE-base64 decoder |
| T5 | `tests/ui-fixtures/frames.json` | 15 fixtures, byte-identical across two runs |
| T6 | `scripts/generate_sh_icons.mjs` → `js/shicons.js` (233 705 B), `sh-icons-contact.html`; CMake + `/js/shicons.js` resource branch | SN3D matches an independent recursion to 4.70e-16. Letters W Y Z X V T R S U Q O M K L N P. All anchors > 0. Two runs identical (sha in `SH_ICONS_META`). 110–199 paths per icon, stroke 0.6. `--check` exits 0 |
| T7 | `ui/public/index.html` | U1 `effectiveFrame`/`paramFrame` + `#chanWell.idle` (0.8). C13 pole hold + half-alpha tick. C12 heading rose, fade, ▲▼ chevron. C14 dead zone = rHead, `180.0°`, `orderIdx()`. C15 bands + station counts from `orderActive`. C16 dirty flags, ease snap 1e-3, puck sprites, fonts/lang marks all. C11 `SH_ICON_STYLE='3d'`. 0 `'mousedown'` listeners, 2 anchors, no innerHTML outside comments |
| T8 | `tests/ui-probes/frame-probe.mjs` | 13/13. The fuma / pole / idle negative controls fail as designed |
| T9 | Build / install | See Numbers |

## Numbers

- **Build:** 0 warnings (the template's deprecated display APIs were replaced).
- **Install:** `build-and-install.sh` exited 0; Standalone rebuilt.
- **auval:** SUCCEEDED.
- **pluginval s10** (now opens the WebView editor): SUCCESS 2× VST3 + 2× AU at T5, 1× each at T9.
- **Harness:** 251/251 (241 + 10), run twice, identical except PERF-02 (210× real time).
  - Digests: 2.1 `9189d911a1fe98d5` PASS, 2.2 `7f35bc9f91e321b9` PASS, 2.3 `edd1acfd21f54bcd` reproduces.
  - ST1: zh-Hans → 2.
  - ST2: absent / "de" → 0.
  - ST3: 0.8500; a fresh instance writes no editorScale.
  - ST4: abc / 3.0 / 0.1 / 1.6 / nan / inf rejected and absent from the re-save.
  - ST5: 0 mismatches over 2 × 12; distance raw 1.00000011920929.
  - ST6: 84 changes over 8 lessons, gestures balanced, idempotent, unknown ids inert, outputLevel raw −11.999999 unchanged.
  - PL1: 390 B typical, 485 B worst case.
  - PL2: left Y 1000 / X 0; yaw90 azRot 9000; pole elRot 9000 / 9000 with azRot 3000 / 15000; stereo mix 0.
  - PL3: one idle emission, then silence; a publish revives.
  - PL4: 15 fixtures / 9 002 B, identical.
- **Pedalboard:**
  - `verify_2_2` 35/35, `verify_2_3` 42/42, `verify_3_state` S1–S3 pass.
  - `verify_2_1` 22/25: the 3 Binaural V3 checks have been stale since Phase 2.3. They expect a clean impulse from the 2.1 placeholder, and the real HRTF convolver smears it. Pre-existing; the file was not edited. **Needs a user decision.**
- **Tracer gate:**
  - Harness 241/0 on the unchanged harness.
  - Natives census 5 = 5. Boot clean, 0 404s.
  - Standalone OK after the occlusion nudge.
  - C++ → page proven via a patched session state (az 180 / order 3 / yaw 60).
  - Page → C++ in WKWebView can't be driven headless and is **untested** (checklist item 9).
- **frame-probe:**
  - A1: W / X / Y 0.997 / 0.997 / 0.
  - A2: behind, X −1.000.
  - A3: bands 2/3/4, inactive rows at 0.3 opacity, 16 icons.
  - A4: puck at (84.79, 166.71).
  - A5: puck unchanged, rim tick to the left, ▲ glyph IoU 0.88 (control 0.16).
  - A6: 1/1 gesture, 0 rear samples, final 90.00.
  - A7: 1/1, final 45.00.
  - A8: 109.5° → 68.5° → 49.5°.
  - A9: tick held at (−140.3, −0.25).
  - A10: 16.33 % of decoder pixels change.
  - A11: rotation station lights; labels 4/9/16.
  - A12: param-drawn W = Y = 0.997; idle opacity 0.8 → 1.0.
  - A13: peaks fall to 0 in 309 ms; the puck follows params while idle and not while live; a silent frame is not dimmed.

## Deviations

- **D1:** The `silent_after_play` fixture adds a second silent tick, because the output tail is held as the peak since the last read.
- **D2:** ST6 asserts Output Level is bit-unchanged at −11.999999, not −12.0f (a host-path set always snaps).
- **D3:** editorScale is persisted, with remove-on-copy.
- **D4:** C7: the timer is the only frame sampler.
- **D5:** C9 corrected pole fixture pair.
- **D6:** C15: bands and station counts come from orderActive.
- **D7:** C17: UI-03 and UI-05 land in 3.2.
- **D8:** `fitMaxForDisplay` returns 1.0 when there is no display.
- **D9:** Deprecated display APIs replaced with their JUCE 8.0.15 equivalents.
- **D10:** The pitch chevron moved to R − 40; at R − 30 it overlapped ▲ and read as a "Y".
- **D11:** `inject-context.py` failed with KeyError `'stages'`. Worked from PLAN, CONTEXT and RESEARCH only.
- **Probe fix:** canvas pixels are blended over the well background before classifying.

## VISUAL CHECKPOINT PENDING: 3.3 not started

Check in the Standalone (`build/plugins/O-simpleAmbisonics/O-simpleAmbisonics_artefacts/Release/Standalone/O-simpleAmbisonics-dev.app`), in Logic, and on the icon sheet `sh-icons-contact.html`:

1. **Meters:** front → left, X hands over to Y while W holds at +1. Behind, X turns amber. Order 1 → 2 → 3 lights 4 / 9 / 16 rows. Peak overlays follow the input.
2. **Sphere:** dragging from above sets azimuth (+90 = left). Crossing behind shows `180.0°`. Clicking on the head does nothing. The side drag sets elevation.
3. **Rotation:** yaw turns the rose, beam and rim tick, never the puck. Pitch past ±90° flips and fades the rose, with the ▲/▼ chevron. At elevation 90 the rim tick holds at half brightness.
4. **Beam and decoder view:** the beam sharpens from order 1 to 3. The decoder view crossfades between the KU100 head and the cardioid pair.
5. **Icons:** pick 3D or slice (the 0.6× and 1× rows decide). If slice: set `SH_ICON_STYLE = 'slice'`, remove the `shicons.js` import, its binary-data entry and its resource branch, then re-run frame-probe and the T9 build.
6. **Resize** 0.6×–1.5×: the aspect holds, text stays sharp, drags track the pointer, and the size persists across reopen.
7. **Idle fallback:** set the output device to none. The well dims, peaks drop to 0, and the puck, bars and beam still follow drags. Restore the device and it is live again.
8. **Look:** spacing, colour and type against the mockup. Known nits: the "3rd order · (3+1)² = 16 chan…" readout is truncated, and the chevron can sit next to the puck when azimuth ≈ yaw.
9. **Hands-on (page → C++):** a knob drag moves its readout, and a lesson moves exactly its recipe controls with Output untouched.

**Open items:** icon pick · `verify_2_1` V3 decision · page → C++ hands-on · truncated order readout · chevron/puck proximity.
