export type MeterSource = (timeSeconds: number) => number

interface MeterSubscription {
  fill: HTMLElement
  peak: HTMLElement | null
  root: HTMLElement
  source: MeterSource
  level: number
  peakLevel: number
  lastAria: number
}

const FRAME_MS = 1000 / 30
const PEAK_FALL = 0.0055
const ARIA_INTERVAL = 900

function clampLevel(value: number): number {
  if (!Number.isFinite(value)) return 0
  return Math.min(1, Math.max(0, value))
}

/**
 * Drives every level meter outside React.
 *
 * Meters update 30 times a second. Keeping that in component state meant the
 * whole console re-rendered on every tick, which is what made the demo mode
 * crawl. Here each meter writes two custom properties to its own DOM node, so
 * a live meter costs no render at all.
 */
class MeterBridge {
  private subscriptions = new Set<MeterSubscription>()
  private frame: number | null = null
  private lastTick = 0
  private paused = false
  private listening = false

  attach(subscription: Omit<MeterSubscription, "level" | "peakLevel" | "lastAria">) {
    const entry: MeterSubscription = {
      ...subscription,
      level: 0,
      peakLevel: 0,
      lastAria: 0,
    }
    this.subscriptions.add(entry)
    this.sync()
    return () => {
      this.subscriptions.delete(entry)
      this.sync()
    }
  }

  setPaused(paused: boolean) {
    this.paused = paused
    // Leave the last painted levels in place: pausing a meter freezes the
    // reading, it does not mute the signal.
    this.sync()
  }

  private paint(entry: MeterSubscription) {
    entry.fill.style.setProperty("--level", entry.level.toFixed(3))
    entry.peak?.style.setProperty("--peak", entry.peakLevel.toFixed(3))
  }

  private sync() {
    if (typeof document === "undefined") return
    const shouldRun =
      !this.paused &&
      this.subscriptions.size > 0 &&
      document.visibilityState === "visible"

    if (shouldRun && this.frame === null) {
      this.listen()
      this.lastTick = 0
      this.frame = window.requestAnimationFrame(this.tick)
    } else if (!shouldRun && this.frame !== null) {
      window.cancelAnimationFrame(this.frame)
      this.frame = null
    }
  }

  private handleVisibility = () => {
    this.sync()
  }

  private listen() {
    if (this.listening || typeof document === "undefined") return
    this.listening = true
    document.addEventListener("visibilitychange", this.handleVisibility)
  }

  private tick = (now: number) => {
    if (this.paused || this.subscriptions.size === 0) {
      this.frame = null
      return
    }
    this.frame = window.requestAnimationFrame(this.tick)

    if (now - this.lastTick < FRAME_MS) return
    this.lastTick = now
    const seconds = now / 1000

    for (const entry of this.subscriptions) {
      const level = clampLevel(entry.source(seconds))
      const peak =
        level >= entry.peakLevel
          ? level
          : Math.max(0, entry.peakLevel - PEAK_FALL)

      const levelMoved = Math.abs(level - entry.level) > 0.002
      const peakMoved = Math.abs(peak - entry.peakLevel) > 0.002
      if (levelMoved || peakMoved) {
        entry.level = level
        entry.peakLevel = peak
        this.paint(entry)
      }

      if (now - entry.lastAria > ARIA_INTERVAL) {
        entry.lastAria = now
        entry.root.setAttribute("aria-valuenow", String(Math.round(level * 100)))
      }
    }
  }
}

export const meterBridge = new MeterBridge()
