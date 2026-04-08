# Stage 4: Polish - Context

## Discussion Summary

**Date:** 2026-04-06
**Participants:** User, Claude

## Requirements Confirmed

- PRESET-ALL: All 24 factory presets from BRIEF (9 Western + 9 Non-Western + 6 Sound Design)
- COMPAT-01: pluginval level 10 for both VST3 and AU
- CHANGELOG: v1.0.0 CHANGELOG.md in plugin folder
- BUILD: Build artifacts only -- no installer/packaging for v1.0

## Constraints Identified

- No OuariconPresetManager integrated yet -- need to add or implement preset system
- No deferred UI items (bore viz, Scala browser, preset browser, botanical illustration) -- all stay deferred
- `instrumentPreset` APVTS parameter exists as morph dropdown -- factory presets are separate (full parameter snapshots)
- Plugin already passes auval and pluginval L5 from Stage 2/3 -- Level 10 is the upgrade target

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Preset count | Full 24 | Complete instrument coverage from BRIEF |
| Pluginval level | 10 (strictest) | Full release confidence |
| Deferred UI | None included | Ship v1.0 clean, expand later |
| Packaging | Build artifacts only | Manual DAW testing before distribution |
| Installer | Deferred | Not needed until user satisfied with quality |

## Factory Preset Specifications

### Western Reed Instruments (9)

| Preset | Bore Character | Double Reed (Psi) | Key Settings |
|--------|---------------|-------------------|--------------|
| Bb Clarinet | 0% (cylindrical) | 0% | Reed hardness ~50%, standard bell, odd harmonics |
| Bass Clarinet | 0% (cylindrical) | 0% | Larger bore diameter, darker bell, lower register |
| Alto Saxophone | ~80% (conical) | 0% | Bright attack chiff, medium bell |
| Tenor Saxophone | ~75% (conical) | 0% | Warmer, subtone-capable, wider bore |
| Soprano Saxophone | ~85% (conical) | 0% | Narrow bore, bright, piercing |
| Baritone Saxophone | ~70% (conical) | 0% | Wide bore, deep, full bell |
| Oboe | ~80% (conical) | ~40% | Narrow bore, nasal, penetrating |
| English Horn | ~75% (conical) | ~35% | Pear-shaped bell (lowpass), mellow |
| Bassoon | ~70% (conical) | ~30% | Long narrow bore, warm, buzzy |

### Non-Western Reed Instruments (9)

| Preset | Bore Character | Double Reed (Psi) | Key Settings |
|--------|---------------|-------------------|--------------|
| Duduk | 0% (cylindrical) | ~25% | Soft, voice-like, limited range, low air noise |
| Shehnai | ~85% (conical) | ~70% | Powerful, nasal, high Psi (quadruple reed) |
| Suona | ~90% (conical) | ~50% | Metal bell, extremely loud, shrill |
| Hichiriki | Reverse bore ~40% | ~45% | Extreme pitch bending, unique timbre |
| Zurna | ~85% (conical) | ~60% | Piercing, outdoor, minimal soft dynamics |
| Piri | 0% (cylindrical) | ~35% | Significant pitch bending, no bell flare |
| Arghul | 0% (cylindrical) | 0% | Dual bore ON, drone -12 semitones, idioglot reed |
| Launeddas | 0% (cylindrical) | 0% | Dual bore ON, slight detune, triple pipe feel |
| Mijwiz | 0% (cylindrical) | 0% | Dual bore ON, drone 0 (unison beating) |

### Sound Design (6)

| Preset | Key Settings |
|--------|-------------|
| Glass Reed | Extreme reed stiffness, low damping, crystalline |
| Metal Wind | High bore losses inverted (infinite sustain), resonant, metallic |
| Impossible Bore | Reverse bore + dual bore + feedback path, alien texture |
| Breath Drone | High air noise, low reed opening, atmospheric wind |
| Giant Clarinet | Bore length extreme, sub-bass territory, cylindrical |
| Micro Reed | Tiny bore diameter, extreme high register, insect-like |

## Scope

### In Scope
- Factory preset system (24 presets with parameter snapshots)
- pluginval level 10 validation (VST3 + AU)
- auval validation
- Bug fixes surfaced by level 10 validation
- v1.0.0 CHANGELOG.md
- Build and install verification

### Out of Scope
- Bore SVG visualization (deferred)
- Scala/TUN file browser (deferred)
- Preset browser UI (deferred)
- Botanical illustration overlay (deferred)
- PKG installer / distribution packaging
- CPU benchmark (informal check only)

## Open Questions

- Will pluginval level 10 surface issues with reed ODE under rapid parameter automation?
- Does the existing `instrumentPreset` morph parameter need adjustment to align with factory presets?
- Which preset format: OuariconPresetManager (if available) or custom XML/JSON parameter snapshots?

## Next Phase

Ready for: research phase
