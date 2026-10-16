# O-simpleReverb - Roadmap

**Current version:** 2.0.0 (installed) · **In flight:** 3.0.0 family retrofit (`improvements/family-retrofit/`)

## History

| Version | Date | What |
|---|---|---|
| 1.0.0 | 2026-01-13 | Initial release (Ouaricon Simple Reverb): `juce::dsp::Reverb`, 6 types, WebView UI |
| 1.5.0 | 2026-01-24 | Renamed O-SimpleReverb |
| 1.5.6 – 1.10.0 | 2026-07 – 09 | Code-review fixes, UI pass (palette, AA, EB Garamond) |
| 1.11.0 – 1.14.0 | 2026-09-27 – 30 | Factory TYPE recall, click-free tone controls, real flutter / shimmer, 48-preset bank, render-check |
| 2.0.0 | 2026-10-01 | Engine rewrite: 16-line FDN, Dattorro plate, dispersive springs; DECAY in seconds per type (`improvements/reverb-engine-rewrite/`) |
| **3.0.0** | 2026-10-15 → | **O-simple family retrofit**: rename to O-simpleReverb, family UI with four live wells, lessons, 0.6–1.5× resize; DSP bit-exact vs 2.0.0 |

## Milestone: family-retrofit (v3.0.0)

| Phase | Status | Output |
|---|---|---|
| Discuss | ✓ 2026-10-15 | `CONTEXT.md` |
| Research + mockup sign-off | ✓ 2026-10-15 | `RESEARCH.md`, `mockup/index.html` |
| Plan | ✓ 2026-10-15 (approved) | `PLAN.md` (T0–T10) |
| Execute | ✓ T0–T10 | `SUMMARY.md` |
| Verify | next | `VERIFICATION.md` |
| Post | after verify | version bump 2.0.0 → 3.0.0, CHANGELOG final, registry `used_by` version, no tag (tags only via `/publish`) |

Task waves (PLAN.md): T0 backup → T1 rename → T2 null harness → T3 tone statics → T4 meter payload → T5 preset core → T6 editor → T7 page → T8 tests → T9 install / validate → T10 docs.

## Requirements → Task Traceability

| Requirement | Tasks |
|---|---|
| COMPAT-01..04 | T1, T8, T9 |
| DSP-01..02 | T2–T5, T9 |
| UI-01..06 | T5–T8 |

## Implementation Notes

- **DSP frozen.** Only banner comments change in `Source/dsp/*.h` and `ModulationFx.h`; the tone-map statics (T3) are a pure extraction proven by the null.
- **One source per well.** The ER taps and the tone curve come from C++ natives that call the same code as the audio path; no JS copies of DSP tables.
- **Lessons never touch disk**; the factory bank stays 8 per type.
- **Shared checkout:** path-scoped commits only; PLUGINS.md row-only edits.

## References

- `.planning/improvements/family-retrofit/` — CONTEXT, RESEARCH, PLAN, SUMMARY
- `plugins/O-simpleAmbisonics/` — the family shape this retrofit ports
- `tests/render-check/` (137 gates, 32 mutants), `tests/pedalboard/verify_3_null.py`, `verify_3_state.py`, `tests/ui-probes/`
