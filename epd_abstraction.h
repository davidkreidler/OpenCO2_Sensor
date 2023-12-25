#ifndef EPD_ABSTRACTION_H
#define EPD_ABSTRACTION_H

#include <stdlib.h>

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
void displayMenu(uint8_t selectedOption);
void displayLEDMenu(uint8_t selectedOption);
void displayCalibrationWarning();
void displayWiFi(bool useWiFi);
void displayWiFiStrengh();
void displayinfo();

#endif /* EPD_ABSTRACTION_H */
