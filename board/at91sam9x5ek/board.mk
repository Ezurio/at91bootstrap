CPPFLAGS += \
	-DCONFIG_AT91SAM9X5EK \
	-mcpu=arm926ej-s

ASFLAGS += \
	-DCONFIG_AT91SAM9X5EK \
	-mcpu=arm926ej-s

ifeq ($(CPU_HAS_PMECC),y)
	PMECC_HEADER := "board/pmecc_header.bin"
endif
