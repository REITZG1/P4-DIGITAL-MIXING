"use client"

import { SimpleGrid } from "@chakra-ui/react"
import { RotaryKnob } from "./RotaryKnob"
import type { MixerAction, MixChannel, SendBus } from "@/lib/mixer/model"

export interface SendsPanelProps {
  channel: MixChannel
  channelIndex: number
  dispatch: React.Dispatch<MixerAction>
  disabled?: boolean
}

const sends: Array<{ bus: SendBus; label: string }> = [
  { bus: "group1", label: "Group 1" },
  { bus: "group2", label: "Group 2" },
  { bus: "aux1", label: "Aux 1" },
  { bus: "aux2", label: "Aux 2" },
]

export function SendsPanel({
  channel,
  channelIndex,
  dispatch,
  disabled = false,
}: SendsPanelProps) {
  return (
    <SimpleGrid columns={{ base: 2, sm: 3, lg: 5 }} gap="5">
      {sends.map(({ bus, label }) => (
        <RotaryKnob
          key={bus}
          label={label}
          value={channel.send[bus]}
          min={0}
          max={1}
          step={0.01}
          disabled={disabled}
          formatValue={(value) => `${Math.round(value * 100)}%`}
          onChange={(value) =>
            dispatch({ type: "set-send", channel: channelIndex, bus, value })
          }
        />
      ))}
      <RotaryKnob
        label="Reverb"
        value={channel.reverb}
        min={0}
        max={1}
        step={0.01}
        disabled={disabled}
        formatValue={(value) => `${Math.round(value * 100)}%`}
        onChange={(value) =>
          dispatch({ type: "set-reverb", channel: channelIndex, value })
        }
      />
    </SimpleGrid>
  )
}
