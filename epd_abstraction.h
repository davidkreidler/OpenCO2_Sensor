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
void displayWriteTestResults(float voltage, uint16_t sensorStatus, uint16_t serial0, uint16_t serial1, uint16_t serial2);
void displayBattery(uint8_t percentage);
void updateDisplay();
void displayRainbow();

#endif /* EPD_ABSTRACTION_H */
