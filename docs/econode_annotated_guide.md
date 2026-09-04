# EcoNode — annotated build guide (the "why" behind every choice)

This follows the same part structure as the extreme-detail guide, but stops at every component and wire to explain *why that specific device, why that specific value, why that specific connection* — tied back to the five-stage theory pipeline (harvest → charge → store → regulate → duty-cycle).

---

## Part B — Why these specific parts (not other ones)

### ESP32-C3-WROOM-02 (the MCU)
**What it is:** A single-core RISC-V microcontroller module with built-in Bluetooth Low Energy, integrated flash memory, and an onboard antenna — already assembled onto a small PCB module rather than a bare chip.

**Why this one, specifically:**
- It has a genuinely low deep-sleep current (~20µA) — this is the number your entire 31-day battery life calculation depends on (Stage 5 theory). Pick a chip with worse sleep current and the whole story falls apart.
- BLE is built-in — no separate radio chip needed, which means fewer parts, fewer failure points, and a simpler schematic.
- It's a *module*, not a bare IC — meaning someone already solved the hard RF antenna design for you. Antenna design is genuinely advanced (see the PCB section below); using a pre-certified module sidesteps that entirely.
- Massive Arduino/Wokwi ecosystem support, which is why you can actually simulate it today.

**What a "more correct" industry choice would be:** Nordic's nRF52 series is the actual industry standard for battery/wearable BLE devices — even lower sleep current, purpose-built for exactly this use case. ESP32-C3 is chosen here for accessibility (better beginner tooling), not because it's the objectively best chip. Worth knowing this distinction if it comes up in an interview.

### BME280 (the sensor)
**Why this one, not a DHT22 or BMP280:** The DHT22 (a common cheaper alternative) uses a slow single-wire protocol and has notably lower accuracy. The BMP280 only measures temperature and pressure — no humidity. The BME280 measures all three (temp, humidity, pressure), talks over I2C (Stage 5's "SDA/SCL" wiring), and — critically for your power budget — draws as little as ~3.4µA in its low-power "forced mode," meaning it barely dents your sleep-current number.

### TP4056 (the charge controller)
**Why this one:** It's the most common, cheapest, best-documented single-cell Li-ion charger IC that exists — this matters because as a first-timer, you want a part with abundant reference designs to check your work against. It implements exactly the CC-then-CV charging behavior from Stage 2 theory: it holds a constant current until the cell hits 4.2V, then holds constant voltage while tapering current.

**Why the charge current value matters:** The IC's charge current is set by a resistor on its `PROG` pin, using the formula `I_charge = 1000mA / R_prog(kΩ)`. For a 500mAh cell, you want roughly 0.2–0.5C (100–250mA) — so you'd pick R_prog around 4kΩ (giving ~250mA) or higher for a gentler charge. Since your solar panel can only supply ~60–100mA anyway, the actual charge current will be limited by the panel, not the resistor — but you still set R_prog sensibly in case someone later charges it over USB instead.

**What a "more correct" industry choice would be:** A dedicated energy-harvesting IC like the TI BQ25570 does MPPT (maximum power point tracking) — it actively adjusts to pull the *most* power out of a small solar cell as light conditions change, rather than just accepting whatever the TP4056 is fed. It's the right part for a genuinely optimized product, but it's also a much harder chip to design around as a first PCB. TP4056 is the beginner-appropriate choice; BQ25570 is the "if you keep going in this direction" upgrade.

### TPS63020 (the buck-boost regulator) — and the missing inductor
**Why buck-boost and not a plain LDO:** covered in the theory section — a Li-ion cell swings from 4.2V down to 3.0V, and a plain LDO can't hold 3.3V once the input drops below ~4.4V. A buck-boost converter steps the voltage *up or down* as needed to hold a steady 3.3V across that whole range.

**The part I missed earlier:** Buck-boost (and all switching) regulators work by rapidly switching current through an **inductor** to store and transfer energy — this isn't optional, it's how the topology fundamentally works. You need to add:
- One inductor (check the TPS63020 datasheet's "typical application circuit" — it'll specify a value, commonly around 2.2µH) connected between the two switch-node pins (`L1` and `L2` on the IC).
- A feedback resistor divider from `VOUT` to the `FB` pin, sized per the datasheet formula, to tell the IC "you're at 3.3V, stop adjusting."

Go find that typical application diagram in the datasheet before you finalize your schematic — this is the one part of the design where copying the datasheet's reference circuit almost exactly is the right move, not a shortcut. Every analog/power engineer does this; it's normal practice, not cheating.

### Battery cell — 3.7V, 500mAh
**Why 500mAh specifically:** It's a size/runtime tradeoff. A 500mAh cell is physically small (roughly the size of a AAA battery in a pouch form factor), keeps the whole device pocket-sized, and — per your Stage 5 math — still gives ~31 days of runtime at this duty cycle. Going bigger (1000mAh+) would extend runtime further but adds bulk and cost for a benefit you don't really need once solar is factored in.

### Solar cell — ~5V, 60–100mA
**Why this voltage/current range:** The TP4056 wants an input voltage a bit above the battery's 4.2V max (typically 4.5–5.5V) to charge effectively — a 5V panel matches this directly. The current rating doesn't need to be large because you calculated the device only needs 0.66mAh/hour on average — even a weak, partially-shaded panel can outrun that.

### Decoupling capacitors — why 0.1µF near ICs, 10µF at the regulator
- **0.1µF ceramic caps** placed right next to every IC's power pin filter out high-frequency noise and respond instantly to sudden current demand. This matters most right at the ESP32: when it wakes from deep sleep, current jumps from 20µA to 120mA almost instantaneously. Without a cap sitting right there to supply that initial surge locally, the voltage rail can sag enough to brown out the chip mid-wake — which would be a maddening, hard-to-diagnose bug.
- **10µF bulk caps at the TPS63020's input and output** exist for the same reason but at a larger scale — they're a local reservoir of charge that absorbs the bigger current swings a switching regulator itself creates, and datasheet reference designs specifically call for them near the regulator to keep its internal feedback loop stable (without them, some switching regulators can oscillate).

### Status LED + 330Ω resistor
**Why 330Ω:** Standard LED current-limiting formula: `R = (V_supply − V_LED) / I_desired`. With a 3.3V rail, a ~2V LED forward voltage, and a target of ~4mA (dim but visible, power-conscious), you get `(3.3−2.0)/0.004 ≈ 325Ω` → round to the nearest standard value, 330Ω.

**A real design tension worth noting:** an LED left continuously on would burn through your power budget fast — 4mA constantly is actually more current than your entire sleep-phase average draw. In a real product you'd flash it briefly during the wake cycle only, controlled in firmware, not leave it lit.

---

## Part D — Why these specific wires (tied back to the theory pipeline)

| Wire/net | Why it exists |
|---|---|
| Solar+/− → TP4056 IN+/IN− | Stage 1 → Stage 2 handoff: raw harvested power enters the charge controller's designated input pins, matching the IC's expected input voltage range. |
| TP4056 BAT+/− → Battery | This is the CC/CV-regulated output of Stage 2 feeding directly into Stage 3 storage — the IC is doing the smart charging math here, not you. |
| Battery+ → TPS63020 VIN | Stage 3 → Stage 4: the raw, sagging 3.0–4.2V battery voltage enters the regulator that will stabilize it. |
| TPS63020 VOUT (3V3) with 10µF cap | Stage 4's output: a stabilized rail. The cap right here is non-negotiable per the datasheet — it's part of making the switching regulator stable, not just "good practice." |
| 3V3 → ESP32 3V3 pin, with 0.1µF cap | Feeds Stage 5's compute. The cap specifically protects against the wake-current surge discussed above. |
| 3V3 → BME280 VCC, with 0.1µF cap | Same reasoning, smaller scale — the sensor's current draw is tiny but still benefits from local filtering. |
| GPIO8 (SDA) / GPIO9 (SCL) → BME280 | Correction from an earlier version of this guide: GPIO21/22 is the *classic ESP32's* default I2C pair, not the ESP32-C3's — on the C3, GPIO21 is TXD and is already needed for serial debug output. GPIO8/9 are genuinely free pins on the C3 with no conflicts, which is why the firmware now calls `Wire.begin(8, 9)` explicitly rather than relying on a default. |
| GPIO2 → resistor → LED → GND | GPIO2 is chosen because it's a free pin on the ESP32-C3 that isn't one of the "strapping pins" (pins like GPIO8/9 that must be in a specific state at boot to control boot mode) — using a strapping pin for something like an LED can accidentally prevent the chip from booting correctly. |
| No-connect flags on unused ESP32 pins | Not a "why" about circuit function — this is about telling KiCad's ERC "I deliberately left this pin unused," so it doesn't flag a false error. Also forces you to actually look at every pin and consciously decide, rather than skip pins by accident. |

---

## Part E — What ERC actually checks (and what it doesn't)

Worth being clear-eyed about this: ERC only checks *electrical connectivity rules* — unconnected pins, conflicting outputs tied together, missing power flags. It has no idea whether your TPS63020 feedback resistor values are correct, whether your inductor value matches the datasheet, or whether your circuit will actually work. A schematic can pass ERC with zero errors and still be functionally wrong. ERC is a spelling-checker, not a fact-checker — the datasheet typical application circuit is your fact-checker.

---

## Part F — Why these specific footprints (SOT23, 0603)

This choice is really about matching the design to *your* fabrication capability, not just electrical correctness:
- **SOT23-5/SOT23-6 packages** (for TP4056, TPS63020) are chosen because they're hand-solderable with a decent soldering iron and some patience — the alternative, QFN packages, have pads hidden underneath the chip and genuinely require hot-air rework or a reflow oven, which is out of reach for a first build.
- **0603 passives** (resistors/caps) are a deliberate middle ground: 0805 is easier still but looks noticeably bulkier and less "modern" in photos; 0402 is small enough that hand-soldering without a microscope becomes genuinely frustrating. 0603 is the sweet spot most hobbyists settle on.

---

## Part H — Is the PCB layout too tough? An honest answer

**Short answer: moderately tough, but worth attempting — with the right expectations.**

What makes it *not* too tough:
- You're not designing an antenna (the ESP32 module has one built-in) — RF layout, the genuinely hard part of PCB design, is already solved for you.
- ~15–20 components on a simple 2-layer board is a completely standard "first board" scope. Plenty of first-timers do this.
- You don't need it to be perfect — even a rough, autorouted board that you never fabricate still demonstrates real competence in a screenshot.

What makes it genuinely harder than a trivial board (be aware of these, don't skip them):
- **The switching regulator needs care.** The TPS63020's inductor loop (the path current takes through the inductor and switch pins) should be kept short and the traces reasonably wide — long, thin traces here can cause real-world instability. For a *simulation-only, never-fabricated* board this doesn't actually matter functionally, but doing it right is the actual skill being demonstrated.
- **Keep the sensor away from the regulator.** The BME280 is a sensitive analog sensor; the TPS63020 is a noisy switching regulator. Standard practice is physical separation — place the sensor as far from the regulator's switch node as the board allows, and keep it on the schematic's "quiet" side.
- **Datasheet typical application circuit is your reference**, especially for the TPS63020's inductor and feedback network — don't wing these values, copy them from the datasheet and just verify they make sense for a 3.3V output.

**My honest recommendation:** try it. Budget 2–3 hours rather than the 1–2 in the original guide, actually pull the TPS63020 datasheet's typical application page before you place its surrounding parts, and don't stress about getting the autorouter's result perfect — the point is demonstrating you understand the process, not shipping a production-ready board.

---

## Part I — Why this specific BLE setup in firmware

The service UUID used in the firmware (`0000181A-...`) isn't arbitrary — it's the **official Bluetooth SIG-assigned UUID for "Environmental Sensing Service."** Using a standard, registered UUID (instead of a random one you made up) means any generic BLE scanner app on a phone would recognize your device as an environmental sensor without needing custom app code — this is how real commercial BLE devices behave, and it's a small detail that signals you understand the BLE ecosystem beyond just "make two devices talk."

---

**Bottom line across every part:** almost every "why" above comes back to the same handful of ideas from the theory pipeline — respect the battery's real voltage range (Stage 3/4), protect against current surges at wake (Stage 5), and choose parts/packages that match what you can actually build and verify at your current skill level. That consistency is exactly what you'd want to be able to explain if someone grilled you on this project in an interview.
