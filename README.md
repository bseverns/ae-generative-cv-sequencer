# Generative Ambient CV Sequencer for AE Modular

An event-driven, multi-lane generative CV module experiment for the AE Modular 0-5 V ecosystem.

This repository is both a build log and a clean-room reimplementation study. It is also an intentional homage to the people and communities whose ideas, patience, care, and technical generosity made the work possible. See [CREDITS.md](CREDITS.md) and [SOURCES.md](SOURCES.md) for attribution and provenance.

## Current Status

Prototype documentation and firmware scaffold. The design has KiCad hardware files, panel artwork, PWM and DAC firmware variants, safety notes, and a draft bill of materials. Treat it as experimental until the validation checklist in [EXPERIMENT.md](EXPERIMENT.md) has measured results attached.

## What It Does

- Maintains three independent CV lanes that move only when events occur.
- Lane 1 steps on a rising gate or trigger.
- Lane 2 steps on a falling gate.
- Lane 3 steps on a CV threshold crossing, selectable above or below.
- Provides global offset, per-lane step amount, event probability, wrap or clip behavior, bounce modes, and resets.
- Outputs Lane 1, Lane 2, Lane 3, and a summed/averaged CV output.

## Hardware At A Glance

- MCU: Arduino Pro Micro, ATmega32U4, 5 V.
- Output option A: PWM into RC filters and buffers.
- Output option B: two MCP4922 SPI DACs into buffers.
- Input conditioning is designed around AE Modular 0-5 V CV and +5 V gates.

The panel sketch in [hw/panel-layout.svg](hw/panel-layout.svg) targets a single-width AE-style module face. It should be treated as a working layout reference until the hardware and ergonomics are validated on a physical build.

## Firmware At A Glance

- [fw/ae_genseq.ino](fw/ae_genseq.ino): PWM output build.
- [fw/ae_genseq_dac.ino](fw/ae_genseq_dac.ino): MCP4922 DAC output build.
- Each lane has a delta value and a probability value. The edit-layer switch lets the same three lane pots edit either movement size or trigger probability.
- The bounce button cycles L2 only, L3 only, both, and none on release. Holding it for 0.8 seconds resets bounce to none without advancing on release.

## Repository Map

- [EXPERIMENT.md](EXPERIMENT.md): purpose, hypotheses, protocol, validation checklist, and evidence log.
- [HARDWARE.md](HARDWARE.md): electrical notes, block diagram, suggested parts, and pin mapping.
- [FIRMWARE.md](FIRMWARE.md): event model, UI model, firmware variants, and DAC wiring.
- [BOM.md](BOM.md): prototype bill of materials.
- [SAFETY_NOTES.md](SAFETY_NOTES.md): AE voltage and interop cautions.
- [CONTRIBUTING.md](CONTRIBUTING.md): contribution expectations.
- [SUPPORT.md](SUPPORT.md): support scope and issue-reporting guidance.

## License Status

Licensing is not finalized. See [LICENSE_PLACEHOLDER.md](LICENSE_PLACEHOLDER.md) before reusing code, docs, schematics, or artwork.
