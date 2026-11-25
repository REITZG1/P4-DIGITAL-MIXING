"use client"

import {
  Box,
  Button,
  Flex,
  Heading,
  HStack,
  SimpleGrid,
  Stack,
  Text,
  VStack,
} from "@chakra-ui/react"
import { Pause, Play } from "lucide-react"
import { useCallback, useMemo } from "react"
import { ChannelStrip } from "./ChannelStrip"
import { Fader } from "./Fader"
import { LevelMeter } from "./LevelMeter"
import { busMeterSources, masterMeterSources } from "@/lib/mixer/demo-signal"
import { formatPercent } from "@/lib/mixer/controls"
import type { MixerAction, MixerState } from "@/lib/mixer/model"
import { CHANNEL_COUNT } from "@/lib/mixer/model"

export interface MixerConsoleProps {
  state: MixerState
  selected: number
  onSelect: (index: number) => void
  dispatch: React.Dispatch<MixerAction>
  disabled?: boolean
  demoPaused: boolean
  onToggleDemoPaused: () => void
}

const busLabels: Array<{ key: "group1" | "group2" | "aux1" | "aux2"; label: string }> = [
  { key: "group1", label: "Group 1" },
  { key: "group2", label: "Group 2" },
  { key: "aux1", label: "Aux 1" },
  { key: "aux2", label: "Aux 2" },
]

export function MixerConsole({
  state,
  selected,
  onSelect,
  dispatch,
  disabled = false,
  demoPaused,
  onToggleDemoPaused,
}: MixerConsoleProps) {
  const masterHandlers = useMemo(
    () =>
      (["lrL", "lrR"] as const).reduce(
        (handlers, parameter) => {
          handlers[parameter] = (value: number) =>
            dispatch({ type: "set-master", parameter, value })
          return handlers
        },
        {} as Record<"lrL" | "lrR", (value: number) => void>,
      ),
    [dispatch],
  )

  // Stable handlers keep every other strip out of the render path while one
  // fader is being dragged.
  const handleToggleMute = useCallback(
    (index: number) => dispatch({ type: "toggle-channel-mute", channel: index }),
    [dispatch],
  )

  const handleSend = useCallback(
    (index: number, value: number) =>
      dispatch({ type: "set-send", channel: index, bus: "lr", value }),
    [dispatch],
  )

  const handleToggleMasterMute = useCallback(
    () => dispatch({ type: "toggle-master-mute" }),
    [dispatch],
  )

  return (
    <Box
      bg="app.surface"
      borderWidth="1px"
      borderColor="app.border"
      rounded="panel"
      boxShadow="panel"
      overflow="hidden"
    >
      <Flex
        px={{ base: 3, md: 4 }}
        py="3"
        gap="3"
        justify="space-between"
        align="center"
        borderBottomWidth="1px"
        borderColor="app.border"
        wrap="wrap"
      >
        <Stack gap="0.5" minW="0">
          <Heading as="h2" fontSize="13px" fontWeight="600" lineHeight="18px" color="app.ink">
            Channel strips
          </Heading>
          <Text fontSize="11px" lineHeight="14px" color="app.muted">
            Level dan fader tiap kanal
          </Text>
        </Stack>

        <Button
          size="sm"
          variant="ghost"
          color="app.muted"
          gap="1.5"
          px="2"
          h={{ base: "11", md: "8" }}
          onClick={onToggleDemoPaused}
          aria-pressed={demoPaused}
          _hover={{ bg: "app.raised", color: "app.ink" }}
        >
          {demoPaused ? <Play size={13} aria-hidden="true" /> : <Pause size={13} aria-hidden="true" />}
          <Text fontSize="11px" fontWeight="500">
            {demoPaused ? "Lanjutkan meter" : "Jeda meter"}
          </Text>
        </Button>
      </Flex>

      <Box px="3" py="3" overflowX="auto" className="strip-scroll">
        <Box
          display="grid"
          gridTemplateColumns={{
            base: `repeat(${CHANNEL_COUNT}, minmax(88px, 1fr)) minmax(136px, 1.1fr)`,
            lg: `repeat(${CHANNEL_COUNT}, minmax(84px, 1fr)) minmax(128px, 1.05fr)`,
          }}
          gap="2"
          minW={{ base: "980px", lg: "864px" }}
          alignItems="stretch"
        >
          {state.channels.map((channel, index) => (
            <ChannelStrip
              key={index}
              channel={channel}
              index={index}
              selected={selected === index}
              onSelect={onSelect}
              onToggleMute={handleToggleMute}
              onSend={handleSend}
              disabled={disabled}
            />
          ))}

          <Box
            borderWidth="1px"
            borderColor="app.borderStrong"
            bg="app.panel"
            rounded="app"
            p="2.5"
          >
            <Flex align="center" justify="space-between" gap="1" px="1" py="0.5">
              <Text
                fontFamily="mono"
                fontSize="10px"
                lineHeight="14px"
                color="app.faint"
                className="tabular"
              >
                MAIN
              </Text>
              <Text
                fontSize="9px"
                fontWeight="600"
                lineHeight="14px"
                letterSpacing="0.06em"
                color="app.accent"
              >
                L/R
              </Text>
            </Flex>
            <Text px="1" fontSize="12px" fontWeight="600" lineHeight="16px" color="app.ink">
              Master
            </Text>

            <HStack gap="1.5" justify="center" align="center" mt="2">
              <VStack gap="1">
                <LevelMeter
                  label="Level Main L"
                  source={masterMeterSources.left}
                  length={128}
                  thickness={8}
                />
                <Text fontSize="9px" fontWeight="600" color="app.faint" lineHeight="12px">
                  L
                </Text>
              </VStack>
              <Fader
                label="Fader Main L"
                value={state.master.lrL}
                min={0}
                max={1}
                step={0.01}
                height={128}
                disabled={disabled}
                caption="L"
                formatValue={formatPercent}
                onChange={masterHandlers.lrL}
              />
              <Fader
                label="Fader Main R"
                value={state.master.lrR}
                min={0}
                max={1}
                step={0.01}
                height={128}
                disabled={disabled}
                caption="R"
                formatValue={formatPercent}
                onChange={masterHandlers.lrR}
              />
              <VStack gap="1">
                <LevelMeter
                  label="Level Main R"
                  source={masterMeterSources.right}
                  length={128}
                  thickness={8}
                />
                <Text fontSize="9px" fontWeight="600" color="app.faint" lineHeight="12px">
                  R
                </Text>
              </VStack>
            </HStack>

            <Box
              as="button"
              onClick={handleToggleMasterMute}
              aria-pressed={state.masterMute}
              mt="2"
              w="full"
              h={{ base: "11", md: "7" }}
              rounded="sm"
              borderWidth="1px"
              borderColor={state.masterMute ? "app.danger" : "app.borderStrong"}
              bg={state.masterMute ? "app.dangerSoft" : "app.surface"}
              color={state.masterMute ? "app.danger" : "app.ink"}
              fontSize="10px"
              fontWeight="600"
              letterSpacing="0.06em"
              cursor="pointer"
              transition="background-color 120ms ease, border-color 120ms ease"
              _hover={{ borderColor: state.masterMute ? "app.danger" : "app.faint" }}
              _focusVisible={{ outlineOffset: "1px" }}
            >
              {state.masterMute ? "MAIN MUTED" : "MAIN MUTE"}
            </Box>
          </Box>
        </Box>
      </Box>

      <Box borderTopWidth="1px" borderColor="app.border" px={{ base: 3, md: 4 }} py="3">
        <Text
          fontSize="10px"
          fontWeight="600"
          letterSpacing="0.08em"
          textTransform="uppercase"
          color="app.faint"
          mb="2"
        >
          Bus level
        </Text>
        <SimpleGrid columns={{ base: 2, md: 4 }} gap={{ base: 3, md: 5 }}>
          {busLabels.map((bus) => (
            <VStack key={bus.key} align="stretch" gap="1.5">
              <Flex justify="space-between" align="baseline">
                <Text fontSize="11px" fontWeight="500" color="app.muted">
                  {bus.label}
                </Text>
                <Text fontFamily="mono" fontSize="10px" color="app.faint">
                  {bus.key.startsWith("group") ? "GRP" : "AUX"}
                </Text>
              </Flex>
              <LevelMeter
                label={`Level ${bus.label}`}
                source={busMeterSources[bus.key]}
                orientation="horizontal"
                thickness={6}
                fluid
              />
            </VStack>
          ))}
        </SimpleGrid>
      </Box>
    </Box>
  )
}
