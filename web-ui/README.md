# P4 Digital Mixer Web UI

Static Next.js interface for the ESP32-P4 digital mixer. The firmware remains the
source of truth for mixer parameters; this application only renders and controls
that state.

## Product facts

- ESP32-P4 digital audio mixer.
- Eight mix channels, fifteen EQ bands per channel.
- Main L/R, Group 1/2, and Aux 1/2 buses.
- 48 kHz sample rate.
- WebSocket control and HTTP device configuration.
- Production UI is served from the device over the same origin.

## Voice card

Voice: technical and dry, with proof.

- Exact, not cold.
- Sure, not loud.
- Product names and audio parameters stay consistent.
- Indonesian interface messages with standard audio terminology.
- Errors say what happened and what the operator can do next.

## Design contract

- Dark, dense desktop application with a single accent colour.
- No violet, decorative gradients, shaders, photographs, or marketing hero.
- The mixer controls are the visual subject.
- Desktop uses an application sidebar and selected-channel workspace.
- Mobile uses a compact channel picker and bottom navigation.
- Chakra UI provides layout, forms, navigation, tabs, overlays, and feedback.
- Only faders, rotary knobs, and level meters are domain-specific controls.

## Commands

```bash
npm ci
npm run dev
npm run lint
npm run typecheck
npm test
npm run build
```

`npm run build` creates the static export in `out/`.
