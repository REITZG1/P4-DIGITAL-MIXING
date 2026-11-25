"use client"

import { SimpleGrid } from "@chakra-ui/react"
import { memo, useMemo } from "react"
import { RotaryKnob } from "./RotaryKnob"
import { formatPercent } from "@/lib/mixer/controls"
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

function SendsPanelBase({ channel, channelIndex, dispatch, disabled = false }: SendsPanelProps) {
  const handlers = useMemo(
    () => ({
      sends: Object.fromEntries(
        sends.map(({ bus }) => [
          bus,
          (value: number) => dispatch({ type: "set-send", channel: channelIndex, bus, value }),
        ]),
      ) as Record<SendBus, (value: number) => void>,
      reverb: (value: number) =>
        dispatch({ type: "set-reverb", channel: channelIndex, value }),
    }),
    [dispatch, channelIndex],
  )

  return (
    <SimpleGrid columns={{ base: 2, sm: 3, lg: 5 }} gap={{ base: 4, md: 6 }} justifyItems="center">
      {sends.map(({ bus, label }) => (
        <RotaryKnob
          key={bus}
          label={label}
          value={channel.send[bus]}
          min={0}
          max={1}
          step={0.01}
          disabled={disabled}
          formatValue={formatPercent}
          onChange={handlers.sends[bus]}
        />
      ))}
      <RotaryKnob
        label="Reverb"
        value={channel.reverb}
        min={0}
        max={1}
        step={0.01}
        disabled={disabled}
        formatValue={formatPercent}
        onChange={handlers.reverb}
      />
    </SimpleGrid>
  )
}

export const SendsPanel = memo(SendsPanelBase)
