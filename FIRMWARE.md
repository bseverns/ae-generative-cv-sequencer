# Firmware Design

## Event model (3 lanes)
- **Lane 1 (rise):** on rising gate/trigger, add/subtract ΔV (set by bipolar pot)
- **Lane 2 (fall):** on falling gate, add/subtract ΔV
- **Lane 3 (threshold):** compare CV_in to threshold (>,< via switch); on crossing, add/subtract ΔV

All lanes accumulate into a logical 10-bit range 0..1023 → mapped to 0..5 V.

## Modes
- **Clip vs Wrap:** either clamp to 0..FS or wrap (modulo full-scale)
- **Bounce:** invert direction at boundaries for ping-pong motion
- **Resets:** gateable resets (e.g., L1&L2; ALL)

## Build targets

| Target | File | Outputs | Notes |
|---|---|---|---|
| PWM | `fw/ae_genseq.ino` | D3, D10, D5, D9 → RC → buffer | Minimal BOM; slight PWM ripple; you can raise PWM carrier via timer tweaks |
| DAC | `fw/ae_genseq_dac.ino` | MCP4922×2 (L1/L2 on chip A, L3/SUM on chip B) | Clean 12-bit CV; needs SPI wiring and VREF care |

### SPI DAC wiring (quick ref)
- Common: MOSI, SCK, GND, +5 V
- Chip selects: `CS_A` (L1/L2), `CS_B` (L3/SUM)
- VREF = 5 V (decouple with 100 nF + 10 µF)
- LDAC tied low (or GPIO for simultaneous update)
- DAC outputs → `lane_dac_buf` → jacks
