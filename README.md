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

## Firmware at a glance
- Event handlers for rising/falling edges and threshold.
- Modes: clip vs wrap; four-position bounce selector (L2 only, L3 only, both, or neither); per-lane resets.
- Bounce push-button acts like a secret rotary switch: each release walks to the next combo, and a dramatic 0.8 s hold slams everything back to "no bounce" so you can regroup mid-jam.
- Pin map matches the design notes in this repo.

## Status
This is a documented reimplementation inspired by The Tuesday Night Machines’ AE forum project. See `SOURCES.md` and `CREDITS.md` for attribution and links.
