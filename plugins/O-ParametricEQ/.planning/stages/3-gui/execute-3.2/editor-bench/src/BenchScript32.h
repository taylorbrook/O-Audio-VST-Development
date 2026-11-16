#pragma once

// PLAN-3.2 T10: the bench helpers for the interactive display, injected after
// kBenchScript (same document-start user script; window.__bench and the
// benchReport channel already exist). The page is the real index.html,
// unmodified: every interaction is a JS-dispatched PointerEvent / WheelEvent /
// MouseEvent / KeyboardEvent on the real elements.
static const char* kBenchScript32 = R"JS(
(function () {
  const BENCH_ID = -987654321;
  const be = window.__JUCE__.backend;
  const P = (type, extra) => {
    try {
      const o = Object.assign({ type: type, pnow: performance.now() }, extra || {});
      const post = window.__benchRawPost;
      post(JSON.stringify({ eventId: '__juce__invoke',
        payload: { name: 'benchReport', params: [o], resultId: BENCH_ID } }));
    } catch (e) {}
  };
  const raf = () => new Promise((r) => requestAnimationFrame(r));
  const raf2 = () => new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r)));
  const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

  // -- eqSections log: every event, in arrival order; a paint report after each
  const Q = { eq: [], paintReport: false, draws: [], drawT0: -1, timing: false, secBy: {}, actBy: {} };
  window.__benchQ = Q;
  const prevEmit = be.emitByBackend;
  be.emitByBackend = function (eventId, str) {
    prevEmit.call(be, eventId, str);           // the page's listener runs inside
    if (eventId !== 'eqSections') return;
    let d = null;
    try { d = JSON.parse(str); } catch (e) {}
    const bands = d && Array.isArray(d.bands) ? d.bands : [];
    const rec = { idx: Q.eq.length, t: performance.now(), n: bands.length,
                  bands: bands.map((b) => b.band),
                  sampleRate: d ? d.sampleRate : null, stereo: d ? d.stereo : null, rateKnown: d ? d.rateKnown : null,
                  placement: bands.map((b) => b.placement), active: bands.map((b) => !!b.active),
                  sections: bands.reduce((a, b) => a + (b.sections ? b.sections.length : 0), 0) };
    Q.eq.push(rec);
    for (const b of bands) { Q.secBy[b.band] = b.sections ? b.sections.length : 0; Q.actBy[b.band] = !!b.active; }
    if (Q.paintReport) {
      // The page's onEqSections queued its render rAF before this one: this
      // callback runs after the page's draw() in the same frame.
      requestAnimationFrame(() => P('eqPaint', { idx: rec.idx, tEq: rec.t, tPaint: performance.now() }));
    }
  };

  // -- draw() timing: the page's draw() starts with clearRect on the curve
  // canvas; its rAF callback ends after draw(). Wrap rAF to time the tail.
  const origRaf = window.requestAnimationFrame.bind(window);
  window.requestAnimationFrame = function (cb) {
    return origRaf((ts) => {
      Q.drawT0 = -1;
      cb(ts);
      if (Q.timing && Q.drawT0 >= 0) Q.draws.push(performance.now() - Q.drawT0);
      Q.drawT0 = -1;
    });
  };
  const origClear = CanvasRenderingContext2D.prototype.clearRect;
  CanvasRenderingContext2D.prototype.clearRect = function () {
    if (Q.timing && this.canvas && this.canvas.id === 'curve' && Q.drawT0 < 0) Q.drawT0 = performance.now();
    return origClear.apply(this, arguments);
  };

  const curve = () => document.getElementById('curve');
  const M = { l: 38, r: 14, t: 12, b: 22 };                     // PLOT_MARGIN (index.html)
  const geom = () => {
    const c = curve(), r = c.getBoundingClientRect();
    const w = Math.round(r.width), h = Math.round(r.height);
    const on = document.querySelector('#seg-range button[aria-checked="true"]')
            || document.querySelector('#seg-range button.on')
            || document.querySelector('#seg-range button[aria-pressed="true"]');
    return { left: r.left, top: r.top, w: w, h: h, plotW: w - M.l - M.r, plotH: h - M.t - M.b,
             range: on ? Number(on.dataset.v) : null, dpr: window.devicePixelRatio || 1,
             vw: window.innerWidth, vh: window.innerHeight };
  };

  const ptr = (type, x, y, o) => {
    const g = geom();
    const ev = new PointerEvent(type, Object.assign({ bubbles: true, cancelable: true, composed: true, button: 0, buttons: type === 'pointerup' ? 0 : 1,
      pointerId: 7, pointerType: 'mouse', isPrimary: true, clientX: g.left + x, clientY: g.top + y }, o || {}));
    curve().dispatchEvent(ev);
  };

  // Ink rows of the summed curve (#3C2F2F, full alpha) in one CSS-px column.
  const inkRuns = (xCss) => {
    const c = curve(), g = geom(), d = c.width / Math.max(1, g.w);
    const X = Math.round(xCss * d);
    const img = c.getContext('2d').getImageData(X, 0, 1, c.height).data;
    const runs = []; let start = -1;
    for (let y = 0; y <= c.height; y++) {
      const i = y * 4;
      const ink = y < c.height && img[i + 3] >= 250
        && Math.abs(img[i] - 60) <= 14 && Math.abs(img[i + 1] - 47) <= 14 && Math.abs(img[i + 2] - 47) <= 14;
      if (ink && start < 0) start = y;
      if (!ink && start >= 0) { runs.push([start / d, (y - 1) / d]); start = -1; }
    }
    return runs;
  };

  Object.assign(window.__bench, {
    eqMark: (tag) => P('eqMark', { id: tag, n: Q.eq.length }),
    eqSince: (tag, from) => P('eqSince', { id: tag, list: Q.eq.slice(from) }),
    paintReports: (on) => { Q.paintReport = !!on; },
    geom: async (tag) => { await raf2(); P('geom', Object.assign({ id: tag }, geom())); },

    // Ink runs at each CSS x (plot-local x = canvas x), after two frames.
    curveProbe: async (tag, xs) => {
      await raf2();
      P('probe', { id: tag, geom: geom(), cols: xs.map((x) => ({ x: x, runs: inkRuns(x) })) });
    },

    click: async (tag, x, y) => { ptr('pointerdown', x, y); ptr('pointerup', x, y); await raf2(); P('done', { id: tag }); },
    // Press at (x0, y0), `steps` moves to (x1, y1) every `dt` ms, then end: 'up' | 'cancel' | 'lost'.
    // midEval: a JS string evaluated once, after the 3rd move (automation mid-drag).
    drag: async (tag, x0, y0, x1, y1, steps, dt, end, shift) => {
      ptr('pointerdown', x0, y0);
      for (let i = 1; i <= steps; i++) {
        await sleep(dt);
        const x = x0 + (x1 - x0) * i / steps, y = y0 + (y1 - y0) * i / steps;
        ptr('pointermove', x, y);
        if (i === 3) P('dragMid', { id: tag });
      }
      await sleep(dt);
      if (end === 'cancel') ptr('pointercancel', x1, y1, { buttons: 0 });
      else if (end === 'lost') ptr('lostpointercapture', x1, y1, { buttons: 0 });
      else ptr('pointerup', x1, y1);
      await raf2();
      P('done', { id: tag });
    },
    // Same, but pauses after the 3rd move until resume() (the driver changes a host value).
    dragPause: async (tag, x0, y0, x1, y1, steps, dt) => {
      Q.resume = null;
      ptr('pointerdown', x0, y0);
      for (let i = 1; i <= steps; i++) {
        await sleep(dt);
        ptr('pointermove', x0 + (x1 - x0) * i / steps, y0 + (y1 - y0) * i / steps);
        if (i === 3) { const p = new Promise((r) => { Q.resume = r; }); P('paused', { id: tag }); await p; }
      }
      await sleep(dt);
      ptr('pointerup', x1, y1);
      await raf2();
      P('done', { id: tag });
    },
    resume: () => { if (Q.resume) Q.resume(); },
    wheel: async (tag, x, y, n, dy, shift, dt) => {
      const g = geom();
      for (let i = 0; i < n; i++) {
        curve().dispatchEvent(new WheelEvent('wheel', { bubbles: true, cancelable: true, deltaY: dy, deltaMode: 0,
          shiftKey: !!shift, clientX: g.left + x, clientY: g.top + y }));
        await sleep(dt);
      }
      P('done', { id: tag });
    },
    dblPlot: async (tag, x, y) => {
      const g = geom();
      ptr('pointerdown', x, y); ptr('pointerup', x, y);
      ptr('pointerdown', x, y); ptr('pointerup', x, y);
      curve().dispatchEvent(new MouseEvent('dblclick', { bubbles: true, cancelable: true, detail: 2, clientX: g.left + x, clientY: g.top + y }));
      await raf2();
      P('done', { id: tag });
    },
    dblEl: async (tag, sel) => {
      const el = document.querySelector(sel);
      if (el) el.dispatchEvent(new MouseEvent('dblclick', { bubbles: true, cancelable: true, detail: 2 }));
      await sleep(30); await raf2();
      P('done', { id: tag, found: !!el });
    },
    clickEl: async (tag, sel) => {
      const el = document.querySelector(sel);
      if (el) el.click();
      await raf2();
      P('done', { id: tag, found: !!el });
    },
    key: async (tag, key) => {
      document.body.dispatchEvent(new KeyboardEvent('keydown', { key: key, bubbles: true, cancelable: true }));
      await raf2();
      P('done', { id: tag });
    },
    // Typed entry on a strip field: Enter opens it, Enter commits.
    typeField: async (tag, fieldId, text) => {
      const el = document.getElementById(fieldId);
      const out = { id: tag, text: text, fieldOff: el ? el.classList.contains('off') : null };
      if (el) {
        el.dispatchEvent(new KeyboardEvent('keydown', { key: 'Enter', bubbles: true, cancelable: true }));
        const input = el.querySelector('input');
        out.inputOpened = !!input;
        if (input) {
          input.value = text;
          input.dispatchEvent(new KeyboardEvent('keydown', { key: 'Enter', bubbles: true, cancelable: true }));
          out.inputClosed = !el.querySelector('input');
        }
      }
      await raf2();
      out.fieldText = el ? el.textContent : null;
      P('typed', out);
    },
    tip: async (tag) => {
      await sleep(140);                                       // past the 110 ms opacity transition
      const t = document.getElementById('tooltip'), r = t.getBoundingClientRect(), cs = getComputedStyle(t);
      P('tip', { id: tag, visible: t.classList.contains('visible') && cs.opacity !== '0', opacity: cs.opacity,
                 text: t.textContent, i18n: t.dataset.i18n || null, rect: { l: r.left, t: r.top, r: r.right, b: r.bottom, w: r.width, h: r.height },
                 vw: window.innerWidth, vh: window.innerHeight, lang: document.documentElement.lang,
                 tipsOn: (document.getElementById('tips-toggle') || {}).getAttribute
                       ? document.getElementById('tips-toggle').getAttribute('aria-pressed') : null });
    },
    pressElsewhere: async (tag) => {
      const s = document.getElementById('strip') || document.body;
      s.dispatchEvent(new PointerEvent('pointerdown', { bubbles: true, cancelable: true, button: 0, pointerId: 8 }));
      s.dispatchEvent(new PointerEvent('pointerup', { bubbles: true, cancelable: true, button: 0, pointerId: 8 }));
      await sleep(20);
      P('done', { id: tag });
    },
    lang: async (tag, l) => { window.__setLanguage(l); await raf2(); P('done', { id: tag }); },
    labels: async (tag) => {
      try {
        const m = await import(new URL('js/i18n.js', location.href).href);
        P('labels', { id: tag, entry: m.LABELS['bands-full'] || null });
      } catch (e) { P('labels', { id: tag, error: String(e) }); }
    },
    // draw() time: `n` redraws, one per frame, driven by the page's own resize listener.
    drawTiming: async (tag, n) => {
      Q.draws = []; Q.timing = true;
      for (let i = 0; i < n; i++) { window.dispatchEvent(new Event('resize')); await raf(); await raf(); }
      Q.timing = false;
      const d = Q.draws.slice().sort((a, b) => a - b);
      P('drawTiming', { id: tag, n: d.length, median: d[d.length >> 1], p99: d[Math.min(d.length - 1, Math.floor(d.length * 0.99))],
                        max: d[d.length - 1], geom: geom(),
                        sections: Object.keys(Q.secBy).reduce((a, b) => a + (Q.actBy[b] ? Q.secBy[b] : 0), 0) });
    },
  });
})();
)JS";
