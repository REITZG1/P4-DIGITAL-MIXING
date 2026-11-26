"use client"

import {
  AudioLines,
  ChevronRight,
  Save,
  Settings2,
  SlidersHorizontal,
} from "lucide-react"
import {
  Box,
  Button,
  Flex,
  Grid,
  HStack,
  IconButton,
  Separator,
  SkipNavContent,
  SkipNavLink,
  Stack,
  Text,
} from "@chakra-ui/react"
import { StatusPill, type StatusTone } from "@/components/ui/status-pill"
import { Tooltip } from "@/components/ui/tooltip"
import type { ConnectionStatus } from "@/lib/mixer/use-mixer-socket"
import type { DeviceInfo } from "@/lib/use-device-status"

export type { ConnectionStatus }

export interface AppShellProps {
  children: React.ReactNode
  status: ConnectionStatus
  deviceInfo: DeviceInfo | null
  dirty: boolean
  saving: boolean
  onSave: () => void
  onOpenDevice: () => void
}

const statusCopy: Record<ConnectionStatus, string> = {
  connecting: "Menghubungkan",
  connected: "Terhubung",
  reconnecting: "Menyambung ulang",
  offline: "Terputus",
}

const statusTone: Record<ConnectionStatus, StatusTone> = {
  connecting: "warning",
  connected: "success",
  reconnecting: "warning",
  offline: "danger",
}

function getDeviceFacts(info: DeviceInfo | null): Array<{ label: string; value: string }> {
  return [
    { label: "Sample rate", value: info?.sampleRate ? `${(info.sampleRate / 1000).toFixed(1)} kHz` : "—" },
    { label: "Mix channel", value: info?.channels ? String(info.channels) : "—" },
    { label: "WiFi STA", value: info ? (info.stationConnected ? "Terhubung" : "AP only") : "—" },
    { label: "Alamat IP", value: info?.stationIp ?? "192.168.4.1" },
  ]
}

export function AppShell({
  children,
  status,
  deviceInfo,
  dirty,
  saving,
  onSave,
  onOpenDevice,
}: AppShellProps) {
  return (
    <Flex direction="column" h="100dvh" overflow="hidden" bg="app.bg">
      <SkipNavLink className="skip-link">Lewati ke mixer</SkipNavLink>

      <Flex
        as="header"
        h="56px"
        px={{ base: 3, sm: 4, lg: 5 }}
        gap="3"
        align="center"
        borderBottomWidth="1px"
        borderColor="app.border"
        bg="app.surface"
        flexShrink={0}
        position="relative"
        zIndex="10"
      >
        <HStack gap="2.5" minW="0">
          <Flex
            boxSize="30px"
            rounded="app"
            bg="app.accent"
            color="app.accentInk"
            align="center"
            justify="center"
            flexShrink={0}
          >
            <AudioLines size={17} aria-hidden="true" />
          </Flex>
          <Stack gap="0" minW="0" display="flex">
            <Text fontSize="13px" fontWeight="600" lineHeight="16px" color="app.ink" truncate>
              P4 Digital Mixer
            </Text>
            <Text
              display={{ base: "none", sm: "block" }}
              fontSize="10px"
              lineHeight="12px"
              letterSpacing="0.05em"
              color="app.faint"
              className="tabular"
            >
              ESP32-P4 · Digital Mixer
            </Text>
          </Stack>
        </HStack>

        <HStack ml="auto" gap={{ base: 1.5, md: 2 }}>
          <StatusPill
            tone={statusTone[status]}
            label={statusCopy[status]}
            pulse={status === "connected"}
          />
          <Separator
            orientation="vertical"
            h="20px"
            borderColor="app.border"
            display={{ base: "none", lg: "block" }}
          />
          <Text
            display={{ base: "none", lg: "block" }}
            fontFamily="mono"
            fontSize="11px"
            color="app.muted"
            className="tabular"
            flexShrink={0}
          >
            {deviceInfo?.sampleRate ? `${(deviceInfo.sampleRate / 1000).toFixed(1)} kHz` : "—"}
          </Text>

          <Tooltip content="Simpan konfigurasi ke perangkat">
            <Button
              size="sm"
              h="9"
              px="3.5"
              gap="2"
              bg="app.accent"
              color="app.accentInk"
              fontWeight="500"
              fontSize="12px"
              lineHeight="16px"
              loading={saving}
              disabled={saving || status === "offline"}
              aria-label={dirty ? "Simpan konfigurasi, ada perubahan belum disimpan" : "Simpan konfigurasi"}
              onClick={onSave}
              _hover={{ bg: "app.accentHover" }}
              _active={{ bg: "app.accentHover" }}
            >
              <Save size={14} aria-hidden="true" />
              <Box as="span" display={{ base: "none", sm: "inline" }}>
                Simpan
              </Box>
              {dirty && (
                <Box boxSize="5px" rounded="full" bg="app.accentInk" aria-hidden="true" />
              )}
            </Button>
          </Tooltip>

          <Tooltip content="Pengaturan perangkat">
            <IconButton
              aria-label="Buka pengaturan perangkat"
              variant="outline"
              h="9"
              w="9"
              minW="9"
              borderColor="app.border"
              color="app.muted"
              bg="app.surface"
              onClick={onOpenDevice}
              _hover={{ bg: "app.raised", color: "app.ink", borderColor: "app.borderStrong" }}
            >
              <Settings2 size={15} aria-hidden="true" />
            </IconButton>
          </Tooltip>
        </HStack>
      </Flex>

      <Grid
        templateColumns={{
          base: "minmax(0, 1fr)",
          lg: "64px minmax(0, 1fr)",
          xl: "232px minmax(0, 1fr)",
        }}
        flex="1"
        minH="0"
      >
        <Flex
          as="aside"
          aria-label="Navigasi aplikasi"
          display={{ base: "none", lg: "flex", xl: "none" }}
          direction="column"
          align="center"
          gap="1"
          py="3"
          borderRightWidth="1px"
          borderColor="app.border"
          bg="app.surface"
        >
          <Tooltip content="Console" positioning={{ placement: "right" }}>
            <RailItem label="Console" active icon={<SlidersHorizontal size={17} />} />
          </Tooltip>
          <Tooltip content="Perangkat" positioning={{ placement: "right" }}>
            <RailItem label="Perangkat" icon={<Settings2 size={17} />} onClick={onOpenDevice} />
          </Tooltip>
          <Box mt="auto" aria-hidden="true">
            <Box
              boxSize="6px"
              rounded="full"
              bg={status === "offline" ? "app.danger" : "app.accent"}
            />
          </Box>
        </Flex>

        <Box
          as="aside"
          display={{ base: "none", xl: "block" }}
          borderRightWidth="1px"
          borderColor="app.border"
          bg="app.surface"
          overflowY="auto"
        >
          <Flex direction="column" h="full" px="3" py="4" gap="5">
            <Stack gap="1">
              <SectionLabel>Workspace</SectionLabel>
              <NavItem active icon={<SlidersHorizontal size={15} />}>
                Console
              </NavItem>
              <NavItem icon={<Settings2 size={15} />} onClick={onOpenDevice}>
                Perangkat
              </NavItem>
            </Stack>

            <Box borderTopWidth="1px" borderColor="app.border" pt="4">
              <SectionLabel mb="2">Perangkat</SectionLabel>
              <Stack gap="1">
                {getDeviceFacts(deviceInfo).map((fact) => (
                  <Flex key={fact.label} justify="space-between" align="baseline" gap="2" px="2" py="1">
                    <Text fontSize="11px" color="app.muted">
                      {fact.label}
                    </Text>
                    <Text fontFamily="mono" fontSize="11px" color="app.ink" className="tabular">
                      {fact.value}
                    </Text>
                  </Flex>
                ))}
              </Stack>
            </Box>

            <Box
              mt="auto"
              borderWidth="1px"
              borderColor="app.border"
              bg="app.panel"
              rounded="app"
              p="3"
            >
              <StatusPill
                tone={statusTone[status]}
                label={statusCopy[status]}
                pulse={status === "connected"}
              />
              <Text mt="2" fontSize="11px" lineHeight="16px" color="app.muted">
                {status === "connected"
                  ? "Kontrol dan meter terhubung langsung ke perangkat."
                  : status === "reconnecting"
                    ? "Koneksi terputus. Mencoba menyambung lagi dengan jeda bertambah."
                    : "Menunggu WebSocket perangkat. Kontrol tetap dapat diatur."}
              </Text>
              <Button
                mt="2.5"
                size="sm"
                variant="outline"
                w="full"
                fontSize="12px"
                lineHeight="16px"
                fontWeight="500"
                borderColor="app.border"
                color="app.ink"
                bg="app.surface"
                onClick={onOpenDevice}
                _hover={{ bg: "app.raised", borderColor: "app.borderStrong" }}
              >
                Buka perangkat
                <ChevronRight size={13} aria-hidden="true" />
              </Button>
            </Box>
          </Flex>
        </Box>

        <SkipNavContent
          as="main"
          id="mixer-content"
          flex="1"
          minH="0"
          overflowY="auto"
          bg="app.bg"
          className="mixer-main"
        >
          {children}
          <Box
            h="calc(60px + env(safe-area-inset-bottom))"
            display={{ base: "block", lg: "none" }}
            aria-hidden="true"
          />
        </SkipNavContent>
      </Grid>

      <HStack
        as="nav"
        aria-label="Navigasi utama"
        display={{ base: "flex", lg: "none" }}
        position="fixed"
        insetInline="0"
        bottom="0"
        h="calc(60px + env(safe-area-inset-bottom))"
        pb="env(safe-area-inset-bottom)"
        px="3"
        gap="2"
        borderTopWidth="1px"
        borderColor="app.border"
        bg="app.surface"
        zIndex="20"
      >
        <MobileNavItem active icon={<SlidersHorizontal size={17} />}>
          Console
        </MobileNavItem>
        <MobileNavItem icon={<Settings2 size={17} />} onClick={onOpenDevice}>
          Perangkat
        </MobileNavItem>
      </HStack>
    </Flex>
  )
}

function RailItem({
  label,
  icon,
  active = false,
  onClick,
}: {
  label: string
  icon: React.ReactNode
  active?: boolean
  onClick?: () => void
}) {
  return (
    <Box
      as="button"
      onClick={onClick}
      aria-label={label}
      aria-current={active ? "page" : undefined}
      display="flex"
      alignItems="center"
      justifyContent="center"
      boxSize="10"
      rounded="app"
      color={active ? "app.accent" : "app.faint"}
      bg={active ? "app.accentSoft" : "transparent"}
      cursor="pointer"
      transition="background-color 120ms ease, color 120ms ease"
      _hover={{ bg: active ? "app.accentSoft" : "app.raised", color: active ? "app.accent" : "app.ink" }}
      _focusVisible={{ outlineOffset: "1px" }}
    >
      {icon}
    </Box>
  )
}

function SectionLabel({ children, mb }: { children: React.ReactNode; mb?: string }) {  return (
    <Text
      px="2"
      mb={mb}
      fontSize="10px"
      fontWeight="600"
      letterSpacing="0.08em"
      textTransform="uppercase"
      color="app.faint"
      lineHeight="16px"
    >
      {children}
    </Text>
  )
}

function NavItem({
  children,
  icon,
  active = false,
  onClick,
}: {
  children: React.ReactNode
  icon: React.ReactNode
  active?: boolean
  onClick?: () => void
}) {
  return (
    <Box
      as="button"
      onClick={onClick}
      aria-current={active ? "page" : undefined}
      position="relative"
      display="flex"
      alignItems="center"
      gap="2.5"
      w="full"
      h="9"
      px="2"
      rounded="app"
      fontSize="12px"
      fontWeight={active ? "600" : "500"}
      color={active ? "app.accent" : "app.muted"}
      bg={active ? "app.accentSoft" : "transparent"}
      cursor="pointer"
      textAlign="start"
      transition="background-color 120ms ease, color 120ms ease"
      _hover={{ bg: active ? "app.accentSoft" : "app.raised", color: active ? "app.accent" : "app.ink" }}
      _focusVisible={{ outlineOffset: "1px" }}
    >
      {active && (
        <Box
          position="absolute"
          left="-1px"
          top="8px"
          bottom="8px"
          w="2px"
          rounded="1px"
          bg="app.accent"
          aria-hidden="true"
        />
      )}
      <Box color={active ? "app.accent" : "app.faint"} display="flex" aria-hidden="true">
        {icon}
      </Box>
      {children}
    </Box>
  )
}

function MobileNavItem({
  children,
  icon,
  active = false,
  onClick,
}: {
  children: React.ReactNode
  icon: React.ReactNode
  active?: boolean
  onClick?: () => void
}) {
  return (
    <Box
      as="button"
      onClick={onClick}
      aria-current={active ? "page" : undefined}
      flex="1"
      display="flex"
      flexDirection="column"
      alignItems="center"
      justifyContent="center"
      gap="1"
      h="44px"
      rounded="app"
      fontSize="11px"
      fontWeight={active ? "600" : "500"}
      color={active ? "app.accent" : "app.muted"}
      bg={active ? "app.accentSoft" : "transparent"}
      cursor="pointer"
      _hover={{ bg: active ? "app.accentSoft" : "app.raised" }}
      _focusVisible={{ outlineOffset: "1px" }}
    >
      <Box display="flex" aria-hidden="true">
        {icon}
      </Box>
      {children}
    </Box>
  )
}
