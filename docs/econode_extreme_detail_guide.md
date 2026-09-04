# EcoNode — extreme-detail build guide

This assumes you have never opened KiCad. Every step names the exact menu, panel, or keyboard shortcut.

---

## Part A — Install and set up

1. Go to kicad.org/download, download **KiCad 8.x** for your OS, install with defaults.
2. Open KiCad. Click **File → New Project**. Name it `EcoNode`, pick a folder. This creates a `.kicad_pro` file plus an empty schematic and PCB.
3. Double-click the schematic icon (`.kicad_sch`) in the project window, or click **Open Schematic Editor** — this is where you'll spend most of your day.
4. In the Schematic Editor, go to **Preferences → Manage Symbol Libraries** and confirm the default KiCad libraries are checked (they are by default on a fresh install) — you need `Device`, `MCU_Espressif`, `Battery_Management`, `Regulator_Linear`, `Connector`.

---

## Part B — Place every symbol (do this before wiring anything)

For each part below: press **A** (or click the symbol icon in the right toolbar) to open the symbol chooser, type the search term, double-click the match, click on the canvas to place it, press **Escape** to stop placing copies.

1. Search `ESP32-C3-WROOM-02` → place near the center-left of the canvas.
2. Search `BME280` → place to the right of the ESP32.
3. Search `TP4056` → place on the far left (this is your power input stage).
4. Search `TPS63020` (buck-boost regulator — see Stage 4 theory above for why this replaces a plain LDO) → place between TP4056 and the ESP32.
5. **Search `L` (inductor)** → place one between the TPS63020's two switch-node pins (usually labeled `L1`/`L2` or `SW1`/`SW2` on the symbol). Buck-boost regulators work by switching current through an inductor — this part is not optional, the IC will not function without it. Check the TPS63020 datasheet's "typical application circuit" for the exact value; it's commonly around 2.2µH.
6. Place **2x resistors** for the feedback divider (search `R`) between `VOUT` and the `FB` pin — again, get the exact values from the datasheet's typical application circuit, since they set your 3.3V output.
7. Search `Battery_Cell` → place below the TP4056, this represents your Li-ion cell.
8. Search `Solar_Cell` (if not found, use a second `Battery_Cell` and rename it "Solar" in Part C) → place to the left of the TP4056.
9. Place **6x capacitor** symbols: search `C`, place 4x near the ESP32 and TPS63020 power pins (0.1µF decoupling), 2x near the TPS63020 input/output (10µF bulk).
10. Place **1x LED** and **1x resistor** (search `LED`, then `R`) for a status indicator — optional but looks complete.
11. Save with **Ctrl+S**.

---

## Part C — Rename and set values

1. Click each capacitor, press **E** (edit properties), set the Value field: `0.1uF` for decoupling caps, `10uF` for bulk caps.
2. Click the resistor, set Value to `330`.
3. Click each part and check the **Reference** field auto-numbered (C1, C2, R1, etc.) — leave as-is.
4. If you used a generic Battery_Cell for the solar input, click it, press **E**, change the Value field to `Solar 5V`.

---

## Part D — Wire everything (the actual circuit)

Use **W** to draw a wire between two pins. For longer/cleaner schematics, use **net labels** instead of drawing every wire physically: press **L**, type a name (e.g. `VBAT`, `3V3`, `GND`, `SDA`, `SCL`), place it at a pin — any two pins with the same label are electrically connected even without a drawn wire.

Wire in this exact order:

1. **Solar → TP4056**: Solar+ pin → TP4056 `IN+` pin. Solar− → TP4056 `IN-`.
2. **TP4056 → Battery**: TP4056 `BAT+` → Battery+ terminal. TP4056 `BAT-` → GND net label.
3. **Battery → TPS63020 input**: Battery+ → TPS63020 `VIN`. GND → GND net label (same net as above).
4. **TPS63020 switch node → inductor**: wire the inductor between the TPS63020's two switch pins (check the datasheet pinout — commonly `L1` and `L2`, or `SW1`/`SW2`). Keep this wire short in your head for now; on the PCB (Part H) this loop should be kept physically short and traces reasonably wide.
5. **TPS63020 output → power rail**: TPS63020 `VOUT` → net label `3V3`. Place a 10µF cap between `3V3` and `GND` right at this pin (bulk decoupling — required for loop stability, not optional).
6. **Feedback divider**: wire the two feedback resistors from Part B in series between `VOUT` and `GND`, with the midpoint connected to the TPS63020's `FB` pin. This tells the IC what voltage it's actually outputting so it can regulate to 3.3V — use the exact resistor values from the datasheet's typical application circuit.
7. **3V3 rail → ESP32**: ESP32 `3V3` pin → net label `3V3`. ESP32 `GND` pin → net label `GND`. Place a 0.1µF cap between them, close to the pin.
8. **3V3 rail → BME280**: BME280 `VCC` → net label `3V3`. BME280 `GND` → net label `GND`. Place a 0.1µF cap between them.
9. **I2C bus**: ESP32-C3 `GPIO8` → net label `SDA` → also place `SDA` label at BME280 `SDI` pin. ESP32-C3 `GPIO9` → net label `SCL` → also place at BME280 `SCK` pin. (Note: GPIO21/22 is the classic-ESP32 default and does not apply to the C3 — GPIO21 on the C3 is TXD, already needed for serial debug output.) Also wire BME280 `CSB` → `3V3` (selects I2C mode over SPI), `SDO` → `GND` (sets I2C address to 0x76), and `VDDIO` → `3V3`.
10. **Status LED**: ESP32 any spare GPIO (e.g. `GPIO2`, avoid strapping pins like GPIO8/9) → resistor → LED anode. LED cathode → `GND`.
11. Every unused pin on the ESP32 module (there will be several) needs a **no-connect flag**: click the pin, press **Q**, or use the no-connect symbol from the toolbar (looks like an X). This is required to pass ERC cleanly.

---

## Part E — Run Electrical Rules Check (ERC)

1. **Inspect menu → Electrical Rules Checker → Run ERC**.
2. Read every warning. Common ones you'll see as a first-timer:
   - "Pin not connected" → you missed a no-connect flag on Part D step 9, or a genuine wiring gap.
   - "Multiple pins in net not driven" → usually means a net label typo (e.g. `GND` vs `Gnd`, KiCad is case-sensitive) — check every net label matches exactly.
3. Fix everything until ERC shows **0 errors**. Warnings about unused power flags on a first schematic are usually fine to leave.

---

## Part F — Assign footprints (needed only if you're doing PCB layout)

1. **Tools → Assign Footprints**.
2. For each part, pick a footprint from the library on the left — ESP32 module footprints are under `RF_Module`, TP4056 and TPS63020 are usually SOT23 packages under `Package_TO_SOT_SMD`, capacitors/resistors under `Capacitor_SMD` / `Resistor_SMD` (pick `C_0603` / `R_0603`).
3. Click **Apply, Save Schematic & Continue**.

---

## Part G — Export your LinkedIn screenshot

1. **File → Plot**, output format **PDF**, plot the schematic sheet.
2. Open the PDF, take a clean screenshot (or export directly as PNG via a screenshot tool) — this is your first LinkedIn image.

---

## Part H — Optional: PCB layout

1. **Tools → Update PCB from Schematic**, click **Update PCB**.
2. In the PCB Editor, you'll see all footprints stacked in one spot — drag them apart into a rough layout: power components (TP4056, TPS63020, inductor, battery connector) on one side, ESP32 + BME280 on the other. Two things worth doing deliberately here: keep the TPS63020-to-inductor loop short (this is the "noisy" switching loop), and keep the BME280 physically as far from the TPS63020 as the board allows, since it's a sensitive analog sensor sitting near a switching regulator.
3. **Route → Autorouter** isn't built into KiCad 8 directly — instead, manually route the 4–6 simple traces yourself using the **X** (route) tool, or export a `.dsn` file (**File → Export → Specctra DSN**) and run it through the free tool at freerouting.org, then import the routed `.ses` file back.
4. **View → 3D Viewer** (or press **Alt+3**) for a 3D render — screenshot this for LinkedIn image #2.

---

## Part I — Wokwi firmware simulation

1. Go to wokwi.com, sign in (free), **New Project → ESP32**.
2. In the parts panel (bottom left, `+` icon), search `BME280`, add it. Wire it: `VCC → 3V3`, `GND → GND`, `SCL → D22`, `SDA → D21` (drag between pins on the diagram).
3. Open `sketch.ino` in the code panel, delete the default content, paste the firmware from `econode_firmware.ino` (already generated for you).
4. Click the green **Play** button. Watch the **Serial Monitor** panel — you should see temperature/humidity/pressure print, then "Sleeping for 600 seconds," then the simulation pauses (deep sleep).
5. Screenshot the serial output — this is your LinkedIn image #3, and the strongest proof that this isn't just a static schematic.

---

## Part J — Power budget worksheet (for your post)

Fill in with your actual measured/simulated numbers if they differ from the estimates:

| Quantity | Value | Source |
|---|---|---|
| Active current | 120 mA | ESP32-C3 datasheet, BLE TX burst |
| Active duration per cycle | 3.2 s | sensor read (~0.2s) + BLE advertise (3s) |
| Sleep current | 20 µA | ESP32-C3 datasheet, deep sleep |
| Cycle period | 600 s | your firmware's `SLEEP_SECONDS` |
| Charge per cycle | ≈ 0.110 mAh | (120mA × 3.2/3600) + (0.02mA × 596.8/3600) |
| Average draw | ≈ 0.66 mAh/hr | 6 cycles/hour × 0.110 mAh |
| Battery capacity | 500 mAh | cell datasheet |
| Estimated battery life | ≈ 31.6 days | 500 ÷ 0.66 |

---

You now have three screenshots (schematic, 3D PCB render if you did Part H, Wokwi serial output) and one table. That's the full post.
