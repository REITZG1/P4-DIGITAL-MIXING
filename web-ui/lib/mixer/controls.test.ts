import { describe, expect, it } from "vitest"
import { formatFrequency, meterPercent, valueFromVerticalDrag } from "./controls"

describe("formatFrequency", () => {
  it("uses compact units for mixer labels", () => {
    expect(formatFrequency(31)).toBe("31")
    expect(formatFrequency(1000)).toBe("1k")
    expect(formatFrequency(18000)).toBe("18k")
  })
})

describe("meterPercent", () => {
  it("clamps meter values to a visible percentage", () => {
    expect(meterPercent(-0.2)).toBe(0)
    expect(meterPercent(0.42)).toBe(42)
    expect(meterPercent(1.4)).toBe(100)
  })
})

describe("valueFromVerticalDrag", () => {
  it("maps upward movement to an increase and quantizes the result", () => {
    expect(valueFromVerticalDrag(0.5, 26, 0, 1, 0.01, 130)).toBe(0.7)
  })

  it("clamps movement at the parameter limits", () => {
    expect(valueFromVerticalDrag(0.9, 200, 0, 1, 0.01, 130)).toBe(1)
    expect(valueFromVerticalDrag(0.1, -200, 0, 1, 0.01, 130)).toBe(0)
  })
})
