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
  Text,
  VStack,
} from "@chakra-ui/react"
import { Fader } from "./Fader"
import { LevelMeter } from "./LevelMeter"
import type { MasterParameter, MixerAction, MixerState, VuState } from "@/lib/mixer/model"

export interface MasterWorkspaceProps {
  state: MixerState
  vu: VuState
  dispatch: React.Dispatch<MixerAction>
  disabled?: boolean
}

const buses: Array<{ key: keyof VuState["bus"]; label: string }> = [
  { key: "masterL", label: "Main L" },
  { key: "masterR", label: "Main R" },
  { key: "group1", label: "Group 1" },
  { key: "group2", label: "Group 2" },
  { key: "aux1", label: "Aux 1" },
  { key: "aux2", label: "Aux 2" },
]

export function MasterWorkspace({ state, vu, dispatch, disabled = false }: MasterWorkspaceProps) {
  const setMaster = (parameter: MasterParameter, value: number) =>
    dispatch({ type: "set-master", parameter, value })

  return (
    <Stack gap="6">
      <Flex align="flex-start" justify="space-between" gap="4">
        <Stack gap="1">
          <Heading as="h2" size="lg">
            Master bus
          </Heading>
          <Text color="app.muted">Output utama dan level bus.</Text>
        </Stack>
        <Badge colorPalette={state.masterMute ? "red" : "green"} variant="subtle">
          {state.masterMute ? "Muted" : "Active"}
        </Badge>
      </Flex>

      <Grid templateColumns={{ base: "1fr", xl: "minmax(280px, 0.8fr) minmax(0, 1.2fr)" }} gap="5">
        <Card.Root bg="app.panel" borderWidth="1px" borderColor="app.border" rounded="app">
          <Card.Body p="5">
            <VStack gap="5">
              <HStack gap="6" justifyContent="center" alignItems="flex-end">
                <VStack gap="2">
                  <LevelMeter label="Main L level" value={vu.bus.masterL} height={190} />
                  <Text fontSize="xs" color="app.muted">Main L</Text>
                </VStack>
                <Fader
                  label="Main L"
                  value={state.master.lrL}
                  min={0}
                  max={1}
                  step={0.01}
                  height={190}
                  disabled={disabled}
                  formatValue={(value) => `${Math.round(value * 100)}%`}
                  onChange={(value) => setMaster("lrL", value)}
                />
                <Fader
                  label="Main R"
                  value={state.master.lrR}
                  min={0}
                  max={1}
                  step={0.01}
                  height={190}
                  disabled={disabled}
                  formatValue={(value) => `${Math.round(value * 100)}%`}
                  onChange={(value) => setMaster("lrR", value)}
                />
                <VStack gap="2">
                  <LevelMeter label="Main R level" value={vu.bus.masterR} height={190} />
                  <Text fontSize="xs" color="app.muted">Main R</Text>
                </VStack>
              </HStack>
              <Button
                w="full"
                size="sm"
                variant={state.masterMute ? "solid" : "outline"}
                colorPalette={state.masterMute ? "red" : "gray"}
                disabled={disabled}
                onClick={() => dispatch({ type: "toggle-master-mute" })}
              >
                {state.masterMute ? "Buka master mute" : "Master mute"}
              </Button>
            </VStack>
          </Card.Body>
        </Card.Root>

        <Card.Root bg="app.panel" borderWidth="1px" borderColor="app.border" rounded="app">
          <Card.Body p="5">
            <Stack gap="5">
              <Heading as="h3" size="sm">Bus meters</Heading>
              <HStack gap="3" justifyContent="space-around" alignItems="flex-end" minH="190px">
                {buses.map((bus) => (
                  <VStack key={bus.key} gap="2">
                    <LevelMeter label={`${bus.label} level`} value={vu.bus[bus.key]} height={150} />
                    <Text fontSize="xs" color="app.muted" textAlign="center">{bus.label}</Text>
                  </VStack>
                ))}
              </HStack>
            </Stack>
          </Card.Body>
        </Card.Root>
      </Grid>
    </Stack>
  )
}
