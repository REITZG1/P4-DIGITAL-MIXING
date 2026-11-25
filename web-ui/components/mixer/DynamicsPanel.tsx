"use client"

import { SimpleGrid } from "@chakra-ui/react"
import { memo } from "react"
import { RotaryKnob } from "./RotaryKnob"
import type { CompressorParameter, MixerAction, MixChannel } from "@/lib/mixer/model"

export interface DynamicsPanelProps {
  channel: MixChannel
  channelIndex: number
  dispatch: React.Dispatch<MixerAction>
  disabled?: boolean
}

const parameters: Array<{
  key: CompressorParameter
  label: string
  min: number
  max: number
  step: number
  format: (value: number) => string
}> = [
  { key: "ratio", label: "Ratio", min: 1, max: 10, step: 0.1, format: (value) => `${value.toFixed(1)}:1` },
  { key: "attack", label: "Attack", min: 0, max: 100, step: 1, format: (value) => `${value.toFixed(0)} ms` },
  { key: "release", label: "Release", min: 10, max: 1000, step: 10, format: (value) => `${value.toFixed(0)} ms` },
  { key: "threshold", label: "Threshold", min: -60, max: 0, step: 1, format: (value) => `${value.toFixed(0)} dB` },
  { key: "gain", label: "Makeup", min: -12, max: 12, step: 0.5, format: (value) => `${value > 0 ? "+" : ""}${value.toFixed(1)} dB` },
]

function DynamicsPanelBase({ channel, channelIndex, dispatch, disabled = false }: DynamicsPanelProps) {
  return (
    <SimpleGrid columns={{ base: 2, sm: 3, lg: 5 }} gap={{ base: 4, md: 6 }} justifyItems="center">
      {parameters.map((parameter) => (
        <RotaryKnob
          key={parameter.key}
          label={parameter.label}
          value={channel.comp[parameter.key]}
          min={parameter.min}
          max={parameter.max}
          step={parameter.step}
          disabled={disabled}
          formatValue={parameter.format}
          onChange={(value) =>
            dispatch({
              type: "set-compressor",
              channel: channelIndex,
              parameter: parameter.key,
              value,
            })
          }
        />
      ))}
    </SimpleGrid>
  )
}

export const DynamicsPanel = memo(DynamicsPanelBase)
