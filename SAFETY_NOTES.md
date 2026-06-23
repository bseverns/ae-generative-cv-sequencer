# Safety & Interop Notes

- **Voltage domain:** AE is 0–5 V only. Do not feed negative voltages or >5 V into any IO.
- **Common ground:** Ensure a shared ground when interfacing to other systems; attenuate/shift external CV to 0–5 V.
- **Power:** Use the AE bus or a stable 5 V rail. Decouple generously.
- **Prototype status:** This is not a certified commercial design. Verify every build with a meter or scope before patching it into other equipment.
- **External systems:** Eurorack and other modular formats may use negative or higher voltages. Shift, clamp, and attenuate before connecting them to this circuit.
