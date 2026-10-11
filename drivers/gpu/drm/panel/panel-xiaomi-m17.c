// SPDX-License-Identifier: GPL-2.0-only
/* Xiaomi M17 panels. Initialization is from the Moonstone downstream DTS. */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

struct xiaomi_m17_panel {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data supplies[2];
	struct gpio_desc *reset_gpio;
	struct gpio_desc *enable_gpio;
	struct drm_dsc_config dsc;
	bool k6s;
};

static void xiaomi_m17_write_pps(struct mipi_dsi_multi_context *dsi_ctx)
{
	struct drm_dsc_picture_parameter_set pps;
	u8 payload[90] = { 0x9e };

	/*
	 * Both vendor sequences send PPS bytes 0..88 through DCS 0x9e.
	 * Generate them from the same config the host uses for its encoder.
	 * The remaining PPS bytes are reserved for this DSC 1.1 RGB mode.
	 */
	drm_dsc_pps_payload_pack(&pps, dsi_ctx->dsi->dsc);
	memcpy(payload + 1, &pps, sizeof(payload) - 1);
	mipi_dsi_dcs_write_buffer_multi(dsi_ctx, payload, sizeof(payload));
}

/* dsi-panel-m17-38-0c-0a-fhdp-dsc-video.dts */
static void xiaomi_m17_on(struct mipi_dsi_multi_context *dsi_ctx)
{
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf1, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xd0, 0x08);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x25, 0xf2);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf2, 0x50);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x2f, 0xf2);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf2, 0x27);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf1, 0xa5, 0xa5);
	mipi_dsi_dcs_exit_sleep_mode_multi(dsi_ctx);
	mipi_dsi_msleep(dsi_ctx, 20);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb2, 0x01, 0x31);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xdf, 0x09, 0x30, 0x95, 0x43, 0xa9, 0x43, 0xa9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x9d, 0x01);
	xiaomi_m17_write_pps(dsi_ctx);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x60, 0x01);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0b);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x40, 0xf2);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf2, 0x03);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x15, 0xf6);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf6, 0xf0);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x28, 0xf6);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf6, 0xf0);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x3b, 0xf6);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf6, 0xf0);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x0a, 0xf4);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf4, 0x98);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x11, 0xf4);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf4, 0xee);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x18, 0xb2);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb2, 0x1c);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfc, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x11, 0xfe);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfe, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfc, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x0d, 0xb2);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb2, 0x05);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x0c, 0xb2);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb2, 0x30);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0b);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfc, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xed, 0x01, 0xcd, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xe1, 0x93);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x06, 0xf4);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf4, 0x1f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfc, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x53, 0x28);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x51, 0x00, 0x00);
	mipi_dsi_msleep(dsi_ctx, 100);
	mipi_dsi_dcs_set_display_on_multi(dsi_ctx);
}

/* dsi-panel-m17-k6s-38-0c-0a-fhdp-dsc-video.dts */
static void xiaomi_k6s_on(struct mipi_dsi_multi_context *dsi_ctx)
{
	mipi_dsi_dcs_exit_sleep_mode_multi(dsi_ctx);
	mipi_dsi_msleep(dsi_ctx, 20);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb2, 0x01, 0x31);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xdf, 0x09, 0x30, 0x95, 0x46, 0xe9);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x9d, 0x01);
	xiaomi_m17_write_pps(dsi_ctx);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x60, 0x21);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0b);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x15, 0xf6);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf6, 0xf0);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x28, 0xf6);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf6, 0xf0);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x3b, 0xf6);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf6, 0xf0);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x0a, 0xf4);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf4, 0x98);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x11, 0xf4);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf4, 0xee);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x18, 0xb2);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb2, 0x1c);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfc, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x11, 0xfe);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfe, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfc, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x0d, 0xb2);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb2, 0x20);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x0c, 0xb2);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb2, 0x30);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf7, 0x0b);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfc, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xed, 0x01, 0xcd, 0x00);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xe1, 0x93);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xb0, 0x00, 0x06, 0xf4);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf4, 0x1f);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0xfc, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x53, 0x20);
	mipi_dsi_dcs_write_seq_multi(dsi_ctx, 0x51, 0x00, 0x00);
	mipi_dsi_msleep(dsi_ctx, 100);
	mipi_dsi_dcs_set_display_on_multi(dsi_ctx);
}

static int xiaomi_m17_panel_prepare(struct drm_panel *panel)
{
	struct xiaomi_m17_panel *ctx = container_of(panel, struct xiaomi_m17_panel, panel);
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret)
		return ret;

	msleep(20);
	gpiod_set_value_cansleep(ctx->enable_gpio, 1);
	/* GPIO_ACTIVE_LOW: 1 asserts reset, 0 releases it. */
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	msleep(10);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	msleep(10);

	ctx->dsi->mode_flags |= MIPI_DSI_MODE_LPM;
	if (ctx->k6s)
		xiaomi_k6s_on(&dsi_ctx);
	else
		xiaomi_m17_on(&dsi_ctx);

	if (dsi_ctx.accum_err) {
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		gpiod_set_value_cansleep(ctx->enable_gpio, 0);
		regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	}

	return dsi_ctx.accum_err;
}

static int xiaomi_m17_panel_unprepare(struct drm_panel *panel)
{
	struct xiaomi_m17_panel *ctx = container_of(panel, struct xiaomi_m17_panel, panel);
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };
	int ret;

	/* The downstream pre-off sequence uses high-speed commands. */
	ctx->dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;
	mipi_dsi_dcs_set_display_off_multi(&dsi_ctx);
	/* Allow power-down to finish even if the DSI bus has failed. */
	msleep(20);
	mipi_dsi_dcs_enter_sleep_mode_multi(&dsi_ctx);
	msleep(150);
	ctx->dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	gpiod_set_value_cansleep(ctx->enable_gpio, 0);
	ret = regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);

	return dsi_ctx.accum_err ?: ret;
}

/* Both downstream timing nodes specify 120 Hz with these porches. */
static const struct drm_display_mode xiaomi_m17_mode = {
	.clock = 393400,
	.hdisplay = 1080,
	.hsync_start = 1080 + 120,
	.hsync_end = 1080 + 120 + 28,
	.htotal = 1080 + 120 + 28 + 120,
	.vdisplay = 2400,
	.vsync_start = 2400 + 20,
	.vsync_end = 2400 + 20 + 2,
	.vtotal = 2400 + 20 + 2 + 10,
	.width_mm = 70,
	.height_mm = 155,
};

static int xiaomi_m17_panel_get_modes(struct drm_panel *panel,
				    struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &xiaomi_m17_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;

	return 1;
}

static const struct drm_panel_funcs xiaomi_m17_panel_funcs = {
	.prepare = xiaomi_m17_panel_prepare,
	.unprepare = xiaomi_m17_panel_unprepare,
	.get_modes = xiaomi_m17_panel_get_modes,
};

static int xiaomi_m17_bl_update_status(struct backlight_device *bl)
{
	struct xiaomi_m17_panel *ctx = bl_get_data(bl);

	/* The vendor 0x51 commands put the most significant byte first. */
	return mipi_dsi_dcs_set_display_brightness_large(ctx->dsi,
						       backlight_get_brightness(bl));
}

static const struct backlight_ops xiaomi_m17_bl_ops = {
	.update_status = xiaomi_m17_bl_update_status,
};

static int xiaomi_m17_panel_probe(struct mipi_dsi_device *dsi)
{
	struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.max_brightness = 2047,
		.brightness = 512,
	};
	struct xiaomi_m17_panel *ctx;
	int ret;

	ctx = devm_kzalloc(&dsi->dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->dsi = dsi;
	ctx->k6s = of_device_is_compatible(dsi->dev.of_node, "xiaomi,moonstone-m17-k6s");
	mipi_dsi_set_drvdata(dsi, ctx);
	ctx->supplies[0].supply = "vddio";
	ctx->supplies[1].supply = "avdd";
	ret = devm_regulator_bulk_get(&dsi->dev, ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret)
		return dev_err_probe(&dsi->dev, ret, "Failed to get panel supplies\n");

	ctx->reset_gpio = devm_gpiod_get(&dsi->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(&dsi->dev, PTR_ERR(ctx->reset_gpio), "Failed to get reset\n");
	ctx->enable_gpio = devm_gpiod_get(&dsi->dev, "enable", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->enable_gpio))
		return dev_err_probe(&dsi->dev, PTR_ERR(ctx->enable_gpio), "Failed to get enable\n");

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	/* Android requests TX EOT append, so do not set NO_EOT_PACKET. */
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST | MIPI_DSI_MODE_LPM;

	ctx->dsc.dsc_version_major = 1;
	ctx->dsc.dsc_version_minor = 1;
	ctx->dsc.pic_width = 1080;
	ctx->dsc.pic_height = 2400;
	ctx->dsc.slice_width = 540;
	/* Both vendor 0x9e payloads encode 40, despite the DTS scalar saying 20. */
	ctx->dsc.slice_height = 40;
	ctx->dsc.slice_count = 2;
	ctx->dsc.bits_per_component = 8;
	ctx->dsc.bits_per_pixel = 8 << 4;
	ctx->dsc.block_pred_enable = true;
	/* MSM fills the rate-control parameters before panel prepare. */
	dsi->dsc = &ctx->dsc;

	drm_panel_init(&ctx->panel, &dsi->dev, &xiaomi_m17_panel_funcs, DRM_MODE_CONNECTOR_DSI);
	ctx->panel.prepare_prev_first = true;
	ctx->panel.backlight = devm_backlight_device_register(&dsi->dev, dev_name(&dsi->dev),
					&dsi->dev, ctx, &xiaomi_m17_bl_ops, &props);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(&dsi->dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to register backlight\n");

	drm_panel_add(&ctx->panel);
	ret = mipi_dsi_attach(dsi);
	if (ret)
		drm_panel_remove(&ctx->panel);

	return ret;
}

static void xiaomi_m17_panel_remove(struct mipi_dsi_device *dsi)
{
	struct xiaomi_m17_panel *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id xiaomi_m17_of_match[] = {
	{ .compatible = "xiaomi,moonstone-m17" },
	{ .compatible = "xiaomi,moonstone-m17-k6s" },
	{ }
};
MODULE_DEVICE_TABLE(of, xiaomi_m17_of_match);

static struct mipi_dsi_driver xiaomi_m17_panel_driver = {
	.probe = xiaomi_m17_panel_probe,
	.remove = xiaomi_m17_panel_remove,
	.driver = {
		.name = "panel-xiaomi-m17",
		.of_match_table = xiaomi_m17_of_match,
	},
};
module_mipi_dsi_driver(xiaomi_m17_panel_driver);

MODULE_DESCRIPTION("Xiaomi M17 DRM Panel Driver");
MODULE_LICENSE("GPL");
