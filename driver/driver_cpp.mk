# Memory support

# RAM type

ifeq ($(CONFIG_SDRAM),y)
CPPFLAGS += -DCONFIG_SDRAM
endif

ifeq ($(CONFIG_SDDRC),y)
CPPFLAGS += -DCONFIG_SDDRC
endif

ifeq ($(CONFIG_DDRC),y)
CPPFLAGS += -DCONFIG_DDRC
endif

ifeq ($(CONFIG_LPDDR2),y)
CPPFLAGS += -DCONFIG_LPDDR2
endif

ifeq ($(CONFIG_DDR2),y)
CPPFLAGS += -DCONFIG_DDR2
endif

# Support for PSRAM on SAM9263EK EBI1

ifeq ($(CONFIG_PSRAM),y)
CPPFLAGS += -DCONFIG_PSRAM
endif

# 16 bit operation

ifeq ($(CONFIG_SDRAM_16BIT),y)
CPPFLAGS += -DCONFIG_SDRAM_16BIT
endif

# SDRAM/DDR/DDR2 size

ifeq ($(CONFIG_RAM_32MB),y)
CPPFLAGS += -DCONFIG_RAM_32MB
endif

ifeq ($(CONFIG_RAM_64MB),y)
CPPFLAGS += -DCONFIG_RAM_64MB
endif

ifeq ($(CONFIG_RAM_128MB),y)
CPPFLAGS += -DCONFIG_RAM_128MB
endif

ifeq ($(CONFIG_RAM_256MB),y)
CPPFLAGS += -DCONFIG_RAM_256MB
endif

ifeq ($(CONFIG_RAM_512MB),y)
CPPFLAGS += -DCONFIG_RAM_512MB
endif

# Boot flash type

ifeq ($(CONFIG_DATAFLASH),y)
CPPFLAGS += -DCONFIG_DATAFLASH
endif

ifeq ($(CONFIG_NANDFLASH),y)
CPPFLAGS += -DCONFIG_NANDFLASH
endif

ifeq ($(CONFIG_SDCARD),y)
CPPFLAGS += -DCONFIG_SDCARD
endif

ifeq ($(CONFIG_FLASH),y)
CPPFLAGS += -DCONFIG_FLASH
ASFLAGS += -DCONFIG_FLASH
endif

ifeq ($(CONFIG_LOAD_LINUX),y)
CPPFLAGS += -DCONFIG_LOAD_LINUX
endif

ifeq ($(CONFIG_LOAD_ANDROID),y)
CPPFLAGS += -DCONFIG_LOAD_ANDROID
endif

ifeq ($(CONFIG_LINUX_IMAGE), y)
CPPFLAGS += -DCONFIG_LINUX_IMAGE
endif

ifeq ($(CONFIG_SDCARD_HS),y)
CPPFLAGS += -DCONFIG_SDCARD_HS
endif

ifeq ($(CONFIG_OF_LIBFDT),y)
CPPFLAGS += -DCONFIG_OF_LIBFDT
endif

# Dataflash support
ifeq ($(CONFIG_DATAFLASH_RECOVERY),y)
CPPFLAGS += -DCONFIG_DATAFLASH_RECOVERY
endif

ifeq ($(CONFIG_SMALL_DATAFLASH),y)
CPPFLAGS += -DCONFIG_SMALL_DATAFLASH
endif

ifeq ($(MEMORY),dataflash)
CPPFLAGS += -DAT91C_SPI_CLK=$(SPI_CLK)
CPPFLAGS += -DAT91C_SPI_PCS_DATAFLASH=$(SPI_BOOT) 
endif

ifeq ($(MEMORY),dataflashcard)
CPPFLAGS += -DAT91C_SPI_CLK=$(SPI_CLK)
CPPFLAGS += -DAT91C_SPI_PCS_DATAFLASH=$(SPI_BOOT) 
endif

# NAND flash support

ifeq ($(CONFIG_NANDFLASH_SMALL_BLOCKS),y)
CPPFLAGS += -DCONFIG_NANDFLASH_SMALL_BLOCKS
endif

ifeq ($(CONFIG_ENABLE_SW_ECC), y)
CPPFLAGS += -DCONFIG_ENABLE_SW_ECC
endif

ifeq ($(CONFIG_USE_PMECC), y)
CPPFLAGS += -DCONFIG_USE_PMECC
endif

ifeq ($(CONFIG_ON_DIE_ECC), y)
CPPFLAGS += -DCONFIG_ON_DIE_ECC
endif

ifeq ($(CONFIG_NANDFLASH_RECOVERY),y)
CPPFLAGS += -DCONFIG_NANDFLASH_RECOVERY
endif

ifeq ($(CONFIG_PMECC_CORRECT_BITS_2), y)
CPPFLAGS += -DPMECC_ERROR_CORR_BITS=2
endif

ifeq ($(CONFIG_PMECC_CORRECT_BITS_4), y)
CPPFLAGS += -DPMECC_ERROR_CORR_BITS=4
endif

ifeq ($(CONFIG_PMECC_CORRECT_BITS_8), y)
CPPFLAGS += -DPMECC_ERROR_CORR_BITS=8
endif

ifeq ($(CONFIG_PMECC_CORRECT_BITS_12), y)
CPPFLAGS += -DPMECC_ERROR_CORR_BITS=12
endif

ifeq ($(CONFIG_PMECC_CORRECT_BITS_24), y)
CPPFLAGS += -DPMECC_ERROR_CORR_BITS=24
endif

ifeq ($(CONFIG_PMECC_SECTOR_SIZE_512), y)
CPPFLAGS += -DPMECC_SECTOR_SIZE=512
endif

ifeq ($(CONFIG_PMECC_SECTOR_SIZE_1024), y)
CPPFLAGS += -DPMECC_SECTOR_SIZE=1024
endif

ifeq ($(CONFIG_ONFI_DETECT_SUPPORT), y)
CPPFLAGS += -DCONFIG_ONFI_DETECT_SUPPORT
endif

ifeq ($(CONFIG_USE_ON_DIE_ECC_SUPPORT), y)
CPPFLAGS += -DCONFIG_USE_ON_DIE_ECC_SUPPORT
endif

# Debug related stuff
ifeq ($(CONFIG_DEBUG_INFO),y)
CPPFLAGS += -DBOOTSTRAP_DEBUG_LEVEL=DEBUG_INFO
endif

ifeq ($(CONFIG_DEBUG_LOUD),y)
CPPFLAGS += -DBOOTSTRAP_DEBUG_LEVEL=DEBUG_LOUD
endif

ifeq ($(CONFIG_DEBUG_VERY_LOUD),y)
CPPFLAGS += -DBOOTSTRAP_DEBUG_LEVEL=DEBUG_VERY_LOUD
endif

ifeq ($(CONFIG_DISABLE_WATCHDOG),y)
CPPFLAGS += -DCONFIG_DISABLE_WATCHDOG
endif

ifeq ($(CONFIG_LONG_FILENAME), y)
CPPFLAGS += -DCONFIG_LONG_FILENAME
endif

ifeq ($(CONFIG_TWI), y)
CPPFLAGS += -DCONFIG_TWI
endif

ifeq ($(CONFIG_TWI0), y)
CPPFLAGS += -DCONFIG_TWI0
endif

ifeq ($(CONFIG_TWI1), y)
CPPFLAGS += -DCONFIG_TWI1
endif

ifeq ($(CONFIG_TWI2), y)
CPPFLAGS += -DCONFIG_TWI2
endif

ifeq ($(CONFIG_TWI3), y)
CPPFLAGS += -DCONFIG_TWI3
endif

ifeq ($(CONFIG_DISABLE_ACT8865_I2C), y)
CPPFLAGS += -DCONFIG_DISABLE_ACT8865_I2C
endif

ifeq ($(CONFIG_PM), y)
CPPFLAGS += -DCONFIG_PM
endif

ifeq ($(CONFIG_MACB), y)
CPPFLAGS += -DCONFIG_MACB
endif

ifeq ($(CONFIG_MAC0_PHY), y)
CPPFLAGS += -DCONFIG_MAC0_PHY
endif

ifeq ($(CONFIG_MAC1_PHY), y)
CPPFLAGS += -DCONFIG_MAC1_PHY
endif

ifeq ($(CONFIG_HDMI), y)
CPPFLAGS += -DCONFIG_HDMI
endif

ifeq ($(CONFIG_WM8904), y)
CPPFLAGS += -DCONFIG_WM8904
endif

ifeq ($(CONFIG_LOAD_HW_INFO), y)
CPPFLAGS += -DCONFIG_LOAD_HW_INFO
endif

ifeq ($(CONFIG_HDMI_ON_TWI0), y)
CPPFLAGS += -DCONFIG_HDMI_ON_TWI0
endif

ifeq ($(CONFIG_HDMI_ON_TWI1), y)
CPPFLAGS += -DCONFIG_HDMI_ON_TWI1
endif

ifeq ($(CONFIG_HDMI_ON_TWI2), y)
CPPFLAGS += -DCONFIG_HDMI_ON_TWI2
endif

ifeq ($(CONFIG_HDMI_ON_TWI3), y)
CPPFLAGS += -DCONFIG_HDMI_ON_TWI3
endif

ifeq ($(CONFIG_CODEC_ON_TWI0), y)
CPPFLAGS += -DCONFIG_CODEC_ON_TWI0
endif

ifeq ($(CONFIG_CODEC_ON_TWI1), y)
CPPFLAGS += -DCONFIG_CODEC_ON_TWI1
endif

ifeq ($(CONFIG_CODEC_ON_TWI2), y)
CPPFLAGS += -DCONFIG_CODEC_ON_TWI2
endif

ifeq ($(CONFIG_CODEC_ON_TWI3), y)
CPPFLAGS += -DCONFIG_CODEC_ON_TWI3
endif

ifeq ($(CONFIG_PMIC_ON_TWI0), y)
CPPFLAGS += -DCONFIG_PMIC_ON_TWI0
endif

ifeq ($(CONFIG_PMIC_ON_TWI1), y)
CPPFLAGS += -DCONFIG_PMIC_ON_TWI1
endif

ifeq ($(CONFIG_PMIC_ON_TWI2), y)
CPPFLAGS += -DCONFIG_PMIC_ON_TWI2
endif

ifeq ($(CONFIG_PMIC_ON_TWI3), y)
CPPFLAGS += -DCONFIG_PMIC_ON_TWI3
endif

ifeq ($(CONFIG_EEPROM_ON_TWI0), y)
CPPFLAGS += -DCONFIG_EEPROM_ON_TWI0
endif

ifeq ($(CONFIG_EEPROM_ON_TWI1), y)
CPPFLAGS += -DCONFIG_EEPROM_ON_TWI1
endif

ifeq ($(CONFIG_EEPROM_ON_TWI2), y)
CPPFLAGS += -DCONFIG_EEPROM_ON_TWI2
endif

ifeq ($(CONFIG_EEPROM_ON_TWI3), y)
CPPFLAGS += -DCONFIG_EEPROM_ON_TWI3
endif

ifeq ($(CONFIG_PM_PMIC), y)
CPPFLAGS += -DCONFIG_PM_PMIC
endif
ifeq ($(CONFIG_AUTOCONFIG_TWI_BUS), y)
CPPFLAGS += -DCONFIG_AUTOCONFIG_TWI_BUS
endif

ifeq ($(CONFIG_SECURE), y)
CPPFLAGS += -DCONFIG_SECURE
endif

ifeq ($(CPU_HAS_PIO4), y)
CPPFLAGS += -DCPU_HAS_PIO4
endif
