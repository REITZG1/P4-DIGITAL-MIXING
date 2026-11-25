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

- Light, dense console application: canvas `#F4F6F8`, surfaces `#FFFFFF`,
  ink `#0F172A`, muted `#5B6B7F`, single teal accent `#0F766E`.
- Flat surfaces separated by 1px `#E2E8F0` hairlines, never heavy shadows.
- 6px radius on controls and strips, 8px on panels and dialogs; 4px spacing scale.
- Inter for the interface, IBM Plex Mono for numeric readouts; labels and
  buttons use weight 500.
- No violet, decorative gradients, shaders, photographs, or marketing hero.
- The mixer controls are the visual subject.
- Desktop keeps all eight channel strips and the master strip on screen at
  once, with a shared EQ / Sends / Dynamics inspector below. Wide viewports
  get a 232px sidebar, mid-widths a compact icon rail.
- Mobile uses a horizontally scrollable strip row and a fixed bottom navigation.
- Chakra UI provides layout, forms, navigation, tabs, overlays, and feedback.
- Only faders, rotary knobs, and level meters are domain-specific controls.
- Meter animation runs outside React: `lib/mixer/meter-bridge.ts` writes CSS
  custom properties at 30 fps from `lib/mixer/demo-signal.ts`, so live levels
  never trigger a component render. Pausing freezes the last reading, and
  reduced-motion users start with meters paused.
- Control updates from drags are coalesced into one animation frame, while
  discrete actions (mute buttons, device state) dispatch immediately. Panel
  callbacks and formatters keep stable identities so memoized faders and knobs
  skip renders when another control changes.

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
