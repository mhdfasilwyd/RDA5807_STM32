PROJECT = fm_radio
STD_PERIPH_LIBS = ./STM32F10x_StdPeriph_Lib_V3.6.0

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
ST_FLASH = st-flash

CFLAGS = -Os -Wall -Wl,--gc-sections -mlittle-endian -mthumb -mcpu=cortex-m3 -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER -DSYSTICK_DELAY

INCLUDES = -I./inc \
	-I./RDA_5807 \
	-I$(STD_PERIPH_LIBS)/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x \
	-I$(STD_PERIPH_LIBS)/Libraries/CMSIS/CM3/CoreSupport \
	-I$(STD_PERIPH_LIBS)/Libraries/STM32F10x_StdPeriph_Driver/inc \
	-I$(STD_PERIPH_LIBS)/Project/STM32F10x_StdPeriph_Template \
	-T$(STD_PERIPH_LIBS)/Project/STM32F10x_StdPeriph_Template/TrueSTUDIO/STM3210B-EVAL/stm32_flash.ld

SOURCES = ./src/main.c \
	./RDA_5807/RDA_5807.c \
	$(STD_PERIPH_LIBS)/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x/system_stm32f10x.c \
	$(STD_PERIPH_LIBS)/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c \
	$(STD_PERIPH_LIBS)/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c \
	$(STD_PERIPH_LIBS)/Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_i2c.c \
	$(STD_PERIPH_LIBS)/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x/startup/TrueSTUDIO/startup_stm32f10x_md.s

OBJS = $(SOURCES:.c=.o)

all: $(PROJECT).elf

$(PROJECT).elf: $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@
	$(OBJCOPY) -O ihex $(PROJECT).elf $(PROJECT).hex
	$(OBJCOPY) -O binary $(PROJECT).elf $(PROJECT).bin

clean:
	rm -f *.o *.elf *.hex *.bin

flash: all
	$(ST_FLASH) write $(PROJECT).bin 0x8000000

erase:
	$(ST_FLASH) erase
