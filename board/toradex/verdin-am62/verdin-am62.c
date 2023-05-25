// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Board specific initialization for Verdin AM62 SoM
 *
 * Copyright 2023 Toradex - https://www.toradex.com/
 *
 */

#include <asm/arch/hardware.h>
#include <asm/arch/sys_proto.h>
#include <asm/io.h>
#include <common.h>
#include <dm/uclass.h>
#include <env.h>
#include <fdt_support.h>
#include <i2c.h>
#include <init.h>
#include <k3-ddrss.h>
#include <power/tps65219.h>
#include <spl.h>

#include "../common/tdx-cfg-block.h"

DECLARE_GLOBAL_DATA_PTR;

#define PMIC_I2C_BUS		0x0
#define PMIC_I2C_ADDRESS	0x30
#define PMIC_BUCK1_VSET_850	0xa

int board_init(void)
{
	struct udevice *dev;
	u8 addr, data[1];
	int err;

	/* Set PMIC Buck1 aka +VDD_CORE to 850 mV */
	err = i2c_get_chip_for_busnum(PMIC_I2C_BUS, PMIC_I2C_ADDRESS, 1, &dev);
	if (err) {
		printf("%s: Cannot find PMIC I2C chip (err=%d)\n", __func__, err);
	} else {
		addr = TPS65219_BUCK1_VOUT_REG;
		err = dm_i2c_read(dev, addr, data, 1);
		if (err)
			debug("failed to get TPS65219_BUCK1_VOUT_REG (err=%d)\n", err);
		data[0] &= ~TPS65219_VOLT_MASK;
		data[0] |= PMIC_BUCK1_VSET_850;
		err = dm_i2c_write(dev, addr, data, 1);
		if (err)
			debug("failed to set TPS65219_BUCK1_VOUT_REG (err=%d)\n", err);
	}

	return 0;
}

int dram_init(void)
{
	gd->ram_size = get_ram_size((long *)CONFIG_SYS_SDRAM_BASE, CONFIG_SYS_SDRAM_SIZE);
	return 0;
}

#if defined(CONFIG_SPL_LOAD_FIT)
int board_fit_config_name_match(const char *name)
{
	return 0;
}
#endif

#if IS_ENABLED(CONFIG_OF_LIBFDT) && IS_ENABLED(CONFIG_OF_BOARD_SETUP)
int ft_board_setup(void *blob, struct bd_info *bd)
{
	return ft_common_board_setup(blob, bd);
}
#endif

static void select_dt_from_module_version(void)
{
	char variant[32];
	char *env_variant = env_get("variant");
	int is_wifi = 0;

	if (IS_ENABLED(CONFIG_TDX_CFG_BLOCK)) {
		/*
		 * If we have a valid config block and it says we are a module with
		 * Wi-Fi/Bluetooth make sure we use the -wifi device tree.
		 */
		is_wifi = (tdx_hw_tag.prodid == VERDIN_AM62Q_WIFI_BT_IT);
	}

	if (is_wifi)
		strlcpy(&variant[0], "wifi", sizeof(variant));
	else
		strlcpy(&variant[0], "nonwifi", sizeof(variant));

	if (strcmp(variant, env_variant)) {
		printf("Setting variant to %s\n", variant);
		env_set("variant", variant);
	}
}

int board_late_init(void)
{
	select_dt_from_module_version();

	return 0;
}

#define CTRLMMR_USB0_PHY_CTRL	0x43004008
#define CTRLMMR_USB1_PHY_CTRL	0x43004018
#define CORE_VOLTAGE		0x80000000

#ifdef CONFIG_SPL_BOARD_INIT
void spl_board_init(void)
{
	u32 val;

	/* Set USB0 PHY core voltage to 0.85V */
	val = readl(CTRLMMR_USB0_PHY_CTRL);
	val &= ~(CORE_VOLTAGE);
	writel(val, CTRLMMR_USB0_PHY_CTRL);

	/* Set USB1 PHY core voltage to 0.85V */
	val = readl(CTRLMMR_USB1_PHY_CTRL);
	val &= ~(CORE_VOLTAGE);
	writel(val, CTRLMMR_USB1_PHY_CTRL);

	/* We use the 32k FOUT from the Epson RX8130CE RTC chip */
	/* In WKUP_LFOSC0 clear the power down bit and set the bypass bit
	 * The bypass bit is required as we provide a CMOS clock signal and
	 * the power down seems to be required also in the bypass case
	 * despite of the datasheet stating otherwise
	 */
	/* Compare with the AM62 datasheet,
	 * Table 7-21. LFXOSC Modes of Operation
	 */
	val = readl(MCU_CTRL_LFXOSC_CTRL);
	val &= ~MCU_CTRL_LFXOSC_32K_DISABLE_VAL;
	val |= MCU_CTRL_LFXOSC_32K_BYPASS_VAL;
	writel(val, MCU_CTRL_LFXOSC_CTRL);
	/* Make sure to mux up to take the SoC 32k from the LFOSC input */
	writel(MCU_CTRL_DEVICE_CLKOUT_LFOSC_SELECT_VAL,
	       MCU_CTRL_DEVICE_CLKOUT_32K_CTRL);
}
#endif
