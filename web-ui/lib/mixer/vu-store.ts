import {
  CHANNEL_COUNT,
  createEmptyVuState,
  type VuState,
} from "./model"
import type { MeterSource } from "./meter-bridge"

/**
 * Latest levels reported by the device over WebSocket.
 *
 * The meter bridge reads these functions every frame, so incoming VU frames
 * never cause a React render.
 */
export interface VuMessage {
  channels?: number[]
  bus?: Partial<VuState["bus"]>
}

let current: VuState = createEmptyVuState()

function clamp(value: number): number {
  if (!Number.isFinite(value)) return 0
  return Math.min(1, Math.max(0, value))
}

export function applyVuMessage(message: VuMessage) {
  if (Array.isArray(message.channels)) {
    current.channels = message.channels.map((value) => clamp(value))
  }
  const bus = message.bus
  if (bus) {
    for (const key of Object.keys(current.bus) as Array<keyof VuState["bus"]>) {
      const value = bus[key]
      if (typeof value === "number") current.bus[key] = clamp(value)
    }
  }
}

export function resetVu() {
  current = createEmptyVuState()
}

export const channelVuSources: MeterSource[] = Array.from(
  { length: CHANNEL_COUNT },
  (_, index) => () => current.channels[index] ?? 0,
)

export const masterVuSources = {
  left: (() => current.bus.masterL) as MeterSource,
  right: (() => current.bus.masterR) as MeterSource,
}

export const busVuSources = {
  group1: (() => current.bus.group1) as MeterSource,
  group2: (() => current.bus.group2) as MeterSource,
  aux1: (() => current.bus.aux1) as MeterSource,
  aux2: (() => current.bus.aux2) as MeterSource,
}
