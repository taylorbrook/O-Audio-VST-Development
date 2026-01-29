# Plugin Planning Templates

Templates for initializing the `.planning/` directory structure in new plugins.

## Directory Structure

When a new plugin is created, initialize this structure:

```
plugins/[PluginName]/
├── .planning/
│   ├── BRIEF.md              ← Use BRIEF-template.md
│   ├── STATUS.md             ← Use STATUS-template.md
│   ├── parameter-spec.md     ← From ideation/mockup
│   ├── ROADMAP.md            ← Created by Stage 0
│   ├── research/
│   │   └── ARCHITECTURE.md   ← Created by Stage 0
│   ├── mockups/              ← Optional, for UI mockup files
│   │   └── v1-ui.yaml
│   └── stages/
│       ├── 0-ideation/
│       │   ├── CONTEXT.md    ← Discuss output
│       │   └── DECISIONS.md  ← Key choices made
│       ├── 1-foundation/
│       │   ├── CONTEXT.md
│       │   ├── PLAN.md
│       │   ├── SUMMARY.md
│       │   └── VERIFICATION.md
│       ├── 2-dsp/
│       │   ├── CONTEXT.md
│       │   ├── RESEARCH.md   ← If complex algorithm
│       │   ├── PLAN.md
│       │   ├── SUMMARY.md
│       │   └── VERIFICATION.md
│       ├── 3-ui/
│       │   ├── CONTEXT.md
│       │   ├── PLAN.md
│       │   ├── SUMMARY.md
│       │   └── VERIFICATION.md
│       └── 4-polish/
│           ├── PLAN.md
│           ├── SUMMARY.md
│           └── VERIFICATION.md
├── Source/
├── CMakeLists.txt
├── CHANGELOG.md
└── NOTES.md
```

## GSD Cycle Per Stage

Each stage follows the GSD cycle:

```
DISCUSS → RESEARCH → PLAN → EXECUTE → VERIFY
   ↓          ↓        ↓        ↓         ↓
CONTEXT.md  RESEARCH.md  PLAN.md  SUMMARY.md  VERIFICATION.md
```

Not all stages need RESEARCH.md - typically only Stage 0 (architecture) and Stage 2 (DSP algorithms).

## Initialization Commands

To initialize a new plugin with this structure:

```bash
PLUGIN_NAME="O-NewPlugin"

mkdir -p plugins/${PLUGIN_NAME}/.planning/{research,mockups,stages/{0-ideation,1-foundation,2-dsp,3-ui,4-polish}}

# Copy templates
cp .claude/templates/plugin-planning/BRIEF-template.md plugins/${PLUGIN_NAME}/.planning/BRIEF.md
cp .claude/templates/plugin-planning/STATUS-template.md plugins/${PLUGIN_NAME}/.planning/STATUS.md

# Replace placeholders
sed -i '' "s/\[PluginName\]/${PLUGIN_NAME}/g" plugins/${PLUGIN_NAME}/.planning/BRIEF.md
sed -i '' "s/\[PluginName\]/${PLUGIN_NAME}/g" plugins/${PLUGIN_NAME}/.planning/STATUS.md
```

## Templates

- **BRIEF-template.md** - Creative brief / project vision (replaces PROJECT.md)
- **STATUS-template.md** - Current state and progress (replaces STATE.md and .planning/STATUS.md)

## Migration from Old Structure

If migrating from the old `.planning/` structure:

| Old Location | New Location |
|--------------|--------------|
| `.planning/BRIEF.md` | `.planning/BRIEF.md` |
| `.planning/parameter-spec.md` | `.planning/parameter-spec.md` |
| `.planning/architecture.md` | `.planning/research/ARCHITECTURE.md` |
| `.planning/plan.md` | `.planning/ROADMAP.md` |
| `.planning/mockups/` | `.planning/mockups/` |
| `.planning/STATUS.md` | `.planning/STATUS.md` |
