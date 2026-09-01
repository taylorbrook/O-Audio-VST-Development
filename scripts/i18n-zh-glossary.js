/*
   This file is part of the Ouaricon Audio plugin suite.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    i18n-zh-glossary.js — the settled Simplified Chinese for terms that recur
    across plugins.

    ── REPORT TODAY, GATE LATER ──────────────────────────────────────────────

    This file is data; the tool that reads it — scripts/i18n-zh-lint.js — ships
    as a REPORT and exits 0 no matter what it finds. It is promoted to a GATE
    (exit 2 on any finding) only once the Stage 2 pilot, O-Chorus, is at zero
    findings. That is the exact lifecycle scripts/i18n-fr-lint.js went through:
    it began report-only on the day 43 of 43 plugins failed it, and became a
    gate on 2026-08-31 when the rollout had taken every plugin to 0. Doing it in
    the other order would let a half-built lint block Stage 2.

    ── WHY THIS EXISTS ───────────────────────────────────────────────────────

    The French rollout scanned every French entry in the suite and found 267
    English label strings carrying MORE THAN ONE French rendering — "Off" had
    nine. Each was a defensible choice made by one author looking at one plugin.
    Forty-three reviewers working without a shared list make that WORSE, not
    better. The French glossary was written AFTER the divergence; this one is
    written BEFORE any translator is dispatched, which is the entire reason
    Stage 1 precedes Stage 2.

    The shared-string set is measured, not guessed: an ESM-import walk over all
    43 plugins on 2026-09-01 found 3789 entries needing a zh value, 1802 unique
    short strings, and 551 strings appearing at MORE THAN ONE site — covering
    2538 of 3789 occurrences (67.0%). Those 551 are what TERMS holds.

    ── THE REVIEW BAR, STATED PLAINLY ────────────────────────────────────────

    This project has NO native Chinese reader. The `reviewed: true` flip the
    French tables got meant literally "the developer read it", and that lane is
    closed here. So the zh rollout ships at `reviewed: 'bt'` — an INDEPENDENT
    back-translation (zh -> en', produced by a separate pass that never saw the
    English) which the developer read against the English source. `'native'`
    stays open as a later upgrade and is a blocker for nothing. `'mt'` — a raw
    machine draft nobody checked — NEVER ships. See scripts/i18n-zh-lint.js
    rule R1, which is what makes that bar mechanical rather than aspirational.

    ── How TERMS is read ─────────────────────────────────────────────────────

    Key:   the English label or tooltip TITLE, lower-cased, trimmed, with a
           trailing period dropped (the lint normalises live entries the same
           way, so the keys must be normalised too).
    Value: an ARRAY of every ACCEPTED zh-Hans rendering, ROOT FIRST.

    The value type is deliberately IDENTICAL to the French glossary's — English
    key to an array of renderings — so Stage-2 tooling can consume the fr and zh
    glossaries through one code path. Budgets and the stay-English set are
    SEPARATE exports (BUDGETS, SAME_AS_EN) rather than a richer value type, for
    the same reason.

    The first value is the ROOT term — what a reviewer reaches for when the
    frame has room. The rest are accepted alternates for a caption whose width
    was pinned. Chinese has no abbreviations, so the only lever a tight cell
    leaves is a SHORTER RENDERING: a 2-character term instead of a 3- or
    4-character one. Where an alternate is listed, the trailing comment names
    the cell that forced it.

    Tooltip BODIES are not matched against TERMS — prose is a person's job.
    Bodies ARE scanned for FORBIDDEN_IN_PROSE.

    ── Exempting one entry ───────────────────────────────────────────────────

    A term can be right in one plugin and wrong in another. The entry carries
    the reason, never silence — same mechanism as the French glossary:

        'label.delay': { en: { t: 'Delay' },
                         'zh-Hans': { t: '延时补偿', reviewed: 'bt',
                                      termNote: 'alignment delay, not the effect' } }

    A termNote exempts the entry from BOTH term rules — Z5 and F1.

  ==============================================================================
*/

'use strict';

// ── EN (lower-cased) -> accepted zh-Hans renderings. ROOT FIRST. ────────────
// This covers the MEASURED shared-string set: every English label or tooltip
// title appearing at more than one site across the 43 plugins. Regenerated live
// on 2026-09-01 by the same dynamic-ESM walk the lint uses — 3789 entries
// (2367 label + 1422 title), 1802 unique, 551 shared, covering 2538 of 3789
// occurrences (67.0%). That reproduces the planning-time table exactly.
//
// Three entries below — 'direct', 'hold', 'diffusion' — are NOT in the measured
// 551. They are single-site today and were settled anyway because each is an
// obvious near-neighbour of a term that IS shared ('direct 1-8', 'hold 2+ notes
// to see intervals', 'damping'), and the cheapest moment to settle a term is
// before a second author renders it differently. Everything else here is
// measured.
const TERMS = {
    // ── chrome, shared by all 43 ────────────────────────────────────────────
    'settings':                   ['设置'],
    'language':                   ['语言'],
    'interface language':         ['界面语言'],
    'hover help':                 ['悬停帮助'],
    'toggle hover help':          ['开关悬停帮助'],
    'about':                      ['关于'],
    'made by':                    ['制作'],
    'close':                      ['关闭'],
    'cancel':                     ['取消'],
    'clear':                      ['清除'],
    'clear all':                  ['全部清除'],
    'confirm?':                   ['确认？', '确认'],   // the 2-char form is the lever for a pinned cell; Chinese has no abbreviations
    'export':                     ['导出'],
    'export html':                ['导出 HTML'],
    'import':                     ['导入'],
    'generate':                   ['生成'],
    'reset':                      ['重置'],
    'store':                      ['存储'],
    'swap':                       ['交换'],
    'learn':                      ['学习'],
    'main':                       ['主界面', '主'],
    'user':                       ['用户'],
    'factory':                    ['出厂'],
    'all':                        ['全部'],
    'all categories':             ['全部类别'],
    'none':                       ['无'],
    'auto':                       ['自动'],
    'manual':                     ['手动'],
    'free':                       ['自由'],
    'custom semitones':           ['自定义半音'],
    'type':                       ['类型'],
    'mode':                       ['模式'],
    'source':                     ['源'],
    'target':                     ['目标'],
    'roles':                      ['角色'],
    'slot':                       ['插槽'],
    'next':                       ['下一个', '下一'],
    'previous':                   ['上一个', '上一'],
    'start':                      ['开始'],
    'end':                        ['结束'],
    'stop':                       ['停止'],
    'on':                         ['开', '开启'],
    'off':                        ['关', '关闭'],
    'in':                         ['输入', '入'],
    'out':                        ['输出', '出'],
    'left':                       ['左'],
    'right':                      ['右'],
    'front':                      ['前'],
    'rear':                       ['后'],
    'min':                        ['最小'],
    'max':                        ['最大'],
    'high':                       ['高'],
    'low':                        ['低'],
    'mid':                        ['中频', '中置', '中'],   // TWO senses in the corpus: the mid BAND (O-Formant, O-FreqPulse — French renders it Medium) and the M/S MID channel (O-MultiBandCompressor). 中频 is the band, 中置 the M/S channel; both are accepted so neither site needs a termNote. Pairs with 'sides' -> 侧向.
    'fine':                       ['微调'],
    'coarse':                     ['粗调'],
    'amount':                     ['量'],
    'count':                      ['数量'],
    'length':                     ['长度'],
    'time':                       ['时间'],
    'elapsed':                    ['已用时'],
    'window':                     ['窗口'],
    'quality':                    ['质量'],
    'performance':                ['性能'],
    'console':                    ['控制台'],
    'monitor':                    ['监听'],
    'meter':                      ['表'],
    'spectrum':                   ['频谱'],
    'spectrum ·':                 ['频谱 ·'],
    'spectrogram':                ['语图', '声谱图'],   // 语图 is the shorter of the two; 声谱图 is the fuller term
    'output scope':               ['输出示波'],
    'signal path':                ['信号路径'],
    'path':                       ['路径'],
    'matrix':                     ['矩阵'],
    // ── presets and files ───────────────────────────────────────────────────
    'preset':                     ['预设'],
    'presets':                    ['预设'],
    'preset name..':              ['预设名称..'],
    'instrument preset':          ['音色预设'],
    'lesson presets':             ['教学预设'],
    'concept presets':            ['概念预设'],
    'next preset':                ['下一个预设'],
    'previous preset':            ['上一个预设'],
    'browse presets':             ['浏览预设'],
    'no presets':                 ['无预设'],
    'no presets available':       ['没有可用预设'],
    'click to browse presets':    ['点击浏览预设'],
    'click to see all presets':   ['点击查看全部预设'],
    'save':                       ['保存'],
    'save preset':                ['保存预设'],
    'save current settings':      ['保存当前设置'],
    'save current settings as a user preset': ['将当前设置保存为用户预设'],
    'load':                       ['载入'],
    'load…':                      ['载入…'],
    'load preset':                ['载入预设'],
    'load preset from file':      ['从文件载入预设'],
    'load your own':              ['载入自己的'],
    'load failed':                ['载入失败'],
    'delete':                     ['删除'],
    'del':                        ['删除'],
    'delete preset':              ['删除预设'],
    'delete preset "{name}"?':    ['删除预设“{name}”？'],
    'delete the current user preset': ['删除当前用户预设'],
    '{name} loaded':              ['已载入 {name}'],
    'loading {name}…':            ['正在载入 {name}…'],
    'source loaded':              ['已载入源'],
    'file load failed at commit': ['提交时文件载入失败'],
    'file transfer failed':       ['文件传输失败'],
    'drop failed: {error}':       ['拖放失败：{error}'],
    'drop session start failed':  ['拖放会话启动失败'],
    'drop a single audio file, not a folder': ['请拖入单个音频文件，而不是文件夹'],
    'otherwise filename tokens (': ['否则使用文件名标记 ('],
    'double-click to edit':       ['双击编辑'],
    'click to play':              ['点击播放'],
    'play ·':                     ['播放 ·'],
    // ── levels, gain and routing ────────────────────────────────────────────
    'mix':                        ['混合', '干湿'],   // NOT 混音 — that is the mixing PROCESS; see FORBIDDEN_IN_LABELS
    'dry/wet':                    ['干湿'],
    'dry':                        ['干'],
    'wet':                        ['湿'],
    'direct':                     ['直达'],
    'blend':                      ['混融'],
    'crossfade':                  ['交叉渐变'],
    'input':                      ['输入'],
    'output':                     ['输出'],
    'output level':               ['输出电平'],
    'output gain':                ['输出增益'],
    'level':                      ['电平'],
    'target level':               ['目标电平'],
    'gain':                       ['增益'],
    'gain offset':                ['增益偏移'],
    'auto-gain':                  ['自动增益'],
    'autogain':                   ['自动增益'],
    'trim':                       ['微调电平', '配平'],
    'volume':                     ['音量'],
    'master volume':              ['主音量'],
    'master':                     ['总控'],   // NOT 主人; 总控 is what a Chinese DAW shows on a master strip
    'mains':                      ['主输出'],
    'sub':                        ['低音', '超低频'],   // the sub OSCILLATOR (O-Bells, O-simpleSubtractive) and the sub-bass BAND (O-FreqPulse). Deliberately NOT 低频 on its own: 低频 leads a reader toward 低频振荡器, which is LFO.
    'solo':                       ['独奏'],
    'mute':                       ['静音'],
    'bypass':                     ['旁通'],
    'effects':                    ['效果'],
    'filter routing':             ['滤波器路由'],
    'sample peak':                ['采样峰值'],
    'short-term':                 ['短时'],
    'integrated':                 ['整体'],
    'ceiling':                    ['上限'],
    'lookahead':                  ['前瞻'],
    'dither':                     ['抖动'],
    'oversampling':               ['过采样'],
    'high fidelity':              ['高保真'],
    'nonlinear':                  ['非线性'],
    'saturate':                   ['饱和'],
    'drive':                      ['驱动'],   // NOT 驾驶; a Chinese DAW shows 驱动 or 过载 for an overdrive stage
    'analog':                     ['模拟'],
    'vintage':                    ['复古'],
    'duck':                       ['闪避'],
    'mono-safe':                  ['单声道兼容'],
    'mono':                       ['单声道'],
    'stereo':                     ['立体声'],
    'stereo width':               ['立体声宽度'],
    'width':                      ['宽度'],
    'pan':                        ['声像'],   // NOT 平底锅; see FORBIDDEN_IN_LABELS
    'pan rnd':                    ['声像随机'],
    'pan spray':                  ['声像散布'],
    'pan sync':                   ['声像同步'],
    'l/r offset':                 ['L/R 偏移'],
    'm/s off':                    ['M/S 关'],
    'ping-pong':                  ['乒乓'],
    'sides':                      ['侧向'],
    'center diverge':             ['中置发散'],
    // ── dynamics and envelopes ──────────────────────────────────────────────
    'attack':                     ['起音'],   // NOT 攻击; see FORBIDDEN_IN_LABELS
    'decay':                      ['衰减'],   // NOT 衰变, which is radioactive decay
    'sustain':                    ['延音'],
    'release':                    ['释音', '释放'],   // NOT 发布; see FORBIDDEN_IN_LABELS
    'dec':                        ['解码'],   // M/S DEcode, not DECay. Both live sites are O-Gain's M/S pair (label.msDec, tip 'ms-dec'), whose English body says "Decode Mid/Side back to L/R".
    'hold':                       ['保持'],
    'envelope':                   ['包络'],
    'adsr envelope':              ['ADSR 包络'],
    'amplitude envelope':         ['振幅包络'],
    'amp envelope':               ['振幅包络'],
    'filter envelope':            ['滤波器包络'],
    'amp attack':                 ['振幅起音'],
    'amp decay':                  ['振幅衰减'],
    'amp sustain':                ['振幅延音'],
    'amp release':                ['振幅释音'],
    'filter attack':              ['滤波起音'],
    'filter decay':               ['滤波衰减'],
    'filter sustain':             ['滤波延音'],
    'filter release':             ['滤波释音'],
    'mod attack':                 ['调制起音'],
    'mod decay':                  ['调制衰减'],
    'mod sustain':                ['调制延音'],
    'mod release':                ['调制释音'],
    'attack amount':              ['起音量'],
    'attack char':                ['起音特性'],
    'attack curve':               ['起音曲线'],
    'attack noise':               ['起音噪声'],
    'sustain curve':              ['延音曲线'],
    'atten curve':                ['衰减曲线'],
    'decay time':                 ['衰减时间'],
    'spectral decay':             ['频谱衰减'],
    'inf. sustain':               ['无限延音'],
    'infinite sustain':           ['无限延音'],
    'hum sustain':                ['嗡鸣延音'],
    'onset':                      ['起始'],
    'transition':                 ['过渡'],
    'smoothing':                  ['平滑'],
    'threshold':                  ['阈值'],
    'ratio':                      ['比率'],
    'ratio snap':                 ['比率吸附'],
    'knee':                       ['拐点'],   // the compressor knee; 拐点 is what Chinese DAWs show
    'dynamics mode':              ['动态模式'],
    'sensitivity':                ['灵敏度'],
    'intensity':                  ['强度'],
    'severity':                   ['程度'],
    'energy':                     ['能量'],
    'confidence':                 ['置信度'],
    'velocity':                   ['力度'],   // MIDI velocity; NOT 速度, which is speed
    'pressure':                   ['压力'],
    'expression':                 ['表情'],
    'note expression':            ['音符表情'],
    // ── modulation, LFO and randomness ──────────────────────────────────────
    'depth':                      ['深度'],
    'rate':                       ['速率'],
    'speed':                      ['速度'],
    'mod':                        ['调制'],
    'lfo depth':                  ['LFO 深度'],
    'lfo rate':                   ['LFO 速率'],
    'flt lfo':                    ['滤波 LFO'],
    'vib rate':                   ['颤音速率'],
    'vib depth':                  ['颤音深度'],
    'vibrato rate':               ['颤音速率'],
    'vibrato depth':              ['颤音深度'],
    'vibrato source':             ['颤音源'],
    'chorus':                     ['合唱'],
    'chorus depth':               ['合唱深度'],
    'chorus rate':                ['合唱速率'],
    'chorus mix':                 ['合唱混合'],
    'chorus bypass':              ['合唱旁通'],
    'phase':                      ['相位'],
    'ph l':                       ['相位 L'],
    'ph r':                       ['相位 R'],
    'spread':                     ['扩散', '展宽'],
    'mode spread':                ['模态扩散'],
    'shape':                      ['形状'],
    'waveform':                   ['波形'],
    'waveform ·':                 ['波形 ·'],
    'sawtooth':                   ['锯齿波', '锯齿'],
    'square':                     ['方波'],
    'pure sine':                  ['纯正弦'],
    'random':                     ['随机'],
    'rnd':                        ['随机'],
    'amp rnd':                    ['振幅随机'],
    'size rnd':                   ['尺寸随机'],
    'pitch rnd':                  ['音高随机'],
    'jitter':                     ['抖动'],
    'drift':                      ['漂移'],
    'drift depth':                ['漂移深度'],
    'humanize':                   ['人性化'],
    'variation':                  ['变化'],
    'evolve':                     ['演化'],
    'morph pad':                  ['变形面板'],
    'vowel morph':                ['元音变形'],
    'warp':                       ['扭曲'],
    'wobble':                     ['摇摆'],
    'wow':                        ['慢抖'],   // wow and flutter: 慢抖 is the slow one, 快抖 the fast
    'flutter':                    ['快抖'],
    'motion':                     ['运动'],
    'motion rate':                ['运动速率'],
    'bloom amount':               ['绽放量'],
    'bloom speed':                ['绽放速度'],
    'seed':                       ['种子'],
    'probability':                ['概率'],
    'prob':                       ['概率'],
    'complexity':                 ['复杂度'],
    'scatter':                    ['散布'],
    'scatter x':                  ['散布 X'],
    'scatter y':                  ['散布 Y'],
    'spacing':                    ['间距'],
    'ping':                       ['脉冲激励', '激励'],
    'trigger':                    ['触发'],   // NOT 扳机; see FORBIDDEN_IN_LABELS
    'momentary':                  ['瞬时'],
    'vel>flt':                    ['力度>滤波'],
    // ── filter and EQ ───────────────────────────────────────────────────────
    'filter':                     ['滤波器', '滤波'],   // NOT 过滤器, which is a water or air filter
    'lp filter':                  ['低通滤波器'],
    'low cut':                    ['低切'],
    'cutoff':                     ['截止'],
    'resonance':                  ['共振'],
    'sharpness (q)':              ['锐度 (Q)'],
    'eq':                         ['均衡', 'EQ'],   // 均衡 when the cell allows; EQ is what many Chinese DAWs actually print
    '3-band eq':                  ['三段均衡'],
    'eq bypass':                  ['均衡旁通'],
    'eq low gain':                ['均衡低频增益'],
    'eq mid gain':                ['均衡中频增益'],
    'eq high gain':               ['均衡高频增益'],
    'eq mid freq':                ['均衡中频频率'],
    'mid freq':                   ['中频频率'],
    'frequency':                  ['频率'],
    'hf shelf':                   ['高频搁架'],
    'lf shelf':                   ['低频搁架'],
    'crossover 1':                ['分频点 1'],
    'crossover 2':                ['分频点 2'],
    'crossover 3':                ['分频点 3'],
    'amount low':                 ['低频量'],
    'amount mid':                 ['中频量'],
    'amount high':                ['高频量'],
    'speed low':                  ['低频速度'],
    'speed mid':                  ['中频速度'],
    'speed high':                 ['高频速度'],
    'tilt':                       ['倾斜'],
    'rolloff':                    ['滚降'],
    'damp':                       ['阻尼'],
    'damping':                    ['阻尼'],
    'dist lpf':                   ['失真低通'],
    'formant':                    ['共振峰'],
    'brightness':                 ['明亮度'],
    'acoustic brightness':        ['声学明亮度'],
    'overtone brightness':        ['泛音明亮度'],
    'brilliance':                 ['亮泽'],
    'tone':                       ['音色'],
    'tone color':                 ['音色'],
    'timbre':                     ['音色'],
    'tone track':                 ['音色跟随'],
    'character':                  ['特性'],
    'nasality':                   ['鼻音度'],
    'nasal place':                ['鼻音位置'],
    'subtone':                    ['弱吹音'],
    // ── delay, reverb and space ─────────────────────────────────────────────
    'delay':                      ['延迟'],
    'delay time':                 ['延迟时间'],
    'delay mix':                  ['延迟混合'],
    'delay mode':                 ['延迟模式'],
    'delay feedback':             ['延迟反馈'],
    'delay bypass':               ['延迟旁通'],
    'pre-dly':                    ['预延迟'],
    'feedback':                   ['反馈'],
    'repeats':                    ['重复'],
    'reverb':                     ['混响'],
    'reverb mix':                 ['混响混合'],
    'reverb size':                ['混响尺寸'],
    'reverb mod':                 ['混响调制'],
    'reverb damping':             ['混响阻尼'],
    'reverb shimmer':             ['混响微光'],
    'reverb bypass':              ['混响旁通'],
    'reverb pre-delay':           ['混响预延迟'],
    'shimmer':                    ['微光'],
    'size':                       ['尺寸', '大小'],
    'bell size':                  ['钟体尺寸'],
    'density':                    ['密度'],
    'diffusion':                  ['扩散度'],
    'air absorption':             ['空气吸收'],
    'air time':                   ['空气时间'],
    'distance':                   ['距离'],
    'position':                   ['位置'],
    'pos':                        ['位置'],
    'direction':                  ['方向'],
    'rotation':                   ['旋转'],
    'elevation':                  ['仰角'],
    'elev range':                 ['仰角范围'],
    'el spread':                  ['仰角扩散'],
    'azimuth':                    ['方位角'],
    'az spread':                  ['方位扩散'],
    'polar':                      ['极坐标'],
    'circle':                     ['圆周'],
    'trajectory':                 ['轨迹'],
    'traj speed':                 ['轨迹速度'],
    'doppler':                    ['多普勒'],
    'speaker layout':             ['扬声器布局'],
    'direct 1–8':                 ['直达 1–8'],
    'hull atten':                 ['外壳衰减'],
    'spin-up time':               ['加速时间'],
    'spin-down time':             ['减速时间'],
    'freeze':                     ['冻结'],
    'frozen pad':                 ['冻结铺垫'],
    'looped pad':                 ['循环铺垫'],
    'reverse':                    ['反向'],
    // ── transport, tempo and sequencing ─────────────────────────────────────
    'tempo':                      ['速度', '节奏速度'],   // the BPM sense; 速度 is what Chinese DAWs print for tempo
    'tempo sync':                 ['节拍同步'],
    'sync':                       ['同步'],
    'sync mode':                  ['同步模式'],
    'timing':                     ['时值'],
    'measure':                    ['小节'],   // NOT 酒吧 for "bar"; a musical bar is 小节
    '1 bar':                      ['1 小节'],
    'division':                   ['分割'],
    'divisions':                  ['分割'],
    'steps':                      ['步'],
    'pulses':                     ['脉冲'],
    'euclidean':                  ['欧几里得'],
    'pattern length':             ['模式长度'],
    'swing':                      ['摇摆'],
    'seq':                        ['音序'],
    'stutter gate':               ['断续门'],
    'loop mode':                  ['循环模式'],
    'loop start':                 ['循环起点'],
    'loop end':                   ['循环终点'],
    'overlap':                    ['重叠'],
    'pass length':                ['通过长度'],
    'scan':                       ['扫描'],
    'enc':                        ['编码'],
    // ── tuning and microtonality ────────────────────────────────────────────
    'tuning':                     ['调音'],
    'tune':                       ['调音'],
    'tuning system':              ['调音体系'],
    'tuning library':             ['调音库'],
    'tuning panel failed to load': ['调音面板载入失败'],
    'partial tune':               ['分音调音'],
    'detune':                     ['失谐'],
    'unison':                     ['齐奏'],
    'glide':                      ['滑音'],
    'glide mode':                 ['滑音模式'],
    'pitch':                      ['音高'],   // NOT 球场; see FORBIDDEN_IN_LABELS
    'pitch mode':                 ['音高模式'],
    'fixed mode':                 ['固定模式'],
    'pb range':                   ['弯音范围'],
    'reference pitch':            ['基准音高'],
    'a4 ref':                     ['A4 基准'],
    'a4 reference':               ['A4 基准音高'],
    'scale':                      ['音阶'],   // NOT 规模; the musical sense
    'scale intervals':            ['音阶音程'],
    'generate scale':             ['生成音阶'],
    'intervals':                  ['音程'],
    'interval':                   ['音程'],
    'intervals: {n}':             ['音程：{n}'],
    'intervals ({n} notes)':      ['音程（{n} 个音）'],
    'inversion':                  ['转位'],
    'voicing':                    ['和声排列'],
    'tonic':                      ['主音'],
    'tonic:':                     ['主音：'],
    'change tonic note (transposes instrument)': ['更改主音（移调乐器）'],
    'root note':                  ['根音'],
    'root key':                   ['根音调'],
    'key root':                   ['调根音'],
    'octave stretch':             ['八度延展'],
    'stretch':                    ['延展'],
    'non-octave':                 ['非八度'],
    'oct':                        ['八度', '八声道'],   // O-Bells means the OCTAVE; O-Orbit's label.fmtOct means the OCTOPHONIC format (French renders it Octo, not Octave). Both accepted.
    'sub octave':                 ['低八度'],
    'equal divisions':            ['等分'],
    'edo (equal division)':       ['等分八度 (EDO)'],
    'just intonation':            ['纯律'],
    'historical':                 ['历史音律'],
    'world':                      ['世界音律'],
    'botanical':                  ['植物律'],
    'rank-2 temperament':         ['二阶音律'],
    'period (c)':                 ['周期 (C)'],
    'generator (c)':              ['生成元 (C)'],
    'harmonic series':            ['泛音列'],
    'start harmonic':             ['起始泛音'],
    'end harmonic':               ['终止泛音'],
    'sub-harmonics':              ['次谐波'],
    'sub harm':                   ['次谐波'],
    'inharmonicity':              ['非谐性'],
    'load .scl':                  ['载入 .scl'],
    'load .kbm':                  ['载入 .kbm'],
    'save .scl':                  ['保存 .scl'],
    'save .kbm':                  ['保存 .kbm'],
    'true keys':                  ['真实键位'],
    'free ks':                    ['自由键位'],
    'scale ks':                   ['音阶键位'],
    'on-screen keyboard':         ['屏幕键盘'],
    'click the keys or use your computer keyboard (a s d f g h j k · w e t y u)': ['点击琴键或使用电脑键盘 (a s d f g h j k · w e t y u)'],
    'hold 2+ notes to see intervals': ['按住 2 个以上音符可查看音程'],
    'notes':                      ['音符'],   // NOT 笔记; see FORBIDDEN_IN_LABELS
    'notes: {n}':                 ['音符：{n}'],
    'active notes':               ['活动音符'],
    'next note':                  ['下一个音符'],
    'previous note':              ['上一个音符'],
    'voice':                      ['声部'],
    'voices':                     ['复音数', '声部'],   // 复音数 is the polyphony count a Chinese DAW prints
    'max voices':                 ['最大复音数'],
    'lyrics':                     ['歌词'],
    'midi mode':                  ['MIDI 模式'],
    // ── physical modelling — excitation, string, bore, body ─────────────────
    'excitation':                 ['激励'],
    'material':                   ['材质'],
    'wood type':                  ['木材种类'],
    'hardness':                   ['硬度'],
    'finger hardness':            ['手指硬度'],
    'stiffness':                  ['劲度'],
    'tension':                    ['张力'],
    'gauge':                      ['弦径'],
    'string':                     ['弦'],
    'active strings':             ['活动弦'],
    'string model':               ['弦模型'],
    'sympathetic':                ['共鸣弦'],
    'pluck position':             ['拨弦位置'],
    'strike':                     ['击奏'],
    'strike time':                ['击奏时间'],
    'mallet':                     ['槌'],   // a percussion mallet; NOT 大槌, which is a bass-drum beater
    'rect click':                 ['矩形击声'],
    'body':                       ['共鸣体'],
    'body time':                  ['共鸣体时间'],
    'resonator':                  ['共鸣器'],
    'bow force':                  ['弓压'],
    'bow pressure':               ['弓压'],
    'bow speed':                  ['弓速'],
    'bow position':               ['弓位'],
    'bow noise':                  ['弓噪'],
    'rosin':                      ['松香'],
    'schelleng diagram':          ['谢伦图'],
    'breath':                     ['气息'],
    'breath pressure':            ['气压'],
    'embouchure':                 ['口型'],
    'air column':                 ['气柱'],
    'air noise':                  ['气噪'],
    'bore profile':               ['管型'],
    'bore character':             ['管体特性'],
    'dual bore':                  ['双管'],
    'tone holes':                 ['音孔'],
    'double reed':                ['双簧'],
    'flutter tongue':             ['花舌'],
    'growl':                      ['喉音'],
    'technique':                  ['技法'],
    'tech':                       ['技法'],
    'instrument':                 ['乐器'],
    'organ':                      ['管风琴'],
    'harmonic drawbars':          ['谐音拉杆'],
    'drone pitch':                ['持续音音高'],
    'carrier null':               ['载波零点'],
    'pitched buzz':               ['有音高嗡鸣'],
    // ── granular, texture and lo-fi ─────────────────────────────────────────
    'grain':                      ['颗粒'],
    'grains':                     ['颗粒'],
    'grain size':                 ['颗粒尺寸'],
    'grain pitch':                ['颗粒音高'],
    'single grain':               ['单颗粒'],
    'smooth cloud':               ['平滑云团'],
    'granular fire':              ['颗粒触发'],
    'fragments':                  ['碎片'],
    'pitch spray':                ['音高散布'],
    'texture':                    ['纹理'],
    'sound':                      ['声音'],
    'noise':                      ['噪声'],
    'hiss':                       ['嘶声'],
    'dropout':                    ['信号跌落'],
    'scratch':                    ['刮擦'],
    'crush':                      ['压碎'],
    'bit depth':                  ['位深'],
    'wear':                       ['磨损'],
    'age':                        ['老化'],
    'era':                        ['年代'],
    'lo-fi bells':                ['低保真钟声'],
    'raw one-shot':               ['原始单次'],
    'hard edges':                 ['硬边缘'],
    'taper':                      ['渐变'],
    'enhance':                    ['增强'],
    'background':                 ['背景'],
    'total span':                 ['总跨度'],
    'synth':                      ['合成器'],
    'sub oscillator':             ['副振荡器', '次低音振荡器'],   // NOT 低频振荡器 — that is the standard Chinese rendering of LFO, so it would name the wrong oscillator entirely.
    'osc a':                      ['振荡器 A'],
    'osc b':                      ['振荡器 B'],
    'osc mix':                    ['振荡器混合'],
    'source mode':                ['源模式'],
    'hover any control for an explanation · pick a lesson to hear a concept': ['悬停任意控件查看说明 · 选择一课来听一个概念'],
    'hover any control for an explanation · pick a concept preset to hear it isolated': ['悬停任意控件查看说明 · 选择一个概念预设来单独试听'],
    'drag vertically · wheel or arrows to trim · double-click to reset': ['垂直拖动 · 滚轮或方向键微调 · 双击重置'],
};

// ── Character budgets. MEASURED CELLS ONLY. ─────────────────────────────────
// maxChars = floor(cellWidthPx / fontSizePx). A key ABSENT from this table is
// UNBUDGETED and lint rule Z6 is inert on it, by design — inventing a budget
// would be a number with no measurement behind it, which is worse than none.
// Stages 2-4 fill these in from the check-ui-labels zh arm as each cell is
// measured. The lint PRINTS the unbudgeted count so the inert coverage is
// disclosed rather than silent.
const CHORUS_CITE = 'plugins/O-Chorus/Source/ui/public/js/i18n.js:39-42 (62 px wrap cliff, '
    + '50 px gate cliff) at the 10 px caption size given in 260901-akh-IMPLEMENTATION-PLAN.md '
    + 'Stage 2 item 6';
const BUDGETS = {
    'depth':  { maxChars: 6, cellWidthPx: 62, fontSizePx: 10, source: CHORUS_CITE },
    'save':   { maxChars: 6, cellWidthPx: 62, fontSizePx: 10, source: CHORUS_CITE },
    'spread': { maxChars: 5, cellWidthPx: 50, fontSizePx: 10, source: CHORUS_CITE },
};

// ── Tokens that STAY ENGLISH in a zh table. ─────────────────────────────────
// These are keyed `sameAsEn: true` in a plugin's table rather than exempted, so
// a human still has to agree with each one. A token here must NOT also carry a
// TERMS rendering — the module-load assertion below refuses that overlap.
// Corpus-proven as of the 2026-09-01 walk: every one of these appears in the
// shared set (or inside a shared compound) and has no Chinese rendering a DAW
// user would recognise. 'EQ' is deliberately ABSENT — 均衡 is real and readable,
// so 'eq' lives in TERMS with 'EQ' as an accepted alternate instead.
const SAME_AS_EN = ['LFO', 'MIDI', 'dB', 'Hz', 'kHz', 'ms', 'BPM', 'ADSR', 'AGC', 'EDO', 'M/S', 'Q'];

// ── Renderings that are wrong wherever they appear. ─────────────────────────
// Same mechanism as the French glossary: a wrong rendering mapping to what
// should have been written instead. The content is different in one structural
// way — Chinese has no word delimiter, so the lint tests CONTAINMENT, not a
// stem with a word-boundary lookahead. Two consequences the next editor must
// keep in mind:
//
//   1. A forbidden rendering that is a SUBSTRING of a correct one will fire on
//      the correct one. 混音 is forbidden for a wet/dry Mix knob and is also the
//      first half of 混音器, the correct rendering of "Mixer". That case is
//      covered because F1 never fires on a rendering the glossary ACCEPTS for
//      the same English — so 'mixer' MUST carry 混音器 in TERMS. Check for this
//      before adding an entry.
//   2. A word that is genuinely correct somewhere in the suite does not belong
//      here at all. 轨道 ("orbit / track") was considered and REJECTED: it is
//      the wrong sense of "Track" but the right word for O-Orbit. A termNote
//      would be needed on every O-Orbit entry to buy one catch elsewhere, which
//      is a bad trade. 发布 stays out of the PROSE table for the same reason —
//      "v1.2 发布" is a correct sentence about a software release.
//
// Every entry below is a machine-translation TELL: the general-purpose sense of
// an English homograph, rendered into Chinese with no audio context.
const FORBIDDEN_IN_LABELS = {
    '\u6df7\u97f3':    '\u6df7\u5408 (\u6216 \u5e72\u6e7f) — \u6df7\u97f3 is the mixing PROCESS, not a wet/dry blend',
    '\u653b\u51fb':    '\u8d77\u97f3 — \u653b\u51fb is a military attack',
    '\u53d1\u5e03':    '\u91ca\u97f3 (\u91ca\u653e) — \u53d1\u5e03 is publishing a product',
    '\u8870\u53d8':    '\u8870\u51cf — \u8870\u53d8 is radioactive decay',
    '\u83b7\u5f97':    '\u589e\u76ca — \u83b7\u5f97 is to obtain something',
    '\u8282\u7701':    '\u4fdd\u5b58 — \u8282\u7701 is to economise',
    '\u94a5\u5319':    '\u8c03 — \u94a5\u5319 is a door key',
    '\u5e73\u5e95\u9505': '\u58f0\u50cf — a pan pot, not a frying pan',
    '\u7b14\u8bb0':    '\u97f3\u7b26 — \u7b14\u8bb0 is a written note',
    '\u89c4\u6a21':    '\u97f3\u9636 — \u89c4\u6a21 is scale in the sense of magnitude',
    '\u7403\u573a':    '\u97f3\u9ad8 — \u7403\u573a is a sports pitch',
    '\u9152\u5427':    '\u5c0f\u8282 — \u9152\u5427 is a drinking bar',
    '\u626c\u673a':    '\u89e6\u53d1 — \u626c\u673a is a gun trigger',
    '\u9a7e\u9a76':    '\u9a71\u52a8 (\u8fc7\u8f7d) — \u9a7e\u9a76 is to drive a vehicle',
    '\u8fc7\u6ee4\u5668': '\u6ee4\u6ce2\u5668 — \u8fc7\u6ee4\u5668 is a water or air filter',
    '\u6837\u54c1':    '\u6837\u672c (\u91c7\u6837) — \u6837\u54c1 is a merchandise sample',
};

// Renderings that are wrong in tooltip PROSE too — the small, unambiguous set.
// Deliberately shorter than the label table: prose has room for a word that
// would be wrong as a caption, so only the unarguable tells are listed.
const FORBIDDEN_IN_PROSE = {
    '\u63d2\u5934':    '\u63d2\u4ef6 — \u63d2\u5934 is an electrical plug',
    '\u8fc7\u6ee4\u5668': '\u6ee4\u6ce2\u5668',
    '\u6837\u54c1':    '\u6837\u672c / \u91c7\u6837',
    '\u653b\u51fb':    '\u8d77\u97f3',
    '\u5e73\u5e95\u9505': '\u58f0\u50cf',
};

// Code-point iteration, NOT `.length`. CJK extension characters live above the
// BMP and are surrogate pairs, so `.length` double-counts them — a 3-character
// caption would measure 4 and Z6 would fire on a caption that fits.
function charCount(s) {
    return [...String(s)].length;
}

// ── Derived, never stored. ──────────────────────────────────────────────────
// charCount is COMPUTED at require time from the root rendering rather than
// written beside it. A stored count is a mirrored constant, and a mirrored
// constant drifts silently the first time someone edits the rendering and not
// the number — the failure mode this repo has already been bitten by in test
// fixtures that mirror plugin constants.
const TERM_META = {};
for (const [en, renderings] of Object.entries(TERMS)) {
    const zh = renderings[0];
    const b = BUDGETS[en];
    TERM_META[en] = {
        zh,
        charCount: charCount(zh),
        maxChars: b && typeof b.maxChars === 'number' ? b.maxChars : null,
    };
}

// A token cannot both stay English and carry a Chinese rendering. Fail loudly
// at load rather than let the lint quietly enforce a contradiction.
{
    const same = SAME_AS_EN.map((s) => String(s).trim().toLowerCase());
    const overlap = Object.keys(TERMS).filter((t) => same.includes(t));
    if (overlap.length)
        throw new Error(`i18n-zh-glossary: SAME_AS_EN token also carries a TERMS rendering: ${overlap.join(', ')}`);
    const orphanBudgets = Object.keys(BUDGETS).filter((k) => !TERMS[k]);
    if (orphanBudgets.length)
        throw new Error(`i18n-zh-glossary: BUDGETS key with no TERMS entry: ${orphanBudgets.join(', ')}`);
}

module.exports = {
    TERMS,
    BUDGETS,
    SAME_AS_EN,
    FORBIDDEN_IN_LABELS,
    FORBIDDEN_IN_PROSE,
    charCount,
    TERM_META,
};
