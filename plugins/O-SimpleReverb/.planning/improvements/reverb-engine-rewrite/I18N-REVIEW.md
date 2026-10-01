# Translations to read: tip.type, tip.decay, tip.size (v2.0.0)

The three bodies were rewritten for the new engines. The French is flagged `reviewed: false` and the
Simplified Chinese `reviewed: 'mt'` in `Source/ui/public/js/i18n.js` until you have read this page.
Titles are unchanged. After reading: flip fr to `reviewed: true` and zh-Hans to `reviewed: 'bt'`.

- French: `scripts/i18n-fr-lint.js` 0 findings for this plugin; glossary terms Taille / Déclin / Type.
- Chinese: `scripts/i18n-zh-lint.js` 0 findings. The back-translation (en') is from a separate pass
  that was given the Chinese only (blinded ids, English and key names withheld): Claude Sonnet, fresh
  subagent, 2026-10-01. The forward pass was this session (Claude Opus).
- Rendered at 500 x 350 in all three languages (`tests/ui_tip_render_check.js`, which now sweeps
  zh-Hans too). The French tip.type is the tallest tip on the page: 233.1 px, 9.1 px clear of the
  bottom edge (it was 202.3 px).

## tip.type

**en**

> Picks one of three reverb engines and its voicing. Booth, Room, Hall and Ambient are four spaces from one room engine, each with its own echo pattern, early reflections and decay time; Plate is a plate reverb, with a faint octave shimmer that grows in the tail; Spring is a spring reverb, dark, with echoes that each sweep from low to high. Size and Decay scale what the type sets, so Size at 100 % on Booth is still a booth. Six settings: Booth, Room, Hall, Spring, Plate, Ambient.

**fr**

> Choisit l’un des trois moteurs de réverbération et sa coloration. Booth, Room, Hall et Ambient sont quatre espaces issus d’un même moteur de salle, chacun avec son propre motif d’échos, ses premières réflexions et son temps de déclin ; Plate est une réverbération à plaque, avec un léger shimmer à l’octave qui grandit dans la queue ; Spring est une réverbération à ressorts, sombre, dont chaque écho glisse du grave vers l’aigu. Taille et Déclin mettent à l’échelle ce que le type a posé : Taille à 100 % sur Booth reste une cabine. Six réglages : Booth, Room, Hall, Spring, Plate, Ambient.

**zh-Hans**

> 选择三种混响引擎之一及其音色。Booth、Room、Hall 和 Ambient 是同一个房间引擎做出的四个空间，各有自己的回声结构、早期反射和衰减时间；Plate 是板式混响，带有在尾音中逐渐增长的轻微八度微光；Spring 是弹簧混响，音色偏暗，每个回声都从低频滑向高频。尺寸与衰减缩放的是该类型已经设定好的东西，因此 Booth 上把尺寸开到 100% 仍然是一个隔音间。六档：Booth、Room、Hall、Spring、Plate、Ambient。

**en' (zh-Hans read back blind)**

> Choose one of three reverb engines and its character. Booth, Room, Hall and Ambient are four spaces made by the same room engine, each with its own echo structure, early reflections and decay time; Plate is a plate reverb, with a slight octave shimmer that gradually grows within the tail; Spring is a spring reverb, with a darker tone, each echo sliding from low frequencies to high frequencies. Size and Decay scale what has already been set for that type, so on Booth, turning Size up to 100% is still a soundproof booth. Six positions: Booth, Room, Hall, Spring, Plate, Ambient. [note: 'three reverb engines' but six types are listed; 隔音间 'soundproof booth/room']

## tip.decay

**en**

> Sets how long the tail lasts, as a multiple of the type's own decay time: at the centre of the knob, exactly 1.0x, that is about 0.4 s on Booth and 7 s on Ambient. The tail keeps its length wherever Size is set. 0.5x to 2.0x.

**fr**

> Règle la durée de la queue, en multiple du temps de déclin propre au type : au centre du bouton, soit exactement 1,0x, cela donne environ 0,4 s sur Booth et 7 s sur Ambient. La queue garde sa durée quelle que soit la Taille. De 0,5x à 2,0x.

**zh-Hans**

> 设定尾音持续多久，以该类型自身衰减时间的倍数计：在旋钮中心，也就是正好 1.0x 处，Booth 约为 0.4 秒，Ambient 约为 7 秒。无论尺寸设在哪里，尾音都保持这个长度。0.5x 到 2.0x。

**en' (zh-Hans read back blind)**

> Sets how long the tail lasts, as a multiple of that type's own decay time: at the center of the knob, exactly 1.0x, Booth is about 0.4 seconds and Ambient about 7 seconds. Wherever the size is set, the tail keeps this length. 0.5x to 2.0x.

## tip.size

**en**

> Scales the space the Type chose, and only the space: the echoes and early reflections move apart or together while the length of the tail stays with Decay. It is relative rather than absolute — the four rooms run from half to double their middle size, Plate and Spring over a narrower range — so Booth at 100 % is still smaller than Hall at 0 %. Moving it while sound rings bends the pitch of the tail for a moment. 0 to 100 %.

**fr**

> Met à l’échelle l’espace choisi par le Type, et rien d’autre : les échos et les premières réflexions s’écartent ou se resserrent, tandis que la durée de la queue reste l’affaire de Déclin. C’est une valeur relative et non absolue — les quatre salles vont de la moitié au double de leur taille médiane, Plate et Spring sur une plage plus étroite — si bien que Booth à 100 % reste plus petit que Hall à 0 %. La déplacer pendant que le son résonne infléchit un instant la hauteur de la queue. 0 à 100 %.

**zh-Hans**

> 缩放类型所选的空间，而且只缩放空间：回声与早期反射彼此拉开或靠拢，尾音的长短仍交给衰减。它是相对的而不是绝对的 — 四种房间从中间大小的一半到两倍，Plate 和 Spring 的范围更窄 — 因此 Booth 在 100% 时仍然小于 Hall 在 0% 时。在声音还在响的时候转动它，会让尾音的音高短暂地弯一下。0 到 100%。

**en' (zh-Hans read back blind)**

> Scales the space chosen by the type, and only scales the space: echoes and early reflections are pulled apart or drawn together, while the length of the tail is still left to Decay. It is relative rather than absolute — for the four rooms from half to twice a medium size, with a narrower range for Plate and Spring — so Booth at 100% is still smaller than Hall at 0%. Turning it while the sound is still ringing makes the pitch of the tail bend briefly. 0 to 100%.

## Notes from the reverse pass

- tip.type: the reader flagged "three reverb engines" against six listed types. That is what the
  English says too (three engines, six types); the Chinese is faithful. 隔音间 reads back as
  "soundproof booth", as it did in the v1.x body.
- tip.size: "half to double their middle size" reads back as "from half to twice a medium size".
- tip.decay: no drift.
