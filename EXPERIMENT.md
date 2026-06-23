# Experiment Plan

## Purpose

This project asks whether a compact AE Modular module can produce musically useful generative control voltage by accumulating small event-driven changes across three independent lanes.

The design is intentionally documented as an experiment: claims should be backed by measurements, patch notes, firmware behavior, or build evidence before they are treated as settled.

## Core Hypotheses

- Event-driven CV lanes can create evolving ambient modulation without requiring a clocked step sequencer grid.
- A three-lane architecture is expressive enough when each lane has separate event logic, delta amount, probability, reset, and wrap/clip behavior.
- A low-cost PWM build is usable for slow generative CV, while the MCP4922 DAC build should provide cleaner output for pitch or more demanding modulation targets.
- AE-compatible 0-5 V IO can be maintained with simple input conditioning and buffered outputs.

## Variables To Evaluate

- Output method: PWM plus RC filter versus MCP4922 DAC.
- Lane event source: rising gate, falling gate, or threshold crossing.
- Movement behavior: clip, wrap, and bounce.
- Trigger probability per lane.
- Offset behavior from the panel pot and external offset CV.
- Musical use: slow ambient modulation, stepped melodic CV, rhythmic modulation, and reset-driven form changes.

## Validation Checklist

- Confirm all inputs remain within the AE 0-5 V domain at the MCU pins.
- Measure all four outputs at minimum, midpoint, and maximum code values.
- Measure PWM ripple after the RC filter and buffer.
- Measure DAC output monotonicity and full-scale range.
- Verify Lane 1 triggers only on rising events.
- Verify Lane 2 triggers only on falling events.
- Verify Lane 3 triggers only on the selected threshold crossing direction.
- Verify probability 0 never steps, 1023 effectively always steps, and mid values produce intermittent motion.
- Verify clip clamps at the rails.
- Verify wrap rolls through the 0/1023 boundary.
- Verify bounce reflects direction at the rails.
- Verify reset L1/L2 and reset all behavior.
- Test D0/D1 serial-pin behavior and the `USE_SAFE_PINS` PWM option.
- Record at least one musical patch recipe for each output method.

## Evidence Log

Add dated notes here as the prototype is tested. Include the firmware file, commit hash when available, hardware revision, instrument used, and measured result.

| Date | Area | Setup | Result | Follow-up |
|---|---|---|---|---|
| TBD | Initial bench validation | TBD | Not yet measured | Build first prototype |

## Open Questions

- Is the PWM output quiet enough for the intended slow-CV use cases?
- Should the summed output be an average, a clipped sum, or a switchable behavior?
- Should lane probability be stored across power cycles or remain session-only?
- Does the edit-layer switch need panel indication, detents, or a different control model?
- Are D0/D1 acceptable as default gate inputs, or should the safe-pin mapping become the primary build?
