import { describe, expect, it } from "vitest"
import {
  EQ_FREQUENCIES,
  createDefaultMixerState,
  mixerReducer,
  serializeMixerState,
  type MixerAction,
} from "./model"

describe("createDefaultMixerState", () => {
  it("creates eight channels with the firmware defaults", () => {
    const state = createDefaultMixerState()

    expect(state.channels).toHaveLength(8)
    expect(EQ_FREQUENCIES).toHaveLength(15)
    expect(state.channels[0].eq).toEqual(Array(15).fill(0))
    expect(state.channels[0].send.lr).toBe(1)
    expect(state.channels[2].send.lr).toBe(0.8)
    expect(state.channels[0].comp).toEqual({
      ratio: 2,
      attack: 10,
      release: 200,
      threshold: -20,
      gain: 0,
    })
    expect(state.master.lrL).toBe(0.8)
    expect(state.master.lrR).toBe(0.8)
    expect(state.masterMute).toBe(false)
  })
})

describe("mixerReducer", () => {
  it("updates one EQ band without mutating the previous state", () => {
    const state = createDefaultMixerState()
    const next = mixerReducer(state, {
      type: "set-eq",
      channel: 0,
      band: 3,
      value: 4.5,
    })

    expect(next.channels[0].eq[3]).toBe(4.5)
    expect(state.channels[0].eq[3]).toBe(0)
    expect(next.channels[1]).toBe(state.channels[1])
  })

  it("clamps and quantizes values to the parameter range", () => {
    const state = createDefaultMixerState()
    const next = mixerReducer(state, {
      type: "set-eq",
      channel: 0,
      band: 0,
      value: 99,
    })

    expect(next.channels[0].eq[0]).toBe(12)
  })

  it("updates sends, compressor values, master levels, and mute", () => {
    const state = createDefaultMixerState()
    const actions: MixerAction[] = [
      { type: "set-send", channel: 1, bus: "aux1", value: 0.45 },
      { type: "set-compressor", channel: 1, parameter: "ratio", value: 3.4 },
      { type: "set-master", parameter: "lrL", value: 0.65 },
      { type: "toggle-channel-mute", channel: 1 },
      { type: "toggle-master-mute" },
    ]
    const next = actions.reduce(mixerReducer, state)

    expect(next.channels[1].send.aux1).toBe(0.45)
    expect(next.channels[1].comp.ratio).toBe(3.4)
    expect(next.master.lrL).toBe(0.65)
    expect(next.channels[1].mute).toBe(true)
    expect(next.masterMute).toBe(true)
  })

  it("ignores invalid channel and band indexes", () => {
    const state = createDefaultMixerState()

    expect(
      mixerReducer(state, { type: "set-eq", channel: 99, band: 0, value: 1 }),
    ).toBe(state)
    expect(
      mixerReducer(state, { type: "set-eq", channel: 0, band: 99, value: 1 }),
    ).toBe(state)
  })
})

describe("serializeMixerState", () => {
  it("matches the firmware WebSocket set payload", () => {
    const state = createDefaultMixerState()
    const payload = serializeMixerState(state)

    expect(payload.type).toBe("set")
    expect(payload.master.lrL).toBe(0.8)
    expect(payload.channels).toHaveLength(8)
    expect(payload.channels[0].eq).toHaveLength(15)
    expect(payload.channels[0].mute).toBe(false)
    expect(payload.mute).toBe(false)
  })
})
