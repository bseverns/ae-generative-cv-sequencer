# Hardware Notes

## AE electrical domain
- AE Modular uses **0–5 V** CV and **+5 V** gates. Keep all IO within this range. Common ground with connected gear is required.

## Block diagram
- Gates/CV in → protection + conditioning → MCU (Pro Micro) ADC/DIO
- MCU outputs:
  - Option A: PWM → RC low-pass → op-amp buffer → CV jacks (L1, L2, L3, SUM)
  - Option B: SPI DAC (MCP4922×2) → op-amp buffer → CV jacks

## Suggested parts
- **MCU:** Arduino Pro Micro (ATmega32U4, 5 V)
- **Op-amp:** MCP6004 (rail-to-rail, 5 V) or TL074 (works for slow CV)
- **Input conditioning:** ~100 kΩ series, Schottky clamps to rails (e.g., BAT54), optional 74HC14 for gate cleanup, 100 nF to GND for deglitch
- **PWM→CV filter:** Start with R = 10 kΩ, C = 1 µF → fc ≈ 15.9 Hz  
  (RC = 0.01 s; fc = 1/(2π·RC) ≈ 15.9 Hz)
- **Output:** unity buffer, then 1 kΩ series to the jack (AE convention)
- **Decoupling:** 100 nF per IC; 10 µF bulk on +5 V near the board input

## Pin mapping (baseline)
- **PWM outs:** D3 (L1), D10 (L2), D5 (L3), D9 (SUM)
- **Pots (ADC):** A6 (L1 ΔV), A7 (L2 ΔV), A8 (L3 ΔV), A3 (L3 threshold), A2 (offset)
- **CV ins:** A1 (L3 comparator input), A0 (offset CV)
- **Switches/Gates:** D2 (L3 above/below), D14 (wrap/clip), D7 (reset L1&L2), D15 (bounce L2&L3), D16 (reset all), D0 (L1 rising gate), D1 (L2 falling gate)

> Note: D0/D1 share USB CDC on 32U4. For serial debugging, use the `USE_SAFE_PINS` build option in the PWM sketch to move gates to D4/D6.
