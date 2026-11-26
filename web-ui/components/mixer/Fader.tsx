"use client"

import { Box, Text, VStack } from "@chakra-ui/react"
import { memo, useRef, type KeyboardEvent, type PointerEvent } from "react"
import { quantize } from "@/lib/mixer/model"

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
  const drag = useRef<{
    pointerId: number
    last: number | null
    bounds: DOMRect | null
  } | null>(null)
  const rootRef = useRef<HTMLDivElement>(null)
  const rangeRef = useRef<HTMLDivElement>(null)
  const thumbRef = useRef<HTMLDivElement>(null)
  const readoutRef = useRef<HTMLParagraphElement>(null)

  const range = max - min
  const percentage = range === 0 ? 0 : (value - min) / range

  // A fader is a handful of DOM nodes. Writing to them directly on pointer
  // move keeps the thumb glued to the pointer with zero React renders.
  const paint = (next: number) => {
    const nextPercentage = range === 0 ? 0 : (next - min) / range
    const text = formatValue(next)
    rangeRef.current?.style.setProperty("height", `${nextPercentage * 100}%`)
    thumbRef.current?.style.setProperty(
      "bottom",
      `calc(${nextPercentage * 100}% - 7px)`,
    )
    if (readoutRef.current) readoutRef.current.textContent = text
    rootRef.current?.setAttribute("aria-valuenow", String(next))
    rootRef.current?.setAttribute("aria-valuetext", text)
  }

  const emit = (next: number, commit = false) => {
    if (drag.current) drag.current.last = next
    paint(next)
    onChange(next)
    if (commit) onChangeEnd?.(next)
  }

  // Bounds are read once on pointerdown. Reading layout on every move would
  // force a reflow right after each paint and undo the point of painting.
  const valueAtPointer = (clientY: number, bounds: DOMRect | null) => {
    if (!bounds || bounds.height === 0) return value
    const ratio = Math.min(1, Math.max(0, (bounds.bottom - clientY) / bounds.height))
    return quantize(min + ratio * range, { min, max, step })
  }

  const handlePointerDown = (event: PointerEvent<HTMLDivElement>) => {
    if (disabled) return
    drag.current = {
      pointerId: event.pointerId,
      last: null,
      bounds: rootRef.current?.getBoundingClientRect() ?? null,
    }
    event.currentTarget.setPointerCapture(event.pointerId)
    event.currentTarget.focus()
    emit(valueAtPointer(event.clientY, drag.current.bounds))
  }

  const handlePointerMove = (event: PointerEvent<HTMLDivElement>) => {
    if (!drag.current || drag.current.pointerId !== event.pointerId) return
    emit(valueAtPointer(event.clientY, drag.current.bounds))
  }

  const finishDrag = (event: PointerEvent<HTMLDivElement>) => {
    if (!drag.current || drag.current.pointerId !== event.pointerId) return
    const last = drag.current.last
    drag.current = null
    if (last !== null) onChangeEnd?.(last)
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
    emit(quantize(nextValue, { min, max, step }), true)
  }

  return (
    <VStack gap="2" w="full" minW="0" alignItems="center">
      <Text
        ref={readoutRef}
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
        w="32px"
        h={`${height}px`}
        flexShrink={0}
        cursor={disabled ? "not-allowed" : "ns-resize"}
        touchAction="none"
        userSelect="none"
        opacity={disabled ? 0.45 : 1}
        outline="none"
        _focusVisible={{ boxShadow: "0 0 0 2px app.accent", borderRadius: "3px" }}
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={finishDrag}
        onPointerCancel={finishDrag}
        onKeyDown={handleKeyDown}
        onWheel={(event) => {
          if (disabled) return
          event.preventDefault()
          emit(quantize(value + (event.deltaY < 0 ? step : -step), { min, max, step }), true)
        }}
      >
        <Box
          position="absolute"
          left="14px"
          top="0"
          bottom="0"
          w="4px"
          rounded="2px"
          bg="app.border"
          pointerEvents="none"
        />
        <Box
          ref={rangeRef}
          position="absolute"
          left="14px"
          bottom="0"
          w="4px"
          h={`${percentage * 100}%`}
          rounded="2px"
          bg="app.accent"
          pointerEvents="none"
        />
        <Box
          ref={thumbRef}
          position="absolute"
          left="9px"
          bottom={`calc(${percentage * 100}% - 7px)`}
          w="14px"
          h="14px"
          rounded="3px"
          bg="app.surface"
          borderWidth="1px"
          borderColor="app.control"
          boxShadow="0 1px 2px rgba(15, 23, 42, 0.18)"
          pointerEvents="none"
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
