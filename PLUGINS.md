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
| O-Bells | 📦 Installed | 4.1.1 | Synth (Physical Modeling Bells) | 2026-07-08 |
| O-Tremolo | 📦 Installed | 1.6.0 | Audio Effect (Tremolo) | 2026-07-08 |
| O-AnalogSaturation | 📦 Installed | 1.1.5 | Audio Effect (Saturation) | 2026-06-30 |
| O-Marimba | 📦 Installed | 1.12.1 | Synth (Physical Model) | 2026-07-08 |
| O-Comp | 📦 Installed | 1.5.0 | Audio Effect (Compressor) | 2026-07-01 |
| O-AnalogEQ | 📦 Installed | 1.1.10 | Audio Effect (EQ) | 2026-06-30 |
| O-DigiDelay | 📦 Installed | 1.2.12 | Audio Effect (Delay) | 2026-07-01 |
| O-SimpleReverb | 📦 Installed | 1.5.6 | Audio Effect (Reverb) | 2026-07-07 |
| O-Polystutter | 📦 Installed | 1.12.3 | Audio Effect (Beat Repeater) | 2026-07-01 | **Packaged:** [O-Polystutter-OuariconAudio.pkg](plugins/O-Polystutter/dist/) (4.5 MB, Signed) |
| O-Lyrica | 📦 Installed | 2.3.2 | Synth (Physical Modeling Harp) | 2026-07-09 |
| O-MultiBandCompressor | 📦 Installed | 1.3.0 | Audio Effect (Dynamics) | 2026-07-01 |
| O-Bass | 📦 Installed | 1.3.3 | Audio Effect (Bass Enhancer) | 2026-07-08 |
| O-IntonationPad | 📦 Installed | 2.8.2 | Synth (Wavetable Pad) | 2026-07-09 |
| O-Detune | 📦 Installed | 1.5.3 | Audio Effect (Detuning) | 2026-07-01 |
| O-Freeze | 📦 Installed | 2.0.1 | Audio Effect (Granular Freeze) | 2026-07-01 |
| O-FreqPulse | 📦 Installed | 1.16.4 | Audio Effect (Spectral Sequencer) | 2026-07-08 |
| O-SpectralShaper | 📦 Installed | 1.3.2 | Audio Effect (Spectral Transient Shaper) | 2026-07-07 |
| O-GrainScatter | 📦 Installed | 2.4.2 | Audio Effect (Granular Stutter Engine) | 2026-07-09 |
| O-Chorus | 📦 Installed | 1.2.3 | Audio Effect (Chorus) | 2026-06-30 |
| O-Orbit | 📦 Installed | 1.0.0 | Audio Effect (Spatial Orbiter) | 2026-02-11 |
| O-TextureForge | 📦 Installed | 1.0.2 | Instrument (Concatenative Synth) | 2026-02-18 |
| O-Texture | 📦 Installed | 0.1.2 | Instrument/Effect (Neural Texture Synth) | 2026-07-15 |
| O-Prism | 📦 Installed | 1.19.1 | Synth (Microtonal Wavetable) | 2026-07-02 |
| O-Gain | 📦 Installed | 1.2.0 | Audio Effect (Gain Staging Utility) | 2026-07-01 |
| O-Formant | 📦 Installed | 1.25.4 | Synth (Physical Model Vocal) | 2026-07-01 |
| O-Bowed | 📦 Installed | 1.4.1 | Synth (Physical Model Bowed String) | 2026-07-08 |
| O-Reed | 📦 Installed | 1.1.0 | Synth (Physical Modeling Reed Wind) | 2026-04-26 |
| O-Wind | 📦 Installed | 1.16.3 | Synth (Physical Model Flute) | 2026-07-10 |
| O-Contrabass | 🚧 Stage 2 | 1.0.0-dev | Synth (Physical Model Bowed Bass) | 2026-07-08 |
| O-Bassoon | 🚧 Stage 0 | - | Synth (Physical Model Bassoon) | 2026-04-27 |
| O-MicrotonalSampler | 📦 Installed | 1.23.6 | Synth (Microtonal Sampler) | 2026-07-01 |
| O-simpleFM | 📦 Installed | 1.2.3 | Synth (Pedagogical 2-Op FM) | 2026-07-15 |
| O-simpleAdditive | ✅ Working | 1.0.4 | Synth (Pedagogical Additive + Wavetable) | 2026-07-15 |
| O-simpleGrain | 📦 Installed | 1.1.2 | Synth (Pedagogical Granular) | 2026-07-16 |
| O-simpleSubtractive | ✅ Working | 1.0.0 | Synth (Pedagogical Subtractive) | 2026-06-25 |
| O-simpleSampler | ✅ Working | 1.0.0 | Synth (Pedagogical Sampler) | 2026-07-01 |
| O-simpleBeatmaker | 📦 Installed | 1.0.2 | Synth (Pedagogical Step-Sequencer Drum Machine) | 2026-07-15 |
| O-simplePhysicalModelSynth | 📦 Installed | 1.0.2 | Synth (Pedagogical Physical Modeling) | 2026-07-16 |

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
