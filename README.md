# Generative Ambient CV Sequencer (AE Modular)

An event-driven, multi-lane, generative CV module for the AE Modular ecosystem (0–5 V domain).

## What it does
- 3 independent “lanes” that modify a CV value on **events**:
  - Lane 1: rising gate/trigger
  - Lane 2: falling gate
  - Lane 3: CV threshold crossing (>, < via switch)
- Global offset, per-lane add/sub amounts, optional **wrap** or **clip**, **bounce** behavior, and **resets**.
- 4 outputs: Lane1, Lane2, Lane3, and a **Summed** output.

## Hardware at a glance
- MCU: Arduino Pro Micro (ATmega32U4, 5 V).
- Outputs: either PWM→RC→Buffer (simple) **or** SPI DAC (MCP4922×2)→Buffer (clean 12-bit).
- Input conditioning for 0–5 V gates/CV (AE standard).

### Panel layout roadmap
If you want to hammer out a front panel before the perfboard dust settles, here’s a single-width AE sketch that keeps the
inputs hugging the upper-left and the outputs stacked on the upper-right just like tangible AE gear. The mid-body controls
call out the core gestures: ΔV knobs, add/sub toggles, per-lane resets, the mischievous bounce button, and the wrap/clip and
global reset straddling the bottom edge.

> Think of it as a punk-rock blueprint: proportions match the AE single module canvas (≈25 mm × 128.5 mm), so you can drop
> the SVG into Inkscape, swap fonts to the official AE face if you care, and laser-print or CNC the faceplate without
> guesswork.

## Firmware at a glance
- Event handlers for rising/falling edges and threshold.
- Modes: clip vs wrap; four-position bounce selector (L2 only, L3 only, both, or neither); per-lane resets.
- Bounce push-button acts like a secret rotary switch: each release walks to the next combo, and a dramatic 0.8 s hold instantly yanks the mode back to "no bounce" (the release won’t advance) so you can regroup mid-jam.
- Pin map matches the design notes in this repo.

## Status
This is a documented reimplementation inspired by The Tuesday Night Machines’ AE forum project. See `SOURCES.md` and `CREDITS.md` for attribution and links.
