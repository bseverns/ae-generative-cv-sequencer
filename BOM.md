# Bill of Materials (prototype)

- 1× Arduino Pro Micro (5 V)
- 1× quad op-amp (MCP6004 or TL074)
- (PWM build) 4× RC filters: 10 kΩ + 1 µF (per output)
- (DAC build) 2× MCP4922 (dual 12-bit DAC) + decoupling
- Input conditioning:
  - 100 kΩ resistors (per gate/CV input)
  - Schottky diodes (BAT54 or similar) to +5 V and GND
  - Optional 74HC14 for gate cleanup
- Output resistors: 4× 1 kΩ
- Pots: L1 ΔV, L2 ΔV, L3 ΔV, L3 threshold, global offset (plus any extras)
- Switches/Buttons: L3 >/<, wrap/clip, bounce gate, reset gates
- Decoupling: 100 nF per IC, 10 µF bulk near +5 V input
- AE bus 2×5 header, panel hardware, jacks/headers as per your panel
