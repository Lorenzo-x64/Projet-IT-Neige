<a id="top"></a>
<h1 align="center">Snow Depth Sensor</h1>
<div align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://github.com/lorenzor0912/Projet-IT-Neige/blob/0a817e1d05e45fb6e63a99a292cdd9ac2ce48b34/ReadMe_IMG/It%20neige.svg" />
    <img src="https://github.com/lorenzor0912/Projet-IT-Neige/blob/0a817e1d05e45fb6e63a99a292cdd9ac2ce48b34/ReadMe_IMG/It%20neige.svg" alt="Main logo" width="400" height="400" />
  </picture>
  <br/><br/>
  <p>
    <img src="https://img.shields.io/badge/Status-Active-22c55e?style=flat-square" alt="Status"/>
    <img src="https://img.shields.io/badge/License-GPL_v3-blue?style=flat-square" alt="License"/>
    <img src="https://img.shields.io/github/last-commit/lorenzor0912/Projet-IT-Neige?style=flat-square&color=blueviolet" alt="Last commit"/>
    <img src="https://img.shields.io/github/stars/lorenzor0912/Projet-IT-Neige?style=flat-square&color=ffd700" alt="Stars"/>
    <img src="https://img.shields.io/github/v/release/lorenzor0912/Projet-IT-Neige?style=flat-square" alt="Release"/>
  </p>
  <p>
    <img src="https://img.shields.io/badge/MCU-ESP32-E7352C?style=flat-square&logo=espressif&logoColor=white" alt="ESP32"/>
    <img src="https://img.shields.io/badge/Firmware-C++-00599C?style=flat-square&logo=cplusplus&logoColor=white" alt="C++"/>
    <img src="https://img.shields.io/badge/Built_with-PlatformIO-FF7F00?style=flat-square&logo=platformio&logoColor=white" alt="PlatformIO"/>
    <img src="https://img.shields.io/badge/CAD-Fusion_360-F36F21?style=flat-square&logo=autodesk&logoColor=white" alt="Fusion 360"/>
    <img src="https://img.shields.io/badge/CAD-NX_Siemens-009999?style=flat-square&logo=siemens&logoColor=white" alt="NX Siemens"/>
  </p>
  <p>
    <img src="https://img.shields.io/badge/Temperature--30°C_to_+50°C-blue?style=flat-square" alt="Temperature"/>
    <img src="https://img.shields.io/badge/Range-7.5m-green?style=flat-square" alt="Range"/>
    <img src="https://img.shields.io/badge/Battery_Life-4_months-orange?style=flat-square" alt="Battery Life"/>
    <img src="https://img.shields.io/badge/Waterproof-IP66-0ea5e9?style=flat-square" alt="IP66"/>
  </p>
<a href="https://lorenzo-x64.github.io/" target="_blank"><img src="https://img.shields.io/badge/%20-View%20Gallery-8B5CF6?style=flat-square&labelColor=6B7280&logo=data:image/svg%2Bxml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIxNiIgaGVpZ2h0PSIxNiIgdmlld0JveD0iMCAwIDI0IDI0IiBmaWxsPSJub25lIiBzdHJva2U9IndoaXRlIiBzdHJva2Utd2lkdGg9IjIiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIgc3Ryb2tlLWxpbmVqb2luPSJyb3VuZCI+PHBhdGggZD0iTTUgN2gxYTIgMiAwIDAgMCAyLTIgMSAxIDAgMCAxIDEtMWg2YTEgMSAwIDAgMSAxIDEgMiAyIDAgMCAwIDIgMmgxYTIgMiAwIDAgMSAyIDJ2OWEyIDIgMCAwIDEtMiAySDVhMiAyIDAgMCAxLTItMlY5YTIgMiAwIDAgMSAyLTJ6Ii8+PGNpcmNsZSBjeD0iMTIiIGN5PSIxMyIgcj0iMyIvPjwvc3ZnPg==" alt="View Gallery"/></a>
  <a href="https://lorenzo-x64.github.io/Snow-Esp-Flasher/" target="_blank"><img src="https://img.shields.io/badge/%20-Online%20Flasher-F59E0B?style=flat-square&labelColor=6B7280&logo=data:image/svg%2Bxml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIxNiIgaGVpZ2h0PSIxNiIgdmlld0JveD0iMCAwIDI0IDI0IiBmaWxsPSJub25lIiBzdHJva2U9IndoaXRlIiBzdHJva2Utd2lkdGg9IjIiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIgc3Ryb2tlLWxpbmVqb2luPSJyb3VuZCI+PHBhdGggZD0iTTQgMTRhMSAxIDAgMCAxLS43OC0xLjYzbDkuOS0xMC4yYS41LjUgMCAwIDEgLjg2LjQ2bC0xLjkyIDYuMDJBMSAxIDAgMCAwIDEzIDEwaDdhMSAxIDAgMCAxIC43OCAxLjYzbC05LjkgMTAuMmEuNS41IDAgMCAxLS44Ni0uNDZsMS45Mi02LjAyQTEgMSAwIDAgMCAxMSAxNHoiLz48L3N2Zz4=" alt="Online Flasher"/></a>
  <a href="https://www.printables.com/model/1731900-esp32-driven-snow-depth-sensor-with-lora-transmiss" target="_blank"><img src="https://img.shields.io/badge/%20-3D%20Files-FA6831?style=flat-square&labelColor=6B7280&logo=printables&logoColor=white" alt="3D Files"/></a>
</div>



## Table of Contents

- [Overview](#overview)
- [Hardware](#hardware)
  - [Main Sensor](#main-sensor)
  - [Microcontroller](#microcontroller)
  - [Communications](#communications)
  - [Storage](#storage)
  - [Power](#power)
- [Software](#software)
- [Installation & Deployment](#installation--deployment)
- [License](#license)

---

## Overview

Autonomous snow depth measurement station designed to operate in harsh climatic conditions (down to -30°C) and to be controlled remotely.

| Feature | Specification |
|---------|---------------|
| Resolution | ±1 cm |
| Operating temperature | -30°C to +50°C |
| Ingress protection | IP66 |
| Battery life | 4 months (theoretical) |
| Measurement frequency | Configurable |
| Connectivity | LoRaWAN |
| Local storage | microSD card |
| Fall Detection | Accelerometer |
| Increased Ultrasonic sensor accuracy | Temp Sensor |
| GUI | Simple To use good looking user interface |



This project ultimately evolved into two versions: a "basic" prototype and an "advanced" one. 

Both share the same goal but differ in complexity and features. Our professor initially specified a simple yet efficient base design: PVC tubing with 3D-printed top and bottom end caps. At first, I wasn’t sold on it. I worried that routing components inside a cylindrical tube would make space management tricky, and the overall design felt a bit too amateurish. That’s why I decided to take it a step further and develop a second version, packing it with more advanced electronic and physical features.

The PVC-based prototype includes only the core features our professor required: GPS, an SD card logger, and the main ultrasonic sensor. The only addition beyond the brief is a simple user interface to control and monitor the device something we added for practicality, even though it wasn't explicitly requested.

The advanced version builds on that foundation. It retains all the base features but adds LoRa transmission for long-range communication, an accelerometer for fall detection, and a temperature sensor. The temperature data isn't just extra information—it's used to apply real-time corrections to the ultrasonic readings, improving accuracy since sound speed varies with air temperature.

And physically, instead of the PVC tube design, this version uses a custom-designed enclosure: more compact, better organized internally, and built to accommodate the extra electronics while staying robust and serviceable.

---

## Hardware

### Main Sensor

**Model**: SEN0313 / A01NYUB

![SEN0313 sensor](https://github.com/lorenzor0912/Projet-IT-Neige/blob/f1702dfe2ce56fabe681698466927644a630968b/ReadMe_IMG/SEN0313.JPG)

<details>
<summary>Detailed technical specifications</summary>

**Characteristics**
- Type: waterproof ultrasonic sensor (IP67)
- Measurement range: 28 cm to 750 cm
- Resolution: 1 cm / accuracy: ±1%
- Detection angle: 70° (with included cone)

**Electrical**
- Supply voltage: 3.3 V to 5 V DC
- Consumption: <15 mA (active) / <5 mA (sleep)
- Interface: UART (9600 bps default)

**Environmental**
- Operating temperature: -15°C to +60°C
- Ingress protection: IP67 (immersion up to 1 m for 30 min)
- Resistance: dust, fog, smoke

**Advantages**
- Direct UART output (no time-of-flight calculation required)
- Removable cone to optimize directivity
- Better penetration than classic HC-SR04 modules
- Flexible supply voltage (3.3 V – 5 V)

**Documentation**
- [Official DFRobot guide](https://www.dfrobot.com/product-1934.html)
- [Datasheet](https://wiki.dfrobot.com/A01NYUB%20Waterproof%20Ultrasonic%20Sensor%20SKU:%20SEN0313)

Compatibility with the JSN-SR04T is currently under evaluation.

</details>

---

### Microcontroller

**Suggested references**: ESP32-DevKitC, ESP32-WROVER (for additional PSRAM), or a dedicated low-power module.

The current choice is the **uPesy ESP32 WROOM Low Power**.

> **Note**: all-in-one modules such as Heltec are **not recommended** — we encountered programming issues with these boards.

---

### Communications

The system uses low-power technologies to maximize battery life.

#### LoRaWAN (primary mode)

- **Module**: Ebyte E220-900T22D
- Very low consumption (~20–50 mA in transmission)
- Long range (>10 km in open terrain)
- No cellular subscription required, ideal for sparse measurements (every 4 h)
- Drawback: requires gateway infrastructure, limited throughput

#### Meshtastic

- Integration under study — interesting due to its mesh topology

---

### Storage

**Solution**: microSD card

- Measurement frequency: variable
- Format: CSV with timestamp + logs from other sensors

---

### Power

**Target battery life: 4 months**

Realistically not achievable without a solar panel and a large battery. Significant firmware optimization is required to minimize power draw.

---

## Software

### The starting point:

We initially started writing the code in the Arduino IDE. But once we decided to integrate a GUI, the project quickly became difficult to manage. The codebase grew too large and complex for the IDE to handle efficiently, so we made the call to start over and rebuild everything from scratch using PlatformIO. That switch turned out to be a game changer it gave us better project structure, faster compile times, and a much smoother development workflow overall.


We even took the liberty of adding a few bonus features just for fun, like a real-time GPS map viewer to track the device’s location. It wasn’t strictly necessary, but it made the prototype much more interactive and satisfying to demo.

Main GUI Features
The interface is hosted on a local Wi-Fi Access Point named “Snow Transceiver” (password-protected). Once your device connects, simply open a browser and navigate to 192.168.4.1 to access the dashboard. The GUI includes:

- Landing Page / Main Dashboard: A live overview displaying real-time snow depth measurements, along with status indicators that show whether all connected sensors and modules are online.

- Ultrasonic Sensor Page: A dedicated control panel for the primary sensor. It includes a tare/calibration function to zero the baseline distance, and is intentionally kept minimal to avoid clutter.

- GPS & Location Page: Real-time latitude, longitude, and altitude readings, along with a movement status indicator. Includes a live, scrollable map with tile rendering for visual tracking

- Accelerometer Dashboard: Live acceleration data in milligrams (mG), with adjustable sensitivity tuning to fine-tune motion or fall-detection thresholds.

- SD Card Logger & Data Management: Continuously logs all sensor data to a CSV file. Features one-click download directly from the GUI, a data preview with outlier/anomaly detection, and extensive logging configuration options.

- Battery Monitor: Displays current charge percentage, estimated remaining runtime, and voltage status for proactive power management.

- LoRa Transmission Settings: Region selection (915 MHz or 868 MHz EU), adjustable TX power (dBm), channel configuration, and additional RF optimization parameters.

- Environmental Sensors: Live temperature and humidity readings for environmental context and real-time ultrasonic compensation.

- System Settings: A centralized configuration hub with deep customization options for thresholds, update intervals, communication parameters, and module-specific tuning across the entire system.



---

## Installation & Deployment

### Bill of materials

| Component | Qty | Unit price | Link |
|-----------|-----|------------|------|
| SEN0313 sensor | 1 | ~€30 | TBD |
| ESP32 module | 1 | ~€5 | TBD |
| LoRa module | 1 | ~€15–25 | TBD |
| SD card | 1 | ~€13 | TBD |
| Battery | 1 | ~€100 | TBD |
| Enclosure (3D printed) | 1 | ~€15 (filament) | To print |
| Sealed connectors | Various | ~€10 | TBD |
| **TOTAL** | | **TBD** | |

---

## License

This project is released under the **GNU General Public License v3.0** — see the [LICENSE](LICENSE) file for details.

---

<div align="center">

### Developed by

<img src="https://github.com/Lorenzo-x64/Snow-Depth-Sensor/blob/f0ce491dcf3d831aa2e25147188997d186dc494c/Img/Sti%20Labs.svg" alt="Sti2D Labs logo" width="600" height="600" />

<p><strong>Thank you!</strong></p>

</div>

<div align="right">
  <a href="#top">↑ Back to top</a>
</div>
