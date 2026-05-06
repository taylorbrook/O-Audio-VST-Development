---
phase: quick-004
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - CMakeLists.txt
  - .claude/branding.json
  - .github/workflows/build-and-release.yml
  - plugins/O-AnalogEQ/CMakeLists.txt
  - plugins/O-AnalogSaturation/CMakeLists.txt
  - plugins/O-Bass/CMakeLists.txt
  - plugins/O-Bells/CMakeLists.txt
  - plugins/O-Comp/CMakeLists.txt
  - plugins/O-Detune/CMakeLists.txt
  - plugins/O-DigiDelay/CMakeLists.txt
  - plugins/O-Freeze/CMakeLists.txt
  - plugins/O-FreqPulse/CMakeLists.txt
  - plugins/O-IntonationPad/CMakeLists.txt
  - plugins/O-Lyrica/CMakeLists.txt
  - plugins/O-Marimba/CMakeLists.txt
  - plugins/O-MultiBandCompressor/CMakeLists.txt
  - plugins/O-Polystutter/CMakeLists.txt
  - plugins/O-SimpleReverb/CMakeLists.txt
  - plugins/O-SpectralShaper/CMakeLists.txt
  - plugins/O-Tremolo/CMakeLists.txt
autonomous: true

must_haves:
  truths:
    - "Local dev builds produce plugins with '-dev' suffix in PRODUCT_NAME and 'Ouaricon Audio Development' as COMPANY_NAME"
    - "GitHub Actions release builds produce plugins with clean PRODUCT_NAME (no suffix) and 'Ouaricon Audio' as COMPANY_NAME"
    - "Dev builds use manufacturer code OuDv, release builds use OuAu"
    - "PKG installer Conclusion.txt references 'Ouaricon Audio' instead of 'Ouaricon Development' in release builds"
  artifacts:
    - path: "CMakeLists.txt"
      provides: "OUARICON_RELEASE option and derived variables"
      contains: "option(OUARICON_RELEASE"
    - path: "plugins/O-Tremolo/CMakeLists.txt"
      provides: "Example of variable-based plugin branding"
      contains: "${OUARICON_COMPANY_NAME}"
    - path: ".github/workflows/build-and-release.yml"
      provides: "Release flag passed to CMake on all 3 platforms"
      contains: "OUARICON_RELEASE=ON"
  key_links:
    - from: "CMakeLists.txt"
      to: "plugins/O-*/CMakeLists.txt"
      via: "CMake variables OUARICON_COMPANY_NAME, OUARICON_MANUFACTURER_CODE, OUARICON_DEV_SUFFIX"
      pattern: "set\\(OUARICON_"
    - from: ".github/workflows/build-and-release.yml"
      to: "CMakeLists.txt"
      via: "-DOUARICON_RELEASE=ON flag in cmake configure steps"
      pattern: "OUARICON_RELEASE=ON"
---

<objective>
Distinguish dev vs release plugin naming so local development builds show "-dev" suffix and "Ouaricon Audio Development" company name, while GitHub Actions release builds produce clean "Ouaricon Audio" branding with no suffix.

Purpose: Prevent confusion between development and production plugin binaries in DAW plugin lists. Dev plugins and release plugins will appear as distinct entries, allowing both to coexist on the developer's machine without conflict.

Output: Updated root CMakeLists.txt with OUARICON_RELEASE option, all 17 plugin CMakeLists.txt using variables instead of hardcoded values, updated CI workflow passing the release flag, and updated branding.json.
</objective>

<execution_context>
@/Users/taylorbrook/.claude/get-shit-done/workflows/execute-plan.md
@/Users/taylorbrook/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@CMakeLists.txt
@.claude/branding.json
@.github/workflows/build-and-release.yml
@plugins/O-AnalogEQ/CMakeLists.txt (reference pattern for all 17 plugins)
</context>

<tasks>

<task type="auto">
  <name>Task 1: Add OUARICON_RELEASE option and branding variables to root CMakeLists.txt</name>
  <files>CMakeLists.txt, .claude/branding.json</files>
  <action>
  In CMakeLists.txt, after the existing `option(OUARICON_LICENSING ...)` block (line 9), add a new option and variable block:

  ```cmake
  # Release branding: OFF for local dev, ON in GitHub Actions publish builds
  option(OUARICON_RELEASE "Use production branding (no -dev suffix)" OFF)

  if(OUARICON_RELEASE)
      set(OUARICON_COMPANY_NAME "Ouaricon Audio")
      set(OUARICON_MANUFACTURER_CODE OuAu)
      set(OUARICON_DEV_SUFFIX "")
      message(STATUS "[Ouaricon] Release branding — COMPANY: Ouaricon Audio, CODE: OuAu")
  else()
      set(OUARICON_COMPANY_NAME "Ouaricon Audio Development")
      set(OUARICON_MANUFACTURER_CODE OuDv)
      set(OUARICON_DEV_SUFFIX "-dev")
      message(STATUS "[Ouaricon] Dev branding — COMPANY: Ouaricon Audio Development, CODE: OuDv, SUFFIX: -dev")
  endif()
  ```

  Also update `.claude/branding.json`:
  - Change `development.full_name` from `"Ouaricon Development"` to `"Ouaricon Audio Development"`
  - Keep `production.full_name` as `"Ouaricon Audio"` (already correct)
  - Keep `production.manufacturer_code` as `"OuAu"` (already correct)
  </action>
  <verify>
  Run `cmake -B /tmp/test-dev-build -S /Users/taylorbrook/Dev/VST-development 2>&1 | grep '\[Ouaricon\]'` and confirm it shows "Dev branding" with "Ouaricon Audio Development" and "OuDv" and "-dev".

  Run `cmake -B /tmp/test-rel-build -S /Users/taylorbrook/Dev/VST-development -DOUARICON_RELEASE=ON 2>&1 | grep '\[Ouaricon\]'` and confirm it shows "Release branding" with "Ouaricon Audio" and "OuAu".

  Clean up: `rm -rf /tmp/test-dev-build /tmp/test-rel-build`
  </verify>
  <done>Root CMakeLists.txt defines OUARICON_RELEASE option (OFF by default) and sets OUARICON_COMPANY_NAME, OUARICON_MANUFACTURER_CODE, and OUARICON_DEV_SUFFIX variables. branding.json updated with "Ouaricon Audio Development" for dev.</done>
</task>

<task type="auto">
  <name>Task 2: Update all 17 Ouaricon plugin CMakeLists.txt to use branding variables</name>
  <files>
    plugins/O-AnalogEQ/CMakeLists.txt
    plugins/O-AnalogSaturation/CMakeLists.txt
    plugins/O-Bass/CMakeLists.txt
    plugins/O-Bells/CMakeLists.txt
    plugins/O-Comp/CMakeLists.txt
    plugins/O-Detune/CMakeLists.txt
    plugins/O-DigiDelay/CMakeLists.txt
    plugins/O-Freeze/CMakeLists.txt
    plugins/O-FreqPulse/CMakeLists.txt
    plugins/O-IntonationPad/CMakeLists.txt
    plugins/O-Lyrica/CMakeLists.txt
    plugins/O-Marimba/CMakeLists.txt
    plugins/O-MultiBandCompressor/CMakeLists.txt
    plugins/O-Polystutter/CMakeLists.txt
    plugins/O-SimpleReverb/CMakeLists.txt
    plugins/O-SpectralShaper/CMakeLists.txt
    plugins/O-Tremolo/CMakeLists.txt
  </files>
  <action>
  For each of the 17 `plugins/O-*/CMakeLists.txt` files, make three replacements in the `juce_add_plugin()` call:

  1. Replace `COMPANY_NAME "Ouaricon Development"` with `COMPANY_NAME "${OUARICON_COMPANY_NAME}"`
  2. Replace `PLUGIN_MANUFACTURER_CODE OuDv` with `PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}`
  3. Append the dev suffix to PRODUCT_NAME: Replace `PRODUCT_NAME "O-PluginName"` with `PRODUCT_NAME "O-PluginName${OUARICON_DEV_SUFFIX}"`

  IMPORTANT: Each plugin has a unique PRODUCT_NAME (e.g., "O-AnalogEQ", "O-Bass", "O-Tremolo"). Preserve the base name exactly and only append `${OUARICON_DEV_SUFFIX}` to each one. The complete list of transformations:

  | Plugin Dir | Current PRODUCT_NAME | New PRODUCT_NAME |
  |---|---|---|
  | O-AnalogEQ | "O-AnalogEQ" | "O-AnalogEQ${OUARICON_DEV_SUFFIX}" |
  | O-AnalogSaturation | "O-AnalogSaturation" | "O-AnalogSaturation${OUARICON_DEV_SUFFIX}" |
  | O-Bass | "O-Bass" | "O-Bass${OUARICON_DEV_SUFFIX}" |
  | O-Bells | "O-Bells" | "O-Bells${OUARICON_DEV_SUFFIX}" |
  | O-Comp | "O-Comp" | "O-Comp${OUARICON_DEV_SUFFIX}" |
  | O-Detune | "O-Detune" | "O-Detune${OUARICON_DEV_SUFFIX}" |
  | O-DigiDelay | "O-DigiDelay" | "O-DigiDelay${OUARICON_DEV_SUFFIX}" |
  | O-Freeze | "O-Freeze" | "O-Freeze${OUARICON_DEV_SUFFIX}" |
  | O-FreqPulse | "O-FreqPulse" | "O-FreqPulse${OUARICON_DEV_SUFFIX}" |
  | O-IntonationPad | "O-IntonationPad" | "O-IntonationPad${OUARICON_DEV_SUFFIX}" |
  | O-Lyrica | "O-Lyrica" | "O-Lyrica${OUARICON_DEV_SUFFIX}" |
  | O-Marimba | "O-Marimba" | "O-Marimba${OUARICON_DEV_SUFFIX}" |
  | O-MultiBandCompressor | "O-MultiBandCompressor" | "O-MultiBandCompressor${OUARICON_DEV_SUFFIX}" |
  | O-Polystutter | "O-Polystutter" | "O-Polystutter${OUARICON_DEV_SUFFIX}" |
  | O-SimpleReverb | "O-SimpleReverb" | "O-SimpleReverb${OUARICON_DEV_SUFFIX}" |
  | O-SpectralShaper | "O-SpectralShaper" | "O-SpectralShaper${OUARICON_DEV_SUFFIX}" |
  | O-Tremolo | "O-Tremolo" | "O-Tremolo${OUARICON_DEV_SUFFIX}" |

  Do NOT change: the cmake target name (first arg to juce_add_plugin), PLUGIN_CODE, FORMATS, VERSION, or any other fields. Only change COMPANY_NAME, PLUGIN_MANUFACTURER_CODE, and PRODUCT_NAME.
  </action>
  <verify>
  Run this to confirm all 17 files were updated correctly:

  ```bash
  # Should return 0 lines (no hardcoded "Ouaricon Development" remaining)
  grep -r 'COMPANY_NAME "Ouaricon Development"' plugins/O-*/CMakeLists.txt

  # Should return 17 lines (all using variable)
  grep -c 'COMPANY_NAME "${OUARICON_COMPANY_NAME}"' plugins/O-*/CMakeLists.txt

  # Should return 0 lines (no hardcoded OuDv for manufacturer)
  grep -r 'PLUGIN_MANUFACTURER_CODE OuDv' plugins/O-*/CMakeLists.txt

  # Should return 17 lines (all using variable)
  grep -c 'PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}' plugins/O-*/CMakeLists.txt

  # Should return 17 lines (all product names have suffix variable)
  grep -c 'OUARICON_DEV_SUFFIX' plugins/O-*/CMakeLists.txt
  ```

  Then do a test cmake configure to ensure no syntax errors:
  ```bash
  cmake -B /tmp/test-plugin-build -S /Users/taylorbrook/Dev/VST-development 2>&1 | tail -5
  rm -rf /tmp/test-plugin-build
  ```
  </verify>
  <done>All 17 Ouaricon plugin CMakeLists.txt files use ${OUARICON_COMPANY_NAME}, ${OUARICON_MANUFACTURER_CODE}, and ${OUARICON_DEV_SUFFIX} instead of hardcoded values. No hardcoded "Ouaricon Development" or "OuDv" remains in any plugin CMakeLists.txt.</done>
</task>

<task type="auto">
  <name>Task 3: Add OUARICON_RELEASE=ON to GitHub Actions workflow and fix PKG installer text</name>
  <files>.github/workflows/build-and-release.yml</files>
  <action>
  In `.github/workflows/build-and-release.yml`, add `-DOUARICON_RELEASE=ON` to the CMake configure step on all three platform jobs:

  1. **macOS** (line ~66-71): Add `-DOUARICON_RELEASE=ON \` to the cmake -B build command, after the existing `-DOUARICON_LICENSING=ON` line.

  2. **Windows** (line ~413): Add `-DOUARICON_RELEASE=ON` to the cmake command. The Windows cmake line is a single line, so append it: `cmake -B build -DCMAKE_BUILD_TYPE=${{ env.BUILD_TYPE }} -DOUARICON_LICENSING=ON -DOUARICON_RELEASE=ON -DOUARICON_SUPABASE_URL=...`

  3. **Linux** (line ~503-507): Add `-DOUARICON_RELEASE=ON \` to the cmake -B build command, after the existing `-DOUARICON_LICENSING=ON` line.

  4. **Fix PKG Conclusion.txt** (line ~287-288): In the "Create PKG Installer" step's Conclusion.txt heredoc, change:
     - `VST3: Plugin browser under "Ouaricon Development"` to `VST3: Plugin browser under "Ouaricon Audio"`
     - `AU: Audio Units > Ouaricon Development > ${PRODUCT_NAME}` to `AU: Audio Units > Ouaricon Audio > ${PRODUCT_NAME}`

  These are the ONLY changes to the workflow file. Do not modify any other steps.
  </action>
  <verify>
  Verify the workflow file contains OUARICON_RELEASE=ON in all three platform configure steps:

  ```bash
  grep -c 'OUARICON_RELEASE=ON' .github/workflows/build-and-release.yml
  # Should return 3 (macOS, Windows, Linux)

  grep 'Ouaricon Development' .github/workflows/build-and-release.yml
  # Should return 0 lines (no more "Ouaricon Development" references)

  grep 'Ouaricon Audio' .github/workflows/build-and-release.yml
  # Should return lines from Welcome.txt, Conclusion.txt, and Distribution.xml sections
  ```

  Validate YAML syntax: `python3 -c "import yaml; yaml.safe_load(open('.github/workflows/build-and-release.yml'))" && echo "YAML valid"`
  </verify>
  <done>GitHub Actions workflow passes -DOUARICON_RELEASE=ON on all 3 platforms (macOS, Windows, Linux). PKG installer Conclusion.txt references "Ouaricon Audio" instead of "Ouaricon Development".</done>
</task>

</tasks>

<verification>
1. Confirm no hardcoded "Ouaricon Development" remains in any plugin CMakeLists.txt
2. Confirm root CMakeLists.txt has OUARICON_RELEASE option with correct variable assignments
3. Confirm GitHub Actions workflow passes OUARICON_RELEASE=ON on all 3 platforms
4. Confirm PKG installer text uses "Ouaricon Audio" for release builds
5. Run a test cmake configure with default settings (dev mode) - should show dev branding messages
6. Run a test cmake configure with -DOUARICON_RELEASE=ON - should show release branding messages
</verification>

<success_criteria>
- Default local builds: plugins appear as "O-PluginName-dev" under "Ouaricon Audio Development" with manufacturer code OuDv
- Release builds (GitHub Actions): plugins appear as "O-PluginName" under "Ouaricon Audio" with manufacturer code OuAu
- All 17 plugins consistently use CMake variables (zero hardcoded branding values)
- CI workflow YAML is valid and passes OUARICON_RELEASE=ON on macOS, Windows, and Linux
- branding.json development name updated to "Ouaricon Audio Development"
</success_criteria>

<output>
After completion, create `.planning/quick/004-dev-vs-release-plugin-naming/004-SUMMARY.md`
</output>
