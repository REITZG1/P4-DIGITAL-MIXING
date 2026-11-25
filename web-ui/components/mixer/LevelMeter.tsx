"use client"

import { Box } from "@chakra-ui/react"
import { useEffect, useRef } from "react"
import { meterBridge, type MeterSource } from "@/lib/mixer/meter-bridge"

export interface LevelMeterProps {
  label: string
  source: MeterSource
  orientation?: "vertical" | "horizontal"
  length?: number
  thickness?: number
  className?: string
  fluid?: boolean
}

export function LevelMeter({
  label,
  source,
  orientation = "vertical",
  length = 132,
  thickness = 8,
  className,
  fluid = false,
}: LevelMeterProps) {
  const rootRef = useRef<HTMLDivElement>(null)
  const fillRef = useRef<HTMLDivElement>(null)
  const peakRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    const root = rootRef.current
    const fill = fillRef.current
    if (!root || !fill) return
    return meterBridge.attach({ root, fill, peak: peakRef.current, source })
  }, [source])

  return (
    <Box
      ref={rootRef}
      role="meter"
      aria-label={label}
      aria-valuemin={0}
      aria-valuemax={100}
      aria-valuenow={0}
      className={className ? `meter ${className}` : "meter"}
      data-orientation={orientation}
      w={fluid ? "full" : orientation === "vertical" ? `${thickness}px` : `${length}px`}
      h={orientation === "vertical" ? `${length}px` : `${thickness}px`}
      minW={fluid ? "0" : undefined}
      flexShrink={0}
    >
      <div ref={fillRef} className="meter-fill" />
      <div className="meter-segments" />
      <div ref={peakRef} className="meter-peak" />
    </Box>
  )
}
