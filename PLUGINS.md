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
| O-Bells | 📦 Installed | 4.5.0 | Synth (Physical Modeling Bells) | 2026-09-05 |
| O-Tremolo | 📦 Installed | 1.10.0 | Audio Effect (Tremolo) | 2026-09-05 |
| O-AnalogSaturation | 📦 Installed | 1.5.0 | Audio Effect (Saturation) |  2026-09-04 |
| O-Marimba | 📦 Installed | 1.14.0 | Synth (Physical Model) | 2026-08-31 |
| O-Comp | 📦 Installed | 1.9.0 | Audio Effect (Compressor) |  2026-09-04 |
| O-AnalogEQ | 📦 Installed | 1.5.0 | Audio Effect (EQ) | 2026-09-05 |
| O-DigiDelay | 📦 Installed | 1.6.0 | Audio Effect (Delay) | 2026-09-05 |
| O-SimpleReverb | 📦 Installed | 1.9.0 | Audio Effect (Reverb) | 2026-09-05 |
| O-Polystutter | 📦 Installed | 1.14.2 | Audio Effect (Beat Repeater) | 2026-08-31 | **Packaged:** [O-Polystutter-OuariconAudio.pkg](plugins/O-Polystutter/dist/) (4.5 MB, Signed, v1.12.4 — repackage pending) |
| O-Lyrica | 📦 Installed | 2.5.1 | Synth (Physical Modeling Harp) | 2026-09-06 |
| O-MultiBandCompressor | 📦 Installed | 1.12.1 | Audio Effect (Dynamics) | 2026-09-07 |
| O-Bass | 📦 Installed | 1.7.0 | Audio Effect (Bass Enhancer) |  2026-09-04 |
| O-IntonationPad | 📦 Installed | 2.10.0 | Synth (Wavetable Pad) | 2026-09-05 |
| O-Detune | 📦 Installed | 1.9.0 | Audio Effect (Detuning) | 2026-09-05 |
| O-Freeze | 📦 Installed | 2.5.0 | Audio Effect (Granular Freeze) |  2026-09-04 |
| O-FreqPulse | 📦 Installed | 1.19.0 | Audio Effect (Spectral Sequencer) | 2026-08-31 |
| O-SpectralShaper | 📦 Installed | 1.8.0 | Audio Effect (Spectral Transient Shaper) | 2026-09-05 |
| O-GrainScatter | 📦 Installed | 2.8.0 | Audio Effect (Granular Stutter Engine) | 2026-09-07 |
| O-Chorus | 📦 Installed | 1.6.3 | Audio Effect (Chorus) | 2026-09-06 |
| O-Orbit | 📦 Installed | 1.2.2 | Audio Effect (Spatial Orbiter) | 2026-08-31 |
| O-TextureForge | 📦 Installed | 1.4.0 | Instrument (Concatenative Synth) | 2026-09-05 |
| O-Texture | 📦 Installed | 0.5.0 | Instrument/Effect (Neural Texture Synth) |  2026-09-04 |
| O-Prism | 📦 Installed | 1.24.0 | Synth (Microtonal Wavetable) |  2026-09-04 |
| O-Strata | 🚧 Stage 0 | - | Synth (3D-Geometry Microtonal Wavetable) | 2026-09-07 |
| O-Gain | 📦 Installed | 1.4.0 | Audio Effect (Gain Staging Utility) | 2026-09-05 |
| O-Formant | 📦 Installed | 1.29.0 | Synth (Physical Model Vocal) | 2026-08-31 |
| O-Bowed | 📦 Installed | 1.9.0 | Synth (Physical Model Bowed String) | 2026-09-07 |
| O-Reed | 📦 Installed | 1.6.0 | Synth (Physical Modeling Reed Wind) | 2026-09-07 |
| O-Wind | 📦 Installed | 1.21.0 | Synth (Physical Model Flute) | 2026-09-07 |
| O-Contrabass | 📦 Installed | 1.10.0 | Synth (Physical Model Bowed Bass) | 2026-09-07 |
| O-Bassoon | 📦 Installed | 1.5.0 | Synth (Physical Model Bassoon) | 2026-09-06 |
| O-MicrotonalSampler | 📦 Installed | 1.27.0 | Synth (Microtonal Sampler) | 2026-09-04 |
| O-simpleFM | 📦 Installed | 1.5.0 | Synth (Pedagogical 2-Op FM) | 2026-09-06 |
| O-simpleAdditive | 📦 Installed | 1.2.0 | Synth (Pedagogical Additive + Wavetable) | 2026-09-01 |
| O-simpleGrain | 📦 Installed | 1.4.2 | Synth (Pedagogical Granular) | 2026-08-31 |
| O-simpleSubtractive | 📦 Installed | 1.4.0 | Synth (Pedagogical Subtractive) | 2026-09-01 |
| O-simpleSampler | ✅ Working | 1.4.3 | Synth (Pedagogical Sampler) | 2026-08-31 |
| O-simpleBeatmaker | 📦 Installed | 1.3.0 | Synth (Pedagogical Step-Sequencer Drum Machine) | 2026-09-06 |
| O-simplePhysicalModelSynth | 📦 Installed | 1.3.0 | Synth (Pedagogical Physical Modeling) | 2026-09-06 |
| O-ReverseDelay | 📦 Installed | 1.12.0 | Audio Effect (Granular Reverse Delay) | 2026-08-31 |
| O-Octagon | 📦 Installed | 1.12.0 | Audio Effect (8-Channel DBAP Spatializer) | 2026-09-04 |
| O-Bitrot | 📦 Installed | 1.16.0 | Audio Effect (Broken-Media Degradation) | 2026-09-04 |
| O-Tapestop | 📦 Installed | 1.7.0 | Audio Effect (Tapestop/Start + Scratch/Continuous Varispeed) | 2026-09-06 |
| O-Emulator | 📦 Installed | 1.4.0 | Audio Effect (Retro Console Emulation) | 2026-09-05 |

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
