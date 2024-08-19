/*
   ESP32S2 Dev Module
   Required Arduino libraries:
   - esp32 waveshare epd
   - Adafruit DotStar
*/

/* Includes display */
#include "epd_abstraction.h"
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include <stdlib.h>
#include "pictures.h"
#define EINK_1IN54V2
//#define EINK_4IN2

/* welcome */
#include <EEPROM.h>

RTC_DATA_ATTR uint16_t refreshes = 1;
RTC_DATA_ATTR UBYTE *BlackImage;
extern bool BatteryMode, comingFromDeepSleep;
extern uint8_t HWSubRev;

extern uint8_t font;
sFONT fonts[3][3] = {
  bahn_big, bahn_mid, bahn_sml,
  gotham_big, gotham_mid, gotham_sml,
  nothing_big, nothing_mid, bahn_sml
};
sFONT big=fonts[font][0];
sFONT mid=fonts[font][1];
sFONT sml=fonts[font][2];

void changeFont(int font) {
  big=fonts[font][0];
  mid=fonts[font][1];
  sml=fonts[font][2];
}

void clearMenu() {
    //refreshes = 1;
    displayWriteMeasuerments(co2, temperature, humidity);
    if (BatteryMode) displayBattery(calcBatteryPercentage(readBatteryVoltage()));
    else if (useWiFi) displayWiFiStrengh(); 
    updateDisplay();
}

bool buttonPressedAgain = false;
void handleButtonPress() {
  uint8_t selectedOption = 0;
  refreshes = 1; // force full update
  comingFromDeepSleep = false; // force display update even if CO2 changed by less than 3%
  displayMenu(selectedOption);

  uint16_t mspressed;
  unsigned long menuStartTime = millis();

  for (;;) { 
    if ((millis() - menuStartTime) > 20000) { // display Menu up to 20 sec
      clearMenu();
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
            break;
          case DISPLAY_MENU:
            OptionsMenu();
            break;
          case CALIBRATE:
            calibrate();
            break;
          case HISTORY:
            history();
            break;
          case WLAN:
            toggleWiFi();
            break;
          case INFO:
            displayinfo();
            while (digitalRead(BUTTON) != 0) delay(100); // wait for button press
            break;
          case RAINBOW:
            rainbowMode();
            setLED(co2);
            refreshes = 1;
            break;
        }
        clearMenu();
        return;
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
          case FONT:
            font += 1;
            font %= 3;
            changeFont(font);
            preferences.begin("co2-sensor", false);
            preferences.putInt("font", font);
            preferences.end();
            break;
          case EXIT_DISPLAY:
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

void displayWelcome() {
  EEPROM.begin(2); // EEPROM_SIZE
  EEPROM.write(0, 1);
  EEPROM.commit();

  initEpdOnce();
#ifdef EINK_1IN54V2
  Paint_DrawBitMap(gImage_welcome);
  Paint_DrawString_EN(1, 1, VERSION, &Font16, WHITE, BLACK);
  EPD_1IN54_V2_Display(BlackImage);
  EPD_1IN54_V2_Sleep();
#endif
/*#ifdef EINK_4IN2
  Paint_DrawBitMap(gImage_welcome);
  EPD_4IN2_Display(BlackImage);
  EPD_4IN2_Sleep();
#endif*/

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
  if (english) {
    Paint_DrawString_EN(45, 120, "Charge", &Font20, BLACK, WHITE);
    Paint_DrawString_EN(45, 145, "Battery", &Font20, BLACK, WHITE);
  } else {
    Paint_DrawString_EN(45, 120, "Batterie", &Font20, BLACK, WHITE);
    Paint_DrawString_EN(45, 145, "aufladen", &Font20, BLACK, WHITE);
  }

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
    /*Paint_DrawString_EN(100, 150, "CO", &Font24, WHITE, BLACK);
    Paint_DrawNum(131, 160, 2, &Font20, BLACK, WHITE);*/
    Paint_DrawString_EN(144, 150, "ppm", &Font24, WHITE, BLACK);

    /* temperature */
    if (useFahrenheit) temperature = (temperature*1.8f)+32.0f; // convert to °F
    if (temperature < 10.0f) Paint_DrawNum(30, 5, temperature, &mid, BLACK, WHITE);
    else                     Paint_DrawNum( 1, 5, temperature, &mid, BLACK, WHITE);
    int offset = 0;
    if (temperature >= 100) offset = 29; // for Fahrenheit > 37,7 °C

    Paint_DrawString_EN(60+offset, 4, useFahrenheit? "*F" : "*C", &bahn_sml, WHITE, BLACK);
    Paint_DrawString_EN(60+offset, 32, ",", &sml, WHITE, BLACK);
    char decimal[4];
    sprintf(decimal, "%d", ((int)(temperature * 10)) % 10);
    Paint_DrawString_EN(71+offset, 27, decimal, &sml, WHITE, BLACK);

    /* humidity */
    Paint_DrawNum(124, 5, humidity, &mid, BLACK, WHITE);
    Paint_DrawString_EN(184, 5, "%", &bahn_sml, WHITE, BLACK);
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
    Paint_DrawString_EN(340, 220, "%", &bahn_sml, WHITE, BLACK);
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

void calculateTempHumStats(int* mintemp, int* maxtemp, int* avgtemp, int* minhum, int* maxhum, int* avghum) {
  int sumtemp = 0, sumhum = 0;
  int value = getTempMeasurement(0);
  *mintemp = value;
  *maxtemp = value;

  value = getHumMeasurement(0);
  *minhum = value;
  *maxhum = value;

  uint16_t index;
  if (overflow) index = NUM_MEASUREMENTS / 3;
  else          index = ceil(currentIndex / 3.0);
  for (int i=0; i<=index; i++) {
    value = getTempMeasurement(i);
    if (value < *mintemp) *mintemp = value;
    if (value > *maxtemp) *maxtemp = value;
    sumtemp += value;

    value = getHumMeasurement(i);
    if (value < *minhum) *minhum = value;
    if (value > *maxhum) *maxhum = value;
    sumhum += value;
  }

  *avgtemp = sumtemp / index;
  *avghum  = sumhum  / index;
}

void calculateStatsCO2(int* min, int* max, int* avg) {
  int sum = 0;
  int value = getCO2Measurement(0);
  *min = value;
  *max = value;

  uint16_t index;
  if (overflow) index = NUM_MEASUREMENTS;
  else          index = currentIndex;
  for (int i=0; i<=index; i++) {
    value = getCO2Measurement(i);
    if (value < *min) *min = value;
    if (value > *max) *max = value;
    sum += value;
  }

  *avg = sum / index;
}

void displayCO2HistoryGraph() {
  int min, max, avg;
  calculateStatsCO2(&min, &max, &avg);
  float yscale = 184.0 / (max - min);
  uint16_t index;
  if (overflow) index = NUM_MEASUREMENTS;
  else          index = currentIndex;
  float stepsPerPixel = index / 200.0;

  Paint_Clear(WHITE);
  Paint_DrawNum(0, 0, min, &Font16, BLACK, WHITE);
  Paint_DrawString_EN((snprintf(0,0,"%+d",min)-1)*11, 0, "ppm", &Font16, WHITE, BLACK);
  Paint_DrawNum(200-11*4, 0, max, &Font16, BLACK, WHITE);
  Paint_DrawString_EN(100-11*2, 0, "O", &Font16, WHITE, BLACK);
  Paint_DrawString_EN(100-11*2, 0, "/", &Font16, WHITE, BLACK);
  Paint_DrawNum(100-11, 0, avg, &Font16, BLACK, WHITE);

  char duration[20];
  sprintf(duration, "%.1f", index/120.0);
  strcat(duration, "h");
  Paint_DrawString_EN(0, 200-16, "-", &Font16, WHITE, BLACK);
  Paint_DrawString_EN(11, 200-16, duration, &Font16, WHITE, BLACK);
  Paint_DrawString_EN(200-11*3, 200-16, "now", &Font16, WHITE, BLACK);

  int privY = getCO2Measurement(0);
  for (int x=1; x<200; x++) {
    int y = getCO2Measurement((int)(x * stepsPerPixel));
    y = 200.0 - ((y - min) * yscale);
    Paint_DrawLine(x-1, privY, x, y, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    privY = y;
    for (int i = 200; i>y; i--) {
      if (!(i%5)) Paint_DrawPoint(x, i, BLACK, DOT_PIXEL_1X1, DOT_FILL_AROUND);
    }
  }
  updateDisplay();
}

void displayTempHumHistoryGraph() {
  int mintemp, maxtemp, avgtemp, minhum, maxhum, avghum;
  calculateTempHumStats(&mintemp, &maxtemp, &avgtemp, &minhum, &maxhum, &avghum);

  float hight = 200.0 - 2*16.0;
  float yscaletemp = hight / (maxtemp - mintemp);
  float yscalehum = hight / (maxhum - minhum);
  uint16_t index;
  if (overflow) index = NUM_MEASUREMENTS / 3;
  else          index = ceil(currentIndex / 3.0);
  float stepsPerPixel = index / 200.0;

  Paint_Clear(WHITE);
  char temp[20];
  sprintf(temp, "%.1f", mintemp/10.0);
  strcat(temp, "C");
  Paint_DrawString_EN(0, 0, temp, &Font16, WHITE, BLACK);
  sprintf(temp, "%.1f", avgtemp/10.0);
  Paint_DrawString_EN(100-11, 0, temp, &Font16, WHITE, BLACK);
  sprintf(temp, "%.1f", maxtemp/10.0);
  Paint_DrawString_EN(200-11*4, 0, temp, &Font16, WHITE, BLACK);

  Paint_DrawString_EN(100-11-17, 4, "O", &Font24, WHITE, BLACK);
  Paint_DrawString_EN(100-11-17, 4, "/", &Font24, WHITE, BLACK);

  Paint_DrawNum(0, 16, minhum, &Font16, BLACK, WHITE);
  Paint_DrawString_EN(11*2, 16, "%", &Font16, WHITE, BLACK);
  Paint_DrawNum(100-11, 16, avghum, &Font16, BLACK, WHITE);
  Paint_DrawString_EN(100+11, 16, "%", &Font16, WHITE, BLACK);
  Paint_DrawNum(200-11*3, 16, maxhum, &Font16, BLACK, WHITE);
  Paint_DrawString_EN(200-11, 16, "%", &Font16, WHITE, BLACK);

  char duration[20];
  sprintf(duration, "%.1f", index/40.0);
  strcat(duration, "h");
  Paint_DrawString_EN(0, 200-16, "-", &Font16, WHITE, BLACK);
  Paint_DrawString_EN(11, 200-16, duration, &Font16, WHITE, BLACK);
  Paint_DrawString_EN(200-11*3, 200-16, "now", &Font16, WHITE, BLACK);

  int privYtemp = getTempMeasurement(0);
  int privYhum = getHumMeasurement(0);

  for (int x=1; x<200; x++) {
    int ytemp = getTempMeasurement((int)(x * stepsPerPixel));
    int yhum =  getHumMeasurement((int)(x * stepsPerPixel));
    ytemp = 200.0 - ((ytemp - mintemp) * yscaletemp);
    yhum = 200.0 - ((yhum - minhum) * yscalehum);
    Paint_DrawLine(x-1, privYtemp, x, ytemp, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    for (int i = 200; i>ytemp; i--) {
      if (!(i%5)) Paint_DrawPoint(x, i, BLACK, DOT_PIXEL_2X2, DOT_FILL_AROUND);
    }
    Paint_DrawLine(x-1, privYhum, x, yhum, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    privYtemp = ytemp;
    privYhum = yhum;
  }
  updateDisplay();
}

uint8_t hour;
void displayHistory(uint8_t qrcodeNumber) {
  if (qrcodeNumber == hour+2) {
    displayCO2HistoryGraph();
    return;
  }
  if (qrcodeNumber == hour+1) {
    displayTempHumHistoryGraph();
    return;
  }

  char buffer[5*120+1];
  int numEnties = currentIndex % 120;
  if (overflow || hour != qrcodeNumber) numEnties = 120; // display all values included in previous hours
  for (int i=0; i<numEnties; i++) {
    char tempStr[6];
    snprintf(tempStr, sizeof(tempStr), "%d", getCO2Measurement(qrcodeNumber*120+i));

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
  if (english) Paint_DrawString_EN(1, 1, "long press = exit", &Font16, WHITE, BLACK);
  else         Paint_DrawString_EN(1, 1, " halten = beenden", &Font16, WHITE, BLACK);
  updateDisplay();
}

void history() {
  // DEMO DATA:
  /*for (int i=0; i<NUM_MEASUREMENTS; i++) {
    saveMeasurement(i, 20.0+i/100.0, 100.0-i/50.0);
  }*/

  uint16_t mspressed;
  unsigned long historyStartTime = millis();
  if (overflow) hour = 23;
  else          hour = currentIndex/120;
  uint8_t qrcodeNumber = hour+2; // start at CO2 graph
  refreshes = 1; // force full update
  displayHistory(qrcodeNumber);

  for (;;) { 
    if ((millis() - historyStartTime) > 30000) { // display History up to 30 sec
      clearMenu();
      return;
    }
    mspressed = 0;
    if (digitalRead(BUTTON) == 0) {
      while(digitalRead(BUTTON) == 0) { // calculate how long BUTTON is pressed
        delay(100);
        mspressed += 100;
        if (mspressed == 1000) break;
      }
      if (mspressed == 1000) { // long press
        clearMenu();
        return;
      } else { // goto next History point
        if (qrcodeNumber == 0) qrcodeNumber = hour+2;
        else qrcodeNumber--;
        refreshes = 1; // force full update
        displayHistory(qrcodeNumber);
        historyStartTime = millis(); // display History again for 30 sec
      }
    }
  }
}

void displayMenu(uint8_t selectedOption) {
  const char* menuItem;
  Paint_Clear(WHITE);
  Paint_DrawString_EN(66, 0, "Menu", &Font24, WHITE, BLACK);
  Paint_DrawLine(10, 23, 190, 23, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

  for (int i=0; i<NUM_OPTIONS; i++) {
    if (english) menuItem = EnglishMenuItems[i];
    else         menuItem = GermanMenuItems[i];
    Paint_DrawString_EN(5, 25*(i+1), menuItem, &Font24, WHITE, BLACK);
  }
  invertSelected(selectedOption);
  updateDisplay();
}

void displayLEDMenu(uint8_t selectedOption) {
  const char* LEDmenuItem;
  Paint_Clear(WHITE);
  Paint_DrawString_EN(75, 0, "LED", &Font24, WHITE, BLACK);
  Paint_DrawLine(10, 23, 190, 23, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

  for (int i=0; i<NUM_LED_OPTIONS; i++) {
    if (english) LEDmenuItem = EnglishLEDmenuItems[i];
    else         LEDmenuItem = GermanLEDmenuItems[i];
    Paint_DrawString_EN(5, 25*(i+1), LEDmenuItem, &Font24, WHITE, BLACK);
  }

  if (english) {
    Paint_DrawString_EN(149, 25, (LEDonBattery? "ON":"OFF"), &Font24, WHITE, BLACK);
    Paint_DrawString_EN(149, 50, (LEDonUSB? "ON":"OFF"), &Font24, WHITE, BLACK);
  } else {
    Paint_DrawString_EN(149, 25, (LEDonBattery? "AN":"AUS"), &Font24, WHITE, BLACK);
    Paint_DrawString_EN(149, 50, (LEDonUSB? "AN":"AUS"), &Font24, WHITE, BLACK);
  }

  Paint_DrawString_EN(149, 75, (useSmoothLEDcolor? "1":"2"), &Font24, WHITE, BLACK);
  Paint_DrawString_EN(166, 75, "/2", &Font24, WHITE, BLACK);
  Paint_DrawNum(149, 100, (int32_t)(ledbrightness/20+1), &Font24, BLACK, WHITE); // 5 25 45 65 85
  Paint_DrawString_EN(166, 100, "/5", &Font24, WHITE, BLACK);

  invertSelected(selectedOption);
  updateDisplay();
}

void displayOptionsMenu(uint8_t selectedOption) {
  const char* OptionsMenuItem;
  Paint_Clear(WHITE);
  Paint_DrawString_EN(40, 0, "DISPLAY", &Font24, WHITE, BLACK);
  Paint_DrawLine(10, 23, 190, 23, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

  for (int i=0; i<NUM_DISPLAY_OPTIONS; i++) {
    if (english) OptionsMenuItem = EnglishOptionsMenuItems[i];
    else         OptionsMenuItem = GermanOptionsMenuItems[i];
    Paint_DrawString_EN(5, 25*(i+1), OptionsMenuItem, &Font24, WHITE, BLACK);
  }
  Paint_DrawString_EN(166, 50, (useFahrenheit? "*F":"*C"), &Font24, WHITE, BLACK);
  Paint_DrawNum(149, 100, (int32_t)(font+1), &Font24, BLACK, WHITE);
  Paint_DrawString_EN(166, 100, "/3", &Font24, WHITE, BLACK);

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

void displayFlappyBird() {
  int birdpos = 100;
#define numObsticals 5
  int obsticals[numObsticals] = {100,90,110,110,100};
  int frame = 0;
  int xpos;
  
  if (comingFromDeepSleep && HWSubRev > 1) {
    Paint_Clear(WHITE);
    EPD_1IN54_V2_Init_Partial_After_Powerdown();
    EPD_1IN54_V2_writePrevImage(BlackImage);
  } else {
    EPD_1IN54_V2_Init_Partial();
  }

  while (birdpos > 0 && birdpos < 200) {
    Paint_Clear(WHITE);

    if (digitalRead(BUTTON) == 0) birdpos -= 10;
    else                          birdpos += 10;
    Paint_DrawString_EN(20, birdpos, "X", &Font24, WHITE, BLACK);

    for (int i = 0; i < numObsticals; i++) {
      xpos =  (i+1)*40 - frame*3;
               // Xstart,Ystart,Xend,Yend
      Paint_DrawLine(xpos, 0, xpos, obsticals[i], BLACK, DOT_PIXEL_4X4, LINE_STYLE_SOLID); //top
      Paint_DrawLine(xpos, 200, xpos, obsticals[i]+50, BLACK, DOT_PIXEL_4X4, LINE_STYLE_SOLID); //buttum
    }
    frame++;
    frame %= 200;

    Paint_DrawNum(200-7*3, 200-12, frame, &Font12, BLACK, WHITE);
    EPD_1IN54_V2_DisplayPart(BlackImage);
  }
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

  if (english) {
    Paint_DrawString_EN(16, 132, "Calibration!", &Font20, BLACK, WHITE);
    Paint_DrawString_EN(1, 152, "Put Sensor outside", &Font16, BLACK, WHITE);
    Paint_DrawString_EN(1, 168, "for 3+ minutes. Or", &Font16, BLACK, WHITE);
    Paint_DrawString_EN(1, 184, "hold knob to stop", &Font16, BLACK, WHITE);
  } else {
    Paint_DrawString_EN(16, 132, "Kalibration!", &Font20, BLACK, WHITE);
    Paint_DrawString_EN(1, 152, "Sensor fur 3+ min.", &Font16, BLACK, WHITE);
    Paint_DrawString_EN(1, 168, "nach drausen legen", &Font16, BLACK, WHITE);
    Paint_DrawString_EN(1, 184, "Knopf = abbrechen", &Font16, BLACK, WHITE);
  }

  updateDisplay();
}

void displayWiFi(bool useWiFi) {
  Paint_Clear(BLACK);

  if (useWiFi) {
    updateBatteryMode();
    if (BatteryMode) {
      if (english) {
        Paint_DrawString_EN(23, 52, "Wi-Fi: ON", &Font24, BLACK, WHITE);
        Paint_DrawString_EN(40, 76, "Connect", &Font24, BLACK, WHITE);
        Paint_DrawString_EN(48, 100, "Power!", &Font24, BLACK, WHITE);
      } else {
        Paint_DrawString_EN(32, 52, "WLAN: AN", &Font24, BLACK, WHITE);
        Paint_DrawString_EN(57, 76, "Strom", &Font24, BLACK, WHITE);
        Paint_DrawString_EN(15, 100, "verbinden!", &Font24, BLACK, WHITE);
      }
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
    if (english) Paint_DrawString_EN(15, 132, "Wi-Fi: OFF", &Font24, BLACK, WHITE);
    else         Paint_DrawString_EN(15, 132, "WLAN: AUS", &Font24, BLACK, WHITE);
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
  if      (signalStrength > -55) { Paint_DrawImage(wifiFullIcon,   5, 74, 40, 40); }
  else if (signalStrength > -70) { Paint_DrawImage(wifiMediumIcon, 5, 74, 40, 40); }
  else {                           Paint_DrawImage(wifiLowIcon,    5, 74, 40, 40); }

  /* time */
  struct tm timeinfo;
  getLocalTime(&timeinfo);  
  //char time[100];
  //sprintf(time, "%d-%02d-%02d %02d:%02d:%02d", (timeinfo.tm_year)+1900, timeinfo.tm_mday, (timeinfo.tm_mon)+1, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  char hour[3];
  char minute[3];
  sprintf(hour, "%02d", timeinfo.tm_hour);
  sprintf(minute, "%02d", timeinfo.tm_min);
  Paint_DrawString_EN(1, 150, hour, &mid, WHITE, BLACK);
                // XCenter,YCenter,radius
  Paint_DrawCircle(1+29*2+4, 150+15, 3, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
  Paint_DrawCircle(1+29*2+4, 150+29, 3, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
  Paint_DrawString_EN(1+29*2+9, 150, minute, &mid, WHITE, BLACK);
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

  // only needed for debugging. This interrupts the SCD40
  /*float tOffset;
  scd4x.stopPeriodicMeasurement();
  scd4x.getTemperatureOffset(tOffset);
  if (BatteryMode) scd4x.startLowPowerPeriodicMeasurement();
  else scd4x.startPeriodicMeasurement();
  Paint_DrawString_EN(1, 145, "T_offset:", &Font16, WHITE, BLACK);
  char offset[20];
  sprintf(offset, "%.1f", tOffset);
  Paint_DrawString_EN(122, 145, offset, &Font16, WHITE, BLACK);*/

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
  Paint_DrawString_EN(20,149, batterpercent, &bahn_sml, WHITE, BLACK);

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
  Paint_DrawString_EN(300, 12, batterpercent, &bahn_sml, WHITE, BLACK);

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
