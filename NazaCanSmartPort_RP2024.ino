/*****************************************************************************************************************************
 NazaCanDecoder - S.Port Telemetry Bridge (Raspberry Pi Pico / RP2040)                                  02.Nov.2026   @frittna
 Naza-M V1/V2 via CAN-Bus (MCP2515) to FrSky SmartPort
 ****************************************************************************************************************************

[ Sensoren am Sender (Sensorsuche; Namen bei eigenen IDs im Sender selbst vergeben) ]
    Sensor (phys. ID)   Name     Daten-ID  Intervall  Inhalt
    GPS (ID4)           GPS      0x0800    1000 ms    Lat/Lon (nur bei Fix >= 2D)
                        GAlt     0x0820     500 ms    Höhe = Naza-Barometerhöhe (altBaro), m
                        GSpd     0x0830     500 ms    Geschwindigkeit
                        Hdg      0x0840     500 ms    Kurs
                        Date     0x0850   10000 ms    Datum/Uhrzeit (abschaltbar: GPS_SEND_DATE_TIME in FrSkySportIntervals.h)
    Vario (ID1)         Alt      0x0100     200 ms    Höhe wie GAlt
                        VSpd     0x0110     100 ms    Steigen/Sinken
    RPM (ID5)           RPM      0x0500     500 ms    Flugmodus (Tabelle unten)
                        Tmp1     0x0400    1000 ms    Anzahl Satelliten
                        Tmp2     0x0410    1000 ms    GPS-Fix-Typ (Tabelle unten)
    Acc (0x1B)          AccX/Y/Z 0x0700/10/20  je Abfrage  Lagevektor aus Pitch/Roll, x1000
    Naza-Extra (0x39)   NCh1..4  0x5240/50/60/70  Abfrage  Naza-Eingang je Kanal (NAZA_CHn_INPUT), NAZA_CHn_MODE: 0 aus, 1 = -1000..1000, 2 = 0..1000; aktiv: NCh1 = Gas (Eingang 6)
                        NBat     0x5210     Abfrage   Naza-Akkuspannung, 1/100 V (1260 = 12.60 V)
                        NBec     0x5220     Abfrage   Naza-BEC-Spannung, 1/100 V
                        NHom     0x5230     Abfrage   Entfernung zur Naza-Home in m (0 = unbekannt); mit der Sender-Entfernung vergleichen
    Alle Intervalle zentral in FrSkySportIntervals.h (Library FrskySportTelemetry). Zusatzwerte per SEND_NAZA_* / NAZA_CHn_MODE oben abschaltbar.
    NCh/NBat/NBec haben eigene Daten-IDs, die der Sender nicht kennt: im Sender unter Telemetrie neu suchen und umbenennen, bei NBat/NBec
    Einheit V und Nachkommastellen 2 einstellen.

[ Flugmodus (RPM-Sensor) ]
    0 = MANUAL, 1 = GPS, 2 = FAILSAFE, 3 = ATTI (so im Decoder definiert). Andere Werte (z. B. 5, 7) sind im Decoder nicht benannt
    und müssen durch Beobachtung am Sender zugeordnet werden.

[ GPS-Fix (Tmp2) ]
    0 = kein Fix, 2 = 2D-Fix (Position, keine verlässliche Höhe), 3 = 3D-Fix (Position und Höhe), 4 = DGPS (im Decoder vorgesehen).
    Der Wert 1 kommt im Decoder nicht vor und wird als "kein Fix" (0) behandelt. Lat/Lon/GAlt/Alt werden erst ab Fix >= 2D aktualisiert.

[ Pegelanpassung Pico (3,3 V) <-> MCP2515-Modul (5 V) ]
    Der Pico arbeitet mit 3,3 V (nicht 5-V-tolerant), das übliche MCP2515-Modul (TJA1050) mit 5 V. Ein I2C-MOSFET-Pegelwandler
    ist für 8 MHz SPI zu langsam. Lösung: 74HCT244 (DIL) für die Pico->MCP-Richtung, Spannungsteiler für MCP->Pico.
    74HCT244: Pin 20 = +5 V (mit 100 nF nach GND), Pin 10 = GND, Pins 1 und 19 (/OE) an GND (Ausgänge aktiv).
      Pico GP17 (CS)   -> Pin 2 -> Pin 18 -> MCP2515 CS
      Pico GP18 (SCK)  -> Pin 4 -> Pin 16 -> MCP2515 SCK
      Pico GP19 (MOSI) -> Pin 6 -> Pin 14 -> MCP2515 SI
      Nicht benutzte Eingänge (8, 11, 13, 15, 17 usw.) an GND legen, damit sie nicht schwimmen.
    HCT erkennt 3,3 V sicher als High (TTL-Schwelle) und gibt 5 V aus.
    Rückrichtung (MCP2515 -> Pico), nur Spannungsteiler, kein Puffer:
      MCP2515 SO  -> 1 kOhm in Reihe -> GP16, dahinter 2 kOhm nach GND  (ca. 3,3 V)
      MCP2515 INT -> 1 kOhm in Reihe -> GP20, dahinter 2 kOhm nach GND
    MCP2515 VCC = 5 V, alle Massen verbinden (Pico, HCT244, MCP2515, Empfänger/BEC).

[ Arduino IDE 2.x einrichten ]
    1. Datei > Voreinstellungen > "Zusätzliche Boardverwalter-URLs":
       https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
    2. Boardverwalter: "Raspberry Pi Pico/RP2040" (von Earle F. Philhower, III) installieren (getestet: 6.3.2).
    3. Werkzeuge > Board > "Raspberry Pi Pico/RP2040" > "Raspberry Pi Pico" wählen (nicht "Pico W"/"Pico 2").
    4. Pico mit gedrückter BOOTSEL-Taste anstecken, dann Werkzeuge > Port wählen (COM bzw. UF2-Laufwerk) und hochladen.
       Serial Monitor: 115200 Baud.
    5. Libraries aus libraries_used.zip nach Documents\Arduino\libraries\ entpacken:
       FrskySportTelemetry (angepasst), NazaCanDecoder (angepasst), mcp_can (Standard 1.5.1).
       Die beiden angepassten Libraries wurden für den RP2040 und die aktuelle Arduino IDE umgebaut (die Originale
       lassen sich dort nicht kompilieren). Bitte nicht durch die Originale aus dem Bibliotheksverwalter ersetzen.

[ DJI Naza-M V2 CAN Hub ] -> Nicht die beiden Anschlussarten bzw. Kabel-Pinouts verwechseln!
    Einzelner 4-poliger Micro-Fit-Sonderanschluss:
    ├── Pin 1: +6 V (ROT)      ──────> NICHT VERBINDEN
    ├── Pin 2: CAN_H (ORANGE)  ──────> CAN_H [ MCP2515 Modul ]
    ├── Pin 3: CAN_L (GELB)    ──────> CAN_L [ MCP2515 Modul ]
    └── Pin 4: GND   (SCHWARZ) ──────> GND [ MCP2515 Modul ]
  
    Drei normale 4-polige Zubehörbuchsen (abweichende Pinbelegung):
    ├── Pin 1: +6 V  (ROT)
    ├── Pin 2: GND   (SCHWARZ)
    ├── Pin 3: CAN_H (ORANGE)
    └── Pin 4: CAN_L (GELB)

[ Raspberry Pi Pico (RP2040) ]
    MCP2515: CS GP17, SCK GP18, SI(MOSI) GP19, SO(MISO) GP16, INT GP20
    ├── CAN-Modulversorgung: 5 VCC 
    ├── SO und INT Leitungen müssen über je einen Spannungsteiler auf 3,3 V gewandelt werden (z. B. 1 kOhm in Reihe / 2 kOhm nach GND), oder 3,3-V-Modul Level
    ├── S.Port: GP0 (TX) und GP1 (RX) mit einer Drahtbrücke verbinden; gemeinsame Leitung über 1 kOhm an das S.Port-Signal
    └── Statusanzeige: Onboard-LED des Pico (GP25), keine externe LED nötig

• CAN_H/CAN_L sind differentielle Datenleitungen, keine Versorgung.
• Der einzelne Micro-Fit-Sonderanschluss und die drei normalen Zubehörbuchsen am DJI CAN Bus Hub haben unterschiedliche Pinbelegungen (siehe oben).
• Am verwendeten 4P-Micro-Fit Stecker liegt +6 V auf Pin 1. Nicht mit Pico oder MCP2515 verbinden
• GPS-Anschluss des GPS-Pilzes nicht der gleichen Steckerform verwechseln!
• Alle GND-Anschlüsse (Hub, MCP2515, Pico und Empfänger/BEC) müssen verbunden sein.
• Den Pico über USB oder eine geeignete geregelte Versorgung speisen; Modul-VCC und Logikpegel anhand des konkreten MCP2515-Boards prüfen.
*/


#include <Arduino.h>
#include <math.h>
#include <SPI.h>
#include <mcp_can.h>            // Cory Fowler MCP2515 Library
#include <NazaCanDecoder.h>    
#include <FrSkySportSensor.h>
#include <FrSkySportSensorGps.h>
#include <FrSkySportSensorVario.h>
#include <FrSkySportSensorRpm.h>
#include <FrSkySportTelemetry.h>

#define SPORT_X_SOURCE 0
#define SPORT_X_SIGN 1
#define SPORT_Y_SOURCE 1
#define SPORT_Y_SIGN 1
#define SPORT_Z_SOURCE 2
#define SPORT_Z_SIGN 1
#define ACC_SPORT_SCALE 1000.0f

// Zusatzwerte direkt von der Naza (1 = senden, 0 = aus)
// Steuerkanäle NCh1..NCh4 = Naza-Eingang 1..4 (Reihenfolge wie an der Naza angeschlossen).
// Modus je Kanal: 0 = aus, 1 = -1000..+1000 (Rohwert), 2 = 0..1000 (z.B. für Gas)
#define NAZA_CH1_MODE 2
#define NAZA_CH2_MODE 0
#define NAZA_CH3_MODE 0
#define NAZA_CH4_MODE 0
// Quell-Eingang je Kanal (1..10 = Index im Naza-Datenpaket). Gas liegt bei diesem Aufbau auf Eingang 6.
#define NAZA_CH1_INPUT 6
#define NAZA_CH2_INPUT 2
#define NAZA_CH3_INPUT 3
#define NAZA_CH4_INPUT 4
#define SEND_NAZA_BATTERY 1
#define SEND_NAZA_BEC 1
// NHom: Entfernung (m) von der aktuellen Position zur Naza-Home-Position; 0 = unbekannt (kein Fix / Home nicht gesetzt)
#define SEND_NAZA_HOME 1
// Physische S.Port-ID des Zusatzsensors (nicht mit FAS40/MLVSS usw. am selben Bus doppeln)
#define NAZA_EXTRA_SENSOR_ID 0x39
#define NAZA_CH1_DATA_ID 0x5240
#define NAZA_CH2_DATA_ID 0x5250
#define NAZA_CH3_DATA_ID 0x5260
#define NAZA_CH4_DATA_ID 0x5270
#define NAZA_BAT_DATA_ID 0x5210
#define NAZA_BEC_DATA_ID 0x5220
#define NAZA_HOME_DATA_ID 0x5230
#define NAZA_EXTRA_ANY (NAZA_CH1_MODE || NAZA_CH2_MODE || NAZA_CH3_MODE || NAZA_CH4_MODE || SEND_NAZA_BATTERY || SEND_NAZA_BEC || SEND_NAZA_HOME)

#undef LED_BUILTIN
#if defined(ARDUINO_ARCH_RP2040)
#define CAN_CS_PIN 17
#define CAN_MISO_PIN 16         // SPI0
#define CAN_SCK_PIN 18
#define CAN_MOSI_PIN 19
#define CAN_INT_PIN 20
#define LED_BUILTIN 25          // Onboard-LED des Raspberry Pi Pico
#define SPORT_SERIAL_ID FrSkySportSingleWireSerial::SERIAL_1
#else
#define CAN_CS_PIN 10
#define CAN_INT_PIN 2
#define LED_BUILTIN 9
#define SPORT_SERIAL_ID FrSkySportSingleWireSerial::SOFT_SERIAL_PIN_7
#endif
#define CAN_DEBUG 0
// 1 = kompakte 1-Hz-Statistik per Serial (115200): Frames, Nachrichten gestartet/ok/defekt, EFLG
#define CAN_STATS 0
// 1 = Test ohne S.Port-Ausgabe: LED blitzt nur bei dekodierter Naza-Nachricht
#define CAN_ONLY_TEST 0
// Alle anstehenden Frames abholen: der MCP2515 hat nur 2 RX-Puffer, Frames kommen alle ~400 us.
#define CAN_FRAMES_PER_LOOP 64

static const uint8_t ACC_SENSOR_ID = 0x1B;
static const uint16_t ACC_X_DATA_ID = 0x0700;
static const uint16_t ACC_Y_DATA_ID = 0x0710;
static const uint16_t ACC_Z_DATA_ID = 0x0720;

class FrSkySportSensorAccCustom : public FrSkySportSensor {
public:
  FrSkySportSensorAccCustom()
    : FrSkySportSensor((FrSkySportSensor::SensorId)ACC_SENSOR_ID),
      accXData(0), accYData(0), accZData(0), sendStage(0) {}

  void setData(float accX, float accY, float accZ) {
    accXData = (int32_t)(accX * ACC_SPORT_SCALE);
    accYData = (int32_t)(accY * ACC_SPORT_SCALE);
    accZData = (int32_t)(accZ * ACC_SPORT_SCALE);
  }

  virtual uint16_t send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now) {
    (void)now;
    if (id != sensorId) return 0;
    switch (sendStage) {
      case 0: serial.sendData(ACC_X_DATA_ID, accXData); sendStage = 1; return ACC_X_DATA_ID;
      case 1: serial.sendData(ACC_Y_DATA_ID, accYData); sendStage = 2; return ACC_Y_DATA_ID;
      default: serial.sendData(ACC_Z_DATA_ID, accZData); sendStage = 0; return ACC_Z_DATA_ID;
    }
  }
private:
  int32_t accXData, accYData, accZData;
  uint8_t sendStage;
};

static int32_t scaleNazaChannel(int16_t raw, uint8_t mode) {
  int32_t v = constrain((int32_t)raw, -1000, 1000);
  return (mode == 2) ? (v + 1000) / 2 : v;
}

// Zusatzwerte der Naza: NCh1..4 (Steuerkanäle), NBat/NBec (1/100 V), NHom (m) auf einer eigenen S.Port-ID.
class FrSkySportSensorNazaExtra : public FrSkySportSensor {
public:
  FrSkySportSensorNazaExtra()
    : FrSkySportSensor((FrSkySportSensor::SensorId)NAZA_EXTRA_SENSOR_ID),
      count(0), stage(0) {
#if NAZA_CH1_MODE
    ids[count] = NAZA_CH1_DATA_ID; values[count++] = 0;
#endif
#if NAZA_CH2_MODE
    ids[count] = NAZA_CH2_DATA_ID; values[count++] = 0;
#endif
#if NAZA_CH3_MODE
    ids[count] = NAZA_CH3_DATA_ID; values[count++] = 0;
#endif
#if NAZA_CH4_MODE
    ids[count] = NAZA_CH4_DATA_ID; values[count++] = 0;
#endif
#if SEND_NAZA_BATTERY
    ids[count] = NAZA_BAT_DATA_ID; values[count++] = 0;
#endif
#if SEND_NAZA_BEC
    ids[count] = NAZA_BEC_DATA_ID; values[count++] = 0;
#endif
#if SEND_NAZA_HOME
    ids[count] = NAZA_HOME_DATA_ID; values[count++] = 0;
#endif
  }

  void setData(const int16_t* rc, float batteryV, float becV, int32_t homeDistM) {
    uint8_t i = 0;
    (void)rc; (void)batteryV; (void)becV; (void)homeDistM;
#if NAZA_CH1_MODE
    values[i++] = scaleNazaChannel(rc[0], NAZA_CH1_MODE);
#endif
#if NAZA_CH2_MODE
    values[i++] = scaleNazaChannel(rc[1], NAZA_CH2_MODE);
#endif
#if NAZA_CH3_MODE
    values[i++] = scaleNazaChannel(rc[2], NAZA_CH3_MODE);
#endif
#if NAZA_CH4_MODE
    values[i++] = scaleNazaChannel(rc[3], NAZA_CH4_MODE);
#endif
#if SEND_NAZA_BATTERY
    values[i++] = (int32_t)(batteryV * 100.0f + 0.5f);
#endif
#if SEND_NAZA_BEC
    values[i++] = (int32_t)(becV * 100.0f + 0.5f);
#endif
#if SEND_NAZA_HOME
    values[i++] = homeDistM;
#endif
  }

  virtual uint16_t send(FrSkySportSingleWireSerial& serial, uint8_t id, uint32_t now) {
    (void)now;
    if (id != sensorId || count == 0) return 0;
    const uint16_t dataId = ids[stage];
    serial.sendData(dataId, values[stage]);
    if (++stage >= count) stage = 0;
    return dataId;
  }
private:
  uint16_t ids[7];
  int32_t values[7];
  uint8_t count, stage;
};

FrSkySportSensorGps gpsSensor;
FrSkySportSensorVario varioSensor;
FrSkySportSensorRpm rpmSensor;
FrSkySportSensorAccCustom gyroSensor;
FrSkySportSensorNazaExtra nazaExtraSensor;
FrSkySportTelemetry telemetry;

MCP_CAN CAN(CAN_CS_PIN);
NazaCanDecoder nazaCan;

// Aktivität: ~150 Nachrichten/s würden die LED dauerhaft leuchten lassen. Daher höchstens ein kurzer Blitz alle 500 ms.
static bool activitySeen = false;
static uint32_t lastBlinkStart = 0;
static bool blinkOn = false;
static const uint32_t ACTIVITY_BLINK_ON_MS = 20UL;
static const uint32_t ACTIVITY_BLINK_PERIOD_MS = 500UL;
// Defekte Naza-Nachricht (außer der unbenutzten 0x1001): LED 500 ms durchgehend an, danach 500 ms dunkel, dann wieder Aktivität.
static uint32_t badLedUntil = 0;
static uint32_t badDarkUntil = 0;
static uint16_t lastBadSeen = 0;
static const uint32_t BAD_LED_DURATION = 500UL;
static const uint32_t BAD_DARK_PAUSE = 500UL;
static uint32_t lastCanRetry = 0;
static uint32_t lastCanLedToggle = 0;
static bool canReady = false;
static bool canErrorLedState = false;

// Entfernung (m) zwischen aktueller Position und Naza-Home (Haversine); 0 = unbekannt.
static int32_t homeDistanceMeters() {
  if (nazaCan.getFixType() < 2) return 0;
  const double lat = nazaCan.getLat(), lon = nazaCan.getLon();
  const double hLat = nazaCan.getHomeLat(), hLon = nazaCan.getHomeLon();
  if (!isfinite(lat) || !isfinite(lon) || !isfinite(hLat) || !isfinite(hLon)) return 0;
  if (fabs(lat) > 90.0 || fabs(lon) > 180.0 || fabs(hLat) > 90.0 || fabs(hLon) > 180.0) return 0;
  if (hLat == 0.0 && hLon == 0.0) return 0;
  const double toRad = 0.017453292519943295;
  const double dLat = (lat - hLat) * toRad, dLon = (lon - hLon) * toRad;
  const double a = sin(dLat / 2) * sin(dLat / 2) +
                   cos(hLat * toRad) * cos(lat * toRad) * sin(dLon / 2) * sin(dLon / 2);
  return (int32_t)(6371000.0 * 2.0 * atan2(sqrt(a), sqrt(1.0 - a)) + 0.5);
}

#if CAN_DEBUG
static uint32_t debugWindowStart = 0;
static uint32_t debugRxFrames = 0;
static uint32_t debugReadErrors = 0;
static uint32_t debugId090 = 0;
static uint32_t debugId108 = 0;
static uint32_t debugId7F8 = 0;
static uint32_t debugOtherIds = 0;
static uint32_t debugDecoded1002 = 0;
static uint32_t debugDecoded1003 = 0;
static uint32_t debugDecoded1009 = 0;
static unsigned long debugLastOtherId = 0;
static const uint8_t DEBUG_RAW_SAMPLES_PER_ID = 6;
static uint8_t debugRawCount[3] = {0, 0, 0};
static uint8_t debugRawLen[3][DEBUG_RAW_SAMPLES_PER_ID] = {};
static uint8_t debugRawData[3][DEBUG_RAW_SAMPLES_PER_ID][8] = {};
static bool debugRawDumpPrinted = false;
#endif

#if CAN_STATS
static uint32_t statWindowStart = 0;
static uint16_t statFrames = 0;
static uint16_t statDec = 0;
static uint16_t statLastStart = 0, statLastOk = 0, statLastBad = 0;
static uint16_t statBoth = 0;

// EFLG-Overflow-Bits (RX0OVR/RX1OVR) zurücksetzen, damit sie pro Sekunde aussagekräftig sind.
static void clearCanOverflowFlags() {
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(CAN_CS_PIN, LOW);
  SPI.transfer(0x05);  // BIT MODIFY
  SPI.transfer(0x2D);  // EFLG
  SPI.transfer(0xC0);  // RX1OVR | RX0OVR
  SPI.transfer(0x00);
  digitalWrite(CAN_CS_PIN, HIGH);
  SPI.endTransaction();
}
#endif

static bool initializeCan() {
  if (CAN.begin(MCP_ANY, CAN_1000KBPS, MCP_8MHZ) != CAN_OK) return false;
  return CAN.setMode(MCP_NORMAL) == CAN_OK;
}

void setup() {
#if CAN_DEBUG || CAN_STATS
  Serial.begin(115200);
#endif

#if defined(ARDUINO_ARCH_RP2040)
  SPI.setRX(CAN_MISO_PIN);
  SPI.setTX(CAN_MOSI_PIN);
  SPI.setSCK(CAN_SCK_PIN);
#endif

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); delay(150);
  digitalWrite(LED_BUILTIN, LOW);

  telemetry.begin(SPORT_SERIAL_ID,
#if NAZA_EXTRA_ANY
                  &gpsSensor, &varioSensor, &rpmSensor, &gyroSensor, &nazaExtraSensor);
#else
                  &gpsSensor, &varioSensor, &rpmSensor, &gyroSensor);
#endif

  // MCP_8MHZ muss zum Quarz auf dem konkreten MCP2515-Modul passen.
  pinMode(CAN_INT_PIN, INPUT);
  canReady = initializeCan();
  lastCanRetry = millis();
#if CAN_DEBUG
  debugWindowStart = lastCanRetry;
#endif
}

// Schnelles Lesen: RX STATUS + READ RX BUFFER (löscht RXnIF selbst) statt drei Transaktionen in mcp_can.
static bool fastReadCan(unsigned long *id, unsigned char *len, unsigned char *buf) {
  static bool preferRx1 = false;  // beide Puffer voll: der ältere Frame liegt in RXB1, wenn RXB0 gerade gelesen wurde
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(CAN_CS_PIN, LOW);
  SPI.transfer(0xB0);
  const uint8_t st = SPI.transfer(0);
  digitalWrite(CAN_CS_PIN, HIGH);

  const bool rx0 = st & 0x40;
  const bool rx1 = st & 0x80;
  if (!rx0 && !rx1) { SPI.endTransaction(); return false; }
#if CAN_STATS
  if (rx0 && rx1) statBoth++;
#endif
  const bool useRx1 = rx1 && (!rx0 || preferRx1);
  preferRx1 = rx0 && rx1 && !useRx1;

  digitalWrite(CAN_CS_PIN, LOW);
  SPI.transfer(useRx1 ? 0x94 : 0x90);
  const uint8_t sidh = SPI.transfer(0);
  const uint8_t sidl = SPI.transfer(0);
  SPI.transfer(0);
  SPI.transfer(0);
  uint8_t dlc = SPI.transfer(0) & 0x0F;
  if (dlc > 8) dlc = 8;
  for (uint8_t i = 0; i < 8; i++) {
    const uint8_t b = SPI.transfer(0);
    if (i < dlc) buf[i] = b;
  }
  digitalWrite(CAN_CS_PIN, HIGH);
  SPI.endTransaction();

  *id = ((unsigned long)sidh << 3) | (sidl >> 5);
  *len = dlc;
  return true;
}

static void pollCan(uint8_t maxFrames) {
    for (uint8_t framesRead = 0;
         framesRead < maxFrames && digitalRead(CAN_INT_PIN) == LOW;
         framesRead++) {
      unsigned long rxId = 0;
      unsigned char len = 0;
      unsigned char rxBuf[8] = {0};

      if (!fastReadCan(&rxId, &len, rxBuf)) {
#if CAN_DEBUG
        debugReadErrors++;
#endif
        break;
      }
#if CAN_DEBUG
      debugRxFrames++;
      uint8_t debugCanIdIndex = 0xFF;
      if (rxId == 0x090) {
        debugId090++;
        debugCanIdIndex = 0;
      } else if (rxId == 0x108) {
        debugId108++;
        debugCanIdIndex = 1;
      } else if (rxId == 0x7F8) {
        debugId7F8++;
        debugCanIdIndex = 2;
      }
      else {
        debugOtherIds++;
        debugLastOtherId = rxId;
      }
      if (debugCanIdIndex < 3 &&
          debugRawCount[debugCanIdIndex] < DEBUG_RAW_SAMPLES_PER_ID) {
        uint8_t sampleIndex = debugRawCount[debugCanIdIndex]++;
        debugRawLen[debugCanIdIndex][sampleIndex] = len;
        for (uint8_t byteIndex = 0;
             byteIndex < len && byteIndex < sizeof(debugRawData[0][0]);
             byteIndex++) {
          debugRawData[debugCanIdIndex][sampleIndex][byteIndex] = rxBuf[byteIndex];
        }
      }
#endif
      if (len == 0 || len > sizeof(rxBuf)) continue;

      uint16_t decodedMessage = nazaCan.decode(rxId, rxBuf, len);
      if (nazaCan.statBad != lastBadSeen) {
        lastBadSeen = nazaCan.statBad;
        if (nazaCan.badId != 0x1001) {
          const uint32_t t = millis();
          badLedUntil = t + BAD_LED_DURATION;
          badDarkUntil = badLedUntil + BAD_DARK_PAUSE;
        }
      }
#if CAN_STATS
      statFrames++;
      if (decodedMessage != NAZA_MESSAGE_NONE) statDec++;
#endif
#if CAN_DEBUG
      if (decodedMessage == NAZA_MESSAGE_MSG1002) debugDecoded1002++;
      else if (decodedMessage == NAZA_MESSAGE_MSG1003) debugDecoded1003++;
      else if (decodedMessage == NAZA_MESSAGE_MSG1009) debugDecoded1009++;
#endif
      if (decodedMessage == NAZA_MESSAGE_MSG1002 || decodedMessage == NAZA_MESSAGE_MSG1003) {
        const uint8_t flightMode = (uint8_t)nazaCan.getMode();
        const uint8_t satellites = nazaCan.getNumSat();
        const uint8_t fixType = nazaCan.getFixType();
        // RPM-Datenfeld = Flugmodus; T1 = Satelliten; T2 = Fix-Typ.
        rpmSensor.setData(flightMode, (float)satellites, (float)fixType);

        const double latitude = nazaCan.getLat();
        const double longitude = nazaCan.getLon();
        const double altitude = nazaCan.getAlt();
        const double speed = nazaCan.getSpeed();
        const double heading = nazaCan.getHeading();

        if (fixType >= 2 && isfinite(latitude) && fabs(latitude) <= 90.0 &&
            isfinite(longitude) && fabs(longitude) <= 180.0 &&
            isfinite(altitude) && isfinite(speed) && speed >= 0.0 &&
            isfinite(heading)) {
          gpsSensor.setData((float)latitude, (float)longitude, (float)altitude,
                            (float)speed, (float)heading,
                            nazaCan.getYear(), nazaCan.getMonth(), nazaCan.getDay(),
                            nazaCan.getHour(), nazaCan.getMinute(), nazaCan.getSecond());
          varioSensor.setData((float)altitude, (float)nazaCan.getVsi());
        }

        activitySeen = true;
      } else if (decodedMessage == NAZA_MESSAGE_MSG1009) {
        activitySeen = true;
        const int16_t rc[4] = { (int16_t)nazaCan.getRcIn(NAZA_CH1_INPUT - 1), (int16_t)nazaCan.getRcIn(NAZA_CH2_INPUT - 1),
                                (int16_t)nazaCan.getRcIn(NAZA_CH3_INPUT - 1), (int16_t)nazaCan.getRcIn(NAZA_CH4_INPUT - 1) };
        nazaExtraSensor.setData(rc,
                                nazaCan.getVoltage(), nazaCan.getBecVoltage(),
                                homeDistanceMeters());
        const float nazaPitch = (float)nazaCan.getPitch();
        const float nazaRoll = (float)nazaCan.getRoll();
        if (isfinite(nazaPitch) && isfinite(nazaRoll)) {
          const float pitchRad = nazaPitch * 0.01745329252f;
          const float rollRad = nazaRoll * 0.01745329252f;
          const float gravity[] = {
            -sinf(pitchRad),
            sinf(rollRad) * cosf(pitchRad),
            -cosf(rollRad) * cosf(pitchRad)
          };

          gyroSensor.setData(gravity[SPORT_X_SOURCE] * SPORT_X_SIGN,
                             gravity[SPORT_Y_SOURCE] * SPORT_Y_SIGN,
                             gravity[SPORT_Z_SOURCE] * SPORT_Z_SIGN);
        }
      }
    }
}

// Wird von der angepassten FrSkySportSingleWireSerial nach jedem gesendeten S.Port-Byte
// aufgerufen, damit der MCP2515 (nur 2 RX-Puffer) beim Senden nicht überläuft.
extern "C" void frskyBetweenBytesHook() {
  if (canReady) pollCan(1);
}

void loop() {
  uint32_t now = millis();
  if (!canReady && now - lastCanRetry >= 1000UL) {
    lastCanRetry = now;
    canReady = initializeCan();
  }

  if (canReady) {
    pollCan(CAN_FRAMES_PER_LOOP);
    now = millis();
    if ((int32_t)(now - badLedUntil) < 0) {
      digitalWrite(LED_BUILTIN, HIGH);
      blinkOn = false;
    } else if ((int32_t)(now - badDarkUntil) < 0) {
      digitalWrite(LED_BUILTIN, LOW);
      blinkOn = false;
    } else if (blinkOn) {
      if (now - lastBlinkStart >= ACTIVITY_BLINK_ON_MS) {
        digitalWrite(LED_BUILTIN, LOW);
        blinkOn = false;
      }
    } else if (activitySeen && now - lastBlinkStart >= ACTIVITY_BLINK_PERIOD_MS) {
      digitalWrite(LED_BUILTIN, HIGH);
      lastBlinkStart = now;
      blinkOn = true;
      activitySeen = false;
    } else {
      digitalWrite(LED_BUILTIN, LOW);
    }
  } else if (now - lastCanLedToggle >= 250UL) {
    lastCanLedToggle = now;
    canErrorLedState = !canErrorLedState;
    digitalWrite(LED_BUILTIN, canErrorLedState ? HIGH : LOW);
  }

#if CAN_STATS
  if (now - statWindowStart >= 1000UL) {
    statWindowStart = now;
    const uint16_t s = nazaCan.statStart, o = nazaCan.statOk, b = nazaCan.statBad;
    // Eine Zeile am Stück formatieren und nur senden, wenn der USB-Puffer Platz hat (blockiert sonst den CAN-Empfang).
    char line[160];
    const int n = snprintf(line, sizeof(line),
        "F=%u start=%u ok=%u bad=%u dec=%u lastBad id=0x%X len=%u got=%u tail=%02X%02X%02X%02X%02X%02X%02X%02X EFLG=0x%X both=%u REC=%u TEC=%u\r\n",
        statFrames, (uint16_t)(s - statLastStart), (uint16_t)(o - statLastOk), (uint16_t)(b - statLastBad),
        statDec, nazaCan.badId, nazaCan.badLen, nazaCan.badIdx,
        nazaCan.badTail[0], nazaCan.badTail[1], nazaCan.badTail[2], nazaCan.badTail[3],
        nazaCan.badTail[4], nazaCan.badTail[5], nazaCan.badTail[6], nazaCan.badTail[7],
        CAN.getError(), statBoth, CAN.errorCountRX(), CAN.errorCountTX());
    if (n > 0 && Serial.availableForWrite() >= n) Serial.write((const uint8_t*)line, (size_t)n);
    clearCanOverflowFlags();
    statBoth = 0;
    statLastStart = s; statLastOk = o; statLastBad = b;
    statFrames = 0; statDec = 0;
  }
#endif

#if CAN_DEBUG
  if (now - debugWindowStart >= 1000UL) {
    Serial.print(F("CAN="));
    Serial.print(canReady ? F("ready") : F("init-fail"));
    Serial.print(F(" RX/s="));
    Serial.print(debugRxFrames);
    Serial.print(F(" readErr/s="));
    Serial.print(debugReadErrors);
    Serial.print(F(" IDs 090/108/7F8/other="));
    Serial.print(debugId090);
    Serial.print('/');
    Serial.print(debugId108);
    Serial.print('/');
    Serial.print(debugId7F8);
    Serial.print('/');
    Serial.print(debugOtherIds);
    Serial.print(F(" lastOther=0x"));
    Serial.print(debugLastOtherId, HEX);
    Serial.print(F(" decoded 1002/1003/1009="));
    Serial.print(debugDecoded1002);
    Serial.print('/');
    Serial.print(debugDecoded1003);
    Serial.print('/');
    Serial.println(debugDecoded1009);

    if (!debugRawDumpPrinted &&
        debugRawCount[0] == DEBUG_RAW_SAMPLES_PER_ID &&
        debugRawCount[1] == DEBUG_RAW_SAMPLES_PER_ID &&
        debugRawCount[2] == DEBUG_RAW_SAMPLES_PER_ID) {
      const unsigned long debugRawIds[] = {0x090, 0x108, 0x7F8};
      for (uint8_t idIndex = 0; idIndex < 3; idIndex++) {
        for (uint8_t sampleIndex = 0;
             sampleIndex < DEBUG_RAW_SAMPLES_PER_ID;
             sampleIndex++) {
          Serial.print(F("RAW 0x"));
          Serial.print(debugRawIds[idIndex], HEX);
          Serial.print(F(" len="));
          Serial.print(debugRawLen[idIndex][sampleIndex]);
          Serial.print(':');
          for (uint8_t byteIndex = 0;
               byteIndex < debugRawLen[idIndex][sampleIndex] &&
               byteIndex < sizeof(debugRawData[0][0]);
               byteIndex++) {
            Serial.print(' ');
            if (debugRawData[idIndex][sampleIndex][byteIndex] < 0x10) {
              Serial.print('0');
            }
            Serial.print(debugRawData[idIndex][sampleIndex][byteIndex], HEX);
          }
          Serial.println();
        }
      }
      debugRawDumpPrinted = true;
    }

    debugRxFrames = 0;
    debugReadErrors = 0;
    debugId090 = 0;
    debugId108 = 0;
    debugId7F8 = 0;
    debugOtherIds = 0;
    debugDecoded1002 = 0;
    debugDecoded1003 = 0;
    debugDecoded1009 = 0;
    debugLastOtherId = 0;
    debugWindowStart = now;
  }
#endif

#if !CAN_ONLY_TEST
  telemetry.send();
#endif
}
