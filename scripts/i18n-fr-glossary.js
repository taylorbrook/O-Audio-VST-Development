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

    i18n-fr-glossary.js — the settled French for terms that recur across plugins.

    WHY THIS EXISTS. Stage N of task 260826-ieq scanned every French entry in the
    suite (5078 rows across 43 plugins) and found 267 English label strings with
    MORE THAN ONE French rendering: "Off" had nine (Arrêt, ARRÊT, Non, NON, Aucun,
    désactivé, Désact., DÉS., Désactivée), "Save" had seven, "Release" had seven,
    "Feedback" had seven. Each rendering was a defensible choice made by one
    author looking at one plugin. Forty-three reviewers working without a shared
    list would make that WORSE, not better. So the list is settled here, once,
    and scripts/i18n-fr-lint.js enforces it.

    THIS IS THE SOURCE OF TRUTH. The prose companion,
    260826-ieq-FR-GLOSSARY.md in the task's .planning/quick directory, explains
    the choices; the lint reads THIS file. Change both together.

    ── How TERMS is read ─────────────────────────────────────────────────────

    Key:   the English label or tooltip TITLE, lower-cased, trimmed.
    Value: every ACCEPTED French rendering, normalised the same way the lint
           normalises a live entry — lower-cased, typographic apostrophe folded
           to ', a trailing period dropped, whitespace collapsed. So one entry
           "enreg" accepts "Enreg.", "ENREG." and "enreg" alike: CASING IS NOT
           CHECKED HERE. Casing is a separate rule (a French label follows the
           casing of the English caption it replaces: an all-caps page stays
           all-caps, with accents on capitals — É, À, Ç).

    The first value is the ROOT term — the one a reviewer reaches for when the
    frame has room. The rest are abbreviations or accepted alternates for a
    caption whose width was pinned in the plugin's Stage K header comment.

    Tooltip BODIES are not matched against TERMS — prose is reviewed by a
    person. Bodies ARE scanned for FORBIDDEN_IN_PROSE.

    ── Exempting one entry ───────────────────────────────────────────────────

    A term can be right in one plugin and wrong in another. O-Octagon's "Delay"
    is a loudspeaker ALIGNMENT delay and "Retard" is the correct French for it,
    while every other "Delay" is an effect and is "Délai". The entry carries
    the reason, in the same spirit as I18N_EXEMPT — a reason, never silence:

        'label.delay': { en: { t: 'Delay' },
                         fr: { t: 'Retard', reviewed: false,
                               termNote: 'alignment delay, not the effect' } },

    The lint accepts any string on `termNote` and reports it as EXEMPT.

  ==============================================================================
*/

'use strict';

// EN (lower-cased) → accepted FR renderings (normalised). ROOT FIRST.
const TERMS = {
    // ── chrome, shared by all 43 ────────────────────────────────────────────
    'settings':                 ['réglages'],
    'interface language':       ['langue de l\'interface'],
    'hover help':               ['aide au survol'],
    'toggle hover help':        ['activer ou désactiver l\'aide au survol'],
    'save':                     ['enregistrer', 'enreg', 'enr'],   // enr: O-Bass, a 28 px content box (Enreg. 38.84)
    'save preset':              ['enregistrer le préréglage'],
    'save current settings':    ['enregistrer les réglages actuels'],
    'save .scl':                ['enreg. .scl', 'enregistrer .scl'],
    'save .kbm':                ['enreg. .kbm', 'enregistrer .kbm'],
    'load':                     ['charger', 'charg', 'ouvrir', 'ouv'],
    'load preset':              ['charger un préréglage', 'ouvrir un préréglage'],
    'load preset from file':    ['charger un préréglage depuis un fichier', 'ouvrir un préréglage depuis un fichier'],
    'load .scl':                ['charger .scl', 'ouvrir .scl', 'charg. .scl'],   // O-Marimba: CHARGER .SCL grows #scala-buttons 200 -> 215.91 px
    'load .kbm':                ['charger .kbm', 'ouvrir .kbm', 'charg. .kbm'],
    'load your own':            ['charger le vôtre'],
    'delete':                   ['supprimer', 'suppr'],
    'del':                      ['suppr'],
    'confirm?':                 ['confirmer ?', 'sûr ?'],           // O-Emulator: Confirmer ? 58.59 px in a 49 px pinned box
    'reset':                    ['réinitialiser', 'réinit'],
    'clear':                    ['effacer'],
    'store':                    ['mémoriser', 'mém'],
    'click to browse presets':  ['cliquer pour parcourir les préréglages'],
    'click to see all presets': ['cliquer pour voir tous les préréglages'],
    'all':                      ['tous', 'toutes'],
    'on':                       ['marche', 'activé', 'activée', 'act'],          // act./dés.: O-Orbit, a 46 px pill (MARCHE 53.06, ACTIVÉ 46.33)
    'off':                      ['arrêt', 'désactivé', 'désactivée', 'aucun', 'aucune', 'dés'],
    'bypass':                   ['contournement', 'contour'],
    'mono':                     ['mono'],
    'min':                      ['min'],
    'max':                      ['max'],
    'in':                       ['entrée', 'entr', 'ent'],
    'out':                      ['sortie', 'sort', 'sor'],
    'input':                    ['entrée'],
    'output':                   ['sortie'],
    'level':                    ['niveau', 'niv'],
    'master':                   ['général'],
    'main':                     ['principal'],
    'mix':                      ['mix'],
    'blend':                    ['mix'],
    'wet':                      ['traité', 'signal traité'],
    'dry':                      ['direct', 'signal direct'],
    'width':                    ['largeur', 'larg'],
    'size':                     ['taille'],
    'shape':                    ['forme'],
    'mode':                     ['mode'],
    'type':                     ['type'],
    'time':                     ['durée'],
    'length':                   ['longueur'],
    'random':                   ['aléatoire', 'aléa'],
    'rnd':                      ['aléa', 'alé'],
    'steps':                    ['pas'],
    'direction':                ['sens'],
    'reverse':                  ['inversion', 'invers'],
    'freeze':                   ['gel', 'geler'],
    'stop':                     ['arrêt', 'arrêter'],
    'humanize':                 ['humanisation', 'humaniser'],
    'quality':                  ['qualité'],
    'intensity':                ['intensité'],
    'position':                 ['position'],
    'seed':                     ['graine'],
    'jitter':                   ['gigue'],
    'dither':                   ['dither'],
    'drift':                    ['dérive'],
    'drift depth':              ['profondeur de dérive', 'prof. dérive'],
    'timing':                   ['décalage', 'cadence'],   // décalage = a nudge; cadence = a time-base heading (O-Tapestop)
    'swing':                    ['swing'],
    'tempo':                    ['tempo'],
    'tempo sync':               ['synchro tempo', 'sync tempo'],   // O-Tremolo: Synchro Tempo wraps at 51.30 in a 42 px box
    'pan sync':                 ['synchro pan', 'sync pan'],       // the matched pair on the same page
    'sync':                     ['synchro', 'sync'],
    'sync mode':                ['mode de synchro', 'mode synchro'],
    '1 bar':                    ['1 mesure', '1 mes'],

    // ── envelopes ───────────────────────────────────────────────────────────
    'attack':                   ['attaque', 'att'],
    'decay':                    ['déclin', 'décl'],
    'sustain':                  ['maintien', 'maint'],
    'release':                  ['relâchement', 'relâch'],
    'decay time':               ['temps de déclin', 'déclin'],
    'amp attack':               ['attaque d\'amplitude', 'att. ampl'],
    'amp decay':                ['déclin d\'amplitude', 'décl. ampl'],
    'amp sustain':              ['maintien d\'amplitude', 'maint. ampl'],
    'amp release':              ['relâchement d\'amplitude', 'relâch. ampl'],
    'amp envelope':             ['enveloppe d\'amplitude'],
    'amplitude envelope':       ['enveloppe d\'amplitude'],
    'filter envelope':          ['enveloppe du filtre'],
    'filter attack':            ['attaque du filtre', 'att. filtre'],
    'filter decay':             ['déclin du filtre', 'décl. filtre'],
    'filter sustain':           ['maintien du filtre', 'maint. filtre'],
    'filter release':           ['relâchement du filtre', 'relâch. filtre'],
    'mod decay':                ['déclin mod'],
    'mod release':              ['relâchement mod', 'relâch. mod'],

    // ── modulation ──────────────────────────────────────────────────────────
    'rate':                     ['vitesse', 'vit'],
    'depth':                    ['profondeur', 'prof'],
    'lfo rate':                 ['vitesse du lfo', 'vit. lfo'],
    'lfo depth':                ['profondeur du lfo', 'prof. lfo'],
    'vibrato rate':             ['vitesse du vibrato'],
    'vibrato depth':            ['profondeur du vibrato'],
    'vib rate':                 ['vit. vibrato', 'vit. vibr'],
    'vib depth':                ['prof. vibrato', 'prof. vibr'],
    'mod':                      ['mod'],
    'waveform':                 ['forme d\'onde', 'onde'],
    'waveform ·':               ['forme d\'onde ·'],
    'phase':                    ['phase'],
    'glide':                    ['portamento'],
    'glide mode':               ['mode de portamento'],
    'detune':                   ['désaccord', 'désacc'],
    'coarse':                   ['grossier', 'gross'],
    'fine':                     ['fin', 'affinage'],   // affinage where an End control already reads Fin (O-simpleSampler N6, 51.78 px in a 54 px cell)
    'end':                      ['fin'],
    'loop crossfade':           ['fondu de boucle'],
    'spread':                   ['étalement', 'étal'],
    'az spread':                ['étalement en azimut', 'étal. az'],     // O-GrainScatter N7: 41.30 px in a 62 px cell
    'el spread':                ['étalement en élévation', 'étal. él'],
    'scatter':                  ['dispersion', 'disp'],
    'scatter x':                ['dispersion x', 'disp. x'],   // O-TextureForge: Dispersion X 65.14 px wraps a 72 px box
    'scatter y':                ['dispersion y', 'disp. y'],
    'crossfade':                ['fondu enchaîné', 'fondu'],   // fondu alone is a fade; O-TextureForge 80.73 / 31.44 px
    'wow':                      ['pleurage', 'pleur'],
    'flutter':                  ['scintillement', 'scint'],
    'flutter tongue':           ['flatterzunge'],
    'warp':                     ['déformation', 'déform'],
    'sub':                      ['sub'],
    'sub octave':               ['sous-octave'],

    // ── filters and EQ ──────────────────────────────────────────────────────
    'filter':                   ['filtre'],
    'cutoff':                   ['coupure'],
    'resonance':                ['résonance', 'réson'],
    'rolloff':                  ['pente'],
    'tilt':                     ['inclinaison'],
    'low':                      ['grave'],
    'mid':                      ['médium', 'méd'],
    'high':                     ['aigu'],
    'low cut':                  ['coupe-bas', 'coupe-b'],
    'high cut':                 ['coupe-haut', 'coupe-h'],
    'lp filter':                ['filtre passe-bas', 'filtre pb'],
    'mid freq':                 ['fréq. médium', 'fréq. méd'],
    'eq bypass':                ['contournement de l\'égaliseur', 'contournement de l\'eq'],
    'eq low gain':              ['gain grave de l\'égaliseur', 'gain grave de l\'eq', 'gain des graves'],
    'eq mid gain':              ['gain médium de l\'égaliseur', 'gain médium de l\'eq', 'gain des médiums'],
    'eq mid freq':              ['fréquence médium de l\'égaliseur', 'fréq. médium de l\'eq', 'fréquence des médiums'],
    'eq high gain':             ['gain aigu de l\'égaliseur', 'gain aigu de l\'eq', 'gain des aigus'],
    '3-band eq':                ['eq 3 bandes'],
    'drive':                    ['saturation', 'satur'],
    'saturate':                 ['saturation', 'satur'],
    'crush':                    ['écrasement', 'écras', 'broyage'],   // O-Emulator: Écrasement 82.45 px slides four knobs; Écras. 46.70 / Broyage 59.17 fit a 60 px column
    'bit depth':                ['résolution'],
    'analog':                   ['analog'],

    // ── dynamics ────────────────────────────────────────────────────────────
    'threshold':                ['seuil'],
    'ratio':                    ['ratio'],
    'knee':                     ['coude'],
    'makeup':                   ['compensation', 'compens'],   // O-MultiBandCompressor: Compensation 69.23 px re-deals a 60.16 px 1fr track
    'makeup gain':              ['gain de compensation'],
    'duck':                     ['ducking'],
    'autogain':                 ['gain auto'],
    'agc':                      ['agc'],
    'learn':                    ['mesurer'],

    // ── delay and reverb ────────────────────────────────────────────────────
    'delay':                    ['délai'],
    'delay time':               ['durée du délai'],
    'delay feedback':           ['réinjection du délai'],
    'delay mode':               ['mode de délai'],
    'delay mix':                ['mix du délai'],
    'feedback':                 ['réinjection', 'réinj'],
    'repeats':                  ['répétitions', 'répét'],
    'reverb':                   ['réverbération', 'réverb'],
    'reverb size':              ['taille de la réverbération', 'taille de la réverb'],
    'reverb pre-delay':         ['pré-délai de la réverbération', 'pré-délai de la réverb'],
    'reverb mix':               ['mix de la réverbération', 'mix de la réverb'],
    'reverb mod':               ['modulation de la réverbération', 'modulation de la réverb'],
    'reverb shimmer':           ['shimmer de la réverbération', 'shimmer de la réverb'],
    'reverb bypass':            ['contournement de la réverbération', 'contournement de la réverb'],
    'reverb damping':           ['amortissement de la réverbération', 'amortissement de la réverb'],
    'pre-delay':                ['pré-délai', 'pré-dél'],
    'pre-dly':                  ['pré-délai', 'pré-dél'],
    'damping':                  ['amortissement', 'amort'],
    'damp':                     ['amortissement', 'amort'],
    'diffusion':                ['diffusion'],
    'air absorption':           ['absorption de l\'air', 'absorption air'],
    'chorus mix':               ['mix du chorus'],
    'overlap':                  ['recouvrement', 'recouvr'],

    // ── granular and sampling ───────────────────────────────────────────────
    'grain size':               ['taille de grain', 'taille grain'],
    'grains':                   ['grains'],
    'pitch':                    ['hauteur', 'haut'],
    'pitch mode':               ['mode de hauteur', 'mode hauteur'],
    'pitch spray':              ['dispersion de hauteur', 'dispersion hauteur'],
    'size rnd':                 ['aléa taille', 'alé. taille'],
    'loop start':               ['début de boucle', 'début boucle'],
    'loop end':                 ['fin de boucle', 'fin boucle'],
    'loop mode':                ['mode de boucle', 'mode boucle'],
    'root key':                 ['note de référence', 'note de réf'],
    'source':                   ['source'],
    'source mode':              ['mode de source', 'mode source'],
    'slot':                     ['empl', 'emplacement'],

    // ── tuning (shared scala-tuning-engine panel and its copies) ────────────
    'tuning':                   ['accord'],
    'tuning library':           ['bibliothèque de gammes', 'bibliothèque'],
    'scale intervals':          ['intervalles de la gamme'],
    'generate scale':           ['générer une gamme', 'générer gamme'],
    'start harmonic':           ['harmonique de départ', 'harm. de départ'],
    'end harmonic':             ['harmonique de fin', 'harm. de fin'],
    'generator (c)':            ['générateur (c)', 'génér. (c)'],
    'historical':               ['historiques'],
    'world':                    ['du monde'],
    'non-octave':               ['non octaviantes'],
    'stretch':                  ['étirement', 'étir'],
    'total span':               ['écart total'],
    'a4 ref':                   ['réf. a4'],
    'hold 2+ notes to see intervals': ['tenir 2 notes ou plus pour voir les intervalles'],
    'tuning panel failed to load.':   ['échec du chargement du panneau d\'accord.'],
    'notes: {n}':               ['notes : {n}'],
    'notes':                    ['notes'],
    'rotation':                 ['rotation'],
    'inversion':                ['renversement', 'renvers'],
    'voices':                   ['voix'],
    'max voices':               ['voix max', 'voix maximales'],
    'pb range':                 ['plage pb'],

    // ── physical modelling ──────────────────────────────────────────────────
    'material':                 ['matériau'],
    'hardness':                 ['dureté'],
    'mallet':                   ['maillet'],   // a mailloche beats a bass drum, not a marimba (O-Marimba N5, O-simplePhysicalModelSynth N6)
    'strike':                   ['frappe'],
    'velocity':                 ['vélocité'],
    'rosin':                    ['colophane', 'coloph'],
    'bow speed':                ['vitesse d\'archet'],
    'bow pressure':             ['pression d\'archet'],
    'bow position':             ['position d\'archet'],
    'bow noise':                ['bruit d\'archet'],
    'bow force':                ['pression d\'archet', 'pression'],
    'string model':             ['modèle de corde', 'modèle corde'],
    'sympathetic':              ['sympathiques'],
    'air column':               ['colonne d\'air', 'colonne'],
    'resonator':                ['résonateur'],
    'double reed':              ['anche double'],
    'bell size':                ['taille du pavillon', 'pavillon'],
    'bore character':           ['caractère de la perce', 'caract. perce'],
    'drone pitch':              ['hauteur du bourdon', 'bourdon'],
    'wood type':                ['essence de bois', 'bois'],
    'pluck position':           ['point de pincement', 'pincement'],
    'mode spread':              ['étalement des modes', 'étalement'],
    'tone':                     ['timbre'],
    'sound':                    ['son'],
    'effects':                  ['effets'],
    'tension':                  ['tension'],

    // ── spatial ─────────────────────────────────────────────────────────────
    'path':                     ['trajectoire', 'tracé'],
    'front':                    ['avant'],
    'rear':                     ['arrière', 'arr'],
    'left':                     ['gauche'],
    'right':                    ['droite'],
    'sides':                    ['côtés'],
    'monitor':                  ['contrôle'],
    'ping':                     ['ping'],
    'speaker layout':           ['disposition des enceintes', 'enceintes'],
    'atten curve':              ['courbe d\'atténuation', 'courbe attén'],
    'elev range':               ['plage d\'élévation', 'plage élév'],
    'center diverge':           ['divergence centrale', 'divergence'],
    'hull atten':               ['atténuation hors enveloppe', 'att. env'],
    'distance':                 ['distance'],

    // ── pedagogic family (O-simple*) ────────────────────────────────────────
    'lesson presets':           ['préréglages de leçon', 'leçons'],
    'concept presets':          ['préréglages pédagogiques'],
    'on-screen keyboard':       ['clavier à l\'écran'],
    'signal path':              ['chaîne du signal'],
    'looped pad':               ['nappe bouclée', 'nappe'],
    'spectral decay':           ['décroissance spectrale', 'décroiss. spectrale'],
    'carrier null':             ['extinction de la porteuse', 'porteuse nulle'],   // 160.7 px vs a 102 px badge on O-simpleFM
    'rect click':               ['clic rectangulaire', 'clic rect'],
    'osc mix':                  ['mix des oscillateurs', 'mix osc'],
    'osc a':                    ['osc a'],
    'osc b':                    ['osc b'],
    'filter routing':           ['routage des filtres', 'routage filt'],
    'free ks':                  ['commande libre', 'cmde libre'],
    'scale ks':                 ['commande gamme', 'cmde gamme'],
    'custom semitones':         ['demi-tons libres'],
    'partial tune':             ['accord des partiels', 'accord partiels'],
    'flt lfo':                  ['lfo filtre', 'lfo filt'],
    'pulses':                   ['impulsions', 'impul'],
    'prob':                     ['prob', 'proba'],
    'probability':              ['probabilité', 'proba'],
    'dropout':                  ['chutes de signal', 'chutes'],
    'hiss':                     ['souffle'],
    'pass length':              ['durée du passage', 'passage'],
    'tone track':               ['suivi de timbre'],
    'lyrics':                   ['paroles'],
    'change tonic note (transposes instrument)': ['changer la tonique (transpose l\'instrument)'],
};

// Words that are wrong wherever they appear in a LABEL or tooltip TITLE.
// Each maps to what the reviewer should have written instead.
const FORBIDDEN_IN_LABELS = {
    'sauver':        'Enregistrer (Enreg.)',
    'sauvegarder':   'Enregistrer — sauvegarder is a backup',
    'lire':          'Charger — lire is to read or play',
    'réverbe':       'Réverbération (Réverb)',
    'genou':         'Coude',
    'germe':         'Graine',
    'rétroaction':   'Réinjection',
    'réaction':      'Réinjection',
    'retour':        'Réinjection — retour is a monitor send',
    'glissé':        'Portamento',
    'dosage':        'Mix',
    'mixage':        'Mix — mixage is the mixing process',
    'mélange':       'Mix',
    'relâche':       'Relâchement (Relâch.)',
    'tenue':         'Maintien',
    'chute':         'Déclin (or Chutes de signal for a dropout)',
    'extinction':    'Relâchement — unless it is a reverb tail or a carrier null',
    'ampleur':       'Profondeur',
    'écart':         'Désaccord (detune) or Étalement (spread); Écart total stays for span',
    'fréq.':         'Vitesse — for a rate; keep Fréq. only for a frequency in Hz',
    'maître':        'Général',
    'dériv.':        'Contournement',
    'flatt.':        'Flatterzunge',
    'frullato':      'Flatterzunge',
};

// Words that are wrong in tooltip PROSE too — the small, unambiguous set.
const FORBIDDEN_IN_PROSE = {
    'plugiciel':     'plugin (masc.) - the suite was split 12 plugins to 3; French DAW documentation says plugin',
    'mailloche':     'maillet - a mailloche is a bass-drum or tam-tam beater',
    'réverbe':       'réverbération / réverb',
    'genou':         'coude',
    'germe':         'graine',
    'rétroaction':   'réinjection',
    'sauver':        'enregistrer',
    'sauvegarder':   'enregistrer',
    'glissé':        'portamento',
};

// Units that must be glued to their number with U+00A0 in French prose.
const UNITS = ['dB', 'ms', 'Hz', 'kHz', 's', 'st', 'cents', 'BPM', 'bpm', 'octaves', 'octave', 'demi-tons', 'demi-ton', 'voix', 'mesures', 'mesure', 'pas', 'm/s', '%'];   // not 'm' or 'N': a bare letter matches prose (O-Bowed N4)

module.exports = { TERMS, FORBIDDEN_IN_LABELS, FORBIDDEN_IN_PROSE, UNITS };
