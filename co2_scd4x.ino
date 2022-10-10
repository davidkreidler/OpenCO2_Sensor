/*
   ESP32S2 Dev Module
   Required Arduino libraries:
   - esp32 waveshare epd
   - Adafruit DotStar
   - Sensirion Core
   - Sensirion I2C SCD4x: https://github.com/Sensirion/arduino-i2c-scd4x
   - WiFiManager: https://github.com/tzapu/WiFiManager
*/

/* Includes display */
#include "DEV_Config.h"
#include "epd_abstraction.h"
#define DISPLAY_POWER 45
#define LED_POWER GPIO_NUM_9
#define USB_PRESENT GPIO_NUM_4
#define BATTERY_VOLTAGE GPIO_NUM_5
//#define TEST_MODE

/* welcome */
#include <EEPROM.h>
#include <Preferences.h>
Preferences preferences;

/* WIFI */
//#define WIFI
#ifdef WIFI
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <BluetoothSerial.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
WiFiManager wifiManager;
#endif

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

RTC_DATA_ATTR bool initDone = false;
RTC_DATA_ATTR bool BatteryMode = false;
RTC_DATA_ATTR bool comingFromDeepSleep = false;
RTC_DATA_ATTR int ledbrightness = 5;
RTC_DATA_ATTR uint16_t co2 = 400;
RTC_DATA_ATTR bool LEDalwaysOn = false;
RTC_DATA_ATTR int HWSubRev = 1; //default only
RTC_DATA_ATTR float maxBatteryVoltage;
#ifdef TEST_MODE
RTC_DATA_ATTR uint16_t sensorStatus;
#endif

#ifdef WIFI
#define tempOffset 13.0
#else
#define tempOffset 4.4 // was 5.8
#endif

void initOnce() {
  initEpdOnce();
  EEPROM.begin(2); // EEPROM_SIZE
#ifdef TEST_MODE
  EEPROM.write(0, 0); //reset welcome
  //EEPROM.write(1, 2); //write HWSubRev 2
  EEPROM.commit();
  preferences.begin("co2-sensor", true); 
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
#else
  int welcomeDone = EEPROM.read(0);
  if (welcomeDone != 1) displayWelcome();
#endif /* TEST_MODE */
  HWSubRev = EEPROM.read(1);
  preferences.begin("co2-sensor", true); 
  maxBatteryVoltage = preferences.getFloat("MBV", 3.95);
  preferences.end();

  scd4x.stopPeriodicMeasurement(); // stop potentially previously started measurement
  scd4x.setSensorAltitude(50);     // Berlin: 50m über NN
  scd4x.setAutomaticSelfCalibration(1);
  scd4x.setTemperatureOffset(tempOffset);
  scd4x.startPeriodicMeasurement();

  displayInit();
  delay(3000); // Wait for co2 measurement
  initDone = true;
}

void setLED(uint16_t co2_value) {
  if (BatteryMode && !LEDalwaysOn) {
    digitalWrite(LED_POWER, HIGH); // LED OFF
    strip.clear();
    strip.show();
    return;
  }
  digitalWrite(LED_POWER, LOW); //LED ON
  delay(10);

  int red = 0, green = 0, blue = 0;

  if (co2_value > 2000) {
    red = 216; green = 2; blue = 131; // magenta
  } else {
    red   =   pow((co2_value - 400), 2) / 10000;
    green = - pow((co2_value - 400), 2) / 4500 + 255;
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
#ifdef WIFI
  //WiFi.setSleep(true);
  WiFi.disconnect(true);  // Disconnect from the network
  WiFi.mode(WIFI_OFF);    // Switch WiFi off
#endif

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
  rtc_gpio_pullup_en(GPIO_NUM_0);
  rtc_gpio_pulldown_dis(GPIO_NUM_0);
  esp_sleep_enable_ext1_wakeup(0x1,ESP_EXT1_WAKEUP_ALL_LOW); // 2^0 = GPIO_NUM_0 

  /* Keep LED enabled */
  if (LEDalwaysOn) gpio_hold_en(LED_POWER);
  else gpio_hold_dis(LED_POWER);
  
  comingFromDeepSleep = true;
  esp_deep_sleep_start();
}

void goto_light_sleep(int ms) {
  comingFromDeepSleep = false;
#ifdef WIFI
  delay(ms);
#else
  esp_sleep_enable_timer_wakeup(ms * 1000);                             // periodic measurement every 5 sec -1.1 sec awake
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);      // RTC IO, sensors and ULP co-processor
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_AUTO);  // RTC slow memory: auto
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);   // RTC fast memory
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);           // XTAL oscillator
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC8M, ESP_PD_OPTION_OFF);          // CPU core
  esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);
  esp_light_sleep_start();
#endif
  //free(BlackImage);
  //BlackImage = NULL;
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

void setup() {
  pinMode(DISPLAY_POWER, OUTPUT);
  pinMode(LED_POWER, OUTPUT);
  digitalWrite(DISPLAY_POWER, HIGH);
  DEV_Module_Init();

  /* scd4x */
  Wire.begin(33, 34); // grün, gelb
  scd4x.begin(Wire);

  if (!initDone) initOnce();

  /* power */
  pinMode(USB_PRESENT, INPUT);
  pinMode(BATTERY_VOLTAGE, INPUT);
  updateBatteryMode();

#ifdef WIFI
  if (!BatteryMode) {
    //if (comingFromDeepSleep) {}
    /*WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) delay(500);*/
    wifiManager.setConfigPortalBlocking(false);
    wifiManager.autoConnect("CO2Sensor");
  }
#endif

  strip.begin();
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1) {
    LEDalwaysOn = !LEDalwaysOn;
    setLED(co2);
    delay(1000);
  }

  if (!BatteryMode && comingFromDeepSleep) {
    delay(1);
    setLED(co2);

    scd4x.stopPeriodicMeasurement();   // stop low power measurement
    scd4x.setTemperatureOffset(tempOffset);
    scd4x.startPeriodicMeasurement();
    /* Wait for co2 measurement */
    delay(5000);
  }
}

uint16_t new_co2 = 0;
float temperature = 0.0f;
float humidity = 0.0f;

void loop() {
  updateBatteryMode(); // check again in USB Power mode

#ifdef WIFI
  if (!BatteryMode) wifiManager.process();
#endif

  bool isDataReady = false;
  uint16_t ready_error = scd4x.getDataReadyFlag(isDataReady);
  if (ready_error || !isDataReady) {
    if (BatteryMode) goto_deep_sleep(29000);
    else goto_light_sleep(4000);
    return; // otherwise continues running!
  }

  // Read co2 Measurement
  uint16_t error = scd4x.readMeasurement(new_co2, temperature, humidity);
  if (error) {
    char errorMessage[256];
    errorToString(error, errorMessage, 256);
    displayWriteError(errorMessage);
  } else {
    /* dont update in Battery mode, unless co2 is +- 10 ppm different */
    if (BatteryMode && comingFromDeepSleep && (abs(new_co2 - co2) < 10)) goto_deep_sleep(29000);
    if (new_co2 > 400) co2 = new_co2;
    setLED(co2);
    displayWriteMeasuerments(co2, temperature, humidity);
  }

#ifdef WIFI
  if (!error && !BatteryMode) {
    if (WiFi.status() == WL_CONNECTED) {
      String payload = "{\"wifi\":" + String(WiFi.RSSI()) + ",";
      payload=payload+"\"pm02\":" + "100";
      payload=payload+",";
      payload=payload+"\"rco2\":" + String(co2);
      payload=payload+",";
      payload=payload+"\"atmp\":" + String(temperature) +   ",\"rhum\":" + String(humidity);
      payload=payload+"}";

      String APIROOT = "http://10.0.0.4:9925/";
      String POSTURL = APIROOT + "sensors/airgradient:" + "1995c6" + "/measures";
      WiFiClient client;
      HTTPClient http;
      http.begin(client, POSTURL);
      http.addHeader("content-type", "application/json");
      int httpCode = http.POST(payload);
      String response = http.getString();
      http.end();
    }
  }
#endif

#ifdef TEST_MODE
  displayWriteTestResults(readBatteryVoltage(), BatteryMode, sensorStatus);
#else
  /* Print Battery % */
  if (BatteryMode) {
    float voltage = readBatteryVoltage();
    if (voltage < 3.1) lowBatteryMode();
    displayBattery(calcBatteryPercentage(voltage));
  }
#endif /* TEST_MODE */

  updateDisplay(comingFromDeepSleep);

#ifndef TEST_MODE
  if (BatteryMode) {
    if (!comingFromDeepSleep) {
      scd4x.stopPeriodicMeasurement();
      scd4x.setTemperatureOffset(0.8);
      scd4x.startLowPowerPeriodicMeasurement();
    }
    goto_deep_sleep(29000);
  }
#endif

  goto_light_sleep(4000);
}
