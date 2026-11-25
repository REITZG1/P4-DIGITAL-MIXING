"use client"

import { Box, Text, VStack } from "@chakra-ui/react"
import { memo, useRef, type CSSProperties, type KeyboardEvent, type PointerEvent } from "react"
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
  size?: number
}

function RotaryKnobBase({
  label,
  value,
  min,
  max,
  step,
  onChange,
  onChangeEnd,
  disabled = false,
  formatValue = (nextValue) => nextValue.toFixed(2),
  size = 40,
}: RotaryKnobProps) {
  const drag = useRef<{
    pointerId: number
    startY: number
    startValue: number
    lastValue: number | null
  } | null>(null)
  const rootRef = useRef<HTMLDivElement>(null)
  const arcRef = useRef<SVGCircleElement>(null)
  const dialRef = useRef<HTMLDivElement>(null)
  const readoutRef = useRef<HTMLParagraphElement>(null)

  const range = max - min
  const percentage = range === 0 ? 0 : (value - min) / range
  const angle = -135 + percentage * 270

  // Pointer moves paint straight to the DOM. No React state is touched, so a
  // drag never waits for a render; the mixer state follows in its own frame.
  const paint = (next: number) => {
    const nextPercentage = range === 0 ? 0 : (next - min) / range
    arcRef.current?.setAttribute("stroke-dasharray", `${nextPercentage * 75} 100`)
    dialRef.current?.style.setProperty("transform", `rotate(${-135 + nextPercentage * 270}deg)`)
    const text = formatValue(next)
    if (readoutRef.current) readoutRef.current.textContent = text
    rootRef.current?.setAttribute("aria-valuenow", String(next))
    rootRef.current?.setAttribute("aria-valuetext", text)
  }

  const update = (nextValue: number, commit = false) => {
    const quantized = quantize(nextValue, { min, max, step })
    if (drag.current) drag.current.lastValue = quantized
    paint(quantized)
    onChange(quantized)
    if (commit) onChangeEnd?.(quantized)
  }

  const handlePointerDown = (event: PointerEvent<HTMLDivElement>) => {
    if (disabled) return
    drag.current = {
      pointerId: event.pointerId,
      startY: event.clientY,
      startValue: value,
      lastValue: null,
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
    const committed = drag.current.lastValue
    drag.current = null
    if (committed !== null) onChangeEnd?.(committed)
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
    <VStack gap="1" minW="0">
      <Box
        ref={rootRef}
        role="slider"
        tabIndex={disabled ? -1 : 0}
        aria-label={label}
        aria-valuemin={min}
        aria-valuemax={max}
        aria-valuenow={value}
        aria-valuetext={formatValue(value)}
        aria-disabled={disabled || undefined}
        position="relative"
        boxSize={`${size}px`}
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
        <svg viewBox="0 0 48 48" width={size} height={size} aria-hidden="true">
          <circle
            cx="24"
            cy="24"
            r="21"
            fill="none"
            stroke="var(--chakra-colors-app-border)"
            strokeWidth="3"
            pathLength="100"
            strokeDasharray="75 25"
            transform="rotate(135 24 24)"
          />
          <circle
            ref={arcRef}
            cx="24"
            cy="24"
            r="21"
            fill="none"
            stroke="var(--chakra-colors-app-accent)"
            strokeWidth="3"
            strokeLinecap="round"
            pathLength="100"
            strokeDasharray={`${percentage * 75} 100`}
            transform="rotate(135 24 24)"
          />
        </svg>
        <Box
          ref={dialRef}
          position="absolute"
          inset={`${size * 0.15}px`}
          borderRadius="full"
          bg="app.panel"
          borderWidth="1px"
          borderColor="app.borderStrong"
          style={{ transform: `rotate(${angle}deg)` } as CSSProperties}
        >
          <Box
            position="absolute"
            top="3px"
            left="50%"
            w="2px"
            h={`${size * 0.19}px`}
            borderRadius="1px"
            bg="app.accent"
            transform="translateX(-50%)"
          />
        </Box>
      </Box>

      <Text
        fontSize="10px"
        fontWeight="500"
        letterSpacing="0.04em"
        color="app.faint"
        textAlign="center"
        lineHeight="14px"
      >
        {label}
      </Text>
      <Text
        ref={readoutRef}
        fontFamily="mono"
        fontSize="11px"
        color="app.ink"
        className="tabular"
        lineHeight="16px"
      >
        {formatValue(value)}
      </Text>
    </VStack>
  )
}

export const RotaryKnob = memo(RotaryKnobBase)
