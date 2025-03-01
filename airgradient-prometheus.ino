/* select board LOLIN C3 MINI to flash */

#include <mutex>
#include <WiFi.h>
#include <WiFiManager.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <ESPmDNS.h>
#include "PMS.h"

// configuration
const char* device_name = "airgradient";
bool        led_enabled = true; // mutable by button push

// primary sensor variable initialization
float primary_pm_1_0              = 0.0;
float primary_pm_2_5              = 0.0;
float primary_pm_10_0             = 0.0;
float primary_pm_count            = 0.0;
float primary_temperature_c       = 0.0;
float primary_temperature_f       = 0.0;
float primary_relative_humidity   = 0.0;
float primary_absolute_humidity   = 0.0;
long  primary_read_count          = 0;
long  primary_read_errors         = 0;

// secondary sensor variable initialization
float secondary_pm_1_0            = 0.0;
float secondary_pm_2_5            = 0.0;
float secondary_pm_10_0           = 0.0;
float secondary_pm_count          = 0.0;
float secondary_temperature_c     = 0.0;
float secondary_temperature_f     = 0.0;
float secondary_relative_humidity = 0.0;
float secondary_absolute_humidity = 0.0;
long  secondary_read_count        = 0;
long  secondary_read_errors       = 0;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>%DEVICE_NAME%</title>
  </head>
  <body>
    <h1>%DEVICE_NAME%</h1>
    <p>
      <a href='http://%DEVICE_NAME%.local/'>
        http://%DEVICE_NAME%.local/
      </a>
    </p>
    <dl>
        <dt>led enabled</dt>
        <dd>%LED_ENABLED%</dd>
        <dt>ip address</dt>
        <dd>%IP_ADDRESS%</dd>
        <dt>ssid</dt>
        <dd>%SSID%</dd>
        <dt>signal strength</dt>
        <dd>%WIFI_SIGNAL_STRENGTH% dBm</dd>
        <dt>metrics</dt>
        <dd><a href='/metrics'>/metrics</a></dd>
    </dl>
  </body>
</html>
)rawliteral";

const char metrics[] PROGMEM = R"rawliteral(
# TYPE led_enabled gauge
# HINT led_enabled Whether or not the status LED is enabled. It can be turned on and off for visual comfort.
led_enabled{device_type="airgradient-open-air",device_name="%DEVICE_NAME%"} %LED_ENABLED%
#
# TYPE wifi_signal_strength gauge
# HINT wifi_signal_strength Wifi signal strength in dBm
wifi_signal_strength{device_type="airgradient-open-air",device_name="%DEVICE_NAME%"} %WIFI_SIGNAL_STRENGTH%
#
# TYPE particulate_matter_1_0 gauge
# HINT particulate_matter_1_0 Particulate matter 1 micron or less in diameter. Unit is micrograms per cubic meter of air. Measured by the Plantower PMS5003T sensor.
particulate_matter_1_0{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="primary"} %PRIMARY_PM_1_0%
particulate_matter_1_0{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="secondary"} %SECONDARY_PM_1_0%
#
# TYPE particulate_matter_2_5 gauge
# HINT particulate_matter_2_5 Particulate matter 2.5 microns or less in diameter. Unit is micrograms per cubic meter of air. Measured by the Plantower PMS5003T sensor.
particulate_matter_2_5{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="primary"} %PRIMARY_PM_2_5%
particulate_matter_2_5{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="secondary"} %SECONDARY_PM_2_5%
#
# TYPE particulate_matter_10_0 gauge
# HINT particulate_matter_10_0 Particulate matter 10 microns or less in diameter. Unit is micrograms per cubic meter of air. Measured by the Plantower PMS5003T sensor.
particulate_matter_10_0{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="primary"} %PRIMARY_PM_10_0%
particulate_matter_10_0{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="secondary"} %SECONDARY_PM_10_0%
#
# TYPE particulate_matter_count gauge
# HINT particulate_matter_count Particulate matter count. Measured by the Plantower PMS5003T sensor.
particulate_matter_count{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="primary"} %PRIMARY_PM_COUNT%
particulate_matter_count{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="secondary"} %SECONDARY_PM_COUNT%
#
# TYPE temperature_c gauge
# HINT temperature_c Ambient temperature in degree Celsius. Measured by the Plantower PMS5003T sensor.
temperature_c{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="primary"} %PRIMARY_TEMPERATURE_C%
temperature_c{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="secondary"} %SECONDARY_TEMPERATURE_C%
#
# TYPE temperature_f gauge
# HINT temperature_f Ambient temperature in degree Fahrenheit. This value is derived from the ambient temperature measured in degree Celsius. It may contain rounding errors.
temperature_f{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="primary"} %PRIMARY_TEMPERATURE_F%
temperature_f{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="secondary"} %SECONDARY_TEMPERATURE_F%
#
# TYPE relative_humidity gauge
# HINT relative_humidity Relative humidity in percent. Measured by the Plantower PMS5003T sensor.
relative_humidity{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="primary"} %PRIMARY_RELATIVE_HUMIDITY%
relative_humidity{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="secondary"} %SECONDARY_RELATIVE_HUMIDITY%
#
# TYPE absolute_humidity gauge
# HINT absolute_humidity Absolute humidity in gram per cubic meter. Calculated from relative humidity and temperature readings by the Plantower PMS5003T sensor. Approximation providing <= .1 percent error for temperatures between -30 degree celsius and +35 degree celsius.
absolute_humidity{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="primary"} %PRIMARY_ABSOLUTE_HUMIDITY%
absolute_humidity{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="secondary"} %SECONDARY_ABSOLUTE_HUMIDITY%
#
# TYPE read_count counter
# HINT read_count Sensor read count since last boot.
read_count{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="primary"} %PRIMARY_READ_COUNT%
read_count{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="secondary"} %SECONDARY_READ_COUNT%
#
# TYPE read_errors counter
# HINT read_errors Sensor read errors since last boot.
read_errors{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="primary"} %PRIMARY_READ_ERRORS%
read_errors{device_type="airgradient-open-air",device_name="%DEVICE_NAME%",sensor="secondary"} %SECONDARY_READ_ERRORS%
)rawliteral";

std::mutex mtx;
PMS pms1(Serial0);
PMS::DATA data_primary;
PMS pms2(Serial1);
PMS::DATA data_secondary;
AsyncWebServer server(80);

// interrupt handler
// button push changes LED enabled state
// e.g. to disable blinking lights at night time
// happy wife -> happy life :)
void IRAM_ATTR isr() {
  std::lock_guard<std::mutex> lck(mtx);
  led_enabled = !led_enabled;
}

void setup() {
    // led
    pinMode(10, OUTPUT);
    led_on();

    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);
    Serial.println("[boot] starting");
    Serial.printf("[boot] device name: %s\n", device_name);

    Serial.println("[boot] setting up pins");
    // push button
    pinMode(9, INPUT_PULLUP);
    attachInterrupt(9, isr, FALLING);
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);

    Serial.println("[boot] accessing sensors");
    // default hardware serial, PMS connector on the right side of the C3 mini on the Open Air
    Serial0.begin(9600); // intentionally no additional arguments to ensure correct readings at Serial0
    // second hardware serial, PMS connector on the left side of the C3 mini on the Open Air
    Serial1.begin(9600, SERIAL_8N1, 0, 1);

    // give the PMSs some time to start
    Serial.println("[boot] waiting for sensors");
    delay(3000);

    // connect to wifi
    Serial.println("[boot] connecting to wifi");
    WiFi.setHostname(device_name);
    WiFiManager wm;
    wm.setTimeout(300);
    if (!wm.autoConnect(device_name)) {
      Serial.println("[boot] error: failed to connect to wifi");
      goto boot_finished; // run device in offline mode
    }
  
    // set up web server
    Serial.println("[boot] setting up web server");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html, processor);
    });
    server.on("/metrics", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain; version=0.0.4", metrics, processor);
      led_blink(); // blinking led indicates scraping
    });
    server.begin();

    // set up mdns
    Serial.println("[boot] setting up mdns");
    if (!MDNS.begin(device_name)) {
      Serial.println("[boot] error: mdns setup failed");
    } else {
      MDNS.addService("http", "tcp", 80);
      Serial.printf("[boot] http://%s.local/\n", device_name);
    }

boot_finished:
    Serial.printf("[boot] led: %s\n", (led_enabled ? "enabled" : "disabled"));
    Serial.println("[boot] finished");
    led_off();
}

void loop() {
  led_on();
  read_sensors();
  led_off();
  print_sensor_data();
  reset_watchdog();
  delay(5000);
}


void reset_watchdog() {
    digitalWrite(2, HIGH);
    delay(20);
    digitalWrite(2, LOW);
}


void read_sensors() {
  std::lock_guard<std::mutex> lck(mtx);
  // primary sensor
  primary_read_count++;
  if (pms1.readUntil(data_primary, 2000)) {
    primary_pm_1_0 = data_primary.PM_AE_UG_1_0;
    primary_pm_2_5 = data_primary.PM_AE_UG_2_5;
    primary_pm_10_0 = data_primary.PM_AE_UG_10_0;
    primary_pm_count = data_primary.PM_RAW_0_3;
    primary_temperature_c = data_primary.AMB_TMP / 10;
    primary_temperature_f = (primary_temperature_c * 1.8) + 32;
    primary_relative_humidity = data_primary.AMB_HUM / 10;
    primary_absolute_humidity = absolute_humidity(primary_relative_humidity, primary_temperature_c);
  } else {
    primary_read_errors++;
  }
  // secondary sensor
  secondary_read_count++;
  if (pms2.readUntil(data_secondary, 2000)) {
    secondary_pm_1_0 = data_secondary.PM_AE_UG_1_0;
    secondary_pm_2_5 = data_secondary.PM_AE_UG_2_5;
    secondary_pm_10_0 = data_secondary.PM_AE_UG_10_0;
    secondary_pm_count = data_secondary.PM_RAW_0_3;
    secondary_temperature_c = data_secondary.AMB_TMP / 10;
    secondary_temperature_f = (secondary_temperature_c * 1.8) + 32;
    secondary_relative_humidity = data_secondary.AMB_HUM / 10;
    secondary_absolute_humidity = absolute_humidity(secondary_relative_humidity, secondary_temperature_c);
  } else {
    secondary_read_errors++;
  }
}


void print_sensor_data() {
    std::lock_guard<std::mutex> lck(mtx);
    Serial.println("");
    Serial.println("sensor readings ------------------ primary -- secondary");
    Serial.printf("particulate matter 1.0 (µg/m³):   %8.0f     %8.0f\n", primary_pm_1_0, secondary_pm_1_0);
    Serial.printf("particulate matter 2.5 (µg/m³):   %8.0f     %8.0f\n", primary_pm_2_5, secondary_pm_2_5);
    Serial.printf("particulate matter 10.0 (µg/m³):  %8.0f     %8.0f\n", primary_pm_10_0, secondary_pm_10_0);
    Serial.printf("particulate matter count:         %8.0f     %8.0f\n", primary_pm_count, secondary_pm_count);
    Serial.printf("ambient temperature (c):          %8.0f     %8.0f\n", primary_temperature_c, secondary_temperature_c);
    Serial.printf("ambient temperature (f):          %8.0f     %8.0f\n", primary_temperature_f, secondary_temperature_f);
    Serial.printf("relative humidity (%%):            %8.0f     %8.0f\n", primary_relative_humidity, secondary_relative_humidity);
    Serial.printf("absolute humidity (%%):            %8.0f     %8.0f\n", primary_absolute_humidity, secondary_absolute_humidity);
    Serial.printf("sensor read count:                %8d     %8d\n", primary_read_count, secondary_read_count);
    Serial.printf("sensor read errors:               %8d     %8d\n", primary_read_errors, secondary_read_errors);
}


String processor(const String& var){
  std::lock_guard<std::mutex> lck(mtx);

  if (var == "DEVICE_NAME") return String(device_name);
  if (var == "SSID") return String(WiFi.SSID());
  if (var == "IP_ADDRESS") return WiFi.localIP().toString();
  if (var == "WIFI_SIGNAL_STRENGTH") return String(WiFi.RSSI());
  if (var == "LED_ENABLED") return String(led_enabled);

  if (var == "PRIMARY_PM_1_0") return String(primary_pm_1_0, 0);
  if (var == "PRIMARY_PM_2_5") return String(primary_pm_2_5, 0);
  if (var == "PRIMARY_PM_10_0") return String(primary_pm_10_0, 0);
  if (var == "PRIMARY_PM_COUNT") return String(primary_pm_count, 0);
  if (var == "PRIMARY_TEMPERATURE_C") return String(primary_temperature_c, 0);
  if (var == "PRIMARY_TEMPERATURE_F") return String(primary_temperature_f, 0);
  if (var == "PRIMARY_RELATIVE_HUMIDITY") return String(primary_relative_humidity, 0);
  if (var == "PRIMARY_ABSOLUTE_HUMIDITY") return String(primary_absolute_humidity, 0);
  if (var == "PRIMARY_READ_COUNT") return String(primary_read_count);
  if (var == "PRIMARY_READ_ERRORS") return String(primary_read_errors);

  if (var == "SECONDARY_PM_1_0") return String(secondary_pm_1_0, 0);
  if (var == "SECONDARY_PM_2_5") return String(secondary_pm_2_5, 0);
  if (var == "SECONDARY_PM_10_0") return String(secondary_pm_10_0, 0);
  if (var == "SECONDARY_PM_COUNT") return String(secondary_pm_count, 0);
  if (var == "SECONDARY_TEMPERATURE_C") return String(secondary_temperature_c, 0);
  if (var == "SECONDARY_TEMPERATURE_F") return String(secondary_temperature_f, 0);
  if (var == "SECONDARY_RELATIVE_HUMIDITY") return String(secondary_relative_humidity, 0);
  if (var == "SECONDARY_ABSOLUTE_HUMIDITY") return String(secondary_absolute_humidity, 0);
  if (var == "SECONDARY_READ_COUNT") return String(secondary_read_count);
  if (var == "SECONDARY_READ_ERRORS") return String(secondary_read_errors);

  return String();
}

// we do metric here, because science. returning g/m^3
// to get pound per cubic foot we can multiply by 0.000062428 downstream
float absolute_humidity(float relative_humidity, float temperature_c) {
  return (6.112 * pow(2.71828, (17.67 * temperature_c)/(temperature_c +243.5)) * relative_humidity * 2.1674) / (273.15 + temperature_c);
}

void led_on() {
  if (led_enabled) {
    digitalWrite(10, HIGH);
  }
}

void led_off() {
  digitalWrite(10, LOW);
}

// forced short quick blinking
// does not honor led_enabled
// use for debug only
void led_blink() {
  led_on();
  delay(100);
  led_off();
  delay(100);
  led_on();
  delay(100);
  led_off();
  delay(100);
  led_on();
  delay(100);
  led_off();
}
