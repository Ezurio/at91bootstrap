/*
 * WB40N Module support
 *
 * Copyright (c) 2008-2020, Laird Connectivity
 */
#include "common.h"
#include "hardware.h"
#include "arch/at91_ccfg.h"
#include "arch/at91_matrix.h"
#include "arch/at91_rstc.h"
#include "arch/at91_pmc.h"
#include "arch/at91_smc.h"
#include "arch/at91_pio.h"
#include "arch/at91_sdramc.h"
#include "gpio.h"
#include "pmc.h"
#include "usart.h"
#include "debug.h"
#include "sdramc.h"
#include "timer.h"
#include "watchdog.h"
#include "string.h"
#include "wb40n.h"

#ifdef CONFIG_USER_HW_INIT
extern void hw_init_hook(void);
#endif

static void at91_dbgu_hw_init(void)
{
	/* Configure DBGU pin */
	const struct pio_desc dbgu_pins[] = {
		{"RXD", AT91C_PIN_PB(14), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"TXD", AT91C_PIN_PB(15), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	/* Configure the dbgu pins */
	pmc_enable_periph_clock(AT91C_ID_PIOB);
	pio_configure(dbgu_pins);
}

static void initialize_dbgu(void)
{
	at91_dbgu_hw_init();

	usart_init(BAUDRATE(MASTER_CLOCK, 115200));
}

#ifdef CONFIG_SDRAM
static void sdramc_hw_init(void)
{
}

static void sdramc_init(void)
{
	struct sdramc_register sdramc_config;

	sdramc_config.cr = AT91C_SDRAMC_NC_9 | AT91C_SDRAMC_NR_13 | AT91C_SDRAMC_CAS_3
				| AT91C_SDRAMC_NB_4_BANKS | AT91C_SDRAMC_DBW_16_BITS
				| AT91C_SDRAMC_TWR_3 | AT91C_SDRAMC_TRC_9
				| AT91C_SDRAMC_TRP_3 | AT91C_SDRAMC_TRCD_3
				| AT91C_SDRAMC_TRAS_6 | AT91C_SDRAMC_TXSR_10;

	sdramc_config.tr = (MASTER_CLOCK * 7) / 1000000;
	sdramc_config.mdr = AT91C_SDRAMC_MD_SDRAM;

	sdramc_hw_init();

	/* Initialize the matrix (memory voltage = 3.3) */
	writel((readl(AT91C_BASE_CCFG + CCFG_EBICSA))
		| AT91C_EBI_CS1A_SDRAMC | AT91C_VDDIOM_SEL_33V
		| (0x01 << 17), /*  set I/O slew selection */
		AT91C_BASE_CCFG + CCFG_EBICSA);

	sdramc_initialize(&sdramc_config, AT91C_BASE_CS1);
}
#endif /* #ifdef CONFIG_SDRAM */

#ifdef CONFIG_HW_INIT

static void eth0_phy_init(void)
{
	const struct pio_desc eth0_phy_pins[] = {
		{"ETX0",	AT91C_PIN_PA(12),	0, PIO_DEFAULT, PIO_OUTPUT},
		{"ETX1",	AT91C_PIN_PA(13),	0, PIO_DEFAULT, PIO_OUTPUT},
		{"ERX0",	AT91C_PIN_PA(14),	0, PIO_DEFAULT, PIO_OUTPUT},
		{"ERX1",	AT91C_PIN_PA(15),	0, PIO_DEFAULT, PIO_OUTPUT},
		{"ETXEN",	AT91C_PIN_PA(16),	0, PIO_DEFAULT, PIO_INPUT},
		{"ERXDV",	AT91C_PIN_PA(17),	0, PIO_DEFAULT, PIO_INPUT},
		{"ERXER",	AT91C_PIN_PA(18),	0, PIO_DEFAULT, PIO_INPUT},
		{"ETXCK",	AT91C_PIN_PA(19),	0, PIO_DEFAULT, PIO_INPUT},
		{"EMDC",	AT91C_PIN_PA(20),	0, PIO_DEFAULT, PIO_OUTPUT},
		{"EMDIO",	AT91C_PIN_PA(21),	0, PIO_DEFAULT, PIO_OUTPUT},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	pio_configure(eth0_phy_pins);
	writel((1 << AT91C_ID_PIOA) | (1 << AT91C_ID_EMAC), (PMC_PCER + AT91C_BASE_PMC));
}

void hw_init(void)
{
	/* Disable watchdog */
	at91_disable_wdt();

	/*
	 * At this stage the main oscillator is supposed to be enabled
	 * PCK = MCK = MOSC
	 */
	pmc_init_pll(0);

	/* Configure PLLA = MOSC * (PLL_MULA + 1) / PLL_DIVA */
	pmc_cfg_plla(PLLA_SETTINGS, PLL_LOCK_TIMEOUT);

	/* PCK = PLLA/2 = 3 * MCK */
	pmc_cfg_mck(MCKR_SETTINGS, PLL_LOCK_TIMEOUT);

	/* Switch MCK on PLLA output */
	pmc_cfg_mck(MCKR_CSS_SETTINGS, PLL_LOCK_TIMEOUT);

	/* Configure PLLB  */
	//Needed for USB only, will be enabled at later stage if desired
	//pmc_cfg_pllb(PLLB_SETTINGS, PLL_LOCK_TIMEOUT);

	/* Enable External Reset */
	writel(AT91C_RSTC_KEY_UNLOCK | AT91C_RSTC_URSTEN, AT91C_BASE_RSTC + RSTC_RMR);

	/* Configure the EBI Slave Slot Cycle to 64 */
	writel((readl((AT91C_BASE_MATRIX + MATRIX_SCFG3)) & ~0xFF) | 0x40,
		(AT91C_BASE_MATRIX + MATRIX_SCFG3));

	/* This is required for the H+W breakout board, which does not have
	 * the phyaddr pins pulled properly at reset. */
	eth0_phy_init();

	/* Init timer */
	timer_init();

	/* Initialize dbgu */
	initialize_dbgu();

#ifdef CONFIG_SDRAM
	/* Initlialize sdram controller */
	sdramc_init();
#endif

#ifdef CONFIG_USER_HW_INIT
	hw_init_hook();
#endif
}
#endif /* #ifdef CONFIG_HW_INIT */

#ifdef CONFIG_SDCARD
static void sdcard_set_of_name_board(char *of_name)
{
	strcpy(of_name, "wb40n.dtb");
}

void at91_mci0_hw_init(void)
{
	const struct pio_desc mci_pins[] = {
		{"MCCK",	AT91C_PIN_PA(8), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"MCCDA",	AT91C_PIN_PA(7), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"MCDA0",	AT91C_PIN_PA(6), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"MCDA1",	AT91C_PIN_PA(9), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"MCDA2",	AT91C_PIN_PA(10), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"MCDA3",	AT91C_PIN_PA(11), 0, PIO_PULLUP, PIO_PERIPH_A},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},

	};

	/* Configure the PIO controller */
	pmc_enable_periph_clock(AT91C_ID_PIOA);
	pio_configure(mci_pins);

	/* Enable the clock */
	pmc_enable_periph_clock(AT91C_ID_MCI);

	/* Set of name function pointer */
	sdcard_set_of_name = &sdcard_set_of_name_board;
}
#endif /* #ifdef CONFIG_SDCARD */

#ifdef CONFIG_NANDFLASH
void nandflash_hw_init(void)
{
	unsigned int reg;

	/* Configure PIOs */
	const struct pio_desc nand_pins[] = {
		{"NANDCS",	CONFIG_SYS_NAND_ENABLE_PIN,	1, PIO_PULLUP, PIO_OUTPUT},
		{(char *)0,	0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	/* Setup Smart Media, first enable the address range of CS3 in HMATRIX user interface  */
	reg = readl(AT91C_BASE_CCFG + CCFG_EBICSA);
	reg |= AT91C_EBI_CS3A_SM;

	writel(reg, AT91C_BASE_CCFG + CCFG_EBICSA);

	/* Configure SMC CS3 */
	writel((AT91C_SMC_NWESETUP_(2)
		| AT91C_SMC_NCS_WRSETUP_(0)
		| AT91C_SMC_NRDSETUP_(2)
		| AT91C_SMC_NCS_RDSETUP_(0)),
		AT91C_BASE_SMC + SMC_SETUP3);

	writel((AT91C_SMC_NWEPULSE_(2)
		| AT91C_SMC_NCS_WRPULSE_(6)
		| AT91C_SMC_NRDPULSE_(2)
		| AT91C_SMC_NCS_RDPULSE_(6)),
		AT91C_BASE_SMC + SMC_PULSE3);

	writel((AT91C_SMC_NWECYCLE_(6)
		|  AT91C_SMC_NRDCYCLE_(6)),
		AT91C_BASE_SMC + SMC_CYCLE3);

	writel((AT91C_SMC_READMODE
		| AT91C_SMC_WRITEMODE
		| AT91C_SMC_NWAITM_NWAIT_DISABLE
		| AT91C_SMC_DBW_WIDTH_BITS_8
		| AT91C_SMC_TDFEN
		| AT91_SMC_TDF_(12)),
		AT91C_BASE_SMC + SMC_CTRL3);

	/* Configure the PIO controller */
	pmc_enable_periph_clock(AT91C_ID_PIOC);
	pio_configure(nand_pins);
}
#endif /* #ifdef CONFIG_NANDFLASH */

