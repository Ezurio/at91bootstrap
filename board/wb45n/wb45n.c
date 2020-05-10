/*
 * WB45N Module support
 *
 * Copyright (c) 2008-2020, Laird Connectivity
 */
#include "common.h"
#include "hardware.h"
#include "arch/at91_ccfg.h"
#include "arch/at91_rstc.h"
#include "arch/at91_pmc.h"
#include "arch/at91_smc.h"
#include "arch/at91_pio.h"
#include "arch/at91_ddrsdrc.h"
#include "gpio.h"
#include "pmc.h"
#include "usart.h"
#include "debug.h"
#include "ddramc.h"
#include "slowclk.h"
#include "timer.h"
#include "watchdog.h"
#include "string.h"
#include "wb45n.h"
#include "board_hw_info.h"

#ifdef CONFIG_USER_HW_INIT
extern void hw_init_hook(void);
#endif

static void at91_dbgu_hw_init(void)
{
	/* Configure DBGU pins */
	const struct pio_desc dbgu_pins[] = {
		{"RXD", AT91C_PIN_PA(9), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"TXD", AT91C_PIN_PA(10), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	pmc_enable_periph_clock(AT91C_ID_PIOA_B);
	pio_configure(dbgu_pins);
}

static void initialize_dbgu(void)
{
	at91_dbgu_hw_init();
	usart_init(BAUDRATE(MASTER_CLOCK, BAUD_RATE));
}

/* Using the Micron MT46H32M16LFBF-(6 & 5 combined) */
static void ddramc_reg_config(struct ddramc_register *ddramc_config)
{
	ddramc_config->mdr = (AT91C_DDRC2_DBW_16_BITS
			| AT91C_DDRC2_MD_LP_DDR_SDRAM);

	ddramc_config->cr = (AT91C_DDRC2_NC_DDR10_SDR9 /* 10 column bits(1K) */
			| AT91C_DDRC2_NR_13              /* 13 row bits (8K) */
			| AT91C_DDRC2_CAS_3              /* CAS Latency 3 */
			| AT91C_DDRC2_NB_BANKS_4         /* 4 banks */
			| AT91C_DDRC2_DLL_RESET_DISABLED /* DLL not reset */
			| AT91C_DDRC2_DECOD_INTERLEAVED);/*Interleaved decode*/

	ddramc_config->cr |= AT91C_DDRC2_EBISHARE;       /* DQM is shared with other controller */

	/*
	 * The DDR2-SDRAM device requires a refresh every 15.625 us or 7.81 us.
	 * With a 133 MHz frequency, the refresh timer count register must to be
	 * set with (15.625 x 133 MHz) ~ 2084 i.e. 0x824
	 * or (7.81 x 133 MHz) ~ 1040 i.e. 0x410.
	 */
	ddramc_config->rtr = 0x411;     /* Refresh timer: 7.8125us */

	/* One clock cycle @ 133 MHz = 7.5 ns */
	ddramc_config->t0pr = (AT91C_DDRC2_TRAS_(6)	/* 6 * 7.5 >= 45 ns */
			| AT91C_DDRC2_TRCD_(3)		/* 3 * 7.5 >= 18 ns */
			| AT91C_DDRC2_TWR_(2)		/* 2 * 7.5 >= 15   ns */
			| AT91C_DDRC2_TRC_(8)		/* 8 * 7.5 >= 75   ns */
			| AT91C_DDRC2_TRP_(3)		/* 3 * 7.5 >= 18   ns */
			| AT91C_DDRC2_TRRD_(2)		/* 2 * 7.5 = 15   ns */
			| AT91C_DDRC2_TWTR_(1)		/* 2 clock cycles min */
			| AT91C_DDRC2_TMRD_(2));	/* 2 clock cycles */

	ddramc_config->t1pr = (AT91C_DDRC2_TXP_(2)	/*  2 clock cycles */
			| AT91C_DDRC2_TXSRD_(0)		/* 0 clock cycles */
			| AT91C_DDRC2_TXSNR_(15)	/* 15 * 7.5 = 142.5 ns*/
			| AT91C_DDRC2_TRFC_(10));	/* 10 * 7.5 = 135 ns */

	ddramc_config->t2pr = AT91C_DDRC2_TRTP_(4);

	ddramc_config->lpr = (AT91C_DDRC2_LPCB_DISABLED
			| AT91C_DDRC2_CLK_FR
			| AT91C_DDRC2_LPDDR2_PWOFF_DISABLED
			| AT91C_DDRC2_PASR_(0)
			| AT91C_DDRC2_DS_(0)
			| AT91C_DDRC2_TIMEOUT_0
			| AT91C_DDRC2_ADPE_FAST
			| AT91C_DDRC2_UPD_MR_(0));
}

static void ddramc_init(void)
{
	unsigned long csa;
	struct ddramc_register ddramc_reg;

	memset(&ddramc_reg, 0, sizeof(ddramc_reg));

	ddramc_reg_config(&ddramc_reg);

	/* ENABLE DDR clock */
	pmc_enable_system_clock(AT91C_PMC_DDR);

	/* Chip select 1 is for DDR2/SDRAM */
	csa = readl(AT91C_BASE_CCFG + CCFG_EBICSA);
	csa |= AT91C_EBI_CS1A_SDRAMC;
	csa &= ~AT91C_EBI_DBPUC;
	csa |= AT91C_EBI_DBPDC;
	csa |= AT91C_EBI_DRV_HD;

	writel(csa, AT91C_BASE_CCFG + CCFG_EBICSA);

	/* LPDDRAM1 Controller initialize */
	lpddram1_initialize(AT91C_BASE_DDRSDRC, AT91C_BASE_CS1, &ddramc_reg);
}

static void gpio_init(void)
{
	/* Setup WB45NBT custom GPIOs */
	const struct pio_desc gpios[] = {
	/*	{"NAME",        PIN NUMBER,     VALUE, ATTRIBUTES,  TYPE },*/
		{"CHIP_PWD_L",  AT91C_PIN_PA(28),   0, PIO_DEFAULT, PIO_OUTPUT},
		{"VBUS_SENSE",  AT91C_PIN_PB(11),   0, PIO_DEFAULT, PIO_INPUT},
		{"VBUS_EN",     AT91C_PIN_PB(12),   0, PIO_DEFAULT, PIO_OUTPUT},
		{"IRQ",	        AT91C_PIN_PB(18),   0, PIO_PULLUP,  PIO_INPUT},
		{(char *)0,	    0,                  0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	/* Configure the PIO controller */
	pmc_enable_periph_clock(AT91C_ID_PIOA_B);
	pio_configure(gpios);
}

#ifdef CONFIG_HW_INIT
void hw_init(void)
{
	/* Disable watchdog */
	at91_disable_wdt();

	/*
	 * At this stage the main oscillator is
	 * supposed to be enabled PCK = MCK = MOSC
	 */
	pmc_init_pll(0);

	/* Configure PLLA = MOSC * (PLL_MULA + 1) / PLL_DIVA */
	pmc_cfg_plla(PLLA_SETTINGS, PLL_LOCK_TIMEOUT);

	/* Switch PCK/MCK on Main clock output */
	pmc_cfg_mck(BOARD_PRESCALER_MAIN_CLOCK, PLL_LOCK_TIMEOUT);

	/* Switch PCK/MCK on PLLA output */
	pmc_cfg_mck(BOARD_PRESCALER_PLLA, PLL_LOCK_TIMEOUT);

	/* Enable External Reset */
	writel(AT91C_RSTC_KEY_UNLOCK | AT91C_RSTC_URSTEN, AT91C_BASE_RSTC + RSTC_RMR);

	/* Init timer */
	timer_init();

#ifdef CONFIG_SCLK
	slowclk_enable_osc32();
#endif

	/* Initialize dbgu */
	initialize_dbgu();

	/* Initialize DDRAM Controller */
	ddramc_init();

	gpio_init();

#ifdef CONFIG_USER_HW_INIT
	hw_init_hook();
#endif
}
#endif /* #ifdef CONFIG_HW_INIT */

#ifdef CONFIG_SDCARD
static void sdcard_set_of_name_board(char *of_name)
{
	strcpy(of_name, "wb45n.dtb");
}

void at91_mci0_hw_init(void)
{
	const struct pio_desc mci_pins[] = {
		{"MCCK", AT91C_PIN_PA(17), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"MCCDA", AT91C_PIN_PA(16), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"MCDA0", AT91C_PIN_PA(15), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"MCDA1", AT91C_PIN_PA(18), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"MCDA2", AT91C_PIN_PA(19), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"MCDA3", AT91C_PIN_PA(20), 0, PIO_PULLUP, PIO_PERIPH_A},
		{(char *)0,	0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	/* Configure the PIO controller */
	pmc_enable_periph_clock(AT91C_ID_PIOA_B);
	pio_configure(mci_pins);

	/* Enable the clock */
	pmc_enable_periph_clock(AT91C_ID_HSMCI0);

	/* Set of name function pointer */
	sdcard_set_of_name = &sdcard_set_of_name_board;
}
#endif /* #ifdef CONFIG_SDCARD */

#ifdef CONFIG_NANDFLASH
void nandflash_hw_init(void)
{
	unsigned int reg;

	/* Configure Nand PINs */
	const struct pio_desc nand_pins_hi[] = {
		{"NANDOE",	CONFIG_SYS_NAND_OE_PIN,		0, PIO_PULLUP, PIO_PERIPH_A},
		{"NANDWE",	CONFIG_SYS_NAND_WE_PIN,		0, PIO_PULLUP, PIO_PERIPH_A},
		{"NANDALE",	CONFIG_SYS_NAND_ALE_PIN,	0, PIO_PULLUP, PIO_PERIPH_A},
		{"NANDCLE",	CONFIG_SYS_NAND_CLE_PIN,	0, PIO_PULLUP, PIO_PERIPH_A},
		{"NANDCS",	CONFIG_SYS_NAND_ENABLE_PIN,	1, PIO_PULLUP, PIO_OUTPUT},
		{"D0",	AT91C_PIN_PD(6), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"D1",	AT91C_PIN_PD(7), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"D2",	AT91C_PIN_PD(8), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"D3",	AT91C_PIN_PD(9), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"D4",	AT91C_PIN_PD(10), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"D5",	AT91C_PIN_PD(11), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"D6",	AT91C_PIN_PD(12), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"D7",	AT91C_PIN_PD(13), 0, PIO_PULLUP, PIO_PERIPH_A},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	const struct pio_desc nand_pins_lo[] = {
		{"NANDOE",	CONFIG_SYS_NAND_OE_PIN,		0, PIO_PULLUP, PIO_PERIPH_A},
		{"NANDWE",	CONFIG_SYS_NAND_WE_PIN,		0, PIO_PULLUP, PIO_PERIPH_A},
		{"NANDALE",	CONFIG_SYS_NAND_ALE_PIN,	0, PIO_PULLUP, PIO_PERIPH_A},
		{"NANDCLE",	CONFIG_SYS_NAND_CLE_PIN,	0, PIO_PULLUP, PIO_PERIPH_A},
		{"NANDCS",	CONFIG_SYS_NAND_ENABLE_PIN,	1, PIO_PULLUP, PIO_OUTPUT},
		{"NWP", AT91C_PIN_PD(10), 0, PIO_PULLUP, PIO_OUTPUT},
		{(char *)0,	0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	reg = readl(AT91C_BASE_CCFG + CCFG_EBICSA);
	reg |= AT91C_EBI_CS3A_SM;
	reg &= ~AT91C_EBI_NFD0_ON_D16;

	reg &= ~AT91C_EBI_DRV;
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
		| AT91C_SMC_NRDCYCLE_(6)),
		AT91C_BASE_SMC + SMC_CYCLE3);

	writel((AT91C_SMC_READMODE
		| AT91C_SMC_WRITEMODE
		| AT91C_SMC_NWAITM_NWAIT_DISABLE
		| AT91C_SMC_DBW_WIDTH_BITS_8
		| AT91C_SMC_TDFEN
		| AT91_SMC_TDF_(12)),
		AT91C_BASE_SMC + SMC_CTRL3);

	/* Configure the PIO controller */
	pio_configure(nand_pins_lo);

	pmc_enable_periph_clock(AT91C_ID_PIOC_D);
}
#endif /* #ifdef CONFIG_NANDFLASH */
