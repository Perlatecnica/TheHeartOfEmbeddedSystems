# STM32CubeIDE Project Setup Guide for HC-05 Bluetooth Driver

This guide walks you through creating a new STM32CubeIDE project from scratch and integrating the HC-05 Bluetooth driver for STM32F401RE Nucleo board.

## Prerequisites

- STM32CubeIDE (version 1.10.0 or later)
- STM32F401RE Nucleo board
- HC-05 Bluetooth module
- USB cable for programming and debugging
- Jumper wires for connections

## Hardware Setup

### Required Connections

| STM32F401RE Nucleo | HC-05 Module | Description |
|-------------------|--------------|-------------|
| PA9 (CN10 pin 21) | RX | UART1 TX → HC-05 RX |
| PA10 (CN10 pin 33) | TX | UART1 RX ← HC-05 TX |
| PA8 (CN10 pin 23) | EN/KEY | Mode control pin |
| GND (CN6 pin 6) | GND | Ground connection |
| 3V3 (CN6 pin 4) | VCC | Power supply (3.3V) |

⚠️ **Note**: Some HC-05 modules require 5V. Check your module specifications and use CN6 pin 5 (+5V) if needed.

## Step 1: Create New STM32CubeIDE Project

### 1.1 Start New Project
1. Open STM32CubeIDE
2. Go to **File** → **New** → **STM32 Project**
3. In the Target Selection window:
   - **Board Selector** tab
   - Type "NUCLEO-F401RE" in the search box
   - Select **NUCLEO-F401RE** from the list
   - Click **Next**

### 1.2 Project Configuration
1. **Project Name**: `HC05_Bluetooth_Driver`
2. **Location**: Use default or choose your preferred location
3. **Targeted Language**: `C`
4. **Targeted Binary Type**: `Executable`
5. **Targeted Project Type**: `STM32Cube`
6. Click **Finish**

### 1.3 Initialize with Default Settings
- When prompted "Initialize all peripherals with their default Mode?", click **Yes**
- This will open STM32CubeMX integrated in the IDE

## Step 2: Configure Peripherals in STM32CubeMX

### 2.1 Configure UART1 for HC-05
1. In the **Pinout & Configuration** tab
2. Navigate to **Connectivity** → **USART1**
3. Set **Mode** to `Asynchronous`
4. The pins PA9 and PA10 should automatically be assigned
5. In **Parameter Settings**:
   - **Baud Rate**: `9600` (will be changed dynamically by driver)
   - **Word Length**: `8 Bits`
   - **Parity**: `None`
   - **Stop Bits**: `1`
   - **Data Direction**: `Receive and Transmit`
   - **Over Sampling**: `16 Samples`

### 2.2 Configure UART2 for Virtual COM Port
1. Navigate to **Connectivity** → **USART2**
2. **Mode** should already be set to `Asynchronous` (default for Nucleo)
3. Verify pins PA2 (TX) and PA3 (RX) are assigned
4. In **Parameter Settings**:
   - **Baud Rate**: `115200`
   - **Word Length**: `8 Bits`
   - **Parity**: `None`
   - **Stop Bits**: `1`
   - **Data Direction**: `Receive and Transmit`
   - **Over Sampling**: `16 Samples`

### 2.3 Configure GPIO for HC-05 EN Pin
1. In the **Pinout view**, click on pin **PA8**
2. Select **GPIO_Output**
3. Navigate to **System Core** → **GPIO**
4. Under **PA8** configuration:
   - **GPIO output level**: `Low`
   - **GPIO mode**: `Output Push Pull`
   - **GPIO Pull-up/Pull-down**: `No pull-up and no pull-down`
   - **Maximum output speed**: `Low`
   - **User Label**: `HC05_EN` (optional but recommended)

### 2.4 Enable UART Interrupts
1. Navigate to **System Core** → **NVIC**
2. In **NVIC** tab, enable:
   - ✅ **USART1 global interrupt**
   - Set **Priority**: `0`
3. USART2 interrupt is not needed for this project

### 2.5 Configure System Clock (Optional)
1. Go to **Clock Configuration** tab
2. The default configuration should work fine
3. Verify **HCLK** is set to `84 MHz` for optimal performance

## Step 3: Generate Code

1. Click **Project** → **Generate Code** (or Ctrl+S)
2. When prompted about file overwriting, choose appropriate options
3. Wait for code generation to complete
4. The project will automatically build

## Step 4: Add HC-05 Driver Files

### 4.1 Create Driver Directory
1. Right-click on project name in **Project Explorer**
2. Select **New** → **Folder**
3. Name it `HC05_Driver`
4. Click **Finish**

### 4.2 Add Driver Files
1. Right-click on `HC05_Driver` folder
2. Select **New** → **File**
3. Create `hc05_driver.h` and paste the header file content
4. Repeat for `hc05_driver.c` with the source file content

### 4.3 Include Driver in Project
1. Right-click on project → **Properties**
2. Go to **C/C++ Build** → **Settings**
3. Under **Tool Settings** → **MCU GCC Compiler** → **Include paths**
4. Click **Add...** (folder icon)
5. Click **Workspace...** and select `HC05_Bluetooth_Driver/HC05_Driver`
6. Click **OK** and **Apply and Close**

## Step 5: Modify main.c

### 5.1 Add Includes
Replace the generated `main.c` content with the provided main.c file, or manually add these modifications:

```c
/* USER CODE BEGIN Includes */
#include "hc05_driver.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */
```

### 5.2 Add Global Variables
```c
/* USER CODE BEGIN PV */
HC05_HandleTypeDef hc05;
/* USER CODE END PV */
```

### 5.3 Add Printf Redirection
```c
/* USER CODE BEGIN 0 */
int __io_putchar(int ch)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}
/* USER CODE END 0 */
```

### 5.4 Add Main Application Code
In the main function, add the HC-05 initialization and main loop as shown in the provided main.c file.

### 5.5 Add Interrupt Handlers
Add the UART interrupt handlers before the main function:

```c
/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1) {
    HC05_IRQHandler(&hc05);
  }
}

void USART1_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart1);
}
/* USER CODE END 4 */
```

## Step 6: Build and Debug Configuration

### 6.1 Build Project
1. Right-click project → **Build Project** (or Ctrl+B)
2. Check **Console** for any compilation errors
3. Resolve any include path or syntax issues

### 6.2 Configure Debug Settings
1. Right-click project → **Debug As** → **Debug Configurations...**
2. Double-click **STM32 Cortex-M C/C++ Application**
3. Verify settings:
   - **C/C++ Application**: `Debug/HC05_Bluetooth_Driver.elf`
   - **Interface**: `SWD`
   - **Port**: Default ST-Link port
4. Click **Apply** and **Debug**

## Step 7: Testing and Verification

### 7.1 Hardware Connection Verification
1. Double-check all wiring connections
2. Ensure proper power supply to HC-05 module
3. Verify EN pin connection to PA8

### 7.2 Serial Terminal Setup
1. Open a serial terminal (PuTTY, Tera Term, Arduino IDE Serial Monitor)
2. Connect to ST-Link Virtual COM Port
3. Set baud rate to **115200, 8N1**
4. Reset the board

### 7.3 Expected Output
You should see:
```
HC-05 initialized successfully
Name set: STM32_HC05
PIN set: 1234
System ready - Waiting for Bluetooth data...
Sending test character via Bluetooth...
```

### 7.4 Bluetooth Testing
1. Use a smartphone with Bluetooth terminal app
2. Search for "STM32_HC05" device
3. Pair using PIN "1234"
4. Send text messages
5. Verify messages appear on serial terminal with "BT RX:" prefix

## Step 8: Troubleshooting

### Common Issues and Solutions

#### Issue: Compilation Errors
**Solution**: 
- Check include paths in project properties
- Verify driver files are correctly added
- Ensure all required includes are present

#### Issue: No HC-05 Response
**Solution**:
- Verify hardware connections
- Check power supply voltage (3.3V or 5V)
- Test EN pin control with multimeter
- Try different HC-05 module

#### Issue: UART Not Working
**Solution**:
- Verify UART pins configuration in CubeMX
- Check interrupt enablement
- Ensure proper baud rate settings
- Test with logic analyzer if available

#### Issue: Bluetooth Not Pairing
**Solution**:
- Check if module enters AT mode properly
- Verify name and PIN configuration
- Reset module using HC05_Reset() function
- Check module LED status

### Debug Tips

1. **Add Debug Prints**: Use printf statements to trace execution
2. **Check Return Values**: Always verify API function return values
3. **Use Debugger**: Set breakpoints to examine variable states
4. **Monitor GPIOs**: Use oscilloscope to verify EN pin control
5. **UART Analysis**: Use logic analyzer to monitor UART communication

## Step 9: Advanced Configuration

### 9.1 Optimize Performance
- Adjust UART buffer sizes in `hc05_driver.h`
- Modify timeout values for specific applications
- Implement custom AT commands for advanced features

### 9.2 Add Features
- Implement command parsing for received data
- Add LED status indicators
- Create custom communication protocol
- Add data logging capabilities

### 9.3 Error Handling
- Implement robust error recovery
- Add watchdog timer protection
- Create connection monitoring
- Add automatic reconnection logic

## Step 10: Version Control (Optional)

### 10.1 Git Integration
1. Right-click project → **Team** → **Share Project**
2. Select **Git** and configure repository
3. Add `.gitignore` for STM32 projects:
```
Debug/
Release/
.metadata/
.settings/
*.launch
```

### 10.2 Documentation
- Document hardware changes
- Keep API documentation updated
- Version tag releases
- Maintain changelog

## Conclusion

You now have a complete HC-05 Bluetooth driver integrated into your STM32CubeIDE project. The driver provides a robust foundation for Bluetooth communication projects and can be easily extended for specific applications.

### Next Steps
- Explore additional AT commands
- Implement application-specific protocols
- Add sensor data transmission
- Create mobile app for communication

For support and updates, refer to the driver API documentation and STM32 community resources.