"use client"

import { Box, Flex, Heading, HStack, Stack, Tabs, Text } from "@chakra-ui/react"
import { memo } from "react"
import { DynamicsPanel } from "./DynamicsPanel"
import { EqPanel } from "./EqPanel"
import { SendsPanel } from "./SendsPanel"
import { StatusPill } from "@/components/ui/status-pill"
import { channelKind, channelName, channelNumber } from "@/lib/mixer/controls"
import type { MixerAction, MixChannel } from "@/lib/mixer/model"

export interface ChannelInspectorProps {
  channel: MixChannel
  channelIndex: number
  dispatch: React.Dispatch<MixerAction>
  disabled?: boolean
}

const tabs = [
  { value: "eq", label: "EQ", hint: "15 band" },
  { value: "sends", label: "Sends", hint: "5 bus" },
  { value: "dynamics", label: "Dynamics", hint: "Compressor" },
]

function ChannelInspectorBase({
  channel,
  channelIndex,
  dispatch,
  disabled = false,
}: ChannelInspectorProps) {
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
        <HStack gap="3" minW="0">
          <Flex
            boxSize="28px"
            rounded="app"
            borderWidth="1px"
            borderColor="app.accentLine"
            bg="app.accentSoft"
            color="app.accent"
            fontFamily="mono"
            fontSize="11px"
            fontWeight="600"
            align="center"
            justify="center"
            className="tabular"
            flexShrink={0}
          >
            {channelNumber(channelIndex)}
          </Flex>
          <Stack gap="0.5" minW="0">
            <Heading as="h2" fontSize="13px" fontWeight="600" lineHeight="18px" color="app.ink" truncate>
              {channelName(channelIndex)}
            </Heading>
            <Text fontSize="11px" lineHeight="14px" color="app.muted" truncate>
              {channelKind(channelIndex) === "IN" ? "Input fisik" : "Mix channel"} · EQ, sends, dynamics
            </Text>
          </Stack>
        </HStack>

        <StatusPill
          tone={channel.mute ? "danger" : "success"}
          label={channel.mute ? "Muted" : "Aktif"}
        />
      </Flex>

      <Tabs.Root defaultValue="eq" size="sm" variant="line">
        <Tabs.List
          borderBottomWidth="1px"
          borderColor="app.border"
          px={{ base: 2, md: 3 }}
          gap="1"
        >
          {tabs.map((tab) => (
            <Tabs.Trigger
              key={tab.value}
              value={tab.value}
              h="10"
              px="3"
              gap="1.5"
              fontSize="12px"
              fontWeight="500"
              color="app.muted"
              _hover={{ color: "app.ink" }}
              _selected={{ color: "app.ink", fontWeight: "600" }}
              _focusVisible={{ outlineOffset: "-2px" }}
              css={{
                // Chakra's line variant draws its own dark indicator; hide it so
                // the teal inset underline below is the only tab indicator.
                "&::before": { display: "none" },
                "&[data-selected]": {
                  boxShadow: "inset 0 -2px 0 0 var(--chakra-colors-app-accent)",
                  borderBottomColor: "transparent",
                },
              }}
            >
              {tab.label}
              <Text as="span" fontSize="10px" color="app.faint" fontWeight="400">
                {tab.hint}
              </Text>
            </Tabs.Trigger>
          ))}
        </Tabs.List>

        <Tabs.Content value="eq" pt={{ base: 3, md: 4 }} px={{ base: 3, md: 4 }} pb={{ base: 4, md: 5 }}>
          <EqPanel
            channel={channel}
            channelIndex={channelIndex}
            dispatch={dispatch}
            disabled={disabled}
          />
        </Tabs.Content>
        <Tabs.Content value="sends" pt={{ base: 4, md: 5 }} px={{ base: 3, md: 4 }} pb={{ base: 5, md: 6 }}>
          <SendsPanel
            channel={channel}
            channelIndex={channelIndex}
            dispatch={dispatch}
            disabled={disabled}
          />
        </Tabs.Content>
        <Tabs.Content value="dynamics" pt={{ base: 4, md: 5 }} px={{ base: 3, md: 4 }} pb={{ base: 5, md: 6 }}>
          <DynamicsPanel
            channel={channel}
            channelIndex={channelIndex}
            dispatch={dispatch}
            disabled={disabled}
          />
        </Tabs.Content>
      </Tabs.Root>
    </Box>
  )
}

export const ChannelInspector = memo(ChannelInspectorBase)
