# `scripts/ui-stub/` — the generic WebView bridge stub

Renders **any** of the 43 plugin pages headless. Before this, only five plugins
had a `tests/ui-stub/juce-stub.js`; the other 38 could not be rendered at all,
and a French-label overflow gate that cannot load the page is not a gate.

| File | What it is |
|---|---|
| `generic-juce-stub.js` | ES module substituted for `js/juce/index.js`. Exports the six names the real module exports. |
| `stub-preamble.js` | Classic script injected as the first child of `<head>`. Installs `window.__JUCE__` before any bundle evaluates. |

Both are driven by `scripts/serve-ui.js`. Nothing here is ever shipped or
embedded.

## Using it

```bash
node scripts/serve-ui.js --plugin O-Tapestop            # inspect the assembled tree
node scripts/serve-ui.js --plugin O-Tapestop --hold     # serve it until ^C
node scripts/boot-all-uis.js                            # render all 43 and report
```

As a module:

```js
const { buildRoot, serve, readEditorSize } = require('./scripts/serve-ui.js');
const built = buildRoot('O-Tapestop');
const { port, close } = await serve(built.root);
```

## What the served tree is, and why it is assembled rather than copied

The WebView never sees the source directory. It sees what
`PluginEditor::getResource()` returns, assembled from a `juce_add_binary_data`
`SOURCES` block that on **8 plugins** pulls files from outside the UI root
(`${CMAKE_SOURCE_DIR}/modules/...`). Copying only the UI root serves a 404 for
those, which presents as a missing panel and nothing else. So:

1. the UI root is copied byte-identical;
2. every `juce_add_binary_data` source gets its **BinaryData symbol** computed
   with JUCE's own mangling rule, that symbol is looked up in the
   `url == "..."` branches of `PluginEditor.cpp`, and the file is placed at
   **that** url;
3. the bridge stub is overlaid at `js/juce/index.js`.

Step 2 is what makes `/modules/preset-manager.js` and `/js/tuning-panel.js`
resolve on O-Bassoon, O-Bowed, O-Contrabass, O-Marimba, O-MicrotonalSampler,
O-Reed, O-ReverseDelay and O-Wind.

**The UI root is read, never guessed.** O-MicrotonalSampler and O-Orbit carry
*both* a `Source/ui/public` and a `Resources/ui`; only one is embedded and the
other is a staging leftover that renders a stale page. The root is derived from
the CMake `SOURCES` list — the same list the build embeds — and probe order is
only the fallback.

**Port 0, always.** A fixed port silently serves a concurrent session's files
(`pattern_ui_test_server_port_clash_serves_other_session`).

## A per-plugin stub always wins

If `plugins/<Name>/tests/ui-stub/juce-stub.js` exists it is used unchanged, and
the preamble is **not** injected. Those five plugins have committed gates that
pass against a tree without it; altering the page for a plugin whose gate is
already green is a regression waiting to be misattributed.

## What this stub does NOT do — read before trusting a result

**It does not reject unknown native-function names.** The five hand-written
stubs do, deliberately: an unlisted name is a bridge gap that would present as a
silently dead control in a DAW (`pattern_webview_native_fn_bridge_gap`), and
rejecting is what makes it visible. Each of those stubs is running a
set-equality census against *its own* `PluginEditor.cpp`.

This file is not running that census. An unknown name resolves a benign value
inferred from its name, and is recorded on `window.__stubUnknownNativeFns` so a
caller can **report** the gap. Reporting is not rejecting:

> **A result obtained through this stub can never discharge a bridge-parity
> claim.** It proves layout and text. It does not prove that a control is wired.

**It does not know any plugin's parameter ranges.** No
`createParameterLayout()` is parsed — a regex over that function provably
undercounts here (O-Prism: 0 usable IDs statically, 173 at runtime), so guessing
would be worse than admitting ignorance. Every slider gets a neutral `0..1`
range with its default at the **middle**, not the minimum: a page with every
knob pinned at minimum is a state the plugin never ships in, and several UIs
hide or grey a section whose depth reads zero — removing elements a label gate
is supposed to measure.

Combo boxes get four single-character choices (`1`–`4`). Non-empty because
`properties.choices[i]` and `.map(...)` appear unguarded in several `app.js`
files; single characters so the fixture's own text cannot dominate a geometry
measurement.

`window.__stubReport.rangesFrom` always states which mode was used. Neutral
fixtures can never be mistaken for measured ranges.

## Seeding real values

Two optional inputs, merged in this order (later wins):

**1. `plugins/<Name>/.planning/params.tsv`** — a `scripts/param-dump` TSV.
Supplies `start` / `end` / `def` / `numSteps` per parameter ID and marks
booleans as toggles. *No plugin has one in the tree today*; generating them is a
`-DOUARICON_BUILD_TESTS=ON` build step Stage E deliberately did not take. The
parser is exercised against a fixture rather than left as dead code.

**2. `plugins/<Name>/tests/ui-stub/generic-overrides.json`** — hand-written, for
what a TSV cannot supply:

```json
{
  "sliders": { "MIX": { "start": 0, "end": 100, "skew": 1, "def": 100 } },
  "toggles": { "ENGAGE": false },
  "combos":  { "MODE": { "choices": ["Stop", "Scratch"], "def": 0 } },
  "natives": { "getPresetListWithCategories": "{\"Factory\":[\"Default\"]}" }
}
```

`natives` is the escape hatch for a name whose **shape genuinely differs between
plugins**. There is one such name today: `getPresetListWithCategories` returns a
`juce::var` object on O-Bells (`PluginEditor.cpp:233-245`) and a **JSON string**
on O-Prism (`:809-821`). The stub defaults to the object, because a page that
`JSON.parse`s an object catches and degrades to an empty list, while a page that
`Object.keys` a string renders one bogus category per character. O-Prism carries
an override for its own shape.

A malformed override file is **reported**, never silently ignored — a typo'd
JSON that boots with neutral fixtures looks exactly like a correct one.

## Known findings this harness surfaced

**O-Bowed and O-Reed load an ES module as a classic script.** Both carry
`<script src="/js/juce/index.js"></script>` with no `type="module"`
(`O-Bowed/Resources/ui/index.html:800`, `O-Reed/Resources/ui/index.html:874`),
which throws `SyntaxError: Unexpected token 'export'` **in the real WebView too**
— the tag is not a harness artefact. The page still works, because the inline
`<script type="module">` below re-imports the same file correctly. Confirmed by
probe: deleting that one line takes both from 1 uncaught error to 0, and the
element count drops by exactly 1 (the removed tag). It is a one-line fix in each
plugin's markup, out of scope for the tooling stage that found it.
