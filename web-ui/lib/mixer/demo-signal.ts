import { CHANNEL_COUNT, type MixerState } from "./model"
import type { MeterSource } from "./meter-bridge"

/**
 * Deterministic demo audio: stable function identities so meters never need to
 * re-subscribe, and levels derived from the real mixer state so muting a
 * channel, lowering a fader, or opening an aux send is visible immediately.
 */

interface DemoChannel {
  mute: boolean
  lr: number
  group1: number
  group2: number
  aux1: number
  aux2: number
}

let channels: DemoChannel[] = Array.from({ length: CHANNEL_COUNT }, () => ({
  mute: false,
  lr: 1,
  group1: 0,
  group2: 0,
  aux1: 0,
  aux2: 0,
}))

let master = { lrL: 0.8, lrR: 0.8, mute: false }

export function syncDemoSignal(state: MixerState) {
  channels = state.channels.map((channel) => ({
    mute: channel.mute,
    lr: channel.send.lr,
    group1: channel.send.group1,
    group2: channel.send.group2,
    aux1: channel.send.aux1,
    aux2: channel.send.aux2,
  }))
  master = {
    lrL: state.master.lrL,
    lrR: state.master.lrR,
    mute: state.masterMute,
  }
}

function programLevel(index: number, seconds: number): number {
  const slow = Math.sin(seconds * 0.83 + index * 1.9) * 0.17
  const body = Math.sin(seconds * 2.61 + index * 0.74) * 0.08
  const detail = Math.sin(seconds * 7.31 + index * 2.4) * 0.04
  const lift = index < 2 ? 0.06 : 0
  return Math.min(1, Math.max(0, 0.4 + lift + slow + body + detail))
}

function channelLevel(index: number, seconds: number): number {
  const channel = channels[index]
  if (!channel || channel.mute) return 0
  return programLevel(index, seconds) * channel.lr
}

function busLevel(
  bus: "lr" | "group1" | "group2" | "aux1" | "aux2",
  gain: number,
  seconds: number,
): number {
  let total = 0
  for (let index = 0; index < channels.length; index += 1) {
    const channel = channels[index]
    if (!channel || channel.mute) continue
    total += programLevel(index, seconds) * channel[bus]
  }
  return Math.min(1, (total / CHANNEL_COUNT) * gain)
}

export const channelMeterSources: MeterSource[] = Array.from(
  { length: CHANNEL_COUNT },
  (_, index) => (seconds: number) => channelLevel(index, seconds),
)

export const masterMeterSources = {
  left: ((seconds: number) =>
    master.mute ? 0 : busLevel("lr", master.lrL * 1.9, seconds)) as MeterSource,
  right: ((seconds: number) =>
    master.mute ? 0 : busLevel("lr", master.lrR * 1.9, seconds)) as MeterSource,
}

export const busMeterSources = {
  group1: ((seconds: number) =>
    busLevel("group1", 2.2, seconds)) as MeterSource,
  group2: ((seconds: number) =>
    busLevel("group2", 2.2, seconds)) as MeterSource,
  aux1: ((seconds: number) => busLevel("aux1", 2.4, seconds)) as MeterSource,
  aux2: ((seconds: number) => busLevel("aux2", 2.4, seconds)) as MeterSource,
}
