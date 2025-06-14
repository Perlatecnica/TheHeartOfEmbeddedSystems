#include "hc05_driver.h"

/**
 * @brief Initialize the HC-05 module
 * @param hc05: Pointer to HC05_HandleTypeDef structure
 * @param huart: Pointer to UART handle used for communication
 * @param en_port: GPIO port for EN pin
 * @param en_pin: GPIO pin for EN control
 * @retval HC05_StatusTypeDef: Operation status
 */
HC05_StatusTypeDef HC05_Init(HC05_HandleTypeDef *hc05, UART_HandleTypeDef *huart,
                            GPIO_TypeDef *en_port, uint16_t en_pin)
{
    if (hc05 == NULL || huart == NULL) {
        return HC05_ERROR;
    }

    // Initialize the structure
    hc05->huart = huart;
    hc05->en_port = en_port;
    hc05->en_pin = en_pin;
    hc05->data_received = 0;
    hc05->rx_index = 0;

    // Clear buffers
    memset(hc05->rx_buffer, 0, HC05_BUFFER_SIZE);
    memset(hc05->tx_buffer, 0, HC05_BUFFER_SIZE);

    // Set module to data mode (EN = LOW)
    HAL_GPIO_WritePin(hc05->en_port, hc05->en_pin, GPIO_PIN_RESET);
    HAL_Delay(100);

    // Start interrupt reception
    HAL_UART_Receive_IT(hc05->huart, (uint8_t*)&hc05->rx_buffer[hc05->rx_index], 1);

    return HC05_OK;
}

/**
 * @brief Set the operating mode of the HC-05 module
 * @param hc05: Pointer to HC05_HandleTypeDef structure
 * @param mode: Mode to set (DATA or AT)
 * @retval HC05_StatusTypeDef: Operation status
 */
HC05_StatusTypeDef HC05_SetMode(HC05_HandleTypeDef *hc05, HC05_ModeTypeDef mode)
{
    if (hc05 == NULL) {
        return HC05_ERROR;
    }

    if (mode == HC05_MODE_AT) {
        // AT mode: EN = HIGH and 38400 baud rate
        HAL_GPIO_WritePin(hc05->en_port, hc05->en_pin, GPIO_PIN_SET);
        HAL_Delay(100);

        // Change baud rate to 38400 for AT mode
        hc05->huart->Init.BaudRate = 38400;
        if (HAL_UART_Init(hc05->huart) != HAL_OK) {
            return HC05_ERROR;
        }
    } else {
        // DATA mode: EN = LOW and 9600 baud rate
        HAL_GPIO_WritePin(hc05->en_port, hc05->en_pin, GPIO_PIN_RESET);
        HAL_Delay(100);

        // Change baud rate to 9600 for data mode
        hc05->huart->Init.BaudRate = 9600;
        if (HAL_UART_Init(hc05->huart) != HAL_OK) {
            return HC05_ERROR;
        }
    }

    HAL_Delay(100); // Wait for mode change

    // Restart interrupt reception
    HAL_UART_Receive_IT(hc05->huart, (uint8_t*)&hc05->rx_buffer[hc05->rx_index], 1);

    return HC05_OK;
}

/**
 * @brief Send an AT command to the HC-05 module
 * @param hc05: Pointer to HC05_HandleTypeDef structure
 * @param command: AT command to send
 * @param response: Buffer for response
 * @param timeout: Timeout in milliseconds
 * @retval HC05_StatusTypeDef: Operation status
 */
HC05_StatusTypeDef HC05_SendATCommand(HC05_HandleTypeDef *hc05, const char *command,
                                     char *response, uint32_t timeout)
{
    if (hc05 == NULL || command == NULL) {
        return HC05_ERROR;
    }

    // Prepare command with terminator
    snprintf(hc05->tx_buffer, HC05_BUFFER_SIZE, "%s\r\n", command);

    // Send the command
    if (HAL_UART_Transmit(hc05->huart, (uint8_t*)hc05->tx_buffer,
                         strlen(hc05->tx_buffer), HC05_UART_TIMEOUT) != HAL_OK) {
        return HC05_ERROR;
    }

    // Wait for response if requested
    if (response != NULL) {
        uint8_t temp_buffer[HC05_AT_RESPONSE_SIZE];
        memset(temp_buffer, 0, HC05_AT_RESPONSE_SIZE);

        if (HAL_UART_Receive(hc05->huart, temp_buffer, HC05_AT_RESPONSE_SIZE-1,
                            timeout) == HAL_OK) {
            strcpy(response, (char*)temp_buffer);
            return HC05_OK;
        } else {
            return HC05_TIMEOUT;
        }
    }

    return HC05_OK;
}

/**
 * @brief Send data through HC-05
 * @param hc05: Pointer to HC05_HandleTypeDef structure
 * @param data: Data to send
 * @retval HC05_StatusTypeDef: Operation status
 */
HC05_StatusTypeDef HC05_SendData(HC05_HandleTypeDef *hc05, const char *data)
{
    if (hc05 == NULL || data == NULL) {
        return HC05_ERROR;
    }

    if (HAL_UART_Transmit(hc05->huart, (uint8_t*)data, strlen(data),
                         HC05_UART_TIMEOUT) != HAL_OK) {
        return HC05_ERROR;
    }

    return HC05_OK;
}

/**
 * @brief Receive data from HC-05
 * @param hc05: Pointer to HC05_HandleTypeDef structure
 * @param data: Buffer for received data
 * @param size: Buffer size
 * @retval HC05_StatusTypeDef: Operation status
 */
HC05_StatusTypeDef HC05_ReceiveData(HC05_HandleTypeDef *hc05, char *data, uint16_t size)
{
    if (hc05 == NULL || data == NULL || size == 0) {
        return HC05_ERROR;
    }

    if (hc05->data_received) {
        uint16_t copy_size = (hc05->rx_index < size-1) ? hc05->rx_index : size-1;
        memcpy(data, hc05->rx_buffer, copy_size);
        data[copy_size] = '\0';

        // Reset buffer
        HC05_ClearBuffer(hc05);

        return HC05_OK;
    }

    return HC05_BUSY;
}

/**
 * @brief Set the device name of HC-05
 * @param hc05: Pointer to HC05_HandleTypeDef structure
 * @param name: Name to set
 * @retval HC05_StatusTypeDef: Operation status
 */
HC05_StatusTypeDef HC05_SetName(HC05_HandleTypeDef *hc05, const char *name)
{
    if (hc05 == NULL || name == NULL) {
        return HC05_ERROR;
    }

    char command[64];
    char response[HC05_AT_RESPONSE_SIZE];

    // Enter AT mode
    HC05_SetMode(hc05, HC05_MODE_AT);

    snprintf(command, sizeof(command), "AT+NAME=%s", name);

    HC05_StatusTypeDef status = HC05_SendATCommand(hc05, command, response, 2000);

    // Return to data mode
    HC05_SetMode(hc05, HC05_MODE_DATA);

    return status;
}

/**
 * @brief Set the PIN of HC-05 device
 * @param hc05: Pointer to HC05_HandleTypeDef structure
 * @param pin: PIN to set
 * @retval HC05_StatusTypeDef: Operation status
 */
HC05_StatusTypeDef HC05_SetPIN(HC05_HandleTypeDef *hc05, const char *pin)
{
    if (hc05 == NULL || pin == NULL) {
        return HC05_ERROR;
    }

    char command[32];
    char response[HC05_AT_RESPONSE_SIZE];

    // Enter AT mode
    HC05_SetMode(hc05, HC05_MODE_AT);

    snprintf(command, sizeof(command), "AT+PSWD=%s", pin);

    HC05_StatusTypeDef status = HC05_SendATCommand(hc05, command, response, 2000);

    // Return to data mode
    HC05_SetMode(hc05, HC05_MODE_DATA);

    return status;
}

/**
 * @brief Set the baud rate of HC-05 device
 * @param hc05: Pointer to HC05_HandleTypeDef structure
 * @param baudrate: Baud rate to set
 * @retval HC05_StatusTypeDef: Operation status
 */
HC05_StatusTypeDef HC05_SetBaudRate(HC05_HandleTypeDef *hc05, uint32_t baudrate)
{
    if (hc05 == NULL) {
        return HC05_ERROR;
    }

    char command[32];
    char response[HC05_AT_RESPONSE_SIZE];

    // Enter AT mode
    HC05_SetMode(hc05, HC05_MODE_AT);

    snprintf(command, sizeof(command), "AT+UART=%lu,0,0", baudrate);

    HC05_StatusTypeDef status = HC05_SendATCommand(hc05, command, response, 2000);

    // Return to data mode
    HC05_SetMode(hc05, HC05_MODE_DATA);

    return status;
}

/**
 * @brief Get the firmware version of HC-05
 * @param hc05: Pointer to HC05_HandleTypeDef structure
 * @param version: Buffer for version string
 * @retval HC05_StatusTypeDef: Operation status
 */
HC05_StatusTypeDef HC05_GetVersion(HC05_HandleTypeDef *hc05, char *version)
{
    if (hc05 == NULL || version == NULL) {
        return HC05_ERROR;
    }

    char response[HC05_AT_RESPONSE_SIZE];

    // Enter AT mode
    HC05_SetMode(hc05, HC05_MODE_AT);

    HC05_StatusTypeDef status = HC05_SendATCommand(hc05, "AT+VERSION?", response, 2000);

    if (status == HC05_OK) {
        strcpy(version, response);
    }

    // Return to data mode
    HC05_SetMode(hc05, HC05_MODE_DATA);

    return status;
}

/**
 * @brief Reset the HC-05 module
 * @param hc05: Pointer to HC05_HandleTypeDef structure
 * @retval HC05_StatusTypeDef: Operation status
 */
HC05_StatusTypeDef HC05_Reset(HC05_HandleTypeDef *hc05)
{
    if (hc05 == NULL) {
        return HC05_ERROR;
    }

    char response[HC05_AT_RESPONSE_SIZE];

    // Enter AT mode
    HC05_SetMode(hc05, HC05_MODE_AT);

    HC05_StatusTypeDef status = HC05_SendATCommand(hc05, "AT+RESET", response, 3000);

    HAL_Delay(2000); // Wait for reset

    // Return to data mode
    HC05_SetMode(hc05, HC05_MODE_DATA);

    return status;
}

/**
 * @brief Check if data is available
 * @param hc05: Pointer to HC05_HandleTypeDef structure
 * @retval uint8_t: 1 if data available, 0 otherwise
 */
uint8_t HC05_DataAvailable(HC05_HandleTypeDef *hc05)
{
    if (hc05 == NULL) {
        return 0;
    }

    return hc05->data_received;
}

/**
 * @brief Clear the reception buffer
 * @param hc05: Pointer to HC05_HandleTypeDef structure
 */
void HC05_ClearBuffer(HC05_HandleTypeDef *hc05)
{
    if (hc05 == NULL) {
        return;
    }

    // Stop current reception
    HAL_UART_AbortReceive_IT(hc05->huart);

    memset(hc05->rx_buffer, 0, HC05_BUFFER_SIZE);
    hc05->rx_index = 0;
    hc05->data_received = 0;

    // Restart interrupt reception
    HAL_UART_Receive_IT(hc05->huart, (uint8_t*)&hc05->rx_buffer[hc05->rx_index], 1);
}

/**
 * @brief UART interrupt handler for reception
 * @param hc05: Pointer to HC05_HandleTypeDef structure
 */
void HC05_IRQHandler(HC05_HandleTypeDef *hc05)
{
    if (hc05 == NULL) {
        return;
    }

    char received_char = hc05->rx_buffer[hc05->rx_index];

    // Ignore unwanted control characters
    if (received_char == 0x01 || received_char == 0x00) {
        // Restart reception without incrementing index
        HAL_UART_Receive_IT(hc05->huart, (uint8_t*)&hc05->rx_buffer[hc05->rx_index], 1);
        return;
    }

    // Increment buffer index
    hc05->rx_index++;

    // Check if received character is a terminator
    if (received_char == '\n' || received_char == '\r') {
        // Remove any termination characters from buffer
        while (hc05->rx_index > 0 &&
               (hc05->rx_buffer[hc05->rx_index-1] == '\n' ||
                hc05->rx_buffer[hc05->rx_index-1] == '\r')) {
            hc05->rx_index--;
        }

        // If we received at least one valid character
        if (hc05->rx_index > 0) {
            hc05->data_received = 1;
            hc05->rx_buffer[hc05->rx_index] = '\0';
        } else {
            // Empty message, restart reception
            HAL_UART_Receive_IT(hc05->huart, (uint8_t*)&hc05->rx_buffer[hc05->rx_index], 1);
        }
    }
    // Check if buffer is full
    else if (hc05->rx_index >= HC05_BUFFER_SIZE-1) {
        hc05->data_received = 1;
        hc05->rx_buffer[hc05->rx_index] = '\0';
    }
    else {
        // Continue receiving next character
        HAL_UART_Receive_IT(hc05->huart, (uint8_t*)&hc05->rx_buffer[hc05->rx_index], 1);
    }
}
