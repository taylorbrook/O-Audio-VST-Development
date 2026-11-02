# Quick 260925-itj: R5, a shared EB Garamond font module, piloted on O-ReverseDelay — Research

**Researched:** 2026-09-25
**Domain:** Bundling a web font into JUCE 8.0.15 WebBrowserComponent UIs (WKWebView / WebView2) through `juce_add_binary_data` and the resource provider
**Confidence:** HIGH for the macOS/Chromium path (probed end to end in the scratchpad). MEDIUM for WKWebView and WebView2 at runtime (JUCE source was read, but no DAW run was done).

## Summary

`modules/ui/` does not exist today. R6 deleted it in `850c89c3`, but the registry kept the `ui` category (`modules/registry.yaml:39-40`). The new module is `modules/ui/eb-garamond/`. It is direct-embedded exactly the way O-Wind, O-Reed and O-Contrabass embed `tuning-panel.js`, and the way O-ReverseDelay embeds `preset-manager.js`: a `${CMAKE_SOURCE_DIR}/modules/...` path in the plugin's single `juce_add_binary_data` SOURCES block, plus an `if (url == "/…")` branch in `getResource()`.

JUCE serves whatever MIME string the provider returns, with no allow-list. The page and the font share one origin (`juce://juce.backend/` on macOS, `https://juce.backend/` on Windows), so fonts need no CORS header. `font/woff2` is all that is needed.

**Feasibility probe.** I ran this in the scratchpad against `serve-ui.buildRoot('O-ReverseDelay')`, with no repo edits. With the three woff2 files and the `@font-face` CSS placed at `/fonts/` and `/css/`, CDP `CSS.getPlatformFontsForNode` reports `EB Garamond(custom)` for every Latin run in en, fr and zh-Hans. Han glyphs still resolve to PingFang SC (`蘋方-簡`). There were 0 misses, and all three FontFaces reported `loaded`.

**The one real risk is vertical metrics.** EB Garamond's line box under `line-height: normal` is 1.305 em. Times New Roman's is 1.15 em. Out of the box, the `.title` grows 28 → 32 px and **208 of 278 elements move**.

Baking Times New Roman's vertical metrics into the subset fixes this. The values are asc 891 / desc 216 / lineGap 42 per 1000, with USE_TYPO_METRICS set. After that, **0 elements move vertically** in every language. The 33 that still register as moved have dy = 0; they are centred text that is now about 5% narrower. The `ascent-override` CSS descriptor is not an option, because Safari/WKWebView does not support it. The headless-Chromium gates would then measure a page that WKWebView never renders.

**Primary recommendation.** Ship three static woff2 files: Regular 400, Italic 400 and Bold 700. Subset them to Latin + Latin-Ext + the punctuation, arrows and fleurons the UI uses, and bake in Times-matched vertical metrics. Total is about 117 KB. Serve them at `/fonts/EBGaramond-{Regular,Italic,Bold}.woff2` from `/css/eb-garamond.css`, and change O-ReverseDelay's `--serif` token to `'EB Garamond', 'Georgia', 'Times New Roman', 'PingFang SC', 'Microsoft YaHei', serif`.

## Project Constraints (from CLAUDE.md + memory)

- **Trunk-based on `main`.** Path-scoped commits only (`git commit -- plugins/O-ReverseDelay modules/ui/eb-garamond modules/registry.yaml PLUGINS.md backups/O-ReverseDelay …`). Never `git add -A` or `-a`. Re-check `git branch --show-current` and `git status --short` right before each commit. O-Formant is currently dirty from another session, so do not touch it.
- **Build + install:** `./scripts/build-and-install.sh O-ReverseDelay`. It resolves the CMake target (`OuariconReverseDelay`) through `resolve_cmake_target()` (`scripts/build-and-install.sh:134,233`). Its Phase 4 does the AU cache clear and the `-dev`↔release variant sweep. Verify with a targeted `auval -v aufx ORvD OuDv`, not `auval -a`, which SIGABRTs here (memory note).
- **The Standalone stays stale** under build-and-install (memory `pattern_build_install_skips_standalone_stale_ui`). Rebuild `OuariconReverseDelay_Standalone` explicitly if the Standalone is used for the visual check.
- **Research docs go in `research/`**, not `docs/`. Nothing here needs one.
- **Do not tag**; tags are only for `/publish` (memory). Stop after the pilot, and do not migrate other plugins.

## Findings (codebase-grounded)

### 1. The direct-embed pattern
- **O-Wind precedent** `[VERIFIED: plugins/O-Wind/CMakeLists.txt:97-98]`: `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/js/tuning-panel.js` and `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/snippets/tuning-panel.css`, inside `juce_add_binary_data(O-Wind_UIResources SOURCES …)`.
- **O-ReverseDelay** `[VERIFIED: plugins/O-ReverseDelay/CMakeLists.txt]`: `juce_add_binary_data(OuariconReverseDelay_UIResources NAMESPACE UIBinaryData HEADER_NAME UIBinaryData.h SOURCES … ${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/js/preset-manager.js)`. There is exactly ONE binary-data target, and `ui_frontend_check.js` §9 asserts it (`tests/ui_frontend_check.js:444-446`). Add the font files to this same block and never create a second target.
- **Symbol mangling** `[VERIFIED: ~/JUCE/extras/Build/juce_build_tools/utils/juce_BuildHelperFunctions.cpp:227-232]`: `file.getFileName().replaceCharacters (" .", "__").retainCharacters ("ABC…xyz_0123456789")`, then `makeValidIdentifier`. Case is kept and hyphens are dropped. The resulting symbols are `EBGaramond-Regular.woff2` → `UIBinaryData::EBGaramondRegular_woff2`, `eb-garamond.css` → `UIBinaryData::ebgaramond_css`, `EBGaramond-Italic.woff2` → `EBGaramondItalic_woff2`, and `EBGaramond-Bold.woff2` → `EBGaramondBold_woff2`.
- **Duplicate basenames** silently get a `2` suffix (`juce_BinaryResourceFile.cpp:50-57`). None of the new names collide with existing O-ReverseDelay symbols (`index_html`, `styles_css`, `app_js`, `i18n_js`, `presetmanager_js`, `index_js`, `check_native_interop_js`, `birds_png`).
- **The provider is a bare-path equality ladder** `[VERIFIED: plugins/O-ReverseDelay/Source/PluginEditor.cpp:141-149, 179-228]`. `makeBinaryResource (data, size, const char* mimeType)` sets the MIME per branch, and there is **no extension→MIME map**. An unknown URL returns `std::nullopt` (404). Add explicit branches. Keep the literal `if (url == "…")` + `UIBinaryData::<sym>` form within about 400 chars: `serve-ui.js` `urlSymbolMap()` pairs URL → symbol by regex over exactly that shape (`scripts/serve-ui.js:235-259`), and a table or loop would break the harness placement.
- **There is no shared resource-provider helper in `modules/`.** `core/webview-relay-manager` is relays only, and its `used_by` is `[]`.

### 2. How WebView font loading works (JUCE 8.0.15 source)
- **macOS** `[VERIFIED: ~/JUCE/modules/juce_gui_extra/native/juce_WebBrowserComponent_mac.mm:519-557, 876-878]`: a WKURLSchemeHandler registered for `juce` passes `[url path]` (decoded, no scheme or host) to the provider. The response headers are `Content-Length` and `Content-Type: resource->mimeType`, plus `Access-Control-Allow-Origin` only if `withAllowedOrigin` is set. Because the root document is itself loaded from `juce://juce.backend/`, font requests are same-scheme and same-origin, so the known WKWebView "custom-scheme font blocked" problem (mixed content from an https page) does not apply. `[CITED: developer.apple.com/forums/thread/789922]`
- **Windows** `[VERIFIED: …/juce_WebBrowserComponent_windows.cpp:841, 884-908]`: `AddWebResourceRequestedFilter(L"*", CONTEXT_ALL)` covers fonts. The provider receives the URI text after `https://juce.backend`, raw and not percent-decoded, so keep filenames free of spaces and other escapable characters. The `Content-Type` header comes from the provider. `withUserDataFolder` is already set (`PluginEditor.cpp:650-660`). Nothing font-specific differs on Windows.
- **Release WKWebView is not inspectable.** `developerExtrasEnabled` is set only under `JUCE_DEBUG` (`mac.mm:880-882`), so proof inside the WKWebView is visual (Standalone/DAW), and the CDP proof runs on headless Chromium.
- **Relative URLs** in `@font-face src` resolve against the stylesheet URL. With `/css/eb-garamond.css`, `url('../fonts/X.woff2')` → `/fonts/X.woff2`. This holds in all three servers: WKWebView, WebView2 and `serve-ui` (which places each embed at its provider URL, `serve-ui.js:389-414`).

### 3. Font source, the build recipe and licence
- **Source** `[VERIFIED: github.com/google/fonts ofl/ebgaramond, last commit f8c1d3d6cc75e30d77130bdcbfbff27e3b6233fe, 2026-06-17 "bump version to 1.003"]`: `EBGaramond[wght].ttf` (851,176 B, sha256 `ef9512f9…9027`), `EBGaramond-Italic[wght].ttf` (754,468 B, `bba2c449…3c4e`) and `OFL.txt` (`09850666…7dd7`). The `wght` axis runs 400–800. Upstream is `octaviopardo/EBGaramond12`. Font v1.003, UPM 1000, `fsType 0` (installable).
- **Licence** `[VERIFIED: OFL.txt line 1]`: "Copyright 2017 The EB Garamond Project Authors (https://github.com/octaviopardo/EBGaramond12)". There is **no "with Reserved Font Name" clause, so there is no RFN**. A subset, metric-baked "Modified Version" may keep the name "EB Garamond". OFL §2 lets it be bundled with any software "provided that each copy contains the above copyright notice and this license … as stand-alone text files, human-readable headers or in the appropriate machine-readable metadata fields". Ship `OFL.txt` in the module, **and** put the copyright line plus the full OFL text in a comment in `eb-garamond.css`, because that file is what ships inside the binary. Keep name IDs 0, 13 and 14 in the subsets.
- **Tools are available** `[VERIFIED: local probe]`: Python 3.14.2, fontTools 4.61.1 (`pyftsubset`, `fonttools varLib.instancer`), `brotli` importable. There is nothing to install.
- **Sizes measured** in the scratchpad (subset, woff2):

  | Option | Files | Total |
  |---|---|---|
  | **Static 400 / 400i / 700 (recommended)** | 37,628 + 38,824 + 41,140 | **~117 KB** |
  | Variable wght 400–800, roman + italic (subset) | 72,308 + 74,928 | ~147 KB |
  | Variable, unsubset | 290,908 + 278,004 | ~569 KB |

- **Why static 700 is the one bold.** The task caps it at one bold. O-ReverseDelay declares `font-weight: 700` (`styles.css:616, 723`) and `600` (`:1567`). CSS matching maps 600 → 700 when only 400 and 700 exist, which is exactly what Times gives today (Regular/Bold only). Suite-wide, `bold` + 700 = 217 declarations and 600 = 153. No bold-italic is used in O-ReverseDelay, so a request for one would synthesise; that is acceptable.
- **Glyph coverage** `[VERIFIED: census of index.html + i18n.js + app.js + styles.css against the cmap]`: every non-CJK character the UI uses is present, including the French accents, `– — ’ · × − ≥ → ↔ ▶ ◀ ❦` and the hair space, **except `▾` U+25BE and `⚙` U+2699**. Those already fall back per glyph today, because Times lacks them too. Default figures are lining and equal-width (`one`/`zero` advance 480), so the `tabular-nums` rule at `styles.css:1114` keeps working when `tnum` is retained.
- **Build recipe.** Commit it as `modules/ui/eb-garamond/tools/build-fonts.sh` so the binaries are reproducible:
  ```bash
  SHA=f8c1d3d6cc75e30d77130bdcbfbff27e3b6233fe
  BASE="https://raw.githubusercontent.com/google/fonts/${SHA}/ofl/ebgaramond"
  curl -sLo VF.ttf  "${BASE}/EBGaramond%5Bwght%5D.ttf"
  curl -sLo VFi.ttf "${BASE}/EBGaramond-Italic%5Bwght%5D.ttf"
  UNI="U+0000-00FF,U+0100-017F,U+0192,U+0218-021B,U+02BB-02BC,U+02C6-02DD,U+0300-0308,U+0327,U+0329,U+1E9E,U+2000-206F,U+20AC,U+2116,U+2122,U+2190-2199,U+2212,U+2215,U+2260,U+2264-2265,U+25B2-25C0,U+2619,U+2766-2767,U+FB00-FB06,U+FEFF,U+FFFD"
  FEAT='kern,liga,mark,locl,lnum,tnum,onum,pnum,smcp,c2sc,case'
  fonttools varLib.instancer VF.ttf  wght=400 --update-name-table -o R.ttf
  fonttools varLib.instancer VF.ttf  wght=700 --update-name-table -o B.ttf
  fonttools varLib.instancer VFi.ttf wght=400 --update-name-table -o I.ttf
  for n in R B I; do pyftsubset $n.ttf --unicodes="$UNI" --layout-features="$FEAT" \
      --name-IDs='0,1,2,3,4,5,6,13,14' --flavor=woff2 --output-file=$n.woff2; done
  # then bake Times-matched vertical metrics (python, fontTools), per file:
  #   hhea.ascent=891 hhea.descent=-216 hhea.lineGap=42
  #   OS/2 sTypoAscender=891 sTypoDescender=-216 sTypoLineGap=42, fsSelection |= 1<<7 (USE_TYPO_METRICS)
  #   OS/2 usWinAscent=max(933, head.yMax) usWinDescent=max(216, -head.yMin)   # win = clip box, keep ink
  #   save flavor='woff2' as EBGaramond-{Regular,Bold,Italic}.woff2
  ```
  Write it with `${SHA}` braces, never `$SHA:`, because of the zsh `:P` modifier trap (memory note). Keep the downloaded TTFs out of the repo.
- **Why these metrics.** Times New Roman (macOS) has hhea 1825/-443/87 at 2048 UPM, which is 0.891/0.216/0.042 = 1.15 em. EB Garamond natively has 1007/-298/0 = 1.305 em. Georgia has 1878/-449/0 at 2048 = 1.136. In the probe, native metrics moved 208/278 elements, a 4 px shift below the title. Ratio-preserving 887/263 still moved 160 by 1 px. **Times-exact 891/216/42 moved 0 vertically** in en, fr and zh-Hans. The ink cost is that `g` descends to -290, 0.074 em below the 216 descent: about 0.5 px at 10 px, and visible only inside tight `overflow: hidden` boxes. O-ReverseDelay has 2 ellipsis and 4 `overflow: hidden` rules, so check them visually.
- **Widths** (sample string advance, em): EBG Regular 38.08, Times Regular 40.05, EBG Bold 41.29, Times Bold 42.05, Georgia 42.68. EB Garamond is **~5% narrower**, so labels shrink and no new overflow is expected. EB Garamond's x-height is 0.40 against Times' 0.447, so text reads slightly smaller. That is a visual and legibility check, not a gate failure.

### 4. CSS
- **O-ReverseDelay has one font token** `[VERIFIED: plugins/O-ReverseDelay/Source/ui/public/css/styles.css:224-233]`: `--serif: 'Garamond', 'EB Garamond', 'Times New Roman', 'PingFang SC', 'Microsoft YaHei', serif;`. The comment says "all ten font-family declarations in this file read var(--serif)". There is no `:lang()` override and no canvas text in `app.js`. `index.html:26` is the only stylesheet link: `<link rel="stylesheet" href="css/styles.css" />`.
- **Edit to line 233:** `--serif: 'EB Garamond', 'Georgia', 'Times New Roman', 'PingFang SC', 'Microsoft YaHei', serif;`. Drop `'Garamond'`. Left first, it would let an Office-installed Windows Garamond beat the bundled face, which defeats Windows parity. Update the v1.12.0 comment above it; its claim "Times New Roman is named here and IS installed" becomes history.
- **Order matters.** The CJK tail stays before the generic `serif`, per the existing comment. The probe confirmed Han → PingFang with the new stack.
- **Add the `<link>` before styles.css:** `<link rel="stylesheet" href="css/eb-garamond.css" />`.
- **`@font-face` snippet** (`modules/ui/eb-garamond/css/eb-garamond.css`):
  ```css
  @font-face { font-family: 'EB Garamond'; src: url('../fonts/EBGaramond-Regular.woff2') format('woff2');
               font-weight: 400; font-style: normal; font-display: block; }
  @font-face { font-family: 'EB Garamond'; src: url('../fonts/EBGaramond-Italic.woff2') format('woff2');
               font-weight: 400; font-style: italic; font-display: block; }
  @font-face { font-family: 'EB Garamond'; src: url('../fonts/EBGaramond-Bold.woff2') format('woff2');
               font-weight: 700; font-style: normal; font-display: block; }
  ```
  - **`font-display: block`.** The bytes come from memory in milliseconds, so `block` avoids a Times → Garamond flash on every editor open. Its failure mode is loud: text stays invisible for about 3 s, and a 404 is caught by the gates.
  - **No `local()`.** A user-installed EB Garamond with native metrics would undo the baked metrics.
  - **No `<link rel=preload>`.** It needs `crossorigin` and risks a double fetch on a custom scheme.
  - **No `unicode-range`.** The subset cmap already bounds fallback.
  - An `@font-face` family shadows any installed family of the same name. `[CITED: w3.org/TR/css-fonts-4 font-family matching]`

### 5. Verification tooling
- **`node scripts/check-ui-labels.js --plugin O-ReverseDelay`.** There is no `--lang` flag. It reads `LANGUAGES` from `js/i18n.js` and sweeps en, fr and zh-Hans in one run, with states from `tests/i18n-states.json`. It serves through `serve-ui` (port 0) and asserts "every requested resource was served" (404 detection, `check-ui-labels.js:1134`). **Baseline run this session: `ALL CHECKS PASSED`, exit 0**, with 7 `<option>`s reported as never measured (a coverage note, not a failure).
- **Font timing.** `check-ui-labels` waits on `waitUntil: 'load'` (`:554`) and `measure-ui` on `networkidle` (`:538`). Neither awaits `document.fonts.ready`. With `font-display: block`, measurements taken before the font arrives would use invisible fallback metrics. The resolved-face probe below must `await page.evaluate(() => document.fonts.ready)`, and it also proves which face the gates measured.
- **`measure-ui.js` does NOT report the resolved face.** Its `ff` field is the declared stack (`measure-ui.js:454`; memory `pattern_declared_font_stack_is_not_the_resolved_face`). The CDP precedent is `plugins/O-Strata/tests/tools/cdp-font-probe.js`: `serve-ui` buildRoot/serve, `readEditorSize`, `window.__setLanguage`, then `DOM.getDocument` / `DOM.querySelector` / `CSS.getPlatformFontsForNode`. For the pilot, add `plugins/O-ReverseDelay/tests/tools/cdp-font-probe.js`, copied from it, with:
  - targets `.title`, `.title-accent`, `.subtitle`, `.group-label`, `.preset-name`, `.tooltip-title`, `.footer-text`;
  - a pass condition that every Latin run is `EB Garamond` with `isCustomFont`, and that zh-Hans Han runs are on PingFang (match the `PingFangSC` PostScript prefix, because the family name is localised as `蘋方-簡`).

  The scratchpad probe (`…/scratchpad/ebg/probe.js`) already does this and adds a before/after rect diff.
- **Plugin-local gates.** Baselines run this session: `ui_frontend_check.js` passed 183 checks with exit 0, and `ui_tooltip_clamp_check.js` passed 155 with exit 0 and `WINDOW budget … 212 into body 212 — 0 px spare`.
- **Three hand-built served trees will silently 404 the font.** Each must be updated in the pilot:
  1. `tests/ui_tooltip_clamp_check.js:171-182` `buildRoot()` hand-copies only `preset-manager.js`, and its MIME map (`:154-159`) has no `.woff2`. Copy `css/eb-garamond.css` and `fonts/*.woff2` there, and add `'.woff2': 'font/woff2'`.
  2. `tests/ui-stub/serve-stub.sh:16-25` needs the same `cp` lines.
  3. `tests/ui_frontend_check.js` §9 (`:380-451`):
     - The `refs` regex only knows `(js|css|img)/`, so `css/eb-garamond.css` will be required in the provider, which is good.
     - The `embeddedFiles` map only recognises `modules/…/js/*.js` (`:420-421`). The new `/css/eb-garamond.css` and `/fonts/*.woff2` provider entries would therefore FAIL "every getResource() path is in the juce_add_binary_data SOURCES list". Extend the map to `\$\{CMAKE_SOURCE_DIR\}/(modules/\S*/(css|fonts)/([^/\s]+))` → `/$2/$3`.
     - Optionally add `url('../fonts/…')` refs parsed from the served CSS, and assert `font/woff2` on the font branches.
- **`serve-ui.js` needs no change.** Its MIME map already has `'.woff2': 'font/woff2'` (`:494`), and placement is automatic given the ladder form.

### 6. The /improve-style bump, v1.21.1 → v1.22.0 (MINOR: a new bundled face)
- **Working tree state.** v1.21.1 is **now committed** (`18b358ed`, 2026-09-25 13:34), and `git status -- plugins/O-ReverseDelay PLUGINS.md` is clean, so the pilot builds on a committed base. `PLUGINS.md:68` reads `| O-ReverseDelay | 📦 Installed | 1.21.1 | … | 2026-09-25 |`. The latest backup is `backups/O-ReverseDelay/v1.21.0/`.
- **Touches:**
  1. `backups/O-ReverseDelay/v1.21.1/` (Phase 0.9: rsync excluding `build/`, then `scripts/verify-backup.sh O-ReverseDelay 1.21.1`).
  2. `CMakeLists.txt` `VERSION 1.21.1` → `1.22.0`, plus the SOURCES entries.
  3. `Source/PluginEditor.cpp`: four provider branches.
  4. `index.html` `<link>`.
  5. `styles.css` `--serif` + comment.
  6. The three test trees and §9 above, plus the new CDP probe.
  7. A `CHANGELOG.md` `## [1.22.0] — <date>` entry.
  8. `NOTES.md` (`- **Version:** 1.22.0` at line 5 + a timeline entry).
  9. The `PLUGINS.md` row (version/date).
  10. `.planning/STATUS.md` only if `/improve` is run properly; its frontmatter does not track the current version (`ship_ready_version: 1.1.0`).
- **Presets.** The VERSION bump also re-seeds the `.factory-version` factory presets (`NOTES.md:81`). This is harmless, and no preset content changes.
- **Module side:**
  - `modules/ui/eb-garamond/{module.yaml, README.md, OFL.txt, css/eb-garamond.css, fonts/EBGaramond-{Regular,Italic,Bold}.woff2, snippets/getResource.cpp, tools/build-fonts.sh}`.
  - A registry entry under category `ui` (`version: 1.0.0`, `used_by: [O-ReverseDelay]`), with the registry header bumped `1.1.0 → 1.2.0` and `last_updated` changed.
  - **Pitfall:** `scripts/regen-registry-used-by.sh` derives `used_by` only from `*.h/*.cpp/*.js` basenames under `cpp/` and `js/` (`:62-71`). Running it would rewrite this module's `used_by` to `[]`. Extend `module_tokens()` to also walk `css/` and `fonts/` for `.css/.woff2`, or do not run it.

### 7. Windows parity (flag only)
- The provider MIME is identical. `NEEDS_WEBVIEW2` and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` are already set. Stock Windows has Georgia and Times but no Garamond, so after this edit Windows renders the bundled face.
- USE_TYPO_METRICS is set, so DirectWrite uses the baked typo metrics and line boxes should match macOS. This is `[ASSUMED]` until a Windows CI build is looked at.
- `windows.cpp:884` passes the raw URI tail, so no spaces are allowed in font filenames. None are planned.

## Recommended concrete layout + edits

```
modules/ui/eb-garamond/
├── module.yaml            # name: eb-garamond, version 1.0.0, category: ui, provides: css + 3 woff2 + served URLs
├── README.md              # the 4 integration steps below + metric-bake rationale + OFL note
├── OFL.txt                # verbatim from google/fonts@f8c1d3d6
├── css/eb-garamond.css    # AGPL header for the CSS + EB Garamond copyright + full OFL text comment + 3 @font-face
├── fonts/EBGaramond-Regular.woff2   (~37.6 KB)
├── fonts/EBGaramond-Italic.woff2    (~38.8 KB)
├── fonts/EBGaramond-Bold.woff2      (~41.1 KB)
├── snippets/getResource.cpp          # the 4 provider branches below
└── tools/build-fonts.sh              # pinned-SHA download → instancer → subset → metric bake
```

**CMake** (append to the existing `OuariconReverseDelay_UIResources` SOURCES; do not add a second target):
```cmake
        # v1.22.0 — shared EB Garamond face (modules/ui/eb-garamond). Served at
        # /css/eb-garamond.css and /fonts/*.woff2 so the CSS's url('../fonts/…')
        # resolves. Symbols (hyphens STRIPPED): ebgaramond_css,
        # EBGaramondRegular_woff2, EBGaramondItalic_woff2, EBGaramondBold_woff2.
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/css/eb-garamond.css
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Regular.woff2
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Italic.woff2
        ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Bold.woff2
```

**Provider** (`PluginEditor.cpp`, before `return std::nullopt;`):
```cpp
    if (url == "/css/eb-garamond.css")
        return makeBinaryResource (UIBinaryData::ebgaramond_css, UIBinaryData::ebgaramond_cssSize,
                                   "text/css; charset=utf-8");
    if (url == "/fonts/EBGaramond-Regular.woff2")
        return makeBinaryResource (UIBinaryData::EBGaramondRegular_woff2,
                                   UIBinaryData::EBGaramondRegular_woff2Size, "font/woff2");
    if (url == "/fonts/EBGaramond-Italic.woff2")
        return makeBinaryResource (UIBinaryData::EBGaramondItalic_woff2,
                                   UIBinaryData::EBGaramondItalic_woff2Size, "font/woff2");
    if (url == "/fonts/EBGaramond-Bold.woff2")
        return makeBinaryResource (UIBinaryData::EBGaramondBold_woff2,
                                   UIBinaryData::EBGaramondBold_woff2Size, "font/woff2");
```

**HTML:** `index.html:26`, insert before it: `<link rel="stylesheet" href="css/eb-garamond.css" />`.

**CSS:** `styles.css:233` becomes `--serif: 'EB Garamond', 'Georgia', 'Times New Roman', 'PingFang SC', 'Microsoft YaHei', serif;`.

**Verification sequence** (before = run now on v1.21.1 and save the output; after = on v1.22.0):
1. `node scripts/check-ui-labels.js --plugin O-ReverseDelay` must exit 0 before and after, including "every requested resource was served".
2. `node plugins/O-ReverseDelay/tests/ui_frontend_check.js` (after the §9 extension) and `node plugins/O-ReverseDelay/tests/ui_tooltip_clamp_check.js` (after the tree and MIME fix) must both exit 0. The WINDOW budget must stay at `212 into body 212`.
3. `node plugins/O-ReverseDelay/tests/tools/cdp-font-probe.js --all-languages` must report EB Garamond (custom) on all Latin runs and PingFang on Han. Also run a rect diff before/after: the expectation is 0 vertical moves.
4. `node scripts/check-i18n.js`, `node scripts/i18n-fr-lint.js` and `node scripts/i18n-zh-lint.js` are regression-only; no copy changes.
5. `./scripts/build-and-install.sh O-ReverseDelay`, then `auval -v aufx ORvD OuDv`, then pluginval if customary.
6. Visual check in the Standalone (rebuilt) or a DAW. This is the only WKWebView proof. The italic "Delay" title accent and the fleurons `❦` (now EB Garamond's own, previously a system fallback) should visibly change face. Also check descenders inside the 2 ellipsis boxes.

## Common Pitfalls
1. **Native EB Garamond metrics shift the page.** They moved 208/278 elements (+4 px under the title) in the probe, and the WINDOW panel has 0 px spare. Bake Times metrics into the font. Do not use CSS `ascent-override`: `[CITED: caniuse.com/mdn-css_at-rules_font-face_ascent-override]` shows Safari stable has no support, so Chromium gates and WKWebView would diverge.
2. **Hand-built test trees 404 the font silently.** In the tooltip-clamp gate and serve-stub.sh, a 404 on a font or CSS falls back to Times, and the gate passes against the wrong face. Update them, and make the CDP probe the arbiter.
3. **`ui_frontend_check` §9 FAILs on the new module paths** until its `embeddedFiles` regex learns `css/` and `fonts/`.
4. **Symbol names.** Writing `EBGaramond_Regular_woff2` is a compile error; hyphens are stripped, not converted to underscores.
5. **Leaving `'Garamond'` ahead of `'EB Garamond'`** lets Windows machines with Office use a different Garamond.
6. **Registry regen wipes `used_by`** for a module without `cpp/` or `js/` files.
7. **`font-display: block` plus a gate that measures before `fonts.ready`** gives invisible-text metrics. Probe on `document.fonts.ready`.

## Assumptions Log
| # | Claim | Section | Risk if wrong |
|---|---|---|---|
| A1 | WKWebView applies the woff2 served by the `juce://` scheme handler with `font/woff2` (same origin, no ACAO) | 2 | Face does not render in the DAW; caught by the visual check in step 6 |
| A2 | WebView2/DirectWrite uses the baked typo metrics (USE_TYPO_METRICS), so Windows line boxes match macOS | 7 | Minor vertical drift on Windows only |
| A3 | WKWebView (CoreText hhea) computes the same line box as headless Chromium on macOS for the baked font | 3 | Gates pass on Chromium while the WKWebView layout differs by about 1 px; visual check |
| A4 | The copyright line plus full OFL text in the embedded CSS comment, together with name IDs 0/13/14, satisfies OFL §2 for a binary-embedded font | 3 | Licence-notice gap; low risk, and adding OFL to installer notices is cheap |

## Security Domain
There is no new input surface. The provider stays an exact-match allow-list, with no path joining and no traversal, and the assets are static and embedded at build time. Supply chain: fonts come from a pinned google/fonts commit with recorded sha256 and are rebuilt by a committed script. No npm or pip packages are installed; fontTools and brotli are already present locally. ASVS V5 and V12 are not applicable beyond this.

## Sources
- **Primary:**
  - JUCE 8.0.15 source at `~/JUCE`: `juce_WebBrowserComponent_mac.mm`, `juce_WebBrowserComponent_windows.cpp`, `juce_BuildHelperFunctions.cpp`, `juce_BinaryResourceFile.cpp`.
  - `github.com/google/fonts/tree/f8c1d3d6…/ofl/ebgaramond` (METADATA.pb, OFL.txt, VF TTFs).
  - The scratchpad probe (`…/scratchpad/ebg/probe.js`, `bake2.py`).
- **Secondary:** [Apple forums: WKURLSchemeHandler fonts on https pages](https://developer.apple.com/forums/thread/789922); [caniuse: ascent-override](https://caniuse.com/mdn-css_at-rules_font-face_ascent-override).
