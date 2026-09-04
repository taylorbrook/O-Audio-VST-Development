# 260903-ukp — the before → after sheet

**Read this, then reply.** Task 1 is committed (`19496d24`): the glossary root moved,
O-Gain shipped as the tracer at **1.3.2 → 1.3.3**, five gates green, auval clean.
The other 42 plugins are still on the old wording *on purpose* — this read is what
authorizes them, and what makes `reviewed: true` legitimate for every prose body below.

Every row is numbered. Reply **"approved"**, or edit any row by number
(*"row 14: …"*); anything you edit ships in your words, not mine.

---

## What the phrase becomes

`aide au survol` is **feminine singular**. `infobulles` is **feminine plural**. That is
why this is not a find-and-replace: every sentence around it has to be re-agreed.

| before | after |
|---|---|
| `cette aide au survol` | `ces infobulles` |
| `l'aide au survol` | `les infobulles` |
| `de l'aide au survol` | `des infobulles` |
| `d'aide au survol` | `des infobulles` — but bare `d'infobulles` after `ni …, ni …` |
| `toute l'aide au survol` | `toutes les infobulles` |
| `chaque aide au survol` | `chaque infobulle` ← the one singular case |
| `Une fois désactivée` | `Une fois désactivées` |
| `si l'aide au survol s'affiche` | `si les infobulles s'affichent` |
| `cette aide au survol est rédigée` | `ces infobulles sont rédigées` |
| `son affichage ou non` (back-ref) | `leur affichage ou non` |

## The scan, corrected

| the plan said | the live grep says |
|---|---|
| 231 occurrences | **230**, in 43 files |
| ~96 distinct lines | **96** — but **17 of them are source COMMENTS**, not strings. 79 are runtime. |
| 4 stale width comments (index.html / styles.css) | **8** — the plan's 4 plus four more *inside i18n.js*: O-FreqPulse:56, O-Lyrica:67, O-Polystutter:69, and O-Polystutter:555's trailing `// 71.77`. O-Gain's (i18n.js:88 + index.html:543) are already done. |
| adjacent finding: 6 prose sites + 2 comments | **13 prose sites + 2 comments** — see §5. One (O-Prism) hides across a `+` continuation and no single-line grep finds it. |
| "two source comments in O-Bass and O-Tremolo" are the only permitted grep hits | **17** comment lines in **11** plugins carried the phrase (**16 in 10** now that O-Gain is done). See §4 — this needs a decision, because Task 3's zero-occurrence proof does not exclude them. |

---

## §1 — The settled shapes (18 distinct strings, 150 of the 207 remaining occurrences)

**1.**  ×36 — O-AnalogEQ, O-AnalogSaturation, O-Bass, O-Bassoon, O-Bells, O-Bitrot, O-Bowed, O-Chorus, O-Comp, O-Contrabass, O-Detune, O-DigiDelay, O-Emulator, O-Formant, O-Freeze, O-FreqPulse, O-GrainScatter, O-IntonationPad, O-Lyrica, O-Marimba, O-MicrotonalSampler, O-MultiBandCompressor, O-Octagon, O-Orbit, O-Polystutter, O-Prism, O-Reed, O-ReverseDelay, O-SimpleReverb, O-SpectralShaper, O-Tapestop, O-Texture, O-TextureForge, O-Tremolo, O-Wind, O-simpleSampler
```
- fr: { t: 'Aide au survol',
+ fr: { t: 'Infobulles',
```

**2.**  ×25 — O-AnalogEQ, O-AnalogSaturation, O-Bass, O-Bassoon, O-Bells, O-Bowed, O-Comp, O-Detune, O-DigiDelay, O-Emulator, O-Formant, O-Freeze, O-GrainScatter, O-Marimba, O-MicrotonalSampler, O-MultiBandCompressor, O-Prism, O-Reed, O-ReverseDelay, O-SimpleReverb, O-SpectralShaper, O-Texture, O-TextureForge, O-Tremolo, O-Wind
```
- 'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Aide au survol', reviewed: true } },
+ 'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true } },
```

**3.**  ×23 — O-AnalogEQ, O-AnalogSaturation, O-Bass, O-Bassoon, O-Bells, O-Bowed, O-Chorus, O-Comp, O-Detune, O-DigiDelay, O-Emulator, O-Formant, O-Freeze, O-GrainScatter, O-MicrotonalSampler, O-Prism, O-Reed, O-ReverseDelay, O-SimpleReverb, O-Texture, O-TextureForge, O-Tremolo, O-Wind
```
- b: 'Active ou désactive cette aide au survol. Une fois désactivée, seuls '
+ b: 'Active ou désactive ces infobulles. Une fois désactivées, seuls '
```

**4.**  ×21 — O-AnalogEQ, O-AnalogSaturation, O-Bass, O-Bassoon, O-Bowed, O-Comp, O-Detune, O-DigiDelay, O-Emulator, O-Formant, O-Freeze, O-GrainScatter, O-MicrotonalSampler, O-Prism, O-Reed, O-ReverseDelay, O-SimpleReverb, O-Texture, O-TextureForge, O-Tremolo, O-Wind
```
- 'aria.helpToggle': { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver l’aide au survol', reviewed: true } },
+ 'aria.helpToggle': { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver les infobulles', reviewed: true } },
```

**5.**  ×7 — O-Bitrot, O-Lyrica, O-MultiBandCompressor, O-Orbit, O-Polystutter, O-SpectralShaper, O-Tapestop
```
- fr: { t: 'Activer ou désactiver l’aide au survol', reviewed: true } },
+ fr: { t: 'Activer ou désactiver les infobulles', reviewed: true } },
```

**6.**  ×6 — O-FreqPulse, O-IntonationPad, O-Lyrica, O-Marimba, O-Polystutter, O-SpectralShaper
```
- b: 'Active ou désactive cette aide au survol. Une fois désactivée, seuls l’engrenage et ce commutateur continuent de s’expliquer.',
+ b: 'Active ou désactive ces infobulles. Une fois désactivées, seuls l’engrenage et ce commutateur continuent de s’expliquer.',
```

**7.**  ×6 — O-simpleAdditive, O-simpleBeatmaker, O-simpleFM, O-simpleGrain, O-simplePhysicalModelSynth, O-simpleSubtractive
```
- fr: { t: "Aide au survol",
+ fr: { t: "Infobulles",
```

**8.**  ×4 — O-IntonationPad, O-Lyrica, O-Polystutter, O-SpectralShaper
```
- b: 'Choisir la langue de cette interface et l’affichage de l’aide au survol. Les deux choix sont conservés avec la session.',
+ b: 'Choisir la langue de cette interface et l’affichage des infobulles. Les deux choix sont conservés avec la session.',
```

**9.**  ×4 — O-simpleBeatmaker, O-simpleFM, O-simpleGrain, O-simplePhysicalModelSynth
```
- fr: { t: "Activer ou désactiver l’aide au survol", reviewed: true },
+ fr: { t: "Activer ou désactiver les infobulles", reviewed: true },
```

**10.**  ×2 — O-Contrabass, O-Orbit
```
- b: "Choisir la langue de l’interface et activer ou désactiver l’aide au survol. Les deux choix sont conservés avec la session.",
+ b: "Choisir la langue de l’interface et activer ou désactiver les infobulles. Les deux choix sont conservés avec la session.",
```

**11.**  ×2 — O-simpleGrain, O-simpleSampler
```
- b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles\u00a0; les valeurs affichées et les deux menus déroulants restent en anglais.",
+ b: "La langue des libellés de cette page et de ces infobulles. L’anglais et le français sont disponibles\u00a0; les valeurs affichées et les deux menus déroulants restent en anglais.",
```

**12.**  ×2 — O-Comp, O-DigiDelay
```
- b: "Ouvre le panneau qui règle la langue de cette interface. Il ne contient rien d’autre : les libellés de cette page et cette aide au survol changent avec elle, et le choix est conservé avec la session — un projet se rouvre dans la langue où il a été enregistré.",
+ b: "Ouvre le panneau qui règle la langue de cette interface. Il ne contient rien d’autre : les libellés de cette page et ces infobulles changent avec elle, et le choix est conservé avec la session — un projet se rouvre dans la langue où il a été enregistré.",
```

**13.**  ×2 — O-Bitrot, O-Octagon
```
- b: 'Choisir la langue de cette aide au survol et activer ou désactiver cette aide. Les deux choix sont conservés avec la session.',
+ b: 'Choisir la langue de ces infobulles et les activer ou les désactiver. Les deux choix sont conservés avec la session.',
```

**14.**  ×2 — O-Bitrot, O-ReverseDelay
```
- b: 'La langue de cette aide au survol et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées et les noms de préréglages restent en anglais.',
+ b: 'La langue de ces infobulles et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées et les noms de préréglages restent en anglais.',
```

**15.**  ×2 — O-FreqPulse, O-Polystutter
```
- b: 'La langue de cette aide au survol et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées, les divisions rythmiques et les noms de préréglages restent en anglais.',
+ b: 'La langue de ces infobulles et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées, les divisions rythmiques et les noms de préréglages restent en anglais.',
```

**16.**  ×2 — O-IntonationPad, O-Marimba
```
- b: 'La langue de cette aide au survol et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées, les noms de notes, les noms de gammes et les noms de préréglages restent en anglais.',
+ b: 'La langue de ces infobulles et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées, les noms de notes, les noms de gammes et les noms de préréglages restent en anglais.',
```

**17.**  ×2 — O-Bassoon, O-Reed
```
- b: 'Ouvre le panneau qui règle la langue de cette interface. Il ne contient rien d’autre : les libellés de cette page et cette aide au survol changent avec elle, et le choix est conservé avec la session — un projet se rouvre dans la langue où il a été enregistré.',
+ b: 'Ouvre le panneau qui règle la langue de cette interface. Il ne contient rien d’autre : les libellés de cette page et ces infobulles changent avec elle, et le choix est conservé avec la session — un projet se rouvre dans la langue où il a été enregistré.',
```

**18.**  ×2 — O-simpleAdditive, O-simpleSubtractive
```
- en: { t: 'Toggle hover help' }, fr: { t: "Activer ou désactiver l’aide au survol", reviewed: true },
+ en: { t: 'Toggle hover help' }, fr: { t: "Activer ou désactiver les infobulles", reviewed: true },
```

---

## §2 — The one-off bodies (57 strings, one plugin each)

These are the real work: prose unique to one plugin, read individually rather than pattern-matched.

**19.**  O-Bells
```
- 'aria.helpToggle':      { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver l’aide au survol', reviewed: true } },
+ 'aria.helpToggle':      { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver les infobulles', reviewed: true } },
```

**20.**  O-IntonationPad
```
- 'aria.helpToggle':     { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver l’aide au survol', reviewed: true } },
+ 'aria.helpToggle':     { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver les infobulles', reviewed: true } },
```

**21.**  O-FreqPulse
```
- 'aria.helpToggle':    { en: { t: 'Toggle hover help' },     fr: { t: 'Activer ou désactiver l’aide au survol', reviewed: true } },
+ 'aria.helpToggle':    { en: { t: 'Toggle hover help' },     fr: { t: 'Activer ou désactiver les infobulles', reviewed: true } },
```

**22.**  O-Marimba
```
- 'aria.helpToggle':  { en: { t: 'Toggle hover help' },      fr: { t: 'Activer ou désactiver l’aide au survol', reviewed: true } },
+ 'aria.helpToggle':  { en: { t: 'Toggle hover help' },      fr: { t: 'Activer ou désactiver les infobulles', reviewed: true } },
```

**23.**  O-simpleSampler
```
- 'aria.helpToggle': { en: { t: 'Toggle hover help' },   fr: { t: "Activer ou désactiver l’aide au survol", reviewed: true } },
+ 'aria.helpToggle': { en: { t: 'Toggle hover help' },   fr: { t: "Activer ou désactiver les infobulles", reviewed: true } },
```

**24.**  O-Chorus
```
- 'aria.helpToggle': { en: { t: 'Toggle hover help' },  fr: { t: 'Activer ou désactiver l’aide au survol', reviewed: true },
+ 'aria.helpToggle': { en: { t: 'Toggle hover help' },  fr: { t: 'Activer ou désactiver les infobulles', reviewed: true },
```

**25.**  O-Lyrica
```
- 'label.hoverHelp':      { en: { t: 'Hover help' }, fr: { t: 'Aide au survol', reviewed: true } },
+ 'label.hoverHelp':      { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true } },
```

**26.**  O-FreqPulse
```
- 'label.hoverHelp':    { en: { t: 'Hover help' }, fr: { t: 'Aide au survol', reviewed: true } },
+ 'label.hoverHelp':    { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true } },
```

**27.**  O-Tapestop
```
- 'label.hoverHelp':   { en: { t: 'Hover help' },  fr: { t: 'Aide au survol', reviewed: true } },
+ 'label.hoverHelp':   { en: { t: 'Hover help' },  fr: { t: 'Infobulles', reviewed: true } },
```

**28.**  O-IntonationPad
```
- 'label.hoverHelp':   { en: { t: 'Hover help' }, fr: { t: 'Aide au survol', reviewed: true } },
+ 'label.hoverHelp':   { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true } },
```

**29.**  O-Polystutter
```
- 'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Aide au survol', reviewed: true } },  // 71.77
+ 'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true } },  // 71.77
```

**30.**  O-Chorus
```
- 'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Aide au survol', reviewed: true }, 'zh-Hans': { t: '悬停帮助', reviewed: 'bt' } },
+ 'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true }, 'zh-Hans': { t: '悬停帮助', reviewed: 'bt' } },
```

**31.**  O-Chorus
```
- + 'de l’interface et le commutateur d’aide au survol. Appuyez sur Échap pour '
+ + 'de l’interface et le commutateur des infobulles. Appuyez sur Échap pour '
```

**32.**  O-Detune
```
- b: "Bascule chaque légende, chaque nom accessible et chaque aide au survol de cette page entre English et Français. Les valeurs affichées ne changent pas : ce sont des nombres et des unités, qui ne se traduisent pas. Le choix est enregistré avec la session.",
+ b: "Bascule chaque légende, chaque nom accessible et chaque infobulle de cette page entre English et Français. Les valeurs affichées ne changent pas : ce sont des nombres et des unités, qui ne se traduisent pas. Le choix est enregistré avec la session.",
```

**33.**  O-SimpleReverb
```
- b: "Bascule toutes les légendes, tous les noms accessibles et toute l’aide au survol de cette page entre l’anglais et le français. Le changement est immédiat et il est enregistré avec l’état du plugin : il revient donc avec la session. Deux réglages : English et Français.",
+ b: "Bascule toutes les légendes, tous les noms accessibles et toutes les infobulles de cette page entre l’anglais et le français. Le changement est immédiat et il est enregistré avec l’état du plugin : il revient donc avec la session. Deux réglages : English et Français.",
```

**34.**  O-simpleBeatmaker
```
- b: "Choisir la langue de l’interface et activer ou désactiver cette aide au survol. La langue est conservée avec la session : un projet se rouvre dans la langue où vous l’avez laissé ; le réglage de l’aide est conservé sur cet ordinateur.",
+ b: "Choisir la langue de l’interface et activer ou désactiver ces infobulles. La langue est conservée avec la session : un projet se rouvre dans la langue où vous l’avez laissé ; le réglage des infobulles est conservé sur cet ordinateur.",
```

**35.**  O-simpleSubtractive
```
- b: "Choisir la langue de l’interface et activer ou désactiver cette aide au survol. La langue est conservée avec la session ; le réglage de l’aide est conservé sur cet ordinateur.",
+ b: "Choisir la langue de l’interface et activer ou désactiver ces infobulles. La langue est conservée avec la session ; le réglage des infobulles est conservé sur cet ordinateur.",
```

**36.**  O-simpleGrain
```
- b: "Choisir la langue de l’interface et l’affichage de l’aide au survol. La langue est conservée avec la session\u00a0; l’état de l’aide est conservé sur cet ordinateur.",
+ b: "Choisir la langue de l’interface et l’affichage des infobulles. La langue est conservée avec la session\u00a0; l’état des infobulles est conservé sur cet ordinateur.",
```

**37.**  O-simpleSampler
```
- b: "Choisir la langue de l’interface et l’affichage de l’aide au survol. Les deux choix sont conservés avec la session.",
+ b: "Choisir la langue de l’interface et l’affichage des infobulles. Les deux choix sont conservés avec la session.",
```

**38.**  O-simpleFM
```
- b: "Choisissez la langue de l’interface et activez ou désactivez cette aide au survol. La langue est conservée avec la session : un projet se rouvre dans la langue où il a été enregistré ; le réglage de l’aide est conservé sur cet ordinateur.",
+ b: "Choisissez la langue de l’interface et activez ou désactivez ces infobulles. La langue est conservée avec la session : un projet se rouvre dans la langue où il a été enregistré ; le réglage des infobulles est conservé sur cet ordinateur.",
```

**39.**  O-simpleAdditive
```
- b: "Choisissez la langue de l’interface et activez ou désactivez cette aide au survol. La langue est conservée avec la session ; le réglage de l’aide est conservé sur cet ordinateur.",
+ b: "Choisissez la langue de l’interface et activez ou désactivez ces infobulles. La langue est conservée avec la session ; le réglage des infobulles est conservé sur cet ordinateur.",
```

**40.**  O-simplePhysicalModelSynth
```
- b: "Choisissez la langue de l’interface, et si l’aide au survol s’affiche. La langue est conservée avec la session ; l’état de l’aide est conservé sur cet ordinateur.",
+ b: "Choisissez la langue de l’interface, et si les infobulles s’affichent. La langue est conservée avec la session ; l’état des infobulles est conservé sur cet ordinateur.",
```

**41.**  O-Comp
```
- b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles. Les valeurs affichées sous les boutons rotatifs et les noms de préréglages restent en anglais, pour que la page et l’hôte s’accordent sur le nom d’un réglage.",
+ b: "La langue des libellés de cette page et de ces infobulles. L’anglais et le français sont disponibles. Les valeurs affichées sous les boutons rotatifs et les noms de préréglages restent en anglais, pour que la page et l’hôte s’accordent sur le nom d’un réglage.",
```

**42.**  O-Emulator
```
- b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles. Les valeurs affichées, les cinq noms de consoles et les noms de préréglages restent en anglais pour que la page et l’hôte s’accordent.",
+ b: "La langue des libellés de cette page et de ces infobulles. L’anglais et le français sont disponibles. Les valeurs affichées, les cinq noms de consoles et les noms de préréglages restent en anglais pour que la page et l’hôte s’accordent.",
```

**43.**  O-DigiDelay
```
- b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles. Les valeurs affichées, les divisions rythmiques et les noms de préréglages restent en anglais, pour que la page et l’hôte nomment toujours la même chose de la même façon.",
+ b: "La langue des libellés de cette page et de ces infobulles. L’anglais et le français sont disponibles. Les valeurs affichées, les divisions rythmiques et les noms de préréglages restent en anglais, pour que la page et l’hôte nomment toujours la même chose de la même façon.",
```

**44.**  O-Texture
```
- b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles. Les valeurs affichées, les six noms de sources et les deux noms de modes restent en anglais pour que la page et la voie d’automation de l’hôte s’accordent sur un même réglage.",
+ b: "La langue des libellés de cette page et de ces infobulles. L’anglais et le français sont disponibles. Les valeurs affichées, les six noms de sources et les deux noms de modes restent en anglais pour que la page et la voie d’automation de l’hôte s’accordent sur un même réglage.",
```

**45.**  O-simpleAdditive
```
- b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles ; les valeurs affichées et les deux menus déroulants restent en anglais.",
+ b: "La langue des libellés de cette page et de ces infobulles. L’anglais et le français sont disponibles ; les valeurs affichées et les deux menus déroulants restent en anglais.",
```

**46.**  O-Orbit
```
- b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles ; les valeurs affichées et les noms de préréglages restent en anglais.",
+ b: "La langue des libellés de cette page et de ces infobulles. L’anglais et le français sont disponibles ; les valeurs affichées et les noms de préréglages restent en anglais.",
```

**47.**  O-simpleSubtractive
```
- b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles ; les valeurs affichées et les quatre menus déroulants restent en anglais.",
+ b: "La langue des libellés de cette page et de ces infobulles. L’anglais et le français sont disponibles ; les valeurs affichées et les quatre menus déroulants restent en anglais.",
```

**48.**  O-Contrabass
```
- b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles ; les valeurs affichées, les noms de notes et les noms de préréglages restent en anglais.",
+ b: "La langue des libellés de cette page et de ces infobulles. L’anglais et le français sont disponibles ; les valeurs affichées, les noms de notes et les noms de préréglages restent en anglais.",
```

**49.**  O-simpleFM
```
- b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles ; les valeurs affichées, les noms de préréglages et les boutons de leçon restent en anglais.",
+ b: "La langue des libellés de cette page et de ces infobulles. L’anglais et le français sont disponibles ; les valeurs affichées, les noms de préréglages et les boutons de leçon restent en anglais.",
```

**50.**  O-simplePhysicalModelSynth
```
- b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles ; les valeurs affichées, les noms de préréglages et les trois menus déroulants restent en anglais.",
+ b: "La langue des libellés de cette page et de ces infobulles. L’anglais et le français sont disponibles ; les valeurs affichées, les noms de préréglages et les trois menus déroulants restent en anglais.",
```

**51.**  O-simpleBeatmaker
```
- b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles ; les valeurs affichées, les noms des six préréglages de leçon et les numéros de note MIDI restent en anglais.",
+ b: "La langue des libellés de cette page et de ces infobulles. L’anglais et le français sont disponibles ; les valeurs affichées, les noms des six préréglages de leçon et les numéros de note MIDI restent en anglais.",
```

**52.**  O-Emulator
```
- b: "Ouvre le panneau qui règle la langue de cette interface. Il ne contient rien d’autre : les libellés de cette page et cette aide au survol changent avec elle, et le choix est conservé avec la session — un projet se rouvre dans la langue dans laquelle il a été enregistré.",
+ b: "Ouvre le panneau qui règle la langue de cette interface. Il ne contient rien d’autre : les libellés de cette page et ces infobulles changent avec elle, et le choix est conservé avec la session — un projet se rouvre dans la langue dans laquelle il a été enregistré.",
```

**53.**  O-Texture
```
- b: "Ouvre le panneau qui règle la langue de cette interface. Il ne contient rien d’autre : les libellés de cette page et cette aide au survol changent avec elle, et le choix est enregistré avec le projet — une session se rouvre donc dans la langue enregistrée.",
+ b: "Ouvre le panneau qui règle la langue de cette interface. Il ne contient rien d’autre : les libellés de cette page et ces infobulles changent avec elle, et le choix est enregistré avec le projet — une session se rouvre donc dans la langue enregistrée.",
```

**54.**  O-MultiBandCompressor
```
- b: 'Active ou désactive cette aide au survol pour toutes les commandes. Lorsqu’elle est désactivée, ce bouton continue de s’expliquer, afin de pouvoir toujours la réactiver. Ce réglage est partagé par toutes les instances sur cette machine.',
+ b: 'Active ou désactive ces infobulles pour toutes les commandes. Lorsqu’elles sont désactivées, ce bouton continue de s’expliquer, afin de pouvoir toujours les réactiver. Ce réglage est partagé par toutes les instances sur cette machine.',
```

**55.**  O-Bass
```
- b: 'Bascule chaque libellé, chaque nom accessible et l’aide au survol de cette '
+ b: 'Bascule chaque libellé, chaque nom accessible et chaque infobulle de cette '
```

**56.**  O-Wind
```
- b: 'Bascule chaque légende et chaque phrase d’aide au survol entre l’anglais et le français. Les valeurs numériques gardent leur format anglais, et l’onglet Accord reste en anglais dans les deux langues.',
+ b: 'Bascule chaque légende et chaque infobulle entre l’anglais et le français. Les valeurs numériques gardent leur format anglais, et l’onglet Accord reste en anglais dans les deux langues.',
```

**57.**  O-MicrotonalSampler
```
- b: 'Bascule toutes les légendes, tous les boutons et toute l’aide au survol de cette page entre l’anglais et le français. Les valeurs affichées, les noms de systèmes d’accord, les noms de notes et les noms de fichiers de préréglages restent inchangés.',
+ b: 'Bascule toutes les légendes, tous les boutons et toutes les infobulles de cette page entre l’anglais et le français. Les valeurs affichées, les noms de systèmes d’accord, les noms de notes et les noms de fichiers de préréglages restent inchangés.',
```

**58.**  O-Tapestop
```
- b: 'Choisir la langue de ce plugin et activer ou désactiver l’aide au survol. Les deux choix sont conservés avec la session.',
+ b: 'Choisir la langue de ce plugin et activer ou désactiver les infobulles. Les deux choix sont conservés avec la session.',
```

**59.**  O-ReverseDelay
```
- b: 'Choisissez la langue de cette aide au survol. Le choix est conservé avec la session.',
+ b: 'Choisissez la langue de ces infobulles. Le choix est conservé avec la session.',
```

**60.**  O-FreqPulse
```
- b: 'Choisissez la langue de cette interface et l’affichage ou non de l’aide au survol. Les deux choix sont conservés avec la session.',
+ b: 'Choisissez la langue de cette interface et l’affichage ou non des infobulles. Les deux choix sont conservés avec la session.',
```

**61.**  O-Marimba
```
- b: 'Choisissez la langue de cette interface et si l’aide au survol s’affiche. Les deux choix sont conservés avec la session.',
+ b: 'Choisissez la langue de cette interface et si les infobulles s’affichent. Les deux choix sont conservés avec la session.',
```

**62.**  O-TextureForge
```
- b: 'Choisit la langue des libellés et de cette aide au survol. Les valeurs affichées restent en anglais. Le choix est enregistré avec la session, pas avec un préréglage. English ou Français.',
+ b: 'Choisit la langue des libellés et de ces infobulles. Les valeurs affichées restent en anglais. Le choix est enregistré avec la session, pas avec un préréglage. English ou Français.',
```

**63.**  O-Tremolo
```
- b: 'Choisit la langue du texte de l’interface et de cette aide au survol. '
+ b: 'Choisit la langue du texte de l’interface et de ces infobulles. '
```

**64.**  O-Octagon
```
- b: 'La langue dans laquelle cette aide au survol est rédigée. L’anglais et le français sont disponibles ; les libellés de la page changent avec elle, mais les nombres et les symboles d’unité restent inchangés.',
+ b: 'La langue dans laquelle ces infobulles sont rédigées. L’anglais et le français sont disponibles ; les libellés de la page changent avec elle, mais les nombres et les symboles d’unité restent inchangés.',
```

**65.**  O-Tapestop
```
- b: 'La langue de cette aide au survol et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées restent en anglais.',
+ b: 'La langue de ces infobulles et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées restent en anglais.',
```

**66.**  O-Lyrica
```
- b: 'La langue de cette aide au survol et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées, les choix des menus, les noms de gammes et les noms de préréglages restent en anglais.',
+ b: 'La langue de ces infobulles et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées, les choix des menus, les noms de gammes et les noms de préréglages restent en anglais.',
```

**67.**  O-SpectralShaper
```
- b: 'La langue de cette aide au survol et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées, les noms de préréglages et les intitulés de catégories restent en anglais.',
+ b: 'La langue de ces infobulles et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées, les noms de préréglages et les intitulés de catégories restent en anglais.',
```

**68.**  O-AnalogSaturation
```
- b: 'La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles. Les boutons de modèle et de qualité gardent leur nom anglais à dessein, afin que la page et la voie d’automation de l’hôte désignent le même réglage de la même façon.',
+ b: 'La langue des libellés de cette page et de ces infobulles. L’anglais et le français sont disponibles. Les boutons de modèle et de qualité gardent leur nom anglais à dessein, afin que la page et la voie d’automation de l’hôte désignent le même réglage de la même façon.',
```

**69.**  O-Bassoon
```
- b: 'La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont proposés. Les valeurs affichées restent des nombres et des unités dans les deux langues, et l’onglet Accord demeure en anglais : son panneau provient d’un module partagé qui n’appartient pas à ce plugin.',
+ b: 'La langue des libellés de cette page et de ces infobulles. L’anglais et le français sont proposés. Les valeurs affichées restent des nombres et des unités dans les deux langues, et l’onglet Accord demeure en anglais : son panneau provient d’un module partagé qui n’appartient pas à ce plugin.',
```

**70.**  O-Reed
```
- b: 'La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont proposés. Les valeurs affichées, les intitulés d’options des menus déroulants et les quinze repères d’instruments du pavé restent en anglais dans les deux langues, tout comme l’onglet Accord : son panneau provient d’un module partagé qui n’appartient pas à ce plugin.',
+ b: 'La langue des libellés de cette page et de ces infobulles. L’anglais et le français sont proposés. Les valeurs affichées, les intitulés d’options des menus déroulants et les quinze repères d’instruments du pavé restent en anglais dans les deux langues, tout comme l’onglet Accord : son panneau provient d’un module partagé qui n’appartient pas à ce plugin.',
```

**71.**  O-MultiBandCompressor
```
- b: 'Langue de cette aide au survol et des libellés de la page. Le choix est conservé avec la session et n’est pas transporté par les préréglages. Les valeurs affichées et les noms de préréglages restent en anglais.',
+ b: 'Langue de ces infobulles et des libellés de la page. Le choix est conservé avec la session et n’est pas transporté par les préréglages. Les valeurs affichées et les noms de préréglages restent en anglais.',
```

**72.**  O-AnalogSaturation
```
- b: 'Ouvre le panneau qui contient la langue de l’interface. C’est tout ce qu’il contient ici : ni interrupteur d’aide au survol, ni autre préférence. Le choix est enregistré avec la session, donc un projet se rouvre dans la langue où il a été enregistré.',
+ b: 'Ouvre le panneau qui contient la langue de l’interface. C’est tout ce qu’il contient ici : ni interrupteur d’infobulles, ni autre préférence. Le choix est enregistré avec la session, donc un projet se rouvre dans la langue où il a été enregistré.',
```

**73.**  O-MultiBandCompressor
```
- b: 'Préférences d’interface — la langue de cette aide au survol, et son affichage ou non.',
+ b: 'Préférences d’interface — la langue de ces infobulles, et leur affichage ou non.',
```

**74.**  O-Contrabass
```
- fr: { t: "Activer ou désactiver l’aide au survol", reviewed: true } },
+ fr: { t: "Activer ou désactiver les infobulles", reviewed: true } },
```

**75.**  O-Octagon
```
- fr: { t: 'Langue de l’aide au survol', reviewed: true } },
+ fr: { t: 'Langue des infobulles', reviewed: true } },
```

---

## §3 — The bare back-references

These clauses contain **no occurrence of the search phrase**. A regex pass leaves them
behind pointing at an antecedent that no longer exists — *le réglage de l'aide* after
*l'aide au survol* has become *les infobulles*. They are already folded into the rows
above; listed here so you can see the trap was checked, not assumed.

| # | rows | the clause, before | after | why it moves |
|---|---|---|---|---|
| B1 | rows **3**, **6** | `Une fois désactivée,` | `Une fois désactivées,` | the participle agrees with the new plural subject; on the same line as the phrase but a separate agreement |
| B2 | row **13** (O-Bitrot, O-Octagon) | `…et activer ou désactiver cette aide.` | `…et les activer ou les désactiver.` | *cette aide* is a second, bare mention of the same thing in the same sentence — repeating *ces infobulles* reads as two objects, so it becomes a pronoun |
| B3 | rows **34, 35, 38, 39** (O-simpleBeatmaker, O-simpleSubtractive, O-simpleFM, O-simpleAdditive) | `le réglage de l'aide est conservé sur cet ordinateur` | `le réglage des infobulles est conservé sur cet ordinateur` | *est conservé* stays singular — the subject is *le réglage*, not the infobulles |
| B4 | rows **36, 40** (O-simpleGrain, O-simplePhysicalModelSynth) | `l'état de l'aide est conservé sur cet ordinateur` | `l'état des infobulles est conservé sur cet ordinateur` | same shape, *l'état* is the subject |
| B5 | row **54** (O-MultiBandCompressor) | `Lorsqu'elle est désactivée, … afin de pouvoir toujours **la** réactiver.` | `Lorsqu'**elles sont** désactivées, … afin de pouvoir toujours **les** réactiver.` | three agreements in one sentence, two of them pronouns with no phrase on the line |
| B6 | row **73** (O-MultiBandCompressor) | `la langue de cette aide au survol, et **son** affichage ou non.` | `la langue de ces infobulles, et **leur** affichage ou non.` | possessive back-reference |
| B7 | row **40** (O-simplePhysicalModelSynth), row **61** (O-Marimba) | `si l'aide au survol **s'affiche**` | `si les infobulles **s'affichent**` | the verb is on the same line but a mechanical determiner swap strands it — this one actually bit during authoring and was caught by re-reading the output |

Rows **32** (O-Detune), **55** (O-Bass) and **56** (O-Wind) take `chaque infobulle`
— **singular** — because the sentence is a distributive list (*chaque légende, chaque
nom accessible, chaque infobulle*) and the parallelism carries more than the plural does.
That is the one place the new term appears in the singular.

---

## §4 — The 16 remaining legacy COMMENT lines (a decision the plan did not anticipate)

The plan expected **two** source comments to carry the old phrase (O-Bass, O-Tremolo).
The original scan found **17 lines across 11 plugins**; O-Gain's is already handled, leaving
**16 lines across 10 plugins** (O-Bass, O-Bitrot, O-FreqPulse, O-Lyrica, O-Polystutter ×2,
O-SpectralShaper ×3, O-Tremolo, O-simpleGrain ×2, O-simplePhysicalModelSynth ×2,
O-simpleSampler ×2). Task 3's zero-occurrence proof
(`grep -ril … --exclude=CHANGELOG.md --exclude-dir=.planning`) does **not** exclude
`i18n.js`, so every one of them would show up as a miss.

They fall into three kinds:

**(a) Four stale WIDTH notes — these must be re-measured regardless, same as the plan's four:**

```
plugins/O-FreqPulse/Resources/ui/js/i18n.js:56   //   Aide -> Aide au survol   76.55 px. The .settings-row is space-between
plugins/O-Lyrica/Resources/ui/js/i18n.js:67      //   Aide au survol (the glossary root) APPLIED at 65.89 px: .settings-row is
plugins/O-Polystutter/Source/ui/public/js/i18n.js:69   //   "Hover help" -> "Aide au survol", the glossary root, at 71.77 px in a
plugins/O-Polystutter/Source/ui/public/js/i18n.js:555  'label.hoverHelp': … },  // 71.77     <- a stale number on a LIVE line
```

**(b) Eleven HISTORY notes** recording the Stage-N settlement (*"Aide" BECAME "Aide au
survol"*, *"aide contextuelle" -> "aide au survol"*), which are true statements about
the past:

```
plugins/O-simpleSampler/Source/ui/public/js/i18n.js:84
    //     agrees with "l'aide au survol".
plugins/O-simplePhysicalModelSynth/Source/ui/public/js/i18n.js:77
    //     says "Aide au survol" — two French names for one control. Now one.
plugins/O-simpleGrain/Source/ui/public/js/i18n.js:27
    //    "l'aide au survol" (one name per control; fr unchanged, reviewed: true kept).
plugins/O-simpleGrain/Source/ui/public/js/i18n.js:101
    //    "l'aide au survol", which is also this page's own tip title for the same
plugins/O-simplePhysicalModelSynth/Source/ui/public/js/i18n.js:34
    //   "Activer ou désactiver l’aide au survol"; reviewed: true stays.
plugins/O-SpectralShaper/Resources/ui/js/i18n.js:76
    //   "Aide" BECAME "Aide au survol". The tip title and the aria-label on this
plugins/O-Bass/Source/ui/public/js/i18n.js:108
    //   "aide au survol"; both restored.
plugins/O-Tremolo/Source/ui/public/js/i18n.js:78
    //   - "aide contextuelle" -> "aide au survol" in tip.language, the settled
plugins/O-SpectralShaper/Resources/ui/js/i18n.js:77
    //   very control already said "aide au survol"; the popover row alone said
plugins/O-SpectralShaper/Resources/ui/js/i18n.js:69
    //   with "l'aide au survol". See the note above .settings-toggle in
plugins/O-simpleSampler/Source/ui/public/js/i18n.js:503
    // agree with "l'aide au survol". Through v1.4.1 this was "Oui"/"Non", and
```

**(c) One structural note** — O-Bitrot:595, a key-inventory comment
(`` `help-toggle` (Hover help / Aide au survol) ``) that is now simply wrong and should
say `Infobulles` whatever else is decided.

**What O-Gain (the tracer) did, as the proposed recipe:** the width note and the
index.html table were re-measured AND the superseded French wording was moved out of the
source entirely — the comment now says *"see CHANGELOG v1.3.3, which is where the
superseded wording is recorded; this file no longer carries it, so a repo grep for it
stays at zero."* The CHANGELOG keeps the history (it is excluded from the proof and its
history is correct); the source keeps the current state. That makes Task 3's `rc=1` a
real zero instead of a list of named exceptions.

**→ This is question 4 below.**

---

## §5 — The adjacent finding

Thirteen prose sites and two comments name this same surface something else. They would
survive this change untouched, leaving **four** French renderings of one concept where
the glossary exists to permit one. (The plan estimated 6 prose sites; the live grep finds
13, and one of them — O-Prism — hides across a `+` continuation where no single-line grep
reaches it.)

**Recommendation: fold them in.** It is 13 prose lines and 2 comments, and one concept
with four names is precisely the defect the glossary was built to close.

### aide contextuelle → infobulles (4 sites)

**A1.**  O-AnalogEQ — `Source/ui/public/js/i18n.js:405-406`
```
- b: 'Choisit la langue de la page : chaque étiquette, chaque nom accessible '
   + 'et cette aide contextuelle. Les valeurs affichées sous les boutons rotatifs '
+ b: 'Choisit la langue de la page : chaque étiquette, chaque nom accessible '
   + 'et chaque infobulle. Les valeurs affichées sous les boutons rotatifs '
```
*distributive list — takes the singular, like rows 32/55/56*

**A2.**  O-Formant — `Source/ui/public/js/i18n.js:767-768`
```
- b: 'Fait passer de l’anglais au français toutes les étiquettes, tous les titres, toutes '
   + 'les légendes de bouton et toute l’aide contextuelle de la page. Les valeurs '
+ b: 'Fait passer de l’anglais au français toutes les étiquettes, tous les titres, toutes '
   + 'les légendes de bouton et toutes les infobulles de la page. Les valeurs '
```
*`toute l’…` → `toutes les …`, matching the settled rule*

**A3.**  O-GrainScatter — `Source/ui/public/js/i18n.js:729`
```
- b: 'Choisit la langue du texte de l’interface et de cette aide contextuelle. Les '
+ b: 'Choisit la langue du texte de l’interface et de ces infobulles. Les '
```
*identical shape to row 63 (O-Tremolo), which already reads `de ces infobulles`*

**A4.**  O-Prism — `Source/ui/public/js/i18n.js:1427-1428  ← SPLIT ACROSS `+`, invisible to a line grep`
```
- b: 'Bascule d’un coup toutes les légendes, tous les titres de section et toute l’aide '
   + 'contextuelle de cette page entre l’anglais et le français, sans rouvrir le '
+ b: 'Bascule d’un coup toutes les légendes, tous les titres de section et toutes les '
   + 'infobulles de cette page entre l’anglais et le français, sans rouvrir le '
```
*the `+` boundary falls mid-phrase; found by gluing continuations before grepping*

**A5.**  O-Bells — `Resources/ui/js/i18n.js:746`
```
- b: 'Bascule chaque libellé et chaque bulle d’aide de la page entre l’anglais et le français. …'
+ b: 'Bascule chaque libellé et chaque infobulle de la page entre l’anglais et le français. …'
```
*`bulle d’aide` is the closest of the four to the new root — distributive, so singular*

**A6.**  O-Freeze — `Source/ui/public/js/i18n.js:337`
```
- b: 'La langue de ces descriptions au survol et des libellés de la page. …'
+ b: 'La langue de ces infobulles et des libellés de la page. …'
```
*becomes byte-identical to row 14 (O-Bitrot, O-ReverseDelay)*

**A7.**  O-Tapestop — `Source/ui/public/js/i18n.js:177`
```
- b: 'Active ou désactive les descriptions au survol pour toutes les commandes de cette page. Le réglage est conservé avec la session.'
+ b: 'Active ou désactive les infobulles pour toutes les commandes de cette page. Le réglage est conservé avec la session.'
```
*already plural — only the noun moves*

**A8–A13.**  O-simpleAdditive, O-simpleBeatmaker, O-simpleFM, O-simpleGrain, O-simplePhysicalModelSynth, O-simpleSubtractive — `(6 sites, one each — byte-identical apart from O-simpleGrain's `\u00a0` before the colon)`
```
- b: "Active ou désactive ces explications au survol. Le réglage est conservé sur cet ordinateur et non dans la session : il vous suit d’un projet à l’autre."
+ b: "Active ou désactive ces infobulles. Le réglage est conservé sur cet ordinateur et non dans la session : il vous suit d’un projet à l’autre."
```
*already plural; `Le réglage … est conservé` stays singular (subject is *le réglage*)*

### The two comments

```
plugins/O-Bass/Source/ui/public/js/i18n.js:107-108
    //   "Parameter values" and used "bulle d’aide" where 21 sibling plugins say
    //   "aide au survol"; both restored.

plugins/O-Tremolo/Source/ui/public/js/i18n.js:78
    //   - "aide contextuelle" -> "aide au survol" in tip.language, the settled
```
Both record the Stage-N settlement accurately. They are covered by question 4, not by
this one — if the comment policy is "the source states the current term and the CHANGELOG
carries the history", these get the same treatment as §4(b).

---

## The three questions (plus one the plan did not foresee)

**1. The wording.** Rows 1–75 above, §3's back-references, and §5 if you fold it in.
Anything you edit ships in your words. This read is what makes `reviewed: true`
legitimate for the bodies — it is the whole reason this checkpoint sits *before* the
42 edits rather than after them. The alternative is shipping at `reviewed: false` and
rebuilding all 42 plugins a second time for a metadata-only change.

**2. The adjacent finding (§5).** 13 prose sites + 2 comments render this concept as
*aide contextuelle* / *bulle d'aide* / *descriptions au survol* / *explications au
survol*. Fold them into `infobulles`, or leave them?
**Recommendation: fold.** Four names for one concept is the defect the glossary exists
to close, and folding costs 13 lines — five of which become byte-identical to rows you
have already read. `Survolez …` verb forms (6 sites) describe the ACTION and stay.

**3. The root-only glossary edit.** Already committed in Task 1 and provable:
`'hover help': ['infobulles']` with `aide au survol` **removed**, not kept as an
accepted alternate. Confirm that is what you want — it means a plugin drifting back is
a red G1 gate rather than a silent pass, and it means repo-wide `fr-lint` reads **exit
2 / 117 findings / 43 of 43 plugins** until the last batch of Task 2 lands.

**4. (new) The 17 legacy comment lines — §4.** Task 3's zero-occurrence proof does not
exclude `i18n.js`, so these decide whether that gate reads a clean `rc=1` or a list of
named exceptions.
**Recommendation: the O-Gain recipe** — re-measure the four stale width notes, correct
O-Bitrot's key inventory, and rewrite the nine history notes so the source states the
current term while the CHANGELOG (excluded from the proof, and correct as history)
carries what it replaced. Reply *"comments: O-Gain recipe"*, or *"comments: leave the
history notes"* and they will be named explicitly in the SUMMARY as the only permitted
grep hits.

---

## What Task 1 already proved

| | |
|---|---|
| commit | `19496d24` — path-scoped: `scripts/i18n-fr-glossary.js`, the prose companion, `plugins/O-Gain` |
| O-Gain | **1.3.2 → 1.3.3**; installed AU and VST3 `Info.plist` both read `1.3.3` |
| positive control, per-plugin | `i18n-fr-lint --plugin O-Gain` → **exit 2**, 3 G1 findings, glossary untouched by any plugin edit |
| positive control, repo-wide | `i18n-fr-lint` → **exit 2**, **117 G1 findings across 43 / 43 plugins** |
| check-i18n (O-Gain) | exit 0, assertion [16] live |
| i18n-fr-lint (O-Gain, after) | exit 0 |
| check-ui-labels (O-Gain) | exit 0 — 0 elements moved, both languages, 28/28 labels seen |
| boot-all-uis --strict-tips | exit 0 — **0 DEAD**, 0 late |
| ui_tip_render_check | **n/a** — O-Gain has no such file |
| auval | `auval -v aufx OGan OuDv` → **AU VALIDATION SUCCEEDED** |
| check-i18n repo-wide | exit 0, unreviewed French **TOTAL = 0** |
| width table | re-measured, not scaled: `Infobulles` **49.69 px** vs the old **70.22**; the fr hover-help row falls 132.22 → **111.69** in a 154 px content box and goes from widest to narrowest. The `.settings-popover` width **pin is unchanged and stays** |

---

## Appendix — the substitution table, as executed

O-Gain was rewritten with this table and Task 2 will apply the same one. Whole-sentence
rules run **first**: a determiner rule that fires early strands the verb it was supposed
to agree — `si les infobulles s'affiche` — which is exactly what happened on the first
pass and was caught by re-reading the generated output rather than by any gate.

```python
SUBS = [
 # 1. whole sentences whose VERB or PRONOUN also has to move
 ("Active ou désactive cette aide au survol. Une fois désactivée,",
  "Active ou désactive ces infobulles. Une fois désactivées,"),
 ("Active ou désactive cette aide au survol pour toutes les commandes. Lorsqu'elle est désactivée, ce bouton continue de s'expliquer, afin de pouvoir toujours la réactiver.",
  "Active ou désactive ces infobulles pour toutes les commandes. Lorsqu'elles sont désactivées, ce bouton continue de s'expliquer, afin de pouvoir toujours les réactiver."),
 ("si l'aide au survol s'affiche",            "si les infobulles s'affichent"),
 ("La langue dans laquelle cette aide au survol est rédigée",
  "La langue dans laquelle ces infobulles sont rédigées"),
 ("la langue de cette aide au survol, et son affichage ou non",
  "la langue de ces infobulles, et leur affichage ou non"),
 ("l'affichage ou non de l'aide au survol",   "l'affichage ou non des infobulles"),
 # 2. constructions where the article must NOT be re-inserted
 ("ni interrupteur d'aide au survol",         "ni interrupteur d'infobulles"),
 # 3. quantifiers (parallelism preserved)
 ("toute l'aide au survol",                   "toutes les infobulles"),
 ("chaque aide au survol",                    "chaque infobulle"),   # the ONE singular case
 ("chaque phrase d'aide au survol",           "chaque infobulle"),
 ("chaque nom accessible et l'aide au survol","chaque nom accessible et chaque infobulle"),
 # 4. determiners
 ("cette aide au survol",                     "ces infobulles"),
 ("de l'aide au survol",                      "des infobulles"),
 ("l'aide au survol",                         "les infobulles"),
 ("d'aide au survol",                         "des infobulles"),
 # 5. titles / labels, both quote styles
 ("'Aide au survol'",                         "'Infobulles'"),
 ('"Aide au survol"',                         '"Infobulles"'),
]

BACKREF = [   # clauses with NO occurrence of the search phrase
 ("le réglage de l'aide est conservé sur cet ordinateur",
  "le réglage des infobulles est conservé sur cet ordinateur"),
 ("l'état de l'aide est conservé sur cet ordinateur",
  "l'état des infobulles est conservé sur cet ordinateur"),
 ("et activer ou désactiver cette aide.",     "et les activer ou les désactiver."),
]
```

*(Apostrophes shown here as ASCII for readability — the executed table uses U+2019 `'`
throughout, and every rewritten string preserves the original's U+2019, its U+00A0 /
`\u00a0` escapes, its `« »` quoting and its `+` continuation split. Only the fragment
carrying the phrase is rewritten; the rest of each line is byte-identical.)*

**The halt rule for Task 2:** anything the live grep turns up that is not a row of this
sheet stops the batch and comes back to you. A string nobody read must not ship at
`reviewed: true`.
