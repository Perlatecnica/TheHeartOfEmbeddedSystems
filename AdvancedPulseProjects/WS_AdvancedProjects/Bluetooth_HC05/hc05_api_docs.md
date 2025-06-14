# HC-05 Bluetooth Driver API Documentation

## Overview

This document provides comprehensive API documentation for the HC-05 Bluetooth module driver designed for STM32 microcontrollers. The driver supports both AT command mode and data communication mode with automatic baud rate switching.

## Features

- 🔧 **Dual Mode Operation**: Automatic switching between AT command mode (38400 baud) and data mode (9600 baud)
- 📡 **Interrupt-based Reception**: Non-blocking data reception using UART interrupts
- ⚙️ **Complete AT Command Support**: Device configuration, name setting, PIN setting, etc.
- 🛡️ **Robust Error Handling**: Comprehensive error checking and timeout management
- 🔄 **Buffer Management**: Automatic buffer clearing and overflow protection
- 📝 **Easy Integration**: Simple API for quick project integration

## Hardware Requirements

### Connections
| STM32F401RE Pin | HC-05 Module Pin | Description |
|----------------|------------------|-------------|
| PA9 (UART1_TX) | RX | Data transmission to module |
| PA10 (UART1_RX) | TX | Data reception from module |
| PA8 (GPIO) | EN/KEY | Mode control pin |
| GND | GND | Ground connection |
| 3.3V/5V | VCC | Power supply |

### Power Requirements
- Supply voltage: 3.3V or 5V (check your module specifications)
- Current consumption: ~30mA (active), ~2mA (sleep)

## API Reference

### Data Types

#### HC05_HandleTypeDef
```c
typedef struct {
    UART_HandleTypeDef *huart;      // UART handle pointer
    GPIO_TypeDef *en_port;          // GPIO port for EN pin
    uint16_t en_pin;                // GPIO pin for EN control
    char rx_buffer[HC05_BUFFER_SIZE]; // Reception buffer
    char tx_buffer[HC05_BUFFER_SIZE]; // Transmission buffer
    uint8_t data_received;          // Data received flag
    uint16_t rx_index;              // Reception buffer index
} HC05_HandleTypeDef;
```

#### HC05_StatusTypeDef
```c
typedef enum {
    HC05_OK = 0,        // Operation successful
    HC05_ERROR = 1,     // Operation failed
    HC05_TIMEOUT = 2,   // Operation timeout
    HC05_BUSY = 3       // Module busy
} HC05_StatusTypeDef;
```

#### HC05_ModeTypeDef
```c
typedef enum {
    HC05_MODE_DATA = 0, // Data mode (9600 baud)
    HC05_MODE_AT = 1    // AT command mode (38400 baud)
} HC05_ModeTypeDef;
```

### Core Functions

#### HC05_Init
```c
HC05_StatusTypeDef HC05_Init(HC05_HandleTypeDef *hc05, 
                            UART_HandleTypeDef *huart, 
                            GPIO_TypeDef *en_port, 
                            uint16_t en_pin);
```
**Description**: Initializes the HC-05 module and driver structure.

**Parameters**:
- `hc05`: Pointer to HC05_HandleTypeDef structure
- `huart`: Pointer to configured UART handle
- `en_port`: GPIO port for EN pin (e.g., GPIOA)
- `en_pin`: GPIO pin number for EN control (e.g., GPIO_PIN_8)

**Returns**: `HC05_StatusTypeDef` - Operation status

**Example**:
```c
HC05_HandleTypeDef hc05;
if (HC05_Init(&hc05, &huart1, GPIOA, GPIO_PIN_8) == HC05_OK) {
    printf("HC-05 initialized successfully\n");
}
```

#### HC05_SetMode
```c
HC05_StatusTypeDef HC05_SetMode(HC05_HandleTypeDef *hc05, 
                               HC05_ModeTypeDef mode);
```
**Description**: Sets the operating mode of the HC-05 module.

**Parameters**:
- `hc05`: Pointer to HC05_HandleTypeDef structure
- `mode`: Mode to set (`HC05_MODE_DATA` or `HC05_MODE_AT`)

**Returns**: `HC05_StatusTypeDef` - Operation status

**Notes**: 
- Automatically switches UART baud rate (9600 for data, 38400 for AT)
- Restarts interrupt reception after mode change

#### HC05_SendData
```c
HC05_StatusTypeDef HC05_SendData(HC05_HandleTypeDef *hc05, 
                                const char *data);
```
**Description**: Sends data through the Bluetooth connection.

**Parameters**:
- `hc05`: Pointer to HC05_HandleTypeDef structure
- `data`: Null-terminated string to send

**Returns**: `HC05_StatusTypeDef` - Operation status

**Example**:
```c
HC05_SendData(&hc05, "Hello Bluetooth!");
```

#### HC05_ReceiveData
```c
HC05_StatusTypeDef HC05_ReceiveData(HC05_HandleTypeDef *hc05, 
                                   char *data, 
                                   uint16_t size);
```
**Description**: Retrieves received data from the buffer.

**Parameters**:
- `hc05`: Pointer to HC05_HandleTypeDef structure
- `data`: Buffer to store received data
- `size`: Size of the receive buffer

**Returns**: `HC05_StatusTypeDef` - Operation status

**Example**:
```c
char received[256];
if (HC05_ReceiveData(&hc05, received, sizeof(received)) == HC05_OK) {
    printf("Received: %s\n", received);
}
```

#### HC05_DataAvailable
```c
uint8_t HC05_DataAvailable(HC05_HandleTypeDef *hc05);
```
**Description**: Checks if data is available in the receive buffer.

**Parameters**:
- `hc05`: Pointer to HC05_HandleTypeDef structure

**Returns**: `1` if data is available, `0` otherwise

**Example**:
```c
if (HC05_DataAvailable(&hc05)) {
    // Process received data
}
```

### Configuration Functions

#### HC05_SetName
```c
HC05_StatusTypeDef HC05_SetName(HC05_HandleTypeDef *hc05, 
                               const char *name);
```
**Description**: Sets the Bluetooth device name.

**Parameters**:
- `hc05`: Pointer to HC05_HandleTypeDef structure
- `name`: Device name (max 32 characters)

**Returns**: `HC05_StatusTypeDef` - Operation status

**Example**:
```c
HC05_SetName(&hc05, "STM32_Device");
```

#### HC05_SetPIN
```c
HC05_StatusTypeDef HC05_SetPIN(HC05_HandleTypeDef *hc05, 
                              const char *pin);
```
**Description**: Sets the pairing PIN code.

**Parameters**:
- `hc05`: Pointer to HC05_HandleTypeDef structure
- `pin`: PIN code (4-16 digits)

**Returns**: `HC05_StatusTypeDef` - Operation status

**Example**:
```c
HC05_SetPIN(&hc05, "1234");
```

#### HC05_SetBaudRate
```c
HC05_StatusTypeDef HC05_SetBaudRate(HC05_HandleTypeDef *hc05, 
                                   uint32_t baudrate);
```
**Description**: Sets the communication baud rate for data mode.

**Parameters**:
- `hc05`: Pointer to HC05_HandleTypeDef structure
- `baudrate`: Baud rate (1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200)

**Returns**: `HC05_StatusTypeDef` - Operation status

**Note**: This changes the module's default data mode baud rate.

### Utility Functions

#### HC05_GetVersion
```c
HC05_StatusTypeDef HC05_GetVersion(HC05_HandleTypeDef *hc05, 
                                  char *version);
```
**Description**: Retrieves the firmware version information.

**Parameters**:
- `hc05`: Pointer to HC05_HandleTypeDef structure
- `version`: Buffer to store version string

**Returns**: `HC05_StatusTypeDef` - Operation status

#### HC05_Reset
```c
HC05_StatusTypeDef HC05_Reset(HC05_HandleTypeDef *hc05);
```
**Description**: Performs a software reset of the HC-05 module.

**Parameters**:
- `hc05`: Pointer to HC05_HandleTypeDef structure

**Returns**: `HC05_StatusTypeDef` - Operation status

#### HC05_ClearBuffer
```c
void HC05_ClearBuffer(HC05_HandleTypeDef *hc05);
```
**Description**: Clears the reception buffer and restarts interrupt reception.

**Parameters**:
- `hc05`: Pointer to HC05_HandleTypeDef structure

#### HC05_SendATCommand
```c
HC05_StatusTypeDef HC05_SendATCommand(HC05_HandleTypeDef *hc05, 
                                     const char *command, 
                                     char *response, 
                                     uint32_t timeout);
```
**Description**: Sends a custom AT command to the module.

**Parameters**:
- `hc05`: Pointer to HC05_HandleTypeDef structure
- `command`: AT command to send (without \r\n)
- `response`: Buffer for response (can be NULL)
- `timeout`: Timeout in milliseconds

**Returns**: `HC05_StatusTypeDef` - Operation status

**Example**:
```c
char response[64];
HC05_SendATCommand(&hc05, "AT+ADDR?", response, 2000);
```

### Interrupt Handler

#### HC05_IRQHandler
```c
void HC05_IRQHandler(HC05_HandleTypeDef *hc05);
```
**Description**: UART interrupt handler for data reception. Must be called from `HAL_UART_RxCpltCallback()`.

**Parameters**:
- `hc05`: Pointer to HC05_HandleTypeDef structure

**Integration Example**:
```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        HC05_IRQHandler(&hc05);
    }
}
```

## Usage Examples

### Basic Setup
```c
#include "hc05_driver.h"

HC05_HandleTypeDef hc05;
UART_HandleTypeDef huart1;

int main(void) {
    // Initialize HAL, clocks, GPIO, UART...
    
    // Initialize HC-05
    if (HC05_Init(&hc05, &huart1, GPIOA, GPIO_PIN_8) == HC05_OK) {
        printf("HC-05 ready\n");
    }
    
    // Configure device
    HC05_SetName(&hc05, "MyDevice");
    HC05_SetPIN(&hc05, "0000");
    
    while (1) {
        if (HC05_DataAvailable(&hc05)) {
            char buffer[256];
            if (HC05_ReceiveData(&hc05, buffer, sizeof(buffer)) == HC05_OK) {
                printf("Received: %s\n", buffer);
                HC05_SendData(&hc05, "ACK");
            }
        }
        HAL_Delay(10);
    }
}
```

### Advanced Configuration
```c
// Get module information
char version[64];
if (HC05_GetVersion(&hc05, version) == HC05_OK) {
    printf("Firmware: %s\n", version);
}

// Custom AT command
char mac_addr[32];
if (HC05_SendATCommand(&hc05, "AT+ADDR?", mac_addr, 2000) == HC05_OK) {
    printf("MAC Address: %s\n", mac_addr);
}

// Change default baud rate
HC05_SetBaudRate(&hc05, 115200);
```

## Configuration Constants

```c
#define HC05_UART_TIMEOUT 1000        // UART operation timeout (ms)
#define HC05_BUFFER_SIZE 256          // Reception/transmission buffer size
#define HC05_AT_RESPONSE_SIZE 64      // AT command response buffer size
```

## Error Handling

Always check return values for proper error handling:

```c
HC05_StatusTypeDef status = HC05_SendData(&hc05, "test");
switch (status) {
    case HC05_OK:
        printf("Data sent successfully\n");
        break;
    case HC05_ERROR:
        printf("Transmission error\n");
        break;
    case HC05_TIMEOUT:
        printf("Operation timeout\n");
        break;
    case HC05_BUSY:
        printf("Module busy\n");
        break;
}
```

## Troubleshooting

### Common Issues

1. **No response to AT commands**
   - Check EN pin connection and voltage levels
   - Verify baud rate (should be 38400 for AT mode)
   - Ensure proper power supply

2. **Data reception not working**
   - Verify UART interrupt is enabled
   - Check `HAL_UART_RxCpltCallback()` implementation
   - Ensure `HC05_IRQHandler()` is called correctly

3. **Module not pairing**
   - Check if module is in discoverable mode
   - Verify PIN configuration
   - Reset module if necessary

### Debug Tips

- Use `printf()` to monitor driver status
- Check return values of all API calls
- Monitor UART traffic with logic analyzer
- Verify GPIO pin states during mode switching

## License

This driver is provided under MIT License. See LICENSE file for details.

## Contributing

Contributions are welcome! Please ensure code follows the existing style and includes appropriate documentation.