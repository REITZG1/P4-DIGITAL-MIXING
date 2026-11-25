"use client"

import { Box, Text, VStack } from "@chakra-ui/react"
import { useRef, type CSSProperties, type KeyboardEvent, type PointerEvent } from "react"
import { quantize } from "@/lib/mixer/model"
import { valueFromVerticalDrag } from "@/lib/mixer/controls"

export interface RotaryKnobProps {
  label: string
  value: number
  min: number
  max: number
  step: number
  onChange: (value: number) => void
  onChangeEnd?: (value: number) => void
  disabled?: boolean
  formatValue?: (value: number) => string
}

export function RotaryKnob({
  label,
  value,
  min,
  max,
  step,
  onChange,
  onChangeEnd,
  disabled = false,
  formatValue = (nextValue) => nextValue.toFixed(2),
}: RotaryKnobProps) {
  const drag = useRef<{ pointerId: number; startY: number; startValue: number } | null>(null)
  const range = max - min
  const percentage = range === 0 ? 0 : (value - min) / range
  const angle = -135 + percentage * 270

  const update = (nextValue: number, commit = false) => {
    const quantized = quantize(nextValue, { min, max, step })
    onChange(quantized)
    if (commit) onChangeEnd?.(quantized)
  }

  const handlePointerDown = (event: PointerEvent<HTMLDivElement>) => {
    if (disabled) return
    drag.current = {
      pointerId: event.pointerId,
      startY: event.clientY,
      startValue: value,
    }
    event.currentTarget.setPointerCapture(event.pointerId)
    event.currentTarget.focus()
  }

  const handlePointerMove = (event: PointerEvent<HTMLDivElement>) => {
    if (!drag.current || drag.current.pointerId !== event.pointerId) return
    update(
      valueFromVerticalDrag(
        drag.current.startValue,
        drag.current.startY - event.clientY,
        min,
        max,
        step,
      ),
    )
  }

  const finishDrag = (event: PointerEvent<HTMLDivElement>) => {
    if (!drag.current || drag.current.pointerId !== event.pointerId) return
    drag.current = null
    onChangeEnd?.(value)
  }

  const handleKeyDown = (event: KeyboardEvent<HTMLDivElement>) => {
    if (disabled) return
    let nextValue: number | undefined
    if (event.key === "ArrowUp" || event.key === "ArrowRight") nextValue = value + step
    if (event.key === "ArrowDown" || event.key === "ArrowLeft") nextValue = value - step
    if (event.key === "PageUp") nextValue = value + step * 10
    if (event.key === "PageDown") nextValue = value - step * 10
    if (event.key === "Home") nextValue = min
    if (event.key === "End") nextValue = max
    if (nextValue === undefined) return
    event.preventDefault()
    update(nextValue, true)
  }

  return (
    <VStack gap="1.5" minW="64px">
      <Box
        as="div"
        role="slider"
        tabIndex={disabled ? -1 : 0}
        aria-label={label}
        aria-valuemin={min}
        aria-valuemax={max}
        aria-valuenow={value}
        aria-valuetext={formatValue(value)}
        aria-disabled={disabled}
        position="relative"
        boxSize="48px"
        cursor={disabled ? "not-allowed" : "ns-resize"}
        touchAction="none"
        userSelect="none"
        opacity={disabled ? 0.45 : 1}
        outline="none"
        _focusVisible={{ boxShadow: "0 0 0 2px app.accent", borderRadius: "full" }}
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={finishDrag}
        onPointerCancel={finishDrag}
        onKeyDown={handleKeyDown}
        onWheel={(event) => {
          if (disabled) return
          event.preventDefault()
          update(value + (event.deltaY < 0 ? step : -step), true)
        }}
      >
        <svg viewBox="0 0 48 48" width="48" height="48" aria-hidden="true">
          <circle
            cx="24"
            cy="24"
            r="21"
            fill="none"
            stroke="#34363a"
            strokeWidth="3"
            pathLength="100"
            strokeDasharray="75 25"
            transform="rotate(135 24 24)"
          />
          <circle
            cx="24"
            cy="24"
            r="21"
            fill="none"
            stroke="#ff4a1f"
            strokeWidth="3"
            strokeLinecap="round"
            pathLength="100"
            strokeDasharray={`${percentage * 75} 100`}
            transform="rotate(135 24 24)"
          />
        </svg>
        <Box
          position="absolute"
          inset="7px"
          borderRadius="full"
          bg="app.raised"
          borderWidth="1px"
          borderColor="app.border"
          style={{ transform: `rotate(${angle}deg)` } as CSSProperties}
        >
          <Box
            position="absolute"
            top="3px"
            left="50%"
            w="2px"
            h="9px"
            borderRadius="full"
            bg="app.accent"
            transform="translateX(-50%)"
          />
        </Box>
      </Box>
      <Text fontSize="xs" color="app.muted" textAlign="center">
        {label}
      </Text>
      <Text fontFamily="mono" fontSize="xs" color="app.ink">
        {formatValue(value)}
      </Text>
    </VStack>
  )
}
