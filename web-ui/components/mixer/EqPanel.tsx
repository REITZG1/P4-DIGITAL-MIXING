"use client"

import { SimpleGrid } from "@chakra-ui/react"
import { memo, useMemo } from "react"
import { Fader } from "./Fader"
import { formatFrequency } from "@/lib/mixer/controls"
import { EQ_FREQUENCIES, type MixerAction, type MixChannel } from "@/lib/mixer/model"

export interface EqPanelProps {
  channel: MixChannel
  channelIndex: number
  dispatch: React.Dispatch<MixerAction>
  disabled?: boolean
}

const formatEqValue = (value: number) => `${value > 0 ? "+" : ""}${value.toFixed(1)}`

function EqPanelBase({ channel, channelIndex, dispatch, disabled = false }: EqPanelProps) {
  // Stable callback identities let memo(Fader) skip the other 14 bands when
  // only one value changes.
  const handlers = useMemo(
    () =>
      EQ_FREQUENCIES.map((_, band) => (value: number) =>
        dispatch({ type: "set-eq", channel: channelIndex, band, value }),
      ),
    [dispatch, channelIndex],
  )

  return (
    <SimpleGrid columns={{ base: 5, sm: 8, xl: 15 }} gap={{ base: 1, md: 2 }} alignItems="end">
      {EQ_FREQUENCIES.map((frequency, band) => (
        <Fader
          key={frequency}
          label={`${formatFrequency(frequency)} Hz band`}
          caption={formatFrequency(frequency)}
          value={channel.eq[band]}
          min={-12}
          max={12}
          step={0.5}
          height={104}
          disabled={disabled}
          formatValue={formatEqValue}
          onChange={handlers[band]}
        />
      ))}
    </SimpleGrid>
  )
}

export const EqPanel = memo(EqPanelBase)
