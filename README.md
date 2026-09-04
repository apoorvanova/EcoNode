**EcoNode — Solar-Powered IoT Environmental Sensor Node**
A battery + solar-powered sensor node built around an ESP32-C3, designed and validated through schematic capture, power budget analysis, and firmware simulation.


**WHAT IS THIS PROJECT ABOUT?**
EcoNode wakes every 10 minutes, reads its environment sensor, advertises the reading over BLE, and returns to deep sleep — spending 99%+ of its life drawing only ~20µA. A 500mAh battery, topped up by a small solar cell, is estimated to run this indefinitely.


**PROJECT STATUS:**
This is a *design and simulation project*, not yet a fabricated physical board.
- ✅ Schematic: complete, fully wired, passes Electrical Rules Check with zero errors.
- ✅ Power budget: fully calculated (see table below).
- ✅ Firmware: written and logically verified against the schematic's pin map.
- ⚠️ Live simulation capture: attempted via both the Wokwi web simulator and a local VS Code + PlatformIO + Wokwi CLI pipeline; both were blocked by simulator-side tooling/server issues on the day of building this, rather than any issue in the code or wiring itself. The `/simulation` folder below contains the full working local-simulation setup for anyone who wants to run it themselves.
- 🔜 Next step: PCB layout and physical fabrication.

**REPO STRUCTURE:**

EcoNode/
├── README.md
├── schematic/
│   └── (KiCad project files + exported schematic PDF)
├── firmware/
│   ├── econode_firmware.ino            Firmware for the real BME280 sensor (matches the schematic)
│   ├── econode_wokwi_bmp280.ino        Wokwi-simulation variant (BMP280 - closest built-in part to BME280)
│   ├── econode_wokwi_bmp180.ino        Wokwi-simulation variant (BMP180 + Adafruit_BMP085 library)
│   └── econode_diagnostic_no_ble.ino   Stripped-down sensor-only test sketch, used for debugging
├── simulation/
│   ├── diagram.json                    Wokwi wiring diagram (ESP32-C3 + BMP180)
│   ├── platformio.ini                  PlatformIO build config for local compilation
│   └── wokwi.toml                      Points the Wokwi VS Code extension at the compiled firmware
├── docs/
│   ├── econode_extreme_detail_guide.md       Full click-by-click KiCad + Wokwi build walkthrough
│   ├── econode_annotated_guide.md            Component-by-component reasoning and datasheet references
│   ├── econode_net_reference.md              Complete pin/net map, organized by network
│   └── econode_from_scratch_by_network.md    Ground-up schematic build guide
└── images/
    ├── schematic.png       Exported schematic
    ├── erc_clean.png       Zero-error Electrical Rules Check confirmation
    └── firmware_code.png   Firmware source (in place of a live serial capture)


*A note on the sensor substitution:*
The schematic and BOM specify a **BME280** (temperature, humidity, pressure — the real, intended part). Neither the Wokwi web simulator nor its VS Code extension include a native BME280 model in their built-in parts library, so the firmware simulation substitutes the closest available part (BMP280, then BMP180 — same I2C interface, missing only the humidity reading). This is a simulation-environment limitation, not a design change — the schematic, wiring, and power budget all reflect the real BME280.

**POWER BUDGET SUMMARY:**

| Quantity | Value |
|---|---|
| Active current | 120 mA |
| Active duration/cycle | 3.2 s |
| Sleep current | 20 µA |
| Cycle period | 600 s |
| Average draw | ≈0.66 mAh/hr |
| Estimated battery life | ≈31.6 days (battery only), indefinite with solar |


**POWER DISTRIBUTION BY COMPONENT:**

| Component | Net | Active current | Sleep/idle current | Notes |
|---|---|---|---|---|
| ESP32-C3 (MCU + BLE) | 3V3 | ~120 mA (sensor read + BLE TX burst) | ~20 µA (deep sleep) | Dominates the active-phase budget; deep sleep is what makes the 31-day estimate possible |
| BME280 (sensor) | 3V3 | ~3.4 µA (forced/one-shot mode) | <1 µA (sleep mode) | Negligible compared to the MCU — barely affects the total |
| TPS63020 (regulator's own consumption) | VBAT → 3V3 | ~50 µA quiescent | ~50 µA quiescent (power-save mode via PS/SYNC) | This is the regulator's own overhead, separate from what it delivers downstream |
| TP4056 (charger) | SOLAR_IN → VBAT | up to ~234 mA (only while actively charging) | ~0 (idle once charge cycle completes) | Only draws from the solar input, not the battery — doesn't count against battery life |
| Status LED (D1) | LED_CTRL | ~4 mA (only during a brief firmware-triggered flash) | 0 mA (off otherwise) | Deliberately gated by firmware, not tied to always-on 3V3, to avoid wrecking the power budget |

**Why this table matters alongside the summary above:** the summary table tells you *how long the battery lasts*; this table tells you *why* — it makes clear that the ESP32's active burst is the single dominant term, and that keeping every other component's active time as short as possible (or firmware-gated, like the LED) is what protects the overall number.

**NEXT STEPS:**
- [ ] Resolve simulation tooling and capture live serial output
- [ ] PCB layout
- [ ] Fabrication and assembly
- [ ] Real-world battery life validation


**LICENSE:**
MIT — feel free to build on this.