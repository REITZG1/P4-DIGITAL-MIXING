"use client"

import { SimpleGrid } from "@chakra-ui/react"
import { Fader } from "./Fader"
import { formatFrequency } from "@/lib/mixer/controls"
import { EQ_FREQUENCIES, type MixerAction, type MixChannel } from "@/lib/mixer/model"

export interface EqPanelProps {
  channel: MixChannel
  channelIndex: number
  dispatch: React.Dispatch<MixerAction>
  disabled?: boolean
}

export function EqPanel({
  channel,
  channelIndex,
  dispatch,
  disabled = false,
}: EqPanelProps) {
  return (
    <SimpleGrid columns={{ base: 5, sm: 8, xl: 15 }} gap="3" alignItems="end">
      {EQ_FREQUENCIES.map((frequency, band) => (
        <Fader
          key={frequency}
          label={formatFrequency(frequency)}
          value={channel.eq[band]}
          min={-12}
          max={12}
          step={0.5}
          height={112}
          disabled={disabled}
          formatValue={(value) => `${value > 0 ? "+" : ""}${value.toFixed(1)}`}
          onChange={(value) =>
            dispatch({ type: "set-eq", channel: channelIndex, band, value })
          }
        />
      ))}
    </SimpleGrid>
  )
}
