# EcoNode — complete net reference (every pin, every net)

**The 4 nets in this design, and what's on each:**

## NET: GND (one single shared ground — every device ties here)
- Solar cell − terminal
- TP4056: `GND` pin, `EPAD` (thermal pad)
- Battery − terminal
- TPS63020: `GND` pin, `PGND` pin, `PAD` (thermal pad)
- Feedback resistor R2 (180kΩ) — the FB-to-GND leg
- TPS63020 `PS/SYNC` pin (tied low = power-save mode, this is intentional)
- ESP32-C3: `GND` pin (pin 9)
- BME280: `GND` pin (pin 1), `GND` pin (pin 7)
- 0.1µF decoupling cap at ESP32 — GND side
- 0.1µF decoupling cap at BME280 — GND side
- LED (D1) — cathode side

## NET: SOLAR_IN (raw ~5V from the panel, only exists on this one short run)
- Solar cell + terminal
- TP4056: `VCC` pin

## NET: VBAT (raw battery voltage, 3.0–4.2V depending on charge state)
- TP4056: `BAT` pin
- Battery + terminal
- TPS63020: `VIN` pin, `VINA` pin
- 10µF caps at VIN (both of them)
- 1µF cap at VINA

## NET: 3V3 (regulated, stable output — everything downstream lives here)
- TPS63020: `VOUT` pin
- 10µF cap at VOUT
- Feedback resistor R1 (1MΩ) — the VOUT-to-FB leg (goes to the FB net, not directly to 3V3 — see below)
- ESP32-C3: `3V3` pin (pin 1)
- 0.1µF decoupling cap at ESP32 — 3V3 side
- BME280: `VDD` pin, `VDDIO` pin, `CSB` pin (tying CSB high selects I2C mode)
- 0.1µF decoupling cap at BME280 — 3V3 side
- LED resistor R3 (330Ω) — the side NOT going to the LED (i.e., R3 connects 3V3... actually see note below)

## NET: FB (a small, separate net — just the midpoint of the divider, only 3 things touch it)
- TPS63020: `FB` pin
- R1 (1MΩ) — one leg
- R2 (180kΩ) — one leg
(R1's other leg is on 3V3, R2's other leg is on GND — this is what makes it a divider)

## NET: SDA (just 2 things)
- ESP32-C3: `IO08` pin
- BME280: `SDI` pin

## NET: SCL (just 2 things)
- ESP32-C3: `IO09` pin
- BME280: `SCK` pin

## NET: LED_CTRL (a spare GPIO controlling the status LED — NOT the same as 3V3)
- ESP32-C3: a spare GPIO (your image shows it near pin `IO16`)
- R3 (330Ω) — one leg
- (R3's other leg → LED anode → LED cathode → GND, completing the loop back to the GND net)

This is a firmware-controlled LED (correct, matches the earlier note about not wanting it hardwired always-on) — the firmware sets this GPIO high briefly during a wake cycle to flash it, rather than leaving it permanently lit and burning battery.

## NET: EN (TPS63020's enable pin) — ⚠️ STILL NEEDS FIXING

This should be its own net, tied to **3V3 or VBAT** (either works — active-high enable) — **not tied to GND.**

Looking at your latest screenshot, the wire path connecting `EN`, `PS/SYNC`, and `GND` at the bottom of the TPS63020 still appears to loop back into the same rail as the TP4056's ground area — meaning **EN is still on the GND net, same issue as last time.** If EN stays on GND, the regulator will never turn on and nothing downstream gets power, no matter how correct everything else is.

**Fix:** disconnect EN from that shared bottom rail, and instead run a short wire (or net label) from EN directly to the `3V3` net or the `VBAT` net. PS/SYNC can stay on GND — that part's fine and intentional.

---

## Pins that are genuinely fine to leave unconnected — but need an explicit no-connect flag

These pins are optional status/feature pins on the TP4056 that this design isn't using. Leaving them electrically floating is fine functionally, but KiCad's ERC will keep flagging them with warning markers until you explicitly tell it "yes, I meant to leave this floating" by pressing **Q** on each pin (or clicking it and adding a no-connect flag from the toolbar):

- TP4056 `CE` — **check your specific TP4056 variant's datasheet first**: some versions need CE actively tied low to enable charging rather than left floating. Don't assume it's safe to leave open without checking.
- TP4056 `CHRG` — status output (LED driver for "currently charging"), fine to leave unconnected if you don't want a charge-indicator LED.
- TP4056 `STDBY` — status output ("charge complete"), same — fine to leave unconnected if unused.
- TP4056 `TEMP` — **also check the datasheet**: many TP4056 variants require TEMP tied to GND (or through a specific resistor) to *disable* the temperature-monitoring safety feature; leaving it truly floating can sometimes make the IC think there's a fault and refuse to charge at all. This one is worth confirming rather than guessing.
- TP4056 `PROG` — this one is NOT optional, it needs a resistor to GND to set charge current (discussed earlier, ~4kΩ for a gentle charge rate). Confirm this resistor is actually present in your schematic — I couldn't fully confirm it from the image.
- TPS63020 `PG` — fine to tie to GND as you have it (just means you're not using the power-good monitoring feature, harmless either way).

---

## Quick self-check before your next screenshot

1. Click on the EN pin of the TPS63020 in KiCad and check the net name shown in the status bar / properties — it should say `3V3` or `VBAT`, not `GND`.
2. Run ERC (**Inspect → Electrical Rules Checker**) and read every remaining warning individually rather than skimming — for each one, decide "this needs a wire" or "this needs a no-connect flag," don't leave any unresolved.
3. Double check the TP4056 datasheet specifically for what CE and TEMP need — I flagged these as uncertain because I genuinely can't confirm the exact required state for your specific part variant without seeing its datasheet directly.
