/*
 * panel-xiaomi-m17.c
 * Upstream-conformant DRM panel driver for the Xiaomi M17 (Poco X5) panel.
 */

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>

struct xiaomi_m17_panel {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data supplies[2];
	struct gpio_desc *reset_gpio;
};

static int xiaomi_m17_configure_dsc(struct mipi_dsi_device *dsi)
{
	struct drm_dsc_config *dsc;

	dsc = devm_kzalloc(&dsi->dev, sizeof(*dsc), GFP_KERNEL);
	if (!dsc)
		return -ENOMEM;

	dsc->pic_width = 1080;
	dsc->pic_height = 2400;
	dsc->slice_width = 540;
	dsc->slice_height = 20;
	dsc->slice_count = 2;
	dsc->bits_per_component = 8;
	dsc->bits_per_pixel = 8 << 4; /* U4.4 fixed point */
	dsc->block_pred_enable = true;

	drm_dsc_compute_rc_parameters(dsc);
	dsi->dsc = dsc;

	return 0;
}

static int xiaomi_m17_panel_prepare(struct drm_panel *panel)
{
	struct xiaomi_m17_panel *ctx = container_of(panel, struct xiaomi_m17_panel, panel);
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct drm_dsc_picture_parameter_set pps;
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0)
		return ret;

	msleep(20);

	gpiod_set_value(ctx->reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value(ctx->reset_gpio, 0);
	msleep(20);
	gpiod_set_value(ctx->reset_gpio, 1);
	msleep(10);

	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xf1, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xd0, 0x08);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x25, 0xf2);
	mipi_dsi_dcs_write_seq(dsi, 0xf2, 0x50);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x2f, 0xf2);
	mipi_dsi_dcs_write_seq(dsi, 0xf2, 0x27);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0xf1, 0xa5, 0xa5);

	mipi_dsi_dcs_exit_sleep_mode(dsi);
	msleep(20);

	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xb2, 0x01, 0x31);
	mipi_dsi_dcs_write_seq(dsi, 0xdf, 0x09, 0x30, 0x95, 0x43, 0xa9, 0x43, 0xa9);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0x9d, 0x01);

	/* Dynamically pack and transmit DSC PPS */
	drm_dsc_pps_payload_pack(&pps, dsi->dsc);
	mipi_dsi_picture_parameter_set(dsi, &pps);

	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0x60, 0x01);
	mipi_dsi_dcs_write_seq(dsi, 0xf7, 0x0b);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x40, 0xf2);
	mipi_dsi_dcs_write_seq(dsi, 0xf2, 0x03);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x15, 0xf6);
	mipi_dsi_dcs_write_seq(dsi, 0xf6, 0xf0);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x28, 0xf6);
	mipi_dsi_dcs_write_seq(dsi, 0xf6, 0xf0);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x3b, 0xf6);
	mipi_dsi_dcs_write_seq(dsi, 0xf6, 0xf0);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x0a, 0xf4);
	mipi_dsi_dcs_write_seq(dsi, 0xf4, 0x98);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x11, 0xf4);
	mipi_dsi_dcs_write_seq(dsi, 0xf4, 0xee);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x18, 0xb2);
	mipi_dsi_dcs_write_seq(dsi, 0xb2, 0x1c);
	mipi_dsi_dcs_write_seq(dsi, 0xfc, 0x5a, 0x5a);
	
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x11, 0xfe);
	mipi_dsi_dcs_write_seq(dsi, 0xfe, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xfc, 0xa5, 0xa5);
	
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x0d, 0xb2);
	mipi_dsi_dcs_write_seq(dsi, 0xb2, 0x05);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x0c, 0xb2);
	mipi_dsi_dcs_write_seq(dsi, 0xb2, 0x30);
	mipi_dsi_dcs_write_seq(dsi, 0xf7, 0x0b);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xfc, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq(dsi, 0xed, 0x01, 0xcd, 0x00);
	mipi_dsi_dcs_write_seq(dsi, 0xe1, 0x93);
	mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x06, 0xf4);
	mipi_dsi_dcs_write_seq(dsi, 0xf4, 0x1f);
	mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_write_seq(dsi, 0xfc, 0xa5, 0xa5);

	mipi_dsi_dcs_write_seq(dsi, 0x53, 0x28); 
	msleep(100);
	
	mipi_dsi_dcs_write_seq(dsi, 0x51, 0x00, 0x00);

	mipi_dsi_dcs_set_display_on(dsi);
	msleep(20);

	return 0;
}

static int xiaomi_m17_panel_unprepare(struct drm_panel *panel)
{
	struct xiaomi_m17_panel *ctx = container_of(panel, struct xiaomi_m17_panel, panel);

	mipi_dsi_dcs_set_display_off(ctx->dsi);
	usleep_range(10000, 11000);
	mipi_dsi_dcs_enter_sleep_mode(ctx->dsi);
	msleep(120);

	gpiod_set_value(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);

	return 0;
}

static const struct drm_display_mode xiaomi_m17_mode = {
	.clock = 414050, 
	.hdisplay = 1080,
	.hsync_start = 1080 + 120,
	.hsync_end = 1080 + 120 + 28,
	.htotal = 1080 + 120 + 120 + 28,
	.vdisplay = 2400,
	.vsync_start = 2400 + 20,
	.vsync_end = 2400 + 20 + 2,
	.vtotal = 2400 + 20 + 10 + 2,
	.width_mm = 67,
	.height_mm = 149,
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

static int xiaomi_m17_panel_probe(struct mipi_dsi_device *dsi)
{
	struct xiaomi_m17_panel *ctx;
	int ret;

	ctx = devm_kzalloc(&dsi->dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->supplies[0].supply = "vddio";
	ctx->supplies[1].supply = "vdd";
	ret = devm_regulator_bulk_get(&dsi->dev, ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0)
		return ret;

	ctx->reset_gpio = devm_gpiod_get(&dsi->dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset_gpio))
		return PTR_ERR(ctx->reset_gpio);

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_MODE_LPM | MIPI_DSI_MODE_NO_EOT_PACKET;

	ret = xiaomi_m17_configure_dsc(dsi);
	if (ret)
		return ret;

	drm_panel_init(&ctx->panel, &dsi->dev, &xiaomi_m17_panel_funcs, DRM_MODE_CONNECTOR_DSI);
	
	/* Required for QCOM hosts to ensure link is powered before transmitting commands */
	ctx->panel.prepare_prev_first = true;

	drm_panel_add(&ctx->panel);
	
	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		drm_panel_remove(&ctx->panel);
		return ret;
	}

	return 0;
}

static void xiaomi_m17_panel_remove(struct mipi_dsi_device *dsi)
{
	struct xiaomi_m17_panel *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id xiaomi_m17_of_match[] = {
	{ .compatible = "xiaomi,moonstone-m17" },
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
