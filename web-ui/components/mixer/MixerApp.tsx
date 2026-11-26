"use client"

import { useCallback, useEffect, useReducer, useRef, useState } from "react"
import { Box, Flex, Heading, Stack, Text } from "@chakra-ui/react"
import { AppShell } from "@/components/app-shell/AppShell"
import { DeviceDrawer } from "@/components/device/DeviceDrawer"
import { StatusPill } from "@/components/ui/status-pill"
import { toaster } from "@/components/ui/toaster"
import { ChannelInspector } from "./ChannelInspector"
import { MixerConsole } from "./MixerConsole"
import { meterBridge } from "@/lib/mixer/meter-bridge"
import { useMixerSocket } from "@/lib/mixer/use-mixer-socket"
import { useDeviceInfo } from "@/lib/use-device-status"
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
  const deviceInfo = useDeviceInfo()
  const prefersReducedMotion = usePrefersReducedMotion()
  const { status } = useMixerSocket(state)

  useEffect(() => {
    meterBridge.setPaused(prefersReducedMotion)
  }, [prefersReducedMotion])

  // Pointer devices can fire far more than 60 events per second. Coalescing
  // actions into one animation frame keeps a drag at one React render per
  // frame without dropping any intermediate value. The socket hook sends the
  // resulting state to the device with its own short debounce.
  const pending = useRef<MixerAction[]>([])
  const frame = useRef<number | null>(null)
  const flush = useCallback(() => {
    frame.current = null
    const actions = pending.current
    pending.current = []
    for (const action of actions) dispatch(action)
  }, [])

  const update = useCallback(
    (action: MixerAction) => {
      setDirty(true)
      // Discrete actions (mute buttons, external state) should answer on the
      // same tick; only continuous control drags need frame coalescing.
      if (
        action.type === "toggle-channel-mute" ||
        action.type === "toggle-master-mute" ||
        action.type === "replace"
      ) {
        dispatch(action)
        return
      }
      pending.current.push(action)
      if (frame.current === null) frame.current = window.requestAnimationFrame(flush)
    },
    [flush],
  )

  useEffect(
    () => () => {
      if (frame.current !== null) window.cancelAnimationFrame(frame.current)
    },
    [],
  )

  const save = useCallback(async () => {
    setSaving(true)
    try {
      const response = await fetch("/api/save", { method: "POST" })
      if (!response.ok) throw new Error(String(response.status))
      setDirty(false)
      toaster.success({
        title: "Konfigurasi tersimpan",
        description: "State mixer disimpan ke perangkat.",
      })
    } catch {
      toaster.error({
        title: "Gagal menyimpan",
        description: "Perangkat tidak merespons. Periksa koneksi lalu coba lagi.",
      })
    } finally {
      setSaving(false)
    }
  }, [])

  return (
    <>
      <AppShell
        status={status}
        deviceInfo={deviceInfo}
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
            />

            <ChannelInspector
              channel={state.channels[selected]}
              channelIndex={selected}
              dispatch={update}
            />
          </Stack>
        </Box>
      </AppShell>

      <DeviceDrawer
        open={deviceOpen}
        onOpenChange={setDeviceOpen}
        status={status}
        deviceInfo={deviceInfo}
      />
    </>
  )
}
