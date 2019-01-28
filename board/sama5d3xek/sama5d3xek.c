/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support
 * ----------------------------------------------------------------------------
 * Copyright (c) 2012, Atmel Corporation

 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "common.h"
#include "hardware.h"
#include "pmc.h"
#include "usart.h"
#include "debug.h"
#include "ddramc.h"
#include "spi.h"
#include "gpio.h"
#include "slowclk.h"
#include "timer.h"
#include "watchdog.h"
#include "string.h"
#include "board_hw_info.h"

#include "arch/at91_pmc.h"
#include "arch/at91_rstc.h"
#include "arch/sama5_smc.h"
#include "arch/at91_pio.h"
#include "arch/at91_ddrsdrc.h"
#include "arch/at91_sfr.h"
#include "sama5d3xek.h"

#define LED0 (12)
#define LED1 (24)
#define LED2 (26)  // Using LED2 for SDA
#define LED3 (22)
#define LED4 (28)
#define BITOUT (10)

#define LED_ON (x) 	pio_set_value( (x), 0);
#define LED_OFF (x) 	pio_set_value( (x), 1);

#ifdef CONFIG_USER_HW_INIT
extern void hw_init_hook(void);
#endif

static void at91_dbgu_hw_init(void)
{
	/* Configure DBGU pin */
	const struct pio_desc dbgu_pins[] = {
		{"RXD", AT91C_PIN_PB(30), 0, PIO_PULLUP, PIO_PERIPH_A},
		{"TXD", AT91C_PIN_PB(31), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	/*  Configure the dbgu pins */
	pmc_enable_periph_clock(AT91C_ID_PIOB);
	pio_configure(dbgu_pins);

	/* Enable clock */
	pmc_enable_periph_clock(AT91C_ID_DBGU);
}

static void initialize_dbgu(void)
{
	at91_dbgu_hw_init();
	usart_init(BAUDRATE(MASTER_CLOCK, 115200));
}

#ifdef CONFIG_DDR2 /* This is defined */
static void ddramc_reg_config(struct ddramc_register *ddramc_config)
{
	ddramc_config->mdr = (AT91C_DDRC2_DBW_32_BITS
				| AT91C_DDRC2_MD_LP_DDR_SDRAM);

	ddramc_config->cr = (AT91C_DDRC2_NC_DDR9_SDR8
				| AT91C_DDRC2_NR_13
				| AT91C_DDRC2_CAS_3
				| AT91C_DDRC2_DLL_RESET_DISABLED /* DLL not reset */
				| AT91C_DDRC2_DQMS_NOT_SHARED
				| AT91C_DDRC2_ENRDM_DISABLE
				| AT91C_DDRC2_NB_BANKS_4
				| AT91C_DDRC2_NDQS_DISABLED
				| AT91C_DDRC2_UNAL_SUPPORTED
				| AT91C_DDRC2_DECOD_INTERLEAVED /* Interleaved decoding */
				| AT91C_DDRC2_OCD_EXIT	/* OCD(0) */
				);   /* Unaligned access is NOT supported */

#if defined(CONFIG_BUS_SPEED_133MHZ)  /* This is defined */
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

	ddramc_config->t1pr = (AT91C_DDRC2_TXP_(1)	/*  1 clock cycles */
			| AT91C_DDRC2_TXSRD_(0)	    /* 0 clock cycles */
			| AT91C_DDRC2_TXSNR_(15)	/* 15 * 7.5758 >= 112.5 ns */
			| AT91C_DDRC2_TRFC_(10));	/* 10 * 7.5758 >= 75 ns */

	ddramc_config->t2pr = AT91C_DDRC2_TRTP_(4);

	ddramc_config->lpr = (AT91C_DDRC2_LPCB_DISABLED
			| AT91C_DDRC2_CLK_FR
			| AT91C_DDRC2_LPDDR2_PWOFF_DISABLED
			| AT91C_DDRC2_PASR_(0)
			| AT91C_DDRC2_DS_(1)
			| AT91C_DDRC2_TIMEOUT_0
			| AT91C_DDRC2_ADPE_FAST
			| AT91C_DDRC2_UPD_MR_(0));

#elif defined(CONFIG_BUS_SPEED_148MHZ)

	ddramc_config->rtr = 0x486;     /* Refresh timer: 7.8125us */

	/* One clock cycle @ 148 MHz = 6.7 ns */
	ddramc_config->t0pr = (AT91C_DDRC2_TRAS_(7)
			| AT91C_DDRC2_TRCD_(3)
			| AT91C_DDRC2_TWR_(3)
			| AT91C_DDRC2_TRC_(9)
			| AT91C_DDRC2_TRP_(3)
			| AT91C_DDRC2_TRRD_(2)
			| AT91C_DDRC2_TWTR_(2)
			| AT91C_DDRC2_TMRD_(2));

	ddramc_config->t1pr = (AT91C_DDRC2_TXP_(2)
			| AT91C_DDRC2_TXSRD_(200)
			| AT91C_DDRC2_TXSNR_(31)
			| AT91C_DDRC2_TRFC_(30));

	ddramc_config->t2pr = (AT91C_DDRC2_TFAW_(7)
			| AT91C_DDRC2_TRTP_(2)
			| AT91C_DDRC2_TRPA_(3)
			| AT91C_DDRC2_TXARDS_(8)
			| AT91C_DDRC2_TXARD_(8));

#elif defined(CONFIG_BUS_SPEED_166MHZ)
	/*
	 * The DDR2-SDRAM device requires a refresh of all rows every 64ms.
	 * ((64ms) / 8192) * 166MHz = 1296 i.e. 0x510
	 */
	ddramc_config->rtr = 0x500;

	/* One clock cycle @ 166 MHz = 6.0 ns */
	ddramc_config->t0pr = (AT91C_DDRC2_TRAS_(8)	/* 8 * 6 = 48 ns */
			| AT91C_DDRC2_TRCD_(3)		/* 3 * 6 = 18 ns */
			| AT91C_DDRC2_TWR_(3)		/* 3 * 6 = 18 ns */
			| AT91C_DDRC2_TRC_(10)		/* 10 * 6 = 60 ns */
			| AT91C_DDRC2_TRP_(3)		/* 3 * 6 = 18 ns */
			| AT91C_DDRC2_TRRD_(2)		/* 2 * 6 = 12 ns */
			| AT91C_DDRC2_TWTR_(2)		/* 2 clock cycles*/
			| AT91C_DDRC2_TMRD_(2));	/* 2 clock cycles at least */

	ddramc_config->t1pr = (AT91C_DDRC2_TXP_(3)	/* 3 * 6 = 18ns, 2 clock cycles a least */
			| AT91C_DDRC2_TXSRD_(202)	/* 202 clock cycles: Exit self refresh delay to Read command */
			| AT91C_DDRC2_TXSNR_(35)	/* 35 * 6 = 210 ns*/
			| AT91C_DDRC2_TRFC_(31));	/* 31 * 6 = 186 ns */

	ddramc_config->t2pr = (AT91C_DDRC2_TFAW_(8)	/* 45 ns for 16bit * 8 bank */
			| AT91C_DDRC2_TRTP_(2)		/* 2 * 6 = 15ns clock cycles min */
			| AT91C_DDRC2_TRPA_(3)		/* 15 ns */
			| AT91C_DDRC2_TXARDS_(10)	/* 7 ~ 10 clock cycles */
			| AT91C_DDRC2_TXARD_(3));	/* 2 ~ 3 clock cycles */

#else
#error "No bus clock provided!"
#endif
}

static void ddramc_init(void)
{
	struct ddramc_register ddramc_reg;
	unsigned int reg;
	unsigned int old_io_cal_reg;

	memset(&ddramc_reg, 0, sizeof(ddramc_reg));

	/* Setup the parameters we'll pass to the register later */
	ddramc_reg_config(&ddramc_reg);

	/* For lpddr1, DQ and DQS input buffers must always be set on */
	writel( SFR_DDRCFG_FDQIEN | SFR_DDRCFG_FDQSIEN, (AT91C_BASE_SFR + SFR_DDRCFG));

	/* enable ddr2 clock */
	pmc_enable_periph_clock(AT91C_ID_MPDDRC);
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
	writel(reg, (AT91C_BASE_MPDDRC + MPDDRC_DLL_MOR));

	/* setting the LPR to 0 is done before the calibration stuff in the sam-ba code, lets do that */
	writel(0, (AT91C_BASE_MPDDRC + HDDRSDRC2_LPR));

	/* TODO: verify right values of calibration?
	 * And what do we do with said calibration?
	 */
	/* MPDDRC I/O Calibration Register */
	/* DDR2 RZQ = 48 Ohm */
	/* TZQIO = (133 * 10^6) * (20 * 10^-9) + 1 = 3.66 == 4 */
	reg = readl(AT91C_BASE_MPDDRC + MPDDRC_IO_CALIBR);
	old_io_cal_reg = reg;
	reg &= ~AT91C_MPDDRC_RDIV;
	reg &= ~AT91C_MPDDRC_TZQIO;
	reg |= AT91C_MPDDRC_RDIV_DDR2_RZQ_66_7;
	reg |= AT91C_MPDDRC_TZQIO_4;
	writel(reg, (AT91C_BASE_MPDDRC + MPDDRC_IO_CALIBR));

	reg = readl(AT91C_BASE_MPDDRC + MPDDRC_HS);
	reg |= AT91C_DDRC2_EN_CALIB; /* Don't disturb the rest of the bits */
	writel(reg, (AT91C_BASE_MPDDRC + MPDDRC_HS));

	/* DDRAM2 Controller initialize */
	lpddram1_initialize(AT91C_BASE_MPDDRC, AT91C_BASE_DDRCS, &ddramc_reg);

	dbg_very_loud("Orig value MPDDRC_IO_CALBR: %d\n", old_io_cal_reg);
	ddramc_print_config_regs(AT91C_BASE_MPDDRC);

	dbg_very_loud("End ddram init\n");

}

#else
#error "No right DDR-SDRAM device type provided"
#endif /* #ifdef CONFIG_DDR2 */

static void one_wire_hw_init(void)
{
	const struct pio_desc one_wire_pio[] = {
		{"1-Wire", AT91C_PIN_PE(25), 1, PIO_DEFAULT, PIO_OUTPUT},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	pmc_enable_periph_clock(AT91C_ID_PIOE);
	pio_configure(one_wire_pio);
}

#if defined(CONFIG_NANDFLASH_RECOVERY) || defined(CONFIG_DATAFLASH_RECOVERY)
static void recovery_buttons_hw_init(void)
{
	/* Configure recovery button PINs */
	const struct pio_desc recovery_button_pins[] = {
		{"RECOVERY_BUTTON", CONFIG_SYS_RECOVERY_BUTTON_PIN, 0, PIO_PULLUP, PIO_INPUT},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	pmc_enable_periph_clock(AT91C_ID_PIOE);
	pio_configure(recovery_button_pins);
}
#endif /* #if defined(CONFIG_NANDFLASH_RECOVERY) || defined(CONFIG_DATAFLASH_RECOVERY) */

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

	writel((1 << AT91C_ID_PIOB), (PMC_PCER + AT91C_BASE_PMC));

	writel(value, base + PIO_REG_PPUDR);	/* PIO_PPUDR */
	writel(value, base + PIO_REG_PPDDR);	/* PIO_PPDDR */
	writel(value, base + PIO_REG_PER);	/* PIO_PER */
	writel(value, base + PIO_REG_OER);	/* PIO_OER */
	writel(value, base + PIO_REG_CODR);	/* PIO_CODR */

	base = AT91C_BASE_PIOC;
	value = EMAC_PINS;

	writel((1 << AT91C_ID_PIOC), (PMC_PCER + AT91C_BASE_PMC));

	writel(value, base + PIO_REG_PPUDR);	/* PIO_PPUDR */
	writel(value, base + PIO_REG_PPDDR);	/* PIO_PPDDR */
	writel(value, base + PIO_REG_PER);	/* PIO_PER */
	writel(value, base + PIO_REG_OER);	/* PIO_OER */
	writel(value, base + PIO_REG_CODR);	/* PIO_CODR */
}

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
	pmc_cfg_plla(PLLA_SETTINGS, PLL_LOCK_TIMEOUT);

	/* Initialize PLLA charge pump */
	pmc_init_pll(AT91C_PMC_IPLLA_3);

	/* Switch PCK/MCK on Main clock output */
	pmc_cfg_mck(BOARD_PRESCALER_MAIN_CLOCK, PLL_LOCK_TIMEOUT);

	/* Switch PCK/MCK on PLLA output */
	pmc_cfg_mck(BOARD_PRESCALER_PLLA, PLL_LOCK_TIMEOUT);

	/* Set GMAC & EMAC pins to output low */
	at91_special_pio_output_low();

	/* Init timer */
	timer_init();

#ifdef CONFIG_SCLK
	slowclk_enable_osc32();
#endif

	/* initialize the dbgu */
	initialize_dbgu();

	/* Initialize MPDDR Controller */
#ifdef CONFIG_DDR2
	ddramc_init();
#elif defined(CONFIG_LPDDR2)
#error lpddr2
	lpddr2_init();
#endif
	/* load one wire information */
	one_wire_hw_init();

#ifdef CONFIG_USER_HW_INIT
	hw_init_hook();
#endif

#if defined(CONFIG_NANDFLASH_RECOVERY) || defined(CONFIG_DATAFLASH_RECOVERY)
	/* Init the recovery buttons pins */
	recovery_buttons_hw_init();
#endif
}
#endif /* #ifdef CONFIG_HW_INIT */

#ifdef CONFIG_DATAFLASH
void at91_spi0_hw_init(void)
{
	/* Configure PIN for SPI0 */
	const struct pio_desc spi0_pins[] = {
		{"MISO",	AT91C_PIN_PD(10), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"MOSI",	AT91C_PIN_PD(11), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"SPCK",	AT91C_PIN_PD(12), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"NPCS",	CONFIG_SYS_SPI_PCS, 1, PIO_DEFAULT, PIO_OUTPUT},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	/* Configure the PIO controller */
	pmc_enable_periph_clock(AT91C_ID_PIOD);
	pio_configure(spi0_pins);

	/* Enable the clock */
	pmc_enable_periph_clock(AT91C_ID_SPI0);
}
#endif /* #ifdef CONFIG_DATAFLASH */

#ifdef CONFIG_SDCARD
static void sdcard_set_of_name_board(char *of_name)
{
	/* CPU TYPE*/
	switch (get_cm_sn()) {
	case BOARD_ID_SAMA5D31_CM:
		strcpy(of_name, "sama5d31ek");
		break;

	case BOARD_ID_SAMA5D33_CM:
		strcpy(of_name, "sama5d33ek");
		break;

	case BOARD_ID_SAMA5D34_CM:
		strcpy(of_name, "sama5d34ek");
		break;

	case BOARD_ID_SAMA5D35_CM:
		strcpy(of_name, "sama5d35ek");
		break;

	case BOARD_ID_SAMA5D36_CM:
		strcpy(of_name, "sama5d36ek");
		break;

	default:
		dbg_info("WARNING: Not correct CPU board ID\n");
		break;
	}

	if (get_dm_sn() == BOARD_ID_PDA_DM)
		strcat(of_name, "_pda");

	strcat(of_name, ".dtb");
}

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
	pmc_enable_periph_clock(AT91C_ID_HSMCI0);
	pio_configure(mci_pins);

	/* Enable the clock */
	pmc_enable_periph_clock(AT91C_ID_HSMCI0);

	/* Set of name function pointer */
	sdcard_set_of_name = &sdcard_set_of_name_board;
}
#endif /* #ifdef CONFIG_SDCARD */

#ifdef CONFIG_FLASH
void norflash_hw_init(void)
{
	const struct pio_desc flash_pins[] = {
		{"FLASH_A1",  AT91C_PIN_PE(1),  0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A2",  AT91C_PIN_PE(2),  0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A3",  AT91C_PIN_PE(3),  0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A4",  AT91C_PIN_PE(4),  0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A5",  AT91C_PIN_PE(5),  0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A6",  AT91C_PIN_PE(6),  0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A7",  AT91C_PIN_PE(7),  0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A8",  AT91C_PIN_PE(8),  0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A9",  AT91C_PIN_PE(9),  0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A10", AT91C_PIN_PE(10), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A11", AT91C_PIN_PE(11), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A12", AT91C_PIN_PE(12), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A13", AT91C_PIN_PE(13), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A14", AT91C_PIN_PE(14), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A15", AT91C_PIN_PE(15), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A16", AT91C_PIN_PE(16), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A17", AT91C_PIN_PE(17), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A18", AT91C_PIN_PE(18), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A19", AT91C_PIN_PE(19), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A20", AT91C_PIN_PE(20), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A21", AT91C_PIN_PE(21), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A22", AT91C_PIN_PE(22), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_A23", AT91C_PIN_PE(23), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{"FLASH_CS0", AT91C_PIN_PE(26), 0, PIO_DEFAULT, PIO_PERIPH_A},
		{(char *)0, 0, 0, PIO_DEFAULT, PIO_PERIPH_A},
	};

	/* Enable the clock */
	pmc_enable_periph_clock(AT91C_ID_SMC);

	/* Configure SMC CS0 for NOR flash */
	writel(AT91C_SMC_SETUP_NWE(1)
		| AT91C_SMC_SETUP_NCS_WR(0)
		| AT91C_SMC_SETUP_NRD(2)
		| AT91C_SMC_SETUP_NCS_RD(0),
		(ATMEL_BASE_SMC + SMC_SETUP0));

	writel(AT91C_SMC_PULSE_NWE(10)
		| AT91C_SMC_PULSE_NCS_WR(11)
		| AT91C_SMC_PULSE_NRD(10)
		| AT91C_SMC_PULSE_NCS_RD(11),
		(ATMEL_BASE_SMC + SMC_PULSE0));

	writel(AT91C_SMC_CYCLE_NWE(11)
		| AT91C_SMC_CYCLE_NRD(14),
		(ATMEL_BASE_SMC + SMC_CYCLE0));

	writel(AT91C_SMC_TIMINGS_TCLR(0)
		| AT91C_SMC_TIMINGS_TADL(0)
		| AT91C_SMC_TIMINGS_TAR(0)
		| AT91C_SMC_TIMINGS_TRR(0)
		| AT91C_SMC_TIMINGS_TWB(0)
		| AT91C_SMC_TIMINGS_RBNSEL(0)
		| AT91C_SMC_TIMINGS_NFSEL,
		(ATMEL_BASE_SMC + SMC_TIMINGS0));

	writel(AT91C_SMC_MODE_READMODE_NRD_CTRL
		| AT91C_SMC_MODE_WRITEMODE_NWE_CTRL
		| AT91C_SMC_MODE_EXNWMODE_DISABLED
		| AT91C_SMC_MODE_DBW_16
		| AT91C_SMC_MODE_TDF_CYCLES(1),
		(ATMEL_BASE_SMC + SMC_MODE0));

	/* Configure the PIO controller. */
	pio_configure(flash_pins);
}
#endif /* #ifdef CONFIG_FLASH */

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
	pmc_enable_periph_clock(AT91C_ID_PIOE);
	pio_configure(nand_pins);

	/* Enable the clock */
	pmc_enable_periph_clock(AT91C_ID_SMC);

	/* Configure SMC CS3 for NAND/SmartMedia */
	writel(AT91C_SMC_SETUP_NWE(1)
		| AT91C_SMC_SETUP_NCS_WR(1) 
		| AT91C_SMC_SETUP_NRD(2) 
		| AT91C_SMC_SETUP_NCS_RD(1), 
		(ATMEL_BASE_SMC + SMC_SETUP3));

	writel(AT91C_SMC_PULSE_NWE(5)
		| AT91C_SMC_PULSE_NCS_WR(7)
		| AT91C_SMC_PULSE_NRD(5)
		| AT91C_SMC_PULSE_NCS_RD(7), 
	 	(ATMEL_BASE_SMC + SMC_PULSE3));

	writel(AT91C_SMC_CYCLE_NWE(8)
		| AT91C_SMC_CYCLE_NRD(9), 
		(ATMEL_BASE_SMC + SMC_CYCLE3));

	writel(AT91C_SMC_TIMINGS_TCLR(3)
		| AT91C_SMC_TIMINGS_TADL(10)
		| AT91C_SMC_TIMINGS_TAR(3)
		| AT91C_SMC_TIMINGS_TRR(4)
		| AT91C_SMC_TIMINGS_TWB(5)
		| AT91C_SMC_TIMINGS_RBNSEL(3)
		| AT91C_SMC_TIMINGS_NFSEL,
		(ATMEL_BASE_SMC + SMC_TIMINGS3));

	writel(AT91C_SMC_MODE_READMODE_NRD_CTRL
		| AT91C_SMC_MODE_WRITEMODE_NWE_CTRL
		| AT91C_SMC_MODE_EXNWMODE_DISABLED
		| AT91C_SMC_MODE_DBW_8
		| AT91C_SMC_MODE_TDF_CYCLES(1),
		(ATMEL_BASE_SMC + SMC_MODE3));
}
#endif /* #ifdef CONFIG_NANDFLASH */
