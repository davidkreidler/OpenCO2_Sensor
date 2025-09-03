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
  FUN,
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
  MAX_BATTERY,
  INVERT,
  TEMP_UNIT,
  LANGUAGE,
  FONT,
  EXIT_DISPLAY,
  NUM_DISPLAY_OPTIONS
};
enum FUNMenuOptions {
  RAINBOW,
  SNAKE,
  //TICTACTOE,
  EXIT_FUN,
  NUM_FUN_OPTIONS
};

/* ENGLISH */
const char* EnglishMenuItems[NUM_OPTIONS] = {
  "LED",
  "Display",
  "Calibrate",
  "History",
  "Wi-Fi",
  "Info",
  "Fun"
};
const char* EnglishLEDmenuItems[NUM_LED_OPTIONS] = {
  "Battery",
  "on USB",
  "Color",
  "Bright",
  "Exit"
};
const char* EnglishOptionsMenuItems[NUM_DISPLAY_OPTIONS] = {
  "Battery",
  "Invert",
  "Unit",
  "English",
  "Font",
  "Exit"
};
const char* EnglishFUNmenuItems[NUM_FUN_OPTIONS] = {
  "Rainbow",
  "Snake",
  //"TicTacToe",
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
  "Spass"
};
const char* GermanLEDmenuItems[NUM_LED_OPTIONS] = {
  "Batterie",
  "mit USB",
  "Farbe",
  "Hell",
  "Beenden"
};
const char* GermanOptionsMenuItems[NUM_DISPLAY_OPTIONS] = {
  "Battery",
  "Invert",
  "Einheit",
  "German",
  "Schrift",
  "Beenden"
};
const char* GermanFUNmenuItems[NUM_FUN_OPTIONS] = {
  "Regenbogen",
  "Snake",
  //"TicTacToe",
  "Beenden"
};

typedef struct {
    uint8_t humidity : 7;      // 7 bits (range 0 to 100)
    uint16_t temperature : 9;  // 9 bits (/10= range 0 to 51.1°C )
} tempHumData;

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
void displayCalibrationWarning();
void DisplayCalibrationFail();
void DisplayCalibrationResult(int correction);
void displayWiFi(bool useWiFi);
void displayWiFiStrengh();

#endif /* EPD_ABSTRACTION_H */
