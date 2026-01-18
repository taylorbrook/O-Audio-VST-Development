# PLUGIN REGISTRY

## State Legend

- **💡 Ideated** - Creative brief exists, no implementation
- **💡 Ideated (Draft Params)** - Creative brief + draft parameters, ready for parallel workflow
- **🚧 Stage N** - In development (specific stage number)
- **✅ Working** - Completed Stage 6, not installed
- **📦 Installed** - Deployed to system folders
- **🐛 Has Issues** - Known problems (combines with other states)
- **🗑️ Archived** - Deprecated

## State Machine Rules

- If status is 🚧: ONLY plugin-workflow can modify (use `/continue` to resume)
- plugin-improve blocks if status is 🚧 (must complete workflow first)

## Build Management

- All plugin builds managed by `build-automation` skill
- Build logs: `logs/[PluginName]/build_TIMESTAMP.log`
- Installed plugins: `~/Library/Audio/Plug-Ins/VST3/` and `~/Library/Audio/Plug-Ins/Components/`

## Plugin Registry

Ouaricon Plugins:
| Plugin Name | Status | Version | Type | Last Updated |
|-------------|--------|---------|------|--------------|
| OuariconTremolo | 📦 Installed | 1.3.0 | Audio Effect (Tremolo) | 2026-01-12 |
| OuariconSaturationModeling | 📦 Installed | 1.0.1 | Audio Effect (Saturation) | 2026-01-14 |
| Ouaricon Marimba | 📦 Installed | 1.9.9 | Synth (Physical Model) | 2026-01-14 |
| OuariconComp | 📦 Installed | 1.1.1 | Audio Effect (Compressor) | 2026-01-12 |
| OuariconAnalogEQ | 📦 Installed | 1.0.10 | Audio Effect (EQ) | 2026-01-11 |
| Ouaricon Digital Delay | 📦 Installed | 1.2.0 | Audio Effect (Delay) | 2026-01-12 |
| OuariconSimpleReverb | 📦 Installed | 1.4.1 | Audio Effect (Reverb) | 2026-01-13 |
| OuariconPolystutter | 📦 Installed | 1.5.0 | Audio Effect (Beat Repeater) | 2026-01-18 |
| OuariconLyrica | 📦 Installed | 1.5.4 | Synth (Physical Modeling Harp) | 2026-01-18 |

Plugins created by **[TÂCHES](https://youtube.com/tachesteaches)**
| Plugin Name | Status | Version | Type | Last Updated |
|-------------|--------|---------|------|--------------|
| GainKnob | 📦 Installed | 1.2.3 | Audio Effect (Utility) | 2025-11-10 |
| TapeAge | 📦 Installed | 1.1.1 | Audio Effect | 2025-11-15 |
| ClapMachine | 💡 Ideated | - | - | 2025-11-10 |
| DriveVerb | 📦 Installed | 1.0.2 | Audio Effect (Reverb) | 2025-11-12 |
| FlutterVerb | 📦 Installed | 1.0.3 | Audio Effect (Reverb) | 2025-11-12 |
| LushVerb | 💡 Ideated | - | Audio Effect (Reverb) | 2025-11-12 |
| OrganicHats | 📦 Installed | 1.0.0 | Synth (Instrument) | 2025-11-12 |
| DrumRoulette | 📦 Installed | 1.0.0 | Instrument (Drum Sampler) | 2025-11-12 |
| Scatter | ✅ Working | 1.0.0 | Audio Effect (Granular Delay) | 2025-11-14 |
| AutoClip | 📦 Installed | 1.0.1 | Audio Effect (Hard Clipper) | 2025-11-15 |
| MinimalKick | 🚧 Stage 5 | - | Synth | 2025-11-13 |
| Drum808 | 📦 Installed | 1.0.0 | Synth (Drum Instrument) | 2025-11-13 |
| LushPad | 📦 Installed | 1.0.0 | Synth (Instrument) | 2025-11-13 |
| Words | 💡 Ideated | - | Utility (MIDI Sequencer) | 2025-11-13 |
| PadForge | 💡 Ideated | - | Synth (Instrument) | 2025-11-14 |
| AngelGrain | 📦 Installed | 1.0.0 | Audio Effect (Granular Delay) | 2025-11-19 |

**For detailed plugin information (lifecycle timeline, known issues, parameters, etc.), see:**
`plugins/[PluginName]/NOTES.md`

## Entry Template

When adding new plugins to this registry, use this format:

```markdown
| [PluginName] | [Emoji] [State] | [X.Y.Z or -] | [Type or -] | YYYY-MM-DD |
```

Create corresponding `plugins/[PluginName]/NOTES.md` with full details:

```markdown
# [PluginName] Notes

## Status
- **Current Status:** [emoji] [State Name]
- **Version:** [X.Y.Z or N/A]
- **Type:** [Type]

## Lifecycle Timeline

- **YYYY-MM-DD:** [Event description]
- **YYYY-MM-DD (Stage N):** [Stage completion description]
- **YYYY-MM-DD (vX.Y.Z):** [Version release description]

## Known Issues

[Issue description or "None"]

## Additional Notes

[Any other relevant information - description, parameters, DSP, GUI, validation, formats, installation locations, use cases, etc.]
```
