/*
 * esp8266_diagnostics.c
 * Funzioni complete di diagnostica per ESP8266-01
 * Versione corretta senza warning di compilazione
 */

#include "main.h"
#include "esp8266_driver.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Variabili esterne dal main.c
extern UART_HandleTypeDef huart1;  // UART per ESP8266
extern UART_HandleTypeDef huart2;  // UART per debug (ST-Link)

// Funzione di stampa (definita già nel main.c)
extern void Print_Status(const char* message);

// ============================================================================
// DICHIARAZIONI DELLE FUNZIONI (per evitare warning di compilazione)
// ============================================================================
static void ESP8266_FlushAndRead(void);
static void ESP8266_TestBaudrates(void);
static void ESP8266_MonitorConnectionResponse(void);

// ============================================================================
// IMPLEMENTAZIONI DELLE FUNZIONI
// ============================================================================

/**
 * @brief Legge e stampa tutto quello che arriva dall'ESP8266
 */
static void ESP8266_FlushAndRead(void) {
    uint8_t rx_data;
    char response[512] = {0};
    int index = 0;
    uint32_t timeout = HAL_GetTick() + 3000;

    Print_Status("Lettura risposta ESP8266:");

    while (HAL_GetTick() < timeout && index < (sizeof(response) - 1)) {
        if (HAL_UART_Receive(&huart1, &rx_data, 1, 10) == HAL_OK) {
            if (rx_data >= 32 && rx_data <= 126) {
                response[index++] = rx_data;
            } else if (rx_data == '\r' || rx_data == '\n') {
                response[index++] = rx_data;
            }
        }
    }

    if (index > 0) {
        response[index] = '\0';
        HAL_UART_Transmit(&huart2, (uint8_t*)response, strlen(response), 2000);
        Print_Status("--- Fine risposta ---");
    } else {
        Print_Status("Nessuna risposta ricevuta");
    }
}

/**
 * @brief Testa diversi baudrate
 */
static void ESP8266_TestBaudrates(void) {
    uint32_t baudrates[] = {115200, 9600, 74880, 57600, 38400, 19200};
    int num_rates = sizeof(baudrates) / sizeof(baudrates[0]);

    for (int i = 0; i < num_rates; i++) {
        char msg[50];
        snprintf(msg, sizeof(msg), "Provo baudrate: %lu", baudrates[i]);
        Print_Status(msg);

        // Riconfigura UART
        huart1.Init.BaudRate = baudrates[i];
        if (HAL_UART_Init(&huart1) != HAL_OK) {
            Print_Status("Errore configurazione UART");
            continue;
        }

        HAL_Delay(100);

        // Invia AT
        const char* at_cmd = "AT\r\n";
        HAL_UART_Transmit(&huart1, (uint8_t*)at_cmd, strlen(at_cmd), 1000);

        // Controlla risposta
        uint8_t rx_data;
        uint32_t timeout = HAL_GetTick() + 1000;
        int got_response = 0;

        while (HAL_GetTick() < timeout) {
            if (HAL_UART_Receive(&huart1, &rx_data, 1, 10) == HAL_OK) {
                got_response = 1;
                break;
            }
        }

        if (got_response) {
            Print_Status("RISPOSTA RICEVUTA! Baudrate corretto trovato.");
            ESP8266_FlushAndRead();
            return;
        }
    }

    Print_Status("Nessun baudrate funzionante trovato");
    // Ripristina baudrate originale
    huart1.Init.BaudRate = 115200;
    HAL_UART_Init(&huart1);
}

/**
 * @brief Monitora in dettaglio la risposta di connessione
 */
static void ESP8266_MonitorConnectionResponse(void) {
    uint8_t rx_data;
    char response[1024] = {0};
    int index = 0;
    uint32_t start_time = HAL_GetTick();
    uint32_t timeout = 20000; // 20 secondi
    uint32_t last_char_time = start_time;

    while ((HAL_GetTick() - start_time) < timeout) {
        if (HAL_UART_Receive(&huart1, &rx_data, 1, 10) == HAL_OK) {
            last_char_time = HAL_GetTick();

            // Stampa carattere ricevuto in tempo reale
            if (rx_data >= 32 && rx_data <= 126) {
                HAL_UART_Transmit(&huart2, &rx_data, 1, 10);
            } else if (rx_data == '\r') {
                HAL_UART_Transmit(&huart2, (uint8_t*)"<CR>", 4, 10);
            } else if (rx_data == '\n') {
                HAL_UART_Transmit(&huart2, (uint8_t*)"<LF>\r\n", 6, 10);
            }

            // Accumula nella risposta
            if (index < sizeof(response) - 1) {
                response[index++] = rx_data;
            }

            // Analizza pattern di risposta
            response[index] = '\0';
            if (strstr(response, "WIFI CONNECTED") != NULL) {
                Print_Status("\r\n>>> WIFI CONNECTED rilevato!");
            }
            if (strstr(response, "WIFI GOT IP") != NULL) {
                Print_Status("\r\n>>> WIFI GOT IP rilevato!");
            }
            if (strstr(response, "+CWJAP:") != NULL && strstr(response, "ERROR") != NULL) {
                Print_Status("\r\n>>> ERRORE di connessione rilevato!");

                // Estrai codice errore
                char *error_pos = strstr(response, "+CWJAP:");
                if (error_pos != NULL) {
                    int error_code = atoi(error_pos + 7);
                    char error_msg[50];

                    switch(error_code) {
                        case 1: strcpy(error_msg, "Timeout connessione"); break;
                        case 2: strcpy(error_msg, "Password errata"); break;
                        case 3: strcpy(error_msg, "AP non trovato"); break;
                        case 4: strcpy(error_msg, "Connessione fallita"); break;
                        default: snprintf(error_msg, sizeof(error_msg), "Errore sconosciuto (%d)", error_code); break;
                    }

                    char full_error[100];
                    snprintf(full_error, sizeof(full_error), "Codice errore %d: %s", error_code, error_msg);
                    Print_Status(full_error);
                }
                break;
            }
            if (strstr(response, "OK") != NULL && strstr(response, "WIFI GOT IP") != NULL) {
                Print_Status("\r\n>>> Connessione completata con successo!");
                break;
            }
        } else {
            // Nessun carattere ricevuto per un po'
            if ((HAL_GetTick() - last_char_time) > 3000 && index > 0) {
                // Non ci sono stati caratteri per 3 secondi, probabilmente finito
                break;
            }
        }
    }

    Print_Status("\r\n=== Fine monitoraggio ===");
}

// ============================================================================
// FUNZIONI PUBBLICHE (chiamabili dal main)
// ============================================================================

/**
 * @brief Test di diagnostica ESP8266 completo
 */
void ESP8266_DiagnosticTest(void) {
    char buffer[256];
    uint8_t rx_data;
    uint32_t timeout;

    Print_Status("=== DIAGNOSTICA ESP8266 ===");

    // Test 1: Verifica alimentazione e connessioni
    Print_Status("Test 1: Verifica basic UART...");
    HAL_Delay(3000); // Attendi che ESP8266 si avvii completamente

    // Test 2: Invio AT semplice
    Print_Status("Test 2: Invio comando AT...");
    const char* at_cmd = "AT\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)at_cmd, strlen(at_cmd), 1000);

    // Leggi risposta carattere per carattere
    Print_Status("Risposta ricevuta:");
    memset(buffer, 0, sizeof(buffer));
    int index = 0;
    timeout = HAL_GetTick() + 2000;

    while (HAL_GetTick() < timeout && index < (sizeof(buffer) - 1)) {
        if (HAL_UART_Receive(&huart1, &rx_data, 1, 10) == HAL_OK) {
            buffer[index++] = rx_data;

            // Stampa carattere ricevuto in formato leggibile
            if (rx_data >= 32 && rx_data <= 126) {
                char single_char[10];
                snprintf(single_char, sizeof(single_char), "'%c' ", rx_data);
                HAL_UART_Transmit(&huart2, (uint8_t*)single_char, strlen(single_char), 100);
            } else {
                char hex_char[10];
                snprintf(hex_char, sizeof(hex_char), "[0x%02X] ", rx_data);
                HAL_UART_Transmit(&huart2, (uint8_t*)hex_char, strlen(hex_char), 100);
            }
        }
    }

    buffer[index] = '\0';
    Print_Status("");

    if (index > 0) {
        snprintf(buffer, sizeof(buffer), "Ricevuti %d byte", index);
        Print_Status(buffer);
        Print_Status("Comunicazione UART funzionante!");

        // Test 3: Reset ESP8266
        Print_Status("Test 3: Reset ESP8266...");
        const char* reset_cmd = "AT+RST\r\n";
        HAL_UART_Transmit(&huart1, (uint8_t*)reset_cmd, strlen(reset_cmd), 1000);

        // Attendi messaggio di reset
        HAL_Delay(3000);
        ESP8266_FlushAndRead();

    } else {
        Print_Status("ERRORE: Nessuna risposta ricevuta!");
        Print_Status("Verifica:");
        Print_Status("1. Alimentazione 3.3V");
        Print_Status("2. Connessioni TX/RX incrociate");
        Print_Status("3. CH_PD collegato a 3.3V");
        Print_Status("4. GPIO0 e GPIO2 a 3.3V");
    }

    // Test 4: Verifica baudrate
    Print_Status("Test 4: Prova baudrate diversi...");
    ESP8266_TestBaudrates();
}

/**
 * @brief Test di connessione step-by-step
 */
void ESP8266_StepByStepTest(void) {
    Print_Status("=== TEST STEP-BY-STEP ===");

    const char* commands[] = {
        "AT\r\n",
        "AT+RST\r\n",
        "ATE0\r\n",
        "AT+CWMODE?\r\n",
        "AT+CWLAP\r\n"
    };

    const char* descriptions[] = {
        "Test comunicazione base",
        "Reset modulo",
        "Disabilita echo",
        "Controlla modalità WiFi",
        "Scansione reti WiFi"
    };

    int num_commands = sizeof(commands) / sizeof(commands[0]);

    for (int i = 0; i < num_commands; i++) {
        Print_Status(descriptions[i]);
        HAL_UART_Transmit(&huart1, (uint8_t*)commands[i], strlen(commands[i]), 1000);

        if (i == 1) { // Dopo reset, attendi di più
            HAL_Delay(3000);
        } else {
            HAL_Delay(1000);
        }

        ESP8266_FlushAndRead();
        Print_Status("---");
    }
}

/**
 * @brief Test specifico per connessione WiFi
 */
void ESP8266_TestWiFiConnection(const char* ssid, const char* password) {
    char buffer[100];

    Print_Status("=== TEST CONNESSIONE WIFI ===");

    // 1. Verifica modalità WiFi
    Print_Status("1. Verifica modalità WiFi corrente...");
    HAL_UART_Transmit(&huart1, (uint8_t*)"AT+CWMODE?\r\n", 12, 1000);
    HAL_Delay(1000);
    ESP8266_FlushAndRead();

    // 2. Imposta modalità Station se necessario
    Print_Status("2. Imposta modalità Station...");
    HAL_UART_Transmit(&huart1, (uint8_t*)"AT+CWMODE=1\r\n", 13, 1000);
    HAL_Delay(1000);
    ESP8266_FlushAndRead();

    // 3. Disconnetti da eventuali reti precedenti
    Print_Status("3. Disconnetti da reti precedenti...");
    HAL_UART_Transmit(&huart1, (uint8_t*)"AT+CWQAP\r\n", 10, 1000);
    HAL_Delay(2000);
    ESP8266_FlushAndRead();

    // 4. Scansiona reti disponibili
    Print_Status("4. Scansiona reti WiFi disponibili...");
    HAL_UART_Transmit(&huart1, (uint8_t*)"AT+CWLAP\r\n", 10, 1000);
    HAL_Delay(5000);
    ESP8266_FlushAndRead();

    // 5. Tenta connessione con debug dettagliato
    snprintf(buffer, sizeof(buffer), "5. Connessione a: %s", ssid);
    Print_Status(buffer);

    char connect_cmd[150];
    snprintf(connect_cmd, sizeof(connect_cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);

    Print_Status("Comando inviato:");
    Print_Status(connect_cmd);

    HAL_UART_Transmit(&huart1, (uint8_t*)connect_cmd, strlen(connect_cmd), 1000);

    // Monitora la risposta in tempo reale
    Print_Status("Monitoraggio risposta in tempo reale:");
    ESP8266_MonitorConnectionResponse();
}

/**
 * @brief Test rapido di comunicazione
 */
void ESP8266_QuickTest(void) {
    Print_Status("=== TEST RAPIDO ESP8266 ===");

    // Comando AT semplice
    Print_Status("Invio AT...");
    HAL_UART_Transmit(&huart1, (uint8_t*)"AT\r\n", 4, 1000);
    HAL_Delay(500);
    ESP8266_FlushAndRead();

    // Informazioni versione
    Print_Status("Richiedo versione firmware...");
    HAL_UART_Transmit(&huart1, (uint8_t*)"AT+GMR\r\n", 8, 1000);
    HAL_Delay(1000);
    ESP8266_FlushAndRead();

    // Status WiFi
    Print_Status("Status WiFi...");
    HAL_UART_Transmit(&huart1, (uint8_t*)"AT+CWJAP?\r\n", 11, 1000);
    HAL_Delay(1000);
    ESP8266_FlushAndRead();
}

/**
 * @brief Test completo - chiamata principale
 */
void ESP8266_FullDiagnostic(const char* ssid, const char* password) {
    Print_Status("=== DIAGNOSTICA COMPLETA ESP8266 ===");

    // Test rapido iniziale
    ESP8266_QuickTest();

    // Test diagnostico completo
    ESP8266_DiagnosticTest();

    // Test step-by-step
    ESP8266_StepByStepTest();

    // Test connessione WiFi se fornite le credenziali
    if (ssid != NULL && password != NULL) {
        ESP8266_TestWiFiConnection(ssid, password);
    }

    Print_Status("=== DIAGNOSTICA COMPLETATA ===");
    Print_Status("Controlla i risultati sopra per identificare problemi.");
}
