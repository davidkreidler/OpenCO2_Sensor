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
extern bool BatteryMode;
extern bool comingFromDeepSleep;
extern int HWSubRev;

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

void displayImage(const unsigned char* image_buffer) {
#ifdef EINK_1IN54V2
  EPD_1IN54_V2_Init();
  Paint_DrawBitMap(image_buffer);
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
  Paint_DrawString_EN(1, 1, VERSION, &Font16, WHITE, BLACK);
  Paint_DrawNum(125, 25, 1, &mid, BLACK, WHITE); // 15 sec for testmode
  EPD_1IN54_V2_Display(BlackImage);
#endif
#ifdef EINK_4IN2
  Paint_DrawString_EN(1, 1, VERSION, &Font16, WHITE, BLACK);
  EPD_4IN2_Display(BlackImage);
#endif
}

void displayInit() {
#ifdef EINK_1IN54V2
  Paint_DrawBitMap(gImage_init);
  Paint_DrawString_EN(1, 1, VERSION, &Font16, WHITE, BLACK);
  EPD_1IN54_V2_Display(BlackImage);
  EPD_1IN54_V2_Sleep();
#endif
#ifdef EINK_4IN2
  Paint_DrawString_EN(1, 1, VERSION, &Font16, WHITE, BLACK);
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
#ifdef ENGLISH
  Paint_DrawString_EN(45, 120, "Charge", &Font20, BLACK, WHITE);
  Paint_DrawString_EN(45, 145, "Battery", &Font20, BLACK, WHITE);
#else
  Paint_DrawString_EN(45, 120, "Batterie", &Font20, BLACK, WHITE);
  Paint_DrawString_EN(45, 145, "aufladen", &Font20, BLACK, WHITE);
#endif

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
    if (useFahrenheit) temperature = (temperature*1.8f)+32.0f; // convert to °F
    if (temperature < 10.0f) Paint_DrawNum(30, 5, temperature, &mid, BLACK, WHITE);
    else                     Paint_DrawNum( 1, 5, temperature, &mid, BLACK, WHITE);
    int offset = 0;
    if (temperature >= 100) offset = 29; // for Fahrenheit

    Paint_DrawString_EN(60+offset, 4, useFahrenheit? "*F" : "*C", &sml, WHITE, BLACK);
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

#include "qrcode.h"
#define ESP_QRCODE_CONFIG() (esp_qrcode_config_t) { \
    .display_func = draw_qr_code, \
    .max_qrcode_version = 26, \
    .qrcode_ecc_level = ESP_QRCODE_ECC_LOW, \
}
bool invertedQR;
void draw_qr_code(const uint8_t * qrcode) {
  int qrcodeSize = esp_qrcode_get_size(qrcode);
  int scaleFactor = 1;
  UWORD Color = invertedQR? WHITE : BLACK;

  // min. 16px border: 200px - 2*16px = 168px usable
  if (qrcodeSize < 24)      scaleFactor = 7;
  else if (qrcodeSize < 28) scaleFactor = 6;
  else if (qrcodeSize < 34) scaleFactor = 5;
  else if (qrcodeSize < 42) scaleFactor = 4;
  else if (qrcodeSize < 56) scaleFactor = 3;
  else if (qrcodeSize < 84) scaleFactor = 2;
  
  int Start = (200 - (qrcodeSize * scaleFactor)) / 2;
  Paint_Clear(invertedQR? BLACK : WHITE);
  for (int y=0; y < qrcodeSize; y++) {
    for (int x=0; x < qrcodeSize; x++) {
      if (esp_qrcode_get_module(qrcode, x, y)) {
        if (scaleFactor > 1) Paint_DrawRectangle(Start + x * scaleFactor,
                                                 Start + y * scaleFactor,
                                                 Start + x * scaleFactor + scaleFactor, 
                                                 Start + y * scaleFactor + scaleFactor,
                                                 Color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        else Paint_SetPixel(x + (200-qrcodeSize)/2, y + (200-qrcodeSize)/2, Color);
      }
    }
  }
}

void displayNoHistory() {
  Paint_Clear(WHITE);
#ifdef ENGLISH
  Paint_DrawString_EN(15, 52, "Disconnect", &Font24, WHITE, BLACK);
  Paint_DrawString_EN(15, 76, " power to ", &Font24, WHITE, BLACK);
  Paint_DrawString_EN(15, 100, "  record  ", &Font24, WHITE, BLACK);
  Paint_DrawString_EN(15+8, 124, " History ", &Font24, WHITE, BLACK);
#else
  Paint_DrawString_EN(58, 52, "Strom", &Font24, WHITE, BLACK);
  Paint_DrawString_EN(15-8, 76, "trennen, um", &Font24, WHITE, BLACK);
  Paint_DrawString_EN(15-8, 100, "Historie zu", &Font24, WHITE, BLACK);
  Paint_DrawString_EN(15+8, 124, "speichern", &Font24, WHITE, BLACK);
#endif
  updateDisplay();
}

void displayHistory(uint16_t measurements[24][120]) {
  extern uint8_t halfminute, hour, qrcodeNumber;
  char buffer[5*120+1];
  int numEnties = halfminute;
  if (hour > qrcodeNumber) numEnties = 120; // display all values included in previous hours

  for (int i=0; i<numEnties; i++) {
    char tempStr[6];
    snprintf(tempStr, sizeof(tempStr), "%d", measurements[qrcodeNumber][i]);

    if (i == 0) snprintf(buffer, sizeof(buffer), "%s", tempStr);
    else snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), " %s", tempStr);
  }

  invertedQR = false;
  esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG();
  esp_qrcode_generate(&cfg, buffer);

  if (qrcodeNumber+1 >= 10) Paint_DrawNum(200-5*11, 200-16, qrcodeNumber+1, &Font16, BLACK, WHITE);
  else Paint_DrawNum(200-4*11, 200-16, qrcodeNumber+1, &Font16, BLACK, WHITE);
  Paint_DrawString_EN(200-3*11, 200-16, "/", &Font16, WHITE, BLACK);
  Paint_DrawNum(200-2*11, 200-16, hour+1, &Font16, BLACK, WHITE);
#ifdef ENGLISH
  Paint_DrawString_EN(1, 1, "Wait 20sec to exit", &Font16, WHITE, BLACK);
#else
  Paint_DrawString_EN(1, 1, "20 Sek. warten", &Font16, WHITE, BLACK);
#endif
  updateDisplay();
}

void displayMenu(uint8_t selectedOption) {
  Paint_Clear(WHITE);
  Paint_DrawString_EN(66, 0, "Menu", &Font24, WHITE, BLACK);
  Paint_DrawLine(10, 23, 190, 23, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

  for (int i=0; i<NUM_OPTIONS; i++) {
    Paint_DrawString_EN(5, 25*(i+1), menuItems[i], &Font24, WHITE, BLACK);
  }
  invertSelected(selectedOption);
  updateDisplay();
}

void displayLEDMenu(uint8_t selectedOption) {
  Paint_Clear(WHITE);
  Paint_DrawString_EN(75, 0, "LED", &Font24, WHITE, BLACK);
  Paint_DrawLine(10, 23, 190, 23, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

  for (int i=0; i<NUM_LED_OPTIONS; i++) {
    Paint_DrawString_EN(5, 25*(i+1), LEDmenuItems[i], &Font24, WHITE, BLACK);
  }

#ifdef ENGLISH
  Paint_DrawString_EN(149, 25, (LEDonBattery? "ON":"OFF"), &Font24, WHITE, BLACK);
  Paint_DrawString_EN(149, 50, (LEDonUSB? "ON":"OFF"), &Font24, WHITE, BLACK);
#else
  Paint_DrawString_EN(149, 25, (LEDonBattery? "AN":"AUS"), &Font24, WHITE, BLACK);
  Paint_DrawString_EN(149, 50, (LEDonUSB? "AN":"AUS"), &Font24, WHITE, BLACK);
#endif
  Paint_DrawString_EN(149, 75, (useSmoothLEDcolor? "1":"2"), &Font24, WHITE, BLACK);
  Paint_DrawString_EN(166, 75, "/2", &Font24, WHITE, BLACK);
  Paint_DrawNum(149, 100, (int32_t)(ledbrightness/20+1), &Font24, BLACK, WHITE); // 5 25 45 65 85
  Paint_DrawString_EN(166, 100, "/5", &Font24, WHITE, BLACK);

  invertSelected(selectedOption);
  updateDisplay();
}

void displayOptionsMenu(uint8_t selectedOption) {
  Paint_Clear(WHITE);
  Paint_DrawString_EN(40, 0, "DISPLAY", &Font24, WHITE, BLACK);
  Paint_DrawLine(10, 23, 190, 23, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

  for (int i=0; i<NUM_DISPLAY_OPTIONS; i++) {
    Paint_DrawString_EN(5, 25*(i+1), OptionsMenuItems[i], &Font24, WHITE, BLACK);
  }
  Paint_DrawString_EN(166, 50, (useFahrenheit? "*F":"*C"), &Font24, WHITE, BLACK);
  invertSelected(selectedOption);
  updateDisplay();
}

void invertSelected(uint8_t selectedOption) {
#ifdef EINK_1IN54V2
  for (int x = 0; x < 200; x++) {
    for (int y = 25*(selectedOption+1)/8; y < 25*(selectedOption+2)/8; y++) {
      BlackImage[y+x*25] = ~BlackImage[y+x*25];
    }
  }
#endif /* EINK_1IN54V2 */
#ifdef EINK_4IN2
  int Y_start = 25 * (selectedOption + 1);
  int Y_end   = 25 * (selectedOption + 2);
  for (int j = 0; j < Y_end - Y_start; j++) {
    for (int i = 0; i < 200/8; i++) {
      BlackImage[(Y_start + j) * 50 + i] = ~BlackImage[(Y_start + j) * 50 + i];
    }
  }
#endif
}

void displayCalibrationWarning() {
  Paint_Clear(BLACK);

  // Exclamation Mark !
  Paint_DrawLine( 100,  50, 100,  85, WHITE, DOT_PIXEL_4X4, LINE_STYLE_SOLID);
  Paint_DrawCircle(100, 105,   5, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);

                // Xstart,Ystart,Xend,Yend
  Paint_DrawLine( 100,  20,  35, 120, WHITE, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
  Paint_DrawLine( 100,  20, 165, 120, WHITE, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
  Paint_DrawLine(  37, 120, 163, 120, WHITE, DOT_PIXEL_4X4, LINE_STYLE_SOLID);

#ifdef ENGLISH 
  Paint_DrawString_EN(16, 132, "Calibration!", &Font20, BLACK, WHITE);
  Paint_DrawString_EN(1, 152, "Put Sensor outside", &Font16, BLACK, WHITE);
  Paint_DrawString_EN(1, 168, "for 3+ minutes. Or", &Font16, BLACK, WHITE);
  Paint_DrawString_EN(1, 184, "hold knob to stop", &Font16, BLACK, WHITE);
#else
  Paint_DrawString_EN(16, 132, "Kalibration!", &Font20, BLACK, WHITE);
  Paint_DrawString_EN(1, 152, "Sensor fur 3+ min.", &Font16, BLACK, WHITE);
  Paint_DrawString_EN(1, 168, "nach drausen legen", &Font16, BLACK, WHITE);
  Paint_DrawString_EN(1, 184, "Knopf = abbrechen", &Font16, BLACK, WHITE);
#endif

  updateDisplay();
}

void displayWiFi(bool useWiFi) {
  Paint_Clear(BLACK);

  if (useWiFi) {
    if (BatteryMode) {
#ifdef ENGLISH
      Paint_DrawString_EN(23, 52, "Wi-Fi: ON", &Font24, BLACK, WHITE);
      Paint_DrawString_EN(40, 76, "Connect", &Font24, BLACK, WHITE);
      Paint_DrawString_EN(48, 100, "Power!", &Font24, BLACK, WHITE);
#else
      Paint_DrawString_EN(32, 52, "WLAN: AN", &Font24, BLACK, WHITE);
      Paint_DrawString_EN(57, 76, "Strom", &Font24, BLACK, WHITE);
      Paint_DrawString_EN(15, 100, "verbinden!", &Font24, BLACK, WHITE);
#endif
    } else {
      invertedQR = true;
      if (WiFi.status() == WL_CONNECTED) {
        char buffer[40] = "http://";
        String ip = WiFi.localIP().toString();
        strcat(buffer, ip.c_str());
        strcat(buffer, ":9925");
        esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG();
        esp_qrcode_generate(&cfg, buffer);

        Paint_DrawString_EN(1,   1, ip.c_str(), &Font16, BLACK, WHITE);
        Paint_DrawString_EN(1, 184, ":9925", &Font16, BLACK, WHITE);
      } else {
        char const *buffer = "WIFI:T:;S:OpenCO2 Sensor;P:;;";
        esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG();
        esp_qrcode_generate(&cfg, buffer);

        Paint_DrawString_EN(1,   1, " 'OpenCO2 Sensor'", &Font16, BLACK, WHITE);
        Paint_DrawString_EN(1, 184, "http://192.168.4.1", &Font16, BLACK, WHITE);
      }
    }
  } else { /* No WiFi symbol */
    Paint_DrawCircle(100, 100,  25, WHITE, DOT_PIXEL_4X4, DRAW_FILL_EMPTY);
    Paint_DrawCircle(100, 100,  50, WHITE, DOT_PIXEL_4X4, DRAW_FILL_EMPTY);
    Paint_DrawCircle(100, 100,  75, WHITE, DOT_PIXEL_4X4, DRAW_FILL_EMPTY);
                  // Xstart,Ystart,Xend,Yend
    Paint_DrawLine(  0,   4, 100, 104, BLACK, DOT_PIXEL_8X8, LINE_STYLE_SOLID);
    Paint_DrawLine(  0,  12, 100, 112, BLACK, DOT_PIXEL_8X8, LINE_STYLE_SOLID);
    Paint_DrawRectangle(0, 50, 60, 100, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawLine(200,   4, 100, 104, BLACK, DOT_PIXEL_8X8, LINE_STYLE_SOLID);
    Paint_DrawLine(200,  12, 100, 112, BLACK, DOT_PIXEL_8X8, LINE_STYLE_SOLID);
    Paint_DrawRectangle(140, 50, 200, 200, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawRectangle(0, 100, 200, 200, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawCircle(100, 95,   4, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    Paint_DrawLine( 60, 90, 140, 10, WHITE, DOT_PIXEL_4X4, LINE_STYLE_SOLID);
#ifdef ENGLISH
    Paint_DrawString_EN(15, 132, "Wi-Fi: OFF", &Font24, BLACK, WHITE);
#else
    Paint_DrawString_EN(15, 132, "WLAN: AUS", &Font24, BLACK, WHITE);
#endif
  }
  updateDisplay();
}

void displayWiFiStrengh() {
  if (WiFi.status() != WL_CONNECTED) {
    const unsigned char wifiAP[] = {
      0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,
      0XFB,0XFF,0XFF,0XFF,0XFF,0X00,0X1F,0XFF,0XFF,0XF8,0X00,0X07,0XFF,0XFF,0XF0,0XFF,
      0XC1,0XFF,0XFF,0XE3,0XFF,0XF8,0XFF,0XFF,0XC7,0XFF,0XFC,0XFF,0XFF,0XFF,0X80,0X7F,
      0XFF,0XFF,0XFE,0X00,0X0F,0XFF,0XFF,0XFC,0X3F,0X87,0XFF,0XFF,0XFC,0XFF,0XE7,0XFF,
      0XFF,0XFF,0XFB,0XFF,0XFF,0XC7,0XFF,0XC0,0X7F,0XFE,0X07,0XFF,0X80,0X3F,0XE0,0X07,
      0XFF,0X9F,0X3F,0X00,0X03,0XFF,0XFF,0XF0,0X00,0X03,0XFF,0XF1,0XE0,0X00,0X03,0XFF,
      0XE1,0XC0,0X00,0X03,0XFF,0XE1,0XC0,0X00,0X03,0XFF,0XF1,0XE0,0X00,0X03,0XFF,0XFF,
      0XF8,0X00,0X03,0XFF,0X9F,0X3F,0X00,0X03,0XFF,0X80,0X3F,0XF0,0X07,0XFF,0XC0,0XFF,
      0XFE,0X07,0XFF,0XFF,0XFF,0XFF,0XE7,0XFC,0XFF,0XE7,0XFF,0XFF,0XFC,0X3F,0X87,0XFF,
      0XFF,0XFE,0X00,0X1F,0XFF,0XFF,0XFF,0X80,0X7F,0XFF,0XFF,0XCF,0XFF,0XFC,0XFF,0XFF,
      0XE3,0XFF,0XF8,0XFF,0XFF,0XF0,0XFF,0XC1,0XFF,0XFF,0XF8,0X00,0X07,0XFF,0XFF,0XFF,
      0X00,0X1F,0XFF,0XFF,0XFF,0XF3,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,
      0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF
    };
    Paint_DrawImage(wifiAP, 150, 150, 40, 40);
    return;
  }

  const unsigned char wifiFullIcon[] = {
    0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0X9F,0XFF,0XFF,0XFF,0XFF,0X87,0XFF,0XFF,0XFF,0XFF,
    0X03,0XFF,0XFF,0XFF,0XFE,0X00,0XFF,0XFF,0XFF,0XFE,0X00,0X7F,0XFF,0XFF,0XFE,0X00,
    0X3F,0XFF,0XFF,0XFC,0X00,0X0F,0XFF,0XFF,0XFC,0X00,0X07,0XFF,0XFF,0XF8,0X00,0X03,
    0XFF,0XFF,0XF8,0X00,0X01,0XFF,0XFF,0XF8,0X00,0X00,0X7F,0XFF,0XF8,0X00,0X00,0X3F,
    0XFF,0XF0,0X00,0X00,0X0F,0XFF,0XF0,0X00,0X00,0X07,0XFF,0XF0,0X00,0X00,0X03,0XFF,
    0XF0,0X00,0X00,0X00,0XFF,0XF0,0X00,0X00,0X00,0X7F,0XF0,0X00,0X00,0X00,0X3F,0XF0,
    0X00,0X00,0X00,0X0F,0XF0,0X00,0X00,0X00,0X0F,0XF0,0X00,0X00,0X00,0X3F,0XF0,0X00,
    0X00,0X00,0X7F,0XF0,0X00,0X00,0X00,0XFF,0XF0,0X00,0X00,0X03,0XFF,0XF0,0X00,0X00,
    0X07,0XFF,0XF0,0X00,0X00,0X0F,0XFF,0XF8,0X00,0X00,0X3F,0XFF,0XF8,0X00,0X00,0X7F,
    0XFF,0XF8,0X00,0X01,0XFF,0XFF,0XF8,0X00,0X03,0XFF,0XFF,0XFC,0X00,0X07,0XFF,0XFF,
    0XFC,0X00,0X0F,0XFF,0XFF,0XFE,0X00,0X3F,0XFF,0XFF,0XFE,0X00,0X7F,0XFF,0XFF,0XFE,
    0X01,0XFF,0XFF,0XFF,0XFF,0X03,0XFF,0XFF,0XFF,0XFF,0X87,0XFF,0XFF,0XFF,0XFF,0X9F,
    0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF
  };
  const unsigned char wifiMediumIcon[] = {
    0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0X8F,0XFF,0XFF,0XFF,0XFF,0X87,0XFF,0XFF,0XFF,0XFF,
    0X03,0XFF,0XFF,0XFF,0XFE,0X00,0XFF,0XFF,0XFF,0XFE,0X30,0X7F,0XFF,0XFF,0XFE,0X38,
    0X3F,0XFF,0XFF,0XFC,0X7E,0X0F,0XFF,0XFF,0XFC,0X7F,0X07,0XFF,0XFF,0XF8,0XFF,0XC3,
    0XFF,0XFF,0XF8,0XFF,0XE0,0XFF,0XFF,0XF8,0XFF,0XE0,0X7F,0XFF,0XF1,0XFF,0XC0,0X3F,
    0XFF,0XF1,0XFF,0XC0,0X0F,0XFF,0XF1,0XFF,0XC0,0X07,0XFF,0XF1,0XFF,0XC0,0X03,0XFF,
    0XF3,0XFF,0X80,0X00,0XFF,0XF3,0XFF,0X80,0X00,0X7F,0XF3,0XFF,0X80,0X00,0X3F,0XF3,
    0XFF,0X80,0X00,0X0F,0XF3,0XFF,0X80,0X00,0X0F,0XF3,0XFF,0X80,0X00,0X3F,0XF3,0XFF,
    0X80,0X00,0X7F,0XF3,0XFF,0X80,0X00,0XFF,0XF1,0XFF,0XC0,0X03,0XFF,0XF1,0XFF,0XC0,
    0X07,0XFF,0XF1,0XFF,0XC0,0X0F,0XFF,0XF1,0XFF,0XE0,0X3F,0XFF,0XF8,0XFF,0XE0,0X7F,
    0XFF,0XF8,0XFF,0XE0,0XFF,0XFF,0XF8,0XFF,0XC3,0XFF,0XFF,0XFC,0X7F,0X07,0XFF,0XFF,
    0XFC,0X7E,0X0F,0XFF,0XFF,0XFE,0X38,0X3F,0XFF,0XFF,0XFE,0X30,0X7F,0XFF,0XFF,0XFE,
    0X00,0XFF,0XFF,0XFF,0XFF,0X03,0XFF,0XFF,0XFF,0XFF,0X87,0XFF,0XFF,0XFF,0XFF,0X8F,
    0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF
  };
  const unsigned char wifiLowIcon[] = {
    0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XCF,0XFF,0XFF,0XFF,0XFF,0X83,0XFF,0XFF,0XFF,0XFF,
    0X81,0XFF,0XFF,0XFF,0XFF,0X00,0XFF,0XFF,0XFF,0XFF,0X18,0X3F,0XFF,0XFF,0XFE,0X3C,
    0X1F,0XFF,0XFF,0XFE,0X3F,0X0F,0XFF,0XFF,0XFC,0X7F,0X87,0XFF,0XFF,0XFC,0X7F,0XC3,
    0XFF,0XFF,0XFC,0X7F,0XE0,0XFF,0XFF,0XFC,0XFF,0XF8,0X7F,0XFF,0XF8,0XFF,0XFC,0X3F,
    0XFF,0XF8,0XFF,0XFE,0X0F,0XFF,0XF8,0XFF,0XFF,0X07,0XFF,0XF9,0XFF,0XFF,0X83,0XFF,
    0XF9,0XFF,0XFF,0XE1,0XFF,0XF1,0XFF,0XFF,0XF0,0XFF,0XF1,0XFF,0XFF,0XF8,0X3F,0XF1,
    0XFF,0XFF,0XFE,0X1F,0XF1,0XFF,0XFF,0XFE,0X1F,0XF1,0XFF,0XFF,0XF8,0X3F,0XF1,0XFF,
    0XFF,0XF0,0XFF,0XF9,0XFF,0XFF,0XE1,0XFF,0XF9,0XFF,0XFF,0X83,0XFF,0XF8,0XFF,0XFF,
    0X07,0XFF,0XF8,0XFF,0XFE,0X1F,0XFF,0XF8,0XFF,0XFC,0X3F,0XFF,0XFC,0XFF,0XF8,0X7F,
    0XFF,0XFC,0X7F,0XE0,0XFF,0XFF,0XFC,0X7F,0XC1,0XFF,0XFF,0XFC,0X7F,0X87,0XFF,0XFF,
    0XFE,0X3E,0X0F,0XFF,0XFF,0XFE,0X3C,0X1F,0XFF,0XFF,0XFF,0X18,0X3F,0XFF,0XFF,0XFF,
    0X00,0XFF,0XFF,0XFF,0XFF,0X81,0XFF,0XFF,0XFF,0XFF,0X83,0XFF,0XFF,0XFF,0XFF,0XCF,
    0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF
  };
  
  int32_t signalStrength = WiFi.RSSI();
                                                                 //xStart,yStart,W_Image,H_Image 
  if        (signalStrength > -55) { Paint_DrawImage(wifiFullIcon,   142, 101, 40, 40); 
  } else if (signalStrength > -70) { Paint_DrawImage(wifiMediumIcon, 142, 101, 40, 40);
  } else {                           Paint_DrawImage(wifiLowIcon,    142, 101, 40, 40); }

  /* time */
  struct tm timeinfo;
  getLocalTime(&timeinfo);  
  //char time[100];
  //sprintf(time, "%d-%02d-%02d %02d:%02d:%02d", (timeinfo.tm_year)+1900, timeinfo.tm_mday, (timeinfo.tm_mon)+1, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  char hour[3];
  char minute[3];
  sprintf(hour, "%02d", timeinfo.tm_hour);
  sprintf(minute, "%02d", timeinfo.tm_min);
  Paint_DrawString_EN(1, 150, hour, &sml, WHITE, BLACK);
                // XCenter,YCenter,radius
  Paint_DrawCircle(1+13*2+3, 150+10, 2, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
  Paint_DrawCircle(1+13*2+3, 150+20, 2, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
  Paint_DrawString_EN(1+13*2+6, 150, minute, &sml, WHITE, BLACK);
}

void displayinfo() {
  extern uint16_t serial0, serial1, serial2;
  Paint_Clear(WHITE);

  Paint_DrawString_EN(0, 1, "MAC Address:", &Font16, WHITE, BLACK);
  Paint_DrawString_EN(0, 17, WiFi.macAddress().c_str(), &Font16, WHITE, BLACK);

  char serial[19] = "SCD4x:";
  char hex[5];
  sprintf(hex, "%04X", serial0);
  strcat(serial, hex);
  sprintf(hex, "%04X", serial1);
  strcat(serial, hex);
  sprintf(hex, "%04X", serial2);
  strcat(serial, hex);
  Paint_DrawString_EN(0, 33, serial, &Font16, WHITE, BLACK);

  char average_temp[40];
  sprintf(average_temp, "%.1f", sumTemp/10);
  strcat(average_temp, "*C");
  Paint_DrawString_EN(1, 49, "ESP32:", &Font16, WHITE, BLACK);
  Paint_DrawString_EN(67, 49, average_temp, &Font16, WHITE, BLACK);

  int uptime_m = esp_timer_get_time() / 1000000 / 60;
  int uptime_h = uptime_m / 60;
  int uptime_d = uptime_h / 24;
  uptime_h %= 24;
  uptime_m %= 60;
  char uptime[40];
  sprintf(uptime, "%dD %d:%02d", uptime_d, uptime_h , uptime_m);
  Paint_DrawString_EN(1, 65, "Up-time:", &Font16, WHITE, BLACK);
  Paint_DrawString_EN(89, 65, uptime, &Font16, WHITE, BLACK);

  Paint_DrawString_EN(1, 81, "Version:", &Font16, WHITE, BLACK);
  Paint_DrawString_EN(89, 81, VERSION, &Font16, WHITE, BLACK);

  Paint_DrawString_EN(1, 97, "Battery Voltage:", &Font16, WHITE, BLACK);
  char batteryvolt[12];
  sprintf(batteryvolt, "%.2f", readBatteryVoltage());
  strcat(batteryvolt, "V max: ");
  Paint_DrawString_EN(1, 113, batteryvolt, &Font16, WHITE, BLACK);
  sprintf(batteryvolt, "%.2f", maxBatteryVoltage);
  strcat(batteryvolt, "V");
  Paint_DrawString_EN(122, 113, batteryvolt, &Font16, WHITE, BLACK);

  if (WiFi.status() == WL_CONNECTED) {
    char signalStrength[19] = "SNR:";
    char buffer[5];
    sprintf(buffer, "%d", WiFi.RSSI());
    strcat(signalStrength, buffer);
    strcat(signalStrength, "dBm");
    Paint_DrawString_EN(1, 168, signalStrength, &Font16, WHITE, BLACK);

    String ip = "IP:" + WiFi.localIP().toString();
    Paint_DrawString_EN(1, 184, ip.c_str(), &Font16, WHITE, BLACK);
  }

  Paint_DrawString_EN(1, 129, "USB_ACTIVE:", &Font16, WHITE, BLACK);
  Paint_DrawString_EN(122, 129, USB_ACTIVE? "yes":"no", &Font16, WHITE, BLACK);

  updateDisplay();
}

void displayWriteError(char errorMessage[256]){
  Paint_Clear(WHITE);
  Paint_DrawString_EN(5, 40, errorMessage, &Font20, WHITE, BLACK);
}

/* TEST_MODE */
void displayWriteTestResults(float voltage, uint16_t sensorStatus) {
  extern uint16_t serial0, serial1, serial2;
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

  char serial[20] = "Serial:";
  char hex[5];
  sprintf(hex, "%04X", serial0);
  Serial.print(hex);
  strcat(serial, hex);
  sprintf(hex, "%04X", serial1);
  Serial.print(hex);
  strcat(serial, hex);
  sprintf(hex, "%04X", serial2);
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
#endif /* EINK_1IN54V2 */

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

void updateDisplay() {
  if (invertDisplay) {
#ifdef EINK_1IN54V2
    for (int x = 0; x < 200; x++) {
      for (int y = 0; y < 200/8; y++) {
        BlackImage[y+x*25] = ~BlackImage[y+x*25];
      }
    }
#endif /* EINK_1IN54V2 */
#ifdef EINK_4IN2
    for (int j = 0; j < 300; j++) {
      for (int i = 0; i < 400/8; i++) {
        BlackImage[j * 50 + i] = ~BlackImage[j * 50 + i];
      }
    }
#endif /* EINK_4IN2 */
  }

#ifdef EINK_1IN54V2
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
#endif /* EINK_1IN54V2 */

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
