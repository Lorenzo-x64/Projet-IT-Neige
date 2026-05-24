# Snow Transceiver — Firmware v0.4.1

A field-grade snow-depth measurement station built on the **uPesy ESP32 WROOM 32E (Low Power)**, using a **DFRobot SEN0313 (A01NYUB)** waterproof ultrasonic sensor, a **u-blox NEO-7M** GPS module, a **Bosch BMI160** 6-DoF IMU for fall and impact detection, an **Ebyte E220-900T22D** LoRa transmitter for remote data relay, and a **micro-SD card** for CSV data logging. Configuration and live monitoring are provided by a built-in Wi-Fi access point and a polished web dashboard served from internal flash (LittleFS).

---

## Highlights by version

**v0.4.1 (this release)**
- **Bug fix: GPS now works.** Both GPS and LoRa were assigned `HardwareSerial(1)` (UART1), causing a hardware conflict — GPS received no data.
- **Ultrasonic stays on rock-solid HW UART2** (it's the primary sensor, reliability comes first); GPS moves to `EspSoftwareSerial`. GPS tolerates this because TinyGPSPlus checksums every NMEA sentence and a missed sentence is replaced by the next one ~1 s later.
- UART map: UART0 = USB monitor · UART1 = LoRa · UART2 = **Ultrasonic** · GPS = SoftwareSerial on GPIO16/17
- **Faster live UI.** Added a lightweight `GET /api/ultrasonic/live` endpoint and a 5 Hz front-end poll loop dedicated to the hero distance / depth readouts. The full `/api/status` poll stays at 1 Hz for everything else (GPS, IMU, battery, LoRa, SD).

**v0.4.0**
- **LoRa transmitter** (Ebyte E220-900T22D) — sends full CSV rows and alert frames to a remote receiver, no external library, direct UART control
- EU 868 MHz / US 915 MHz region toggle, manual channel slider 0–80, four TX power levels
- 16-bit obscurity encryption (E220 built-in CRYPT register) configurable from the dashboard
- TX trigger: follow capture timing, or transmit on an independent manual timer
- Four alert types sent immediately: FALL, IMPACT, SD-full (>=90%), sensor-disconnect (>30 s stale)
- 60-second heartbeat so the receiver can distinguish a quiet transmitter from a lost one
- **Battery monitor** — uPesy on-board voltage divider on GPIO35, SoC% + discharge-rate trend + time-remaining estimate
- New **Sensor Graph** on the overview page: 60-sample rolling line chart of distance + depth

**v0.3.0** — BMI160 fall/impact detection, /falllog.csv, adjustable thresholds, live arc gauge

**v0.2.0** — GPS integration, offline raster-tile map, outlier detection, CSV rotation, deep sleep

---

## 1. Hardware

| Role | Part |
|---|---|
| MCU | uPesy ESP32 WROOM 32E (Low Power), 4 MB flash |
| Distance sensor | DFRobot SEN0313 / A01NYUB (UART, IP67, 28-750 cm) |
| GPS | u-blox NEO-7M / Velleman VMA430 / WPI430, UART, 3.3 V |
| IMU | Bosch BMI160 6-DoF (accelerometer + gyroscope), I2C, 3.3 V |
| Storage | Generic micro-SD breakout, SPI, 3.3 V (FAT32) |
| LoRa | Ebyte E220-900T22D, 868/915 MHz, 22 dBm, UART, 3.3 V TTL |
| Battery | Li-Po via uPesy JST port; measured via on-board GPIO35 divider |

---

## 2. Wiring

### UART assignments

The ESP32 has three hardware UART controllers. All three are used:

| UART | Peripheral | Pins |
|------|-----------|------|
| UART0 | USB / serial monitor | GPIO1 (TX), GPIO3 (RX) — built-in |
| UART1 | LoRa E220 | GPIO33 (TX→LoRa), GPIO32 (RX←LoRa) |
| UART2 | **Ultrasonic A01NYUB** (primary sensor) | GPIO4 (RX only) |
| SoftwareSerial | GPS NEO-7M | GPIO17 (TX→GPS), GPIO16 (RX←GPS) |

> Ultrasonic gets the dedicated hardware UART because it is the primary measurement and its reliability is non-negotiable. GPS runs on `EspSoftwareSerial` because TinyGPSPlus validates every NMEA sentence with a checksum, the receiver sends multiple sentences per second, and position updates are slow — so an occasional dropped sentence costs nothing.

### 2.1 SEN0313 Ultrasonic sensor (UART)

| Sensor pin | ESP32 | Notes |
|---|---|---|
| VCC | 5 V | Sensor requires 5 V |
| GND | GND | Common ground |
| TX | GPIO 4 | Sensor output -> ESP32 RX |
| RX | *float* | Leave unconnected (selects filtered output mode) |

### 2.2 Micro-SD card (SPI)

| SD pin | ESP32 | Notes |
|---|---|---|
| 3V3 | 3.3 V | |
| GND | GND | |
| CS | GPIO 5 | |
| MOSI | GPIO 23 | |
| MISO | GPIO 19 | |
| CLK | GPIO 18 | |

> Format the card as **FAT32** with allocation unit <= 32 KB before first use.

### 2.3 GPS NEO-7M (UART)

| GPS pin | ESP32 | Notes |
|---|---|---|
| VCC | 3.3 V | **3.3 V only — 5 V will damage the chip** |
| GND | GND | |
| TX | GPIO 16 | GPS output -> ESP32 RX |
| RX | GPIO 17 | ESP32 TX -> GPS input |

### 2.4 BMI160 IMU (I2C)

| IMU pin | ESP32 | Notes |
|---|---|---|
| VIN | 3.3 V | |
| GND | GND | |
| SDA | GPIO 21 | I2C data |
| SCL | GPIO 22 | I2C clock |
| SAO | 3.3 V | Address select: 3.3 V = 0x69, GND = 0x68 |
| CS | 3.3 V | Tie HIGH to force I2C mode |
| INT1, INT2, SCX, SDX, OCS | — | Leave unconnected |

### 2.5 Ebyte E220-900T22D LoRa transmitter (UART)

The E220-900T22D is a **DIP module** with 7 signal pins along one edge.
Looking at the module from the **top** with the SMA antenna connector on the right,
the pins run down the LEFT edge, top to bottom:

```
        TOP VIEW
        
        +---------------------------------+
 pin 1 -| M0                    [SMA] ---|- antenna
 pin 2 -| M1                             |
 pin 3 -| RXD                            |
 pin 4 -| TXD                            |
 pin 5 -| AUX                            |
 pin 6 -| VCC                            |
 pin 7 -| GND                            |
        | (3 fixed holes below, no signal)|
        +---------------------------------+
```

Connect each pin to the ESP32:

| E220 pin | E220 label | ESP32 GPIO | Direction | Notes |
|:--------:|-----------|:----------:|-----------|-------|
| **1** | M0 | GPIO 25 | ESP32 -> E220 | Mode select bit 0. Do not leave floating |
| **2** | M1 | GPIO 26 | ESP32 -> E220 | Mode select bit 1. Do not leave floating |
| **3** | RXD | GPIO 33 | ESP32 -> E220 | ESP32 transmits TO the module on this pin |
| **4** | TXD | GPIO 32 | E220 -> ESP32 | E220 transmits TO the ESP32 on this pin |
| **5** | AUX | GPIO 27 | E220 -> ESP32 | Busy/ready signal. Add a 4.7 kOhm pull-up from this pin to 3.3 V |
| **6** | VCC | 3.3 V | Power | 3.3 V is safest; module accepts 3.0-5.5 V but data pins are 3.3 V TTL only |
| **7** | GND | GND | Power | Common ground |

```
                     ESP32 WROOM 32E (uPesy LP)
                    +----------------------------+
                    |                            |
  E220-900T22D      |                            |
  +-----------+     |                            |
  |1  M0 -----+-----+ GPIO25  (LORA_M0_PIN)     |
  |2  M1 -----+-----+ GPIO26  (LORA_M1_PIN)     |
  |3  RXD ----+-----+ GPIO33  (ESP32 TX->LoRa)  |
  |4  TXD ----+-----+ GPIO32  (ESP32 RX<-LoRa)  |
  |5  AUX ----+-+---+ GPIO27  (LORA_AUX_PIN)    |
  |6  VCC ----+-+---+ 3.3 V                     |
  |7  GND ----+--+--+ GND                       |
  +-----------+  |  |                            |
                 |  +----------------------------+
  4.7 kOhm      |
  3.3V --[R]----+   (pull-up on AUX, required)
```

**Important rules:**

1. **Antenna mandatory** — always attach an SMA antenna before powering on. Running the transmitter without an antenna damages the internal power amplifier.
2. **3.3 V TTL only** — even if you power VCC from 5 V, the RXD/TXD/AUX signal pins must see 3.3 V signals. The uPesy ESP32 runs at 3.3 V so no level-shifter is needed.
3. **Pull-up on AUX** — solder a 4.7 kOhm resistor from GPIO27 to the 3.3 V rail. Without it, noise on the wire fools the firmware into thinking the module is always busy and all transmissions will time out.
4. **Do not float M0/M1** — if you do not want to wire them to GPIOs, tie them both to GND (this forces normal transmission mode permanently, but you lose the ability to change channel/key from the dashboard).

---

## 3. Build and flash

```bash
# 1. Flash the firmware
pio run -t upload

# 2. Upload the dashboard files (HTML/CSS/JS) to LittleFS -- REQUIRED
pio run -t uploadfs

# 3. Open the serial monitor
pio device monitor
```

You must run **both** commands after a clean clone. Re-run `uploadfs` any time you change files inside `data/`.

If the build fails with `eiaextensions.h not found` (a Curie-only header in the BMI160 library), clear the cache and retry:

```bash
rm -rf .pio
pio run -t upload
```

---

## 4. Connecting

1. Power the ESP32.
2. Connect to Wi-Fi network **Snow Transceiver** (password `1234567890`).
3. Open `http://192.168.4.1` in a browser.

Set the time once via **Settings -> Sync time from this device**. Until then, timestamps appear as `BOOT+12.345s` (still strictly sortable as text).

---

## 5. LoRa transmitter detail

### 5.1 Mode pin behaviour

| M1 | M0 | Mode | Used for |
|:--:|:--:|------|----------|
| 0 | 0 | Normal (mode 0) | All normal transmissions |
| 1 | 1 | Configuration (mode 3) | Writing channel/power/key registers |

The firmware drives M0/M1 automatically. During normal operation both are LOW. When you click Apply on the LoRa page, the firmware briefly switches to mode 3, writes the 8-byte config, waits for the AUX rising edge, then returns to mode 0.

### 5.2 Channel and frequency

```
frequency_MHz = 850.125 + channel x 1
```

| Region | Channel | Frequency |
|--------|:-------:|-----------|
| EU 868 MHz | 18 | 868.125 MHz |
| US 915 MHz | 65 | 915.125 MHz |

Both transmitter and receiver must use the same channel.

**EU duty-cycle warning:** ETSI limits 868 MHz transmissions to 1% of the time (36 s/hour). At a 10 s capture interval with TX-on-every-capture you would exceed this. Use a capture interval >= 30 s, or switch to manual TX trigger with period >= 60 s.

### 5.3 Encryption

The E220 has a hardware 16-bit key (registers CRYPT_H/CRYPT_L). Set it on the LoRa page. The receiver must configure the same key. This is obscurity-grade — it blocks casual snooping but is not real cryptography.

### 5.4 Frame protocol (for receiver implementers)

```
Byte 0      0xA5    MAGIC (sync)
Byte 1      1       VERSION
Byte 2      TYPE    see table
Byte 3      LEN     payload length (0..240)
Bytes 4-5   SEQ     little-endian uint16 sequence counter
Bytes 6..   PAYLOAD ASCII text, LEN bytes
Last 2      CRC16   CRC-16/CCITT-FALSE over all preceding bytes
                    init=0xFFFF  poly=0x1021
```

| TYPE | Payload |
|:----:|---------|
| 0x01 | Full CSV row (no trailing CRLF) |
| 0x02 | `FALL,<mag_g>,<dur_ms>,<iso8601>` |
| 0x03 | `IMPACT,<mag_g>,<iso8601>` |
| 0x04 | `SDFULL,<used_pct>,<iso8601>` |
| 0x05 | `LOST,<ULTRASONIC\|GPS\|IMU\|BATTERY>,<iso8601>` |
| 0x06 | `HB,<uptime_s>,0,<free_heap>,<bat_pct>` (every 60 s) |

CSV row schema (11 columns):
```
timestamp,distance_cm,depth_cm,outlier,event,lat,lon,alt_m,speed_kmh,sats,hdop
```

---

## 6. Battery monitor

- GPIO35 on the uPesy LP board has a factory-wired resistive divider
- Formula: `V_bat = 1.435 * (raw / 4095) * 3.3`
- SoC: linear interpolation, 3.30 V = 0%, 4.20 V = 100%
- When USB-C is connected, the divider reads V_BUS (>4.30 V) and the dashboard shows "On USB" — battery voltage is not readable while on USB
- Time-remaining estimate: records SoC once per minute, linear regression over 60 samples, resets when USB connected

---

## 7. REST API summary

| Method | Path | Notes |
|---|---|---|
| GET | `/api/status` | Full live snapshot including LoRa counters and battery |
| GET | `/api/ultrasonic/live` | Lightweight feed (distance, depth, freshness, frame count) — used by the front page at 5 Hz for instant readouts |
| GET/POST | `/api/settings` | All settings; LoRa POST triggers immediate module reconfiguration |
| POST | `/api/tare` | Tare from live reading |
| POST | `/api/tare/manual` | Body `{"value": 240.5}` |
| POST | `/api/tare/reset` | Reset to 0 |
| POST | `/api/time` | Body `{"epoch": 1746362096}` |
| GET | `/api/sd/info` | SD space + active file |
| GET | `/api/sd/list` | All CSV files |
| GET | `/api/sd/download?file=/snowlog.csv` | Download CSV |
| GET | `/api/sd/preview?file=/snowlog.csv&lines=50` | Grid preview |
| POST | `/api/sd/clear` | Clear active log (never touches /falllog.csv) |
| GET | `/api/imu/events` | Last 32 events |
| POST | `/api/imu/reset_peak` | Zero peak tracker |
| GET | `/tiles?z=&x=&y=` | Serve map tile from SD |

---

## 8. Troubleshooting

The serial monitor prints a status banner every 5 seconds.

### LoRa

| Symptom | Cause | Fix |
|---|---|---|
| LoRa page badge: "INIT FAILED" | E220 not responding | Check pin wiring table above; verify 4.7 kOhm AUX pull-up; check antenna is fitted |
| `TX abort: AUX never went high` | AUX pull-up missing | Add 4.7 kOhm from GPIO27 to 3.3 V |
| Receiver hears nothing | Channel mismatch | Both sides must use identical channel number |
| Receiver sees garbled bytes | Key mismatch | Both sides must use identical key, or both must have encryption disabled |
| Duty-cycle warning on dashboard | Capture interval too short for EU | Set capture interval >= 30 s or use manual TX trigger >= 60 s |

### Other sensors

| Symptom | Monitor | Fix |
|---|---|---|
| Distance stuck at "-- cm" | `[ US ] WAIT  frames=0` | Verify sensor TX -> GPIO4, 5 V supply present |
| 404 on CSS/JS | — | Run `pio run -t uploadfs` |
| SD not detected | `[ SD ] !! NOT DETECTED !!` | Check wiring + FAT32 format; firmware retries every 30 s |
| GPS no data / dashboard shows "—" | `[ GPS ] chars=0` | Was a UART1 conflict with LoRa (fixed in v0.4.1). Re-flash with the updated firmware. |
| GPS never syncs time | `[ TIME ] source=boot` | Move antenna outdoors; cold start takes 1-2 min |
| IMU shows Offline | `[ IMU ] !! NOT DETECTED !!` | Verify SDA->21, SCL->22, SAO->3.3V, CS->3.3V |
| Battery shows "On USB" | — | Unplug USB cable to read actual SoC (hardware limitation) |
| BMI160 build error: `eiaextensions.h` | Compiler error | Run `rm -rf .pio && pio run -t upload` |

---

## 9. License
