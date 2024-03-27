/*
   ESP32S2 Dev Module
   Required Arduino libraries:
   - esp32 waveshare epd
   - Adafruit DotStar
   - Sensirion Core
   - Sensirion I2C SCD4x: https://github.com/Sensirion/arduino-i2c-scd4x
   - WiFiManager: https://github.com/tzapu/WiFiManager
*/
#define VERSION "v4.4"

/* Includes display */
#include "DEV_Config.h"
#include "epd_abstraction.h"
#define DISPLAY_POWER 45
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
#include <SPIFFS.h>
const int port = 9925;
WebServer server(port);
#endif /* airgradient */

//#define MQTT
#ifdef MQTT
#ifdef airgradient
#error only activate one: MQTT or airgradient
#endif /*airgradient*/
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
Adafruit_DotStar strip(1, 40, 39, DOTSTAR_BRG); // numLEDs, DATAPIN, CLOCKPIN
#include "driver/rtc_io.h"

/* scd4x */
#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
SensirionI2CScd4x scd4x;


#ifndef ARDUINO_USB_MODE
#error This ESP32 SoC has no Native USB interface
#elif ARDUINO_USB_MODE == 1
#error This sketch should be used when USB is in OTG mode and MSC On Boot enabeled
#endif
#include "USB.h"

RTC_DATA_ATTR bool USB_ACTIVE = false;
RTC_DATA_ATTR bool initDone = false;
RTC_DATA_ATTR bool BatteryMode = false;
RTC_DATA_ATTR bool comingFromDeepSleep = false;
RTC_DATA_ATTR int ledbrightness = 5;
RTC_DATA_ATTR bool LEDonBattery = false;
RTC_DATA_ATTR bool LEDonUSB = true;
RTC_DATA_ATTR bool useSmoothLEDcolor = true;
RTC_DATA_ATTR bool invertDisplay = false;
RTC_DATA_ATTR bool useFahrenheit = false;
RTC_DATA_ATTR int HWSubRev = 1; //default only
RTC_DATA_ATTR float maxBatteryVoltage;
RTC_DATA_ATTR bool useWiFi;
RTC_DATA_ATTR bool english = false; //default only

/* TEST_MODE */
RTC_DATA_ATTR bool TEST_MODE = false;
RTC_DATA_ATTR uint16_t sensorStatus;
RTC_DATA_ATTR uint16_t serial0;
RTC_DATA_ATTR uint16_t serial1;
RTC_DATA_ATTR uint16_t serial2;

RTC_DATA_ATTR uint16_t co2 = 400;
RTC_DATA_ATTR float temperature = 0.0f;
RTC_DATA_ATTR float humidity = 0.0f;

/* WIFI */
bool shouldSaveConfig = false;
void saveConfigCallback() {
  shouldSaveConfig = true;
}

#ifdef airgradient
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

void HandleRoot() {
  server.send(200, "text/plain", GenerateMetrics() );
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
  if (useWiFi) return 13.0;
  else return 4.4;
}

void initOnce() {
  initEpdOnce();
  EEPROM.begin(2); // EEPROM_SIZE

  int welcomeDone = EEPROM.read(0);
  if (welcomeDone != 1) TEST_MODE = true;

  if (TEST_MODE) {
    EEPROM.write(0, 0); //reset welcome
    EEPROM.write(1, 2); //write HWSubRev 2
    EEPROM.commit();
    preferences.begin("co2-sensor", false); 
    preferences.putFloat("MBV", 3.95); //default maxBatteryVoltage
    preferences.end();

    digitalWrite(LED_POWER, LOW); //LED on
    strip.begin();
    strip.setPixelColor(0, 5, 5, 5); //index, green, red, blue
    strip.show();

    displayInitTestMode();

    scd4x.stopPeriodicMeasurement();
    //scd4x.performFactoryReset();
    //delay(100);
    scd4x.performSelfTest(sensorStatus);
  }

  HWSubRev = EEPROM.read(1);
  preferences.begin("co2-sensor", true);
  maxBatteryVoltage = preferences.getFloat("MBV", 3.95);
  useWiFi = preferences.getBool("WiFi", false);
  LEDonBattery = preferences.getBool("LEDonBattery", false);
  LEDonUSB = preferences.getBool("LEDonUSB", true);
  ledbrightness = preferences.getInt("ledbrightness", 5);
  useSmoothLEDcolor = preferences.getBool("useSmoothLEDcolor", true);
  invertDisplay = preferences.getBool("invertDisplay", false);
  useFahrenheit = preferences.getBool("useFahrenheit", false);
  english = preferences.getBool("english", false);
  preferences.end();

  scd4x.stopPeriodicMeasurement(); // stop potentially previously started measurement
  scd4x.getSerialNumber(serial0, serial1, serial2);
  scd4x.setSensorAltitude(50);     // Berlin: 50m über NN
  scd4x.setAutomaticSelfCalibration(1);
  scd4x.setTemperatureOffset(getTempOffset());
  scd4x.startPeriodicMeasurement();

  displayInit();
  delay(3000); // Wait for co2 measurement
  initDone = true;
}

void setLED(uint16_t co2) {
  updateBatteryMode();
  if ((BatteryMode && !LEDonBattery)
  || (!BatteryMode && !LEDonUSB)) {
    digitalWrite(LED_POWER, HIGH); // LED OFF
    strip.clear();
    strip.show();
    return;
  }
  digitalWrite(LED_POWER, LOW); //LED ON
  delay(10);

  int red = 0, green = 0, blue = 0;

  if (useSmoothLEDcolor) {
    if (co2 > 2000) {
      red = 216; green = 2; blue = 131; // magenta
    } else {
      red   =   pow((co2 - 400), 2) / 10000;
      green = - pow((co2 - 400), 2) / 4500 + 255;
    }
  } else {
    if (co2 < 600) {
      green = 255;
    } else if(co2 < 800) {
      red = 60; green = 200;
    } else if(co2 < 1000) {
      red = 140; green = 120;
    } else if(co2 < 1500) {
      red = 200; green = 60;
    } else if(co2 < 2000) {
      red = 255;
    } else {
      red = 216; green = 2; blue = 131; // magenta
    }
  }
  if (red < 0) red = 0;
  if (red > 255) red = 255;
  if (green < 0) green = 0;
  if (green > 255) green = 255;
  if (blue < 0) blue = 0;
  if (blue > 255) blue = 255;

  red =   (int)(red   * (ledbrightness / 100.0));
  green = (int)(green * (ledbrightness / 100.0));
  blue =  (int)(blue  * (ledbrightness / 100.0));

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

  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO);   // RTC IO, sensors and ULP co-processor
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_AUTO); // RTC slow memory: auto
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);  // RTC fast memory
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);          // XTAL oscillator
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC8M, ESP_PD_OPTION_OFF);         // CPU core
  esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);
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
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO);    // RTC IO, sensors and ULP co-processor
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_AUTO);  // RTC slow memory: auto
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);   // RTC fast memory
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);           // XTAL oscillator
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC8M, ESP_PD_OPTION_OFF);          // CPU core
  esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);

  /* Wakeup by usb power */
  rtc_gpio_pullup_dis(USB_PRESENT);
  rtc_gpio_pulldown_dis(USB_PRESENT);
  esp_sleep_enable_ext0_wakeup(USB_PRESENT, 1);

  /* Wakeup by IO0 button */
  rtc_gpio_pullup_en(BUTTON);
  rtc_gpio_pulldown_dis(BUTTON);
  esp_sleep_enable_ext1_wakeup(0x1,ESP_EXT1_WAKEUP_ALL_LOW); // 2^0 = GPIO_NUM_0 = BUTTON

  /* Keep LED enabled */
  if (LEDonBattery) gpio_hold_en(LED_POWER);
  else gpio_hold_dis(LED_POWER);
  
  comingFromDeepSleep = true;
  esp_deep_sleep_start();
}

static void usbEventCallback(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
  if(event_base == ARDUINO_USB_EVENTS){
    switch (event_id){
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
    for (int i=0; i<(ms/100); i++) {
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
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO);    // RTC IO, sensors and ULP co-processor
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_AUTO);  // RTC slow memory: auto
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);   // RTC fast memory
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);           // XTAL oscillator
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC8M, ESP_PD_OPTION_OFF);          // CPU core
    esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);
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
  else               voltage = (analogRead(BATTERY_VOLTAGE) * 3.33) / 5084.0 + 0.02;

  if ((voltage > maxBatteryVoltage) && (voltage < 4.2) && (digitalRead(USB_PRESENT) == LOW)) {
     maxBatteryVoltage = voltage;
     preferences.begin("co2-sensor", false);
     preferences.putFloat("MBV", voltage); //save maxBatteryVoltage
     preferences.end();
  }
  return voltage;
}

uint8_t calcBatteryPercentage(float voltage) {
  voltage += (4.2 - maxBatteryVoltage); // in field calibration

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
  for (int i=0; i<180; i++) {
    if (digitalRead(BUTTON) == 0) return; // abort
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
  displayImage(gImage_rainbow); //  gImage_santa
  digitalWrite(LED_POWER, LOW); //LED ON

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
  for(int j = 0; j < 256; j++) {
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
    if (j == 255) j=0;
    if (digitalRead(BUTTON) == 0) return;
    delay(20);
  }
}

RTC_DATA_ATTR uint8_t hour = 0;
RTC_DATA_ATTR uint8_t halfminute = 0;
RTC_DATA_ATTR uint16_t measurements[24][120];
void saveMeasurement(uint16_t co2) {
  if (halfminute == 120) {
    halfminute=0;
    hour++;
  }
  if (hour == 24) {
    for (int i=0; i<23; ++i) memcpy(measurements[i], measurements[i + 1], sizeof(uint16_t) * 120);
    hour = 23;
  }

  measurements[hour][halfminute] = co2;
  halfminute++;
}

uint8_t qrcodeNumber = 0;
void history() {
  //DEMO DATA:
  /*hour = 2;
  for (int i=0; i<120; i++) {
    measurements[0][i] = 400+i;
    measurements[1][i] = 520+i;
    measurements[2][i] = 1000+i;
  }
  halfminute = 120;*/

  if (halfminute == 0 && hour == 0) { // no history data
    displayNoHistory();
    unsigned long StartTime = millis();
    for (;;) if ((millis() - StartTime) > 20000 || digitalRead(BUTTON) == 0) return; // wait for button press OR up to 20 sec
  }

  qrcodeNumber = hour; // start at current hour
  for (int i=0; i<200; i++) {
    if (digitalRead(BUTTON) == 0) {  // goto next qr code
      displayHistory(measurements);
      delay(500);
      if (qrcodeNumber == hour) qrcodeNumber = 0;
      else qrcodeNumber++;
      i = 0; // display qrcode again for 20 sec
    }
    delay(100);
  }
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
    goto_light_sleep(1); // clear ESP_SLEEP_WAKEUP_GPIO
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
  while (digitalRead(BUTTON) != 0) { // wait for button press
    delay(100);
    if (!ip_shown && WiFi.status() == WL_CONNECTED) {
      ip_shown = true;
      displayWiFi(useWiFi); // to update displayed IP
      wifiManager.process();
    }
    if (BatteryMode && (digitalRead(USB_PRESENT) == HIGH)) { // power got connected
      BatteryMode = false;
      handleWiFiChange();
    }
  }
}

enum MenuOptions {
  LED,
  DISPLAY_MENU,
  CALIBRATE,
  HISTORY,
  WLAN,
  INFO,
  RAINBOW,
  NUM_OPTIONS
};
enum LEDMenuOptions {
  onBATTERY,
  onUSB,
  COLOR,
  BRIGHTNESS,
  EXIT_LED,
  NUM_LED_OPTIONS
};
enum DisplayMenuOptions {
  INVERT,
  TEMP_UNIT,
  LANGUAGE,
  EXIT_DISPLAY,
  NUM_DISPLAY_OPTIONS
};

/* ENGLISH */
const char* EnglishMenuItems[NUM_OPTIONS] = {
  "LED",
  "Display",
  "Calibrate",
  "History",
  "Wi-Fi",
  "Info",
  "Rainbow"//"Santa"
};
const char* EnglishLEDmenuItems[NUM_LED_OPTIONS] = {
  "Battery",
  "on USB",
  "Color",
  "Bright",
  "Exit"
};
const char* EnglishOptionsMenuItems[NUM_DISPLAY_OPTIONS] = {
  "Invert",
  "Unit",
  "English",
  "Exit"
};

/* GERMAN */
const char* GermanMenuItems[NUM_OPTIONS] = {
  "LED",
  "Display",
  "Kalibrieren",
  "Historie",
  "WLAN",
  "Info",
  "Regenbogen"//"Weihnachten"
};
const char* GermanLEDmenuItems[NUM_LED_OPTIONS] = {
  "Batterie",
  "mit USB",
  "Farbe",
  "Hell",
  "Beenden"
};
const char* GermanOptionsMenuItems[NUM_DISPLAY_OPTIONS] = {
  "Invert",
  "Einheit",
  "German",
  "Beenden"
};

bool buttonPressedAgain = false;
void handleButtonPress() {
  uint8_t selectedOption = 0;
  extern int refreshes;
  refreshes = 1; // force full update
  comingFromDeepSleep = false; // force display update even if CO2 changed by less than 3%
  displayMenu(selectedOption);

  uint16_t mspressed;
  unsigned long menuStartTime = millis();

  for (;;) { 
    if ((millis() - menuStartTime) > 20000) { // display Menu up to 20 sec
      refreshes = 1;
      return;
    }

    mspressed = 0;
    if (digitalRead(BUTTON) == 0) {
      while(digitalRead(BUTTON) == 0) { // calculate how long BUTTON is pressed
        delay(100);
        mspressed += 100;
        if (mspressed > 1000) break;
      }
      if (mspressed > 1000) { // long press
        switch (selectedOption) {
          case LED:
            LEDMenu();
            refreshes = 1;
            return;
          case DISPLAY_MENU:
            OptionsMenu();
            refreshes = 1;
            return;
          case CALIBRATE:
            calibrate();
            refreshes = 1;
            return;
          case HISTORY:
            history();
            refreshes = 1;
            return;
          case WLAN:
            toggleWiFi();
            refreshes = 1;
            return;
          case INFO:
            displayinfo();
            while (digitalRead(BUTTON) != 0) delay(100); // wait for button press
            refreshes = 1;
            return;
          case RAINBOW:
            rainbowMode();
            setLED(co2);
            refreshes = 1;
            return;
        }
      } else { // goto next Menu point
        buttonPressedAgain = true; // display at least once
        while (buttonPressedAgain) {
          buttonPressedAgain = false;
          selectedOption++;
          selectedOption %= NUM_OPTIONS;
          attachInterrupt(digitalPinToInterrupt(BUTTON), buttonInterrupt, FALLING);
          displayMenu(selectedOption);
          detachInterrupt(digitalPinToInterrupt(BUTTON));
          menuStartTime = millis(); // display Menu again for 20 sec
          if (digitalRead(BUTTON) == 0) break; // long press detected
        }
      }
    }
  }
}

void buttonInterrupt() {
  buttonPressedAgain = true;
}

void LEDMenu() {
  uint8_t selectedOption = 0;
  displayLEDMenu(selectedOption);
  uint16_t mspressed;
  unsigned long menuStartTime = millis();

  for (;;) { 
    if ((millis() - menuStartTime) > 20000) return; // display LED Menu up to 20 sec
    mspressed = 0;
    if (digitalRead(BUTTON) == 0) {
      while(digitalRead(BUTTON) == 0) { // calculate how long BUTTON is pressed
        delay(100);
        mspressed += 100;
        if (mspressed > 1000) break;
      }
      if (mspressed > 1000) { // long press
        switch (selectedOption) {
          case onBATTERY:
            LEDonBattery = !LEDonBattery;
            preferences.begin("co2-sensor", false);
            preferences.putBool("LEDonBattery", LEDonBattery);
            preferences.end();
            break;
          case onUSB:
            LEDonUSB = !LEDonUSB;
            preferences.begin("co2-sensor", false);
            preferences.putBool("LEDonUSB", LEDonUSB);
            preferences.end();
            break;
          case COLOR:
            useSmoothLEDcolor = !useSmoothLEDcolor;
            preferences.begin("co2-sensor", false);
            preferences.putBool("useSmoothLEDcolor", useSmoothLEDcolor);
            preferences.end();
            break;
          case BRIGHTNESS:
            ledbrightness += 20; // 5 25 45 65 85
            ledbrightness %= 100;
            preferences.begin("co2-sensor", false);
            preferences.putInt("ledbrightness", ledbrightness);
            preferences.end();
            break;
          case EXIT_LED:
            while(digitalRead(BUTTON) == 0) {} // wait until button is released
            return;
        }
        setLED(co2);
        displayLEDMenu(selectedOption);
        menuStartTime = millis(); // display LED Menu again for 20 sec
      } else { // goto next Menu point
        buttonPressedAgain = true; // display at least once
        while (buttonPressedAgain) {
          buttonPressedAgain = false;
          selectedOption++;
          selectedOption %= NUM_LED_OPTIONS;
          attachInterrupt(digitalPinToInterrupt(BUTTON), buttonInterrupt, FALLING);
          displayLEDMenu(selectedOption);
          detachInterrupt(digitalPinToInterrupt(BUTTON));
          menuStartTime = millis(); // display LED Menu again for 20 sec
          if (digitalRead(BUTTON) == 0) break; // long press detected
        }
      }
    }
  }
}

void OptionsMenu() {  
  uint8_t selectedOption = 0;
  displayOptionsMenu(selectedOption);

  uint16_t mspressed;
  unsigned long menuStartTime = millis();

  for (;;) { 
    if ((millis() - menuStartTime) > 20000) return; // display up to 20 sec
    mspressed = 0;
    if (digitalRead(BUTTON) == 0) {
      while(digitalRead(BUTTON) == 0) { // calculate how long BUTTON is pressed
        delay(100);
        mspressed += 100;
        if (mspressed > 1000) break;
      }
      if (mspressed > 1000) { // long press
        switch (selectedOption) {
          case INVERT:
            invertDisplay = !invertDisplay;
            preferences.begin("co2-sensor", false);
            preferences.putBool("invertDisplay", invertDisplay);
            preferences.end();
            break;
          case TEMP_UNIT:
            useFahrenheit = !useFahrenheit;
            preferences.begin("co2-sensor", false);
            preferences.putBool("useFahrenheit", useFahrenheit);
            preferences.end(); 
            break;
          case LANGUAGE:
            english = !english;
            preferences.begin("co2-sensor", false);
            preferences.putBool("english", english);
            preferences.end(); 
            break;
          case EXIT_DISPLAY:
            while(digitalRead(BUTTON) == 0) {} // wait until button is released
            return;
        }
        displayOptionsMenu(selectedOption);
        menuStartTime = millis(); // display again for 20 sec
      } else { // goto next Menu point
        buttonPressedAgain = true; // display at least once
        while (buttonPressedAgain) {
          buttonPressedAgain = false;
          selectedOption++;
          selectedOption %= NUM_DISPLAY_OPTIONS;
          attachInterrupt(digitalPinToInterrupt(BUTTON), buttonInterrupt, FALLING);
          displayOptionsMenu(selectedOption);
          detachInterrupt(digitalPinToInterrupt(BUTTON));
          menuStartTime = millis(); // display again for 20 sec
          if (digitalRead(BUTTON) == 0) break; // long press detected
        }
      }
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

  char Hostname[28];
  snprintf(Hostname, 28, "OpenCO2-Sensor%llX", ESP.getEfuseMac());
  WiFi.setHostname(Hostname); // hostname when conneced to home network

  wifiManager.setConfigPortalBlocking(false);
  wifiManager.autoConnect("OpenCO2 Sensor");  // name of broadcasted SSID  

#ifdef MQTT
  loadCredentials();
  if(mqtt_server[0] != '\0' && mqtt_port[0] != '\0'){
      mqttClient.connect(mqtt_server, (int)mqtt_port);
  }
#endif /* MQTT */

#ifdef airgradient
  server.on("/", HandleRoot);
  server.on("/metrics", HandleRoot);
  server.onNotFound(HandleNotFound);
  server.begin();
  Serial.println("HTTP server started at ip " + WiFi.localIP().toString() + ":" + String(port));
#endif /* airgradient */

  configTime(0, 0, "pool.ntp.org");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); // Europe/Berlin time zone from https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  tzset();
}

float currentTemp = temperatureRead();
RTC_DATA_ATTR float ESP32temps[10] = {currentTemp,currentTemp,currentTemp,currentTemp,currentTemp,currentTemp,currentTemp,currentTemp,currentTemp,currentTemp};
RTC_DATA_ATTR float sumTemp = currentTemp * 10;
RTC_DATA_ATTR int indexTemp = 0;
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
  Wire.begin(33, 34); // grün, gelb
  scd4x.begin(Wire);

  USB.onEvent(usbEventCallback);
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
    if (TEST_MODE) displayWelcome(); // exit TEST_MODE via IO button
    handleButtonPress();
  }

  if (!BatteryMode && comingFromDeepSleep) {
    delay(1);
    setLED(co2);

    scd4x.stopPeriodicMeasurement();   // stop low power measurement
    scd4x.setTemperatureOffset(getTempOffset());
    scd4x.startPeriodicMeasurement();
    /* Wait for co2 measurement */
    delay(5000);
  }

  if (useWiFi && !BatteryMode) startWiFi();
}


void loop() {
  if (!useWiFi && esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO) handleButtonPress();
  updateBatteryMode(); // check again in USB Power mode
  measureESP32temperature();

  if (useWiFi && !BatteryMode) wifiManager.process();

  bool isDataReady = false;
  uint16_t ready_error = scd4x.getDataReadyFlag(isDataReady);
  if (ready_error || !isDataReady) {
    // needed to overwrite displayed Menu 
    displayWriteMeasuerments(co2, temperature, humidity);
    if (BatteryMode) displayBattery(calcBatteryPercentage(readBatteryVoltage()));
    else if (useWiFi) displayWiFiStrengh(); 
    updateDisplay();

    if (BatteryMode) goto_deep_sleep(29000);
    else goto_light_sleep(4000);
    return; // otherwise continues running!
  }

  // Read co2 Measurement
  uint16_t new_co2 = 400;
  float new_temperature = 0.0f;
  uint16_t error = scd4x.readMeasurement(new_co2, new_temperature, humidity);
  if (error) {
    char errorMessage[256];
    errorToString(error, errorMessage, 256);
    displayWriteError(errorMessage);
  } else {
    if (BatteryMode) saveMeasurement(new_co2);
    /* dont update in Battery mode, unless CO2 has changed by 3% or temperature by 0.5°C */
    if (!TEST_MODE && BatteryMode && comingFromDeepSleep) {
      if ((abs(new_co2 - co2) < (0.03*co2)) && (fabs(new_temperature - temperature) < 0.5)) {
        goto_deep_sleep(30000);
      }
    }

    if (new_co2 > 400) co2 = new_co2;
    temperature = new_temperature;
    setLED(co2);
    displayWriteMeasuerments(co2, temperature, humidity);
  }

  if (useWiFi) {
#ifdef MQTT
    if (!error && !BatteryMode) {
      if (WiFi.status() == WL_CONNECTED) {
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
    }
#endif /* MQTT */

#ifdef airgradient
    if (!error && !BatteryMode) {
      if (WiFi.status() == WL_CONNECTED) {
        server.handleClient();
      }
    }
#endif /* airgradient */
  }

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
    goto_deep_sleep(29500);
  }

  goto_light_sleep(4000);
}
