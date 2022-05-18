/*
 * WB50N Module support
 *
 * Copyright (c) 2008-2020, Laird Connectivity
 */
#include "common.h"
#include "hardware.h"
#include "pmc.h"
#include "usart.h"
#include "debug.h"
#include "ddramc.h"
#include "gpio.h"
#include "slowclk.h"
#include "timer.h"
#include "watchdog.h"
#include "string.h"
#include "board.h"

#include "arch/at91_pmc/pmc.h"
#include "arch/at91_rstc.h"
#include "arch/sama5_smc.h"
#include "arch/at91_pio.h"
#include "arch/at91_ddrsdrc.h"
#include "arch/at91_sfr.h"
#include "wb50n.h"

static void at91_dbgu_hw_init(void)
{
	/* Configure DBGU pin */
	const struct pio_desc dbgu_pins[] = {
		{"RXD", AT91C_PIN_PB(30), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"TXD", AT91C_PIN_PB(31), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	/*  Configure the dbgu pins */
	pmc_enable_periph_clock(AT91C_ID_PIOB, PMC_PERIPH_CLK_DIVIDER_NA);
	pio_configure(dbgu_pins);

	/* Enable clock */
	pmc_enable_periph_clock(AT91C_ID_DBGU, PMC_PERIPH_CLK_DIVIDER_NA);
}

static void initialize_dbgu(void)
{
	at91_dbgu_hw_init();
	usart_init(BAUDRATE(MASTER_CLOCK, 115200));
}

#ifdef CONFIG_LPDDR1
static void ddramc_reg_config(struct ddramc_register *ddramc_config)
{
	ddramc_config->mdr = (AT91C_DDRC2_DBW_32_BITS
				| AT91C_DDRC2_MD_LP_DDR_SDRAM);

	ddramc_config->cr = (AT91C_DDRC2_NC_DDR9_SDR8
				| AT91C_DDRC2_NR_13
				| AT91C_DDRC2_CAS_3
				| AT91C_DDRC2_DISABLE_RESET_DLL /* DLL not reset */
				| AT91C_DDRC2_DQMS_NOT_SHARED
				| AT91C_DDRC2_ENRDM_DISABLE
				| AT91C_DDRC2_NB_BANKS_4
				| AT91C_DDRC2_NDQS_DISABLED
				| AT91C_DDRC2_UNAL_SUPPORTED
				| AT91C_DDRC2_DECOD_INTERLEAVED /* Interleaved decoding */
				| AT91C_DDRC2_OCD_EXIT	/* OCD(0) */
				);   /* Unaligned access is NOT supported */

	/* Timing for MT46H16M32LF (5 & 6) */
	/*
	 * The SDRAM device requires a refresh of all rows at least every 64ms.
	 * ((64ms) / 8192) * 132 MHz = 1031 i.e. 0x407
	 */
	ddramc_config->rtr = 0x407;     /* Refresh timer: 64 ms */

	/* One clock cycle @ 132 MHz = 7.5758 ns */
	ddramc_config->t0pr = (AT91C_DDRC2_TRAS_(6)	/* 6 * 7.5758 = 45 ns */
			| AT91C_DDRC2_TRCD_(3)		/* 3 * 7.5758 >= 18 ns */
			| AT91C_DDRC2_TWR_(2)		/* 2 * 7.5758 >= 15 ns */
			| AT91C_DDRC2_TRC_(8)		/* 8 * 7.5758 >= 75 ns */
			| AT91C_DDRC2_TRP_(3)		/* 3 * 7.5758 >= 18 ns */
			| AT91C_DDRC2_TRRD_(2)		/* 2 * 7.5758 >= 15 ns */
			| AT91C_DDRC2_TWTR_(2)		/* 2 clock cycles min */
			| AT91C_DDRC2_TMRD_(2));	/* 2 clock cycles */

	ddramc_config->t1pr = (AT91C_DDRC2_TXP_(2)	/* 2 clock cycles */
			| AT91C_DDRC2_TXSRD_(16)	/* 16 * 7.5758 >= 121.2 ns */
			| AT91C_DDRC2_TXSNR_(16)	/* 16 * 7.5758 >= 121.2 ns */
			| AT91C_DDRC2_TRFC_(10));	/* 10 * 7.5758 >= 75 ns */

	ddramc_config->t2pr = AT91C_DDRC2_TRTP_(4);

	ddramc_config->lpr = (AT91C_DDRC2_LPCB_DISABLED
			| AT91C_DDRC2_CLK_FR
			| AT91C_DDRC2_PASR_(0)
			| AT91C_DDRC2_DS_(1)
			| AT91C_DDRC2_TIMEOUT_0
			| AT91C_DDRC2_ADPE_FAST
			| AT91C_DDRC2_UPD_MR_NO_UPDATE);
}

static void ddramc_init(void)
{
	struct ddramc_register ddramc_reg;
	unsigned int reg;

	memset(&ddramc_reg, 0, sizeof(ddramc_reg));

	/* Setup the parameters we'll pass to the register later */
	ddramc_reg_config(&ddramc_reg);

	/* For lpddr1, DQ and DQS input buffers must always be set on */
	reg = readl(AT91C_BASE_SFR + SFR_DDRCFG);
	reg |= AT91C_DDRCFG_FDQIEN | AT91C_DDRCFG_FDQSIEN;
	writel(reg, AT91C_BASE_SFR + SFR_DDRCFG);

	/* enable ddr2 clock */
	pmc_enable_periph_clock(AT91C_ID_MPDDRC, PMC_PERIPH_CLK_DIVIDER_NA);
	pmc_enable_system_clock(AT91C_PMC_DDR);

	/* Init the special register for sama5d3x */
	/* MPDDRC DLL Slave Offset Register: DDR2 configuration */
	reg = AT91C_MPDDRC_S0OFF_1
		| AT91C_MPDDRC_S2OFF_1
		| AT91C_MPDDRC_S3OFF_1;
	writel(reg, (AT91C_BASE_MPDDRC + MPDDRC_DLL_SOR));

	/* MPDDRC DLL Master Offset Register */
	/* write master + clk90 offset */
	reg = AT91C_MPDDRC_MOFF_7
		| AT91C_MPDDRC_CLK90OFF_31
		| AT91C_MPDDRC_SELOFF_ENABLED;
	writel(reg, AT91C_BASE_MPDDRC + MPDDRC_DLL_MOR);

	/* setting the LPR to 0 is done before the calibration stuff in the sam-ba code, lets do that */
	writel(0, (AT91C_BASE_MPDDRC + HDDRSDRC2_LPR));

	/* MPDDRC I/O Calibration Register */
	/* DDR2 RZQ = 48 Ohm */
	/* TZQIO = (133 * 10^6) * (20 * 10^-9) + 1 = 3.66 == 4 */
	reg = readl(AT91C_BASE_MPDDRC + MPDDRC_IO_CALIBR);
	reg &= ~AT91C_MPDDRC_RDIV;
	reg &= ~AT91C_MPDDRC_TZQIO;
	reg |= AT91C_MPDDRC_RDIV_DDR2_RZQ_66_7;
	reg |= AT91C_MPDDRC_TZQIO_4;
	writel(reg, AT91C_BASE_MPDDRC + MPDDRC_IO_CALIBR);

	reg = readl(AT91C_BASE_MPDDRC + MPDDRC_LPDDR2_HS);
	reg |= AT91C_DDRC2_EN_CALIB; /* Don't disturb the rest of the bits */
	writel(reg, AT91C_BASE_MPDDRC + MPDDRC_LPDDR2_HS);

	/* DDRAM Controller initialize */
	lpddr1_sdram_initialize(AT91C_BASE_MPDDRC, AT91C_BASE_DDRCS, &ddramc_reg);

	reg = readl(AT91C_BASE_SFR + SFR_DDRCFG);
	reg &= ~(AT91C_DDRCFG_FDQIEN | AT91C_DDRCFG_FDQSIEN);
	writel(reg, AT91C_BASE_SFR + SFR_DDRCFG);
}
#endif /* #ifdef CONFIG_LPDDR1 */

#ifdef CONFIG_USER_HW_INIT
/*
 * Special setting for PM.
 * Since for the chips with no EMAC or GMAC, No actions is done to make
 * its phy to enter the power save mode when linux system enter suspend
 * to memory or standby.
 * And it causes the VDDCORE current is higher than our expection.
 * So set GMAC clock related pins GTXCK(PB8), GRXCK(PB11), GMDCK(PB16),
 * G125CK(PB18) and EMAC clock related pins EREFCK(PC7), EMDC(PC8)
 * to Pullup and Pulldown disabled, and output low.
 */

#define GMAC_PINS	((0x01 << 8) | (0x01 << 11) \
				| (0x01 << 16) | (0x01 << 18))

#define EMAC_PINS	((0x01 << 7) | (0x01 << 8))

static void at91_special_pio_output_low(void)
{
	unsigned int base;
	unsigned int value;

	base = AT91C_BASE_PIOB;
	value = GMAC_PINS;

	pmc_enable_periph_clock(AT91C_ID_PIOB, PMC_PERIPH_CLK_DIVIDER_NA);

	writel(value, base + PIO_REG_PPUDR);	/* PIO_PPUDR */
	writel(value, base + PIO_REG_PPDDR);	/* PIO_PPDDR */
	writel(value, base + PIO_REG_PER);	/* PIO_PER */
	writel(value, base + PIO_REG_OER);	/* PIO_OER */
	writel(value, base + PIO_REG_CODR);	/* PIO_CODR */

	base = AT91C_BASE_PIOC;
	value = EMAC_PINS;

	pmc_enable_periph_clock(AT91C_ID_PIOC, PMC_PERIPH_CLK_DIVIDER_NA);

	writel(value, base + PIO_REG_PPUDR);	/* PIO_PPUDR */
	writel(value, base + PIO_REG_PPDDR);	/* PIO_PPDDR */
	writel(value, base + PIO_REG_PER);	/* PIO_PER */
	writel(value, base + PIO_REG_OER);	/* PIO_OER */
	writel(value, base + PIO_REG_CODR);	/* PIO_CODR */
}
#endif

#ifdef CONFIG_USER_HW_INIT
static void gpio_init(void)
{
	/* Setup WB50NBT custom GPIOs to match BB40 */
	const struct pio_desc gpios[] = {
		/*  {"NAME",        PIN NUMBER,     VALUE, ATTRIBUTES, TYPE },*/

		/* GPIOs
		 * The LED GPIO pins go to a header && through a buffer to LEDs,
		 *     active low to light LED
		 * BOOT1/BOOT2 are odd, go to header to short a PU to gnd
		 * GPIO_4, connects to CON37 and also direct-drives D10 through
		 *     a 1k to GND
		 * IRQ needs to NOT have a PU on it. There is an error on the
		 *     BB40 where it's pulled up to 1.8v.
		 */
		{"GPIO_5",	AT91C_PIN_PA(0),	0, PIO_DEFAULT, PIO_INPUT},  /* BOOT1, has pullup on BB40 */
		{"GPIO_6",	AT91C_PIN_PA(3),	0, PIO_DEFAULT, PIO_INPUT},  /* BOOT2, has pullup on BB40 */
		{"GPIO_3",	AT91C_PIN_PA(10),	0, PIO_PULLUP, PIO_INPUT},  /* GPIO2 */
		{"LED0",	AT91C_PIN_PA(12),	0, PIO_DEFAULT, PIO_OUTPUT},
		{"GPIO_4",	AT91C_PIN_PA(14),	0, PIO_DEFAULT, PIO_OUTPUT},  /* GPIO3 */
		{"STAT0",	AT91C_PIN_PA(22),	0, PIO_DEFAULT, PIO_OUTPUT},
		{"LED1",	AT91C_PIN_PA(24),	0, PIO_DEFAULT, PIO_OUTPUT},
		{"LED2",	AT91C_PIN_PA(26),	0, PIO_DEFAULT, PIO_OUTPUT},
		{"STAT1",	AT91C_PIN_PA(28),	0, PIO_DEFAULT, PIO_OUTPUT},
		{"IRQ",	        AT91C_PIN_PE(31),	0, PIO_DEFAULT,  PIO_INPUT},

		/* USB */
		{"VBUS_EN",	AT91C_PIN_PA(2),	0, PIO_DEFAULT, PIO_OUTPUT},
		{"OVER_CUR",	AT91C_PIN_PA(4),	0, PIO_DEFAULT, PIO_INPUT},  /* pullup on BB40 */
		{"VBUS_SENSE",	AT91C_PIN_PB(13),	0, PIO_PULLUP, PIO_INPUT},

		/* UART0 pins */
		{"UART0_DCD",	AT91C_PIN_PD(7),	0, PIO_DEFAULT, PIO_INPUT}, /* BB40 with a PU */
		{"UART0_RI",	AT91C_PIN_PD(8),	0, PIO_DEFAULT, PIO_INPUT}, /* BB40 with a PU */
		{"UART0_DSR",	AT91C_PIN_PD(11),	0, PIO_DEFAULT, PIO_INPUT}, /* BB40 with a PU */
		{"UART0_DTR",	AT91C_PIN_PD(13),	0, PIO_DEFAULT, PIO_OUTPUT},

		/* WiFi and BT SIP */
		{"WAKE_ON_WLAN",AT91C_PIN_PC(31),	0, PIO_DEFAULT, PIO_INPUT}, /* WB50 has a PU on this line */
		{"WLAN_PWD_L",	AT91C_PIN_PE(3),	0, PIO_DEFAULT, PIO_OUTPUT}, /* WB50 has a PU on this line, set in reset by default */
		{"BT_PWD_L",	AT91C_PIN_PE(5),	0, PIO_DEFAULT, PIO_OUTPUT}, /* Leave in reset by default */
		{"BT_WAKEUP_HOST",AT91C_PIN_PE(10),	0, PIO_DEFAULT, PIO_OUTPUT}, /* WB50 schematic in error. This is an output, and it should always stay low */
		{"WIFI_GPIO10",	AT91C_PIN_PE(13),	0, PIO_DEFAULT, PIO_INPUT}, /* WB50 has PU on this */

		{(char *)0,	0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	/* Configure the PIO controller */
	pmc_enable_periph_clock(AT91C_ID_PIOA, PMC_PERIPH_CLK_DIVIDER_NA);
	pmc_enable_periph_clock(AT91C_ID_PIOB, PMC_PERIPH_CLK_DIVIDER_NA);
	pmc_enable_periph_clock(AT91C_ID_PIOC, PMC_PERIPH_CLK_DIVIDER_NA);
	pmc_enable_periph_clock(AT91C_ID_PIOD, PMC_PERIPH_CLK_DIVIDER_NA);
	pmc_enable_periph_clock(AT91C_ID_PIOE, PMC_PERIPH_CLK_DIVIDER_NA);
	pio_configure(gpios);
}
#endif

#if defined(CONFIG_PM)
void at91_disable_smd_clock(void)
{
	/*
	 * set pin DIBP to pull-up and DIBN to pull-down
	 * to save power on VDDIOP0
	 */
	pmc_enable_system_clock(AT91C_PMC_SMDCK);
	pmc_set_smd_clock_divider(AT91C_PMC_SMDDIV);
	pmc_enable_periph_clock(AT91C_ID_SMD, PMC_PERIPH_CLK_DIVIDER_NA);
	writel(0xF, (0x0C + AT91C_BASE_SMD));
	pmc_disable_periph_clock(AT91C_ID_SMD);
	pmc_disable_system_clock(AT91C_PMC_SMDCK);
}
#endif

#ifdef CONFIG_HW_INIT
void hw_init(void)
{
	/* Disable watchdog */
	at91_disable_wdt();

	/*
	 * At this stage the main oscillator
	 * is supposed to be enabled PCK = MCK = MOSC
	 */

	/* Configure PLLA = MOSC * (PLL_MULA + 1) / PLL_DIVA */
	pmc_cfg_plla(PLLA_SETTINGS);

	/* Initialize PLLA charge pump */
	pmc_init_pll(AT91C_PMC_IPLLA_3);

	/* Switch PCK/MCK on Main clock output */
	pmc_mck_cfg_set(0, BOARD_PRESCALER_MAIN_CLOCK,
			AT91C_PMC_MDIV | AT91C_PMC_CSS);

	/* Switch PCK/MCK on PLLA output */
	pmc_mck_cfg_set(0, BOARD_PRESCALER_PLLA,
			AT91C_PMC_MDIV | AT91C_PMC_CSS);

#ifdef CONFIG_USER_HW_INIT
	/* Set GMAC & EMAC pins to output low */
	at91_special_pio_output_low();
#endif

	/* Init timer */
	timer_init();

#ifdef CONFIG_SCLK
	slowclk_enable_osc32();
#endif

	/* initialize the dbgu */
	initialize_dbgu();

#ifdef CONFIG_LPDDR1
	/* Initialize MPDDR Controller */
	ddramc_init();
#endif

#ifdef CONFIG_USER_HW_INIT
	gpio_init();
#endif

}
#endif /* #ifdef CONFIG_HW_INIT */

#ifdef CONFIG_SDCARD
#ifdef CONFIG_OF_LIBFDT
void at91_board_set_dtb_name(char *of_name)
{
	strcpy(of_name, "wb50n.dtb");
}
#endif

void at91_mci0_hw_init(void)
{
	const struct pio_desc mci_pins[] = {
		{"MCCK", AT91C_PIN_PD(9), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"MCCDA", AT91C_PIN_PD(0), 0, PIO_DEFAULT, PIO_PERIPH_A},

		{"MCDA0", AT91C_PIN_PD(1), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"MCDA1", AT91C_PIN_PD(2), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"MCDA2", AT91C_PIN_PD(3), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"MCDA3", AT91C_PIN_PD(4), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"MCDA4", AT91C_PIN_PD(5), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"MCDA5", AT91C_PIN_PD(6), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"MCDA6", AT91C_PIN_PD(7), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"MCDA7", AT91C_PIN_PD(8), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	/* Configure the PIO controller */
	pmc_enable_periph_clock(AT91C_ID_PIOD, PMC_PERIPH_CLK_DIVIDER_NA);
	pio_configure(mci_pins);

	/* Enable the clock */
	pmc_enable_periph_clock(AT91C_ID_HSMCI0, PMC_PERIPH_CLK_DIVIDER_NA);
}
#endif /* #ifdef CONFIG_SDCARD */

#ifdef CONFIG_NANDFLASH
void nandflash_hw_init(void)
{
	/* Configure nand pins */
	const struct pio_desc nand_pins[] = {
		{"NANDALE", AT91C_PIN_PE(21), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"NANDCLE", AT91C_PIN_PE(22), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"NWP", AT91C_PIN_PE(14), 0, PIO_PULLUP, PIO_OUTPUT},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	/* Configure the nand controller pins*/
	pmc_enable_periph_clock(AT91C_ID_PIOE, PMC_PERIPH_CLK_DIVIDER_NA);
	pio_configure(nand_pins);

	/* Enable the clock */
	pmc_enable_periph_clock(AT91C_ID_SMC, PMC_PERIPH_CLK_DIVIDER_NA);

	/* Configure SMC CS3 for NAND/SmartMedia */
	/* Values below are for 132 MHz clock */
	writel(AT91C_SMC_SETUP_NWE(1)
		| AT91C_SMC_SETUP_NCS_WR(0)
		| AT91C_SMC_SETUP_NRD(0)
		| AT91C_SMC_SETUP_NCS_RD(0),
		(ATMEL_BASE_SMC + SMC_SETUP3));

	writel(AT91C_SMC_PULSE_NWE(3)
		| AT91C_SMC_PULSE_NCS_WR(6)
		| AT91C_SMC_PULSE_NRD(3)
		| AT91C_SMC_PULSE_NCS_RD(6),
		(ATMEL_BASE_SMC + SMC_PULSE3));

	writel(AT91C_SMC_CYCLE_NWE(6)
		| AT91C_SMC_CYCLE_NRD(6),
		(ATMEL_BASE_SMC + SMC_CYCLE3));

	writel(AT91C_SMC_TIMINGS_TCLR(2)
		| AT91C_SMC_TIMINGS_TADL(8)
		| AT91C_SMC_TIMINGS_TAR(2)
		| AT91C_SMC_TIMINGS_TRR(3)
		| AT91C_SMC_TIMINGS_TWB(8)
		| AT91C_SMC_TIMINGS_RBNSEL(3)
		| AT91C_SMC_TIMINGS_NFSEL,
		(ATMEL_BASE_SMC + SMC_TIMINGS3));

	writel(AT91C_SMC_MODE_READMODE_NRD_CTRL
		| AT91C_SMC_MODE_WRITEMODE_NWE_CTRL
		| AT91C_SMC_MODE_EXNWMODE_DISABLED
		| AT91C_SMC_MODE_DBW_8
		| AT91C_SMC_MODE_TDF_MODE
		| AT91C_SMC_MODE_TDF_CYCLES(12),
		(ATMEL_BASE_SMC + SMC_MODE3));
}
#endif /* #ifdef CONFIG_NANDFLASH */
