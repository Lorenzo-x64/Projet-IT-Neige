// ============================================================================
//  Config.h  --  central project configuration
//  All pin assignments, network credentials, and tunable constants live here.
//  Change any value in this single file rather than hunting through sources.
// ============================================================================
#pragma once

#include <Arduino.h>

// ----------------------------------------------------------------------------
//  Firmware identification
// ----------------------------------------------------------------------------
#define FW_NAME          "Snow Transceiver"
#define FW_VERSION       "0.4.0"
#define FW_BUILD_DATE    __DATE__ " " __TIME__

// ----------------------------------------------------------------------------
//  Wi-Fi Access Point
// ----------------------------------------------------------------------------
#define WIFI_AP_SSID     "Snow Transceiver"
#define WIFI_AP_PASSWORD "1234567890"          // WPA2, must be 8..63 chars
#define WIFI_AP_CHANNEL  1
#define WIFI_AP_IP       IPAddress(192, 168, 4, 1)
#define WIFI_AP_GATEWAY  IPAddress(192, 168, 4, 1)
#define WIFI_AP_SUBNET   IPAddress(255, 255, 255, 0)

// ----------------------------------------------------------------------------
//  Ultrasonic sensor  (DFRobot SEN0313 / A01NYUB)
//  - UART, 9600 8N1, 4-byte frames: [0xFF | DATA_H | DATA_L | SUM]
//  - Distance (mm) = (DATA_H << 8) | DATA_L     -> divide by 10 for cm
//  - Operating range: 28 cm .. 750 cm (datasheet)
//  - Response time : ~100 ms
// ----------------------------------------------------------------------------
#define US_RX_PIN              4              // GPIO connected to sensor TX
#define US_BAUD                9600
#define US_DIST_MIN_CM         28.0f          // datasheet blind zone
#define US_DIST_MAX_CM         750.0f         // datasheet maximum
#define US_TIMEOUT_MS          2000           // mark sensor stale if no read

// ----------------------------------------------------------------------------
//  SD card  (generic micro-SD SPI module, 3.3 V)
//  Pins follow the user's tested wiring. The module exposes 6 pads:
//    GND, 3V3, CS, MOSI, MISO, CLK
// ----------------------------------------------------------------------------
#define SD_CS_PIN              5
#define SD_SCK_PIN             18
#define SD_MISO_PIN            19
#define SD_MOSI_PIN            23
#define SD_LOG_FILE            "/snowlog.csv"
#define SD_FULL_THRESHOLD_PCT  90.0f
#define SD_FULL_CHECK_INTERVAL 300000UL       // 5 min between space checks

// ----------------------------------------------------------------------------
//  Logging / sampling
// ----------------------------------------------------------------------------
#define DEFAULT_SAMPLE_INTERVAL_S   10        // user-configurable from UI
#define MIN_SAMPLE_INTERVAL_S       1
#define MAX_SAMPLE_INTERVAL_S       172800    // 2 days

// ----------------------------------------------------------------------------
//  CSV rotation strategies
//
//      None    -> single file (legacy behaviour)
//      Daily   -> snowlog-YYYY-MM-DD.csv
//      Weekly  -> snowlog-YYYY-Www.csv     (ISO week)
//      Monthly -> snowlog-YYYY-MM.csv
//      Rows    -> snowlog-NNNNNN.csv       (rolls every N rows)
// ----------------------------------------------------------------------------
#define DEFAULT_ROTATION            0         // 0=none 1=daily 2=weekly 3=monthly 4=rows
                                              // Default: single indefinite snowlog.csv.
                                              // All five modes selectable in Settings -> CSV Rotation.
#define DEFAULT_ROTATE_ROWS         10000

// ----------------------------------------------------------------------------
//  Deep sleep
//
//  When enabled, the device powers down between samples and wakes via the
//  ESP32 RTC timer.  WiFi/AsyncTCP/HTTP all stop while sleeping -- the
//  dashboard becomes unreachable until the next sample window.
//
//  We refuse to enable deep sleep below this minimum interval because the
//  WiFi stack takes ~1.5 s to come up after wake, eating most of the savings
//  for short intervals.
// ----------------------------------------------------------------------------
#define DEFAULT_DEEP_SLEEP_ENABLED  false
#define DEEP_SLEEP_MIN_INTERVAL_S   30        // refuse deep sleep below this
#define DEEP_SLEEP_AWAKE_WINDOW_MS  20000UL   // stay awake N ms each cycle to
                                              // serve the dashboard before
                                              // going back to sleep

// ----------------------------------------------------------------------------
//  Outlier detection (the "strange value" highlighter)
//
//  A reading is flagged when EITHER:
//      - it differs from the previous reading by more than JUMP_CM cm,
//        AND the relative change is greater than JUMP_RATIO
//      - it sits more than SIGMA standard deviations from the rolling mean
//        of the last WINDOW samples
//
//  Both checks suppress false positives at startup until WINDOW samples
//  have accumulated.
// ----------------------------------------------------------------------------
#define OUTLIER_WINDOW              16
#define OUTLIER_SIGMA               3.0f
#define OUTLIER_JUMP_CM             50.0f
#define OUTLIER_JUMP_RATIO          5.0f

// ----------------------------------------------------------------------------
//  GPS  (u-blox NEO-7M / Velleman VMA430-WPI430)
//      9600 8N1 NMEA, 3.3 V power.
//      Module runs on EspSoftwareSerial.  All three HW UARTs are taken:
//      UART0=USB, UART1=LoRa, UART2=ultrasonic (primary sensor, reserved).
//      GPS tolerates this because TinyGPSPlus checksums every NMEA sentence
//      and a dropped one is simply replaced by the next one ~1 s later.
// ----------------------------------------------------------------------------
#define GPS_RX_PIN                  16        // ESP32 RX <- GPS TX
#define GPS_TX_PIN                  17        // ESP32 TX -> GPS RX
#define GPS_BAUD                    9600
#define GPS_TZ_OFFSET_H             0         // keep UTC; TZ done at display

// ----------------------------------------------------------------------------
//  IMU / Accelerometer  (BMI160 6-DoF)
//
//  Wiring (all on the I2C bus -- 3.3 V only, NOT 5 V):
//      VIN -> 3.3 V        (some breakouts also expose 3V3 directly)
//      GND -> GND
//      SDA -> GPIO21       (default ESP32 I2C SDA)
//      SCL -> GPIO22       (default ESP32 I2C SCL)
//      SAO -> 3.3 V        (selects address 0x69; tie to GND for 0x68)
//      CS  -> 3.3 V        (forces I2C mode -- some boards have a pull-up)
//      INT1, INT2, SCX, SDX, OCS  --  not connected
//
//  Detection thresholds and cooldown are user-adjustable from the dashboard
//  (saved to NVS via AppSettings).  The values below are the *defaults*
//  used on a freshly flashed device.
// ----------------------------------------------------------------------------
#define IMU_SDA_PIN                 21
#define IMU_SCL_PIN                 22
#define IMU_I2C_ADDR                0x69      // SAO=HIGH; use 0x68 if SAO=GND
#define IMU_I2C_FREQ_HZ             400000UL  // fast-mode I2C
#define IMU_ACCEL_LSB_PER_G         16384.0f  // ±2g range -> 16384 LSB/g

#define DEFAULT_FALL_THRESH_G       0.30f     // mag < this for window -> fall
#define DEFAULT_IMPACT_THRESH_G     3.50f     // mag > this -> impact
#define DEFAULT_FALL_WINDOW_MS      150       // sustained free-fall duration
#define DEFAULT_ALERT_COOLDOWN_MS   10000UL   // min time between alerts
#define DEFAULT_ACCEL_LOG_THRESH_G  2.00f     // mag > this -> in-memory ringbuf

// Valid setting ranges (clamped on the way in)
#define MIN_FALL_THRESH_G           0.05f
#define MAX_FALL_THRESH_G           1.00f
#define MIN_IMPACT_THRESH_G         1.50f
#define MAX_IMPACT_THRESH_G         16.0f     // ±2g range maxes near 2.83g per-axis
                                              // but mag can climb above with quick rotations
#define MIN_FALL_WINDOW_MS          50
#define MAX_FALL_WINDOW_MS          2000
#define MIN_ALERT_COOLDOWN_MS       1000UL
#define MAX_ALERT_COOLDOWN_MS       300000UL

// In-memory ring buffer for recent events (served via /api/imu/events).
// Disk side is unbounded (appends to /falllog.csv).
#define IMU_EVENT_RING_SIZE         32

// ----------------------------------------------------------------------------
//  LoRa transmitter  (Ebyte E220-900T22D, LLCC68, 850-930 MHz)
//
//  This is a TRANSMITTER ONLY in our firmware -- it pushes CSV rows and
//  alert frames to a remote receiver.  The receiver firmware lives in a
//  separate project (the user maintains it), and uses the same E220 module
//  paired with another ESP32.
//
//  Wiring (all 3.3V logic; the E220 itself is 5V-tolerant on power but
//  3.3V on UART):
//      VCC -> 3.3 V         (or 5 V if the breakout has its own LDO)
//      GND -> GND
//      M0  -> GPIO25        (mode-select bit 0, ESP32 output)
//      M1  -> GPIO26        (mode-select bit 1, ESP32 output)
//      RXD -> GPIO33        (ESP32 TX -> LoRa RX)
//      TXD -> GPIO32        (LoRa TX -> ESP32 RX)
//      AUX -> GPIO27        (busy/ready, ESP32 input, 4k7 pull-up
//                            recommended on long jumper-wire setups)
//
//  We use UART1 here so UART2 stays dedicated to the ultrasonic sensor and
//  UART0 stays free for USB programming/console.
// ----------------------------------------------------------------------------
#define LORA_M0_PIN                 25
#define LORA_M1_PIN                 26
#define LORA_AUX_PIN                27
#define LORA_RX_PIN                 32        // ESP32 RX <- E220 TX
#define LORA_TX_PIN                 33        // ESP32 TX -> E220 RX

// E220 channel math: actual freq = 850.125 + CH * 1 MHz.  Module supports
// channels 0..80 (datasheet section 7.2 / "Channel Control").
#define LORA_FREQ_BASE_MHZ          850.125f
#define LORA_CHANNEL_MAX            80

// Regional defaults (user-toggleable in the dashboard)
#define LORA_REGION_EU              0     // 868 MHz ISM
#define LORA_REGION_US              1     // 915 MHz ISM
#define LORA_DEFAULT_REGION         LORA_REGION_EU
#define LORA_DEFAULT_CHANNEL_EU     18    // 868.125 MHz
#define LORA_DEFAULT_CHANNEL_US     65    // 915.125 MHz

// Air data rate -- 2.4 kbps gives the longest range and best link budget;
// higher rates are faster but reach less far.  See datasheet 7.2/REG0.
#define LORA_AIR_RATE_DEFAULT       0     // 0=2.4k 1=2.4k 2=2.4k 3=4.8k
                                          // 4=9.6k 5=19.2k 6=38.4k 7=62.5k
                                          // (low values are valid -- module
                                          // treats 0/1/2 as 2.4 kbps)

// TX power: 22 dBm is the max for this module; lower it to comply with
// regional limits if needed (EU: 14 dBm typical; US: 22 dBm allowed).
// 0=22dBm 1=17dBm 2=13dBm 3=10dBm
#define LORA_TX_POWER_DEFAULT       0

// Encryption: built-in 16-bit XOR-style key in the E220 (registers 06H/07H).
// This is obscurity-grade, NOT real cryptography -- but matching keys at
// both ends are required for the receiver to even see the bytes correctly.
#define LORA_DEFAULT_ENCRYPT        false
#define LORA_DEFAULT_KEY            0x0000

// Module-side address+channel filtering (we use broadcast 0xFFFF + a
// shared channel so any receiver on the same channel hears us).
#define LORA_DEFAULT_ADDR_HI        0xFF
#define LORA_DEFAULT_ADDR_LO        0xFF

// TX trigger mode
#define LORA_TX_TRIGGER_FOLLOW      0     // send every snow log row
#define LORA_TX_TRIGGER_MANUAL      1     // send on a separate periodic timer
#define LORA_DEFAULT_TX_TRIGGER     LORA_TX_TRIGGER_FOLLOW
#define LORA_DEFAULT_TX_PERIOD_S    60    // used in manual mode

// Frame protocol -- shared between transmitter (this firmware) and
// receiver (separate project).  See LoRaTx.h for the full protocol spec.
#define LORA_PROTOCOL_VERSION       1

// ----------------------------------------------------------------------------
//  Battery monitor  (uPesy ESP32 WROOM 32E Low Power -- on-board divider)
//
//  The uPesy LP board ships with a 1MΩ / 2.7MΩ voltage divider hardwired
//  between V_BAT and GPIO35 (input-only ADC1 channel, safe with WiFi).
//  The official conversion formula given by uPesy is:
//
//        V_BAT = 1.435 * (raw / 4095) * 3.3
//
//  IMPORTANT: this only reads the *battery* voltage when the board is
//  running off the JST Li-Po port.  When USB-C is connected the board is
//  powered from V_BUS instead and GPIO35 reads V_IN, not V_BAT.  We use
//  this fact to detect "on battery" vs "USB connected" (raw > USB_GUARD).
//
//  ADC1 is used (channel 7 = GPIO35) so it keeps working while WiFi is up.
// ----------------------------------------------------------------------------
#define BATTERY_ADC_PIN             35
#define BATTERY_DIVIDER_RATIO       1.435f
#define BATTERY_VREF                3.30f
#define BATTERY_ADC_RESOLUTION      4095.0f
#define BATTERY_SAMPLES             32        // averaging window per read
#define BATTERY_READ_INTERVAL_MS    2000      // re-sample every N ms

// Li-Po SoC curve corner points (rough but good enough for a UI bar)
#define BATTERY_FULL_V              4.20f     // 100%
#define BATTERY_EMPTY_V             3.30f     //   0%
#define BATTERY_LOW_V               3.50f     // <-- "low battery" warn
#define BATTERY_USB_GUARD_V         4.30f     // raw above this -> assume USB

// ----------------------------------------------------------------------------
//  AHT10 Temperature & Humidity sensor  (I2C, shared bus with BMI160)
//
//  The AHT10 uses I2C address 0x38 — no conflict with BMI160 (0x69).
//  Shares the existing I2C bus: SDA=GPIO21, SCL=GPIO22.  No new pins.
//
//  Wiring:
//      VIN  -> 3.3 V      (AHT10 is 1.8-3.6 V; most breakouts have an LDO)
//      GND  -> GND
//      SDA  -> GPIO21     (same as BMI160 SDA)
//      SCL  -> GPIO22     (same as BMI160 SCL)
//
//  The sensor takes ~80 ms per measurement at the default 8x oversampling.
//  We trigger a new measurement every AHT10_READ_INTERVAL_MS and cache the
//  result; the cached value is stamped into the CSV row at log time.
// ----------------------------------------------------------------------------
#define AHT10_I2C_ADDR              0x38
#define AHT10_READ_INTERVAL_MS      2000      // re-sample every 2 s
#define AHT10_STALE_MS              10000     // "stale" threshold for alerts
#define AHT10_TEMP_MIN_C           -40.0f    // sanity clamps
#define AHT10_TEMP_MAX_C            85.0f
#define AHT10_HUM_MIN_PCT           0.0f
#define AHT10_HUM_MAX_PCT           100.0f

// Web server// ----------------------------------------------------------------------------
#define WEB_PORT               80
#define DNS_PORT               53             // captive portal redirect

// ----------------------------------------------------------------------------
//  NVS namespaces  (Preferences library)
// ----------------------------------------------------------------------------
#define NVS_NS_TARE            "snow_tare"
#define NVS_NS_SETTINGS        "snow_set"
#define NVS_NS_TIME            "snow_time"

// ----------------------------------------------------------------------------
//  Debug helpers
// ----------------------------------------------------------------------------
#ifdef DEBUG_SNOW
  #define DBG(tag, ...)   do { Serial.printf("[%-6s] ", tag); Serial.printf(__VA_ARGS__); Serial.println(); } while (0)
#else
  #define DBG(tag, ...)   do { Serial.printf("[%-6s] ", tag); Serial.printf(__VA_ARGS__); Serial.println(); } while (0)
#endif
