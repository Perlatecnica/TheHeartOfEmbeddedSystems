################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../HC05_Driver/hc05_driver.c 

OBJS += \
./HC05_Driver/hc05_driver.o 

C_DEPS += \
./HC05_Driver/hc05_driver.d 


# Each subdirectory must supply rules for building sources it contributes
HC05_Driver/%.o HC05_Driver/%.su HC05_Driver/%.cyclo: ../HC05_Driver/%.c HC05_Driver/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F401xE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../HC05_Driver -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-HC05_Driver

clean-HC05_Driver:
	-$(RM) ./HC05_Driver/hc05_driver.cyclo ./HC05_Driver/hc05_driver.d ./HC05_Driver/hc05_driver.o ./HC05_Driver/hc05_driver.su

.PHONY: clean-HC05_Driver

