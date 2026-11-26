import {
  cpSync,
  existsSync,
  mkdirSync,
  readFileSync,
  readdirSync,
  rmSync,
  statSync,
  writeFileSync,
} from "node:fs"
import { extname, join, relative, resolve } from "node:path"

const root = resolve(process.cwd())
const from = join(root, "build")
const to = join(root, "spiffs")

if (!existsSync(join(from, "index.html"))) {
  console.error("prepare-fs: jalankan `npm run build` dulu")
  process.exit(1)
}

rmSync(to, { recursive: true, force: true })
mkdirSync(to, { recursive: true })

for (const file of ["index.html", "404.html", "icon.svg", "share.svg"]) {
  if (existsSync(join(from, file))) cpSync(join(from, file), join(to, file))
}
cpSync(join(from, "_next"), join(to, "_next"), { recursive: true })

// ESP32 SPIFFS limits object names to 32 characters. Next.js emits hashed
// filenames that are much longer, so move only those files into /a/<n>.<ext>
// and rewrite every reference to the short absolute path.
const files = []
const walk = (dir) => {
  for (const entry of readdirSync(dir, { withFileTypes: true })) {
    const full = join(dir, entry.name)
    if (entry.isDirectory()) walk(full)
    else if (entry.isFile()) files.push(full)
  }
}
walk(to)

const rel = (file) => relative(to, file).split("\\").join("/")
const longFiles = files.filter((file) => rel(file).length + 1 > 30)
const pairs = longFiles.map((file, index) => ({
  file,
  oldRel: rel(file),
  newRel: `a/${index}${extname(file)}`,
}))

if (pairs.length) {
  const byOld = new Map(pairs.map((pair) => [pair.oldRel, pair.newRel]))
  const sorted = [...pairs].sort((a, b) => b.oldRel.length - a.oldRel.length)

  const rewrite = (text) => {
    for (const { oldRel, newRel } of sorted) {
      const variants = [
        `./${oldRel}`,
        `/${oldRel}`,
        oldRel,
        oldRel.replace(/^_next\//, ""),
      ]
      for (const variant of new Set(variants)) {
        const target = variant.startsWith("./") ? `./${newRel}` : `/${newRel}`
        text = text.split(variant).join(target)
      }
    }

    // Webpack builds lazy chunk URLs at runtime from a hash table.
    const marker = '"static/chunks/"+e+"."+({'
    const start = text.indexOf(marker)
    const end = start >= 0 ? text.indexOf('}[e]+".js"', start) : -1
    if (start >= 0 && end >= 0) {
      const expression = text.slice(start, end + '[e]+".js"'.length)
      const entries = [...expression.matchAll(/(\d+):"([a-f0-9]+)"/g)]
      const mapping = entries
        .map((match) => {
          const oldFile = `_next/static/chunks/${match[1]}.${match[2]}.js`
          const short = byOld.get(oldFile)
          return short ? `${match[1]}:"${short}"` : null
        })
        .filter(Boolean)
        .join(",")
      if (mapping) text = text.replace(expression, `"/"+({${mapping}})[e]`)
    }
    return text
  }

  for (const file of files) {
    if (!/\.(html|js|css|json|txt|svg)$/.test(file)) continue
    const original = readFileSync(file, "utf8")
    const updated = rewrite(original)
    if (updated !== original) writeFileSync(file, updated)
  }

  mkdirSync(join(to, "a"), { recursive: true })
  for (const { file, newRel } of pairs) {
    cpSync(file, join(to, newRel))
    rmSync(file)
  }
  for (const dir of ["_next/static/chunks", "_next/static/css", "_next/static/media"]) {
    const target = join(to, dir)
    if (existsSync(target) && !readdirSync(target).some((name) => statSync(join(target, name)).isFile())) {
      rmSync(target, { recursive: true, force: true })
    }
  }
}

console.log(`prepare-fs: ${to} siap untuk SPIFFS (${pairs.length} path dipendekkan)`)
