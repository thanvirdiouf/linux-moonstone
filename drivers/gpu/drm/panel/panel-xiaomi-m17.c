#include <linux/of.h>
#include <linux/mod_devicetable.h>
// SPDX-License-Identifier: GPL-2.0-only
/*
 * Xiaomi Poco X5 5G (m17-k6s) DSI Panel Driver
 * Extracted and unified for Mainline Linux DRM
 */

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>

struct xiaomi_m17_panel {
	struct drm_panel base;
	struct mipi_dsi_device *dsi;
	struct regulator *vddio;
	struct regulator *vdd;
	struct gpio_desc *reset_gpio;
};

static inline struct xiaomi_m17_panel *to_xiaomi_m17_panel(struct drm_panel *panel)
{
	return container_of(panel, struct xiaomi_m17_panel, base);
}

/* 
 * QCOM downstream DTS arrays pack DSI sequences using a 7-byte header:
 * [0] Data Type
 * [1] Last Command flag
 * [2] Virtual Channel (VC)
 * [3] Ack request flag
 * [4] Wait time in MS after transfer
 * [5] Payload Length MSB
 * [6] Payload Length LSB
 * [7+] Payload...
 * 
 * We parse this directly at runtime to allow verbatim copy-pasting 
 * of downstream vendor blobs without manual DCS macro translation.
 */
static int qcom_dts_mipi_dsi_send_seq(struct mipi_dsi_device *dsi, const u8 *seq, size_t len)
{
	const u8 *p = seq;
	int ret;

	while (len >= 7) {
		u8 type = p[0];
		u8 wait = p[4];
		u16 plen = (p[5] << 8) | p[6];

		if (len < 7 + plen)
			return -EINVAL;

		if (plen > 0) {
			struct mipi_dsi_msg msg = {
				.type = type,
				.channel = dsi->channel,
				.tx_buf = p + 7,
				.tx_len = plen,
			};
			ret = dsi->host->ops->transfer(dsi->host, &msg);
			if (ret < 0)
				return ret;
		}

		if (wait)
			msleep(wait);

		p += 7 + plen;
		len -= (7 + plen);
	}
	return 0;
}

/* 
 * Formatted from downstream qcom,mdss-dsi-on-command.
 * Truncated for brevity - paste the full array from your DTS 
 * replacing spaces with ', 0x'.
 */
static const u8 xiaomi_m17_k6s_on_cmds[] = {
    0x05, 0x01, 0x00, 0x00, 0x14, 0x00, 0x02, 0x11, 0x00,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0xf0, 0x5a, 0x5a,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0xb2, 0x01, 0x31,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x06, 0xdf, 0x09, 0x30, 0x95, 0x46, 0xe9,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0xf0, 0xa5, 0xa5,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0x9d, 0x01,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x5a, 0x9e, 0x11, 0x00, 0x00, 0x89, 0x30, 0x80, 0x09, 0x60,
        0x04, 0x38, 0x00, 0x28, 0x02, 0x1c, 0x02, 0x1c, 0x02, 0x00, 0x02, 0x0e, 0x00, 0x20, 0x03, 0xdd,
        0x00, 0x07, 0x00, 0x0c, 0x02, 0x77, 0x02, 0x8b, 0x18, 0x00, 0x10, 0xf0, 0x03, 0x0c, 0x20, 0x00,
        0x06, 0x0b, 0x0b, 0x33, 0x0e, 0x1c, 0x2a, 0x38, 0x46, 0x54, 0x62, 0x69, 0x70, 0x77, 0x79, 0x7b,
        0x7d, 0x7e, 0x01, 0x02, 0x01, 0x00, 0x09, 0x40, 0x09, 0xbe, 0x19, 0xfc, 0x19, 0xfa, 0x19, 0xf8,
        0x1a, 0x38, 0x1a, 0x78, 0x1a, 0xb6, 0x2a, 0xf6, 0x2b, 0x34, 0x2b, 0x74, 0x3b, 0x74, 0x6b, 0xf4,
        0x00,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0xf0, 0x5a, 0x5a,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0x60, 0x21,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0xf7, 0x0b,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0xf0, 0xa5, 0xa5,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0xf0, 0x5a, 0x5a,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04, 0xb0, 0x00, 0x15, 0xf6,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0xf6, 0xf0,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04, 0xb0, 0x00, 0x28, 0xf6,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0xf6, 0xf0,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04, 0xb0, 0x00, 0x3b, 0xf6,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0xf6, 0xf0,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04, 0xb0, 0x00, 0x0a, 0xf4,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0xf4, 0x98,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04, 0xb0, 0x00, 0x11, 0xf4,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0xf4, 0xee,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04, 0xb0, 0x00, 0x18, 0xb2,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0xb2, 0x1c,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0xfc, 0x5a, 0x5a,
    0x39, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xb0, 0x00, 0x11, 0xfe,
    0x39, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xfe, 0x00,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0xfc, 0xa5, 0xa5,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04, 0xb0, 0x00, 0x0d, 0xb2,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0xb2, 0x20,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04, 0xb0, 0x00, 0x0c, 0xb2,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0xb2, 0x30,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0xf7, 0x0b,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0xf0, 0xa5, 0xa5,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0xf0, 0x5a, 0x5a,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0xfc, 0x5a, 0x5a,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04, 0xed, 0x01, 0xcd, 0x00,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0xe1, 0x93,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04, 0xb0, 0x00, 0x06, 0xf4,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0xf4, 0x1f,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0xf0, 0xa5, 0xa5,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0xfc, 0xa5, 0xa5,
    0x39, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0x53, 0x20,
    0x39, 0x01, 0x00, 0x00, 0x64, 0x00, 0x03, 0x51, 0x00, 0x00,
    0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0x29, 0x00
};

static int xiaomi_m17_panel_prepare(struct drm_panel *panel)
{
	struct xiaomi_m17_panel *ctx = to_xiaomi_m17_panel(panel);
	int ret;

	ret = regulator_enable(ctx->vddio);
	if (ret)
		return ret;

	ret = regulator_enable(ctx->vdd);
	if (ret)
		goto err_vddio;

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(10000, 11000); // 10ms downstream equivalent
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	msleep(10); // Wait for reset to latch
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	msleep(10); 

	ret = qcom_dts_mipi_dsi_send_seq(ctx->dsi, xiaomi_m17_k6s_on_cmds, 
					 sizeof(xiaomi_m17_k6s_on_cmds));
	if (ret < 0) {
		dev_err(panel->dev, "Failed to send init sequence: %d\n", ret);
		goto err_vdd;
	}

	return 0;

err_vdd:
	regulator_disable(ctx->vdd);
err_vddio:
	regulator_disable(ctx->vddio);
	return ret;
}

static int xiaomi_m17_panel_enable(struct drm_panel *panel)
{
	struct xiaomi_m17_panel *ctx = to_xiaomi_m17_panel(panel);
	
	/* The parsed array already includes DCS sleep out (0x11) and display on (0x29) 
	 * so redundant calls are not strictly necessary, but maintained for DRM standards. */
	msleep(120); // V-sync stabilization
	return mipi_dsi_dcs_set_display_on(ctx->dsi);
}

static int xiaomi_m17_panel_disable(struct drm_panel *panel)
{
	struct xiaomi_m17_panel *ctx = to_xiaomi_m17_panel(panel);

	mipi_dsi_dcs_set_display_off(ctx->dsi);
	msleep(20);
	return mipi_dsi_dcs_enter_sleep_mode(ctx->dsi);
}

static int xiaomi_m17_panel_unprepare(struct drm_panel *panel)
{
	struct xiaomi_m17_panel *ctx = to_xiaomi_m17_panel(panel);

	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	regulator_disable(ctx->vdd);
	regulator_disable(ctx->vddio);

	return 0;
}

static const struct drm_display_mode xiaomi_m17_mode = {
	.clock = 393392, /* (1080+120+120+28)*(2400+10+20+2)*120Hz / 1000 */
	.hdisplay = 1080,
	.hsync_start = 1080 + 120,       /* h-front-porch */
	.hsync_end = 1080 + 120 + 28,    /* + h-pulse-width */
	.htotal = 1080 + 120 + 28 + 120, /* + h-back-porch */
	.vdisplay = 2400,
	.vsync_start = 2400 + 20,        /* v-front-porch */
	.vsync_end = 2400 + 20 + 2,      /* + v-pulse-width */
	.vtotal = 2400 + 20 + 2 + 10,    /* + v-back-porch */
	.width_mm = 70,
	.height_mm = 155,
	.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
};

static int xiaomi_m17_panel_get_modes(struct drm_panel *panel,
				      struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &xiaomi_m17_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);

	connector->display_info.width_mm = 70;
	connector->display_info.height_mm = 155;
	connector->display_info.bpc = 8; // bits-per-component

	return 1;
}

static const struct drm_panel_funcs xiaomi_m17_panel_funcs = {
	.prepare = xiaomi_m17_panel_prepare,
	.enable = xiaomi_m17_panel_enable,
	.disable = xiaomi_m17_panel_disable,
	.unprepare = xiaomi_m17_panel_unprepare,
	.get_modes = xiaomi_m17_panel_get_modes,
};

static int xiaomi_m17_dsc_init(struct xiaomi_m17_panel *ctx)
{
	struct drm_dsc_config *dsc;

	dsc = devm_kzalloc(&ctx->dsi->dev, sizeof(*dsc), GFP_KERNEL);
	if (!dsc)
		return -ENOMEM;

	/* Mapped from downsteam mdss-dsc-* attributes */
	dsc->pic_width = 1080;
	dsc->pic_height = 2400;
	dsc->slice_width = 540;
	dsc->slice_height = 20;
	dsc->slice_count = dsc->pic_width / dsc->slice_width; // 2 slices
	dsc->bits_per_component = 8;
	dsc->bits_per_pixel = 8 << 4; // DSC DRM API represents bpp in U4.4 format

	ctx->dsi->dsc = dsc;
	return 0;
}

static int xiaomi_m17_panel_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct xiaomi_m17_panel *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->vddio = devm_regulator_get(dev, "vddio");
	if (IS_ERR(ctx->vddio))
		return PTR_ERR(ctx->vddio);

	ctx->vdd = devm_regulator_get(dev, "vdd");
	if (IS_ERR(ctx->vdd))
		return PTR_ERR(ctx->vdd);

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "Failed to get reset GPIO\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	drm_panel_init(&ctx->base, dev, &xiaomi_m17_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	/* Downstream: qcom,mdss-dsi-tx-eot-append means we do NOT skip EOT */
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_MODE_LPM;

	ret = xiaomi_m17_dsc_init(ctx);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to initialize DSC\n");

	drm_panel_add(&ctx->base);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		drm_panel_remove(&ctx->base);
		return dev_err_probe(dev, ret, "Failed to attach to DSI host\n");
	}

	return 0;
}

static void xiaomi_m17_panel_remove(struct mipi_dsi_device *dsi)
{
	struct xiaomi_m17_panel *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->base);
}

static const struct of_device_id xiaomi_m17_of_match[] = {
	{ .compatible = "xiaomi,m17-k6s-panel" },
	{ .compatible = "xiaomi,m17-panel" }, // Add variant logic as needed
	{ }
};
MODULE_DEVICE_TABLE(of, xiaomi_m17_of_match);

static struct mipi_dsi_driver xiaomi_m17_panel_driver = {
	.driver = {
		.name = "panel-xiaomi-m17",
		.of_match_table = xiaomi_m17_of_match,
	},
	.probe = xiaomi_m17_panel_probe,
	.remove = xiaomi_m17_panel_remove,
};
module_mipi_dsi_driver(xiaomi_m17_panel_driver);

MODULE_AUTHOR("Poco X5 Upstream");
MODULE_DESCRIPTION("Xiaomi Poco X5 5G (m17) DSI Panel Driver");
MODULE_LICENSE("GPL v2");
