CPPFLAGS += \
	-DCONFIG_WB45N \
	-mcpu=arm926ej-s

ASFLAGS += \
	-DCONFIG_WB45N \
	-mcpu=arm926ej-s

ifeq ($(CPU_HAS_PMECC),y)
	PMECC_HEADER := "board/pmecc_header4.bin"
endif
