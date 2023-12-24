################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../bcm_spi_io.c \
../cfg_file.c \
../crc8.c \
../glob_vars.c \
../land_socket.c \
../main.c \
../main_sens.c \
../power.c \
../rfid.c \
../token.c 

OBJS += \
./bcm_spi_io.o \
./cfg_file.o \
./crc8.o \
./glob_vars.o \
./land_socket.o \
./main.o \
./main_sens.o \
./power.o \
./rfid.o \
./token.o 

C_DEPS += \
./bcm_spi_io.d \
./cfg_file.d \
./crc8.d \
./glob_vars.d \
./land_socket.d \
./main.d \
./main_sens.d \
./power.d \
./rfid.d \
./token.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabihf-gcc -D_GNU_SOURCE=1 -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


