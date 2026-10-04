// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (C) 2026 Danila Tikhonov <danila@jiaxyga.com>
 */

#include <clk-uclass.h>
#include <dm.h>
#include <errno.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/io.h>

#include <dt-bindings/clock/histb-clock.h>

#define CRG_I2C				0x06c	/* PERI_CRG27 */
#define CRG_SSP				0x070	/* PERI_CRG28 */
#define CRG_SDIO0			0x09c	/* PERI_CRG39 */
#define CRG_EMMC			0x0a0	/* PERI_CRG40 */
#define CRG_USB2			0x0b8	/* PERI_CRG46 */

#define HI3798MV300_CLK_SDIO0		0x09c
#define HI3798MV300_CLK_SSP		0x070
#define HI3798MV300_CLK_USB2		0x0b8

#define HI3798MV300_CLK_I2C0		0x1ac
#define HI3798MV300_CLK_I2C1		0x1b0
#define HI3798MV300_CLK_I2C2		0x1b4

#define SDIO_CLK_EN			BIT(0)
#define SDIO_CIU_CLK_EN			BIT(1)
#define SDIO_SRST_REQ			BIT(4)

#define SDIO_CLK_SEL_SHIFT		8
#define SDIO_CLK_SEL_MASK		GENMASK(10, 8)

#define SDIO_SAP_PS_SHIFT		12
#define SDIO_SAP_PS_MASK		GENMASK(14, 12)

#define SDIO_DRV_PS_SHIFT		16
#define SDIO_DRV_PS_MASK		GENMASK(18, 16)

#define USB2_BUS_CKEN			BIT(0)
#define USB2_OHCI48M_CKEN		BIT(1)
#define USB2_OHCI12M_CKEN		BIT(2)
#define USB2_OTG_UTMI_CKEN		BIT(3)
#define USB2_HST_PHY_CKEN		BIT(4)
#define USB2_UTMI0_CKEN			BIT(5)
#define USB2_UTMI1_CKEN			BIT(6)

#define USB2_BUS_SRST_REQ		BIT(12)
#define USB2_UTMI0_SRST_REQ		BIT(13)
#define USB2_UTMI1_SRST_REQ		BIT(14)
#define USB2_HST_PHY_SRST_REQ		BIT(16)
#define USB2_OTG_PHY_SRST_REQ		BIT(17)

#define I2C0_CLK_EN			BIT(4)
#define I2C0_SRST_REQ			BIT(5)

#define I2C1_CLK_EN			BIT(8)
#define I2C1_SRST_REQ			BIT(9)

#define I2C2_CLK_EN			BIT(12)
#define I2C2_SRST_REQ			BIT(13)

#define SSP_CLK_EN			BIT(0)
#define SSP_SRST_REQ			BIT(1)

#define USB2_CLK_MASK			GENMASK(7, 0)
#define USB2_RESET_MASK			GENMASK(17, 12)

struct hi3798mv300_clk_priv {
	void __iomem *base;
};

static void hi3798mv300_clk_gate_enable(struct hi3798mv300_clk_priv *priv,
					u32 reg, u32 gate_mask,
					u32 reset_mask)
{
	u32 val;

	val = readl(priv->base + reg);

	val |= gate_mask;
	val &= ~reset_mask;

	writel(val, priv->base + reg);
}

static ulong
hi3798mv300_sdio0_get_rate(struct hi3798mv300_clk_priv *priv)
{
	u32 val, sel;

	val = readl(priv->base + CRG_SDIO0);
	sel = (val & SDIO_CLK_SEL_MASK) >> SDIO_CLK_SEL_SHIFT;

	switch (sel) {
	case 0:
		return 100000000;
	case 1:
		return 50000000;
	case 2:
		return 150000000;
	case 3:
		return 25000000;
	default:
		return 0;
	}
}

static ulong
hi3798mv300_sdio0_set_rate(struct hi3798mv300_clk_priv *priv, ulong rate)
{
	u32 val, sel;

	switch (rate) {
	case 25000000:
		sel = 3;
		break;
	case 50000000:
		sel = 1;
		break;
	case 100000000:
		sel = 0;
		break;
	case 150000000:
		sel = 2;
		break;
	default:
		return -EINVAL;
	}

	val = readl(priv->base + CRG_SDIO0);
	val &= ~SDIO_CLK_SEL_MASK;
	val |= sel << SDIO_CLK_SEL_SHIFT;
	writel(val, priv->base + CRG_SDIO0);

	return rate;
}

static int hi3798mv300_clk_enable(struct clk *clk)
{
	struct hi3798mv300_clk_priv *priv = dev_get_priv(clk->dev);
	u32 val;

	switch (clk->id) {
	case HI3798MV300_CLK_SDIO0:
		hi3798mv300_clk_gate_enable(priv, CRG_SDIO0,
					   SDIO_CLK_EN |
					   SDIO_CIU_CLK_EN,
					   SDIO_SRST_REQ);
		udelay(100);
		return 0;
	case HISTB_MMC_CIU_CLK:
	case HISTB_MMC_BIU_CLK:
		return 0;
	case HI3798MV300_CLK_I2C0:
		hi3798mv300_clk_gate_enable(priv, CRG_I2C,
					   I2C0_CLK_EN,
					   I2C0_SRST_REQ);
		return 0;
	case HI3798MV300_CLK_I2C1:
		hi3798mv300_clk_gate_enable(priv, CRG_I2C,
					   I2C1_CLK_EN,
					   I2C1_SRST_REQ);
		return 0;
	case HI3798MV300_CLK_I2C2:
		hi3798mv300_clk_gate_enable(priv, CRG_I2C,
					   I2C2_CLK_EN,
					   I2C2_SRST_REQ);
		return 0;
	case HI3798MV300_CLK_SSP:
		hi3798mv300_clk_gate_enable(priv, CRG_SSP,
					   SSP_CLK_EN,
					   SSP_SRST_REQ);
		return 0;
	case HI3798MV300_CLK_USB2:
		val = readl(priv->base + CRG_USB2);

		val |= USB2_CLK_MASK;
		val &= ~USB2_RESET_MASK;

		writel(val, priv->base + CRG_USB2);

		return 0;
	default:
		return -ENOENT;
	}
}

static int hi3798mv300_clk_disable(struct clk *clk)
{
	switch (clk->id) {
	case HI3798MV300_CLK_SDIO0:
	case HISTB_MMC_CIU_CLK:
	case HISTB_MMC_BIU_CLK:
	case HI3798MV300_CLK_I2C0:
	case HI3798MV300_CLK_I2C1:
	case HI3798MV300_CLK_I2C2:
	case HI3798MV300_CLK_SSP:
	case HI3798MV300_CLK_USB2:
		return 0;
	default:
		return -ENOENT;
	}
}

static ulong hi3798mv300_clk_get_rate(struct clk *clk)
{
	struct hi3798mv300_clk_priv *priv = dev_get_priv(clk->dev);

	switch (clk->id) {
	case HI3798MV300_CLK_SDIO0:
		return hi3798mv300_sdio0_get_rate(priv);
	case HISTB_MMC_CIU_CLK:
	case HISTB_MMC_BIU_CLK:
		return 50000000;
	case HI3798MV300_CLK_I2C0:
	case HI3798MV300_CLK_I2C1:
	case HI3798MV300_CLK_I2C2:
	case HI3798MV300_CLK_SSP:
		return 100000000;
	case HI3798MV300_CLK_USB2:
		return 48000000;
	default:
		return -ENOENT;
	}
}

static ulong hi3798mv300_clk_set_rate(struct clk *clk, ulong rate)
{
	struct hi3798mv300_clk_priv *priv = dev_get_priv(clk->dev);

	switch (clk->id) {
	case HI3798MV300_CLK_SDIO0:
		return hi3798mv300_sdio0_set_rate(priv, rate);

	default:
		return -ENOENT;
	}
}

static int hi3798mv300_clk_probe(struct udevice *dev)
{
	struct hi3798mv300_clk_priv *priv = dev_get_priv(dev);

	priv->base = dev_read_addr_ptr(dev);
	if (!priv->base)
		return -EINVAL;

	return 0;
}

static const struct clk_ops hi3798mv300_clk_ops = {
	.enable		= hi3798mv300_clk_enable,
	.disable	= hi3798mv300_clk_disable,
	.get_rate	= hi3798mv300_clk_get_rate,
	.set_rate	= hi3798mv300_clk_set_rate,
};

static const struct udevice_id hi3798mv300_clk_ids[] = {
	{ .compatible = "hisilicon,hi3798mv300-crg" },
	{ }
};

U_BOOT_DRIVER(hi3798mv300_clk) = {
	.name		= "hi3798mv300_clk",
	.id		= UCLASS_CLK,
	.of_match	= hi3798mv300_clk_ids,
	.ops		= &hi3798mv300_clk_ops,
	.probe		= hi3798mv300_clk_probe,
	.priv_auto	= sizeof(struct hi3798mv300_clk_priv),
};
