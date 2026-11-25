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
          value: '"Familjen Grotesk", system-ui, sans-serif',
        },
        body: {
          value: '"Familjen Grotesk", system-ui, sans-serif',
        },
        mono: {
          value: '"IBM Plex Mono", ui-monospace, monospace',
        },
      },
      colors: {
        app: {
          bg: { value: "#18191b" },
          surface: { value: "#222427" },
          panel: { value: "#1c1d20" },
          raised: { value: "#292b2f" },
          ink: { value: "#eff0f1" },
          muted: { value: "#9c9ea3" },
          faint: { value: "#6f7278" },
          border: { value: "#34363a" },
          accent: { value: "#ff4a1f" },
          accentInk: { value: "#18191b" },
          success: { value: "#4ade80" },
          warning: { value: "#fbbf24" },
          danger: { value: "#f87171" },
        },
      },
      radii: {
        app: { value: "10px" },
      },
    },
    recipes: {
      button: defineRecipe({
        base: {
          fontWeight: "500",
          transition: "transform 140ms ease, background-color 140ms ease, border-color 140ms ease",
          _hover: { transform: "translateY(-1px)" },
          _active: { transform: "translateY(0) scale(0.985)" },
        },
      }),
    },
  },
})

export const system = createSystem(defaultConfig, config)
