import { describe, expect, it } from "vitest"
import { formatFrequency, meterPercent, valueFromVerticalDrag, channelName, channelKind, channelNumber, formatPercent } from "./controls"

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

describe("channel labels", () => {
  it("names the two physical inputs and the remaining mix channels", () => {
    expect(channelName(0)).toBe("Input 1")
    expect(channelName(1)).toBe("Input 2")
    expect(channelName(2)).toBe("Mix 3")
    expect(channelName(7)).toBe("Mix 8")
  })

  it("keeps a two-digit strip number and short kind tag", () => {
    expect(channelNumber(0)).toBe("01")
    expect(channelNumber(7)).toBe("08")
    expect(channelKind(0)).toBe("IN")
    expect(channelKind(5)).toBe("MIX")
  })
})

describe("formatPercent", () => {
  it("rounds a linear level to a whole percentage", () => {
    expect(formatPercent(0)).toBe("0%")
    expect(formatPercent(0.804)).toBe("80%")
    expect(formatPercent(1)).toBe("100%")
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
