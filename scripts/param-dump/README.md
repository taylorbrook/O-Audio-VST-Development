# param-dump — runtime parameter inventory

A ~40-line console app that constructs a plugin processor through the standard
JUCE factory `createPluginFilter()`, walks `AudioProcessor::getParameters()`,
and prints a TSV inventory to stdout.

## Why this exists

Static parsing of `createParameterLayout()` provably fails in this repo:

| Plugin | Regex over the source finds | Runtime walk finds |
|---|---|---|
| O-Emulator | 2 | 5 — four come from a local `percent()` factory lambda (`Source/PluginProcessor.cpp:61-74`) |
| O-Prism | 0 usable IDs | **173** — the IDs are string-concatenated (`Source/PluginProcessor.cpp:76-87`) and never appear as literals. The planning estimate of 81 was itself a regex undercount; the runtime walk is why the real number is knowable. |

Only a runtime walk sees the real set. The dump is the machine-generated
skeleton for authoring tooltip copy; the prose is hand-written.

## Wiring a plugin

Three lines in `plugins/<Name>/CMakeLists.txt`, gated by the suite-wide
`OUARICON_BUILD_TESTS` cache option so a normal build is unaffected:

```cmake
option(OUARICON_BUILD_TESTS "Build O-Foo test targets" OFF)

if(OUARICON_BUILD_TESTS)
    include(${CMAKE_SOURCE_DIR}/scripts/param-dump/ParamDump.cmake)
    ouaricon_add_param_dump(OFoo ${CMAKE_CURRENT_SOURCE_DIR}/Source)
endif()
```

The first argument is the **CMake target** (`OFoo`), which is often not the
folder name. The generated console target is named after the **folder**, so
`plugins/O-Emulator/` (target `OEmulator`) produces `O-Emulator-param-dump` —
the same convention the render harnesses use.

## Running

```bash
cmake -B build -G Ninja -DOUARICON_BUILD_TESTS=ON
cmake --build build --target O-Prism-param-dump
./build/scripts/.../O-Prism-param-dump > plugins/O-Prism/.planning/params.tsv
```

The binary lands under the plugin's own build dir; `find build -name '*-param-dump' -perm +111`
locates it without guessing the layout.

## Columns

| Column | Source |
|---|---|
| `id` | `getParameterID()` (`<no-id>` if the parameter is not ID-bearing — a blank cell can never be mistaken for a successful read) |
| `name` | `getName(128)` |
| `label` | `getLabel()` — the unit, where one was given |
| `numSteps` | `getNumSteps()`; `2147483647` means continuous |
| `textAtMin` / `textAtMax` | `getText(0.0f, 64)` / `getText(1.0f, 64)` |
| `defaultNorm` | `getDefaultValue()` — **normalised 0..1** |
| `defaultText` | `getText(getDefaultValue(), 64)` |
| `flags` | `automatable,discrete,boolean,meta,inverted` |

Every non-row line is `#`-prefixed (three metadata lines plus the column header),
so `dump | grep -vc '^#'` is exactly the parameter count and `# params` states it
directly.

Tabs, newlines and backslashes inside any cell are escaped (`\t`, `\n`, `\\`),
so the TSV stays unambiguous.

Where `label` is empty the range must be phrased from the UI's own formatter —
read how that plugin's `app.js` renders the readout. Do not invent a unit.

## Design constraints

- **Plugin-agnostic.** No plugin class name appears in `main.cpp`; the factory
  `createPluginFilter()` is the only entry point.
- **Nothing is mirrored.** Every `JucePlugin_*` value is read back off the
  plugin target's own `JUCE_*` properties. An empty property is a
  `FATAL_ERROR`, never a guessed stamp — a mirrored fixture constant has
  drifted silently twice in this repo.
- **The editor TU is never compiled.** The target sets `JUCE_WEB_BROWSER=0`,
  matching the render-harness rule that keeps a WebView swap from breaking a
  console target.
