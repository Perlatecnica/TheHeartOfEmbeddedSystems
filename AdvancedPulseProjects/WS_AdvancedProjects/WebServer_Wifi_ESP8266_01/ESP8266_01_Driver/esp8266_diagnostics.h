/*
 * esp8266_diagnostics.h
 * Header per funzioni di diagnostica ESP8266-01
 * Versione corretta senza warning
 */

#ifndef ESP8266_DIAGNOSTICS_H
#define ESP8266_DIAGNOSTICS_H

#include "stm32f4xx_hal.h"

// Prototipi delle funzioni pubbliche di diagnostica
void ESP8266_DiagnosticTest(void);
void ESP8266_StepByStepTest(void);
void ESP8266_TestWiFiConnection(const char* ssid, const char* password);
void ESP8266_QuickTest(void);
void ESP8266_FullDiagnostic(const char* ssid, const char* password);

#endif // ESP8266_DIAGNOSTICS_H
