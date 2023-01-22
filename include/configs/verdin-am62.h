/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Configuration header file for Verdin AM62 SoM
 *
 * Copyright 2023 Toradex - https://www.toradex.com/
 */

#ifndef __CONFIG_VERDIN_AM62_H
#define __CONFIG_VERDIN_AM62_H

#include <environment/ti/k3_dfu.h>
#include <linux/sizes.h>

#define CONFIG_LOADADDR			0x88200000

/* DDR Configuration */
#define CONFIG_SYS_SDRAM_BASE1		0x880000000
#define CONFIG_SYS_BOOTM_LEN		SZ_64M

#ifdef CONFIG_SYS_K3_SPL_ATF
#define CONFIG_SPL_FS_LOAD_PAYLOAD_NAME	"tispl.bin"
#endif

#if defined(CONFIG_TARGET_VERDIN_AM62_A53)
#define CONFIG_SPL_MAX_SIZE		SZ_1M
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SPL_TEXT_BASE + SZ_4M)
#else /* CONFIG_TARGET_VERDIN_AM62_A53 */
#define CONFIG_SPL_MAX_SIZE		CONFIG_SYS_K3_MAX_DOWNLODABLE_IMAGE_SIZE
/*
 * Maximum size in memory allocated to the SPL BSS. Keep it as tight as
 * possible (to allow the build to go through), as this directly affects
 * our memory footprint. The less we use for BSS the more we have available
 * for everything else.
 */
#define CONFIG_SPL_BSS_MAX_SIZE		0x3000
/*
 * Link BSS to be within SPL in a dedicated region located near the top of
 * the MCU SRAM, this way making it available also before relocation. Note
 * that we are not using the actual top of the MCU SRAM as there is a memory
 * location allocated for Device Manager data and some memory filled in by
 * the boot ROM that we want to read out without any interference from the
 * C context.
 */
#define CONFIG_SPL_BSS_START_ADDR	(0x43c3e000 -\
					 CONFIG_SPL_BSS_MAX_SIZE)
/* Set the stack right below the SPL BSS section */
#define CONFIG_SYS_INIT_SP_ADDR		0x43c3a7f0
/* Configure R5 SPL post-relocation malloc pool in DDR */
#define CONFIG_SYS_SPL_MALLOC_START	0x84000000
#define CONFIG_SYS_SPL_MALLOC_SIZE	SZ_16M
#endif /* CONFIG_TARGET_VERDIN_AM62_A53 */

#define MEM_LAYOUT_ENV_SETTINGS \
	"fdt_addr_r=0x90200000\0" \
	"kernel_addr_r=" __stringify(CONFIG_LOADADDR) "\0" \
	"kernel_comp_addr_r=0x80200000\0" \
	"kernel_comp_size=0x08000000\0" \
	"ramdisk_addr_r=0x90300000\0" \
	"scriptaddr=0x90280000\0"

/* Enable Distro Boot */
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 1) \
	func(MMC, mmc, 0) \
	func(DHCP, dhcp, na)
#include <config_distro_bootcmd.h>

#define EXTRA_ENV_DFUARGS \
	DFU_ALT_INFO_EMMC \
	DFU_ALT_INFO_RAM

/* Initial environment variables */
#define CONFIG_EXTRA_ENV_SETTINGS \
	BOOTENV \
	EXTRA_ENV_DFUARGS \
	MEM_LAYOUT_ENV_SETTINGS \
	"boot_scripts=boot.scr\0" \
	"boot_script_dhcp=boot.scr\0" \
	"console=ttyS2\0" \
	"fdt_board=dev\0" \
	"setup=setenv setupargs console=tty1 console=${console},${baudrate} " \
		"consoleblank=0 earlycon=ns16550a,mmio32,0x02800000\0" \
	"update_uboot=askenv confirm Did you load flash.bin (y/N)?; " \
		"if test \"$confirm\" = \"y\"; then " \
		"setexpr blkcnt ${filesize} + 0x1ff && setexpr blkcnt " \
		"${blkcnt} / 0x200; mmc dev 0 1; mmc write ${loadaddr} 0x0 " \
		"${blkcnt}; fi\0"

/* Now for the remaining common defines */
#include <configs/ti_armv7_common.h>

#ifdef CONFIG_SYS_MALLOC_LEN
#undef CONFIG_SYS_MALLOC_LEN
#endif
#define CONFIG_SYS_MALLOC_LEN		SZ_128M

#endif /* __CONFIG_VERDIN_AM62_H */
