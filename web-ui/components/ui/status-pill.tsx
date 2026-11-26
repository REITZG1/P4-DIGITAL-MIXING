import { Box, HStack, Text } from "@chakra-ui/react"

export type StatusTone = "neutral" | "success" | "warning" | "danger" | "accent"

const tones: Record<
  StatusTone,
  { bg: string; border: string; color: string; dot: string }
> = {
  neutral: {
    bg: "app.panel",
    border: "app.border",
    color: "app.muted",
    dot: "app.faint",
  },
  success: {
    bg: "app.successSoft",
    border: "app.successLine",
    color: "app.success",
    dot: "app.successDot",
  },
  warning: {
    bg: "app.warningSoft",
    border: "app.warningLine",
    color: "app.warning",
    dot: "app.warningDot",
  },
  danger: {
    bg: "app.dangerSoft",
    border: "app.dangerLine",
    color: "app.danger",
    dot: "app.dangerDot",
  },
  accent: {
    bg: "app.accentSoft",
    border: "app.accentLine",
    color: "app.accent",
    dot: "app.accent",
  },
}

export interface StatusPillProps {
  label: string
  tone?: StatusTone
  pulse?: boolean
  className?: string
}

export function StatusPill({ label, tone = "neutral", pulse = false, className }: StatusPillProps) {
  const palette = tones[tone]
  return (
    <HStack
      className={className}
      gap="1.5"
      px="2"
      h="22px"
      rounded="4px"
      borderWidth="1px"
      borderColor={palette.border}
      bg={palette.bg}
      color={palette.color}
      flexShrink={0}
    >
      <Box
        boxSize="6px"
        rounded="full"
        bg={palette.dot}
        aria-hidden="true"
        css={
          pulse
            ? {
                "@media (prefers-reduced-motion: no-preference)": {
                  animation: "statusPulse 2.4s ease-in-out infinite",
                },
                "@keyframes statusPulse": {
                  "0%, 100%": { opacity: 1 },
                  "50%": { opacity: 0.35 },
                },
              }
            : undefined
        }
      />
      <Text fontSize="11px" fontWeight="500" lineHeight="1" whiteSpace="nowrap">
        {label}
      </Text>
    </HStack>
  )
}
