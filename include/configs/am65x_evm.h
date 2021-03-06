/*
 * Configuration header file for K3 AM654 EVM
 *
 * Copyright (C) 2017-2018 Texas Instruments Incorporated - http://www.ti.com/
 *	Lokesh Vutla <lokeshvutla@ti.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __CONFIG_AM654_EVM_H
#define __CONFIG_AM654_EVM_H

#include <linux/sizes.h>
#include <config_distro_bootcmd.h>
#include <environment/ti/mmc.h>

#define CONFIG_ENV_SIZE			(128 << 10)

/* DDR Configuration */
#define CONFIG_NR_DRAM_BANKS		2
#define CONFIG_SYS_SDRAM_BASE1		0x880000000

/* SPL Loader Configuration */
#ifdef CONFIG_TARGET_AM654_A53_EVM
#define CONFIG_SPL_TEXT_BASE		0x80080000
#define CONFIG_SYS_SPI_U_BOOT_OFFS	0x280000
#else
#define CONFIG_SPL_TEXT_BASE		0x41c00000
#define CONFIG_SYS_SPI_U_BOOT_OFFS	0x80000
#endif

/* Clock Defines */
#define V_OSCK				24000000
#define V_SCLK				V_OSCK

#ifdef CONFIG_K3_SPL_ATF
#define CONFIG_SPL_FS_LOAD_PAYLOAD_NAME	"tispl.bin"
#endif

#define CONFIG_SKIP_LOWLEVEL_INIT

#define CONFIG_SPL_MAX_SIZE		CONFIG_MAX_DOWNLODABLE_IMAGE_SIZE
#define CONFIG_SYS_INIT_SP_ADDR         (CONFIG_SPL_TEXT_BASE +	\
					CONFIG_NON_SECURE_MSRAM_SIZE - 4)
#define PARTS_DEFAULT \
	/* Linux partitions */ \
	"name=rootfs,start=0,size=-,uuid=${uuid_gpt_rootfs}\0"

/* U-Boot general configuration */
#define EXTRA_ENV_AM65X_BOARD_SETTINGS					\
	"findfdt="							\
		"if test $board_name = am65x; then "			\
			"setenv name_fdt k3-am654-base-board.dtb; fi;"	\
		"if test $board_name = am65x_idk; then "         \
			"setenv name_fdt k3-am654-base-board.dtb; "	\
			"setenv name_overlays \"k3-am654-pcie-usb2.dtbo k3-am654-idk.dtbo\"; fi;"\
		"if test $board_name = am65x_evm; then "         \
			"setenv name_fdt k3-am654-base-board.dtb; "	\
			"setenv name_overlays \"k3-am654-pcie-usb3.dtbo k3-am654-evm-csi2-ov490.dtbo k3-am654-evm-oldi-lcd1evm.dtbo\"; "\
		"else if test $name_fdt = undefined; then "		\
			"echo WARNING: Could not determine device tree to use;"\
		"fi; fi; "						\
		"setenv fdtfile ${name_fdt};"				\
		"setenv overlay_files ${name_overlays}\0"		\
	"loadaddr=0x80080000\0"						\
	"fdtaddr=0x82000000\0"						\
	"overlayaddr=0x83000000\0"					\
	"name_kern=Image\0"						\
	"console=ttyS2,115200n8\0"					\
	"args_all=setenv optargs earlycon=ns16550a,mmio32,0x02800000 "	\
		"${mtdparts}\0"						\
	"run_kern=booti ${loadaddr} ${rd_spec} ${fdtaddr}\0"

/* U-Boot MMC-specific configuration */
#define EXTRA_ENV_AM65X_BOARD_SETTINGS_MMC				\
	"boot=mmc\0"							\
	"mmcdev=1\0"							\
	"bootpart=1:2\0"						\
	"bootdir=/boot\0"						\
	"rd_spec=-\0"							\
	"init_mmc=run args_all args_mmc\0"				\
	"get_fdt_mmc=load mmc ${bootpart} ${fdtaddr} ${bootdir}/${name_fdt}\0" \
	"get_overlay_mmc="						\
		"fdt address ${fdtaddr};"				\
		"fdt resize 0x100000;"					\
		"for overlay in $overlay_files;"			\
		"do;"							\
		"load mmc ${bootpart} ${overlayaddr} ${bootdir}/${overlay};"	\
		"fdt apply ${overlayaddr};"				\
		"done;\0"						\
	"get_kern_mmc=load mmc ${bootpart} ${loadaddr} "		\
		"${bootdir}/${name_kern}\0"				\
	"partitions=" PARTS_DEFAULT

#ifdef CONFIG_TARGET_AM654_A53_EVM
#define EXTRA_ENV_AM65X_BOARD_SETTINGS_MTD				\
	"mtdids=" CONFIG_MTDIDS_DEFAULT "\0"				\
	"mtdparts=" CONFIG_MTDPARTS_DEFAULT "\0"
#else
#define EXTRA_ENV_AM65X_BOARD_SETTINGS_MTD
#endif

#define EXTRA_ENV_AM65X_BOARD_SETTINGS_UBI				\
	"init_ubi=run args_all args_ubi; sf probe; "			\
		"ubi part ospi.rootfs; ubifsmount ubi:rootfs;\0"	\
	"get_kern_ubi=ubifsload ${loadaddr} ${bootdir}/${name_kern}\0"	\
	"get_fdt_ubi=ubifsload ${fdtaddr} ${bootdir}/${name_fdt}\0"	\
	"args_ubi=setenv bootargs ${console} ${optargs} rootfstype=ubifs "\
	"root=ubi0:rootfs rw ubi.mtd=ospi.rootfs\0"

/* Incorporate settings into the U-Boot environment */
#define CONFIG_EXTRA_ENV_SETTINGS					\
	DEFAULT_MMC_TI_ARGS						\
	EXTRA_ENV_AM65X_BOARD_SETTINGS					\
	EXTRA_ENV_AM65X_BOARD_SETTINGS_MMC				\
	EXTRA_ENV_AM65X_BOARD_SETTINGS_MTD				\
	EXTRA_ENV_AM65X_BOARD_SETTINGS_UBI

#define CONFIG_SUPPORT_EMMC_BOOT

/* Non Kconfig SF configs */
#define CONFIG_SPL_SPI_LOAD
#define CONFIG_SF_DEFAULT_SPEED		0
#define CONFIG_SF_DEFAULT_MODE		0
#define CONFIG_MTD_PARTITIONS
#ifdef CONFIG_ENV_IS_IN_SPI_FLASH
#define CONFIG_ENV_OFFSET		0x780000
#define CONFIG_ENV_SECT_SIZE		0x20000
#endif

/* MMC ENV related defines */
#ifdef CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV		0
#define CONFIG_SYS_MMC_ENV_PART	1
#define CONFIG_ENV_SIZE		(128 << 10)
#define CONFIG_ENV_OFFSET		0x780000
#define CONFIG_ENV_OFFSET_REDUND	(CONFIG_ENV_OFFSET + CONFIG_ENV_SIZE)
#define CONFIG_SYS_REDUNDAND_ENVIRONMENT
#endif

/* Now for the remaining common defines */
#include <configs/ti_armv7_common.h>

#endif /* __CONFIG_AM654_EVM_H */
