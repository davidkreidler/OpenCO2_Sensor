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

/* WEB SETTINGS PAGE (/settings) - texts in both languages, HTML entities for umlauts */
enum WebTexts {
  WT_TITLE,
  WT_SAVED,
  WT_INVALID,
  WT_CURRENT_VALUE,
  WT_USER_CORRECTION,
  WT_SW_WARNING,
  WT_CORR_LABEL,
  WT_CORR_HINT,
  WT_SAVE,
  WT_JUMP,
  WT_HUMIDITY,
  WT_BATTERY_NOTE,
  WT_DEVICE_SETTINGS,
  WT_TEMP_UNIT,
  WT_LANGUAGE,
  WT_LANGUAGE_VALUE,
  WT_INVERT,
  WT_ON,
  WT_OFF,
  WT_TOGGLE,
  WT_BACK,
  WT_CALIBRATE,
  WT_CAL_HINT,
  WT_CAL_CONFIRM,
  WT_CAL_RUNNING,
  WT_CAL_REMAINING,
  WT_CAL_RESULT,
  WT_CAL_DONE,
  WT_CAL_FAIL,
  WT_CAL_ABORT,
  WT_MAIN_TEMPERATURE,
  WT_MAIN_HUMIDITY,
  WT_MAIN_HISTORY,
  WT_MAIN_VERSION,
  WT_TILE_TOOLTIP,
  NUM_WEB_TEXTS
};
const char* EnglishWebTexts[NUM_WEB_TEXTS] = {
  "Temperature correction",
  "Saved.",
  "Invalid value. Allowed:",
  "Current value displayed",
  "User correction",
  "Part of the correction is applied in software. Humidity is not compensated for this part.",
  "Correction of the displayed temperature in",
  "Negative lowers the displayed value (e.g. heat source nearby).<br>Positive raises the displayed value (e.g. cold window).",
  "Save",
  "When applying the new correction value with the save button, the temperature jumps up ~{N} degrees and after about 180 sec the value is stable.",
  "The relative humidity is corrected by the sensor together with the temperature offset set in the sensor.",
  "While in battery mode, applying a correction with positive temperature offset does not affect the sensor&#39;s humidity values unlike it would do in modes with USB-C plugged in.",
  "Device settings",
  "Temperature unit",
  "Language",
  "English",
  "Invert display",
  "On",
  "Off",
  "Toggle",
  "Back",
  "Calibrate CO2",
  "Place the sensor outdoors, powered by a power bank, go back inside and start here.<br>The calibration takes about 3 minutes and sets the current value to 420 ppm. This page waits until it is finished.",
  "Is the sensor outdoors?\\nThe current CO2 value will be set to 420 ppm.",  // \\n = line break in the JavaScript confirm dialog
  "Calibration running. Please wait about 3 minutes and do not close this page.",
  "seconds remaining",
  "Calibration done. Correction:",
  "You can bring the sensor back in. The device restarts now.",
  "Calibration failed. The device restarts now.",
  "Calibration aborted (button pressed on the device). The device restarts now.",
  "Temperature",
  "Humidity",
  "History",
  "Installed Version:",
  "Settings, CO2 Calibration and Temperature correction"
};
const char* GermanWebTexts[NUM_WEB_TEXTS] = {
  "Temperaturkorrektur",
  "Gespeichert.",
  "Ung&uuml;ltiger Wert. Erlaubt:",
  "Aktuell angezeigter Wert",
  "Korrekturwert",
  "Ein Teil der Korrektur wird per Software angewendet. Die Luftfeuchte wird f&uuml;r diesen Teil nicht korrigiert.",
  "Korrektur der angezeigten Temperatur in",
  "Negativ senkt den angezeigten Wert (z.&nbsp;B. W&auml;rmequelle in der N&auml;he).<br>Positiv hebt den angezeigten Wert an (z.&nbsp;B. kaltes Fenster).",
  "Speichern",
  "Beim Speichern eines neuen Korrekturwerts springt die Temperatur um ca. {N} Grad nach oben; nach ca. 180&nbsp;s ist der Wert stabil.",
  "Die relative Luftfeuchte wird vom Sensor zusammen mit dem im Sensor gesetzten Temperatur-Offset korrigiert.",
  "Im Akkubetrieb wirkt sich eine Korrektur mit positivem Temperatur-Offset nicht auf die Luftfeuchtewerte des Sensors aus, anders als bei eingestecktem USB-C.",
  "Ger&auml;teeinstellungen",
  "Temperatur-Einheit",
  "Sprache",
  "Deutsch",
  "Display invertieren",
  "An",
  "Aus",
  "Umschalten",
  "Zur&uuml;ck",
  "CO2 kalibrieren",
  "Sensor ins Freie stellen, mit Powerbank versorgt, wieder hineingehen und hier starten.<br>Die Kalibrierung dauert ca. 3 Minuten und setzt den aktuellen Wert auf 420 ppm. Diese Seite wartet, bis sie fertig ist.",
  "Steht der Sensor im Freien?\\nDer aktuelle CO2-Wert wird auf 420 ppm gesetzt.",
  "Kalibrierung l&auml;uft. Bitte ca. 3 Minuten warten und diese Seite nicht schlie&szlig;en.",
  "Sekunden verbleibend",
  "Kalibrierung abgeschlossen. Korrektur:",
  "Der Sensor kann wieder hereingeholt werden. Das Ger&auml;t startet jetzt neu.",
  "Kalibrierung fehlgeschlagen. Das Ger&auml;t startet jetzt neu.",
  "Kalibrierung abgebrochen (Taste am Ger&auml;t gedr&uuml;ckt). Das Ger&auml;t startet jetzt neu.",
  "Temperatur",
  "Luftfeuchte",
  "Historie",
  "Installierte Version:",
  "Einstellungen, CO2-Kalibrierung und Temperaturkorrektur"
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
void toggleInvertDisplay();
void toggleTempUnit();
void toggleLanguage();
const char* webText(int id);
int runCalibration(int &correction, void (*tick)(int));  // defined in OpenCO2_Sensor.ino

#endif /* EPD_ABSTRACTION_H */
