# Stage 3 — GUI · Phase 3.1 (Layout + controls + bindings) — Summary

**Plugin:** O-Granulator · **Stage / phase:** 3-gui / 3.1 · **GSD phase:** execute · **Date:** 2026-11-06
**Commits:** 1b115397 (T4) · 5b341dd4 (T5–T6) · 4c022d8c (T7) · afd6a3df (T8–T10) · 96ec55ea (T11 run, which changed no files, + T12) · **Logs:** ${TMPDIR}/ogran-31/

## Result
The v2 WebView editor ships. GranulatorEditor binds 140/59/21 relays → webView → attachments, registers 21 native functions and serves the page through granulatorUiResource. The processor gained exactly three things: OuariconPresetManager (files only), Telemetry + readTelemetry, and getWindowCurve over a new const Engine::windowCurve. Audio did not move. UI-01 is partial; the hands-on runs at 3.1 verify.

## Gate table
| Gate | Result |
|---|---|
| Entry check | PASS (6 hashes, diff since 9831f498 empty, cmp clean) |
| Harness default (final) | 474 PASS / 0 FAIL, rc 0, 7 min 30 s (463 carried + PM1, PM2, PM2n, PM3, PM3n, PM4, PM5, PM5n, WC1, WC2, TL31) |
| Carried diff vs B | empty (T4 and final) |
| PM2n / PM3n / PM5n | fail their predicates: output gain 0.000 dB / seed 0 / seed 0x77 |
| N3/N2/N1/N0 --strict, R12 | rc 0 ×5, max\|Δ\| 0, controls differ |
| HA | 79/79, 142 untouched, 79/79 (T0, T4, final) |
| Session-codec bytes (codec_bytes.py) | identical B vs T4, 9 fixtures incl. second reopens |
| param-dump | diff-clean |
| Build | VST3 + AU + Standalone, 0 warnings |
| auval / pluginval 10 VST3 + AU | PASS (Tail Time) / SUCCESS ×2, editor tests included |
| Standalone smoke | page drawn (ref-3.1/standalone-t6.jpg) |
| generic-overrides.json | byte-identical across two generations; diff-clean |
| ui_frontend_check.js | 0 failures; NC1 exit 3 ([8] fails), NC2 exit 2 ([3] fails) |
| ui_binding_probe.js | 220/220 host→UI, 220/220 UI→host; NC exit 2, 219/219 |
| check-i18n · fr · zh | 0 · 0 · 0 |
| check-ui-labels | 0; seed=param-dump (220 rows) + overrides; states default + 9 |
| boot-all-uis --strict-tips | clean 1/1, 0 DEAD, 0 late |

T0 UI pre-state: check-i18n / fr / zh 0. check-ui-labels rc 1 and boot FAIL: the Stage 1 placeholder editor has no setSize.

## New harness rows
PM1: 219 params with 0 diffs; scale, 3 masks and seed equal; revision 1 → 6; output gain untouched. PM2: −12 dB survives the load; no output_gain key in the file. PM3: no customState → 12-EDO, masks all-on, seed kept. PM4: preset → reopen → change → reopen equals the changed state; one child of each kind; 0 live custom children. PM5: 0xFEDCBA9876543211 exact, stored as the string "-81985529216486895". WC1: worst 5.86e-7 vs closed-form Hann. WC2: skew ±100, window 1, window 0.5 / skew 37 all live and endpoint-0; the processor reads the base params. TL31: 18/18 (sources, pitch, 2 routed modNorm, 3.2/3.3 fields zero/NaN, liveness).

## Deviations
1. Row TL1 → TL31 (a 2.5a TL1 already exists).
2. createEditor switched at T5: the consoles depend on the plugin target.
3. The --dump-ui-overrides code went in with T4. T7 fixes a freed temporary in it (output was byte-identical either way).
4. Extra tests/hostcache/codec_bytes.py for the P98 byte comparison through the installed VST3.
5. Carried extract made explicit: C-timing detail dropped, HR3 "prepare … ms" masked.
6. Telemetry has* flags and the shared telemetryToVar in ScaleStateJson.h. "frozen" is no longer emitted (the page never reads it; ufc [12] caught it).
7. i18n-states has 9 states (P108 = §6's 8 + the matrix state; the gate row said + 8). mod-free and streams-tempo use eval clicks to avoid a pointer-parked tooltip, which gave a hover-only GEOMETRY DIFF.
8. PM3n uses a callback swap; PM5n writes the seed as a JSON number; preset seeds are accepted only as strings.
9. Overrides sliders include label (the override entry replaces the TSV entry).
10. check-ui-labels coverage: 341 of 2344 measured. 2000 of the unmeasured are closed-select matrix options; 4 others include #fxcount-s2…s4 (shown only for enabled streams).
No page change (byte-identical v2); no module, scripts or contract edit except REQUIREMENTS UI-01 → partial.

## Timings
Harness 7:17 (B) / 7:27 (T4) / 7:30 (final); C7 17.12 % → 17.20 %; N-suites + R12 ≈ 52 s; builds ≈ 20 s; pluginval ≈ 1–2 min per format; UI gate sequence ≈ 4 min.

## Carried
HANDS-ON-3.1.md at 3.1 verify. 3.2: wave ring + grain seqlock. 3.3: counts, meters, onset, loop gain, LFO phase, and the DAW pass (D27). Env 1e30 pin → Stage 4; C7 calibration and Z6a harness items; W2o rule. A states entry with s2–s4 enabled once counts are live.
