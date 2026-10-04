// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (C) 2026 Danila Tikhonov <danila@jiaxyga.com>
 */

#include <init.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <asm/io.h>

#define MV300_CRG_BASE          0xf8a22000UL
#define MV300_PERI_BASE         0xf8a20000UL

#define MV300_USB_CRG           (MV300_CRG_BASE + 0xb8)
#define MV300_USB_PHY_CRG       (MV300_CRG_BASE + 0xbc)

#define MV300_USB_PHY0_BASE     0xf9865000UL
#define MV300_USB_PHY1_BASE     0xf9865400UL
#define MV300_USB_PHY2_BASE     0xf9960000UL

#define MV300_USB_OTG_CTRL      (MV300_PERI_BASE + 0x12c)

static void hi3798mv300_usb_init(void)
{
	u32 val;

	/*
	 * Vendor U-Boot FUN_00c57d18()
	 *
	 * Assert USB resets.
	 */
	val = readl(MV300_USB_CRG);
	val |= 0x0003f000;
	writel(val, MV300_USB_CRG);
	udelay(200);

	/*
	 * Initial CRG setup.
	 */
	val = readl(MV300_USB_PHY_CRG);
	val |= 0x00000750;
	writel(val, MV300_USB_PHY_CRG);
	udelay(200);

	/*
	 * PERI USB/OTG control:
	 * vendor does:
	 *
	 *   reg &= 0xfffffdfe;
	 *
	 * i.e. clears mask 0x201.
	 */
	val = readl(MV300_USB_OTG_CTRL);
	val &= 0xfffffdfe;
	writel(val, MV300_USB_OTG_CTRL);
	udelay(100);

	/*
	 * More PHY clock/control setup.
	 */
	val = readl(MV300_USB_PHY_CRG);
	val |= 0x5;
	writel(val, MV300_USB_PHY_CRG);
	udelay(300);

	val = readl(MV300_USB_PHY_CRG);
	val &= 0xffffffaf;	/* clear 0x50 */
	writel(val, MV300_USB_PHY_CRG);
	udelay(500);

	val = readl(MV300_USB_PHY_CRG);
	val &= 0xfffe7fff;	/* clear 0x18000 */
	writel(val, MV300_USB_PHY_CRG);
	udelay(200);

	/*
	 * PHY0 @ 0xf9865000
	 */
	writel(0x05, MV300_USB_PHY0_BASE + 0x18);
	writel(0x5c, MV300_USB_PHY0_BASE + 0x08);

	/*
	 * PHY1 @ 0xf9865400
	 */
	writel(0x5c, MV300_USB_PHY1_BASE + 0x08);

	udelay(20);

	writel(0x1c, MV300_USB_PHY0_BASE + 0x00);
	writel(0x1c, MV300_USB_PHY1_BASE + 0x00);

	udelay(20);

	writel(0x84, MV300_USB_PHY0_BASE + 0x14);
	writel(0x84, MV300_USB_PHY1_BASE + 0x14);

	udelay(20);

	writel(0xd7, MV300_USB_PHY0_BASE + 0x10);
	writel(0xd7, MV300_USB_PHY1_BASE + 0x10);

	udelay(20);

	writel(0x68, MV300_USB_PHY0_BASE + 0x7c);
	writel(0x68, MV300_USB_PHY1_BASE + 0x7c);

	udelay(20);

	/*
	 * Third PHY block @ 0xf9960000
	 */
	writel(0x05, MV300_USB_PHY2_BASE + 0x18);
	writel(0x5c, MV300_USB_PHY2_BASE + 0x08);

	udelay(20);

	writel(0x1c, MV300_USB_PHY2_BASE + 0x00);

	udelay(20);

	writel(0x84, MV300_USB_PHY2_BASE + 0x14);

	udelay(20);

	writel(0xd7, MV300_USB_PHY2_BASE + 0x10);

	udelay(20);

	/*
	 * Note: PHY2 differs from PHY0/1 here.
	 */
	writel(0xe8, MV300_USB_PHY2_BASE + 0x7c);

	udelay(20);

	/*
	 * Vendor clears bits 8..10 in CRG+0xbc.
	 */
	val = readl(MV300_USB_PHY_CRG);
	val &= 0xfffff8ff;
	writel(val, MV300_USB_PHY_CRG);

	udelay(2000);

	/*
	 * Enable USB clocks.
	 */
	val = readl(MV300_USB_CRG);
	val |= 0xff;
	writel(val, MV300_USB_CRG);

	udelay(200);

	/*
	 * Finally deassert USB resets 12..17.
	 */
	val = readl(MV300_USB_CRG);
	val &= 0xfffc0fff;
	writel(val, MV300_USB_CRG);

	udelay(200);
}

int board_init(void)
{
	hi3798mv300_usb_init();

	return 0;
}