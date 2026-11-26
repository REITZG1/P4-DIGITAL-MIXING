"use client"

import { useEffect, useState } from "react"

export interface DeviceInfo {
  sampleRate: number | null
  channels: number | null
  masterMute: boolean
  compressor: boolean
  stationConnected: boolean
  stationSsid: string | null
  stationIp: string | null
}

const POLL_MS = 5000

/** Reads the firmware's /api/status endpoint; never used to drive meters. */
export function useDeviceInfo() {
  const [info, setInfo] = useState<DeviceInfo | null>(null)

  useEffect(() => {
    let mounted = true
    const load = async () => {
      try {
        const response = await fetch("/api/status", { cache: "no-store" })
        if (!response.ok) return
        const data = await response.json()
        if (!mounted) return
        setInfo({
          sampleRate: typeof data.sr === "number" ? data.sr : null,
          channels: typeof data.ch === "number" ? data.ch : null,
          masterMute: Boolean(data.mute),
          compressor: Boolean(data.comp),
          stationConnected: Boolean(data.sta_connected),
          stationSsid: typeof data.sta_ssid === "string" ? data.sta_ssid : null,
          stationIp: typeof data.sta_ip === "string" ? data.sta_ip : null,
        })
      } catch {
        if (mounted) setInfo(null)
      }
    }

    void load()
    const timer = window.setInterval(load, POLL_MS)
    return () => {
      mounted = false
      window.clearInterval(timer)
    }
  }, [])

  return info
}
