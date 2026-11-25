"use client"

import {
  Badge,
  Button,
  Card,
  Flex,
  Grid,
  HStack,
  Heading,
  Stack,
  Tabs,
  Text,
  VStack,
} from "@chakra-ui/react"
import { DynamicsPanel } from "./DynamicsPanel"
import { EqPanel } from "./EqPanel"
import { Fader } from "./Fader"
import { LevelMeter } from "./LevelMeter"
import { SendsPanel } from "./SendsPanel"
import type { MixerAction, MixChannel, VuState } from "@/lib/mixer/model"

export interface ChannelWorkspaceProps {
  channel: MixChannel
  channelIndex: number
  vu: VuState
  dispatch: React.Dispatch<MixerAction>
  disabled?: boolean
}

export function ChannelWorkspace({
  channel,
  channelIndex,
  vu,
  dispatch,
  disabled = false,
}: ChannelWorkspaceProps) {
  return (
    <Stack gap="6">
      <Flex align="flex-start" justify="space-between" gap="4">
        <Stack gap="1">
          <Heading as="h2" size="lg">
            Channel {channelIndex + 1}
          </Heading>
          <Text color="app.muted">
            {channelIndex < 2 ? `Input ${channelIndex + 1}` : "Mix channel"}
          </Text>
        </Stack>
        <Badge colorPalette={channel.mute ? "red" : "green"} variant="subtle">
          {channel.mute ? "Muted" : "Active"}
        </Badge>
      </Flex>

      <Grid templateColumns={{ base: "1fr", lg: "190px minmax(0, 1fr)" }} gap="5">
        <Card.Root bg="app.panel" borderWidth="1px" borderColor="app.border" rounded="app">
          <Card.Body p="5">
            <VStack gap="5">
              <HStack gap="4" justifyContent="center" alignItems="flex-end">
                <LevelMeter
                  label={`Channel ${channelIndex + 1} level`}
                  value={vu.channels[channelIndex] ?? 0}
                  height={190}
                />
                <Fader
                  label="L/R"
                  value={channel.send.lr}
                  min={0}
                  max={1}
                  step={0.01}
                  height={190}
                  disabled={disabled}
                  formatValue={(value) => `${Math.round(value * 100)}%`}
                  onChange={(value) =>
                    dispatch({ type: "set-send", channel: channelIndex, bus: "lr", value })
                  }
                />
              </HStack>
              <Button
                w="full"
                size="sm"
                variant={channel.mute ? "solid" : "outline"}
                colorPalette={channel.mute ? "red" : "gray"}
                disabled={disabled}
                onClick={() => dispatch({ type: "toggle-channel-mute", channel: channelIndex })}
              >
                {channel.mute ? "Buka mute" : "Mute channel"}
              </Button>
            </VStack>
          </Card.Body>
        </Card.Root>

        <Card.Root bg="app.panel" borderWidth="1px" borderColor="app.border" rounded="app">
          <Card.Body p={{ base: 4, md: 6 }}>
            <Tabs.Root defaultValue="eq" colorPalette="orange">
              <Tabs.List>
                <Tabs.Trigger value="eq">EQ</Tabs.Trigger>
                <Tabs.Trigger value="sends">Sends &amp; FX</Tabs.Trigger>
                <Tabs.Trigger value="dynamics">Dynamics</Tabs.Trigger>
              </Tabs.List>

              <Tabs.Content value="eq" pt="6">
                <EqPanel
                  channel={channel}
                  channelIndex={channelIndex}
                  dispatch={dispatch}
                  disabled={disabled}
                />
              </Tabs.Content>
              <Tabs.Content value="sends" pt="6">
                <SendsPanel
                  channel={channel}
                  channelIndex={channelIndex}
                  dispatch={dispatch}
                  disabled={disabled}
                />
              </Tabs.Content>
              <Tabs.Content value="dynamics" pt="6">
                <DynamicsPanel
                  channel={channel}
                  channelIndex={channelIndex}
                  dispatch={dispatch}
                  disabled={disabled}
                />
              </Tabs.Content>
            </Tabs.Root>
          </Card.Body>
        </Card.Root>
      </Grid>
    </Stack>
  )
}
