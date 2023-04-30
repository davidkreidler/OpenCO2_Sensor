/*
   ESP32S2 Dev Module
   Required Arduino libraries:
   - esp32 waveshare epd
   - Adafruit DotStar
*/

/* Includes display */
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include <stdlib.h>
#include "pictures.h"
#define EINK_1IN54V2
//#define EINK_4IN2

/* welcome */
#include <EEPROM.h>

RTC_DATA_ATTR int refreshes = 1;
RTC_DATA_ATTR UBYTE *BlackImage;

sFONT big=bahn_big; //gotham_big nothing_big bahn_big
sFONT mid=bahn_mid; //gotham_mid nothing_mid bahn_mid
sFONT sml=bahn_sml; //gotham_sml nothing_sml bahn_sml

void displayWelcome() {
  initEpdOnce();
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
  EEPROM.begin(2); // EEPROM_SIZE
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

void displayRainbow() {
#ifdef EINK_1IN54V2
  EPD_1IN54_V2_Init();
  Paint_DrawBitMap(gImage_rainbow);
  EPD_1IN54_V2_Display(BlackImage);
  EPD_1IN54_V2_Sleep();
#endif
}

void initEpdOnce() {
#ifdef EINK_1IN54V2
  BlackImage = (UBYTE *)malloc(5200);
  Paint_NewImage(BlackImage, EPD_1IN54_V2_WIDTH, EPD_1IN54_V2_HEIGHT, 270, WHITE);
  EPD_1IN54_V2_Init();
#endif
#ifdef EINK_4IN2
  BlackImage = (UBYTE *)malloc(15400);
  Paint_NewImage(BlackImage, EPD_4IN2_WIDTH, EPD_4IN2_HEIGHT, 0, WHITE);
  EPD_4IN2_Init_Fast();
#endif
  Paint_Clear(WHITE);
}

void displayInitTestMode() {
#ifdef EINK_1IN54V2
  Paint_DrawBitMap(gImage_init);
  Paint_DrawNum(125, 25, 1, &mid, BLACK, WHITE);
  EPD_1IN54_V2_Display(BlackImage);
#endif
#ifdef EINK_4IN2
  EPD_4IN2_Display(BlackImage);
#endif
}

void displayInit() {
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
}

void displayLowBattery() {
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
}

void displayWriteMeasuerments(uint16_t co2, float temperature, float humidity) {
  Paint_Clear(WHITE);
#ifdef EINK_1IN54V2
    /* co2 */
    if      (co2 > 9999) Paint_DrawNum(27, 78, co2, &mid, BLACK, WHITE);
    else if (co2 < 1000) Paint_DrawNum(30, 65, co2, &big, BLACK, WHITE);
    else                 Paint_DrawNum( 6, 65, co2, &big, BLACK, WHITE);
    Paint_DrawString_EN(100, 150, "CO", &Font24, WHITE, BLACK);
    Paint_DrawNum(131, 160, 2, &Font20, BLACK, WHITE);
    Paint_DrawString_EN(144, 150, "ppm", &Font24, WHITE, BLACK);

    /* temperature */
//#define useFahrenheit
#ifdef useFahrenheit
    temperature = (temperature*1.8f)+32.0f; // convert to °F
    char unit[3] = "*F";
#else
    char unit[3] = "*C";
#endif
    if (temperature < 10.0f) Paint_DrawNum(30, 5, temperature, &mid, BLACK, WHITE);
    else                     Paint_DrawNum( 1, 5, temperature, &mid, BLACK, WHITE);
    int offset = 0;
#ifdef useFahrenheit
    if (temperature >= 100) offset = 29;
#endif
    Paint_DrawString_EN(60+offset, 4, unit, &sml, WHITE, BLACK);
    Paint_DrawString_EN(60+offset, 32, ",", &sml, WHITE, BLACK);
    char decimal[4];
    sprintf(decimal, "%d", ((int)(temperature * 10)) % 10);
    Paint_DrawString_EN(71+offset, 27, decimal, &sml, WHITE, BLACK);

    /* humidity */
    Paint_DrawNum(124, 5, humidity, &mid, BLACK, WHITE);
    Paint_DrawString_EN(184, 5, "%", &sml, WHITE, BLACK);
#endif /* EINK_1IN54V2 */
#ifdef EINK_4IN2
    /* co2 */
                                 // Xstart,Ystart
    if      (co2 > 9999) Paint_DrawNum(102, 88, co2, &big, BLACK, WHITE);
    else if (co2 < 1000) Paint_DrawNum(196, 88, co2, &big, BLACK, WHITE);
    else                 Paint_DrawNum(149, 88, co2, &big, BLACK, WHITE);
    Paint_DrawString_EN(337, 143, "ppmn", &sml, WHITE, BLACK);

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
    Paint_DrawNum(       94, 145,   2, &sml, BLACK, WHITE);                    //2

    /* temperature */
    if (temperature < 10.0f) Paint_DrawNum(69, 220, temperature, &big, BLACK, WHITE);
    else                     Paint_DrawNum(32, 220, temperature, &big, BLACK, WHITE);
    Paint_DrawString_EN(140, 220, "*C", &sml, WHITE, BLACK);
    Paint_DrawLine(137, 287, 137, 287, BLACK, DOT_PIXEL_4X4, LINE_STYLE_SOLID);

    char decimal[4];
    sprintf(decimal, "%d", ((int)(temperature * 10)) % 10);
    Paint_DrawString_EN(145, 247, decimal, &mid, WHITE, BLACK);

    /* humidity */
    Paint_DrawNum(240, 220, humidity, &big, BLACK, WHITE);
    Paint_DrawString_EN(340, 220, "%", &sml, WHITE, BLACK);
#endif
}

void displayWriteError(char errorMessage[256]){
  Paint_DrawString_EN(5, 40, errorMessage, &Font20, WHITE, BLACK);
}

/* TEST_MODE */
void displayWriteTestResults(float voltage, bool BatteryMode, uint16_t sensorStatus, uint16_t serial0, uint16_t serial1, uint16_t serial2) {
  char batteryvolt[8] = "";
  dtostrf(voltage, 1, 3, batteryvolt);
  char volt[10] = "V";
  strcat(batteryvolt, volt);
  Serial.print(voltage);
  Serial.print('\t');
  Paint_DrawString_EN(0, 138, batteryvolt, &Font16, WHITE, BLACK);

  char BatteryMode_s[9] = "";
  strcat(BatteryMode_s, "USB-C:");
  strcat(BatteryMode_s, BatteryMode ? "no" : "yes");
  Serial.print(BatteryMode);
  Serial.print('\t');
  Paint_DrawString_EN(77, 138, BatteryMode_s, &Font16, WHITE, BLACK);

  char sensorStatus_s[20] = "";
  strcat(sensorStatus_s, "SCD4x:");
  if (sensorStatus == 0) {
    strcat(sensorStatus_s, "ok");
  } else {
    char snum[5];
    itoa(sensorStatus, snum, 10);
    strcat(sensorStatus_s, snum);
  }
  Serial.print(sensorStatus);
  Serial.print('\t');
  Paint_DrawString_EN(0, 154, sensorStatus_s, &Font16, WHITE, BLACK);

  char mac[20];
  snprintf(mac, 20, "MacAdd:%llX", ESP.getEfuseMac());
  Serial.print(ESP.getEfuseMac(), HEX);
  Serial.print('\t');
  Paint_DrawString_EN(0, 176, mac, &Font12, WHITE, BLACK);

  char serial[20]= "Serial:";
  char hex[4];
  sprintf(hex, "%4X", serial0);
  Serial.print(hex);
  strcat(serial, hex);
  sprintf(hex, "%4X", serial1);
  Serial.print(hex);
  strcat(serial, hex);
  sprintf(hex, "%4X", serial2);
  Serial.println(hex);
  strcat(serial, hex);
  //Serial.print('\n');
  Paint_DrawString_EN(0, 188, serial, &Font12, WHITE, BLACK);

  Paint_DrawNum(158, 180, (int32_t)refreshes, &Font16, BLACK, WHITE);
}

void displayBattery(uint8_t percentage) {
  char batterpercent[8] = "";
  dtostrf(percentage, 3, 0, batterpercent);
  char percent[10] = "%";
  strcat(batterpercent, percent);

#ifdef EINK_1IN54V2
                // Xstart,Ystart,Xend,Yend
  Paint_DrawRectangle( 1, 143, 92, 178, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY); //case
  Paint_DrawRectangle(92, 151, 97, 170, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY); //nippel
  //Paint_DrawRectangle( 1, 143, 91*(percentage/100.0)+1, 178, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
  Paint_DrawString_EN(20,149, batterpercent, &sml, WHITE, BLACK);

  /* invert the filled part of the Battery */
  for (int x = (200-(90*(percentage/100.0))); x < (200-2); x++) {
    for (int y = 145/8; y < 179/8; y++) {
      BlackImage[y+x*25] = ~BlackImage[y+x*25];
    }
  }
#endif
#ifdef EINK_4IN2
  Paint_DrawRectangle(279, 10, 385, 37, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY); //case
  Paint_DrawRectangle(385, 16, 390, 31, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY); //nippel
  Paint_DrawString_EN(300, 12, batterpercent, &sml, WHITE, BLACK);

  /* invert the filled part of the Battery */
  int X_start=280;
  int X_end=280+(106*(percentage/100.0));
  int Y_start=11;
  int Y_end=35;
  for (int j = 0; j < Y_end - Y_start; j++) {
    for (int i = 0; i < (X_end - X_start)/8; i++) {
      BlackImage[(Y_start + j)*50 + X_start/8 + i] = ~BlackImage[(Y_start + j)*50 + X_start/8 + i];
    }
  }
#endif
}

void updateDisplay(bool comingFromDeepSleep) {
#ifdef EINK_1IN54V2
//#define invertDisplay
#ifdef invertDisplay
  for (int x = 0; x < 200; x++) {
    for (int y = 0; y < 200/8; y++) {
      BlackImage[y+x*25] = ~BlackImage[y+x*25];
    }
  }
#endif
  if (comingFromDeepSleep && !BatteryMode) refreshes = 1;
  if (refreshes == 1) {
    // Full update
    EPD_1IN54_V2_Init();
    EPD_1IN54_V2_DisplayPartBaseImage(BlackImage);
  } else { 
    // Partial update
    if(comingFromDeepSleep && HWSubRev > 1) {
      EPD_1IN54_V2_Init_Partial_After_Powerdown();
      EPD_1IN54_V2_writePrevImage(BlackImage);
    } else {
      EPD_1IN54_V2_Init_Partial();
    }
    EPD_1IN54_V2_DisplayPart(BlackImage); 
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
}
