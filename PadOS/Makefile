
# /c/Program Files (x86)/Atmel/Studio/7.0/toolchain/arm/arm-gnu-toolchain/bin/arm-none-eabi-g++.exe
# /c/ST/STM32CubeIDE_1.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-arm-embedded.7-2018-q2-update.win32_1.0.0.201904081647/tools/bin/arm-none-eabi-g++.exe
# /c/ST/STM32CubeIDE_1.1.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.7-2018-q2-update.win32_1.0.0.201904181610/tools/bin/arm-none-eabi-g++.exe

#arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32H743xx -DDEBUG -c
#                   -I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/STM32H7xx_HAL_Driver/Inc
#                   -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.d" -MT"$@" --specs=nano.specs
#                   -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

#-mthumb -D__SAME70Q21__ -DARM_MATH_CM7=true -D__machine_ino_t_defined -D__ino_t=int64_t -D__PADOS__ -DDEBUG
#     -I".." -I"../ASF/common/utils" -I"../ASF/sam/utils" -I"../ASF/sam/utils/header_files" -I"../src/config" -I"$(BOOST)" -I"C:\Program Files (x86)\Atmel\Studio\7.0\Packs\atmel\SAME70_DFP\2.4.166\same70a\include" -I"." -I"C:\Program Files (x86)\Atmel\Studio\7.0\Packs\arm\CMSIS\5.4.0\CMSIS\Core\Include"
#     -Og -ffunction-sections -ffast-math -mlong-calls -g3 -Wall -Wextra -mcpu=cortex-m7 -c -mfloat-abi=hard -mtp=cp15 -mtls-dialect=gnu2
#     -Wsuggest-override -Wnoexcept -Wno-unused-parameter -Wno-multichar -Wno-sign-compare -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" 

INCLUDE_PATHS = -I. -ICore/Inc -IPlatformDrivers/CMSIS/Include -IPlatformDrivers/CMSIS/Device/ST/STM32H7xx/Include -IPlatformDrivers/STM32H7xx_HAL_Driver/Inc \
				-I../Drivers/CMSIS/Include -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/STM32H7xx_HAL_Driver/Inc

#-I"." -I"C:\Program Files (x86)\Atmel\Studio\7.0\Packs\arm\CMSIS\5.4.0\CMSIS\Core\Include" -I"C:\Program Files (x86)\Atmel\Studio\7.0\Packs\atmel\SAME70_DFP\2.4.166\same70a\include" -I"../../../PadOS/PadOS" -I"../../../PadOS/PadOS/ASF/common/utils" -I"../../../PadOS/PadOS/ASF/sam/utils" -I"../../../PadOS/PadOS/ASF/sam/utils/header_files"

FLAGS_WARNINGS = -Wall -Wextra -Wno-unused-parameter -Wno-multichar -Wno-sign-compare
FLAGS_OPTIMIZING = -g3 -Og
FLAGS_SYMBOLS = -D__PADOS__ -DUSE_HAL_DRIVER_______________ -DSTM32H743xx -D__FPU_PRESENT -DDEBUG -DARM_MATH_CM7=true -D__machine_ino_t_defined -D__ino_t=int64_t
FLAGS   := $(FLAGS_WARNINGS) $(FLAGS_SYMBOLS) $(INCLUDE_PATHS) $(FLAGS_OPTIMIZING) -mcpu=cortex-m7 -c -ffunction-sections -fdata-sections -fstack-usage -mthumb
FLAGS   := $(FLAGS) -ffast-math -mfpu=fpv5-d16 -mfloat-abi=hard -mtp=cp15 -mtls-dialect=gnu2
#FLAGS   := $(FLAGS) -mlong-calls
FLAGS   := $(FLAGS) -Wno-psabi
 
CFLAGS  := $(FLAGS) -std=gnu11
CPPFLAGS := $(FLAGS) -Wsuggest-override -Wnoexcept

CC   = arm-none-eabi-gcc
CCPP = arm-none-eabi-g++

OBJDIR	:= Debug
SUBDIRS := Kernel Kernel/Drivers/STM32 Kernel/HAL/STM32 Kernel/VFS System System/Ptr System/Signals System/Threads
SOURCES := Init.cpp posix.cpp

include $(foreach dir, $(SUBDIRS), $(dir)/Files.mk)


LIB := $(OBJDIR)/libPadOS.a

include makefile.rules

all:	$(TARGET_DIRS) $(LIB)

$(TARGET_DIRS):
	mkdir -p $@

$(LIB): $(OBJFILES) 
	arm-none-eabi-ar rcsu $@ $+
	@echo 'Finished building target: $@'
	@echo ' '


clean:
	rm -f $(OBJFILES) $(DEPFILES) $(patsubst %.o, %.su, $(OBJFILES))
#	rmdir $(TARGET_DIRS_REVERSE)
	
-include $(DEPFILES)
