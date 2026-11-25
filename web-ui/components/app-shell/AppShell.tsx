"use client"

import {
  AudioLines,
  CircleDot,
  Gauge,
  Pause,
  Play,
  Save,
  Settings2,
  SlidersHorizontal,
  Wifi,
  WifiOff,
} from "lucide-react"
import {
  Badge,
  Box,
  Button,
  Flex,
  Grid,
  Heading,
  HStack,
  IconButton,
  NativeSelect,
  Separator,
  SkipNavContent,
  SkipNavLink,
  Stack,
  Text,
  VStack,
} from "@chakra-ui/react"
import { CHANNEL_COUNT } from "@/lib/mixer/model"
import { Tooltip } from "@/components/ui/tooltip"

export type MixerSelection = "master" | number
export type ConnectionStatus = "demo" | "connecting" | "connected" | "offline"

export interface AppShellProps {
  children: React.ReactNode
  selected: MixerSelection
  onSelect: (selection: MixerSelection) => void
  status: ConnectionStatus
  dirty: boolean
  saving: boolean
  demoPaused: boolean
  onToggleDemoPaused: () => void
  onSave: () => void
  onOpenDevice: () => void
}

const statusCopy: Record<ConnectionStatus, string> = {
  demo: "Mode demo",
  connecting: "Menghubungkan",
  connected: "Terhubung",
  offline: "Terputus",
}

export function AppShell({
  children,
  selected,
  onSelect,
  status,
  dirty,
  saving,
  demoPaused,
  onToggleDemoPaused,
  onSave,
  onOpenDevice,
}: AppShellProps) {
  const online = status === "connected" || status === "demo"

  return (
    <Grid
      templateColumns={{ base: "minmax(0, 1fr)", md: "220px minmax(0, 1fr)" }}
      h="100dvh"
      overflow="hidden"
      bg="app.bg"
    >
      <SkipNavLink id="mixer-content" className="skip-link">Lewati ke mixer</SkipNavLink>
      <Box as="aside" display={{ base: "none", md: "block" }} borderRightWidth="1px" borderColor="app.border" bg="app.surface">
        <VStack h="full" align="stretch" gap="0">
          <HStack px="5" h="72px" gap="3" borderBottomWidth="1px" borderColor="app.border">
            <Flex boxSize="36px" align="center" justify="center" rounded="app" bg="app.accent" color="app.accentInk">
              <AudioLines size={20} />
            </Flex>
            <Stack gap="0">
              <Text fontWeight="600" fontSize="sm">P4 Digital Mixer</Text>
              <Text fontSize="xs" color="app.muted">ESP32-P4</Text>
            </Stack>
          </HStack>

          <Stack px="3" py="4" gap="1">
            <Text px="3" pb="2" fontSize="xs" color="app.muted">Workspace</Text>
            <Button
              justifyContent="flex-start"
              variant="ghost"
              colorPalette="gray"
              color="app.accent"
              size="sm"
              gap="2"
              aria-current="page"
            >
              <SlidersHorizontal size={16} />
              Mixer
            </Button>
            <Button
              justifyContent="flex-start"
              variant="ghost"
              color="app.muted"
              size="sm"
              gap="2"
              onClick={onOpenDevice}
            >
              <Settings2 size={16} />
              Perangkat
            </Button>
          </Stack>

          <Separator />

          <Stack px="3" py="4" gap="1" flex="1" minH="0" overflowY="auto">
            <Text px="3" pb="2" fontSize="xs" color="app.muted">Channel</Text>
            <ChannelButton
              label="Master"
              selected={selected === "master"}
              onClick={() => onSelect("master")}
              icon={<Gauge size={15} />}
            />
            {Array.from({ length: CHANNEL_COUNT }, (_, index) => (
              <ChannelButton
                key={index}
                label={`Channel ${index + 1}`}
                selected={selected === index}
                onClick={() => onSelect(index)}
              />
            ))}
          </Stack>

          <HStack px="5" h="64px" gap="2" borderTopWidth="1px" borderColor="app.border">
            {online ? <Wifi size={15} color="#4ade80" /> : <WifiOff size={15} color="#f87171" />}
            <Text fontSize="xs" color="app.muted">{statusCopy[status]}</Text>
          </HStack>
        </VStack>
      </Box>

      <Flex direction="column" minW="0" minH="0">
        <HStack
          as="header"
          h="64px"
          px={{ base: 4, md: 6 }}
          gap="3"
          justify="space-between"
          borderBottomWidth="1px"
          borderColor="app.border"
          bg="app.surface"
          flexShrink={0}
        >
          <HStack gap="3" minW="0">
            <Flex display={{ base: "flex", md: "none" }} boxSize="32px" align="center" justify="center" rounded="app" bg="app.accent" color="app.accentInk">
              <AudioLines size={18} />
            </Flex>
            <Stack gap="0" minW="0">
              <Heading as="h1" size="xl" truncate>P4 Digital Mixer</Heading>
              <Text fontSize="xs" color="app.muted" truncate>8 channel · 48 kHz</Text>
            </Stack>
          </HStack>

          <HStack gap="2">
            <Badge colorPalette={online ? "green" : "red"} variant="subtle" display={{ base: "none", sm: "inline-flex" }}>
              <CircleDot size={10} />
              {statusCopy[status]}
            </Badge>
            {dirty && (
              <Text fontSize="xs" color="app.warning" display={{ base: "none", lg: "block" }}>
                Belum disimpan
              </Text>
            )}
            {status === "demo" && (
              <Tooltip content={demoPaused ? "Lanjutkan meter demo" : "Jeda meter demo"}>
                <IconButton
                  aria-label={demoPaused ? "Lanjutkan meter demo" : "Jeda meter demo"}
                  variant="ghost"
                  size="sm"
                  onClick={onToggleDemoPaused}
                >
                  {demoPaused ? <Play size={15} /> : <Pause size={15} />}
                </IconButton>
              </Tooltip>
            )}
            <Tooltip content="Simpan konfigurasi ke perangkat">
              <Button
                size="sm"
                colorPalette="orange"
                gap="2"
                loading={saving}
                disabled={saving || status === "offline"}
                aria-label="Simpan konfigurasi"
                onClick={onSave}
              >
                <Save size={15} />
                <Text display={{ base: "none", sm: "inline" }}>Simpan</Text>
              </Button>
            </Tooltip>
            <Tooltip content="Pengaturan perangkat">
              <IconButton
                aria-label="Buka pengaturan perangkat"
                variant="outline"
                size="sm"
                onClick={onOpenDevice}
              >
                <Settings2 size={16} />
              </IconButton>
            </Tooltip>
          </HStack>
        </HStack>

        <HStack
          as="nav"
          display={{ base: "flex", md: "none" }}
          aria-label="Pemilih channel"
          px="4"
          py="3"
          gap="3"
          borderBottomWidth="1px"
          borderColor="app.border"
          bg="app.panel"
          flexShrink={0}
        >
          <Text fontSize="sm" color="app.muted" flexShrink={0}>Channel</Text>
          <NativeSelect.Root size="lg" flex="1">
            <NativeSelect.Field
              aria-label="Pilih channel"
              value={selected === "master" ? "master" : String(selected)}
              onChange={(event) =>
                onSelect(event.target.value === "master" ? "master" : Number(event.target.value))
              }
            >
              <option value="master">Master</option>
              {Array.from({ length: CHANNEL_COUNT }, (_, index) => (
                <option key={index} value={index}>Channel {index + 1}</option>
              ))}
            </NativeSelect.Field>
            <NativeSelect.Indicator />
          </NativeSelect.Root>
        </HStack>

        <SkipNavContent
          as="main"
          id="mixer-content"
          flex="1"
          minH="0"
          overflowY="auto"
          px={{ base: 4, md: 6, xl: 8 }}
          py={{ base: 5, md: 7 }}
          pb={{ base: 32, md: 7 }}
          className="mixer-main"
        >
          {children}
          <Box h="24" display={{ base: "block", md: "none" }} aria-hidden="true" />
        </SkipNavContent>
      </Flex>

      <HStack
        as="nav"
        aria-label="Navigasi utama"
        display={{ base: "flex", md: "none" }}
        position="fixed"
        insetInline="0"
        bottom="0"
        h="calc(64px + env(safe-area-inset-bottom))"
        pb="env(safe-area-inset-bottom)"
        justify="space-around"
        borderTopWidth="1px"
        borderColor="app.border"
        bg="app.surface"
        zIndex="20"
      >
        <Button variant="ghost" color="app.accent" gap="2"><SlidersHorizontal size={17} />Mixer</Button>
        <Button variant="ghost" color="app.muted" gap="2" onClick={onOpenDevice}><Settings2 size={17} />Perangkat</Button>
      </HStack>
    </Grid>
  )
}

function ChannelButton({
  label,
  selected,
  onClick,
  icon,
}: {
  label: string
  selected: boolean
  onClick: () => void
  icon?: React.ReactNode
}) {
  return (
    <Button
      justifyContent="flex-start"
      variant="ghost"
      colorPalette="gray"
      color={selected ? "app.accent" : "app.muted"}
      fontWeight={selected ? "600" : "400"}
      size="sm"
      gap="2"
      aria-current={selected ? "true" : undefined}
      onClick={onClick}
    >
      {icon}
      {label}
    </Button>
  )
}
