# Research: two-pane grouped preset menu (R5 / D3 / D4)

Read-only research, 2026-11-14. Paths are relative to `plugins/O-GranulatorInstrument/` unless they start with `modules/` or `scripts/` (repo root).

## Recommendation (short version)

- **Plugin-local. Do not touch the shared module.** The menu is already 100 % plugin code in `index.html`. The module only supplies `refresh()`, `getPresetList()`, `getCurrentPreset()`, `loadPreset()` and `loadFromFile()`. Nothing in the two-pane work needs a module change, so there is no fork and no FORK-REVIEW drift.
- **Add one plugin-local native, `getPresetMenu`**, built by a pure encoder in `Source/GiUiJson.h`. It returns an ordered array: `[{ group: "drones", presets: [{ name, short }, …] }, …, { group: "user", presets: [...] }]`. It works out factory vs user on the C++ side, from the Factory directory, which is the same truth `isFactoryPreset` and `loadPreset` use. This is the O-Orbit precedent (`plugins/O-Orbit/Source/PluginEditor.cpp:451-459`: "the preset MENU's only data source", with group order carried in the data).
- **Left-pane group headers are static markup with `data-i18n`**, the same way `#factory-list` does it (`index.html:1317-1321`). That is what keeps check-i18n rule 13 green: setLabel keys must be plain literals, so `setLabel(el, \`grp.${k}\`)` fails.

## 1. How the menu is built today

### Markup and CSS (plugin-local, `Source/ui/public/index.html`)

- `:142-150` holds the `#preset-menu` CSS (`left 248, top 52, width 300, max-height 420, overflow-y auto, z 40`) and the `.pm-item` (22 px rows, 12 px, nowrap and ellipsis), `.pm-item.sel`, `.pm-sep` and `.pm-file` rules.
- **`:554` overrides the box to `#preset, #preset-menu { left: 280px; width: 280px; }`**, so the effective menu is 280 px wide at x = 280. The PN1 header comment saying "300 px" is stale.
- `:779` is the bar: `#preset-prev`, `#preset-name` (`aria-haspopup="listbox"`, `aria-expanded`) containing `#preset-name-text` and `#preset-kind`, then `#preset-next` and `#preset-save`.
- `:780` is `<div id="preset-menu" role="listbox" data-i18n-aria="aria.presetMenu" hidden></div>`, which is empty until the menu opens.

### JS (plugin-local, same file)

- `closePresetMenu()` is at `:4836-4842`.
- `openPresetMenu()` is at `:4844-4872`. It calls `presetManager.refresh()`, then `getPresetList()`, and builds one flat `<button class="pm-item">` per name. The current preset gets `.sel`. Each item sets `dataset.userText = '1'` and `textContent = name` (the full name) and loads with `presetManager.loadPreset(name)` on click. After the presets come a `.pm-sep` and **one** file item, `.pm-item.pm-file`, `setLabel(file, 'label.loadFromFile')` → `presetManager.loadFromFile()`. That is the only file action in the menu. Save is the bar's `#preset-save` button, wired by the module.
- `initPresetBar()` is at `:4882-4899`. It does `import('./js/preset-manager.js')`, passes in the bar elements, and toggles the menu from `#preset-name` click.
- `refreshPresetKind()` (`:4874-4880`) sets Factory or User through `isFactoryPreset(current)`.
- Dismissal: a capture-phase `pointerdown` outside the menu closes it (`:3802-3803`). `Escape` closes it (`:3820`). **There is no arrow-key handling today.** Items are native `<button>`s and focus never moves into the menu.
- State: `let presetMenuOpen` is at `:2106`.

### Shared module

- `Source/ui/public/modules/preset-manager.js` (461 lines) is a **configure-time copy** and is gitignored. Root `.gitignore:21-27` has `plugins/*/Source/ui/public/modules/*` with the note "OuariconModules.cmake auto-copies module JS…". `ui_frontend_check [7]` asserts it is untracked.
- The bytes that are actually embedded come straight from the repo module. `CMakeLists.txt:89` has `ouaricon_add_module(... preset-manager)` and `:150` has `${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/js/preset-manager.js` in SOURCES. They are served at `/js/preset-manager.js` (`PluginEditor.cpp:561-563`).
- The local copy is identical to `modules/persistence/preset-manager/js/preset-manager.js` (diff clean).
- `modules/registry.yaml:129-163` lists preset-manager **1.1.1**, and the processor pins it (`PluginProcessor.h:32`).

### How the JS learns factory vs user: it doesn't, except one name at a time

- `getPresetList` (`PluginEditor.cpp:1078-1084`) returns `OuariconPresetManager::getPresetList()` (`modules/persistence/preset-manager/cpp/OuariconPresetManager.h:580-606`). That is the Factory/*.json names plus the User/*.json names, **with any user name equal to a factory name dropped and the result case-insensitively sorted**. It carries no flag, folder or category.
- `isFactoryPreset(name)` (`PluginEditor.cpp:1103-1107` → `.h:326-330`) checks whether `Factory/<sanitized>.json` exists. It is a single-name round trip, and the page uses it only for the bar's Factory/User tag.
- **Conclusion:** grouping needs factory membership for every name. A per-open loop of 160 `isFactoryPreset` calls would work but is slow, and the completions are subject to the dropped-completion trap. A user preset named `Pads · X` must land in User (D4), so a prefix test alone is wrong. Hence `getPresetMenu`.

## 2. Plugin-local vs module

**Plugin-local.** Reasons:

1. The rendering code is already plugin-local, so there is nothing to fork. The module's JS has no menu at all; its only UI is the display, prev/next and save. The module's C++ has no category concept.
2. Six other plugins solved grouped menus plugin-locally, each with its own native: O-Orbit and O-Bitrot `getPresetListGrouped`, O-Prism and O-Strata `getPresetListWithCategories`, O-Bells, O-Formant. O-Bells, O-Formant, O-Prism and O-Strata did it through **forked** `OuariconPresetManager.h` copies, which is exactly the drift to avoid. The O-Orbit form (an editor-side native over the unmodified module) is the clean precedent.
3. A shared two-pane menu would be a new module extraction across six or more plugins with differing shapes. Out of scope; note it as a later candidate.

### Proposed C++ (smallest correct change)

- **`GiUiJson.h`**: `inline juce::var presetMenuToVar(const juce::StringArray& all, const juce::StringArray& factoryNames)`. It is pure, with no WebView types (keeps `ui_frontend_check [13]` green).
  - Fixed group order: drones, expressive, field, keys, live, motion, pads, rhythmic, textures, user.
  - A factory name goes to the group of its `"<Cat> · "` prefix. `short` is the text after the separator (`ogi::ids::dot()`, `GiParamIds.h:129-133`). An unknown prefix goes to user, as a fail-safe.
  - Any non-factory name goes to user with `short = name`.
  - Inside a group, `getPresetList` order is kept (already sorted).
  - Empty factory groups are still emitted, so the left pane is constant.
- **`PluginEditor.cpp`**: `.withNativeFunction("getPresetMenu", …)`. It calls `pm.getPresetList()` and gets the factory names from `pm.getFactoryPresetsDirectory().findChildFiles(... "*.json")`. That is the same truth as `isFactoryPreset`, and the same truth as `loadPreset`, which searches Factory first (`.h:506-509`).
- **`tests/render-harness/main.cpp:1544`**: add `natives->setProperty("getPresetMenu", GiUiJson::presetMenuToVar(<all 160 spec names>, <same>))`. This gives the stub a real 160-name fixture, following the "one encoder" pattern of `[17]` (`ui_frontend_check.js:1020-1023`). Add a harness row that calls the encoder with synthetic inputs: per-group counts 20×7 / 8 / 12; `Pads · Fake` given only in `all` lands in user (negative control); `short` round-trips back to the full name.
- **No state, parameter or format change.** One extra native only, in line with D5.

### JS

- `openPresetMenu()`:
  - Call `await presetManager.refresh()` first, which keeps the current name fresh.
  - Then `callNative('getPresetMenu')`. Go through `callNative` so the `[3]` funnel scan sees the literal name, and so stub overrides mutated in the page are picked up on every open.
  - Fill the right pane from the active group.
  - Loading still goes through `presetManager.loadPreset(fullName)`.
- The bar keeps the full name; the module's `_updateDisplay` writes it, so there is no change.
- Open selects the current preset's group: the group containing `getCurrentPreset()`, else drones.
- The file item stays: `.pm-file` "Load from file…" sits under the left pane after a `.pm-sep`.

## 3. i18n: files and scripts

### New keys

New keys go in `Source/ui/public/js/i18n.js`, with `en`, `fr` (`reviewed: false`) and `zh-Hans` (`reviewed: 'mt'`), following `list.src*` at `:1746-1779`.

- **Key names:** suggested `list.pg<Group>`, nine of them. **Reuse `label.user` (`:1621-1625`: User / Utilisateur / 用户) for the User header** rather than adding a duplicate.
- Group labels are plural and distinct from the singular source categories (`list.srcDrone` "Drone"), so new keys are needed. Reuse their translations where the meaning matches.

| key | en | fr (reuse / glossary) | zh-Hans (reuse / glossary) |
|---|---|---|---|
| list.pgDrones | Drones | Bourdons (= list.srcDrone fr) | 持续音 (= list.srcDrone) |
| list.pgExpressive | Expressive | Expressifs | 表情 (zh glossary `expression`) |
| list.pgField | Field | Terrain (= list.srcField) | 实地录音 (= list.srcField) |
| list.pgKeys | Keys | Claviers | 键盘 |
| list.pgLive | Live | En direct | 现场 |
| list.pgMotion | Motion | Mouvement | 运动 (zh glossary `motion`) |
| list.pgPads | Pads | Nappes (fr glossary `looped pad` → nappe) | 铺垫 (zh glossary `looped pad` → 循环铺垫) |
| list.pgRhythmic | Rhythmic | Rythmiques (= list.srcRhythmic) | 节奏 (= list.srcRhythmic) |
| list.pgTextures | Textures | Textures, **`sameAsEn: true`** (check-i18n rule 4) | 纹理 (= list.srcTexture) |
| (label.user) | User | Utilisateur | 用户 |

Optional new aria keys: `aria.presetGroups` ("Preset groups") and possibly `aria.presets`. `aria.presetMenu` (`:1221`) already exists.

### Markup

The ten header buttons are static in `index.html`, each `<div role="option" id="pm-g-drones" data-group="drones"><span data-i18n="list.pgDrones">Drones</span></div>`. The right-pane items are `dataset.userText = '1'`, as today, so the name text is exempt as user text.

### Scripts to run (repo root)

- `node scripts/check-i18n.js --plugin O-GranulatorInstrument`. Rules 4, 5, 13 and 15: every key resolves and no key is dead, so all nine must be referenced in markup.
- `node scripts/i18n-fr-lint.js --plugin O-GranulatorInstrument` (glossary `scripts/i18n-fr-glossary.js`).
- `node scripts/i18n-zh-lint.js --plugin O-GranulatorInstrument` (glossary `scripts/i18n-zh-glossary.js`).
- `node scripts/check-ui-labels.js --plugin O-GranulatorInstrument`. It replays `tests/i18n-states.json`, which currently holds 49 states with `factory-list-open` last. **Append** a `preset-menu-open` state **last**. Its eval sets `window.__stubOverrides.natives.getPresetMenu`, if it is not already dumped, and then calls `document.getElementById('preset-name').click()`. Choose the widest group (Pads / Rhythmic) so the right pane is full.
- **Bump the probe pin** at `tests/ui_binding_probe.js:2284`: `states.length === (MUT_STATES ? 50 : 49)` becomes `51 : 50`. Update the message text too.

## 4. Existing tests and what changes

- **`tests/preset_name_fit.js`** (PN1):
  - The regex at `:51` lists 8 categories and has **no `Field`**. Add it.
  - `:67` has `names.length !== 64`. Change it to 160.
  - The menu measure (`:90-104`) builds a bare `.pm-item` in `#preset-menu`. It must instead measure the **short** name in the right-pane item, appended inside the right listbox so it has the real width, in en, fr and zh-Hans.
  - Keep the full-name `.pname b` check (`:81-88`).
  - Add a group-header fit row: each header in the left pane at each language, nowrap.
  - Keep the PN1n control. Add a control for the short-name pane: a 40-character short name must overflow.
- **`tests/ui_frontend_check.js`**:
  - `[3]` `:277`: `pageNames.size === 22` becomes 23.
  - `:143`: `N_NATIVE = 36` becomes 37.
  - `:278` `pmNames === 10` is unchanged, because the module is untouched.
  - Add a static `[18]`, modelled on `[17]` (`:990-1024`):
    - The `#preset-menu` markup has 10 group options in the D1 order, each with a key that has en, fr and zh-Hans entries.
    - The page writes and clears `aria-activedescendant` for both listboxes.
    - `getPresetMenu` answers through `GiUiJson::presetMenuToVar` in both the editor and the harness dump. The `[17]` regex trio is the pattern.
- **`tests/ui_binding_probe.js`**:
  - `PRESET_FNS` (`:225-226`) is unchanged; those are module natives.
  - Only the ST1 pin (`:2284`) changes.
  - Boot quiescence (`:419-427`) does not depend on the menu.
- **`tests/ui_factory_picker_check.js`**: no change needed. Its header (`:20-52`) describes FK1–FK4: listbox with focus, Up/Down wrap, Left/Right to the nearest row of the neighbouring column, Home/End, type-ahead, aria-activedescendant after every move, Enter, and Escape refocusing the trigger. This is the pattern to copy, both for the test and for the code at `index.html:3976-4108` (`setFactoryKb` `:4026-4032`, `onFactoryKey` `:4065-4077`, `openFactoryList` `:4079-4098`, `closeFactoryList(refocus)` `:4100-4107`). One small addition to consider: opening the preset menu should `closeFactoryList()`, and the reverse, as `openFactoryList` already does for the scale popovers (`:4083`).
- **New `tests/ui_preset_menu_check.js`** (the R5 "menu probe"), a port of `ui_factory_picker_check.js`:
  - PM1 content: 10 groups; per-group counts 20 / 20 / 12 / 20 / 8 / 20 / 20 / 20 / 20 / user; short names; user presets only in User (fixture with a user `Pads · Mine`).
  - PM2 geometry: en, fr and zh-Hans; inside the 1200 × 800 frame; no right-pane scroll for 20 rows.
  - PM3 keyboard: Up/Down within a pane (wrap or clamp, decide at plan); Left/Right between panes; Enter calls `loadPreset` with the **full** name (assert on `__stubNativeCalls`); Escape closes and refocuses `#preset-name`; hover on a group switches the pane.
  - PM4: open selects the current preset's group, and dismissal works.
  - Controls: a missing group, a 60-character short name, and a fixture that merges a user preset into Pads.
- **Stub note:** `scripts/ui-stub/generic-juce-stub.js:270-288` answers `getPresetList` as `['Default','Preset A','Preset B']`. An un-dumped `getPresetMenu` gets `benignDefault` → `null` (`:402-409`), so the JS must tolerate null and show an empty User. Dumping `getPresetMenu` from the harness avoids this.

## 5. Layout fit

- **Frame:** 1200 × 800 (`ui_frontend_check [11]`, `PluginEditor setSize(1200, 800)`). Pop-up margins: `FRAME_W=1200, POP_EDGE=15, POP_BOTTOM=785` (`index.html:1927`).
- **Width:** the menu's left edge is 280 (`:554`). At 460 px the right edge is 740, far inside 1185.
- **Height:** the right pane at its largest is 20 rows × 22 px = 440, plus 12 px padding, = 452. That puts the bottom at 52 + 452 = 504, inside 785. **The current `max-height: 420px` would scroll a 20-row group**, so raise it to at least 452, or use 20 px rows (412). The left pane is 10 × 22 + separator 11 + file row 22 ≈ 253. User can grow without bound: give the right pane `overflow-y: auto` for User only.
- **Measured** in the real served page with EB Garamond loaded, `.pm-item` at 12 px, Playwright via `scripts/serve-ui.js`. Script at the scratchpad path; it is not part of the repo.
  - Widest current **short** name: "Threshold Catch" 78.2 px, then "Next Note Grab" 75.4 and "Pentatonic Mist" 73.3. These are the same in every language because names are not localized.
  - Widest full name: "Expressive · Wheel Morph" 118.8 px.
  - Theoretical worst case: names ≤ 24 characters and the shortest prefix is `Keys · ` / `Live · ` / `Pads · ` (7), so a short name is ≤ 17 characters, about 110 px.
  - Group headers, en: widest Expressive 47.2 and Rhythmic 46.6.
  - Group headers, fr: widest **Rythmiques 57.2**, Mouvement 56.6, Utilisateur 48.8; "Charger un fichier…" 92.
  - Group headers, zh: widest 实地录音 49.0; "从文件载入…" 69.5.
- **Suggested split:** left pane about 150 px (fits the 92 px French file item plus 24 px padding), right pane about 310 px. Both have large slack, so 460 is comfortable and could even shrink. Pin the left pane width explicitly rather than content-sizing it (memory: pin content-sized boxes at the widest language).

## 6. Accessibility pattern

Copy the factory picker: **listbox + `aria-activedescendant`, with focus held on the listbox** (`index.html:1317`, `:4026-4032`, `:4097`).

- `#preset-menu` becomes a container. It can no longer be `role="listbox"`, because a listbox cannot contain listboxes. Use `role="group"` (or `dialog`) with `data-i18n-aria="aria.presetMenu"`. Set the trigger `#preset-name` to `aria-haspopup="dialog"` (or keep `listbox` if the container stays a group; decide at plan).
- **Left pane:** `role="listbox" tabindex="-1"` labelled "Preset groups" (new aria key). Each group is `role="option"`, with `aria-selected` on the active group.
- **Right pane:** `role="listbox" tabindex="-1"` with `aria-labelledby` pointing at the active group's header span. Options have `role="option"`, ids like `pm-p-<n>`, and `aria-selected="true"` on the current preset.
- **Keys** (one capture-phase keydown handler like `onFactoryKey`, `:4065-4077`):
  - Up/Down move the active option in the focused pane; hovering a group or moving through groups re-fills the right pane.
  - Right moves focus to the right listbox (first item, or the current preset if it is in this group); Left goes back to the groups.
  - Home/End.
  - Enter: in the right pane, load the preset; on a group, move to the right pane; on the file row, load from file.
  - Escape closes and refocuses `#preset-name`. Tab closes.
  - Type-ahead in the right pane is optional (`factoryType`, `:4049-4063`).
- **File row:** a real `<button class="pm-file">` below the left listbox. It is reached by Down past the last group, which moves real focus to it, or by Tab. Enter or click activates it.
- **Hover:** use `pointermove` / `pointerenter` on a group to switch the pane, the same pattern as `:4019-4021`. Leave the capture-phase `pointerdown` dismissal (`:3802`) as it is.

## Risks / notes

- The prev/next arrows still walk the flat sorted list (module), so User presets stay interleaved by sort. That is out of scope, but mention it to Taylor.
- A user preset whose name equals a factory name is invisible: dropped in `getPresetList`, and Factory wins on load. This is existing behaviour and is unchanged.
- `.bank-hash` rewrites happen in the constructor, so the Factory directory holds exactly the 160 at runtime. A harness row comparing `factoryPresetSpecs()` names against the encoder fixture keeps the dump honest.
