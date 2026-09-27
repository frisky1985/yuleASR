// usage: node api-surface-diff.js <ourDir> <theirDir> — 对比两目录头文件中的 Module_Api 名称集合
const fs = require("fs");
const path = require("path");

function walk(d) {
  if (!fs.existsSync(d)) return [];
  const o = [];
  for (const e of fs.readdirSync(d, { withFileTypes: true })) {
    const f = path.join(d, e.name);
    if (e.isDirectory()) o.push(...walk(f));
    else o.push(f);
  }
  return o;
}

const re = /\b([A-Z][A-Za-z0-9_]*_[A-Za-z0-9_]+)\s*\(/g;
function names(dir) {
  const s = new Set();
  for (const f of walk(dir)) {
    if (!f.endsWith(".h")) continue;
    const t = fs.readFileSync(f, "utf8")
      .replace(/\/\*[\s\S]*?\*\//g, " ")
      .replace(/\/\/[^\n]*/g, " ")
      .replace(/^[ \t]*#.*$/gm, " ");
    for (const m of t.matchAll(re)) s.add(m[1]);
  }
  return s;
}

const A = names(process.argv[2]);
const B = names(process.argv[3]);
const only = [...B].filter((x) => !A.has(x)).sort();
console.log("theirs_only(" + only.length + "): " + only.slice(0, 60).join(" ") + (only.length > 60 ? " ..." : ""));
const rev = [...A].filter((x) => !B.has(x)).sort();
console.log("ours_only(" + rev.length + "): " + rev.slice(0, 40).join(" ") + (rev.length > 40 ? " ..." : ""));
