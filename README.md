# Naza CAN to FrSky SmartPort (RP2040)

-->Für DEUTSCH runterscrollen! / For GERMAN language scroll down.

This project connects a DJI Naza-M V1/V2 over CAN bus to FrSky S.Port telemetry. It uses a Raspberry Pi Pico (RP2040) and an MCP2515 CAN module.

A Lua graphical telemetry script that works on a QX7 running EdgeTX 2.11 is available in my similar earlier project: [NazaSmartPort_Gyro_v1_Arduino_328P](https://github.com/frittna/NazaSmartPort_Gyro_v1_Arduino_328P).

> **Warning:** The DJI CAN-BUS HUB does not use the same pinout on all connectors. In particular, do not connect the +6 V pin on the single Micro-Fit connector to the Pico or MCP2515. Verify the connector pinout and the power requirements of your specific MCP2515 module before powering the system.

## Project files

- [`NazaCanSmartPort_RP2024.ino`](NazaCanSmartPort_RP2024.ino) – Arduino sketch
- [`libraries_used.zip`](libraries_used.zip) – Arduino libraries used by the project, including RP2040-adapted versions
- [`VERDRAHTUNG.pdf`](VERDRAHTUNG.pdf) – wiring overview
- [`DJI CAN-BUS HUB.png`](<DJI CAN-BUS HUB.png>), [`MCP2515.png`](MCP2515.png), [`RP2040 PINOUT -Raspberry-Pi-Pico.jpg`](<RP2040 PINOUT -Raspberry-Pi-Pico.jpg>) – component and pinout references

## Requirements

- Raspberry Pi Pico with RP2040 (not Pico W or Pico 2)
- DJI Naza-M V1/V2 and DJI CAN-BUS HUB
- MCP2515 module with an 8 MHz crystal
- FrSky receiver / S.Port connection
- Arduino IDE 2.x
- RP2040 board package by Earle F. Philhower, III (tested with version 6.3.2)

### Install the libraries

1. In Arduino IDE, open **File > Preferences** and add this board manager URL:
   `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`
2. Install **Raspberry Pi Pico/RP2040** by Earle F. Philhower, III using the Boards Manager.
3. Extract `libraries_used.zip` into `Documents\Arduino\libraries\`. It contains:
   - `FrskySportTelemetry` (adapted for RP2040)
   - `NazaCanDecoder` (adapted for RP2040)
   - `mcp_can` (Cory Fowler, version 1.5.1)
4. Do not replace the adapted `FrskySportTelemetry` and `NazaCanDecoder` libraries with unmodified versions from the Library Manager.

### Compile and upload

1. Open `NazaCanSmartPort_RP2024.ino` in Arduino IDE.
2. Select **Raspberry Pi Pico** under **Tools > Board > Raspberry Pi Pico/RP2040**.
3. Connect the Pico over USB while holding **BOOTSEL**, select the appropriate port, and upload the sketch.

The serial monitor is configured for 115200 baud. In the default configuration, it only prints messages when `CAN_DEBUG` or `CAN_STATS` is enabled.

## Wiring

See [`VERDRAHTUNG.pdf`](VERDRAHTUNG.pdf) for the complete wiring overview. Main signal connections:

| Signal | Pico pin | MCP2515 / S.Port |
|---|---:|---|
| CAN CS | GP17 | CS |
| CAN SCK | GP18 | SCK |
| CAN MOSI | GP19 | SI |
| CAN MISO | GP16 | SO, level-shifted to 3.3 V |
| CAN INT | GP20 | INT, level-shifted to 3.3 V |
| S.Port | GP0 (TX) and GP1 (RX) | Connect TX to RX; connect the joined line to S.Port through a 1 kΩ resistor |
| Status | GP25 | Onboard LED |

The Pico operates at 3.3 V and is **not 5 V tolerant**. With a typical 5 V MCP2515/TJA1050 module, buffer the Pico-to-module signals and level-shift SO and INT to 3.3 V for the Pico. The sketch describes a 74HCT244 for buffering and voltage dividers (1 kΩ in series, 2 kΩ to GND) for the return signals. **The image filename `74HCT245N.png` refers to a 74HCT245N, while the sketch names a 74HCT244.** These ICs have different pinouts: check the marking on the fitted IC and use the matching pinout. Do not mix the two pinouts.

Connect all grounds (Pico, level shifter, MCP2515, and receiver/BEC). CAN_H and CAN_L are data lines, not power. Power the Pico via USB or a suitable regulated supply; do not use the DJI hub's +6 V line to power the Pico or MCP2515.

**DJI CAN-BUS HUB:** On the single 4-pin Micro-Fit connector, pin 1 is +6 V (do not connect), pin 2 is CAN_H, pin 3 is CAN_L, and pin 4 is GND. The three standard accessory sockets have a different pinout: pin 1 is +6 V, pin 2 is GND, pin 3 is CAN_H, and pin 4 is CAN_L. Do not confuse the GPS puck connector with the CAN connector.

## Telemetry data

| FrSky sensor | Sensor ID | Data ID | Content |
|---|---:|---:|---|
| GPS | `0x04` | `0x0800` | Latitude/longitude; available from a 2D GPS fix |
| GAlt | `0x04` | `0x0820` | Naza barometric altitude in m |
| GSpd | `0x04` | `0x0830` | Speed |
| Hdg | `0x04` | `0x0840` | Heading |
| Date | `0x04` | `0x0850` | Date/time; 10 s interval, can be disabled in `FrSkySportIntervals.h` |
| Alt | `0x01` | `0x0100` | Altitude, same as GAlt |
| VSpd | `0x01` | `0x0110` | Climb/descent rate |
| RPM | `0x05` | `0x0500` | Flight mode |
| Tmp1 / Tmp2 | `0x05` | `0x0400` / `0x0410` | Satellite count / GPS fix type |
| AccX / AccY / AccZ | `0x1B` | `0x0700` / `0x0710` / `0x0720` | Attitude vector from pitch/roll, scaled by 1000 |
| NCh1–NCh4 | `0x39` | `0x5240` / `0x5250` / `0x5260` / `0x5270` | Naza input channels; only NCh1 is enabled by default |
| NBat / NBec / NHom | `0x39` | `0x5210` / `0x5220` / `0x5230` | Naza battery and BEC voltage in 1/100 V; distance to the home position in m |

The Naza extra sensor data IDs (`0x39`) may not be configured on your transmitter. Run a sensor discovery and rename the values as needed. Set NBat and NBec to unit **V** with two decimal places. Do not use the same physical sensor ID for another sensor on the S.Port bus.

The sketch maps RPM values as `0 = MANUAL`, `1 = GPS`, `2 = FAILSAFE`, and `3 = ATTI`. Other values reported by a particular Naza system are not named and must be identified by observing the transmitter. GPS position and altitude values are updated only from fix type 2.

## Configuration options

Settings are near the top of the sketch:

- `NAZA_CH1_MODE` through `NAZA_CH4_MODE`: `0` disables the channel, `1` sends values from `-1000` to `1000`, and `2` scales values to `0` to `1000`.
- `NAZA_CH1_INPUT` through `NAZA_CH4_INPUT`: Naza input channel (1–10); by default, NCh1 uses input 6 (throttle).
- `SEND_NAZA_BATTERY`, `SEND_NAZA_BEC`, `SEND_NAZA_HOME`: enable or disable extra telemetry values.
- `CAN_DEBUG`, `CAN_STATS`: enable serial CAN diagnostics.

The sketch initializes the MCP2515 at 1 Mbit/s and 8 MHz. The configured crystal frequency must match the module.

## Notes

This project is for telemetry. Check the readings and sensor assignments on the transmitter on the ground before flight. Incorrect power, level shifting, or CAN wiring can damage components.

---

# Naza CAN zu FrSky SmartPort (RP2040)

Dieses Projekt verbindet eine DJI Naza-M V1/V2 über den CAN-Bus mit der FrSky-S.Port-Telemetrie. Als Controller dient ein Raspberry Pi Pico (RP2040), als CAN-Interface ein MCP2515-Modul.

Ein Lua-Skript, das mit einer QX7 unter EdgeTX 2.11 funktioniert, findest du in meinem ähnlichen früheren Projekt: [NazaSmartPort_Gyro_v1_Arduino_328P](https://github.com/frittna/NazaSmartPort_Gyro_v1_Arduino_328P).

> **Achtung:** Die Verdrahtung am DJI CAN-BUS HUB ist nicht an allen Buchsen gleich. Insbesondere darf die +6-V-Leitung des einzelnen Micro-Fit-Anschlusses nicht mit dem Pico oder dem MCP2515 verbunden werden. Vor dem Einschalten die Pinbelegung und die Versorgung des konkreten MCP2515-Moduls prüfen.

## Projektdateien

- [`NazaCanSmartPort_RP2024.ino`](NazaCanSmartPort_RP2024.ino) – Arduino-Sketch
- [`libraries_used.zip`](libraries_used.zip) – verwendete Arduino-Bibliotheken, darunter die für RP2040 angepassten Versionen
- [`VERDRAHTUNG.pdf`](VERDRAHTUNG.pdf) – Verdrahtungsübersicht
- [`DJI CAN-BUS HUB.png`](<DJI CAN-BUS HUB.png>), [`MCP2515.png`](MCP2515.png), [`RP2040 PINOUT -Raspberry-Pi-Pico.jpg`](<RP2040 PINOUT -Raspberry-Pi-Pico.jpg>) – Bauteil- und Pinout-Referenzen

## Voraussetzungen

- Raspberry Pi Pico mit RP2040 (nicht Pico W oder Pico 2)
- DJI Naza-M V1/V2 und DJI CAN-BUS HUB
- MCP2515-Modul mit 8-MHz-Quarz
- FrSky-Empfänger bzw. S.Port-Anschluss
- Arduino IDE 2.x
- RP2040-Boardpaket von Earle F. Philhower, III (im Sketch getestet mit Version 6.3.2)

### Bibliotheken installieren

1. In der Arduino IDE unter **Datei > Voreinstellungen** folgende Boardverwalter-URL ergänzen:
   `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`
2. Im Boardverwalter **Raspberry Pi Pico/RP2040** von Earle F. Philhower, III installieren.
3. `libraries_used.zip` nach `Documents\Arduino\libraries\` entpacken. Es enthält:
   - `FrskySportTelemetry` (für RP2040 angepasst)
   - `NazaCanDecoder` (für RP2040 angepasst)
   - `mcp_can` (Cory Fowler, Version 1.5.1)
4. Die angepassten Bibliotheken `FrskySportTelemetry` und `NazaCanDecoder` nicht durch die unveränderten Versionen aus dem Bibliotheksverwalter ersetzen.

### Kompilieren und Hochladen

1. `NazaCanSmartPort_RP2024.ino` in der Arduino IDE öffnen.
2. Unter **Werkzeuge > Board > Raspberry Pi Pico/RP2040** das Board **Raspberry Pi Pico** wählen.
3. Den Pico mit gedrückter **BOOTSEL**-Taste per USB anschließen, den passenden Port wählen und den Sketch hochladen.

Der serielle Monitor ist auf 115200 Baud ausgelegt, gibt im Standardbetrieb aber nur bei aktiviertem `CAN_DEBUG` oder `CAN_STATS` Meldungen aus.

## Verdrahtung

Die vollständige Übersicht ist in [`VERDRAHTUNG.pdf`](VERDRAHTUNG.pdf) enthalten. Die wesentlichen Signalverbindungen:

| Verbindung | Pico-Pin | MCP2515 / S.Port |
|---|---:|---|
| CAN CS | GP17 | CS |
| CAN SCK | GP18 | SCK |
| CAN MOSI | GP19 | SI |
| CAN MISO | GP16 | SO, über Pegelanpassung auf 3,3 V |
| CAN INT | GP20 | INT, über Pegelanpassung auf 3,3 V |
| S.Port | GP0 (TX) und GP1 (RX) | TX und RX verbinden; gemeinsame Leitung über 1 kΩ an S.Port |
| Status | GP25 | Onboard-LED |

Der Pico arbeitet mit 3,3 V und ist **nicht 5-V-tolerant**. Beim üblichen 5-V-MCP2515/TJA1050-Modul müssen daher die Leitungen vom Pico zum Modul gepuffert und SO sowie INT in Richtung Pico auf 3,3 V angepasst werden. Der Sketch beschreibt dafür einen 74HCT244 und Spannungsteiler (1 kΩ in Reihe, 2 kΩ nach GND). **Der Dateiname `74HCT245N.png` verweist dagegen auf einen 74HCT245N.** Diese ICs haben unterschiedliche Pinbelegungen: Die Beschriftung des verwendeten ICs prüfen und den dazu passenden Anschlussplan verwenden. Die Pinbelegungen nicht verwechseln.

Alle Masseanschlüsse von Pico, Pegelanpassung, MCP2515 und Empfänger/BEC müssen verbunden sein. CAN_H und CAN_L sind Datenleitungen, keine Versorgung. Den Pico über USB oder eine geeignete geregelte Versorgung speisen; die +6 V des DJI-Hubs nicht als Pico- oder MCP2515-Versorgung verwenden.

**DJI CAN-BUS HUB:** Am einzelnen 4-poligen Micro-Fit-Sonderanschluss gilt Pin 1 = +6 V (nicht verbinden), Pin 2 = CAN_H, Pin 3 = CAN_L, Pin 4 = GND. Die drei normalen Zubehörbuchsen haben eine andere Belegung: Pin 1 = +6 V, Pin 2 = GND, Pin 3 = CAN_H, Pin 4 = CAN_L. Auch der GPS-Anschluss des GPS-Pilzes ist nicht mit dem CAN-Anschluss zu verwechseln.

## Übertragene Telemetrie

| FrSky-Sensor | Sensor-ID | Daten-ID | Inhalt |
|---|---:|---:|---|
| GPS | `0x04` | `0x0800` | Breiten-/Längengrad; erst ab GPS-Fix 2D |
| GAlt | `0x04` | `0x0820` | Naza-Barometerhöhe in m |
| GSpd | `0x04` | `0x0830` | Geschwindigkeit |
| Hdg | `0x04` | `0x0840` | Kurs |
| Date | `0x04` | `0x0850` | Datum/Uhrzeit; Intervall 10 s, in `FrSkySportIntervals.h` abschaltbar |
| Alt | `0x01` | `0x0100` | Höhe wie GAlt |
| VSpd | `0x01` | `0x0110` | Steigen/Sinken |
| RPM | `0x05` | `0x0500` | Flugmodus |
| Tmp1 / Tmp2 | `0x05` | `0x0400` / `0x0410` | Satellitenzahl / GPS-Fix-Typ |
| AccX / AccY / AccZ | `0x1B` | `0x0700` / `0x0710` / `0x0720` | Lagevektor aus Pitch/Roll, skaliert mit 1000 |
| NCh1–NCh4 | `0x39` | `0x5240` / `0x5250` / `0x5260` / `0x5270` | Naza-Eingangskanäle; standardmäßig nur NCh1 aktiv |
| NBat / NBec / NHom | `0x39` | `0x5210` / `0x5220` / `0x5230` | Naza-Akku- und BEC-Spannung in 1/100 V; Entfernung zur Home-Position in m |

Die Daten-IDs der Naza-Zusatzwerte (`0x39`) sind nicht bei jedem Sender vorkonfiguriert. Eine Sensorsuche durchführen und die Werte anschließend passend umbenennen. Für NBat und NBec Einheit **V** und zwei Nachkommastellen einstellen. Keine doppelte physische Sensor-ID auf demselben S.Port-Bus verwenden.

Der Sketch definiert `0 = MANUAL`, `1 = GPS`, `2 = FAILSAFE`, `3 = ATTI` für den RPM-Wert. Weitere vom konkreten Naza-System gelieferte Werte sind nicht benannt und müssen am Sender beobachtet werden. Die GPS-Position und Höhenwerte werden erst ab Fix-Typ 2 aktualisiert.

## Anpassbare Optionen

Die Einstellungen befinden sich am Anfang des Sketches:

- `NAZA_CH1_MODE` bis `NAZA_CH4_MODE`: `0` deaktiviert, `1` sendet den Wert von `-1000` bis `1000`, `2` skaliert auf `0` bis `1000`.
- `NAZA_CH1_INPUT` bis `NAZA_CH4_INPUT`: Naza-Eingangskanal (1–10); standardmäßig ist NCh1 auf Eingang 6 (Gas) gelegt.
- `SEND_NAZA_BATTERY`, `SEND_NAZA_BEC`, `SEND_NAZA_HOME`: Zusatzwerte ein- oder ausschalten.
- `CAN_DEBUG`, `CAN_STATS`: serielle CAN-Diagnose aktivieren.

Der MCP2515 wird im Sketch mit 1 Mbit/s und 8 MHz initialisiert. Die Quarzfrequenz muss zum tatsächlich verwendeten Modul passen.

## Hinweise

Dieses Projekt dient der Telemetrieübertragung. Vor einem Flug die Messwerte und Sensorzuordnungen am Sender am Boden prüfen. Eine falsche Versorgung, Pegelanpassung oder CAN-Pinbelegung kann Bauteile beschädigen.
