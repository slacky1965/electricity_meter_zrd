# Set Project Name
PROJECT_NAME := electricity_meter_zrd

# Set the serial port number for downloading the firmware
DOWNLOAD_PORT := COM3

COMPILE_OS = $(shell uname -o)
LINUX_OS = GNU/Linux

ifeq ($(COMPILE_OS),$(LINUX_OS))	
	COMPILE_PREFIX = /opt/tc32/bin/tc32
else
	COMPILE_PREFIX = C:/TelinkSDK/opt/tc32/bin/tc32
	WIN32 = -DWIN32=1
endif

AS      = $(COMPILE_PREFIX)-elf-as
CC      = $(COMPILE_PREFIX)-elf-gcc
LD      = $(COMPILE_PREFIX)-elf-ld
NM      = $(COMPILE_PREFIX)-elf-nm
OBJCOPY = $(COMPILE_PREFIX)-elf-objcopy
OBJDUMP = $(COMPILE_PREFIX)-elf-objdump
ARCH	= $(COMPILE_PREFIX)-elf-ar
SIZE	= $(COMPILE_PREFIX)-elf-size

LIBS := -lzb_router -ldrivers_8258 -lsoft-fp

DEVICE_TYPE = -DROUTER=1
MCU_TYPE = -DMCU_CORE_8258=1
BOOT_FLAG = -DMCU_CORE_8258 -DMCU_STARTUP_8258

SDK_PATH := ./tl_zigbee_sdk
SRC_PATH := ./src
OUT_PATH := ./out
OTA_PATH := ./zigbee2mqtt/OTA
MAKE_INCLUDES := ./make
TOOLS_PATH := ./tools
VERSION_RELEASE := V$(shell awk -F " " '/APP_RELEASE/ {gsub("0x",""); printf "%.1f", $$3/10.0; exit}' $(SRC_PATH)/include/version_cfg.h)
VERSION_BUILD := $(shell awk -F " " '/APP_BUILD/ {gsub("0x",""); printf "%02d", $$3; exit}' ./src/include/version_cfg.h)


TL_Check = $(TOOLS_PATH)/tl_check_fw.py
TL_OTA_TOOL = ./zigbee_ota_tool_v2.2.exe

INCLUDE_PATHS := \
-I$(SDK_PATH)/platform \
-I$(SDK_PATH)/proj/common \
-I$(SDK_PATH)/proj \
-I$(SDK_PATH)/zigbee/common/includes \
-I$(SDK_PATH)/zigbee/zbapi \
-I$(SDK_PATH)/zigbee/bdb/includes \
-I$(SDK_PATH)/zigbee/gp \
-I$(SDK_PATH)/zigbee/zdo \
-I$(SDK_PATH)/zigbee/zcl \
-I$(SDK_PATH)/zigbee/ota \
-I$(SDK_PATH)/zbhci \
-I$(SRC_PATH) \
-I$(SRC_PATH)/include \
-I$(SRC_PATH)/include/boards \
-I$(SRC_PATH)/devices/include \
-I$(SRC_PATH)/devices/cosemlib/hdlc \
-I$(SRC_PATH)/devices/cosemlib/src \
-I$(SRC_PATH)/devices/cosemlib/share/util \
-I$(SRC_PATH)/common \
-I./common

 
LS_FLAGS := $(SDK_PATH)/platform/boot/8258/boot_8258.link

GCC_FLAGS := \
-ffunction-sections \
-fdata-sections \
-Wall \
-O2 \
-fpack-struct \
-fshort-enums \
-finline-small-functions \
-std=gnu99 \
-fshort-wchar \
-fms-extensions

GCC_FLAGS += \
$(DEVICE_TYPE) \
$(MCU_TYPE)

#$(WIN32)


OBJ_SRCS := 
S_SRCS := 
ASM_SRCS := 
C_SRCS := 
S_UPPER_SRCS := 
O_SRCS := 
FLASH_IMAGE := 
ELFS := 
OBJS := 
LST := 
SIZEDUMMY := 


RM := rm -rf

# All of the sources participating in the build are defined here
-include $(MAKE_INCLUDES)/zdo.mk
-include $(MAKE_INCLUDES)/zcl.mk
-include $(MAKE_INCLUDES)/wwah.mk
-include $(MAKE_INCLUDES)/ss.mk
-include $(MAKE_INCLUDES)/ota.mk
-include $(MAKE_INCLUDES)/mac.mk
-include $(MAKE_INCLUDES)/gp.mk
-include $(MAKE_INCLUDES)/common.mk
-include $(MAKE_INCLUDES)/bdb.mk
-include $(MAKE_INCLUDES)/aps.mk
-include $(MAKE_INCLUDES)/af.mk
-include $(MAKE_INCLUDES)/zbhci.mk
-include $(MAKE_INCLUDES)/proj.mk
-include $(MAKE_INCLUDES)/platformS.mk
-include $(MAKE_INCLUDES)/div_mod.mk
-include $(MAKE_INCLUDES)/platform.mk
-include ./project.mk

# Add inputs and outputs from these tool invocations to the build variables 
LST_FILE := $(OUT_PATH)/$(PROJECT_NAME).lst
BIN_FILE := $(OUT_PATH)/$(PROJECT_NAME).bin
ELF_FILE := $(OUT_PATH)/$(PROJECT_NAME).elf
LOWER_NAME := $(shell echo $(PROJECT_NAME) | tr [:upper:] [:lower:])
FIRMWARE_FILE := $(LOWER_NAME)_$(VERSION_RELEASE).$(VERSION_BUILD).bin
BOOTLOADER := $(TOOLS_PATH)/bootloader/bootloader_8258_512k.bin

SIZEDUMMY += \
sizedummy \


# All Target
all: pre-build main-build

flash: $(BIN_FILE)
	@python3 $(TOOLS_PATH)/TlsrPgm.py -p$(DOWNLOAD_PORT) -z11 -a-100 -s -m we 0x8000 $(BIN_FILE)
	
	
erase-flash:
	@python3 $(TOOLS_PATH)/TlsrPgm.py -p$(DOWNLOAD_PORT) -z11 -a-100 -s ea

flash-bootloader:
	@python3 $(TOOLS_PATH)/TlsrPgm.py -p$(DOWNLOAD_PORT) -z11 -a-100 -s -m we 0 $(BOOTLOADER)

reset:
	@python3 $(TOOLS_PATH)/TlsrPgm.py -p$(DOWNLOAD_PORT) -z11 -a-100 -s -t50 -a2550 -m -w i

# Main-build Target
main-build: clean $(ELF_FILE) secondary-outputs

#main-build: clean-project $(ELF_FILE) secondary-outputs

# Tool invocations
$(ELF_FILE): $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: TC32 C Linker'
	$(LD) --gc-sections -L $(SDK_PATH)/zigbee/lib/tc32 -L $(SDK_PATH)/platform/lib -L $(SDK_PATH)/platform/tc32 -T $(LS_FLAGS) -o "$(ELF_FILE)" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	
$(LST_FILE): $(ELF_FILE)
	@echo 'Invoking: TC32 Create Extended Listing'
	$(OBJDUMP) -x -D -l -S $(ELF_FILE)  > $(LST_FILE)
	@echo 'Finished building: $@'
	@echo ' '

$(BIN_FILE): $(ELF_FILE)
	@echo 'Create Flash image (binary format)'
	@$(OBJCOPY) -v -O binary $(ELF_FILE)  $(BIN_FILE)
	@python3 $(TL_Check) $(BIN_FILE)
	@echo 'Finished building: $@'
	@echo ' '
	@cp $(BIN_FILE) $(FIRMWARE_FILE)
	@echo 'Copy $(BIN_FILE) to $(FIRMWARE_FILE)'
	@echo ' '


sizedummy: $(ELF_FILE)
	@echo 'Invoking: Print Size'
	@$(SIZE) -t $(ELF_FILE)
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(FLASH_IMAGE) $(ELFS) $(OBJS) $(SIZEDUMMY) $(LST_FILE) $(ELF_FILE) *.bin
	-@echo ' '

clean-project:
	-$(RM) $(FLASH_IMAGE) $(ELFS) $(SIZEDUMMY) $(LST_FILE) $(ELF_FILE) $(PROJECT_NAME)_$(VERSION_RELEASE).$(VERSION_BUILD).bin
	-$(RM) -R $(OUT_PATH)/$(SRC_PATH)/*.o
	-$(RM) -R $(OUT_PATH)/$(SRC_PATH)/common/*.o
	-$(RM) -R $(OUT_PATH)/$(SRC_PATH)/devices/*.o
	-@echo ' '
	
	
pre-build:
	mkdir -p $(foreach s,$(OUT_DIR),$(OUT_PATH)$(s))
	mkdir -p $(OTA_PATH); 
	-@echo ' '

post-build:
	-"$(TOOLS_PATH)/tl_check_fw.sh" $(OUT_PATH)/$(PROJECT_NAME) tc32
	-@echo ' '
	
secondary-outputs: $(BIN_FILE) $(LST_FILE) $(FLASH_IMAGE) $(SIZEDUMMY)

.PHONY: all clean dependents pre-build
.SECONDARY: main-build pre-build post-build

