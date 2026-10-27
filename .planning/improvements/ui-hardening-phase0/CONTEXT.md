# CONTEXT — Cross-plugin UI hardening, Phase 0 ("stop the bleeding")

Milestone: `ui-hardening-phase0` (cross-plugin; state lives at repo root, not in one plugin)
Created: 2026-10-26
Origin: `/research` Level 3 (4 parallel investigations) → `/improve` → milestone. Discuss phase
satisfied in-conversation; scope below was approved by Taylor.

## Goal
Remove the live UI reliability defects and the broken CI gate found by the research, with
no visual change and no shared-kit work (that is Phase 3, pending a decision record).

## Approved scope (all PATCH bumps, path-scoped commits, NO tags)

### B1 — CI
- UI Static Gates CI has been red on every push since 2026-09-28: CI never runs CMake
  configure, so the gitignored configure-time copies `plugins/*/Source/ui/public/modules/*.js`
  (preset-manager ×14, webview-drop-streaming ×3) are missing → check-i18n [12] fails on 11 plugins.
- Fix: a `scripts/sync-ui-modules` step that performs the same copy, run in CI before check-i18n.
  No plugin rebuilds. Direct embed is DEFERRED to the Phase 3 kit (decision taken here).
- Fix the masked O-DigiDelay fr-lint finding `js.savePresetPrompt`.
- Acceptance: check-i18n + fr-lint green on a fresh `git archive` of HEAD.

### B2 — Generator templates (stop re-emitting closed bugs)
- Editor template: `withBackend(webview2)`, `NEEDS_WEBVIEW2`, WebView2 static link,
  `withKeepPageLoadedWhenBrowserIsHidden()` mandatory (not "OPTIONAL"), app-data user folder.
- `index-template.html:340` calls nonexistent `Juce.getToggleButtonState`.
- `gui-agent.md` omits `withBackend`.

### B3 — Defects in free trees (one bump per plugin, bundling every fix it needs)
- O-Strata: localStorage key collides with O-Prism (`index.html:2739`) — both served from
  `juce://juce.backend/`, so they share hover-help state on macOS.
- O-IntonationPad (`app.js:1302,1320`), O-Polystutter (`app.js:553`): `prompt`/`confirm`/`alert`
  are no-ops in JUCE's WKWebView → in-page dialog.
- Locale-dependent `String::formatted("%f")` building JSON → `juce::var`/`JSON::toString`:
  O-Bass, O-Comp, O-Bowed, O-Orbit. **Flag:** O-Orbit has a v1.4.2 DAW check pending.
- WebView2 user-data folder out of bare `%TEMP%` → the convention shipped in
  O-simpleSubtractive v1.7.4 (`15968d17`): `userApplicationDataDirectory/…/WebView2`, created
  before options, plus `areOptionsSupported()` probe + native "needs WebView2" notice.
  Plugins: O-AnalogEQ, O-AnalogSaturation, O-Bass, O-Bells, O-Chorus, O-Comp, O-Detune,
  O-IntonationPad, O-Lyrica, O-Marimba, O-MultiBandCompressor, O-Polystutter,
  O-SpectralShaper, O-Tremolo.

### B4 — BLOCKED (another session has uncommitted work in these trees)
- O-Reed (20 unregistered native-function calls, open since 2026-08-31)
- O-Bassoon (3: `generateRank2`, `saveScalaFile`, `saveKBMFile`)
- O-simpleFM (missing `withBackend`), O-Gain (`%f`), O-Freeze (`%TEMP%`)
- Also never touch O-GrainScatter (live uncommitted DSP work as of 2026-10-26).
- Unblock condition: `git status --short plugins/<Name>` empty.

## Constraints
- Trunk-based, shared checkout: path-scope every commit; re-check branch + staging right before
  each commit (see project CLAUDE.md).
- Backup each plugin to `backups/<Plugin>/v<current>/` before editing.
- No visual change in Phase 0. No DSP / parameter / state-format change.

## Testing
Build + pluginval/auval per touched plugin; `check-i18n` + `boot-all-uis` suite-wide.
DAW hands-on deferred to Taylor (list it per plugin).

## Out of scope (later phases of the roadmap)
Static gates (P1), shared C++ editor shell + JUCE overrides (P2), UI kit (P3, needs decision
record reversing `260826-ieq-CONTEXT.md:51-53`), visual refresh (P4).
Open decisions: macOS floor (10.15 recommended), house paper (cream recommended), spacebar spike.
