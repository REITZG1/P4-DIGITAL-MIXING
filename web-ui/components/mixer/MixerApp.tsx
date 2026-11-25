"use client"

import { useCallback, useEffect, useReducer, useState } from "react"
import { Box, Flex, Heading, Stack, Text } from "@chakra-ui/react"
import { AppShell } from "@/components/app-shell/AppShell"
import { DeviceDrawer } from "@/components/device/DeviceDrawer"
import { StatusPill } from "@/components/ui/status-pill"
import { toaster } from "@/components/ui/toaster"
import { ChannelInspector } from "./ChannelInspector"
import { MixerConsole } from "./MixerConsole"
import { syncDemoSignal } from "@/lib/mixer/demo-signal"
import { meterBridge } from "@/lib/mixer/meter-bridge"
import { usePrefersReducedMotion } from "@/lib/use-prefers-reduced-motion"
import {
  createDefaultMixerState,
  mixerReducer,
  type MixerAction,
} from "@/lib/mixer/model"

export function MixerApp() {
  const [state, dispatch] = useReducer(mixerReducer, undefined, createDefaultMixerState)
  const [selected, setSelected] = useState(0)
  const [dirty, setDirty] = useState(false)
  const [saving, setSaving] = useState(false)
  const [deviceOpen, setDeviceOpen] = useState(false)
  const prefersReducedMotion = usePrefersReducedMotion()
  const [pausedByUser, setPausedByUser] = useState<boolean | null>(null)
  const demoPaused = pausedByUser ?? prefersReducedMotion

  // The demo meters read the mixer state straight from this module, so no
  // meter value ever enters React state.
  useEffect(() => {
    syncDemoSignal(state)
  }, [state])

  useEffect(() => {
    meterBridge.setPaused(demoPaused)
  }, [demoPaused])

  const update = useCallback((action: MixerAction) => {
    dispatch(action)
    setDirty(true)
  }, [])

  const save = useCallback(async () => {
    setSaving(true)
    await new Promise((resolve) => window.setTimeout(resolve, 350))
    setDirty(false)
    setSaving(false)
    toaster.success({
      title: "Konfigurasi tersimpan",
      description: "State demo disimpan. Pengiriman ke ESP32 aktif saat perangkat terhubung.",
    })
  }, [])

  return (
    <>
      <AppShell
        status="demo"
        dirty={dirty}
        saving={saving}
        onSave={save}
        onOpenDevice={() => setDeviceOpen(true)}
      >
        <Box
          maxW="1440px"
          mx="auto"
          w="full"
          px={{ base: 4, md: 6, xl: 8 }}
          py={{ base: 4, md: 5, xl: 6 }}
        >
          <Stack gap={{ base: 4, md: 5 }}>
            <Flex justify="space-between" align="flex-end" gap="3" wrap="wrap">
              <Stack gap="1" minW="0">
                <Heading
                  as="h1"
                  fontSize={{ base: "18px", md: "20px" }}
                  fontWeight="600"
                  letterSpacing="-0.01em"
                  lineHeight="1.25"
                  color="app.ink"
                >
                  Console
                </Heading>
                <Text fontSize={{ base: "12px", md: "13px" }} lineHeight="1.5" color="app.muted">
                  Delapan kanal, dua input fisik, dan bus Main L/R, Group 1–2, Aux 1–2.
                </Text>
              </Stack>
              <StatusPill
                tone={dirty ? "warning" : "neutral"}
                label={dirty ? "Perubahan belum disimpan" : "Tidak ada perubahan"}
              />
            </Flex>

            <MixerConsole
              state={state}
              selected={selected}
              onSelect={setSelected}
              dispatch={update}
              demoPaused={demoPaused}
              onToggleDemoPaused={() => setPausedByUser(!demoPaused)}
            />

            <ChannelInspector
              channel={state.channels[selected]}
              channelIndex={selected}
              dispatch={update}
            />
          </Stack>
        </Box>
      </AppShell>

      <DeviceDrawer open={deviceOpen} onOpenChange={setDeviceOpen} status="demo" />
    </>
  )
}
