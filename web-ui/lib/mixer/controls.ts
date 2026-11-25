import { quantize } from "./model"

export function formatFrequency(frequency: number): string {
  return frequency >= 1000 ? `${frequency / 1000}k` : `${frequency}`
}

/** The firmware build exposes two physical inputs; the rest are mix channels. */
export const INPUT_CHANNEL_COUNT = 2

export function channelName(index: number): string {
  return index < INPUT_CHANNEL_COUNT ? `Input ${index + 1}` : `Mix ${index + 1}`
}

export function channelKind(index: number): "IN" | "MIX" {
  return index < INPUT_CHANNEL_COUNT ? "IN" : "MIX"
}

export function channelNumber(index: number): string {
  return String(index + 1).padStart(2, "0")
}

export function formatPercent(value: number): string {
  return `${Math.round(value * 100)}%`
}

export function meterPercent(value: number): number {
  if (!Number.isFinite(value)) return 0
  return Math.round(Math.min(1, Math.max(0, value)) * 100)
}

export function valueFromVerticalDrag(
  startValue: number,
  upwardDelta: number,
  min: number,
  max: number,
  step: number,
  sensitivity = 130,
): number {
  const range = max - min
  return quantize(startValue + (upwardDelta / sensitivity) * range, {
    min,
    max,
    step,
  })
}
