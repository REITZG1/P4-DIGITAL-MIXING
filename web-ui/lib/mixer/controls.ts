import { quantize } from "./model"

export function formatFrequency(frequency: number): string {
  return frequency >= 1000 ? `${frequency / 1000}k` : `${frequency}`
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
