# Product

## Register

product

## Users

Zarya serves people who need a dependable desktop proxy without learning the internals of
Xray or sing-box. The primary workflow is to import or choose a profile, start it, understand
the current runtime and system-proxy state, and recover safely when something fails.
Advanced users also need direct access to routing, DNS, subscriptions, logs, diagnostics, and
experimental features without making the primary workflow feel complicated.

## Product Purpose

Zarya makes proxy profiles and external cores manageable as one calm desktop application.
Success means that the recommended Xray system-proxy path is obvious, state changes are
trustworthy, recovery is always visible, and advanced configuration remains available through
progressive disclosure.

## Brand Personality

Calm, reliable, and clear. Zarya should feel like a quiet control surface: confident enough to
stay out of the user's way, explicit whenever an action affects the operating system, and
precise when something needs attention.

## Anti-references

- A visual clone of Telegram Desktop; Zarya uses Desktop App Toolkit components but keeps its
  own product structure and identity.
- Cyberpunk VPN dashboards with neon gradients, glowing maps, speed gauges, or decorative
  glass panels.
- Dense administration consoles that expose every advanced control before the primary task is
  understood.
- Interfaces that communicate runtime, warning, or recovery state through color alone.

## Design Principles

1. **State before decoration.** Current runtime, profile, system proxy, and recovery state are
   always easier to find than secondary actions.
2. **Progressive power.** The default path stays simple; expert controls remain nearby but do
   not dominate it.
3. **Predictable desktop behavior.** Keyboard navigation, focus, dialogs, tray behavior, and
   platform integrations follow established desktop conventions.
4. **Recovery is a first-class action.** Stop, restore, retry, diagnostics, and safe exit are
   never hidden behind decorative UI.
5. **One component vocabulary.** The same action, field, status, and feedback patterns look and
   behave consistently across every surface.

## Accessibility & Inclusion

Target WCAG 2.1 AA-equivalent contrast and interaction quality for the desktop UI. Every
workflow must be operable by keyboard, expose meaningful accessible names and focus state, and
avoid using color as the only state signal. Motion is limited to short state feedback and must
respect reduced-motion preferences. English and Russian copy must remain usable at translated
text lengths and common display scaling levels.
