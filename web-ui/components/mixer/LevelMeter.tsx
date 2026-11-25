import { Box } from "@chakra-ui/react"
import { meterPercent } from "@/lib/mixer/controls"

export interface LevelMeterProps {
  label: string
  value: number
  height?: number
  width?: number
}

export function LevelMeter({
  label,
  value,
  height = 180,
  width = 10,
}: LevelMeterProps) {
  const percentage = meterPercent(value)
  const color = percentage >= 92 ? "app.danger" : percentage >= 72 ? "app.warning" : "app.success"

  return (
    <Box
      role="meter"
      aria-label={label}
      aria-valuemin={0}
      aria-valuemax={100}
      aria-valuenow={percentage}
      position="relative"
      w={width}
      h={height}
      flexShrink={0}
      overflow="hidden"
      borderRadius="full"
      bg="#0d0e10"
      borderWidth="1px"
      borderColor="app.border"
    >
      <Box
        position="absolute"
        insetInline="0"
        bottom="0"
        height={`${percentage}%`}
        bg={color}
        transition="height 70ms linear, background-color 120ms ease"
      />
      <Box
        position="absolute"
        inset="0"
        backgroundImage="repeating-linear-gradient(to bottom, transparent 0, transparent 5px, #0d0e10 5px, #0d0e10 7px)"
        pointerEvents="none"
      />
    </Box>
  )
}
