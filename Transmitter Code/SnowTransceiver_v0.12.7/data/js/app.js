// ============================================================================
//  Snow Transceiver — dashboard logic
// ============================================================================
(() => {
  "use strict";

  // ---------- helpers ----------------------------------------------------
  const $  = (sel, root = document) => root.querySelector(sel);
  const $$ = (sel, root = document) => Array.from(root.querySelectorAll(sel));

  const toast = (msg, kind = "ok") => {
    const el = document.createElement("div");
    el.className = `toast toast--${kind}`;
    el.innerHTML =
      `<svg><use href="#${kind === "ok" ? "i-check" : "i-warn"}"/></svg>` +
      `<span>${msg}</span>`;
    $("#toaster").appendChild(el);
    setTimeout(() => {
      el.classList.add("is-out");
      el.addEventListener("animationend", () => el.remove(), { once: true });
    }, 2400);
  };

  const api = async (path, opts = {}) => {
    const ctrl = new AbortController();
    const t = setTimeout(() => ctrl.abort(), opts.timeout || 6000);
    try {
      const res = await fetch(path, {
        ...opts,
        signal: ctrl.signal,
        headers: opts.body
          ? { "Content-Type": "application/json", ...(opts.headers || {}) }
          : opts.headers || {},
      });
      const isJson = (res.headers.get("content-type") || "").includes("json");
      const body   = isJson ? await res.json() : await res.text();
      if (!res.ok) throw Object.assign(new Error("http"), { status: res.status, body });
      return body;
    } finally {
      clearTimeout(t);
    }
  };

  // ---------- formatting -------------------------------------------------
  const isImperial = () => document.documentElement.dataset.units === "imperial";
  const fmtCm = (cm) => {
    if (cm === null || cm === undefined || Number.isNaN(cm)) return "––.–";
    if (cm < 0) return "––.–";
    return isImperial() ? (cm / 2.54).toFixed(1) : cm.toFixed(1);
  };
  const fmtBytes = (b) => {
    if (!b || b < 0) return "–";
    const u = ["B","KB","MB","GB","TB"];
    let i = 0; let n = b;
    while (n >= 1024 && i < u.length - 1) { n /= 1024; i++; }
    return `${n.toFixed(n >= 100 ? 0 : 1)} ${u[i]}`;
  };
  const fmtUptime = (s) => {
    s = Math.floor(s);
    const d = Math.floor(s / 86400); s -= d * 86400;
    const h = Math.floor(s / 3600);  s -= h * 3600;
    const m = Math.floor(s / 60);    s -= m * 60;
    if (d) return `${d}d ${h}h ${m}m`;
    if (h) return `${h}h ${m}m ${s}s`;
    if (m) return `${m}m ${s}s`;
    return `${s}s`;
  };
  const fmtSecondsHuman = (s) => {
    if (s < 60)    return `${s}s`;
    if (s < 3600)  return `${(s/60).toFixed(s%60===0?0:1)}m`;
    if (s < 86400) return `${(s/3600).toFixed(s%3600===0?0:1)}h`;
    return `${(s/86400).toFixed(s%86400===0?0:1)}d`;
  };

  // ---------- router -----------------------------------------------------
  const PAGE_TITLES = {
    dashboard: "Overview",
    snow:      "Snow Measurements",
    gps:       "GPS & Location",
    imu:       "Motion & Fall Detection",
    sd:        "Storage",
    battery:   "Battery",
    lora:      "LoRa Transmitter",
    env:       "Temperature & Humidity",
    settings:  "Settings",
  };
  const validRoutes = Object.keys(PAGE_TITLES);

  const navigate = (route) => {
    if (!validRoutes.includes(route)) route = "dashboard";

    $$(".page").forEach(p => p.classList.toggle(
      "page--active", p.dataset.page === route));

    $$(".rail__item").forEach(i => i.classList.toggle(
      "is-active", i.dataset.route === route));

    $("#page-title").textContent = PAGE_TITLES[route];

    if (route === "sd")        refreshSdPage();
    if (route === "settings")  refreshSettingsPage();
    if (route === "gps")       mapResize();
    if (route === "imu")       { arcResize(); refreshImuEvents(); }
    if (route === "battery")   ringResize();
    if (route === "lora")      refreshLoraPage();
    if (route === "env")       envResize();
    if (route === "dashboard") chartResize();
  };

  const initRouter = () => {
    const route = () => location.hash.replace("#","") || "dashboard";
    window.addEventListener("hashchange", () => navigate(route()));
    navigate(route());
  };

  // ---------- live status (polled every second) ------------------------
  let lastDistText = null;
  let lastDepthText = null;
  let mapLastLat = null, mapLastLon = null;

  const updateUnitLabels = () => {
    const u = isImperial() ? "in" : "cm";
    $$("[data-metric][data-imperial]").forEach(el => { el.textContent = u; });
  };

  const setReadout = (id, cmValue, prevText) => {
    const el = document.getElementById(id);
    if (!el) return prevText;
    const txt = fmtCm(cmValue);
    if (txt !== prevText) {
      el.textContent = txt;
      el.classList.toggle("is-stale", txt === "––.–");
      if (txt !== "––.–") {
        el.classList.remove("is-flash");
        // eslint-disable-next-line no-unused-expressions
        el.offsetWidth;
        el.classList.add("is-flash");
      }
    }
    return txt;
  };

  // ---------- smooth-tween readout --------------------------------------
  // Used by the 5 Hz fast loop on the dashboard / Snow page to avoid the
  // "violent" rapid-fire number repaint that came from setReadout firing
  // an .is-flash animation every 200 ms. Instead, we animate the displayed
  // value smoothly from the previously-rendered number to the new target
  // using requestAnimationFrame. New samples mid-tween cancel the in-flight
  // tween and start a fresh one from the current animated value, so the
  // motion always lines up with what the user sees -- no jumps, no flicker.
  //
  // The tween uses an ease-out cubic over ~260 ms which is just past one
  // refresh interval -- long enough to smooth jitter, short enough to feel
  // responsive.
  const tweenState = {};
  const TWEEN_MS = 260;
  const tweenReadout = (id, target) => {
    const el = document.getElementById(id);
    if (!el) return;
    // Sentinel / stale -- bypass tween, just print the dashes.
    if (target == null || target < 0 || Number.isNaN(target)) {
      const txt = fmtCm(target);
      if (el.textContent !== txt) el.textContent = txt;
      el.classList.toggle("is-stale", txt === "––.–");
      const st = tweenState[id];
      if (st && st.raf) cancelAnimationFrame(st.raf);
      tweenState[id] = { current: NaN, raf: null };
      return;
    }
    el.classList.remove("is-stale");

    const st = tweenState[id];
    const startVal = (st && !Number.isNaN(st.current)) ? st.current : target;
    if (st && st.raf) cancelAnimationFrame(st.raf);

    // No real change? Don't bother animating; just refresh the text.
    if (Math.abs(target - startVal) < 0.05) {
      el.textContent = fmtCm(target);
      tweenState[id] = { current: target, raf: null };
      return;
    }

    const start = performance.now();
    const step = (now) => {
      const t = Math.min(1, (now - start) / TWEEN_MS);
      const eased = 1 - Math.pow(1 - t, 3);    // ease-out cubic
      const v = startVal + (target - startVal) * eased;
      tweenState[id].current = v;
      el.textContent = fmtCm(v);
      if (t < 1) {
        tweenState[id].raf = requestAnimationFrame(step);
      } else {
        el.textContent = fmtCm(target);
        tweenState[id].current = target;
        tweenState[id].raf = null;
      }
    };
    tweenState[id] = { current: startVal, raf: requestAnimationFrame(step) };
  };

  const renderStatus = (s) => {
    // Cache the latest settings snapshot for cross-page consumers
    // (e.g. LoRa duty-cycle warning needs to know capture interval).
    state.lastSettings = s.settings || {};

    // Connectivity pill
    const fresh = s.sensor.fresh;
    const sdOk  = s.sd.ready;
    const pill  = $("#conn-pill");
    pill.classList.remove("is-ok", "is-warn", "is-err");
    if (fresh && sdOk)        { pill.classList.add("is-ok");   $("#conn-text").textContent = "Online · Logging"; }
    else if (fresh && !sdOk)  { pill.classList.add("is-warn"); $("#conn-text").textContent = "Sensor OK · No SD"; }
    else if (!fresh && sdOk)  { pill.classList.add("is-warn"); $("#conn-text").textContent = "Awaiting sensor"; }
    else                      { pill.classList.add("is-err");  $("#conn-text").textContent = "Sensor + SD offline"; }

    // Topbar clock
    if (s.time && s.time.iso) {
      const iso = s.time.iso;
      const m = /T(\d{2}:\d{2}:\d{2})/.exec(iso);
      $("#topbar-time").textContent = m ? m[1] : iso.replace(/^BOOT\+/, "+");
    }

    // ---------- Hero readouts ----------
    lastDistText  = setReadout("hero-distance", s.sensor.distance_cm, lastDistText);
    lastDepthText = setReadout("hero-depth",    s.snow.depth_cm,      lastDepthText);
    $("#hero-fresh").textContent = s.sensor.fresh ? "Live" : "Stale";
    $("#hero-tare-info").textContent =
      "Tare " + fmtCm(s.snow.tare_cm) + " " + (isImperial() ? "in" : "cm");

    // ---------- System card ----------
    $("#sys-sensor").textContent  = s.sensor.fresh ? "OK" : "No data";
    $("#sys-gps").textContent     = s.gps.fix
                                       ? `${s.gps.sats} sats · HDOP ${(s.gps.hdop ?? 0).toFixed(1)}`
                                       : "No fix";
    const sysImu = $("#sys-imu");
    if (sysImu) sysImu.textContent = (s.imu && s.imu.ready)
                                       ? `${(s.imu.mag ?? 0).toFixed(2)} g`
                                       : "Not detected";
    $("#sys-sd").textContent      = s.sd.ready
                                       ? (s.sd.nearly_full ? "Nearly full" : "Ready")
                                       : "Not detected";
    $("#sys-clients").textContent = s.ap_clients;
    $("#sys-heap").textContent    = fmtBytes(s.free_heap);
    $("#sys-uptime").textContent  = fmtUptime(s.uptime_s);

    // ---------- Logging card ----------
    $("#log-active").textContent   = s.sd.active_file || "–";
    $("#log-rows").textContent     = s.sd.rows.toLocaleString();
    $("#log-interval").textContent = fmtSecondsHuman(s.settings.interval_s);
    $("#log-outliers").textContent = (s.snow.outliers ?? 0).toLocaleString();
    $("#log-frames").textContent   = s.sensor.frames.toLocaleString();
    $("#log-cks").textContent      = s.sensor.checksum_err.toLocaleString();

    // ---------- Snow page ----------
    const snowDistEl  = document.getElementById("snow-distance");
    if (snowDistEl)  snowDistEl.textContent = fmtCm(s.sensor.distance_cm);
    const snowDepthEl = document.getElementById("snow-depth");
    if (snowDepthEl) snowDepthEl.textContent = fmtCm(s.snow.depth_cm);
    const snowTareEl  = document.getElementById("snow-tare");
    if (snowTareEl)  snowTareEl.textContent  = fmtCm(s.snow.tare_cm);

    // ---------- GPS page ----------
    renderGps(s.gps);
    if (s.imu) renderImu(s.imu);
    if (s.battery) renderBattery(s.battery);
    if (s.env)     renderEnv(s.env);
    if (s.lora)    renderLora(s.lora);

    // Feed the live sensor graph (only for fresh readings, otherwise we
    // pollute the chart with stale data that flat-lines).
    if (s.sensor.fresh) {
      pushChartSample(s.sensor.distance_cm, s.snow.depth_cm);
    } else {
      pushChartSample(null, null);    // gap in the line, signals stale
    }

    // ---------- SD live row count ----------
    const sdRowsEl = document.getElementById("sd-rows");
    if (sdRowsEl) sdRowsEl.textContent = s.sd.rows.toLocaleString();
    const sdActiveEl = document.getElementById("sd-active");
    if (sdActiveEl) sdActiveEl.textContent = s.sd.active_file || "–";

    // ---------- About / time source ----------
    const fwEl = document.getElementById("about-fw");
    if (fwEl) fwEl.textContent = `${s.fw_name} ${s.fw_version}`;
    const ts = document.getElementById("time-source");
    if (ts) ts.textContent = `source: ${s.time.source}`;
  };

  // ---------- GPS rendering --------------------------------------------
  const renderGps = (g) => {
    const fmtCoord = (v, latLon) => {
      if (v === null || v === undefined) return "––.––––––°";
      const sfx = latLon === "lat" ? (v >= 0 ? "N" : "S")
                                   : (v >= 0 ? "E" : "W");
      return `${Math.abs(v).toFixed(6)}° ${sfx}`;
    };

    $("#gps-fix-badge").textContent = g.fix ? (g.fresh ? "Fix" : "Stale fix") : "No fix";
    $("#gps-fix-badge").classList.toggle("card__badge--accent", g.fix && g.fresh);

    $("#gps-lat").textContent   = fmtCoord(g.lat, "lat");
    $("#gps-lon").textContent   = fmtCoord(g.lon, "lon");
    $("#gps-alt").textContent   = g.alt_m != null ? `${g.alt_m.toFixed(1)} m` : "–.– m";
    $("#gps-speed").textContent = g.speed_kmh != null ? `${g.speed_kmh.toFixed(1)} km/h` : "–.– km/h";
    $("#gps-course").textContent = g.course != null ? `${g.course.toFixed(0)}°` : "–°";
    $("#gps-sats").textContent   = g.sats ?? 0;
    $("#gps-hdop").textContent   = g.hdop != null ? g.hdop.toFixed(1) : "–";
    $("#gps-fix-sent").textContent = (g.fix_sentences ?? 0).toLocaleString();
    $("#gps-chars").textContent    = (g.chars ?? 0).toLocaleString();
    $("#gps-fc").textContent       = (g.failed_checksum ?? 0).toLocaleString();

    // HDOP-derived signal quality badge
    let qual = "––";
    if (g.hdop != null) {
      if      (g.hdop < 1)  qual = "Excellent";
      else if (g.hdop < 2)  qual = "Good";
      else if (g.hdop < 5)  qual = "Moderate";
      else if (g.hdop < 10) qual = "Fair";
      else                  qual = "Poor";
    }
    $("#gps-hdop-badge").textContent = `HDOP ${g.hdop != null ? g.hdop.toFixed(1) : "–"} · ${qual}`;

    // Satellite "bars": fixed 12 slots, fill proportionally to sats + HDOP
    renderSatBars(g.sats ?? 0, g.hdop);

    // Map
    if (g.fix && g.lat != null && g.lon != null) {
      mapLastLat = g.lat;
      mapLastLon = g.lon;
      mapDraw(g.lat, g.lon);
      const ovr = $("#map-overlay");
      if (ovr) ovr.classList.add("is-hidden");
    } else {
      mapDraw(null, null);
    }
  };

  const renderSatBars = (sats, hdop) => {
    const wrap = $("#gps-sat-bars");
    if (!wrap) return;
    if (wrap.children.length === 0) {
      for (let i = 0; i < 12; i++) {
        const b = document.createElement("div");
        b.className = "bars__b is-empty";
        wrap.appendChild(b);
      }
    }
    const bars = wrap.children;
    // Highlight `sats` of them, with heights modulated by HDOP quality.
    const max = 12;
    const lit = Math.min(max, sats);
    const baseQ = hdop ? Math.max(0.2, Math.min(1, 2.5 / hdop)) : 0.5;
    for (let i = 0; i < max; i++) {
      const el = bars[i];
      if (i < lit) {
        el.classList.remove("is-empty");
        // Vary heights for visual richness, modulated by quality
        const wave = 0.55 + 0.45 * Math.sin((i * 1.3) + 0.5);
        const h = Math.round(20 + 28 * wave * baseQ);
        el.style.height = `${h}px`;
      } else {
        el.classList.add("is-empty");
        el.style.height = "8px";
      }
    }
  };

  // ---------- Dashboard sensor graph (live line chart) -----------------
  // Captures the most recent N readings of distance + depth and renders
  // a smoothed line chart with gridlines + axis labels.  Updates every
  // /api/status poll (= 1 Hz).
  const CHART_HISTORY_SIZE = 60;        // last 60 samples (~60s at 1Hz)
  const chartState = {
    canvas: null,
    ctx: null,
    distHist:  [],   // each entry: { t: ms, v: cm | null }
    depthHist: [],
  };

  const chartResize = () => {
    const c = chartState.canvas;
    if (!c) return;
    const dpr = window.devicePixelRatio || 1;
    const r = c.getBoundingClientRect();
    c.width  = Math.round(r.width * dpr);
    c.height = Math.round(r.height * dpr);
    if (chartState.ctx) chartState.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    drawChart();
  };

  const pushChartSample = (distCm, depthCm) => {
    const now = Date.now();
    chartState.distHist.push ({ t: now, v: (distCm  == null ? null : distCm)  });
    chartState.depthHist.push({ t: now, v: (depthCm == null ? null : depthCm) });
    while (chartState.distHist.length  > CHART_HISTORY_SIZE) chartState.distHist.shift();
    while (chartState.depthHist.length > CHART_HISTORY_SIZE) chartState.depthHist.shift();
    drawChart();
  };

  // Draw a series with a stroked line + soft area fill underneath.
  const drawSeries = (ctx, samples, plotArea, yMin, yMax, color, fillAlpha) => {
    const { x: x0, y: y0, w, h } = plotArea;
    if (samples.length < 2) return;

    const n = samples.length;
    const xAt = (i) => x0 + (i / (CHART_HISTORY_SIZE - 1)) * w;
    const yAt = (v) => {
      if (v == null || isNaN(v)) return null;
      const t = (v - yMin) / (yMax - yMin);
      return y0 + h - t * h;
    };

    // Build smoothed path using cardinal-spline-style midpoint averaging
    const pts = [];
    for (let i = 0; i < n; i++) {
      const y = yAt(samples[i].v);
      pts.push(y == null ? null : { x: xAt(i), y });
    }

    // Filled area under line (with soft alpha)
    ctx.save();
    ctx.beginPath();
    let started = false;
    for (let i = 0; i < pts.length; i++) {
      const p = pts[i];
      if (p == null) { started = false; continue; }
      if (!started) {
        ctx.moveTo(p.x, y0 + h);
        ctx.lineTo(p.x, p.y);
        started = true;
      } else {
        const prev = pts[i - 1];
        const midX = (prev.x + p.x) / 2;
        ctx.bezierCurveTo(midX, prev.y, midX, p.y, p.x, p.y);
      }
    }
    if (started) {
      const last = pts.filter(p => p != null).pop();
      ctx.lineTo(last.x, y0 + h);
      ctx.closePath();
      // Build an rgba fill from the line colour + alpha
      const m = color.match(/rgb\((\d+),(\d+),(\d+)\)/);
      const r = m ? `rgba(${m[1]},${m[2]},${m[3]},${fillAlpha})` : color;
      ctx.fillStyle = r;
      ctx.fill();
    }
    ctx.restore();

    // Stroked line on top
    ctx.save();
    ctx.beginPath();
    started = false;
    for (let i = 0; i < pts.length; i++) {
      const p = pts[i];
      if (p == null) { started = false; continue; }
      if (!started) { ctx.moveTo(p.x, p.y); started = true; }
      else {
        const prev = pts[i - 1];
        const midX = (prev.x + p.x) / 2;
        ctx.bezierCurveTo(midX, prev.y, midX, p.y, p.x, p.y);
      }
    }
    ctx.lineWidth = 2;
    ctx.strokeStyle = color;
    ctx.lineCap = "round";
    ctx.lineJoin = "round";
    ctx.shadowColor = color;
    ctx.shadowBlur = 6;
    ctx.stroke();
    ctx.restore();
  };

  const drawChart = () => {
    const c = chartState.canvas;
    if (!c || !chartState.ctx) return;
    const ctx = chartState.ctx;
    const w = c.clientWidth, h = c.clientHeight;
    ctx.clearRect(0, 0, w, h);

    // Plot area (leave room for axis labels). padT is large enough to
    // host the "cm" / "in" unit hint above the topmost tick number
    // without the two glyphs overlapping -- previously padT=10 left no
    // vertical gap and the unit label was visually stuck onto the top
    // tick value, especially on the smaller mobile chart (180 px tall).
    const padL = 40, padR = 14, padT = 22, padB = 22;
    const plot = { x: padL, y: padT, w: w - padL - padR, h: h - padT - padB };

    // Determine y-axis range from current data (auto-scale).
    const all = [...chartState.distHist, ...chartState.depthHist]
      .map(s => s.v).filter(v => v != null && !isNaN(v));
    let yMin = 0, yMax = 10;
    if (all.length > 0) {
      yMin = Math.min(...all);
      yMax = Math.max(...all);
      const pad = Math.max(2, (yMax - yMin) * 0.15);
      yMin = Math.max(0, yMin - pad);
      yMax = yMax + pad;
      if (yMax - yMin < 5) yMax = yMin + 5;   // never collapse to a flat line
    }

    // Gridlines & labels
    ctx.font = '10px ui-monospace, "SF Mono", Menlo, monospace';
    ctx.fillStyle = "rgba(255,255,255,0.45)";
    ctx.strokeStyle = "rgba(255,255,255,0.06)";
    ctx.lineWidth = 1;
    const TICKS = 5;
    ctx.textAlign = "right";
    ctx.textBaseline = "middle";
    for (let i = 0; i <= TICKS; i++) {
      const y = plot.y + (i / TICKS) * plot.h;
      const v = yMax - (i / TICKS) * (yMax - yMin);
      ctx.beginPath();
      ctx.moveTo(plot.x, y);
      ctx.lineTo(plot.x + plot.w, y);
      ctx.stroke();
      ctx.fillText(v.toFixed(0), plot.x - 6, y);
    }

    // x-axis label hint
    ctx.fillStyle = "rgba(255,255,255,0.30)";
    ctx.textAlign = "left";
    ctx.textBaseline = "alphabetic";
    ctx.fillText("oldest", plot.x, plot.y + plot.h + 14);
    ctx.textAlign = "right";
    ctx.fillText("now", plot.x + plot.w, plot.y + plot.h + 14);

    // Series (distance = cyan/accent, depth = green)
    const conv = isImperial() ? (v => v / 2.54) : (v => v);
    const distSeries  = chartState.distHist.map (s => ({ t: s.t, v: s.v == null ? null : conv(s.v) }));
    const depthSeries = chartState.depthHist.map(s => ({ t: s.t, v: s.v == null ? null : conv(s.v) }));

    drawSeries(ctx, distSeries,  plot, yMin, yMax, "rgb(92,200,255)", 0.10);
    drawSeries(ctx, depthSeries, plot, yMin, yMax, "rgb(61,220,132)", 0.12);

    // Y-axis unit hint -- pinned to the top-left of the canvas. Using
    // textBaseline:"top" + a fixed y near 0 keeps the label clear of the
    // topmost tick number which lives at y=plot.y with middle baseline
    // (and would otherwise share the same row).
    ctx.fillStyle = "rgba(255,255,255,0.45)";
    ctx.textAlign = "left";
    ctx.textBaseline = "top";
    ctx.fillText(isImperial() ? "in" : "cm", 4, 2);

    // Update the "last N samples" label
    const lbl = $("#chart-window");
    if (lbl) lbl.textContent = `last ${chartState.distHist.length} samples`;
  };

  const initChart = () => {
    chartState.canvas = $("#chart-canvas");
    if (chartState.canvas) {
      chartState.ctx = chartState.canvas.getContext("2d");
      chartResize();
      window.addEventListener("resize", () => requestAnimationFrame(chartResize));
    }
  };

  // ---------- LoRa transmitter page -----------------------------------
  const POWER_DBM = ["22 dBm", "17 dBm", "13 dBm", "10 dBm"];

  // Each region's channel range on the E220 (1 MHz steps from 850.125 MHz).
  // EU 868 ISM band 863-870 MHz -> channels 13..19   (default 18 = 868.125)
  // US 915 ISM band 902-928 MHz -> channels 52..77   (default 65 = 915.125)
  const REGIONS = {
    0: { id: 0, key: "eu", label: "EU 868 MHz", min: 13, max: 19, default: 18 },
    1: { id: 1, key: "us", label: "US 915 MHz", min: 52, max: 77, default: 65 },
  };
  const REGION_CHANNEL_DEFAULT = { 0: REGIONS[0].default, 1: REGIONS[1].default };

  // Per-user "which bands are visible in the LoRa tab" -- stored client-side
  // in localStorage so it survives refreshes without firmware involvement.
  // Defaults: both EU and US enabled, "all" mode off.
  //   - eu / us: independently hide each region's filter button
  //   - all:     bypasses filtering entirely. Channels 0..LORA_CHANNEL_MAX
  //              are shown as one big grid and the region toggle is hidden.
  const BAND_PREF_KEY = "lora_bands_enabled";
  const LORA_CH_MAX   = 80;      // mirrors LORA_CHANNEL_MAX in Config.h
  const loadBandPrefs = () => {
    try {
      const raw = localStorage.getItem(BAND_PREF_KEY);
      const p = raw ? JSON.parse(raw) : {};
      const all = p.all === true;
      let eu = p.eu !== false;
      let us = p.us !== false;
      // When "all" is off, at least one of EU/US must stay enabled.
      if (!all && !eu && !us) eu = true;
      return { eu, us, all };
    } catch { return { eu: true, us: true, all: false }; }
  };
  const saveBandPrefs = (p) => {
    localStorage.setItem(BAND_PREF_KEY, JSON.stringify(p));
  };
  const isRegionEnabled = (regionId) => {
    const p = loadBandPrefs();
    return regionId === 0 ? p.eu : p.us;
  };
  const isChannelInRegion = (ch, regionId) => {
    const r = REGIONS[regionId];
    return r && ch >= r.min && ch <= r.max;
  };
  const regionForChannel = (ch) => {
    if (isChannelInRegion(ch, 0)) return 0;
    if (isChannelInRegion(ch, 1)) return 1;
    return null;
  };

  const channelToMhz = (ch) => 850.125 + (ch | 0) * 1.0;

  const renderLora = (lo) => {
    if (!lo) return;
    const stateLabel = {
      disabled:     "Disabled",
      init_failed:  "INIT FAILED",
      idle:         "Idle",
      configuring:  "Configuring…",
      transmitting: "Transmitting…",
    }[lo.state] || lo.state;

    const stateBadge = $("#lora-state");
    if (stateBadge) {
      stateBadge.textContent = stateLabel;
      stateBadge.classList.toggle("card__badge--accent", lo.state === "idle" || lo.state === "transmitting");
      stateBadge.classList.toggle("card__badge--err",    lo.state === "init_failed");
    }

    const fmtFreq = (mhz) => mhz.toFixed(3);
    if ($("#lora-freq"))      $("#lora-freq").textContent = fmtFreq(lo.freq_mhz ?? channelToMhz(lo.channel ?? 0));
    if ($("#lora-ch-label"))  $("#lora-ch-label").textContent = `channel ${lo.channel ?? "–"}`;
    if ($("#lora-pwr-label")) $("#lora-pwr-label").textContent = POWER_DBM[lo.tx_power_idx ?? 0] || "–";
    if ($("#lora-enc-label")) $("#lora-enc-label").textContent = lo.encrypt ? "encrypted" : "unencrypted";

    if ($("#lora-tx-count")) $("#lora-tx-count").textContent = (lo.tx_count ?? 0).toLocaleString();
    if ($("#lora-tx-fail"))  $("#lora-tx-fail").textContent  = (lo.tx_failures ?? 0).toLocaleString();
    if ($("#lora-tx-bytes")) $("#lora-tx-bytes").textContent = fmtBytes(lo.tx_bytes ?? 0);
    if ($("#lora-last-tx")) {
      $("#lora-last-tx").textContent = (lo.last_tx_age_ms == null || lo.last_tx_age_ms < 0)
                                       ? "—" : fmtAgeShort(lo.last_tx_age_ms);
    }
    if ($("#lora-seq")) $("#lora-seq").textContent = (lo.tx_count ?? 0).toLocaleString();

    // Duty-cycle warning -- EU 868 MHz ETSI mandates max 1% duty cycle.
    // Rough estimate: at 2.4kbps air rate, ~80-byte CSV row takes ~270ms.
    // 1% of an hour = 36s.  Number of TX/h = 36s / 0.27s ≈ 133 packets/h
    // ≈ 27s minimum interval to stay safe.  Round up for margin.
    const warn = $("#lora-warn");
    const intervalS = state.lastSettings?.interval_s;
    if (warn && lo.region === 0 && lo.enabled && intervalS && intervalS < 30) {
      warn.textContent = `Warning: capture interval ${intervalS}s may exceed EU 868 MHz 1% duty-cycle limit (need ≥ 30 s).`;
    } else if (warn) {
      warn.textContent = "";
    }
  };

  const setLoraSegActive = (selector, value) => {
    $$(selector).forEach(b => {
      const v = (b.dataset.loraRegion ?? b.dataset.loraPwr);
      b.classList.toggle("is-active", parseInt(v, 10) === value);
    });
  };

  // Currently selected channel for the LoRa form (mirrored from button grid).
  let loraSelectedCh = null;

  // (Re)build the channel grid. In "all" mode shows every channel 0..80;
  // otherwise shows just the channels valid for the given region. If the
  // current selection is outside the rendered range, snaps to a sensible
  // default (region default, or 18 when in "all" mode).
  const renderChannelGrid = (regionId, selectedCh) => {
    const grid = $("#lora-ch-grid");
    if (!grid) return;
    const prefs = loadBandPrefs();

    let chMin, chMax, defaultCh, regionDefault;
    if (prefs.all) {
      chMin = 0;
      chMax = LORA_CH_MAX;
      regionDefault = REGIONS[0].default;          // arbitrary safe default
    } else {
      const r = REGIONS[regionId];
      if (!r) return;
      chMin = r.min;
      chMax = r.max;
      regionDefault = r.default;
    }

    const ch = (selectedCh != null && selectedCh >= chMin && selectedCh <= chMax)
                 ? selectedCh : regionDefault;
    loraSelectedCh = ch;
    grid.innerHTML = "";
    for (let c = chMin; c <= chMax; c++) {
      const b = document.createElement("button");
      b.type = "button";
      b.className = "ch-btn" + (c === ch ? " is-active" : "");
      b.dataset.ch = String(c);
      b.setAttribute("role", "radio");
      b.setAttribute("aria-checked", c === ch ? "true" : "false");
      // Tag the EU and US region defaults so the user still sees the
      // "preferred" channel for each band when "show all" is on.
      let tag = "";
      if (c === REGIONS[0].default) tag = "EU default";
      else if (c === REGIONS[1].default) tag = "US default";
      b.innerHTML =
        `<span class="ch-btn__num">CH ${c}</span>` +
        `<span class="ch-btn__freq">${channelToMhz(c).toFixed(3)} MHz</span>` +
        (tag ? `<span class="ch-btn__tag">${tag}</span>` : "");
      b.addEventListener("click", () => {
        loraSelectedCh = c;
        grid.querySelectorAll(".ch-btn").forEach(x => {
          const on = (x.dataset.ch | 0) === c;
          x.classList.toggle("is-active", on);
          x.setAttribute("aria-checked", on ? "true" : "false");
        });
        const hint = $("#lora-ch-hint");
        if (hint) hint.textContent = `ch ${c} · ${channelToMhz(c).toFixed(3)} MHz`;
        markLoraDirty();
      });
      grid.appendChild(b);
    }
    const hint = $("#lora-ch-hint");
    if (hint) hint.textContent = `ch ${ch} · ${channelToMhz(ch).toFixed(3)} MHz`;
  };

  // Show or hide each region button based on the user's band preferences.
  // In "all" mode the whole region selector disappears -- channels 0..80
  // are presented as a single flat grid. When the active region has been
  // disabled, fall back to the first one that's still enabled.
  const applyBandVisibility = (activeRegion) => {
    const p = loadBandPrefs();
    const seg = $("#lora-region-seg");
    if (p.all) {
      if (seg) seg.style.display = "none";
      return activeRegion;
    }
    if (seg) seg.style.display = "";
    let usable = activeRegion;
    $$('[data-lora-region]').forEach(btn => {
      const band = btn.dataset.band;
      const enabled = (band === "eu") ? p.eu : p.us;
      btn.style.display = enabled ? "" : "none";
    });
    if (usable === 0 && !p.eu) usable = 1;
    if (usable === 1 && !p.us) usable = 0;
    return usable;
  };

  let loraFormDirty = false;
  const markLoraDirty = () => {
    loraFormDirty = true;
    const btn = $("#lora-apply");
    if (btn) {
      btn.classList.add("btn--accent");
      btn.textContent = "Apply •";
    }
  };

  const refreshLoraPage = () => {
    fetch("/api/settings", { cache: "no-store" })
      .then(r => r.json())
      .then(s => {
        if ($("#lora-on"))        $("#lora-on").checked       = !!s.lora_on;
        if ($("#lora-encrypt"))   $("#lora-encrypt").checked  = !!s.lora_encrypt;
        if ($("#lora-key"))       $("#lora-key").value        = s.lora_key ?? 0;
        if ($("#lora-trigger-follow")) $("#lora-trigger-follow").checked = (s.lora_trigger | 0) === 0;
        if ($("#lora-period"))    $("#lora-period").value     = s.lora_tx_period_s ?? 60;

        // Decide which region to display: respect the firmware setting if
        // the user still has that band enabled, otherwise snap to the first
        // enabled band (and the channel will snap to that band's default).
        // In "all" mode the region selector is hidden entirely.
        const prefs = loadBandPrefs();
        let region = s.lora_region ?? 0;
        region = applyBandVisibility(region);

        // Stored channel might be outside the displayed region (band
        // changed, or first run). Snap to region default so we never show
        // a "configured value that nothing represents in the grid".
        let ch = s.lora_channel ?? REGIONS[region].default;
        if (!prefs.all && !isChannelInRegion(ch, region)) {
          ch = REGIONS[region].default;
        }

        setLoraSegActive('[data-lora-region]', region);
        setLoraSegActive('[data-lora-pwr]',    s.lora_tx_power ?? 0);
        renderChannelGrid(region, ch);

        // Show/hide conditional sub-fields based on toggle states
        const keyVis = !!s.lora_encrypt;
        if ($("#lora-key-field"))    $("#lora-key-field").style.display    = keyVis  ? "" : "none";
        const periodVis = (s.lora_trigger | 0) === 1;
        if ($("#lora-period-field")) $("#lora-period-field").style.display = periodVis ? "" : "none";

        loraFormDirty = false;
        const btn = $("#lora-apply");
        if (btn) { btn.classList.remove("btn--accent"); btn.textContent = "Apply"; }
      })
      .catch(() => {});
  };

  const initLoraPage = () => {
    // Region segmented buttons -- rebuild the channel grid and snap to that
    // region's default channel when the user clicks one.
    $$('[data-lora-region]').forEach(btn => {
      btn.addEventListener("click", () => {
        const r = parseInt(btn.dataset.loraRegion, 10);
        if (!isRegionEnabled(r)) return;          // shouldn't happen (hidden)
        setLoraSegActive('[data-lora-region]', r);
        renderChannelGrid(r, REGIONS[r].default);
        markLoraDirty();
      });
    });

    // TX-power segmented buttons
    $$('[data-lora-pwr]').forEach(btn => {
      btn.addEventListener("click", () => {
        setLoraSegActive('[data-lora-pwr]', parseInt(btn.dataset.loraPwr, 10));
        markLoraDirty();
      });
    });

    // Plain inputs -> mark dirty + show/hide conditional fields
    ["#lora-on", "#lora-encrypt", "#lora-key", "#lora-trigger-follow", "#lora-period"].forEach(sel => {
      const el = $(sel);
      if (!el) return;
      el.addEventListener("input", () => {
        if (sel === "#lora-encrypt") {
          $("#lora-key-field").style.display = el.checked ? "" : "none";
        }
        if (sel === "#lora-trigger-follow") {
          $("#lora-period-field").style.display = el.checked ? "none" : "";
        }
        markLoraDirty();
      });
    });

    // Apply -> POST every field at once
    if ($("#lora-apply")) {
      $("#lora-apply").addEventListener("click", () => {
        const region = $$('[data-lora-region].is-active')[0]?.dataset.loraRegion ?? "0";
        const pwr    = $$('[data-lora-pwr].is-active')[0]?.dataset.loraPwr ?? "0";
        const regionInt = parseInt(region, 10);
        // Channel comes from the grid, NOT a hidden cached value -- this is
        // what gates "user picked a 915 channel while EU is selected". In
        // "all" mode any channel 0..LORA_CH_MAX is acceptable, so we only
        // run the region-clamp when filtering is on.
        const prefs = loadBandPrefs();
        let ch = loraSelectedCh;
        if (ch == null || ch < 0 || ch > LORA_CH_MAX) {
          ch = REGIONS[regionInt]?.default ?? 18;
        } else if (!prefs.all && !isChannelInRegion(ch, regionInt)) {
          ch = REGIONS[regionInt].default;
        }
        const body = {
          lora_on:           !!$("#lora-on").checked,
          lora_region:       regionInt,
          lora_channel:      ch | 0,
          lora_tx_power:     parseInt(pwr, 10),
          lora_encrypt:      !!$("#lora-encrypt").checked,
          lora_key:          parseInt($("#lora-key").value || 0, 10) & 0xFFFF,
          lora_trigger:      $("#lora-trigger-follow").checked ? 0 : 1,
          lora_tx_period_s:  parseInt($("#lora-period").value || 60, 10),
        };
        fetch("/api/settings", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify(body),
        })
          .then(r => r.json())
          .then(() => {
            const btn = $("#lora-apply");
            if (btn) {
              btn.textContent = "Saved ✓";
              btn.classList.remove("btn--accent");
              setTimeout(() => { btn.textContent = "Apply"; }, 1200);
            }
            loraFormDirty = false;
            setTimeout(refreshLoraPage, 200);
          })
          .catch(err => {
            const btn = $("#lora-apply");
            if (btn) btn.textContent = "Failed";
            console.error(err);
          });
      });
    }
  };

  // ---------- IMU page rendering ---------------------------------------
  const imuState = {
    canvas: null,
    ctx: null,
    lastEventCounts: { fall: -1, impact: -1 },
    // Acceleration arc-gauge range (in milli-g).  The BMI160 is configured
    // for ±2g full-scale, so a vector magnitude up to ~2000 mg is possible
    // in real handling/impact scenarios.  Range chosen 0..2000 mg so:
    //   - 0    = green   (free-fall)
    //   - 1000 = yellow  (gravity at rest, mid-arc)
    //   - 2000 = red     (~2g impact)
    arcMin: 0,
    arcMax: 2000,
  };

  const arcResize = () => {
    const c = imuState.canvas;
    if (!c) return;
    const dpr = window.devicePixelRatio || 1;
    const r = c.getBoundingClientRect();
    c.width  = Math.round(r.width * dpr);
    c.height = Math.round(r.height * dpr);
    if (imuState.ctx) imuState.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  };

  // Helper: linear interpolate two CSS hex colours
  const lerpColor = (c1, c2, t) => {
    const r = (h) => [
      parseInt(h.slice(1, 3), 16),
      parseInt(h.slice(3, 5), 16),
      parseInt(h.slice(5, 7), 16),
    ];
    const a = r(c1), b = r(c2);
    const m = a.map((v, i) => Math.round(v + (b[i] - v) * t));
    return `rgb(${m[0]},${m[1]},${m[2]})`;
  };

  // Sample the green->yellow->red gradient at parameter t in [0..1]
  const accelGradient = (t) => {
    const GREEN  = "#3ddc84";
    const YELLOW = "#f5c842";
    const RED    = "#ff453a";
    if (t < 0.5) return lerpColor(GREEN, YELLOW, t * 2);
    return lerpColor(YELLOW, RED, (t - 0.5) * 2);
  };

  // Semi-circular acceleration gauge.  180° arc opening downward.
  //
  // Visual model:
  //   1) DIM track: full arc rendered in a low-alpha colored gradient so
  //      the user sees green->yellow->red zones even at rest.
  //   2) BRIGHT fill: from the left endpoint up to the current value, the
  //      same gradient is rendered at full opacity.
  //   3) End cap: round disc at the leading edge of the bright fill.
  //
  // No needle.  No tick marks (the range labels in HTML do that job).
  const drawArcGauge = (mg) => {
    const c = imuState.canvas;
    if (!c || !imuState.ctx) return;
    const ctx = imuState.ctx;
    const w = c.clientWidth, h = c.clientHeight;
    ctx.clearRect(0, 0, w, h);

    const cx     = w / 2;
    const cy     = h * 0.82;
    const radius = Math.min(w * 0.42, h * 0.7);

    // Geometry: 180° arc opening downward.
    const startAng = Math.PI;
    const endAng   = 2 * Math.PI;
    const sweep    = endAng - startAng;          // = π

    // Map current value to a 0..1 progress along the arc, clamped.
    const t = Math.max(0, Math.min(1,
      (mg - imuState.arcMin) / (imuState.arcMax - imuState.arcMin)));
    const curAng = startAng + sweep * t;

    const lineW = Math.max(14, radius * 0.20);

    // 1) DIM coloured track -- ALWAYS visible, even at 0% fill.
    //    Drawn as 64 short coloured sub-arcs at low opacity.
    const STEPS = 64;
    ctx.lineCap   = "butt";
    ctx.lineWidth = lineW;
    for (let i = 0; i < STEPS; i++) {
      const a0 = startAng + (i      / STEPS) * sweep;
      const a1 = startAng + ((i + 1) / STEPS) * sweep + 0.005;
      const colourT = (i + 0.5) / STEPS;
      // Use the same gradient sample but compose with low alpha by
      // converting "rgb(r,g,b)" -> "rgba(r,g,b,0.18)"
      const base = accelGradient(colourT);
      const dim  = base.replace("rgb(", "rgba(").replace(")", ",0.18)");
      ctx.beginPath();
      ctx.strokeStyle = dim;
      ctx.arc(cx, cy, radius, a0, a1);
      ctx.stroke();
    }

    // 2) BRIGHT filled portion -- from left endpoint to curAng.
    if (t > 0) {
      const filledSteps = Math.max(1, Math.round(STEPS * t));
      for (let i = 0; i < filledSteps; i++) {
        const a0 = startAng + (i      / STEPS) * sweep;
        const a1 = startAng + ((i + 1) / STEPS) * sweep + 0.005;
        const colourT = (i + 0.5) / STEPS;
        ctx.beginPath();
        ctx.strokeStyle = accelGradient(colourT);
        ctx.arc(cx, cy, radius, a0, a1);
        ctx.stroke();
      }

      // 3) Round end caps on the bright fill (small filled discs).
      const drawCap = (ang, colour) => {
        ctx.fillStyle = colour;
        ctx.beginPath();
        ctx.arc(
          cx + Math.cos(ang) * radius,
          cy + Math.sin(ang) * radius,
          lineW / 2, 0, Math.PI * 2);
        ctx.fill();
      };
      drawCap(startAng, accelGradient(0));
      drawCap(curAng,   accelGradient(t));
    }
  };

  const drawArcGaugeFromAccel = (magG) => {
    // Convert g → mg for the arc display
    const mg = (magG ?? 0) * 1000.0;
    // Update centre readout
    const num = $("#imu-mag");
    if (num) num.textContent = mg.toFixed(0);
    drawArcGauge(mg);
  };

  // ---------- Environment page (AHT10 temp + humidity) ----------------
  const ENV_HIST = 60;
  const envState = {
    tempHist: [], humHist: [],
    tempCanvas: null, humCanvas: null,
    tempCtx: null,   humCtx: null,
  };

  const envResize = () => {
    [
      [envState.tempCanvas, envState.tempCtx],
      [envState.humCanvas,  envState.humCtx],
    ].forEach(([c, ctx]) => {
      if (!c) return;
      const dpr = window.devicePixelRatio || 1;
      const r = c.getBoundingClientRect();
      c.width  = Math.round(r.width * dpr);
      c.height = Math.round(r.height * dpr);
      if (ctx) ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    });
    drawEnvSparkline(envState.tempCanvas, envState.tempCtx, envState.tempHist, "#3ddc84");
    drawEnvSparkline(envState.humCanvas,  envState.humCtx,  envState.humHist,  "#5cc8ff");
  };

  const drawEnvSparkline = (c, ctx, hist, color) => {
    if (!c || !ctx || hist.length < 2) return;
    const w = c.clientWidth, h = c.clientHeight;
    ctx.clearRect(0, 0, w, h);
    const vals = hist.map(s => s.v).filter(v => v != null && !isNaN(v));
    if (vals.length < 2) return;
    const vMin = Math.min(...vals), vMax = Math.max(...vals);
    const range = vMax - vMin || 1;
    const pad = 6;
    const xAt = i => (i / (ENV_HIST - 1)) * w;
    const yAt = v => v == null ? null : (h - pad) - ((v - vMin) / range) * (h - pad * 2);
    const pts = hist.map((s, i) => ({ x: xAt(i), y: yAt(s.v) }));
    // Area fill
    ctx.save();
    ctx.beginPath();
    let started = false;
    for (let i = 0; i < pts.length; i++) {
      const p = pts[i];
      if (p.y == null) { started = false; continue; }
      if (!started) { ctx.moveTo(p.x, h); ctx.lineTo(p.x, p.y); started = true; }
      else {
        const pv = pts[i-1], mid = (pv.x + p.x) / 2;
        ctx.bezierCurveTo(mid, pv.y, mid, p.y, p.x, p.y);
      }
    }
    if (started) {
      const last = pts.filter(p => p.y != null).pop();
      ctx.lineTo(last.x, h); ctx.closePath();
      const m = color.match(/^#(..)(..)(..)$/);
      if (m) ctx.fillStyle = `rgba(${parseInt(m[1],16)},${parseInt(m[2],16)},${parseInt(m[3],16)},0.13)`;
      ctx.fill();
    }
    ctx.restore();
    // Line
    ctx.save();
    ctx.beginPath();
    started = false;
    for (let i = 0; i < pts.length; i++) {
      const p = pts[i];
      if (p.y == null) { started = false; continue; }
      if (!started) { ctx.moveTo(p.x, p.y); started = true; }
      else {
        const pv = pts[i-1], mid = (pv.x + p.x) / 2;
        ctx.bezierCurveTo(mid, pv.y, mid, p.y, p.x, p.y);
      }
    }
    ctx.lineWidth = 2; ctx.strokeStyle = color;
    ctx.lineCap = "round"; ctx.lineJoin = "round";
    ctx.shadowColor = color; ctx.shadowBlur = 8;
    ctx.stroke();
    ctx.restore();
  };

  const tempComfort = (t) => {
    if (t == null || isNaN(t)) return "";
    if (t < -10)  return "Extremely cold";
    if (t <   0)  return "Below freezing";
    if (t <  10)  return "Cold";
    if (t <  18)  return "Cool";
    if (t <  23)  return "Comfortable temperature";
    if (t <  27)  return "Warm";
    if (t <  32)  return "Hot";
    return "Very hot";
  };

  const humComfort = (h) => {
    if (h == null || isNaN(h)) return "";
    if (h < 20) return "Very dry";
    if (h < 40) return "Dry";
    if (h < 60) return "Good humidity level";
    if (h < 70) return "Slightly humid";
    if (h < 80) return "Humid";
    return "Very humid";
  };

  const renderEnv = (env) => {
    if (!env) return;
    const t = env.temp_c, h = env.humidity;
    const imperial = isImperial();

    envState.tempHist.push({ v: (t != null && !isNaN(t)) ? t : null });
    envState.humHist.push ({ v: (h != null && !isNaN(h)) ? h : null });
    while (envState.tempHist.length > ENV_HIST) envState.tempHist.shift();
    while (envState.humHist.length  > ENV_HIST) envState.humHist.shift();

    // Temperature
    const tn = $("#env-temp-num");
    if (tn) {
      if (t == null || isNaN(t)) { tn.textContent = "––.–"; }
      else { tn.textContent = (imperial ? env.temp_f : t).toFixed(1); }
    }
    const tu = $("#env-temp-unit");
    if (tu) tu.textContent = imperial ? "°F" : "°C";
    const tb = $("#env-temp-status");
    if (tb) {
      tb.textContent = env.fresh ? "Live" : env.ready ? "Stale" : "Offline";
      tb.classList.toggle("card__badge--accent", !!env.fresh);
    }
    const tl = $("#env-temp-label");
    if (tl) tl.textContent = tempComfort(t);

    // Humidity
    const hn = $("#env-hum-num");
    if (hn) hn.textContent = (h != null && !isNaN(h)) ? Math.round(h) : "––";
    const hb = $("#env-hum-status");
    if (hb) {
      hb.textContent = env.fresh ? "Live" : env.ready ? "Stale" : "Offline";
      hb.classList.toggle("card__badge--accent", !!env.fresh);
    }
    const hl = $("#env-hum-label");
    if (hl) hl.textContent = humComfort(h);

    const bar = $("#env-humbar");
    if (bar && h != null && !isNaN(h)) {
      const pct = Math.max(0, Math.min(100, h));
      bar.style.width = pct + "%";
      bar.style.background = pct < 30 ? "#3ddc84"
                           : pct > 70 ? "#5cc8ff"
                           : "linear-gradient(to right, #3ddc84, #5cc8ff)";
    }

    // kv details
    const set = (id, v) => { const e = $(id); if (e) e.textContent = v; };
    set("#env-kv-temp-c", (t != null && !isNaN(t)) ? t.toFixed(2) + " °C" : "–");
    set("#env-kv-temp-f", (env.temp_f != null && !isNaN(env.temp_f)) ? env.temp_f.toFixed(1) + " °F" : "–");
    set("#env-kv-hum",    (h != null && !isNaN(h)) ? h.toFixed(1) + " %" : "–");
    if (t != null && h != null && !isNaN(t) && !isNaN(h)) {
      let comfort = "Acceptable";
      if (t >= 18 && t <= 24 && h >= 40 && h <= 60) comfort = "Ideal conditions";
      else if (t > 27 && h > 60) comfort = "Hot and humid";
      else if (h < 30)            comfort = "Dry — consider humidifier";
      else if (t < 0)             comfort = "Below freezing";
      set("#env-kv-comfort", comfort);
    } else { set("#env-kv-comfort", "–"); }

    // Sparklines: only draw when page visible
    if (document.querySelector('.page--active')?.dataset.page === 'env') {
      drawEnvSparkline(envState.tempCanvas, envState.tempCtx, envState.tempHist, "#3ddc84");
      drawEnvSparkline(envState.humCanvas,  envState.humCtx,  envState.humHist,  "#5cc8ff");
    }
  };

  const initEnv = () => {
    envState.tempCanvas = $("#env-temp-canvas");
    envState.humCanvas  = $("#env-hum-canvas");
    if (envState.tempCanvas) envState.tempCtx = envState.tempCanvas.getContext("2d");
    if (envState.humCanvas)  envState.humCtx  = envState.humCanvas.getContext("2d");
    window.addEventListener("resize", () => requestAnimationFrame(envResize));
    envResize();
  };

  // ---------- Battery ring gauge ---------------------------------------
  const batState = { canvas: null, ctx: null };

  const ringResize = () => {
    const c = batState.canvas;
    if (!c) return;
    const dpr = window.devicePixelRatio || 1;
    const r = c.getBoundingClientRect();
    c.width  = Math.round(r.width * dpr);
    c.height = Math.round(r.height * dpr);
    if (batState.ctx) batState.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    drawBatteryRing();
  };

  const drawBatteryRing = (pct) => {
    const c = batState.canvas;
    if (!c || !batState.ctx) return;
    const ctx = batState.ctx;
    const w = c.clientWidth, h = c.clientHeight;
    ctx.clearRect(0, 0, w, h);

    const cx = w / 2;
    const cy = h / 2;
    const radius = Math.min(w, h) * 0.40;
    const lineW  = Math.max(14, radius * 0.16);

    // Background ring (dark gray, full circle)
    ctx.lineCap = "butt";
    ctx.lineWidth = lineW;
    ctx.strokeStyle = "rgba(255,255,255,0.07)";
    ctx.beginPath();
    ctx.arc(cx, cy, radius, 0, Math.PI * 2);
    ctx.stroke();

    if (pct == null) return;

    // Progress arc, clockwise from 12 o'clock (= -π/2)
    const startAng = -Math.PI / 2;
    const sweep    = (pct / 100) * (Math.PI * 2);
    const endAng   = startAng + sweep;

    // Pick colour: orange/coral default, red below 20%, green above 80%
    let colour = "#e74c3c";              // base coral
    if (pct >= 80) colour = "#3ddc84";   // green when full
    else if (pct < 20) colour = "#ff453a"; // bright red when critical

    ctx.lineCap = "round";
    ctx.lineWidth = lineW;
    ctx.strokeStyle = colour;
    ctx.beginPath();
    ctx.arc(cx, cy, radius, startAng, endAng);
    ctx.stroke();
  };

  const renderBattery = (b) => {
    if (!b) return;
    const pct = (b.percent != null) ? b.percent : 0;
    const v   = b.voltage;
    const onUsb = !!b.on_usb;

    drawBatteryRing(pct);

    const numEl = $("#bat-pct");
    if (numEl) numEl.textContent = onUsb ? "USB" : Math.round(pct);

    if ($("#bat-status")) {
      $("#bat-status").textContent =
        !b.ready    ? "Reading…" :
        onUsb       ? "On USB power" :
        b.low       ? "Low battery" :
        pct >= 80   ? "Full" :
        pct >= 30   ? "OK" : "Charge soon";
      $("#bat-status").classList.toggle("card__badge--accent", !onUsb && pct >= 30);
    }
    if ($("#bat-voltage")) $("#bat-voltage").textContent =
        v == null ? "–.–– V" : `${v.toFixed(2)} V`;
    if ($("#bat-soc")) $("#bat-soc").textContent =
        onUsb ? "n/a (USB)" : `${Math.round(pct)} %`;
    if ($("#bat-source")) $("#bat-source").textContent =
        onUsb ? "USB-C (charging)" : "Battery (Li-Po)";

    // Discharge rate + ETA + history window
    const rate = b.rate_pct_per_hr;
    const hrs  = b.hours_remaining;
    const histMin = b.history_min ?? 0;

    if ($("#bat-rate")) {
      if (onUsb) {
        $("#bat-rate").textContent = "n/a (USB)";
      } else if (rate == null) {
        $("#bat-rate").textContent = `gathering data… (${histMin}/5 min)`;
      } else if (rate > 0.05) {
        $("#bat-rate").textContent = `+${rate.toFixed(2)} %/h (charging)`;
      } else if (rate < -0.05) {
        $("#bat-rate").textContent = `${rate.toFixed(2)} %/h`;
      } else {
        $("#bat-rate").textContent = `${rate.toFixed(2)} %/h (stable)`;
      }
    }
    if ($("#bat-eta")) {
      if (onUsb) {
        $("#bat-eta").textContent = "n/a (USB)";
      } else if (hrs == null || isNaN(hrs)) {
        $("#bat-eta").textContent = (rate == null) ? "—" : "stable / charging";
      } else {
        $("#bat-eta").textContent = fmtHoursHuman(hrs);
      }
    }
    if ($("#bat-hist")) {
      $("#bat-hist").textContent = onUsb
          ? "cleared (on USB)"
          : `${histMin} min recorded`;
    }

    // Dashboard system card row
    const sb = $("#sys-bat");
    if (sb) sb.textContent = onUsb ? "On USB"
                                   : (v == null ? "—" : `${v.toFixed(2)} V · ${Math.round(pct)}%`);
  };

  // Format a duration in hours as "Xh Ym" or "Ym"
  const fmtHoursHuman = (h) => {
    if (h == null || isNaN(h)) return "—";
    if (h < 0)     return "—";
    if (h < 1/60)  return "< 1m";
    const totalMin = Math.round(h * 60);
    if (totalMin < 60) return `${totalMin}m`;
    const hh = Math.floor(totalMin / 60);
    const mm = totalMin % 60;
    if (hh < 24) return mm > 0 ? `${hh}h ${mm}m` : `${hh}h`;
    const dd = Math.floor(hh / 24);
    const hr = hh % 24;
    return hr > 0 ? `${dd}d ${hr}h` : `${dd}d`;
  };

  const renderImu = (im) => {
    // Status badge
    $("#imu-status").textContent = im.ready
        ? (im.last_event ? im.last_event : "ok")
        : "Offline";

    // Big magnitude readout (in mg, set by drawArcGaugeFromAccel below)
    // Older code wrote "1.00" g here -- that's now overwritten by the gauge.

    // Axes bars: ±2g range mapped to 0..100% width with the left edge at 50%
    const setBar = (id, val) => {
      const b = $(id);
      if (!b) return;
      const v = Math.max(-2, Math.min(2, val ?? 0));
      const pct = Math.abs(v) / 2 * 50;     // 0..50%
      if (v >= 0) {
        b.style.left  = "50%";
        b.style.width = pct + "%";
      } else {
        b.style.left  = (50 - pct) + "%";
        b.style.width = pct + "%";
      }
      b.style.background = (Math.abs(v) > 1.5) ? "var(--err)" : "var(--accent)";
    };
    setBar("#bar-x", im.ax);
    setBar("#bar-y", im.ay);
    setBar("#bar-z", im.az);
    const setNum = (id, val) => {
      const e = $(id);
      if (!e) return;
      const v = val ?? 0;
      e.textContent = (v >= 0 ? "+" : "") + v.toFixed(2);
    };
    setNum("#num-x", im.ax);
    setNum("#num-y", im.ay);
    setNum("#num-z", im.az);

    // Peak card
    $("#imu-peak-mag").textContent = (im.peak_mag ?? 0).toFixed(2) + " g";
    $("#imu-peak-x").textContent   = (im.peak_ax  ?? 0).toFixed(2) + " g";
    $("#imu-peak-y").textContent   = (im.peak_ay  ?? 0).toFixed(2) + " g";
    $("#imu-peak-z").textContent   = (im.peak_az  ?? 0).toFixed(2) + " g";
    $("#imu-peak-age").textContent =
        (im.peak_age_ms == null || im.peak_age_ms < 0) ? "–"
        : fmtAgeShort(im.peak_age_ms);

    // Counters
    $("#imu-fall-count").textContent   = (im.fall_count   ?? 0).toLocaleString();
    $("#imu-impact-count").textContent = (im.impact_count ?? 0).toLocaleString();

    // Arc gauge (only redraw if the page is visible to save CPU)
    const onImuPage = document.querySelector('.page--active')?.dataset.page === 'imu';
    if (onImuPage && imuState.canvas) {
      drawArcGaugeFromAccel(im.mag);
    } else {
      // Still update the centre readout text so the dashboard summary
      // doesn't read "––––" while we're on another page.
      const num = $("#imu-mag");
      if (num) num.textContent = ((im.mag ?? 0) * 1000).toFixed(0);
    }

    // If a new event happened, refresh the events table
    const f = im.fall_count   ?? 0;
    const i = im.impact_count ?? 0;
    if (f !== imuState.lastEventCounts.fall ||
        i !== imuState.lastEventCounts.impact) {
      imuState.lastEventCounts.fall   = f;
      imuState.lastEventCounts.impact = i;
      if (onImuPage) refreshImuEvents();
      // Toast a heads-up if we already had a previous count
      if (imuState.lastEventCounts.fall > 0 || imuState.lastEventCounts.impact > 0) {
        if (im.last_event === "FALL")   toast("Free-fall event detected", "warn");
        if (im.last_event === "IMPACT") toast("Impact detected",          "warn");
      }
    }
  };

  const fmtAgeShort = (ms) => {
    const s = Math.round(ms / 1000);
    if (s < 60)    return `${s}s ago`;
    if (s < 3600)  return `${Math.round(s/60)}m ago`;
    if (s < 86400) return `${Math.round(s/3600)}h ago`;
    return `${Math.round(s/86400)}d ago`;
  };

  const refreshImuEvents = async () => {
    try {
      const r = await api("/api/imu/events");
      const wrap = $("#imu-evlist");
      if (!wrap) return;
      if (!r.events || r.events.length === 0) {
        wrap.innerHTML = `<div class="evlist__empty">No events recorded yet.</div>`;
        return;
      }
      wrap.innerHTML = "";
      r.events.forEach(e => {
        const row = document.createElement("div");
        row.className = "evrow";
        const cls = e.type === "IMPACT" ? "evrow__type--impact" : "evrow__type--fall";
        const det = e.type === "FALL"
            ? `mag ${e.mag.toFixed(2)}g · ${e.duration_ms}ms · X${e.ax.toFixed(2)} Y${e.ay.toFixed(2)} Z${e.az.toFixed(2)}`
            : `mag ${e.mag.toFixed(2)}g · X${e.ax.toFixed(2)} Y${e.ay.toFixed(2)} Z${e.az.toFixed(2)}`;
        row.innerHTML =
          `<span class="evrow__type ${cls}">${e.type}</span>` +
          `<span class="evrow__mag">${e.mag.toFixed(2)} g</span>` +
          `<span class="evrow__det">${det}</span>` +
          `<span class="evrow__age">${fmtAgeShort(e.age_ms)}</span>`;
        wrap.appendChild(row);
      });
    } catch (e) {
      // Silently ignore -- list will populate on next status tick
    }
  };

  const initImu = () => {
    imuState.canvas = $("#imu-arc");
    if (imuState.canvas) {
      imuState.ctx = imuState.canvas.getContext("2d");
      arcResize();
      window.addEventListener("resize", () => requestAnimationFrame(arcResize));
    }
  };

  const initBattery = () => {
    batState.canvas = $("#bat-ring");
    if (batState.canvas) {
      batState.ctx = batState.canvas.getContext("2d");
      ringResize();
      window.addEventListener("resize", () => requestAnimationFrame(ringResize));
    }
  };

  // Cached threshold values (still used to label the Settings sliders)
  const state = {
    thresholds: { fall_g: 0.30, impact_g: 3.50, fall_window_ms: 150, alert_cooldown_ms: 10000 },
  };

  // ---------- Map (canvas) ---------------------------------------------
  // Strategy: try to fetch slippy-map tiles from /tiles?z=&x=&y= (served
  // from SD card).  If missing, fall back to a clean vector graticule that
  // shows lat/lon grid lines centred on the current position.
  const mapState = {
    canvas: null,
    ctx: null,
    z: 16,            // default zoom
    tileCache: new Map(),
    haveTiles: null,  // null=unknown, true=yes, false=no
  };

  const lonToTileX = (lon, z) => Math.floor(((lon + 180) / 360) * (1 << z));
  const latToTileY = (lat, z) => {
    const r = lat * Math.PI / 180;
    return Math.floor((1 - Math.log(Math.tan(r) + 1 / Math.cos(r)) / Math.PI) / 2 * (1 << z));
  };
  const lonToPx = (lon, z) => ((lon + 180) / 360) * (1 << z) * 256;
  const latToPx = (lat, z) => {
    const r = lat * Math.PI / 180;
    return (1 - Math.log(Math.tan(r) + 1 / Math.cos(r)) / Math.PI) / 2 * (1 << z) * 256;
  };

  const mapResize = () => {
    const c = mapState.canvas;
    if (!c) return;
    const dpr = window.devicePixelRatio || 1;
    const r = c.getBoundingClientRect();
    c.width  = Math.round(r.width * dpr);
    c.height = Math.round(r.height * dpr);
    if (mapState.ctx) mapState.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    if (mapLastLat != null && mapLastLon != null) mapDraw(mapLastLat, mapLastLon);
    else mapDraw(null, null);
  };

  const tryLoadTile = (z, x, y) => {
    const key = `${z}/${x}/${y}`;
    if (mapState.tileCache.has(key)) return mapState.tileCache.get(key);
    const p = new Promise((resolve) => {
      const img = new Image();
      img.onload  = () => resolve(img);
      img.onerror = () => resolve(null);
      img.src = `/tiles?z=${z}&x=${x}&y=${y}`;
    });
    mapState.tileCache.set(key, p);
    return p;
  };

  const drawGraticule = (lat, lon) => {
    const ctx = mapState.ctx;
    const c = mapState.canvas;
    const w = c.clientWidth, h = c.clientHeight;
    ctx.clearRect(0, 0, w, h);

    // Black-themed background + radial vignette
    const grd = ctx.createRadialGradient(w/2, h/2, 0, w/2, h/2, Math.max(w,h)/1.2);
    grd.addColorStop(0, "rgba(92,200,255,0.06)");
    grd.addColorStop(1, "rgba(0,0,0,1)");
    ctx.fillStyle = grd;
    ctx.fillRect(0, 0, w, h);

    // Grid
    ctx.strokeStyle = "rgba(255,255,255,0.06)";
    ctx.lineWidth = 1;
    const step = 40;
    for (let x = step; x < w; x += step) {
      ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, h); ctx.stroke();
    }
    for (let y = step; y < h; y += step) {
      ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
    }
    // Heavy axes through the centre
    ctx.strokeStyle = "rgba(255,255,255,0.16)";
    ctx.lineWidth = 1;
    ctx.beginPath(); ctx.moveTo(w/2, 0); ctx.lineTo(w/2, h); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(0, h/2); ctx.lineTo(w, h/2); ctx.stroke();

    // Position marker if we have a fix
    if (lat != null && lon != null) {
      const cx = w / 2, cy = h / 2;
      // Pulse halo
      const t = (Date.now() / 1000) % 2;
      const r = 8 + (t / 2) * 28;
      const a = 1 - (t / 2);
      ctx.beginPath();
      ctx.arc(cx, cy, r, 0, Math.PI * 2);
      ctx.strokeStyle = `rgba(92,200,255,${0.5 * a})`;
      ctx.lineWidth = 1.5;
      ctx.stroke();

      // Solid dot
      ctx.beginPath();
      ctx.arc(cx, cy, 7, 0, Math.PI * 2);
      ctx.fillStyle = "#5cc8ff";
      ctx.fill();
      ctx.lineWidth = 2;
      ctx.strokeStyle = "rgba(0,0,0,0.6)";
      ctx.stroke();

      // Coordinate label
      ctx.font = '11px ui-monospace, "SF Mono", Menlo, monospace';
      ctx.fillStyle = "rgba(255,255,255,0.85)";
      const txt = `${lat.toFixed(5)}, ${lon.toFixed(5)}`;
      const tw = ctx.measureText(txt).width;
      ctx.fillStyle = "rgba(0,0,0,0.65)";
      ctx.fillRect(cx - tw/2 - 6, cy + 16, tw + 12, 20);
      ctx.fillStyle = "rgba(255,255,255,0.85)";
      ctx.fillText(txt, cx - tw/2, cy + 30);
    }
  };

  const drawTiledMap = async (lat, lon) => {
    const ctx = mapState.ctx;
    const c = mapState.canvas;
    const w = c.clientWidth, h = c.clientHeight;
    const z = mapState.z;

    const cxPx = lonToPx(lon, z);
    const cyPx = latToPx(lat, z);
    const tileX = Math.floor(cxPx / 256);
    const tileY = Math.floor(cyPx / 256);
    const offX = cxPx - tileX * 256;
    const offY = cyPx - tileY * 256;

    // How many tiles do we need around the centre?
    const tilesW = Math.ceil(w / 256) + 1;
    const tilesH = Math.ceil(h / 256) + 1;

    ctx.fillStyle = "#000000";
    ctx.fillRect(0, 0, w, h);

    let drewAny = false;
    const promises = [];
    for (let dx = -Math.floor(tilesW/2); dx <= Math.floor(tilesW/2); dx++) {
      for (let dy = -Math.floor(tilesH/2); dy <= Math.floor(tilesH/2); dy++) {
        const tx = tileX + dx;
        const ty = tileY + dy;
        const screenX = w/2 + dx * 256 - offX;
        const screenY = h/2 + dy * 256 - offY;
        promises.push(tryLoadTile(z, tx, ty).then(img => {
          if (img) {
            ctx.drawImage(img, screenX, screenY, 256, 256);
            drewAny = true;
          }
        }));
      }
    }
    await Promise.all(promises);

    if (!drewAny) {
      // No tiles -> fall back to graticule
      mapState.haveTiles = false;
      $("#map-source-tag").textContent = "vector graticule";
      drawGraticule(lat, lon);
      return;
    }
    mapState.haveTiles = true;
    $("#map-source-tag").textContent = `tiles · z${z}`;

    // Subtle dim overlay so the marker pops
    ctx.fillStyle = "rgba(0,0,0,0.15)";
    ctx.fillRect(0, 0, w, h);

    // Marker on top
    const cx = w/2, cy = h/2;
    const t = (Date.now() / 1000) % 2;
    const r = 8 + (t / 2) * 28;
    const a = 1 - (t / 2);
    ctx.beginPath();
    ctx.arc(cx, cy, r, 0, Math.PI * 2);
    ctx.strokeStyle = `rgba(92,200,255,${0.7 * a})`;
    ctx.lineWidth = 2;
    ctx.stroke();

    ctx.beginPath();
    ctx.arc(cx, cy, 7, 0, Math.PI * 2);
    ctx.fillStyle = "#5cc8ff";
    ctx.fill();
    ctx.lineWidth = 2;
    ctx.strokeStyle = "rgba(0,0,0,0.7)";
    ctx.stroke();
  };

  const mapDraw = (lat, lon) => {
    if (!mapState.canvas) return;
    if (lat == null || lon == null) {
      drawGraticule(null, null);
      return;
    }
    if (mapState.haveTiles === false) {
      drawGraticule(lat, lon);
    } else {
      drawTiledMap(lat, lon);
    }
  };

  const initMap = () => {
    mapState.canvas = $("#map-canvas");
    if (!mapState.canvas) return;
    mapState.ctx = mapState.canvas.getContext("2d");
    mapResize();
    window.addEventListener("resize", () => requestAnimationFrame(mapResize));
    // Animate the position marker pulse
    setInterval(() => {
      if (mapLastLat != null) mapDraw(mapLastLat, mapLastLon);
    }, 60);
  };

  // ---------- live status loop ------------------------------------------
  let pollTimer = null;
  const startPolling = () => {
    if (pollTimer) clearInterval(pollTimer);
    const tick = async () => {
      try {
        const s = await api("/api/status");
        renderStatus(s);
      } catch (e) {
        const pill = $("#conn-pill");
        pill.classList.remove("is-ok", "is-warn");
        pill.classList.add("is-err");
        $("#conn-text").textContent = "Reconnecting…";
      }
    };
    tick();
    pollTimer = setInterval(tick, 1000);
  };

  // ---------- fast ultrasonic loop (5 Hz) -------------------------------
  // The ultrasonic sensor is the primary measurement and the user wants the
  // live readout to feel instant. /api/status carries everything (GPS, IMU,
  // battery, LoRa stats…) and is bulky, so we only poll it at 1 Hz. For the
  // hero distance/depth readouts we hit the lightweight /api/ultrasonic/live
  // endpoint at 5 Hz. The fresh-pill, hero values, and Snow page values get
  // updated every 200 ms; the chart and "system" cards stay on the 1 Hz path
  // so we don't oversample the rolling graph.
  let fastTimer = null;
  let fastFailStreak = 0;
  const startFastUltrasonicLoop = () => {
    if (fastTimer) clearInterval(fastTimer);
    const tick = async () => {
      try {
        const u = await api("/api/ultrasonic/live");
        fastFailStreak = 0;

        // Smooth ~260ms tween between the previously displayed number and
        // the new sample -- replaces the old setReadout() path that fired
        // a .is-flash colour pulse on every 5 Hz update (visually noisy).
        tweenReadout("hero-distance", u.distance_cm);
        tweenReadout("hero-depth",    u.depth_cm);
        tweenReadout("snow-distance", u.distance_cm);
        tweenReadout("snow-depth",    u.depth_cm);
        // Keep the previous-text caches in sync so the 1 Hz status loop
        // doesn't repaint on top of us if a value happens to match.
        lastDistText  = fmtCm(u.distance_cm);
        lastDepthText = fmtCm(u.depth_cm);

        const heroFresh = document.getElementById("hero-fresh");
        if (heroFresh) heroFresh.textContent = u.fresh ? "Live" : "Stale";
      } catch (e) {
        // Keep going silently for a couple of failures; the 1 Hz loop will
        // surface a real connection error to the user via the conn pill.
        fastFailStreak++;
      }
    };
    tick();
    fastTimer = setInterval(tick, 200);
  };

  // ---------- SD page ---------------------------------------------------
  let sdRefreshing = false;
  const refreshSdPage = async () => {
    if (sdRefreshing) return;
    sdRefreshing = true;
    try {
      const [info, list, prev] = await Promise.all([
        api("/api/sd/info"),
        api("/api/sd/list"),
        api("/api/sd/preview?lines=50"),
      ]);
      const badge = $("#sd-status-badge");
      badge.textContent = info.ready ? (info.nearly_full ? "Nearly full" : "Ready") : "Offline";

      $("#sd-total").textContent = fmtBytes(info.total_bytes);
      $("#sd-used").textContent  = fmtBytes(info.used_bytes);
      $("#sd-free").textContent  = fmtBytes(info.free_bytes);
      $("#sd-rows").textContent  = info.rows.toLocaleString();
      $("#sd-active").textContent = info.active_file || "–";

      const pct = info.total_bytes ? Math.min(100, (info.used_bytes / info.total_bytes) * 100) : 0;
      const bar = $("#sd-bar");
      bar.style.width = pct.toFixed(1) + "%";
      bar.classList.remove("is-warn", "is-err");
      if      (pct > 95) bar.classList.add("is-err");
      else if (pct > 85) bar.classList.add("is-warn");

      // File list
      const listEl = $("#sd-filelist");
      listEl.innerHTML = "";
      if (!list.files || list.files.length === 0) {
        listEl.innerHTML = `<div class="filelist__empty">No CSV files found yet</div>`;
      } else {
        list.files.forEach(f => {
          const item = document.createElement("div");
          item.className = "fileitem";
          item.innerHTML =
            `<span class="fileitem__name">${f.name}</span>
             <span class="fileitem__meta">
               <span>${fmtBytes(f.size)}</span>
               <a class="fileitem__dl" href="/api/sd/download?file=${encodeURIComponent(f.name)}" download>
                 <svg><use href="#i-download"/></svg> Download
               </a>
             </span>`;
          listEl.appendChild(item);
        });
      }

      // CSV grid preview (replaces the raw <pre>)
      $("#sd-preview-name").textContent = info.active_file || "–";
      renderCsvGrid(prev || "");
    } catch (e) {
      toast("Could not load SD info", "warn");
    } finally {
      sdRefreshing = false;
    }
  };

  // ---------- CSV grid renderer (spreadsheet-style) -----------------
  // Schema (v0.3.0): timestamp,distance_cm,depth_cm,outlier,event,
  //                  lat,lon,alt_m,speed_kmh,sats,hdop
  // - Row-number column (sticky left, like Excel/Sheets)
  // - All real column names visible across the top
  // - Vertical grid lines + alternating zebra stripes
  // - Outlier flag visualised as red row + bold red distance
  // - Event column rendered as FALL / IMPACT pill badges
  // - Newest rows at the top so the most recent reading is visible
  //   without scrolling
  // Localstorage preference: when false, the SD-preview suppresses the
  // red outlier-row tint/edge so all rows look identical regardless of
  // the flag. Default true.
  const OUTLIER_HL_KEY = "outlier_highlight";
  const isOutlierHighlightOn = () => {
    try { return localStorage.getItem(OUTLIER_HL_KEY) !== "0"; }
    catch { return true; }
  };
  const setOutlierHighlight = (on) => {
    try { localStorage.setItem(OUTLIER_HL_KEY, on ? "1" : "0"); } catch {}
    applyOutlierHighlightClass();
  };
  const applyOutlierHighlightClass = () => {
    const wrap = document.getElementById("sd-csvgrid");
    if (!wrap) return;
    wrap.classList.toggle("csvgrid--no-outlier", !isOutlierHighlightOn());
  };

  const renderCsvGrid = (text) => {
    const wrap = $("#sd-csvgrid");
    wrap.innerHTML = "";
    applyOutlierHighlightClass();

    const lines = text.split(/\r?\n/).filter(l => l.length > 0);
    if (lines.length < 2) {
      wrap.innerHTML = `<div class="csvgrid__loading">No data yet — waiting for first row.</div>`;
      return;
    }

    const headers = lines[0].split(",");
    const rows    = lines.slice(1).map(l => l.split(","));

    // Map raw CSV header names to friendly column titles.  Older files (v0.1.x)
    // are shorter; this map covers all supported schemas.
    const HEADER_LABELS = {
      "timestamp":   "Time (UTC)",
      "distance_cm": isImperial() ? "Dist (in)" : "Dist (cm)",
      "depth_cm":    isImperial() ? "Depth (in)": "Depth (cm)",
      "outlier":     "",   // hidden -- visualised as row tint
      "event":       "Event",
      "lat":         "Latitude",
      "lon":         "Longitude",
      "alt_m":       "Alt (m)",
      "speed_kmh":   "Speed",
      "sats":        "Sats",
      "hdop":        "HDOP",
      "magnitude_g": "Mag (g)",
      "ax_g":        "Ax (g)",
      "ay_g":        "Ay (g)",
      "az_g":        "Az (g)",
      "duration_ms": "Dur (ms)",
    };
    const numCols = new Set([
      "distance_cm","depth_cm","lat","lon","alt_m","speed_kmh","sats","hdop",
      "magnitude_g","ax_g","ay_g","az_g","duration_ms",
    ]);
    const convertCols = new Set(["distance_cm","depth_cm"]);
    const outlierIdx  = headers.indexOf("outlier");
    const eventIdx    = headers.indexOf("event");

    const table = document.createElement("table");
    const thead = document.createElement("thead");
    const trH = document.createElement("tr");

    // Row-number header cell (sticky left)
    const thRn = document.createElement("th");
    thRn.className = "rownum";
    thRn.textContent = "#";
    trH.appendChild(thRn);

    headers.forEach((h, i) => {
      if (h === "outlier") return;     // raw flag hidden; we tint the row
      const th = document.createElement("th");
      th.textContent = HEADER_LABELS[h] || h;
      trH.appendChild(th);
    });
    thead.appendChild(trH);
    table.appendChild(thead);

    const tbody = document.createElement("tbody");
    // Newest first, but row numbers reflect insertion order (1 = oldest)
    const total = rows.length;
    for (let r = total - 1; r >= 0; r--) {
      const cells = rows[r];
      const tr = document.createElement("tr");
      const isOut = outlierIdx >= 0 && cells[outlierIdx] === "1";
      const evVal = (eventIdx >= 0 ? (cells[eventIdx] || "") : "").trim();
      if (isOut) tr.classList.add("is-outlier");
      if (evVal) {
        tr.classList.add("has-event");
        if (evVal === "IMPACT") tr.classList.add("is-impact");
      }

      // Row-number cell
      const tdRn = document.createElement("td");
      tdRn.className = "rownum";
      tdRn.textContent = (r + 1);
      tr.appendChild(tdRn);

      headers.forEach((h, i) => {
        if (h === "outlier") return;
        const td = document.createElement("td");
        let v = cells[i] ?? "";

        if (h === "event") {
          td.className = "event";
          if (v === "FALL")        td.innerHTML = `<span class="ev ev--fall">FALL</span>`;
          else if (v === "IMPACT") td.innerHTML = `<span class="ev ev--impact">IMPACT</span>`;
          else { td.textContent = "—"; td.classList.add("empty"); }
        } else if (v === "") {
          td.textContent = "—";
          td.classList.add("empty");
        } else if (numCols.has(h)) {
          let num = parseFloat(v);
          if (!isNaN(num)) {
            if (convertCols.has(h) && isImperial()) num = num / 2.54;
            td.textContent = (h === "sats" || h === "duration_ms") ? num.toFixed(0)
                           : (h === "lat" || h === "lon")          ? num.toFixed(5)
                           : (h === "ax_g" || h === "ay_g" || h === "az_g" || h === "magnitude_g")
                                                                   ? num.toFixed(2)
                                                                   : num.toFixed(1);
          } else {
            td.textContent = v;
          }
          td.classList.add("num");
        } else {
          td.textContent = v;
          if (h === "timestamp") td.classList.add("timestamp");
        }
        tr.appendChild(td);
      });
      tbody.appendChild(tr);
    }
    table.appendChild(tbody);
    wrap.appendChild(table);
  };

  // ---------- Settings page --------------------------------------------
  const refreshSettingsPage = async () => {
    try {
      const s = await api("/api/settings");
      // Theme
      $$('.seg__opt[data-theme]').forEach(b => b.classList.toggle(
        "is-active", b.dataset.theme === (s.theme === 1 ? "light" : "dark")));
      document.documentElement.dataset.theme = (s.theme === 1) ? "light" : "dark";

      // Units
      $$('.seg__opt[data-units]').forEach(b => b.classList.toggle(
        "is-active", b.dataset.units === (s.units === 1 ? "imperial" : "metric")));
      document.documentElement.dataset.units = (s.units === 1) ? "imperial" : "metric";
      updateUnitLabels();

      // Interval — pick the largest unit that gives an integer value
      const sec = s.interval_s;
      let unit = 1;
      if (sec % 86400 === 0 && sec >= 86400)      unit = 86400;
      else if (sec % 3600 === 0 && sec >= 3600)   unit = 3600;
      else if (sec % 60   === 0 && sec >= 60)     unit = 60;
      $("#input-interval-value").value = sec / unit;
      $("#input-interval-unit").value  = String(unit);

      // Rotation
      $$('.seg__opt[data-rot]').forEach(b => b.classList.toggle(
        "is-active", parseInt(b.dataset.rot, 10) === s.rotation));
      $("#input-rotate-rows").value = s.rotate_rows;
      // The "Rows per file" form is only meaningful when "By rows" (rot=4)
      // is active. Hide it otherwise so the user isn't presented with an
      // editable field that has no effect on the currently selected mode.
      const rotForm = $("#form-rotate-rows");
      if (rotForm) rotForm.style.display = (s.rotation === 4) ? "" : "none";

      // Deep sleep
      $("#input-deep-sleep").checked = !!s.deep_sleep;
      $("#sleep-status").textContent = s.deep_sleep
          ? (s.interval_s >= 30 ? "Active" : "Pending (interval < 30s)")
          : "Off";

      // Temperature correction for ultrasonic distance
      const tcEl = $("#input-temp-correction");
      if (tcEl) tcEl.checked = !!s.temp_correction;

      // Fall-detection thresholds
      const fall   = s.fall_g            ?? 0.30;
      const impact = s.impact_g          ?? 3.50;
      const fwin   = s.fall_window_ms    ?? 150;
      const cd     = s.alert_cooldown_ms ?? 10000;
      state.thresholds.fall_g            = fall;
      state.thresholds.impact_g          = impact;
      state.thresholds.fall_window_ms    = fwin;
      state.thresholds.alert_cooldown_ms = cd;

      const fEl = $("#input-fall-g");   if (fEl)  fEl.value  = fall;
      const iEl = $("#input-impact-g"); if (iEl)  iEl.value  = impact;
      const wEl = $("#input-fall-win"); if (wEl)  wEl.value  = fwin;
      const cEl = $("#input-alert-cd"); if (cEl)  cEl.value  = cd;
      updateThresholdLabels();
    } catch (e) {
      toast("Could not load settings", "warn");
    }
  };

  const updateThresholdLabels = () => {
    const f  = parseFloat($("#input-fall-g")?.value   || state.thresholds.fall_g);
    const i  = parseFloat($("#input-impact-g")?.value || state.thresholds.impact_g);
    const w  = parseInt($("#input-fall-win")?.value   || state.thresholds.fall_window_ms, 10);
    const c  = parseInt($("#input-alert-cd")?.value   || state.thresholds.alert_cooldown_ms, 10);
    if ($("#lbl-fall-g"))   $("#lbl-fall-g").textContent   = f.toFixed(2) + " g";
    if ($("#lbl-impact-g")) $("#lbl-impact-g").textContent = i.toFixed(2) + " g";
    if ($("#lbl-fall-win")) $("#lbl-fall-win").textContent = w + " ms";
    if ($("#lbl-alert-cd")) $("#lbl-alert-cd").textContent =
        c >= 60000 ? `${Math.round(c / 60000)} min`
                   : `${Math.round(c / 1000)} s`;
  };

  // ---------- Wire everything up ---------------------------------------
  document.addEventListener("DOMContentLoaded", () => {
    initRouter();
    initMap();
    initImu();
    initEnv();
    initBattery();
    initChart();
    initLoraPage();
    startPolling();
    startFastUltrasonicLoop();
    updateUnitLabels();

    // ----- Snow: tare-now button --------------------------------------
    $("#btn-tare-now").addEventListener("click", async () => {
      try {
        const r = await api("/api/tare", { method: "POST" });
        toast(`Tare set to ${fmtCm(r.tare_cm)} ${isImperial() ? "in" : "cm"}`, "ok");
      } catch (e) {
        toast(e.body?.error || "Tare failed (no fresh reading?)", "warn");
      }
    });

    $("#btn-tare-reset").addEventListener("click", async () => {
      try {
        const r = await api("/api/tare/reset", { method: "POST" });
        // Don't wait for the next 1 Hz status poll -- the user needs to see
        // immediate confirmation that the tare really went to zero, otherwise
        // the visible "Tare X cm" pill keeps showing the old value for up to
        // a second and makes the button feel broken.
        const tareTxt = `${fmtCm(r?.tare_cm ?? 0)} ${isImperial() ? "in" : "cm"}`;
        const heroTare = document.getElementById("hero-tare-info");
        if (heroTare) heroTare.textContent = `Tare ${tareTxt}`;
        const snowTare = document.getElementById("snow-tare");
        if (snowTare) snowTare.textContent = fmtCm(r?.tare_cm ?? 0);
        toast("Tare reset to 0", "ok");
      } catch (e) { toast("Could not reset tare", "warn"); }
    });

    $("#form-tare-manual").addEventListener("submit", async (e) => {
      e.preventDefault();
      const raw = parseFloat($("#input-tare-manual").value);
      if (isNaN(raw) || raw < 0) { toast("Enter a positive number", "warn"); return; }
      const cm = isImperial() ? raw * 2.54 : raw;
      try {
        const r = await api("/api/tare/manual", {
          method: "POST",
          body: JSON.stringify({ value: cm }),
        });
        toast(`Manual tare ${fmtCm(r.tare_cm)} ${isImperial() ? "in" : "cm"} saved`, "ok");
        $("#input-tare-manual").value = "";
      } catch (e) { toast("Could not save tare", "warn"); }
    });

    // ----- SD: clear log + refresh ------------------------------------
    $("#btn-sd-clear").addEventListener("click", async () => {
      if (!confirm("Clear the active log file? The CSV will be emptied (header preserved). This cannot be undone.")) return;
      try {
        await api("/api/sd/clear", { method: "POST" });
        toast("Active log cleared", "ok");
        refreshSdPage();
      } catch (e) { toast("Clear failed", "err"); }
    });
    $("#btn-sd-refresh").addEventListener("click", refreshSdPage);

    // ----- Settings: theme --------------------------------------------
    $$('.seg__opt[data-theme]').forEach(btn => {
      btn.addEventListener("click", async () => {
        $$('.seg__opt[data-theme]').forEach(b => b.classList.remove("is-active"));
        btn.classList.add("is-active");
        const theme = btn.dataset.theme;
        document.documentElement.dataset.theme = theme;
        try {
          await api("/api/settings", {
            method: "POST",
            body: JSON.stringify({ theme: theme === "light" ? 1 : 0 }),
          });
        } catch (e) { toast("Could not save theme", "warn"); }
      });
    });

    // ----- Settings: units --------------------------------------------
    $$('.seg__opt[data-units]').forEach(btn => {
      btn.addEventListener("click", async () => {
        $$('.seg__opt[data-units]').forEach(b => b.classList.remove("is-active"));
        btn.classList.add("is-active");
        const u = btn.dataset.units;
        document.documentElement.dataset.units = u;
        updateUnitLabels();
        try {
          await api("/api/settings", {
            method: "POST",
            body: JSON.stringify({ units: u === "imperial" ? 1 : 0 }),
          });
        } catch (e) { toast("Could not save units", "warn"); }
      });
    });

    // ----- Settings: interval -----------------------------------------
    $("#form-interval").addEventListener("submit", async (e) => {
      e.preventDefault();
      const v = parseInt($("#input-interval-value").value, 10);
      const u = parseInt($("#input-interval-unit").value, 10);
      if (isNaN(v) || v < 1) { toast("Interval must be >= 1", "warn"); return; }
      const sec = v * u;
      if (sec < 1 || sec > 172800) {
        toast("Interval must be between 1 second and 2 days", "warn"); return;
      }
      try {
        await api("/api/settings", {
          method: "POST",
          body: JSON.stringify({ interval_s: sec }),
        });
        toast(`Interval saved: ${fmtSecondsHuman(sec)}`, "ok");
      } catch (e) { toast("Could not save interval", "warn"); }
    });

    // Quick interval presets
    $$('#quick-intervals .seg__opt').forEach(btn => {
      btn.addEventListener("click", () => {
        const sec = parseInt(btn.dataset.sec, 10);
        let unit = 1, val = sec;
        if (sec % 86400 === 0 && sec >= 86400)      { unit = 86400; val = sec / 86400; }
        else if (sec % 3600 === 0 && sec >= 3600)   { unit = 3600;  val = sec / 3600; }
        else if (sec % 60   === 0 && sec >= 60)     { unit = 60;    val = sec / 60; }
        $("#input-interval-value").value = val;
        $("#input-interval-unit").value  = String(unit);
      });
    });

    // Show/hide the "Rows per file" form depending on whether "By rows"
    // (data-rot=4) is the active rotation mode. The field is meaningless
    // for the other modes (Single file / Daily / Weekly / Monthly), so
    // hiding it removes the visual question "should I fill this in?".
    const syncRowsFieldVisibility = (rot) => {
      const form = $("#form-rotate-rows");
      if (form) form.style.display = (rot === 4) ? "" : "none";
    };

    // ----- Settings: rotation -----------------------------------------
    $$('.seg__opt[data-rot]').forEach(btn => {
      btn.addEventListener("click", async () => {
        $$('.seg__opt[data-rot]').forEach(b => b.classList.remove("is-active"));
        btn.classList.add("is-active");
        const rot = parseInt(btn.dataset.rot, 10);
        syncRowsFieldVisibility(rot);
        try {
          await api("/api/settings", {
            method: "POST",
            body: JSON.stringify({ rotation: rot }),
          });
          const labels = ["Single file","Daily","Weekly","Monthly","By rows"];
          toast(`Rotation: ${labels[rot]}`, "ok");
        } catch (e) { toast("Could not save rotation", "warn"); }
      });
    });

    $("#form-rotate-rows").addEventListener("submit", async (e) => {
      e.preventDefault();
      const n = parseInt($("#input-rotate-rows").value, 10);
      if (isNaN(n) || n < 100) { toast("Min 100 rows", "warn"); return; }
      try {
        await api("/api/settings", {
          method: "POST",
          body: JSON.stringify({ rotate_rows: n }),
        });
        toast(`Row limit saved: ${n.toLocaleString()}`, "ok");
      } catch (e) { toast("Could not save row limit", "warn"); }
    });

    // ----- Settings: deep sleep ---------------------------------------
    $("#input-deep-sleep").addEventListener("change", async (e) => {
      const on = e.target.checked;
      try {
        await api("/api/settings", {
          method: "POST",
          body: JSON.stringify({ deep_sleep: on }),
        });
        toast(on ? "Deep sleep enabled" : "Deep sleep disabled", "ok");
      } catch (err) {
        toast("Could not save deep sleep", "warn");
        e.target.checked = !on;
      }
    });

    // ----- Settings: ultrasonic temperature correction ----------------
    const tcEl = $("#input-temp-correction");
    if (tcEl) {
      tcEl.addEventListener("change", async (e) => {
        const on = e.target.checked;
        try {
          await api("/api/settings", {
            method: "POST",
            body: JSON.stringify({ temp_correction: on }),
          });
          toast(on ? "Temperature correction ON" : "Temperature correction OFF", "ok");
        } catch (err) {
          toast("Could not save temperature correction", "warn");
          e.target.checked = !on;
        }
      });
    }

    // ----- Settings: SD preview -- outlier-row red highlight ----------
    // Local-only preference (no firmware involvement). Just toggles a
    // class on the csvgrid wrapper that the CSS uses to suppress the
    // red tint and red left-edge accent.
    const olEl = $("#setting-outlier-highlight");
    if (olEl) {
      olEl.checked = isOutlierHighlightOn();
      olEl.addEventListener("change", () => {
        setOutlierHighlight(olEl.checked);
        toast(olEl.checked
                ? "Outlier rows will be highlighted in red"
                : "Outlier highlight disabled", "ok");
      });
    }

    // ----- Settings: LoRa band visibility -----------------------------
    // Three checkboxes that control which region selectors appear on the
    // LoRa tab. Stored client-side (localStorage); the firmware is unaware.
    //   - EU 868 / US 915: independently hide each region's filter button
    //   - Show all       : bypasses filtering, shows channels 0..80 as one
    //                      flat grid (the region selector is hidden too)
    // If the user disables a band that's currently active, refreshLoraPage()
    // will snap to the other one's default channel.
    const bandEu  = $("#setting-band-eu");
    const bandUs  = $("#setting-band-us");
    const bandAll = $("#setting-band-all");
    if (bandEu && bandUs && bandAll) {
      const syncLockStates = () => {
        // While "show all" is on, EU/US filters have no effect. Visually
        // dim them and disable interaction so the user understands why.
        const locked = bandAll.checked;
        [bandEu, bandUs].forEach(cb => {
          cb.disabled = locked;
          cb.closest(".switch").classList.toggle("is-locked", locked);
        });
      };

      const prefs0 = loadBandPrefs();
      bandEu.checked  = prefs0.eu;
      bandUs.checked  = prefs0.us;
      bandAll.checked = prefs0.all;
      syncLockStates();

      const persist = () => {
        const next = {
          eu:  bandEu.checked,
          us:  bandUs.checked,
          all: bandAll.checked,
        };
        // When "all" is OFF we need at least one of EU/US enabled.
        if (!next.all && !next.eu && !next.us) {
          bandEu.checked = true;
          next.eu = true;
          toast("At least one band must stay enabled", "warn");
        }
        saveBandPrefs(next);
        syncLockStates();
        refreshLoraPage();
      };
      bandEu.addEventListener("change",  persist);
      bandUs.addEventListener("change",  persist);
      bandAll.addEventListener("change", persist);
    }

    // ----- Settings: sync time ----------------------------------------
    $("#btn-set-time").addEventListener("click", async () => {
      const epoch = Math.floor(Date.now() / 1000);
      try {
        const r = await api("/api/time", {
          method: "POST",
          body: JSON.stringify({ epoch }),
        });
        toast(`Time synced (${r.iso})`, "ok");
      } catch (e) { toast("Could not sync time", "warn"); }
    });

    // ----- Settings: fall-detection threshold sliders -----------------
    ["#input-fall-g", "#input-impact-g", "#input-fall-win", "#input-alert-cd"]
      .forEach(sel => {
        const el = $(sel);
        if (el) el.addEventListener("input", updateThresholdLabels);
      });

    $("#form-thresholds")?.addEventListener("submit", async (e) => {
      e.preventDefault();
      const payload = {
        fall_g:            parseFloat($("#input-fall-g").value),
        impact_g:          parseFloat($("#input-impact-g").value),
        fall_window_ms:    parseInt($("#input-fall-win").value, 10),
        alert_cooldown_ms: parseInt($("#input-alert-cd").value, 10),
      };
      try {
        const r = await api("/api/settings", {
          method: "POST",
          body: JSON.stringify(payload),
        });
        // Cache the just-saved values
        state.thresholds.fall_g            = r.fall_g            ?? payload.fall_g;
        state.thresholds.impact_g          = r.impact_g          ?? payload.impact_g;
        state.thresholds.fall_window_ms    = r.fall_window_ms    ?? payload.fall_window_ms;
        state.thresholds.alert_cooldown_ms = r.alert_cooldown_ms ?? payload.alert_cooldown_ms;
        toast("Fall-detection thresholds saved", "ok");
      } catch (err) {
        toast("Could not save thresholds", "warn");
      }
    });

    $("#btn-thresh-reset")?.addEventListener("click", async () => {
      const defaults = {
        fall_g: 0.30, impact_g: 3.50,
        fall_window_ms: 150, alert_cooldown_ms: 10000,
      };
      try {
        await api("/api/settings", {
          method: "POST",
          body: JSON.stringify(defaults),
        });
        Object.assign(state.thresholds, defaults);
        $("#input-fall-g").value   = defaults.fall_g;
        $("#input-impact-g").value = defaults.impact_g;
        $("#input-fall-win").value = defaults.fall_window_ms;
        $("#input-alert-cd").value = defaults.alert_cooldown_ms;
        updateThresholdLabels();
        toast("Restored to factory defaults", "ok");
      } catch (e) { toast("Could not restore defaults", "warn"); }
    });

    // ----- IMU: reset peak --------------------------------------------
    $("#btn-imu-reset-peak")?.addEventListener("click", async () => {
      try {
        await api("/api/imu/reset_peak", { method: "POST" });
        toast("Peak reset", "ok");
      } catch (e) { toast("Could not reset peak", "warn"); }
    });
  });
})();
