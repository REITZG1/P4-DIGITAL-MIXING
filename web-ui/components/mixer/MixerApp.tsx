"use client"

import { useEffect, useReducer, useState } from "react"
import { AppShell, type MixerSelection } from "@/components/app-shell/AppShell"
import { DeviceDrawer } from "@/components/device/DeviceDrawer"
import { toaster } from "@/components/ui/toaster"
import { ChannelWorkspace } from "./ChannelWorkspace"
import { MasterWorkspace } from "./MasterWorkspace"
import {
  createDefaultMixerState,
  createEmptyVuState,
  mixerReducer,
  type MixerAction,
} from "@/lib/mixer/model"

export function MixerApp() {
  const [state, dispatch] = useReducer(mixerReducer, undefined, createDefaultMixerState)
  const [vu, setVu] = useState(createEmptyVuState)
  const [selected, setSelected] = useState<MixerSelection>(0)
  const [dirty, setDirty] = useState(false)
  const [saving, setSaving] = useState(false)
  const [deviceOpen, setDeviceOpen] = useState(false)
  const [demoPaused, setDemoPaused] = useState(false)

  useEffect(() => {
    if (demoPaused || window.matchMedia("(prefers-reduced-motion: reduce)").matches) return
    const timer = window.setInterval(() => {
      const time = performance.now() / 1000
      setVu({
        channels: state.channels.map((_, index) => {
          const wave = Math.sin(time * 2.4 + index * 0.7) * 0.28
          const detail = Math.sin(time * 6.1 + index) * 0.12
          return Math.max(0, Math.min(1, 0.38 + wave + detail))
        }),
        bus: {
          masterL: state.masterMute ? 0 : Math.max(0, Math.min(1, 0.55 + Math.sin(time * 2.1) * 0.22)),
          masterR: state.masterMute ? 0 : Math.max(0, Math.min(1, 0.55 + Math.sin(time * 2.1 + 0.2) * 0.22)),
          group1: Math.max(0, Math.sin(time * 1.7) * 0.3),
          group2: Math.max(0, Math.sin(time * 1.9 + 1) * 0.26),
          aux1: Math.max(0, Math.sin(time * 2.3 + 2) * 0.2),
          aux2: Math.max(0, Math.sin(time * 2.6 + 3) * 0.18),
        },
      })
    }, 80)

    return () => window.clearInterval(timer)
  }, [state.channels, state.masterMute, demoPaused])

  const update = (action: MixerAction) => {
    dispatch(action)
    setDirty(true)
  }

  const save = async () => {
    setSaving(true)
    await new Promise((resolve) => window.setTimeout(resolve, 350))
    setDirty(false)
    setSaving(false)
    toaster.success({
      title: "State demo disimpan",
      description: "Konfigurasi akan dikirim ke ESP32 saat mode perangkat aktif.",
    })
  }

  return (
    <>
      <AppShell
        selected={selected}
        onSelect={setSelected}
        status="demo"
        dirty={dirty}
        saving={saving}
        demoPaused={demoPaused}
        onToggleDemoPaused={() => setDemoPaused((paused) => !paused)}
        onSave={save}
        onOpenDevice={() => setDeviceOpen(true)}
      >
        {selected === "master" ? (
          <MasterWorkspace state={state} vu={vu} dispatch={update} />
        ) : (
          <ChannelWorkspace
            channel={state.channels[selected]}
            channelIndex={selected}
            vu={vu}
            dispatch={update}
          />
        )}
      </AppShell>
      <DeviceDrawer
        open={deviceOpen}
        onOpenChange={setDeviceOpen}
        status="demo"
      />
    </>
  )
}
