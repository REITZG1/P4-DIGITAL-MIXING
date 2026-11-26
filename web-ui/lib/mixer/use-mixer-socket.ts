"use client"

import { useCallback, useEffect, useRef, useState } from "react"
import { serializeMixerState, type MixerState } from "./model"
import { applyVuMessage, resetVu } from "./vu-store"

export type ConnectionStatus = "connecting" | "connected" | "reconnecting" | "offline"

const RETRY_MS = 2000
const MAX_RETRY_MS = 15000
const SEND_DEBOUNCE_MS = 50

/** Same-origin WebSocket to the mixer firmware, with automatic reconnect. */
export function useMixerSocket(state: MixerState) {
  const [status, setStatus] = useState<ConnectionStatus>("connecting")
  const socket = useRef<WebSocket | null>(null)
  const latestState = useRef(state)

  useEffect(() => {
    latestState.current = state
  }, [state])

  const sendState = useCallback((next: MixerState) => {
    const ws = socket.current
    if (!ws || ws.readyState !== WebSocket.OPEN) return
    ws.send(JSON.stringify(serializeMixerState(next)))
  }, [])

  useEffect(() => {
    let stopped = false
    let retry: number | undefined
    let retryDelay = RETRY_MS
    let hasConnected = false

    // The status pill only changes on a real transition. Retries keep the
    // stable "reconnecting" label instead of flickering connecting/offline
    // every two seconds while the device is booting or unreachable.
    const connect = (initial = false) => {
      if (stopped) return
      if (initial) setStatus("connecting")
      const protocol = window.location.protocol === "https:" ? "wss:" : "ws:"
      const ws = new WebSocket(`${protocol}//${window.location.host}/ws`)
      socket.current = ws

      ws.onopen = () => {
        if (stopped) {
          ws.close()
          return
        }
        hasConnected = true
        retryDelay = RETRY_MS
        setStatus("connected")
        sendState(latestState.current)
      }

      ws.onmessage = (event) => {
        try {
          const message = JSON.parse(event.data)
          if (message?.type === "vu") applyVuMessage(message)
        } catch {
          // Ignore malformed frames; the next VU frame will refresh the meters.
        }
      }

      ws.onerror = () => ws.close()

      ws.onclose = () => {
        if (stopped) return
        socket.current = null
        resetVu()
        setStatus(hasConnected ? "reconnecting" : "offline")
        retry = window.setTimeout(() => connect(), retryDelay)
        retryDelay = Math.min(retryDelay * 2, MAX_RETRY_MS)
      }
    }

    connect(true)
    return () => {
      stopped = true
      if (retry !== undefined) window.clearTimeout(retry)
      socket.current?.close()
      socket.current = null
    }
  }, [sendState])

  useEffect(() => {
    if (status !== "connected") return
    const timer = window.setTimeout(() => sendState(state), SEND_DEBOUNCE_MS)
    return () => window.clearTimeout(timer)
  }, [state, status, sendState])

  return { status, sendState }
}
