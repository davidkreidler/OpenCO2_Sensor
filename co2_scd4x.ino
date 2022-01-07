/*
   ESP32S2 Dev Module
   Required Arduino libraries:
   - esp32 waveshare epd
   - Adafruit DotStar
   - Sensirion Core
   - Sensirion I2C SCD4x: https://github.com/Sensirion/arduino-i2c-scd4x
   - WiFiManager: https://github.com/tzapu/WiFiManager
*/

/* deep & light sleep */
#include <esp_sleep.h>

/* eInk display */
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include <stdlib.h>
#include "init.h"
#define DISPLAY_POWER 45
#define LED_POWER 9
#define EINK_1IN54V2
//#define EINK_4IN2
//#define TEST_MODE

/* welcome */
#include <EEPROM.h>

/* WIFI */
#define WIFI    // enable WiFi
#define INFLUX  // enable influxdb

#ifdef WIFI
#include <WiFi.h>

// WiFi manager
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>

// Update database
#include "time.h"
#include <HTTPClient.h>
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
RTC_DATA_ATTR int refreshes = 1;
RTC_DATA_ATTR UBYTE *BlackImage;
RTC_DATA_ATTR bool BatteryMode = false;
RTC_DATA_ATTR bool comingFromDeepSleep = false;
RTC_DATA_ATTR int ledbrightness = 5;
RTC_DATA_ATTR uint16_t co2 = 400;
RTC_DATA_ATTR bool LEDalwaysOn = false;
RTC_DATA_ATTR int HWSubRev = 1; //default only
#ifdef TEST_MODE
RTC_DATA_ATTR uint16_t sensorStatus;
#endif

#ifndef WIFI
#define tempOffset       4.4 // was 5.8
#else
#define tempOffset      13.0
#define WIFI_RETRY_MAX  20
#define WIFI_TIMEOUT    10000

bool wifi_ready = false;
RTC_DATA_ATTR bool wifi_init = false;
RTC_DATA_ATTR int wifi_retry_count = 0;
RTC_DATA_ATTR unsigned long wifi_retry_time = 0;
RTC_DATA_ATTR bool tic = true;

const char* ntpServer = "pool.ntp.org";   // NTP server to request epoch time
const char* hostname = "CO2Sensor";
WiFiManager wifiManager;

unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return(0);
  }
  time(&now);
  return now;
}

void WiFiStationConnected(arduino_event_id_t event, arduino_event_info_t info) {
  if (!wifi_ready) configTime(0, 0, ntpServer);
  wifi_ready = true;
}

void WiFiStationDisconnected(arduino_event_id_t event, arduino_event_info_t info) {
  wifi_ready = false;
  wifi_retry_count = 0;
}

void wifi_setup() {
  WiFi.setHostname(hostname);

  WiFi.onEvent(WiFiStationConnected, ARDUINO_EVENT_WIFI_STA_GOT_IP);  // ARDUINO_EVENT_WIFI_STA_CONNECTED
  WiFi.onEvent(WiFiStationDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.onEvent(WiFiStationDisconnected, ARDUINO_EVENT_WIFI_STA_LOST_IP);

  wifiManager.setConfigPortalBlocking(false);
  wifiManager.autoConnect(hostname);

  wifi_init = true;
}

void wifi_disable() {
  wifi_init = false;

  //WiFi.setSleep(true);
  WiFi.disconnect(true);  // Disconnect from the network
  WiFi.mode(WIFI_OFF);    // Switch WiFi off
}

void wifi_reconnect() {
  if (wifi_retry_count < WIFI_RETRY_MAX && (millis()-wifi_retry_time) > WIFI_TIMEOUT) {
    if ((WiFi.status() != WL_CONNECTED) && (WiFi.getMode() & WIFI_MODE_STA)) {
      wifi_retry_count++;
      wifi_retry_time = millis();

      if (WiFi.reconnect()) {
        if (WiFi.status() == WL_CONNECTED) wifi_retry_count = 0;
      } else if (wifi_retry_count == WIFI_RETRY_MAX) {
        wifi_disable();
      }
    }
  }
}

#endif

void displayWelcome() {
#ifdef EINK_1IN54V2
  Paint_DrawBitMap(gImage_welcome);
  EPD_1IN54_V2_Display(BlackImage);
  EPD_1IN54_V2_Sleep();
#endif
/*#ifdef EINK_4IN2
  Paint_DrawBitMap(gImage_welcome);
  EPD_4IN2_Display(BlackImage);
  EPD_4IN2_Sleep();
#endif*/

  EEPROM.write(0, 1);
  EEPROM.commit();

  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);     // RTC IO, sensors and ULP co-processor
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);   // RTC slow memory: auto
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);   // RTC fast memory
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);           // XTAL oscillator
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC8M, ESP_PD_OPTION_OFF);          // CPU core
  esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);
  esp_deep_sleep_start();
}

void initOnce() {
#ifdef EINK_1IN54V2
  BlackImage = (UBYTE *)malloc(5000);
  Paint_NewImage(BlackImage, EPD_1IN54_V2_WIDTH, EPD_1IN54_V2_HEIGHT, 270, WHITE);
  EPD_1IN54_V2_Init();
#endif
#ifdef EINK_4IN2
  BlackImage = (UBYTE *)malloc(EPD_4IN2_WIDTH / 4 );
  Paint_NewImage(BlackImage, EPD_4IN2_WIDTH, EPD_4IN2_HEIGHT, 0, WHITE);
  EPD_4IN2_Init_Fast();
#endif
  Paint_Clear(WHITE);

  EEPROM.begin(2); // EEPROM_SIZE
#ifdef TEST_MODE
  EEPROM.write(0, 0); //reset welcome
  //EEPROM.write(1, 2); //write HWSubRev 2
  EEPROM.commit();
  
  digitalWrite(LED_POWER, LOW); //LED on
  strip.begin();
  strip.setPixelColor(0, 5, 5, 5); //index, green, red, blue
  strip.show();

#ifdef EINK_1IN54V2
  Paint_DrawBitMap(gImage_init);
  Paint_DrawNum(125, 25, 1, &bahn_mid, BLACK, WHITE);
  EPD_1IN54_V2_Display(BlackImage);
#endif
#ifdef EINK_4IN2
  EPD_4IN2_Display(BlackImage);
#endif

  scd4x.stopPeriodicMeasurement();
  scd4x.performSelfTest(sensorStatus);
#else
  int welcomeDone = EEPROM.read(0);
  if (welcomeDone != 1) displayWelcome();
#endif /* TEST_MODE */
  HWSubRev = EEPROM.read(1);

  scd4x.stopPeriodicMeasurement(); // stop potentially previously started measurement
  scd4x.setSensorAltitude(50);     // Berlin: 50m über NN
  scd4x.setAutomaticSelfCalibration(1);
  scd4x.setTemperatureOffset(tempOffset);
  scd4x.startPeriodicMeasurement();

#ifdef EINK_1IN54V2
  Paint_DrawBitMap(gImage_init);
  EPD_1IN54_V2_Display(BlackImage);
  EPD_1IN54_V2_Sleep();
#endif
#ifdef EINK_4IN2
  EPD_4IN2_Display(BlackImage);
  EPD_4IN2_Sleep();
#endif
  //refreshes++;
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
  delay(1);

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
  refreshes = 1;
  Paint_Clear(BLACK);
                // Xstart,Ystart,Xend,Yend
  Paint_DrawRectangle( 50,  40, 150, 90, WHITE, DOT_PIXEL_3X3, DRAW_FILL_EMPTY);
  Paint_DrawRectangle(150,  55, 160, 75, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
  Paint_DrawLine(      60, 100, 140, 30, WHITE, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
  Paint_DrawString_EN(45, 120, "Batterie", &Font20, BLACK, WHITE);
  Paint_DrawString_EN(45, 145, "aufladen", &Font20, BLACK, WHITE);

#ifdef EINK_1IN54V2
  EPD_1IN54_V2_Init();
  EPD_1IN54_V2_Display(BlackImage);
  EPD_1IN54_V2_DisplayPartBaseImage(BlackImage);
  EPD_1IN54_V2_Sleep();
#endif
#ifdef EINK_4IN2
  EPD_4IN2_Init_Fast();
  EPD_4IN2_Display(BlackImage);
  EPD_4IN2_Sleep();
#endif
  gpio_hold_dis((gpio_num_t)LED_POWER); //led off

  esp_sleep_enable_ext0_wakeup((gpio_num_t)4, 1);
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
  if (wifi_init) wifi_disable();
#endif
  esp_sleep_enable_ext0_wakeup((gpio_num_t)4, 1);
  esp_sleep_enable_timer_wakeup(ms * 1000);                             // periodic measurement every 30 sec - 0.83 sec awake
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO);    // RTC IO, sensors and ULP co-processor
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_AUTO);  // RTC slow memory: auto
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);   // RTC fast memory
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);           // XTAL oscillator
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC8M, ESP_PD_OPTION_OFF);          // CPU core
  esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);

  /* Wakeup by IO0 button */
  rtc_gpio_pullup_en((gpio_num_t)0);
  rtc_gpio_pulldown_dis((gpio_num_t)0);
  esp_sleep_enable_ext1_wakeup((uint64_t)0x1, ESP_EXT1_WAKEUP_ALL_LOW);

  if (LEDalwaysOn) gpio_hold_en((gpio_num_t)LED_POWER);
  else gpio_hold_dis((gpio_num_t)LED_POWER);
  
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
  BatteryMode = (digitalRead(4) == LOW);
}

double readBatteryVoltage() {
  // IO5 for voltage divider measurement
  if (HWSubRev == 2) return ((analogRead(5) * 3.33) / 5358.0);
  return ((analogRead(5) * 3.33) / 5084.0) + 0.02;
}

uint8_t calcBatteryPercentage(double voltage) {
#ifdef EINK_1IN54V2
  voltage += 0.11; // offset for type of Battery used
#endif

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
  pinMode(4, INPUT);  // usb Power
  pinMode(5, INPUT);  // Battery Voltage
  updateBatteryMode();

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
  }
}

uint16_t new_co2 = 0;
float temperature = 0.0f;
float humidity = 0.0f;

char decimal[2];

#ifdef WIFI

WiFiClient client;
HTTPClient http;

#ifdef INFLUX
const String host = "192.168.178.28";
const uint16_t port = 8086;
const String url = "/write?db=scd4x&precision=s";
const bool https = true;
const String ctype = "text/plain";
#elif
const String host = "10.0.0.4";
const uint16_t port = 9925;
const String url = "/sensors/airgradient:1995c6/measures";
const bool https = false;
const String ctype = "application/json";
#endif

int update_database() {
  #ifdef INFLUX
  String payload = "sensor ";
  payload += "rssi=" + String(WiFi.RSSI()) + ",";
  payload += "ip=" + String(WiFi.localIP()) + ",";
  payload += "co2=" + String(co2) + ",";
  payload += "temperature=" + String(temperature) +   ",";
  payload += "humidity=" + String(humidity) + " " + String(getTime(), DEC);
  #elif
  String payload = "{";
  payload += "\"wifi\":" + String(WiFi.RSSI()) + ",";
  payload += "\"pm02\":" + "100" + ",";
  payload += "\"rco2\":" + String(co2) + ",";
  payload += "\"atmp\":" + String(temperature) + ",";
  payload += "\"rhum\":" + String(humidity);
  payload += "}";
  #endif

  http.begin(client, host, port, url, https);
  http.addHeader("content-type", ctype);
  http.POST(payload);
  int httpCode = http.POST(payload);
  // String response = http.getString();
  http.end();

  return httpCode;
}
#endif

void loop() {
  Paint_Clear(WHITE);

  // check again in USB Power mode
  updateBatteryMode();

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
    Paint_DrawString_EN(5, 40, errorMessage, &Font20, WHITE, BLACK);
  } else {
    /* dont update in Battery mode, unless co2 is +- 10 ppm different */
    if (BatteryMode && comingFromDeepSleep && (abs(new_co2 - co2) < 10)) goto_deep_sleep(29000);
    if (new_co2 > 400) co2 = new_co2;
    setLED(co2);

#ifdef EINK_1IN54V2
    /* co2 */
    if      (co2 > 9999) Paint_DrawNum(27, 78, co2, &bahn_mid, BLACK, WHITE);
    else if (co2 < 1000) Paint_DrawNum(30, 65, co2, &bahn_big, BLACK, WHITE);
    else                 Paint_DrawNum( 6, 65, co2, &bahn_big, BLACK, WHITE);
    Paint_DrawString_EN(142, 150, "ppmn", &bahn_sml, WHITE, BLACK);

    /* temperature */
//#define useFahrenheit
#ifdef useFahrenheit
    temperature = (temperature*1.8f)+32.0f; // convert to °F
    char unit[3] = "*F";
#else
    char unit[3] = "*C";
#endif
    if (temperature < 10.0f) Paint_DrawNum(30, 5, temperature, &bahn_mid, BLACK, WHITE);
    else                     Paint_DrawNum( 1, 5, temperature, &bahn_mid, BLACK, WHITE);
    int offset = 0;
#ifdef useFahrenheit
    if (temperature >= 100) offset = 29;
#endif
    Paint_DrawString_EN(60+offset, 4, unit, &bahn_sml, WHITE, BLACK);
    Paint_DrawString_EN(60+offset, 32, ",", &bahn_sml, WHITE, BLACK);
    sprintf(decimal, "%d", ((int)(temperature * 10)) % 10);
    Paint_DrawString_EN(71+offset, 27, decimal, &bahn_sml, WHITE, BLACK);

    /* humidity */
    Paint_DrawNum(124, 5, humidity, &bahn_mid, BLACK, WHITE);
    Paint_DrawString_EN(184, 5, "%", &bahn_sml, WHITE, BLACK);
#endif /* EINK_1IN54V2 */
#ifdef EINK_4IN2
    /* co2 */
                                 // Xstart,Ystart
    if      (co2 > 9999) Paint_DrawNum(102, 88, co2, &bahn_big, BLACK, WHITE);
    else if (co2 < 1000) Paint_DrawNum(196, 88, co2, &bahn_big, BLACK, WHITE);
    else                 Paint_DrawNum(149, 88, co2, &bahn_big, BLACK, WHITE);
    Paint_DrawString_EN(337, 143, "ppmn", &bahn_sml, WHITE, BLACK);

    /* devider lines */
             // Xstart,Ystart,Xend,Yend
    Paint_DrawLine( 10, 210, 390, 210, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
    Paint_DrawLine(200, 210, 200, 290, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID);

    /* House synmbol */
    Paint_DrawLine( 13, 120,  70,  66, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID); //top left
    Paint_DrawLine( 70,  66, 126, 120, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID); //top right
    Paint_DrawLine( 27, 109,  27, 171, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID); //left
    Paint_DrawLine(112, 109, 112, 171, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID); //right
    Paint_DrawLine( 27, 171, 112, 171, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID); //button
    Paint_DrawLine( 99,  70,  99,  91, BLACK, DOT_PIXEL_5X5, LINE_STYLE_SOLID); //chimney
                 // XCenter,YCenter,radius
    Paint_DrawCircle(    54, 132,  16, BLACK, DOT_PIXEL_3X3, DRAW_FILL_EMPTY);      //C
    Paint_DrawRectangle( 58, 112,  72, 152, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);  //C
    Paint_DrawCircle(    80, 132,  16, BLACK, DOT_PIXEL_3X3, DRAW_FILL_EMPTY);      //O
    Paint_DrawNum(       94, 145,   2, &bahn_sml, BLACK, WHITE);                    //2

    /* temperature */
    if (temperature < 10.0f) Paint_DrawNum(69, 220, temperature, &bahn_big, BLACK, WHITE);
    else                     Paint_DrawNum(32, 220, temperature, &bahn_big, BLACK, WHITE);
    Paint_DrawString_EN(140, 220, "*C", &bahn_sml, WHITE, BLACK);
    Paint_DrawLine(137, 287, 137, 287, BLACK, DOT_PIXEL_4X4, LINE_STYLE_SOLID);

    sprintf(decimal, "%d", ((int)(temperature * 10)) % 10);
    Paint_DrawString_EN(145, 247, decimal, &bahn_mid, WHITE, BLACK);

    /* humidity */
    Paint_DrawNum(240, 220, humidity, &bahn_big, BLACK, WHITE);
    Paint_DrawString_EN(340, 220, "%", &bahn_sml, WHITE, BLACK);
#endif
  }

#ifdef TEST_MODE
  double voltage = readBatteryVoltage();
  char batteryvolt[8] = "";
  dtostrf(voltage, 1, 3, batteryvolt);
  char volt[10] = "V";
  strcat(batteryvolt, volt);
  Paint_DrawString_EN(0, 140, batteryvolt, &Font20, WHITE, BLACK);

  char BatteryMode_s[9] = "";
  strcat(BatteryMode_s, "USB-C:");
  strcat(BatteryMode_s, BatteryMode ? "no" : "yes");
  Paint_DrawString_EN(0, 160, BatteryMode_s, &Font20, WHITE, BLACK);

  char sensorStatus_s[20] = "";
  strcat(sensorStatus_s, "SCD4x:");
  if (sensorStatus == 0) {
    strcat(sensorStatus_s, "good");
  } else {
    char snum[5];
    itoa(sensorStatus, snum, 10);
    strcat(sensorStatus_s, snum);
  }
  Paint_DrawString_EN(0, 180, sensorStatus_s, &Font20, WHITE, BLACK);
  Paint_DrawNum(158, 180, (int32_t)refreshes, &Font20, BLACK, WHITE);
#else

  /* Print Battery % */
  if (BatteryMode) {
    double voltage = readBatteryVoltage();
    if (voltage < 3.1) lowBatteryMode();
    uint8_t percentage = calcBatteryPercentage(voltage);

#ifdef EINK_1IN54V2
                  // Xstart,Ystart,Xend,Yend
    Paint_DrawRectangle( 15, 145, 120, 169, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(120, 149, 125, 165, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawRectangle( 15, 145, 105*(percentage/100.0)+15, 169, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
#endif
#ifdef EINK_4IN2
    Paint_DrawRectangle(225, 10, 330, 34, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(330, 14, 335, 30, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(225, 10, 105*(percentage/100.0)+225, 34, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    char batterpercent[8] = "";
    dtostrf(percentage, 3, 0, batterpercent);
    char percent[10] = "%";
    strcat(batterpercent, percent);
    Paint_DrawString_EN(342, 10, batterpercent, &bahn_sml, WHITE, BLACK);
#endif
  } else {
    #ifdef WIFI
    int ret = 0;

    if (!wifi_init) {
      wifi_setup();
    } else {
      wifiManager.process();

      if (!error && (WiFi.status() == WL_CONNECTED) && wifi_ready) ret = update_database();

      wifi_reconnect();
    }

    String wifi_status = "WiFi " + String((WiFi.status() == WL_CONNECTED)? (String(WiFi.RSSI()) + "dBm"):"off");
    Paint_DrawString_EN(5, 150, wifi_status.c_str(), &Font16, WHITE, BLACK);

    if (ret != 0) {
      String str_ready = "Update " + String(ret) + String(tic ? "":" .");
      Paint_DrawString_EN(5, 170, str_ready.c_str(), &Font16, WHITE, BLACK);
      tic = !tic;
    }
    #endif
  }
#endif /* TEST_MODE */

#ifdef EINK_1IN54V2
  if (refreshes == 1) {
    EPD_1IN54_V2_Init();
    EPD_1IN54_V2_DisplayPartBaseImage(BlackImage);
  } else {
    EPD_1IN54_V2_Init_Partial();
    EPD_1IN54_V2_DisplayPart(BlackImage); // partial update
  }
  EPD_1IN54_V2_Sleep();
#endif
#ifdef EINK_4IN2
  if (refreshes == 1) {
    EPD_4IN2_Init_Fast();
    EPD_4IN2_Display(BlackImage);
  } else {
    EPD_4IN2_Init_Partial();
    EPD_4IN2_PartialDisplay(0, 0, EPD_4IN2_WIDTH, EPD_4IN2_HEIGHT, BlackImage);
  }
  EPD_4IN2_Sleep();
#endif
  if (refreshes == 720) { // every hour or every six hours on battery
    refreshes = 0;        // force full update
  }
  refreshes++;

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
