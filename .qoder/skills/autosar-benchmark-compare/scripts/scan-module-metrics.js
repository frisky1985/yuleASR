// usage: node scan-module-metrics.js <parentDir>  — 每个子目录视为一个模块
const fs = require("fs");
const path = require("path");

const base = process.argv[2];
if (!base) { console.error("usage: node scan-module-metrics.js <parentDir>"); process.exit(1); }

function walk(dir) {
  const out = [];
  for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
    const f = path.join(dir, e.name);
    if (e.isDirectory()) out.push(...walk(f));
    else out.push(f);
  }
  return out;
}

const proto = /^[A-Za-z_][A-Za-z0-9_]*(\s+[A-Za-z_][A-Za-z0-9_]*)*\s*\*?\s*([A-Z][A-Za-z0-9_]*)\s*\(/gm;

const rows = [];
for (const name of fs.readdirSync(base).sort()) {
  const dir = path.join(base, name);
  let st;
  try { st = fs.statSync(dir); } catch { continue; }
  if (!st.isDirectory()) continue;
  let loc = 0, cf = 0, hf = 0;
  const apis = new Set();
  for (const f of walk(dir)) {
    if (!/\.(c|h)$/.test(f)) continue;
    let t;
    try { t = fs.readFileSync(f, "utf8"); } catch { continue; }
    loc += t.split("\n").length;
    if (f.endsWith(".c")) { cf++; continue; }
    hf++;
    t = t.replace(/\/\*[\s\S]*?\*\//g, " ").replace(/\/\/[^\n]*/g, " ");
    for (const m of t.matchAll(proto)) apis.add(m[2]);
  }
  rows.push({ name, cf, hf, api: apis.size, loc });
}

rows.sort((a, b) => b.loc - a.loc);
console.log("MODULE\t.c\t.h\tapi~\tloc");
for (const r of rows) console.log([r.name, r.cf, r.hf, r.api, r.loc].join("\t"));
const sum = (k) => rows.reduce((s, r) => s + r[k], 0);
console.log(["TOTAL", sum("cf"), sum("hf"), sum("api"), sum("loc")].join("\t"));
