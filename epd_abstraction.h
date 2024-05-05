#ifndef EPD_ABSTRACTION_H
#define EPD_ABSTRACTION_H

#include <stdlib.h>

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
  FONT,
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
  "Font",
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
  "Schrift",
  "Beenden"
};

void handleButtonPress();
void changeFont(int font);
void displayWelcome();
void initEpdOnce();
void displayInitTestMode();
void displayInit();
void displayLowBattery();
void displayWriteError(char errorMessage[256]);
void displayWriteMeasuerments(uint16_t co2, uint16_t temperature, uint16_t humidity);
void displayWriteTestResults(float voltage, uint16_t sensorStatus);
void displayBattery(uint8_t percentage);
void updateDisplay();
void displayImage(const unsigned char* image_buffer);
void displayHistory(uint16_t measurements[24][120]);
void displayNoHistory();
void displayCalibrationWarning();
void displayWiFi(bool useWiFi);
void displayWiFiStrengh();

#endif /* EPD_ABSTRACTION_H */
