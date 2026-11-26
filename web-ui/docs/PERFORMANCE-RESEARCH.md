# Laporan Riset + Pecahan Kode: Lag & Rasa "Kaku" Slider/Knob Mixer

Tanggal: 26 November 2026. Satu file ini berisi narasi riset **dan** potongan kode yang relevan, supaya bisa dikirim ke GPT tanpa melampirkan file satu per satu.

---

## A. Ringkasan

Tiga masalah berbeda:

| Lapisan | Gejala | Penyebab | Status |
|---|---|---|---|
| Meter | Lag parah saat demo aktif | `setVu` tiap 80 ms render seluruh tree | Selesai, commit `43d0b46` |
| Interaksi | Slider/knob/tombol/navigasi lambat | Tiap `pointermove` dispatch ke reducer root → seluruh console render; callback inline menggagalkan `memo` | Selesai, commit `d09e2cd` |
| Drag | Tombol/animasi oke, slide masih kaku | Thumb digambar dari prop React + state internal Chakra Slider; koalesensi tambah ±1 frame | Prototype, belum di-commit |

Kesimpulan: **render cepat tetap terasa kaku bila thumb menunggu render.** Solusi terakhir: knob/fader menulis DOM langsung saat drag; React menyusul.

## B. Sumber Context7

```bash
npx ctx7@latest library react "avoiding unnecessary re-renders useCallback memo state colocation"
# → /reactjs/react.dev
npx ctx7@latest docs /reactjs/react.dev \
  "memo useCallback preventing re-renders when parent state changes and passing stable callbacks to memoized children"
npx ctx7@latest library "Chakra UI" "Slider controlled value onValueChange onValueChangeEnd"
# → /chakra-ui/chakra-ui, /websites/chakra-ui
npx ctx7@latest docs /websites/chakra-ui \
  "Slider Root controlled value onValueChange onValueChangeEnd thumb hidden input v3"
npx ctx7@latest docs /reactjs/react.dev \
  "useSyncExternalStore subscribe external store getSnapshot high frequency updates without re-rendering parent"
```

- React: `useCallback` menjaga identitas fungsi agar anak `memo` benar-benar skip; `getSnapshot` `useSyncExternalStore` stabil (`Object.is`).
- Chakra v3: Slider = `Root/Control/Track/Range/Thumb/HiddenInput`, `onValueChange`/`onValueChangeEnd` receives `{ value }`, value terkontrol → thumb bergantung commit React.

## C. Metodologi pengukuran

A/B: `git stash` → `npm run build` → jalankan script CDP → `git stash pop` → build lagi. Metrik utama: `MutationObserver` di dalam halaman, event → perubahan atribut DOM. Script di `%TEMP%\opencode\` (tidak di repo): `meter-check`, `perf-check`, `cpu-check`, `latency-check`, `drag-latency-check`, `live-drag-check`. `perf-check`/`cpu-check` terlalu berisik (idle ~3,4 s dari 4,4 s), jadi pelengkap.

## D. Data pengukuran

| Interaksi | Sebelum median (p95) | Sesudah median (p95) |
|---|---|---|
| Knob keydown | 67,7–84,6 ms (90,7–112,7) | 27,3–44,1 ms (40,2–68,2) |
| Tombol MUTE | 59,4–68,5 ms (84,4–93,7) | 19,4–24,2 ms (25,5–34,7) |
| Navigasi kanal | 87 ms (108,9) | 68,6 ms (92,1) |
| Drag knob (prop React) | 49,9 ms (67,8) | — |
| Drag knob (state lokal) | 34,8 ms (49,3) | — |
| **Drag knob (DOM langsung)** | — | **0,2–0,5 ms (0,9–1,5; max 2,3)** |
| **Drag fader (DOM langsung)** | — | **0,4–0,7 ms (0,8–3,4)** |

Bukti realtime sebelum `pointerup`:

| Objek | Sebelum | Saat drag | Sesudah lepas |
|---|---|---|---|
| Fader Input 1 | value `1`, meter `0.591` | value `0.28`, thumb `calc(28% - 7px)`, meter `0.214` | value `0.28`, meter `0.161` |
| Knob Group 1 | value `0` | value `0.55`, arc `41.25 100` | value `0.55` |

---

## E. Pecahan kode

### E1. `components/mixer/MixerApp.tsx` — meter di luar React + koalesensi aksi

```tsx
export function MixerApp() {
  const [state, dispatch] = useReducer(mixerReducer, undefined, createDefaultMixerState)
  const [dirty, setDirty] = useState(false)

  // Meter bridge: tidak ada nilai meter yang masuk ke state React.
  useEffect(() => { syncDemoSignal(state) }, [state])
  useEffect(() => { meterBridge.setPaused(demoPaused) }, [demoPaused])

  // Drag bisa mengirim >60 event/detik. Kumpulkan aksi kontinu per frame;
  // aksi diskrit tetap dispatch seketika agar tombol tidak terasa delay.
  const pending = useRef<MixerAction[]>([])
  const frame = useRef<number | null>(null)
  const flush = useCallback(() => {
    frame.current = null
    const actions = pending.current
    pending.current = []
    for (const action of actions) dispatch(action)
  }, [])

  const update = useCallback((action: MixerAction) => {
    setDirty(true)
    if (
      action.type === "toggle-channel-mute" ||
      action.type === "toggle-master-mute" ||
      action.type === "replace"
    ) {
      dispatch(action)
      return
    }
    pending.current.push(action)
    if (frame.current === null) frame.current = window.requestAnimationFrame(flush)
  }, [flush])

  useEffect(() => () => {
    if (frame.current !== null) window.cancelAnimationFrame(frame.current)
  }, [])
}
```

### E2. `components/mixer/EqPanel.tsx` — formatter + handler stabil

```tsx
const formatEqValue = (value: number) => `${value > 0 ? "+" : ""}${value.toFixed(1)}`

function EqPanelBase({ channel, channelIndex, dispatch, disabled = false }: EqPanelProps) {
  const handlers = useMemo(
    () => EQ_FREQUENCIES.map((_, band) => (value: number) =>
      dispatch({ type: "set-eq", channel: channelIndex, band, value })),
    [dispatch, channelIndex],
  )
  return (
    <SimpleGrid columns={{ base: 5, sm: 8, xl: 15 }} gap={{ base: 1, md: 2 }}>
      {EQ_FREQUENCIES.map((frequency, band) => (
        <Fader
          key={frequency}
          value={channel.eq[band]}
          min={-12} max={12} step={0.5} height={104}
          formatValue={formatEqValue}
          onChange={handlers[band]}
          /* label, caption, disabled dihilangkan untuk ringkas */
        />
      ))}
    </SimpleGrid>
  )
}
export const EqPanel = memo(EqPanelBase)
```

### E3. `components/mixer/SendsPanel.tsx` — handler stabil + formatter bersama

```tsx
function SendsPanelBase({ channel, channelIndex, dispatch, disabled = false }: SendsPanelProps) {
  const handlers = useMemo(() => ({
    sends: Object.fromEntries(
      sends.map(({ bus }) => [bus,
        (value: number) => dispatch({ type: "set-send", channel: channelIndex, bus, value })]),
    ) as Record<SendBus, (value: number) => void>,
    reverb: (value: number) => dispatch({ type: "set-reverb", channel: channelIndex, value }),
  }), [dispatch, channelIndex])

  return (
    <SimpleGrid columns={{ base: 2, sm: 3, lg: 5 }}>
      {sends.map(({ bus, label }) => (
        <RotaryKnob
          key={bus} value={channel.send[bus]} min={0} max={1} step={0.01}
          formatValue={formatPercent} onChange={handlers.sends[bus]}
        />
      ))}
      <RotaryKnob
        value={channel.reverb} min={0} max={1} step={0.01}
        formatValue={formatPercent} onChange={handlers.reverb}
      />
    </SimpleGrid>
  )
}
export const SendsPanel = memo(SendsPanelBase)
```

### E4. `components/mixer/DynamicsPanel.tsx`

```tsx
const handlers = useMemo(() => Object.fromEntries(
  parameters.map(({ key }) => [key, (value: number) => dispatch({
    type: "set-compressor", channel: channelIndex, parameter: key, value,
  })]),
) as Record<CompressorParameter, (value: number) => void>, [dispatch, channelIndex])
// ... onChange={handlers[parameter.key]}, formatValue={parameter.format} (stable, level modul)
```

### E5. `components/mixer/MixerConsole.tsx` — handler master/mute stabil

```tsx
const masterHandlers = useMemo(
  () => (["lrL", "lrR"] as const).reduce((handlers, parameter) => {
    handlers[parameter] = (value: number) =>
      dispatch({ type: "set-master", parameter, value })
    return handlers
  }, {} as Record<"lrL" | "lrR", (value: number) => void>),
  [dispatch],
)
const handleToggleMute = useCallback(
  (index: number) => dispatch({ type: "toggle-channel-mute", channel: index }), [dispatch])
const handleSend = useCallback(
  (index: number, value: number) =>
    dispatch({ type: "set-send", channel: index, bus: "lr", value }), [dispatch])
const handleToggleMasterMute = useCallback(
  () => dispatch({ type: "toggle-master-mute" }), [dispatch])
```

`ChannelInspector` juga dibungkus `memo(...)`.

### E6. `components/mixer/RotaryKnob.tsx` — inti perbaikan drag

```tsx
const drag = useRef<{ pointerId: number; startY: number; startValue: number; lastValue: number | null } | null>(null)
const rootRef = useRef<HTMLDivElement>(null)
const arcRef = useRef<SVGCircleElement>(null)
const dialRef = useRef<HTMLDivElement>(null)
const readoutRef = useRef<HTMLParagraphElement>(null)

const range = max - min
const percentage = range === 0 ? 0 : (value - min) / range
const angle = -135 + percentage * 270

// Pointer move menulis DOM langsung: tanpa state React, tanpa menunggu render.
const paint = (next: number) => {
  const pct = range === 0 ? 0 : (next - min) / range
  arcRef.current?.setAttribute("stroke-dasharray", `${pct * 75} 100`)
  dialRef.current?.style.setProperty("transform", `rotate(${-135 + pct * 270}deg)`)
  const text = formatValue(next)
  if (readoutRef.current) readoutRef.current.textContent = text
  rootRef.current?.setAttribute("aria-valuenow", String(next))
  rootRef.current?.setAttribute("aria-valuetext", text)
}

const update = (nextValue: number, commit = false) => {
  const quantized = quantize(nextValue, { min, max, step })
  if (drag.current) drag.current.lastValue = quantized
  paint(quantized)
  onChange(quantized)           // tetap realtime ke state mixer/meter
  if (commit) onChangeEnd?.(quantized)
}

const handlePointerDown = (event: PointerEvent<HTMLDivElement>) => {
  if (disabled) return
  drag.current = { pointerId: event.pointerId, startY: event.clientY, startValue: value, lastValue: null }
  event.currentTarget.setPointerCapture(event.pointerId)
  event.currentTarget.focus()
}

const handlePointerMove = (event: PointerEvent<HTMLDivElement>) => {
  if (!drag.current || drag.current.pointerId !== event.pointerId) return
  update(valueFromVerticalDrag(
    drag.current.startValue, drag.current.startY - event.clientY, min, max, step))
}

const finishDrag = (event: PointerEvent<HTMLDivElement>) => {
  if (!drag.current || drag.current.pointerId !== event.pointerId) return
  const committed = drag.current.lastValue
  drag.current = null
  if (committed !== null) onChangeEnd?.(committed)
}
```

JSX menjaga ARIA/focus/keyboard:

```tsx
<Box
  ref={rootRef} role="slider" tabIndex={disabled ? -1 : 0}
  aria-label={label} aria-valuemin={min} aria-valuemax={max}
  aria-valuenow={value} aria-valuetext={formatValue(value)}
  aria-disabled={disabled || undefined}
  touchAction="none" userSelect="none"
  _focusVisible={{ boxShadow: "0 0 0 2px app.accent", borderRadius: "full" }}
  onPointerDown={handlePointerDown} onPointerMove={handlePointerMove}
  onPointerUp={finishDrag} onPointerCancel={finishDrag}
  onKeyDown={handleKeyDown}
  onWheel={(event) => { if (disabled) return; event.preventDefault();
    update(value + (event.deltaY < 0 ? step : -step), true) }}
>
  <svg viewBox="0 0 48 48" ...>
    <circle ... strokeDasharray="75 25" transform="rotate(135 24 24)" />
    <circle ref={arcRef} ... strokeDasharray={`${percentage * 75} 100`} />
  </svg>
  <Box ref={dialRef} style={{ transform: `rotate(${angle}deg)` }} ... />
</Box>
```

### E7. `components/mixer/Fader.tsx` — Chakra Slider diganti slider ringan

```tsx
const paint = (next: number) => {
  const pct = range === 0 ? 0 : (next - min) / range
  const text = formatValue(next)
  rangeRef.current?.style.setProperty("height", `${pct * 100}%`)
  thumbRef.current?.style.setProperty("bottom", `calc(${pct * 100}% - 7px)`)
  if (readoutRef.current) readoutRef.current.textContent = text
  rootRef.current?.setAttribute("aria-valuenow", String(next))
  rootRef.current?.setAttribute("aria-valuetext", text)
}

const emit = (next: number, commit = false) => {
  if (drag.current) drag.current.last = next
  paint(next)                 // visual instan
  onChange(next)              // state mixer menyusul per frame
  if (commit) onChangeEnd?.(next)
}

// Bounds dibaca sekali saat pointerdown; membaca layout tiap move memicu reflow.
const valueAtPointer = (clientY: number, bounds: DOMRect | null) => {
  if (!bounds || bounds.height === 0) return value
  const ratio = Math.min(1, Math.max(0, (bounds.bottom - clientY) / bounds.height))
  return quantize(min + ratio * range, { min, max, step })
}

const handlePointerDown = (event: PointerEvent<HTMLDivElement>) => {
  if (disabled) return
  drag.current = {
    pointerId: event.pointerId,
    last: null,
    bounds: rootRef.current?.getBoundingClientRect() ?? null,
  }
  event.currentTarget.setPointerCapture(event.pointerId)
  event.currentTarget.focus()
  emit(valueAtPointer(event.clientY, drag.current.bounds))
}
const handlePointerMove = (event: PointerEvent<HTMLDivElement>) => {
  if (!drag.current || drag.current.pointerId !== event.pointerId) return
  emit(valueAtPointer(event.clientY))
}
```

JSX inti (tiga lapisan: track, range, thumb):

```tsx
<Box ref={rootRef} role="slider" tabIndex={disabled ? -1 : 0}
  aria-valuemin={min} aria-valuemax={max} aria-valuenow={value}
  aria-valuetext={formatValue(value)} touchAction="none"
  onPointerDown={handlePointerDown} onPointerMove={handlePointerMove}
  onPointerUp={finishDrag} onPointerCancel={finishDrag}
  onKeyDown={handleKeyDown}
  w="32px" h={`${height}px`} position="relative">
  <Box position="absolute" left="14px" top="0" bottom="0" w="4px" bg="app.border" pointerEvents="none" />
  <Box ref={rangeRef} position="absolute" left="14px" bottom="0" w="4px"
    h={`${percentage * 100}%`} bg="app.accent" pointerEvents="none" />
  <Box ref={thumbRef} position="absolute" left="9px"
    bottom={`calc(${percentage * 100}% - 7px)`} w="14px" h="14px"
    bg="app.surface" borderWidth="1px" borderColor="app.control" pointerEvents="none" />
</Box>
```

`components/ui/slider.tsx` (pembungkus Chakra Slider) **dihapus** karena tidak ada pemakai lain.

### E8. `lib/mixer/meter-bridge.ts` — pola yang jadi acuan (sudah di-commit)

```ts
const FRAME_MS = 1000 / 30
private tick = (now: number) => {
  this.frame = window.requestAnimationFrame(this.tick)
  if (now - this.lastTick < FRAME_MS) return
  this.lastTick = now
  const seconds = now / 1000
  for (const entry of this.subscriptions) {
    const level = clampLevel(entry.source(seconds))
    entry.fill.style.setProperty("--level", level.toFixed(3))
    entry.peak?.style.setProperty("--peak", entry.peakLevel.toFixed(3))
  }
}
```

---

## F. Trade-off & risiko

1. Visual ditulis DOM langsung; React tetap sumber kebenaran saat render. Aman bila rumus `paint()` identik dengan JSX (sudah demikian).
2. ARIA/focus/keyboard/wheel dipertahankan, tapi belum diuji screen reader sungguhan.
3. `setPointerCapture` dipakai; di benchmark headless distub. Perlu uji touch nyata.
4. Chakra Slider dihapus dari Fader → perlu cek regresi visual.
5. **Kuantisasi belum diubah** (EQ 0,5 dB, level 0,01). Rasa kaku bisa juga dari step kasar — hipotesis belum teruji; eksperimen preview halus + snap saat lepas akan mengubah nilai yang terlihat.
6. `npm test` belum diulang setelah rewrite terakhir; `typecheck`, `lint`, `build` hijau.

## G. Rekomendasi

1. Ulangi `npm test`; cek visual 1440/1024/820/390/320.
2. Uji keyboard/wheel/touch di Chrome nyata tanpa stub.
3. Uji hipotesis kuantisasi sebelum mengubah step.
4. Tambah tes komponen `Fader`/`RotaryKnob`.
5. Keputusan: state lokal saat drag **ditolak**; DOM langsung **diterima secara empiris**, menunggu persetujuan produk.

## H. Tanggapan terhadap review eksternal (GPT)

Review eksternal sebagian benar, sebagian terlalu umum, dan beberapa saran sudah kami terapkan.

### Yang benar dan sudah sesuai implementasi
- **Pointer Events** (`pointerdown/move/up`) sudah dipakai di `Fader` dan `RotaryKnob`, plus `setPointerCapture`.
- **ARIA APG** sudah ada: `role="slider"`, `tabIndex`, `aria-label`, `aria-valuemin/max/now/valueText`, `aria-disabled`, fokus keyboard.
- **Quantization** sudah ada lewat `quantize(value, { min, max, step })` di `lib/mixer/model.ts`.
- **React Profiler / Chrome Performance** memang disarankan; kami sudah memakai CPU profile + `MutationObserver` latency, belum `<Profiler>` (profiling build React tidak tersedia di production export biasa).

### Yang perlu dikoreksi
- **"Manipulasi DOM langsung kurang efisien karena selalu memicu reflow"** — terlalu menyederhanakan. Reflow dipicu oleh *layout read* (`getBoundingClientRect`) setelah write, bukan oleh write itu sendiri. Implementasi kami menulis `style.height`, `style.bottom`, `transform`, `stroke-dasharray`, dan atribut ARIA; itu di-batch browser. Hasil ukur terakhir: knob **0,2–0,5 ms**, fader **0,4 ms** median event→DOM (setelah bounds di-cache). Kami memang menemukan satu celah: `getBoundingClientRect()` awalnya dibaca tiap `pointermove` di Fader → **sudah diubah** dibaca sekali saat `pointerdown` dan disimpan di ref.
- **"Keys di reconciliation O(n)"** — tidak relevan dengan masalah ini; daftar strip/key sudah stabil dan tidak berubah saat drag.
- **Klaim bug Chakra v2 `thumbAlignment=center`** — proyek memakai v3; kami tidak lagi memakai Chakra Slider di Fader, jadi klaim v2 tidak berlaku.
- **"State kontrol idealnya tetap di React"** — benar untuk sebagian besar UI, tapi kasus ini adalah *high-frequency continuous input* (mouse 125–1000 Hz). Kami tetap menjaga React sebagai sumber kebenaran: `onChange` dipanggil tiap move dan di-flush per frame; DOM hanya dipakai untuk visual sementara antar-frame.
- **Zustand/Redux** — tidak perlu ditambahkan. Reducer + `useRef` koalesensi sudah memberi satu render per frame; menambah store akan menambah dependensi dan masalah seleksi tanpa manfaat terukur di aplikasi ini.

### Yang masih menjadi gap nyata
1. Belum ada tes otomatis untuk pointer/keyboard `Fader`/`RotaryKnob` (disarankan Vitest + Testing Library).
2. Belum ada eksperimen kuantisasi halus-saat-drag + snap-saat-lepas; rasa "kaku" bisa berasal dari step 0,5 dB / 0,01, bukan latensi.
3. Belum ada uji touch sungguhan (benar bahwa Pointer Events mencakup mouse/pen/touch, tapi `setPointerCapture` perlu diverifikasi di perangkat).
4. Profiler commit time belum diukur dengan React Profiler khusus.

Keputusan: tidak menambah Zustand/Redux; tidak menambah `<Profiler>` ke produksi; memperbaiki layout read di Fader (sudah). Langkah berikutnya yang disarankan: tes komponen + eksperimen kuantisasi.

## I. Status kerja

```
Mode demo sudah dihapus. Web UI sekarang memakai WebSocket nyata:
- lib/mixer/use-mixer-socket.ts  (hubung /ws, reconnect, kirim aksi "set")
- lib/mixer/vu-store.ts          (frame "vu" dari firmware → meter bridge)
- lib/use-device-status.ts       (/api/status untuk info perangkat)
- components/mixer/MixerApp.tsx  (status connected/connecting/offline, save via /api/save)
- demo-signal.ts                 dihapus

Verifikasi dengan WebSocket palsu: tidak ada teks "Mode demo", frame vu
langsung mengubah meter (0.75 / 0.60), drag mengirim aksi "set" dengan
8 channel, dan status menampilkan "Terhubung".
```

Catatan firmware: handler `set` di `main/web_server.cpp` belum membaca field
`mute`, jadi mute belum sampai ke perangkat. Perlu ditambah di firmware.
