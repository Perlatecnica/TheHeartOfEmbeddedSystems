/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - WiFi Scanner with Auto Connection
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "esp8266_driver.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>  // For atoi()

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TARGET_SSID "SCEL-net"         // SSID to search for and connect
#define TARGET_PASSWORD "Perlatecnica" // WiFi password

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
ESP8266_t esp8266;
uint8_t led_state = 0;  // 0 = OFF, 1 = ON
char ip_address[16] = {0};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void Print_Status(const char* message);
int Scan_WiFi_Networks_Detailed(void);
int Connect_To_WiFi(const char* ssid, const char* password);
int Start_Web_Server(uint16_t port);
void Process_Client_Request(void);
void Send_HTTP_Response(uint8_t connection_id, const char* status, const char* content);
void Handle_LED_Command(const char* command);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief Print debug messages on UART2
 */
void Print_Status(const char* message) {
    HAL_UART_Transmit(&huart2, (uint8_t*)message, strlen(message), 1000);
    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, 100);
}

/**
 * @brief Connect to WiFi network
 * @param ssid Network name
 * @param password Network password
 * @return 1 if connected successfully, 0 if failed
 */
int Connect_To_WiFi(const char* ssid, const char* password) {
    Print_Status("\n=== WIFI CONNECTION ===");

    char msg[150];
    snprintf(msg, sizeof(msg), "Attempting to connect to: %s", ssid);
    Print_Status(msg);

    // First disconnect from any existing connection
    Print_Status("Disconnecting from any previous connection...");
    ESP8266_ClearBuffer(&esp8266);
    HAL_UART_Transmit(&huart1, (uint8_t*)"AT+CWQAP\r\n", 10, 1000);
    HAL_Delay(2000);

    // Clear any pending data
    uint8_t dummy;
    while (HAL_UART_Receive(&huart1, &dummy, 1, 10) == HAL_OK);

    // Build connection command
    char cmd[200];
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);

    Print_Status("Sending connection command...");
    ESP8266_ClearBuffer(&esp8266);
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd, strlen(cmd), 1000);

    // Wait for connection (can take up to 30 seconds for some routers)
    Print_Status("Waiting for connection (up to 30 seconds)...");

    char response_buffer[2048];
    memset(response_buffer, 0, sizeof(response_buffer));
    int buffer_index = 0;
    uint32_t start_time = HAL_GetTick();
    uint32_t timeout = 30000; // 30 seconds

    int wifi_connected = 0;
    int got_ip = 0;
    int error_detected = 0;

    // Keep reading until we get a clear response or timeout
    while ((HAL_GetTick() - start_time) < timeout && buffer_index < (sizeof(response_buffer) - 1)) {
        uint8_t rx_data;
        if (HAL_UART_Receive(&huart1, &rx_data, 1, 100) == HAL_OK) {
            response_buffer[buffer_index++] = rx_data;
            response_buffer[buffer_index] = '\0';

            // Check for success indicators
            if (strstr(response_buffer, "WIFI CONNECTED") != NULL) {
                if (!wifi_connected) {
                    wifi_connected = 1;
                    Print_Status(">>> WiFi Connected!");
                }
            }

            if (strstr(response_buffer, "WIFI GOT IP") != NULL) {
                got_ip = 1;
                Print_Status(">>> Got IP Address!");
                // If we got IP, wait a bit more for any additional messages
                HAL_Delay(2000);

                // Read any remaining data
                while (HAL_UART_Receive(&huart1, &rx_data, 1, 100) == HAL_OK &&
                       buffer_index < (sizeof(response_buffer) - 1)) {
                    response_buffer[buffer_index++] = rx_data;
                }
                break;
            }

            // Check for specific error codes
            if (strstr(response_buffer, "+CWJAP:1") != NULL) {
                Print_Status("Error: Connection timeout");
                error_detected = 1;
                break;
            }
            if (strstr(response_buffer, "+CWJAP:2") != NULL) {
                Print_Status("Error: Wrong password");
                error_detected = 1;
                break;
            }
            if (strstr(response_buffer, "+CWJAP:3") != NULL) {
                Print_Status("Error: Cannot find AP");
                error_detected = 1;
                break;
            }
            if (strstr(response_buffer, "+CWJAP:4") != NULL) {
                Print_Status("Error: Connection failed");
                error_detected = 1;
                break;
            }
        }
    }

    // Debug: show what we received
    Print_Status("\nDebug - Full response:");
    // Print response in chunks
    int resp_len = strlen(response_buffer);
    for (int i = 0; i < resp_len; i += 200) {
        char chunk[201];
        int chunk_len = (resp_len - i) > 200 ? 200 : (resp_len - i);
        strncpy(chunk, response_buffer + i, chunk_len);
        chunk[chunk_len] = '\0';
        Print_Status(chunk);
    }

    // If we got both WIFI CONNECTED and GOT IP, assume success
    if (wifi_connected && got_ip && !error_detected) {
        Print_Status("\n=== CONNECTION SUCCESSFUL (based on response) ===");

        // Wait extra time for connection to stabilize
        Print_Status("Waiting for connection to stabilize...");
        HAL_Delay(5000);

        // Now get IP address
        Print_Status("\nGetting IP address...");
        ESP8266_ClearBuffer(&esp8266);
        HAL_UART_Transmit(&huart1, (uint8_t*)"AT+CIFSR\r\n", 10, 1000);
        HAL_Delay(3000);

        // Read IP response
        char ip_buffer[512];
        memset(ip_buffer, 0, sizeof(ip_buffer));
        int ip_index = 0;
        uint32_t ip_timeout = HAL_GetTick() + 3000;

        while (HAL_GetTick() < ip_timeout && ip_index < (sizeof(ip_buffer) - 1)) {
            uint8_t rx_data;
            if (HAL_UART_Receive(&huart1, &rx_data, 1, 10) == HAL_OK) {
                ip_buffer[ip_index++] = rx_data;
            }
        }
        ip_buffer[ip_index] = '\0';

        Print_Status("\nIP Response:");
        Print_Status(ip_buffer);

        // Look for IP address in response
        char* staip = strstr(ip_buffer, "STAIP");
        if (!staip) {
            staip = strstr(ip_buffer, "192.168");
            if (!staip) {
                staip = strstr(ip_buffer, "10.");
            }
        }

        if (staip) {
            // Try to extract IP
            char ip_addr[20] = {0};
            int ip_found = 0;

            // Look for pattern like "192.168.x.x"
            for (int i = 0; i < strlen(staip) && i < 15; i++) {
                if ((staip[i] >= '0' && staip[i] <= '9') || staip[i] == '.') {
                    ip_addr[i] = staip[i];
                    ip_found = 1;
                } else if (ip_found && staip[i] == '"') {
                    break;
                } else if (ip_found) {
                    break;
                }
            }

            if (strlen(ip_addr) > 6) {
                snprintf(msg, sizeof(msg), "\nYour IP Address is: %s", ip_addr);
                Print_Status(msg);
                Print_Status("\n*** SUCCESSFULLY CONNECTED! ***");
                strcpy(ip_address, ip_addr);  // Save IP for later use
                esp8266.connected = 1;  // Mark as connected
                return 1;
            }
        }

        // Even if we can't get IP, if we got CONNECTED and GOT IP messages, assume success
        Print_Status("\n*** CONNECTED (IP parsing failed but connection messages received) ***");
        esp8266.connected = 1;  // Mark as connected
        return 1;

    } else if (error_detected) {
        Print_Status("\n=== CONNECTION FAILED WITH ERROR! ===");
        return 0;

    } else {
        Print_Status("\n=== CONNECTION TIMEOUT OR UNCLEAR RESPONSE ===");

        // Try one more verification
        Print_Status("\nFinal verification attempt...");
        HAL_Delay(3000);

        ESP8266_ClearBuffer(&esp8266);
        HAL_UART_Transmit(&huart1, (uint8_t*)"AT+CWJAP?\r\n", 11, 1000);
        HAL_Delay(3000);

        char check_buffer[512];
        memset(check_buffer, 0, sizeof(check_buffer));
        int check_index = 0;
        uint32_t check_timeout = HAL_GetTick() + 3000;

        while (HAL_GetTick() < check_timeout && check_index < (sizeof(check_buffer) - 1)) {
            uint8_t rx_data;
            if (HAL_UART_Receive(&huart1, &rx_data, 1, 10) == HAL_OK) {
                check_buffer[check_index++] = rx_data;
            }
        }
        check_buffer[check_index] = '\0';

        Print_Status("Verification response:");
        Print_Status(check_buffer);

        if (strstr(check_buffer, ssid) != NULL || strstr(check_buffer, "No AP") == NULL) {
            Print_Status("Connection verified!");
            return 1;
        } else {
            Print_Status("Not connected");
            Print_Status("\nPossible issues:");
            Print_Status("1. ESP8266 firmware may need update");
            Print_Status("2. Try simpler password (no special chars)");
            Print_Status("3. Router security settings (WPA2 only recommended)");
            Print_Status("4. Power supply issues (ESP8266 needs stable 3.3V)");
            return 0;
        }
    }
}

/**
 * @brief Detailed WiFi network scan
 * @return 1 if target network found, 0 if not found
 */
int Scan_WiFi_Networks_Detailed(void) {
    Print_Status("\n=== DETAILED WIFI NETWORK SCAN ===");

    // Large buffer to hold all networks
    char scan_buffer[4096];
    memset(scan_buffer, 0, sizeof(scan_buffer));

    // Set Station mode
    Print_Status("Setting Station mode...");
    ESP8266_ClearBuffer(&esp8266);
    HAL_UART_Transmit(&huart1, (uint8_t*)"AT+CWMODE=1\r\n", 13, 1000);
    HAL_Delay(2000);

    // Clear buffer
    uint8_t dummy;
    while (HAL_UART_Receive(&huart1, &dummy, 1, 10) == HAL_OK);

    // Send scan command
    Print_Status("Sending AT+CWLAP command...");
    ESP8266_ClearBuffer(&esp8266);
    HAL_UART_Transmit(&huart1, (uint8_t*)"AT+CWLAP\r\n", 10, 1000);

    // Wait for response (scan can take time)
    Print_Status("Waiting for response (may take 10-15 seconds)...");

    uint32_t start_time = HAL_GetTick();
    uint32_t timeout = 15000; // 15 seconds timeout
    int buffer_index = 0;
    uint8_t rx_data;
    int found_ok = 0;

    // Collect all data until OK is found or timeout
    while ((HAL_GetTick() - start_time) < timeout && buffer_index < (sizeof(scan_buffer) - 1)) {
        if (HAL_UART_Receive(&huart1, &rx_data, 1, 100) == HAL_OK) {
            scan_buffer[buffer_index++] = rx_data;
            scan_buffer[buffer_index] = '\0';

            // Check if we received OK (end of scan)
            if (buffer_index >= 2) {
                if (scan_buffer[buffer_index-2] == 'O' && scan_buffer[buffer_index-1] == 'K') {
                    found_ok = 1;
                    // Continue reading for a few more ms to be sure
                    HAL_Delay(500);
                    while (HAL_UART_Receive(&huart1, &rx_data, 1, 10) == HAL_OK &&
                           buffer_index < (sizeof(scan_buffer) - 1)) {
                        scan_buffer[buffer_index++] = rx_data;
                    }
                    break;
                }
            }
        }
    }

    scan_buffer[buffer_index] = '\0';

    // Print response information
    char info_msg[100];
    snprintf(info_msg, sizeof(info_msg), "Received %d bytes in %lu ms",
             buffer_index, HAL_GetTick() - start_time);
    Print_Status(info_msg);

    if (found_ok) {
        Print_Status("Scan completed successfully!");
    } else {
        Print_Status("Timeout or buffer full");
    }

    // Print entire buffer for debug
    Print_Status("\n--- COMPLETE RAW RESPONSE ---");
    // Print in chunks to avoid UART overflow
    int chunk_size = 200;
    for (int i = 0; i < buffer_index; i += chunk_size) {
        int len = (buffer_index - i) < chunk_size ? (buffer_index - i) : chunk_size;
        HAL_UART_Transmit(&huart2, (uint8_t*)&scan_buffer[i], len, 1000);
        HAL_Delay(50); // Small pause between chunks
    }
    Print_Status("\n--- END OF RAW RESPONSE ---");

    // Parse and print found networks
    Print_Status("\n=== WIFI NETWORKS FOUND ===");

    char* line_start = scan_buffer;
    char* line_end;
    int network_count = 0;
    int target_found = 0;

    while ((line_end = strstr(line_start, "\r\n")) != NULL) {
        *line_end = '\0'; // Terminate the line

        // Look for lines starting with +CWLAP:
        if (strstr(line_start, "+CWLAP:") == line_start) {
            network_count++;

            // Extract network information
            char* info_start = line_start + 7; // Skip "+CWLAP:"

            // Format: +CWLAP:(enc,ssid,rssi,mac,channel,freq_offset,freq_cali)
            // Example: +CWLAP:(3,"MyNetwork",-70,"aa:bb:cc:dd:ee:ff",1,0,0)

            char network_info[200];
            snprintf(network_info, sizeof(network_info), "Network %d: %s", network_count, info_start);
            Print_Status(network_info);

            // Extract SSID
            char* ssid_start = strchr(info_start, '"');
            if (ssid_start) {
                ssid_start++; // Skip first quote
                char* ssid_end = strchr(ssid_start, '"');
                if (ssid_end) {
                    char ssid[65] = {0};
                    int ssid_len = ssid_end - ssid_start;
                    if (ssid_len < 64) {
                        strncpy(ssid, ssid_start, ssid_len);
                        ssid[ssid_len] = '\0';

                        // Print extracted SSID
                        char ssid_msg[100];
                        snprintf(ssid_msg, sizeof(ssid_msg), "  SSID: [%s]", ssid);
                        Print_Status(ssid_msg);

                        // Check if it's the target network
                        if (strcmp(ssid, TARGET_SSID) == 0) {
                            target_found = 1;
                            Print_Status("  *** TARGET NETWORK FOUND! ***");
                        }

                        // Extract RSSI (signal strength)
                        char* rssi_start = ssid_end + 2; // Skip "," after SSID
                        if (*rssi_start == ',') rssi_start++;
                        char* rssi_end = strchr(rssi_start, ',');
                        if (rssi_end) {
                            char rssi[10] = {0};
                            int rssi_len = rssi_end - rssi_start;
                            if (rssi_len < 10) {
                                strncpy(rssi, rssi_start, rssi_len);
                                char rssi_msg[50];
                                snprintf(rssi_msg, sizeof(rssi_msg), "  Signal: %s dBm", rssi);
                                Print_Status(rssi_msg);
                            }
                        }
                    }
                }
            }

            Print_Status(""); // Empty line between networks
        }

        line_start = line_end + 2; // Skip \r\n
    }

    // Final summary
    Print_Status("=== SCAN SUMMARY ===");
    snprintf(info_msg, sizeof(info_msg), "Total networks found: %d", network_count);
    Print_Status(info_msg);

    if (target_found) {
        snprintf(info_msg, sizeof(info_msg), "Network '%s' WAS FOUND!", TARGET_SSID);
        Print_Status(info_msg);
    } else {
        snprintf(info_msg, sizeof(info_msg), "Network '%s' was NOT found", TARGET_SSID);
        Print_Status(info_msg);
        Print_Status("Possible reasons:");
        Print_Status("1. Router is off or out of range");
        Print_Status("2. SSID is hidden (not broadcasting)");
        Print_Status("3. SSID contains special characters");
        Print_Status("4. Case mismatch in network name");
    }

    // Additional test: try searching with specific parameters
    if (!target_found) {
        Print_Status("\n=== SPECIFIC SEARCH ===");
        snprintf(info_msg, sizeof(info_msg), "Searching specifically for: %s", TARGET_SSID);
        Print_Status(info_msg);

        // Command to search for specific SSID
        char cmd[100];
        snprintf(cmd, sizeof(cmd), "AT+CWLAP=\"%s\"\r\n", TARGET_SSID);

        ESP8266_ClearBuffer(&esp8266);
        HAL_UART_Transmit(&huart1, (uint8_t*)cmd, strlen(cmd), 1000);
        HAL_Delay(5000);

        // Read response
        memset(scan_buffer, 0, 1000);
        buffer_index = 0;
        timeout = HAL_GetTick() + 5000;

        while (HAL_GetTick() < timeout && buffer_index < 999) {
            if (HAL_UART_Receive(&huart1, &rx_data, 1, 10) == HAL_OK) {
                scan_buffer[buffer_index++] = rx_data;
            }
        }
        scan_buffer[buffer_index] = '\0';

        Print_Status("Specific search result:");
        if (strstr(scan_buffer, "+CWLAP:") != NULL) {
            Print_Status("NETWORK FOUND with specific search!");
            Print_Status(scan_buffer);
        } else if (strstr(scan_buffer, "OK") != NULL) {
            Print_Status("Command executed but network not found");
        } else {
            Print_Status("No response or error");
            Print_Status(scan_buffer);
        }
    }

    // Return whether target network was found
    return target_found;
}

/**
 * @brief Start HTTP server on ESP8266
 * @param port Port number (usually 80)
 * @return 1 if successful, 0 if failed
 */
int Start_Web_Server(uint16_t port) {
    Print_Status("\n=== STARTING WEB SERVER ===");

    char msg[100];

    // Enable multiple connections
    Print_Status("Enabling multiple connections...");
    ESP8266_ClearBuffer(&esp8266);
    HAL_UART_Transmit(&huart1, (uint8_t*)"AT+CIPMUX=1\r\n", 13, 1000);
    HAL_Delay(2000);

    // Start server
    snprintf(msg, sizeof(msg), "Starting server on port %d...", port);
    Print_Status(msg);

    char cmd[50];
    snprintf(cmd, sizeof(cmd), "AT+CIPSERVER=1,%d\r\n", port);
    ESP8266_ClearBuffer(&esp8266);
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd, strlen(cmd), 1000);
    HAL_Delay(2000);

    // Set server timeout
    Print_Status("Setting server timeout...");
    ESP8266_ClearBuffer(&esp8266);
    HAL_UART_Transmit(&huart1, (uint8_t*)"AT+CIPSTO=180\r\n", 15, 1000);
    HAL_Delay(1000);

    Print_Status("*** WEB SERVER STARTED ***");
    snprintf(msg, sizeof(msg), "Server URL: http://%s:%d",
             strlen(ip_address) > 0 ? ip_address : "ESP_IP", port);
    Print_Status(msg);
    Print_Status("Commands:");
    Print_Status("  GET /ledon   - Turn ON LED");
    Print_Status("  GET /ledoff  - Turn OFF LED");
    Print_Status("  GET /status  - Get LED status");

    return 1;
}

/**
 * @brief Handle LED commands
 * @param command Command string
 */
void Handle_LED_Command(const char* command) {
    if (strstr(command, "ledon") != NULL) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        led_state = 1;
        Print_Status("LED turned ON");
    } else if (strstr(command, "ledoff") != NULL) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        led_state = 0;
        Print_Status("LED turned OFF");
    }
}

/**
 * @brief Send HTTP response to client
 * @param connection_id Connection ID
 * @param status HTTP status
 * @param content Response content
 */
void Send_HTTP_Response(uint8_t connection_id, const char* status, const char* content) {
    char response[512];
    char cmd[50];

    Print_Status("Sending HTTP response...");

    // Build HTTP response
    snprintf(response, sizeof(response),
            "HTTP/1.1 %s\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "\r\n"
            "%s",
            status, (int)strlen(content), content);

    // Clear any pending data first
    uint8_t dummy;
    while (HAL_UART_Receive(&huart1, &dummy, 1, 10) == HAL_OK);

    // Send data command
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d,%d\r\n", connection_id, (int)strlen(response));
    Print_Status(cmd);
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd, strlen(cmd), 1000);

    // Wait for '>' prompt with better error handling
    uint32_t start_time = HAL_GetTick();
    uint32_t timeout = 3000; // 3 seconds timeout
    int prompt_found = 0;
    char debug_buffer[100];
    int debug_idx = 0;

    while ((HAL_GetTick() - start_time) < timeout) {
        uint8_t rx_data;
        if (HAL_UART_Receive(&huart1, &rx_data, 1, 50) == HAL_OK) {
            // Collect response for debugging
            if (debug_idx < 99) {
                debug_buffer[debug_idx++] = rx_data;
                debug_buffer[debug_idx] = '\0';
            }

            if (rx_data == '>') {
                prompt_found = 1;
                Print_Status("Got > prompt");
                break;
            }

            // Check for ERROR response
            if (debug_idx > 5 && strstr(debug_buffer, "ERROR") != NULL) {
                Print_Status("ERROR response from ESP8266");
                Print_Status(debug_buffer);
                return;
            }
        }
    }

    if (!prompt_found) {
        Print_Status("ERROR: No > prompt received, debug buffer:");
        Print_Status(debug_buffer);

        // Try alternative approach - send without waiting for prompt
        Print_Status("Trying to send data anyway...");
        HAL_Delay(100);
    }

    // Send the response data
    HAL_UART_Transmit(&huart1, (uint8_t*)response, strlen(response), 2000);
    Print_Status("Response data sent");

    // Wait for send confirmation
    HAL_Delay(1000);

    // Read any response
    char send_result[100];
    int result_idx = 0;
    uint32_t result_timeout = HAL_GetTick() + 2000;

    while (HAL_GetTick() < result_timeout && result_idx < 99) {
        uint8_t rx_data;
        if (HAL_UART_Receive(&huart1, &rx_data, 1, 10) == HAL_OK) {
            send_result[result_idx++] = rx_data;
        }
    }
    send_result[result_idx] = '\0';

    if (strstr(send_result, "SEND OK") != NULL) {
        Print_Status("SEND OK received");
    } else if (result_idx > 0) {
        Print_Status("Send result:");
        Print_Status(send_result);
    }

    // Close connection
    HAL_Delay(500);
    snprintf(cmd, sizeof(cmd), "AT+CIPCLOSE=%d\r\n", connection_id);
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd, strlen(cmd), 1000);
    HAL_Delay(500);

    // Clear any remaining data
    while (HAL_UART_Receive(&huart1, &dummy, 1, 10) == HAL_OK);

    Print_Status("Response complete");
}

/**
 * @brief Process incoming client requests
 */
void Process_Client_Request(void) {
    static char rx_buffer[1024];
    static uint16_t rx_index = 0;
    static uint32_t last_check = 0;
    uint8_t rx_data;

    // Read all available data
    while (HAL_UART_Receive(&huart1, &rx_data, 1, 10) == HAL_OK) {
        if (rx_index < (sizeof(rx_buffer) - 1)) {
            rx_buffer[rx_index++] = rx_data;
            rx_buffer[rx_index] = '\0';
        }
    }

    // Check if we have data to process
    if (rx_index > 0) {
        // Look for +IPD pattern
        char* ipd_start = strstr(rx_buffer, "+IPD,");
        if (ipd_start != NULL) {
            Print_Status("\n*** NEW CLIENT CONNECTION! ***");

            // Extract connection ID
            uint8_t conn_id = ipd_start[5] - '0';

            // Find data length and start
            char* comma = strchr(ipd_start + 5, ',');
            if (comma != NULL) {
                // Get length
                int length = atoi(comma + 1);

                // Find start of data (after ':')
                char* data_start = strchr(comma, ':');
                if (data_start != NULL) {
                    data_start++; // Skip ':'

                    char msg[100];
                    snprintf(msg, sizeof(msg), "Connection ID: %d, Length: %d", conn_id, length);
                    Print_Status(msg);

                    // Print first line of request
                    char first_line[100] = {0};
                    int i;
                    for (i = 0; i < 99 && data_start[i] != '\r' && data_start[i] != '\n' && data_start[i] != '\0'; i++) {
                        first_line[i] = data_start[i];
                    }
                    first_line[i] = '\0';

                    Print_Status("Request:");
                    Print_Status(first_line);

                    // Process HTTP request
                    char response_body[200];

                    // Check for endpoints in the first line
                    if (strstr(first_line, "GET /ledon") != NULL) {
                        Handle_LED_Command("ledon");
                        snprintf(response_body, sizeof(response_body),
                                "{\"status\":\"success\",\"led\":\"on\",\"message\":\"LED turned on\"}");
                        Send_HTTP_Response(conn_id, "200 OK", response_body);
                    }
                    else if (strstr(first_line, "GET /ledoff") != NULL) {
                        Handle_LED_Command("ledoff");
                        snprintf(response_body, sizeof(response_body),
                                "{\"status\":\"success\",\"led\":\"off\",\"message\":\"LED turned off\"}");
                        Send_HTTP_Response(conn_id, "200 OK", response_body);
                    }
                    else if (strstr(first_line, "GET /status") != NULL) {
                        Print_Status("Processing /status request");
                        snprintf(response_body, sizeof(response_body),
                                "{\"status\":\"success\",\"led\":\"%s\",\"ip\":\"%s\"}",
                                led_state ? "on" : "off",
                                strlen(ip_address) > 0 ? ip_address : "192.168.1.2");
                        Send_HTTP_Response(conn_id, "200 OK", response_body);
                    }
                    else if (strstr(first_line, "GET / ") != NULL) {
                        // Root page
                        Print_Status("Processing root request");
                        snprintf(response_body, sizeof(response_body),
                                "{\"message\":\"ESP8266 LED Control\",\"endpoints\":[\"/ledon\",\"/ledoff\",\"/status\"]}");
                        Send_HTTP_Response(conn_id, "200 OK", response_body);
                    }
                    else if (strstr(first_line, "POST") != NULL) {
                        Print_Status("Processing POST request");
                        // Handle POST requests
                        char* body_start = strstr(data_start, "\r\n\r\n");
                        if (body_start != NULL) {
                            body_start += 4;

                            if (strstr(body_start, "ledon") != NULL) {
                                Handle_LED_Command("ledon");
                                snprintf(response_body, sizeof(response_body),
                                        "{\"status\":\"success\",\"led\":\"on\"}");
                            }
                            else if (strstr(body_start, "ledoff") != NULL) {
                                Handle_LED_Command("ledoff");
                                snprintf(response_body, sizeof(response_body),
                                        "{\"status\":\"success\",\"led\":\"off\"}");
                            }
                            else {
                                snprintf(response_body, sizeof(response_body),
                                        "{\"status\":\"error\",\"message\":\"Unknown command\"}");
                            }
                            Send_HTTP_Response(conn_id, "200 OK", response_body);
                        }
                    }
                    else {
                        // Unknown request
                        Print_Status("Unknown request - sending 404");
                        snprintf(response_body, sizeof(response_body),
                                "{\"status\":\"error\",\"message\":\"Unknown endpoint: %s\"}", first_line);
                        Send_HTTP_Response(conn_id, "404 Not Found", response_body);
                    }
                }
            }

            // Clear buffer after processing
            rx_index = 0;
            memset(rx_buffer, 0, sizeof(rx_buffer));
        }
        else {
            // No +IPD found, check if buffer is getting full
            if (rx_index > 500) {
                // Print what we have for debug
                Print_Status("Buffer without +IPD:");
                char temp[101];
                strncpy(temp, rx_buffer, 100);
                temp[100] = '\0';
                Print_Status(temp);

                // Clear buffer
                rx_index = 0;
                memset(rx_buffer, 0, sizeof(rx_buffer));
            }
        }
    }

    // Periodic alive message
    if ((HAL_GetTick() - last_check) > 10000) {
        Print_Status("Server listening...");
        last_check = HAL_GetTick();
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  Print_Status("\r\n=== ESP8266 WiFi Scanner ===");
  Print_Status("Initializing ESP8266...");

  // Initialize ESP8266 with minimal checks
  esp8266.huart = &huart1;
  HAL_Delay(3000); // Wait for ESP8266 to boot

  // Basic communication test
  Print_Status("Testing communication...");
  ESP8266_ClearBuffer(&esp8266);
  HAL_UART_Transmit(&huart1, (uint8_t*)"AT\r\n", 4, 1000);
  HAL_Delay(1000);

  int received = 0;
  uint32_t timeout = HAL_GetTick() + 2000;

  while (HAL_GetTick() < timeout) {
      uint8_t rx_data;
      if (HAL_UART_Receive(&huart1, &rx_data, 1, 10) == HAL_OK) {
          received++;
      }
  }

  if (received > 0) {
      Print_Status("Communication OK!");
      Print_Status("Received response from ESP8266");

      // Reset ESP8266
      Print_Status("Resetting module...");
      HAL_UART_Transmit(&huart1, (uint8_t*)"AT+RST\r\n", 8, 1000);
      HAL_Delay(5000); // Wait for complete reset

      // Clear buffer after reset
      uint8_t dummy;
      while (HAL_UART_Receive(&huart1, &dummy, 1, 10) == HAL_OK);

      // Disable echo
      HAL_UART_Transmit(&huart1, (uint8_t*)"ATE0\r\n", 6, 1000);
      HAL_Delay(500);

      // Perform WiFi scan
      int network_found = Scan_WiFi_Networks_Detailed();

      // If target network was found, try to connect
      if (network_found) {
          Print_Status("\n=== ATTEMPTING CONNECTION ===");
          if (Connect_To_WiFi(TARGET_SSID, TARGET_PASSWORD)) {
              Print_Status("\n*** SUCCESSFULLY CONNECTED TO WIFI! ***");
              Print_Status("System ready for next operations");

              // Start web server
              if (Start_Web_Server(80)) {
                  Print_Status("\n*** SYSTEM FULLY OPERATIONAL ***");
                  Print_Status("You can now control the LED via HTTP");

                  // Main server loop
                  uint32_t last_status_time = 0;
                  while (1) {
                      // Process incoming requests
                      Process_Client_Request();

                      // Periodic status update every 30 seconds
                      if ((HAL_GetTick() - last_status_time) > 30000) {
                          char status_msg[100];
                          snprintf(status_msg, sizeof(status_msg),
                                  "Server active - LED: %s - IP: %s",
                                  led_state ? "ON" : "OFF",
                                  strlen(ip_address) > 0 ? ip_address : "unknown");
                          Print_Status(status_msg);
                          last_status_time = HAL_GetTick();
                      }

                      // Small delay to prevent CPU overload
                      HAL_Delay(10);
                  }
              } else {
                  Print_Status("Failed to start web server!");
              }
          } else {
              Print_Status("\n*** FAILED TO CONNECT TO WIFI ***");
              Print_Status("Check password and try again");
          }
      } else {
          Print_Status("\n*** TARGET NETWORK NOT FOUND ***");
          Print_Status("Cannot attempt connection");
      }

  } else {
      Print_Status("ERROR: No communication with ESP8266!");
      Print_Status("Check:");
      Print_Status("- 3.3V power supply");
      Print_Status("- TX/RX connections");
      Print_Status("- CH_PD to 3.3V");
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // If connected and server is running, process requests
    if (esp8266.connected) {
        Process_Client_Request();
    }

    // Press blue button to repeat scan
    if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) {
        Print_Status("\n\n=== NEW SCAN ===");
        int found = Scan_WiFi_Networks_Detailed();

        if (found) {
            if (Connect_To_WiFi(TARGET_SSID, TARGET_PASSWORD)) {
                Print_Status("Connected successfully!");
                // Start server if not already running
                Start_Web_Server(80);
                // Return to main loop
            }
        }

        // Debounce
        while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) {
            HAL_Delay(50);
        }
    }

    // Status LED - solid when server running, blinking when not
    static uint32_t last_blink = 0;
    if (!esp8266.connected && (HAL_GetTick() - last_blink) > 500) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        last_blink = HAL_GetTick();
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
