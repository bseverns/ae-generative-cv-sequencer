# Safety & Interop Notes

- **Voltage domain:** AE is 0–5 V only. Do not feed negative voltages or >5 V into any IO.
- **Common ground:** Ensure a shared ground when interfacing to other systems; attenuate/shift external CV to 0–5 V.
- **Power:** Use the AE bus or a stable 5 V rail. Decouple generously.
