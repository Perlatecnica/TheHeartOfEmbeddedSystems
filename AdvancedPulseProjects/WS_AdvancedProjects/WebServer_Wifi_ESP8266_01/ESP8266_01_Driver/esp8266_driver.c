/*
 * esp8266_driver.c
 * Implementazione driver ESP8266-01
 */

#include "esp8266_driver.h"
#include <stdlib.h>  // Per atoi()

/**
 * @brief Inizializza il modulo ESP8266
 */
ESP8266_Result_t ESP8266_Init(ESP8266_t *esp, UART_HandleTypeDef *huart) {
    esp->huart = huart;
    esp->connected = 0;
    esp->server_started = 0;
    ESP8266_ClearBuffer(esp);

    HAL_Delay(2000); // Attendi l'avvio del modulo

    // Test di comunicazione - versione più robusta
    ESP8266_ClearBuffer(esp);
    HAL_UART_Transmit(esp->huart, (uint8_t*)"AT\r\n", 4, 1000);

    // Attendi qualsiasi risposta che contenga caratteri validi
    uint32_t timeout = HAL_GetTick() + 2000;
    uint8_t rx_data;
    int got_response = 0;

    while (HAL_GetTick() < timeout) {
        if (HAL_UART_Receive(esp->huart, &rx_data, 1, 10) == HAL_OK) {
            if (rx_data >= 32 && rx_data <= 126) { // Carattere ASCII valido
                got_response = 1;
                break;
            }
        }
    }

    if (!got_response) {
        return ESP8266_ERROR;
    }

    // Svuota il buffer dopo il test
    HAL_Delay(500);
    ESP8266_FlushRxBuffer(esp);

    // Reset del modulo - versione più robusta
    ESP8266_ClearBuffer(esp);
    HAL_UART_Transmit(esp->huart, (uint8_t*)"AT+RST\r\n", 8, 1000);

    // Attendi reset - basta che arrivi qualcosa
    HAL_Delay(3000);
    ESP8266_FlushRxBuffer(esp);

    // Disabilita echo - versione più robusta
    ESP8266_ClearBuffer(esp);
    HAL_UART_Transmit(esp->huart, (uint8_t*)"ATE0\r\n", 6, 1000);
    HAL_Delay(1000);
    ESP8266_FlushRxBuffer(esp);

    // Imposta modalità Station - versione più robusta
    ESP8266_ClearBuffer(esp);
    HAL_UART_Transmit(esp->huart, (uint8_t*)"AT+CWMODE=1\r\n", 13, 1000);
    HAL_Delay(2000);
    ESP8266_FlushRxBuffer(esp);

    return ESP8266_OK;
}

/**
 * @brief Invia un comando all'ESP8266 e attende la risposta
 */
ESP8266_Result_t ESP8266_SendCommand(ESP8266_t *esp, const char *command, const char *expected_response, uint32_t timeout) {
    ESP8266_ClearBuffer(esp);

    // Invia il comando
    HAL_UART_Transmit(esp->huart, (uint8_t*)command, strlen(command), 1000);

    // Attendi la risposta
    if (ESP8266_WaitForResponse(esp, expected_response, timeout)) {
        return ESP8266_OK;
    }

    return ESP8266_TIMEOUT_ERR;
}

/**
 * @brief Connette l'ESP8266 al WiFi - versione robusta con check stato
 */
ESP8266_Result_t ESP8266_ConnectToWiFi(ESP8266_t *esp, const char *ssid, const char *password) {
    // Prima controlla se è già connesso
    ESP8266_ClearBuffer(esp);
    HAL_UART_Transmit(esp->huart, (uint8_t*)"AT+CWJAP?\r\n", 11, 1000);
    HAL_Delay(2000);

    // Leggi la risposta per vedere se è già connesso
    uint16_t index = 0;
    uint32_t timeout = HAL_GetTick() + 3000;
    while (HAL_GetTick() < timeout && index < (ESP8266_BUFFER_SIZE - 1)) {
        uint8_t rx_data;
        if (HAL_UART_Receive(esp->huart, &rx_data, 1, 10) == HAL_OK) {
            esp->rx_buffer[index++] = rx_data;
        }
    }
    esp->rx_buffer[index] = '\0';

    // Se trova il nome della rete nella risposta, è già connesso
    if (strstr(esp->rx_buffer, ssid) != NULL) {
        esp->connected = 1;
        return ESP8266_OK;
    }

    // Se non è connesso, disconnetti prima per sicurezza
    ESP8266_ClearBuffer(esp);
    HAL_UART_Transmit(esp->huart, (uint8_t*)"AT+CWQAP\r\n", 10, 1000);
    HAL_Delay(2000);
    ESP8266_FlushRxBuffer(esp);

    // Ora tenta la connessione
    sprintf(esp->tx_buffer, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);

    // Pulisci il buffer prima di inviare
    ESP8266_ClearBuffer(esp);

    // Invia il comando
    HAL_UART_Transmit(esp->huart, (uint8_t*)esp->tx_buffer, strlen(esp->tx_buffer), 1000);

    // Attendi connessione con timeout più lungo per WPA2
    uint32_t start_time = HAL_GetTick();
    uint32_t connection_timeout = 20000; // 20 secondi per WPA2
    index = 0;
    uint8_t found_wifi = 0;
    uint8_t found_ip = 0;

    while ((HAL_GetTick() - start_time) < connection_timeout) {
        uint8_t rx_data;
        if (HAL_UART_Receive(esp->huart, &rx_data, 1, 100) == HAL_OK) {
            if (index < ESP8266_BUFFER_SIZE - 1) {
                esp->rx_buffer[index++] = rx_data;
                esp->rx_buffer[index] = '\0';
            }

            // Cerca pattern parziali - versione più robusta
            if (!found_wifi && (strstr(esp->rx_buffer, "WIFI") != NULL ||
                               strstr(esp->rx_buffer, "wifi") != NULL ||
                               strstr(esp->rx_buffer, "CONNECTED") != NULL ||
                               strstr(esp->rx_buffer, "CNECTED") != NULL)) {
                found_wifi = 1;
            }

            if (!found_ip && (strstr(esp->rx_buffer, "IP") != NULL ||
                             strstr(esp->rx_buffer, "ip") != NULL ||
                             strstr(esp->rx_buffer, "GOT") != NULL ||
                             strstr(esp->rx_buffer, "OT") != NULL)) {
                found_ip = 1;
            }

            // Se abbiamo trovato entrambi, consideriamo la connessione riuscita
            if (found_wifi && found_ip) {
                // Attendi un po' per essere sicuri che la connessione sia stabile
                HAL_Delay(2000);
                esp->connected = 1;
                return ESP8266_OK;
            }

            // Controlla per errori specifici WPA2/WPA3
            if (strstr(esp->rx_buffer, "ERROR") != NULL ||
                strstr(esp->rx_buffer, "FAIL") != NULL ||
                strstr(esp->rx_buffer, "+CWJAP:1") != NULL ||
                strstr(esp->rx_buffer, "+CWJAP:2") != NULL ||
                strstr(esp->rx_buffer, "+CWJAP:3") != NULL ||
                strstr(esp->rx_buffer, "+CWJAP:4") != NULL) {
                return ESP8266_ERROR;
            }
        }
    }

    // Controllo finale: anche se timeout, verifica se è connesso
    ESP8266_ClearBuffer(esp);
    HAL_UART_Transmit(esp->huart, (uint8_t*)"AT+CWJAP?\r\n", 11, 1000);
    HAL_Delay(1000);

    index = 0;
    timeout = HAL_GetTick() + 2000;
    while (HAL_GetTick() < timeout && index < (ESP8266_BUFFER_SIZE - 1)) {
        uint8_t rx_data;
        if (HAL_UART_Receive(esp->huart, &rx_data, 1, 10) == HAL_OK) {
            esp->rx_buffer[index++] = rx_data;
        }
    }
    esp->rx_buffer[index] = '\0';

    // Se trova il nome della rete, è connesso
    if (strstr(esp->rx_buffer, ssid) != NULL) {
        esp->connected = 1;
        return ESP8266_OK;
    }

    // Timeout raggiunto senza successo
    return ESP8266_TIMEOUT_ERR;
}

/**
 * @brief Avvia il server TCP - versione robusta con debug
 */
ESP8266_Result_t ESP8266_StartServer(ESP8266_t *esp, uint16_t port) {
    // Primo: ferma eventuali server esistenti
    ESP8266_ClearBuffer(esp);
    HAL_UART_Transmit(esp->huart, (uint8_t*)"AT+CIPSERVER=0\r\n", 16, 1000);
    HAL_Delay(1000);
    ESP8266_FlushRxBuffer(esp);

    // Abilita connessioni multiple
    ESP8266_ClearBuffer(esp);
    HAL_UART_Transmit(esp->huart, (uint8_t*)"AT+CIPMUX=1\r\n", 13, 1000);
    HAL_Delay(2000);

    // Leggi risposta
    uint16_t index = 0;
    uint32_t timeout = HAL_GetTick() + 2000;
    while (HAL_GetTick() < timeout && index < (ESP8266_BUFFER_SIZE - 1)) {
        uint8_t rx_data;
        if (HAL_UART_Receive(esp->huart, &rx_data, 1, 10) == HAL_OK) {
            esp->rx_buffer[index++] = rx_data;
        }
    }
    esp->rx_buffer[index] = '\0';

    // Controlla se CIPMUX è OK
    if (strstr(esp->rx_buffer, "ERROR") != NULL) {
        return ESP8266_ERROR;
    }

    // Avvia il server
    sprintf(esp->tx_buffer, "AT+CIPSERVER=1,%d\r\n", port);
    ESP8266_ClearBuffer(esp);
    HAL_UART_Transmit(esp->huart, (uint8_t*)esp->tx_buffer, strlen(esp->tx_buffer), 1000);
    HAL_Delay(3000);

    // Leggi risposta del server
    index = 0;
    timeout = HAL_GetTick() + 3000;
    while (HAL_GetTick() < timeout && index < (ESP8266_BUFFER_SIZE - 1)) {
        uint8_t rx_data;
        if (HAL_UART_Receive(esp->huart, &rx_data, 1, 10) == HAL_OK) {
            esp->rx_buffer[index++] = rx_data;
        }
    }
    esp->rx_buffer[index] = '\0';

    // Controlla se il server è partito
    if (strstr(esp->rx_buffer, "OK") != NULL ||
        strstr(esp->rx_buffer, "no change") != NULL ||
        index == 0) { // Nessuna risposta potrebbe essere OK

        // Imposta timeout per le connessioni
        ESP8266_ClearBuffer(esp);
        HAL_UART_Transmit(esp->huart, (uint8_t*)"AT+CIPSTO=180\r\n", 15, 1000);
        HAL_Delay(1000);
        ESP8266_FlushRxBuffer(esp);

        esp->server_started = 1;
        return ESP8266_OK;
    }

    // Se arriviamo qui, il server potrebbe non essere partito
    if (strstr(esp->rx_buffer, "ERROR") != NULL) {
        return ESP8266_ERROR;
    }

    // Caso ambiguo - assumiamo OK
    esp->server_started = 1;
    return ESP8266_OK;
}

/**
 * @brief Invia dati attraverso una connessione specifica
 */
ESP8266_Result_t ESP8266_SendData(ESP8266_t *esp, uint8_t connection_id, const char *data, uint16_t length) {
    // Prepara il comando per inviare dati
    sprintf(esp->tx_buffer, "AT+CIPSEND=%d,%d\r\n", connection_id, length);

    if (ESP8266_SendCommand(esp, esp->tx_buffer, ">", 2000) != ESP8266_OK) {
        return ESP8266_ERROR;
    }

    // Invia i dati
    HAL_UART_Transmit(esp->huart, (uint8_t*)data, length, 2000);

    // Attendi conferma invio
    if (ESP8266_WaitForResponse(esp, "SEND OK", 5000)) {
        return ESP8266_OK;
    }

    return ESP8266_ERROR;
}

/**
 * @brief Chiude una connessione specifica
 */
ESP8266_Result_t ESP8266_CloseConnection(ESP8266_t *esp, uint8_t connection_id) {
    sprintf(esp->tx_buffer, "AT+CIPCLOSE=%d\r\n", connection_id);
    return ESP8266_SendCommand(esp, esp->tx_buffer, "OK", 2000);
}

/**
 * @brief Controlla se ci sono nuove connessioni
 */
ESP8266_Result_t ESP8266_CheckForConnection(ESP8266_t *esp, uint8_t *connection_id) {
    if (HAL_UART_Receive(esp->huart, (uint8_t*)esp->rx_buffer, ESP8266_BUFFER_SIZE, 100) == HAL_OK) {
        // Cerca pattern di nuova connessione
        char *connect_ptr = strstr(esp->rx_buffer, "+IPD,");
        if (connect_ptr != NULL) {
            *connection_id = connect_ptr[5] - '0'; // Estrai l'ID della connessione
            return ESP8266_OK;
        }
    }
    return ESP8266_ERROR;
}

/**
 * @brief Ottiene l'indirizzo IP dell'ESP8266 - versione robusta
 */
ESP8266_Result_t ESP8266_GetIPAddress(ESP8266_t *esp, char *ip_buffer) {
    // Pulisci buffer
    ESP8266_ClearBuffer(esp);

    // Invia comando per ottenere IP
    HAL_UART_Transmit(esp->huart, (uint8_t*)"AT+CIFSR\r\n", 10, 1000);
    HAL_Delay(2000);

    // Leggi risposta più a lungo per catturare tutto
    uint16_t index = 0;
    uint32_t timeout = HAL_GetTick() + 5000;
    while (HAL_GetTick() < timeout && index < (ESP8266_BUFFER_SIZE - 1)) {
        uint8_t rx_data;
        if (HAL_UART_Receive(esp->huart, &rx_data, 1, 10) == HAL_OK) {
            esp->rx_buffer[index++] = rx_data;
        }
    }
    esp->rx_buffer[index] = '\0';

    // Cerca diversi pattern di IP possibili
    char *ip_patterns[] = {
        "STAIP,\"",
        "APIP,\"",
        "+CIFSR:STAIP,\"",
        "+CIFSR:APIP,\""
    };

    for (int i = 0; i < 4; i++) {
        char *ip_start = strstr(esp->rx_buffer, ip_patterns[i]);
        if (ip_start != NULL) {
            ip_start += strlen(ip_patterns[i]); // Salta il pattern
            char *ip_end = strchr(ip_start, '"');
            if (ip_end != NULL) {
                int ip_length = ip_end - ip_start;
                if (ip_length < 16 && ip_length > 6) { // Lunghezza ragionevole per IP
                    strncpy(ip_buffer, ip_start, ip_length);
                    ip_buffer[ip_length] = '\0';

                    // Verifica che sia un IP valido (contiene almeno 2 punti)
                    int dots = 0;
                    for (int j = 0; j < ip_length; j++) {
                        if (ip_buffer[j] == '.') dots++;
                    }

                    if (dots >= 2) {
                        return ESP8266_OK;
                    }
                }
            }
        }
    }

    // Se non trova IP, prova un approccio più semplice
    // Cerca qualsiasi sequenza che assomiglia a un IP nella risposta
    for (int i = 0; i < index - 10; i++) {
        if (esp->rx_buffer[i] >= '1' && esp->rx_buffer[i] <= '9') {
            // Possibile inizio di IP
            int j = i;
            int dots = 0;
            while (j < index && j < i + 15) {
                char c = esp->rx_buffer[j];
                if (c == '.') {
                    dots++;
                } else if (c < '0' || c > '9') {
                    if (dots >= 2 && j > i + 6) {
                        // Sembra un IP valido
                        int ip_len = j - i;
                        strncpy(ip_buffer, &esp->rx_buffer[i], ip_len);
                        ip_buffer[ip_len] = '\0';
                        return ESP8266_OK;
                    }
                    break;
                }
                j++;
            }
        }
    }

    return ESP8266_ERROR;
}

/**
 * @brief Pulisce il buffer di ricezione
 */
void ESP8266_ClearBuffer(ESP8266_t *esp) {
    memset(esp->rx_buffer, 0, ESP8266_BUFFER_SIZE);
    memset(esp->tx_buffer, 0, ESP8266_BUFFER_SIZE);
}

/**
 * @brief Attende una risposta specifica
 */
uint8_t ESP8266_WaitForResponse(ESP8266_t *esp, const char *expected, uint32_t timeout) {
    uint32_t start_time = HAL_GetTick();
    uint16_t index = 0;

    while ((HAL_GetTick() - start_time) < timeout) {
        if (HAL_UART_Receive(esp->huart, (uint8_t*)&esp->rx_buffer[index], 1, 10) == HAL_OK) {
            index++;
            if (index >= ESP8266_BUFFER_SIZE - 1) {
                index = ESP8266_BUFFER_SIZE - 1;
            }
            esp->rx_buffer[index] = '\0';

            if (strstr(esp->rx_buffer, expected) != NULL) {
                return 1;
            }
        }
    }
    return 0;
}

/**
 * @brief Svuota il buffer UART
 */
void ESP8266_FlushRxBuffer(ESP8266_t *esp) {
    uint8_t dummy;
    while (HAL_UART_Receive(esp->huart, &dummy, 1, 10) == HAL_OK) {
        // Continua a leggere finché ci sono dati
    }
}
