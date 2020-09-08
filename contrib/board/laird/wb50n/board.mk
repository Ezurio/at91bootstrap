gcc_cortexa5=$(shell $(CC) --target-help | grep cortex-a5)

ifeq (, $(findstring cortex-a5,$(gcc_cortexa5)))
CPPFLAGS += -DCONFIG_WB50N

ASFLAGS += \
	-DCONFIG_WB50N
else
CPPFLAGS += \
	-DCONFIG_WB50N \
	-mcpu=cortex-a5 \
	-mtune=cortex-a5

ASFLAGS += \
	-DCONFIG_WB50N \
	-mcpu=cortex-a5
endif

ifeq ($(CPU_HAS_PMECC),y)
	PMECC_HEADER := "board/pmecc_header8.bin"
endif
