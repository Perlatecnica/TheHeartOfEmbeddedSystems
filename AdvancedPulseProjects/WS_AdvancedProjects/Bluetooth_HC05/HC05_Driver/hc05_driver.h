#ifndef HC05_DRIVER_H
#define HC05_DRIVER_H

#include "stm32f4xx_hal.h"
#include <string.h>
#include <stdio.h>

// Configuration definitions
#define HC05_UART_TIMEOUT 1000
#define HC05_BUFFER_SIZE 256
#define HC05_AT_RESPONSE_SIZE 64

// HC-05 driver structure
typedef struct {
    UART_HandleTypeDef *huart;      // UART handle pointer
    GPIO_TypeDef *en_port;          // GPIO port for EN pin
    uint16_t en_pin;                // GPIO pin for EN control
    char rx_buffer[HC05_BUFFER_SIZE]; // Reception buffer
    char tx_buffer[HC05_BUFFER_SIZE]; // Transmission buffer
    uint8_t data_received;          // Data received flag
    uint16_t rx_index;              // Reception buffer index
} HC05_HandleTypeDef;

// HC-05 module states
typedef enum {
    HC05_OK = 0,        // Operation successful
    HC05_ERROR = 1,     // Operation failed
    HC05_TIMEOUT = 2,   // Operation timeout
    HC05_BUSY = 3       // Module busy
} HC05_StatusTypeDef;

// Operating modes
typedef enum {
    HC05_MODE_DATA = 0, // Data mode (9600 baud)
    HC05_MODE_AT = 1    // AT command mode (38400 baud)
} HC05_ModeTypeDef;

// Function prototypes
HC05_StatusTypeDef HC05_Init(HC05_HandleTypeDef *hc05, UART_HandleTypeDef *huart,
                            GPIO_TypeDef *en_port, uint16_t en_pin);
HC05_StatusTypeDef HC05_SetMode(HC05_HandleTypeDef *hc05, HC05_ModeTypeDef mode);
HC05_StatusTypeDef HC05_SendATCommand(HC05_HandleTypeDef *hc05, const char *command,
                                     char *response, uint32_t timeout);
HC05_StatusTypeDef HC05_SendData(HC05_HandleTypeDef *hc05, const char *data);
HC05_StatusTypeDef HC05_ReceiveData(HC05_HandleTypeDef *hc05, char *data, uint16_t size);
HC05_StatusTypeDef HC05_SetName(HC05_HandleTypeDef *hc05, const char *name);
HC05_StatusTypeDef HC05_SetPIN(HC05_HandleTypeDef *hc05, const char *pin);
HC05_StatusTypeDef HC05_SetBaudRate(HC05_HandleTypeDef *hc05, uint32_t baudrate);
HC05_StatusTypeDef HC05_GetVersion(HC05_HandleTypeDef *hc05, char *version);
HC05_StatusTypeDef HC05_Reset(HC05_HandleTypeDef *hc05);
uint8_t HC05_DataAvailable(HC05_HandleTypeDef *hc05);
void HC05_ClearBuffer(HC05_HandleTypeDef *hc05);
void HC05_IRQHandler(HC05_HandleTypeDef *hc05);

#endif /* HC05_DRIVER_H */
