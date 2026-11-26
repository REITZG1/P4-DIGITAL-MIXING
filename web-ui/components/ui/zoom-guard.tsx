"use client"

import { useEffect } from "react"

/**
 * Keeps the console at a fixed scale. Pinch gestures and Ctrl/Cmd+wheel are
 * browser zoom, not app input, so they are blocked at the document level.
 */
export function ZoomGuard() {
  useEffect(() => {
    const blockZoomWheel = (event: WheelEvent) => {
      if (event.ctrlKey || event.metaKey) event.preventDefault()
    }
    const blockGesture = (event: Event) => event.preventDefault()
    const blockPinch = (event: TouchEvent) => {
      if (event.touches.length > 1) event.preventDefault()
    }
    const blockZoomKeys = (event: KeyboardEvent) => {
      if (!(event.ctrlKey || event.metaKey)) return
      if (["+", "=", "-", "_"].includes(event.key)) event.preventDefault()
    }

    document.addEventListener("wheel", blockZoomWheel, { passive: false })
    document.addEventListener("gesturestart", blockGesture)
    document.addEventListener("gesturechange", blockGesture)
    document.addEventListener("gestureend", blockGesture)
    document.addEventListener("touchmove", blockPinch, { passive: false })
    document.addEventListener("keydown", blockZoomKeys)

    return () => {
      document.removeEventListener("wheel", blockZoomWheel)
      document.removeEventListener("gesturestart", blockGesture)
      document.removeEventListener("gesturechange", blockGesture)
      document.removeEventListener("gestureend", blockGesture)
      document.removeEventListener("touchmove", blockPinch)
      document.removeEventListener("keydown", blockZoomKeys)
    }
  }, [])

  return null
}
