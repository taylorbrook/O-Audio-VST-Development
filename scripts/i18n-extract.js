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

    i18n-extract.js — the in-scope-text inventory, and the skeletons that make
    it review-work instead of transcription.

    ~2,400 static text nodes, ~575 visible-text attributes and ~80 JS-written
    prose strings across 43 plugins. Hand-transcribing that is how a table ends
    up subtly different from the markup it replaced; the Stage D retrofits
    compared their generated tables back against the source byte-for-byte and
    that comparison is what proved nothing had been re-typed. This tool is the
    generator that comparison needs.

    ── THREE scanners, because the three sources have nothing in common ────────

    1. HTML TEXT NODES. Parsed, not regexed: an attribute value containing `>`
       and a `<` inside a comment both defeat a regex, and the parse also yields
       the SOURCE OFFSETS the patch draft needs.

    2. VISIBLE-TEXT ATTRIBUTES — title, aria-label, placeholder, alt. Invisible
       to a text-node sweep and to a JS sweep alike. aria-label is the
       accessible name; it is user-visible text by any definition that matters.

    3. JS-WRITTEN STRINGS. A markup sweep reaches none of them. The controller
       is js/app.js on most plugins and an inline <script type="module"> on
       O-Bitrot, so it is read from wherever it lives — the same wrong-shaped
       assumption check-i18n.js already had to fix once.

    ── Classification is a SUGGESTION WITH A REASON, never a silent drop ───────

    LABEL       localize
    READOUT     numeric or unit-only — exempt under D-03, not touched
    UNIT        the whole string is a unit symbol
    ENDONYM     a language name, never translated
    PRESET-NAME exempt under D-02 — the name IS the JSON filename
    UNSURE      a human reads this row

    Misfiling in the LABEL direction is cheap: an over-reported candidate costs
    one glance. Misfiling in the READOUT direction ships English. So the READOUT
    rule is deliberately strict — a string keeps its LABEL classification unless,
    with its interpolations removed, it contains no run of two letters that is
    not a known unit token.

    ── Outputs ────────────────────────────────────────────────────────────────

    <plugin>/.planning/i18n-inventory.tsv     one row per candidate
    <plugin>/.planning/i18n-index-draft.html  index.html with data-i18n added
    <plugin>/.planning/i18n-labels-skeleton.js  a LABELS block, fr left as TODO

    The skeleton's French is the literal string `TODO` on purpose:
    check-i18n assertion 4 rejects a straight en passthrough and assertion 5
    demands an explicit `reviewed` flag, so an unfilled skeleton cannot pass.

    Usage:
        node scripts/i18n-extract.js --plugin O-Tapestop
        node scripts/i18n-extract.js --all
        node scripts/i18n-extract.js --plugin O-Prism --out-dir /tmp/x
        node scripts/i18n-extract.js --all --dry-run     (table only, no files)

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const path = require('path');
const S    = require(path.join(__dirname, 'serve-ui.js'));

// ═══════════════════════════════════════════════════════════ HTML scanner ══

const VOID_TAGS = new Set(['area', 'base', 'br', 'col', 'embed', 'hr', 'img', 'input',
                           'link', 'meta', 'param', 'source', 'track', 'wbr']);
const RAW_TAGS  = new Set(['script', 'style']);

// Returns { elements, texts }. Every record carries source offsets, which is
// what makes a mechanical patch against index.html possible — a regex-driven
// rewrite cannot place an attribute inside the right start tag when the same
// tag text appears twice on the page.
function scanHtml(src) {
    const elements = [];
    const texts    = [];
    const stack    = [];

    let i = 0;
    const n = src.length;
    let textStart = 0;

    const flushText = (end) => {
        const raw = src.slice(textStart, end);
        if (raw.trim().length === 0) return;
        texts.push({
            raw,
            text: decodeEntities(raw).replace(/\s+/g, ' ').trim(),
            start: textStart,
            end,
            parent: stack.length ? stack[stack.length - 1] : null,
            ancestors: stack.slice(),
        });
    };

    while (i < n) {
        if (src[i] !== '<') { ++i; continue; }

        if (src.startsWith('<!--', i)) {
            flushText(i);
            const close = src.indexOf('-->', i);
            i = close < 0 ? n : close + 3;
            textStart = i;
            continue;
        }

        if (src.startsWith('<!', i) || src.startsWith('<?', i)) {
            flushText(i);
            const close = src.indexOf('>', i);
            i = close < 0 ? n : close + 1;
            textStart = i;
            continue;
        }

        const closeTag = /^<\/([A-Za-z][-\w]*)\s*>/.exec(src.slice(i, i + 64));
        if (closeTag) {
            flushText(i);
            const tag = closeTag[1].toLowerCase();
            for (let k = stack.length - 1; k >= 0; --k) {
                if (stack[k].tag === tag) { stack[k].closeAt = i; stack.length = k; break; }
            }
            i += closeTag[0].length;
            textStart = i;
            continue;
        }

        const openTag = /^<([A-Za-z][-\w]*)/.exec(src.slice(i, i + 64));
        if (!openTag) { ++i; continue; }

        flushText(i);

        const tag = openTag[1].toLowerCase();
        const tagStart = i;
        let j = i + openTag[0].length;

        // Attribute scan. Quote-aware, so a `>` inside a value cannot end the
        // tag early — several pages carry inline style and onclick values that
        // contain one.
        const attrs = {};
        while (j < n && src[j] !== '>') {
            if (/\s/.test(src[j])) { ++j; continue; }
            if (src[j] === '/') { ++j; continue; }

            const nameStart = j;
            while (j < n && /[^\s=/>]/.test(src[j])) ++j;
            const name = src.slice(nameStart, j).toLowerCase();
            if (!name) { ++j; continue; }

            while (j < n && /\s/.test(src[j])) ++j;
            if (src[j] !== '=') { attrs[name] = { value: '', start: nameStart, end: j, quote: null, nameStart }; continue; }
            ++j;
            while (j < n && /\s/.test(src[j])) ++j;

            let quote = null, valueStart = j, value = '';
            if (src[j] === '"' || src[j] === "'") {
                quote = src[j];
                valueStart = ++j;
                while (j < n && src[j] !== quote) ++j;
                value = src.slice(valueStart, j);
                ++j;
            } else {
                while (j < n && /[^\s>]/.test(src[j])) ++j;
                value = src.slice(valueStart, j);
            }
            attrs[name] = { value, start: valueStart, end: valueStart + value.length, quote, nameStart };
        }

        const tagEnd = j < n ? j + 1 : n;
        const selfClosing = src[j - 1] === '/' || VOID_TAGS.has(tag);

        const el = {
            index: elements.length,
            tag,
            attrs,
            id: attrs.id ? attrs.id.value : null,
            classes: attrs.class ? attrs.class.value.split(/\s+/).filter(Boolean) : [],
            tagStart,
            tagEnd,
            closeAt: null,
            depth: stack.length,
            parent: stack.length ? stack[stack.length - 1] : null,
            childElements: 0,
        };
        if (el.parent) ++el.parent.childElements;
        elements.push(el);

        if (RAW_TAGS.has(tag) && !selfClosing) {
            const close = src.toLowerCase().indexOf(`</${tag}`, tagEnd);
            el.rawStart = tagEnd;
            el.rawEnd   = close < 0 ? n : close;
            el.raw      = src.slice(el.rawStart, el.rawEnd);
            i = close < 0 ? n : src.indexOf('>', close) + 1;
            textStart = i;
            continue;
        }

        if (!selfClosing) stack.push(el);
        i = tagEnd;
        textStart = i;
    }

    flushText(n);
    return { elements, texts };
}

// The Latin-1 named set plus the punctuation this repo's pages actually use.
// It has to cover the ACCENTED names, not just the structural five: a page that
// writes Fran&ccedil;ais leaves an undecoded entity looking like prose, which
// misclassifies an endonym as a label AND generates the key
// `lang-select.fran-ccedil-ais`. Both were real, on O-Tapestop, before this
// table grew. Values are written as \u escapes so a byte-level edit to this
// file cannot silently swap a non-breaking space for an ordinary one.
const ENTITIES = {
    amp: '&', lt: '<', gt: '>', quot: '"', apos: "'", nbsp: '\u00a0',
    mdash: '\u2014', ndash: '\u2013', hellip: '\u2026', times: '\u00d7',
    divide: '\u00f7', deg: '\u00b0', laquo: '\u00ab', raquo: '\u00bb',
    rsquo: '\u2019', lsquo: '\u2018', ldquo: '\u201c', rdquo: '\u201d',
    sbquo: '\u201a', bdquo: '\u201e', dagger: '\u2020', Dagger: '\u2021',
    permil: '\u2030', middot: '\u00b7', bull: '\u2022', minus: '\u2212',
    plusmn: '\u00b1', micro: '\u00b5', frac12: '\u00bd', frac14: '\u00bc',
    frac34: '\u00be', sup1: '\u00b9', sup2: '\u00b2', sup3: '\u00b3',
    larr: '\u2190', rarr: '\u2192', uarr: '\u2191', darr: '\u2193', harr: '\u2194',
    copy: '\u00a9', reg: '\u00ae', trade: '\u2122', sect: '\u00a7', para: '\u00b6',
    euro: '\u20ac', pound: '\u00a3', yen: '\u00a5', cent: '\u00a2', curren: '\u00a4',
    shy: '\u00ad', ensp: '\u2002', emsp: '\u2003', thinsp: '\u2009', zwnj: '', zwj: '',
    agrave: '\u00e0', Agrave: '\u00c0', aacute: '\u00e1', Aacute: '\u00c1',
    acirc: '\u00e2', Acirc: '\u00c2', atilde: '\u00e3', Atilde: '\u00c3',
    auml: '\u00e4', Auml: '\u00c4', aring: '\u00e5', Aring: '\u00c5',
    aelig: '\u00e6', AElig: '\u00c6', ccedil: '\u00e7', Ccedil: '\u00c7',
    egrave: '\u00e8', Egrave: '\u00c8', eacute: '\u00e9', Eacute: '\u00c9',
    ecirc: '\u00ea', Ecirc: '\u00ca', euml: '\u00eb', Euml: '\u00cb',
    igrave: '\u00ec', Igrave: '\u00cc', iacute: '\u00ed', Iacute: '\u00cd',
    icirc: '\u00ee', Icirc: '\u00ce', iuml: '\u00ef', Iuml: '\u00cf',
    ntilde: '\u00f1', Ntilde: '\u00d1',
    ograve: '\u00f2', Ograve: '\u00d2', oacute: '\u00f3', Oacute: '\u00d3',
    ocirc: '\u00f4', Ocirc: '\u00d4', otilde: '\u00f5', Otilde: '\u00d5',
    ouml: '\u00f6', Ouml: '\u00d6', oslash: '\u00f8', Oslash: '\u00d8',
    oelig: '\u0153', OElig: '\u0152',
    ugrave: '\u00f9', Ugrave: '\u00d9', uacute: '\u00fa', Uacute: '\u00da',
    ucirc: '\u00fb', Ucirc: '\u00db', uuml: '\u00fc', Uuml: '\u00dc',
    yacute: '\u00fd', Yacute: '\u00dd', yuml: '\u00ff', Yuml: '\u0178',
    szlig: '\u00df', scaron: '\u0161', Scaron: '\u0160',
    thorn: '\u00fe', eth: '\u00f0', iexcl: '\u00a1', iquest: '\u00bf',
};

function decodeEntities(s) {
    return s.replace(/&(#x?[0-9A-Fa-f]+|\w+);/g, (m, body) => {
        if (body[0] === '#') {
            const code = body[1] === 'x' || body[1] === 'X'
                ? parseInt(body.slice(2), 16) : parseInt(body.slice(1), 10);
            return Number.isFinite(code) ? String.fromCodePoint(code) : m;
        }
        return body in ENTITIES ? ENTITIES[body] : m;
    });
}

// ═══════════════════════════════════════════════════════════ JS scanner ═══

// Comment-stripped but position-preserving, so a line number reported against
// the stripped source still points at the right line of the original. Replacing
// a comment with spaces rather than deleting it is what buys that.
function stripJsComments(src) {
    let out = '';
    let i = 0;
    const n = src.length;
    let prevSig = '';
    const REGEX_PRECEDERS = new Set(['', '(', ',', '=', ':', '[', '!', '&', '|', '?',
                                     '{', '}', ';', '+', '-', '*', '%', '~', '^', '<', '>', 'n']);

    while (i < n) {
        const c = src[i], d = src[i + 1];

        if (c === '/' && d === '/') {
            while (i < n && src[i] !== '\n') { out += ' '; ++i; }
            continue;
        }
        if (c === '/' && d === '*') {
            while (i < n && !(src[i] === '*' && src[i + 1] === '/')) { out += src[i] === '\n' ? '\n' : ' '; ++i; }
            out += '  '; i += 2;
            continue;
        }
        if (c === '"' || c === "'" || c === '`') {
            const quote = c;
            out += c; ++i;
            while (i < n) {
                if (src[i] === '\\') { out += src[i] + (src[i + 1] || ''); i += 2; continue; }
                if (src[i] === quote) break;
                out += src[i]; ++i;
            }
            out += quote; ++i;
            prevSig = quote;
            continue;
        }
        if (c === '/' && REGEX_PRECEDERS.has(prevSig)) {
            let inClass = false;
            out += c; ++i;
            while (i < n) {
                if (src[i] === '\\') { out += src[i] + (src[i + 1] || ''); i += 2; continue; }
                if (src[i] === '[') inClass = true;
                else if (src[i] === ']') inClass = false;
                else if (src[i] === '/' && !inClass) break;
                else if (src[i] === '\n') break;
                out += src[i]; ++i;
            }
            out += src[i] || ''; ++i;
            prevSig = '/';
            continue;
        }

        out += c;
        if (!/\s/.test(c)) prevSig = c;
        ++i;
    }
    return out;
}

const lineOf = (src, offset) => src.slice(0, offset).split('\n').length;

// ══════════════════════════════════════════════════════ classification ════

// Units are language-neutral under D-03 and are the ONLY two-letter runs a
// READOUT may contain. Ordered longest-first so `kHz` is consumed before `Hz`.
const UNITS = ['dBFS', 'LUFS', 'dBu', 'dBV', 'kHz', 'BPM', 'RMS', 'MHz', 'dB', 'Hz',
               'ms', 'st', 'ct', 'px', 'sec', 'min', 'oct', 'semi', 'cents', 'cent',
               'deg', 'bit', 'bits', 'kB', 'MB', 'x', 's', 'Q', 'v', 'V'];

const ENDONYMS = ['English', 'Français', 'Francais', 'Deutsch', 'Español', 'Espanol',
                  'Italiano', 'Português', 'Nederlands', 'Svenska', 'Polski',
                  '日本語', '中文', '한국어', 'Русский'];

const LETTER_RUN = /[A-Za-zÀ-ÿ]{2}/;

function hasProse(s) { return LETTER_RUN.test(s); }

// Strips interpolations, numbers, punctuation and unit tokens. Whatever letters
// survive are prose.
function residue(s) {
    let t = String(s)
        .replace(/\$\{[^}]*\}/g, ' ')       // template interpolation
        .replace(/\{\w+\}/g, ' ')           // an i18n {token}
        .replace(/&[#\w]+;/g, ' ');         // an entity, already-decoded or not

    for (const u of UNITS) {
        t = t.replace(new RegExp(`(^|[^A-Za-zÀ-ÿ])${u.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')}(?![A-Za-zÀ-ÿ])`, 'g'), '$1 ');
    }

    return t.replace(/[-+0-9.,:;/()\[\]%°′″×–—…\s|]/g, ' ');
}

function classify(text, ctx = {}) {
    const t = String(text).trim();

    if (t.length === 0) return { cls: 'READOUT', reason: 'empty' };
    if (ENDONYMS.includes(t)) return { cls: 'ENDONYM', reason: 'a language name is never translated' };

    const bare = t.replace(/\$\{[^}]*\}/g, '').trim();
    if (UNITS.includes(bare)) return { cls: 'UNIT', reason: `unit symbol, language-neutral (D-03)` };

    if (!hasProse(residue(t)))
        return { cls: 'READOUT', reason: 'no two-letter run survives once interpolations, numbers and unit tokens are removed (D-03)' };

    if (ctx.presetContext)
        return { cls: 'UNSURE', reason: 'this element DISPLAYS a preset name — exempt under D-02, because the name IS the JSON filename. Confirm it is the name and not a caption beside it' };

    if (/^[←-⇿─-➿⬀-⯿️■-◿\s]+$/.test(t))
        return { cls: 'READOUT', reason: 'glyph-only, carries no words' };

    return { cls: 'LABEL', reason: 'prose survives the readout filter' };
}

// ═══════════════════════════════════════════════════════════ key naming ═══

function slug(s) {
    return String(s).toLowerCase()
        .normalize('NFD').replace(/[̀-ͯ]/g, '')
        .replace(/[^a-z0-9]+/g, '-')
        .replace(/^-|-$/g, '')
        .split('-').slice(0, 4).join('-') || 'x';
}

function describeElement(el) {
    if (!el) return 'document';
    if (el.id) return `#${el.id}`;
    const cls = el.classes.length ? '.' + el.classes.join('.') : '';
    return `${el.tag}${cls}`;
}

// The element's own id beats everything: `document.querySelector` returns the
// FIRST match in document order, which is precisely how O-Octagon's
// `.vunit-group` tip nearly landed on the wrong control in Stage C. Where the
// element has no id, the key is scoped by the nearest id'd ancestor so two
// identical captions in different panels cannot collide.
function suggestKey(el, text, taken) {
    let base;
    if (el && el.id) {
        base = el.id;
    } else {
        let anc = el ? el.parent : null;
        while (anc && !anc.id) anc = anc.parent;
        const scope = anc ? anc.id : (el && el.classes[0]) || 'ui';
        base = `${slug(scope)}.${slug(text)}`;
    }

    let key = base;
    let k = 2;
    while (taken.has(key)) key = `${base}-${k++}`;
    taken.add(key);
    return key;
}

// ══════════════════════════════════════════════════════════ the extract ═══

const VISIBLE_ATTRS = ['title', 'aria-label', 'placeholder', 'alt'];

// Vendored shared modules and bundles: they are not the plugin's own copy and
// editing them here would edit every plugin that embeds them, which the
// no-shared-module rule makes a much larger decision than this tool should take.
const JS_SKIP = [/(^|\/)js\/juce\//, /(^|\/)modules\//, /preset-manager\.js$/,
                 /tuning-panel\.js$/, /webview-drop-streaming\.js$/,
                 /\.bundle\.js$/, /\.min\.js$/, /i18n\.js$/];

function listJsFiles(uiRoot) {
    const out = [];
    const walk = (dir) => {
        for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
            const p = path.join(dir, e.name);
            if (e.isDirectory()) { walk(p); continue; }
            if (!e.name.endsWith('.js')) continue;
            const rel = path.relative(uiRoot, p).split(path.sep).join('/');
            if (JS_SKIP.some((re) => re.test(rel))) continue;
            out.push({ abs: p, rel });
        }
    };
    walk(uiRoot);
    return out.sort((a, b) => a.rel.localeCompare(b.rel));
}

function extractPlugin(name, opts = {}) {
    const ui = S.resolveUiRoot(name);
    if (!ui) return { name, error: 'no UI root' };

    const indexPath = path.join(ui.uiRoot, 'index.html');
    if (!fs.existsSync(indexPath)) return { name, error: 'no index.html' };

    const html = fs.readFileSync(indexPath, 'utf8');
    const { elements, texts } = scanHtml(html);

    const rows  = [];
    const taken = new Set();
    const patch = [];   // { at, insert } against index.html

    // ── existing keys, so a generated key REUSES a tooltip key rather than
    //    minting a near-duplicate. On most controls the tooltip TITLE already
    //    is the label text, and two keys holding the same string are two places
    //    for it to drift.
    const existing = readExistingI18n(ui.uiRoot);

    // ── 1. HTML text nodes ────────────────────────────────────────────────
    for (const t of texts) {
        if (!hasProse(t.text)) continue;
        const el = t.parent;
        if (el && (el.tag === 'script' || el.tag === 'style' || el.tag === 'title')) continue;

        const already = el && (el.attrs['data-i18n'] || el.attrs['data-tip'] || el.attrs['data-tooltip']);
        const presetContext = looksLikePresetName(el);
        const { cls, reason } = classify(t.text, { presetContext });

        // A text node whose parent also holds element children is a mixed node.
        // Keying the PARENT would make applyLabel's textContent write delete
        // those children — the systemic form of the failure that erased
        // authored captions once already
        // (pattern_js_state_updater_overwrites_html_labels).
        const mixed = el ? el.childElements > 0 : false;

        const reuse = existing.byText.get(t.text) || null;
        const key = reuse || suggestKey(el, t.text, taken);

        rows.push({
            source: 'html-text',
            file: 'index.html',
            line: lineOf(html, t.start),
            selector: describeElement(el),
            attr: '',
            cls: mixed && cls === 'LABEL' ? 'UNSURE' : cls,
            reason: mixed && cls === 'LABEL'
                ? 'the element holds element children as well as this text — key the text in its own <span>, or applyLabel will delete the siblings'
                : reason,
            keyed: already ? 'yes' : 'no',
            key: reuse ? `${key} (reuses an existing entry)` : key,
            text: t.text,
        });

        if (cls === 'LABEL' && !mixed && !already && el)
            patch.push({ at: elementAttrInsertPoint(el), insert: ` data-i18n="${key}"` });
    }

    // ── 2. visible-text attributes ────────────────────────────────────────
    for (const el of elements) {
        for (const a of VISIBLE_ATTRS) {
            const rec = el.attrs[a];
            if (!rec || !hasProse(decodeEntities(rec.value))) continue;

            const text = decodeEntities(rec.value).replace(/\s+/g, ' ').trim();
            const { cls, reason } = classify(text, { presetContext: looksLikePresetName(el) });

            // Contract section 4: a native title= on an element that also has a
            // data-tip produces a second, untranslated OS tooltip competing with
            // the measure-then-pin renderer. It is DELETED, not localized.
            const isCompetingTitle = a === 'title'
                && !!(el.attrs['data-tip'] || el.attrs['data-tooltip'] || el.attrs['data-tip-title']);

            const keyedAlready = !!el.attrs[`data-i18n-${a === 'aria-label' ? 'aria' : a}`];
            const key = suggestKey(el, `${text} ${a}`, taken);

            rows.push({
                source: 'html-attr',
                file: 'index.html',
                line: lineOf(html, rec.start),
                selector: describeElement(el),
                attr: a,
                cls: isCompetingTitle ? 'DELETE' : cls,
                reason: isCompetingTitle
                    ? 'native title= on an element that already carries a data-tip — DELETE it (contract §4); it renders a competing untranslated OS tooltip'
                    : (a === 'title' && cls === 'LABEL'
                        ? 'native title= is the only help this element has — move the text to data-i18n-aria (contract §4)'
                        : reason),
                keyed: keyedAlready ? 'yes' : 'no',
                key: `${key}`,
                text,
            });
        }
    }

    // ── 3. JS-written strings ─────────────────────────────────────────────
    const jsSources = [];
    for (const f of listJsFiles(ui.uiRoot))
        jsSources.push({ label: f.rel, code: fs.readFileSync(f.abs, 'utf8') });

    // The controller is an inline <script type="module"> on O-Bitrot. Reading
    // only js/app.js there would report "zero JS prose" on a plugin that has
    // some — the same wrong-shaped assumption check-i18n.js already had to fix.
    for (const el of elements) {
        if (el.tag !== 'script' || !el.raw) continue;
        const type = el.attrs.type ? el.attrs.type.value : '';
        if (type && type !== 'module' && type !== 'text/javascript') continue;
        if (el.raw.trim().length === 0) continue;
        jsSources.push({ label: `index.html inline <script${type ? ` type="${type}"` : ''}> @line ${lineOf(html, el.tagStart)}`,
                         code: el.raw, inlineOffset: el.rawStart, inlineHtml: html });
    }

    for (const src of jsSources) rows.push(...scanJsSource(src, taken));

    // ── artefacts ─────────────────────────────────────────────────────────
    const outDir = opts.outDir || path.join(S.pluginRoot(name), '.planning');
    const result = {
        name,
        uiRoot: ui.label,
        rows,
        counts: countBy(rows),
        outDir,
        written: [],
    };

    if (!opts.dryRun) {
        fs.mkdirSync(outDir, { recursive: true });

        const tsv = path.join(outDir, 'i18n-inventory.tsv');
        fs.writeFileSync(tsv, renderTsv(name, ui.label, rows));
        result.written.push(tsv);

        const draft = path.join(outDir, 'i18n-index-draft.html');
        fs.writeFileSync(draft, applyPatch(html, patch));
        result.written.push(draft);

        const skel = path.join(outDir, 'i18n-labels-skeleton.js');
        fs.writeFileSync(skel, renderSkeleton(name, rows, existing));
        result.written.push(skel);
    }

    return result;
}

function ancestorMatches(el, re) {
    let a = el;
    while (a) {
        if (a.id && re.test(a.id)) return a;
        if (a.classes.some((c) => re.test(c))) return a;
        a = a.parent;
    }
    return null;
}

// "Sits near a preset control" is far too wide: it swept up O-Tapestop's Save,
// Load and Delete BUTTON captions, which are ordinary labels that must be
// localized, and reporting them UNSURE alongside the one genuine preset-name
// node buries the row that matters. A preset NAME is displayed, not actuated —
// so this is the element itself naming a preset display, or an <option> inside
// a preset control. Never an ancestor, never a <button>.
function looksLikePresetName(el) {
    if (!el) return false;
    if (el.tag === 'button' || el.tag === 'a') return false;
    if (el.tag === 'option') return !!ancestorMatches(el, /preset|patch/i);
    const own = [el.id || '', ...el.classes].join(' ');
    return /(preset|patch)[-_]?(name|current|display|title|value)/i.test(own);
}

// Immediately before the FIRST existing attribute, so the new one lands at the
// head of the tag and every existing attribute keeps its order, its quoting and
// its line breaks — several tags here wrap across four lines and rewriting them
// would bury the one-attribute change in a whitespace diff.
function elementAttrInsertPoint(el) {
    const starts = Object.values(el.attrs).map((a) => a.nameStart).filter(Number.isFinite);
    return starts.length ? Math.min(...starts) : el.tagEnd - 1;
}

// Applied back-to-front so an earlier insertion cannot shift a later offset.
function applyPatch(src, patch) {
    const sorted = patch.slice().sort((a, b) => b.at - a.at);
    let out = src;
    for (const p of sorted) out = out.slice(0, p.at) + p.insert.trim() + ' ' + out.slice(p.at);
    return out;
}

// ── the JS scanner proper ──────────────────────────────────────────────────

// A string is MARKUP when it opens a tag — `<name` followed by whitespace, a
// slash or the closing bracket. Deliberately narrow: `a < b && c` and the
// comparison `if (x<y)` are not markup, and neither is a sentence containing a
// stray angle bracket such as the caption "Vel>Flt".
function looksLikeMarkup(value) {
    return /<[A-Za-z][-\w]*(\s|\/|>)/.test(value);
}

// The visible-text attributes, and the data-* attribute that keys each one.
// Same table as check-i18n.js assertion 11, and the same rule: `title` has no
// keyed form because contract section 4 DELETES it rather than localizing it.
const MARKUP_KEYED_BY = {
    'aria-label':  'data-i18n-aria',
    'placeholder': 'data-i18n-placeholder',
    'alt':         'data-i18n-alt',
};

// Parse an innerHTML payload and return the pieces of it that are genuinely
// unkeyed user-visible copy, each with its OFFSET INSIDE THE PAYLOAD so the
// reported line number lands on the offending line rather than on the opening
// backtick of a hundred-line template.
function markupRows(html) {
    const { elements, texts } = scanHtml(html);
    const out = [];

    const keyedAncestor = (el) => {
        let a = el;
        while (a) { if (a.attrs['data-i18n']) return true; a = a.parent; }
        return false;
    };

    for (const t of texts) {
        const el = t.parent;
        if (el && (el.tag === 'script' || el.tag === 'style' || el.tag === 'title')) continue;
        if (el && keyedAncestor(el)) continue;
        out.push({ offset: t.start, text: t.text, attr: '' });
    }

    for (const el of elements) {
        // `.start` on an attribute record is the offset of its VALUE, not of
        // its name — see the attribute scanner in scanHtml above.
        if (el.attrs.title)
            out.push({ offset: el.attrs.title.start,
                       text: decodeEntities(el.attrs.title.value), attr: 'title' });

        for (const [attr, keyAttr] of Object.entries(MARKUP_KEYED_BY)) {
            const rec = el.attrs[attr];
            if (!rec || el.attrs[keyAttr]) continue;
            out.push({ offset: rec.start, text: decodeEntities(rec.value), attr });
        }
    }

    return out;
}

// Every i18n KEY declared inside markup a module INJECTS with innerHTML.
//
// check-i18n assertion 15's dead-key sweep collects references from three
// places: attributes parsed out of index.html, plain-literal setLabel keys, and
// plain-literal `.dataset.i18n* =` assignments. All three read the page as
// AUTHORED. A module that builds a subtree from a markup template and keys the
// captions inside it — which is how O-IntonationPad's tuning panel writes all
// 37 of its captions — declares its keys in none of those three places, so
// every one of them reported DEAD while being read on every language change.
//
// Same discovery path as the innerHTML branch of scanJsSource above, so the two
// assertions agree by construction: a template assertion 12 parses for unkeyed
// copy is the same template this reads keys out of. Comments are stripped, so a
// commented-out template does not resurrect a key.
function markupKeyRefs(code) {
    const stripped = stripJsComments(code);
    const keys = new Set();

    // EVERY MARKUP LITERAL IN THE MODULE, not only one sitting syntactically on
    // an `.innerHTML =` right-hand side.
    //
    // The RHS rule was wrong-shaped and this is the sixteenth of that shape in
    // this task. It reads `el.innerHTML = \'<div data-i18n="k">\'` and misses the
    // commonest way a page in this repo builds a subtree:
    //
    //     let html = `<div class="hdr" data-i18n="label.h">Intervals</div>`;
    //     for (...) html += `<div class="row">...</div>`;
    //     list.innerHTML = html;
    //
    // The RHS of that assignment is the identifier `html` and carries no
    // literal at all, so every key declared in the template reported DEAD while
    // being read on every language change — O-Marimba\'s interval column
    // declares seven that way. A negative control confirmed the same rule makes
    // assertion 12 VACUOUS on the identical shape: raw unkeyed English inside an
    // accumulated template passes green, which is the more serious half.
    //
    // Scanning every literal that looksLikeMarkup() is MONOTONE here: this
    // function only ever ADDS to assertion 15\'s "referenced" set, so a broader
    // sweep can retire a false dead-key report and can never invent a failure.
    // The paths it now also covers — insertAdjacentHTML, outerHTML,
    // createContextualFragment, a template held in a const — were invisible to
    // the old rule for exactly the same reason.
    //
    // THE MIRROR-IMAGE HALF IS DELIBERATELY NOT FIXED HERE, and is named rather
    // than left to be rediscovered. scanJsSource() has the same RHS-only
    // discovery rule, which makes assertion 12 VACUOUS on the accumulator
    // shape: raw unkeyed English inside a template built up with `html +=` and
    // written to innerHTML later passes green. Measured with a negative
    // control, not reasoned.
    //
    // It is not fixed in this commit because the obvious broadening ARGUES WITH
    // CORRECT CODE. O-Lyrica v2.4.0 keys five such nodes by id AFTER injection —
    // window.__setLabel(document.getElementById(\'interval-list-header\'), ...) —
    // which is legal canon and is what its own CHANGELOG says it does; the
    // English left in the template is the authored fallback contract §1
    // requires. A broadened assertion 12 reported all five as violations of a
    // rule the code was obeying, for the sixteenth time in this task. Fixing it
    // properly needs the "keyed after injection by id" arm AND a nested-template
    // literal reader, and half of that is worse than none.
    for (const l of collectLiterals(stripped)) {
        if (!looksLikeMarkup(l.value)) continue;
        for (const el of scanHtml(l.value).elements)
            for (const a of ['data-i18n', 'data-i18n-aria', 'data-i18n-placeholder', 'data-i18n-alt'])
                if (el.attrs[a] && el.attrs[a].value) keys.add(el.attrs[a].value);
    }

    return keys;
}

function scanJsSource(src, taken) {
    const rows = [];
    const code = stripJsComments(src.code);

    const emit = (kind, offset, text, extra) => {
        const line = src.inlineOffset != null
            ? lineOf(src.inlineHtml, src.inlineOffset + offset)
            : lineOf(src.code, offset);
        const base = classify(text);

        let cls = base.cls;
        let reason = base.reason;

        if (cls === 'LABEL' && kind === 'js-composed')
            reason = 'prose sits OUTSIDE the interpolation — needs a {token} entry, not a flat one';
        if (cls === 'LABEL' && extra.conditional)
            reason = 'written from a CONDITIONAL expression (ternary / || fallback). Contract §6: the inflection is authored around, not engineered — check-i18n assertion 13 rejects a ternary inside a setLabel argument';

        rows.push({
            source: kind,
            file: src.label,
            line,
            selector: extra.receiver || '',
            attr: extra.attr || '',
            cls,
            reason,
            keyed: extra.keyed ? 'yes' : 'no',
            key: suggestKey(null, text, taken),
            text,
        });
    };

    // ── the RHS of a textContent / innerText / innerHTML assignment ──────
    //
    // Reading only a literal that sits IMMEDIATELY after the `=` misses every
    // interesting shape. On O-Bitrot alone that rule found 0 of 12 sites: the
    // real ones are `btn.textContent = on ? "On" : "Off"` and
    // `btn.textContent = btn.dataset.label || "Delete"`. Those conditionals are
    // exactly what contract §6 is about, so they must be REPORTED, not dropped.
    // The whole RHS is read and every literal in it is collected.
    // The RECEIVER is not matched, only described. An earlier version required
    // the receiver to be an identifier path and so missed
    // `document.getElementById("status").textContent = "Free Run"` entirely —
    // the parentheses are not in an identifier character class, and that is one
    // of the two commonest shapes in this repo. A negative control caught it.
    // `+=` is included: appending prose ships English exactly as assigning it does.
    for (const m of code.matchAll(/\.\s*(textContent|innerText|innerHTML)\s*(\+?=)(?!=)\s*/g)) {
        const rhsAt = m.index + m[0].length;
        const rhs   = readExpression(code, rhsAt, ';');
        if (!rhs) continue;

        const before = code.slice(Math.max(0, m.index - 48), m.index);
        const receiver = `${(before.match(/[\w$.\[\]()'"]*$/) || [''])[0].slice(-32)}.${m[1]}`;

        const lits = collectLiterals(rhs.text);
        const prose = lits.filter((l) => hasProse(l.value));
        if (prose.length === 0) continue;

        const conditional = lits.length > 1 || /\?|\|\||\?\?|&&/.test(stripLiterals(rhs.text));

        for (const l of prose) {
            const kind = l.template && /\$\{/.test(l.value) ? 'js-composed' : 'js-prose';

            // ── innerHTML IS MARKUP, AND MARKUP IS NOT COPY ──────────────
            //
            // hasProse() asks "does this string contain a run of two letters",
            // which is the right question for a textContent assignment and the
            // WRONG one for an innerHTML assignment: `div`, `class`, `span` and
            // `data-i18n` are all runs of two letters, so a template that
            // carries no user-visible word at all reported as prose. The gate
            // was describing a violation of a rule the code was obeying —
            // `container.innerHTML = '<div class="tk-hint" data-i18n="label.tkHint"></div>'`
            // has no copy in it whatsoever and was reported as a raw prose
            // write. It had never fired before because no shipped canon-v2
            // plugin built markup with innerHTML; O-IntonationPad's tuning panel
            // is the first, and it builds its whole skeleton that way.
            //
            // The fix is NOT to skip anything that looks like a tag — that
            // would let `el.innerHTML = '<div>Hold 2+ notes</div>'` through,
            // which IS unkeyed copy. The markup is PARSED, and its text nodes
            // and visible-text attributes are judged by exactly the rules
            // assertions 10 and 11 already apply to index.html: a text node
            // inside a [data-i18n] element is keyed and is not reported, one
            // outside it is; an aria-label / placeholder / alt is keyed when its
            // data-i18n-* twin sits on the same element; and a native title= is
            // ALWAYS reported, because contract section 4 deletes it and
            // assertion 11 only ever looked in index.html so an injected one was
            // invisible to every gate in the repo.
            //
            // Strictly stronger than what it replaces, not weaker.
            if (m[1] === 'innerHTML' && looksLikeMarkup(l.value)) {
                for (const r of markupRows(l.value))
                    emit(kind, rhsAt + l.start + r.offset, r.text,
                         { receiver, conditional, attr: r.attr });
                continue;
            }

            emit(kind, rhsAt + l.start, l.value, { receiver, conditional });
        }
    }

    // ── setAttribute on one of the four visible-text attributes ─────────
    for (const m of code.matchAll(/\.setAttribute\s*\(\s*(['"])(title|aria-label|placeholder|alt)\1\s*,\s*/g)) {
        const argAt = m.index + m[0].length;
        const arg   = readExpression(code, argAt, ')');
        if (!arg) continue;

        const lits  = collectLiterals(arg.text);
        const prose = lits.filter((l) => hasProse(l.value));
        if (prose.length === 0) continue;

        const conditional = lits.length > 1 || /\?|\|\||\?\?|&&/.test(stripLiterals(arg.text));

        for (const l of prose)
            emit(l.template && /\$\{/.test(l.value) ? 'js-composed' : 'js-prose',
                 argAt + l.start, l.value,
                 { receiver: 'setAttribute', attr: m[2], conditional });
    }

    return rows;
}

// NOTE: there is deliberately no "skip a setLabel call" filter here. A site
// converted to setLabel(el, key) writes no textContent at all, so it produces no
// row in the first place. A filter would be dead code pretending to be a rule.

// Reads an expression from `at` up to the first `stop` character that sits at
// nesting depth zero, honouring strings, templates and nested template
// interpolations. A newline at depth zero also ends it, so a missing semicolon
// cannot swallow the rest of the file.
function readExpression(code, at, stop) {
    let i = at, depth = 0;
    const n = code.length;

    while (i < n) {
        const c = code[i];

        if (c === '"' || c === "'" || c === '`') {
            const lit = readLiteralAt(code, i);
            i = lit ? lit.end : i + 1;
            continue;
        }
        if (c === '(' || c === '[' || c === '{') { ++depth; ++i; continue; }
        if (c === ')' || c === ']' || c === '}') {
            if (depth === 0 && c === stop) break;
            --depth; ++i; continue;
        }
        if (depth === 0 && c === stop) break;
        if (depth === 0 && c === '\n' && stop === ';') {
            // An unterminated statement on one line is normal (ASI). Stop only
            // if the next non-space character cannot continue the expression.
            const rest = code.slice(i + 1).match(/^\s*(\S)/);
            if (!rest || !/[?:.,+\-*/%&|)\]}]/.test(rest[1])) break;
        }
        ++i;
    }

    return i > at ? { text: code.slice(at, i), start: at, end: i } : null;
}

function readLiteralAt(code, i) {
    const q = code[i];
    if (q !== '"' && q !== "'" && q !== '`') return null;

    let j = i + 1, value = '';
    while (j < code.length) {
        if (code[j] === '\\') {
            const esc = code[j + 1];
            value += esc === 'n' ? '\n' : esc === 't' ? '\t' : esc;
            j += 2;
            continue;
        }
        if (q === '`' && code[j] === '$' && code[j + 1] === '{') {
            // Copy the interpolation through verbatim so classify() can see
            // where the prose sits relative to it, and so a nested template or
            // a `}` inside a string cannot end the literal early.
            let d = 0, k = j;
            for (; k < code.length; ++k) {
                if (code[k] === '{') ++d;
                else if (code[k] === '}') { if (--d === 0) { ++k; break; } }
                else if (code[k] === '"' || code[k] === "'" || code[k] === '`') {
                    const inner = readLiteralAt(code, k);
                    if (inner) k = inner.end - 1;
                }
            }
            value += code.slice(j, k);
            j = k;
            continue;
        }
        if (code[j] === q) break;
        value += code[j];
        ++j;
    }
    return { value, template: q === '`', start: i, end: j + 1 };
}

function collectLiterals(text) {
    const out = [];
    let i = 0;
    while (i < text.length) {
        const c = text[i];
        if (c === '"' || c === "'" || c === '`') {
            const lit = readLiteralAt(text, i);
            if (lit) { out.push(lit); i = lit.end; continue; }
        }
        ++i;
    }
    return out;
}

// Blanks out literals so a `?` inside a string cannot be mistaken for a ternary.
function stripLiterals(text) {
    let out = '', i = 0;
    while (i < text.length) {
        const c = text[i];
        if (c === '"' || c === "'" || c === '`') {
            const lit = readLiteralAt(text, i);
            if (lit) { out += ' '.repeat(lit.end - i); i = lit.end; continue; }
        }
        out += c;
        ++i;
    }
    return out;
}

// ── the two entry points check-i18n.js calls ───────────────────────────────
//
// Exposed so the GATE and the WORKLIST count the same things. Two independent
// scanners disagreeing about what a label is would produce a gate that
// contradicts the inventory a plugin stage is working from, and the resolution
// would be a judgement call every time.

function extractJsRows(src) {
    return scanJsSource(src, new Set());
}

// Every setLabel(el, key, vars) call in a module, with the facts assertions 13
// and 15 need: whether the key is a plain string literal, and whether any
// argument carries inflection logic.
function readSetLabelCalls(code) {
    const stripped = stripJsComments(code);
    const calls = [];

    // BOTH SPELLINGS OF THE SAME CALL. The canon publishes `window.__setLabel =
    // setLabel` and says, in its own comment, that this is how a SIBLING MODULE
    // writes a localized label without app.js having to export anything. So
    // `window.__setLabel(el, 'key')` is not a workaround — it is the only
    // in-canon spelling available to js/venue.js, and a scan that matched only
    // the bare name reported every one of those keys as DEAD while treating a
    // ternary in the same call as invisible to assertion 13.
    //
    // Same shape as the dataset.i18nAria gap fixed in a1d80957: the gate
    // describing a violation of a rule the code is obeying, because the only
    // legal way to do the thing was outside what the regex could see.
    for (const m of stripped.matchAll(
            /(?:^|[^.\w$])setLabel\s*\(|(?:^|[^\w$])(?:window\.)?__setLabel\s*\(/g)) {
        // `function setLabel(el, key, vars)` is the DEFINITION, not a call. Its
        // parameter list reads as a non-literal key and reported the canon
        // block itself as a violation of assertion 13 the first time this ran.
        const nameAt = m.index + m[0].indexOf('setLabel');
        if (/\b(function|class)\s+$/.test(stripped.slice(Math.max(0, nameAt - 20), nameAt))) continue;

        const argsAt = m.index + m[0].length;
        const args   = readExpression(stripped, argsAt, ')');
        if (!args) continue;

        // Split the argument list at top-level commas. A comma inside a nested
        // call, an object literal or a template interpolation is not a
        // separator, and treating one as such would read `{a: 1, b: 2}` as two
        // arguments and report a perfectly ordinary call as malformed.
        const parts = [];
        let depth = 0, start = 0, i = 0;
        while (i < args.text.length) {
            const c = args.text[i];
            if (c === '"' || c === "'" || c === '`') {
                const lit = readLiteralAt(args.text, i);
                i = lit ? lit.end : i + 1;
                continue;
            }
            if (c === '(' || c === '[' || c === '{') ++depth;
            else if (c === ')' || c === ']' || c === '}') --depth;
            else if (c === ',' && depth === 0) { parts.push(args.text.slice(start, i)); start = i + 1; }
            ++i;
        }
        parts.push(args.text.slice(start));

        const keyArg = (parts[1] || '').trim();
        const keyLit = keyArg && /^['"]/.test(keyArg) ? readLiteralAt(keyArg, 0) : null;
        const wholeCallBare = stripLiterals(args.text);

        calls.push({
            line: lineOf(code, argsAt),
            key: keyLit && keyLit.end === keyArg.length ? keyLit.value : null,
            args: parts.map((x) => x.trim()),
            conditional: /\?[^.]|\|\||\?\?/.test(wholeCallBare),
        });
    }

    return calls;
}

// ── reading a plugin's existing i18n.js, if it has one ─────────────────────
function readExistingI18n(uiRoot) {
    const f = path.join(uiRoot, 'js', 'i18n.js');
    const out = { keys: new Set(), byText: new Map(), present: false };
    if (!fs.existsSync(f)) return out;

    const src = fs.readFileSync(f, 'utf8');
    out.present = true;

    // The table is data; evaluating it is what check-i18n already does, and its
    // assertion 7 independently proves the file has nothing but declarations.
    try {
        const vm = require('vm');
        const transformed = src.replace(/(^|\n)(\s*)export\s+(const|let|function|class)\s/g, '$1$2$3 ');
        const sandbox = { console: { warn() {}, error() {}, log() {} } };
        vm.createContext(sandbox);
        vm.runInContext(`${transformed}\n;globalThis.__x = { I18N: typeof I18N === 'undefined' ? {} : I18N };`,
                        sandbox, { timeout: 5000 });
        const I18N = sandbox.__x.I18N || {};
        for (const k of Object.keys(I18N)) {
            out.keys.add(k);
            const t = I18N[k] && I18N[k].en && I18N[k].en.t;
            if (typeof t === 'string' && t.trim() && !out.byText.has(t.trim())) out.byText.set(t.trim(), k);
        }
    } catch (e) { out.error = e.message; }

    return out;
}

// ══════════════════════════════════════════════════════════════ output ════

function countBy(rows) {
    const c = {};
    for (const r of rows) {
        c[r.cls] = (c[r.cls] || 0) + 1;
        c[`src:${r.source}`] = (c[`src:${r.source}`] || 0) + 1;
    }
    return c;
}

const TSV_COLS = ['source', 'file', 'line', 'selector', 'attr', 'class', 'keyed', 'suggestedKey', 'text', 'reason'];
const cell = (v) => String(v == null ? '' : v).replace(/\\/g, '\\\\').replace(/\t/g, '\\t').replace(/\n/g, '\\n');

function renderTsv(name, uiRoot, rows) {
    const lines = [
        `# plugin\t${name}`,
        `# uiRoot\t${uiRoot}`,
        `# generated\t${new Date().toISOString()}`,
        '# NOTE\tclass is a SUGGESTION WITH A REASON, never a silent drop. Read every UNSURE and every DELETE row.',
        '# ' + TSV_COLS.join('\t'),
    ];
    for (const r of rows)
        lines.push([r.source, r.file, r.line, r.selector, r.attr, r.cls, r.keyed, r.key, r.text, r.reason].map(cell).join('\t'));
    return lines.join('\n') + '\n';
}

function renderSkeleton(name, rows, existing) {
    const labels = rows.filter((r) => r.cls === 'LABEL' && r.keyed === 'no');
    const seen = new Set();

    const body = [];
    for (const r of labels) {
        const key = r.key.replace(/ \(reuses.*$/, '');
        if (seen.has(key)) continue;
        seen.add(key);

        if (existing.keys.has(key)) {
            body.push(`    // '${key}' already exists in I18N (its tooltip title is this label) — trLabel falls`);
            body.push(`    // back to it, so NO entry is needed here. Source: ${r.file}:${r.line}`);
            body.push('');
            continue;
        }

        body.push(`    // ${r.file}:${r.line}  ${r.selector}${r.attr ? ' @' + r.attr : ''}`);
        body.push(`    '${key}': {`);
        body.push(`        en: { t: ${JSON.stringify(r.text)}, b: '' },`);
        body.push(`        fr: { t: 'TODO', b: '', reviewed: false },`);
        body.push('    },');
        body.push('');
    }

    return `// ============================================================================
// i18n-labels-skeleton.js — GENERATED for ${name} by scripts/i18n-extract.js
//
// NOT a shippable file. Paste the reviewed entries into
// <uiroot>/js/i18n.js as the LABELS export, then delete this file.
//
// Every en value below was MOVED verbatim from the source, never re-typed. The
// Stage D retrofits compared their generated tables back against the markup
// byte-for-byte and that comparison is what proved nothing had drifted; do the
// same here before pasting.
//
// The French is the literal string 'TODO' on purpose. check-i18n assertion 4
// rejects a straight English passthrough and assertion 5 demands an explicit
// reviewed flag, so an unfilled skeleton CANNOT pass.
//
// ${labels.length} label candidate(s), ${seen.size} distinct key(s).
// ============================================================================

export const LABELS = Object.freeze({

${body.join('\n')}
});
`;
}

// ═══════════════════════════════════════════════════════════════ main ═════

const argv = process.argv.slice(2);
const val  = (f) => { const i = argv.indexOf(f); return i >= 0 && i + 1 < argv.length ? argv[i + 1] : null; };
const only = val('--plugin');
const all  = argv.includes('--all');
const opts = { outDir: val('--out-dir'), dryRun: argv.includes('--dry-run') };

if (require.main === module) {
    if (!only && !all) {
        console.log('usage: node scripts/i18n-extract.js --plugin <Name> | --all [--out-dir DIR] [--dry-run]');
        process.exit(2);
    }

    const names = all ? S.listPlugins() : [only];
    const summary = [];

    for (const n of names) {
        const perPluginOut = opts.outDir && all ? path.join(opts.outDir, n) : opts.outDir;
        const r = extractPlugin(n, { ...opts, outDir: perPluginOut });

        if (r.error) { console.log(`  SKIP ${n}: ${r.error}`); continue; }

        summary.push(r);

        if (!all) {
            console.log(`i18n-extract — ${n} (${r.uiRoot})`);
            for (const [k, v] of Object.entries(r.counts).sort()) console.log(`  ${k.padEnd(20)} ${v}`);
            for (const f of r.written) console.log(`  wrote ${f}`);
        }
    }

    if (all) {
        const num = (r, k) => r.counts[k] || 0;
        summary.sort((a, b) => num(b, 'LABEL') - num(a, 'LABEL'));

        console.log('\ni18n-extract --all  (size-ranked worklist)\n');
        console.log('  plugin                          LABEL  UNSURE  DELETE  READOUT   text   attr  js-prose  js-comp');
        for (const r of summary) {
            console.log(`  ${r.name.padEnd(30)} ${String(num(r, 'LABEL')).padStart(5)} `
                + `${String(num(r, 'UNSURE')).padStart(7)} ${String(num(r, 'DELETE')).padStart(7)} `
                + `${String(num(r, 'READOUT')).padStart(8)} ${String(num(r, 'src:html-text')).padStart(6)} `
                + `${String(num(r, 'src:html-attr')).padStart(6)} ${String(num(r, 'src:js-prose')).padStart(9)} `
                + `${String(num(r, 'src:js-composed')).padStart(8)}`);
        }

        const tot = (k) => summary.reduce((a, r) => a + num(r, k), 0);
        console.log(`\n  ${'TOTAL'.padEnd(30)} ${String(tot('LABEL')).padStart(5)} `
            + `${String(tot('UNSURE')).padStart(7)} ${String(tot('DELETE')).padStart(7)} `
            + `${String(tot('READOUT')).padStart(8)} ${String(tot('src:html-text')).padStart(6)} `
            + `${String(tot('src:html-attr')).padStart(6)} ${String(tot('src:js-prose')).padStart(9)} `
            + `${String(tot('src:js-composed')).padStart(8)}`);
        console.log(`\n  ${summary.length} plugins scanned.`);
    }
}

module.exports = {
    scanHtml, stripJsComments, classify, residue, suggestKey, extractPlugin,
    looksLikeMarkup, markupRows, markupKeyRefs,
    decodeEntities, extractJsRows, readSetLabelCalls, readExpression, collectLiterals,
};
