/*
 * webserver.h
 * Web Server per controllo LED
 * Nucleo F401RE + ESP8266-01
 */

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "esp8266_driver.h"
#include "stm32f4xx_hal.h"

// Definizioni per il web server
#define WEBSERVER_PORT 80
#define HTTP_BUFFER_SIZE 2048

// Stati del LED
typedef enum {
    LED_OFF = 0,
    LED_ON = 1
} LED_State_t;

// Struttura del web server
typedef struct {
    ESP8266_t *esp8266;
    LED_State_t led_state;
    GPIO_TypeDef* led_port;
    uint16_t led_pin;
} WebServer_t;

// Prototipi delle funzioni
void WebServer_Init(WebServer_t *server, ESP8266_t *esp, GPIO_TypeDef* led_port, uint16_t led_pin);
void WebServer_Process(WebServer_t *server);
void WebServer_HandleRequest(WebServer_t *server, uint8_t connection_id, const char *request);
void WebServer_SendHTMLPage(WebServer_t *server, uint8_t connection_id);
void WebServer_SendHTTPResponse(WebServer_t *server, uint8_t connection_id, const char *content, uint16_t content_length);
void WebServer_SetLED(WebServer_t *server, LED_State_t state);
const char* WebServer_GetHTMLContent(WebServer_t *server);

#endif // WEBSERVER_H
