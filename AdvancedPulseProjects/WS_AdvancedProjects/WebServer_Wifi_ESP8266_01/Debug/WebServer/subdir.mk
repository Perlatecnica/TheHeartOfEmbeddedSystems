################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../WebServer/webserver.c 

OBJS += \
./WebServer/webserver.o 

C_DEPS += \
./WebServer/webserver.d 


# Each subdirectory must supply rules for building sources it contributes
WebServer/%.o WebServer/%.su WebServer/%.cyclo: ../WebServer/%.c WebServer/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../ESP8266_01_Driver -I../WebServer -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-WebServer

clean-WebServer:
	-$(RM) ./WebServer/webserver.cyclo ./WebServer/webserver.d ./WebServer/webserver.o ./WebServer/webserver.su

.PHONY: clean-WebServer

