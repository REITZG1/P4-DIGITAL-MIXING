"use client"

import { Box, Text, VStack } from "@chakra-ui/react"
import { memo } from "react"
import { Slider } from "@/components/ui/slider"

export interface FaderProps {
  label: string
  value: number
  min: number
  max: number
  step: number
  onChange: (value: number) => void
  onChangeEnd?: (value: number) => void
  disabled?: boolean
  height?: number
  formatValue?: (value: number) => string
  caption?: string
}

function FaderBase({
  label,
  value,
  min,
  max,
  step,
  onChange,
  onChangeEnd,
  disabled = false,
  height = 132,
  formatValue = (nextValue) => nextValue.toFixed(2),
  caption,
}: FaderProps) {
  return (
    <VStack gap="2" w="full" minW="0" alignItems="center">
      <Text
        fontFamily="mono"
        fontSize="11px"
        fontWeight="500"
        color="app.ink"
        className="tabular"
        py="1px"
        px="1"
        minW="10"
        lineHeight="16px"
        textAlign="center"
      >
        {formatValue(value)}
      </Text>

      <Box h={`${height}px`} w="32px" display="flex" justifyContent="center">
        <Slider
          orientation="vertical"
          value={[value]}
          min={min}
          max={max}
          step={step}
          disabled={disabled}
          aria-label={label}
          size="md"
          h={`${height}px`}
          onValueChange={(details) => onChange(details.value[0])}
          onValueChangeEnd={(details) => onChangeEnd?.(details.value[0])}
          css={{
            "--slider-thumb-size": "14px",
            height: `${height}px`,
            '& [data-part="control"]': { height: `${height}px`, width: "32px" },
            '& [data-part="track"]': {
              width: "4px",
              borderRadius: "2px",
              background: "var(--chakra-colors-app-border)",
            },
            '& [data-part="range"]': { background: "var(--chakra-colors-app-accent)" },
            '& [data-part="thumb"]': {
              border: "1px solid var(--chakra-colors-app-control)",
              background: "var(--chakra-colors-app-surface)",
              borderRadius: "3px",
              boxShadow: "0 1px 2px rgba(15, 23, 42, 0.18)",
            },
            '& [data-part="thumb"]:focus-visible': {
              outline: "2px solid var(--chakra-colors-app-accent)",
              outlineOffset: "1px",
            },
          }}
        />
      </Box>

      <Text
        fontSize="10px"
        fontWeight="500"
        letterSpacing="0.04em"
        color="app.faint"
        textAlign="center"
        lineHeight="14px"
      >
        {caption ?? label}
      </Text>
    </VStack>
  )
}

export const Fader = memo(FaderBase)
