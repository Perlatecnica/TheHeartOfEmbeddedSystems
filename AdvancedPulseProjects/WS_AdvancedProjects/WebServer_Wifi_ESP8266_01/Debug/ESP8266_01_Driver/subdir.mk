################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ESP8266_01_Driver/esp8266_diagnostics.c \
../ESP8266_01_Driver/esp8266_driver.c 

OBJS += \
./ESP8266_01_Driver/esp8266_diagnostics.o \
./ESP8266_01_Driver/esp8266_driver.o 

C_DEPS += \
./ESP8266_01_Driver/esp8266_diagnostics.d \
./ESP8266_01_Driver/esp8266_driver.d 


# Each subdirectory must supply rules for building sources it contributes
ESP8266_01_Driver/%.o ESP8266_01_Driver/%.su ESP8266_01_Driver/%.cyclo: ../ESP8266_01_Driver/%.c ESP8266_01_Driver/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../ESP8266_01_Driver -I../WebServer -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-ESP8266_01_Driver

clean-ESP8266_01_Driver:
	-$(RM) ./ESP8266_01_Driver/esp8266_diagnostics.cyclo ./ESP8266_01_Driver/esp8266_diagnostics.d ./ESP8266_01_Driver/esp8266_diagnostics.o ./ESP8266_01_Driver/esp8266_diagnostics.su ./ESP8266_01_Driver/esp8266_driver.cyclo ./ESP8266_01_Driver/esp8266_driver.d ./ESP8266_01_Driver/esp8266_driver.o ./ESP8266_01_Driver/esp8266_driver.su

.PHONY: clean-ESP8266_01_Driver

