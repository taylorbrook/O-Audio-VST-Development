# B — UI reliability bug classes across the Ouaricon suite

Research date: 2026-10-26. Read-only. Scope: the 46 JUCE 8.0.15 WebView plugins in `plugins/*/`, at the current working tree. Five plugins carry uncommitted in-flight work: O-Bassoon, O-Freeze, O-Gain, O-Reed and O-simpleFM.

**Sources**
- 298 project memory notes and their sub-indexes.
- `troubleshooting/`.
- 46 CHANGELOGs: 51,877 lines, 1,368 version headings, 2,143 "Fixed" bullets.
- `git log`, plus the CI run history of `ui-static-gates.yml`.
- The 2026-09-24 suite UI review: `.planning/quick/260924-nho-*/260924-nho-UI-DESIGN-REVIEW.md`.
- Fresh censuses (scripts in `scratchpad/scripts/`) and a fresh `boot-all-uis` run (46/46 boot clean).
- The JUCE 8.0.15 source at `/Users/taylorbrook/JUCE`, plus JUCE GitHub issues and commits, the JUCE forum and Microsoft docs.

**How to read the counts**
- **Keyword counts** from CHANGELOGs are approximate. Each class count below was hand-checked against the matching bullets.
- **Census numbers** are greps over the shipped UI roots, resolved by `index.html` as `serve-ui.js` does. Treat them as "candidates" where marked.

---

## 0. Headlines

1. **UI work now dominates maintenance.**
   - UI-touching "Fixed" bullets: about 30% in Jan–Mar 2026, 54–65% in Aug–Oct (912/2,143 overall; keyword heuristic).
   - Since 2026-08-01, 567 of 957 plugin commits (59%) touched UI files, and 300 were UI-only.
2. **Most UI defects are silent.** Nothing errors, and build, auval, pluginval and the render harness all pass. Of the 7 layers below, 5 failure modes are a promise that never settles, a page that never re-reads, a dropped event, a fallback backend, or a stuck gesture.
3. **Two framework defaults sit behind the worst Windows and host classes. Neither is pinned by a shared module.**
   - On Windows, `Backend::defaultBackend` is the legacy IE control, and JUCE falls back to it silently, which gives "page not found".
   - `unloadPageWhenHidden` is on by default, so the page navigates to `about:blank` on hide (FL Studio, nested visibility; JUCE #1577 and #1560 are open).
   - Each editor sets its own options:
     - 1/46 still lacks `withBackend` (O-simpleFM).
     - 26/46 lack `withKeepPageLoadedWhenBrowserIsHidden()`.
     - 44/46 have no `areOptionsSupported` fallback.
     - 45/46 put the WebView2 user data folder in `%TEMP%`.
4. **Hand-copied code is why every fix is replicated N times.** `check-ui-canon` today reports:
   - tooltip renderer: 29 distinct variants;
   - hover-help init: 29;
   - knob binding: 23;
   - knob visual: 19;
   - preset-manager.js: 6 forks;
   - tuning-panel.js: 4 forks.

   The click-away fix landed as about 39 separate commits on 2026-10-19. Hover-help took 22 commits, and i18n took about 60 batch commits.
5. **The generator re-introduces closed classes.**
   - `ui-mockup/assets/webview-templates/PluginEditor-webview.cpp` has no `withBackend` and no WebView2 user data folder, and labels keepPageLoaded "OPTIONAL".
   - Its CMake snippet has no `NEEDS_WEBVIEW2` and no static link.
   - `index-template.html:340` calls `Juce.getToggleButtonState`, which does not exist. That is the closed 2025 FlutterVerb class.
   - `gui-agent.md:622-643` also omits `withBackend`.
6. **The UI CI gate has been red for 18 consecutive pushes (since 2026-09-28), and nobody noticed.**
   - CI never runs CMake configure, so the gitignored `Source/ui/public/modules/*.js` copies are missing and check-i18n [12] fails on 11 plugins.
   - That false red masks a real fr-lint finding: O-DigiDelay `js.savePresetPrompt` T4, added in `aeebc2a2` on 2026-09-30. `node scripts/i18n-fr-lint.js` exits 2 locally today.
7. **Seven open bridge, API and keyboard defects found by fresh census:**
   - O-Reed: 20 unregistered native functions, so the tuning tab is dead. Known since 2026-08-31.
   - O-Bassoon: 3 unregistered functions (`generateRank2`, `saveScalaFile`, `saveKBMFile`), so Rank-2, Save .SCL and Save .KBM hang. **New.**
   - O-IntonationPad rename (`prompt`) and delete (`confirm`), and O-Polystutter delete (`confirm`), are dead on macOS. JUCE's WKWebView implements no JS dialog panels.
   - Locale-dependent `String::formatted("%f")` payloads in 5 plugins.
   - 42/46 editors leave a host gesture open when closed mid-drag.
   - Host key pass-through (spacebar) is handled in 0/46. Upstream JUCE has had no fix since 2024.
8. **Dirty-tree exclusion strands sweeps.**
   - 5 plugins have month-old uncommitted UI passes, dated 2026-09-25/27. They missed both the 2026-10-19 value-entry port and its click-away fix.
   - O-simpleFM also missed the 2026-10-14 `withBackend` sweep.
   - No ledger records the owed sweeps.
9. **The headless gate is blind to the biggest bridge class.**
   - `boot-all-uis` reports 46/46 clean today while O-Reed and O-Bassoon have 23 dead calls. The stub invents a value for any unknown native and the panels mount lazily.
   - A <1 s static name-closure check catches all of them; the script is in the scratchpad.
10. **Upgrading JUCE is not a lever for WebView reliability.**
    - JUCE 9.0.x (latest 9.0.4, 2026-10-12) adds only the TypeScript `@juce-framework/webview` package for WebView.
    - The 8.0.x fixes that matter here (FL Studio blank `744986733`, Ableton DPI `245060021`, multi-instance freeze `90e72dbd9`, Esc crash `7f4176e259`) are already in 8.0.15.
    - `8.0.16-security` (2026-10-13) is a drop-in String-hardening patch.

---

## 1. Taxonomy (classes × occurrences × root cause × state × structural fix)

**Fix type key**
- **SH**: a shared C++ editor shell, a direct-embed header module.
- **JM**: a shared JS runtime module, direct-embedded like `tuning-panel.js`, so there is zero drift by construction.
- **FP**: a framework patch through `vendored/JUCE-overrides/`, which `check-juce-overrides.sh` already fingerprints.
- **G**: a gate, static unless noted.
- **GEN**: the generator and template contract.
- **PROC**: process.

### L1 — Editor shell and WebView lifecycle (C++)

**C1 — Windows WebView2 not selected or not available, so the IE control loads and shows "page not found" or a blank page**
- **Occurrences:**
  - 2026-02-06 audit: 34/35 plugins lacked `NEEDS_WEBVIEW2` plus static linking.
  - 2026-10-14: 11 plugins lacked `withBackend`, and 10 were fixed (`b477dc83` covered 9, `8a83d78f` covered 1).
  - O-simpleSubtractive v1.7.4 still showed "page not found" in Reaper: the WebView2 environment failed and JUCE fell back to IE silently.
  - O-DigiDelay WR-05: a shared user data folder caused lock contention, and the IE fallback followed.
  - 17 plugins / 21 versions in CHANGELOGs.
- **Root cause:**
  - Windows `defaultBackend` is IE, and the fallback is silent (`juce_WebBrowserComponent_windows.cpp`).
  - Every editor builds its own `Options`.
  - Mac-only verification can't see it.
  - All three generator surfaces omit `withBackend`.
- **State: OPEN.**
  - O-simpleFM has no `withBackend` at HEAD or in the working tree (`Source/PluginEditor.cpp:281-289`).
  - 44/46 have no `areOptionsSupported` probe; only O-simpleSubtractive and O-TextureForge do.
  - 45/46 put the user data folder in `%TEMP%`.
- **Structural fix:**
  - **SH** `makeOuariconWebOptions(name)`:
    - `withBackend(webview2)`;
    - user data folder at `File::windowsLocalAppData/Ouaricon/WebView2/<plugin>`, created first;
    - `withStatusBarDisabled` and `withBuiltInErrorPageDisabled`;
    - `areOptionsSupported` probe plus a native notice with the Runtime link (O-simpleSubtractive 1.7.4 pattern).

    Note: O-simpleSubtractive used `userApplicationDataDirectory`, which is *Roaming* `%APPDATA%`. A browser cache belongs in Local; MS docs use `LocalApplicationData`.
  - **G** options lint: every `PluginEditor.cpp` uses the shell, or has `withBackend` + `withUserDataFolder` + keepPageLoaded.
  - **GEN** fix the three templates.
  - **PROC** a Windows editor smoke test: open the editor and assert that navigation completed. pluginval opens the editor but cannot see a blank page.
  - Done already: the installer bundles the WebView2 bootstrapper (`ea44af28`).

**C2 — Page unloaded on hide (`about:blank`), so the UI is blank or loses its state when shown again**
- **Occurrences:**
  - O-Tremolo 1.1.4: a static `hasNavigated` gave a blank window on reopen.
  - Upstream: JUCE fixed one FL Studio path (`744986733`, in 8.0.15).
  - #1577 (FL Studio macOS) is still open, with the comment "FL Studio wants `.withKeepPageLoadedWhenBrowserIsHidden()`".
  - #1560 is still open: a parent shown later never re-runs `checkWindowAssociation`, so the page stays blank on macOS.
- **Root cause:** the JUCE default is `unloadPageWhenHidden = true` (`juce_WebBrowserComponent_mac.mm:734-755` navigates to `about:blank`, then `goBack()`). Re-show runs the whole page reload. Page-only state such as the open tab, popovers and session-only toggles is lost.
- **State: OPEN, latent.** 26/46 lack keepPageLoaded:
  - O-AnalogEQ, O-AnalogSaturation, O-Bass, O-Bells, O-Bitrot, O-Chorus, O-Comp;
  - O-Contrabass, O-Detune, O-DigiDelay, O-Emulator, O-Freeze, O-FreqPulse;
  - O-GrainScatter, O-IntonationPad, O-Lyrica, O-Marimba, O-MicrotonalSampler;
  - O-MultiBandCompressor, O-Polystutter, O-Prism, O-Reed, O-SpectralShaper;
  - O-Strata, O-TextureForge, O-Tremolo.
- **Structural fix:**
  - **SH** sets keepPageLoaded unconditionally. Safe here: completions drop only when the WebView's *own* visible flag is false, and 0 editors call `setVisible` on it.
  - **G** options lint.
  - **GEN** stop calling it "OPTIONAL (FL Studio fix)".

**C3 — Host gesture left open when the editor is destroyed mid-drag**
- **Occurrences:** found in the O-simpleWavetable 1.0.1 review. 4 editors have `closeOpenGestures`: O-Octagon, O-simpleAmbisonics, O-simpleReverb and O-simpleWavetable.
- **Root cause:** `~WebSliderParameterAttachment` only calls `removeListener` (`juce_ParameterAttachments.cpp:290`). A `sliderDragStarted` with no matching ended event is never closed, so touch automation keeps writing and the next begin nests.
- **State: OPEN** in 42/46.
- **Structural fix:**
  - **FP (recommended):** a 6-line override. A `gestureOpen` flag is set in `sliderDragStarted` and cleared in `sliderDragEnded`; the destructor calls `attachment.endGesture()` if the flag is still set. It lives in `juce_ParameterAttachments.{h,cpp}` and covers all 46 plus future plugins.
  - **SH alternative:** the O-simpleWavetable `closeOpenGestures()` mixin (`PluginEditor.cpp:540-560`).

**C4 — Async FileChooser completion use-after-free, plus the MSVC `SafePointer` init-capture compile break**
- **Occurrences:** 17 plugins / 26 versions mention SafePointer fixes. Seven carriers broke the Windows build (O-Lyrica, O-IntonationPad, O-Polystutter, O-Contrabass, O-MicrotonalSampler, O-Wind, O-simplePhysicalModelSynth).
- **Root cause:** each editor hand-rolls `launchAsync([this, …])`. The `complete` callback holds a raw `Impl*`, so it must not be called after teardown.
- **State:** closed for the known sites. The class reopens with every new chooser.
- **Structural fix:**
  - **SH** `launchChooserSafely(editor, chooser, complete, body)`, with the hoisted SafePointer and no completion on the null path.
  - **G** grep for `= juce::Component::SafePointer` inside capture lists.

**C5 — Locale-dependent float formatting in C++→JS payloads**
- **Occurrences:** O-MultiBandCompressor 1.6.1 WR-01: meters and crossover lines froze on comma-decimal host locales.
- **Root cause:** `String::formatted("%f")` goes through `vswprintf` and honours `LC_NUMERIC`. "1,5" in a JS call is a syntax error. `%f` with NaN or Inf prints `nan`/`inf`, which is a ReferenceError.
- **State: OPEN** in 5 plugins:
  - O-Bass `PluginEditor.cpp:268` (meters);
  - O-Comp `:286` (meters);
  - O-Gain `:217-238` (all meters and LUFS);
  - O-Bowed `:160` (bow-state JSON);
  - O-Orbit `:740` (motionUpdate JSON).
- **Structural fix:**
  - **SH** `emitToPage(id, juce::var)`, built only from `var`/`DynamicObject`. `JSON::toString` is locale-free and writes non-finite values as `null` (`juce_JSON.cpp:500-511`).
  - **G** ban `String::formatted`, `std::to_string` and `snprintf` in editors.

**C6 — Non-finite or unescaped values in payloads**
- **Occurrences:**
  - O-Prism IN-16: a NaN froze the waveform through a `JSON.parse` throw.
  - O-Octagon IN-12: a host NaN.
  - O-Lyrica IN-16: hand-concatenated JSON was not escaped.
- **Root cause:** hand-built JSON.
- **State:** closed where found. Covered by C5's helper.
- **Structural fix:** **SH**, the same emit helper.

### L2 — Bridge contract (C++ ↔ JS names and types)

**C7 — Native function called from JS but never registered, so the promise never settles and the control is silently dead**
- **Occurrences:**
  - O-simpleGrain `uiMidi` (v1.0.2);
  - O-Lyrica 1.5.1, preset bar;
  - O-Reed tuning tab (2026-08-31);
  - O-Bassoon (found today).

  `boot-all-uis` cannot see any of them: the stub invents a value and the mounts are lazy.
- **Root cause:** JUCE `handleNativeFunctionCall` does `jassertfalse; return;` on an unknown name (`juce_WebBrowserComponent.cpp:307-313`), so no completion arrives. The shared `tuning-panel.js` calls 22 names, and each consumer must register all of them by hand.
- **State: OPEN.**
  - O-Reed: 20 of 22 panel names unregistered (lazy mount at `Resources/ui/index.html:2081`).
  - O-Bassoon: `generateRank2`, `saveScalaFile` and `saveKBMFile` unregistered. The panel always renders the Save .SCL/.KBM buttons and the Rank-2 option (`tuning-panel.js:223,240,281-282,903,946,955`).
- **Structural fix:**
  - **G** static name-closure: every `getNativeFunction('x')` literal in page JS and CMake-embedded module JS needs a `"x"` literal in `Source/*.{cpp,h}`. The script is `scripts/bridge_diff2.py`; it is <1 s, and its 3 false positives can be allow-listed.
  - **SH** each shared module ships its own `registerNativeFunctions(options, engine)`, so consumers can't half-register. O-MicrotonalSampler already splices the drop-streaming module's entries this way.
  - **JM** `callNative(name, …, {timeoutMs})`, which rejects with a visible error.
  - **FP (optional):** complete unknown names with `var()` instead of `return`.

**C8 — Relay and parameter binding drift**
- **Occurrences:** 6 incidents in 6 plugins, all shipped:
  - O-Wind CR-01: the entire FX tab, 21 parameters, had no relays;
  - O-Lyrica 1.35.1: FX tab dead;
  - O-Formant CR-04: 3 knobs;
  - O-GrainScatter CR-01: the flagship Scan knob;
  - O-Polystutter 1.3.1: a relay for a removed parameter gave a nullptr and a blank UI;
  - O-Bells WR-03: a ComboBox in C++ was bound as a slider in JS.
- **Root cause:**
  - Relays are hand-listed per editor. For example, O-Polystutter has 156 `make_unique<Web*Relay>` lines, O-Bells 69, O-Lyrica 68 and O-Formant 57.
  - The JUCE frontend `getSliderState(id)` silently creates an inert state for an unknown id.
- **State:** closed where found. The class is structurally open: 35+ editors hand-list their relays.
- **Structural fix:**
  - **SH** generate relays and attachments from `processor.getParameters()` by type (Float→Slider, Bool→Toggle, Choice→ComboBox), and expose a native `getParameterIds()`.
  - **JM** a boot assertion that every `get*State(id)` the page uses is in that list.
  - **G** a static closure check from page ids to `createParameterLayout` ids.

**C9 — Wrong JS namespace or API**
- **Occurrences:**
  - O-MicrotonalSampler v1.0–1.1: tuning panel given `window.__JUCE__`;
  - O-FreqPulse WR-01: tooltip preference never persisted;
  - O-Marimba 1.13.0: `window.JuceAPI` never existed;
  - O-Prism 1.8.1: old `addListener` API threw a TypeError;
  - O-Lyrica 1.7.x: modules;
  - 2025: TapeAge, LushVerb, FlutterVerb (`getToggleButtonState`, `type=module`, `valueChangedEvent` argument).

  6 plugins / 8 versions since January.
- **Root cause:** untyped JS against a small API, inside try/catch that swallows the error.
- **State:** closed in plugins. **OPEN in the generator:** `index-template.html:340` still calls `getToggleButtonState`.
- **Structural fix:**
  - **G** `// @ts-check` plus `.d.ts` from JUCE 9.0.1's `@juce-framework/webview` (types only, no runtime change), or a lint that allows only the `Juce` exports.
  - **GEN** fix the template.

**C10 — Load-time JS throw kills the module (TDZ, ReferenceError after extraction, undefined variable)**
- **Occurrences:**
  - O-MultiBandCompressor 1.4.0: crossover drag dead;
  - O-MicrotonalSampler 1.13.0: drag-drop dead;
  - O-Lyrica 1.7.3;
  - O-MicrotonalSampler WR-02: latent.
- **Root cause:** ES module evaluation aborts at the first throw. No inspector exists in Release, and 0/46 pages report JS errors to C++.
- **State:** `boot-all-uis` catches default-state throws (46/46 clean today) but only runs by hand. Lazy panels are unchecked.
- **Structural fix:**
  - **G** run `boot-all-uis` page-error and 404 checks in CI.
  - **JM** `window.onerror`/`unhandledrejection` → native `reportUiError` → `~/Library/Logs/Ouaricon/<plugin>.log`. This turns every silent field failure into a log line.

### L3 — State sync (C++ state → page)

**C11 — Readout, reset or value-entry range drift (JS re-derives `NormalisableRange`)**
- **Occurrences:**
  - 16 plugins / 20 versions fixed;
  - O-MicrotonalSampler: wrong for about 20 versions;
  - O-Prism: 11/68 resets wrong;
  - O-Marimba shared analog-eq-unit: 3.5× wrong;
  - O-Lyrica A4 415–465 against a real 400–480, found *again* on 2026-10-19;
  - lambda ranges are invisible to the frontend (O-Strata).
- **Root cause:** a second source of truth in JS. Skew bugs are invisible unless skew ≠ 1. The 2026-01 generator template taught the anti-pattern.
- **State:** partly closed. Candidates: 11 pages have no `getScaledValue()` use (O-AnalogEQ, O-AnalogSaturation, O-Bass, O-Bassoon, O-Chorus, O-DigiDelay, O-Marimba, O-Polystutter, O-Prism, O-Reed, O-TextureForge). Some derive from `.properties` correctly, so each needs verifying.
- **Structural fix:**
  - **JM** readout and knob canon: `getScaledValue()`, `propertiesChanged`, and `getParameterDefaults` (already in the R2 docs).
  - **G** a defaults detector (`bindKnob` default against `createParameterLayout`).
  - **SH** transmit lambda ranges through a native function.

**C12 — Non-APVTS state never re-pushed after host restore, preset load, undo or A/B**
- **Occurrences:**
  - O-SpectralShaper 1.6.1: curves;
  - O-DigiDelay WR-07: preset name;
  - O-Formant WR-10: preset name, lyrics, language, tuning panel;
  - O-Chorus IN-03: preset name, fixed with a 2 Hz poll;
  - O-Prism REG-02: restore killed live preview;
  - O-Prism 1.13.2: A4 knob init hard-coded.
- **Root cause:** a one-shot pull or push at load. The shared `preset-manager.js` reads `getCurrentPreset` once, and there is no host→page channel.
- **State: OPEN, latent.** About 10/46 have a revision or generation channel (O-DigiDelay, O-Formant, O-SpectralShaper, O-Octagon, O-GrainScatter, O-Tapestop, O-simpleAmbisonics, O-simpleReverb, O-simpleWavetable) or a poll (O-Chorus). About 35 module consumers go stale after DAW undo or A/B.
- **Structural fix:**
  - **SH and module:** the `OuariconPresetManager` C++ side bumps an atomic `stateRevision` in `setStateFromXml`, `loadPreset` and `setCurrentProgram`, and the editor timer emits `stateChanged`.
  - **JM:** `preset-manager.js` subscribes and refreshes.
  - One module change; rebuild every plugin.

**C13 — Poll vs local-edit race (a stale snapshot overwrites an in-flight edit, or a re-render destroys input)**
- **Occurrences:**
  - O-Octagon B2;
  - O-simpleBeatmaker IN-08;
  - O-MicrotonalSampler WR-03;
  - O-simpleGrain IN-08;
  - O-Prism WR-16: native writes were reverted by the APVTS→engine sync.

  5 plugins.
- **Root cause:** no generation stamps; DOM rebuilt under focus.
- **State:** closed where found; the pattern recurs.
- **Structural fix:** **JM** a state-sync client with generation-stamped payloads, "dirty until acked", and no rebuild of a focused subtree.

**C14 — Initial-state and handshake race**
- **Occurrences:**
  - O-SpectralShaper `callAfterDelay(100)` one-shot push (`PluginEditor.cpp:355`);
  - upstream "BR: WebView freezes DAW": `evaluateJavascript` before page load with multiple instances (Windows), fixed by `90e72dbd9`, in 8.0.15.
- **Root cause:** timer-guessed readiness. `pageFinishedLoading` is used in 0/46.
- **State:** latent. Only 3 plugins have a `uiReady` handshake (O-simpleAmbisonics, O-simpleReverb, O-simpleWavetable).
- **Structural fix:** **SH** a `uiReady` handshake with counters snapshotted at ready (the O-simpleReverb pattern, `PluginEditor.cpp:365,554`).

**C15 — Completions dropped while the WebView's own visible flag is false**
- **Occurrences:** O-Octagon review (corrected 2026-08-25).
- **Root cause:** `emitEventIfBrowserIsVisible` (`juce_WebBrowserComponent.cpp:336-345,607-611`).
- **State:** closed and latent. 0 editors hide the WebView.
- **Structural fix:** **G** ban `setVisible` on the WebBrowserComponent. **JM** fixed-interval polls with an in-flight guard.

### L4 — Interaction (pointer, keyboard, focus, entry)

**C16 — Value-entry box stays open with a frozen readout when the next press is a dial that calls `preventDefault`**
- **Occurrences:** reproduced in about 30/41. Fixed in 41 plugins through about 39 commits on 2026-10-19 (`261019-ryp`).
- **Root cause:** the blur commit never fires, and every page carries its own copy of the entry code.
- **State:** closed in 41. O-Bassoon, O-Freeze, O-Gain, O-Reed and O-simpleFM have no value entry yet; they were excluded as D-04 dirty trees and still have dblclick-reset. They must get the *fixed* port.
- **Structural fix:** **JM** a knob and value-entry module, embedded directly, never copied.

**C17 — Drag latched after release outside the WebView, host modal or focus loss; stuck on-screen-keyboard notes**
- **Occurrences:** O-Chorus IN-02, O-SpectralShaper 1.10.0, O-simpleAdditive WR-04/05, O-simpleBeatmaker IN-07, O-simpleFM WR-01 and O-Bells. 6 plugins.
- **Root cause:** `document` mouseup with no pointer capture. Only 4 plugins handle `lostpointercapture` (review §1.4), and 0 handle window `blur` uniformly. Combined with C3, this leaves a stuck automation gesture.
- **State:** OPEN, latent, in most Family A′/B/C pages.
- **Structural fix:** **JM** pointer lifecycle (`setPointerCapture`; `pointerup`, `pointercancel` and `lostpointercapture` each end the drag once; `blur` and `pagehide` end it too). **G** a probe that fires `lostpointercapture` and asserts that `sliderDragEnded` follows.

**C18 — `pointerdown` + `preventDefault` kills `document` mousedown dismissers**
- **Occurrences:** O-Chorus IN-02.
- **Root cause:** compatibility mouse events are suppressed.
- **State:** closed where found.
- **Structural fix:** **JM** dismiss on capture-phase `pointerdown`.

**C19 — Tooltip re-opens on click, focus or hover ("focus latch = 3 defects")**
- **Occurrences:** item 58 and "click parks a tip": O-AnalogSaturation, O-Bass, O-DigiDelay, O-Reed, O-Texture, O-Lyrica, O-Orbit, O-simpleGrain, O-simpleSampler, O-simpleAdditive and O-Bells. About 11 plugins.
- **Root cause:** 29 distinct tooltip renderers.
- **State:** closed where found; drift is ongoing.
- **Structural fix:** **JM** one tooltip renderer, the O-ReverseDelay canon. Make `check-ui-canon` a gate once burned down.

**C20 — Tooltip geometry (shrink-to-fit ribbon, `offsetWidth` rounding, covering its own control)**
- **Occurrences:** O-MultiBandCompressor 1.4.1, O-FreqPulse 1.17.0 (46/53 tips covered their control), O-SpectralShaper, O-Bitrot, O-Contrabass, O-Orbit and O-ReverseDelay. 7 plugins.
- **Root cause:** measure-then-place on a `position:fixed` box with `width:auto`.
- **State:** a clamp gate exists in 3 plugins and a tip-render gate in 23.
- **Structural fix:** **JM** the same renderer. **G** one parametrised `scripts/` clamp gate at the shipping frame instead of 23 copies.

**C21 — Native `title=` duplicate OS tooltip (untranslated)**
- **Occurrences:** O-Marimba (8), O-Lyrica, O-simpleGrain, O-AnalogEQ and O-MicrotonalSampler.
- **Root cause:** a `title` fallback in the page.
- **State: CLOSED.** `boot-all-uis` finds title=0 on 46/46 today.
- **Structural fix:** **G** keep the `title`=0 assertion.

**C22 — Host keyboard shortcuts swallowed while the WebView has focus (spacebar, transport, typing-keyboard MIDI)**
- **Occurrences:**
  - upstream: forum t/63987, open since 2024, reported in Ableton and Logic on mac and on Windows in 2026;
  - JUCE #1522 (AU Ableton arrows and paste) was fixed in 8.0.x.
- **Root cause:** the native view consumes key events. JUCE has no pass-through option.
- **State: OPEN** for 0/46. In-suite cousins:
  - O-simpleFM WR-02: QWERTY notes fired while a control had focus;
  - O-Octagon R3: IME Enter;
  - O-Octagon B10: keys opened gestures.
- **Structural fix:**
  - **FP (requires decision):** add `keyDown:`/`keyUp:` to the mac WKWebView class, forwarding unhandled keys to `nextResponder` (the forum workaround), behind an opt-in. The page marks the keys it consumes.
  - On Windows: a spike on WebView2 `AcceleratorKeyPressed` forwarding.
  - **JM** one keyboard policy module.

**C23 — JS modal dialogs are no-ops in JUCE's WKWebView**
- **Occurrences:** O-Prism CR-10 (Save dead on mac), O-Strata (fork) and O-Formant CR-05.
- **Root cause:** `juce_WebBrowserComponent_mac.mm` registers only `runOpenPanel…` UI-delegate methods (`:315`, `:574`). WebKit then treats `confirm` as Cancel and `prompt` as `null`. See the WebKit `WKUIDelegate.h` reference below.
- **State: OPEN** in 3 sites:
  - O-IntonationPad `js/app.js:1302` (`prompt`, rename) and `:1320` (`confirm`, delete);
  - O-Polystutter `js/app.js:553` (`confirm`, delete).

  The shared `preset-manager.js` also falls back to `window.confirm` when a consumer gives no `onConfirmDelete`.
- **Structural fix:**
  - **JM** an in-DOM dialog module.
  - **G** ban `prompt`, `confirm` and `alert`.
  - **FP (optional):** implement the three `WKUIDelegate` panels.

**C24 — Mouse-only knobs and missing accessible names**
- **Occurrences:** 34/44 mouse-only (review §5.1). Accessible-name fixes in 12 plugins.
- **Root cause:** hand-copied knobs.
- **State:** OPEN (R7, opportunistic).
- **Structural fix:** **JM** the knob module ships arrows, ARIA and `aria-valuetext`.

### L5 — Rendering, layout, i18n and fonts

**C25 — Paint order or stacking contexts make controls unclickable**
- **Occurrences:**
  - settings popover under the tab bar: O-Bassoon (since v1.1.0), O-Wind and O-MicrotonalSampler, so the language selector was dead;
  - O-MultiBandCompressor popover vs analyser;
  - O-Bitrot menu vs tip;
  - O-Lyrica footer over sliders;
  - O-Orbit overlay.

  7 plugins.
- **Root cause:** a z-index tie broken by document order (`body{display:flex}`). Rect and attribute gates are blind to it.
- **State:** closed where found. The hit-test probe exists only in a Stage M scratch folder.
- **Structural fix:** **G** an `elementFromPoint` reachability assertion for every interactive control in every gate state, inside check-ui-labels.

**C26 — Language-dependent geometry (wraps, pins, select widths, Han line-height, form controls not inheriting the font)**
- **Occurrences:** 13 plugins / 14 versions fixed, plus about 60 wave commits.
- **Root cause:** per-language width pins, and controls that inherit no `font-family`.
- **State:** check-ui-labels and measure-ui run by hand.
- **Structural fix:** **G** run check-ui-labels in CI (Playwright). A 4th language multiplies the cost.

**C27 — Face never rendered, canvas text left on the fallback face, or the registry missing consumers**
- **Occurrences:** R5 EB Garamond reached 34 plugins. Canvas `fillText` kept the fallback face (O-Formant). `used_by` lists 1 of 34.
- **Root cause:** no bundled font before R5; canvas paints with whatever face is loaded at draw time.
- **State:** mostly closed.
- **Structural fix:** **JM** `document.fonts.load()` before repainting canvases. **PROC** backfill the registry.

**C28 — SVG transform and canvas sizing / HiDPI**
- **Occurrences:**
  - O-Octagon: arcs 507 px off (`transform-box`);
  - O-TextureForge: canvas stuck at 300×150;
  - DPR read once, so canvases blur after moving to another display: O-Formant IN-22c, O-simpleAmbisonics IN-07, O-Texture IN-10.
- **Root cause:** per-page canvas code. Census: 30 pages draw canvases; 4 have no DPR handling at all (O-Bells, O-Comp, O-Lyrica, O-Tremolo); only 4 re-arm on a resolution change (O-Formant, O-Octagon, O-simpleAmbisonics, O-simpleReverb).
- **State:** OPEN, latent: about 22 blur after a display move and 4 are always blurry on Retina.
- **Structural fix:** **JM** `hidpiCanvas(canvas, draw)`, using `ResizeObserver` and re-armed `matchMedia('(resolution: Ndppx)')`.

**C29 — Shared-module layout regression reaching installed binaries**
- **Occurrences:** tuning-panel v3.0.0 scrambled the grid. O-Wind, O-Bowed and O-Reed stayed broken until each was rebuilt.
- **Root cause:** direct embed has zero drift, but there is no record of which binaries are stale.
- **State:** closed.
- **Structural fix:** **PROC** module bump → rebuild obligations ledger.

**C30 — Stale in-page version literals**
- **Occurrences:** 8 plugins / 14 versions (O-Bells, O-SpectralShaper ×3, O-Detune ×2, O-AnalogSaturation ×2, O-Bassoon, O-Wind, O-simpleReverb, O-MicrotonalSampler).
- **Root cause:** a hand literal in `index.html`.
- **State:** recurs. O-Bassoon's label is still a literal.
- **Structural fix:** **SH** a `getPluginVersion` native from `JucePlugin_VersionString` (O-Wind 1.18.2 did this). **G** no `v\d+\.\d+\.\d+` literal in pages.

**C31 — Web features above the WKWebView baseline**
- **Occurrences:** deployment target macOS 10.13, while `:has()` is used in 13 pages' CSS, `.at()` in O-TextureForge, and the `popover` attribute is unsupported (JUCE #1618).
- **Root cause:** gates run Chromium and the newest WebKit only.
- **State:** latent; unverified on old macOS.
- **Structural fix:** **G** browserslist-based compat lint, or raise the minimum macOS to what is tested.

### L6 — Presets and persistence surfaced through the UI (adjacent)

**C32 — Preset UI coherence**
- **Occurrences:**
  - ◀▶ vs a grouped dropdown: O-SpectralShaper, O-Bitrot, O-DigiDelay;
  - out-of-list ◀▶ snaps to 0: O-Bass, O-DigiDelay, O-SpectralShaper;
  - a failed load still showed the new name: O-simpleReverb;
  - partial presets inherited stale parameters: WR-01, 11 plugins.
- **Root cause:** coherence "by coincidence", plus forked JS copies (6/11 tracked `preset-manager.js` are FORK).
- **State:** mostly closed.
- **Structural fix:** **JM** embed `preset-manager.js` directly everywhere. **G** content-hash check (exists, report-only).

**C33 — Save dialog has no factory guard**
- **Occurrences:** O-AnalogEQ WR-07.
- **Root cause:** `savePresetToFile` has no `isFactoryPreset` guard (module v1.0.9).
- **State: OPEN** in the module and 9 call sites: O-Bitrot, O-Contrabass, O-Emulator, O-FreqPulse, O-Orbit, O-ReverseDelay, O-simpleFM, O-Tapestop and O-Wind.
- **Structural fix:** **module** a location-aware guard (O-AnalogEQ `PresetSaveGuard.h`) moved into preset-manager.

**C34 — Stale `<CustomState>` child restores the first reopen's custom state on the second reopen**
- **Occurrences:** O-Bowed 1.9.3 proof.
- **Root cause:** module `setStateFromXml` replaces the whole tree without stripping (`OuariconPresetManager.h:617-640`, v1.0.9).
- **State: OPEN** in the module. Consumers per memory: O-Bells, O-Contrabass, O-Lyrica, O-ReverseDelay, O-SpectralShaper and O-Wind. O-Octagon and O-Tapestop are not callers.
- **Structural fix:** **module** strip inside `setStateFromXml`. **G** a two-reopen harness probe.

### L7 — Meta classes: why everything above recurs

**M1 — Hand-copy without a canon gate**
- **Evidence:** the canon gate gives 1 i18n runtime variant across 46. Without it: 29/29/23/19 variants (tooltip, hover-help, knob binding, knob visual), 6 preset-manager forks and 4 tuning-panel forks. Click-away took about 39 commits, value entry 17, hover-help 22, and i18n about 60 batch commits.
- **Structural fix:**
  - **JM** direct embed (zero drift) for runtime JS.
  - **G** `check-ui-canon.js` moves from report-only to gate once the burn-down finishes.

**M2 — Generator lags the contract**
- **Evidence:**
  - `PluginEditor-webview.cpp:43,55-57` (no backend, keepPageLoaded "OPTIONAL");
  - CMake snippet: no `NEEDS_WEBVIEW2` or static link;
  - `index-template.html:340` calls `getToggleButtonState`;
  - `gui-agent.md:622-643` has no `withBackend`;
  - `troubleshooting/patterns/juce8-critical-patterns.md` and `stage-3-patterns.md`, "REQUIRED READING", have no WebView2-backend, user-data-folder or keepPageLoaded entries.
- **Structural fix:**
  - **GEN** templates call the SH/JM modules instead of inlining.
  - **G** lint the templates with the same lints as plugins.

**M3 — Gates not load-bearing**
- **Evidence:**
  - UI Static Gates red on 18 straight pushes (2026-09-28 → 2026-10-20) from the configure-copy gap, masking a real fr-lint finding;
  - boot-all-uis blind to lazy bridge gaps;
  - about 15 memory notes describe gates that pass vacuously.
- **Structural fix:**
  - **PROC/G** have check-i18n resolve `ouaricon_add_module` copies from `modules/`, or run the copy step in CI.
  - Make the job a required check.
  - The SessionStart hook prints the last UI-gate CI conclusion.
  - Add a pre-commit hook for the static set.

**M4 — Dirty-tree exclusion strands sweeps**
- **Evidence:**
  - 5 plugins with UI passes uncommitted since 2026-09-25/27 missed the D-04 sweeps;
  - O-simpleFM missed `withBackend`.
- **Structural fix:** **PROC** a per-plugin "owed sweeps" list in STATUS.md that `/improve` must drain before its commit. Sweeps record exclusions there, not only in a SUMMARY.

**M5 — Mac-only verification**
- **Evidence:** WebView2/IE, locale, DPI and Windows key handling are invisible to the Mac gates. pluginval runs on Windows only at release.
- **Structural fix:** **PROC** a Windows editor smoke in `ci-tests.yml` (workflow_dispatch exists).

---

## 2. Known-open UI defects (at the current working tree)

| # | Plugin(s) | Defect | Evidence |
|---|---|---|---|
| 1 | O-simpleFM | No `withBackend(webview2)`, so Windows shows "page not found" | `Source/PluginEditor.cpp:281-289` (only `withWinWebView2Options`); missed `b477dc83` |
| 2 | O-Reed | Tuning tab dead: 20/22 shared-panel natives unregistered, so the promise hangs | `scripts/bridge_diff2.py`; `Resources/ui/index.html:2081` lazy mount; open since 2026-08-31 |
| 3 | O-Bassoon | Tuning panel's Rank-2 generator, Save .SCL and Save .KBM hang (3 natives unregistered) | `modules/tuning/scala-tuning-engine/js/tuning-panel.js:223,240,903,946,955`; O-Bassoon editor lacks the names |
| 4 | O-IntonationPad | User-preset Rename (`prompt`) and Delete (`confirm`) are no-ops on macOS | `Source/ui/public/js/app.js:1302,1320` |
| 5 | O-Polystutter | Preset Delete (`confirm`) is a no-op on macOS. It also carries an old vendored module copy with an unguarded `confirm` | `Source/ui/public/js/app.js:553`; `modules/preset-manager.js:336` |
| 6 | O-Bass, O-Comp, O-Gain, O-Bowed, O-Orbit | Locale-dependent `String::formatted("%f")` payloads. Meters and motion freeze on comma-decimal locales; `%f` NaN/Inf prints `nan`/`inf` | `PluginEditor.cpp` :268, :286, :217-238, :160, :740 |
| 7 | 42/46 (all except O-Octagon, O-simpleAmbisonics, O-simpleReverb, O-simpleWavetable) | Host gesture stays open if the editor closes mid-drag | `juce_ParameterAttachments.cpp:290` |
| 8 | 26/46 (list in C2) | Page unloads on hide, so the page is blank or loses state on re-show (FL Studio, nested visibility) | JUCE #1577 and #1560 open |
| 9 | 44/46 | No WebView2 availability probe or native fallback notice. 45/46 put the user data folder in `%TEMP%`; O-simpleSubtractive uses Roaming `%APPDATA%` | grep census |
| 10 | 46/46 | Host keyboard shortcuts swallowed while the WebView has focus | upstream forum t/63987 |
| 11 | About 35 module consumers | Preset name and custom state go stale after a DAW undo, A/B, host preset recall or session restore with the editor open | C12 census |
| 12 | Module + 9 call sites | `savePresetToFile` has no factory guard (silent shadow or overwrite of factory presets) | `OuariconPresetManager.h`; memory note |
| 13 | Module (v1.0.9) + about 6 consumers | Stale `<CustomState>` after the second reopen | `OuariconPresetManager.h:617-640` |
| 14 | O-Bells, O-Comp, O-Lyrica, O-Tremolo (always); about 22 more (after a display move) | Blurry canvases: no DPR handling, or DPR read once | canvas census |
| 15 | CI | UI Static Gates red since 2026-09-28: a harness gap plus a real O-DigiDelay fr-lint T4 | `gh run list --workflow ui-static-gates.yml`; `plugins/O-DigiDelay/Source/ui/public/js/i18n.js:408` |
| 16 | Generator | Templates emit an IE-fallback editor (no backend, no `NEEDS_WEBVIEW2`, no static link) and a dead toggle (`getToggleButtonState`) | `.claude/skills/ui-mockup/assets/webview-templates/*`; `.claude/agents/gui-agent.md:622-643` |
| 17 | O-Bassoon, O-Freeze, O-Gain, O-Reed, O-simpleFM | Missing the suite's double-click value entry and Alt-reset port. Not a bug, but the port must include the click-away fix | D-04 in `261019-gk9`/`ryp` SUMMARYs |
| 18 | O-Bassoon (+ any with a hand literal) | In-page version label is a hand literal | O-Bassoon CHANGELOG 1.6.0 |
| 19 | Latent: 11 pages with no `getScaledValue` | Readout or reset drift candidates; verify each | readout census |
| 20 | Latent: most pages without `lostpointercapture`/`blur` handling | Drag latch risk; with #7 this gives a stuck automation gesture | review §1.4; UI census |

Recorded, not re-verified: O-IntonationPad has 55 `line-height-normal` and 2 `wrap-count` measure-ui findings (zh). The O-Chorus CJK tail is inert (it renders Songti, not PingFang). Both are listed in `project_i18n_zh_hans_rollout.md`.

---

## 3. Structural program: the fewest changes that kill the most classes

Ordered by classes killed ÷ effort. Every per-plugin change still needs a rebuild, because the UI ships in `juce_add_binary_data`.

1. **Fix the gate that already exists (M3).** XS effort.
   - Make check-i18n [12] resolve `ouaricon_add_module` copies from `modules/`, or run the copy in CI.
   - Fix O-DigiDelay T4.
   - Make the workflow a required check.
   - Surface its conclusion in the SessionStart hook.
   - Without this, every gate below repeats the 28-day red streak.
2. **Add four static gates (<2 s, no browser) to `ui-static-gates.yml` and a pre-commit hook.**
   - **(a) Bridge closure** (C7). The scratch script is a ready prototype.
   - **(b) Parameter closure:** page `get*State` ids ↔ `createParameterLayout` ↔ relay names (C8).
   - **(c) Editor options lint:** `withBackend`, user data folder, keepPageLoaded, no `setVisible` on the WebView (C1, C2, C15).
   - **(d) Banned APIs:** `prompt`/`confirm`/`alert`, `String::formatted`/`to_string` in editors, `getToggleButtonState`, `title=`, `offsetWidth` in tip code, version literals (C5, C9, C21, C23, C30).

   Run all of them on the generator templates too (M2).
3. **SH: `modules/ui/web-editor-shell` (direct-embed header).**
   - `makeOuariconWebOptions()`, covering C1 and C2;
   - relay and attachment generation from `getParameters()` plus `getParameterIds`, covering C8;
   - a `var`-only `emitToPage`, covering C5 and C6;
   - a `uiReady` handshake plus a `stateRevision` channel, covering C12 and C14;
   - `launchChooserSafely`, covering C4;
   - `getPluginVersion`, covering C30;
   - `reportUiError` → log file, covering C10.

   Adopt it per plugin through `/improve`, starting with O-simpleFM and new plugins. It is the generator's default from day one.
4. **FP: three small `vendored/JUCE-overrides` patches.**
   - **(a)** `WebSliderParameterAttachment` destructor ends an open gesture (C3): 46 plugins, about 6 lines, no per-plugin code.
   - **(b)** Unknown native → complete with `var()` instead of hanging (C7 defence in depth).
   - **(c) Requires decision:** mac WKWebView `keyDown:`/`keyUp:` pass-through for unhandled keys (C22). Optionally the `WKUIDelegate` JS panels (C23).

   The override fingerprint gate (`scripts/check-juce-overrides.sh`) already exists. Each JUCE bump re-applies them.
5. **JM: direct-embedded runtime modules, not copies.**
   - knob + value entry + pointer lifecycle + ARIA, using the O-ReverseDelay canon plus O-Prism entry, for C11 and C16–C19 and C24;
   - tooltip renderer, for C19 and C20;
   - in-DOM dialog, for C23;
   - `hidpiCanvas`, for C28;
   - state-sync client with generation stamps, for C13;
   - `callNative` with timeout, for C7.

   This follows the recorded "no shared runtime retrofit" decision (review "Not recommended"): new plugins take the modules, and existing ones converge only when touched by `/improve`, so there is no sweep. `check-ui-canon` tracks the burn-down.
6. **Preset-manager module v1.1:**
   - strip `<CustomState>` (C34);
   - location-aware save guard (C33);
   - `stateRevision` emit (C12);
   - embed directly in the 6 forked consumers (C32).
7. **PROC:**
   - an owed-sweeps ledger per plugin, so dirty trees can't strand fixes (M4);
   - a Windows editor smoke test (M5);
   - module bump → rebuild ledger (C29);
   - commit or park the 5 month-old uncommitted UI passes.
8. **JUCE upgrade:**
   - Take `8.0.16-security` as a drop-in.
   - JUCE 9.0.x adds no WebView reliability fix; only TS types, which are useful for gate 2's typing.
   - Do not expect upstream to fix C2, C22 or C23 soon. #1560 and #1577 are open, and the keyboard thread had no fix through 2026-09.

---

## 4. Upstream evidence (JUCE, WebKit, WebView2)

| Item | Status | In local 8.0.15? | Relevance |
|---|---|---|---|
| [JUCE #1577](https://github.com/juce-framework/JUCE/issues/1577): FL Studio macOS blank on Tab, mouseout dead spots | open; the 2025-10-04 comment says keepPageLoaded fixes it | n/a | C2 |
| [JUCE #1560](https://github.com/juce-framework/JUCE/issues/1560): `checkWindowAssociation` not called when a parent is shown, so the page stays blank on mac | open | n/a | C2 |
| [commit 744986733](https://github.com/juce-framework/JUCE/commit/744986733): "MacOS: Fix WebBrowserComponent going blank in FL Studio" | fixed 2025-10-06 | yes (`didFinishNavigationCallback` present) | C2 (one path) |
| [commit a34bd7413](https://github.com/juce-framework/JUCE/commit/a34bd7413): Windows state handling when invisible | fixed 2024-08 | yes | C2 |
| [commit 245060021](https://github.com/juce-framework/JUCE/commit/245060021): WebView2 DPI mismatch with Ableton "Auto-Scale Plug-In Window" | fixed 2026-07-09 | yes (`juce_WebBrowserComponent_windows.cpp:1061-1065`) | HiDPI on Windows |
| [Forum: BR WebView freezes DAW](https://forum.juce.com/t/br-webview-freezes-daw/64917) / [commit 90e72dbd9](https://github.com/juce-framework/JUCE/commit/90e72dbd9): multi-instance WebView2 creation loop; JS before page load | fixed 2025-01-17 | yes | C14 |
| [Forum: WebViewPluginDemo crashes when repeatedly launched](https://forum.juce.com/t/juce-8-webview-bug-webviewplugindemo-crashes-when-repeatedly-launched/62131) (`ae23783f`) | fixed 2024-07 | yes | lifecycle |
| [JUCE #1522](https://github.com/juce-framework/JUCE/issues/1522): AU in Ableton, WebView keys (paste, arrows); fixed by `7f4176e259` (Esc crash, `ComponentPeerHelpers`) | fixed on develop 2025-03; issue still open | yes (`juce_ComponentPeerHelpers.cpp` present) | C22 cousin |
| [Forum: Fixing WebView Keyboard Focus Issue](https://forum.juce.com/t/fixing-webview-keyboard-focus-issue/63987): spacebar and keyboard MIDI swallowed (Ableton/Logic mac; Windows reports 2026-02 and 2026-09). JUCE staff (2024-10): "plans to ease the usage of the WebView with keyboard shortcuts". Workaround: `.mm` `keyDown:`/`keyUp:` → `nextResponder` | no JUCE fix through 2026-09 | no | C22 |
| [JUCE #1642](https://github.com/juce-framework/JUCE/issues/1642): WebView2 COM events silently fail with MSVC /O2 or /Ox (white screen) | closed 2026-10-16, not reproducible by JUCE or the reporter | n/a | watch item for C1 (Windows Release blank) |
| [JUCE #1618](https://github.com/juce-framework/JUCE/issues/1618): `popover` attribute unsupported by the WKWebView WebKit | open; JUCE says external dependency | n/a | C31 |
| [JUCE #1561](https://github.com/juce-framework/JUCE/issues/1561): defaults (`withNativeIntegrationEnabled` off) silently break resource providers | open | n/a | argues for SH options |
| [Forum: WebBrowserComponent very poor linux experience](https://forum.juce.com/t/webbrowsercomponent-very-poor-linux-experience/66681); [#1730](https://github.com/juce-framework/JUCE/issues/1730), [#1731](https://github.com/juce-framework/JUCE/issues/1731) (Linux needs GTK/WebKit dev packages); [#1559](https://github.com/juce-framework/JUCE/issues/1559) (cannot hide on Linux) | open | n/a | not shipped here; blockers if Linux is ever targeted |
| [WebKit `WKUIDelegate.h`](https://webkit.googlesource.com/WebKit/+/5088a8fc944bceb7e8a403e0cde6a58f89ef7d28/Source/WebKit/UIProcess/API/Cocoa/WKUIDelegate.h): if the confirm or prompt panel methods are unimplemented, WebKit behaves as if the user cancelled. JUCE's mac delegate implements only `runOpenPanel` (local `.mm:315,574`) | by design | n/a | C23 |
| [MS Learn: Manage user data folders](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/user-data-folder): specify a custom user data folder with read/write access; one session per folder; creation fails if options differ; delete only after the browser process exits. The sample path is LocalApplicationData | docs | n/a | C1 (Local, not Temp or Roaming) |
| [Forum: WebView2 does not use Edge but IE](https://forum.juce.com/t/webview2-does-not-use-edge-but-ie/50492): runtime missing or a host needing a user data folder causes the IE fallback | — | n/a | C1 |
| [JUCE releases](https://github.com/juce-framework/JUCE/releases): 8.0.16-security (2026-10-13, "protection against malformed String input"); 9.0.0–9.0.4 (2026-07-21 → 10-12) | — | — | §3.8 |
| [JUCE 9.0.1 announcement](https://forum.juce.com/t/uce-9-0-1-is-out/69269) / [BREAKING_CHANGES](https://raw.githubusercontent.com/juce-framework/JUCE/master/BREAKING_CHANGES.md): interop moved to `native/typescript/webview-interop`; npm `@juce-framework/webview` | — | — | gate 2(b)/C9 typing |
| Drag-drop: [forum 45733](https://forum.juce.com/t/webbrowsercomponent-consumes-drag-events/45733), [forum 66000](https://forum.juce.com/t/webview-drop-file-from-daw-into-plugin/66000): WKWebView consumes OS drags; JS `webkitGetAsEntry` streaming is the only working route | known | n/a | closed in-suite (`modules/core/webview-drop-streaming`, O-MicrotonalSampler v1.0.4); O-TextureForge works around it with a button |

**Vendored JUCE frontend lib**

- **`js/juce/index.js`:** 49 copies across 3 hashes. All differ from 8.0.15 only by CRLF or one blank line.
- **`check_native_interop.js`:** 4 hashes.
  - 7 shipped copies predate the Feb-2025 `removeEventListener` fix ([JUCE #1500](https://github.com/juce-framework/JUCE/issues/1500), commit `389516fee`): O-MultiBandCompressor, O-AnalogSaturation, O-Gain, O-Comp, O-Bass, O-IntonationPad and O-Marimba.
  - O-Detune, O-SpectralShaper and O-Freeze ship a 4-line placeholder instead of the real file.
- **Harmless inside a host.** The native side injects the same backend at document start (`juce_WebBrowserComponent.cpp:47-160`). The page copy only defines `window.__JUCE__.backend` when it is undefined, so the injected 8.0.15 code wins.
- **Matters only in browser harnesses.** There the copies behave differently, which feeds the M3 "gate differs from host" problem. Low priority: replace them with the 8.0.15 file whenever a page is touched.

---

## 5. Method and reproducibility

Scripts are in `/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/39983b81-85a2-44fc-8aff-422a28a35353/scratchpad/scripts/`. All of them read only.
- `classify2.py` (output `../changelog_classes2.json`): 35 regex classes over CHANGELOG "Fixed" bullets, with samples. `classify.py` is an earlier coarse pass.
- `bridge_diff2.py`: native-function name closure between page and module JS and the editor's C++ literals. Remaining false positives:
  - O-MicrotonalSampler (the module splices its own entries);
  - O-simpleSampler and O-simpleWavetable (they import only the encoder);
  - O-Marimba (the hit is a comment).
- `ui_census.py`: pointer capture, `lostpointercapture`, `pointercancel`, document mouseup, getScaledValue, keydown and so on. `readout_census.py` is the range-mirror heuristic, `canvas_census.py` covers DPR and resolution re-arm, and `ui_share.py` gives the UI share of fixes by month.
- `boot-all-uis` run: `../boot-all-uis.txt`, `../boot-all-uis.json` (46/46 clean, 2 late O-Bells tips, 0 dead, 30 invented natives).

Shell censuses (inline in the session):
- `grep -c withBackend|withUserDataFolder|withKeepPageLoadedWhenBrowserIsHidden|NEEDS_WEBVIEW2|STATIC_LINKING|parameterGestureChanged|SafePointer` per editor;
- `areOptionsSupported` and the user-data-folder location;
- `String::formatted` with `%f`;
- `window.prompt/confirm/alert`;
- `pagehide`, `window.onerror`;
- relay construction style.

CI: `gh run list --workflow ui-static-gates.yml` (20 runs: 18 failure since 2026-09-28, 2 success before) and `gh run view 39454657864 --log-failed`.

Not done: no build, no DAW or hands-on test, no Windows run. The open defects marked latent, and items #2–#5, follow from code plus the JUCE source; none was reproduced in a host.
