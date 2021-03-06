/*
 * K3: Architecture initialization
 *
 * Copyright (C) 2017-2018 Texas Instruments Incorporated - http://www.ti.com/
 *	Lokesh Vutla <lokeshvutla@ti.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <spl.h>
#include <dm.h>
#include <dm/uclass-internal.h>
#include <dm/pinctrl.h>
#include <remoteproc.h>
#include <linux/libfdt.h>
#include <linux/soc/ti/ti_sci_protocol.h>
#include <image.h>
#include <asm/sections.h>
#include <asm/armv7_mpu.h>
#include <asm/arch/hardware.h>
#include <asm/arch/boardcfg_data.h>

#ifdef CONFIG_SPL_BUILD
#ifdef CONFIG_CPU_V7R
struct mpu_region_config k3_mpu_regions[16] = {
	/*
	 * Make all 4GB as Device Memory and not executable. We are overriding
	 * it with next region for any requirement.
	 */
	{0x00000000, REGION_0, XN_EN, PRIV_RW_USR_RW, SHARED_WRITE_BUFFERED,
	 REGION_4GB},

	/* SPL code area marking it as WB and Write allocate. */
	{CONFIG_SPL_TEXT_BASE, REGION_1, XN_DIS, PRIV_RW_USR_RW,
	 O_I_WB_RD_WR_ALLOC, REGION_8MB},

	/* U-Boot's code area marking it as WB and Write allocate */
	{CONFIG_SYS_SDRAM_BASE, REGION_2, XN_DIS, PRIV_RW_USR_RW,
	 O_I_WB_RD_WR_ALLOC, REGION_2GB},
	{0x0, 3, 0x0, 0x0, 0x0, 0x0},
	{0x0, 4, 0x0, 0x0, 0x0, 0x0},
	{0x0, 5, 0x0, 0x0, 0x0, 0x0},
	{0x0, 6, 0x0, 0x0, 0x0, 0x0},
	{0x0, 7, 0x0, 0x0, 0x0, 0x0},
	{0x0, 8, 0x0, 0x0, 0x0, 0x0},
	{0x0, 9, 0x0, 0x0, 0x0, 0x0},
	{0x0, 10, 0x0, 0x0, 0x0, 0x0},
	{0x0, 11, 0x0, 0x0, 0x0, 0x0},
	{0x0, 12, 0x0, 0x0, 0x0, 0x0},
	{0x0, 13, 0x0, 0x0, 0x0, 0x0},
	{0x0, 14, 0x0, 0x0, 0x0, 0x0},
	{0x0, 15, 0x0, 0x0, 0x0, 0x0},
};
#endif

static void mmr_unlock(u32 base, u32 partition)
{
	/* Translate the base address */
	phys_addr_t part_base = base + partition * CTRL_MMR0_PARTITION_SIZE;

	/* Unlock the requested partition if locked using two-step sequence */
	writel(CTRLMMR_LOCK_KICK0_UNLOCK_VAL, part_base + CTRLMMR_LOCK_KICK0);
	writel(CTRLMMR_LOCK_KICK1_UNLOCK_VAL, part_base + CTRLMMR_LOCK_KICK1);
}

static void ctrl_mmr_unlock(void)
{
	/* Unlock all WKUP_CTRL_MMR0 module registers */
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 0);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 1);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 2);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 3);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 6);
	mmr_unlock(WKUP_CTRL_MMR0_BASE, 7);

	/* Unlock all MCU_CTRL_MMR0 module registers */
	mmr_unlock(MCU_CTRL_MMR0_BASE, 0);
	mmr_unlock(MCU_CTRL_MMR0_BASE, 1);
	mmr_unlock(MCU_CTRL_MMR0_BASE, 2);
	mmr_unlock(MCU_CTRL_MMR0_BASE, 6);

	/* Unlock all CTRL_MMR0 module registers */
	mmr_unlock(CTRL_MMR0_BASE, 0);
	mmr_unlock(CTRL_MMR0_BASE, 1);
	mmr_unlock(CTRL_MMR0_BASE, 2);
	mmr_unlock(CTRL_MMR0_BASE, 3);
	mmr_unlock(CTRL_MMR0_BASE, 6);
	mmr_unlock(CTRL_MMR0_BASE, 7);

}

static void store_boot_index_from_rom(void)
{
	u32 *boot_index = (u32 *)K3_BOOT_PARAM_TABLE_INDEX_VAL;

	*boot_index = *(u32 *)(CONFIG_BOOT_PARAM_TABLE_INDEX);
}

#ifdef CONFIG_K3_LOAD_SYSFW
static int locate_system_controller_firmware(int *addr, int *len)
{
	struct image_header *header;
	int images, node;
	const void *fit;

	if (IS_ENABLED(CONFIG_SPL_SEPARATE_BSS))
		fit = (ulong *)&_image_binary_end;
	else
		fit = (ulong *)&__bss_end;

	header = (struct image_header *)fit;
	if (image_get_magic(header) != FDT_MAGIC) {
		debug("No FIT image appended to SPL\n");
		return -EINVAL;
	}

	/* find the node holding the images information */
	images = fdt_path_offset(fit, FIT_IMAGES_PATH);
	if (images < 0) {
		debug("%s: Cannot find /images node: %d\n", __func__, images);
		return -ENOENT;
	}

	/* Find the subnode holding system controller firmware */
	node = fdt_subnode_offset(fit, images, "sysfw");
	if (node < 0) {
		debug("%s: Cannot find fdt node sysfw in FIT: %d\n",
		      __func__, node);
		return -EINVAL;
	}

	fit_image_get_data_offset(fit, node, addr);
	*addr += (int)fit + ((fdt_totalsize(fit) + 3) & ~3);
	fit_image_get_data_size(fit, node, len);

	return 0;
}
#endif

void board_init_f(ulong dummy)
{
#ifdef CONFIG_K3_LOAD_SYSFW
	int ret, fw_addr, len;
	struct ti_sci_handle *ti_sci;
	struct ti_sci_board_ops *board_ops;
#endif
#if defined(CONFIG_K3_LOAD_SYSFW) || defined(CONFIG_K3_AM654_DDRSS)
	struct udevice *dev;
#endif

	/*
	 * Cannot delay this further as there is a chance that
	 * BOOT_PARAM_TABLE_INDEX can be over written by SPL MALLOC section.
	 */
	store_boot_index_from_rom();

	/* Make all control module registers accessible */
	ctrl_mmr_unlock();

#ifdef CONFIG_CPU_V7R
	setup_mpu_regions(k3_mpu_regions, ARRAY_SIZE(k3_mpu_regions));
#endif

	/* Init DM early in-order to invoke system controller */
	spl_early_init();

#ifdef CONFIG_K3_LOAD_SYSFW
	/*
	 * Process pinctrl for the serial0 a.k.a. WKUP_UART0 module and continue
	 * regardless of the result of pinctrl. Do this without probing the
	 * device, but instead by searching the device that would request the
	 * given sequence number if probed. The UART will be used by the system
	 * firmware (SYSFW) image for various purposes and SYSFW depends on us
	 * to initialize its pin settings.
	 */
	ret = uclass_find_device_by_seq(UCLASS_SERIAL, 0, true, &dev);
	if (!ret) {
		pinctrl_select_state(dev, "default");
	}

	/* Try to locate firmware image and load it to system controller */
	if (!locate_system_controller_firmware(&fw_addr, &len)) {
		debug("Firmware located. Now try to load\n");
		/*
		 * It is assumed that remoteproc device 0 is the corresponding
		 * system-controller that runs SYSFW.
		 * Make sure DT reflects the same.
		 */
		ret = rproc_dev_init(0);
		if (ret) {
			debug("rproc failed to be initialized: ret= %d\n",
			      ret);
			return;
		}

		ret = rproc_load(0, fw_addr, len);
		if (ret) {
			debug("Firmware failed to start on rproc: ret= %d\n",
			      ret);
			return;
		}

		ret = rproc_start(0);
		if (ret) {
			debug("Firmware init failed on rproc: ret= %d\n",
			      ret);
			return;
		}
	}

	/* Bring up the Device Management and Security Controller (SYSFW) */
	ret = uclass_get_device_by_name(UCLASS_FIRMWARE, "dmsc", &dev);
	if (ret) {
		debug("Failed to initialize SYSFW (%d)\n", ret);
		return;
	}

	ti_sci = (struct ti_sci_handle *)(ti_sci_get_handle_from_sysfw(dev));
	board_ops = &ti_sci->ops.board_ops;

	/* Apply power/clock (PM) specific configuration to SYSFW */
	ret = board_ops->board_config_pm(ti_sci,
					 (u64)(u32)&am65_boardcfg_pm_data,
					 sizeof(am65_boardcfg_pm_data));
	if (ret) {
		debug("Failed to set board PM configuration (%d)\n", ret);
		return;
	}

	/*
	 * Now with the SYSFW having the PM configuration applied successfully,
	 * we can finally bring up our regular U-Boot console.
	 */
	preloader_console_init();

	/* Output System Firmware version info */
	printf("SYSFW ABI: %d.%d (firmware rev 0x%04x '%.*s')\n",
	       ti_sci->version.abi_major, ti_sci->version.abi_minor,
	       ti_sci->version.firmware_revision,
	       sizeof(ti_sci->version.firmware_description),
	       ti_sci->version.firmware_description);

	/* Apply the remainder of the board configuration to SYSFW */
	ret = board_ops->board_config(ti_sci,
				      (u64)(u32)&am65_boardcfg_data,
				      sizeof(am65_boardcfg_data));
	if (ret) {
		debug("Failed to set board configuration (%d)\n", ret);
		return;
	}

	ret = board_ops->board_config_rm(ti_sci,
					 (u64)(u32)&am65_boardcfg_rm_data,
					 sizeof(am65_boardcfg_rm_data));
	if (ret) {
		debug("Failed to set board RM configuration (%d)\n", ret);
		return;
	}

	ret = board_ops->board_config_security(ti_sci,
					 (u64)(u32)&am65_boardcfg_security_data,
					 sizeof(am65_boardcfg_security_data));
	if (ret) {
		debug("Failed to set board security configuration (%d)\n", ret);
		return;
	}
#else
	/* Prepare console output */
	preloader_console_init();
#endif

#ifdef CONFIG_K3_AM654_DDRSS
	ret = uclass_get_device(UCLASS_RAM, 0, &dev);
	if (ret) {
		printf("DRAM init failed: %d\n", ret);
		return;
	}
#endif
}

u32 spl_boot_mode(const u32 boot_device)
{
	switch (boot_device) {
	case BOOT_DEVICE_MMC2:
		return MMCSD_MODE_FS;
	default:
		return MMCSD_MODE_EMMCBOOT;
	}
}

static u32 __get_backup_bootmedia(u32 devstat)
{
	u32 bkup_boot = (devstat & CTRLMMR_MAIN_DEVSTAT_BKUP_BOOTMODE_MASK) >>
			CTRLMMR_MAIN_DEVSTAT_BKUP_BOOTMODE_SHIFT;

	switch (bkup_boot) {
#define __BKUP_BOOT_DEVICE(n)			\
	case BACKUP_BOOT_DEVICE_##n:		\
		return BOOT_DEVICE_##n;
	__BKUP_BOOT_DEVICE(USB);
	__BKUP_BOOT_DEVICE(UART);
	__BKUP_BOOT_DEVICE(ETHERNET);
	__BKUP_BOOT_DEVICE(MMC2);
	__BKUP_BOOT_DEVICE(SPI);
	__BKUP_BOOT_DEVICE(HYPERFLASH);
	__BKUP_BOOT_DEVICE(I2C);
	};

	return BOOT_DEVICE_RAM;
}

static u32 __get_primary_bootmedia(u32 devstat)
{
	u32 bootmode = devstat & CTRLMMR_MAIN_DEVSTAT_BOOTMODE_MASK;

	if (bootmode == BOOT_DEVICE_OSPI || bootmode ==	BOOT_DEVICE_QSPI)
		bootmode = BOOT_DEVICE_SPI;

	return bootmode;
}

u32 spl_boot_device(void)
{
	u32 devstat = readl(CTRLMMR_MAIN_DEVSTAT);
	u32 bootindex = readl(K3_BOOT_PARAM_TABLE_INDEX_VAL);

	if (bootindex == K3_PRIMARY_BOOTMODE)
		return __get_primary_bootmedia(devstat);
	else
		return __get_backup_bootmedia(devstat);
}
#endif

#ifndef CONFIG_SYSRESET
void reset_cpu(ulong ignored)
{
}
#endif

#ifdef CONFIG_K3_SPL_ATF

#define AM6_DEV_MCU_RTI0			134
#define AM6_DEV_MCU_RTI1			135
#define AM6_DEV_MCU_ARMSS0_CPU0			159
#define AM6_DEV_MCU_ARMSS0_CPU1			245

void release_resources_for_core_shutdown(void)
{
	struct udevice *dev;
	struct ti_sci_handle *ti_sci;
	struct ti_sci_dev_ops *dev_ops;
	int ret;
	u32 i;

	const u32 put_device_ids[] = {
		AM6_DEV_MCU_RTI0,
		AM6_DEV_MCU_RTI1,
		AM6_DEV_MCU_ARMSS0_CPU1,
		AM6_DEV_MCU_ARMSS0_CPU0,	/* Handle CPU0 after CPU1 */
	};

	/* Get handle to Device Management and Security Controller (SYSFW) */
	ret = uclass_get_device_by_name(UCLASS_FIRMWARE, "dmsc", &dev);
	if (ret) {
		printf("Failed to get handle to SYSFW (%d)\n", ret);
		hang();
	}

	ti_sci = (struct ti_sci_handle *)(ti_sci_get_handle_from_sysfw(dev));
	dev_ops = &ti_sci->ops.dev_ops;

	/* Iterate through list of devices to put (shutdown) */
	for (i = 0; i < ARRAY_SIZE(put_device_ids); i++) {
		u32 id = put_device_ids[i];

		ret = dev_ops->put_device(ti_sci, id);
		if (ret) {
			printf("Failed to put device %u (%d)\n", id, ret);
			hang();
		}
	}
}

void __noreturn jump_to_image_no_args(struct spl_image_info *spl_image)
{
	int ret;

	/*
	 * It is assumed that remoteproc device 1 is the corresponding
	 * cortex A core which runs ATF. Make sure DT reflects the same.
	 */
	ret = rproc_dev_init(1);
	if (ret) {
		printf("%s: ATF failed to Initialize on rproc: ret= %d\n",
		       __func__, ret);
		hang();
	}

	ret = rproc_load(1, spl_image->entry_point, 0x200);
	if (ret) {
		printf("%s: ATF failed to load on rproc: ret= %d\n",
		       __func__, ret);
		hang();
	}

	/* Add an extra newline to differentiate the ATF logs from SPL */
	printf("Starting ATF on ARM64 core...\n\n");

	ret = rproc_start(1);
	if (ret) {
		printf("%s: ATF failed to start on rproc: ret= %d\n",
		       __func__, ret);
		hang();
	}

	debug("Releasing resources...\n");
	release_resources_for_core_shutdown();

	debug("Finalizing core shutdown...\n");
	while (1)
		asm volatile("wfe");
}
#endif
