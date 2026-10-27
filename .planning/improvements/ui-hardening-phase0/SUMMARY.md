# Summary: Cross-plugin UI hardening, Phase 0

**Scope:** cross-plugin (18 plugins + CI + generator templates)
**Milestone:** ui-hardening-phase0
**Created:** 2026-10-26
**Phase:** Execute
**Base HEAD:** `ea44af28` · **Domain:** mixed · **Agent:** general-purpose (inline)

All PATCH bumps, path-scoped temp-index commits (only the plugin's own PLUGINS.md row,
patched onto HEAD's blob; the other session's uncommitted PLUGINS.md rows were never staged).
**No tags.** Backups: `backups/<Plugin>/v<old>/` for all 18 (gitignored).

---

## B1 — CI gate

| Task | Result | Commit |
|---|---|---|
| T1.1 `scripts/sync-ui-modules.js` — mirrors `ouaricon_add_module()` JS copy (category order, COPYONLY, `--check`) | 14 copies written / 4 already current on a fresh archive | `670b2ebf` |
| T1.2 `ui-static-gates.yml` sync step before check-i18n | — | `670b2ebf` |
| T1.3 O-DigiDelay `js.savePresetPrompt` NBSP (fr T4) → 1.9.3 | — | `da14304c` |

Baseline reproduced on fresh `git archive ea44af28`: check-i18n exit 11 (11 × [12]).
After, on a fresh archive of the final HEAD (`ce7f870e`) + sync: **check-i18n 0, fr-lint 0, zh-lint 0.**

## B2 — Generator templates (`2a44ad87`)

- Editor template, `gui-agent.md`, `ui-design-rules.md` Rule 14: `withBackend(webview2)`, app-data
  user folder `Ouaricon/WebView2/<Name>` created first, `areOptionsSupported()` probe + native
  notice + runtime link, null-safe `webView` uses, `withKeepPageLoadedWhenBrowserIsHidden` REQUIRED.
  `ui-finalization-agent.md` documents the new placeholders.
- CMake snippet: `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`.
- `Juce.getToggleButtonState` → `Juce.getToggleState` everywhere (0 hits left). Beyond plan: the
  index template's toggle block also used `getNormalisedValue()` and read a listener argument —
  ToggleState has `getValue()/setValue(bool)` and calls listeners with no argument; fixed.
- Acceptance grep `tempDirectory|getToggleButtonState` → 1 hit, prose in a checklist ("no `tempDirectory`").

## B3 — Per-plugin fixes

| Plugin | Fixes | Version | Build | auval | pluginval 10 | Commit |
|---|---|---|---|---|---|---|
| O-DigiDelay | T1.3 | 1.9.2 → 1.9.3 | ✓ | PASS | PASS* | `da14304c` |
| O-AnalogEQ | T3.4 | 1.5.6 → 1.5.7 | ✓ | PASS | PASS* | `dbd0b96b` |
| O-AnalogSaturation | T3.4 | 1.8.2 → 1.8.3 | ✓ | PASS | PASS* | `58c06244` |
| O-Bass | T3.3, T3.4 | 1.8.2 → 1.8.3 | ✓ | PASS | PASS* | `5106e797` |
| O-Bells | T3.4 | 4.9.2 → 4.9.3 | ✓ | PASS | PASS | `18710372` |
| O-Chorus | T3.4 | 1.10.2 → 1.10.3 | ✓ | PASS | PASS | `f338538f` |
| O-Comp | T3.3, T3.4 | 1.11.2 → 1.11.3 | ✓ | PASS | PASS | `998cc1bb` |
| O-Detune | T3.4 | 1.10.1 → 1.10.2 | ✓ | PASS | PASS | `0c7ac20e` |
| O-Lyrica | T3.4 | 2.6.2 → 2.6.3 | ✓ | PASS | **FAIL (pre-existing, see below)** | `38855299` |
| O-Marimba | T3.4 | 1.16.2 → 1.16.3 | ✓ | PASS | PASS | `55c1138c` |
| O-MultiBandCompressor | T3.4 | 1.14.1 → 1.14.2 | ✓ | PASS | PASS | `7ee21ae2` |
| O-SpectralShaper | T3.4 | 1.10.1 → 1.10.2 | ✓ | PASS | PASS | `4717e867` |
| O-Tremolo | T3.4 | 1.11.1 → 1.11.2 | ✓ | PASS | PASS | `bb540e53` |
| O-Bowed | T3.3 | 1.10.2 → 1.10.3 | ✓ | PASS | PASS | `e919de75` |
| O-Orbit | T3.3 | 1.4.4 → 1.4.5 | ✓ | PASS | PASS | `f13ad85f` |
| O-Strata | T3.1 | 1.1.2 → 1.1.3 | ✓ | PASS | PASS | `e3932795` |
| O-IntonationPad | T3.2, T3.4 | 2.11.2 → 2.11.3 | ✓ | PASS | PASS | `2225df83` |
| O-Polystutter | T3.2, T3.4 | 1.16.2 → 1.16.3 | ✓ | PASS | PASS | `ce7f870e` |

\* first four ran with `--skip-gui-tests`; the other 14 ran with the editor tests on.

**T3.1** Strata key `oprism.tipsEnabled` → `ostrata.tipsEnabled` (pref resets once; CHANGELOG notes it).

**T3.2** IntonationPad: rename = in-place `<input>` (reuses `.preset-save-input`; Enter/blur commit,
Esc cancel; presses inside don't load the preset); delete = armed two-click (2.5 s). Polystutter: armed
two-click delete. No new strings: `dialog.renamePrompt` → field aria-label (`data-i18n-aria`),
`dialog.deleteConfirm` / `msg-delete-preset` → `setLabel()` on the name cell while armed. Found on the
way: IntonationPad's prompts read keys `label.renamePrompt`/`label.deleteConfirm` that don't exist
(the strings live at `dialog.*`). The question text uses the normal ink: `--danger` measured 3.14:1
(Polystutter) / 2.60:1 (IntonationPad) as text, below AA, so only the × turns red (as on its existing
hover). Polystutter's vendored `preset-manager.js promptDelete()` left alone — unreferenced (no
`deleteButton` passed). Headless probe (`serve-ui` + stubbed natives): 21/21 PASS, incl. "no native
dialog opened" and no page errors.

**T3.3** `String::formatted("%f")` → `juce::String (v, decimals)` (JUCE writes with the classic
locale — verified in `juce_String.cpp`). Payload type/shape unchanged; non-finite → the page's own
floor: Comp LED/envelope −60 dB, GR 0; Bass −60 / LED 0; Orbit angles 0, dist 1 (its initial
`motionState`); Bowed 0.

**T3.4** 14 ports of `15968d17`. Identical notice code per plugin; colours from each page's CSS
tokens (commented by token name); layout scales by height (`s = clamp(H/260, 0.6, 1)`) so the
125-px O-Chorus and 220-px O-AnalogEQ fit. Every `webView->` use outside native-function callbacks
is null-guarded (timers, `goToURL`, `notifyPresetLoaded`, `resized`). Polystutter builds its WebView
in the member-init list, so the probe lives in an immediately-invoked lambda there.

## Suite-wide

- `boot-all-uis`: 46/46 boot before and after; per-plugin page errors, console errors, request
  failures, dead tip bindings, missing label keys and text-element counts **identical**.
- check-i18n / fr-lint / zh-lint: exit 0 in the working tree and on a fresh archive of HEAD.

## Open items

1. **O-Lyrica pluginval "Plugin state restoration" — RESOLVED in O-Lyrica v2.6.6 (`11359791`).**
   The cause given here first ("a random state with both on cannot round-trip") was wrong. The
   plugin's state did round-trip. The HOST's copy went stale: `parameterChanged()` wrote the
   partner toggle OFF from inside the VST3 wrapper's thread-local `inParameterChangedCallback`
   scope (`juce_audio_plugin_client_VST3.cpp:825-833`, `paramChanged` :1472). That scope drops every
   nested change report, so the host kept the partner's pre-write value (the 0.453 pluginval read
   back is the host's random write, not the plugin's value). Fix: the exclusion moved to the page's
   click handler and the toggles are independent (Scale-Locked keeps priority). pluginval 10 passes
   on 0x69c6be7, 0x14cb29d, 0x2ed10e1 and 0x1; auval passes.
2. **Windows unverified.** No Windows machine here; all `#if JUCE_WINDOWS` code was reviewed by
   reading and is identical in shape to the shipped O-simpleSubtractive v1.7.4.
3. B4 still blocked (O-Reed, O-Bassoon, O-simpleFM, O-Gain, O-Freeze; never O-GrainScatter).
   Unblock: `git status --short plugins/<Name>` empty.

4. **O-Lyrica verdict (verify):** PRE-EXISTING CONFIRMED (resolved since: item 1) — 2.6.2 (`38855299^`, out-of-tree) and 2.6.3 fail the
   same Glissando restore identically on the same 3 random seeds. pluginval 1.0.4 has no `--seed`; the pinning
   flag is `--random-seed`.
5. **O-Polystutter `ce7f870e` carried another session's v1.17.0 DSP work** (TriggerScheduler, CounterRng,
   RepeatLane/TapeDegrader, PluginProcessor, scheduler-harness, CMake VERSION 1.17.0) — the plugin-path
   pathspec swept in its uncommitted files. At that commit CMake says 1.17.0 while CHANGELOG/PLUGINS.md say
   1.16.3, and the 1.16.3 entry's "no DSP change" is false for that commit. `25b50e0b` completed and
   reconciled it at 1.17.0; history not rewritten. Lesson: a plugin-path pathspec is not safe when another
   session has dirty files in the same plugin — stage explicit files.
6. **Post-verify fixes (B2 follow-up):** `validate-gui-bindings.py` toggle regex now matches `getToggleState`
   (was `getToggleButtonState` only → template-built toggles skipped silently); troubleshoot-agent example
   gains `withBackend(webview2)`.

## DAW hands-on for Taylor

- IntonationPad: preset browser ✎ rename in place (Enter / click away / Esc) and ✕ two-click delete (Logic + Live, macOS)
- Polystutter: preset menu × two-click delete
- Strata + Prism open together: hover-help toggles are independent (Strata's resets to On once)
- Comp / Bass meters, Orbit motion display, Bowed visualization still animate — ideally once with macOS Region set to France
- O-Orbit: the still-pending v1.4.2 halo check (now v1.4.5)
- Windows, when available: one T3.4 plugin opens normally; with the WebView2 runtime removed it shows the notice + link
