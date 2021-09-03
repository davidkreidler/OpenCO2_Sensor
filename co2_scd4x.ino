/* Includes display */
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include <stdlib.h>
#include "init.h"
/* welcome */
#include <EEPROM.h>

/* APP */
//#define USEAPP
#ifdef USEAPP
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

char auth[] = "xxx";
char ssid[] = "xxx";
char pass[] = "xxx";
#endif

/* led */
#include <Adafruit_DotStar.h>
#include <SPI.h>

#define DATAPIN 40
#define CLOCKPIN 39
Adafruit_DotStar strip(1, DATAPIN, CLOCKPIN, DOTSTAR_BRG);

/* scd4x */
#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
SensirionI2CScd4x scd4x;

RTC_DATA_ATTR bool initDone = false;
RTC_DATA_ATTR int refreshes = 1;
RTC_DATA_ATTR UBYTE *BlackImage;
RTC_DATA_ATTR bool BatteryMode = false;
RTC_DATA_ATTR bool commingFromDeepSleep = false;
RTC_DATA_ATTR bool fensterAuf = false;
RTC_DATA_ATTR bool fensterZu = false;
RTC_DATA_ATTR int ledbrightness = 5;
RTC_DATA_ATTR uint16_t co2 = 400;
RTC_DATA_ATTR int dataReadyErrorCount = 0;

#ifdef USEAPP
#define tempOffset 11.0
#else
#define tempOffset 4.6 // was 5.8
#endif

void displayWelcome() {
  Paint_DrawBitMap(gImage_welcome);
  EPD_1IN54_V2_Display(BlackImage);
  EPD_1IN54_V2_Sleep();

  EEPROM.write(0, 1);
  EEPROM.commit();

  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);    // RTC IO, sensors and ULP co-processor
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF); // RTC slow memory: auto
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);  // RTC fast memory
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);          // XTAL oscillator
  esp_sleep_pd_config(ESP_PD_DOMAIN_CPU, ESP_PD_OPTION_OFF);           // CPU core
  esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);           // Number of domains
  esp_deep_sleep_start();
}

void initOnce() {
  const int Imagesize = 5000; //25 * 200
  BlackImage = (UBYTE *)malloc(Imagesize);
  Paint_NewImage(BlackImage, EPD_1IN54_V2_WIDTH, EPD_1IN54_V2_HEIGHT, 270, WHITE);
  EPD_1IN54_V2_Init();
  Paint_Clear(WHITE); 
  EEPROM.begin(1); //EEPROM_SIZE
  int welcomeDone = EEPROM.read(0);
  if(welcomeDone != 1) displayWelcome();

  scd4x.stopPeriodicMeasurement(); // stop potentially previously started measurement
  scd4x.setSensorAltitude(50);     // Berlin: 50m über NN
  scd4x.setAutomaticSelfCalibration(1);
  scd4x.setTemperatureOffset(tempOffset);
  scd4x.startPeriodicMeasurement();
  
  Paint_DrawBitMap(gImage_init);
  EPD_1IN54_V2_Display(BlackImage);
  EPD_1IN54_V2_Sleep();
  //refreshes++;
  delay(3000); // Wait for co2 measurement
  initDone = true;
}

#ifdef USEAPP
void sendNotifications(uint16_t co2) {
  if (co2 > 1500) {
    if (!fensterAuf) {
      char message[128];
      snprintf(message, sizeof(message), "Fenster öffnen! CO2: %D ppm", co2);
      Blynk.notify(message);
      fensterAuf = true;
    }
  } else {
    if (co2 < 1400) fensterAuf = false;
  }

  if (co2 < 600) {
    if (!fensterZu) {
      char message[128];
      snprintf(message, sizeof(message), "Fenster schließen CO2: %D ppm", co2);
      Blynk.notify(message);
      fensterZu = true;
    }
  } else {
    if (co2 > 700) fensterZu = false;
  }
}
/* get led brightness slider */
/*BLYNK_WRITE(V4) {
  ledbrightness = param.asInt();
}*/
#endif

void setLED(uint16_t co2) {
  int red   =   pow((co2-400),2)/10000;       //= 0.13 * co2 - 90;
  int green = - pow((co2-400),2)/4500 + 255;  //-0.21 * co2 + 318;

  if (red < 0) red = 0;
  if (red > 255) red = 255;
  if (green < 0) green = 0;
  red = (int)(red * (ledbrightness/100.0));
  green = (int)(green * (ledbrightness/100.0));
    
  strip.setPixelColor(0, green, red, 0); //index, green, red, blue
  strip.show();
}

void lowBatteryMode() {
  scd4x.stopPeriodicMeasurement();
  Paint_Clear(BLACK);
                    //Xstart,Ystart,Xend,Yend
  Paint_DrawRectangle( 50,  40, 150, 90, WHITE, DOT_PIXEL_3X3, DRAW_FILL_EMPTY);
  Paint_DrawRectangle(150,  55, 160, 75, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
  Paint_DrawLine(      60, 100, 140, 30, WHITE, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
  Paint_DrawString_EN(45, 120, "Batterie", &Font20, BLACK, WHITE);
  Paint_DrawString_EN(45, 145, "aufladen", &Font20, BLACK, WHITE);

  EPD_1IN54_V2_Init();
  EPD_1IN54_V2_Display(BlackImage);
  EPD_1IN54_V2_DisplayPartBaseImage(BlackImage);
  EPD_1IN54_V2_DisplayPart(BlackImage); // sonst grauer schleier
  EPD_1IN54_V2_Sleep();

  esp_sleep_enable_ext0_wakeup((gpio_num_t)4,1);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO);   // RTC IO, sensors and ULP co-processor
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_AUTO); // RTC slow memory: auto
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);  // RTC fast memory
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);          // XTAL oscillator
  esp_sleep_pd_config(ESP_PD_DOMAIN_CPU, ESP_PD_OPTION_OFF);           // CPU core
  esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);           // Number of domains
  esp_deep_sleep_start();
}

void goto_deep_sleep() {
  esp_sleep_enable_ext0_wakeup((gpio_num_t)4,1);
  esp_sleep_enable_timer_wakeup(29000000);  // periodic measurement every 30 sec - 0.83 sec awake 
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO);    // RTC IO, sensors and ULP co-processor
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_AUTO); // RTC slow memory: auto
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);  // RTC fast memory
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);          // XTAL oscillator
  esp_sleep_pd_config(ESP_PD_DOMAIN_CPU, ESP_PD_OPTION_OFF);           // CPU core
  esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);           // Number of domains
  commingFromDeepSleep = true;
  esp_deep_sleep_start();
}

void goto_light_sleep(int ms){
  commingFromDeepSleep = false;
#ifdef USEAPP
  delay(3900);
#else
  esp_sleep_enable_timer_wakeup(ms*1000);  // periodic measurement every 5 sec -1.1 sec awake
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);    // RTC IO, sensors and ULP co-processor
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_AUTO); // RTC slow memory: auto
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);  // RTC fast memory
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);          // XTAL oscillator
  esp_sleep_pd_config(ESP_PD_DOMAIN_CPU, ESP_PD_OPTION_OFF);           // CPU core
  esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);           // Number of domains
  esp_light_sleep_start();
#endif
  //free(BlackImage);
  //BlackImage = NULL;
}

void setup() {
  DEV_Module_Init();

  /* scd4x */
  Wire.begin(33,34); // grün, gelb
  scd4x.begin(Wire);

  if (!initDone) initOnce();

  /* power */
  pinMode(4, INPUT);  // usb Power
  pinMode(5, INPUT);  // Battery Voltage
  
  BatteryMode = (digitalRead(4) == LOW);  
#ifdef USEAPP
  if (!BatteryMode) Blynk.begin(auth, ssid, pass); //(auth, ssid, pass, IPAddress(139,59,206,133), 8080);
#endif 
  strip.begin();
  if (!BatteryMode && commingFromDeepSleep) {
    delay(10);
    setLED(co2);
    scd4x.stopPeriodicMeasurement();   // stop low power measurement
    scd4x.setTemperatureOffset(tempOffset);
    scd4x.startPeriodicMeasurement();
    goto_light_sleep(4000); // Wait for co2 measurement
    //delay(5000);
  }
}

void loop(){
  BatteryMode = (digitalRead(4) == LOW); //check again in USB Power mode
  Paint_Clear(WHITE);
#ifdef USEAPP
  if (!BatteryMode) Blynk.run(); 
#endif

  uint16_t dataReady;
  uint16_t ready_error = scd4x.getDataReadyStatus(dataReady);
  if (ready_error || !(((dataReady | (~dataReady + 1)) >> 11) & 1)) {
    //Least significant 11 bits are 0 -> data not ready
    dataReadyErrorCount++;
    if (BatteryMode) goto_deep_sleep();
    else goto_light_sleep(4000);
    return; //otherwise continues running... why?
  }

  // Read co2 Measurement
  uint16_t new_co2;
  float temperature;
  float humidity;
  uint16_t error = scd4x.readMeasurement(new_co2, temperature, humidity);
  if (error) {
    char errorMessage[256];
    errorToString(error, errorMessage, 256);
    Paint_DrawString_EN(5, 40, errorMessage, &Font20, WHITE,BLACK);
  } else {
    if (new_co2 > 400) co2 = new_co2;
    /* CO2 */
    if (co2 < 1000) Paint_DrawNum(30, 65, co2, &bahn_big, BLACK, WHITE);
    else            Paint_DrawNum( 6, 65, co2, &bahn_big, BLACK, WHITE);
    Paint_DrawString_EN(142, 150, "ppmn", &bahn_sml, WHITE, BLACK);

    /* temperature */
    Paint_DrawNum(1, 5, temperature, &bahn_mid, BLACK, WHITE);
    Paint_DrawString_EN(60, 4, "*C", &bahn_sml, WHITE, BLACK);
    Paint_DrawString_EN(60, 32, ",", &bahn_sml, WHITE, BLACK);
    char nachkommer[2];
    sprintf(nachkommer, "%d", ((int)(floor(temperature*10))) % 10);
    Paint_DrawString_EN(71, 27, nachkommer, &bahn_sml, WHITE, BLACK);
  
    /* humidity */
    Paint_DrawNum(124, 5, humidity, &bahn_mid, BLACK, WHITE);
    Paint_DrawString_EN(184, 5, "%", &bahn_sml, WHITE, BLACK);   
  }

  if (!BatteryMode) {
    setLED(co2);
  } else if (!commingFromDeepSleep) {
    strip.clear();
    strip.show();
  }
      
#ifdef USEAPP
  if(!error && !BatteryMode) {
    if (refreshes % 6 == 1) {  //0, 6, 12
      Blynk.virtualWrite(V5, co2);
      Blynk.virtualWrite(V6, temperature);
      Blynk.virtualWrite(V7, humidity);
      sendNotifications(co2);
    }
  }
#endif    

  /* Print Battery % */  
  if (BatteryMode) {
    float voltage = ((analogRead(5)*3.33)/5084); //IO5 /was 5210
    voltage += 0.02; //offset measurement
    if (voltage < 3.1) lowBatteryMode();

    char batteryvolt[8] = "";
    dtostrf(voltage, 1, 3, batteryvolt);
    char volt[10] = "V";
    strcat(batteryvolt, volt);
    //Paint_DrawString_EN(100, 180, batteryvolt, &Font20, WHITE, BLACK);
        
    uint8_t percentage = 0;
    if (voltage <= 3.61) percentage = 46 * pow((voltage-3.1),2);
    else if (voltage > 4.19) percentage = 100;
    //else percentage = 2808.3808*pow(voltage,4) - 43560.9157*pow(voltage,3) + 252848.5888*pow(voltage,2) - 650767.4615*voltage + 626532.5703;
    else percentage = 2836.9625 * pow(voltage,4) - 43987.4889 * pow(voltage,3) + 255233.8134 * pow(voltage,2) - 656689.7123*voltage + 632041.7303; 
    
    //                  Xstart,Ystart,Xend,Yend
    Paint_DrawRectangle( 15, 145, 120, 169, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(120, 149, 125, 165, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(15, 145, 105*(percentage/100.0)+15, 169, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
  }

  //Paint_DrawNum(0, 0, (int32_t)refreshes, &Font20, BLACK, WHITE);
  char dataReadyErrorCount_c[10];
  sprintf(dataReadyErrorCount_c, "%d", dataReadyErrorCount);
  Paint_DrawString_EN(160, 180, dataReadyErrorCount_c, &Font20, WHITE, BLACK);
  if (refreshes == 1) {
    EPD_1IN54_V2_Init();
    EPD_1IN54_V2_Display(BlackImage);
    EPD_1IN54_V2_DisplayPartBaseImage(BlackImage);
    EPD_1IN54_V2_DisplayPart(BlackImage); // sonst grauer schleier
  } else {
    EPD_1IN54_V2_DisplayPart(BlackImage); // partial update
  }
  EPD_1IN54_V2_Sleep();
  if (refreshes == 40) {
    refreshes = 0; // force full update
  }
  refreshes++;

  if (BatteryMode) {
    if (!commingFromDeepSleep){
      scd4x.stopPeriodicMeasurement();
      scd4x.setTemperatureOffset(0.8); 
      scd4x.startLowPowerPeriodicMeasurement();
    }
    goto_deep_sleep();
  }
  goto_light_sleep(4000);
}
