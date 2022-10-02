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

void initEpdOnce() {
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
}

void displayInitTestMode() {
#ifdef EINK_1IN54V2
  Paint_DrawBitMap(gImage_init);
  Paint_DrawNum(125, 25, 1, &bahn_mid, BLACK, WHITE);
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

void displayWriteMeasuerments(uint16_t co2,uint16_t temperature, uint16_t humidity) {
  Paint_Clear(WHITE);
#ifdef EINK_1IN54V2
    /* co2 */
    if      (co2 > 9999) Paint_DrawNum(27, 78, co2, &bahn_mid, BLACK, WHITE);
    else if (co2 < 1000) Paint_DrawNum(30, 65, co2, &bahn_big, BLACK, WHITE);
    else                 Paint_DrawNum( 6, 65, co2, &bahn_big, BLACK, WHITE);
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
    if (temperature < 10.0f) Paint_DrawNum(30, 5, temperature, &bahn_mid, BLACK, WHITE);
    else                     Paint_DrawNum( 1, 5, temperature, &bahn_mid, BLACK, WHITE);
    int offset = 0;
#ifdef useFahrenheit
    if (temperature >= 100) offset = 29;
#endif
    Paint_DrawString_EN(60+offset, 4, unit, &bahn_sml, WHITE, BLACK);
    Paint_DrawString_EN(60+offset, 32, ",", &bahn_sml, WHITE, BLACK);
    char decimal[2];
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

    char decimal[2];
    sprintf(decimal, "%d", ((int)(temperature * 10)) % 10);
    Paint_DrawString_EN(145, 247, decimal, &bahn_mid, WHITE, BLACK);

    /* humidity */
    Paint_DrawNum(240, 220, humidity, &bahn_big, BLACK, WHITE);
    Paint_DrawString_EN(340, 220, "%", &bahn_sml, WHITE, BLACK);
#endif
}

void displayWriteError(char errorMessage[256]){
  Paint_DrawString_EN(5, 40, errorMessage, &Font20, WHITE, BLACK);
}

void displayWriteTestResults(float voltage, bool BatteryMode, uint16_t sensorStatus) {
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
}

void displayBattery(uint8_t percentage) {
#ifdef EINK_1IN54V2
                // Xstart,Ystart,Xend,Yend
  Paint_DrawRectangle( 1, 143, 92, 178, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY); //case
  Paint_DrawRectangle(92, 151, 97, 170, BLACK, DOT_PIXEL_2X2, DRAW_FILL_EMPTY); //nippel
  //Paint_DrawRectangle( 1, 143, 91*(percentage/100.0)+1, 178, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);

  char batterpercent[8] = "";
  dtostrf(percentage, 3, 0, batterpercent);
  char percent[10] = "%";
  strcat(batterpercent, percent);
  Paint_DrawString_EN(20,149, batterpercent, &bahn_sml, WHITE, BLACK);

  /* invert the filled part of the Battery */
  for (int x = (200-(90*(percentage/100.0))); x < (200-2); x++) {
    for (int y = 145/8; y < 179/8; y++) {
      BlackImage[y+x*25] = ~BlackImage[y+x*25];
    }
  }
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
  if (refreshes == 1) {
    EPD_1IN54_V2_Init();
    EPD_1IN54_V2_DisplayPartBaseImage(BlackImage);
  } else {
    EPD_1IN54_V2_Init_Partial();
    if(comingFromDeepSleep) EPD_1IN54_V2_writePrevImage(BlackImage);
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
}
