# eb-garamond

Bundled **EB Garamond** for Ouaricon WebView UIs. It ships static Regular 400, Italic 400 and Bold 700 woff2 files (37.7 + 38.9 + 41.3 KB, about 117 KB in total), subset to Latin + Latin-Ext plus the punctuation, arrows and fleurons the UIs use. Times New Roman's vertical metrics are baked in, so a plugin that moves from Times to this face keeps every line box where it was.

- **Font licence:** SIL Open Font License 1.1. Copyright 2017 The EB Garamond Project Authors.
- **Code licence:** the stylesheet, snippet and build script are AGPL-3.0-or-later.
- **First consumer:** O-ReverseDelay v1.22.0 (the R5 pilot).

```
eb-garamond/
├── module.yaml
├── OFL.txt                          verbatim from google/fonts@f8c1d3d6
├── css/eb-garamond.css              3 @font-face rules + AGPL header + full OFL text
├── fonts/EBGaramond-Regular.woff2   400 normal
├── fonts/EBGaramond-Italic.woff2    400 italic
├── fonts/EBGaramond-Bold.woff2      700 normal
├── snippets/getResource.cpp         4 provider branches (copy-paste, not compiled)
└── tools/build-fonts.sh             pinned, sha256-checked, reproducible rebuild
```

## Integration (direct embed)

The module is embedded the same way `preset-manager.js` and `tuning-panel.js` reach their plugins. There is no CMake helper.

1. **Append to the plugin's EXISTING single UI binary-data target.** Never add a second `juce_add_binary_data` target: one that defaults to `NAMESPACE BinaryData` duplicate-symbols at link time.
   ```cmake
       ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/css/eb-garamond.css
       ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Regular.woff2
       ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Italic.woff2
       ${CMAKE_SOURCE_DIR}/modules/ui/eb-garamond/fonts/EBGaramond-Bold.woff2
   ```
2. **Paste the four branches** from `snippets/getResource.cpp` into `getResource()`, before its final `return std::nullopt;`. The stylesheet is served at `/css/eb-garamond.css` and the fonts at `/fonts/*.woff2`, because the stylesheet's `url('../fonts/…')` resolves against `/css/`. MIME types are `text/css; charset=utf-8` and `font/woff2`.
3. **Link the stylesheet before the plugin's own stylesheet:**
   ```html
   <link rel="stylesheet" href="css/eb-garamond.css" />
   <link rel="stylesheet" href="css/styles.css" />
   ```
4. **Put `'EB Garamond'` first in the plugin's font token.** Georgia goes before Times, and the CJK tail goes before the generic `serif`:
   ```css
   --serif: 'EB Garamond', 'Georgia', 'Times New Roman', 'PingFang SC', 'Microsoft YaHei', serif;
   ```
   Drop any bare `'Garamond'` entry. Left ahead of the bundled face, it lets an Office-installed Windows Garamond win.

### Symbol names

`juce_add_binary_data` **strips** hyphens. It does not convert them to underscores.

| File | Symbol |
|---|---|
| `eb-garamond.css` | `ebgaramond_css` |
| `EBGaramond-Regular.woff2` | `EBGaramondRegular_woff2` |
| `EBGaramond-Italic.woff2` | `EBGaramondItalic_woff2` |
| `EBGaramond-Bold.woff2` | `EBGaramondBold_woff2` |

`EBGaramond_Regular_woff2` is a compile error.

## Why the Times metrics are baked in

- **The problem.** EB Garamond's native line box under `line-height: normal` is 1.305 em (1007 / -298 / 0). Times New Roman's is 1.15 em (1825 / -443 / 87 at 2048 UPM, which is 891 / -216 / 42 per 1000). On O-ReverseDelay the native face moved **208 of 278 elements**, including a 4 px shift below the title. Its WINDOW panel has 0 px to spare.
- **The fix.** The build writes 891 / -216 / 42 into hhea and the OS/2 typo fields and sets USE_TYPO_METRICS (fsSelection bit 7). With that, **0 elements move vertically** in en, fr and zh-Hans.
- **Why not CSS.** The `ascent-override` descriptor has no WKWebView/Safari support. The headless-Chromium gates would then measure a page the macOS WebView never renders.
- **The ink cost.** The `g` descender reaches -290, 0.074 em below the 216 descent. That is about 0.5 px at 10 px, and it shows only inside tight `overflow: hidden` boxes. Look at the ellipsis boxes when you adopt the font.
- **Windows clip box.** usWinAscent / usWinDescent stay at max(933, yMax) / max(216, -yMin), so Windows never clips ink.

The face is about 5% narrower than Times and has a smaller x-height (0.40 against 0.447). Labels shrink, so no new overflow is expected, but text reads slightly smaller.

## Stylesheet choices

- **`font-display: block`.** The bytes come from the plugin binary in milliseconds, so `block` avoids a Times → Garamond flash on every editor open. Its failure mode is loud (invisible text for about 3 s), and the gates catch the 404 that causes it.
- **No `local()` source.** A user-installed EB Garamond with native metrics would outrank these files and undo the baked metrics.
- **No `<link rel=preload>`.** It needs `crossorigin` and risks a double fetch on a custom scheme.
- **No `unicode-range`.** The subset's cmap already bounds per-glyph fallback.

## Pitfalls when adopting

1. **Hand-built served trees 404 the font silently.** A test that copies `Source/ui/public` into a temp root must also copy `css/eb-garamond.css` and `fonts/*.woff2`. Otherwise the page falls back to Times and every layout gate passes against the wrong face. O-ReverseDelay's `ui_tooltip_clamp_check.js` and `tests/ui-stub/serve-stub.sh` show the pattern.
2. **MIME maps need `'.woff2': 'font/woff2'`.** `scripts/serve-ui.js` already has it; hand-rolled servers often do not.
3. **§9-style closure gates must map `css/` and `fonts/` module paths** to their served paths. O-ReverseDelay's `ui_frontend_check.js` §9 also resolves every `url(...)` in embedded stylesheets and asserts `font/woff2` on `.woff2` branches.
4. **Wait for `document.fonts.ready` before measuring.** With `font-display: block`, a gate that measures at `load` can see invisible fallback metrics.
5. **Check the resolved face, not the declared stack.** `measure-ui.js`'s `ff` is the declared stack. Use CDP `CSS.getPlatformFontsForNode`, as in `plugins/O-ReverseDelay/tests/tools/cdp-font-probe.js`.
6. **serve-ui CMake tokenising.** `scripts/serve-ui.js` strips `#` comment lines, then treats every whitespace token in the `SOURCES` block that ends in a file extension as a source, with a balanced-paren scan. Keep comments inside the block free of filenames and unbalanced parentheses, so the rule holds even for tools that do not strip comments.
7. **Keep the literal `if (url == "…") return makeBinaryResource (UIBinaryData::…)` shape.** serve-ui pairs each URL with the first symbol within about 400 characters.

## Glyph gaps

The subset has no **U+25BE ▾** or **U+2699 ⚙**. They fall back per glyph to a system face, exactly as they did under Times, which lacks them too.

## Windows parity

**ASSUMED, not yet observed.** The provider MIME and the same-origin serving are identical on WebView2, and USE_TYPO_METRICS should make DirectWrite use the baked typo metrics. Nobody has looked at a Windows build yet.

## Rebuilding the fonts

```bash
bash modules/ui/eb-garamond/tools/build-fonts.sh
```

- **Inputs.** The script downloads `EBGaramond[wght].ttf`, `EBGaramond-Italic[wght].ttf` and `OFL.txt` from google/fonts at commit `f8c1d3d6cc75e30d77130bdcbfbff27e3b6233fe`. It checks all three against pinned sha256 values and aborts on a mismatch.
- **Steps.** It instances 400 / 700 / 400 italic with the fontTools instancer, subsets with `pyftsubset` (keeping name IDs 0-6, 13 and 14), bakes the metrics, and writes `fonts/` and `OFL.txt`.
- **Reproducibility.** `SOURCE_DATE_EPOCH` is pinned, so two runs are byte-identical. Compare against the output hashes in `module.yaml`.
- **Requirements.** curl, shasum, python3 with fontTools and brotli. The script installs nothing, and the downloaded TTFs never land in the repo.

## OFL obligations

- **What these files are.** They are an OFL "Modified Version" (subset, static instances, re-baked metrics). The licence declares **no Reserved Font Name**, so the family name "EB Garamond" may be kept.
- **The notice must travel with every copy.** `OFL.txt` sits in this module. The copyright line and the full OFL text also sit in a comment in `css/eb-garamond.css`, which is the file embedded in each plugin binary. The subset keeps name IDs 0 (copyright), 13 (licence) and 14 (licence URL).
- **No selling the fonts on their own.** They may be bundled with software, which is how they are used here.
- If an installer gains a third-party notices page, list EB Garamond / OFL-1.1 there too.
