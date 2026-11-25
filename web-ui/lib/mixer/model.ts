export const EQ_FREQUENCIES = [
  31, 63, 125, 250, 500, 1000, 2000, 4000, 6000, 8000, 10000, 12000, 14000,
  16000, 18000,
] as const

export const CHANNEL_COUNT = 8
export const EQ_BAND_COUNT = EQ_FREQUENCIES.length

export type SendBus = "lr" | "group1" | "group2" | "aux1" | "aux2"
export type MasterParameter = "lrL" | "lrR" | "group1" | "group2" | "aux1" | "aux2"
export type CompressorParameter = "ratio" | "attack" | "release" | "threshold" | "gain"

export interface CompressorParams {
  ratio: number
  attack: number
  release: number
  threshold: number
  gain: number
}

export interface ChannelSends {
  lr: number
  group1: number
  group2: number
  aux1: number
  aux2: number
}

export interface MixChannel {
  eq: number[]
  send: ChannelSends
  reverb: number
  comp: CompressorParams
  mute: boolean
}

export interface MasterState {
  lrL: number
  lrR: number
  group1: number
  group2: number
  aux1: number
  aux2: number
}

export interface MixerState {
  master: MasterState
  masterMute: boolean
  channels: MixChannel[]
}

export interface VuState {
  channels: number[]
  bus: {
    masterL: number
    masterR: number
    group1: number
    group2: number
    aux1: number
    aux2: number
  }
}

export function createEmptyVuState(): VuState {
  return {
    channels: Array(CHANNEL_COUNT).fill(0),
    bus: {
      masterL: 0,
      masterR: 0,
      group1: 0,
      group2: 0,
      aux1: 0,
      aux2: 0,
    },
  }
}

export type MixerAction =
  | { type: "set-eq"; channel: number; band: number; value: number }
  | { type: "set-send"; channel: number; bus: SendBus; value: number }
  | { type: "set-reverb"; channel: number; value: number }
  | { type: "set-compressor"; channel: number; parameter: CompressorParameter; value: number }
  | { type: "set-master"; parameter: MasterParameter; value: number }
  | { type: "toggle-channel-mute"; channel: number }
  | { type: "toggle-master-mute" }
  | { type: "replace"; state: MixerState }

interface ParameterRule {
  min: number
  max: number
  step: number
}

const LEVEL_RULE: ParameterRule = { min: 0, max: 1, step: 0.01 }
const EQ_RULE: ParameterRule = { min: -12, max: 12, step: 0.5 }

const COMPRESSOR_RULES: Record<CompressorParameter, ParameterRule> = {
  ratio: { min: 1, max: 10, step: 0.1 },
  attack: { min: 0, max: 100, step: 1 },
  release: { min: 10, max: 1000, step: 10 },
  threshold: { min: -60, max: 0, step: 1 },
  gain: { min: -12, max: 12, step: 0.5 },
}

export function quantize(value: number, rule: ParameterRule): number {
  if (!Number.isFinite(value)) return rule.min
  const clamped = Math.min(rule.max, Math.max(rule.min, value))
  const stepped = Math.round(clamped / rule.step) * rule.step
  return Number(stepped.toFixed(4))
}

export function createDefaultMixerState(): MixerState {
  return {
    master: {
      lrL: 0.8,
      lrR: 0.8,
      group1: 0.8,
      group2: 0.8,
      aux1: 0.8,
      aux2: 0.8,
    },
    masterMute: false,
    channels: Array.from({ length: CHANNEL_COUNT }, (_, index) => ({
      eq: Array(EQ_BAND_COUNT).fill(0),
      send: {
        lr: index < 2 ? 1 : 0.8,
        group1: 0,
        group2: 0,
        aux1: 0,
        aux2: 0,
      },
      reverb: 0,
      comp: {
        ratio: 2,
        attack: 10,
        release: 200,
        threshold: -20,
        gain: 0,
      },
      mute: false,
    })),
  }
}

function updateChannel(
  state: MixerState,
  channelIndex: number,
  update: (channel: MixChannel) => MixChannel,
): MixerState {
  if (channelIndex < 0 || channelIndex >= state.channels.length) return state
  const channels = state.channels.slice()
  channels[channelIndex] = update(channels[channelIndex])
  return { ...state, channels }
}

export function mixerReducer(state: MixerState, action: MixerAction): MixerState {
  switch (action.type) {
    case "replace":
      return action.state
    case "set-eq":
      if (action.band < 0 || action.band >= EQ_BAND_COUNT) return state
      return updateChannel(state, action.channel, (channel) => {
        const eq = channel.eq.slice()
        eq[action.band] = quantize(action.value, EQ_RULE)
        return { ...channel, eq }
      })
    case "set-send":
      return updateChannel(state, action.channel, (channel) => ({
        ...channel,
        send: {
          ...channel.send,
          [action.bus]: quantize(action.value, LEVEL_RULE),
        },
      }))
    case "set-reverb":
      return updateChannel(state, action.channel, (channel) => ({
        ...channel,
        reverb: quantize(action.value, LEVEL_RULE),
      }))
    case "set-compressor":
      return updateChannel(state, action.channel, (channel) => ({
        ...channel,
        comp: {
          ...channel.comp,
          [action.parameter]: quantize(action.value, COMPRESSOR_RULES[action.parameter]),
        },
      }))
    case "set-master":
      return {
        ...state,
        master: {
          ...state.master,
          [action.parameter]: quantize(action.value, LEVEL_RULE),
        },
      }
    case "toggle-channel-mute":
      return updateChannel(state, action.channel, (channel) => ({
        ...channel,
        mute: !channel.mute,
      }))
    case "toggle-master-mute":
      return { ...state, masterMute: !state.masterMute }
    default:
      return state
  }
}

export function serializeMixerState(state: MixerState) {
  return {
    type: "set" as const,
    master: { ...state.master },
    mute: state.masterMute,
    channels: state.channels.map((channel) => ({
      eq: channel.eq.slice(),
      send: { ...channel.send },
      reverb: channel.reverb,
      comp: { ...channel.comp },
      mute: channel.mute,
    })),
  }
}
