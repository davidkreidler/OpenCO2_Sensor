/*
   OpenCO2 Sensor using ESP32 and SCD4x

   Arduino board: ESP32S2 Dev Module
   Required Arduino libraries:
   - esp32 waveshare epd
   - Adafruit DotStar
   - Sensirion Core
   - Sensirion I2C SCD4x: https://github.com/Sensirion/arduino-i2c-scd4x
   - WiFiManager: https://github.com/tzapu/WiFiManager
   - ArduinoMqttClient (if MQTT is defined)
*/
#define VERSION "v5.5"

#define HEIGHT_ABOVE_SEA_LEVEL 50             // Berlin
#define TZ_DATA "CET-1CEST,M3.5.0,M10.5.0/3"  // Europe/Berlin time zone from https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#define LIGHT_SLEEP_TIME 500
#define DEEP_SLEEP_TIME 29124
#define DEEP_SLEEP_TIME_NO_DISPLAY_UPDATE DEEP_SLEEP_TIME + 965 // offset for no display update
static unsigned long lastMeasurementTimeMs = 0;

/* Includes display */
#include "DEV_Config.h"
#include "epd_abstraction.h"
#define DISPLAY_POWER GPIO_NUM_45
#define LED_POWER GPIO_NUM_9
#define USB_PRESENT GPIO_NUM_4
#define BATTERY_VOLTAGE GPIO_NUM_5
#define BUTTON GPIO_NUM_0

/* welcome */
#include <EEPROM.h>
#include <Preferences.h>
Preferences preferences;

/* WIFI */
#include <WiFi.h>
#include <WiFiManager.h>
WiFiManager wifiManager;

#define airgradient
#ifdef airgradient
/* use https://github.com/geerlingguy/internet-pi to store values */
#include <WebServer.h>
const int port = 9925;
WebServer server(port);
#endif /* airgradient */

// #define MQTT
#ifdef MQTT
#ifdef airgradient
#error only activate one: MQTT or airgradient
#endif /* airgradient */
#include <ArduinoMqttClient.h>
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
char mqtt_server[40];
char mqtt_port[6];
char api_token[34];

WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
WiFiManagerParameter custom_api_token("apikey", "API token", api_token, 32);
#endif /* MQTT */

/* led */
#include <Adafruit_DotStar.h>
#include <SPI.h>
Adafruit_DotStar strip(1, 40, 39, DOTSTAR_BRG);  // numLEDs, DATAPIN, CLOCKPIN
#include "driver/rtc_io.h"

/* scd4x */
#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
SensirionI2CScd4x scd4x;


#ifndef ARDUINO_USB_MODE
#error This ESP32 SoC has no Native USB interface
#elif ARDUINO_USB_MODE == 1
#error This sketch should be used when USB is in OTG mode and MSC On Boot enabled
#endif
#include "USB.h"
#include <USBMSC.h>
USBMSC usbmsc;

RTC_DATA_ATTR bool USB_ACTIVE = false, initDone = false, BatteryMode = false, comingFromDeepSleep = false;
RTC_DATA_ATTR bool LEDonBattery, LEDonUSB, useSmoothLEDcolor, invertDisplay, useFahrenheit, useWiFi, english;
RTC_DATA_ATTR uint8_t ledbrightness, HWSubRev, font;
RTC_DATA_ATTR float maxBatteryVoltage;

/* TEST_MODE */
RTC_DATA_ATTR bool TEST_MODE;
RTC_DATA_ATTR uint16_t sensorStatus, serial0, serial1, serial2;

RTC_DATA_ATTR uint16_t co2 = 400;
RTC_DATA_ATTR float temperature = 0.0f, humidity = 0.0f;

RTC_DATA_ATTR uint16_t currentIndex = 0;
RTC_DATA_ATTR bool overflow = false;
#define NUM_MEASUREMENTS (24*120)
RTC_DATA_ATTR uint16_t co2measurements[NUM_MEASUREMENTS];             // every 30 sec
RTC_DATA_ATTR tempHumData tempHumMeasurements[NUM_MEASUREMENTS / 4];  // every 2 minutes

/* WIFI */
bool shouldSaveConfig = false;
void saveConfigCallback() {
  shouldSaveConfig = true;
}

#ifdef airgradient
const static byte tblFavicon[] PROGMEM = {
  0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
  0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x30, 0x08, 0x03, 0x00, 0x00, 0x00, 0x60, 0xDC, 0x09,
  0xB5, 0x00, 0x00, 0x00, 0x19, 0x74, 0x45, 0x58, 0x74, 0x53, 0x6F, 0x66, 0x74, 0x77, 0x61, 0x72,
  0x65, 0x00, 0x41, 0x64, 0x6F, 0x62, 0x65, 0x20, 0x49, 0x6D, 0x61, 0x67, 0x65, 0x52, 0x65, 0x61,
  0x64, 0x79, 0x71, 0xC9, 0x65, 0x3C, 0x00, 0x00, 0x00, 0x06, 0x50, 0x4C, 0x54, 0x45, 0x4D, 0x93,
  0x51, 0xFF, 0xFF, 0xFF, 0xF3, 0x08, 0xB3, 0x68, 0x00, 0x00, 0x00, 0x02, 0x74, 0x52, 0x4E, 0x53,
  0xFF, 0x00, 0xE5, 0xB7, 0x30, 0x4A, 0x00, 0x00, 0x00, 0x8B, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA,
  0xDC, 0xD6, 0x3B, 0x12, 0x80, 0x20, 0x0C, 0x45, 0xD1, 0x9B, 0xFD, 0x6F, 0xDA, 0xCE, 0x41, 0xC8,
  0x57, 0x44, 0x1D, 0x28, 0xE1, 0x1D, 0x0A, 0x08, 0x13, 0x10, 0x65, 0x70, 0x0E, 0x65, 0xCD, 0x49,
  0xAB, 0x86, 0x20, 0x3E, 0x10, 0xC2, 0x78, 0x47, 0xC8, 0xE4, 0x5B, 0x41, 0x2A, 0xDF, 0x08, 0x72,
  0xF9, 0x3A, 0x60, 0x04, 0x90, 0x12, 0xF7, 0x01, 0xE4, 0xC4, 0x7B, 0x80, 0x05, 0x80, 0x8F, 0x81,
  0xF9, 0x32, 0x74, 0xA0, 0xEF, 0x62, 0x03, 0xA3, 0xBE, 0x9E, 0x03, 0xD6, 0x9B, 0xDA, 0x1A, 0xD4,
  0x8F, 0xD5, 0xA8, 0x79, 0xAF, 0x34, 0x8A, 0x37, 0xBD, 0x05, 0x28, 0x9F, 0xD2, 0x0A, 0x20, 0x31,
  0x90, 0xDF, 0x03, 0x51, 0xDB, 0x4D, 0xB2, 0x3F, 0x88, 0x33, 0x89, 0xDF, 0xFF, 0xBD, 0x96, 0xD5,
  0xAC, 0x7A, 0x53, 0x53, 0x40, 0x6C, 0x10, 0x7C, 0x1D, 0xFA, 0x89, 0xE8, 0x73, 0xD2, 0xEF, 0x70,
  0x09, 0x1C, 0x02, 0x0C, 0x00, 0xB9, 0xE7, 0x04, 0x19, 0xD7, 0xEF, 0x56, 0xBE, 0x00, 0x00, 0x00,
  0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};
void handleFavicon() {
  server.sendHeader("Content-Encoding", "identity");
  server.send_P(200, "image/png", (const char*)tblFavicon, sizeof(tblFavicon));
}

String GenerateMetrics() {
  String message = "";
  String idString = "{id=\"" + String("Open CO2 Sensor") + "\",mac=\"" + WiFi.macAddress().c_str() + "\"}";

  message += "# HELP rco2 CO2 value, in ppm\n";
  message += "# TYPE rco2 gauge\n";
  message += "rco2";
  message += idString;
  message += String(co2);
  message += "\n";

  message += "# HELP atmp Temperature, in degrees Celsius\n";
  message += "# TYPE atmp gauge\n";
  message += "atmp";
  message += idString;
  message += String(temperature);
  message += "\n";

  message += "# HELP rhum Relative humidity, in percent\n";
  message += "# TYPE rhum gauge\n";
  message += "rhum";
  message += idString;
  message += String(humidity);
  message += "\n";

  return message;
}

String getHexColors(uint16_t co2) {
  int red, green, blue;
  getColor(co2, &red, &green, &blue);
  char hexString[7];
  sprintf(hexString, "%02X%02X%02X", red, green, blue);
  return String(hexString);
}

void HandleRootClient() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  String message = "<!DOCTYPE html>\n <html>\n";
  message += "<head>\n <title>OpenCO2 Sensor</title>\n <link rel='icon' href='/favicon.ico' type='image/png' />\n <meta http-equiv='refresh' content='300'>\n";
  message += "<style> .container { display: flex; gap: 15px; } .rounded-box { font-family: Verdana, Geneva, sans-serif; width: 400px; height: 300px; border-radius: 25px; position: relative; display: flex; flex-direction: column; justify-content: center; font-size: 4em; border: 4px solid grey; } .descr-text { position: absolute; top: 10px; left: 10px; font-size: 0.5em; } .center-text { font-size: 1.5em; text-align: center; } .unit-text { font-size: 0.5em; } </style>";
  message += "</head>\n";

  message += "<script src='https://cdn.plot.ly/plotly-latest.min.js'></script>\n";
  message += "<body style='color: grey; background: black;'>\n";

  message += "<div class='container'><div class='rounded-box' style='background-color:#" + getHexColors(co2) + "; color:'grey';'><div class='descr-text'>CO2</div><div class='center-text'><b>" + String(co2) + "</b><div class='unit-text'>ppm</div></div></div>\n";
  char tempString[6];
  if (useFahrenheit) sprintf(tempString, "%.1f",(temperature * 1.8f) + 32.0f);  // convert to °F
  else               sprintf(tempString, "%.1f", temperature);
  message += "<div class='rounded-box'><div class='descr-text'>Temperature</div><div class='center-text'><b>" + String(tempString) + "</b><div class='unit-text'>";
  message += String(useFahrenheit? "*F": "*C") + "</div></div></div>\n";
  message += "<div class='rounded-box'><div class='descr-text'>Humidity</div><div class='center-text'><b>" + String((int)humidity) + "</b><div class='unit-text'>%</div></div></div></div>\n";
  message += "<div id='CO2Plot' style='width:100%;max-width:1400px'></div>\n";
  message += "<div id='TempHumPlot' style='width:100%;max-width:1400px'></div>\n";
  message += "<script>\n";

  char time[20];
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  strftime(time, 20, "%Y-%m-%dT%H:%M:%S", &timeinfo);
  message += "const endTime = new Date('" + String(time) + "').getTime();\n";

  uint16_t index;
  if (overflow) index = NUM_MEASUREMENTS;
  else          index = currentIndex;
  time_t now = mktime(&timeinfo);
  time_t timestamp = now - index * 30;
  struct tm* timeinfo2 = localtime(&timestamp);
  strftime(time, 20, "%Y-%m-%dT%H:%M:%S", timeinfo2);
  message += "const startTime = new Date('" + String(time) + "').getTime();\n";
  message += "const numPoints = " + String(index) + ";\n";

  message += "function generateValues(start, end, numPoints) { let values = []; let step = (end - start) / (numPoints - 1); for (let i = 0; i < numPoints; i++) { values.push(start + (step * i));} return values;}\n";
  message += "let times = generateValues(startTime, endTime, numPoints).map(time => new Date(time));\n";
  message += "const yValues = [";
  server.sendContent(message);

  const size_t bufferSize = 2048;
  String Buffer, Element;
  Buffer.reserve(bufferSize);
  for (int i = 0; i < index; i++) {
    Element = String(getCO2Measurement(i)) + ",";
    if (Buffer.length() + Element.length() > bufferSize) {
      server.sendContent(Buffer);
      Buffer = "";
   }
   Buffer += Element;
  }
  server.sendContent(Buffer);

  message = "];\n";
  message += "const data = [{x:times, y:yValues, mode:'lines'}];\n";
  message += "const layout = {yaxis: { title: 'CO2 (ppm)'}, title: 'History', plot_bgcolor:'black', paper_bgcolor:'black'};\n";
  message += "Plotly.newPlot('CO2Plot', data, layout);\n";
  server.sendContent(message);

  Buffer = "const y1Values = [";
  index = index / 4.0;
  for (int i = 0; i < index; i++) {
    if (useFahrenheit) sprintf(tempString, "%.1f",(getTempMeasurement(i)/10.0 * 1.8f) + 32.0f);  // convert to °F
    else               sprintf(tempString, "%.1f", getTempMeasurement(i)/10.0);
    Element = String(tempString) + ",";
    if (Buffer.length() + Element.length() > bufferSize) {
      server.sendContent(Buffer);
      Buffer = "";
    }
    Buffer += Element;
  }
  server.sendContent(Buffer);

  Buffer = "];\nconst y2Values = [";
  for (int i = 0; i < index; i++) {
    Element = String(getHumMeasurement(i)) + ",";
    if (Buffer.length() + Element.length() > bufferSize) {
      server.sendContent(Buffer);
      Buffer = "";
    }
    Buffer += Element;
  }
  server.sendContent(Buffer);

  message = "];\nconst numPoints2 = " + String(index) + ";\n";
  message += "let times2 = generateValues(startTime, endTime, numPoints2).map(time => new Date(time));\n";
  message += "const data2 = [{x: times2, y: y1Values, name: 'Temperature', mode:'lines'}, ";
  message += "{x: times2, y: y2Values, name: 'Humidity', yaxis: 'y2', mode:'lines'}];\n";
  message += "const layout2 = { showlegend: false, yaxis: {title: 'Temperature (" + String(useFahrenheit? "*F" : "*C") ;
  message += ")'}, yaxis2: { title: 'Humidity (%)', overlaying: 'y', side: 'right'}, plot_bgcolor:'black', paper_bgcolor:'black'};\n";
  message += "Plotly.newPlot('TempHumPlot', data2, layout2);\n";

  message += "</script>\n</body>\n</html>\n";
  server.sendContent(message);
  server.sendContent("");  // Send finish
}

void HandleRoot() {
  server.send(200, "text/plain", GenerateMetrics());
}

void HandleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/html", message);
}
#endif /* airgradient */

#ifdef MQTT
void saveCredentials() {
  preferences.begin("co2-sensor", false);
  preferences.putString("mqtt_server", custom_mqtt_server.getValue());
  preferences.putString("mqtt_port", custom_mqtt_port.getValue());
  preferences.putString("api_token", custom_api_token.getValue());
  preferences.end();
}

void loadCredentials() {
  preferences.begin("co2-sensor", true);
  String s_mqtt_server = preferences.getString("mqtt_server", "");
  String s_mqtt_port = preferences.getString("mqtt_port", "");
  String s_api_token = preferences.getString("api_token", "");
  preferences.end();

  strcpy(mqtt_server, s_mqtt_server.c_str());
  strcpy(mqtt_port, s_mqtt_port.c_str());
  strcpy(api_token, s_api_token.c_str());
}
#endif /* MQTT */

float getTempOffset() {
  if (useWiFi) return 12.2;
  else return 4.4;
}

void initOnce() {
  initEpdOnce();
  EEPROM.begin(2);  // EEPROM_SIZE

  int welcomeDone = EEPROM.read(0);
  if (welcomeDone != 1) TEST_MODE = true;

  if (TEST_MODE) {
    EEPROM.write(0, 0);  // reset welcome
    EEPROM.write(1, 2);  // write HWSubRev 2
    EEPROM.commit();
    preferences.begin("co2-sensor", false);
    preferences.putFloat("MBV", 3.95);  // default maxBatteryVoltage
    preferences.end();

    digitalWrite(LED_POWER, LOW);  // LED on
    strip.begin();
    strip.setPixelColor(0, 5, 5, 5);  // index, green, red, blue
    strip.show();

    displayInitTestMode();

    scd4x.stopPeriodicMeasurement();
    // scd4x.performFactoryReset();
    // delay(100);
    scd4x.performSelfTest(sensorStatus);
  }

  HWSubRev = EEPROM.read(1);
  preferences.begin("co2-sensor", true);
  maxBatteryVoltage = preferences.getFloat("MBV", 3.95);
  useWiFi = preferences.getBool("WiFi", false);
  LEDonBattery = preferences.getBool("LEDonBattery", false);
  LEDonUSB = preferences.getBool("LEDonUSB", true);
  ledbrightness = preferences.getInt("ledbrightness", 5);
  font = preferences.getInt("font", 0);
  if (font == 2) font = 1; // remove gotham font
  changeFont(font);
  useSmoothLEDcolor = preferences.getBool("useSmoothLEDcolor", true);
  invertDisplay = preferences.getBool("invertDisplay", false);
  useFahrenheit = preferences.getBool("useFahrenheit", false);
  english = preferences.getBool("english", false);
  preferences.end();

  scd4x.stopPeriodicMeasurement();  // stop potentially previously started measurement
  scd4x.getSerialNumber(serial0, serial1, serial2);
  scd4x.setSensorAltitude(HEIGHT_ABOVE_SEA_LEVEL);
  scd4x.setAutomaticSelfCalibration(1);
  scd4x.setTemperatureOffset(getTempOffset());
  scd4x.startPeriodicMeasurement();

  displayInit();
  delay(3000);  // Wait for co2 measurement
  initDone = true;
}

void getColor(uint16_t co2, int* red, int* green, int* blue) {
  *red = 0; *green = 0; *blue = 0;
  if (useSmoothLEDcolor) {
    if (co2 > 2000) {
      *red = 216; *green = 2; *blue = 131;  // magenta
    } else {
      *red = pow((co2 - 400),   2) / 10000;
      *green = -pow((co2 - 400), 2) / 4500 + 255;

      if (*red < 0)     *red = 0;
      if (*red > 255)   *red = 255;
      if (*green < 0)   *green = 0;
      if (*green > 255) *green = 255;
    }
  } else {
    if (co2 < 600) {
      *green = 255;
    } else if (co2 < 800) {
      *red = 60; *green = 200;
    } else if (co2 < 1000) {
      *red = 140; *green = 120;
    } else if (co2 < 1500) {
      *red = 200; *green = 60;
    } else if (co2 < 2000) {
      *red = 255;
    } else {
      *red = 216; *green = 2; *blue = 131;  // magenta
    }
  }
}

void setLED(uint16_t co2) {
  updateBatteryMode();
  if ((BatteryMode && !LEDonBattery)
      || (!BatteryMode && !LEDonUSB)) {
    digitalWrite(LED_POWER, HIGH);  // LED OFF
    strip.clear();
    strip.show();
    return;
  }
  digitalWrite(LED_POWER, LOW);  // LED ON
  delay(10);

  int red, green, blue;
  getColor(co2, &red, &green, &blue);

  red   = (int)(red   * (ledbrightness / 100.0));
  green = (int)(green * (ledbrightness / 100.0));
  blue  = (int)(blue  * (ledbrightness / 100.0));

  strip.setPixelColor(0, green, red, blue);
  strip.show();
}

void lowBatteryMode() {
  scd4x.stopPeriodicMeasurement();
  scd4x.powerDown();
  displayLowBattery();
  gpio_hold_dis(LED_POWER);

  /* Wakeup by usb power */
  rtc_gpio_pullup_dis(USB_PRESENT);
  rtc_gpio_pulldown_dis(USB_PRESENT);
  esp_sleep_enable_ext0_wakeup(USB_PRESENT, 1);
  esp_deep_sleep_start();
}

void goto_deep_sleep(int ms) {
  if (useWiFi) {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    esp_wifi_stop();
    delay(1);
  }

  esp_sleep_enable_timer_wakeup(ms * 1000);                             // periodic measurement every 30 sec - 0.83 sec awake

  /* Wakeup by usb power */
  rtc_gpio_pullup_dis(USB_PRESENT);
  rtc_gpio_pulldown_dis(USB_PRESENT);
  esp_sleep_enable_ext0_wakeup(USB_PRESENT, 1);

  /* Wakeup by IO0 button */
  rtc_gpio_pullup_en(BUTTON);
  rtc_gpio_pulldown_dis(BUTTON);
  esp_sleep_enable_ext1_wakeup((1ULL << BUTTON), ESP_EXT1_WAKEUP_ANY_LOW);

  /* Keep LED enabled */
  if (LEDonBattery) gpio_hold_en(LED_POWER);
  else gpio_hold_dis(LED_POWER);

  /* Keep Display power enabled 
  gpio_hold_en(DISPLAY_POWER);
  gpio_deep_sleep_hold_en();*/

  comingFromDeepSleep = true;
  esp_deep_sleep_start();
}

static void usbEventCallback(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  if (event_base == ARDUINO_USB_EVENTS) {
    switch (event_id) {
      case ARDUINO_USB_STARTED_EVENT:
        USB_ACTIVE = true;
        break;
      case ARDUINO_USB_STOPPED_EVENT:
        USB_ACTIVE = false;
        break;
      case ARDUINO_USB_SUSPEND_EVENT:
        USB_ACTIVE = false;
        break;
      case ARDUINO_USB_RESUME_EVENT:
        USB_ACTIVE = true;
        break;

      default:
        break;
    }
  }
}

void goto_light_sleep(int ms) {
  comingFromDeepSleep = false;

  if (useWiFi || TEST_MODE || USB_ACTIVE) {
    for (int i = 0; i < (ms / 100); i++) {
      if (digitalRead(BUTTON) == 0) {
        handleButtonPress();
        return;
      }
      delay(100);
    }
  } else {
    gpio_wakeup_enable(BUTTON, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();

    esp_sleep_enable_timer_wakeup(ms * 1000);                             // periodic measurement every 5 sec -1.1 sec awake
    esp_light_sleep_start();
  }
}

void updateBatteryMode() {
  BatteryMode = (digitalRead(USB_PRESENT) == LOW);
}

float readBatteryVoltage() {
  // IO5 for voltage divider measurement
  float voltage;
  if (HWSubRev == 2) voltage = (analogRead(BATTERY_VOLTAGE) * 3.33) / 5358.0;
  else voltage = (analogRead(BATTERY_VOLTAGE) * 3.33) / 5084.0 + 0.02;

  if ((voltage > maxBatteryVoltage) && (voltage < 4.2) && (digitalRead(USB_PRESENT) == LOW)) {
    maxBatteryVoltage = voltage;
    preferences.begin("co2-sensor", false);
    preferences.putFloat("MBV", voltage);  // save maxBatteryVoltage
    preferences.end();
  }
  return voltage;
}

uint8_t calcBatteryPercentage(float voltage) {
  voltage += (4.2 - maxBatteryVoltage);  // in field calibration

  if (voltage <= 3.62)
    return 75 * pow((voltage - 3.2), 2.);
  else if (voltage <= 4.19)
    return 2836.9625 * pow(voltage, 4) - 43987.4889 * pow(voltage, 3) + 255233.8134 * pow(voltage, 2) - 656689.7123 * voltage + 632041.7303;
  else
    return 100;
}

void calibrate() {
  /* Only run this, if calibration is needed!
   let the Sensor run outside for 3+ minutes before.
 */
  displayCalibrationWarning();
  delay(500);
  for (int i = 0; i < 180; i++) {
    if (digitalRead(BUTTON) == 0) return;  // abort
    delay(1000);
  }

  scd4x.stopPeriodicMeasurement();
  delay(500);
  uint16_t frcCorrection;
  scd4x.performForcedRecalibration((uint16_t)420, frcCorrection);
  delay(400);
  ESP.restart();
}

#include "pictures.h"
void rainbowMode() {
  displayImage(gImage_rainbow);  // gImage_santa
  digitalWrite(LED_POWER, LOW);  // LED ON

  // Santa
  /*for(int j = 0; j < 256; j++) {
    int red = 0, green = 0, blue = 0;

    if (j < 85) {
      red = ((float)j / 85.0f) * 255.0f;
    } else if (j < 170) {
      green = ((float)(j - 85) / 85.0f) * 255.0f;
    } else if (j < 256) {
      red = ((float)(j - 170) / 85.0f) * 255.0f;
      blue = ((float)(j - 170) / 85.0f) * 255.0f;
      green = ((float)(j - 170) / 85.0f) * 255.0f;
    }

    strip.setPixelColor(0, green, red, blue);
    strip.show();
    if (j == 255) j=0;
    if (digitalRead(BUTTON) == 0) return;
    delay(20);
  }*/

  // Rainbow
  for (int j = 0; j < 256; j++) {
    int red = 1, green = 0, blue = 0;

    if (j < 85) {
      red = ((float)j / 85.0f) * 255.0f;
      blue = 255 - red;
    } else if (j < 170) {
      green = ((float)(j - 85) / 85.0f) * 255.0f;
      red = 255 - green;
    } else if (j < 256) {
      blue = ((float)(j - 170) / 85.0f) * 255.0f;
      green = 255 - blue;
    }

    strip.setPixelColor(0, green, red, blue);
    strip.show();
    if (j == 255) j = 0;
    if (digitalRead(BUTTON) == 0) return;
    delay(20);
  }
}

void saveMeasurement(uint16_t co2, float temperature, float humidity) {
  co2measurements[currentIndex] = co2;
  if (!(currentIndex % 4)) { // every 2 minutes
    tempHumMeasurements[currentIndex / 4].temperature = (uint16_t)(temperature * 10);
    tempHumMeasurements[currentIndex / 4].humidity    = (uint8_t)  humidity;
  }

  currentIndex++;
  if (currentIndex >= NUM_MEASUREMENTS) {
    currentIndex = 0;
    overflow = true;
  }
}

uint16_t getCO2Measurement(uint16_t index) {
  if (!overflow) return co2measurements[index];
  else           return co2measurements[(currentIndex + index) % NUM_MEASUREMENTS];
}
uint16_t getTempMeasurement(uint16_t index) {
  if (!overflow) return tempHumMeasurements[index].temperature;
  else           return tempHumMeasurements[(int)(ceil(currentIndex/4.0) + index) % (NUM_MEASUREMENTS/4)].temperature;
}
uint8_t getHumMeasurement(uint16_t index) {
  if (!overflow) return tempHumMeasurements[index].humidity;
  else           return tempHumMeasurements[(int)(ceil(currentIndex/4.0) + index) % (NUM_MEASUREMENTS/4)].humidity;
}

void handleWiFiChange() {
  scd4x.stopPeriodicMeasurement();
  scd4x.setTemperatureOffset(getTempOffset());
  scd4x.startPeriodicMeasurement();
  if (useWiFi) {
    startWiFi();
  } else {
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    esp_wifi_stop();
    goto_light_sleep(1);  // clear ESP_SLEEP_WAKEUP_GPIO
  }
}

void toggleWiFi() {
  useWiFi = !useWiFi;
  preferences.begin("co2-sensor", false);
  preferences.putBool("WiFi", useWiFi);
  preferences.end();
  displayWiFi(useWiFi);
  if (!BatteryMode) handleWiFiChange();
  delay(500);

  bool ip_shown = false;
  while (digitalRead(BUTTON) != 0) {  // wait for button press
    delay(100);
    
    if (useWiFi && !BatteryMode) {
      if (WiFi.status() != WL_CONNECTED) wifiManager.process();
#ifdef airgradient
      if (WiFi.status() == WL_CONNECTED) server.handleClient();
#endif /* airgradient */
    }

    if (!ip_shown && WiFi.status() == WL_CONNECTED) {
      delay(100);
      ip_shown = true;
      displayWiFi(useWiFi);  // to update displayed IP
    }
    if (BatteryMode && (digitalRead(USB_PRESENT) == HIGH)) {  // power got connected
      BatteryMode = false;
      handleWiFiChange();
      displayWiFi(useWiFi);
    }
  }
}

void startWiFi() {
  wifiManager.setSaveConfigCallback([]() {
#ifdef MQTT
    saveCredentials();
#endif
  });

  wifiManager.setSaveConfigCallback(saveConfigCallback);
#ifdef MQTT
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_api_token);
#endif /* MQTT */

  WiFi.setHostname("OpenCO2");  // hostname when connected to home network
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setClass("invert"); // dark theme
  wifiManager.setWiFiAutoReconnect(true);
  wifiManager.autoConnect("OpenCO2 Sensor");  // name of broadcasted SSID

#ifdef MQTT
  loadCredentials();
  if (mqtt_server[0] != '\0' && mqtt_port[0] != '\0') {
    mqttClient.connect(mqtt_server, (int)mqtt_port);
  }
#endif /* MQTT */

#ifdef airgradient
  server.on("/", HandleRootClient);
  server.on("/metrics", HandleRoot);
  server.on("/favicon.ico", handleFavicon);
  server.onNotFound(HandleNotFound);
  server.begin();
  Serial.println("HTTP server started at ip " + WiFi.localIP().toString() + ":" + String(port));
#endif /* airgradient */

  configTime(0, 0, "pool.ntp.org");
  setenv("TZ", TZ_DATA, 1);
  tzset();
}

float currentTemp = temperatureRead();
RTC_DATA_ATTR float ESP32temps[10] = { currentTemp, currentTemp, currentTemp, currentTemp, currentTemp, currentTemp, currentTemp, currentTemp, currentTemp, currentTemp };
RTC_DATA_ATTR float sumTemp = currentTemp * 10;
RTC_DATA_ATTR uint8_t indexTemp = 0;
void measureESP32temperature() {
  currentTemp = temperatureRead();
  sumTemp -= ESP32temps[indexTemp];
  ESP32temps[indexTemp] = currentTemp;
  sumTemp += currentTemp;
  indexTemp = (indexTemp + 1) % 10;
}

void setup() {
  pinMode(DISPLAY_POWER, OUTPUT);
  pinMode(LED_POWER, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  digitalWrite(DISPLAY_POWER, HIGH);
  DEV_Module_Init();

  /* scd4x */
  Wire.begin(33, 34);  // green, yellow
  scd4x.begin(Wire);

  USB.onEvent(usbEventCallback);
  usbmsc.isWritable(true);
  if (!initDone) initOnce();

#if ARDUINO_USB_CDC_ON_BOOT && !ARDUINO_USB_MODE
  if (TEST_MODE) Serial.begin(115200);
#endif

  /* power */
  pinMode(USB_PRESENT, INPUT);
  pinMode(BATTERY_VOLTAGE, INPUT);
  updateBatteryMode();

  strip.begin();
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1) {
    if (TEST_MODE) displayWelcome();  // exit TEST_MODE via IO button
    handleButtonPress();
  }

  if (!BatteryMode && comingFromDeepSleep) {
    delay(1);
    setLED(co2);

    scd4x.stopPeriodicMeasurement();  // stop low power measurement
    scd4x.setTemperatureOffset(getTempOffset());
    scd4x.startPeriodicMeasurement();
    /* Wait for co2 measurement */
    delay(5000);
  }

  if (useWiFi && !BatteryMode) startWiFi();
}


void loop() {
  if (!useWiFi && esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO) handleButtonPress();
  updateBatteryMode();  // check again in USB Power mode
  measureESP32temperature();

  if (useWiFi && !BatteryMode) {
    if (WiFi.status() != WL_CONNECTED) {
      wifiManager.autoConnect("OpenCO2 Sensor"); // Attempt to reconnect
      wifiManager.process();
    }
#ifdef airgradient
    if (WiFi.status() == WL_CONNECTED) server.handleClient();
#endif /* airgradient */
  }

  // force 5 seconds measurement Interval when not on Battery
  if (!BatteryMode && !comingFromDeepSleep && (millis() - lastMeasurementTimeMs < 5000)) {
    if (useWiFi) {
      if (millis() - lastMeasurementTimeMs > LIGHT_SLEEP_TIME) {
        goto_light_sleep(LIGHT_SLEEP_TIME);
        return;  // otherwise continues running!
      }
    }
    goto_light_sleep(5000 - (millis() - lastMeasurementTimeMs));
  }

  bool isDataReady = false;
  uint16_t ready_error = scd4x.getDataReadyFlag(isDataReady);
  if (ready_error || !isDataReady) {
    if (BatteryMode && comingFromDeepSleep) goto_deep_sleep(DEEP_SLEEP_TIME/2);
    else goto_light_sleep(LIGHT_SLEEP_TIME/2);
    return;  // otherwise continues running!
  }

  // Read co2 measurement
  uint16_t new_co2 = 400;
  float new_temperature = 0.0f;
  uint16_t error = scd4x.readMeasurement(new_co2, new_temperature, humidity);
  lastMeasurementTimeMs = millis();
  if (error) {
    char errorMessage[256];
    errorToString(error, errorMessage, 256);
    displayWriteError(errorMessage);
  } else {
    extern uint16_t refreshes;
    if (BatteryMode || (refreshes % 6 == 1)) saveMeasurement(new_co2, new_temperature, humidity);
    /* don't update in Battery mode, unless CO2 has changed by 3% or temperature by 0.5°C */
    if (!TEST_MODE && BatteryMode && comingFromDeepSleep) {
      if ((abs(new_co2 - co2) < (0.03 * co2)) && (fabs(new_temperature - temperature) < 0.5)) {
        goto_deep_sleep(DEEP_SLEEP_TIME_NO_DISPLAY_UPDATE);
      }
    }

    if (new_co2 > 400) co2 = new_co2;
    temperature = new_temperature;
    setLED(co2);
    displayWriteMeasuerments(co2, temperature, humidity);
  }

#ifdef MQTT
  if (!error && !BatteryMode && useWiFi && WiFi.status() == WL_CONNECTED) {
    mqttClient.beginMessage("co2_ppm");
    mqttClient.print(co2);
    mqttClient.endMessage();
    mqttClient.beginMessage("temperature");
    mqttClient.print(temperature);
    mqttClient.endMessage();
    mqttClient.beginMessage("humidity");
    mqttClient.print(humidity);
    mqttClient.endMessage();
  }
#endif /* MQTT */

  if (TEST_MODE) {
#if ARDUINO_USB_CDC_ON_BOOT && !ARDUINO_USB_MODE
    Serial.print(co2);
    Serial.print('\t');
    Serial.print(temperature);
    Serial.print('\t');
    Serial.print(humidity);
    Serial.print('\t');
#endif
    displayWriteTestResults(readBatteryVoltage(), sensorStatus);
  } else {
    /* Print Battery % */
    if (BatteryMode) {
      float voltage = readBatteryVoltage();
      if (voltage < 3.2) lowBatteryMode();
      displayBattery(calcBatteryPercentage(voltage));
    } else if (useWiFi) {
      displayWiFiStrengh();
    }
  }

  updateDisplay();

  if (BatteryMode) {
    if (!comingFromDeepSleep) {
      scd4x.stopPeriodicMeasurement();
      scd4x.setTemperatureOffset(0.8);
      scd4x.startLowPowerPeriodicMeasurement();
    }
    goto_deep_sleep(DEEP_SLEEP_TIME);
  }

  goto_light_sleep(LIGHT_SLEEP_TIME);
}
