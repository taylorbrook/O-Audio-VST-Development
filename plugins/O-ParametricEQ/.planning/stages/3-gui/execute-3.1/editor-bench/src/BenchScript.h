#pragma once

// Injected through the real editor's OPEQ_EDITOR_BENCH hook (withUserScript):
// runs at document start, after JUCE's low-level integration script
// (window.__JUCE__.backend exists) and before the page's modules. The page is
// the real Source/ui/public/index.html, unmodified.
//
// Reports go to C++ through the editor's bench-only native function
// `benchReport`, invoked with a fixed negative resultId so the page's own
// PromiseHandler (ids from 0 up) ignores the completion.
//
// The driver calls window.__bench.* with evaluateJavascript. Script order is
// preserved by WKWebView, so a check queued after a burst of parameter events
// runs after them.
static const char* kBenchScript = R"JS(
(function () {
  const BENCH_ID = -987654321;
  const be = window.__JUCE__.backend;
  const S = { expected: 196, props: new Set(), allPropsDone: false, check: {},
              evals: 0, posts: 0, armed: null, frame: 0, ticking: false, idle: null,
              warns: [], nWarn: 0, nErr: 0, unknownWarn: 0, natives: {}, nativeOrder: [] };
  window.__benchS = S;

  const rawPost = window.__JUCE__.postMessage;
  const P = (type, extra) => {
    try {
      const o = Object.assign({ type: type, pnow: performance.now() }, extra || {});
      rawPost.call(window.__JUCE__, JSON.stringify({ eventId: '__juce__invoke',
        payload: { name: 'benchReport', params: [o], resultId: BENCH_ID } }));
    } catch (e) {}
  };
  P('scriptStart');

  // -- Console + error capture --
  const keep = (kind, args) => {
    const msg = Array.prototype.map.call(args, (a) => { try { return String(a && a.message ? a.message : a); } catch (e) { return '?'; } }).join(' ');
    if (msg.indexOf('unknown to the backend') >= 0) S.unknownWarn++;
    if (S.warns.length < 20) S.warns.push(kind + ': ' + msg.slice(0, 240));
  };
  const ow = console.warn.bind(console), oe = console.error.bind(console);
  console.warn  = function () { S.nWarn++; keep('warn', arguments);  ow.apply(null, arguments); };
  console.error = function () { S.nErr++;  keep('error', arguments); oe.apply(null, arguments); };
  window.addEventListener('error', (e) => { S.nErr++; keep('onerror', [e.message + ' @' + e.filename + ':' + e.lineno]); });
  window.addEventListener('unhandledrejection', (e) => { S.nErr++; keep('rejection', [e.reason]); });

  // -- Real input (diagnostic): pointer / wheel / key events reaching the page.
  // The bench issues none during idle; isTrusted separates a person's input
  // from the bench's own synthetic events (typed entry, chip click).
  S.input = [];
  for (const ty of ['pointerdown', 'pointerup', 'wheel', 'keydown', 'dblclick']) {
    window.addEventListener(ty, (e) => {
      if (S.input.length < 40)
        S.input.push({ type: ty, trusted: e.isTrusted, x: Math.round(e.clientX || 0), y: Math.round(e.clientY || 0),
                       t: Math.round(performance.now()), idle: !!S.idle });
    }, true);
  }

  // -- Page -> C++ messages (bench reports go through rawPost, not counted) --
  window.__JUCE__.postMessage = function (s) {
    const m = /"eventId":"([^"]*)"/.exec(s);
    const id = m ? m[1] : '?';
    S.posts++;
    if (S.idle) S.idle.posts[id] = (S.idle.posts[id] || 0) + 1;
    if (id === '__juce__invoke') {
      const n = /"name":"([^"]*)"/.exec(s);
      const name = n ? n[1] : '?';
      S.natives[name] = (S.natives[name] || 0) + 1;
      if (S.nativeOrder.length < 40) S.nativeOrder.push(name);
      if (name === 'pageReady') {
        const r = rawPost.call(window.__JUCE__, s);
        P('pageReady', { n: S.natives[name] });
        return r;
      }
    }
    return rawPost.call(window.__JUCE__, s);
  };

  const tick = () => { S.frame++; if (S.ticking) requestAnimationFrame(tick); };
  const startTicker = () => { if (!S.ticking) { S.ticking = true; requestAnimationFrame(tick); } };
  const raf2 = () => new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r)));
  const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

  // -- C++ -> page events --
  const relayRe = /^__juce__(slider|toggle|comboBox)(.*)$/;
  const origEmit = be.emitByBackend.bind(be);

  // armed: { id, targets: Map(eventId -> {k, v}), bad: Set(eventId), ... }
  const matches = (eventId, t) => {
    const name = eventId.replace(relayRe, '$2');
    if (t.k === 's') { const st = J && J.getSliderState(name); return st && Math.abs(st.getScaledValue() - t.v) <= 1e-4 * Math.max(1, Math.abs(t.v)); }
    if (t.k === 't') { const st = J && J.getToggleState(name); return st && st.getValue() === t.v; }
    const st = J && J.getComboBoxState(name); return st && st.getChoiceIndex() === t.v;
  };

  be.emitByBackend = function (eventId, str) {
    if (eventId === '__juce__complete' && str.indexOf(String(BENCH_ID)) >= 0) { origEmit(eventId, str); return; }
    S.evals++;
    if (S.idle) S.idle.evals[eventId] = (S.idle.evals[eventId] || 0) + 1;
    const t0 = performance.now();
    origEmit(eventId, str);                  // the page's own listeners run here
    const dt = performance.now() - t0;
    const A = S.armed;
    if (A) { A.dispatchMs += dt; A.bytes += str.length; }

    if (!relayRe.test(eventId)) return;
    if (str.indexOf('"propertiesChanged"') >= 0) {
      if (/(b24|b01)_(freq|type)$/.test(eventId)) {
        const o = JSON.parse(str);
        S.check[eventId.replace(/^__juce__(slider|comboBox)/, '')] =
          o.choices ? { n: o.choices.length, choices: o.choices } : { start: o.start, end: o.end, skew: o.skew, interval: o.interval, name: o.name };
      }
      S.props.add(eventId);
      if (!S.allPropsDone && S.props.size >= S.expected) {
        S.allPropsDone = true;
        P('allProps', { n: S.props.size, evals: S.evals, posts: S.posts, check: S.check });
        requestAnimationFrame(() => {
          P('firstPaint');
          requestAnimationFrame(() => P('secondFrame', { nWarn: S.nWarn, nErr: S.nErr, unknownWarn: S.unknownWarn,
                                                          warns: S.warns.slice(), natives: Object.assign({}, S.natives),
                                                          nativeOrder: S.nativeOrder.slice() }));
        });
      }
    } else if (A && !A.done && str.indexOf('"valueChanged"') >= 0) {
      A.nEvents++;
      if (A.tFirst === undefined) { A.tFirst = performance.now(); A.fFirst = S.frame; }
      const t = A.targets.get(eventId);
      if (t) { if (matches(eventId, t)) A.bad.delete(eventId); else A.bad.add(eventId); }
      if (A.bad.size === 0) settle(A);
    }
  };

  const settle = (A) => {
    A.done = true;
    const tl = performance.now();
    P('settled', { id: A.id, nEvents: A.nEvents, msArmToFirst: A.tFirst - A.t0, msArmToLast: tl - A.t0,
                   framesArmToFirst: A.fFirst - A.f0, framesArmToLast: S.frame - A.f0,
                   dispatchMs: A.dispatchMs, bytes: A.bytes });
    requestAnimationFrame(() => {
      P('settledPaint', { id: A.id, msArmToPaint: performance.now() - A.t0, framesArmToPaint: S.frame - A.f0 });
    });
  };

  // The page's own JUCE frontend module instance (module map: same URL, same
  // instance, same cached states as the page's static import).
  // Absolute URL: a bare relative specifier does not resolve from an injected
  // classic script. Same URL as the page's './js/juce/index.js' => same instance.
  let J = null;
  const juceUrl = () => new URL('js/juce/index.js', location.href).href;
  const juce = async () => { if (!J) J = await import(juceUrl()); return J; };
  window.addEventListener('DOMContentLoaded', () => { juce().catch((e) => keep('error', ['bench import: ' + e])); });

  const kindOf = (o) => o.k;
  const stateOf = (o) => o.k === 's' ? J.getSliderState(o.id) : o.k === 't' ? J.getToggleState(o.id) : J.getComboBoxState(o.id);
  const relayId = (o) => (o.k === 's' ? '__juce__slider' : o.k === 't' ? '__juce__toggle' : '__juce__comboBox') + o.id;

  window.__bench = {
    // targets: [{id, k:'s'|'t'|'c', v: scaled | bool | index}] for parameters that will change
    arm: (o) => {
      const A = { id: o.id, targets: new Map(), bad: new Set(), nEvents: 0, t0: performance.now(), f0: S.frame,
                  done: false, dispatchMs: 0, bytes: 0 };
      for (const t of o.targets) {
        const ev = relayId(t);
        A.targets.set(ev, t);
        if (!matches(ev, t)) A.bad.add(ev);
      }
      A.expected = A.bad.size;
      S.armed = A;
      startTicker();
      P('armed', { id: o.id, expected: A.expected, nTargets: o.targets.length });
      if (A.bad.size === 0) settle(A);
    },
    disarm: () => { S.armed = null; S.ticking = false; },

    idleStart: () => { S.ticking = false; S.idle = { t0: performance.now(), evals: {}, posts: {} }; },
    idleEnd: () => {
      const I = S.idle; S.idle = null;
      if (I) P('idle', { ms: performance.now() - I.t0, evals: I.evals, posts: I.posts, input: S.input.filter((x) => x.idle) });
    },

    raf: () => {
      const ts = [];
      const f = (t) => { ts.push(t); if (ts.length < 121) requestAnimationFrame(f); else {
        const d = []; for (let i = 1; i < ts.length; i++) d.push(ts[i] - ts[i - 1]);
        d.sort((a, b) => a - b);
        P('raf', { median: d[d.length >> 1], mean: (ts[ts.length - 1] - ts[0]) / d.length, max: d[d.length - 1] });
      } };
      requestAnimationFrame(f);
    },

    console: () => P('console', { nWarn: S.nWarn, nErr: S.nErr, unknownWarn: S.unknownWarn, warns: S.warns.slice(), input: S.input.slice(),
                                  natives: Object.assign({}, S.natives) }),

    // Native function through the page's frontend (same PromiseHandler)
    callNative: async (tag, name, args) => {
      try {
        const m = await juce();
        const r = await m.getNativeFunction(name)(...args);
        P('native', { id: tag, name: name, result: r === undefined ? null : r });
      } catch (e) { P('native', { id: tag, name: name, error: String(e) }); }
    },

    // E-bind-in: list [{id, k, norm, scaled, textNum, v}] (host values after the C++ sets)
    bindIn: async (tag, list) => {
      await juce();
      let n = 0, bad = [], maxNormErr = 0, maxScaledErr = 0, maxTextErr = 0, nSlider = 0, nScaledExact = 0;
      for (const o of list) {
        const st = stateOf(o); n++;
        if (o.k === 's') {
          nSlider++;
          const pn = st.getNormalisedValue(), ps = st.getScaledValue();
          const en = Math.abs(pn - o.norm), es = Math.abs(ps - o.scaled), et = Math.abs(ps - o.textNum);
          if (es === 0) nScaledExact++;
          maxNormErr = Math.max(maxNormErr, en); maxScaledErr = Math.max(maxScaledErr, es); maxTextErr = Math.max(maxTextErr, et);
          if (!(en <= 1e-6) || !(et <= 1e-6 * Math.max(1, Math.abs(o.textNum))))
            if (bad.length < 20) bad.push({ id: o.id, pageNorm: pn, hostNorm: o.norm, pageScaled: ps, hostText: o.textNum });
        } else if (o.k === 't') {
          if (st.getValue() !== o.v && bad.length < 20) bad.push({ id: o.id, page: st.getValue(), host: o.v });
        } else {
          const pn = st.getNormalisedValue ? st.getNormalisedValue() : null;
          if (st.getChoiceIndex() !== o.v && bad.length < 20) bad.push({ id: o.id, page: st.getChoiceIndex(), host: o.v, pageNorm: pn });
        }
      }
      P('bindIn', { id: tag, n: n, nSlider: nSlider, nScaledExact: nScaledExact, nBad: bad.length, bad: bad,
                    maxNormErr: maxNormErr, maxScaledErr: maxScaledErr, maxTextErr: maxTextErr });
    },

    // E-bind-out: list [{id, k, v}] with v normalised (slider) | bool | index
    bindOut: async (tag, list) => {
      await juce();
      let n = 0;
      for (const o of list) {
        const st = stateOf(o);
        if (o.k === 's') { st.sliderDragStarted(); st.setNormalisedValue(o.v); st.sliderDragEnded(); }
        else if (o.k === 't') st.setValue(o.v);
        else st.setChoiceIndex(o.v);
        n++;
      }
      P('bindOut', { id: tag, n: n });
    },

    clickChip: async (tag, b) => {
      const c = document.querySelector('#chips .chip[data-b="' + b + '"]');
      if (c) c.click();
      await raf2();
      P('chip', { id: tag, found: !!c, stripEmpty: document.getElementById('strip').classList.contains('empty'),
                  loz: (document.getElementById('band-loz') || {}).textContent });
    },

    // E-disabled: DOM after the page's render of the current type / slope
    probeDisabled: async (tag) => {
      await raf2();
      const $ = (id) => document.getElementById(id);
      P('disabled', { id: tag,
        stripEmpty: $('strip').classList.contains('empty'),
        loz: $('band-loz').textContent,
        typeIndex: $('combo-type').selectedIndex, slopeIndex: $('combo-slope').selectedIndex,
        gainOff: $('field-gain').classList.contains('off'), gainAria: $('field-gain').getAttribute('aria-disabled'),
        slopeDisabled: $('combo-slope').disabled, slopeWrapOff: $('wrap-slope').classList.contains('off'),
        qOff: $('field-q').classList.contains('off'), qAria: $('field-q').getAttribute('aria-disabled'),
        freqOff: $('field-freq').classList.contains('off') });
    },

    // E-type: the real typed-entry path on the selected band's frequency field
    typeEntry: async (tag, text) => {
      const el = document.getElementById('field-freq');
      const out = { id: tag, text: text, fieldOff: el.classList.contains('off') };
      el.dispatchEvent(new KeyboardEvent('keydown', { key: 'Enter', bubbles: true, cancelable: true }));
      const input = el.querySelector('input');
      out.inputOpened = !!input;
      if (input) {
        out.inputInitial = input.value;
        input.value = text;
        input.dispatchEvent(new KeyboardEvent('keydown', { key: 'Enter', bubbles: true, cancelable: true }));
        out.inputClosed = !el.querySelector('input');
      }
      await raf2();
      const m = await juce();
      out.pageScaled = m.getSliderState('b01_freq').getScaledValue();
      out.pageNorm = m.getSliderState('b01_freq').getNormalisedValue();
      out.fieldText = el.textContent;
      P('typeEntry', out);
    },

    // E-res: every URL the page references (page source: src / href / static and
    // dynamic imports; stylesheets: url()), plus the full embedded list, fetched.
    // WKWebView's resource timing omits custom-scheme loads, so references are
    // read from the sources; a request the provider cannot serve is also logged
    // by the editor ("Resource not found"), which the driver collects.
    res: async (tag, expected) => {
      const abs = (u, base) => new URL(u, base || location.href).href;
      const referenced = new Set([location.href]);
      const scan = (text, base, css) => {
        // Comments are not requests (preset-manager.js has a usage example in one)
        text = text.replace(/<!--[\s\S]*?-->/g, '').replace(/\/\*[\s\S]*?\*\//g, '').replace(/^\s*\/\/.*$/gm, '');
        const res = css ? [/url\(\s*['"]?([^'")]+)['"]?\s*\)/g]
                        : [/(?:src|href)\s*=\s*["']([^"'#]+)["']/g, /\bfrom\s+['"]([^'"]+)['"]/g, /\bimport\s*\(\s*['"]([^'"]+)['"]/g];
        const out = [];
        for (const re of res) { let m; while ((m = re.exec(text))) { try { out.push(abs(m[1], base)); } catch (e) {} } }
        return out.filter((u) => u.startsWith(location.origin));
      };
      const pageText = await (await fetch(location.href)).text();
      const queue = scan(pageText, location.href, false);
      for (let i = 0; i < queue.length; i++) {
        const u = queue[i];
        if (referenced.has(u)) continue;
        referenced.add(u);
        if (/\.(css|js)$/.test(u)) {
          try { const t = await (await fetch(u)).text(); for (const v of scan(t, u, /\.css$/.test(u))) queue.push(v); } catch (e) {}
        }
      }
      const urls = Array.from(new Set(Array.from(referenced).concat(expected.map((p) => abs(p)))));
      const rows = [];
      for (const u of urls) {
        try {
          const r = await fetch(u);
          const b = await r.arrayBuffer();
          rows.push({ url: u, status: r.status, type: r.headers.get('content-type'), bytes: b.byteLength,
                      referenced: referenced.has(u) });
        } catch (e) { rows.push({ url: u, error: String(e), referenced: referenced.has(u) }); }
      }
      P('res', { id: tag, origin: location.origin, href: location.href, referenced: Array.from(referenced), rows: rows });
    },

    // E-res fonts: document.fonts + a pixel probe of the "1k" axis label on the
    // response canvas against EB Garamond and fallback renderings of it.
    fonts: async (tag) => {
      const out = { id: tag };
      try {
        await document.fonts.ready;
        out.check16 = document.fonts.check("16px 'EB Garamond'");
        out.check105 = document.fonts.check("10.5px 'EB Garamond'");
        out.checkItalic = document.fonts.check("italic 10px 'EB Garamond'");
        out.faces = []; document.fonts.forEach((f) => out.faces.push(f.family + ' ' + f.style + ' ' + f.weight + ' ' + f.status));
        await raf2();
        const cv = document.getElementById('curve');
        const r = cv.getBoundingClientRect();
        const d = cv.width / Math.max(1, Math.round(r.width));
        const M = { l: 38, r: 14, t: 12, b: 22 };                  // PLOT_MARGIN (index.html)
        const w = Math.round(r.width), h = Math.round(r.height);
        const plotW = w - M.l - M.r, plotH = h - M.t - M.b;
        const fx = (f) => M.l + (Math.log(f) - Math.log(10)) / Math.log(30000 / 10) * plotW;
        const x0 = fx(1000), base = M.t + plotH + 15;
        out.dpr = d; out.cssW = w; out.cssH = h; out.labelX = x0; out.labelBase = base;
        // Ink extent in a box around the label (below the plot frame, between neighbours)
        const inkWidth = (ctx, bx, by, bw, bh, scale) => {
          const img = ctx.getImageData(Math.round(bx * scale), Math.round(by * scale), Math.round(bw * scale), Math.round(bh * scale));
          let minX = 1e9, maxX = -1, n = 0;
          for (let y = 0; y < img.height; y++) for (let x = 0; x < img.width; x++) {
            if (img.data[(y * img.width + x) * 4 + 3] > 96) { n++; if (x < minX) minX = x; if (x > maxX) maxX = x; }
          }
          return maxX < 0 ? { w: 0, n: 0 } : { w: (maxX - minX + 1) / scale, n: n };
        };
        const box = { x: x0 - 24, y: M.t + plotH + 3, w: 48, h: 17 };
        out.page = inkWidth(cv.getContext('2d'), box.x, box.y, box.w, box.h, d);
        // References drawn at the label's own position on a canvas of the page
        // canvas's size: rasterisation matches pixel for pixel when the face does.
        const ref = (font) => {
          const c = document.createElement('canvas'); c.width = cv.width; c.height = cv.height;
          const cx = c.getContext('2d'); cx.setTransform(d, 0, 0, d, 0, 0);
          cx.fillStyle = '#3C2F2F'; cx.font = font; cx.textAlign = 'center'; cx.textBaseline = 'alphabetic';
          cx.fillText('1k', x0, base);
          return { cx: cx, ink: inkWidth(cx, box.x, box.y, box.w, box.h, d), adv: cx.measureText('1k').width, font: cx.font };
        };
        const alphaDiff = (a, b) => {
          const g = (cx) => cx.getImageData(Math.round(box.x * d), Math.round(box.y * d), Math.round(box.w * d), Math.round(box.h * d)).data;
          const A = g(a), B = g(b); let sum = 0, n = 0;
          for (let i = 3; i < A.length; i += 4) { const e = Math.abs(A[i] - B[i]); sum += e; if (e > 32) n++; }
          return { sumAbsAlpha: sum, pixelsOver32: n };
        };
        const pageCx = cv.getContext('2d');
        const mk = (font) => { const q = ref(font); return { ink: q.ink, adv: q.adv, font: q.font, diffVsPage: alphaDiff(pageCx, q.cx) }; };
        out.refEBG = mk('10.5px "EB Garamond", Garamond, "Times New Roman", serif');
        out.refFallback = mk('10.5px Garamond, "Times New Roman", serif');
        out.refSerif = mk('10.5px serif');
        out.dEBG = Math.abs(out.page.w - out.refEBG.ink.w);
        out.dFallback = Math.abs(out.page.w - out.refFallback.ink.w);
        out.dRefs = Math.abs(out.refEBG.ink.w - out.refFallback.ink.w);
      } catch (e) { out.error = String(e); }
      P('fonts', out);
    },
  };

  document.addEventListener('DOMContentLoaded', () => P('dcl', { evals: S.evals, posts: S.posts, props: S.props.size }));
  window.addEventListener('load', () => P('load', { evals: S.evals, posts: S.posts, props: S.props.size }));
})();
)JS";
