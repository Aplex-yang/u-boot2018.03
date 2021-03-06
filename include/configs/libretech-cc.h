/*
 * Configuration for LibreTech CC
 *
 * Copyright (C) 2017 Baylibre, SAS
 * Author: Neil Armstrong <narmstrong@baylibre.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#define CONFIG_MISC_INIT_R

#define CONFIG_PHY_ADDR		8

#define MESON_FDTFILE_SETTING "fdtfile=amlogic/meson-gxl-s905x-libretech-cc.dtb\0"

#include <configs/meson-gxbb-common.h>

#endif /* __CONFIG_H */
