/*
 * esp8266_driver.h
 * Driver per modulo ESP8266-01
 * Nucleo F401RE + ESP8266-01 Web Server
 */

#ifndef ESP8266_DRIVER_H
#define ESP8266_DRIVER_H

#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>

// Definizioni
#define ESP8266_DEFAULT_TIMEOUT 5000
#define ESP8266_BUFFER_SIZE 1024
#define ESP8266_MAX_CONNECTIONS 4

// Struttura per gestire l'ESP8266
typedef struct {
    UART_HandleTypeDef *huart;
    char rx_buffer[ESP8266_BUFFER_SIZE];
    char tx_buffer[ESP8266_BUFFER_SIZE];
    uint8_t connected;
    uint8_t server_started;
} ESP8266_t;

// Enumerazioni per i tipi di risposta
typedef enum {
    ESP8266_OK = 0,
    ESP8266_ERROR = 1,
    ESP8266_TIMEOUT_ERR = 2,
    ESP8266_BUSY = 3
} ESP8266_Result_t;

// Prototipi delle funzioni
ESP8266_Result_t ESP8266_Init(ESP8266_t *esp, UART_HandleTypeDef *huart);
ESP8266_Result_t ESP8266_SendCommand(ESP8266_t *esp, const char *command, const char *expected_response, uint32_t timeout);
ESP8266_Result_t ESP8266_ConnectToWiFi(ESP8266_t *esp, const char *ssid, const char *password);
ESP8266_Result_t ESP8266_StartServer(ESP8266_t *esp, uint16_t port);
ESP8266_Result_t ESP8266_SendData(ESP8266_t *esp, uint8_t connection_id, const char *data, uint16_t length);
ESP8266_Result_t ESP8266_CloseConnection(ESP8266_t *esp, uint8_t connection_id);
ESP8266_Result_t ESP8266_CheckForConnection(ESP8266_t *esp, uint8_t *connection_id);
ESP8266_Result_t ESP8266_GetIPAddress(ESP8266_t *esp, char *ip_buffer);
void ESP8266_ClearBuffer(ESP8266_t *esp);
uint8_t ESP8266_WaitForResponse(ESP8266_t *esp, const char *expected, uint32_t timeout);
void ESP8266_FlushRxBuffer(ESP8266_t *esp);

#endif // ESP8266_DRIVER_H
