/*
 * WB40N Module support
 *
 * Copyright (c) 2008-2020, Ezurio
 */
#ifndef __WB40N_H__
#define __WB40N_H__

/*
 * PMC Setting
 *
 * The main oscillator is enabled as soon as possible in the c_startup
 * and MCK is switched on the main oscillator.
 * PLL initialization is done later in the hw_init() function
 */
#define MASTER_CLOCK		132096000
#define PLL_LOCK_TIMEOUT	1000000

#define PLLA_SETTINGS		0x202A3F01

/* Switch MCK on PLLA output PCK = PLLA/2 = 3 * MCK */
#define MCKR_SETTINGS		0x1300
#define MCKR_CSS_SETTINGS	(AT91C_PMC_CSS_PLLA_CLK | MCKR_SETTINGS)

/*
 * NandFlash Settings
 */
#define CONFIG_SYS_NAND_BASE		AT91C_BASE_CS3
#define CONFIG_SYS_NAND_MASK_ALE	(1 << 21)
#define CONFIG_SYS_NAND_MASK_CLE	(1 << 22)

#define CONFIG_SYS_NAND_ENABLE_PIN	AT91C_PIN_PC(14)

/*
 * MCI Settings
 */
#define CONFIG_SYS_BASE_MCI	AT91C_BASE_MCI

#endif	/* #ifndef __WB40N_H__ */
