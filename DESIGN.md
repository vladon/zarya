---
name: Zarya
description: A calm and dependable desktop control surface for proxy profiles and runtimes.
colors:
  signal-blue: "#1f6feb"
  signal-blue-hover: "#1858c7"
  light-canvas: "#f4f6f8"
  light-surface: "#ffffff"
  light-panel: "#eef1f4"
  light-ink: "#1b1f24"
  light-muted: "#5b6570"
  light-border: "#d0d7de"
  dark-canvas: "#0d1117"
  dark-surface: "#161b22"
  dark-panel: "#21262d"
  dark-ink: "#e6edf3"
  dark-muted: "#8b949e"
  dark-border: "#30363d"
  danger: "#cf222e"
  warning: "#bf8700"
  success: "#1a7f37"
  experimental: "#8250df"
typography:
  title:
    fontFamily: "system-ui"
    fontSize: "16px"
    fontWeight: 600
    lineHeight: 1.4
  body:
    fontFamily: "system-ui"
    fontSize: "13px"
    fontWeight: 400
    lineHeight: 1.45
  label:
    fontFamily: "system-ui"
    fontSize: "13px"
    fontWeight: 600
    lineHeight: 1.35
rounded:
  sm: "4px"
  md: "6px"
spacing:
  xs: "4px"
  sm: "8px"
  md: "16px"
  lg: "24px"
components:
  button-primary:
    backgroundColor: "{colors.signal-blue}"
    textColor: "{colors.light-surface}"
    typography: "{typography.label}"
    rounded: "{rounded.sm}"
    padding: "8px 16px"
  button-secondary:
    backgroundColor: "{colors.light-panel}"
    textColor: "{colors.light-ink}"
    typography: "{typography.label}"
    rounded: "{rounded.sm}"
    padding: "8px 16px"
  input:
    backgroundColor: "{colors.light-surface}"
    textColor: "{colors.light-ink}"
    typography: "{typography.body}"
    rounded: "{rounded.sm}"
    padding: "6px 8px"
---

# Design System: Zarya

## Overview

**Creative North Star: "The Quiet Control Surface"**

Zarya uses restrained neutral layers and a single signal-blue accent so the current state and
next safe action remain obvious. Desktop App Toolkit supplies the interaction quality,
animation, and component mechanics, while Zarya's information architecture and compact visual
language remain its own.

The interface is calm but not sparse. Operational details may be dense when they help diagnose
or configure the runtime, but decoration never competes with state, recovery, or warnings.

**Key Characteristics:**

- Restrained neutral surfaces with one functional accent.
- Compact system typography and predictable desktop density.
- Clear semantic state labels that include text as well as color.
- Short state transitions only; no decorative choreography.
- Progressive disclosure for expert settings.

## Colors

Signal Blue is reserved for primary actions, focus, selection, and links. Neutral layers create
structure; semantic colors communicate operational state.

### Primary

- **Signal Blue:** the sole interactive accent for the current selection, focus, links, and the
  primary action.
- **Signal Blue Hover:** feedback for hover and active emphasis, never a decorative second blue.

### Secondary

- **Success, Warning, Danger, and Experimental:** semantic states accompanied by an icon or
  explicit label.

### Neutral

- **Canvas:** the application background.
- **Surface:** content, editors, and focused work areas.
- **Panel:** toolbars, secondary groups, and inactive controls.
- **Ink and Muted:** primary and supporting text.
- **Border:** separation and focus structure where tonal layering is insufficient.

**The One Signal Rule.** Signal Blue occupies at most a small minority of a screen and appears
only where interaction or current state earns it.

## Typography

**Display Font:** system UI sans-serif

**Body Font:** system UI sans-serif

**Label/Mono Font:** system UI sans-serif; platform monospace only for logs and generated data

**Character:** Familiar, compact, and neutral. Typography must disappear into the task rather
than introduce a separate editorial voice.

### Hierarchy

- **Title** (semibold, 16px): dialog titles and major surface headings.
- **Body** (regular, 13px): controls, descriptions, and operational detail.
- **Label** (semibold, 13px): buttons, field labels, table headers, and status emphasis.

**The Desktop Scale Rule.** Do not use oversized display typography; hierarchy comes from
weight, grouping, and spacing.

## Elevation

Zarya is flat by default. Canvas, surface, and panel tones establish depth; one-pixel borders
separate adjacent controls when needed. Shadows are reserved for real overlays such as menus,
tooltips, and modal layers.

**The Physical Layer Rule.** A shadow is allowed only when a surface genuinely floats above
another interaction layer.

## Components

### Buttons

- **Shape:** gently rounded (4px) with compact 34px-class desktop height.
- **Primary:** Signal Blue fill, light text, one per action group.
- **Hover / Focus:** palette shift plus a visible focus state; short ripple feedback is allowed.
- **Secondary:** tonal panel background or light treatment without decorative shadow.

### Chips

- **Style:** compact tinted background with explicit text; semantic color is never the only
  signal.
- **State:** used for runtime, support, experimental, and recovery status—not as decoration.

### Cards / Containers

- **Corner Style:** restrained rounding (6px maximum).
- **Background:** canvas, surface, or panel tone.
- **Shadow Strategy:** none at rest.
- **Border:** one pixel only where tonal separation is insufficient.
- **Internal Padding:** 8px for compact groups, 16–24px for primary surfaces.

### Inputs / Fields

- **Style:** surface background, one-pixel border, 4px radius.
- **Focus:** Signal Blue border or toolkit focus treatment with keyboard visibility.
- **Error / Disabled:** explicit message plus semantic color; disabled text remains readable.

### Navigation

Menus, toolbar actions, and future navigation surfaces retain standard desktop ordering,
keyboard access, tooltips, and visible current selection. Experimental destinations remain
hidden behind `FeatureGate` on stable builds.

### Status Surface

Runtime, profile, system proxy, routing, and recovery are grouped as one operational summary.
The primary button reflects the next safe action; diagnostics and logs appear when they are
relevant.

## Do's and Don'ts

### Do:

- **Do** use Signal Blue only for primary interaction, focus, selection, and links.
- **Do** pair success, warning, danger, and experimental colors with explicit text.
- **Do** preserve keyboard order, accessible names, and translated EN/RU text lengths.
- **Do** use Desktop App Toolkit components with Zarya-owned layout and information hierarchy.
- **Do** keep stop, restore, retry, and diagnostics actions easy to find.

### Don't:

- **Don't** make Zarya a visual clone of Telegram Desktop.
- **Don't** introduce cyberpunk VPN dashboards, neon gradients, glowing maps, speed gauges, or
  decorative glass panels.
- **Don't** expose every advanced control before the primary task is understood.
- **Don't** communicate runtime, warning, or recovery state through color alone.
- **Don't** use decorative shadows, oversized rounding, gradient text, or motion without a
  state-change purpose.
