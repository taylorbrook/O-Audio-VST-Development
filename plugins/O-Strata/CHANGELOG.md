# O-Strata Changelog

## v1.0.0 (unreleased)

**Foundation.** O-Strata is forked from O-Prism v1.24.0 (commit `e88ec412`,
2026-09-07) as a new, independent plugin: `PLUGIN_CODE OuSt`, product name
`O-Strata`, APVTS state identifier `OStrataParameters`, its own preset folder.
O-Prism is untouched and the two plugins coexist in a host.

Stage 1 (this entry):
- Removed the wavetable library: `WavetableFactory`, `UserWavetableManager`,
  `WavetableImporter`, `WavetableEditor`, the `oscATable`/`oscBTable`
  parameters, the user-wavetable state child, 14 wavetable native functions,
  the wavetable tab/editor/selector UI and their i18n rows. Both oscillators
  read a published table pointer (`oscTablePtr[]`), initialised to the sine
  placeholder; the retire/reaper machinery stays for Phase 2.1's bake scheduler.
- Added the 48 geometry parameters of `parameter-spec.md` v1 (24 per
  oscillator: Source/Frames/Shape Drive, Mesh ×6, Volume ×5, Terrain ×10).
  Inert in Stage 1 — automatable, persisted, not mod-matrix destinations.
  219 parameters total.
- `delayDivision` gains a slider relay (it was bound in O-Prism's page but
  never followed host automation).
- Linked `juce_cryptography` (SHA-256 for Stage 4.1 path-linked imports).
- Factory presets carried over with their table entries stripped; every preset
  now plays the sine placeholder and is re-authored in Stage 4.1.

Stage 1, second pass (re-parameterise, 2026-09-10):
- The 48 baked-geometry parameters are replaced by the 34 live wave-terrain
  parameters of `parameter-spec.md` v2 (17 per oscillator: Terrain (7 surfaces),
  Terrain Freq (exact-log 0.25–8×), Terrain Mod X/Y, Pitch Track, Saturation,
  Terrain Blur, Terrain Edge, Orbit (11 shapes), Orbit Aspect, Orbit Rotation,
  Orbit Centre X/Y, Orbit Mod, Feedback, Feedback Damp, Quality (Bandlimited /
  2× / 4×, default 2×)) — **205 parameters**. Inert until Phase 2.1.
- `osc?Pos` is now **Orbit Size** (default 0.5, was 0.0); `osc?Unison` is 1–4
  (was 1–8). **This is the last free range change**: no O-Strata preset has
  shipped; from v1.0.0 every range or list change needs its own migration gate.
- Mod-matrix destinations 26 → **46**: indices 1/2 relabelled `OscA/OscB Orbit
  Size`; 20 per-oscillator destinations appended (Orbit Aspect, Orbit Rot,
  Orbit CX, Orbit CY, Orbit Mod, Terrain Freq, Terrain Mod X/Y, Feedback,
  Saturation — all A then all B). Accumulated by the matrix, not read by the
  voice until Phase 2.1.
- State child `geometryImports` → `terrainImports` (still written empty).
- Factory bank reset: the inherited O-Prism presets are removed; Stage 1 ships
  one `Init` preset at the 205-parameter defaults. The real bank is Phase 4.1.
- 8 `WebComboBoxRelay`s added for the four Choice parameters per oscillator
  (relays only — the page is unchanged until Stage 3).
