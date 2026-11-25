"use client"

import { Box, Text, VStack } from "@chakra-ui/react"
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
}

export function Fader({
  label,
  value,
  min,
  max,
  step,
  onChange,
  onChangeEnd,
  disabled = false,
  height = 190,
  formatValue = (nextValue) => nextValue.toFixed(2),
}: FaderProps) {
  return (
    <VStack gap="2" minW="44px" w="full" alignItems="center">
      <Text
        fontFamily="mono"
        fontSize="xs"
        color="app.ink"
        bg="app.bg"
        borderWidth="1px"
        borderColor="app.border"
        borderRadius="md"
        px="2"
        py="1"
        minW="12"
        textAlign="center"
      >
        {formatValue(value)}
      </Text>
      <Box h={height} minW="44px" display="flex" justifyContent="center">
        <Slider
          orientation="vertical"
          value={[value]}
          min={min}
          max={max}
          step={step}
          disabled={disabled}
          aria-label={label}
          colorPalette="orange"
          size="lg"
          h={height}
          minW="44px"
          onValueChange={(details) => onChange(details.value[0])}
          onValueChangeEnd={(details) => onChangeEnd?.(details.value[0])}
          css={{
            "--slider-thumb-size": "18px",
            '& [data-part="control"]': { height: `${height}px` },
            '& [data-part="track"]': { width: "6px", bg: "app.raised" },
          }}
        />
      </Box>
      <Text fontSize="xs" color="app.muted" textAlign="center">
        {label}
      </Text>
    </VStack>
  )
}
