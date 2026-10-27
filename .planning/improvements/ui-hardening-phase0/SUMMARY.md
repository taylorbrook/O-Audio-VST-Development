# Summary: Cross-plugin UI hardening, Phase 0

**Scope:** cross-plugin (18 plugins + CI + generator templates)
**Milestone:** ui-hardening-phase0
**Created:** 2026-10-26
**Phase:** Complete (closed 2026-10-27; DAW hands-on passed)
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
| O-Polystutter | T3.2, T3.4 | 1.16.2 → 1.17.0 † | ✓ | PASS | PASS | `ce7f870e`, `25b50e0b` |

\* first four ran with `--skip-gui-tests`; the other 14 ran with the editor tests on.

† UI fixes shipped as part of 1.17.0, not as a standalone 1.16.3: `ce7f870e` (labelled 1.16.3) also swept in
another session's 1.17.0 DSP rewrite, completed and reconciled by `25b50e0b`. See issue 5.

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

## B4 — formerly blocked trees (2026-10-27)

All five trees were clean at start (`git status --short plugins/<Name>` empty). Backups are in `backups/<P>/v<old>/`, except O-Freeze: its `v2.7.0` snapshot belongs to the other session's 2.8.0 work, and my pre-edit state is HEAD `9a28095d`. Commits are temp-index + CAS, stage explicit files only, and patch only the plugin's own PLUGINS.md row onto HEAD's blob. **No tags.**

| Plugin | Fixes | Version | Build | auval | pluginval 10 | Commit |
|---|---|---|---|---|---|---|
| O-Reed | 20 tuning-panel natives registered + T3.4 (G2) | 1.7.0 → 1.7.1 | ✓ 0 editor warnings | PASS | PASS (0x520d875) | `6d483a7e` |
| O-Bassoon | `generateRank2` / `saveScalaFile` / `saveKBMFile` registered | 1.6.0 → 1.6.1 | ✓ | PASS | PASS (0x7feaa6d) | `f44a6fa4` |
| O-simpleFM | `withBackend(webview2)` (Windows block) | 1.6.0 → 1.6.1 | ✓ | PASS | PASS (0x59f133e) | `3a581c41` |
| O-Gain | `%f` meter payload → `juce::String (v, 6)` | 1.10.0 → 1.10.1 | ✓ | PASS | PASS (0x21f624) | `6e8055fb` |
| O-Freeze | T3.4 | 2.7.0 → 2.7.1 | ✓ out of tree* | PASS | PASS (0x1ada380) | `85085fda` |

All runs used GUI tests on, against the installed bundle.

**O-Reed.** The 20 natives use O-Wind's shapes, with JSON built through `juce::JSON` and file dialogs guarded by `fileDialogOpen` + SafePointer. `getMasterTune`/`setMasterTune` go through the `referencePitch` parameter: Reed's engine is fed from the parameter in `processBlock`, so the engine value is stale until the next block. The T3.4 port is identical in shape to B3; colours come from `--bg-paper` / `--brown-text` / `--brown-frame` / `--green-dark`. **Known limitation (not fixed):** Reed does not persist tuning-engine state (custom scale / tonic / stretch) in `getStateInformation`. O-Wind does. Adding it is a state-format change, out of Phase-0 scope, and recorded in the CHANGELOG. Candidate for `/improve O-Reed`.

**O-Bassoon.** The 3 natives match the 17 existing siblings' idiom (string-built JSON, `[this, complete]`). Its temp folder is a named `%TEMP%` subfolder, so it is in the 29-plugin later-phase set and was not touched here.

**O-simpleFM.** The fix is only `withBackend`. Its named `%TEMP%` subfolder is in the later-phase set.

**O-Gain.** Keys and 6-decimal precision are unchanged. Non-finite values map to the page's floors: amplitude 0 (→ `ampToDb` −100), LUFS/dBFS −100 (`lufsOrFloor`, `> -99` guards), elapsed 0.

**O-Freeze — concurrent session.** Session `vst-development-eb` was mid-`/improve O-Freeze` 2.8.0, with uncommitted `PluginProcessor.*`, `CounterRng.h` and `tests/blocksize-null`. The two sessions agreed an order over cross-session messages: my 2.7.1 goes first and stages explicit files only (`PluginEditor.{cpp,h}`, CHANGELOG, CMakeLists = HEAD + the VERSION line), then they ship 2.8.0 on top. \* I verified it **out of tree**: `git archive HEAD` + my 4 files, so their processor rewrite was never in the tested binary. The installed `O-Freeze-dev` will be their 2.8.0 build (both changes) once they install. This avoids the G1 failure mode (a plugin-path pathspec sweeping another session's WIP).

**Gates.** Native-function census (served JS `getNativeFunction` names vs `withNativeFunction` names, tuning-panel.js included where embedded): 0 unregistered in all five. Before this batch it was 20 for Reed and 3 for Bassoon. `check-i18n` 0 (46 plugins), `i18n-fr-lint` 0, `i18n-zh-lint` 0. No page/JS files changed in B4.

**Windows unverified** (same as B3): the `#if JUCE_WINDOWS` code for Reed/Freeze T3.4 and the simpleFM backend was reviewed by reading only.

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
3. ~~B4 still blocked~~ **B4 complete 2026-10-27** (see the B4 section below).

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

## DAW hands-on for Taylor — PASSED 2026-10-27 (Windows items still pending a host)

- IntonationPad: preset browser ✎ rename in place (Enter / click away / Esc) and ✕ two-click delete (Logic + Live, macOS)
- Polystutter: preset menu × two-click delete
- Strata + Prism open together: hover-help toggles are independent (Strata's resets to On once)
- Comp / Bass meters, Orbit motion display, Bowed visualization still animate — ideally once with macOS Region set to France
- O-Orbit: the still-pending v1.4.2 halo check (now v1.4.5)
- Windows, when available: one T3.4 plugin opens normally; with the WebView2 runtime removed it shows the notice + link (also O-Reed / O-Freeze); O-simpleFM opens its page, not "page not found"
- **B4** O-Reed tuning tab: library list loads, pick a tuning, edit an interval, tonic, octave stretch, master-tune knob moves `referencePitch`, EDO/harmonic/Rank-2 generate, Load/Save .scl/.kbm, Export HTML (note: tuning edits are not saved with the session — known limitation)
- **B4** O-Bassoon tuning panel: Rank-2 generator, Save .SCL, Save .KBM
- **B4** O-Gain meters + Learn panel still animate (ideally once with the macOS Region set to France)
