# Stage 3 Round A — zh-Hans back-translation triples (blind pass, 2026-09-12)

Source: `scripts/i18n-zh-backtranslate.js --emit O-Strata --plugin O-Strata` (407 rows, blinded), filtered to the 60 Round A entries at `reviewed: 'mt'` (32 LABELS keys incl. the 2 changed strings + 14 tips × title / body). A fresh agent that never saw the English or the keys back-translated the Chinese; `--ingest` joined the 60 rows. Score is a SORT KEY (worst drift first), not a verdict. Flags stay `'mt'` until Taylor reads these and flips them to `'bt'` by hand (Round A verify).

```
i18n-zh-backtranslate --ingest — en -> zh -> en' triples, worst drift first
  provenance (reverse pass): "Stage 3 Round A blind back-translation, 2026-09-12"
  manifest: /private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/5e414f96-acbb-44e7-8e9c-545cf8f8929d/scratchpad/strata-zh.tsv.manifest.json
  forward pass recorded at emit: "Stage 3 Round A, 2026-09-12"
  joined: 60

  0.00  O-Strata|title|tip.terFbDamp
        en   Fb Damp
        zh   反馈阻尼
        en'  Feedback Damping
  0.00  O-Strata|label|label.feedbackDamp
        en   Fb Damp
        zh   反馈阻尼
        en'  Feedback Damping
  0.00  O-Strata|title|tip.terFreq
        en   Freq
        zh   频率
        en'  Frequency
  0.00  O-Strata|label|label.terrainFreq
        en   Freq
        zh   频率
        en'  Frequency
  0.00  O-Strata|label|label.approx
        en   approx.
        zh   近似
        en'  Approximate
  0.00  O-Strata|label|label.bandlimited
        en   Bandlimited
        zh   带限
        en'  Band-limited
  0.39  O-Strata|title|tip.terOrbMod
        en   Orbit Mod
        zh   轨道调制
        en'  Orbit Modulation
  0.39  O-Strata|label|label.terrainModY
        en   Mod Y
        zh   调制 Y
        en'  Modulation Y
  0.39  O-Strata|title|tip.terModX
        en   Mod X
        zh   调制 X
        en'  Modulation X
  0.39  O-Strata|title|tip.terTrack
        en   Pitch Track
        zh   音高跟踪
        en'  Pitch Tracking
  0.39  O-Strata|label|label.orbitMod
        en   Orbit Mod
        zh   轨道调制
        en'  Orbit Modulation
  0.39  O-Strata|label|label.terrainModX
        en   Mod X
        zh   调制 X
        en'  Modulation X
  0.39  O-Strata|label|label.pitchTrack
        en   Pitch Track
        zh   音高跟踪
        en'  Pitch Tracking
  0.39  O-Strata|title|tip.terModY
        en   Mod Y
        zh   调制 Y
        en'  Modulation Y
  0.56  O-Strata|label|label.silentAbove
        en   silent above {note}
        zh   {note} 以上静音
        en'  Muted above {note}
  0.63  O-Strata|body|tip.terTrack
        en   How much Terrain Freq scales down with note pitch — 100 % keeps the harmonic count constant across the keyboard. Range 0 to 100 %.
        zh   地形频率随音高下降的程度——100% 时泛音数量在整个键盘上保持不变。范围 0 到 100%。
        en'  How much the terrain frequency falls with pitch — at 100% the harmonic count stays constant across the whole keyboard. Range 0 to 100%.
  0.65  O-Strata|label|label.sourceMissing
        en   Source missing — using library fallback
        zh   源文件缺失——使用库内替代
        en'  Source file missing — using the library substitute
  0.67  O-Strata|label|label.orbitAspect
        en   Aspect
        zh   纵横比
        en'  Aspect Ratio
  0.67  O-Strata|title|tip.terAspect
        en   Aspect
        zh   纵横比
        en'  Aspect Ratio
  0.70  O-Strata|body|tip.terFbDamp
        en   One-pole smoothing of the feedback displacement — the stability / character control. Range 0 to 100 %.
        zh   对反馈位移的一阶平滑——稳定性与音色特征的控制。范围 0 到 100%。
        en'  First-order smoothing of the feedback displacement — a control for stability and timbral character. Range 0 to 100%.
  0.71  O-Strata|label|label.imageProjected
        en   image projected at F = {f} · fit {pct} %
        zh   图像投影 F = {f} · 拟合 {pct} %
        en'  Image projection F = {f} · fit {pct} %
  0.77  O-Strata|body|tip.terModX
        en   Terrain-specific shape input: ring spacing, saddle skew, well depth. Range 0 to 100 %.
        zh   与地形相关的形状输入：环间距、鞍面倾斜、井深。范围 0 到 100%。
        en'  Terrain-dependent shape input: ring spacing, saddle tilt, well depth. Range 0 to 100%.
  0.78  O-Strata|label|label.dropPng
        en   Drop PNG
        zh   拖入 PNG
        en'  Drop PNG here
  0.78  O-Strata|body|tip.terCY
        en   Centre offset — asymmetry and even harmonics; draggable in the view. Range −1 to 1.
        zh   中心偏移——不对称性与偶次泛音；可在视图中拖动。范围 −1 到 1。
        en'  Centre offset — asymmetry and even harmonics; can be dragged in the view. Range −1 to 1.
  0.78  O-Strata|body|tip.terCX
        en   Centre offset — asymmetry and even harmonics; draggable in the view. Range −1 to 1.
        zh   中心偏移——不对称性与偶次泛音；可在视图中拖动。范围 −1 到 1。
        en'  Centre offset — asymmetry and even harmonics; can be dragged in the view. Range −1 to 1.
  0.81  O-Strata|body|tip.terOrbMod
        en   Shape parameter of the chosen orbit: superellipse exponent, limaçon loop, epitrochoid inner ratio, squarcle corner. Range 0 to 1.
        zh   所选轨道的形状参数：超椭圆指数、蚶线环、外旋轮线内比、方圆角。范围 0 到 1。
        en'  Shape parameter of the selected orbit: superellipse exponent, limaçon loop, epitrochoid inner ratio, squircle corner. Range 0 to 1.
  0.81  O-Strata|body|tip.terModY
        en   Second terrain-specific shape input. Range 0 to 100 %.
        zh   第二个与地形相关的形状输入。范围 0 到 100%。
        en'  Second terrain-dependent shape input. Range 0 to 100%.
  0.86  O-Strata|body|tip.terFb
        en   Trajectory feedback: the previous output displaces the next orbit point. Range 0 to 100 %.
        zh   轨迹反馈：前一个输出会移动下一个轨道点。范围 0 到 100%。
        en'  Trajectory feedback: the previous output moves the next orbit point. Range 0 to 100%.
  0.87  O-Strata|body|tip.terSize
        en   Orbit radius 0.05 to 1.0 — the same parameter as the Synth tab's Orbit Size. Range 0 to 100 %.
        zh   轨道半径 0.05 到 1.0——与合成器页的轨道尺寸是同一个参数。范围 0 到 100%。
        en'  Orbit radius 0.05 to 1.0 — the same parameter as Orbit Size on the synth page. Range 0 to 100%.
  0.92  O-Strata|label|label.modMatrixInfo
        en   Route any source to any destination. 16 slots available, 46 destinations.
        zh   将任意源路由到任意目标。16 个槽位，46 个目标。
        en'  Route any source to any destination. 16 slots, 46 destinations.
  0.93  O-Strata|body|tip.terSat
        en   tanh drive on the scanned value; identity at 0. Range 0 to 100 %.
        zh   对扫描值施加 tanh 驱动；0 时为恒等。范围 0 到 100%。
        en'  Applies tanh drive to the scanned value; identity at 0. Range 0 to 100%.
  0.95  O-Strata|body|tip.terFreq
        en   Spatial frequency multiplier — the main brightness / harmonic-count control. Range 0.25× to 8×.
        zh   地形的空间频率倍数——主要的亮度和泛音数量控制。范围 0.25× 到 8×。
        en'  Spatial frequency multiplier of the terrain — the main brightness and harmonic-count control. Range 0.25× to 8×.
  1.00  O-Strata|label|label.orbitCentreX
        en   Centre X
        zh   中心 X
        en'  Centre X
  1.00  O-Strata|label|label.orbitRotation
        en   Rotation
        zh   旋转
        en'  Rotation
  1.00  O-Strata|label|label.import
        en   Import…
        zh   导入…
        en'  Import…
  1.00  O-Strata|label|label.imageBlur
        en   Image Blur
        zh   图像模糊
        en'  Image Blur
  1.00  O-Strata|body|tip.terBlur
        en   Gaussian pre-blur on PNG terrains. Range 0 to 100 %.
        zh   对 PNG 地形的高斯预模糊。范围 0 到 100%。
        en'  Gaussian pre-blur of the PNG terrain. Range 0 to 100%.
  1.00  O-Strata|title|tip.terSize
        en   Size
        zh   尺寸
        en'  Size
  1.00  O-Strata|title|tip.terRot
        en   Rotation
        zh   旋转
        en'  Rotation
  1.00  O-Strata|label|label.saturation
        en   Saturation
        zh   饱和
        en'  Saturation
  1.00  O-Strata|title|tip.terFb
        en   Feedback
        zh   反馈
        en'  Feedback
  1.00  O-Strata|label|label.orbitSize
        en   Orbit Size
        zh   轨道尺寸
        en'  Orbit Size
  1.00  O-Strata|body|tip.terAspect
        en   Minor / major axis ratio of the orbit. Range 0.1 to 1.
        zh   轨道的短轴与长轴之比。范围 0.1 到 1。
        en'  Ratio of the orbit's minor axis to its major axis. Range 0.1 to 1.
  1.00  O-Strata|label|label.webglUnavailable
        en   WebGL unavailable — 2D fallback
        zh   WebGL 不可用——2D 回退
        en'  WebGL unavailable — 2D fallback
  1.00  O-Strata|label|label.readoutPartials
        en   {n} partials at {note}
        zh   {note} 处 {n} 个分音
        en'  {n} partials at {note}
  1.00  O-Strata|label|label.hintView
        en   Drag: centre · Wheel: size · ⌥ drag: rotation
        zh   拖动：中心 · 滚轮：尺寸 · ⌥ 拖动：旋转
        en'  Drag: centre · Wheel: size · ⌥ drag: rotation
  1.00  O-Strata|title|tip.terBlur
        en   Image Blur
        zh   图像模糊
        en'  Image Blur
  1.00  O-Strata|title|tip.terCY
        en   Centre Y
        zh   中心 Y
        en'  Centre Y
  1.00  O-Strata|label|aria.viewWaveform
        en   Waveform view
        zh   波形视图
        en'  Waveform View
  1.00  O-Strata|label|label.orbit
        en   Orbit
        zh   轨道
        en'  Orbit
  1.00  O-Strata|label|aria.view3d
        en   3D view
        zh   3D 视图
        en'  3D View
  1.00  O-Strata|label|tab.terrain
        en   Terrain
        zh   地形
        en'  Terrain
  1.00  O-Strata|title|tip.terCX
        en   Centre X
        zh   中心 X
        en'  Centre X
  1.00  O-Strata|label|label.orbitCentreY
        en   Centre Y
        zh   中心 Y
        en'  Centre Y
  1.00  O-Strata|label|label.subtitle
        en   Microtonal Wave-Terrain Synthesizer
        zh   微分音波地形合成器
        en'  Microtonal wave-terrain synthesizer
  1.00  O-Strata|label|label.terrain
        en   Terrain
        zh   地形
        en'  Terrain
  1.00  O-Strata|label|label.quality
        en   Quality
        zh   质量
        en'  Quality
  1.00  O-Strata|label|label.edgeMode
        en   Edge Mode
        zh   边缘模式
        en'  Edge Mode
  1.00  O-Strata|body|tip.terRot
        en   X / Y phase relationship — PWM and phase-distortion-like motion. Range 0 to 360°.
        zh   X / Y 相位关系——类似 PWM 和相位失真的运动。范围 0 到 360°。
        en'  X / Y phase relationship — PWM-like and phase-distortion-like motion. Range 0 to 360°.
  1.00  O-Strata|title|tip.terSat
        en   Saturation
        zh   饱和
        en'  Saturation

  The score is a SORT KEY, not a verdict. A high score is not a pass: a
  back-translation can be lexically identical and still name a different control.

REPORT ONLY — exit 0. This becomes a gate (exit 2) once the O-Chorus pilot is at zero findings.
```
