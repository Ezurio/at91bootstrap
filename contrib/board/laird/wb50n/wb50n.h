/*
 * WB50N Module support
 *
 * Copyright (c) 2008-2020, Laird Connectivity
 */
#ifndef __WB50N_H__
#define __WB50N_H__

/*
 * PMC Setting
 *
 * The main oscillator is enabled as soon as possible in the lowlevel_clock_init
 * and MCK is switched on the main oscillator.
 */
#define BOARD_MAINOSC		12000000

#if defined(CONFIG_BUS_SPEED_133MHZ)

#define MASTER_CLOCK		132000000

#if defined(CONFIG_CPU_CLK_528MHZ)

/* PCK = 528MHz, MCK = 132MHz */
#define PLLA_MULA		43
#define BOARD_PCK		((unsigned long)(BOARD_MAINOSC * \
							(PLLA_MULA + 1)))
#define BOARD_MCK		((unsigned long)((BOARD_MAINOSC * \
							(PLLA_MULA + 1)) / 4))

#define BOARD_CKGR_PLLA		(AT91C_CKGR_SRCA | AT91C_CKGR_OUTA_0)
#define BOARD_PLLACOUNT		(0x3F << 8)
#define BOARD_MULA		((AT91C_CKGR_MULA << 2) & (PLLA_MULA << 18))
#define BOARD_DIVA		(AT91C_CKGR_DIVA & 1)

#define BOARD_PRESCALER_MAIN_CLOCK	(AT91C_PMC_MDIV_4 \
					| AT91C_PMC_CSS_MAIN_CLK)

#define BOARD_PRESCALER_PLLA		(AT91C_PMC_MDIV_4 \
					| AT91C_PMC_CSS_PLLA_CLK)

#elif defined(CONFIG_CPU_CLK_396MHZ)

/* PCK = 396MHz, MCK = 132MHz */
#define PLLA_MULA		65
#define BOARD_PCK		((unsigned long)((BOARD_MAINOSC * \
						(PLLA_MULA + 1)) / 2))
#define BOARD_MCK		((unsigned long)((BOARD_MAINOSC * \
						(PLLA_MULA + 1)) / 2 / 3))

#define BOARD_CKGR_PLLA		(AT91C_CKGR_SRCA | AT91C_CKGR_OUTA_0)
#define BOARD_PLLACOUNT		(0x3F << 8)
#define BOARD_MULA		((AT91C_CKGR_MULA << 2) & (PLLA_MULA << 18))
#define BOARD_DIVA		(AT91C_CKGR_DIVA & 1)

/* Master Clock Register */
#define BOARD_PRESCALER_MAIN_CLOCK	(AT91C_PMC_PLLADIV2_2 \
					| AT91C_PMC_MDIV_3 \
					| AT91C_PMC_CSS_MAIN_CLK)

#define BOARD_PRESCALER_PLLA		(AT91C_PMC_PLLADIV2_2 \
					| AT91C_PMC_MDIV_3 \
					| AT91C_PMC_CSS_PLLA_CLK )

#elif defined(CONFIG_CPU_CLK_266MHZ)

/* PCK = 264MHz, MCK = 132MHz */
#define PLLA_MULA		43
#define BOARD_PCK		((unsigned long)((BOARD_MAINOSC * \
						(PLLA_MULA + 1)) / 2))
#define BOARD_MCK		((unsigned long)((BOARD_MAINOSC * \
						(PLLA_MULA + 1)) / 2 / 2))

#define BOARD_CKGR_PLLA		(AT91C_CKGR_SRCA | AT91C_CKGR_OUTA_0)
#define BOARD_PLLACOUNT		(0x3F << 8)
#define BOARD_MULA		((AT91C_CKGR_MULA << 2) & (PLLA_MULA << 18))
#define BOARD_DIVA		(AT91C_CKGR_DIVA & 1)

#define BOARD_PRESCALER_MAIN_CLOCK	(AT91C_PMC_PLLADIV2_2 \
					| AT91C_PMC_MDIV_2 \
					| AT91C_PMC_CSS_MAIN_CLK)

#define BOARD_PRESCALER_PLLA		(AT91C_PMC_PLLADIV2_2 \
					| AT91C_PMC_MDIV_2 \
					| AT91C_PMC_CSS_PLLA_CLK)

#else
#error "No cpu clock provided!"
#endif /* #if defined(CONFIG_CPU_CLK_528MHZ) */

#else
#error "No main clock provided!"
#endif /* #if defined(CONFIG_BUS_SPEED_133MHZ) */

#define PLLA_SETTINGS		(BOARD_CKGR_PLLA \
				| BOARD_PLLACOUNT \
				| BOARD_MULA \
				| BOARD_DIVA)

#define PLL_LOCK_TIMEOUT	10000

/*
 * NandFlash Settings
 */
#define CONFIG_SYS_NAND_BASE		AT91C_BASE_CS3
#define CONFIG_SYS_NAND_MASK_ALE	(1 << 21)
#define CONFIG_SYS_NAND_MASK_CLE	(1 << 22)

#undef CONFIG_SYS_NAND_ENABLE_PIN

#define CONFIG_LOOKUP_TABLE_ALPHA_OFFSET	0x14000
#define CONFIG_LOOKUP_TABLE_INDEX_OFFSET	0x10000

#define CONFIG_LOOKUP_TABLE_ALPHA_OFFSET_1024	0x20000
#define CONFIG_LOOKUP_TABLE_INDEX_OFFSET_1024	0x18000

/*
 * MCI Settings
 */
#define CONFIG_SYS_BASE_MCI	AT91C_BASE_HSMCI0

#endif /* #ifndef __WB50N_H__ */
