/*
 * webserver.c
 * Implementazione Web Server
 */

#include "webserver.h"
#include <string.h>
#include <stdio.h>

// Pagina HTML per l'interfaccia web
const char HTML_PAGE[] =
"<!DOCTYPE html>"
"<html>"
"<head>"
"    <title>Nucleo F401RE LED Control</title>"
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
"    <style>"
"        body { font-family: Arial, sans-serif; text-align: center; margin: 50px; background-color: #f0f0f0; }"
"        .container { background-color: white; padding: 30px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); max-width: 400px; margin: 0 auto; }"
"        h1 { color: #333; margin-bottom: 30px; }"
"        .led-status { font-size: 24px; margin: 20px 0; padding: 15px; border-radius: 5px; }"
"        .led-on { background-color: #4CAF50; color: white; }"
"        .led-off { background-color: #f44336; color: white; }"
"        .button { background-color: #008CBA; color: white; padding: 15px 32px; text-decoration: none; display: inline-block; font-size: 16px; margin: 10px; cursor: pointer; border: none; border-radius: 5px; transition: background-color 0.3s; }"
"        .button:hover { background-color: #005f7a; }"
"        .button-on { background-color: #4CAF50; }"
"        .button-on:hover { background-color: #45a049; }"
"        .button-off { background-color: #f44336; }"
"        .button-off:hover { background-color: #da190b; }"
"    </style>"
"</head>"
"<body>"
"    <div class=\"container\">"
"        <h1>Nucleo F401RE<br>LED Control</h1>"
"        <div class=\"led-status %s\">LED: %s</div>"
"        <a href=\"/led/on\" class=\"button button-on\">Accendi LED</a>"
"        <a href=\"/led/off\" class=\"button button-off\">Spegni LED</a>"
"        <br><br>"
"        <a href=\"/\" class=\"button\">Aggiorna</a>"
"    </div>"
"</body>"
"</html>";

/**
 * @brief Inizializza il web server
 */
void WebServer_Init(WebServer_t *server, ESP8266_t *esp, GPIO_TypeDef* led_port, uint16_t led_pin) {
    server->esp8266 = esp;
    server->led_state = LED_OFF;
    server->led_port = led_port;
    server->led_pin = led_pin;

    // Inizializza il LED spento
    WebServer_SetLED(server, LED_OFF);
}

/**
 * @brief Processa le richieste del web server
 */
void WebServer_Process(WebServer_t *server) {
    uint8_t connection_id;

    // Controlla se ci sono nuove connessioni o dati
    if (ESP8266_CheckForConnection(server->esp8266, &connection_id) == ESP8266_OK) {
        // Estrai la richiesta HTTP dal buffer
        char *request_start = strstr(server->esp8266->rx_buffer, "GET ");
        if (request_start != NULL) {
            WebServer_HandleRequest(server, connection_id, request_start);
        }

        // Chiudi la connessione dopo aver inviato la risposta
        HAL_Delay(100);
        ESP8266_CloseConnection(server->esp8266, connection_id);

        // Pulisci il buffer per la prossima richiesta
        ESP8266_ClearBuffer(server->esp8266);
    }
}

/**
 * @brief Gestisce una richiesta HTTP specifica
 */
void WebServer_HandleRequest(WebServer_t *server, uint8_t connection_id, const char *request) {
    // Analizza la richiesta
    if (strstr(request, "GET /led/on") != NULL) {
        // Richiesta per accendere il LED
        WebServer_SetLED(server, LED_ON);
        WebServer_SendHTMLPage(server, connection_id);
    }
    else if (strstr(request, "GET /led/off") != NULL) {
        // Richiesta per spegnere il LED
        WebServer_SetLED(server, LED_OFF);
        WebServer_SendHTMLPage(server, connection_id);
    }
    else if (strstr(request, "GET /") != NULL) {
        // Richiesta per la pagina principale
        WebServer_SendHTMLPage(server, connection_id);
    }
    else {
        // Richiesta non riconosciuta - invia 404
        const char* not_found =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n\r\n"
            "<html><body><h1>404 - Page Not Found</h1></body></html>";

        ESP8266_SendData(server->esp8266, connection_id, not_found, strlen(not_found));
    }
}

/**
 * @brief Invia la pagina HTML principale
 */
void WebServer_SendHTMLPage(WebServer_t *server, uint8_t connection_id) {
    static char http_response[HTTP_BUFFER_SIZE];
    static char html_content[HTTP_BUFFER_SIZE - 200]; // Lascia spazio per gli header HTTP

    // Genera il contenuto HTML con lo stato corrente del LED
    const char* led_class = (server->led_state == LED_ON) ? "led-on" : "led-off";
    const char* led_status = (server->led_state == LED_ON) ? "ACCESO" : "SPENTO";

    snprintf(html_content, sizeof(html_content), HTML_PAGE, led_class, led_status);

    // Crea la risposta HTTP completa
    snprintf(http_response, sizeof(http_response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "Cache-Control: no-cache\r\n\r\n%s",
        (int)strlen(html_content), html_content);

    // Invia la risposta
    ESP8266_SendData(server->esp8266, connection_id, http_response, strlen(http_response));
}

/**
 * @brief Invia una risposta HTTP generica
 */
void WebServer_SendHTTPResponse(WebServer_t *server, uint8_t connection_id, const char *content, uint16_t content_length) {
    char http_header[200];

    snprintf(http_header, sizeof(http_header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n",
        content_length);

    // Invia prima gli header
    ESP8266_SendData(server->esp8266, connection_id, http_header, strlen(http_header));

    // Poi invia il contenuto
    ESP8266_SendData(server->esp8266, connection_id, content, content_length);
}

/**
 * @brief Controlla il LED
 */
void WebServer_SetLED(WebServer_t *server, LED_State_t state) {
    server->led_state = state;

    if (state == LED_ON) {
        HAL_GPIO_WritePin(server->led_port, server->led_pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(server->led_port, server->led_pin, GPIO_PIN_RESET);
    }
}
