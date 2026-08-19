# 🛡️ G.U.A.R.D
### Gas Unit for Air Risk Detection

An ESP32-based IoT safety system that detects gas leaks and responds on its own — no human needed to trigger the alarm, ventilate the space, or notify anyone.

`ESP32` `MQ-2` `Blynk IoT` `Embedded C++` `IoT Safety System`

 ![Arduino IDE](https://img.shields.io/badge/built%20with-Arduino%20IDE-00979D) ![Platform](https://img.shields.io/badge/platform-ESP32-orange) ![Dashboard](https://img.shields.io/badge/dashboard-Blynk-green)

---

## What it does

Drop a gas concentration above a safe threshold, and G.U.A.R.D reacts in under a second:

- 🔴 Flips on a red alarm LED, kills the green "safe" LED
- 🔊 Sounds a double-beep buzzer pattern
- 🚪 Swings open a servo-driven ventilation door
- 📲 Pushes a live alert to your phone via Blynk
- 📊 Streams the live gas reading to a cloud dashboard

And it's fail-safe by design — read the [Safety Logic](#safety-logic) section below, it's the core idea of the whole project.

## Demo

| Safe State | Active Alarm |
|---|---|
| Gas level well under threshold, dashboard green | Gas level over threshold, door open, alert fired |

*(Screenshots of the live dashboard and breadboard prototype live in `/media`.)*

## Hardware

| Part | Role | ESP32 Pin |
|---|---|---|
| MQ-2 Gas Sensor | Reads gas concentration | GPIO 34 (ADC1) |
| SG90 Micro Servo | Opens the ventilation door | GPIO 18 (PWM) |
| Active Buzzer | Audible alarm | GPIO 19 |
| Red LED | Alarm indicator | GPIO 21 |
| Green LED | Safe-state indicator | GPIO 22 |

Everything's powered off a shared breadboard rail; see `Circuit_Diagram.jpeg` for the full wiring.

## Setup

1. **Flash the ESP32** — open the firmware in Arduino IDE / PlatformIO, install the `Blynk` library, set your WiFi + Blynk auth token.
2. **Wire it up** — follow the pin table above and the circuit diagram.
3. **Build the Blynk dashboard** — 4 widgets, mapped to virtual pins:
   - `V0` → Gauge (Gas Level, range 0–4095)
   - `V1` → LED (Alarm Status)
   - `V2` → Switch (Manual Reset)
   - `V3` → Switch (Manual Override)
4. **Power on and wait 20s** — the MQ-2 needs a warm-up period before readings are trusted.
5. **Test it** — wave a lighter (unlit!) or lighter fluid near the sensor to trigger a reading spike and watch the alarm chain fire.

## Safety Logic

The interesting design decision in this project isn't the sensor — it's what happens *after* the alarm fires.

> A single "off" button on a gas alarm is dangerous. If it just silences everything, the space could still be full of gas and nobody would know.

So G.U.A.R.D splits "turn it off" into two deliberately different actions:

| Action | What it does | What it *doesn't* do |
|---|---|---|
| **Manual Reset** | Silences buzzer, closes door, resets LEDs | Does **not** resume monitoring — system stays locked |
| **Manual Override** | Full reset — clears the lock and resumes live monitoring | — |

This means once someone acknowledges an alarm, the system won't silently re-trigger — but it also won't silently stay armed. A human has to explicitly say "the space is clear now" via Override before it starts watching again.

Alarms are also rate-limited (60s cooldown) so a sustained leak doesn't spam your phone with notifications.

## Firmware Behavior at a Glance

```
Gas > threshold?  → Buzzer ON, Door opens, Alert sent, Monitoring active
Manual Reset      → Buzzer OFF, Door closes, Monitoring LOCKED
Manual Override   → Buzzer OFF, Door closes, Monitoring RESUMED
```

## Testing Notes

- No false triggers during the 20s warm-up window ✔️
- Alarm chain (LED/buzzer/door/dashboard/push) confirmed firing above threshold (1800/4095) ✔️
- Reset correctly withholds re-triggering while gas stays elevated ✔️
- Override fully resets state and resumes monitoring, toggle self-resets ✔️
- Dashboard reflects real-time online/offline + sensor state ✔️

## Roadmap

- [ ] Calibrate `GAS_THRESHOLD` against a certified reference gas source
- [ ] Add a second sensor (MQ-135 or smoke/temp) for cross-validated detection
- [ ] Battery backup for power-outage operation
- [ ] Local alarm fallback if WiFi/Blynk drops
- [ ] Historical data logging + trend view on the dashboard


## 🙋 Author

**Galla Indranag**

B.Tech ECE, NRI Institute of Technology, Guntur

Embedded Systems & IoT Engineering (ESSCI-certified), SRM 

- GitHub: [@indra675](https://github.com/indra675)
- LinkedIn: [linkedin.com/in/gallaindranag](https://linkedin.com/in/gallaindranag)
