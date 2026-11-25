"use client"

import {
  Badge,
  Button,
  Drawer,
  Heading,
  Separator,
  Stack,
  Text,
} from "@chakra-ui/react"
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

export function DeviceDrawer({ open, onOpenChange, status }: DeviceDrawerProps) {
  return (
    <Drawer.Root open={open} onOpenChange={(details) => onOpenChange(details.open)} placement="end">
      <Drawer.Backdrop />
      <Drawer.Positioner>
        <Drawer.Content bg="app.surface" borderLeftWidth="1px" borderColor="app.border">
          <Drawer.Header borderBottomWidth="1px" borderColor="app.border">
            <Drawer.Title>Perangkat</Drawer.Title>
            <Drawer.CloseTrigger aria-label="Tutup pengaturan perangkat" />
          </Drawer.Header>
          <Drawer.Body p="6">
            <Stack gap="6">
              <Stack gap="2">
                <Badge colorPalette={status === "offline" ? "red" : "green"} variant="subtle" alignSelf="flex-start">
                  {statusLabel[status]}
                </Badge>
                <Heading size="lg">ESP32-P4 Digital Mixer</Heading>
                <Text color="app.muted">
                  {status === "demo"
                    ? "UI sedang memakai state demo. Integrasi WebSocket dan HTTP perangkat aktif pada build produksi."
                    : "Status dan konfigurasi perangkat."}
                </Text>
              </Stack>
              <Separator />
              <Stack gap="3">
                <Text fontSize="sm" color="app.muted">Sample rate</Text>
                <Text fontFamily="mono">48.000 Hz</Text>
                <Text fontSize="sm" color="app.muted">Mix channel</Text>
                <Text fontFamily="mono">8</Text>
                <Text fontSize="sm" color="app.muted">EQ per channel</Text>
                <Text fontFamily="mono">15 band</Text>
              </Stack>
            </Stack>
          </Drawer.Body>
          <Drawer.Footer borderTopWidth="1px" borderColor="app.border">
            <Button variant="outline" onClick={() => onOpenChange(false)}>Tutup</Button>
          </Drawer.Footer>
        </Drawer.Content>
      </Drawer.Positioner>
    </Drawer.Root>
  )
}
