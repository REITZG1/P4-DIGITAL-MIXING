import {
  createSystem,
  defaultConfig,
  defineConfig,
  defineRecipe,
} from "@chakra-ui/react"

const config = defineConfig({
  theme: {
    tokens: {
      fonts: {
        heading: {
          value: '"Inter Variable", Inter, system-ui, -apple-system, sans-serif',
        },
        body: {
          value: '"Inter Variable", Inter, system-ui, -apple-system, sans-serif',
        },
        mono: {
          value: '"IBM Plex Mono", ui-monospace, SFMono-Regular, monospace',
        },
      },
      colors: {
        app: {
          bg: { value: "#F4F6F8" },
          surface: { value: "#FFFFFF" },
          panel: { value: "#FAFBFC" },
          raised: { value: "#F1F5F9" },
          ink: { value: "#0F172A" },
          muted: { value: "#5B6B7F" },
          faint: { value: "#64748B" },
          border: { value: "#E2E8F0" },
          borderStrong: { value: "#CBD5E1" },
          control: { value: "#94A3B8" },
          accent: { value: "#0F766E" },
          accentHover: { value: "#115E59" },
          accentSoft: { value: "#ECF7F5" },
          accentLine: { value: "#B7DED8" },
          accentInk: { value: "#FFFFFF" },
          success: { value: "#15803D" },
          successSoft: { value: "#F0FDF4" },
          successLine: { value: "#BBF7D0" },
          successDot: { value: "#16A34A" },
          warning: { value: "#B45309" },
          warningSoft: { value: "#FFFBEB" },
          warningLine: { value: "#FDE68A" },
          warningDot: { value: "#D97706" },
          danger: { value: "#B91C1C" },
          dangerSoft: { value: "#FEF2F2" },
          dangerLine: { value: "#FECACA" },
          dangerDot: { value: "#DC2626" },
          trace: { value: "#EEF2F6" },
        },
      },
      radii: {
        app: { value: "6px" },
        panel: { value: "8px" },
      },
      shadows: {
        panel: { value: "0 1px 2px rgba(15, 23, 42, 0.04)" },
        overlay: { value: "0 12px 32px rgba(15, 23, 42, 0.12)" },
      },
    },
    recipes: {
      button: defineRecipe({
        base: {
          fontWeight: "500",
          transition:
            "background-color 120ms ease, border-color 120ms ease, color 120ms ease",
        },
      }),
    },
  },
})

export const system = createSystem(defaultConfig, config)
