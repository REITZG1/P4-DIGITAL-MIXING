"use client"

import { Box, Drawer, Flex, Heading, HStack, Stack, Text } from "@chakra-ui/react"
import { StatusPill, type StatusTone } from "@/components/ui/status-pill"
import type { ConnectionStatus } from "@/components/app-shell/AppShell"

export interface DeviceDrawerProps {
  open: boolean
  onOpenChange: (open: boolean) => void
  status: ConnectionStatus
}

const statusLabel: Record<ConnectionStatus, string> = {
  demo: "Mode demo",
  connecting: "Menghubungkan",
  connected: "Terhubung",
  offline: "Terputus",
}

const statusTone: Record<ConnectionStatus, StatusTone> = {
  demo: "accent",
  connecting: "warning",
  connected: "success",
  offline: "danger",
}

const facts: Array<{ label: string; value: string }> = [
  { label: "Perangkat", value: "ESP32-P4" },
  { label: "Sample rate", value: "48.0 kHz" },
  { label: "Input fisik", value: "2" },
  { label: "Mix channel", value: "8" },
  { label: "EQ per kanal", value: "15 band" },
  { label: "Bus", value: "Main L/R · Group 1–2 · Aux 1–2" },
  { label: "Transport", value: "WebSocket + HTTP" },
]

export function DeviceDrawer({ open, onOpenChange, status }: DeviceDrawerProps) {
  return (
    <Drawer.Root open={open} onOpenChange={(details) => onOpenChange(details.open)} placement="end">
      <Drawer.Backdrop />
      <Drawer.Positioner>
        <Drawer.Content
          m={{ base: "0", sm: "2" }}
          h={{ base: "100dvh", sm: "calc(100dvh - 16px)" }}
          rounded={{ base: "0", sm: "panel" }}
          borderWidth={{ base: "0", sm: "1px" }}
          borderColor="app.border"
          bg="app.surface"
          boxShadow="overlay"
        >
          <Drawer.Header
            px="5"
            py="4"
            borderBottomWidth="1px"
            borderColor="app.border"
          >
            <HStack justify="space-between" gap="3">
              <Stack gap="0.5">
                <Drawer.Title fontSize="14px" fontWeight="600" color="app.ink">
                  Perangkat
                </Drawer.Title>
                <Text fontSize="11px" color="app.muted">
                  Status dan konfigurasi mixer
                </Text>
              </Stack>
              <Drawer.CloseTrigger
                aria-label="Tutup pengaturan perangkat"
                rounded="sm"
                color="app.muted"
                _hover={{ bg: "app.raised", color: "app.ink" }}
              />
            </HStack>
          </Drawer.Header>

          <Drawer.Body px="5" py="5">
            <Stack gap="5">
              <Flex align="center" justify="space-between" gap="3">
                <Text fontSize="12px" fontWeight="500" color="app.ink">
                  Koneksi
                </Text>
                <StatusPill
                  tone={statusTone[status]}
                  label={statusLabel[status]}
                  pulse={status === "connected"}
                />
              </Flex>

              <Box borderWidth="1px" borderColor="app.border" rounded="app" overflow="hidden">
                {facts.map((fact, index) => (
                  <Flex
                    key={fact.label}
                    justify="space-between"
                    align="baseline"
                    gap="4"
                    px="3"
                    py="2.5"
                    borderTopWidth={index === 0 ? "0" : "1px"}
                    borderColor="app.border"
                    bg={index % 2 === 0 ? "app.surface" : "app.panel"}
                  >
                    <Text fontSize="11px" color="app.muted" flexShrink={0}>
                      {fact.label}
                    </Text>
                    <Text
                      fontSize="11px"
                      color="app.ink"
                      fontFamily={fact.label === "Bus" ? undefined : "mono"}
                      textAlign="end"
                    >
                      {fact.value}
                    </Text>
                  </Flex>
                ))}
              </Box>

              <Box
                borderWidth="1px"
                borderColor="app.accentLine"
                bg="app.accentSoft"
                rounded="app"
                p="3"
              >
                <Heading as="h3" fontSize="12px" fontWeight="600" color="app.accent" mb="1">
                  Data simulasi
                </Heading>
                <Text fontSize="11px" lineHeight="1.6" color="app.muted">
                  {status === "demo"
                    ? "Kontrol, meter, dan penyimpanan berjalan lokal. Setelah WebSocket perangkat aktif, state yang sama dikirim ke ESP32-P4."
                    : "Kontrol terhubung ke perangkat. Perubahan langsung dikirim lewat WebSocket."}
                </Text>
              </Box>
            </Stack>
          </Drawer.Body>
        </Drawer.Content>
      </Drawer.Positioner>
    </Drawer.Root>
  )
}
