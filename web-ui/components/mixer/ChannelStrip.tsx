"use client"

import { Box, Flex, HStack, Text } from "@chakra-ui/react"
import { memo } from "react"
import { Fader } from "./Fader"
import { LevelMeter } from "./LevelMeter"
import { channelMeterSources } from "@/lib/mixer/demo-signal"
import { channelKind, channelName, channelNumber, formatPercent } from "@/lib/mixer/controls"
import type { MixChannel } from "@/lib/mixer/model"

export interface ChannelStripProps {
  channel: MixChannel
  index: number
  selected: boolean
  onSelect: (index: number) => void
  onToggleMute: (index: number) => void
  onSend: (index: number, value: number) => void
  disabled?: boolean
}

const METER_HEIGHT = 128

function ChannelStripBase({
  channel,
  index,
  selected,
  onSelect,
  onToggleMute,
  onSend,
  disabled = false,
}: ChannelStripProps) {
  const name = channelName(index)

  return (
    <Box
      position="relative"
      borderWidth="1px"
      borderColor={selected ? "app.accentLine" : "app.border"}
      bg={selected ? "app.accentSoft" : "app.surface"}
      rounded="app"
      p="2.5"
      minW="0"
      transition="background-color 120ms ease, border-color 120ms ease"
    >
      {selected && (
        <Box
          position="absolute"
          top="-1px"
          left="-1px"
          right="-1px"
          h="2px"
          bg="app.accent"
          borderTopRadius="app"
          aria-hidden="true"
        />
      )}

      <Box
        as="button"
        onClick={() => onSelect(index)}
        aria-pressed={selected}
        aria-label={`${name}, buka kontrol kanal`}
        w="full"
        textAlign="start"
        cursor="pointer"
        rounded="sm"
        px="1"
        py="0.5"
        mx="-1"
        mt="-1"
        _hover={{ bg: "rgba(15, 23, 42, 0.04)" }}
        _focusVisible={{ outlineOffset: "1px" }}
      >
        <Flex align="center" justify="space-between" gap="1">
          <Text
            fontFamily="mono"
            fontSize="10px"
            lineHeight="14px"
            color="app.faint"
            className="tabular"
          >
            {channelNumber(index)}
          </Text>
          <Text
            fontSize="9px"
            fontWeight="600"
            lineHeight="14px"
            letterSpacing="0.06em"
            color={selected ? "app.accent" : "app.faint"}
          >
            {channelKind(index)}
          </Text>
        </Flex>
        <Text fontSize="12px" fontWeight="600" lineHeight="16px" color="app.ink" truncate>
          {name}
        </Text>
      </Box>

      <HStack gap="2" justify="center" align="center" mt="2">
        <LevelMeter
          label={`Level ${name}`}
          source={channelMeterSources[index]}
          length={METER_HEIGHT}
          thickness={8}
        />
        <Fader
          label={`Fader ${name}`}
          value={channel.send.lr}
          min={0}
          max={1}
          step={0.01}
          height={METER_HEIGHT}
          disabled={disabled}
          caption="Main"
          formatValue={formatPercent}
          onChange={(value) => onSend(index, value)}
        />
      </HStack>

      <Box
        as="button"
        onClick={() => onToggleMute(index)}
        aria-pressed={channel.mute}
        aria-label={`${channel.mute ? "Buka mute" : "Mute"} ${name}`}
        mt="2"
        w="full"
        h={{ base: "11", md: "7" }}
        rounded="sm"
        borderWidth="1px"
        borderColor={channel.mute ? "app.danger" : "app.border"}
        bg={channel.mute ? "app.dangerSoft" : "app.surface"}
        color={channel.mute ? "app.danger" : "app.muted"}
        fontSize="10px"
        fontWeight="600"
        letterSpacing="0.06em"
        cursor="pointer"
        transition="background-color 120ms ease, border-color 120ms ease, color 120ms ease"
        _hover={{
          borderColor: channel.mute ? "app.danger" : "app.borderStrong",
          color: channel.mute ? "app.danger" : "app.ink",
        }}
        _focusVisible={{ outlineOffset: "1px" }}
      >
        {channel.mute ? "MUTED" : "MUTE"}
      </Box>
    </Box>
  )
}

export const ChannelStrip = memo(ChannelStripBase)
