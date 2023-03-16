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
#include <k3-ddrss.h>
#include <power/tps65219.h>
#include <spl.h>

#include "../common/tdx-cfg-block.h"

DECLARE_GLOBAL_DATA_PTR;

/* from 5.8 Boot Memory Maps */
#define BOOT_PARAMETER_TABLE 0x43c3f298
/* from 5.6.1 Common Header */
#define BOOT_PARAMETER_PRIMARY_PERIPHERAL (*(u16 *)(BOOT_PARAMETER_TABLE + 0x04))
#define BOOT_PARAMETER_SECONDARY_PERIPHERAL (*(u16 *)(BOOT_PARAMETER_TABLE + 0x204))
#define BOOT_PARAMETER_EMMC 101
#define BOOT_PARAMETER_SD 100
#define BOOT_PARAMETER_USB_DFU 70

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
	return fdtdec_setup_mem_size_base();
}

int dram_init_banksize(void)
{
	return fdtdec_setup_memory_banksize();
}

#if defined(CONFIG_SPL_LOAD_FIT)
int board_fit_config_name_match(const char *name)
{
	return 0;
}
#endif

#if IS_ENABLED(CONFIG_SPL_BUILD)
#if IS_ENABLED(CONFIG_K3_AM64_DDRSS)
static void fixup_ddr_driver_for_ecc(struct spl_image_info *spl_image)
{
	struct udevice *dev;
	int ret;

	dram_init_banksize();

	ret = uclass_get_device(UCLASS_RAM, 0, &dev);
	if (ret)
		panic("Cannot get RAM device for ddr size fixup: %d\n", ret);

	ret = k3_ddrss_ddr_fdt_fixup(dev, spl_image->fdt_addr, gd->bd);
	if (ret)
		printf("Error fixing up ddr node for ECC use! %d\n", ret);
}
#else
static void fixup_memory_node(struct spl_image_info *spl_image)
{
	u64 start[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];
	int bank;
	int ret;

	dram_init();
	dram_init_banksize();

	for (bank = 0; bank < CONFIG_NR_DRAM_BANKS; bank++) {
		start[bank] = gd->bd->bi_dram[bank].start;
		size[bank] = gd->bd->bi_dram[bank].size;
	}

	/* dram_init functions use SPL fdt, and we must fixup u-boot fdt */
	ret = fdt_fixup_memory_banks(spl_image->fdt_addr, start, size,
				     CONFIG_NR_DRAM_BANKS);
	if (ret)
		printf("Error fixing up memory node! %d\n", ret);
}
#endif

void spl_perform_fixups(struct spl_image_info *spl_image)
{
#if IS_ENABLED(CONFIG_K3_AM64_DDRSS)
	fixup_ddr_driver_for_ecc(spl_image);
#else
	fixup_memory_node(spl_image);
#endif
}
#endif

#if IS_ENABLED(CONFIG_OF_LIBFDT) && IS_ENABLED(CONFIG_OF_BOARD_SETUP)
int ft_board_setup(void *blob, struct bd_info *bd)
{
	return ft_common_board_setup(blob, bd);
}
#endif

#if !defined(CONFIG_SPL_BUILD)
/* Should check if we were booted over dfu, and if so go to DFU / UMS for download */
void decide_on_dfu(void)
{
	/*
	 * SD boot with sdcard,		7000f290: 00000000
	 * SD boot without sdcard, dfu,	7000f290: 00000001
	 * eMMC Boot, eMMC flashed	7000f290: 00000000
	 * eMMC Boot, eMMC zeroed	7000f290: 00000001
	 * DFU Boot			7000f290: 00000000
	 *
	 * If this Memory location will stay untouched in future versions is
	 * unknown, so the code might break.
	 * Compare with arch/arm/mach-k3/am625_init.c which however is SPL only
	 */
	u32 bootindex = *(u32 *)(CONFIG_SYS_K3_BOOT_PARAM_TABLE_INDEX);
	u32 bootmode = bootindex & 1 ? BOOT_PARAMETER_SECONDARY_PERIPHERAL :
				       BOOT_PARAMETER_PRIMARY_PERIPHERAL;

	if (bootmode == BOOT_PARAMETER_USB_DFU) {
		printf("DFU boot mode detected, going to DFU again for further downloads\n");
		env_set("bootcmd", "setenv dfu_alt_info $dfu_alt_info_emmc;"
				   "dfu 0 mmc 0; ums 0 mmc 0");
	}

	debug("Booting from %s boot device\n", bootindex & 1 ? "Secondary" :
							       "Primary");
	debug("Primary boot device as read from boot parameters %hd\n",
	      BOOT_PARAMETER_PRIMARY_PERIPHERAL);
	debug("Secondary boot device as read from boot parameters %hd\n",
	      BOOT_PARAMETER_SECONDARY_PERIPHERAL);
}
#else
void decide_on_dfu(void) {}
#endif /* CONFIG_SPL_BUILD */

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

	/* set bootcmd to start DFU and then UMS mode if booted from DFU */
	decide_on_dfu();

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

	/* Init DRAM size for R5/A53 SPL */
	dram_init_banksize();
}
#endif
