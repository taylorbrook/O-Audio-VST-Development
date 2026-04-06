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
| O-Bells | 📦 Installed | 3.2.1 | Synth (Physical Modeling Bells) | 2026-02-19 |
| O-Tremolo | 📦 Installed | 1.4.7 | Audio Effect (Tremolo) | 2026-02-12 |
| O-AnalogSaturation | 📦 Installed | 1.1.3 | Audio Effect (Saturation) | 2026-02-25 |
| O-Marimba | 📦 Installed | 1.12.0 | Synth (Physical Model) | 2026-02-09 |
| O-Comp | 📦 Installed | 1.4.3 | Audio Effect (Compressor) | 2026-03-06 |
| O-AnalogEQ | 📦 Installed | 1.1.7 | Audio Effect (EQ) | 2026-02-09 |
| O-DigiDelay | 📦 Installed | 1.2.9 | Audio Effect (Delay) | 2026-02-14 |
| O-SimpleReverb | 📦 Installed | 1.5.5 | Audio Effect (Reverb) | 2026-02-15 |
| O-Polystutter | 📦 Installed | 1.12.2 | Audio Effect (Beat Repeater) | 2026-03-06 | **Packaged:** [O-Polystutter-OuariconAudio.pkg](plugins/O-Polystutter/dist/) (4.5 MB, Signed) |
| O-Lyrica | 📦 Installed | 2.0.2 | Synth (Physical Modeling Harp) | 2026-03-06 |
| O-MultiBandCompressor | 📦 Installed | 1.2.0 | Audio Effect (Dynamics) | 2026-01-26 |
| O-Bass | 📦 Installed | 1.3.1 | Audio Effect (Bass Enhancer) | 2026-01-28 |
| O-IntonationPad | 📦 Installed | 2.7.2 | Synth (Wavetable Pad) | 2026-03-09 |
| O-Detune | 📦 Installed | 1.5.2 | Audio Effect (Detuning) | 2026-02-18 |
| O-Freeze | 📦 Installed | 2.0.0 | Audio Effect (Granular Freeze) | 2026-04-04 |
| O-FreqPulse | 📦 Installed | 1.16.2 | Audio Effect (Spectral Sequencer) | 2026-03-06 |
| O-SpectralShaper | 📦 Installed | 1.3.0 | Audio Effect (Spectral Transient Shaper) | 2026-03-08 |
| O-GrainScatter | 📦 Installed | 2.4.0 | Audio Effect (Granular Stutter Engine) | 2026-03-09 |
| O-Chorus | 📦 Installed | 1.2.1 | Audio Effect (Chorus) | 2026-02-25 |
| O-Orbit | 📦 Installed | 1.0.0 | Audio Effect (Spatial Orbiter) | 2026-02-11 |
| O-TextureForge | 📦 Installed | 1.0.2 | Instrument (Concatenative Synth) | 2026-02-18 |
| O-Texture | 📦 Installed | 0.1.0 | Instrument/Effect (Neural Texture Synth) | 2026-02-14 |
| O-Prism | 📦 Installed | 1.11.0 | Synth (Microtonal Wavetable) | 2026-03-09 |
| O-Gain | 📦 Installed | 1.0.0 | Audio Effect (Gain Staging Utility) | 2026-03-07 |
| O-Formant | 📦 Installed | 1.2.0 | Synth (Physical Model Vocal) | 2026-04-06 |
| O-Bowed | 📦 Installed | 1.0.1 | Synth (Physical Model Bowed String) | 2026-04-06 |
| O-Reed | 🚧 Stage 0 | - | Synth (Physical Modeling Reed Wind) | 2026-04-04 |
| O-Wind | 📦 Installed | 1.1.0 | Synth (Physical Model Flute) | 2026-04-06 |

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
